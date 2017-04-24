/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    filedescriptorclient.cpp
 * @date    26.10.2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "filedescriptorclient.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#define BUFFER_SIZE 1024
#define MAX_WRITE_SIZE 1024
#define FD_DELIMITER_CHAR char(0x04)

using namespace jsonrpc;
using namespace std;

FileDescriptorClient::FileDescriptorClient(int inputfd, int outputfd) :
  inputfd(inputfd), outputfd(outputfd)
{
}

FileDescriptorClient::~FileDescriptorClient()
{
}

void FileDescriptorClient::SendRPCMessage(const std::string& message,
  std::string& result) throw (JsonRpcException)
{

    this->writeMessage(message);


    if (!IsReadable(inputfd))
        throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
            "The input file descriptor is not readable");

    this->readMessage(result);
}

bool FileDescriptorClient::IsReadable(int fd)
{
  int o_accmode = 0;
  int ret = fcntl(fd, F_GETFL, &o_accmode);
  if (ret == -1)
    return false;
  return ((o_accmode & O_ACCMODE) == O_RDONLY ||
          (o_accmode & O_ACCMODE) == O_RDWR);
}

bool FileDescriptorClient::writeMessage(const string &message)
{
    const char* toSend = message.c_str();
    size_t messageSize = message.size();
    do
    {
        ssize_t byteWritten = write(outputfd, toSend, min(messageSize, (size_t)MAX_WRITE_SIZE));
        if (byteWritten < 1)
            throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                "Unknown error occured while writing to the output file descriptor");

        messageSize -= byteWritten;
        toSend = toSend + byteWritten;
    } while(messageSize > 0);
    char delim = FD_DELIMITER_CHAR;
    write(outputfd, &delim, 1);
    return true;
}

bool FileDescriptorClient::readMessage(string &message)
{
    ssize_t nbytes = 0;
    char buffer[BUFFER_SIZE];
    do
    {
      nbytes = read(inputfd, buffer, BUFFER_SIZE);
      message.append(buffer, nbytes);
    } while(message.find(FD_DELIMITER_CHAR) == string::npos);
    message.pop_back();
    return nbytes > 0;
}
