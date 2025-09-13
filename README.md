# Enhancing Deformation Analysis UI

Enhancing Deformation Analysis UI is a C++ application for inspecting deformation in sequences of microscopy images. The tool is aimed at material scientists working with scanning electron microscopy (SEM), transmission electron microscopy (TEM) and scanning transmission electron microscopy (STEM). It combines classical image processing, machine-learning denoising and strain analysis in a single package.

## Features

- **Image enhancement** – denoising using built-in filters or AI models.
- **Crack detection** – automatic detection with adjustable sensitivity and minimum crack size.
- **Deformation quantification** – feature tracking to generate strain maps and metrics.
- **Histograms and statistics** – intensity distributions and frame/sequence stats.
- **Export** – save results to CSV, TIFF, or GIF; batch export supported.
- **Batch processing** – command-line interface for large datasets.

## Quick Start

### Cloning

```bash
git clone --recursive https://github.com/OSU-Enhancing-Deformation-Analysis/EnhancingDeformationAnalysisUI.git
```

If you forgot `--recursive`:

```bash
git submodule update --init
```

### Dependencies

- CMake ≥ 3.5  
- OpenCV ≥ 4.0.0 (`OpenCV_DIR` must point to `OpenCVConfig.cmake`)  
- TensorFlow C API  
- CUDA/cuDNN for GPU acceleration  
  - Windows: CUDA 11.x, cuDNN 8.x  
  - Linux: CUDA 12.x, cuDNN 9.x  

### Building

**Linux**

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Set this for CUDA:

```bash
export XLA_FLAGS="--xla_gpu_cuda_data_dir=/usr/lib/cuda"
```

**Windows**

THIS WILL BE IMPROVED SOON, setup.py DOESN'T INSTALL DEPENDENCIES CORRECTLY
1. Run `setup.py` in the repo root.  
2. Open the solution in Visual Studio.  
3. Set `eda-gui.exe` as startup project and press F5.  

## Usage

### Graphical Interface

- Load image folders (TIFF sequences).  
- Crop, stabilise, denoise, detect cracks.  
- View histograms, deformation maps, measurements.  
- Export frames, GIFs, CSV summaries.  
- Compare original vs processed frames.  

### Command Line

```bash
./eda-cli \
  --folder <image_dir> \
  [--crop <pixels>] \
  [--denoise <method>] \
  [--analyze] \
  [--calculate-widths] \
  [--output <output_dir>]
```

## Project Structure

```
core/
├── include/deformation_core/      # Public API headers for the core library
├── src/                           # Implementation of core algorithms
│   ├── DeformationAnalysisInterface.*  # Analysis pipeline orchestrator
│   ├── DenoiseInterface.*             # TensorFlow model integration
│   ├── CrackDetector.*                # Crack detection algorithms
│   ├── FeatureTracker.*               # Feature tracking and strain computation
│   ├── Stabilizer.*                   # Image stabilisation routines
│   ├── ThreadPool.*                   # Asynchronous processing support
│   └── …                              # Other core modules
└── CMakeLists.txt                    # Builds the `deformation_core` static library

apps/
├── gui/                              # Graphical user interface application
│   ├── src/
│   │   ├── main.cpp                 # GUI entry point
│   │   ├── Application.*            # Manages windowing, rendering and UI
│   │   └── …                        # Additional GUI components
│   └── CMakeLists.txt               # Builds the `eda-gui` executable, links against `deformation_core`
└── cli/                              # Command‑line interface application
    ├── src/
    │   ├── main_cli.cpp             # CLI entry point
    │   ├── cli.cpp                  # CLI command handling
    │   ├── utils.cpp                # CLI helper functions
    │   └── …                        # Other CLI components
    └── CMakeLists.txt               # Builds the `eda-cli` executable, links against `deformation_core`

libs/
├── cppflow/                         # TensorFlow C++ wrapper (submodule)
├── imgui/                           # ImGui GUI library (submodule)
├── glfw/                            # Window and input management
├── glad/                            # OpenGL loader
├── gif-h/                           # GIF export functionality
├── libtiff/                         # TIFF I/O
├── stb_image/                       # Image loading utilities
└── …                                # Other vendored dependencies

assets/
└── models/                          # Pre‑trained AI models (SEM/TEM/STEM and strain)

CMakeLists.txt                        # Top‑level build configuration
```

## Architecture

- **Core Processing**: `DeformationAnalysisInterface` coordinates preprocessing and analysis; `ImageAnalysis` generates metrics.  
- **AI Integration**: `DenoiseInterface` wraps TensorFlow models (SEM/TEM/STEM). Runs on GPU if available.  
- **User Interface**: Dear ImGui + OpenGL, multi-tabbed, threaded workers keep UI responsive.  
- **Threading/Data Flow**: Tile-based GPU processing; main thread handles rendering; workers handle compute and I/O.  

## Use Cases

- Crack initiation and propagation studies.  
- Strain localisation visualisation.  
- Failure mechanism analysis.  
- Batch processing of microscopy datasets.  

## Troubleshooting

- Ensure TIFFs are standards-compliant.  
- Verify CUDA/cuDNN versions match requirements.  
- On Linux, set `XLA_FLAGS` if GPU not detected.  
- Enable `UI_PROFILE=ON` or process subsets for performance.  

## Contributing

Open issues with detailed descriptions and sample images if possible. Contributions are welcome.
