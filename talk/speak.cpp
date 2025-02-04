#include <queue>
#include <string>
#include <vector>
#include <thread>
#include <iostream>

// JSON
#include <nlohmann/json.hpp>

// PortAudio
#include "portaudio.h"

// Picovoice Porcupine
#include "pv_porcupine.h"

// WebSockets
#include <libwebsockets.h>

// Base64 encoding
#include "base64.hpp"

// Keys
#include "keys.h"

// -----------------------------------------------------------
// Configuration
// -----------------------------------------------------------

// Configure
static const std::string MODEL            = "gpt-4o-realtime-preview-2024-10-01";
static const std::string VOICE            = "ash";
static const std::string INSTRUCTIONS     = "You are Deskman, a friendly home assistance robot, with a physical appearance of a robot head and shoulders on a desk.";

// Wake words
static const std::vector<std::string> KEYWORDS = {"computer"};

// Audio parameters
static const PaSampleFormat AUDIO_FORMAT  = paInt16;
static const int CHANNELS                 = 1;
static const int SAMPLE_RATE              = 24000;
static const int FRAMES_PER_BUFFER        = 512 * 2;

// -----------------------------------------------------------
// AudioHandler
// -----------------------------------------------------------
class AudioHandler {
public:
    AudioHandler(): streamIn(nullptr), streamOut(nullptr), isRecording(false) {
        Pa_Initialize();
    }

    ~AudioHandler() {
        cleanup();
    }

