// Deskman robot.
// Thomas Jacobs

#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <condition_variable>

#ifdef ALSA
#include <alsa/asoundlib.h>
#endif

// JSON
#include <nlohmann/json.hpp>

// Picovoice Porcupine
#include "pv_porcupine.h"

// WebSockets
#include <libwebsockets.h>

// Base64 encoding
#include "base64.hpp"

// Keys
#include "keys.h"

using namespace std;
using namespace nlohmann;

// -----------------------------------------------------------
// Configuration
// -----------------------------------------------------------

static const string OPENAI_KEY       = OPENAI_KEY_DEFINE;
static const string PICOVOICE_KEY    = PICOVOICE_KEY_DEFINE;
static const string MODEL            = "gpt-4o-realtime-preview-2024-10-01";
static const string VOICE            = "ash";
static const string INSTRUCTIONS     =
    """You are Deskman, a friendly home assistance robot, with a physical appearance of a robot head and shoulders on a desk.\n"\
    "Output <UP>, <DOWN>, <LEFT>, or <RIGHT> if asked to move your head in any direction.""";

// Wake words
static const vector<string> KEYWORDS = {"computer"};

// Audio parameters
static const int SAMPLE_RATE = 24000;
static const int CHANNELS    = 1;
static const int FRAMES_PER_BUFFER = 512 * 10;

// -----------------------------------------------------------
// Utility functions for movement
// -----------------------------------------------------------
void move_head(int x, int y) {
    cout << "\nMove head: " << x << ", " << y << endl;
}

void move_face(int eyes, int smile) {
    cout << "Move face: " << eyes << ", " << smile << endl;
}

void move_head(const string &direction) {
    if      (direction == "Up")    move_head(  0,  40);
    else if (direction == "Down")  move_head(  0, -40);
    else if (direction == "Left")  move_head( 40,   0);
    else if (direction == "Right") move_head(-40,   0);
}

// -----------------------------------------------------------
// AudioHandler using ALSA
// -----------------------------------------------------------
class AudioHandler {
public:
    AudioHandler() : capture_handle(nullptr), playback_handle(nullptr) {
        initAudio();
    }

    ~AudioHandler() {
        cleanup();
    }

    // Initialize both input and output devices
    bool initAudio() {
        return openAudioInput() && openAudioOutput();
    }

    #ifdef ALSA
    static const snd_pcm_format_t AUDIO_FORMAT = SND_PCM_FORMAT_S16_LE;

    // Set up ALSA capture
    bool openAudioInput() {
        if (snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
            cerr << "Failed to open ALSA capture device." << endl;
            return false;
        }
        if (configureDevice(capture_handle) < 0) {
            cerr << "Failed to configure ALSA capture device." << endl;
            return false;
        }
        return true;
    }

    // Set up ALSA playback
    bool openAudioOutput() {
        if (snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            cerr << "Failed to open ALSA playback device." << endl;
            return false;
        }
        if (configureDevice(playback_handle) < 0) {
            cerr << "Failed to configure ALSA playback device." << endl;
            return false;
        }
        return true;
    }

    // Configure common device params
    int configureDevice(snd_pcm_t *handle) {
        snd_pcm_hw_params_t *hw_params;
        snd_pcm_hw_params_alloca(&hw_params);
        if (snd_pcm_hw_params_any(handle, hw_params) < 0) return -1;
        if (snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) return -1;
        if (snd_pcm_hw_params_set_format(handle, hw_params, AUDIO_FORMAT) < 0) return -1;
        unsigned int rate = SAMPLE_RATE;
        if (snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0) < 0) return -1;
        if (snd_pcm_hw_params_set_channels(handle, hw_params, CHANNELS) < 0) return -1;
        if (snd_pcm_hw_params(handle, hw_params) < 0) return -1;
        return 0;
    }

    // Record a chunk of audio data, returns a vector of int16_t samples.
    vector<int16_t> recordChunk() {
        vector<int16_t> chunk(FRAMES_PER_BUFFER, 0);
        if (capture_handle) {
            snd_pcm_sframes_t framesRead = snd_pcm_readi(capture_handle, chunk.data(), FRAMES_PER_BUFFER);
            if (framesRead < 0) {
                // Try to recover
                snd_pcm_recover(capture_handle, (int)framesRead, 0);
                // Return an empty chunk if we can't read
            }
        }
        return chunk;
    }


    // Play (output) a chunk of audio data
    void playChunk(const vector<int16_t>& data) {
        if (playback_handle) {
            snd_pcm_sframes_t framesWritten = snd_pcm_writei(playback_handle, data.data(), data.size());
            if (framesWritten < 0) {
                snd_pcm_recover(playback_handle, (int)framesWritten, 0);
            }
        }
    }

    // Stop recording
    void stopRecording() {
        if (capture_handle) {
            snd_pcm_close(capture_handle);
            capture_handle = nullptr;
        }
    }

    // Clean up
    void cleanup() {
        if (capture_handle) {
            snd_pcm_close(capture_handle);
            capture_handle = nullptr;
        }
        if (playback_handle) {
            snd_pcm_close(playback_handle);
            playback_handle = nullptr;
        }
    }

private:
    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;

    #else
        
    bool openAudioInput() {

        return true;
    }

    bool openAudioOutput() {

        return true;
    }

    vector<int16_t> recordChunk() {
        vector<int16_t> chunk(FRAMES_PER_BUFFER, 0);
        this_thread::sleep_for(chrono::seconds(1));
        return chunk;
    }

    void playChunk(const vector<int16_t>& data) {

    }

    void stopRecording() {

    }

    void cleanup() {

    }

private:
    int *capture_handle;
    int *playback_handle;

    #endif
};

