/**
    @file ConnectionRemote.cpp
    @author Lime Microsystems
    @brief Implementation of EVB7 connection of serial COM port.
*/

#include "ConnectionRemote.h"
#include <string>
#include "string.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include "FPGA_common.h"
#ifdef __unix__

#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#include <Winsock.h>
#endif // LINUX

using namespace std;
using namespace lime;

ConnectionRemote::ConnectionRemote(const char *comName)
{
    CloseRemote();
    remoteIP = std::string(comName);
    socketFd = -1;
#ifndef __unix__
    WSADATA wsaData;
    if( int err = WSAStartup(0x0202, &wsaData))
        printf("WSAStartup error %i\n", err);
#endif
}

ConnectionRemote::~ConnectionRemote(void)
{
    Close();
#ifndef __unix__
    WSACleanup();
#endif
}

void ConnectionRemote::Close(void)
{
    if(socketFd)
    {
#ifndef __unix__
        //shutdown(socketFd, SD_BOTH);
        closesocket(socketFd);
#else
        shutdown(socketFd, SHUT_RDWR);
        close(socketFd);
#endif
    }
    socketFd = -1;
}

bool ConnectionRemote::IsOpen(void)
{
    return true;
}

int ConnectionRemote::Open()
{
    if (socketFd < 0)
        return Connect(remoteIP.c_str(), 5000);
    return 0;
}

int ConnectionRemote::Connect(const char* ip, uint16_t port)
{
    struct sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(ip);
    clientService.sin_port = htons(port);

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd < 0)
    {
#ifndef __unix__
        printf("socket failed with error: %d\n", WSAGetLastError());
#else
        printf("socket failed with error %i", socketFd);
#endif

        return -1;
    }

    if(int result = connect(socketFd, (struct sockaddr*)&clientService, sizeof(clientService)) )
    {
#ifndef __unix__
        printf("connect failed with error: %d\n", WSAGetLastError());
        closesocket(socketFd);
#else
        printf("connect failed with error: %d\n", result);
        close(socketFd);
#endif
        return -1;
    }
    return 0;
}

int ConnectionRemote::TransferPacket(GenericPacket &pkt)
{
    int status;
    std::lock_guard<std::mutex> lock(mTransferLock);
    if (Open() == 0)
        status = LMS64CProtocol::TransferPacket(pkt);
    else
        status = -1;
    Close();
    return status;
}

int ConnectionRemote::Write(const unsigned char *data, int len, int timeout_ms)
{
    int bytesWritten = 0;
    while(bytesWritten < len)
    {
        int wrBytes = send(socketFd, (char*)data+bytesWritten, len-bytesWritten, 0);
        if(wrBytes < 0)
        {
            printf("Write failed\n");
            return 0;
        }
        bytesWritten += wrBytes;
        std::this_thread::yield();
    }
    return bytesWritten;
}

int ConnectionRemote::Read(unsigned char *response, int len, int timeout_ms)
{
    int bytesRead = 0;
    while(bytesRead < len)
    {
        int rdBytes = recv(socketFd, (char*)response+bytesRead, len-bytesRead, 0);
        if(rdBytes < 0)
        {
            printf("Read failed\n");
            return 0;
        }
        bytesRead += rdBytes;
        std::this_thread::yield();
    }
    return bytesRead;
}

int ConnectionRemote::GetBuffersCount() const
{
    return 1;
}
int ConnectionRemote::CheckStreamSize(int size) const
{
    return size;
}






