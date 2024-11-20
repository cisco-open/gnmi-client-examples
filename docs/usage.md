# Usage

## Basic gRPC Overview

This library uses gRPC's implementation of a **Subscribe RPC**. gRPC supports four types of RPC, but this library focuses on:

- **Bidirectional Streaming RPC**:
  Both the client and server can send and receive a stream of messages.

### gRPC and Protocol Buffers

To understand how **Subscribe RPCs** work, it's important to know about **Protocol Buffers**. This is a language-agnostic, binary serialization format by Google, used to define data structures and services for gRPC. These are described in `.proto` files.

In gRPC, services and their methods are defined in these `.proto` files. For this library, the key focus is **bidirectional streaming**—where multiple requests and responses are exchanged as a stream over a single connection.

The `.proto` files and their generated counterparts can be found at **third-party/gnmi**. The library uses RPCs and messages defined by gNMI. More details on the proto files are available in the [Prerequisites section](#prerequisites).

### gRPC Subscription Flow

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

### Library-Specific Streaming Subscription Implementation

> [!NOTE] Example usage can be found in the **examples** directory.

1. **Initialize a `gnmi_client_connection` Object**:
   This class manages the gRPC connection setup using the `channel_arguments` struct. It internally constructs the `grpc::Channel`. Typically, gRPC establishes the connection when the RPC is made, but the library provides a `wait_for_grpc_server_connection` method to establish the connection prior to making an RPC call.

2. **Create a `gnmi_stream_imp` Object**:
   Pass the `grpc::Channel` from the previous step into this object. The library handles the stub creation and streaming logic (`ClientReaderWriter`) for you, abstracting most of the gRPC-specific details.
