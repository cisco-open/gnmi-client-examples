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
#ifndef MGBL_GNMI_CONN_H_
#define MGBL_GNMI_CONN_H_

#include <grpcpp/grpcpp.h>
#include "gnmi.grpc.pb.h"
#include "rpc/mgbl_rpc.h"

namespace mgbl_api
{
/** \addtogroup gnmi
 *  @{
 */
/**
 * @class gnmi_client_connection
 * @brief Creates the gnmi client connection for the user.
 *
 * Helps user to create grpc::Channel object to be passed in
 * to other classes to perform RPC's with gnmi/grpc.
 */
class gnmi_client_connection
{
   public:
    /**
     * @brief Constructs a channel object based off the channel_args.
     * If the channel arguments are not valid, an exception is thrown.
     *
     * @param channel_args The arguments for the channel.
     */
    explicit gnmi_client_connection(const rpc_channel_args& channel_args);

    gnmi_client_connection(gnmi_client_connection&&) = default;
    gnmi_client_connection& operator=(gnmi_client_connection&&) = default;
    gnmi_client_connection(const gnmi_client_connection&) = delete;
    gnmi_client_connection& operator=(const gnmi_client_connection&) = delete;

    virtual ~gnmi_client_connection() = default;

    /**
     * @brief Returns the channel object.
     *
     * @return A shared pointer to the grpc::Channel object.
     */
    std::shared_ptr<grpc::Channel> get_channel();

    /**
     * @brief Waits until the connection to the server is in a ready state.
     *
     * Used for initial connection to the server. Will return true if the
     * connection is established. Returns false if all attempts fail.
     *
     * @param retries Number of retries.
     * @param interval_between_retries Interval between retries.
     * @return True if the connection is established, false otherwise.
     */
    virtual bool wait_for_grpc_server_connection(uint32_t retries,
                                                 std::chrono::seconds interval_between_retries);

   private:
    std::shared_ptr<grpc::Channel> channel;
    std::shared_ptr<grpc::ChannelCredentials> creds;
};
/** @}*/  // end of gnmi
}  // namespace mgbl_api
#endif  // MGBL_GNMI_CONN_H_