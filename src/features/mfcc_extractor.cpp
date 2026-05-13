#include "mfcc_extractor.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MFCCExtractor::MFCCExtractor(int sample_rate, int fft_size,
                             int num_filters, int num_coeffs,
                             double low_freq, double pre_emphasis,
                             int lifter, int delta_window)
    : sample_rate_(sample_rate), fft_size_(fft_size),
      num_filters_(num_filters), num_coeffs_(num_coeffs),
      lifter_(lifter), pre_emphasis_(pre_emphasis)
{
    double high_freq = sample_rate_ / 2.0;
    build_filterbank(low_freq, high_freq);
    build_dct_matrix();
    build_lifter_weights();
}

double MFCCExtractor::hz_to_mel(double hz) {
    return 2595.0 * std::log10(1.0 + hz / 700.0);
}

double MFCCExtractor::mel_to_hz(double mel) {
    return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0);
}

void MFCCExtractor::build_filterbank(double low_freq, double high_freq) {
    int num_bins = fft_size_ / 2 + 1;
    double min_mel = hz_to_mel(low_freq);
    double max_mel = hz_to_mel(high_freq);
    double mel_step = (max_mel - min_mel) / (num_filters_ + 1);

    std::vector<double> center_hz(num_filters_ + 2);
    for (int i = 0; i < num_filters_ + 2; ++i)
        center_hz[i] = mel_to_hz(min_mel + i * mel_step);

    std::vector<int> bins(num_filters_ + 2);
    for (int i = 0; i < num_filters_ + 2; ++i)
        bins[i] = static_cast<int>(std::floor((fft_size_ + 1) * center_hz[i] / sample_rate_));

    filterbank_.assign(num_filters_, std::vector<double>(num_bins, 0.0));
    for (int i = 0; i < num_filters_; ++i) {
        for (int j = bins[i]; j < bins[i + 1]; ++j)
            filterbank_[i][j] = static_cast<double>(j - bins[i]) / (bins[i + 1] - bins[i]);
        for (int j = bins[i + 1]; j < bins[i + 2]; ++j)
            filterbank_[i][j] = static_cast<double>(bins[i + 2] - j) / (bins[i + 2] - bins[i + 1]);
    }
}

void MFCCExtractor::build_dct_matrix() {
    dct_matrix_.assign(num_coeffs_, std::vector<double>(num_filters_, 0.0));
    for (int i = 0; i < num_coeffs_; ++i)
        for (int j = 0; j < num_filters_; ++j)
            dct_matrix_[i][j] = std::cos(M_PI * i / num_filters_ * (j + 0.5));
}

void MFCCExtractor::build_lifter_weights() {
    lifter_weights_.resize(num_coeffs_, 1.0);
    if (lifter_ > 0) {
        for (int i = 0; i < num_coeffs_; ++i)
            lifter_weights_[i] = 1.0 + (lifter_ / 2.0) * std::sin(M_PI * i / lifter_);
    }
}

void MFCCExtractor::apply_pre_emphasis(std::vector<double>& signal) const {
    if (pre_emphasis_ <= 0.0 || signal.empty()) return;
    for (size_t i = signal.size() - 1; i > 0; --i)
        signal[i] -= pre_emphasis_ * signal[i - 1];
}

void MFCCExtractor::apply_lifter(std::vector<double>& mfcc) const {
    for (size_t i = 0; i < mfcc.size() && i < lifter_weights_.size(); ++i)
        mfcc[i] *= lifter_weights_[i];
}

std::vector<double> MFCCExtractor::compute(const std::vector<double>& power_spectrum) const {
    std::vector<double> energies(num_filters_, 0.0);
    int num_bins = static_cast<int>(power_spectrum.size());

    for (int i = 0; i < num_filters_; ++i) {
        for (int j = 0; j < num_bins; ++j)
            energies[i] += power_spectrum[j] * filterbank_[i][j];
        energies[i] = std::log(std::max(energies[i], 1e-10));
    }

    std::vector<double> mfcc(num_coeffs_, 0.0);
    for (int i = 0; i < num_coeffs_; ++i)
        for (int j = 0; j < num_filters_; ++j)
            mfcc[i] += energies[j] * dct_matrix_[i][j];

    if (lifter_ > 0) apply_lifter(mfcc);
    return mfcc;
}

std::vector<double> MFCCExtractor::compute_delta(const std::vector<double>& mfcc,
                                                  const std::vector<double>& prev,
                                                  const std::vector<double>& next) {
    size_t n = mfcc.size();
    std::vector<double> delta(n, 0.0);
    if (prev.empty() || next.empty()) return delta;
    for (size_t i = 0; i < n; ++i)
        delta[i] = (next[i] - prev[i]) / 2.0;
    return delta;
}

double MFCCExtractor::compute_log_energy(const std::vector<double>& power_spectrum) {
    double total = 0.0;
    for (auto v : power_spectrum) total += v;
    return std::log(std::max(total, 1e-12));
}
