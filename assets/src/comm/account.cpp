
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>

#include <sstream>


#include <iostream>
#include <string>
#include <vector>
#include <regex>


using namespace std;

namespace EVTrack
{

vector<string> splitString(const string& in, const string& delim);


int checkAccountByName(uint8 type, char* name, char* pwd, uint16* userID)
{

    ifstream file;
    file.open("user.conf", ios::in);
    string str;


    while (getline(file, str))
    {

        vector<string> ret = splitString(str, " ");

        if (ret.size() < 4)
        {
            continue;
        }
        if (type != atoi(ret[1].c_str()))
        {
            continue;
        }
        if (strcmp(ret[2].c_str(), name))
        {
            continue;
        }

        *userID = (uint16)atoi(ret[0].c_str());

        if (strcmp(ret[3].c_str(), pwd) == 0)
        {
            file.close();
            return 0;
        }
        file.close();
        return 1;//pwd error

    }
    file.close();
    return 2;//name error

}


int checkAccountById(int id, char* pwd, char* name)
{

    ifstream file;
    file.open("user.txt", ios::in);
    string str;

    while (getline(file, str))
    {
        vector<string> ret = splitString(str, " ");
        if (ret.size() < 4)
        {
            continue;
        }
        if (atoi(ret[0].c_str()) != id)
        {
            continue;
        }
        strcpy(name, ret[2].c_str());

        if (strcmp(ret[3].c_str(), pwd) == 0)
        {
            file.close();
            return 0;
        }
        file.close();
        return 1;//pwd error

    }
    file.close();
    return 3;//id error

}


}
