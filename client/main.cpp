#include <iostream>
#include "Client.h"
#include "ConfigReader.h"
#include <algorithm>
#include <iostream>
#include <Lmwksta.h>
#include <StrSafe.h>

using namespace std;

int main() {

    ConfigReader cr;
    auto conf = cr.read("config.txt");

    string server_ip = conf["control_server"].substr(0, conf["control_server"].find(":"));
    int server_port = atoi(conf["control_server"].substr(conf["control_server"].find(":") + 1).c_str());
    string screenshot_dir = conf["screenshot_dir"];
    int send_status_timer = atoi(conf["timer_send_status"].c_str());
 

}