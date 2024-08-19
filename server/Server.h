#pragma once

#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Линковка с библиотекой ws2_32.lib

class Server {
public:
    Server(uint16_t port) : port_(port), listen_socket_(INVALID_SOCKET) {}

    bool start() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
            return false;
        }

        listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) {
            std::cerr << "socket() failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

        if (bind(listen_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen() failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        std::cout << "Server is running on port " << port_ << "..." << std::endl;
        accept_connections();
        return true;
    }

private:
    uint16_t port_;
    SOCKET listen_socket_;

    void accept_connections() {
        while (true) {
            sockaddr_in client_addr;
            int client_addr_size = sizeof(client_addr);
            SOCKET client_socket = accept(listen_socket_, (struct sockaddr*)&client_addr, &client_addr_size);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
                continue;
            }

            // Преобразование IP-адреса клиента в строку
            char client_ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) != nullptr) {
                std::cout << "Client connected from: " << client_ip << std::endl;
            }
            else {
                std::cerr << "inet_ntop() failed with error: " << WSAGetLastError() << std::endl;
            }

        }
    }
};

