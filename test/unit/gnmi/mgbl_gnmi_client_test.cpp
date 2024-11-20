#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mgbl_api.h"
#include "mgbl_api_impl.h"

using namespace mgbl_api;
/*
 * Unit tests for gnmi_client_connection constructor
 *
 * gnmi_client_connection creates a channel which can be secured by ssl_tls
 * It connects to a Uri with credentials and a certificate or private key
 * if encryption is activated.
 *
 */

/*
 * We test if gnmi_client_connection throws an exception if the certificate is empty
 * and ssl encryption is true.
 */
TEST(GnmiClientTest, CheckSslCertificateEmpty)
{
    std::string empty_cert = "";
    EXPECT_THROW(gnmi_client_connection connection(rpc_channel_args{"", true, empty_cert, "", ""}),
                 std::invalid_argument);
}

/*
 * We test if gnmi_client_connection doesn't throws an exception if the certificate is not empty.
 */
TEST(GnmiClientTest, CheckSslCertificateValid)
{
    EXPECT_NO_THROW(
        gnmi_client_connection connection(rpc_channel_args{"", true, "a certificate", "", ""}));
}

/*
 * Unit tests for wait_for_grpc_server_connection
 *
 * wait_for_grpc_server_connection tries to establish a grpc connection with the server.
 * It will try for `retries` tries every `interval_between_retries`.
 *
 * If the connection is not established after `retries` tries then false is returned with a log
 * massage. Else true is returned is the connection is established.
 *
 */

/*
 * We test if the connection fails with a wrong uri.
 */
TEST(GnmiClientConnectionTest, WaitForConnectionRetries)
{
    rpc_channel_args args = {"invalid_uri", false, "", "", ""};
    gnmi_client_connection connection(args);

    bool connected = connection.wait_for_grpc_server_connection(3, std::chrono::seconds(1));
    EXPECT_FALSE(connected);
}