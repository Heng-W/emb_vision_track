/**
 * @brief 创建一个管道，fork一个进程，执行shell
 */
#include "sys_cmd.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>

namespace util
{

int system(const std::string& cmd, std::string& result)
{
    FILE* fp = popen(cmd.c_str(), "r"); // 将命令输出通过管道读取到文件流
    if (!fp)
    {
        result = std::string("run command failed:") + strerror(errno);
        return -1;
    }

    std::stringstream ss;
    char lineBuf[512];

    // fread(buf, sizeof(char), sizeof(buf), fp);
    while (fgets(lineBuf, sizeof(lineBuf), fp))
    {
        if (ss.tellp() > (4 << 10))
        {
            break;
        }
        ss << lineBuf;
    }

    int ret = pclose(fp);
    if (ret == -1 || !WIFEXITED(ret))
    {
        result = std::string("run command failed:") + strerror(errno);
        return -1;
    }

    int exitcode = WEXITSTATUS(ret);

    result = ss.str();

    return exitcode;
}

} // namespace util


