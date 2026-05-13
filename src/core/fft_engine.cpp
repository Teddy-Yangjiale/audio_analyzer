#include "fft_engine.h"
#include <fftw3.h>
#include <algorithm>
#include <cstring>

FFTEngine::FFTEngine(int fft_size, WindowType window_type)
    : fft_size_(fft_size), num_bins_(fft_size / 2 + 1),
      window_type_(window_type),
      fft_in_(nullptr), fft_out_(nullptr), plan_(nullptr), plan_valid_(false)
{
    window_ = generate_window(window_type_, fft_size_);
    fft_in_ = static_cast<double*>(fftw_malloc(sizeof(double) * fft_size_));
    fft_out_ = static_cast<fftw_complex*>(
        fftw_malloc(sizeof(fftw_complex) * num_bins_));
    plan_ = fftw_plan_dft_r2c_1d(fft_size_, fft_in_, fft_out_, FFTW_MEASURE);
    plan_valid_ = true;
}

FFTEngine::~FFTEngine() {
    if (plan_valid_) fftw_destroy_plan(plan_);
    if (fft_in_) fftw_free(fft_in_);
    if (fft_out_) fftw_free(fft_out_);
}

FFTFrame FFTEngine::compute_frame(const double* samples) {
    FFTFrame frame;
    frame.power_spectrum.resize(num_bins_, 0.0);

    for (int i = 0; i < fft_size_; ++i)
        fft_in_[i] = samples[i] * window_[i];

    fftw_execute(plan_);

    for (int k = 0; k < num_bins_; ++k) {
        double re = fft_out_[k][0];
        double im = fft_out_[k][1];
        frame.power_spectrum[k] = re * re + im * im;
    }
    return frame;
}

std::vector<FFTFrame> FFTEngine::compute_stft(const double* pcm, int num_samples,
                                              int hop_size, int sample_rate) {
    std::vector<FFTFrame> frames;

    if (num_samples <= 0) return frames;

    int num_frames = (num_samples - fft_size_) / hop_size + 1;
    if (num_frames < 1) num_frames = 1;
    frames.reserve(num_frames);

    std::vector<double> frame_buffer(fft_size_, 0.0);

    for (int i = 0; i < num_frames; ++i) {
        int offset = i * hop_size;
        int available = num_samples - offset;
        int copy_len = std::min(fft_size_, available);

        std::memcpy(frame_buffer.data(), pcm + offset, copy_len * sizeof(double));
        if (copy_len < fft_size_)
            std::fill(frame_buffer.begin() + copy_len, frame_buffer.end(), 0.0);

        auto f = compute_frame(frame_buffer.data());
        f.frame_index = i;
        f.timestamp = static_cast<double>(i * hop_size) / sample_rate;
        frames.push_back(std::move(f));
    }

    return frames;
}
