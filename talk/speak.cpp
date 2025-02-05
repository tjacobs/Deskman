// Deskman robot.
// Thomas Jacobs

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
using namespace std;
using namespace nlohmann;

// Configure
static const string MODEL            = "gpt-4o-realtime-preview-2024-10-01";
static const string VOICE            = "ash";
static const string INSTRUCTIONS     = 
"""You are Deskman, a friendly home assistance robot, with a physical appearance of a robot head and shoulders on a desk. \
Output <UP>, <DOWN>, <LEFT>, or <RIGHT> if asked to move your head in any direction.""";

// Wake words
static const vector<string> KEYWORDS = {"computer"};

// Audio parameters
static const int FRAMES_PER_BUFFER        = 512 * 10;
static const int SAMPLE_RATE              = 24000;
static const int CHANNELS                 = 1;
static const PaSampleFormat AUDIO_FORMAT  = paInt16;

// Functions
void move_head(int x, int y)        { cout << endl << "Move head: " << x << ", " << y << endl; }
void move_face(int eyes, int smile) { cout << "Move face: " << eyes << ", " << smile << endl; }

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
            nullptr,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,
            nullptr,
            nullptr
        );
        if (err != paNoError) {
            cerr << "Pa_OpenStream input failed: " << Pa_GetErrorText(err) << endl;
            return false;
        }

        // Start stream
        err = Pa_StartStream(streamIn);
        if (err != paNoError) {
            cerr << "Pa_StartStream input failed: " << Pa_GetErrorText(err) << endl;
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
            nullptr,
            &outputParameters,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,
            nullptr,
            nullptr
        );
        if (err != paNoError) {
            cerr << "Pa_OpenStream output failed: " << Pa_GetErrorText(err) << endl;
            return false;
        }

        // Start stream
        err = Pa_StartStream(streamOut);
        if (err != paNoError) {
            cerr << "Pa_StartStream output failed: " << Pa_GetErrorText(err) << endl;
            return false;
        }

        return true;
    }

    void startRecording() {
        isRecording = true;
        audioBuffer.clear();
        if (!startAudioStreamIn()) {
            cerr << "Cannot open input stream for recording.\n";
        }
    }

    void stopRecording() {
        isRecording = false;
        stopAudioStreamIn();
    }

    vector<int16_t> recordChunk() {
        vector<int16_t> chunk(FRAMES_PER_BUFFER, 0);
        if (streamIn && isRecording) {
            PaError err = Pa_ReadStream(streamIn, chunk.data(), FRAMES_PER_BUFFER);
            if (err != paNoError) {
                // Handle error
            }
        }
        return chunk;
    }

    // For output
    void enqueuePlayback(const vector<int16_t>& audioData) {
        //cout << "Queuing " << audioData.size() << " samples." << endl;
        lock_guard<mutex> lock(playMutex);
        playQueue.push(audioData);
        playCond.notify_one();
    }

    void startPlaybackThread() {
        if (!playThread.joinable()) {
            playThread = thread(&AudioHandler::playLoop, this);
        }
    }

    void stopPlaybackThread() {
        lock_guard<mutex> lock(playMutex);
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
    vector<int16_t> audioBuffer;

    // Playback
    thread playThread;
    queue<vector<int16_t>> playQueue;
    mutex playMutex;
    condition_variable playCond;
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
            cerr << "Failed to open output audio stream.\n";
            return;
        }
        while (true) {
            vector<int16_t> front;
            unique_lock<mutex> lock(playMutex);
            playCond.wait(lock, [this]{ return !playQueue.empty() || !playbackRunning; });
            if (!playbackRunning && playQueue.empty()) { break; }
            front = playQueue.front();
            playQueue.pop();

            // Write to PortAudio output
            Pa_WriteStream(streamOut, front.data(), front.size());
        }
        stopAudioStreamOut();
    }
};

