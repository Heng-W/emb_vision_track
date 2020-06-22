
#include "field_control.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <iostream>

#include "control_macro.hpp"
#include "../vision/locate.h"
#include "../vision/video_device.h"




#define IOC_MAGIC 0x81

#define COMBINE_CMD(cmd,ch) _IOWR(IOC_MAGIC,((cmd)<<1|((ch)&0x1)),int)


namespace EVTrack
{

namespace vision
{

extern bool trackFlag;

}

enum SERVO_DEVICE_CMD
{
    SET_ANGLE,
    GET_ANGLE,
    GET_FREQ,
    GET_DUTY,
    START_PWM,
    STOP_PWM
};

float FieldControl::angleHDef = 90;
float FieldControl::angleVDef = 90;


std::atomic<float> FieldControl::angleH(FieldControl::angleHDef);
std::atomic<float> FieldControl::angleV(FieldControl::angleVDef);

volatile int FieldControl::angleHSet = FieldControl::angleHDef * 100;
volatile int FieldControl::angleVSet = FieldControl::angleVDef * 100;
volatile bool FieldControl::valSetFlag = false;
volatile bool FieldControl::autoCtlFlag = false;


volatile bool FieldControl::pidSetFlag = false;
volatile float FieldControl::pidParams[4] = {SERVO_CTRL_KP, SERVO_CTRL_KD, SERVO_CTRL_KP, SERVO_CTRL_KD};


FieldControl::FieldControl():
    Control("/dev/servo", true),
    pidAngleH_(VideoDevice::IMAGE_W / 2, SERVO_CTRL_DT, SERVO_CTRL_KP, 0, SERVO_CTRL_KD),
    pidAngleV_(VideoDevice::IMAGE_H / 2, SERVO_CTRL_DT, SERVO_CTRL_KP, 0, SERVO_CTRL_KD)
{
    pidAngleH_.setOutputLimit(20);
    pidAngleV_.setOutputLimit(20);


}


void FieldControl::perform()
{
    if (pidSetFlag)
    {
        pidSetFlag = false;
        pidAngleH_.setParams(pidParams[0], 0, pidParams[1]);
        pidAngleV_.setParams(pidParams[2], 0, pidParams[3]);
    }

    if (!autoCtlFlag)
    {
        if (valSetFlag)
        {
            valSetFlag = false;
            angleH = (float)angleHSet / 100;
            angleV = (float)angleVSet / 100;
        }
    }
    else
    {
        if (deviceFd_ < 0 || !vision::trackFlag) return;

        //  if (!Locate::findFlag) return;
        float error;
        float angle = angleH;
        error = pidAngleH_.getError(Locate::xpos);
        if (fabs(error) > POS_ERROR_MIN)
            angle = angleH + pidAngleH_.calculate(error);
        limit(angle, (float)MIN_ANGLE_H, (float)MAX_ANGLE_H);
        angleH = angle;

        angle = angleV;
        error = pidAngleV_.getError(Locate::ypos);
        if (fabs(error) > POS_ERROR_MIN)
            angle = angleV - pidAngleV_.calculate(error);
        limit(angle, (float)MIN_ANGLE_V, (float)MAX_ANGLE_V);
        angleV = angle;
    }
    //angleV = limit(pidAngleV_.calculate(locate.getYpos()) + DEFAULT_ANGLE_V, MIN_ANGLE_V, MAX_ANGLE_V);

    if (deviceFd_ < 0) return;
    ioctl(deviceFd_, COMBINE_CMD(SET_ANGLE, 0), &angleH);
    ioctl(deviceFd_, COMBINE_CMD(SET_ANGLE, 1), &angleV);

}



}
