#ifndef EVT_VIDEO_DEVICE_H
#define EVT_VIDEO_DEVICE_H

#include <string>


namespace EVTrack
{

struct VideoBuffer;

class VideoDevice
{
public:

    //  typedef void (VideoDevice::*pfun)(void*);

    VideoDevice(std::string deviceName = "/dev/video0", int bufferCnt = 4, float fps = 15);
    ~VideoDevice();

    void setDisposeFunction(void(*pfun)(void*))
    {
        pDisposeRawImage_ = pfun;
    }

    int updateFrame();

    int openDevice();
    void closeDevice();

    void dispCapInfo();

    void setFormat();
    void dispFormat();

    void setStreamParm();

    void requestBuffer();
    void releaseBuffer();

    void startStream();
    void stopStream();

    int init();
    void release();

    static const int IMAGE_W = 320;
    static const int IMAGE_H = 240;



protected:
    std::string deviceName_;
    int bufferCnt_;
    float fps_;
    int deviceFd_;
    VideoBuffer* buffers_;
    void (*pDisposeRawImage_)(void*);

};


}

#endif //EVT_VIDEO_DEVICE_H

