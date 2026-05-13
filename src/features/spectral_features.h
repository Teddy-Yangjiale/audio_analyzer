#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct SpectralFrame {
    double centroid;
    double rolloff;
    double bandwidth;
    double flatness;
    double flux;
    std::vector<double> contrast;
};

class SpectralExtractor {
public:
    SpectralExtractor(int sample_rate = 44100, int fft_size = 1024,
                      double rolloff_percent = 0.85)
        : sample_rate_(sample_rate), fft_size_(fft_size),
          rolloff_percent_(rolloff_percent), first_frame_(true)
    {
        int num_bins = fft_size_ / 2 + 1;
        prev_power_.resize(num_bins, 0.0);
    }

    SpectralFrame compute_frame(const std::vector<double>& power_spectrum) {
        SpectralFrame result{};
        int num_bins = static_cast<int>(power_spectrum.size());
        if (num_bins == 0) return result;

        double total_energy = 0.0;
        double weighted_sum = 0.0;
        double log_sum = 0.0;

        for (int k = 0; k < num_bins; ++k) {
            double mag = power_spectrum[k];
            total_energy += mag;
            double freq = bin_to_freq(k);
            weighted_sum += freq * mag;
        }

        if (total_energy < 1e-12) {
            prev_power_ = power_spectrum;
            first_frame_ = false;
            return result;
        }

        result.centroid = weighted_sum / total_energy;

        double cum_energy = 0.0;
        double rolloff_thresh = rolloff_percent_ * total_energy;
        for (int k = 0; k < num_bins; ++k) {
            cum_energy += power_spectrum[k];
            if (cum_energy >= rolloff_thresh) {
                result.rolloff = bin_to_freq(k);
                break;
            }
        }

        double bw_sum = 0.0;
        for (int k = 0; k < num_bins; ++k) {
            double freq = bin_to_freq(k);
            double diff = freq - result.centroid;
            bw_sum += power_spectrum[k] * diff * diff;
        }
        result.bandwidth = std::sqrt(bw_sum / total_energy);

        double geom_sum = 0.0;
        double arith_sum = 0.0;
        for (int k = 0; k < num_bins; ++k) {
            double mag = std::max(power_spectrum[k], 1e-12);
            geom_sum += std::log(mag);
            arith_sum += mag;
        }
        double geom_mean = std::exp(geom_sum / num_bins);
        double arith_mean = arith_sum / num_bins;
        result.flatness = (arith_mean > 1e-12) ? geom_mean / arith_mean : 0.0;

        double flux_val = 0.0;
        if (!first_frame_) {
            for (int k = 0; k < num_bins; ++k) {
                double diff = power_spectrum[k] - prev_power_[k];
                flux_val += std::max(diff, 0.0);
            }
        }
        result.flux = flux_val;

        result.contrast = compute_contrast(power_spectrum);

        prev_power_ = power_spectrum;
        first_frame_ = false;
        return result;
    }

private:
    int sample_rate_;
    int fft_size_;
    double rolloff_percent_;
    bool first_frame_;
    std::vector<double> prev_power_;

    double bin_to_freq(int bin) const {
        return static_cast<double>(bin) * sample_rate_ / fft_size_;
    }

    std::vector<double> compute_contrast(const std::vector<double>& power_spectrum) {
        int num_bins = static_cast<int>(power_spectrum.size());
        double nyquist = sample_rate_ / 2.0;

        std::vector<double> contrast(6, 0.0);
        double low_freq = 200.0;
        int num_bands = 6;

        for (int b = 0; b < num_bands; ++b) {
            double f_low = low_freq * std::pow(2.0, b);
            double f_high = f_low * 2.0;
            if (f_low >= nyquist) break;

            int k1 = std::max(0, static_cast<int>(std::floor(f_low / sample_rate_ * fft_size_)));
            int k2 = std::min(num_bins - 1, static_cast<int>(std::ceil(f_high / sample_rate_ * fft_size_)));

            if (k2 <= k1) continue;

            double peak_val = 0.0, valley_val = 1e100;
            double total = 0.0;
            int count = 0;

            for (int k = k1; k <= k2; ++k) {
                double v = power_spectrum[k];
                peak_val = std::max(peak_val, v);
                if (v > 0) valley_val = std::min(valley_val, v);
                total += v;
                ++count;
            }

            double peak_db = (peak_val > 1e-12) ? 10.0 * std::log10(peak_val) : -120.0;
            double valley_db = (valley_val > 1e-12 && valley_val < 1e99) ? 10.0 * std::log10(valley_val) : -120.0;
            contrast[b] = peak_db - valley_db;
        }

        return contrast;
    }
};
