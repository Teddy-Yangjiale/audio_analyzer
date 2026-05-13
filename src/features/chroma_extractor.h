#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class ChromaExtractor {
public:
    ChromaExtractor(int sample_rate = 44100, int fft_size = 1024,
                    double a4_freq = 440.0, int num_octaves = 7,
                    double tuning = 0.0)
        : sample_rate_(sample_rate), fft_size_(fft_size),
          a4_freq_(a4_freq), num_octaves_(num_octaves), tuning_(tuning)
    {}

    std::vector<double> compute(const std::vector<double>& power_spectrum) const {
        std::vector<double> chroma(12, 0.0);
        int num_bins = static_cast<int>(power_spectrum.size());
        double min_freq = a4_freq_ * std::pow(2.0, -4.0);
        double max_freq = min_freq * std::pow(2.0, num_octaves_);

        for (int k = 0; k < num_bins; ++k) {
            double freq = bin_to_freq(k);
            if (freq < min_freq || freq > max_freq) continue;

            double midi = 69.0 + 12.0 * std::log2(freq / a4_freq_) + tuning_;
            int pitch_class = static_cast<int>(std::round(midi)) % 12;
            if (pitch_class < 0) pitch_class += 12;

            chroma[pitch_class] += power_spectrum[k];
        }

        double total = 0.0;
        for (auto v : chroma) total += v;
        if (total > 1e-12) {
            for (auto& v : chroma) v /= total;
        }

        return chroma;
    }

private:
    int sample_rate_;
    int fft_size_;
    double a4_freq_;
    int num_octaves_;
    double tuning_;

    double bin_to_freq(int bin) const {
        return static_cast<double>(bin) * sample_rate_ / fft_size_;
    }
};
