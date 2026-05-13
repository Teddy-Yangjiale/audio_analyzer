# Audio Analyzer — C++ 音频特征提取引擎

## 项目简介

基于 C++17 的高性能音频特征提取工具。支持任意 FFmpeg 兼容格式（MP3 / WAV / FLAC / AAC 等），
通过数字信号处理提取多维度时域、频域特征，输出标准化 JSON。提供 **CLI 命令行** 和 **HTTP Server** 两种模式，
适合机器学习流水线、音频检索、可视化分析等场景。

---

## 主要特性

- **广泛格式支持**：基于 FFmpeg，支持 MP3 / WAV / FLAC / AAC / OGG 等主流音频格式
- **完整特征集**（每帧 60+ 维）：
  - MFCC (13) + Delta (13) + Delta-Delta (13) + Log Energy
  - 时域：RMS、ZCR、Peak、Crest Factor
  - 频谱：Centroid、Rolloff、Flux、Bandwidth、Flatness、Contrast (6 bands)
  - 色度图 (12)
  - FFT 功率谱
- **DSP 管线**：预加重 → Hann 窗 → 50% 重叠 STFT → Mel 滤波器组 → DCT → Liftering
- **CLI 可配置**：`--features` 按需选择，`--fft-size` / `--window` / `--mfcc-coeffs` 等参数可调
- **HTTP Server**：`POST /upload` 接口，接收音频文件返回 JSON，支持 CORS
- **模块化架构**：core / features / config / utils 分层，RAII 封装 FFmpeg + FFTW3

---

## 技术栈

| 组件 | 用途 |
|------|------|
| C++17 | 编程语言 |
| CMake + Ninja | 构建系统 |
| Conan 2.x | 包管理器 |
| FFmpeg 4.4.4 | 音频解码、重采样 |
| FFTW3 3.3.10 | 快速傅里叶变换 |
| CrowCpp 1.1.0 | HTTP 服务器 |
| nlohmann/json 3.11.2 | JSON 序列化 |

---

## 环境准备

- CMake >= 3.15
- Conan 2.x
- C++17 编译器（MSVC 2019+ / GCC 9+ / Clang 10+）
- Ninja（推荐）

---

## 编译

```bash
# 1. 安装 Conan 依赖（生成 conan_toolchain.cmake 到项目根目录）
conan install . --output-folder=. --build=missing

# 2. 配置 CMake（指定 Conan toolchain）
cmake -S . -B build3 -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. 编译
cmake --build build3 --config Release

# 4. 运行测试（可选）
./build3/test_features.exe
```

---

## 使用说明

### CLI 模式

```bash
# 默认全特征分析
./build3/analyzer.exe music.mp3

# 只提取 MFCC + 时域特征
./build3/analyzer.exe --features mfcc,td music.mp3

# 自定义 FFT 参数
./build3/analyzer.exe --fft-size 2048 --window hamming --mfcc-coeffs 20 music.mp3

# 查看所有选项
./build3/analyzer.exe --help
```

### Server 模式

```bash
./build3/analyzer.exe --server
# 启动后访问 http://localhost:8080
# POST /upload  字段名: audio_file
```

### CLI 参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--features <list>` | 选择特征：mfcc,fft,td,spectral,chroma（逗号分隔） | 全开 |
| `--fft-size <n>` | FFT 窗大小 | 1024 |
| `--hop-size <n>` | 帧移 | fft_size/2 |
| `--sample-rate <hz>` | 目标采样率 | 44100 |
| `--mfcc-coeffs <n>` | MFCC 系数个数 | 13 |
| `--pre-emphasis <f>` | 预加重系数 | 0.97 |
| `--window <name>` | 窗类型：hann / hamming / blackman / rectangular | hann |
| `--fft-bins <n>` | 输出 FFT bin 数 | 64 |
| `--server` | HTTP Server 模式 | — |
| `--help` | 显示帮助 | — |

---

## 输出 JSON 格式

```json
[
  {
    "mfcc":         [-316.1, -91.2, 70.8, ...],
    "mfcc_delta":   [1.2, -0.8, 0.3, ...],
    "mfcc_delta2":  [0.1, 0.0, -0.1, ...],
    "energy":       4.69,
    "fft":          [2.0e-7, 5.2e-8, ...],
    "centroid":     18853.5,
    "rolloff":      19681.3,
    "bandwidth":    1004.7,
    "flatness":     0.0023,
    "flux":         0.0,
    "contrast":     [8.2, 10.1, 7.5, ...],
    "chroma":       [0.12, 0.05, 0.08, ...],
    "rms":          0.00037,
    "zcr":          0.85,
    "peak":         0.0011,
    "crest":        2.89
  }
]
```

---

## 处理管线

```
音频文件 → FFmpeg 解码 → 重采样 (mono 32-bit float) → 预加重
→ 分帧 + Hann 窗 → FFTW3 FFT → 功率谱
→ Mel 滤波器组 → log → DCT → Liftering → MFCC + Delta + Delta-Delta
→ 频谱特征 (Centroid / Rolloff / Flux / Bandwidth / Flatness / Contrast)
→ 色度图 (12 pitch classes)
→ 时域特征 (RMS / ZCR / Peak / Crest)
→ JSON 输出
```

---

## 项目结构

```
audio_analyzer/
├── main.cpp                          # CLI + HTTP Server 入口
├── CMakeLists.txt                    # 构建配置
├── conanfile.txt                     # Conan 依赖声明
├── CHANGELOG.md                      # 版本变更
├── test.mp3                          # 测试音频
├── src/
│   ├── core/
│   │   ├── audio_decoder.h/.cpp      # FFmpeg RAII 解码 + 重采样
│   │   └── fft_engine.h/.cpp         # FFTW3 RAII 封装 + STFT
│   ├── features/
│   │   ├── mfcc_extractor.h/.cpp     # MFCC 管线 (预加重/liftering/delta)
│   │   ├── time_domain_features.h    # RMS / ZCR / Peak / Crest
│   │   ├── spectral_features.h       # Centroid / Rolloff / Flux 等
│   │   └── chroma_extractor.h        # 12 维色度图
│   ├── config/
│   │   └── config.h                  # CLI 参数解析 + 特征选择
│   └── utils/
│       └── window_functions.h        # Hann / Hamming / Blackman 等
└── tests/
    └── test_features.cpp             # 24 个单元测试
```

---

## 开源协议

MIT License

---

**由 Teddy-Yangjiale 开发维护**
