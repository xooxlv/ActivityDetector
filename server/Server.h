#pragma once

#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <vector>
#include <ws2tcpip.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <mutex>

using namespace std;

#pragma comment(lib, "ws2_32.lib") // Линковка с библиотекой ws2_32.lib

enum Command {
    GET_STATE,
    GET_SCREENSHOT
};

struct ClientData {
    SOCKET sock = 0;

    string ip;
    string hostName;
    string domain;
    string lastActivityTime;
    string screenshotPath;
};

class Server {
private:
    uint16_t port_;
    SOCKET listen_socket_;
    vector<ClientData> cld;
    int generator = 1;
    mutex mtx;
    ClientData parse_client_data(const std::string& input, SOCKET sock) {
        ClientData clientData;
        std::istringstream stream(input);
        std::string line;

        std::unordered_map<std::string, std::string*> fieldMap = {
            {"hostname:", &clientData.hostName},
            {"domain:", &clientData.domain},
            {"last_activ_time:", &clientData.lastActivityTime},
            {"ip:", &clientData.ip}
        };

        while (std::getline(stream, line)) {
            size_t delimiterPos = line.find(':');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos + 1);
                std::string value = line.substr(delimiterPos + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                if (fieldMap.find(key) != fieldMap.end()) {
                    *(fieldMap[key]) = value;
                }
            }
        }

        clientData.sock = sock;
        return clientData;
    }

    void saveBmp(string fileName, char* data, int size) {
        std::ofstream outfile(fileName, std::ios::binary);

        if (!outfile) {
            std::cerr << "Не удалось открыть файл для записи." << std::endl;
        }

        outfile.write(data, size);

        outfile.close();

        if (outfile.fail()) {
            std::cerr << "Ошибка при закрытии файла." << std::endl;
        }
    }

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
                send(client_socket, "GET_STATE", 10, 0);

                char buff[1024] = {};
                recv(client_socket, buff, 1024, 0);


                auto data = parse_client_data(string(buff), client_socket);
                auto itr = find_if(cld.begin(), cld.end(), [&data](ClientData c) {
                    return data.hostName == c.hostName; });
                if (itr == cld.end())
                    cld.push_back(data);
                else *itr = data;


            }
            else {
                std::cerr << "inet_ntop() failed with error: " << WSAGetLastError() << std::endl;
            }


        }
    }
public:
    Server(uint16_t port) : port_(port), listen_socket_(INVALID_SOCKET) {}

    bool start() {
        thread *th = new thread([this]() {
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
            });

        th->detach();
        return true;
    }

    const auto get_all_clients_data() {
        return &this->cld;
    }

    void send_command(string host, Command cmd) {
        unique_lock<mutex> lock(mtx);

        auto itr = find_if(cld.begin(), cld.end(), [&host](ClientData c) {
            return host == c.hostName; });

        if (itr == cld.end()) {
            return;
        }

        SOCKET client = (*itr).sock;

        if (cmd == Command::GET_SCREENSHOT) {
            char* bmp = nullptr;
            try {
                send(client, "GET_SCREENSHOT", 15, 0);
                char response[1024] = { 0 };
                recv(client, response, 1024, 0);
                int bmpSize = atoi(response);
                if (bmpSize == 0) {
                    throw exception("BMP size == 0");
                }
                cout << bmpSize << endl;
                send(client, response, strlen(response), 0);
                bmp = new char[bmpSize] {0};
                recv(client, bmp, bmpSize, 0);
                auto screenshot_path = to_string(generator++) + "err.bmp";
                saveBmp(screenshot_path, bmp, bmpSize);
                (*itr).screenshotPath = screenshot_path;
                delete[] bmp;
            }
            catch (exception ex){
                if (bmp != nullptr)
                    delete[] bmp;
                (*itr).screenshotPath = "";
            }


        }
        else if (cmd == Command::GET_STATE) {
            send(client, "GET_STATE", 10, 0);
            char buf[1024] = {};
            recv(client, buf, 1024, 0);
            auto parsed = this->parse_client_data(buf, client);
            if (parsed.hostName.length() > 0) {
                (*itr) = parsed;
            }
        }
    }
};

