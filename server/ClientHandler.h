#pragma once
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class ClientHandler {
public:
    ClientHandler(SOCKET client_socket) : socket_(client_socket) {}

    void start() {
        // Создаем поток для обработки клиента
        std::thread(&ClientHandler::handle, this).detach();
    }

private:
    SOCKET socket_;

    void handle() {
        try {
            char buffer[1024];
            int bytes_received;

            while ((bytes_received = recv(socket_, buffer, sizeof(buffer), 0)) > 0) {
                // Отправляем обратно клиенту
                send(socket_, buffer, bytes_received, 0);
            }

            if (bytes_received == SOCKET_ERROR) {
                std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in handle_client: " << e.what() << std::endl;
        }

        // Закрытие соединения с клиентом
        closesocket(socket_);
    }
};
