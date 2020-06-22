
#include "video_frame.h"
#include <string>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"


namespace EVTrack
{

VideoFrame::VideoFrame():
    VideoDevice("/dev/video0", 5),
    frameYUYV_(IMAGE_H, IMAGE_W, CV_8UC2),
    frameBGR_(IMAGE_H, IMAGE_W, CV_8UC3)
    //frame_(IMAGE_H, IMAGE_W, CV_8UC3,image_)
{
    // pDisposeRawImage_ = (pfun)&VideoFrame::disposeRawImage;
    init();
}

void VideoFrame::convertImage(void* rawImage)
{
    frameYUYV_.data = (uchar*)rawImage;
    cv::cvtColor(frameYUYV_, frameBGR_, CV_YUV2BGR_YUYV);
}


}
