
#include "field_control.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <algorithm>

#include "../util/singleton.hpp"
#include "../util/logger.h"
#include "../vision/vision_event_loop.h"
#include "control_params.h"

#define IOC_MAGIC 0x81

#define COMBINE_CMD(cmd,ch) _IOWR(IOC_MAGIC,((cmd)<<1|((ch)&0x1)),int)


namespace evt
{

enum SERVO_DEVICE_CMD
{
    SET_ANGLE,
    GET_ANGLE,
    GET_FREQ,
    GET_DUTY,
    START_PWM,
    STOP_PWM
};

FieldControl::FieldControl()
    : pidAngleH_(INITIAL_CENTER_W, SERVO_CTRL_DT, SERVO_CTRL_KP, 0, SERVO_CTRL_KD),
      pidAngleV_(INITIAL_CENTER_H, SERVO_CTRL_DT, SERVO_CTRL_KP, 0, SERVO_CTRL_KD),
      autoMode_(false),
      angleH_(DEFAULT_ANGLE_H),
      angleV_(DEFAULT_ANGLE_V),
      angleHDefault_(DEFAULT_ANGLE_H),
      angleVDefault_(DEFAULT_ANGLE_V)
{
    pidAngleH_.setOutputLimit(20);
    pidAngleV_.setOutputLimit(20);

}

FieldControl::~FieldControl()
{
    if (deviceFd_ >= 0)
    {
        close(deviceFd_);
    }
}

bool FieldControl::openDevice(const char* deviceName)
{
    deviceFd_ = ::open(deviceName, O_RDWR);
    if (deviceFd_ < 0)
    {
        LOG(ERROR) << "open " << deviceName;
        return false;
    }
    return true;
}

void FieldControl::perform()
{
    if (!autoMode_ || deviceFd_ < 0) return;

    VisionEventLoop& vision = util::Singleton<VisionEventLoop>::instance();

    if (!vision.trackEnabled()) return;

    Rect result = vision.trackResult();

    float error;
    float angle = angleH_;
    error = pidAngleH_.getError(result.xpos);
    if (fabs(error) > POS_ERROR_MIN)
    {
        angle = angleH_ + pidAngleH_.calculate(error);
    }
    angle = std::max(angle, MIN_ANGLE_H);
    angle = std::min(angle, MAX_ANGLE_H);
    angleH_ = angle;

    angle = angleV_;
    error = pidAngleV_.getError(result.ypos);
    if (fabs(error) > POS_ERROR_MIN)
    {
        angle = angleV_ - pidAngleV_.calculate(error);
    }
    angle = std::max(angle, MIN_ANGLE_V);
    angle = std::min(angle, MAX_ANGLE_V);
    angleV_ = angle;

    if (deviceFd_ < 0) return;
    ioctl(deviceFd_, COMBINE_CMD(SET_ANGLE, 0), &angleH_);
    ioctl(deviceFd_, COMBINE_CMD(SET_ANGLE, 1), &angleV_);
}

} // namespace evt
