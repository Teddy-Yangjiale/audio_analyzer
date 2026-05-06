#define _WIN32_WINNT 0x0601
#define _USE_MATH_DEFINES 
#define NOMINMAX
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <fftw3.h>
#include "crow.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ==========================================
// MFCC 特征提取器类
// ==========================================
class MFCCExtractor {
private:
    int num_filters, num_coeffs, fft_size, sample_rate;
    std::vector<std::vector<double>> filterbank;
    double hz_to_mel(double hz) { return 2595.0 * std::log10(1.0 + hz / 700.0); }
    double mel_to_hz(double mel) { return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0); }

public:
    MFCCExtractor(int sr, int fs, int nf = 26, int nc = 13) : sample_rate(sr), fft_size(fs), num_filters(nf), num_coeffs(nc) {
        filterbank.resize(num_filters, std::vector<double>(fft_size / 2 + 1, 0.0));
        double min_mel = hz_to_mel(300.0), max_mel = hz_to_mel(sample_rate / 2.0);
        double mel_step = (max_mel - min_mel) / (num_filters + 1);
        std::vector<double> center_hz(num_filters + 2);
        for (int i = 0; i < num_filters + 2; ++i) center_hz[i] = mel_to_hz(min_mel + i * mel_step);
        std::vector<int> bins(num_filters + 2);
        for (int i = 0; i < num_filters + 2; ++i) bins[i] = static_cast<int>(std::floor((fft_size + 1) * center_hz[i] / sample_rate));
        for (int i = 0; i < num_filters; ++i) {
            for (int j = bins[i]; j < bins[i + 1]; ++j) filterbank[i][j] = (j - bins[i]) / (double)(bins[i + 1] - bins[i]);
            for (int j = bins[i + 1]; j < bins[i + 2]; ++j) filterbank[i][j] = (bins[i + 2] - j) / (double)(bins[i + 2] - bins[i + 1]);
        }
    }

    std::vector<double> compute(const std::vector<double>& power_spec) {
        std::vector<double> energies(num_filters, 0.0);
        for (int i = 0; i < num_filters; ++i) {
            for (int j = 0; j < power_spec.size(); ++j) energies[i] += power_spec[j] * filterbank[i][j];
            energies[i] = std::log(energies[i] < 1e-10 ? 1e-10 : energies[i]);
        }
        std::vector<double> res(num_coeffs, 0.0);
        for (int i = 0; i < num_coeffs; ++i)
            for (int j = 0; j < num_filters; ++j) res[i] += energies[j] * std::cos(M_PI * i / num_filters * (j + 0.5));
        return res;
    }
};

// ==========================================
// 核心分析函数：全量处理音频文件
// ==========================================
json analyze_audio_file(const std::string& filepath) {
    json result = json::array();

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filepath.c_str(), nullptr, nullptr) != 0) return result;
    avformat_find_stream_info(fmt_ctx, nullptr);

    int stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    AVCodecParameters* codec_par = fmt_ctx->streams[stream_idx]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_par);
    avcodec_open2(codec_ctx, codec, nullptr);

    SwrContext* swr = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, 44100, 
        (codec_ctx->channel_layout ? codec_ctx->channel_layout : av_get_default_channel_layout(codec_ctx->channels)),
        codec_ctx->sample_fmt, codec_ctx->sample_rate, 0, nullptr);
    swr_init(swr);

    const int N = 1024;
    double* fft_in = (double*)fftw_malloc(sizeof(double) * N);
    fftw_complex* fft_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, fft_in, fft_out, FFTW_ESTIMATE);
    MFCCExtractor mfcc_ext(44100, N);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frm = av_frame_alloc();
    uint8_t* out_buf = nullptr;
    av_samples_alloc(&out_buf, nullptr, 1, 8192, AV_SAMPLE_FMT_FLT, 0);

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == stream_idx) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frm) == 0) {
                    int count = swr_convert(swr, &out_buf, 8192, (const uint8_t**)frm->data, frm->nb_samples);
                    if (count >= N) {
                        float* p = reinterpret_cast<float*>(out_buf);
                        for (int i = 0; i < N; ++i) fft_in[i] = static_cast<double>(p[i]);
                        fftw_execute(plan);
                        
                        std::vector<double> spec(N / 2 + 1);
                        for (int k = 0; k <= N / 2; ++k) spec[k] = (fft_out[k][0] * fft_out[k][0] + fft_out[k][1] * fft_out[k][1]);
                        
                        json f;
                        f["mfcc"] = mfcc_ext.compute(spec);
                        // 压缩：只取前 64 个频段
                        f["fft"] = std::vector<double>(spec.begin(), spec.begin() + 64);
                        result.push_back(f);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    // 清理
    av_freep(&out_buf); fftw_destroy_plan(plan); fftw_free(fft_in); fftw_free(fft_out);
    swr_free(&swr); av_frame_free(&frm); av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx); avformat_close_input(&fmt_ctx);

    return result;
}

// ==========================================
// 主函数与 Crow 路由
// ==========================================
int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string(argv[1]) != "--server") {
        json data = analyze_audio_file(argv[1]); // 使用我们之前写好的全量分析函数
        std::cout << data.dump() << std::endl;
        return 0;
    }
    crow::SimpleApp app;

    // 路由：处理音频文件上传
    CROW_ROUTE(app, "/upload").methods("POST"_method, "OPTIONS"_method)
    ([&](const crow::request& req) {
        // 处理跨域请求 (CORS)
        if (req.method == "OPTIONS"_method) return crow::response(204);

        crow::multipart::message msg(req);
        auto it = msg.part_map.find("audio_file");
        if (it == msg.part_map.end()) return crow::response(400, "Missing file");

        // 1. 保存上传的文件到磁盘
        std::string tmp_name = "analyzing_temp.mp3";
        std::ofstream out(tmp_name, std::ios::binary);
        out << it->second.body;
        out.close();

        // 2. 进行全量数据分析
        std::cout << "[Workstation] 正在深度分析文件: " << tmp_name << " ...\n";
        json data = analyze_audio_file(tmp_name);
        std::cout << "[Workstation] 分析完成，共 " << data.size() << " 帧。\n";

        // 3. 返回结果
        crow::response res(data.dump());
        res.add_header("Access-Control-Allow-Origin", "*"); // 允许跨域
        return res;
    });

    std::cout << "Audio Analysis Station Backend started at http://localhost:8080\n";
    app.port(8080).multithreaded().run();
}