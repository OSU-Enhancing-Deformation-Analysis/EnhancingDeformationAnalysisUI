# EnhancingDeformationAnalysisUI

A comprehensive tool for material scientists and researchers to analyze SEM (Scanning Electron Microscope) imagery of materials undergoing deformation. This application combines computer vision techniques with AI-powered models to enhance image quality, detect structural changes, and quantify deformation metrics.

## Key Capabilities

- **Advanced Image Enhancement**: AI-powered denoising optimized for SEM imagery
- **Real-time Crack Detection**: Automated identification of cracks and structural defects
- **Deformation Quantification**: Track and measure material changes over time
- **Multi-format Analysis Export**: Generate reports in CSV for further analysis
- **Batch Processing**: Efficiently process large datasets with CLI support
- **GPU Acceleration**: Utilize CUDA for improved performance on supported hardware

## Installation

To clone this repo:
```bash
git clone --recursive git@github.com:OSU-Enhancing-Deformation-Analysis/EnhancingDeformationAnalysisUI
```
Don't miss the `--recursive` part of that command to get all required submodules!

If you missed that part, initialize the submodules with:
```bash
git submodule update --init
```

This program uses [tk_r_em](https://github.com/Ivanlh20/tk_r_em)'s models for denoising SEM images at various resolutions and microscopy types.

## Building

### Prerequisites
- CMake 3.5 or higher
- OpenCV ~4.0.0 or higher with the OpenCV_DIR environment variable set to the path of the OpenCVConfig.cmake file
- [Tensorflow C API](https://www.tensorflow.org/install/lang_c) installed on system path (findable by CMake)
- On Windows: CUDA 11 & cuDNN 8.x.x installed to PATH
- On Linux: CUDA 12.x & cuDNN 9.x.x installed to PATH, Zenity for file browsing operations

### Compilation

#### Linux (maybe macOS?)
```bash
cd EnhancingDeformationAnalysisUI && mkdir build
cd build && cmake ..
make -j4  # or however many cores you want to use
```

For CUDA support on Linux:
```bash
export XLA_FLAGS=--xla_gpu_cuda_data_dir=/usr/lib/cuda
```

#### Windows
1. Use the setup script `setup.py` to set up the project and install OpenCV and TensorFlow
2. Make sure to set the environment variables as instructed
3. Open the folder using Visual Studio (assuming Desktop C++ package installed)
4. Select the correct startup item from the dropdown: `EnhancingDeformationAnalysisUI.exe`
5. Use F5 to build and run

## Using the Application

### GUI Mode

Launch the application without any command-line arguments to use the GUI:

1. **Loading Images**: 
   - Click "Select Folder" to choose a directory containing TIFF images
   - Each image set opens in its own tab
   - Supports time-series SEM image sequences

2. **Preprocessing**:
   - Crop images to remove unneeded parts (scale bars, timestamps, etc.)
   - Stabilize image sequence to reduce movement and vibration artifacts
   - Remove/filter specific frames with artifacts or focus issues
   - Apply denoising using standard methods (blur) or specialized AI models optimized for:
     - High-resolution SEM (sfr_hrsem)
     - Low-resolution SEM (sfr_lrsem)
     - TEM and STEM variants at different resolutions
   - Detect cracks with adjustable parameters for sensitivity and minimum size

3. **Image Analysis**:
   - View histograms for each image to analyze intensity distributions
   - Compare images visually with synchronized navigation
   - Export analysis data as CSV for further processing
   - Generate statistics on image features, crack properties, and deformation metrics

4. **Export Options**:
   The Image Comparison tab provides comprehensive export functionality:
   
   **Single Frame Export**:
   - Export current frame from original sequence as TIFF
   - Export current frame from processed sequence as TIFF
   
   **Batch Export**:
   - Export all original frames as individual TIFF files
   - Export all processed frames as individual TIFF files
   - Export processed sequence as animated GIF (with configurable playback speed)
   
   **Analysis Data Export**:
   - Export histogram and SNR analysis data as CSV files
   - Export crack width measurements and feature tracking data as CSV
   - Export strain analysis results for quantitative research

5. **Feature Tracking**:
   - Track points manually by selecting features of interest
   - Track automatically using detected cracks or structural features
   - Measure deformation over time with displacement vectors
   - Calculate strain fields between image frames

6. **Strain Analysis**:
   - Generate strain maps to visualize deformation patterns
   - Calculate local and global strain metrics
   - Export strain data for quantitative analysis
   - Compare strain evolution across multiple time points

### Command Line Interface

For batch processing or integration into automated workflows, use the CLI mode:

```bash
./EnhancingDeformationAnalysisUI --folder <path> [--crop <pixels>] [--denoise <blur/sfr_hrsem/...>] [--analyze <output.csv>] [--calculate-widths <widths.csv>] [--output <path>]
```

Options:
- `--folder <path>`: Directory containing TIFF images (required)
- `--crop <pixels>`: Remove specified number of pixels from image bottom
- `--denoise <filter>`: Apply denoising filter (options: blur, sfr_hrsem, sfr_lrsem, etc.)
- `--analyze <output.csv>`: Generate image analysis statistics
- `--calculate-widths <widths.csv>`: Calculate and export crack width measurements
- `--output <path>`: Save processed images to specified directory

## Technical Architecture

The application is built with a modular architecture:

- **Core Analysis Engine**: C++ classes for image processing and deformation analysis
- **TensorFlow Integration**: Uses cppflow to interface with TensorFlow for AI model inference
- **UI Layer**: ImGui-based interface for interactive visualization and parameter adjustment
- **Threading Support**: Background processing for responsive UI during heavy computations
- **File I/O**: Support for TIFF formats commonly used in microscopy

## Developer Architecture Guide

### Project Structure

```
src/
├── core/                           # Core processing algorithms
│   ├── DeformationAnalysisInterface.*  # Main analysis orchestration
│   ├── ImageAnalysis.*                 # Image processing utilities
│   ├── CrackDetector.*                 # Crack detection algorithms
│   ├── FeatureTracker.*                # Point tracking and strain calculation
│   ├── Stabilizer.*                    # Image stabilization
│   ├── DenoiseInterface.*              # AI model integration
│   ├── Tiler.*                         # Image tiling for large datasets
│   └── ThreadPool.*                    # Asynchronous processing
├── ui/                             # User interface components
│   ├── Application.*                   # Main UI application class
│   ├── ImageSet.*                      # Image sequence management
│   └── PreprocessingTab.*              # Preprocessing controls
├── OpenGL/                         # Graphics rendering
│   └── Texture.*                       # OpenGL texture management
├── main.cpp                        # Application entry point
├── cli.cpp/.hpp                    # Command-line interface
└── utils.*                         # Shared utilities
```

### Key Components

#### Core Processing Pipeline
- **DeformationAnalysisInterface**: Interface for the models we trained
- **ImageAnalysis**: Handles image analysis, such as histograms of each frame or specific regions
- **CrackDetector**: Implements crack detection using morphological operations and contour analysis
- **FeatureTracker**: Handles optical flow-based feature tracking and crack width calculations
- **Stabilizer**: Image stabilization using feature matching

#### AI Integration
- **DenoiseInterface**: Wrapper around TensorFlow models via cppflow
    - **Model Location**: `/assets/models/tk_r_em/` contains pre-trained models (from tk_r_em) for different SEM types
    - **Supported Models**: sfr_hrsem, sfr_lrsem, sfr_hrtem, sfr_lrtem, sfr_hrstem, sfr_lrstem
- **StrainAnalysis**: Models trained by our capstone group to predict displacement in a sequence of images
    - **Model Location**: `/assets/models/` contains the model `batch-m4-combo`

#### UI Architecture
- **ImGui-based**: Immediate mode GUI for responsive real-time interaction
- **Tab System**: Each image set gets its own tab with independent state
- **OpenGL Backend**: Hardware-accelerated image display with zoom/pan capabilities
- **Threading**: UI runs on main thread, heavy processing on worker threads via ThreadPool

### Build System

#### Dependencies
- **CMake**: Build system configuration in `CMakeLists.txt`
- **Submodules**: External libraries in `libs/` (cppflow, imgui, glfw, etc.)
- **External**: OpenCV (system), TensorFlow C API (system)

#### Key Libraries
```
libs/
├── cppflow/        # TensorFlow C++ wrapper
├── imgui/          # Immediate mode GUI
├── glfw/           # Window/input management
├── glad/           # OpenGL loader
├── gif-h/          # GIF export functionality
└── libtiff/        # TIFF file I/O
```

### Data Flow

1. **Image Loading**: `ImageSet` loads TIFF sequences, manages metadata
2. **Preprocessing**: User applies crop, stabilization, denoising via UI controls
3. **Analysis**: `DeformationAnalysisInterface` coordinates feature tracking, crack detection
4. **Visualization**: Results displayed in real-time via OpenGL textures
5. **Export**: CSV data export, processed image sequences, GIF animations

### Threading Model

- **Main Thread**: UI rendering, user interaction
- **Worker Threads**: Image processing, AI inference, file I/O
- **ThreadPool**: Manages background tasks with configurable thread count
- **Synchronization**: Results passed back to main thread via callbacks

### Memory Management

- **Image Storage**: Images stored on GPU memory
- **GPU Memory**: TensorFlow/PyTorch models run on GPU when available (CUDA)
- **Caching**: Processed results cached to avoid recomputation
- **Streaming**: Large datasets processed in tiles to manage memory usage

### Configuration

- **Models**: Model selection and parameters in UI
- **Build Options**: CMake flags for CUDA, profiling, debug builds
- **Runtime**: No external config files - all settings in UI state

### Extension Points

To add new functionality:
- **Placeholder**:

### Testing & Debugging

- **Debug Builds**: CMake debug configuration available
- **Profiling**: UI_PROFILE=ON CMake option for performance analysis
- **Logging**: Console output for debugging, no formal logging framework

### Deployment

- **Docker**: Dockerfile for CI/CD on GitHub
- **Windows**: Visual Studio project files, setup.py for dependencies
- **Linux**: Standard CMake build process, pull libraries yourself

## Common Use Cases

### Material Testing Analysis
Process image sequences from material testing experiments to quantify:
- Crack initiation and propagation 
- Strain localization patterns
- Material failure mechanisms

### Nanomaterial Characterization
Analyze deformation behavior in:
- Nanocomposites
- Thin films
- Advanced ceramic materials

### Quality Control
Identify defects and anomalies in materials:
- Detect manufacturing flaws
- Assess material degradation
- Verify material performance under stress

## Troubleshooting

### Common Issues

- **Image Loading Failures**: Ensure TIFF files are standard format without compression artifacts
- **GPU Acceleration Issues**: 
  - Verify CUDA and cuDNN versions match requirements
  - On Linux, make sure XLA_FLAGS is set correctly
- **Slow Performance**: 
  - Enable profiling with `UI_PROFILE=ON` in CMake to identify bottlenecks
  - Consider using smaller image subsets for initial analysis

### Error Reporting

For bugs or feature requests, please submit an issue on the GitHub repository with:
- Description of the problem
- Steps to reproduce
- Sample images (if possible)

## Credits
- [tk_r_em](https://github.com/Ivanlh20/tk_r_em) for the AI models used for denoising
