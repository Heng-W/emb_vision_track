#ifndef EVT_CONTROL_EVENT_H
#define EVT_CONTROL_EVENT_H

#include <pthread.h>

#include "field_control.h"
#include "motion_control.h"
#include "../comm/server.h"

namespace EVTrack
{

namespace control
{

class Event
{
public:
    Event(Server& server);
    void eventLoop();//事件循环
private:
    Server& server_;
    pthread_t thread_;//设备控制线程（周期100ms）
    FieldControl fieldControl_;
    MotionControl motionControl_;

};

}

}


#endif  //EVT_CONTROL_EVENT_H
