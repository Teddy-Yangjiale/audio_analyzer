#pragma once
#include <vector>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris
};

inline std::vector<double> generate_window(WindowType type, int size) {
    std::vector<double> w(size, 1.0);
    switch (type) {
    case WindowType::Hann:
        for (int i = 0; i < size; ++i)
            w[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (size - 1)));
        break;
    case WindowType::Hamming:
        for (int i = 0; i < size; ++i)
            w[i] = 0.54 - 0.46 * std::cos(2.0 * M_PI * i / (size - 1));
        break;
    case WindowType::Blackman:
        for (int i = 0; i < size; ++i)
            w[i] = 0.42 - 0.5 * std::cos(2.0 * M_PI * i / (size - 1))
                 + 0.08 * std::cos(4.0 * M_PI * i / (size - 1));
        break;
    case WindowType::BlackmanHarris:
        for (int i = 0; i < size; ++i)
            w[i] = 0.35875 - 0.48829 * std::cos(2.0 * M_PI * i / (size - 1))
                 + 0.14128 * std::cos(4.0 * M_PI * i / (size - 1))
                 - 0.01168 * std::cos(6.0 * M_PI * i / (size - 1));
        break;
    case WindowType::Rectangular:
    default:
        break;
    }
    return w;
}

inline const char* window_type_name(WindowType type) {
    switch (type) {
    case WindowType::Rectangular:   return "rectangular";
    case WindowType::Hann:          return "hann";
    case WindowType::Hamming:       return "hamming";
    case WindowType::Blackman:      return "blackman";
    case WindowType::BlackmanHarris:return "blackman_harris";
    default:                        return "unknown";
    }
}

inline WindowType window_type_from_string(const std::string& s) {
    if (s == "hann")           return WindowType::Hann;
    if (s == "hamming")        return WindowType::Hamming;
    if (s == "blackman")       return WindowType::Blackman;
    if (s == "blackman_harris")return WindowType::BlackmanHarris;
    if (s == "rectangular")    return WindowType::Rectangular;
    return WindowType::Hann;
}
