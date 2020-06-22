#ifndef EVT_CONTROL_MACRO_HPP
#define EVT_CONTROL_MACRO_HPP



#define POS_ERROR_MIN  6

#define DISTCTL_EN_ANGLE 6

#define SERVO_CTRL_DT  0.1
#define SERVO_CTRL_KP  0.01
#define SERVO_CTRL_KI  0
#define SERVO_CTRL_KD  0.001


#define DEFAULT_ANGLE_H 90
#define MIN_ANGLE_H 0
#define MAX_ANGLE_H 180

#define DEFAULT_ANGLE_V 90
#define MIN_ANGLE_V 20
#define MAX_ANGLE_V 160



#define MOTOR_OUT_DEAD_VAL_L 4
#define MOTOR_OUT_DEAD_VAL_R 4

#define MOTOR_CTRL_DT  0.1


#define DIR_CTRL_DT 0.1
#define DIR_CTRL_KP 1.5
#define DIR_CTRL_KI 0
#define DIR_CTRL_KD 0.001


#define DIST_CTRL_DT 0.1
#define DIST_CTRL_KP 4
#define DIST_CTRL_KI 0.001
#define DIST_CTRL_KD 0


namespace EVTrack
{

template <typename T>
inline void limit(T& val, T absMax)
{
    if (val > absMax)
    {
        val = absMax;
    }
    else if (val < -absMax)
    {
        val = -absMax;
    }
}


template <typename T>
inline void limit(T& val, T min, T max)
{
    if (val > max)
    {
        val = max;
    }
    else if (val < min)
    {
        val = min;
    }
}


/*
inline void limit(float& val, float absMax)
{
    if (val > absMax)
    {
        val = absMax;
    }
    else if (val < -absMax)
    {
        val = -absMax;
    }
}

inline void limit(float& val, float min, float max)
{
    if (val > max)
    {
        val = max;
    }
    else if (val < min)
    {
        val = min;
    }
}
*/

}


#endif  //EVT_CONTROL_MACRO_HPP
