#ifndef EVT_COMMON_H
#define EVT_COMMON_H

namespace EVTrack
{


typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;


enum class Command
{
    init,
    exit,
    reset,
    notice,
    message,
    image,
    locate,
    set_target,
    start_track,
    stop_track,
    enable_motion_autoctl,
    disable_motion_autoctl,
    start_motor,
    stop_motor,
    set_motor_val,
    get_motor_val,
    set_motion_ctl_pid,
    enable_field_autoctl,
    disable_field_autoctl,
    set_servo_val,
    get_servo_val,
    set_field_ctl_pid,
    get_client_cnt,
    send_to_all_clients,
    send_to_client,
    enable_multi_scale,
    disable_multi_scale,
    system_cmd,
    restart_server,
    halt,
    reboot,
    update_client_cnt,
    logout,
    update_pid
};



}

#endif //EVT_COMMON_H
