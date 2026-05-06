# 音频分析核心引擎 (C++) / Audio Analyzer Core

## 项目简介

本项目是一个基于 C++17 开发的高性能音频特征提取工具。其核心功能是读取原始 `.wav` 格式音频文件，通过数字信号处理（DSP）算法提取梅尔频率倒谱系数（MFCC）及其一阶差分（Delta）特征。该工具输出标准化的 JSON 格式数据，旨在为机器学习流水线或实时音频分析任务提供高效、可靠的底层支持。

---

## 主要特性

* **高性能运算**：核心算法基于 FFTW3 库实现，确保快速傅里叶变换（FFT）的执行效率。
* **工业级标准**：利用 libsndfile 库处理音频输入输出，支持多种采样率和位深度。
* **全面的特征提取**：支持提取 13 维原始 MFCC 和 13 维一阶差分特征（共计 26 维特征向量）。
* **标准化数据输出**：结果以 JSON 格式输出，方便与 Python、JavaScript 或其他高级语言编写的程序无缝对接。
* **现代工程化构建**：采用 Conan 2.0 管理第三方依赖，结合 Ninja 构建系统，实现极速编译和跨平台兼容。

---

## 技术栈

* **编程语言**：C++17
* **构建系统**：CMake (>= 3.15) + Ninja
* **包管理器**：Conan 2.0
* **核心库依赖**：
    * **FFTW3**：用于执行快速傅里叶变换。
    * **libsndfile**：用于音频文件的读取、解码与元数据处理。
    * **nlohmann_json**：用于特征数据的 JSON 序列化输出。

---

## 环境准备

在开始编译前，请确保系统中已安装以下工具：

* CMake (3.15 或更高版本)
* Conan 2.0
* Ninja (推荐的构建后端)
* C++17 兼容的编译器 (如 MSVC 2019+, GCC 9+, Clang 10+)

---

## 编译指南

本项目使用 Conan 自动处理库依赖。请在项目根目录下执行以下步骤：

```bash
# 1. 安装依赖并生成构建配置文件
conan install . --output-folder=build --build=missing

# 2. 配置 CMake 项目 (推荐使用 Ninja 生成器)
cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# 3. 执行编译
ninja
```

编译完成后，项目目录下将生成可执行文件 `analyzer.exe` (或 `analyzer`)。

---

## 使用说明

调用编译生成的程序并指定音频文件路径，分析结果将直接打印至标准输出（stdout）：

```powershell
./analyzer.exe "path/to/your/audio.wav"
```

### 输出数据示例
```json
[
  {
    "frame": 0,
    "mfcc": [12.45, -3.21, 0.54, 0.12, ... ]
  },
  {
    "frame": 1,
    "mfcc": [11.89, -2.98, 0.45, 0.08, ... ]
  }
]
```

---

## 技术实现原理

本引擎遵循标准的语音信号处理流程：

1.  **预加重 (Pre-emphasis)**：通过高通滤波器平衡频谱，增强高频部分的能量。
2.  **分帧与加窗 (Framing & Windowing)**：将连续信号切分为短帧，并应用 **Hamming 窗** 以减少频谱泄漏。
3.  **快速傅里叶变换 (FFT)**：将时域信号转换为频域能量分布。
4.  **梅尔滤波器组 (Mel Filter Bank)**：应用梅尔刻度滤波器，模拟人类听觉对不同频率的感知灵敏度。
5.  **离散余弦变换 (DCT)**：对梅尔能量的对数进行变换，提取倒谱系数并实现特征去相关。

---

## 项目结构

```text
.
├── main.cpp            # 核心数字信号处理与特征提取逻辑
├── CMakeLists.txt      # CMake 构建配置
├── conanfile.txt       # Conan 依赖声明
├── .gitignore          # Git 忽略规则
└── README.md           # 项目文档
```

---

## 开源协议

本项目基于 **MIT License** 协议开源。

---

**由 Teddy-Yangjiale 开发维护**
