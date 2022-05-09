#ifndef EVT_CONTROL_MOTION_CONTROL_H
#define EVT_CONTROL_MOTION_CONTROL_H

#include <utility>
#include "../util/common.h"
#include "pid.h"

namespace evt
{

// 机器人小车运动控制
class MotionControl
{
public:
    MotionControl();
    ~MotionControl();

    DISALLOW_COPY_AND_ASSIGN(MotionControl);

    bool openDevice(const char* deviceName);

    void perform();

    void enableAutoMode(bool autoMode) { autoMode_ = autoMode; }
    bool isAutoMode() const { return autoMode_; }

    int leftCtlVal() const { return leftCtlVal_; }
    int rightCtlVal() const { return rightCtlVal_; }

    void setDirectionKp(float val) { pidDirection_.setKp(val); }
    void setDirectionKd(float val) { pidDirection_.setKd(val); }

    void setDistanceKp(float val) { pidDistance_.setKp(val); }
    void setDistanceKi(float val) { pidDistance_.setKi(val); }

    void setDirectionPDParams(float kp, float kd) { pidDirection_.setParams(kp, 0, kd); }
    void setDistancePIParams(float kp, float ki) { pidDistance_.setParams(kp, ki, 0); }

    std::pair<float, float> directionPDParams() const { return {pidDirection_.kp(), pidDirection_.kd()}; }
    std::pair<float, float> distancePIParams() const { return {pidDistance_.kp(), pidDistance_.ki()}; }

    void startMotor();

    void stopMotor()
    {
        leftCtlVal_ = rightCtlVal_ = 0;
        disableMotor();
    }

    void setControlValue(int leftValSet, int rightValSet)
    {
        leftCtlVal_ = leftValSet;
        rightCtlVal_ = rightValSet;
        updateDuty();
    }

    //int trackWidth;

    //int leftValSet; // 左电机控制量设定值
    //int rightValSet; // 右电机控制量设定值
    //bool stopSignal;
    //bool startSignal;
    //bool valSetFlag;
    //bool autoCtlFlag; // 手自动控制标志


    //bool pidSetFlag;
    //float pidParams[4];

private:
    void updateDuty(); // 更新电机占空比
    void enableMotor();
    void disableMotor();


    PID pidDirection_; // 自动模式下方向PD控制
    PID pidDistance_; // 自动模式下距离PI控制（需多尺度追踪）

    int deviceFd_;
    bool autoMode_;
    bool enabled_;

    int leftCtlVal_; // 左电机控制量
    int rightCtlVal_; // 右电机控制量

    int motorDeadValue_[4]; // 电机死区

    int distTime_;
    int dirTime_;
    int motorPeriod_;
    int motorMinPeriod_;
};

} // namespace evt

#endif // EVT_CONTROL_MOTION_CONTROL_H
