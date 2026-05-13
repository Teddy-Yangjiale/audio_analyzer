#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "../utils/window_functions.h"

struct AnalysisConfig {
    int sample_rate = 44100;
    int fft_size = 1024;
    int hop_size = 512;
    double pre_emphasis = 0.97;
    int mfcc_num_filters = 26;
    int mfcc_num_coeffs = 13;
    double mfcc_low_freq = 300.0;
    int mfcc_lifter = 22;
    double spectral_rolloff_percent = 0.85;
    WindowType window_type = WindowType::Hann;

    bool enable_mfcc = true;
    bool enable_fft = true;
    bool enable_time_domain = true;
    bool enable_spectral = true;
    bool enable_chroma = true;

    int fft_output_bins = 64;

    std::string filepath;

    static AnalysisConfig parse_args(int argc, char* argv[]) {
        AnalysisConfig cfg;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help") {
                cfg.filepath = ""; // handled by caller
                return cfg;
            }
            else if (arg == "--features" && i + 1 < argc) {
                cfg.enable_mfcc = false;
                cfg.enable_fft = false;
                cfg.enable_time_domain = false;
                cfg.enable_spectral = false;
                cfg.enable_chroma = false;

                std::string val = argv[++i];
                std::istringstream ss(val);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    if (token == "mfcc") cfg.enable_mfcc = true;
                    else if (token == "fft") cfg.enable_fft = true;
                    else if (token == "td") cfg.enable_time_domain = true;
                    else if (token == "spectral") cfg.enable_spectral = true;
                    else if (token == "chroma") cfg.enable_chroma = true;
                }
            }
            else if (arg == "--fft-size" && i + 1 < argc) {
                cfg.fft_size = std::stoi(argv[++i]);
                cfg.hop_size = cfg.fft_size / 2;
                cfg.fft_output_bins = cfg.fft_size / 16;
            }
            else if (arg == "--hop-size" && i + 1 < argc) {
                cfg.hop_size = std::stoi(argv[++i]);
            }
            else if (arg == "--sample-rate" && i + 1 < argc) {
                cfg.sample_rate = std::stoi(argv[++i]);
            }
            else if (arg == "--mfcc-coeffs" && i + 1 < argc) {
                cfg.mfcc_num_coeffs = std::stoi(argv[++i]);
            }
            else if (arg == "--pre-emphasis" && i + 1 < argc) {
                cfg.pre_emphasis = std::stod(argv[++i]);
            }
            else if (arg == "--window" && i + 1 < argc) {
                cfg.window_type = window_type_from_string(argv[++i]);
            }
            else if (arg == "--fft-bins" && i + 1 < argc) {
                cfg.fft_output_bins = std::stoi(argv[++i]);
            }
            else if (arg[0] != '-') {
                cfg.filepath = arg;
            }
        }

        return cfg;
    }

    static void print_usage() {
        std::cout << "Usage: analyzer.exe [--server] [<audio_file>] [options]\n\n"
                  << "Options:\n"
                  << "  --features <list>     Comma-separated: mfcc,fft,td,spectral,chroma\n"
                  << "  --fft-size <n>        FFT size (default 1024)\n"
                  << "  --hop-size <n>        Hop size (default fft_size/2)\n"
                  << "  --sample-rate <hz>    Target sample rate (default 44100)\n"
                  << "  --mfcc-coeffs <n>     Number of MFCC coefficients (default 13)\n"
                  << "  --pre-emphasis <f>    Pre-emphasis coefficient (default 0.97)\n"
                  << "  --window <name>       Window type: hann, hamming, blackman, rectangular\n"
                  << "  --fft-bins <n>        FFT bins in output (default 64)\n"
                  << "  --server              Start HTTP server mode\n"
                  << "  --help                Show this help\n";
    }
};
