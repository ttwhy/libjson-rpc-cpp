/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_connector_unixdomainsocket.cpp
 * @date    6/8/2015
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifdef UNIXDOMAINSOCKET_TESTING
#include <catch.hpp>
#include <jsonrpccpp/server/connectors/unixdomainsocketserver.h>
#include <jsonrpccpp/client/connectors/unixdomainsocketclient.h>
#include "mockclientconnectionhandler.h"

#include "checkexception.h"

using namespace jsonrpc;
using namespace std;

#define TEST_MODULE "[connector_unixdomainsocket]"

#define SOCKET_PATH "/tmp/jsonrpccpp-socket"

namespace testunixdomainsocketserver
{
    struct F {
            UnixDomainSocketServer server;
            UnixDomainSocketClient client;
            MockClientConnectionHandler handler;

            F() :
                server(SOCKET_PATH),
                client(SOCKET_PATH)
            {
                unlink(SOCKET_PATH);
                server.SetHandler(&handler);
                REQUIRE(server.StartListening());
            }
            ~F()
            {
                server.StopListening();
                unlink(SOCKET_PATH);
            }
    };

    bool check_exception1(JsonRpcException const&ex)
    {
        return ex.GetCode() == Errors::ERROR_CLIENT_CONNECTOR;
    }
}
using namespace testunixdomainsocketserver;

TEST_CASE_METHOD(F, "test_unixdomainsocket_success", TEST_MODULE)
{
    handler.response = "exampleresponse";
    handler.timeout = 100;
    string result;
    string request = "examplerequest";
    string expectedResult = "exampleresponse";

    client.SendRPCMessage(request, result);

    CHECK(handler.request == request);
    CHECK(result == expectedResult);
}

TEST_CASE_METHOD(F, "test_unixdomainsocket_long_post", TEST_MODULE)
{
    int mb = 2;
    unsigned long size = mb * 1023 * 1022;
    char* str = (char*) malloc(size * sizeof(char));
    if (str == NULL)
    {
        FAIL("Could not allocate enough memory for test");
    }
    for (unsigned long i=0; i < size; i++)
    {
        str[i] = (char)('a'+(i%26));
    }
    str[size-1] = '\0';

    handler.response = str;
    string response;
    client.SendRPCMessage(str, response);

    CHECK(handler.request.size() == strlen(str));
    CHECK(handler.request == str);
    CHECK(response == handler.response);
    CHECK(response.size() == strlen(str));

    free(str);
    server.StopListening();
}


TEST_CASE("test_unixdomainsocket_server_multiplestart", TEST_MODULE)
{
    UnixDomainSocketServer server(SOCKET_PATH);
    CHECK(server.StartListening() == true);
    CHECK(server.StartListening() == false);

    UnixDomainSocketServer server2(SOCKET_PATH);
    CHECK(server2.StartListening() == false);
    CHECK(server2.StopListening() == false);

    CHECK(server.StopListening() == true);

    unlink(SOCKET_PATH);
}

TEST_CASE("test_unixdomainsocket_client_invalid", TEST_MODULE)
{
    UnixDomainSocketClient client("tmp/someinvalidpath");
    string result;
    CHECK_EXCEPTION_TYPE(client.SendRPCMessage("foobar", result), JsonRpcException, check_exception1);
}

#endif
