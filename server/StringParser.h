#pragma once
#include <Windows.h>
#include <string>
using namespace std;

class StringParser
{
public:
	static wstring str_to_wstr(const string& str);
	static string wstr_to_str(const wstring& wstr);

	static LPWSTR lpcstr_to_lpwstr(LPCSTR str);
	static LPSTR lpcwstr_to_lpstr(LPCWSTR wstr);

};


