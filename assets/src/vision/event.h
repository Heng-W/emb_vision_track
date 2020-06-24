#ifndef EVT_VISION_EVENT_H
#define EVT_VISION_EVENT_H

#include <pthread.h>


namespace EVTrack
{

class Server;

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
    pthread_t pickThread_;//图像采集线程
    pthread_t disposeThread_;//图像处理线程
    pthread_t packetThread_;//图像压缩打包线程
    Server& server_;



};


}

}


#endif  //EVT_VISION_EVENT_H
