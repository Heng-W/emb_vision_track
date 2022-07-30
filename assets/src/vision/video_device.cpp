
#include "video_device.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <iostream>

#include "../util/logger.h"

// #include "yuyv2bgr.h"

using namespace std;


namespace evt
{

struct VideoBuffer
{
    void* start;
    size_t length;
    size_t offset;
};


VideoDevice::VideoDevice(const char* deviceName, int bufferCnt, float fps, bool mmap)
    : deviceName_(deviceName),
      bufferCnt_(bufferCnt),
      fps_(fps),
      deviceFd_(-1),
      buffers_(nullptr),
      mmap_(mmap),
      quit_(false)
{
    if (openDevice())
    {
        setFormat();
        imageSize_ = querySize();
        LOG(INFO) << "image width: " << imageSize_.width << ", "
                  << "image height: " << imageSize_.height;
    }
}


VideoDevice::~VideoDevice()
{
    release();
    quit_ = true;
    if (thread_) thread_->join();
}

void VideoDevice::start()
{
    thread_.reset(new std::thread([this]
    {
        init();
        while (!quit_)
        {
            updateFrame();
        }
    }));
}


int VideoDevice::updateFrame()
{
    if (!mmap_)
    {
        int size = read(deviceFd_, buffers_->start, buffers_->length);
        if (fetchFrameCallback_)
        {
            fetchFrameCallback_((char*)buffers_->start, imageSize_.width, imageSize_.height, ImageType::YUYV);
        }
        return size;
    }
    struct v4l2_buffer vbuf;
    memset(&vbuf, 0, sizeof(vbuf));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(deviceFd_, VIDIOC_DQBUF, &vbuf) < 0) //出列采集的帧缓冲
    {
        perror("VIDIOC_DQBUF");
        return -1;
    }
    // memcpy(image.data,(buffers_+vbuf.index)->start,(buffers_+vbuf.index)->length);
    // yuyv2bgr(IMAGE_W, IMAGE_H, rawImage, image_);
    if (fetchFrameCallback_)
    {
        fetchFrameCallback_((char*)(buffers_ + vbuf.index)->start, imageSize_.width, imageSize_.height, ImageType::YUYV);
    }
    if (ioctl(deviceFd_, VIDIOC_QBUF, &vbuf) < 0) //再将其入列
    {
        perror("VIDIOC_QBUF");
        return -1;
    }
    return 0;
}

bool VideoDevice::openDevice()
{
    assert(deviceFd_ < 0);
    deviceFd_ = open(deviceName_.c_str(), O_RDWR, 0); // 打开设备
    if (deviceFd_ < 0)
    {
        LOG(ERROR) << "open camera";
        return false;
    }
    return true;
}


void VideoDevice::closeDevice()
{
    if (deviceFd_ < 0) return;
    close(deviceFd_);
    deviceFd_ = -1;
}


void VideoDevice::dispCapInfo()
{
    struct v4l2_capability cap;
    if (ioctl(deviceFd_, VIDIOC_QUERYCAP, &cap) < 0) //获取摄像头参数
    {
        perror("VIDIOC_QUERYCAP");
        return;
    }
    cout << " driver: " << cap.driver << endl;
    cout << " card: " << cap.card << endl;
    cout << " bus_info: " << cap.bus_info << endl;
    // cout << " version: " << (cap.version >> 16) & 0XFF << (cap.version >> 8) & 0XFF << cap.version & 0XFF << endl;
    cout << " capabilities: " << cap.capabilities << endl;
}

Size VideoDevice::querySize()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(deviceFd_, VIDIOC_G_FMT, &fmt) < 0)
    {
        perror("VIDIOC_G_FMT");
        return {0, 0};
    }
    return {(int)fmt.fmt.pix.width, (int)fmt.fmt.pix.height};
}

void VideoDevice::setFormat()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(deviceFd_, VIDIOC_G_FMT, &fmt) < 0)
    {
        perror("VIDIOC_G_FMT");
        return;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // fmt.fmt.pix.width = kImageWidth;
    // fmt.fmt.pix.height = kImageHeight;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(deviceFd_, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("VIDIOC_S_FMT");
    }
    // rawImageSize_ = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height; // 计算图片大小
}


