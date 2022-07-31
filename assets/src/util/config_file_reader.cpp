
#include "config_file_reader.h"
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include "string_util.h"

using namespace std;

namespace util
{

ConfigFileReader::ConfigFileReader(const char* fileName)
    : fileName_(fileName)
{
    loadFile(fileName);
}


const char* ConfigFileReader::get(const char* key)
{
    auto it = variables_.find(key);
    return it != variables_.end() ? it->second.c_str() : nullptr;
}

void ConfigFileReader::loadFile(const char* fileName)
{
    ifstream file;
    file.open(fileName, ios::in); // 读取配置文件
    string str;

    while (getline(file, str))
    {
        vector<string> res = splitString(str, "=");
        if (res.size() != 2 || res[0][0] == '#')
        {
            continue;
        }
        variables_[res[0]] = res[1];
    }
    file.close();
}

} // namespace util



