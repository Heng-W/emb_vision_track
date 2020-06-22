#include "event.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <string>


namespace EVTrack
{
namespace control
{
static void* controlThread(void* pVoid)
{
    Event* p = (Event*)pVoid;
    p->eventLoop();
    return p;
}


Event::Event(Server& server):
    server_(server)
{
    int ret = pthread_create(&thread_, NULL, controlThread, this);
    if (ret != 0)
    {
        perror("Create pthread");
    }

}

static void sleepMs(unsigned long ms)
{
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}


void Event::eventLoop()
{
    //  static int cnt = 0;
    while (server_.isRunning())
    {

        motionControl_.perform();
        fieldControl_.perform();

        sleepMs(100);
    }
}



}
}
