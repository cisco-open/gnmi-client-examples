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
#include <cstring>
#include <string>
#include <vector>
#include <time.h>
#include "gnmi_client.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using gnmi::gNMI;
using gnmi::SubscribeRequest;
using gnmi::SubscribeResponse;
using grpc::WriteOptions;

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md

using json = nlohmann::json;
GNMIClient* client;

void test_callback_func(interface_stats* stats)
{
    std::cout <<"Here are the stats listed: SubscribeResponse" << std::endl;
    std::cout <<"--------------------------------" << stats->name << "--------------------------------" << std::endl;
    std::cout <<"       in-pkts: " << stats->in_pkts << std::endl;
    std::cout <<"       out-pkts: " << stats->out_pkts << std::endl;
    std::cout <<"       in-octets: " << stats->in_octets << std::endl;
    std::cout <<"       out-octets: " << stats->out_octets << std::endl;
    std::cout <<"       in-multicast-pkts: " << stats->in_multicast_pkts << std::endl;
    std::cout <<"       out-multicast-pkts: " << stats->out_multicast_pkts << std::endl;
    std::cout <<"       in-broadcast-pkts: " << stats->in_broadcast_pkts << std::endl;
    std::cout <<"       out-broadcast-pkts: " << stats->out_broadcast_pkts << std::endl;
    std::cout <<"       in-unicast-pkts: " << stats->in_unicast_pkts << std::endl;
    std::cout <<"       out-unicast-pkts: " << stats->out_unicast_pkts << std::endl;
    std::cout <<"       in-errors: " << stats->in_errors << std::endl;
    std::cout <<"       out-errors: " << stats->out_errors << std::endl;
    std::cout <<"       in-unknown-protos: " << stats->in_unknown_protos << std::endl;
    std::cout <<"       in-fcs-errors: " << stats->in_fcs_errors << std::endl;
    std::cout <<"       in-discards: " << stats->in_discards << std::endl;
    std::cout <<"       out-discards: " << stats->out_discards << std::endl;
}

void test_once_func(std::vector<interface_stats> temp)
{
    for(int i = 0; i < temp.size(); i++){
        interface_stats stats = temp[i];
        std::cout <<"Here are the stats listed: SubscribeResponse" << std::endl;
        std::cout <<"--------------------------------" << stats.name << "--------------------------------" << std::endl;
        std::cout <<"       in-pkts: " << stats.in_pkts << std::endl;
        std::cout <<"       out-pkts: " << stats.out_pkts << std::endl;
        std::cout <<"       in-octets: " << stats.in_octets << std::endl;
        std::cout <<"       out-octets: " << stats.out_octets << std::endl;
        std::cout <<"       in-multicast-pkts: " << stats.in_multicast_pkts << std::endl;
        std::cout <<"       out-multicast-pkts: " << stats.out_multicast_pkts << std::endl;
        std::cout <<"       in-broadcast-pkts: " << stats.in_broadcast_pkts << std::endl;
        std::cout <<"       out-broadcast-pkts: " << stats.out_broadcast_pkts << std::endl;
        std::cout <<"       in-unicast-pkts: " << stats.in_unicast_pkts << std::endl;
        std::cout <<"       out-unicast-pkts: " << stats.out_unicast_pkts << std::endl;
        std::cout <<"       in-errors: " << stats.in_errors << std::endl;
        std::cout <<"       out-errors: " << stats.out_errors << std::endl;
        std::cout <<"       in-unknown-protos: " << stats.in_unknown_protos << std::endl;
        std::cout <<"       in-fcs-errors: " << stats.in_fcs_errors << std::endl;
        std::cout <<"       in-discards: " << stats.in_discards << std::endl;
        std::cout <<"       out-discards: " << stats.out_discards << std::endl;
    }
}

std::string printPath(gnmi::Path* path)
{
    int           path_elem_size;
    std::string   element;

    path_elem_size = path->elem_size();
    for(int i=0; i < path_elem_size; i++) {
        element.append(path->elem( i ).name());
        for (auto j = path->elem(i).key().begin(); j != path->elem(i).key().end(); ++j ) {
            element.append("[");
            element.append(j->first);
            element.append("=");
            element.append(j->second);
            element.append("]");
        }
        element.append("/");
    }
    return element;
}

gnmi::Path convertPath(const std::string& path)
{
    gnmi::Path               temp;
    int                      start = 0;
    std::string              elem;
    for(int i = 0; i < path.size(); i++) {
        if(path[i] == '/') {
            elem = path.substr(start, i-start);
            temp.add_elem()->set_name(elem);
            start = i+1;
        }
    }
    return temp;
}


