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
#include <iostream>
#include <memory>
#include <getopt.h>
#include <cstring>
#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "../protos/gnmi/generated/gnmi.grpc.pb.h"
#include "../protos/gnmi/generated/gnmi.pb.h"
#include <nlohmann/json.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using gnmi::gNMI;
using gnmi::SubscribeRequest;
using gnmi::SubscribeResponse;
using grpc::WriteOptions;

/*
* structure which has all the leaves under interface counters container
* as attributes.
*/
typedef struct interface_stats_ {
    std::string      name; /* name of the interface */
    uint64_t  in_pkts = 0;
    uint64_t  out_pkts = 0;
    uint64_t  in_octets = 0;
    uint64_t  out_octets = 0;
    uint64_t  in_multicast_pkts = 0;
    uint64_t  out_multicast_pkts = 0;
    uint64_t  in_broadcast_pkts = 0;
    uint64_t  out_broadcast_pkts = 0;
    uint64_t  in_unicast_pkts = 0;
    uint64_t  out_unicast_pkts = 0;
    uint64_t  in_errors = 0;
    uint64_t  out_errors = 0;
    uint64_t  in_unknown_protos = 0;
    uint64_t  in_fcs_errors = 0;
    uint64_t  in_discards = 0;
    uint64_t  out_discards = 0;
} interface_stats;

typedef void (*cb_fn_type)(interface_stats *stats);

class ArgumentsPassed{
    public:
        std::string      username;
        std::string      password;
        std::string      path_of_interest;
        int              rpc_type;

        // Mode: 0 = STREAM, 1 = ONCE, 2 = POLL
        int              mode;
        // we multiply by 1000000000 since this parameter is in nanoseconds
        uint64_t         sample_interval_sec = 1000000000;
        // Default is set to gnmi::Encoding::PROTO = 0
        gnmi::Encoding   encoding;
};

// Arguments which to be changed
class ChangeableArguments{
    public:
        std::string      username;
        std::string      password;

        // we multiply by 1000000000 since this parameter is in nanoseconds thus default is 1 second
        uint64_t         sample_interval_sec = 1000000000;
        // Default is set to gnmi::Encoding::PROTO = 0
        gnmi::Encoding   encoding = gnmi::Encoding::PROTO;
};

class GNMIClient {
public:
    GNMIClient(std::shared_ptr<Channel> channel)
            : stub_(gNMI::NewStub(channel)){}

    void Subscribe(ArgumentsPassed info, cb_fn_type cb, std::vector<interface_stats>& stats);

    void printSubscribeRequest(SubscribeRequest& request);

    bool createStruct(SubscribeResponse*  response, interface_stats* ret, std::string path_of_interest, gnmi::Encoding encoding);
    bool jsonIetfStruct(gnmi::Notification notification, interface_stats* ret, std::string path_of_interest);
    bool protoStruct(gnmi::Notification notification, interface_stats* ret, std::string path_of_interest);

private:
    std::unique_ptr<gNMI::Stub> stub_;
};

/*
* This API will get the stats for the interface(s) provided in the key.
* This will build the corresponding xpath string and subscribe to it.
* get_interface_stats(std::string key, cb_fn_type cb, ChangeableArguments requirements) is for STREAM or POLL
* get_interface_stats(std::string key, std::vector<interface_stats>& test, ChangeableArguments requirements) is for ONCE
*
* Argument: key
*  IN - key to be used in the corresponding xpath.
*
* Argument: cb
*  IN - callback to be invoked for every update.
*
* Argument: ret_vector
*  IN - vector of type interface stats. Will push data to this vector
*
* Argument: requirements
*  IN - input struct which user provides data for subscribe request
*/
int get_interface_stats(std::string key, cb_fn_type cb, ChangeableArguments requirements);

int get_interface_stats(std::string key, std::vector<interface_stats>& ret_vector, ChangeableArguments requirements);