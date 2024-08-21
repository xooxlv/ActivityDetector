#include "OS.h"
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")


using namespace std;

string OS::getLastUserActivityTime()
{
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

string OS::getHostIp()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return "";
    }

    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
        WSACleanup();
        return "";
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addrInfo = nullptr;
    addrinfo* p = nullptr;
    std::string ipAddress;

    if (getaddrinfo(hostName, nullptr, &hints, &addrInfo) != 0) {
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

string OS::getPCName()
{
    char pcName[200];
    unsigned long pcNameLen = 200;
    GetComputerNameA(pcName, &pcNameLen);
    return string(pcName);
}

string OS::getPCDomain()
{
    CHAR domain[MAX_PATH];
    DWORD size = MAX_PATH;
    string domainName = "";
    if (GetComputerNameExA(ComputerNameDnsDomain, domain, &size))
        domainName = domain;
    return domainName;
}

string OS::getExecProgramPath()
{
    vector<wchar_t> pathBuffer(MAX_PATH);
    DWORD length = GetModuleFileName(nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));

    if (length == 0 || length == pathBuffer.size()) {
        return "";
    }

    return string(pathBuffer.begin(), pathBuffer.begin() + length);
}

void OS::addProgramToAutorun(string programName, string execPath)
{
    HKEY hKey;
    LONG result;
    wstring executablePath = wstring(execPath.begin(), execPath.end());

    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);

    result = RegSetValueEx(hKey,
        wstring(programName.begin(), programName.end()).c_str(),
        0,REG_SZ, reinterpret_cast<const BYTE*>(executablePath.c_str()),
        (executablePath.size() + 1) * sizeof(wchar_t));

    RegCloseKey(hKey);
}
