#ifndef EVT_CONTROL_CONTROL_PARAMS_H
#define EVT_CONTROL_CONTROL_PARAMS_H

namespace evt
{

constexpr int POS_ERROR_MIN = 6;

constexpr int DISTCTL_EN_ANGLE = 6;

constexpr float SERVO_CTRL_DT = 0.1f;
constexpr float SERVO_CTRL_KP = 0.01f;
constexpr float SERVO_CTRL_KI = 0;
constexpr float SERVO_CTRL_KD = 0.001;


constexpr float DEFAULT_ANGLE_H = 90;
constexpr float MIN_ANGLE_H = 0;
constexpr float MAX_ANGLE_H = 180;

constexpr float DEFAULT_ANGLE_V = 90;
constexpr float MIN_ANGLE_V = 20;
constexpr float MAX_ANGLE_V = 160;



constexpr int MOTOR_OUT_DEAD_VAL_L = 4;
constexpr int MOTOR_OUT_DEAD_VAL_R = 4;

constexpr float MOTOR_CTRL_DT = 0.1;


constexpr float DIR_CTRL_DT = 0.1;
constexpr float DIR_CTRL_KP = 1.5;
constexpr float DIR_CTRL_KI = 0;
constexpr float DIR_CTRL_KD = 0.001;


constexpr float DIST_CTRL_DT = 0.1;
constexpr float DIST_CTRL_KP = 4;
constexpr float DIST_CTRL_KI = 0.001;
constexpr float DIST_CTRL_KD = 0;

constexpr int INITIAL_CENTER_W = 320;
constexpr int INITIAL_CENTER_H = 240;

constexpr int TRACK_WIDTH = 120;

} // namespace evt


#endif // EVT_CONTROL_CONTROL_PARAMS_H
