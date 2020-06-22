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
    void convertImage(void* rawImage);


private:
    cv::Mat frameYUYV_;
    cv::Mat frameBGR_;
    //cv::Mat frame_;

};


}


#endif  //EVT_VIDEO_FRAME_H
