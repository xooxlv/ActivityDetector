#include "ConfigReader.h"
#include <fstream>
#include <iostream>
#include <regex>
using namespace std;

map<string, string> ConfigReader::read(string confPaht)
{
    std::string line;
    map<string, string> conf;

    std::ifstream in(confPaht);

    if (in.is_open())
    {
        while (std::getline(in, line))
        {
            string delimiter = " ";
            size_t pos = 0;
            string key;
            string val; 

            pos = line.find(delimiter);
            key = line.substr(0, pos);
            line.erase(0, pos + delimiter.length());
            val = line;

            pair<string, string> p;
            p.first = key;
            p.second = line;

            conf.insert(p);
        }
    }
    in.close(); 

    return  conf;
}
