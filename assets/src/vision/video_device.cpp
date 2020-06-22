
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

//#include "yuyv2bgr.h"

using namespace std;


namespace EVTrack
{

struct VideoBuffer
{
    void* start;
    size_t length;
    size_t offset;
};


//unsigned char VideoDevice::image_[IMAGE_W * IMAGE_H * 3];

VideoDevice::VideoDevice(std::string deviceName, int bufferCnt, float fps):
    deviceName_(deviceName),
    bufferCnt_(bufferCnt),
    fps_(fps),
    deviceFd_(-1),
    buffers_(NULL),
    pDisposeRawImage_(NULL)
{
}


VideoDevice::~VideoDevice()
{
    release();
}


int VideoDevice::updateFrame()
{
    struct v4l2_buffer vbuf;
    memset(&vbuf, 0, sizeof(vbuf));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(deviceFd_, VIDIOC_DQBUF, &vbuf) < 0) //出列采集的帧缓冲
    {
        if (errno == EAGAIN)
        {
            return -2;
        }
        perror("VIDIOC_DQBUF");
        return -1;
    }
    // memcpy(image.data,(buffers_+vbuf.index)->start,(buffers_+vbuf.index)->length);
    //yuyv2bgr(IMAGE_W, IMAGE_H, rawImage, image_);
    if (pDisposeRawImage_ != NULL)
    {
        //(this->*pDisposeRawImage_)((buffers_ + vbuf.index)->start);
        (*pDisposeRawImage_)((buffers_ + vbuf.index)->start);
    }
    if (ioctl(deviceFd_, VIDIOC_QBUF, &vbuf) < 0) //再将其入列
    {
        perror("VIDIOC_QBUF");
        return 1;
    }
    return 0;
}

int VideoDevice::openDevice()
{
    if (deviceFd_ >= 0) return 1;
    deviceFd_ = open(deviceName_.c_str(), O_RDWR | O_NONBLOCK, 0);  //打开设备
    if (deviceFd_ < 0)
    {
        perror("Open camera");
        return -1;
    }
    return 0;
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

void VideoDevice::setFormat()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = IMAGE_W;
    fmt.fmt.pix.height = IMAGE_H;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(deviceFd_, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("VIDIOC_S_FMT");
    }
    //    rawImageSize_ = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height; //计算图片大小

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
    for (int i = 0; i < bufferCnt_; i++)
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
        if (ioctl(deviceFd_, VIDIOC_QBUF, &buf) < 0) //申请到的缓冲进入队列
        {
            perror("VIDIOC_QBUF");
        }
    }
}

void VideoDevice::releaseBuffer()
{
    if (buffers_ == NULL) return;
    for (int i = 0; i < bufferCnt_; i++)
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
    if (ioctl(deviceFd_, VIDIOC_STREAMON, &type) < 0) //开始捕捉图像数据
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
    if (openDevice() < 0)
        return -1;
    setFormat();
    setStreamParm();
    requestBuffer();
    startStream();

    return 0;
}

void VideoDevice::release()
{
    stopStream();
    releaseBuffer();
    closeDevice();
}

}

