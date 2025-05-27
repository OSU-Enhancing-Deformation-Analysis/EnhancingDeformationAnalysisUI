from ubuntu:latest

env DEBIAN_FRONTEND=noninteractive

run apt-get update \
 && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    wget \
    unzip \
    libopencv-dev \
    libglfw3-dev \
    libtiff-dev \
    libwayland-dev \
 && rm -rf /var/lib/apt/lists/*

# fetch tensorflow c api (cpu only)
run wget -q --no-check-certificate https://storage.googleapis.com/tensorflow/versions/2.18.0/libtensorflow-cpu-linux-x86_64.tar.gz
run tar -C /usr/local -xzf libtensorflow-cpu-linux-x86_64.tar.gz
run rm libtensorflow-cpu-linux-x86_64.tar.gz

# fetch libtorch c++ api (cpu only)
run wget -q --no-check-certificate -O libtorch.zip https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.7.0%2Bcpu.zip \
 && unzip libtorch.zip -d /opt \
 && rm libtorch.zip

# make lib paths available
workdir /src
