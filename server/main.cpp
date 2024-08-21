#include "Server.h"
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <commctrl.h>
#include <vector>
#include <thread>

#include "StringParser.h"

#pragma comment(lib, "comctl32.lib")
using namespace std;

Server serv(4444);      // сервер

HINSTANCE hInst;        // состояние окна
HWND hListView;         // список клиентов
HWND hButton;           // кнопка получения скриншота
HWND hWnd;              // основное окно
HWND hWndComboBox;      // выпадающий список клиентов для получения скриншота
HWND hwndChild;         // окно скриншота

int displayedClients = 0;                       // количество отображенных клиентов на listView
vector<ClientData>* allClientData = nullptr;    // ссылка на всех клиентов сервера
vector<string> pcNamesComboBox;                 // все клиенты на comboboxe

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateListView(HWND hWnd);
void CreateGetScreenshotControl(HWND hWnd);



void UpdateListView() {
    LVITEM lvi;

    lvi.mask = LVIF_TEXT;
    for (int i = displayedClients; i < (*allClientData).size(); i++) {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = StringParser::lpcstr_to_lpwstr((*allClientData)[i].ip.c_str());
        ListView_InsertItem(hListView, &lvi);
        ListView_SetItemText(hListView, i, 1, StringParser::lpcstr_to_lpwstr((*allClientData)[i].hostName.c_str()));
        ListView_SetItemText(hListView, i, 2, StringParser::lpcstr_to_lpwstr((*allClientData)[i].domain.c_str()));
        ListView_SetItemText(hListView, i, 3, StringParser::lpcstr_to_lpwstr((*allClientData)[i].lastActivityTime.c_str()));
        displayedClients++;
    }

    for (int i = 0; i < (*allClientData).size(); i++) {
        ListView_SetItemText(hListView, i, 0, StringParser::lpcstr_to_lpwstr((*allClientData)[i].ip.c_str()));
        ListView_SetItemText(hListView, i, 1, StringParser::lpcstr_to_lpwstr((*allClientData)[i].hostName.c_str()));
        ListView_SetItemText(hListView, i, 2, StringParser::lpcstr_to_lpwstr((*allClientData)[i].domain.c_str()));
        ListView_SetItemText(hListView, i, 3, StringParser::lpcstr_to_lpwstr((*allClientData)[i].lastActivityTime.c_str()));
    }
}

void UpdateComboBox() {
    for (auto v : (*allClientData)) {
        if (find(pcNamesComboBox.begin(), pcNamesComboBox.end(), v.hostName) == pcNamesComboBox.end()) {
            SendMessage(hWndComboBox, CB_ADDSTRING, 0, 
                (LPARAM)StringParser::lpcstr_to_lpwstr(v.hostName.c_str()));
            pcNamesComboBox.push_back(v.hostName);
        }
    }
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
        DestroyWindow(hWnd);
        //PostQuitMessage(0);
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

    hwndChild = CreateWindowEx(
        0,
        L"ChildWindowClass",
        L"Скриншот",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1250, 750, // Размер окна
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

            auto pcName = StringParser::wstr_to_str(text);
            serv.send_command(pcName, Command::GET_SCREENSHOT);
            allClientData = serv.get_all_clients_data();

            auto itr = find_if((*allClientData).begin(), (*allClientData).end(), [&pcName](ClientData cl) {
                return cl.hostName == pcName;
                });

            if (itr != (*allClientData).end()) {
                string path = itr->screenshotPath;
                if (path.empty()) {
                    MessageBox(hWnd, L"Клиент не в сети", L"Невозможно получить скриншот", MB_ICONERROR);
                    break;
                }
                system(path.c_str());
                break;
                HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
                CreateChildWindow(hInstance, hWnd, StringParser::lpcstr_to_lpwstr(path.c_str()));
            }
        }
        else if (wParam == 12) {
            UpdateListView();
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
            allClientData = serv.get_all_clients_data();
            for (auto cld : (*allClientData)) {
                serv.send_command(cld.hostName, Command::GET_STATE);
            }
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
    hWnd = CreateWindow(L"MYWINDOWCLASS", L"Сервер контроля", WS_OVERLAPPEDWINDOW,
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
    wcex.lpszClassName = L"MYWINDOWCLASS";
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
