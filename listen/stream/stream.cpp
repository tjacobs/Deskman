
// Listen to speech, show images based on commands spoken.

#include "common-sdl.h"
#include "common.h"
#include "whisper.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <curl/curl.h>

// Generate images from Black forest labs Flux
#define API_URL_POST "https://api.bfl.ml/v1/flux-pro-1.1"
#define API_URL_GET "https://api.bfl.ml/v1/get_result?id=%s"
#define API_KEY "d4f34e29-13c0-407d-bfea-86953020f5f3"
#define TEMP_IMAGE_FILE "generated_image.jpg"

// Buffer to hold the response data from CURL
struct Memory {
    char *response;
    size_t size;
};

bool create_image(const char* prompt);
bool generate_image(const char *prompt, char *image_id);
bool get_image_url(const char *image_id, char *image_url);
bool download_image(const char* url, const char* filename);
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

// Command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;
    bool flash_attn    = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_out;
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

static bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"    || arg == "--threads")       { params.n_threads     = std::stoi(argv[++i]); }
        else if (                  arg == "--step")          { params.step_ms       = std::stoi(argv[++i]); }
        else if (                  arg == "--length")        { params.length_ms     = std::stoi(argv[++i]); }
        else if (                  arg == "--keep")          { params.keep_ms       = std::stoi(argv[++i]); }
        else if (arg == "-c"    || arg == "--capture")       { params.capture_id    = std::stoi(argv[++i]); }
        else if (arg == "-mt"   || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"   || arg == "--audio-ctx")     { params.audio_ctx     = std::stoi(argv[++i]); }
        else if (arg == "-vth"  || arg == "--vad-thold")     { params.vad_thold     = std::stof(argv[++i]); }
        else if (arg == "-fth"  || arg == "--freq-thold")    { params.freq_thold    = std::stof(argv[++i]); }
        else if (arg == "-tr"   || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-nf"   || arg == "--no-fallback")   { params.no_fallback   = true; }
        else if (arg == "-ps"   || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-kc"   || arg == "--keep-context")  { params.no_context    = false; }
        else if (arg == "-l"    || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-m"    || arg == "--model")         { params.model         = argv[++i]; }
        else if (arg == "-f"    || arg == "--file")          { params.fname_out     = argv[++i]; }
        else if (arg == "-tdrz" || arg == "--tinydiarize")   { params.tinydiarize   = true; }
        else if (arg == "-sa"   || arg == "--save-audio")    { params.save_audio    = true; }
        else if (arg == "-ng"   || arg == "--no-gpu")        { params.use_gpu       = false; }
        else if (arg == "-fa"   || arg == "--flash-attn")    { params.flash_attn    = true; }

        else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int /*argc*/, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help          [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,     --threads N     [%-7d] number of threads to use during computation\n",    params.n_threads);
    fprintf(stderr, "            --step N        [%-7d] audio step size in milliseconds\n",                params.step_ms);
    fprintf(stderr, "            --length N      [%-7d] audio length in milliseconds\n",                   params.length_ms);
    fprintf(stderr, "            --keep N        [%-7d] audio to keep from previous step in ms\n",         params.keep_ms);
    fprintf(stderr, "  -c ID,    --capture ID    [%-7d] capture device ID\n",                              params.capture_id);
    fprintf(stderr, "  -mt N,    --max-tokens N  [%-7d] maximum number of tokens per audio chunk\n",       params.max_tokens);
    fprintf(stderr, "  -ac N,    --audio-ctx N   [%-7d] audio context size (0 - all)\n",                   params.audio_ctx);
    fprintf(stderr, "  -vth N,   --vad-thold N   [%-7.2f] voice activity detection threshold\n",           params.vad_thold);
    fprintf(stderr, "  -fth N,   --freq-thold N  [%-7.2f] high-pass frequency cutoff\n",                   params.freq_thold);
    fprintf(stderr, "  -tr,      --translate     [%-7s] translate from source language to english\n",      params.translate ? "true" : "false");
    fprintf(stderr, "  -nf,      --no-fallback   [%-7s] do not use temperature fallback while decoding\n", params.no_fallback ? "true" : "false");
    fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",                           params.print_special ? "true" : "false");
    fprintf(stderr, "  -kc,      --keep-context  [%-7s] keep context between audio chunks\n",              params.no_context ? "false" : "true");
    fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",                                params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME   [%-7s] model path\n",                                     params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",                          params.fname_out.c_str());
    fprintf(stderr, "  -tdrz,    --tinydiarize   [%-7s] enable tinydiarize (requires a tdrz model)\n",     params.tinydiarize ? "true" : "false");
    fprintf(stderr, "  -sa,      --save-audio    [%-7s] save the recorded audio to a file\n",              params.save_audio ? "true" : "false");
    fprintf(stderr, "  -ng,      --no-gpu        [%-7s] disable GPU inference\n",                          params.use_gpu ? "false" : "true");
    fprintf(stderr, "  -fa,      --flash-attn    [%-7s] flash attention during inference\n",               params.flash_attn ? "true" : "false");
    fprintf(stderr, "\n");
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) return 1;

    params.keep_ms   = std::min(params.keep_ms,   params.step_ms);
    params.length_ms = std::max(params.length_ms, params.step_ms);

    const int n_samples_step = (1e-3*params.step_ms  )*WHISPER_SAMPLE_RATE;
    const int n_samples_len  = (1e-3*params.length_ms)*WHISPER_SAMPLE_RATE;
    const int n_samples_keep = (1e-3*params.keep_ms  )*WHISPER_SAMPLE_RATE;
    const int n_samples_30s  = (1e-3*30000.0         )*WHISPER_SAMPLE_RATE;

    const bool use_vad = n_samples_step <= 0; // sliding window mode uses VAD

    const int n_new_line = !use_vad ? std::max(1, params.length_ms / params.step_ms - 1) : 1; // number of steps to print new line

    params.no_timestamps  = !use_vad;
    params.no_context    |= use_vad;
    params.max_tokens     = 0;

    // init audio

    audio_async audio(params.length_ms);
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 1;
    }

    audio.resume();

    // whisper init
    if (params.language != "auto" && whisper_lang_id(params.language.c_str()) == -1){
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    struct whisper_context_params cparams = whisper_context_default_params();

    cparams.use_gpu    = params.use_gpu;
    cparams.flash_attn = params.flash_attn;

    struct whisper_context * ctx = whisper_init_from_file_with_params(params.model.c_str(), cparams);

    std::vector<float> pcmf32    (n_samples_30s, 0.0f);
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32_new(n_samples_30s, 0.0f);

    std::vector<whisper_token> prompt_tokens;

    // print some info about the processing
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "%s: processing %d samples (step = %.1f sec / len = %.1f sec / keep = %.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                n_samples_step,
                float(n_samples_step)/WHISPER_SAMPLE_RATE,
                float(n_samples_len )/WHISPER_SAMPLE_RATE,
                float(n_samples_keep)/WHISPER_SAMPLE_RATE,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        if (!use_vad) {
            fprintf(stderr, "%s: n_new_line = %d, no_context = %d\n", __func__, n_new_line, params.no_context);
        } else {
            fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n", __func__);
        }

        fprintf(stderr, "\n");
    }

    int n_iter = 0;

    bool is_running = true;

    std::ofstream fout;
    if (params.fname_out.length() > 0) {
        fout.open(params.fname_out);
        if (!fout.is_open()) {
            fprintf(stderr, "%s: failed to open output file '%s'!\n", __func__, params.fname_out.c_str());
            return 1;
        }
    }

    // Save wav file
    wav_writer wavWriter;
    if (params.save_audio) {
        // Get current date/time for filename
        time_t now = time(0);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localtime(&now));
        std::string filename = std::string(buffer) + ".wav";
        wavWriter.open(filename, WHISPER_SAMPLE_RATE, 16, 1);
    }
    printf("[Start speaking now]\n");
    fflush(stdout);

    // Time
    auto t_last  = std::chrono::high_resolution_clock::now();
    const auto t_start = t_last;

    // Main loop
    while (is_running) {
        // Save audio
        if (params.save_audio) wavWriter.write(pcmf32_new.data(), pcmf32_new.size());

        // Handle Ctrl + C
        is_running = sdl_poll_events();
        if (!is_running) break;

        // Process new audio, if not using Voice Activity Detection
        if (!use_vad) {
            while (true) {
                audio.get(params.step_ms, pcmf32_new);

                if ((int) pcmf32_new.size() > 2*n_samples_step) {
                    fprintf(stderr, "\n\n%s: WARNING: cannot process audio fast enough, dropping audio ...\n\n", __func__);
                    audio.clear();
                    continue;
                }

                if ((int) pcmf32_new.size() >= n_samples_step) {
                    audio.clear();
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            const int n_samples_new = pcmf32_new.size();

            // Take up to params.length_ms audio from previous iteration
            const int n_samples_take = std::min((int) pcmf32_old.size(), std::max(0, n_samples_keep + n_samples_len - n_samples_new));

            pcmf32.resize(n_samples_new + n_samples_take);

            for (int i = 0; i < n_samples_take; i++) {
                pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
            }

            memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new*sizeof(float));

            pcmf32_old = pcmf32;
        } else {
            const auto t_now  = std::chrono::high_resolution_clock::now();
            const auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count();

            if (t_diff < 2000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                continue;
            }

            audio.get(2000, pcmf32_new);

            if (::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, false)) {
                audio.get(params.length_ms, pcmf32);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                continue;
            }

            t_last = t_now;
        }

        // Run the inference
        {
            // Params
            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            wparams.print_progress   = false;
            wparams.print_special    = params.print_special;
            wparams.print_realtime   = false;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.translate        = params.translate;
            wparams.single_segment   = !use_vad;
            wparams.max_tokens       = params.max_tokens;
            wparams.language         = params.language.c_str();
            wparams.n_threads        = params.n_threads;
            wparams.audio_ctx        = params.audio_ctx;
            wparams.tdrz_enable      = params.tinydiarize; //wparams.temperature_inc  = -1.0f; // disable temperature fallback
            wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc; 
            wparams.prompt_tokens    = params.no_context ? nullptr : prompt_tokens.data();
            wparams.prompt_n_tokens  = params.no_context ? 0       : prompt_tokens.size();

            // Process
            if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                return 6;
            }

            // Print result;
            {
                // No voice activity detection mode
                if (!use_vad) {
                    // Non-VAD mode
                    printf("\33[2K\r");
                    printf("%s", std::string(100, ' ').c_str()); // Print long empty line to clear the previous line
                    printf("\33[2K\r");
                } else {
                    // VAD mode
                    const int64_t t1 = (t_last - t_start).count()/1000000;
                    const int64_t t0 = std::max(0.0, t1 - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);
                    printf("\n");
                    printf("### Transcription %d START | t0 = %d ms | t1 = %d ms\n", n_iter, (int) t0, (int) t1);
                    printf("\n");
                }

                // Get segments
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i) {
                    const char * text = whisper_full_get_segment_text(ctx, i);

                    // If no timestamps needed
                    if (params.no_timestamps) {
                        // Print text
                        printf("%s", text);

                        // If the user said "Show", then show them an image
                        if (strcasestr(text, "Show") != NULL) create_image(text);

                        // Show
                        fflush(stdout);

                        // Write to file
                        if (params.fname_out.length() > 0) fout << text;
                    } else {
                        // Print timestamps
                        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
                        std::string output = "[" + to_timestamp(t0, false) + " --> " + to_timestamp(t1, false) + "]  " + text;
                        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) output += " [SPEAKER_TURN]";
                        output += "\n";
                        printf("%s", output.c_str());
                        fflush(stdout);
                        if (params.fname_out.length() > 0) fout << output;
                    }
                }

                if (params.fname_out.length() > 0) {
                    fout << std::endl;
                }

                if (use_vad) {
                    printf("\n");
                    printf("### Transcription %d END\n", n_iter);
                }
            }

            ++n_iter;

            if (!use_vad && (n_iter % n_new_line) == 0) {
                printf("\n");

                // Keep part of the audio for next iteration to try to mitigate word boundary issues
                pcmf32_old = std::vector<float>(pcmf32.end() - n_samples_keep, pcmf32.end());

                // Add tokens of the last full length segment as the prompt
                if (!params.no_context) {
                    prompt_tokens.clear();
                    const int n_segments = whisper_full_n_segments(ctx);
                    for (int i = 0; i < n_segments; ++i) {
                        const int token_count = whisper_full_n_tokens(ctx, i);
                        for (int j = 0; j < token_count; ++j) {
                            prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
                        }
                    }
                }
            }
            fflush(stdout);
        }
    }

    // Done
    audio.pause();
    whisper_print_timings(ctx);
    whisper_free(ctx);
    return 0;
}



