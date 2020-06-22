#ifndef EVT_VISION_EVENT_H
#define EVT_VISION_EVENT_H

#include <pthread.h>
#include "../comm/server.h"

namespace EVTrack
{

namespace vision
{

class Event
{
public:
    Event(Server& server_);
    void pickEventLoop();
    void disposeEventLoop();
    void packetEventLoop();

private:
    pthread_t pickThread_;
    pthread_t disposeThread_;
    pthread_t packetThread_;
    Server& server_;



};


}

}


#endif  //EVT_VISION_EVENT_H
