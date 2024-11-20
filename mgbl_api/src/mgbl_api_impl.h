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
#ifndef MGBL_API_IMPL_H_
#define MGBL_API_IMPL_H_

#include <fmt/format.h>
#include <grpcpp/grpcpp.h>
#include <mutex>
#include <thread>
#include "gnmi.pb.h"
#include "gnmi/mgbl_gnmi_helper.h"
#include "logger/logger.h"
#include "mgbl_api.h"

namespace mgbl_api
{
/** \addtogroup gnmi
 *  @{
 */
/**
 * @brief The Impl class is a helper
 * class for the GnmiClient class.
 */
/*
   Reconsider if this class is helpful or shouldn't be merged with GnmiClient and later extract
   helper with different boundaries
*/
class GnmiClientDetails
{
   public:
    /** ClientReaderWriter used within rpc_register_stats_stream and
     * rpc_stream_close
     */
    std::shared_ptr<grpc::ClientReaderWriter<gnmi::SubscribeRequest, gnmi::SubscribeResponse>>
        subscribe_stream_rw;

    /** gRPC ClientContext used within rpc_register_stats_stream and
     * rpc_stream_close
     */
    std::shared_ptr<grpc::ClientContext> stream_context = nullptr;

    /** Stub created on instantiation of GnmiClient. */
    std::shared_ptr<gnmi::gNMI::Stub> stub;

    /** Used to separate types of different streams into their own instance */
    std::string client_type;

    /** Thread use within rpc_register_stats_stream and rpc_stream_close */
    std::thread receive_thread;

    /** Used to keep client context in line with subscription */
    std::mutex subscription_mode_stream_mtx;

    /**
     * @brief Helper function to populate the subscription request object.
     * @param request Pointer to the SubscribeRequest.
     * @param rpc_args Configuration for the RPC call.
     * @param info Configuration for the subscription paths.
     * @return Internal error code indicating success or failure.
     */
    internal_error_code subscribe_request_helper(gnmi::SubscribeRequest* request,
                                                 const rpc_args& rpc_args,
                                                 const rpc_stream_args& info);

    /**
     * @brief Checks if the SubscribeResponse object is valid.
     * @param response The SubscribeResponse object.
     * @param response_stats Pointer to the response stats.
     * @param interface The interface object.
     * @return Internal error code indicating success or failure.
     */
    std::pair<std::shared_ptr<PBRBase::pbr_stat>, internal_error_code> check_response(
        const gnmi::SubscribeResponse& response, PBRBase& pbr_counter);

    /**
     * @brief Decode a gnmi::Update as Json IETF format and returns a map of the flattened json
     *
     * @param data_update A gnmi::Update object
     * @param flattened_json Pointer to the map to be populated
     * @throws nlohmann::json::exception if the json string is not valid
     */
    void gnmi_decode_json_ietf(
        const gnmi::Update& data_update,
        std::shared_ptr<std::unordered_map<std::string, std::string>>& flattened_json);

    /**
     * @brief Decode a gnmi::Update as Proto format and returns a map of the flattened proto
     *
     * @param data_update A gnmi::Update object
     * @param flattened_proto Pointer to the map to be populated
     */
    void gnmi_decode_proto(
        const gnmi::Update& data_update,
        std::shared_ptr<std::unordered_map<std::string, std::string>>& flattened_proto);

    /**
     * @brief Generates and populates pbr return structure from the subscription response
     *
     * @param notification Reference to the gnmi::Notification object
     * @param pbr_stats Pointer to the pbr_stats structure to be populated
     * @return internal_error_code Error code indicating the result of the operation
     */
    std::pair<std::shared_ptr<std::unordered_map<std::string, std::string>>, internal_error_code>
    gnmi_parse_response(const gnmi::Notification& notification, std::string path_origin);

    ~GnmiClientDetails()
    {
        if (receive_thread.joinable())
        {
            receive_thread.join();  // Ensure thread cleanup
        }
    }
};
/** @} */  // end of gnmi
}  // namespace mgbl_api
#endif  // MGBL_API_IMPL_H_