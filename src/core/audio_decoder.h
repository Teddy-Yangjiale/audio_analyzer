#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct DecodeResult {
    std::vector<float> samples;
    int sample_rate = 0;
    int channels = 0;
    int original_sample_rate = 0;
    int original_channels = 0;
    std::string codec_name;
    double duration_seconds = 0.0;
    bool success = false;
    std::string error_message;
};

class AudioDecoder {
public:
    explicit AudioDecoder(int target_sample_rate = 44100);
    ~AudioDecoder();

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    DecodeResult decode(const std::string& filepath);

private:
    int target_sample_rate_;
};
