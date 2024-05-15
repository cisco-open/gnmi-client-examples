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
RUN apt-get update && apt-get install -y git vim doxygen autoconf automake libtool \
                       build-essential pkg-config curl make \
                       unzip python3 python3-pip python3-venv wget libssl-dev

# NEW AREA

# Install protocol buffer compiler https://grpc.io/docs/protoc-installation/
ARG PROTOC_VER=3.18.3
ARG PB_REL=https://github.com/protocolbuffers/protobuf/releases
RUN curl -LO ${PB_REL}/download/v${PROTOC_VER}/protoc-${PROTOC_VER}-linux-x86_64.zip
RUN unzip protoc-${PROTOC_VER}-linux-x86_64.zip -d /usr/local

# CPP grpc build and install
# reference https://grpc.io/docs/languages/cpp/quickstart
ARG MY_INSTALL_DIR=/usr/local
RUN mkdir -p ${MY_INSTALL_DIR}

ENV PATH="${PATH}:${MY_INSTALL_DIR}/bin"

#get cmake
RUN wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.21.7/cmake-3.21.7-Linux-x86_64.sh
RUN sh cmake-linux.sh -- --skip-license --prefix=${MY_INSTALL_DIR}

# Clone GRPC repo
RUN git clone --recurse-submodules -b v1.46.3 --depth 1 --shallow-submodules https://github.com/grpc/grpc
WORKDIR ${WS}/grpc

RUN mkdir -p cmake/build
WORKDIR ${WS}/grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${MY_INSTALL_DIR} ../..
RUN make -j
RUN make install
WORKDIR ${WS}

#Clone the JSON for Modern C++ 
RUN git clone --recurse-submodules -b v3.11.3 --depth 1 --shallow-submodules https://github.com/nlohmann/json.git
WORKDIR ${WS}/json

ARG MY_INSTALL_LIB=/usr/local/lib
RUN mkdir -p cmake/build
WORKDIR ${WS}/json/cmake/build
RUN cmake -DJSON_Install=ON -DJSON_BuildTests_INIT=OFF -DCMAKE_INSTALL_DATADIR=${MY_INSTALL_LIB}  ../..
RUN make -j
RUN make install
WORKDIR ${WS}
ENV PATH="${PATH}:."
