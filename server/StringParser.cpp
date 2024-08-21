#include "StringParser.h"



wstring StringParser::str_to_wstr(const string& str) {
    return wstring(str.begin(), str.end());
}

string StringParser::wstr_to_str(const wstring& wstr) {
    return string(wstr.begin(), wstr.end());
}

LPWSTR StringParser::lpcstr_to_lpwstr(LPCSTR str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    LPWSTR wstr = new WCHAR[size_needed];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, size_needed);
    return wstr;
}

LPSTR StringParser::lpcwstr_to_lpstr(LPCWSTR wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    LPSTR str = new CHAR[size_needed];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, size_needed, NULL, NULL);
    return str;
}