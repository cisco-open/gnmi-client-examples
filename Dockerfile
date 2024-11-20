ARG UBUNTU_VER=18.04
FROM ubuntu:${UBUNTU_VER}
# Here we define UBUNTU_VER again, so it can be used after FROM.
ARG UBUNTU_VER

# Set the working directory in the container
ARG WS=/app
WORKDIR ${WS}

# Copy the current directory contents into the container at /app
COPY . .

RUN ln -snf /usr/share/zoneinfo/$CONTAINER_TIMEZONE /etc/localtime && echo $CONTAINER_TIMEZONE > /etc/timezone
# Install necessary dependencies
RUN apt-get update && apt-get install -y\
    # Required by gRPC
    build-essential autoconf libtool pkg-config\
    git automake make unzip\
    python3 python3-pip python3-venv wget libssl-dev \
    doxygen graphviz libboost-all-dev

# Install Sphinx and Breathe
RUN pip3 install --upgrade sphinx breathe sphinx-rtd-theme myst-parser

# CPP grpc build and install
# reference https://grpc.io/docs/languages/cpp/quickstart
ARG MY_INSTALL_DIR=/usr/local
RUN mkdir -p ${MY_INSTALL_DIR}

ENV PATH="${PATH}:${MY_INSTALL_DIR}/bin"

#get cmake
RUN wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-x86_64.sh
RUN sh cmake-linux.sh -- --skip-license --prefix=${MY_INSTALL_DIR}

ENV PATH="${PATH}:."
