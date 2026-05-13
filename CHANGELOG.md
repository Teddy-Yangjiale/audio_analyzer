# Changelog

## [v0.2.0] — 2026-05-13

### 重构 + 特征增强版本

#### 架构重构
- 拆分单文件为模块化结构：
  - `src/core/` — `AudioDecoder`（FFmpeg RAII）、`FFTEngine`（FFTW3 RAII）
  - `src/features/` — `MFCCExtractor`、`TimeDomainExtractor`、`SpectralExtractor`、`ChromaExtractor`
  - `src/config/` — `AnalysisConfig` 配置系统
  - `src/utils/` — `window_functions` 窗函数库
  - `tests/` — 24 个单元测试

#### 核心改进
- Hann 窗替代矩形窗，消除频谱泄漏
- 50% 重叠 STFT（hop_size = fft_size / 2）
- FFTW plan 升级为 `FFTW_MEASURE` 精度
- 解码全缓冲分离（先解码后分析）
- DCT 矩阵预计算（MFCC）
- 移除废弃 FFmpeg API

#### 新增特征类型
| 类别 | 特征 |
|------|------|
| 时域 | RMS 能量、峰值幅度、零交叉率、波峰因子 |
| 频谱 | 频谱质心、衰减点（85%）、带宽、平坦度、通量、对比度（6 个八度带） |
| MFCC | 预加重、liftering、delta（速度）、delta-delta（加速度）、log 能量 |
| 色度 | 12 维色度向量（A4=440Hz 基准，归一化） |

#### CLI 改进
- `--features mfcc,td,spectral,chroma` 按需选择特征
- `--fft-size`、`--hop-size`、`--mfcc-coeffs`、`--window`、`--pre-emphasis` 等可配参数
- `--help` 显示帮助信息
- 各特征可通过 config 独立开关

#### 输出 JSON 格式（每帧）
```json
{
  "mfcc": [13], "mfcc_delta": [13], "mfcc_delta2": [13], "energy": double,
  "fft": [64],
  "centroid": double, "rolloff": double, "bandwidth": double,
  "flatness": double, "flux": double, "contrast": [6],
  "chroma": [12],
  "rms": double, "zcr": double, "peak": double, "crest": double
}
```

---

## [v0.1.0] — 2026-05-13

### Initial version: 单文件音频特征提取工具

#### 项目概况
- 单文件 C++17 应用（`main.cpp`，166 行）
- 构建系统：CMake 3.15+ / Conan 2.x / MSVC (v145) + Ninja
- 依赖：FFmpeg 4.4.4、FFTW3 3.3.10、CrowCpp 1.1.0、nlohmann/json 3.11.2

#### 功能

**CLI 模式**
- `analyzer.exe <audio_file>`：解码音频文件，输出全帧分析 JSON 到 stdout

**Server 模式**
- `analyzer.exe --server`：在 `http://localhost:8080` 启动 HTTP 服务
- `POST /upload`：接收 `multipart/form-data`（字段名 `audio_file`），返回分析 JSON
- `OPTIONS` 预检：返回 204，支持 CORS 跨域
- 多线程模式运行

**分析流程**
1. FFmpeg 解码 → 自动选择最优音频流
2. libswresample 重采样 → mono / 32-bit float / 44.1 kHz
3. FFTW3 短时傅里叶变换 → 窗口 1024 样点，real-to-complex
4. MFCC 提取 → 26 个 Mel 滤波器（300 Hz ~ Nyquist），13 维系数
5. 输出 JSON → 每帧 `{ "mfcc": [...], "fft": [...] }`（FFT 截取前 64 bin）

**MFCC 实现细节**
- Mel 滤波器组：26 个三角形滤波器，中心频率线性分布在 Mel 尺度
- 能量压缩：`log(max(energy, 1e-10))`
- 离散余弦变换（DCT-II）：直接双重循环 O(n²) 实现
- 无预加重、无 liftering、无 delta/delta-delta

**输出 JSON 格式**
```json
[
  { "mfcc": [13 floats], "fft": [64 floats] },
  ...
]
```

#### 已知限制
| 类别 | 问题 |
|------|------|
| 解码 | 无窗函数（矩形窗），频谱泄漏严重 |
| 解码 | 无帧重叠，帧间不连续 |
| 解码 | swr_convert 输出缓冲 8192，超限截断；每帧只用前 1024 样点 |
| 解码 | 使用废弃 API `av_get_default_channel_layout` |
| MFCC | 缺少预加重滤波器 |
| MFCC | 缺少 liftering |
| MFCC | 缺少 delta / delta-delta |
| MFCC | DCT-II 无预计算，O(n²) |
| 工程 | 单文件，无模块拆分 |
| 工程 | 无 RAII 封装，资源泄漏风险 |
| 工程 | 无错误处理 |
| 工程 | 无日志、无配置、无测试、无 README |
| 服务器 | 临时文件固定覆盖 `analyzing_temp.mp3`，并发不安全 |
| 服务器 | 无请求大小限制，无健康检查端点 |
