# Contributing

## Dev Environment Setup

### Linux

Install system dependencies, then download PyTorch and optionally TensorFlow:

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake git libopencv-dev libjpeg-dev libpng-dev \
  libwayland-dev xorg-dev libxkbcommon-dev wayland-protocols extra-cmake-modules

# Download libtorch (CPU)
wget -qO- https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.7.0%2Bcpu.zip | busybox unzip - -d /opt

# Build
cmake -S . -B build \
  -DTorch_DIR=/opt/libtorch/share/cmake/Torch
cmake --build build -j$(nproc)
```

### macOS (Apple Silicon)

```bash
brew install opencv cmake

# Download libtorch (ARM64)
curl -L -o libtorch.zip https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.7.0.zip
sudo unzip libtorch.zip -d /opt && rm libtorch.zip

cmake -S . -B build \
  -DTorch_DIR=/opt/libtorch/share/cmake/Torch
cmake --build build -j$(sysctl -n hw.ncpu)
```

PyTorch will automatically use the Metal Performance Shaders (MPS) backend on Apple Silicon when available.

### Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the C++ workload.
2. Download [OpenCV Windows release](https://github.com/opencv/opencv/releases) and extract it.
3. Download [libtorch](https://pytorch.org/get-started/locally/) (CPU or CUDA) and extract it.
4. Either open the folder in Visual Studio (CMake auto-configures), or:

```pwsh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DOpenCV_DIR=C:/opencv/opencv/build `
  -DTorch_DIR=C:/pytorch/libtorch/share/cmake/Torch
cmake --build build
```

### Enabling TensorFlow

TensorFlow is off by default. To enable it:

1. Download the [TensorFlow C API](https://www.tensorflow.org/install/lang_c) for your platform.
2. Add these flags to your cmake configure:

```
-DEDA_WITH_TENSORFLOW=ON
-DCMAKE_MODULE_PATH=<repo>/libs/cppflow/cmake/modules
-Dtensorflow_INCLUDE_DIRS=<tf_install>/include
-Dtensorflow_LIBRARIES=<tf_install>/lib/libtensorflow.so   # or tensorflow.lib on Windows
```

Note: cppflow's `Findtensorflow.cmake` module requires `CMAKE_MODULE_PATH` to be set explicitly because it uses `CMAKE_SOURCE_DIR` instead of `CMAKE_CURRENT_SOURCE_DIR` internally.

### CUDA / GPU

For GPU-accelerated PyTorch inference, download the CUDA variant of libtorch instead of the CPU variant. CUDA 11.x/cuDNN 8.x on Windows, CUDA 12.x/cuDNN 9.x on Linux. On macOS, GPU acceleration uses MPS automatically.

## Architecture

### Module Overview

```
core/              Static library - all processing logic, no UI dependencies
apps/gui/          ImGui + OpenGL desktop application
apps/cli/          Command-line batch processor
libs/              Vendored dependencies (GLFW, ImGui, libtiff, cppflow, glad, gif-h, stb_image)
assets/models/     Pre-trained AI models (PyTorch .pt and TensorFlow SavedModel)
```

### Core Library (`core/`)

The core is a static library linked by both the GUI and CLI. All core classes use **static methods and static state** (no instantiation). Key interfaces:

| Class | Purpose |
|-------|---------|
| `DeformationAnalysisInterface` | PyTorch inference for strain/deformation on consecutive frame pairs |
| `DenoiseInterface` | TensorFlow-based denoising (SEM/TEM/STEM models) and Gaussian blur fallback |
| `CrackDetector` | Morphological crack detection with connected component analysis |
| `FeatureTracker` | Lucas-Kanade optical flow tracking and crack width profiling |
| `Stabilizer` | Feature-matching affine stabilization across frames |
| `ImageAnalysis` | Histograms, SNR, and region-of-interest statistics |
| `Tiler` | Splits images into 256x256 tiles for GPU memory management |
| `ThreadPool` | Singleton worker pool for async dispatch |

### Data Flow

```
Load TIFF folder -> uint32_t* BGRA buffers
  -> Stabilize (affine alignment to reference frame)
  -> Denoise (TensorFlow, tiled)
  -> Crack Detection (morphological)
  -> Deformation Analysis (PyTorch, tiled, consecutive pairs)
  -> Feature Tracking (optical flow)
  -> Export (TIFF / GIF / CSV)
```

Images are stored as `uint32_t*` BGRA buffers throughout. Core functions wrap these in `cv::Mat` for processing and write results back via `memcpy`.

### Threading Model

All core interfaces provide synchronous and `*Async()` variants. The async pattern:

1. Async method sets `m_processing = true`, `m_progress = 0`
2. Dispatches work to the singleton `ThreadPool`
3. Returns a `std::future<bool>` immediately
4. The GUI polls futures with `wait_for(0s)` each frame (non-blocking)
5. Progress is read from static members (e.g. `DenoiseInterface::m_progress`)

The `ThreadPool` is initialized once with `hardware_concurrency` threads (minimum 2).

### Tile-Based GPU Processing

AI models expect fixed 256x256 inputs. The `Tiler` class splits large images into tiles with two strategies:

- **Cropped** — non-overlapping tiles with border padding
- **Blended** — overlapping tiles blended on reassembly

`Tiler::CreateTiles()` splits, the model processes each tile, and `Tiler::StitchTiles()` reconstructs the full image.

### GUI (`apps/gui/`)

- `Application` — GLFW window, OpenGL context, ImGui setup, main render loop
- `ImageSet` — one per loaded folder; manages textures and tabs (comparison, preprocessing, deformation, tracking, analysis)
- `PreprocessingTab` — UI for stabilization, denoising, crack detection; dispatches async core calls
- `Texture` — thin OpenGL texture wrapper used by ImGui
- `utils` — platform-specific file dialogs (Windows COM, Linux Zenity, macOS Cocoa), TIFF/GIF/CSV I/O

### CLI (`apps/cli/`)

Parses subcommands (`crop`, `denoise`, `analyze`, `widths`, `motion`) into a pipeline executed in user-specified order. Minimal dependencies — links only against `deformation_core` and `libtiff`.

## CI/CD

### Workflows

| Workflow | Trigger | What it does |
|----------|---------|--------------|
| `ci.yml` | PRs to `main`, tag pushes | Builds on Windows, Linux, macOS; creates GitHub releases on tags |
| `build-linux-image.yml` | Dockerfile changes on `main`, manual dispatch | Builds and pushes the Linux CI Docker image to GHCR |

### How It Works

- **Windows**: Downloads pre-built OpenCV, libtorch, and TensorFlow C API (cached between runs). Managed by the reusable composite action at `.github/actions/setup-windows-deps/action.yml`.
- **Linux**: Pulls a Docker image from GHCR with all deps pre-installed. Dockerfile lives in the repo root.
- **macOS**: Installs OpenCV via Homebrew, downloads libtorch (cached). No TensorFlow on macOS currently.
- **Releases**: On tag push (`v*.*.*`), the Windows build packages the exe + assets and the release job publishes to GitHub Releases.

### Updating Dependency Versions

**Windows deps** (OpenCV, TensorFlow, PyTorch): edit the `default` values in `.github/actions/setup-windows-deps/action.yml`. One file, all three versions.

**Linux deps**: edit the `Dockerfile`, push to `main`. The image rebuild workflow triggers automatically.

**macOS deps**: edit the libtorch version/URL in the `build-macos` job in `ci.yml`.

## Conventions

- **Image buffers**: `uint32_t*` BGRA, 4 bytes per pixel. All core APIs use this format.
- **Async API**: every long-running operation has a synchronous version and an `*Async()` variant returning `std::future<bool>`.
- **Progress**: static `m_progress` (float, 0-1) and `m_processing` (bool) on each interface class.
- **Platform code**: gated by `#ifdef __APPLE__`, `#ifdef _WIN32`, etc. macOS Objective-C++ lives in `utils.mm`.
- **Feature flags**: `EDA_WITH_PYTORCH`, `EDA_WITH_TENSORFLOW` — code behind these compiles conditionally via `#ifdef`.