bool create_image(const char* prompt) {
    char image_id[128];
    char image_url[512];

    // Generate the image and get the ID
    if (!generate_image(prompt, image_id)) {
        fprintf(stderr, "Failed to generate image\n");
        return 1;
    }
    printf("Generated image ID: %s\n", image_id);

    while (true) {
        sleep(1);

        // Get the URL of the generated image
        if (!get_image_url(image_id, image_url)) {
            continue;
        }
        break;
    }

    // Download the image
    if (!download_image(image_url, TEMP_IMAGE_FILE)) {
        fprintf(stderr, "Failed to download image\n");
        return 1;
    }
    printf("Image downloaded successfully as %s\n", TEMP_IMAGE_FILE);


    // Initialize SDL objects to NULL
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Surface* image = NULL;
    SDL_Texture* texture = NULL;

    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image for multiple image formats
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF) == 0) {
        fprintf(stderr, "SDL_image could not initialize: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create SDL window
    window = SDL_CreateWindow("Image Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer for the window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load downloaded image
    image = IMG_Load(TEMP_IMAGE_FILE);
    if (!image) {
        fprintf(stderr, "Unable to load image: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create texture from loaded image
    texture = SDL_CreateTextureFromSurface(renderer, image);
    if (!texture) {
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(image);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Event handling variables
    SDL_Event event;
    bool quit = false;

    // Main event loop
    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
        }

        // Clear renderer
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render texture
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Present renderer
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // Limit frame rate
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}


// Callback function for libcurl to write data to a file
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// Function to download image using libcurl
bool download_image(const char* url, const char* filename) {
    CURL* curl;
    FILE* file;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Could not initialize libcurl\n");
        return false;
    }

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file for writing: %s\n", filename);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curl);

    fclose(file);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to download image: %s\n", curl_easy_strerror(res));
        return false;
    }

    return true;
}

// Callback to write response data into the buffer
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = (char*)realloc(mem->response, mem->size + total_size + 1);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, total_size);
    mem->size += total_size;
    mem->response[mem->size] = '\0';

    return total_size;
}

