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

    //更新读取一帧
    int updateFrame();

    int openDevice();
    void closeDevice();

    //摄像头参数信息
    void dispCapInfo();

    //摄像头工作模式
    void setFormat();
    void dispFormat();

    //视频流参数设置，设定帧率
    void setStreamParm();

    //图像缓冲区管理
    void requestBuffer();
    void releaseBuffer();

    //视频流控制
    void startStream();
    void stopStream();

    int init();
    void release();

    //图像采集像素
    static const int IMAGE_W = 320;
    static const int IMAGE_H = 240;



protected:
    std::string deviceName_;//摄像头设备名称
    int bufferCnt_;//缓冲区数量
    float fps_;//帧率
    int deviceFd_;
    VideoBuffer* buffers_;//图像缓冲
    void (*pDisposeRawImage_)(void*);//图像预处理函数指针

};


}

#endif //EVT_VIDEO_DEVICE_H

