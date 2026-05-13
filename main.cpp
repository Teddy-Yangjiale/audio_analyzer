#define _WIN32_WINNT 0x0601

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "crow.h"
#include <nlohmann/json.hpp>

#include "src/config/config.h"
#include "src/core/audio_decoder.h"
#include "src/core/fft_engine.h"
#include "src/features/mfcc_extractor.h"
#include "src/features/time_domain_features.h"
#include "src/features/spectral_features.h"
#include "src/features/chroma_extractor.h"

using json = nlohmann::json;

json analyze_audio_file(const std::string& filepath, const AnalysisConfig& cfg) {
    json result = json::array();

    AudioDecoder decoder(cfg.sample_rate);
    auto decode_result = decoder.decode(filepath);
    if (!decode_result.success) {
        std::cerr << "[Error] " << decode_result.error_message << std::endl;
        return result;
    }

    if (decode_result.samples.empty()) return result;

    std::vector<double> pcm_double(decode_result.samples.size());
    for (size_t i = 0; i < decode_result.samples.size(); ++i)
        pcm_double[i] = static_cast<double>(decode_result.samples[i]);

    FFTEngine fft(cfg.fft_size, cfg.window_type);
    MFCCExtractor mfcc_ext(cfg.sample_rate, cfg.fft_size,
                           cfg.mfcc_num_filters, cfg.mfcc_num_coeffs,
                           cfg.mfcc_low_freq, cfg.pre_emphasis, cfg.mfcc_lifter);

    if (cfg.pre_emphasis > 0.0 && cfg.enable_mfcc)
        mfcc_ext.apply_pre_emphasis(pcm_double);

    auto frames = fft.compute_stft(pcm_double.data(),
        static_cast<int>(pcm_double.size()), cfg.hop_size, cfg.sample_rate);

    SpectralExtractor sp_ext(cfg.sample_rate, cfg.fft_size, cfg.spectral_rolloff_percent);
    ChromaExtractor chroma_ext(cfg.sample_rate, cfg.fft_size);
    TimeDomainExtractor td_ext;

    std::vector<std::vector<double>> all_mfcc, all_delta, all_delta2;

    if (cfg.enable_mfcc) {
        all_mfcc.reserve(frames.size());
        for (auto& frame : frames)
            all_mfcc.push_back(mfcc_ext.compute(frame.power_spectrum));

        all_delta.resize(all_mfcc.size());
        all_delta2.resize(all_mfcc.size());
        for (size_t i = 0; i < all_mfcc.size(); ++i) {
            const auto& prev = (i > 0) ? all_mfcc[i - 1] : all_mfcc[i];
            const auto& next = (i + 1 < all_mfcc.size()) ? all_mfcc[i + 1] : all_mfcc[i];
            all_delta[i] = MFCCExtractor::compute_delta(all_mfcc[i], prev, next);
        }
        for (size_t i = 0; i < all_delta.size(); ++i) {
            const auto& prev = (i > 0) ? all_delta[i - 1] : all_delta[i];
            const auto& next = (i + 1 < all_delta.size()) ? all_delta[i + 1] : all_delta[i];
            all_delta2[i] = MFCCExtractor::compute_delta(all_delta[i], prev, next);
        }
    }

    for (size_t i = 0; i < frames.size(); ++i) {
        auto& frame = frames[i];
        json f;

        if (cfg.enable_mfcc) {
            f["mfcc"] = all_mfcc[i];
            f["mfcc_delta"] = all_delta[i];
            f["mfcc_delta2"] = all_delta2[i];
            f["energy"] = MFCCExtractor::compute_log_energy(frame.power_spectrum);
        }

        if (cfg.enable_fft) {
            f["fft"] = std::vector<double>(
                frame.power_spectrum.begin(),
                frame.power_spectrum.begin() + std::min<size_t>(cfg.fft_output_bins, frame.power_spectrum.size()));
        }

        if (cfg.enable_spectral) {
            auto sp = sp_ext.compute_frame(frame.power_spectrum);
            f["centroid"] = sp.centroid;
            f["rolloff"] = sp.rolloff;
            f["bandwidth"] = sp.bandwidth;
            f["flatness"] = sp.flatness;
            f["flux"] = sp.flux;
            f["contrast"] = sp.contrast;
        }

        if (cfg.enable_chroma) {
            f["chroma"] = chroma_ext.compute(frame.power_spectrum);
        }

        if (cfg.enable_time_domain) {
            int offset = static_cast<int>(i) * cfg.hop_size;
            int td_len = std::min(cfg.fft_size, static_cast<int>(pcm_double.size()) - offset);
            if (td_len > 0) {
                auto td = td_ext.compute_frame(pcm_double.data() + offset, td_len, cfg.sample_rate);
                f["rms"] = td.rms;
                f["zcr"] = td.zcr;
                f["peak"] = td.peak;
                f["crest"] = td.crest_factor;
            }
        }

        result.push_back(std::move(f));
    }

    return result;
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        AnalysisConfig::print_usage();
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--server") {
        crow::SimpleApp app;
        CROW_ROUTE(app, "/upload").methods("POST"_method, "OPTIONS"_method)
        ([&](const crow::request& req) {
            if (req.method == "OPTIONS"_method) return crow::response(204);
            crow::multipart::message msg(req);
            auto it = msg.part_map.find("audio_file");
            if (it == msg.part_map.end()) return crow::response(400, "Missing file");

            std::string tmp_name = "analyzing_temp.mp3";
            std::ofstream out(tmp_name, std::ios::binary);
            out << it->second.body;
            out.close();

            AnalysisConfig cfg;
            std::cout << "[Workstation] Analyzing: " << tmp_name << " ...\n";
            json data = analyze_audio_file(tmp_name, cfg);
            std::cout << "[Workstation] Done. " << data.size() << " frames.\n";

            crow::response res(data.dump());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        });
        std::cout << "Audio Analysis Station Backend at http://localhost:8080\n";
        app.port(8080).multithreaded().run();
        return 0;
    }

    auto cfg = AnalysisConfig::parse_args(argc, argv);

    if (cfg.filepath.empty()) {
        std::cerr << "Usage: analyzer.exe [options] <audio_file>\n"
                  << "       analyzer.exe --server\n"
                  << "       analyzer.exe --help\n";
        return 1;
    }

    json data = analyze_audio_file(cfg.filepath, cfg);
    std::cout << data.dump() << std::endl;
    return 0;
}