void 
GNMIClient::Subscribe(ArgumentsPassed info, cb_fn_type cb, std::vector<interface_stats>& stats) 
{
    SubscribeRequest request;
    std::string      path = info.path_of_interest;
    auto*            subscription_list = request.mutable_subscribe();
    auto*            subscription = subscription_list->add_subscription();

    subscription_list->set_mode((gnmi::SubscriptionList_Mode)info.mode);
    // set encoding
    subscription_list->set_encoding(info.encoding);
    subscription->set_mode(gnmi::SubscriptionMode::SAMPLE);
    subscription->set_sample_interval(info.sample_interval_sec);
    
    // set path (since we used set_allocated we have to provide the memory but it will handle freeing it by itself)
    gnmi::Path* temp_path = new gnmi::Path();
    *temp_path = convertPath(path);
    subscription->set_allocated_path(temp_path);

    ClientContext context;
    context.AddMetadata("username", info.username);
    context.AddMetadata("password", info.password);

    auto streamReaderWriter = stub_->Subscribe(&context);

    // printSubscribeRequest(request);
    WriteOptions       wrOpts;
    streamReaderWriter->Write(request, wrOpts);
    SubscribeResponse response;

    // Check response
    while (streamReaderWriter->Read(&response)) {
        interface_stats one_stat;
        bool val = createStruct(&response, &one_stat, path, info.encoding);
        if(val == true) {
            // ONCE CASE
            if(info.mode == 1){
                stats.push_back(one_stat);
            }
            // STEAM OR POLL CASE
            else {
                if(cb != NULL) {
                    (*cb)(&one_stat);
                }

            }

        }
    }

    Status status = streamReaderWriter->Finish();

    if (!status.ok()) {
        std::cout << "Subscribe rpc failed: " << status.error_message() << std::endl;
    }
}

void 
GNMIClient::printSubscribeRequest(SubscribeRequest& request)
{
    int                subs_size;
    gnmi::Path         prefix;
    gnmi::Path         path;

    std::cout << std::endl << "--------SubscribeRequest--------" << std::endl;
    if(request.subscribe().has_prefix()) {
        prefix =  request.subscribe().prefix();
        std::cout << "SubscriptionList Prefix: " << printPath(&prefix) << std::endl;
    }

    subs_size = request.subscribe().subscription_size();
    for(int j=0; j < subs_size; j++) {
        std::cout << "Subscription[" << j << "]: "<<  std::endl;
        if(request.subscribe().subscription(j).has_path()) {
            path = request.subscribe().subscription(j).path();
            std::cout << "Path: "<<  printPath(&path) << std::endl;
        }
        else {
            std::cout << "Path: Does not Exist"<<  std::endl;
        }
    }
}

bool 
GNMIClient::createStruct(SubscribeResponse*  response, interface_stats* ret, std::string path_of_interest, gnmi::Encoding encoding)
{
    // Indicate target has sent all values associated with the subscription at least once.
    if(response->sync_response() == true) {
        return false;
    }
    if(response->has_update()) {
        gnmi::Notification       notification;
        notification = response->update();
        switch((int)encoding){
            // JSON_IETF
            case 4:
                return jsonIetfStruct(notification,ret,path_of_interest);
                break;
            //PROTO
            case 2:
                return protoStruct(notification,ret,path_of_interest);
                break;
            default:
                break;
        }
    }
    else {
        std::cout << "No new Notifications" << std::endl;
        return false;
    }
    return true;
}