void VideoDevice::dispFormat()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(deviceFd_, VIDIOC_G_FMT, &fmt) < 0)
    {
        perror("VIDIOC_G_FMT");
        return;
    }
    cout << "v4l2_format" << endl;
    cout << " type: " << fmt.type << endl;
    cout << " width: " << fmt.fmt.pix.width << endl;
    cout << " height: " << fmt.fmt.pix.height << endl;
    char fmtstr[8];
    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
    cout << " pixelformat: " << fmtstr << endl;
    cout << " field: " << fmt.fmt.pix.field << endl;
    cout << " bytesperline: " << fmt.fmt.pix.bytesperline << endl;
    cout << " sizeimage: " << fmt.fmt.pix.sizeimage << endl;
    cout << " colorspace: " << fmt.fmt.pix.colorspace << endl;
    cout << " priv: " << fmt.fmt.pix.priv << endl;
}

void VideoDevice::setStreamParm()
{
    struct v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = (int)fps_;
    // streamparm.parm.capture.capturemode=1;
    if (ioctl(deviceFd_, VIDIOC_S_PARM, &streamparm) < 0)
    {
        perror("VIDIOC_S_PARM");
        return;
    }
    if (ioctl(deviceFd_, VIDIOC_G_PARM, &streamparm) < 0)
    {
        perror("VIDIOC_G_PARM");
        return;
    }
    int num = streamparm.parm.capture.timeperframe.numerator;
    int den = streamparm.parm.capture.timeperframe.denominator;
    if (num > 0)
    {
        fps_ = (float)den / num;
        cout << "fps: " << fps_ << endl;
    }
}


void VideoDevice::requestBuffer()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = bufferCnt_;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(deviceFd_, VIDIOC_REQBUFS, &req) < 0) //申请缓冲
    {
        perror("VIDIOC_REQBUFS");
        return;
    }
    bufferCnt_ = req.count;

    buffers_ = (VideoBuffer*)calloc(bufferCnt_, sizeof(*buffers_));
    struct v4l2_buffer buf;
    for (int i = 0; i < bufferCnt_; ++i)
    {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(deviceFd_, VIDIOC_QUERYBUF, &buf) < 0)
        {
            perror("VIDIOC_QUERYBUF");
            return;
        }
        buffers_[i].length = buf.length;
        buffers_[i].offset = (size_t)buf.m.offset;
        buffers_[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, deviceFd_, buf.m.offset);
        if (buffers_[i].start == MAP_FAILED)
        {
            perror("mmap");
            return;
        }
        if (ioctl(deviceFd_, VIDIOC_QBUF, &buf) < 0) // 申请到的缓冲进入队列
        {
            perror("VIDIOC_QBUF");
        }
    }
}

void VideoDevice::releaseBuffer()
{
    if (buffers_ == NULL) return;
    for (int i = 0; i < bufferCnt_; ++i)
    {
        if (munmap(buffers_[i].start, buffers_[i].length) < 0)
        {
            perror("munmap");
        }
    }
    free(buffers_);
    buffers_ = NULL;
}

void VideoDevice::startStream()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(deviceFd_, VIDIOC_STREAMON, &type) < 0) // 开始捕捉图像数据
    {
        perror("VIDIOC_STREAMON");
    }
}

void VideoDevice::stopStream()
{
    if (deviceFd_ < 0) return;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(deviceFd_, VIDIOC_STREAMOFF, &type) < 0) //停止捕捉
    {
        perror("VIDIOC_STREAMOFF");
    }
}

int VideoDevice::init()
{
    if (mmap_)
    {
        setStreamParm();
        requestBuffer();
        startStream();
    }
    else if (buffers_ == NULL)
    {
        buffers_ = (VideoBuffer*)calloc(1, sizeof(*buffers_));
        buffers_->length = imageSize_.width * imageSize_.height * 2;
        buffers_->start = malloc(buffers_->length);
    }

    return 0;
}

void VideoDevice::release()
{
    if (mmap_)
    {
        stopStream();
        releaseBuffer();
    }
    else if (buffers_ != NULL)
    {
        free(buffers_->start);
        free(buffers_);
        buffers_ = NULL;
    }
    closeDevice();
}

} // namespace evt