    bool startAudioStreamIn() {
        // Check already open
        if (streamIn) return true;

        // Params
        PaStreamParameters inputParameters;
        memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.device = Pa_GetDefaultInputDevice();
        inputParameters.channelCount = CHANNELS;
        inputParameters.sampleFormat = AUDIO_FORMAT;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        // Open stream
        PaError err = Pa_OpenStream(
            &streamIn,
            &inputParameters,
            nullptr, // no output
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,
            nullptr, // no callback, we use blocking read
            nullptr
        );
        if(err != paNoError) {
            std::cerr << "Pa_OpenStream input failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        // Start stream
        err = Pa_StartStream(streamIn);
        if(err != paNoError) {
            std::cerr << "Pa_StartStream input failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        return true;
    }

    bool startAudioStreamOut() {
        // Check already open
        if (streamOut) return true;

        // Params
        PaStreamParameters outputParameters;
        memset(&outputParameters, 0, sizeof(outputParameters));
        outputParameters.device = Pa_GetDefaultOutputDevice();
        outputParameters.channelCount = CHANNELS;
        outputParameters.sampleFormat = AUDIO_FORMAT;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        // Open stream
        PaError err = Pa_OpenStream(
            &streamOut,
            nullptr, // no input
            &outputParameters,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,
            nullptr, // no callback, we use blocking write
            nullptr
        );
        if (err != paNoError) {
            std::cerr << "Pa_OpenStream output failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        // Start stream
        err = Pa_StartStream(streamOut);
        if(err != paNoError) {
            std::cerr << "Pa_StartStream output failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        return true;
    }

    void startRecording() {
        isRecording = true;
        audioBuffer.clear();
        if (!startAudioStreamIn()) {
            std::cerr << "Cannot open input stream for recording.\n";
        }
    }

    void stopRecording() {
        isRecording = false;
        stopAudioStreamIn();
    }

    std::vector<int16_t> recordChunk() {
        std::vector<int16_t> chunk(FRAMES_PER_BUFFER, 0);
        if (streamIn && isRecording)
        {
            PaError err = Pa_ReadStream(streamIn, chunk.data(), FRAMES_PER_BUFFER);
            if (err != paNoError) {
                // Handle error
            }
        }
        return chunk;
    }

    // For output
    void enqueuePlayback(const std::vector<int16_t>& audioData) {
        std::cout << "Playing " << audioData.size() << " size." << std::endl;
        std::lock_guard<std::mutex> lock(playMutex);
        playQueue.push(audioData);
        playCond.notify_one();
    }

    void startPlaybackThread() {
        if (!playThread.joinable()) {
            playThread = std::thread(&AudioHandler::playLoop, this);
        }
    }

    void stopPlaybackThread() {
        std::lock_guard<std::mutex> lock(playMutex);
        playbackRunning = false;
        playCond.notify_one();
        if (playThread.joinable()) {
            playThread.join();
        }
    }

    void cleanup() {
        stopAudioStreamIn();
        stopAudioStreamOut();
        stopPlaybackThread();
        Pa_Terminate();
    }

private:
    PaStream* streamIn;
    PaStream* streamOut;
    bool isRecording;
    std::vector<int16_t> audioBuffer;

    // Playback
    std::thread playThread;
    std::queue<std::vector<int16_t>> playQueue;
    std::mutex playMutex;
    std::condition_variable playCond;
    bool playbackRunning = true;

    void stopAudioStreamIn() {
        if (streamIn) {
            Pa_StopStream(streamIn);
            Pa_CloseStream(streamIn);
            streamIn = nullptr;
        }
    }

    void stopAudioStreamOut() {
        if (streamOut) {
            Pa_StopStream(streamOut);
            Pa_CloseStream(streamOut);
            streamOut = nullptr;
        }
    }

    void playLoop() {
        if (!startAudioStreamOut()) {
            std::cerr << "Failed to open output audio stream.\n";
            return;
        }
        while (true) {
            std::vector<int16_t> front;
            std::unique_lock<std::mutex> lock(playMutex);
            playCond.wait(lock, [this]{ return !playQueue.empty() || !playbackRunning; });
            if (!playbackRunning && playQueue.empty()) {
                break;
            }
            front = playQueue.front();
            playQueue.pop();

            // Write to PortAudio output
            Pa_WriteStream(streamOut, front.data(), FRAMES_PER_BUFFER);
        }
        stopAudioStreamOut();
    }
};

// -----------------------------------------------------------
// RealtimeClient (WebSocket connection to OpenAI Realtime API)
// -----------------------------------------------------------
class RealtimeClient {
public:
    RealtimeClient(const std::string& instructions_, const std::string& voice_): instructions(instructions_), voice(voice_), context(nullptr), wsi(nullptr) {
        // Build session config JSON
        nlohmann::json sessionConfig = {
            {"modalities", {"audio", "text"}},
            {"instructions", instructions},
            {"voice", voice},
            {"input_audio_format", "pcm16"},
            {"output_audio_format", "pcm16"},
            {"turn_detection", {
                {"type", "server_vad"},
                {"threshold", 0.5},
                {"prefix_padding_ms", 300},
                {"silence_duration_ms", 600}
            }},
            {"input_audio_transcription", {{"model", "whisper-1"}}},
            {"temperature", 0.6}
        };
        sessionConfigStr = sessionConfig.dump();
    }

    ~RealtimeClient() {
        // Clean up websockets
        if (context) {
            lws_context_destroy(context);
            context = nullptr;
        }
    }

    // Initialize the WebSocket client
    bool initWebSocket() {
        struct lws_context_creation_info info;
        memset(&info, 0, sizeof info);

        // Only log errors and warnings
        int logs = LLL_ERR | LLL_WARN; //| LLL_INFO;
        lws_set_log_level(logs, NULL);

        // Use the default event loop (1 thread)
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.gid = -1;
        info.uid = -1;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

        // Create
        context = lws_create_context(&info);
        if (!context) {
            std::cerr << "Failed to create lws context.\n";
            return false;
        }

        // Connect to wss://api.openai.com/v1/realtime?model=MODEL
        struct lws_client_connect_info ccinfo = {0};
        ccinfo.context = context;
        ccinfo.address = "api.openai.com";
        ccinfo.host = ccinfo.address;
        ccinfo.port = 443;
        std::string path = "/v1/realtime?model=" + MODEL;
        ccinfo.path = path.c_str();
        ccinfo.origin = "origin";
        ccinfo.ssl_connection = LCCSCF_USE_SSL;
        ccinfo.userdata = this;
        wsi = lws_client_connect_via_info(&ccinfo);
        if (!wsi) {
            std::cerr << "Failed to connect to server.\n";
            return false;
        }
        return true;
    }

    // The main service loop for websockets can be run in a dedicated thread
    void serviceLoop() {
        while (true) {
            lws_service(context, 50);
            if (stopRequested) { break; }
        }
    }

    // Send a JSON event
    void sendEvent(const nlohmann::json& event) {
        // Get mutex and payload
        std::lock_guard<std::mutex> lock(writeMutex);
        std::string payload = event.dump();

        // The libwebsockets library requires us to allocate buffer with extra space for the WS framing
        size_t len = LWS_PRE + payload.size();
        std::vector<unsigned char> buf(len);
        memcpy(buf.data() + LWS_PRE, payload.data(), payload.size());

        // Send
        lws_write(wsi, buf.data() + LWS_PRE, payload.size(), LWS_WRITE_TEXT);
    }

    // Called once the connection is established, we send "session.update"
    void onConnected() {
        nlohmann::json evt{
            {"type", "session.update"},
            {"session", nlohmann::json::parse(sessionConfigStr)}
        };
        sendEvent(evt);
        printf("Sent session.update\n");
    }

    // Handler for incoming messages
    void onMessage(const std::string& msg) {
        // Parse JSON
        auto j = nlohmann::json::parse(msg, nullptr, false);
        if (j.is_discarded()) {
            // Invalid JSON
            std::cout << "Bad JSON: " << msg << std::endl;
            return;
        }
        if (j.contains("type")) {
            std::string type = j["type"].get<std::string>();
            if (type == "response.audio_transcript.delta") {
                std::cout << j["delta"].get<std::string>();
                std::flush(std::cout);
            }
            else if (type == "response.audio.done") {
                std::cout << std::endl;
            }
            else if (type == "response.audio.delta") {
                // Base64 decode
                std::cout << "Audio data" << std::endl;
                std::string b64data = j["delta"].get<std::string>();
                std::vector<uint8_t> audioBytes = base64Decode(b64data);

                // Convert to int16_t for playback
                std::vector<int16_t> samples(audioBytes.size()/2);
                memcpy(samples.data(), audioBytes.data(), audioBytes.size());
                audioHandler.enqueuePlayback(samples);
            }
            else if (type == "response.done") {
                std::cout << "Response generation completed.\n";
            }
            else if (type == "session.created") {
                std::cout << "Session created, starting playback.\n";
                audioHandler.startPlaybackThread();
            }
            else if (type == "session.updated") {
                std::cout << "Event: " << j["type"] << j.dump() << std::endl;
            }
            else if (type == "error") {
                std::cerr << "Error event received: " << j.dump() << std::endl;
            }
            else {
                std::cout << "Event: " << j["type"] << /* j.dump() << */ std::endl;
            }
        }
    }

    // Called on close
    void onClose() {
        std::cout << "WebSocket closed.\n";
    }

    void close() {
        stopRequested = true;
    }

    // For external usage
    AudioHandler audioHandler;

private:
    // The libwebsockets callbacks
    static int callback_openai(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
        auto* client = reinterpret_cast<RealtimeClient*>(lws_wsi_user(wsi));
        //printf("CALLBACK %d\n", reason);
        switch (reason) {
            case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
                {
                    // The 'in' is a pointer to pointer to the free space in the buffer, 'len' is how much space we have
                    unsigned char** p = (unsigned char**) in;
                    unsigned char* end = (*p) + len;

                    // Add "Authorization: Bearer <OPENAI_API_KEY>"
                    std::string authValue = "Bearer " + OPENAI_API_KEY;
                    int ret = lws_add_http_header_by_name(wsi,
                        (unsigned char*)"Authorization:",
                        (unsigned char*)authValue.c_str(),
                        authValue.size(),
                        p, end);

                    // Add "OpenAI-Beta: realtime=v1"
                    ret = lws_add_http_header_by_name(wsi,
                        (unsigned char*)"OpenAI-Beta:",
                        (unsigned char*)"realtime=v1",
                        strlen("realtime=v1"),
                        p, end);
                }
                break;

            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                // Connection established
                printf("Connected.\n");
                client->onConnected();
                break;
            case LWS_CALLBACK_CLIENT_RECEIVE:
                // Received a message
                if (in && len > 0) {
                    std::string msg((char*)in, len);
                    client->onMessage(msg);
                }
                break;
            case LWS_CALLBACK_CLIENT_CLOSED:
                client->onClose();
                break;
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                std::cout << "Connection error:" << std::endl;
                if (in && len > 0) {
                    std::string msg((char*)in, len);
                    std::cout << msg << std::endl;
                }
            default:
                std::cout << "Other:" << std::endl;
                if (in && len > 0) {
                    std::string msg((char*)in, len);
                    std::cout << msg << std::endl;
                }
                break;
        }
        return 0;
    }

    // Data
    static struct lws_protocols protocols[];
    struct lws_context* context;
    struct lws* wsi;
    std::mutex writeMutex;
    bool stopRequested = false;

    // Params
    std::string instructions;
    std::string voice;
    std::string sessionConfigStr;
};

// Definition of the protocols
struct lws_protocols RealtimeClient::protocols[] = {
    {
        "realtime-protocol",
        callback_openai,
        100*1024, // per-session data size
        100*1024, // rx buffer size
    },
    { nullptr, nullptr, 0, 0 } // terminator
};

// -----------------------------------------------------------
// PorcupineWakeword
// -----------------------------------------------------------
class PorcupineWakeword {
public:
    PorcupineWakeword(const std::vector<std::string>& keywords, float sensitivity): handle(nullptr), listening(false) {
        // Init
        pv_status_t status; //pv_porcupine_init(
//            PICOVOICE_KEY.c_str(), // Access key
//            pv_porcupine_keywords::PV_KEYWORD_COMPUTER, // Example built-in keyword
//            0,
//            sensitivity,
//            &handle
//        );
        if (status != PV_STATUS_SUCCESS) {
            std::cerr << "Failed to init Porcupine.\n";
            handle = nullptr;
        }
    }

    ~PorcupineWakeword() {
        if (handle) {
            pv_porcupine_delete(handle);
        }
    }

    // Start listening in a blocking fashion, can move to separate thread
    bool listen() {
        // Check init
        if (!handle) { return false; }

        // Open portaudio stream
        listening = true;
        PaStream* stream;
        Pa_Initialize();
        PaStreamParameters inputParameters;
        memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.device = Pa_GetDefaultInputDevice();
        inputParameters.channelCount = 1;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
        PaError err = Pa_OpenStream(&stream,
                                    &inputParameters,
                                    nullptr,
                                    pv_sample_rate(),
                                    pv_porcupine_frame_length(),
                                    paClipOff,
                                    nullptr,
                                    nullptr);
        if (err != paNoError) {
            std::cerr << "Pa_OpenStream failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }
        Pa_StartStream(stream);
        std::cout << "Listening for wake word...\n";

        // Listen
        while (listening) {
            // Read data
            std::vector<int16_t> buffer(pv_porcupine_frame_length());
            err = Pa_ReadStream(stream, buffer.data(), pv_porcupine_frame_length());
            if (err != paNoError) {
                // Handle error
                continue;
            }
            int32_t keyword_index = -1;
            pv_status_t status = pv_porcupine_process(handle, buffer.data(), &keyword_index);
            if (status != PV_STATUS_SUCCESS) {
                // Handle error
                continue;
            }

            // Detected?
            if (keyword_index >= 0) {
                std::cout << "Wake word detected!\n";
                stop();
            }
        }

        // Got it
        std::cout << "Stopped listening for wake word.\n";
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
        return true;
    }

    void stop() {
        listening = false;
    }

private:
    pv_porcupine_t* handle;
    std::atomic<bool> listening;
};

// -----------------------------------------------------------
// VoiceAssistant
// -----------------------------------------------------------
class VoiceAssistant {
public:
    VoiceAssistant(): realtimeClient(INSTRUCTIONS, VOICE), wakeword(KEYWORDS, 0.5f) { }

    void run() {
        // Init websockets and start a thread that runs the websocket’s service loop:
        if (!realtimeClient.initWebSocket()) {
            std::cerr << "WebSocket init failed.\n";
            return;
        }
        std::thread wsThread([this] { realtimeClient.serviceLoop(); });

        // The main loop: wait for wake word, then capture audio, send to OpenAI, etc.
        while (true) {
            // 1. Block waiting for wake word
//            wakeword.listen();

            // 2. Start conversation
            startConversation();

            // 3. Sleep
            std::this_thread::sleep_for(std::chrono::seconds(10));
            break;
        }

        // Clean up
        realtimeClient.close();
        if (wsThread.joinable()) { wsThread.join(); }
        realtimeClient.audioHandler.cleanup();
    }

private:
    void startConversation() {
        std::cout << "Starting conversation...\n";

        // Start recording from the user
//        realtimeClient.audioHandler.startRecording();

        // Send audio chunks to the OpenAI realtime API
        if (false)
        for (int i=0; i<50; i++) {
            // Get audio chunk
            auto chunk = realtimeClient.audioHandler.recordChunk();
            if (!chunk.empty()) {
                std::cout << "Listening... " << i << " frames read: " << chunk.size() << std::endl;

                // Base64 encode
                std::string b64chunk = base64Encode(reinterpret_cast<const uint8_t*>(chunk.data()), chunk.size()*sizeof(int16_t));

                // Send to OpenAI
                nlohmann::json evt{ {"type",  "input_audio_buffer.append"}, {"audio", b64chunk} };
                realtimeClient.sendEvent(evt);
            }
        }

        // Stop recording
//        realtimeClient.audioHandler.stopRecording();

        // Commit the audio buffer
        nlohmann::json evt{ {"type", "input_audio_buffer.commit"}};
//        realtimeClient.sendEvent(evt);
//        std::cout << "Sent input_audio_buffer.commit\n";

        // Sleep
        std::this_thread::sleep_for(std::chrono::seconds(4));

        // Ask for a response
        nlohmann::json evtResponse{ {"type", "response.create"} };
        realtimeClient.sendEvent(evtResponse);
        std::cout << "Sent response.create\n";

        // Sleep
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

private:
    RealtimeClient realtimeClient;
    PorcupineWakeword wakeword;
};

// -----------------------------------------------------------
// Main
// -----------------------------------------------------------
int main() {
    VoiceAssistant assistant;
    assistant.run();
    return 0;
}