bool 
GNMIClient::jsonIetfStruct(gnmi::Notification notification, interface_stats* ret, std::string path_of_interest)
{
    gnmi::Path               prefix;
    int                      update_size;
    std::string              complete_prefix;
    std::string              temp_var;
    std::string              printed_path;
    gnmi::Update             data_update;
    gnmi::Path               path;
    gnmi::Value              val;
    gnmi::TypedValue         typedVal;

    if(notification.has_prefix()) {
        prefix =  notification.prefix();
    }
    else {
        // no prefix means there is no existing interface
        return false;
    }

    // Update: a set of path-value pairs indicating the path whose value has changed in the data tree
    update_size = notification.update_size();
    if(!update_size) {
        std::cout << "Update: Does not Exist " <<  std::endl;
        return false;
    }
    for(int j=0; j < update_size; j++) {
        data_update = notification.update(j);
        if (data_update.has_path()) {
            path = data_update.path();
            printed_path = printPath(&path);

            // This is to make sure we only get the interface state/counters data
            size_t test_path = path_of_interest.find("counters");
            size_t test_prefix = printed_path.find("counters");
            if(test_path != std::string::npos && test_prefix == std::string::npos){
                return false;
            }

            complete_prefix = prefix.origin() + ":" + printed_path;
            ret->name = complete_prefix;
        } else {
            // no path means we won't hit the cases we wanted
            continue;
        }

        if (data_update.has_val()) {
            // Explicitly Typed update Value
            typedVal = data_update.val();

            if (typedVal.has_json_ietf_val()) {
                std::string json_string = typedVal.json_ietf_val();
                json json_struct  = json::parse(json_string);

                temp_var = json_struct["in-pkts"];
                ret->in_pkts = std::stoull(temp_var);
                temp_var = json_struct["in-octets"];
                ret->in_octets = std::stoull(temp_var);
                temp_var = json_struct["in-multicast-pkts"];
                ret->in_multicast_pkts = std::stoull(temp_var);
                temp_var = json_struct["in-broadcast-pkts"];
                ret->in_broadcast_pkts = std::stoull(temp_var);
                temp_var = json_struct["in-unicast-pkts"];
                ret->in_unicast_pkts = std::stoull(temp_var);
                temp_var = json_struct["in-errors"];
                ret->in_errors = std::stoull(temp_var);
                temp_var = json_struct["in-unknown-protos"];
                ret->in_unknown_protos = std::stoull(temp_var);
                temp_var = json_struct["in-fcs-errors"];
                ret->in_fcs_errors = std::stoull(temp_var);
                temp_var = json_struct["in-discards"];
                ret->in_discards = std::stoull(temp_var);
                temp_var = json_struct["out-pkts"];
                ret->out_pkts = std::stoull(temp_var);
                temp_var = json_struct["out-octets"];
                ret->out_octets = std::stoull(temp_var);
                temp_var = json_struct["out-multicast-pkts"];
                ret->out_multicast_pkts = std::stoull(temp_var);
                temp_var = json_struct["out-broadcast-pkts"];
                ret->out_broadcast_pkts = std::stoull(temp_var);
                temp_var = json_struct["out-unicast-pkts"];
                ret->out_unicast_pkts = std::stoull(temp_var);
                temp_var = json_struct["out-errors"];
                ret->out_errors = std::stoull(temp_var);
                temp_var = json_struct["out-discards"];
                ret->out_discards = std::stoull(temp_var);
            }
        }
    }
    return true;
}

bool 
GNMIClient::protoStruct(gnmi::Notification notification, interface_stats* ret, std::string path_of_interest)
{
    gnmi::Path               prefix;
    int                      update_size;
    std::string              complete_prefix;
    std::string              partial_path;
    gnmi::Update             data_update;
    gnmi::Path               path;
    gnmi::Value              val;
    gnmi::TypedValue         typedVal;

    if(notification.has_prefix()) {
        prefix =  notification.prefix();
        // Sometimes more information is sent with subscribe requests than what was asked. This is to only check counters is in path and prefix
        std::string printedPrefix = printPath(&prefix);
        size_t test_path = path_of_interest.find("counters");
        size_t test_prefix = printedPrefix.find("counters");
        if(test_path != std::string::npos && test_prefix == std::string::npos){
            return false;
        }

        complete_prefix = prefix.origin() + ":" + printPath(&prefix);
        ret->name = complete_prefix;
    }
    else {
        // no prefix means there is no existing interface
        return false;
    }

    // Update: a set of path-value pairs indicating the path whose value has changed in the data tree
    update_size = notification.update_size();
    if(!update_size) {
        std::cout << "Update: Does not Exist " <<  std::endl;
        return false;
    }

    for(int j=0; j < update_size; j++) {
        data_update = notification.update(j);
        if (data_update.has_path()) {
            path = data_update.path();
            partial_path = printPath(&path);
        } else {
            // no path means we won't hit the cases we wanted
            continue;
        }
        if (data_update.has_val()) {
            // Explicitly Typed update Value
            typedVal = data_update.val();
            if (typedVal.value_case() == gnmi::TypedValue::kUintVal) {
                if (partial_path == "in-pkts/") {
                    ret->in_pkts = typedVal.uint_val();
                }
                else if(partial_path == "in-octets/") {
                    ret->in_octets = typedVal.uint_val();
                }
                else if(partial_path == "in-multicast-pkts/") {
                    ret->in_multicast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "in-broadcast-pkts/") {
                    ret->in_broadcast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "in-unicast-pkts/") {
                    ret->in_unicast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "in-errors/") {
                    ret->in_errors = typedVal.uint_val();
                }
                else if(partial_path == "in-unknown-protos/") {
                    ret->in_unknown_protos = typedVal.uint_val();
                }
                else if(partial_path == "in-fcs-errors/") {
                    ret->in_fcs_errors = typedVal.uint_val();
                }
                else if(partial_path == "in-discards/") {
                    ret->in_discards = typedVal.uint_val();
                }
                else if(partial_path == "out-pkts/") {
                    ret->out_pkts = typedVal.uint_val();
                }
                else if(partial_path == "out-octets/") {
                    ret->out_octets = typedVal.uint_val();
                }
                else if(partial_path == "out-multicast-pkts/") {
                    ret->out_multicast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "out-broadcast-pkts/") {
                    ret->out_broadcast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "out-unicast-pkts/") {
                    ret->out_unicast_pkts = typedVal.uint_val();
                }
                else if(partial_path == "out-errors/") {
                    ret->out_errors = typedVal.uint_val();
                }
                else if(partial_path == "out-discards/") {
                    ret->out_discards = typedVal.uint_val();
                }
            }
        }
    }
    return true;
}

