#include "Client.h"

TCPClient::TCPClient(const std::string& host, int port)
    : host_(host), port_(port), socket_(INVALID_SOCKET) {
    // Инициализация WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации WinSock." << std::endl;
        throw std::runtime_error("WSAStartup failed");
    }
}

TCPClient::~TCPClient()
{
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
    }
    WSACleanup();
}

void TCPClient::connect()
{
    // Создание сокета
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета." << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    // Настройка адреса сервера
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_);

    // Использование inet_pton для преобразования IP-адреса
    if (inet_pton(AF_INET, host_.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "Ошибка преобразования IP-адреса." << std::endl;
        throw std::runtime_error("inet_pton failed");
    }

    // Подключение к серверу
    if (::connect(socket_, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Ошибка подключения к серверу." << std::endl;
        throw std::runtime_error("Connection failed");
    }
    std::cout << "Соединение с сервером установлено." << std::endl;
}
void TCPClient::sendMessage(const std::string& message)
{
    if (send(socket_, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Ошибка отправки сообщения." << std::endl;
        throw std::runtime_error("Send failed");
    }
    std::cout << "Сообщение отправлено." << std::endl;
}

void TCPClient::sendMessage(char* data, ULONG64 size)
{
    if (send(socket_, data, size, 0) == SOCKET_ERROR) {
        std::cerr << "Ошибка отправки сообщения." << std::endl;
        throw std::runtime_error("Send failed");
    }
    std::cout << "Сообщение отправлено." << std::endl;
}

std::string TCPClient::receiveMessage()
{
    const int bufferSize = 1024;
    char buffer[bufferSize];
    int received = recv(socket_, buffer, bufferSize - 1, 0);
    if (received == SOCKET_ERROR) {
        std::cerr << "Ошибка получения сообщения." << std::endl;
        throw std::runtime_error("Receive failed");
    }
    buffer[received] = '\0';  // Добавление нуль-терминатора
    return std::string(buffer);
}

