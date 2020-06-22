
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include "control/field_control.h"
#include "control/motion_control.h"
#include "control/def_var.h"

using namespace std;


namespace EVTrack
{

namespace vision
{

extern int pickDelay;
extern int pickTimeOutDelay;

}

//分割字符串
vector<string> splitString(const string& in, const string& delim)
{
    vector<string> ret;
    try
    {
        regex re{delim};
        return vector<string>
        {
            sregex_token_iterator(in.begin(), in.end(), re, -1),
            sregex_token_iterator()
        };
    }
    catch (const std::exception& e)
    {
        cout << "error:" << e.what() << endl;
    }
    return ret;
}


typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

hash_t hash_(char const* str)
{
    hash_t ret{basis};

    while (*str)
    {
        ret ^= *str;
        ret *= prime;
        str++;
    }

    return ret;
}


constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
    return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
}

constexpr unsigned long long operator "" _hash(char const* p, size_t)
{
    return hash_compile_time(p);
}

void initVar()
{

    stringstream stream;

    ifstream file;
    file.open("var.conf", ios::in);//读取配置文件
    string str;

    while (getline(file, str))
    {


        vector<string> ret = splitString(str, "=");
        if (ret.size() != 2)
        {
            continue;
        }
		//利用hash实现字符串switch
        switch (hash_(ret[0].c_str()))
        {
            case "motorDeadValue0"_hash:

                stream << ret[1];
                stream >> MotionControl::motorDeadValue[0];
                stream.clear();
                break;

            case "motorDeadValue1"_hash:

                stream << ret[1];
                stream >> MotionControl::motorDeadValue[1];
                stream.clear();
                break;

            case "motorDeadValue2"_hash:

                stream << ret[1];
                stream >> MotionControl::motorDeadValue[2];
                stream.clear();
                break;

            case "motorDeadValue3"_hash:

                stream << ret[1];
                stream >> MotionControl::motorDeadValue[3];
                stream.clear();
                break;

            case "angleHDef"_hash:
                stream << ret[1];
                stream >> FieldControl::angleHDef;
                stream.clear();

                FieldControl::angleHSet = FieldControl::angleHDef * 100;
                FieldControl::valSetFlag = true;
                break;

            case "angleVDef"_hash:
                stream << ret[1];
                stream >> FieldControl::angleVDef;
                stream.clear();

                FieldControl::angleVSet = FieldControl::angleVDef * 100;
                FieldControl::valSetFlag = true;
                break;

            case "trackWidth"_hash:
                stream << ret[1];
                stream >> MotionControl::trackWidth;
                stream.clear();
                break;
            case "pickDelay"_hash:
                stream << ret[1];
                stream >> vision::pickDelay;
                stream.clear();
                break;

            case "pickTimeOutDelay"_hash:
                stream << ret[1];
                stream >> vision::pickTimeOutDelay;
                stream.clear();
                break;
            case "distTime"_hash:
                stream << ret[1];
                stream >> var::distTime;
                stream.clear();

                break;

            case "dirTime"_hash:
                stream << ret[1];
                stream >> var::dirTime;
                stream.clear();

                break;
            case "motorPeriod"_hash:
                stream << ret[1];
                stream >> var::motorPeriod;
                stream.clear();

                break;

            case "motorMinPeriod"_hash:
                stream << ret[1];
                stream >> var::motorMinPeriod;
                stream.clear();

                break;


            default:
                break;
        }


    }
    file.close();
}

}



