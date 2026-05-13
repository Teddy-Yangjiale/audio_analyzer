#pragma once
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>

struct TimeDomainFrame {
    double rms;
    double peak;
    double zcr;
    double crest_factor;
};

class TimeDomainExtractor {
public:
    TimeDomainExtractor() = default;

    TimeDomainFrame compute_frame(const double* samples, int frame_size, int sample_rate = 44100) const {
        TimeDomainFrame result{};

        if (frame_size <= 0) return result;

        double sum_sq = 0.0;
        double peak = 0.0;
        int zc_count = 0;

        for (int i = 0; i < frame_size; ++i) {
            double s = samples[i];
            sum_sq += s * s;
            double abs_s = std::abs(s);
            if (abs_s > peak) peak = abs_s;
            if (i > 0 && ((samples[i - 1] >= 0.0) != (s >= 0.0)))
                ++zc_count;
        }

        result.rms = std::sqrt(sum_sq / frame_size);
        result.peak = peak;
        result.zcr = static_cast<double>(zc_count) / (frame_size - 1);
        result.crest_factor = (result.rms > 1e-12) ? peak / result.rms : 0.0;

        return result;
    }
};