// -----------------------------------------------------------
// OpenAIClient (WebSocket connection to OpenAI Realtime API)
// -----------------------------------------------------------
class OpenAIClient {
public:
    OpenAIClient(const string& instructions_, const string& voice_): instructions(instructions_), voice(voice_), context(nullptr), wsi(nullptr) {
        // Build session config JSON
        json sessionConfig = {
            {"modalities", {"audio", "text"}},
            {"instructions", instructions},
            {"voice", voice},
            {"input_audio_format", "pcm16"},
            {"output_audio_format", "pcm16"},
            {"turn_detection", {}},
            {"tools", {
                {
                    {"type", "function"},
                    {"name", "generate_horoscope"},
                    {"description", "Give today's horoscope for an astrological sign."},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"sign", {
                                {"type", "string"},
                                {"description", "The sign for the horoscope."},
                                {"enum", {
                                    "Aries",
                                    "Taurus", 
                                    "Gemini",
                                    "Cancer",
                                    "Leo",
                                    "Virgo",
                                    "Libra", 
                                    "Scorpio",
                                    "Sagittarius",
                                    "Capricorn",
                                    "Aquarius",
                                    "Pisces"
                                }}
                            }}
                        }},
                        {"required", {"sign"}}
                    }}
                }
            }},
            {"tool_choice", "auto"},
            {"input_audio_transcription", {{"model", "whisper-1"}}},
            {"temperature", 0.6}
        };
        sessionConfigStr = sessionConfig.dump();
    }

    ~OpenAIClient() {
        // Clean up websockets
        if (context) {
            lws_context_destroy(context);
            context = nullptr;
        }
    }

    // Initialize the WebSocket client
    bool initWebSocket() {
        // Context
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
            cerr << "Failed to create lws context.\n";
            return false;
        }

        // Connect to wss://api.openai.com/v1/realtime?model=MODEL
        struct lws_client_connect_info ccinfo = {0};
        ccinfo.context = context;
        ccinfo.address = "api.openai.com";
        ccinfo.host = ccinfo.address;
        ccinfo.port = 443;
        string path = "/v1/realtime?model=" + MODEL;
        ccinfo.path = path.c_str();
        ccinfo.origin = "origin";
        ccinfo.ssl_connection = LCCSCF_USE_SSL;
        ccinfo.userdata = this;
        wsi = lws_client_connect_via_info(&ccinfo);
        if (!wsi) {
            cerr << "Failed to connect to server.\n";
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
    void sendEvent(const json& event) {
        // Get mutex and payload
        lock_guard<mutex> lock(writeMutex);
        string payload = event.dump();

        // The libwebsockets library requires us to allocate buffer with extra space for the WS framing
        size_t len = LWS_PRE + payload.size();
        vector<unsigned char> buf(len);
        memcpy(buf.data() + LWS_PRE, payload.data(), payload.size());

        // Send
        lws_write(wsi, buf.data() + LWS_PRE, payload.size(), LWS_WRITE_TEXT);
    }

    // Called once the connection is established, we send "session.update"
    void onConnected() {
        json evt{
            {"type", "session.update"},
            {"session", json::parse(sessionConfigStr)}
        };
        sendEvent(evt);
    }

    // Handler for incoming messages
    void onMessage(const string& msg) {
        // Parse JSON
        auto j = json::parse(msg, nullptr, false);
        if (j.is_discarded()) {
            // Invalid JSON
            cout << "Bad JSON: " << msg << endl;
            return;
        }
        if (j.contains("type")) {
            string type = j["type"].get<string>();
            if (type == "response.audio_transcript.delta") {
                // Get this chunk of response
                string part = j["delta"].get<string>();
                cout << "[" << part << "]";
                response += part;
                flush(cout);

                // Commands
                if (response.find("<UP>")      != string::npos) { move_head(   0,   40); move_face( 0,  1); response.clear(); }
                if (response.find("<DOWN>")    != string::npos) { move_head(   0,  -40); move_face( 0, -1); response.clear(); }
                if (response.find("<LEFT>")    != string::npos) { move_head(  40,    0); move_face( 0,  0); response.clear(); }
                if (response.find("<RIGHT>")   != string::npos) { move_head( -40,    0); move_face( 0,  0); response.clear(); }
                if (response.find("<UP 2>")    != string::npos) { move_head( 900,    0); move_face( 5,  0); response.clear(); }
                if (response.find("<DOWN 2>")  != string::npos) { move_head(-900,    0); move_face(-5,  0); response.clear(); }
                if (response.find("<LEFT 2>")  != string::npos) { move_head(   0,  200); move_face( 5,  0); response.clear(); }
                if (response.find("<RIGHT 2>") != string::npos) { move_head(   0, -200); move_face(-5,  0); response.clear(); }
            }
            else if (type == "response.audio.done") {
                cout << endl;
                response.clear();
            }
            else if (type == "response.function_call_arguments.done") {
                // Parse the complete function arguments
                auto args = json::parse(j["arguments"].get<string>());
                string funcName = j["name"].get<string>();
                
                // Call the corresponding C function
                if (funcName == "generate_horoscope") {
                    string sign = args["sign"].get<string>();
                    generate_horoscope(sign);
                }
            }
            else if (type == "response.audio.delta") {
                // Base64 decode
                string b64data = j["delta"].get<string>();
                vector<uint8_t> audioBytes = base64Decode(b64data);

                // Convert to int16_t for playback
                vector<int16_t> samples(audioBytes.size()/2);
                memcpy(samples.data(), audioBytes.data(), audioBytes.size());
                audioHandler.enqueuePlayback(samples);
            }
            else if (type == "response.done") {
                cout << "Response generation completed.\n";
                talking = false;
            }
            else if (type == "session.created") {
                audioHandler.startPlaybackThread();
            }
            else if (type == "session.updated") {
                //cerr << "Event: " << j.dump() << endl;
                ready = true;
            }
            else if (type == "error") {
                cerr << "Error event received: " << j.dump() << endl;
            }
            else {
                //cout << "Event: " << j["type"] << /* j.dump() << */ endl;
            }
        }
    }

    // Called on close
    void onClose() {
        cout << "WebSocket closed.\n";
    }

    void close() {
        stopRequested = true;
    }

    // Function
    string generate_horoscope(string sign) {
        string horoscope = "Today's horoscope for " + sign + ": ";
        horoscope += "The stars align in your favor. Take bold steps forward with confidence.";
        cout << horoscope << endl;
        return horoscope;
    }

    // For external usage
    AudioHandler audioHandler;
    bool ready = false;
    bool talking = false;
    string response;

private:
    // The libwebsockets callbacks
    static int callback_openai(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
        auto* client = reinterpret_cast<OpenAIClient*>(lws_wsi_user(wsi));
        //printf("Callback reason: %d\n", reason);
        switch (reason) {
            case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
                {
                    // The 'in' is a pointer to pointer to the free space in the buffer, 'len' is how much space we have
                    unsigned char** p = (unsigned char**) in;
                    unsigned char* end = (*p) + len;

                    // Add "Authorization: Bearer <OPENAI_API_KEY>"
                    string authValue = "Bearer " + OPENAI_API_KEY;
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
                printf("Connected to OpenAI.\n");
                client->onConnected();
                break;
            case LWS_CALLBACK_CLIENT_RECEIVE:
                // Received a message
                if (in && len > 0) {
                    string msg((char*)in, len);
                    client->onMessage(msg);
                }
                break;
            case LWS_CALLBACK_CLIENT_CLOSED:
                client->onClose();
                break;
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                cout << "Connection error:" << endl;
                if (in && len > 0) {
                    string msg((char*)in, len);
                    cout << msg << endl;
                }
            default:
                if (false) {
                    cout << "Other:" << endl;
                    if (in && len > 0) {
                        string msg((char*)in, len);
                        cout << msg << endl;
                    }
                }
                break;
        }
        return 0;
    }

    // Data
    static struct lws_protocols protocols[];
    struct lws_context* context;
    struct lws* wsi;
    mutex writeMutex;
    bool stopRequested = false;

    // Params
    string instructions;
    string voice;
    string sessionConfigStr;
};

// Definition of the websocket protocols
struct lws_protocols OpenAIClient::protocols[] = {
    {
        "realtime-protocol",
        callback_openai,
        100*1024, // Per-session data size
        100*1024, // Receive buffer size
    },
    { nullptr, nullptr, 0, 0 } // Terminator
};

// -----------------------------------------------------------
// Wakeword
// -----------------------------------------------------------
class Wakeword {
public:
    Wakeword(const vector<string>& keywords): handle(nullptr), listening(false) {
        // Init
        const char* keyword_paths[] = { "computer_mac.ppn" };
        float sensitivities[] = {0.5f};
        pv_status_t status = pv_porcupine_init(
            PICOVOICE_KEY.c_str(),
            "porcupine_params.pv",
            1,
            keyword_paths,
            sensitivities,
            &handle
        );
        if (status != PV_STATUS_SUCCESS) {
            cerr << "Failed to init Porcupine wake word.\n";
            handle = nullptr;
        }
    }

    ~Wakeword() {
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
            cerr << "Pa_OpenStream failed: " << Pa_GetErrorText(err) << endl;
            return false;
        }
        Pa_StartStream(stream);
        cout << "Listening for wake word...\n";

        // Listen
        while (listening) {
            // Read data
            vector<int16_t> buffer(pv_porcupine_frame_length());
            err = Pa_ReadStream(stream, buffer.data(), pv_porcupine_frame_length());
            if (err != paNoError) { continue; }
            int32_t keyword_index = -1;
            pv_status_t status = pv_porcupine_process(handle, buffer.data(), &keyword_index);
            if (status != PV_STATUS_SUCCESS) { continue; }

            // Detected?
            if (keyword_index >= 0) {
                cout << "Wake word detected!\n";
                stop();
            }
        }

        // Got it
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
    atomic<bool> listening;
};

// -----------------------------------------------------------
// VoiceAssistant
// -----------------------------------------------------------
class VoiceAssistant {
public:
    VoiceAssistant(): openAIClient(INSTRUCTIONS, VOICE), wakeword(KEYWORDS) { }

    void run() {
        // Init websockets and start a thread that runs the websocket’s service loop
        if (!openAIClient.initWebSocket()) {
            cerr << "WebSocket init failed.\n";
            return;
        }
        thread wsThread([this] { openAIClient.serviceLoop(); });

        // Sleep
        this_thread::sleep_for(chrono::seconds(2));

        // The main loop: wait for wake word, then capture audio, send to OpenAI, etc.
        while (true) {
            // 1. Block waiting for wake word
            wakeword.listen();

            // 2. Start conversation
            startConversation();

            // 3. Sleep
            this_thread::sleep_for(chrono::seconds(1));
        }

        // Clean up
        openAIClient.close();
        if (wsThread.joinable()) { wsThread.join(); }
        openAIClient.audioHandler.cleanup();
    }

private:
    void startConversation() {
        cout << "Starting conversation...\n";

        // Start recording from the user
        openAIClient.audioHandler.startRecording();

        // Sleep until OpenAI is session.updated ready
        while (!openAIClient.ready) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        // Send audio chunks to the OpenAI realtime API
        for (int i = 0; i < 20; i++) {
            // Get audio chunk
            auto chunk = openAIClient.audioHandler.recordChunk();
            if (!chunk.empty()) {
                cout << "Listening... " << i << endl; //<< " frames read: " << chunk.size() << endl;

                // Base64 encode
                string b64chunk = base64Encode(reinterpret_cast<const uint8_t*>(chunk.data()), chunk.size()*sizeof(int16_t));

                // Send to OpenAI
                json evt{ {"type",  "input_audio_buffer.append"}, {"audio", b64chunk} };
                openAIClient.sendEvent(evt);
            }
        }

        // Stop recording
        cout << "Done listening." << endl;
        openAIClient.audioHandler.stopRecording();

        // Commit the audio buffer
        json evt{ {"type", "input_audio_buffer.commit"}};
        openAIClient.sendEvent(evt);
        cout << "Sent input_audio_buffer.commit\n";

        // Ask for a response
        json evtResponse{ {"type", "response.create"} };
        openAIClient.sendEvent(evtResponse);
        cout << "Sent response.create\n";

        // Sleep until OpenAI is done
        openAIClient.talking = true;
        while (openAIClient.talking) { this_thread::sleep_for(chrono::milliseconds(100)); }
    }

private:
    OpenAIClient openAIClient;
    Wakeword wakeword;
};

// -----------------------------------------------------------
// Main
// -----------------------------------------------------------
int main() {
    VoiceAssistant assistant;
    assistant.run();
    return 0;
}
