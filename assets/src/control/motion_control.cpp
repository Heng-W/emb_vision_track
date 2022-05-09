
#include "motion_control.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <algorithm>

#include "../util/singleton.hpp"
#include "../util/logger.h"
#include "../vision/vision_event_loop.h"
#include "control_params.h"
#include "control_event_loop.h"


#define IOC_MAGIC 'x'

#define COMBINE_CMD(cmd,ch) _IOWR(IOC_MAGIC,((cmd)<<1|((ch)&0x1)),int)


namespace evt
{

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


MotionControl::MotionControl():
    pidDirection_(DEFAULT_ANGLE_H, DIR_CTRL_DT, DIR_CTRL_KP, 0, DIR_CTRL_KD),
    pidDistance_(TRACK_WIDTH, DIST_CTRL_DT, DIST_CTRL_KP, DIST_CTRL_KI, 0),
    autoMode_(false),
    enabled_(false),
    leftCtlVal_(0),
    rightCtlVal_(0),
    motorDeadValue_{200, 500, 260, 500},
    distTime_(1),
    dirTime_(2),
    motorPeriod_(30),
    motorMinPeriod_(10)
{
    pidDirection_.setOutputLimit(100);
    pidDistance_.setOutputLimit(100);
    pidDistance_.setIntegralLimit(100);
    pidDistance_.setIntegralSeparation(10);

}

MotionControl::~MotionControl()
{
    disableMotor();
}

bool MotionControl::openDevice(const char* deviceName)
{
    deviceFd_ = ::open(deviceName, O_RDWR);
    if (deviceFd_ < 0)
    {
        LOG(ERROR) << "open " << deviceName;
        return false;
    }
    startMotor();
    return true;
}

void MotionControl::enableMotor()
{
    if (deviceFd_ < 0 || enabled_) return;
    enabled_ = true;

    ioctl(deviceFd_, COMBINE_CMD(START_PWM, 0), 0);
    ioctl(deviceFd_, COMBINE_CMD(START_PWM, 1), 0);
}

void MotionControl::disableMotor()
{
    if (deviceFd_ < 0 || !enabled_) return;
    enabled_ = false;

    ioctl(deviceFd_, COMBINE_CMD(STOP_MOTOR, 0), 0);
    ioctl(deviceFd_, COMBINE_CMD(STOP_MOTOR, 1), 0);
}

void MotionControl::startMotor()
{
    if (deviceFd_ < 0) return;
    enableMotor();
    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 0), 0);
    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 1), 0);
}

void MotionControl::perform()
{
    if (!autoMode_) return;

    VisionEventLoop& vision = util::Singleton<VisionEventLoop>::instance();

    auto& fieldControl = util::Singleton<ControlEventLoop>::instance().fieldControl();

    if (!vision.trackEnabled()) return;

    //  pidDirection_.updateSetpoint(FieldControl::angleHDef);
    float dirError = pidDirection_.getError(fieldControl.angleH());

    float outputDirection = pidDirection_.calculate(dirError);


    // pidDistance_.updateSetpoint(vision::targetWidth);
    float outputDistance = 0;

    Rect result = vision.trackResult();

    static int adjustTimeCnt;

    if (vision.multiScaleEnabled() && adjustTimeCnt < distTime_ && fabs(dirError) < DISTCTL_EN_ANGLE && fieldControl.angleV() > (fieldControl.angleVDefault() - 75))
    {
        outputDistance = pidDistance_.calculate(pidDistance_.getError(result.width));

        leftCtlVal_ = static_cast<int>(outputDistance + outputDirection);
        rightCtlVal_ = static_cast<int>(outputDistance - outputDirection);

        ++adjustTimeCnt;
    }
    else if (adjustTimeCnt < dirTime_)
    {
        if (outputDirection > 0)
        {
            leftCtlVal_ = 0; // (int)outputDirection;
            rightCtlVal_ = -static_cast<int>(outputDirection);
        }
        else
        {
            leftCtlVal_ = static_cast<int>(outputDirection);
            rightCtlVal_ = 0; // -(int)outputDirection;
        }
        ++adjustTimeCnt;
    }
    else if (adjustTimeCnt > motorMinPeriod_ &&
             fabs(result.xpos - vision.imageWidth() / 2) < 12 && fabs(result.ypos - vision.imageHeight()) < 10)
    {
        adjustTimeCnt = 0;
    }
    else
    {
        ++adjustTimeCnt;
        leftCtlVal_ = 0;
        rightCtlVal_ = 0;
        if (adjustTimeCnt > motorPeriod_) adjustTimeCnt = 0;
    }
    updateDuty();
}

void MotionControl::updateDuty()
{
    if (deviceFd_ < 0) return;

    int outputLeft = leftCtlVal_;
    int outputRight = rightCtlVal_;

    if (outputLeft > 0)  outputLeft += motorDeadValue_[0];
    else if (outputLeft < 0) outputLeft -= motorDeadValue_[1];

    if (outputRight > 0)  outputRight += motorDeadValue_[2];
    else if (outputRight < 0) outputRight -= motorDeadValue_[3];

    outputLeft = std::max(outputLeft, -1000);
    outputLeft = std::min(outputLeft, 1000);

    outputRight = std::max(outputRight, -1000);
    outputRight = std::min(outputRight, 1000);

    if (!enabled_)
    {
        enableMotor();
    }

    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 0), outputLeft);
    ioctl(deviceFd_, COMBINE_CMD(SET_MOTOR, 1), outputRight);
}

} // namespace evt
