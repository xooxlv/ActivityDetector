#pragma once
#include <string>
using namespace std;

class Screenshoter
{
private:
	int generator = 1;

public:
	string makeScreenshot(string dir);
	char* screenshotToMemory(string path, unsigned int* size);
	void freeMemory(char* screenshot);
};