ArgumentsPassed stream_args(std::string key, int mode, ChangeableArguments requirements)
{
    ArgumentsPassed temp_args;
    if (requirements.username == "") {
        std::cout << " Username not provided" << std::endl;
    } else {
        temp_args.username = requirements.username;
    }
    if (requirements.password == "") {
        std::cout << " Password not provided" << std::endl;
    } else {
        temp_args.password = requirements.password;
    }

    if (requirements.encoding == gnmi::Encoding::JSON_IETF || requirements.encoding == gnmi::Encoding::PROTO) {
        // Default is always set to gnmi::Encoding::PROTO
        temp_args.encoding = requirements.encoding;
    }
    temp_args.sample_interval_sec = requirements.sample_interval_sec;
    temp_args.path_of_interest = key;
    temp_args.path_of_interest = "interfaces/interface[name=" + temp_args.path_of_interest + "]/state/counters/";
    temp_args.rpc_type = 0;
    temp_args.mode = mode;
    return temp_args;
}
int get_interface_stats(std::string key, cb_fn_type cb, ChangeableArguments requirements)
{
    // Set mode to 0 as we are streaming
    ArgumentsPassed temp_args = stream_args(key,0, requirements);

    std::vector<interface_stats> dummy;
    client->Subscribe(temp_args, cb, dummy);
    return 0;
}

int get_interface_stats(std::string key, std::vector<interface_stats>& ret_vector, ChangeableArguments requirements)
{
    // Set mode to 1 for ONCE case
    ArgumentsPassed temp_args = stream_args(key,1, requirements);
    client->Subscribe(temp_args, NULL, ret_vector);
    return 0;
}

int main(int argc, char** argv) {
    // Change this to fit your server ip and port
    std::string server_ip = "111.111.111.111";
    std::string server_port= "11111";

    // If using unix socket:
    // server_ip = unix;
    // server_port = <location_of_socket_file>;

    std::string server_address = server_ip + ":" + server_port;
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    client = new GNMIClient(channel);

    std::vector<interface_stats> test;
    ChangeableArguments temp;
    // Provide correct username and password
    temp.username = "username";
    temp.password = "password";
    temp.sample_interval_sec *= 10;
    temp.encoding = gnmi::Encoding::PROTO;

    int option_char;
    int rc = 0;

    // Example tests:
    while ((option_char = getopt(argc, argv, "abcd")) != -1) {
        switch (option_char) {
            case 'a':
                temp.encoding = gnmi::Encoding::PROTO;
                rc = get_interface_stats("MgmtEth0/RP0/CPU0/0", test, temp);
                test_once_func(test);
                break;
            case 'b':
                temp.encoding = gnmi::Encoding::JSON_IETF;
                rc = get_interface_stats("*", test, temp);
                test_once_func(test);
                break;
            case 'c':
                temp.encoding = gnmi::Encoding::PROTO;
                rc = get_interface_stats("MgmtEth0/RP0/CPU0/0", &test_callback_func,temp);
                break;
            case 'd':
                temp.encoding = gnmi::Encoding::JSON_IETF;
                rc = get_interface_stats("*", &test_callback_func,temp);
                break;
            default:
                fprintf (stderr, "usage: %s -a (mode once case) or -b (mode once * case) or -c (stream case) -d (stream * case)\n", argv[0]);
                return 1;
        }
    }
    // Testing the Once Case
    // int rc = get_interface_stats("MgmtEth0/RP0/CPU0/0", test, temp);
    // test_once_func(test);
    
    // Testing the Stream
    // int rc = get_interface_stats("*", &test_callback_func,temp);

    return 0;
}