#ifndef EVT_VIDEO_DEVICE_H
#define EVT_VIDEO_DEVICE_H

#include <thread>
#include <functional>

namespace evt
{

enum class ImageType
{
    BGR, YUYV
};

struct Size
{
    int width, height;
};

struct VideoBuffer;

class VideoDevice
{
public:
    using FetchFrameCallback = std::function<void(char* data, int width, int height, ImageType type)>;

    VideoDevice(const char* deviceName = "/dev/video0", int bufferCnt = 4, float fps = 15, bool mmap = true);
    ~VideoDevice();

    VideoDevice(const VideoDevice&) = delete;
    VideoDevice& operator=(const VideoDevice&) = delete;

    void setFetchFrameCallback(const FetchFrameCallback& cb)
    { fetchFrameCallback_ = cb; }

    void start();

    // 更新读取一帧
    int updateFrame();

    bool isOpened() const { return deviceFd_ >= 0; }

    bool openDevice();
    void closeDevice();

    // 摄像头参数信息
    void dispCapInfo();

    Size querySize();

    // 摄像头工作模式
    void setFormat();
    void dispFormat();

    // 视频流参数设置，设定帧率
    void setStreamParm();

    // 图像缓冲区管理
    void requestBuffer();
    void releaseBuffer();

    // 视频流控制
    void startStream();
    void stopStream();

    Size imageSize() const { return imageSize_; }

private:

    int init();
    void release();

    std::string deviceName_;
    int bufferCnt_;
    float fps_;
    Size imageSize_;

    int deviceFd_;
    VideoBuffer* buffers_; // 图像缓冲
    const bool mmap_;
    FetchFrameCallback fetchFrameCallback_;

    std::unique_ptr<std::thread> thread_;
    bool quit_;

};


} // namespace evt

#endif // EVT_VIDEO_DEVICE_H

