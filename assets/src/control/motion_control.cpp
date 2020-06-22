

#include "motion_control.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <iostream>

#include "control_macro.hpp"
#include "field_control.h"
#include "../vision/locate.h"
#include "def_var.h"


#define IOC_MAGIC 'x'

#define COMBINE_CMD(cmd,ch) _IOWR(IOC_MAGIC,((cmd)<<1|((ch)&0x1)),int)


namespace EVTrack
{


namespace vision
{

extern bool trackFlag;

extern int targetWidth;
extern int targetHeight;

extern bool useMultiScale;

}

int MotionControl::trackWidth = 120;

volatile int MotionControl::leftValSet;
volatile int MotionControl::rightValSet;
int MotionControl::leftCtlVal;
int MotionControl::rightCtlVal;

volatile bool MotionControl::stopSignal = false;
volatile bool MotionControl::startSignal = false;
volatile bool MotionControl::valSetFlag = false;
volatile bool MotionControl::autoCtlFlag = false;


volatile bool MotionControl::pidSetFlag = false;
volatile float MotionControl::pidParams[4] = {DIR_CTRL_KP, DIR_CTRL_KD, DIST_CTRL_KP, DIST_CTRL_KI};

int MotionControl::motorDeadValue[4] = {200, 500, 260, 500};

static int adjustTimeCnt;


enum MOTOR_DEVICE_CMD
{
    SET_MOTOR,
    SET_DUTY,
    SET_CMP,
    SET_DUR,
    SET_PRES,
    SET_DIV,
    GET_COUNT,
    GET_CMP,
    GET_DUR,
    GET_PRES,
    GET_DIV,
    GET_FREQ,
    GET_DUTY,
    START_PWM,
    STOP_PWM,
    STOP_MOTOR
};


inline void MotionControl::enableMotor()
{
    if (deviceFd_ < 0 || isEnabled_) return;
    isEnabled_ = true;

    ioctl(deviceFd_, COMBINE_CMD(START_PWM, 0), 0);
    ioctl(deviceFd_, COMBINE_CMD(START_PWM, 1), 0);
}

inline void MotionControl::disableMotor()
{

    if (deviceFd_ < 0 || !isEnabled_) return;
    isEnabled_ = false;

    ioctl(deviceFd_, COMBINE_CMD(STOP_MOTOR, 0), 0);
    ioctl(deviceFd_, COMBINE_CMD(STOP_MOTOR, 1), 0);
}


MotionControl::MotionControl():
    Control("/dev/motor", true),
    pidDirection_(FieldControl::angleHDef, DIR_CTRL_DT, DIR_CTRL_KP, 0, DIR_CTRL_KD),
    pidDistance_(trackWidth, DIST_CTRL_DT, DIST_CTRL_KP, DIST_CTRL_KI, 0),
    isEnabled_(false)
{
    pidDirection_.setOutputLimit(100);

    pidDistance_.setOutputLimit(100);

    pidDistance_.setIntegralLimit(100);

    pidDistance_.setIntegralSeparation(10);


    enableMotor();
    if (deviceFd_ >= 0)
    {
        ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 0), 0);
        ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 1), 0);
    }
}



MotionControl::~MotionControl()
{

    disableMotor();

    std::cout << "Destruct MotionControl" << std::endl;

}


void MotionControl::perform()
{
    if (pidSetFlag)
    {
        pidSetFlag = false;
        pidDirection_.setParams(pidParams[0], 0, pidParams[1]);
        pidDistance_.setParams(pidParams[2], pidParams[3], 0);
    }

    if (!autoCtlFlag)//manual
    {
        if (stopSignal)
        {
            stopSignal = false;

            leftCtlVal = 0;
            rightCtlVal = 0;

            disableMotor();

        }
        else if (startSignal)
        {
            startSignal = false;

            enableMotor();
            if (deviceFd_ >= 0)
            {
                ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 0), 0);
                ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 1), 0);
            }
        }

        if (valSetFlag)
        {
            valSetFlag = false;

            leftCtlVal = leftValSet;
            rightCtlVal = rightValSet;


            updateDuty();

        }
    }
    else//auto
    {
        if (!vision::trackFlag) return;

        //  pidDirection_.updateSetpoint(FieldControl::angleHDef);
        float dirError = pidDirection_.getError(FieldControl::angleH);

        float outputDirection = pidDirection_.calculate(dirError);


        // pidDistance_.updateSetpoint(vision::targetWidth);
        float outputDistance = 0;

        if (vision::useMultiScale && adjustTimeCnt < var::distTime && fabs(dirError) < DISTCTL_EN_ANGLE && FieldControl::angleV > (FieldControl::angleVDef - 75))
        {
            outputDistance = pidDistance_.calculate(pidDistance_.getError(Locate::width));

            leftCtlVal = (int)(outputDistance + outputDirection);
            rightCtlVal = (int)(outputDistance - outputDirection);

            adjustTimeCnt++;
            //  adjustTimeCnt=0;
        }
        else if (adjustTimeCnt < var::dirTime)
        {
            if (outputDirection > 0)
            {


                leftCtlVal = 0; // (int)outputDirection;
                rightCtlVal = -(int)outputDirection;
            }
            else
            {
                leftCtlVal = (int)outputDirection;
                rightCtlVal = 0; // -(int)outputDirection;

            }
            adjustTimeCnt++;
        }
        else if (adjustTimeCnt > var::motorMinPeriod && fabs(Locate::xposError()) < 12 && fabs(Locate::yposError()) < 10)
        {
            adjustTimeCnt = 0;
        }
        else
        {
            adjustTimeCnt++;
            leftCtlVal = 0;
            rightCtlVal = 0;
            if (adjustTimeCnt > var::motorPeriod) adjustTimeCnt = 0;

        }

        updateDuty();


    }


}

void MotionControl::updateDuty()
{
    if (deviceFd_ < 0) return;

    int outputLeft = leftCtlVal;
    int outputRight = rightCtlVal;

    if (outputLeft > 0)  outputLeft += motorDeadValue[0];
    else if (outputLeft < 0) outputLeft -= motorDeadValue[1];

    if (outputRight > 0)  outputRight += motorDeadValue[2];
    else if (outputRight < 0) outputRight -= motorDeadValue[3];

    limit(outputLeft, 1000);
    limit(outputRight, 1000);


    if (!isEnabled_)
    {
        enableMotor();
    }

    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 0), outputLeft);
    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 1), outputRight);


}





}
