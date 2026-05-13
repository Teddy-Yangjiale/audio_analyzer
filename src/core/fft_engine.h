#pragma once
#include <vector>
#include <utility>
#include "../utils/window_functions.h"

extern "C" {
#include <fftw3.h>
}

struct FFTFrame {
    std::vector<double> power_spectrum;
    int frame_index;
    double timestamp;
};

class FFTEngine {
public:
    FFTEngine(int fft_size, WindowType window_type = WindowType::Rectangular);
    ~FFTEngine();

    FFTEngine(const FFTEngine&) = delete;
    FFTEngine& operator=(const FFTEngine&) = delete;

    int fft_size() const { return fft_size_; }
    int num_bins() const { return num_bins_; }
    const std::vector<double>& window() const { return window_; }
    WindowType window_type() const { return window_type_; }

    FFTFrame compute_frame(const double* samples);
    std::vector<FFTFrame> compute_stft(const double* pcm, int num_samples,
                                       int hop_size, int sample_rate = 44100);

private:
    int fft_size_;
    int num_bins_;
    WindowType window_type_;
    std::vector<double> window_;

    double* fft_in_;
    fftw_complex* fft_out_;
    fftw_plan plan_;
    bool plan_valid_;
};
