
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include "comm/server.h"
#include "vision/event.h"
#include "control/event.h"

using namespace EVTrack;


namespace EVTrack
{
void initVar();
}

static void sleepMs(unsigned long ms);

Server server;


int main(int argc, char* argv [])
{
    initVar();

    control::Event controlEvent(server);
    vision::Event visionEvent(server);
    sleepMs(1000);

    server.startup();
    sleepMs(1000);

    return 0;
}



static void sleepMs(unsigned long ms)
{
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}

