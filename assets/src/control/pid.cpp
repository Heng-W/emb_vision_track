
#include "pid.h"
#include <math.h>
#include "control_macro.hpp"


namespace EVTrack
{


PID::PID(float setpoint, float dt, float kp, float ki, float kd):
    setpoint_(setpoint),
    dt_(dt),
    pidParams_{kp, ki, kd},
    lastError_(0),
    errorMin_(0),
    integral_(0),
    output_(0),
    lastOutput_(0),
    finalOutput_(0),
    integralLimit_(0),
    outputLimit_(0),
    integralRange_(0),
    integralCoef_(0),
    divideCnt_(0)
{
}

float PID::calculate(float error)
{
    float integralOut = 0;

    if (pidParams_[1] != 0)
    {
        if (integralRange_ != 0 && fabs(error) > integralRange_)
        {
            integralCoef_ = 0;
        }
        else
        {
            integralCoef_ = 1;
            integral_ += error;
        }
        if (integralLimit_ != 0)
        {
            //积分限幅
            limit(integral_, integralLimit_);
        }
        integralOut = integralCoef_ * pidParams_[1] * integral_;

    }
    output_ = pidParams_[0] * error + integralOut + pidParams_[2] * (error - lastError_);
    if (outputLimit_ != 0)
    {
        //输出限幅
        limit(output_, outputLimit_);
    }
    lastError_ = error;
    return output_;
}

float PID::divideOutput()
{
    static int idx;
    float value = output_ - lastOutput_;

    finalOutput_ = value * (++idx) / divideCnt_ + lastOutput_;
    if (idx >= divideCnt_)
    {
        idx = 0;
    }
    return finalOutput_;
}



void PID::reset()
{
    lastError_ = 0;
    integral_ = 0;
    lastOutput_ = 0;
    output_ = 0;
    divideCnt_ = 0;
}

}


