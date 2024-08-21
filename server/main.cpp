#include "Server.h"
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <commctrl.h>
#include <vector>
#include <thread>

#pragma comment(lib, "comctl32.lib")
using namespace std;

Server serv(4444);

HINSTANCE hInst;
LPCWSTR szTitle = L"Мое Графическое Окно";
LPCWSTR szWindowClass = L"MYWINDOWCLASS";

HWND hListView;
HWND hButton;
HWND hWnd;
HWND hWndComboBox;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateListView(HWND hWnd);
void CreateGetScreenshotControl(HWND hWnd);

LVITEM lvi;

LPWSTR strToLpwstr(string s) {
    auto ws = wstring(s.begin(), s.end());
    WCHAR* cws = new WCHAR[100];
    lstrcpyW(cws, ws.c_str());
    return cws;
}

int old_count = 0;
vector<ClientData> doo;
void UpdateListView(vector<ClientData> cld) {
    lvi.mask = LVIF_TEXT;
    for (int i = old_count; i < cld.size(); i++) {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = strToLpwstr(cld[i].ip);
        ListView_InsertItem(hListView, &lvi);
        ListView_SetItemText(hListView, i, 1, strToLpwstr(cld[i].hostName));
        ListView_SetItemText(hListView, i, 2, strToLpwstr(cld[i].domain));
        ListView_SetItemText(hListView, i, 3, strToLpwstr(cld[i].lastActivityTime));
        old_count++;
    }

    for (int i = 0; i < cld.size(); i++) {
        ListView_SetItemText(hListView, i, 0, strToLpwstr(cld[i].ip));
        ListView_SetItemText(hListView, i, 1, strToLpwstr(cld[i].hostName));
        ListView_SetItemText(hListView, i, 2, strToLpwstr(cld[i].domain));
        ListView_SetItemText(hListView, i, 3, strToLpwstr(cld[i].lastActivityTime));
    }
}

vector<string> incb;
void UpdateComboBox() {
    for (auto& v : doo) {
        if (find(incb.begin(), incb.end(), v.hostName) == incb.end()) {
            SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)strToLpwstr(v.hostName));
            incb.push_back(v.hostName);
        }
    }
}

std::string wcharToString(const wchar_t* wchar_str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wchar_str);
}

void DrawBmpImage(HWND hwnd, LPCWSTR bmpFilePath) {
    HDC hdc;
    HDC hdcMem;
    HBITMAP hBitmap;
    BITMAP bitmap;

    hBitmap = (HBITMAP)LoadImage(NULL, bmpFilePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBitmap == NULL) {
        MessageBox(hwnd, L"Ошибка загрузки изображения!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    hdc = GetDC(hwnd);
    hdcMem = CreateCompatibleDC(hdc);

    if (!hdc || !hdcMem) {
        MessageBox(hwnd, L"Ошибка создания контекста устройства", L"Ошибка", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);

    int targetWidth = 1200;
    int targetHeight = 700;

    if (!StretchBlt(hdc, 0, 0, targetWidth, targetHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY)) {
        MessageBox(hwnd, L"Ошибка StretchBlt!", L"Ошибка", MB_OK | MB_ICONERROR);
    }

    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdc);
    DeleteObject(hBitmap);
}


LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps = {}; // Инициализация структуры PAINTSTRUCT
        HDC hdc = BeginPaint(hWnd, &ps);
        if (hdc) { // Проверка на успешное получение контекста устройства
            LPCWSTR bmpFilePath = (LPCWSTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (bmpFilePath) {
                DrawBmpImage(hWnd, bmpFilePath);
            }
            EndPaint(hWnd, &ps);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


void CreateChildWindow(HINSTANCE hInstance, HWND parentHwnd, LPCWSTR bmpFilePath) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = ChildWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ChildWindowClass";

    RegisterClass(&wc);

    HWND hwndChild = CreateWindowEx(
        0,
        L"ChildWindowClass",
        L"Скриншот",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1300, 800, // Размер окна
        parentHwnd, NULL, hInstance, NULL
    );

    SetWindowLongPtr(hwndChild, GWLP_USERDATA, (LONG_PTR)bmpFilePath);
    ShowWindow(hwndChild, SW_SHOW);
    UpdateWindow(hwndChild);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateListView(hWnd);
        CreateGetScreenshotControl(hWnd);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            wchar_t text[256] = {};
            GetDlgItemText(hWnd, 2, text, 256);

            auto pcName = wcharToString(text);
            serv.send_command(pcName, Command::GET_SCREENSHOT);
            auto dd = serv.get_all_clients_data();
            auto itr = find_if(dd.begin(), dd.end(), [&pcName](ClientData cl) {
                return cl.hostName == pcName;
                });

            if (itr != dd.end()) {
                string path = itr->screenshotPath;
                HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
                CreateChildWindow(hInstance, hWnd, strToLpwstr(path));
            }
        }
        else if (wParam == 12) {
            UpdateListView(doo);
            UpdateComboBox();
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    serv.start();

    thread recv_new_stat([&]() {
        while (true) {
            Sleep(1000);
            auto client_data = serv.get_all_clients_data();
            for (auto cld : client_data) {
                serv.send_command(cld.hostName, Command::GET_STATE);
            }
            doo = client_data;
            SendMessage(hWnd, WM_COMMAND, 12, 0);
        }
        });
    recv_new_stat.detach();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    return RegisterClassEx(&wcex);
}

void CreateListView(HWND hWnd) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    hListView = CreateWindow(WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | WS_DISABLED | WS_BORDER,
        50, 10, 700, 200, hWnd, nullptr, hInst, nullptr);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 150;

    lvc.pszText = const_cast<LPWSTR>(L"IP");
    ListView_InsertColumn(hListView, 0, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Имя компьютера");
    ListView_InsertColumn(hListView, 1, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Домен");
    ListView_InsertColumn(hListView, 2, &lvc);

    lvc.cx = 250;
    lvc.pszText = const_cast<LPWSTR>(L"Время последней активности");
    ListView_InsertColumn(hListView, 3, &lvc);
}

void CreateGetScreenshotControl(HWND hWnd) {
    hWndComboBox = CreateWindowEx(0, WC_COMBOBOX, NULL,
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        800, 10, 200, 75, hWnd, (HMENU)2, hInst, NULL);

    hButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Запросить скриншот",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        800,         // x position 
        50,         // y position 
        200,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        HMENU(1),       // No menu.
        hInst,
        NULL);
}
