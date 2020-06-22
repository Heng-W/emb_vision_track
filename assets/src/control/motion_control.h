#ifndef EVT_MOTION_CONTROL_H
#define EVT_MOTION_CONTROL_H

#include "control.h"
#include "pid.h"


namespace EVTrack
{

//机器人小车运动控制
class MotionControl: public Control
{
public:
    MotionControl();
    ~MotionControl();


    void perform();
    void updateDuty();//更新电机占空比

    void enableMotor();
    void disableMotor();

    static int trackWidth;

    static volatile int leftValSet;//左电机控制量设定值
    static volatile int rightValSet;//右电机控制量设定值
    static int leftCtlVal;//左电机控制量
    static int rightCtlVal;//右电机控制量

    static volatile bool stopSignal;
    static volatile bool startSignal;
    static volatile bool valSetFlag;
    static volatile bool autoCtlFlag;//手自动控制标志


    static volatile bool pidSetFlag;
    static volatile float pidParams[4];

    static int motorDeadValue[4];//电机死区
private:
    PID pidDirection_;//自动模式下方向PD控制
    PID pidDistance_;//自动模式下距离PI控制（需多尺度追踪）
    bool isEnabled_;

};


}


#endif  //EVT_MOTION_CONTROL_H
