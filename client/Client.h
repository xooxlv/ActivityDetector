#pragma once
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class TCPClient {
public:
    TCPClient(const std::string& host, int port);

    ~TCPClient();

    void connect();
     
    void sendMessage(const std::string& message);
    void sendMessage(char* data, ULONG64 size);

    std::string receiveMessage();

private:
    std::string host_;
    int port_;
    SOCKET socket_;
};