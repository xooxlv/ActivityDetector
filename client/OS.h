#pragma once
#include <string>
using namespace std;

class OS
{
public:
	static string getLastUserActivityTime();
	static string getHostIp();
	static string getPCName();
	static string getPCDomain();
	static string getExecProgramPath();
	static void addProgramToAutorun(string programName, string execPath);
};

