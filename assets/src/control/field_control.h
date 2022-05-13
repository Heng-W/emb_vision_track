#ifndef EVT_CONTROL_FIELD_CONTROL_H
#define EVT_CONTROL_FIELD_CONTROL_H

#include <utility>
#include "../util/common.h"
#include "pid.h"

namespace evt
{

// 摄像头云台视野控制
class FieldControl
{
public:
    FieldControl();
    ~FieldControl();

    DISALLOW_COPY_AND_ASSIGN(FieldControl);

    bool openDevice(const char* deviceName = "/dev/servo");

    void perform();

    void enableAutoMode(bool autoMode) { autoMode_ = autoMode; }
    bool isAutoMode() const { return autoMode_; }

    void setAngleHKp(float val) { pidAngleH_.setKp(val); }
    void setAngleHKd(float val) { pidAngleH_.setKd(val); }

    void setAngleVKp(float val) { pidAngleV_.setKp(val); }
    void setAngleVKd(float val) { pidAngleV_.setKd(val); }

    void setAngleHPDParams(float kp, float kd) { pidAngleH_.setParams(kp, 0, kd); }
    void setAngleVPDParams(float kp, float kd) { pidAngleV_.setParams(kp, 0, kd); }

    std::pair<float, float> angleHPDParams() const { return {pidAngleH_.kp(), pidAngleH_.kd()}; }
    std::pair<float, float> angleVPDParams() const { return {pidAngleV_.kp(), pidAngleV_.kd()}; }

    void setAngleH(float angleH) { angleH_ = angleH; }
    float angleH() const { return angleH_; }

    void setAngleV(float angleV) { angleV_ = angleV; }
    float angleV() const { return angleV_; }

    void setAngleHDefault(float angleH) { angleHDefault_ = angleH; }
    float angleHDefault() const { return angleHDefault_; }

    void setAngleVDefault(float angleV) { angleVDefault_ = angleV; }
    float angleVDefault() const { return angleVDefault_; }

    void resetAngleH() { angleH_ = angleHDefault_; }
    void resetAngleV() { angleV_ = angleVDefault_; }

private:
    PID pidAngleH_; // 自动模式下水平角PD控制
    PID pidAngleV_; // 自动模式下垂直角PD控制

    int deviceFd_;
    bool autoMode_; // 手自动控制标志
    float angleH_, angleV_;
    float angleHDefault_, angleVDefault_;

};

} // namespace evt

#endif // EVT_CONTROL_FIELD_CONTROL_H