// -----------------------------------------------------------
// OpenAIClient (WebSocket connection to OpenAI Realtime API)
// -----------------------------------------------------------
class OpenAIClient {
public:

    // Functions
    vector<string> functions = {"move_head", "move_face"};

    OpenAIClient(const string& instructions_, const string& voice_): instructions(instructions_), voice(voice_), context(nullptr), wsi(nullptr) {
        // Build session config JSON
        json sessionConfig = {
            {"modalities", {"audio", "text"}},
            {"instructions", instructions},
            {"voice", voice},
            {"input_audio_format", "pcm16"},
            {"output_audio_format", "pcm16"},
            {"turn_detection", {/*
                {"type", "server_vad"},
                {"threshold", 0.5},
                {"prefix_padding_ms", 300},
                {"silence_duration_ms", 600}
            */}},
            {"tools", {
                {
                    {"type", "function"},
                    {"name", functions[0]},
                    {"description", "Move your head to point more left, right, up, or down, to look in a direction."},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"direction", {
                                {"type", "string"},
                                {"description", "The direction to move."},
                                {"enum", {
                                    "Up",
                                    "Down", 
                                    "Left",
                                    "Right"
                                }}
                            }}
                        }},
                        {"required", {"direction"}}
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

        // Configure
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.gid = -1;
        info.uid = -1;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        #ifdef __linux__
        info.client_ssl_ca_filepath = "/etc/ssl/certs/ca-certificates.crt";
        #endif

        // Create
        context = lws_create_context(&info);
        if (!context) {
            cerr << "Failed to create lws context." << endl;
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
            cerr << "Failed to connect to server." << endl;
            return false;
        }
        return true;
    }

    // Service loop
    void serviceLoop() {
        while (true) {
            lws_service(context, 50);
            if (stopRequested) break;
        }
    }

    // Send event
    void sendEvent(const json &event) {
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
        //cout << msg << endl;
        auto j = json::parse(msg, nullptr, false);
        if (j.is_discarded()) {
            cerr << "Bad JSON: " << msg << endl;
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
                /*if (response.find("<UP>")      != string::npos) { move_head(   0,   40); move_face( 0,  1); response.clear(); }
                if (response.find("<DOWN>")    != string::npos) { move_head(   0,  -40); move_face( 0, -1); response.clear(); }
                if (response.find("<LEFT>")    != string::npos) { move_head(  40,    0); move_face( 0,  0); response.clear(); }
                if (response.find("<RIGHT>")   != string::npos) { move_head( -40,    0); move_face( 0,  0); response.clear(); }
                if (response.find("<UP 2>")    != string::npos) { move_head( 900,    0); move_face( 5,  0); response.clear(); }
                if (response.find("<DOWN 2>")  != string::npos) { move_head(-900,    0); move_face(-5,  0); response.clear(); }
                if (response.find("<LEFT 2>")  != string::npos) { move_head(   0,  200); move_face( 5,  0); response.clear(); }
                if (response.find("<RIGHT 2>") != string::npos) { move_head(   0, -200); move_face(-5,  0); response.clear(); }
                */
            }
            else if (type == "response.audio.done") {
                cout << endl;
                response.clear();
            }
            else if (type == "response.function_call_arguments.done") {
                // Parse the complete function arguments
                auto args = json::parse(j["arguments"].get<string>());
                string function = j["name"].get<string>();
                
                // Call the corresponding function
                if (function == functions[0]) {
                    string direction = args["direction"].get<string>();
                    move_head(direction);
                }
            }
            else if (type == "response.audio.delta") {
                // Base64 decode
                string b64data = j["delta"].get<string>();
                vector<uint8_t> audioBytes = base64Decode(b64data);

                // Convert to int16_t for playback
                vector<int16_t> samples(audioBytes.size() / 2);
                memcpy(samples.data(), audioBytes.data(), audioBytes.size());
                //audioHandler.enqueuePlayback(samples);

                // Play
                audioHandler.playChunk(samples);
            }
            else if (type == "response.done") {
                cout << "Response generation completed.\n";
                talking = false;
            }
            else if (type == "session.created") {
                //audioHandler.startPlaybackThread();
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
        cout << "Websocket closed." << endl;
    }

    void close() {
        stopRequested = true;
    }

    // Access to AudioHandler
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

                    // Add "Authorization: Bearer <OPENAI_KEY>"
                    string authValue = "Bearer " + OPENAI_KEY;
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
                    string msg((char *)in, len);
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
                break;
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
    struct lws_context *context;
    struct lws *wsi;
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
    { nullptr, nullptr, 0, 0 }
};

// -----------------------------------------------------------
// Wakeword
// -----------------------------------------------------------
class Wakeword {
public:
    Wakeword(const vector<string>& keywords): handle(nullptr), listening(false) {
        // Init
        #ifdef __linux__
            const char* keyword_paths[] = { "hey_robot_pi.ppn" };
        #else
            const char* keyword_paths[] = { "computer_mac.ppn" };
        #endif
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
            cerr << "Failed to init Porcupine wake word: " << pv_status_to_string(status) << "\n";
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

/*        // Open portaudio stream
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
        */
        return true;
    }

    void stop() {
        listening = false;
    }

private:
    pv_porcupine_t *handle;
    bool listening;
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
            cerr << "Websocket init failed.\n";
            return;
        }
        thread wsThread([this] { openAIClient.serviceLoop(); });

        // Wait for session creation
        this_thread::sleep_for(chrono::seconds(2));

        // Main loop
        while (true) {
            // Listen for wake word
            wakeword.listen();

            // Start conversation
            startConversation();

            // Sleep
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
        //openAIClient.audioHandler.startRecording();

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
