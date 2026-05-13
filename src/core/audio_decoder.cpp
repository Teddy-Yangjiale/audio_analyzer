#include "audio_decoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <cmath>

AudioDecoder::AudioDecoder(int target_sample_rate)
    : target_sample_rate_(target_sample_rate) {}

AudioDecoder::~AudioDecoder() {}

DecodeResult AudioDecoder::decode(const std::string& filepath) {
    DecodeResult result{};
    result.success = false;

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filepath.c_str(), nullptr, nullptr) != 0) {
        result.error_message = "Failed to open file: " + filepath;
        return result;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        result.error_message = "Failed to find stream info";
        return result;
    }

    int stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_idx < 0) {
        avformat_close_input(&fmt_ctx);
        result.error_message = "No audio stream found";
        return result;
    }

    AVCodecParameters* codec_par = fmt_ctx->streams[stream_idx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
    if (!codec) {
        avformat_close_input(&fmt_ctx);
        result.error_message = "Unsupported codec";
        return result;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        avformat_close_input(&fmt_ctx);
        result.error_message = "Failed to allocate codec context";
        return result;
    }
    if (avcodec_parameters_to_context(codec_ctx, codec_par) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        result.error_message = "Failed to copy codec params";
        return result;
    }
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        result.error_message = "Failed to open codec";
        return result;
    }

    result.original_sample_rate = codec_ctx->sample_rate;
    result.original_channels = codec_ctx->channels;
    result.codec_name = codec->name ? codec->name : "unknown";
    result.sample_rate = target_sample_rate_;
    result.channels = 1;
    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        result.duration_seconds = static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE;
    }

    int64_t in_ch_layout = codec_ctx->channel_layout;
    if (in_ch_layout == 0) {
        in_ch_layout = av_get_default_channel_layout(codec_ctx->channels);
    }

    SwrContext* swr = swr_alloc_set_opts(nullptr,
        AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, target_sample_rate_,
        in_ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
        0, nullptr);
    if (!swr || swr_init(swr) < 0) {
        if (swr) swr_free(&swr);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        result.error_message = "Failed to initialize resampler";
        return result;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frm = av_frame_alloc();
    uint8_t* out_buf = nullptr;
    av_samples_alloc(&out_buf, nullptr, 1, 8192, AV_SAMPLE_FMT_FLT, 0);

    int estimated_samples = target_sample_rate_ *
        static_cast<int>(std::ceil(result.duration_seconds)) + 8192;
    result.samples.reserve(estimated_samples);

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == stream_idx) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frm) == 0) {
                    int count = swr_convert(swr, &out_buf, 8192,
                        const_cast<const uint8_t**>(frm->data), frm->nb_samples);
                    if (count > 0) {
                        float* p = reinterpret_cast<float*>(out_buf);
                        result.samples.insert(result.samples.end(), p, p + count);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_freep(&out_buf);
    av_frame_free(&frm);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    result.success = true;
    return result;
}
