/*
 * Copyright (c) 2024 Cisco Systems, Inc. and its affiliates
 * All rights reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef MGBL_RPC_H_
#define MGBL_RPC_H_

#include <grpcpp/grpcpp.h>

#include <utility>
#include "gnmi.grpc.pb.h"

namespace mgbl_api
{
/** \addtogroup rpc
 *  @{
 */
/**
 * @enum error_code
 * @brief Enum for specifying the type of error.
 * More information on RPC Failure and RPC Status can be found in the grpc documents
 * linked within the README.
 */
enum class error_code
{
    SUCCESS = 0,
    CLIENT_TYPE_FAILURE,
    WRITES_FAILED,
    RPC_FAILURE
};

/**
 * @enum stream_mode
 * @brief Enum for specifying the stream mode.
 */
enum class stream_mode
{
    STREAM, /**< STREAM Stream mode*/
    ONCE    /**< ONCE Once mode*/
};

/**
 * @brief Struct for configuring the Channel Credentials.
 */
struct rpc_channel_args
{
    std::string server_address;       /**< The endpoint which the user wishes to connect to */
    bool ssl_tls = false;             /**< Whether or not the user wants SSL encryption */
    std::string pem_roots_certs_path; /**< Root certificates for SSL */
    std::string pem_private_key_path; /**< Private key for SSL */
    std::string pem_cert_chain_path;  /**< Certificate chain for SSL */

    rpc_channel_args() = default;
    rpc_channel_args(std::string address, bool tls)
        : server_address(std::move(address)), ssl_tls(tls)
    {
    }
    rpc_channel_args(std::string address, bool tls, std::string roots_certs,
                     std::string private_key, std::string cert_chain)
        : server_address(std::move(address)),
          ssl_tls(tls),
          pem_roots_certs_path(std::move(roots_certs)),
          pem_private_key_path(std::move(private_key)),
          pem_cert_chain_path(std::move(cert_chain))
    {
    }
};

/**
 * @brief Struct for configuring the client context.
 */
struct client_context_args
{
    std::string username;      /**< Username for the endpoint */
    std::string password;      /**< Password for the endpoint */
    bool set_deadline = false; /**< Whether to set a deadline for the RPC call */
    std::chrono::system_clock::time_point deadline; /**< The deadline for the RPC call */
};

/**
 * @brief Struct for configuring the subscription.
 */
struct rpc_args
{
    static constexpr uint64_t DEFAULT_SAMPLE_INTERVAL_NSEC =
        1000000000; /**< Default sample interval time in nanoseconds */
    uint64_t sample_interval_nsec =
        DEFAULT_SAMPLE_INTERVAL_NSEC; /**< Interval time between gnmi responses in nanoseconds */
    gnmi::Encoding encoding =
        gnmi::Encoding::PROTO;              /**< Encoding of the responses through the RPC call*/
    stream_mode mode = stream_mode::STREAM; /**< Type of stream */
    int rpc_type = 0;                       /**< Type of RPC call */
};
/** @} */  // end of rpc
}  // namespace mgbl_api
#endif  // MGBL_RPC_H_