<a id="readme-top"></a>

# gNMI Client Examples

[![Release][Release-url]](CHANGELOG.md)
[![Lint][Lint-url]][Lint-status]
[![Contributor Covenant][Contributor-svg]](CODE_OF_CONDUCT.md)
[![Maintainer][Maintainer-svg]](MAINTAINERS.md)

## Table of Contents

1. [About The Project](#about-the-project)
   - [Built With](#built-with)
2. [Getting Started](#getting-started)
   - [Prerequisites](#prerequisites)
   - [Installation](#installation)
   - [Build from source](#build-from-source)
3. [Usage](#usage)
4. [Roadmap](#roadmap)
5. [Contributing](#contributing)
6. [License](#license)
7. [Contact](#contact)
8. [Acknowledgments](#acknowledgements)

## About The Project

This repo provides a library that shows how to use a subscribe request through gRPC Network Management Interface (gNMI) in order to retrieve PBR counter statistics with Cisco XR models. This project will expand to other data in future releases. Support for C++ is currently provided. Potential for other language support in future releases.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Built with

- [CMake][Cmake-url]
- [Clang][Clang-url]
- [Docker][Docker-url]
- [gTest][gtest-url]

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Getting Started

### Prerequisites

These are all provided/handled for ease of use:

User can find the specification for gNMI here:\
<https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md>

User can find information regarding open-config here:\
<https://github.com/openconfig/public/tree/master>

User can find the protos used here (Updated protos on April 22 2024):
<https://github.com/openconfig/gnmi/tree/master/proto>
The commands used to get the protos were:
git clone <> followed by:
git checkout SHA:

```sh
git clone https://github.com/openconfig/gnmi.git
git checkout 5588964b559c9afee319909dd022b6706fe4a162
```

User might need to add more protos in the future as dependencies for these proto files can change.
The gnmi.proto file has one change made on it:\
`import "github.com/openconfig/gnmi/proto/gnmi_ext/gnmi_ext.proto"; -> import "gnmi_ext.proto";`

>[!NOTE] We use docker containers, which handles the installation of the required packages and libraries. If the user wants to run this in their own environment, then please refer to the [Dockerfile](gnmi_client/Dockerfile) for the necessary installation packages.

### Installation

1. Download last release

   ```sh
   curl -LO https://github.com/cisco-open/gnmi-client-examples/<latest_release>
   ```

2. Include `mgbl_api` into your project.

### Build from source

1. Clone the repo.

   ```sh
   git clone https://github.com/cisco-open/gnmi-client-examples/releases/tag/<latest_release>
   ```

2. Build Docker environment.

   ```sh
   make build
   ```

3. To go into docker environment.

   ```sh
   make sdk-bash
   ```

4. To build the library. The first build could take some time building the dependencies.\

> By default Cmake will not build the tests
> To tell Cmake to build extra targets, do
> `cmake -DENABLE_UNIT_TESTS=ON -DENABLE_FUNC_TESTS=ON -DENABLE_SPHINX_DOC=ON ..`

   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```

5. To install the lib

   ```sh
   make install
   ```

6. To build the examples.

   ```sh
    make examples
   ```

7. To run unit tests.

   ```sh
   make unit_tests
   ```

7. To create the documentation.\
`build/subprojects/Build/documentation/sphinx/index.html` or in your local directory `/usr/local/docs/sphinx/`

   ```sh
   make documentation
   ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Usage
> [!NOTE] It is highly recommened to view the documentation to understand how to use this library. Below is a brief overview.

### Basic gRPC Overview

This library uses gRPC's implementation of a **Subscribe RPC**. gRPC supports four types of RPC, but this library focuses on:

- **Bidirectional Streaming RPC**:
  Both the client and server can send and receive a stream of messages.

#### gRPC and Protocol Buffers

To understand how **Subscribe RPCs** work, it's important to know about **Protocol Buffers**. This is a language-agnostic, binary serialization format by Google, used to define data structures and services for gRPC. These are described in `.proto` files.

In gRPC, services and their methods are defined in these `.proto` files. For this library, the key focus is **bidirectional streaming**—where multiple requests and responses are exchanged as a stream over a single connection.

The `.proto` files and their generated counterparts can be found [here](mgbl_api/third-party/gnmi). The library uses RPCs and messages defined by gNMI. More details on the proto files are available in the [Prerequisites section](#prerequisites).

#### gRPC Subscription Flow

gRPC abstracts communication channels and streams into a unified concept, allowing you to define a service and its methods, specifying the parameters and return types.

Here’s a general workflow for subscription-based streaming with gRPC:

1. **Create a `grpc::Channel`**:
   This represents the connection to the server.

2. **Create a Stub**:
   The stub is a client generated from the proto definitions. It uses the `grpc::Channel` to perform RPCs.

3. **Create a `grpc::ClientContext`**:
   This allows you to attach metadata and configure settings for the RPC call. Its lifetime should match the stub's.

4. **Use `ClientReaderWriter` for Streaming**:
   For bidirectional streaming, gRPC provides the `ClientReaderWriter` API. This API allows you to send requests and receive responses asynchronously via the stub.

5. **Send Requests and Wait for Responses**:
   Once all requests are sent, signal the server that you are finished. The server will respond and, when complete, signal the client to stop listening for further responses. The process ends with receiving a `grpc::StatusCode`.

#### Library-Specific Streaming Subscription Implementation

> [!NOTE] Example usage can be found in the [examples](examples/) directory.

1. **Initialize a `gnmi_client_connection` Object**:
   This class manages the gRPC connection setup using the `channel_arguments` struct. It internally constructs the `grpc::Channel`. Typically, gRPC establishes the connection when the RPC is made, but the library provides a `wait_for_grpc_server_connection` method to establish the connection prior to making an RPC call.

2. **Create a `gnmi_stream_imp` Object**:
   Pass the `grpc::Channel` from the previous step into this object. The library handles the stub creation and streaming logic (`ClientReaderWriter`) for you, abstracting most of the gRPC-specific details.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Major Functions for Gathering PBR Stats

The following functions allow you to collect PBR (Policy-Based Routing) stats. The statistics provided are:

- `byte-count`
- `packet-count`
- `collection_timestamp_seconds`
- `collection_timestamp_nanoseconds`
- `path_grp_name`
- `policy_action_type`

### 1. `rpc_register_stats_once`

```cpp
std::pair<error_code, grpc::Status> GnmiClient::rpc_register_stats_once(
    const client_context_args& context_args, rpc_args& rpc_args)
```

This is a blocking call that subscribes to PBR stats in "ONCE" mode. It retrieves all responses, parses them, and populates the counter structure for each `pbr_key`. Each `pbr_key` is a combination of `key_policy` and `key_rule`. After all responses are received or an rpc failure occurs, it returns a pair containing an `error_code` and a `grpc::Status`. Can be called multiple times one after another.

- **Parameters:**
  - `context_args`: Configuration settings for the stream.
  - `rpc_args`: Parameters for the subscription.

Using `"*"` for `key_policy` and `key_rule` will retrieve stats for all policy-rule combinations.

### 2. `rpc_register_stats_stream`

```cpp
/**
 * @brief Makes a stream request to the server.
 * @param context_args Configuration for the client context.
 * @param rpc_args Configuration for the RPC call.
 * @return Error code indicating success or failure.
 */
error_code GnmiClient::rpc_register_stats_stream(const client_context_args& context_args,
                                                 rpc_args& rpc_args)
```

This is a non-blocking call that subscribes in "STREAM" mode. It sends the request and creates a receive thread that continuously waits for responses until the user cancels the RPC or an error occurs.

- **Parameters:**
  - `context_args`: Configuration for the stream. Needs to exist for the lifetime of the rpc.
  - `rpc_args`: Parameters for the subscription.

For each response received, the user-defined `_rpc_success_handler` will be executed in the receive thread, receiving a `GnmiCounters` reference as an argument. This handler must process responses for each `key_policy` and `key_rule` combination, similar to the `rpc_register_stats_once` function.

If the RPC fails, the user-defined `_rpc_failed_handler` will be called in the receive thread, passing a `grpc::Status` to handle the failure as needed.

**Note:** If multiple requests are sent, the same receive thread will handle all responses. After calling this function, always call `stream_pbr_close` to cancel the RPC and join the thread. If the receive thread exists when sending multiple requests, it will keep using the original context_args. If the user wants to use different context_args, either create a new instance of the `GnmiClient`
and do the request there, or call `stream_pbr_close` and then use this register function again.

### 3. `rpc_stream_close`

```cpp
error_code rpc_stream_close();
```

This is a blocking call that tries to cancel the RPC and waits to join the receive thread. **Always call this after `rpc_register_stats_stream`.**

> For more information please visit the [official documentation](build/subprojects/Build/documentation/sphinx/index.html) and the given [examples](examples/).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Roadmap

See the [open issues](https://github.com/cisco-open/gnmi-client-examples/issues) for a list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions Users make are **greatly appreciated**. For detailed contributing guidelines, please see [CONTRIBUTING.md](CONTRIBUTING.md)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## License

Distributed under the `Apache License, Version 2.0` License. See [LICENSE](LICENSE) for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Contact

Project Link: [https://github.com/cisco-open/gnmi-client-examples](https://github.com/cisco-open/gnmi-client-examples)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Acknowledgements

This template was adapted from
[https://github.com/othneildrew/Best-README-Template](https://github.com/othneildrew/Best-README-Template).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

[Release-url]: https://img.shields.io/github/v/release/cisco-ospo/oss-template?display_name=tag
[Lint-url]: https://github.com/cisco-ospo/oss-template/actions/workflows/lint.yml/badge.svg?branch=main
[Lint-status]: https://github.com/cisco-open/gnmi-client-examples/actions/workflows/lint.yml
[contributor-svg]: https://img.shields.io/badge/Contributor%20Covenant-2.1-fbab2c.svg
[Maintainer-svg]: https://img.shields.io/badge/Maintainer-Cisco-00bceb.svg

[Cmake-url]: https://cmake.org/
[Clang-url]: https://clang.llvm.org/
[Docker-url]: https://clang.llvm.org/
[gtest-url]: https://google.github.io/googletest/
