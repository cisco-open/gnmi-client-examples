# Cpp GNMI Tutorial

## Table of Contents
- [Explanation of Content](#start)
- [Server Setup](#server)
- [How to Run The Function and Explanation](#quick)
- [Customizing get_interface_stats](#custom)

#### <a name='start'></a>Explanation of Content

A function will be provided, that demonstrates how to use a subscribe request through gRPC Network Management Interface (gNMI) in order to retrieve interface counter statistics with open-config models. Said function is called get_interface_stats.

NOTE: If you only want to be able to run the code then you only need to follow the Server Setup and How to Run the Function and Explanation sections.
#### <a name='server'></a>Server Setup

On the server side, we need to configure GRPC and enable the service layer through the following CLI configuration:
    ! Configure GRPC
    configure
    grpc port #portnumber
    grpc no-tls
    commit
    end

#### <a name='quick'></a>How to Run the Function and Explanation

The function is called get_interface_stats and to use it, all you have to do is call it.

It is an overloaded function that allows you to get the counters of an interface for streaming/polling and stream only-once purposes.
The path of the counter statistics will always be:  
    "interfaces/interface[name=" + interface_name + "]/state/counters/"

Other arguments which are hardcoded into the function but can be changed by the user. More information found here: [Customizing get_interface_stats](#custom)

And more information on this can be provided here:  
https://github.com/openconfig/public/blob/master/release/models/interfaces/openconfig-interfaces.yang

The interface counters this function provides the stats for are:  
in-pkts  
out-pkts  
in-octets  
out-octets  
in-multicast-pkts  
out-multicast-pkts  
in-broadcast-pkts  
out-broadcast-pkts  
in-unicast-pkts  
out-unicast-pkts  
in-errors  
out-errors  
in-unknown-protos  
in-fcs-errors  
in-discards  
out-discards

Now to go over an example:

1. User needs to change the server_ip, server_port, username, and password in main.
    ```sh
        std::string server_ip = "111.111.111.111";
        std::string server_port= "11111";
        temp.username = "username";
        temp.password = "password";
    ```
2. Rebuild in docker enviornment
    ```sh
        $ cd src/
        $ make
    ```
3. Run the examples provided or create your own
    ```sh
        $./gnmi_client -a
        $./gnmi_client -b
        $./gnmi_client -c
        $./gnmi_client -d
    ```

##### Explanation of overloaded functions:

1. get_interface_stats(std::string key, cb_fn_type cb, ChangeableArguments requirements):

    This function requires you to provide a string key (which would be your interface name or "\*" for all interfaces), and a callback_function. 
    This callback function can be anything of your choosing. 
    When a response is received by the client from the server and the response includes the counter stats, your callback function will be run. 
    Your callback function needs to take in the argument interface_stats. 
    interface_stats is a provided struct made for returning the stats of a specific interface. 
    For this function, a response is received for each interface. 
    Thus, if you provide "\*" as the key, indicating you want stats of every interface, then you will get an individual response for every interface and your callback function will be run for every response recieved.  
    ChangeableArguments is struct you need to pass in, in order to give necessary information to subscribeRequest. More info found in Customizing get_interface_stats section  

2. get_interface_stats(std::string key, std::vector<interface_stats>& test, ChangeableArguments requirements):

    This function requires you to provide a string key (which would be your interface name or "\*" for all interfaces), and an vector of interface_stats type. 
    interface_stats is a provided struct made for returning the interface stats. 
    This funcution will push_back a new interface_stats entry in the vector you provided. 
    Each element in the vector represents the stats of a specific interface. Thus, if you provide "\*" as the key, indicating you want stats of every interface, then you will get an separate entry in your vector for each interface.  
    ChangeableArguments is struct you need to pass in, in order to give necessary information to subscribeRequest. More info found in Customizing get_interface_stats section  

##### Example of overloaded functions:

1. Example of STREAM option:  

```
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
    ChangeableArguments temp;
    temp.username = "username";
    temp.password = "password";
    temp.sample_interval_sec *= 10;
    temp.encoding = gnmi::Encoding::PROTO;
    int rc = get_interface_stats("FourHundred0/0/0/0", &test_callback_func, temp);
```

2. Example of only-once Stream:  

```
    ChangeableArguments temp;
    temp.username = "username";
    temp.password = "password";
    temp.sample_interval_sec *= 10;
    temp.encoding = gnmi::Encoding::JSON_IETF;
    std::vector<interface_stats> test;  
    int rc =  get_interface_stats("*", test, temp);  
    print_function(temp);
```

##### Choosing Encoding Method:

There exists support for two encoding methods. JSON_IETF and PROTOBUF.
Here are the statistics of how long each method actually takes.

1. Streaming Test(end to end): PROTOBUF Encoding vs JSON_IETF Encoding Average of 40 tests. The tests are denoted in seconds.  
First row indicates milliseconds it takes from running get_interface_stats(std::string key, cb_fn_type cb) to first callback function being run.  
Note: Callback function made to do nothing.  
Every row after indicates from callback function being run to next callback function. This is to show decoding time.  
This is tested with only one interface

| Protobuf(millisec)  | Json_ietf (millisec) | Protobuf-Json_ietf (millisec) |
| --- | --- | --- |
|      2.5249       |      2.49725      |      0.0277       |
|      1.291533     |      1.33663      |      -0.0451      |
|      1.057        |      1.0838       |      -0.0268      |
|      1.045533     |      1.10547      |      -0.0599      |
|      1.191767     |      0.89323      |      0.29853      |
|      1.1471       |      1.02457      |      0.12253      |
|      1.051533     |      0.91833      |      0.1332       |
|      0.9542       |      0.97067      |      -0.0165      |
|      0.967567     |      0.89703      |      0.0705       |
|      0.9036       |      0.8818       |      0.0218       |

2. Streaming Test(Interpretation of responses): PROTOBUF Encoding vs JSON_IETF Encoding Average of 40 tests. The tests are denoted in milliseconds.  
Each row indicates how many milliseconds it takes for the interpretation of responses and getting that data into the interface_stats structs for a continuous stream.
This is test with only one interface

| Protobuf(millisec)  | Json_ietf (millisec) | Protobuf-Json_ietf (millisec) |
| --- | --- | --- |
|      0.01367      |      0.2365       |      -0.22283      |
|      0.128        |      0.152        |      -0.024        |
|      0.0819       |      0.1467       |      -0.0648       |
|      0.1064       |      0.1546       |      -0.0482       |
|      0.0927       |      0.1399       |      -0.0472       |
|      0.1235       |      0.1142       |      -0.0093       |
|      0.142        |      0.155        |      -0.013        |
|      0.1555       |      0.1669       |      -0.0114       |
|      0.1133       |      0.1484       |      -0.0351       |
|      0.1053       |      0.1314       |      -0.0261       |


3. Stream-ONCE Test (End to End): PROTOBUF Encoding vs JSON_IETF Encoding. Each row is Average of 100 tests. The tests are denoted in milliseconds.  
Each row indicates how many milliseconds it takes from running get_interface_stats(std::string key, std::vector<interface_stats>& test) to it finishing.  
This is tested with only one interface

| Protobuf(millisec)  | Json_ietf (millisec) | Protobuf-Json_ietf (millisec) |
| --- | --- | --- |
|      1.8493       |      2.1323       |      -0.2833       |
|      1.8651       |      1.4906       |      0.3745        |
|      2.2228       |      1.7803       |      0.4425        |
|      2.2017       |      1.6909       |      0.5108        |

4. Stream-ONCE Test (interpretation of responses): PROTOBUF Encoding vs JSON_IETF Encoding.  
Each row is Average of 100 tests. The tests are denoted in milliseconds.  
Each row indicates how many milliseconds it takes for the interpretation of responses and getting that data into the interface_stats structs.  
This is test with only one interface

| Protobuf(millisec)  | Json_ietf (millisec) | Protobuf-Json_ietf (millisec) |
| --- | --- | --- |
|      0.11251      |      0.17604      |      -0.06353      |
|      0.09652      |      0.16368      |      -0.06716      |
|      0.10548      |      0.15892      |      -0.05344      |
|      0.1199       |      0.15112      |      -0.03122      |

5. Explaning the test results:  
In both tests, one can see that using JSON_IETF vs PROTOBUF Encoding produced similar results. Both took about the same time from End-to-End. Sometimes protobuf is faster and other times json_ietf is faster. But for Decoding and interpretation of responses, proto is consistently faster. This is because:

    - Protobuf: One can consider the response from the server a struct which has other structs that conatins the data needed. Because of how the proto messages are structed, there are multiple update entries in that one notification response received from the server. Where each update entry would correspond to each counter. Each update entry is parsed, so that each counter is updated with it's corresponding entry. This is check by the gnmi::Path provided by the update entry, which is parsed through. It would be terrible if one denoted in-pkts to equal out-pkts. Even though the path is parsed through, it remains the only thing parsed, and the data values are stored as their original types and no conversion is needed. Thus, it appears as though the response message is what causes the end-to-end case to be slower (about half the time) but interpolation of response is faster (all the time) using protobuf encoding.

    - Json_ietf: When a resposne is received from the server, it returns all counters and their values within a json_ietf object. The json object is parsed, in order to get the counter values. But, but this is only done once per response. No multiple update entries with their own path to parse through. Even so, the interpolation of the responses is slower for json_ietf since the json object needs to be converted to a string and then each counter needs to be converted to their specific type for the interface_stats struct.

#### <a name='custom'></a>Customizing get_interface_stats

Within the get_interface_stats function a client->subscribe request is ran, which takes in three arguments.  
GNMIClient::Subscribe(ArgumentsPassed info, cb_fn_type cb, std::vector<interface_stats>& stats);  

This Subscribe function is called within the the get_interface_stats function

1. The ArgumentsPassed is a struct which contains information regarding the Subscribe request.  
This is a provided function that populates the ArgumentsPassed struct which will be passed into the Subscribe Request.

```
    ArgumentsPassed stream_args(std::string key, int mode,ChangeableArguments requirements)
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
        // Mode: 0 = STREAM, 1 = ONCE, 2 = POLL
        temp_args.mode = mode;
        return temp_args;
    }
```
As you can see this function requires the ChangeableArguments struct. This struct only composes of four variables which need to be set by you  
and passed into get_interface_stats.
```
    class ChangeableArguments{
        public:
            std::string      username;
            std::string      password;

            // multiply by 1000000000 since this parameter is in nanoseconds thus default is 1 second
            uint64_t         sample_interval_sec = 1000000000;
            // Default is set to gnmi::Encoding::PROTO = 0
            gnmi::Encoding   encoding = gnmi::Encoding::PROTO;
    };
```

2. The callback function is there if you want a callback function to run everytime a response is received. In the case of stream once-only, you can pass NULL for this.

3. This parameter is for the stream once-only case and you can provide an empty struct here in the overloaded get_interface_stats function if it for Stream case.