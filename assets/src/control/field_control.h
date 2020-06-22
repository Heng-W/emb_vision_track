#ifndef EVT_FIELD_CONTROL_H
#define EVT_FIELD_CONTROL_H

#include "control.h"
#include "pid.h"

namespace EVTrack
{

class FieldControl: public Control
{
public:
    FieldControl();
    void perform();

    static float angleHDef;
    static float angleVDef;

    static std::atomic<float> angleH, angleV;
    static volatile int angleHSet;
    static volatile int angleVSet;
    static volatile bool valSetFlag;;
    static volatile bool autoCtlFlag;

    static volatile bool pidSetFlag;
    static volatile float pidParams[4];
private:
    PID pidAngleH_;
    PID pidAngleV_;


};



}


#endif  //EVT_FIELD_CONTROL_H
