/* 创建一个管道，fork一个进程，
执行shell，利用读取文件方式获得输出。*/
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"

namespace EVTrack
{

static char buf[1024];
static int len;

inline void resolveSysCmd(const char* cmd)
{
    FILE* stream;

    stream = popen(cmd, "r"); //将命令输出通过管道读取到文件流

    len = fread(buf, sizeof(char), sizeof(buf), stream); //将数据流读取到缓冲区

    pclose(stream);


}

void writeSysCmdResult(Packet& p, const char* cmd)
{
    resolveSysCmd(cmd);
    p.writeBytes(buf, len);
}


}
