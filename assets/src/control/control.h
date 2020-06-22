#ifndef EVT_CONTROL_H
#define EVT_CONTROL_H

#include <string>
#include <atomic>

namespace EVTrack
{

class Control
{
public:
    Control(std::string deviceName, bool openFlag);
    ~Control();

    void openDevice();
    void closeDevice();

protected:
    const std::string deviceName_;
    int deviceFd_;

};



}


#endif  //EVT_CONTROL_H
