#ifndef EVT_PID_H
#define EVT_PID_H



namespace EVTrack
{

class PID
{

public:
    PID(float setpoint, float dt, float Kp, float Ki, float Kd);
    float calculate(float error);//计算更新PID输出值
    float divideOutput();//输出划分到每个小周期

	//更新设定值
    void updateSetpoint(float setpoint)
    {
        setpoint_ = setpoint;
    }

	//设置PID参数
    void setParams(float kp, float ki, float kd)
    {
        pidParams_[0] = kp;
        pidParams_[1] = ki;
        pidParams_[2] = kd;
    }
	
	//获取误差
    float getError(float processVal)
    {
        return setpoint_ - processVal;
    }

	//输出限幅
    void setOutputLimit(float outputLimit)
    {
        outputLimit_ = outputLimit;
    }
	
	//积分限幅
    void setIntegralLimit(float integralLimit)
    {
        integralLimit_ = integralLimit;
    }
	
	//积分分离，防止饱和
    void setIntegralSeparation(float val)
    {
        integralRange_ = val;
    }

	//设置划分的周期数量
    void setDivideCnt(int val)
    {
        divideCnt_ = val;
    }
	
	//参数复位
    void reset();


private:
    float setpoint_;//设定值
    float dt_;//周期
    float pidParams_[3];//PID参数
    //float kp_, ki_, kd_;
    float lastError_;
    float integral_;
    float output_;
    float lastOutput_;
    float finalOutput_;
    float integralLimit_;//积分限幅值
    float outputLimit_;//输出限幅值
    float integralRange_;//积分分离阈值
    float integralCoef_;//积分分离使用的系数
    int divideCnt_;

};



}


#endif //EVT_PID_H
