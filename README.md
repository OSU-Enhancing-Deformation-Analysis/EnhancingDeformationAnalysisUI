# Enhancing Deformation Analysis UI

A C++ desktop application for inspecting deformation in sequences of microscopy images (SEM/TEM/STEM). Combines classical image processing, ML-based denoising and strain analysis in a single package.

## Features

- Image enhancement and AI denoising
- Crack detection with adjustable sensitivity
- Feature tracking, strain maps and deformation metrics
- Histogram and statistics overlays
- Export to CSV, TIFF, or GIF
- CLI for batch processing

## Building

### Prerequisites

- CMake >= 3.20
- OpenCV >= 4.0
- PyTorch C++ API (libtorch)
- TensorFlow C API (optional)
- CUDA/cuDNN for GPU acceleration (optional)

### Clone

```bash
git clone --recursive https://github.com/OSU-Enhancing-Deformation-Analysis/EnhancingDeformationAnalysisUI.git
cd EnhancingDeformationAnalysisUI
```

If you forgot `--recursive`: `git submodule update --init`

### Linux

```bash
cmake -S . -B build -DTorch_DIR=/path/to/libtorch/share/cmake/Torch
cmake --build build -j$(nproc)
```

### macOS

```bash
brew install opencv
cmake -S . -B build -DTorch_DIR=/path/to/libtorch/share/cmake/Torch
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Windows

Install OpenCV, libtorch, and optionally the TensorFlow C API. Then either:

- Open the project folder in Visual Studio 2022 and let CMake auto-configure, or:

```pwsh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DOpenCV_DIR=path/to/opencv/build `
  -DTorch_DIR=path/to/libtorch/share/cmake/Torch
cmake --build build
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `EDA_WITH_PYTORCH` | `ON` | PyTorch backend |
| `EDA_WITH_TENSORFLOW` | `OFF` | TensorFlow/cppflow backend |
| `EDA_BUILD_GUI` | `ON` | Build GUI application |
| `EDA_BUILD_CLI` | `ON` | Build CLI application |

## Usage

### GUI

Load a folder of TIFF images, then crop, stabilise, denoise, detect cracks, view deformation maps, and export results.

### CLI

```bash
./eda-cli \
  --folder <image_dir> \
  [--crop <pixels>] \
  [--denoise <method>] \
  [--analyze] \
  [--calculate-widths] \
  [--output <output_dir>]
```

## Contributing

Open issues with detailed descriptions and sample images if possible. Contributions are welcome.
