#ifndef EVT_MOTION_CONTROL_H
#define EVT_MOTION_CONTROL_H

#include "control.h"
#include "pid.h"


namespace EVTrack
{


class MotionControl: public Control
{
public:
    MotionControl();
    ~MotionControl();


    void perform();
    void updateDuty();

    void enableMotor();
    void disableMotor();

    static int trackWidth;

    static volatile int leftValSet;
    static volatile int rightValSet;
    static int leftCtlVal;
    static int rightCtlVal;

    static volatile bool stopSignal;
    static volatile bool startSignal;
    static volatile bool valSetFlag;
    static volatile bool autoCtlFlag;


    static volatile bool pidSetFlag;
    static volatile float pidParams[4];

    static int motorDeadValue[4];
private:
    PID pidDirection_;
    PID pidDistance_;
    bool isEnabled_;

};


}


#endif  //EVT_MOTION_CONTROL_H
