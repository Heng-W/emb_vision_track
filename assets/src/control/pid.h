#ifndef EVT_PID_H
#define EVT_PID_H



namespace EVTrack
{

class PID
{

public:
    PID(float setpoint, float dt, float Kp, float Ki, float Kd);
    float calculate(float error);
    float divideOutput();

    void updateSetpoint(float setpoint)
    {
        setpoint_ = setpoint;
    }

    void setParams(float kp, float ki, float kd)
    {
        pidParams_[0] = kp;
        pidParams_[1] = ki;
        pidParams_[2] = kd;
    }
    float getError(float processVal)
    {
        return setpoint_ - processVal;
    }

    void setOutputLimit(float outputLimit)
    {
        outputLimit_ = outputLimit;
    }
    void setIntegralLimit(float integralLimit)
    {
        integralLimit_ = integralLimit;
    }
    void setIntegralSeparation(float val)
    {
        integralRange_ = val;
    }

    void setDivideCnt(int val)
    {
        divideCnt_ = val;
    }
    void reset();


private:
    float setpoint_;
    float dt_;
    float pidParams_[3];
    //   float kp_, ki_, kd_;
    float lastError_;
    float errorMin_;
    float integral_;
    float output_;
    float lastOutput_;
    float finalOutput_;
    float integralLimit_;
    float outputLimit_;
    float integralRange_;
    float integralCoef_;
    int divideCnt_;

};



}


#endif //EVT_PID_H
