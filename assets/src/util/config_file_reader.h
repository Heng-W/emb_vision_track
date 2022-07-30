#ifndef UTIL_CONFIG_FILE_READER_H
#define UTIL_CONFIG_FILE_READER_H

#include <string>
#include <unordered_map>

namespace util
{

class ConfigFileReader
{
public:
    ConfigFileReader(const char* fileName);

    const char* get(const char* key);

    const char* fileName() const { return fileName_.c_str(); }


private:
    void loadFile(const char* fileName);

    std::unordered_map<std::string, std::string> variables_;
    std::string fileName_;

};

} // namespace util

#endif // UTIL_CONFIG_FILE_READER_H
