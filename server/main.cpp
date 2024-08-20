#include "Server.h"
#include <iostream>
#include <string>

#include <commctrl.h>
#include <vector>
#pragma comment(lib, "comctl32.lib")
using namespace std;

HINSTANCE hInst;
LPCWSTR szTitle = L"Мое Графическое Окно";
LPCWSTR szWindowClass = L"MYWINDOWCLASS";

HWND hListView;
HWND hButton;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateListView(HWND hWnd);


LVITEM lvi;

LPWSTR strToLpwstr(string s) {
    auto ws = wstring(s.begin(), s.end());
    WCHAR cws[100] = {};
    lstrcpyW(cws, ws.c_str());
    return cws;
}

void UpdateListView(HWND hWnd, vector<ClientData>& cld) {

   
    lvi.mask = LVIF_TEXT;
    for (int i = 0; i < cld.size(); i++) {
        ListView_InsertItem(hListView, strToLpwstr(cld[i].ip));
        ListView_SetItemText(hListView, i, 0, strToLpwstr(cld[i].ip));
        ListView_SetItemText(hListView, i, 1, strToLpwstr(cld[i].hostName));
        ListView_SetItemText(hListView, i, 2, strToLpwstr(cld[i].domain));
        ListView_SetItemText(hListView, i, 3, strToLpwstr(cld[i].lastActivityTime));
    }

    //// Первая строка
    //lvi.iItem = 0; // индекс строки
    //lvi.iSubItem = 0; // индекс столбца
    //lvi.pszText = const_cast<LPWSTR>(L"192.168.1.1");
  

    //// Вторая строка
    //lvi.iItem = 1; // индекс строки
    //lvi.iSubItem = 0; // индекс столбца
    //lvi.pszText = const_cast<LPWSTR>(L"192.168.1.2");
    //ListView_InsertItem(hListView, &lvi);

    //ListView_SetItemText(hListView, 1, 1, (LPWSTR)L"Computer2");
    //ListView_SetItemText(hListView, 1, 2, (LPWSTR)L"Domain2");
    //ListView_SetItemText(hListView, 1, 3, (LPWSTR)L"11:00 AM");

    //// Третья строка
    //lvi.iItem = 2; // индекс строки
    //lvi.iSubItem = 0; // индекс столбца
    //lvi.pszText = const_cast<LPWSTR>(L"192.168.1.3");
    //ListView_InsertItem(hListView, &lvi);

    //ListView_SetItemText(hListView, 2, 1, (LPWSTR)L"Computer3");
    //ListView_SetItemText(hListView, 2, 2, (LPWSTR)L"Domain3");
    //ListView_SetItemText(hListView, 2, 3, (LPWSTR)L"12:00 PM");

    //std::vector<HWND> hButton; // Массив для хранения хендлов кнопок

    //hButton.resize(3); // Количество кнопок равно количеству строк

    //RECT rc;
    //for (int i = 0; i < 3; ++i) {
    //    ListView_GetItemRect(hListView, i, &rc, LVIR_LABEL);

    //    hButton[i] = CreateWindow(L"BUTTON", L"Действие",
    //        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    //        rc.right + 500, rc.top, 80, rc.bottom - rc.top, hListView, (HMENU)(i + 1), hInst, nullptr);
    //}
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateListView(hWnd);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            MessageBox(hWnd, L"Кнопка нажата!", L"Сообщение", MB_OK);
        }
        break;

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
    Server serv(4444);
    serv.start();

    thread recv_new_stat([&]() {
        while (true) {
            Sleep(1000  * 10);
            auto client_data = serv.get_all_clients_data();
            for (auto cld : client_data) {
                serv.send_command(cld.hostName, Command::GET_STATE);

            }
            UpdateListView(hListView, client_data);

        }
    });
    recv_new_stat.detach();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;

}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
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
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
        10, 10, 1000, 200, hWnd, nullptr, hInst, nullptr);

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

    lvc.cx = 150;
    lvc.pszText = const_cast<LPWSTR>(L"Управление");
    ListView_InsertColumn(hListView, 4, &lvc);

}