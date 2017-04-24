/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketclient.cpp
 * @date    11.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "unixdomainsocketclient.h"
#include <string>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

#define BUFFER_SIZE 64
#ifndef PATH_MAX
#define PATH_MAX 108
#endif
#define UD_DELIMITER_CHAR char(0x04)

using namespace jsonrpc;
using namespace std;

UnixDomainSocketClient::UnixDomainSocketClient(const std::string& path)
: path(path)
{
}

UnixDomainSocketClient::~UnixDomainSocketClient()
{
}

void UnixDomainSocketClient::SendRPCMessage(const std::string& message, std::string& result) throw (JsonRpcException)
{
	sockaddr_un address;
	int socket_fd, nbytes;
	char buffer[BUFFER_SIZE];
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
        throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, "Could not create unix domain socket");
	}
    memset(&address, 0, sizeof(sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, PATH_MAX, "%s", this->path.c_str());

    if(connect(socket_fd, (struct sockaddr *) &address,  sizeof(sockaddr_un)) != 0)
    {
        throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, "Could not connect to: " + this->path);
    }

    const char* toSend = message.c_str();
    ssize_t bytesToWrite = message.size();
    do
    {
        ssize_t byteWritten = write(socket_fd, toSend, bytesToWrite);
        bytesToWrite -= byteWritten;
        toSend += byteWritten;
    } while(bytesToWrite > 0);
    char delim = UD_DELIMITER_CHAR;
    write(socket_fd, &delim, 1);

    do
    {
        nbytes = read(socket_fd, buffer, BUFFER_SIZE);
        result.append(buffer,nbytes);
    } while(result.find(UD_DELIMITER_CHAR) == string::npos);
    result.pop_back();
    close(socket_fd);
}
