
#include <fstream>
#include "util/string_util.h"

using namespace std;

namespace evt
{

// 通过用户名验证账户
int checkAccountByUserName(const string& userName, const string& pwd, uint32_t* userId, uint8_t* type)
{
    ifstream file("conf/user.conf");

    string str;
    while (getline(file, str))
    {
        vector<string> ret = util::splitString(str, " ");

        if (ret.size() < 4 || ret[2] != userName)
        {
            continue;
        }

        *userId = atoi(ret[0].c_str());
        *type = atoi(ret[1].c_str());

        if (ret[3] == pwd)
        {
            return 0;
        }
        return 1; // pwd error

    }
    return 2; // userName error

}

// 通过ID验证账户
int checkAccountById(uint32_t id, const string& pwd, string* userName)
{
    ifstream file("conf/user.conf");

    string str;
    while (getline(file, str))
    {
        vector<string> ret = util::splitString(str, " ");
        if (ret.size() < 4 || atoll(ret[0].c_str()) != id)
        {
            continue;
        }
        *userName = std::move(ret[2]);

        if (ret[3] == pwd)
        {
            return 0;
        }
        return 1; // pwd error

    }
    return 3; // id error

}


} // namespace evt
