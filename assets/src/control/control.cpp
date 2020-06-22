
#include "control.h"
#include <fcntl.h>
#include <unistd.h>


namespace EVTrack
{


Control::Control(std::string deviceName, bool openFlag):
    deviceName_(deviceName),
    deviceFd_(-1)
{
    if (openFlag)
    {
        openDevice();
    }

}


Control::~Control()
{
    closeDevice();
}

void Control::openDevice()
{
    if (deviceFd_ >= 0) return;
    deviceFd_ = open(deviceName_.c_str(), O_RDWR);
    if (deviceFd_ < 0)
    {
        perror(("open " + deviceName_).c_str());
    }
}

void Control::closeDevice()
{
    if (deviceFd_ < 0) return;
    close(deviceFd_);
    deviceFd_ = -1;
}




}
