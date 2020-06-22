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
    void eventLoop();
private:
    Server& server_;
    pthread_t thread_;
    FieldControl fieldControl_;
    MotionControl motionControl_;

};

}

}


#endif  //EVT_CONTROL_EVENT_H
