#pragma once
#include <map>
#include <string>
using namespace std;

class ConfigReader
{
public:
	map<string, string> read(string confPaht);
};

