/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketserver.cpp
 * @date    07.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "unixdomainsocketserver.h"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <jsonrpccpp/common/specificationparser.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <string>

using namespace jsonrpc;
using namespace std;

#define BUFFER_SIZE 1024
#ifndef PATH_MAX
#define PATH_MAX 108
#endif
#define UD_DELIMITER_CHAR char(0x04)

UnixDomainSocketServer::UnixDomainSocketServer(const string &socket_path) :
    running(false),
    socket_path(socket_path.substr(0, PATH_MAX))
{
}

bool UnixDomainSocketServer::StartListening()
{
    if(!this->running)
    {
        //Create and bind socket here.
        //Then launch the listenning loop.
        if (access(this->socket_path.c_str(), F_OK) != -1)
            return false;

        this->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

        if(this->socket_fd < 0)
        {
            return false;
        }

        unlink(this->socket_path.c_str());

        fcntl(this->socket_fd, F_SETFL, FNDELAY);

        /* start with a clean address structure */
        memset(&(this->address), 0, sizeof(struct sockaddr_un));

        this->address.sun_family = AF_UNIX;
        snprintf(this->address.sun_path, PATH_MAX, "%s", this->socket_path.c_str());

        if(bind(this->socket_fd, reinterpret_cast<struct sockaddr *>(&(this->address)), sizeof(struct sockaddr_un)) != 0)
        {
            return false;
        }

        if(listen(this->socket_fd, 5) != 0)
        {
            return false;
        }

        //Launch listening loop there
        this->running = true;
        int ret = pthread_create(&(this->listenning_thread), NULL, UnixDomainSocketServer::LaunchLoop, this);
        if(ret != 0)
        {
            pthread_detach(this->listenning_thread);
        }
        this->running = static_cast<bool>(ret==0);

        return this->running;
    }
    else
    {
        return false;
    }
}

bool UnixDomainSocketServer::StopListening()
{
    if(this->running)
    {
        this->running = false;
        pthread_join(this->listenning_thread, NULL);
        close(this->socket_fd);
        unlink(this->socket_path.c_str());
        return !(this->running);
    }
    else
    {
        return false;
    }
}

bool UnixDomainSocketServer::SendResponse(const string& response, void* addInfo)
{
    bool result = false;
    int connection_fd = reinterpret_cast<intptr_t>(addInfo);
    result = this->WriteToSocket(connection_fd, response);
    close(connection_fd);
    return result;
}

void* UnixDomainSocketServer::LaunchLoop(void *p_data)
{
    pthread_detach(pthread_self());
    UnixDomainSocketServer *instance = reinterpret_cast<UnixDomainSocketServer*>(p_data);;
    instance->ListenLoop();
    return NULL;
}

void UnixDomainSocketServer::ListenLoop()
{
    int connection_fd;
    socklen_t address_length = sizeof(this->address);
    while(this->running)
    {
        if((connection_fd = accept(this->socket_fd, reinterpret_cast<struct sockaddr *>(&(this->address)),  &address_length)) > 0)
        {
            pthread_t client_thread;
            struct ClientConnection *params = new struct ClientConnection();
            params->instance = this;
            params->connection_fd = connection_fd;
            int ret = pthread_create(&client_thread, NULL, UnixDomainSocketServer::GenerateResponse, params);
            if(ret != 0)
            {
                pthread_detach(client_thread);
                delete params;
                params = NULL;
            }
        }
        else
        {
            usleep(25000);
        }
    }
}

void* UnixDomainSocketServer::GenerateResponse(void *p_data)
{
    pthread_detach(pthread_self());
    struct ClientConnection* params = reinterpret_cast<struct ClientConnection*>(p_data);
    UnixDomainSocketServer *instance = params->instance;
    int connection_fd = params->connection_fd;
    delete params;
    params = NULL;
    int nbytes;
    char buffer[BUFFER_SIZE];
    string request;
    do
    { //The client sends its json formatted request and a delimiter request.
        nbytes = read(connection_fd, buffer, BUFFER_SIZE);
        request.append(buffer,nbytes);
    } while(request.find(UD_DELIMITER_CHAR) == string::npos);
    request.pop_back();
    instance->OnRequest(request, reinterpret_cast<void*>(connection_fd));
    return NULL;
}


bool UnixDomainSocketServer::WriteToSocket(int fd, const string& toWrite)
{
    const char* data = toWrite.c_str();
    ssize_t bytesToWrite = toWrite.size();
    ssize_t byteWritten = 0;
    do
    {
        byteWritten = write(fd, data, bytesToWrite);
        if(byteWritten > 0)
        {
            bytesToWrite -= byteWritten;
            data += byteWritten;
        }
    } while(bytesToWrite > 0 && byteWritten > 0);
    char delim = UD_DELIMITER_CHAR;
    write(fd,&delim,1);
    return bytesToWrite == 0;
}
