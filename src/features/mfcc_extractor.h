#pragma once
#include <vector>
#include <string>

class MFCCExtractor {
public:
    MFCCExtractor(int sample_rate = 44100, int fft_size = 1024,
                  int num_filters = 26, int num_coeffs = 13,
                  double low_freq = 300.0, double pre_emphasis = 0.97,
                  int lifter = 22, int delta_window = 2);

    void apply_pre_emphasis(std::vector<double>& signal) const;
    std::vector<double> compute(const std::vector<double>& power_spectrum) const;
    void apply_lifter(std::vector<double>& mfcc) const;

    static std::vector<double> compute_delta(const std::vector<double>& mfcc,
                                             const std::vector<double>& prev,
                                             const std::vector<double>& next);
    static double compute_log_energy(const std::vector<double>& power_spectrum);

    int num_coeffs() const { return num_coeffs_; }

private:
    int sample_rate_;
    int fft_size_;
    int num_filters_;
    int num_coeffs_;
    int lifter_;
    double pre_emphasis_;
    std::vector<std::vector<double>> filterbank_;
    std::vector<std::vector<double>> dct_matrix_;
    std::vector<double> lifter_weights_;

    static double hz_to_mel(double hz);
    static double mel_to_hz(double mel);
    void build_filterbank(double low_freq, double high_freq);
    void build_dct_matrix();
    void build_lifter_weights();
};
