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

#include "mgbl_api.h"
#include <fmt/format.h>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <string>
#include "gnmi/mgbl_gnmi_client.h"
#include "gnmi/mgbl_gnmi_helper.h"
#include "mgbl_api_impl.h"
#include "pbr/mgbl_pbr.h"

namespace mgbl_api
{
using json = nlohmann::json;

/* Helper functions for parsing and generating response data */
/** \addtogroup gnmi
 *  @{
 */

/**
 * @brief Creates the gnmi client connection for the user.
 * @param channel_args Configuration for the channel.
 * Helps user to create grpc::Channel object to be passed in
 * to other classes to perform RPC's with gnmi/grpc.
 */
gnmi_client_connection::gnmi_client_connection(const rpc_channel_args& channel_args)
{
    if (channel_args.ssl_tls)
    {
        if (channel_args.pem_roots_certs_path.empty())
        {
            logger_manager::get_instance().log("Root certificate location is empty",
                                               log_level::ERROR);
            throw std::invalid_argument("Root certificate location is empty");
        }

        grpc::SslCredentialsOptions ssl_opts;
        auto read = [](const std::string& filename, std::string& data)
        {
            std::ifstream file(filename.c_str(), std::ios::in);

            if (file.is_open())
            {
                std::stringstream ss;
                ss << file.rdbuf();
                file.close();
                data = ss.str();
            }
        };
        read(channel_args.pem_roots_certs_path, ssl_opts.pem_root_certs);
        read(channel_args.pem_private_key_path, ssl_opts.pem_private_key);
        read(channel_args.pem_cert_chain_path, ssl_opts.pem_cert_chain);
        creds = grpc::SslCredentials(ssl_opts);
    }
    else
    {
        this->creds = grpc::InsecureChannelCredentials();
    }
    this->channel = grpc::CreateChannel(channel_args.server_address, this->creds);
}

/**
 * @brief Returns a shared pointer to the grpc::Channel object.
 */
std::shared_ptr<grpc::Channel> gnmi_client_connection::get_channel()
{
    return this->channel;
}

/**
 * @brief Waits until the connection to the server is in a ready state.
 * @param retries Number of retries.
 * @param interval_between_retries Interval between retries.
 * @return True if the connection is established, false otherwise.
 */
bool gnmi_client_connection::wait_for_grpc_server_connection(
    uint32_t retries, std::chrono::seconds interval_between_retries)
{
    for (uint32_t retry_count = 0; retry_count < retries; ++retry_count)
    {
        this->channel->WaitForConnected(std::chrono::system_clock::now() +
                                        interval_between_retries);
        if (this->channel->GetState(false) == grpc_connectivity_state::GRPC_CHANNEL_READY)
        {
            return true;
        }
        logger_manager::get_instance().log("Retry attempt number: " + std::to_string(retry_count) +
                                               " of " + std::to_string(retries),
                                           log_level::INFO);
    }
    logger_manager::get_instance().log("Connection to server failed", log_level::ERROR);
    return false;
}

/**
 * @brief Helper function to populate the subscription request object.
 * @param request Pointer to the SubscribeRequest.
 * @param rpc_args Configuration for the RPC call.
 * @param info Configuration for the subscription paths.
 * @return Internal error code indicating success or failure.
 */
internal_error_code GnmiClientDetails::subscribe_request_helper(gnmi::SubscribeRequest* request,
                                                                const rpc_args& rpc_args,
                                                                const rpc_stream_args& info)
{
    auto* subscription_list = request->mutable_subscribe();

    subscription_list->set_mode(static_cast<gnmi::SubscriptionList_Mode>(rpc_args.mode));

    // set encoding
    if (rpc_args.encoding == gnmi::Encoding::JSON_IETF ||
        rpc_args.encoding == gnmi::Encoding::PROTO)
    {
        // Default is always set to gnmi::Encoding::PROTO
        subscription_list->set_encoding(rpc_args.encoding);
    }
    else
    {
        std::string msg = fmt::format("Unknown encoding: {}", static_cast<int>(rpc_args.encoding));
        logger_manager::get_instance().log(msg, log_level::ERROR);
        return internal_error_code::UNSUPPORTED_ENCODING;
    }

    for (int i = 0; i < info.paths_of_interest.size(); i++)
    {
        std::string path = info.paths_of_interest.at(i);
        auto* subscription = subscription_list->add_subscription();
        subscription->set_mode(gnmi::SubscriptionMode::SAMPLE);
        subscription->set_sample_interval(rpc_args.sample_interval_nsec);
        // set path (since we used set_allocated we have to provide the memory but
        // it will handle freeing it by itself)
        gnmi::Path* temp_path = new gnmi::Path();
        try
        {
            *temp_path = string_to_gnmipath(path);
        }
        catch (const std::invalid_argument& e)
        {
            std::string msg = fmt::format(
                "Invalid argument while attempting to "
                "convert gnmi path to string: {}",
                e.what());
            logger_manager::get_instance().log(msg, log_level::WARNING);
        }
        subscription->set_allocated_path(temp_path);
    }
    return internal_error_code::SUCCESS;
}

/**
 * @brief Checks if the SubscribeResponse object is valid.
 * @param response The SubscribeResponse object.
 * @param response_stats Pointer to the response stats.
 * @param interface The interface object.
 * @return Internal error code indicating success or failure.
 */
std::pair<std::shared_ptr<PBRBase::pbr_stat>, internal_error_code>
GnmiClientDetails::check_response(const gnmi::SubscribeResponse& response, PBRBase& pbr_counter)
{
    std::pair<std::shared_ptr<PBRBase::pbr_stat>, internal_error_code> result = {
        nullptr, internal_error_code::SUCCESS};
    // Indicate target has sent all values associated with the subscription at
    // least once.
    if (response.sync_response())
    {
        logger_manager::get_instance().log(
            "Target has sent all values associated "
            "with the subscription at least once.",
            log_level::VERBOSE);
    }

    if (response.has_update())
    {
        std::map<std::string, std::string> flattened_map;
        const gnmi::Notification& notification = response.update();
        auto expected_gnmi_map = gnmi_parse_response(notification, pbr_counter.path_origin);
        if (expected_gnmi_map.second != internal_error_code::SUCCESS)
        {
            return {{}, expected_gnmi_map.second};
        }
        auto pbr_stat = pbr_counter.unordered_map_to_stats(*expected_gnmi_map.first);
        result.first = pbr_stat;
    }
    else
    {
        logger_manager::get_instance().log("No new Notifications", log_level::VERBOSE);
        result.second = internal_error_code::NO_NOTIFICATION;
    }
    return result;
}

/**
 * @brief Decode a gnmi::Update as Json IETF format and returns a map of the flattened json
 *
 * @param data_update A gnmi::Update object
 * @param flattened_json Pointer to the map to be populated
 * @throws nlohmann::json::exception if the json string is not valid
 */
void GnmiClientDetails::gnmi_decode_json_ietf(
    const gnmi::Update& data_update,
    std::shared_ptr<std::unordered_map<std::string, std::string>>& flattened_json)
{
    std::string json_string = data_update.val().json_ietf_val();
    nlohmann::json json_struct = nlohmann::json::parse(json_string);

    std::unordered_map<std::string, std::string> result;
    auto flatten_struct = json_struct.flatten();
    for (auto it = flatten_struct.begin(); it != flatten_struct.end(); ++it)
    {
        std::string key = it.key();
        std::string new_key = key;

        if (key.find("/0/") != std::string::npos)
        {
            // Replace /0/ with /
            new_key = std::regex_replace(key, std::regex("/0/"), "/");
        }
        else
        {
            new_key = key;
        }

        if (it.value().type() == nlohmann::json::value_t::string)
        {
            // If type is already string then dump with put the string in another string
            result[new_key] = it.value();
        }
        else
        {
            result[new_key] = it.value().dump();
        }
    }
    flattened_json->insert(result.begin(), result.end());
}
/**
 * @brief Decode a gnmi::Update as Proto format and returns a map of the flattened proto
 *
 * @param data_update A gnmi::Update object
 * @param flattened_proto Pointer to the map to be populated
 */
void GnmiClientDetails::gnmi_decode_proto(
    const gnmi::Update& data_update,
    std::shared_ptr<std::unordered_map<std::string, std::string>>& flattened_proto)
{
    const std::string partial_path = gnmipath_to_string(data_update.path());
    const gnmi::TypedValue& typedVal = data_update.val();

    switch (typedVal.value_case())
    {
        case gnmi::TypedValue::kUintVal:
            (*flattened_proto)[partial_path] = std::to_string(typedVal.uint_val());
            break;
        case gnmi::TypedValue::kIntVal:
            (*flattened_proto)[partial_path] = std::to_string(typedVal.int_val());
            break;
        case gnmi::TypedValue::kStringVal:
            (*flattened_proto)[partial_path] = typedVal.string_val();
            break;
        case gnmi::TypedValue::kBoolVal:
            (*flattened_proto)[partial_path] = typedVal.bool_val() ? "true" : "false";
            break;
        case gnmi::TypedValue::kDoubleVal:
            (*flattened_proto)[partial_path] = std::to_string(typedVal.double_val());
            break;
        // Add more cases as needed for other value types
        default:
            (*flattened_proto)[partial_path] = "unsupported_type";
            break;
    }
};
/**
 * @brief Generates and populates pbr return structure from the subscription response
 *
 * @param notification Reference to the gnmi::Notification object
 * @param pbr_stats Pointer to the pbr_stats structure to be populated
 * @return internal_error_code Error code indicating the result of the operation
 */
std::pair<std::shared_ptr<std::unordered_map<std::string, std::string>>, internal_error_code>
GnmiClientDetails::gnmi_parse_response(const gnmi::Notification& notification,
                                       const std::string path_origin)
{
    gnmi::Path prefix;
    gnmi::Path printed_path;
    gnmi::Update data_update;
    gnmi::Path path;
    gnmi::Encoding encoding = gnmi::Encoding::PROTO;
    std::shared_ptr<std::unordered_map<std::string, std::string>> result =
        std::make_shared<std::unordered_map<std::string, std::string>>();

    if (!notification.has_prefix())
    {
        logger_manager::get_instance().log("Response contained no prefix", log_level::ERROR);
        return {nullptr, internal_error_code::NO_PREFIX_IN_RESPONSE};
    }
    if (notification.prefix().origin().find(path_origin) != std::string::npos)
    {
        prefix = notification.prefix();
    }
    else
    {
        logger_manager::get_instance().log("Response contained wrong prefix", log_level::ERROR);
        return {nullptr, internal_error_code::UNKNOWN_ERROR};
    }

    // Update: a set of path-value pairs indicating the path whose value has
    // changed in the data tree
    int update_size = notification.update_size();
    if (update_size == 0)
    {
        logger_manager::get_instance().log("Update does not exist.", log_level::VERBOSE);
        return {nullptr, internal_error_code::NO_UPDATE_IN_NOTIFICATION};
    }

    for (int j = 0; j < update_size; j++)
    {
        data_update = notification.update(j);
        if (!data_update.has_path())
        {
            continue;
        }
        if (data_update.has_val())
        {
            if (data_update.val().has_json_ietf_val())
            {
                encoding = gnmi::Encoding::JSON_IETF;
                path = data_update.path();
                try
                {
                    gnmi_decode_json_ietf(data_update, result);
                }
                catch (const nlohmann::json::exception& e)
                {
                    std::string msg = fmt::format(
                        "Json exception while parsing update as Json IETF: {}", e.what());
                    logger_manager::get_instance().log(msg, log_level::WARNING);
                }
                catch (const std::exception& e)
                {
                    std::string msg =
                        fmt::format("Exception while parsing update as Json IETF: {}", e.what());
                    logger_manager::get_instance().log(msg, log_level::WARNING);
                }
            }
            /* Other type of encodings in the future (Bytes, ASCII, ...) */
            /* https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md#23-structured-data-types
             */
            else
            {
                // Default is protobuf encoding
                encoding = gnmi::Encoding::PROTO;
                gnmi_decode_proto(data_update, result);
            }
        }
    }

    printed_path = (encoding == gnmi::Encoding::PROTO)
                       ? prefix  // Path in Protobuf case is reliant on the prefix
                       : path;

    for (int i = 0; i < printed_path.elem_size(); i++)
    {
        if (printed_path.elem(i).name() == "policy-map")
        {
            auto key_map = printed_path.elem(i).key();
            auto it = key_map.find("policy-name");
            if (it != key_map.end())
            {
                (*result)["policy_name"] = it->second;
            }
        }
        else if (printed_path.elem(i).name() == "rule-name")
        {
            auto key_map = printed_path.elem(i).key();
            auto it = key_map.find("rule-name");
            if (it != key_map.end())
            {
                (*result)["rule_name"] = it->second;
            }
        }
    }

    if ((*result)["policy_name"].empty())
    {
        logger_manager::get_instance().log("Could not find policy name on return path",
                                           log_level::ERROR);
        return {nullptr, internal_error_code::NO_POLICY_NAME_IN_RESPONSE};
    }

    if ((*result)["rule_name"].empty())
    {
        logger_manager::get_instance().log("Could not find rule name on return path",
                                           log_level::ERROR);
        return {nullptr, internal_error_code::NO_RULE_NAME_IN_RESPONSE};
    }

    return {std::move(result), internal_error_code::SUCCESS};
}
/**
 * @brief Constructor for GnmiClient.
 * @param channel Shared pointer to the grpc::Channel.
 * @param interface The interface object.
 * Creates a stub which is used to make RPC calls.
 */
GnmiClient::GnmiClient(const std::shared_ptr<grpc::Channel>& channel,
                       std::shared_ptr<GnmiCounters> interface)
    : interface(std::move(interface)), impl_(std::make_shared<GnmiClientDetails>())
{
    impl_->stub = gnmi::gNMI::NewStub(channel);
}

/**
 * @brief Destructor for GnmiClient.
 * Joins the receive thread if it is active.
 */
error_code GnmiClient::rpc_stream_close()
{
    if (impl_->receive_thread.joinable())
    {
        std::unique_lock<std::mutex> lock(impl_->subscription_mode_stream_mtx);
        if (impl_->stream_context != nullptr)
        {
            impl_->stream_context->TryCancel();
        }

        // Need to unlock before joining read thread.
        lock.unlock();
        impl_->receive_thread.join();
    }
    if (impl_->subscribe_stream_rw != nullptr)
    {
        impl_->subscribe_stream_rw.reset();
    }
    return error_code::SUCCESS;
}

/**
 * @brief Makes a single request to the server.
 * @param context_args Configuration for the client context.
 * @param rpc_args Configuration for the RPC call.
 * @return Pair of error code and grpc::Status indicating success or failure.
 */
std::pair<error_code, grpc::Status> GnmiClient::rpc_register_stats_once(
    const client_context_args& context_args, rpc_args& rpc_args)
{
    error_code err = error_code::SUCCESS;
    grpc::Status status;
    gnmi::SubscribeRequest request;
    gnmi::SubscribeResponse response;
    grpc::WriteOptions wr_opts;
    rpc_stream_args info;

    std::pair<error_code, grpc::Status> ret_pair(error_code::SUCCESS, status);

    /*
        Further there is implication that everything is a pbr counter, which should be
        presented in better way codewise, but because that's only one counter to support
        as for now, then it will be planned with further extensions
    */
    if (interface->name() != "pbr")
    {
        std::string err_message = fmt::format(
            "Error, this client can only do one type of request. "
            "Current request type is {}\n",
            interface->name());
        logger_manager::get_instance().log(err_message, log_level::ERROR);
        ret_pair.first = error_code::CLIENT_TYPE_FAILURE;
        return ret_pair;
    }
    auto pbr_interface = std::dynamic_pointer_cast<PBRBase>(interface);

    grpc::ClientContext stream_once_context;
    if (context_args.username.empty() || context_args.password.empty())
    {
        logger_manager::get_instance().log("Username or password is empty", log_level::ERROR);
        ret_pair.first = error_code::CLIENT_TYPE_FAILURE;
        return ret_pair;
    }
    stream_once_context.AddMetadata("username", context_args.username);
    stream_once_context.AddMetadata("password", context_args.password);
    if (context_args.set_deadline)
    {
        stream_once_context.set_deadline(context_args.deadline);
    }

    info.paths_of_interest = pbr_interface->get_gnmi_paths();
    info.prefix = pbr_interface->path_origin;
    rpc_args.mode = stream_mode::ONCE;
    impl_->subscribe_request_helper(&request, rpc_args, info);

    std::shared_ptr<grpc::ClientReaderWriter<gnmi::SubscribeRequest, gnmi::SubscribeResponse>>
        subscribe_once_rw(impl_->stub->Subscribe(&stream_once_context));

    if (subscribe_once_rw->Write(request, wr_opts))
    {
        logger_manager::get_instance().log("Subscribe Once Request: Write operation succeeded",
                                           log_level::INFO);
    }
    else
    {
        logger_manager::get_instance().log("Subscribe Once Request: Write operation failed",
                                           log_level::ERROR);
    }

    while (subscribe_once_rw->Read(&response))
    {
        auto expected_response_stats = impl_->check_response(response, *pbr_interface);
        if (expected_response_stats.second == internal_error_code::SUCCESS)
        {
            logger_manager::get_instance().log("Client received a response.", log_level::VERBOSE);
            pbr_interface->add_stats(expected_response_stats.first);
        }
    }

    if (!subscribe_once_rw->WritesDone())
    {
        logger_manager::get_instance().log(
            "Signal to server that we are done writing, was unsuccessful", log_level::WARNING);
    }

    status = subscribe_once_rw->Finish();
    if (!status.ok())
    {
        std::string result = fmt::format("Subscribe rpc failed: {}", status.error_message());
        logger_manager::get_instance().log(result, log_level::ERROR);
        ret_pair.first = error_code::RPC_FAILURE;
    }
    else
    {
        logger_manager::get_instance().log("Subscribe rpc passed ", log_level::INFO);
    }
    ret_pair.second = status;
    return ret_pair;
}

/**
 * @brief Makes a stream request to the server.
 * @param context_args Configuration for the client context.
 * @param rpc_args Configuration for the RPC call.
 * @return Error code indicating success or failure.
 */
error_code GnmiClient::rpc_register_stats_stream(const client_context_args& context_args,
                                                 rpc_args& rpc_args)
{
    error_code err = error_code::SUCCESS;
    gnmi::SubscribeRequest request;
    grpc::WriteOptions wr_opts;
    rpc_stream_args info;

    if (interface->name() != "pbr")
    {
        std::string err_message = fmt::format(
            "Error, this client can only do one type of request. "
            "Current request type is {}",
            interface->name());
        logger_manager::get_instance().log(err_message, log_level::ERROR);
        return error_code::CLIENT_TYPE_FAILURE;
    }

    auto pbr_interface = std::dynamic_pointer_cast<PBRBase>(interface);
    info.paths_of_interest = pbr_interface->get_gnmi_paths();
    info.prefix = pbr_interface->path_origin;
    rpc_args.mode = stream_mode::STREAM;
    impl_->subscribe_request_helper(&request, rpc_args, info);

    // Check if the receive thread is active. If not then we have to create it.
    if (!impl_->receive_thread.joinable())
    {
        std::lock_guard<std::mutex> lock(impl_->subscription_mode_stream_mtx);

        impl_->stream_context = std::make_shared<grpc::ClientContext>();
        impl_->stream_context->AddMetadata("username", context_args.username);
        impl_->stream_context->AddMetadata("password", context_args.password);
        if (context_args.set_deadline)
        {
            impl_->stream_context->set_deadline(context_args.deadline);
        }

        std::shared_ptr<grpc::ClientReaderWriter<gnmi::SubscribeRequest, gnmi::SubscribeResponse>>
            srw(impl_->stub->Subscribe(impl_->stream_context.get()));
        impl_->subscribe_stream_rw = srw;

        /*
         * This lambda function handles the receive loop for the gNMI stream.
         * It processes responses and populates pbr_stats via pbr_function_handler.
         * Once the stream finishes, it cleans up the stream context.
         */
        impl_->receive_thread = std::thread(
            [this]()
            {
                gnmi::SubscribeResponse response;
                // Check response
                while (impl_->subscribe_stream_rw->Read(&response))
                {
                    PBRBase::pbr_stat response_stats;
                    auto pbr_interface = std::dynamic_pointer_cast<PBRBase>(interface);
                    // If the interface no longer exists, we do not want to use it
                    if (pbr_interface == nullptr)
                    {
                            continue;
                    }
                    auto expected_response_stats = impl_->check_response(response, *pbr_interface);
                    if (expected_response_stats.second == internal_error_code::SUCCESS)
                    {
                        logger_manager::get_instance().log("Client received a response.",
                                                           log_level::VERBOSE);
                        pbr_interface->add_stats(expected_response_stats.first);
                        rpc_success_handler(pbr_interface);
                    }
                    else
                    {
                        std::string message =
                            fmt::format("Error while processing response: {}",
                                        static_cast<int>(expected_response_stats.second));
                        logger_manager::get_instance().log(message, log_level::ERROR);
                    }
                }

                // ClientContext should only be alive for the duration of the stream
                std::lock_guard<std::mutex> lock(impl_->subscription_mode_stream_mtx);

                impl_->subscribe_stream_rw->WritesDone();
                grpc::Status status = impl_->subscribe_stream_rw->Finish();

                if (!status.ok())
                {
                    if (status.error_code() != grpc::StatusCode::CANCELLED)
                    {
                        std::string result = fmt::format(
                            "Subscribe stream RPC failed with error: {}", status.error_message());
                        logger_manager::get_instance().log(result, log_level::ERROR);
                        rpc_failed_handler(status);
                    }
                    else
                    {
                        std::string result =
                            fmt::format("Subscribe rpc passed: {}", status.error_message());
                        logger_manager::get_instance().log(result, log_level::INFO);
                    }
                }
                else
                {
                    logger_manager::get_instance().log("Subscribe rpc passed.", log_level::INFO);
                }
            });
    }
    /*
     * Locking required when write is called after disconnect.
     * In such case, a WritesDone and Finish was called, so prevent a write
     */
    std::lock_guard<std::mutex> lock(impl_->subscription_mode_stream_mtx);
    if (impl_->stream_context != nullptr)
    {
        bool check_write = impl_->subscribe_stream_rw->Write(request, wr_opts);
        if (check_write)
        {
            logger_manager::get_instance().log(
                "Subscribe Stream Request: Write operation succeeded", log_level::INFO);
        }
        else
        {
            logger_manager::get_instance().log("Subscribe Stream Request: Write operation failed",
                                               log_level::ERROR);
            err = error_code::WRITES_FAILED;
        }
    }
    else
    {
        logger_manager::get_instance().log("Please close the stream. An rpc failure occurred",
                                           log_level::ERROR);
        err = error_code::RPC_FAILURE;
    }

    return err;
}

std::vector<std::string> GnmiClient::get_counter_gnmi_paths(const GnmiCounters& counter) const
{
    try
    {
        if (const auto* pbr = dynamic_cast<const PBRBase*>(&counter))
        {
            return pbr->get_gnmi_paths();
        }
        throw std::bad_cast();
    }
    catch (const std::bad_cast& e)
    {
        std::string msg = fmt::format(
            "Bad cast exception from GnmiCounters to the required derived class. Actual class: {}",
            typeid(counter).name());
        logger_manager::get_instance().log(msg, log_level::ERROR);
    }
}
/** @} */  // end of gnmi
}  // namespace mgbl_api
