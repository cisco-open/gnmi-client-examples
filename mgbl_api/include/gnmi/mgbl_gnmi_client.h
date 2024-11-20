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
#ifndef MGBL_GNMI_CLIENT_H_
#define MGBL_GNMI_CLIENT_H_

#include <grpcpp/grpcpp.h>

#include <utility>
#include "gnmi.grpc.pb.h"
#include "mgbl_api.h"
#include "mgbl_api_impl.h"
#include "rpc/mgbl_rpc.h"

namespace mgbl_api
{
/** \addtogroup gnmi
 *  @{
 */
/**
 * @class GnmiClient
 * @brief Template class for gNMI client.
 *
 * @param CounterInterface The interface type.
 */
class GnmiClient
{
   public:
    /**
     * @brief Constructor takes a grpc::Channel shared ptr and a CounterInterface type.
     * It creates a stub which is used to make rpc calls.
     *
     * @param channel Shared pointer to the grpc::Channel object.
     * @param interface The CounterInterface type.
     */
    GnmiClient(const std::shared_ptr<grpc::Channel>& channel,
               std::shared_ptr<GnmiCounters> interface);
    GnmiClient() = delete;
    GnmiClient(const GnmiClient&) = delete;
    GnmiClient(GnmiClient&&) = delete;
    GnmiClient& operator=(const GnmiClient&) = delete;
    GnmiClient& operator=(GnmiClient&&) = delete;

    ~GnmiClient() noexcept
    {
        rpc_stream_close();
        // destructor body
        // Join the threads and calls stream stop;
    }

    /**
     * @brief Gets called when RPC Failure happens.
     *
     * User will have to decide how they handle an rpc failure.
     * Handler called after RPC fails within `rpc_register_stats_stream`.
     * It will be called within the receive thread.
     *
     * Rpc failure can happen for a multitude of reasons.
     * Channel Shutdown, Rpc cancelled, etc...
     * Cancel is expected when using `stream_pbr_close`, so it will not
     * call this function.
     *
     * @param status The grpc::Status object.
     */
    void set_rpc_failed_handler(std::function<void(grpc::Status)> handler)
    {
        rpc_failed_handler = std::move(handler);
    }

    /**
     * @brief User defined function that is called after a response is received and parsed with no
     * errors.
     *
     * Specifically used within the `rpc_register_stats_stream` receive thread.
     *
     * If not defined by the user this function will do nothing.
     * It is up to the user to decide what they want to do with the response.
     *
     * @param response_stats The response stats.
     */
    void set_rpc_success_handler(std::function<void(std::shared_ptr<GnmiCounters>)> handler)
    {
        rpc_success_handler = std::move(handler);
    }

    /**
     * @brief This pertains only to subscription mode as Stream.
     *
     * User should always call `stream_pbr_close` after the `rpc_register_stats_stream`.
     * User has the responsibility of closing the stream themselves.
     *
     * It tries to end the stream without regard for responses.
     * This will result in an rpc_failure, thus a
     * grpc::StatusCode::CANCELLED is expected.
     * Then receive thread is joined.
     *
     * This is a blocking call.
     *
     * @return An error_code.
     */
    error_code rpc_stream_close();

    /**
     * @brief Creates and sends subscription once request and blocks until response is received or
     * connection/rpc error occurs.
     *
     * The request will use the paths from CounterInterface.
     * Requires The context arguments for the stream, and subscription rpc metadata.
     * The returned data will be directly stored in the stats vector of the CounterInterface.
     *
     * @param context_args The context arguments for the stream.
     * @param rpc_args The subscription rpc metadata.
     * @return A pair of error_code and grpc::Status.
     */
    std::pair<error_code, grpc::Status> rpc_register_stats_once(
        const client_context_args& context_args, rpc_args& rpc_args);

    /**
     * @brief Creates and sends subscription stream request.
     *
     * Starts the receive function in a new thread which will later be joined by calling the
     * `rpc_stream_close` function. Then sends the request.
     * If the receive thread already exists, it only sends the request.
     *
     * The request will use the paths from CounterInterface.
     * Requires context arguments for the stream, and subscription rpc metadata.
     *
     * The user should always call `rpc_stream_close` after a transaction (successful or not).
     *
     * The user needs to implement the `rpc_failed_handler` to handle when, an RPC failure occurs,
     * as grpc::Status is returned only in the receive thread.
     *
     * The user needs to implement the `rpc_success_handler`. Called within the receive thread when
     * a response is received and checked successfully.
     *
     * @param context_args The context arguments for the stream.
     * @param rpc_args The subscription rpc metadata.
     * @return An error_code.
     */
    error_code rpc_register_stats_stream(const client_context_args& context_args,
                                         rpc_args& rpc_args);


    /**
     * @brief get_counter_gnmi_paths returns the paths for the given counter.
     */
    std::vector<std::string> get_counter_gnmi_paths(const GnmiCounters& counter) const;

   protected:
    std::shared_ptr<GnmiClientDetails> impl_;

   private:
    std::shared_ptr<GnmiCounters> interface;
    std::function<void(grpc::Status)> rpc_failed_handler;
    std::function<void(std::shared_ptr<GnmiCounters>)> rpc_success_handler;
};
/** @}*/  // end of gnmi
}  // namespace mgbl_api
#endif  // MGBL_GNMI_CLIENT_H_