#ifndef EVT_VIDEO_FRAME_H
#define EVT_VIDEO_FRAME_H

#include "opencv2/opencv.hpp"
#include "video_device.h"

namespace EVTrack
{

class VideoFrame : public VideoDevice
{
public:
    VideoFrame();

    int update()
    {
        //readFrame(frameBGR_,frameYUYV_);
        return updateFrame();
    }

    cv::Mat& getFrameRef()
    {
        return frameBGR_;
    }

    //原始YUYV格式转换成BGR图像
    void convertImage(void* rawImage);


private:
    cv::Mat frameYUYV_;//YUYV图像帧
    cv::Mat frameBGR_;//BGR图像帧
    //cv::Mat frame_;

};


}


#endif  //EVT_VIDEO_FRAME_H
