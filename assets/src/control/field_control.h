#ifndef EVT_FIELD_CONTROL_H
#define EVT_FIELD_CONTROL_H

#include "control.h"
#include "pid.h"

namespace EVTrack
{

//摄像头云台视野控制
class FieldControl: public Control
{
public:
    FieldControl();
    void perform();

    static float angleHDef;//水平角默认值
    static float angleVDef;//垂直角默认值

    static std::atomic<float> angleH, angleV;
    static volatile int angleHSet;//水平角设定值
    static volatile int angleVSet;//垂直角设定值
    static volatile bool valSetFlag;;
    static volatile bool autoCtlFlag;//手自动控制标志

    static volatile bool pidSetFlag;
    static volatile float pidParams[4];//两组PD参数
private:
    PID pidAngleH_;//自动模式下水平角PD控制
    PID pidAngleV_;//自动模式下垂直角PD控制


};



}


#endif  //EVT_FIELD_CONTROL_H
