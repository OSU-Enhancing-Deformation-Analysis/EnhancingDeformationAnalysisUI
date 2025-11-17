
# -------------------------------
# CUDA environment variables
# -------------------------------
export CUDA_HOME=/usr/local/apps/cuda/11.8
export PATH=$CUDA_HOME/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$CUDA_HOME/extras/CUPTI/lib64:$LD_LIBRARY_PATH

# -------------------------------
# OpenCV environment variables
# -------------------------------
export OpenCV_DIR=$HOME/opt/opencv-4.10.0/build
export CPATH=$HOME/opt/opencv-4.10.0/include/opencv4:$CPATH
export LD_LIBRARY_PATH=$HOME/opt/opencv-4.10.0/lib64:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$HOME/opt/opencv-4.10.0/lib64/pkgconfig:$PKG_CONFIG_PATH

# -------------------------------
# LibTorch (GPU) environment variables
# -------------------------------
export Torch_DIR=$HOME/libtorch_gpu/share/cmake/Torch
export LD_LIBRARY_PATH=$HOME/libtorch_gpu/lib:$LD_LIBRARY_PATH

# -------------------------------
# Confirm success
# -------------------------------
echo "Environment ready!"
echo "  - CUDA_HOME: $CUDA_HOME"
echo "  - OpenCV_DIR: $OpenCV_DIR"
echo "  - Torch_DIR: $Torch_DIR"
echo "  - GCC: $(gcc --version | head -n 1)"
echo "  - nvcc: $(nvcc --version | tail -n 1)"


# The cmake execution to get the cmake to run for this project under HPC.
# cmake .. \
#  -DCMAKE_BUILD_TYPE=Release \
#  -DCMAKE_CXX_STANDARD=17 \
#  -DCMAKE_CUDA_STANDARD=17 \
#  -DCMAKE_PREFIX_PATH="$HOME/libtorch_gpu/share/cmake/Torch;$HOME/opt/opencv-4.10.0/build" \
#  -DOpenCV_DIR=$HOME/opt/opencv-4.10.0/build \
#  -DEDA_WITH_PYTORCH=ON \
#  -DEDA_BUILD_GUI=ON \
#  -DEDA_BUILD_CLI=ON \
#  -DEDA_USE_VENDORED_DEPS=ON
