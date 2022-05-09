#ifndef UTIL_SYS_CMD_H
#define UTIL_SYS_CMD_H

#include <string>

namespace util
{

// 解析Linux命令
int system(const std::string& cmd, std::string& result);

inline std::string system(const std::string& cmd)
{
    std::string result;
    system(cmd, result);
    return result;
}

} // namespace util

#endif // UTIL_SYS_CMD_H

