#include "../src/utils/window_functions.h"
#include "../src/features/time_domain_features.h"
#include "../src/features/mfcc_extractor.h"
#include "../src/features/spectral_features.h"
#include "../src/features/chroma_extractor.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

static int tests_passed = 0;
static int tests_failed = 0;

void check(const char* name, bool condition) {
    if (condition) {
        ++tests_passed;
    } else {
        std::cerr << "  FAIL: " << name << std::endl;
        ++tests_failed;
    }
}

void test_window_functions() {
    std::cout << "Window Functions:\n";

    auto hann = generate_window(WindowType::Hann, 1024);
    check("hann[0] near 0", std::abs(hann[0]) < 1e-10);
    check("hann[511] near 1", std::abs(hann[511] - 1.0) < 1e-3);
    check("hann size", hann.size() == 1024);

    auto rect = generate_window(WindowType::Rectangular, 100);
    check("rect all ones", rect[50] == 1.0);

    auto hamming = generate_window(WindowType::Hamming, 512);
    check("hamming[0] ~ 0.08", std::abs(hamming[0] - 0.08) < 0.01);
    check("hamming center ~ 1", std::abs(hamming[255] - 1.0) < 0.1);

    auto blackman = generate_window(WindowType::Blackman, 256);
    check("blackman[0] near 0", std::abs(blackman[0]) < 1e-3);

    auto bh = generate_window(WindowType::BlackmanHarris, 256);
    check("blackman_harris[0] near 0", std::abs(bh[0]) < 1e-4);
}

void test_time_domain() {
    std::cout << "Time Domain Features:\n";

    TimeDomainExtractor td;

    std::vector<double> silence(128, 0.0);
    auto r1 = td.compute_frame(silence.data(), 128);
    check("silence rms=0", std::abs(r1.rms) < 1e-12);
    check("silence peak=0", std::abs(r1.peak) < 1e-12);
    check("silence zcr=0", std::abs(r1.zcr) < 1e-12);

    std::vector<double> sine(1024);
    for (int i = 0; i < 1024; ++i)
        sine[i] = std::sin(2.0 * M_PI * 100.0 * i / 44100.0);
    auto r2 = td.compute_frame(sine.data(), 1024, 44100);
    check("sine rms ~ 0.707", std::abs(r2.rms - 0.7071) < 0.01);
    check("sine peak ~ 1", std::abs(r2.peak - 1.0) < 0.01);
    check("sine crest ~ 1.414", std::abs(r2.crest_factor - 1.414) < 0.05);

    std::vector<double> alternating(100);
    for (int i = 0; i < 100; ++i) alternating[i] = (i % 2 == 0) ? 1.0 : -1.0;
    auto r3 = td.compute_frame(alternating.data(), 100);
    check("alternating zcr ~ 1", std::abs(r3.zcr - 1.0) < 0.01);
}

void test_mfcc() {
    std::cout << "MFCC:\n";

    MFCCExtractor mfcc(44100, 1024, 26, 13, 300.0, 0.97, 22);

    check("num_coeffs = 13", mfcc.num_coeffs() == 13);

    std::vector<double> flat_spec(513, 1.0);
    auto coeffs = mfcc.compute(flat_spec);
    check("mfcc size = 13", coeffs.size() == 13);
    check("mfcc c0 non-zero", std::abs(coeffs[0]) > 0.0);

    std::vector<double> sig2(1024, 0.0);
    for (int i = 0; i < 1024; ++i)
        sig2[i] = std::sin(2.0 * M_PI * 440.0 * i / 44100.0);
    auto pre_copy = sig2;
    mfcc.apply_pre_emphasis(sig2);
    check("pre_emphasis changed signal", sig2[2] != pre_copy[2]);
}

void test_chroma() {
    std::cout << "Chroma:\n";

    ChromaExtractor chroma(44100, 1024);

    std::vector<double> empty_spec(513, 0.0);
    auto c1 = chroma.compute(empty_spec);
    check("empty chroma size=12", c1.size() == 12);

    std::vector<double> tone_spec(513, 0.0);

    int bin_440 = static_cast<int>(std::round(440.0 / 44100.0 * 1024.0));
    tone_spec[bin_440] = 1.0;
    auto c2 = chroma.compute(tone_spec);
    double max_val = 0.0;
    int max_idx = 0;
    for (int i = 0; i < 12; ++i) {
        if (c2[i] > max_val) { max_val = c2[i]; max_idx = i; }
    }
    check("440Hz maps to A (pitch class 9)", max_idx == 9 && max_val > 0.5);
}

void test_spectral() {
    std::cout << "Spectral Features (basic):\n";

    SpectralExtractor sp(44100, 1024);

    std::vector<double> spec(513, 0.0);

    int bin_1000 = static_cast<int>(std::round(1000.0 / 44100.0 * 1024.0));
    spec[bin_1000] = 1.0;

    auto f1 = sp.compute_frame(spec);
    check("centroid near 1000 Hz", std::abs(f1.centroid - 1000.0) < 100.0);
    check("rolloff near 1000 Hz", std::abs(f1.rolloff - 1000.0) < 100.0);

    spec.assign(513, 1.0);
    auto f2 = sp.compute_frame(spec);
    check("flat flatness ~ 1", std::abs(f2.flatness - 1.0) < 0.01);
}

int main() {
    test_window_functions();
    test_time_domain();
    test_mfcc();
    test_chroma();
    test_spectral();

    std::cout << "\n" << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}