// Function to perform a POST request and get the generated image ID
bool generate_image(const char *prompt, char *image_id) {
    CURL *curl = curl_easy_init();
    struct Memory chunk = {0, 0};

    if (!curl) {
        fprintf(stderr, "Could not initialize CURL\n");
        return false;
    }
    
    // Prepare JSON data with the prompt
    char post_data[512];
    snprintf(post_data, sizeof(post_data),
             "{ \"prompt\": \"%s\", \"width\": 1024, \"height\": 768, \"prompt_upsampling\": true, \"seed\": 42, \"safety_tolerance\": 2, \"output_format\": \"jpeg\" }",
             prompt);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "X-Key: " API_KEY);

    curl_easy_setopt(curl, CURLOPT_URL, API_URL_POST);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "POST request failed: %s\n", curl_easy_strerror(res));
        free(chunk.response);
        return false;
    }

    // Parse the response for the "id" field
    const char *id_key = "\"id\":\"";
    char *id_start = strstr(chunk.response, id_key);
    if (!id_start) {
        fprintf(stderr, "No ID found in response\n");
        free(chunk.response);
        return false;
    }

    id_start += strlen(id_key);
    char *id_end = strchr(id_start, '\"');
    if (!id_end) {
        fprintf(stderr, "Invalid ID format\n");
        free(chunk.response);
        return false;
    }

    strncpy(image_id, id_start, id_end - id_start);
    image_id[id_end - id_start] = '\0';

    free(chunk.response);
    return true;
}

// Function to perform a GET request to retrieve the image URL
bool get_image_url(const char *image_id, char *image_url) {
    CURL *curl = curl_easy_init();
    struct Memory chunk = {0, 0};
    char url[256];

    if (!curl) {
        fprintf(stderr, "Could not initialize CURL\n");
        return false;
    }

    snprintf(url, sizeof(url), API_URL_GET, image_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));
        free(chunk.response);
        return false;
    }

    // Parse the response for the "sample" field
    const char *url_key = "\"sample\":\"";
    char *url_start = strstr(chunk.response, url_key);
    if (!url_start) {
        fprintf(stderr, "Waiting for image... %s\n", chunk.response);
        free(chunk.response);
        return false;
    }

    url_start += strlen(url_key);
    char *url_end = strchr(url_start, '\"');
    if (!url_end) {
        fprintf(stderr, "Invalid URL format\n");
        free(chunk.response);
        return false;
    }

    strncpy(image_url, url_start, url_end - url_start);
    image_url[url_end - url_start] = '\0';

    free(chunk.response);
    return true;
}




