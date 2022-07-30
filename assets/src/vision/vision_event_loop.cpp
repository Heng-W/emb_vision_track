
#include "vision_event_loop.h"

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/imgcodecs/legacy/constants_c.h"
// #include "opencv2/videoio/legacy/constants_c.h"

#include "../util/logger.h"
#include "../util/timestamp.h"
#include "video_device.h"
#include "yuyv2bgr.h"
#include "kcf_tracker/kcf_tracker.h"


namespace evt
{

constexpr int JPEG_QUALITY_DEFAULT_VALUE = 80;
// 压缩参数
const std::vector<int> qualityOption = {CV_IMWRITE_JPEG_QUALITY, JPEG_QUALITY_DEFAULT_VALUE};

static std::unique_ptr<cv::Mat> imageBuf_;


VisionEventLoop::VisionEventLoop()
    : trackResult_{0, 0, 0, 0},
      enableTrack_(false),
      resetTracker_(false),
      enableMultiScale_(false),
      quit_(false),
      imageWidth_(0),
      imageHeight_(0)
{
    tracker_.reset(new KCFTracker(enableMultiScale_));
}

VisionEventLoop::~VisionEventLoop() = default;

bool VisionEventLoop::openDevice(const char* deviceName)
{
    device_.reset(new VideoDevice(deviceName));
    if (!device_->isOpened()) return false;

    Size imageSize = device_->imageSize();
    imageWidth_ = imageSize.width;
    imageHeight_ = imageSize.height;

    device_->setFetchFrameCallback([this](char* data, int width, int height, ImageType type)
    {
        if (type == ImageType::YUYV)
        {
            std::unique_ptr<cv::Mat> frame(new cv::Mat());
            cv::cvtColor(cv::Mat(height, width, CV_8UC2, data), *frame, CV_YUV2BGR_YUYV);

            std::lock_guard<std::mutex> lock(mutex_);
            imageBuf_ = std::move(frame);
            notEmpty_.notify_one();
        }
        else if (type == ImageType::BGR)
        {
            cv::Mat img(height, width, CV_8UC3, data);
            std::unique_ptr<cv::Mat> frame(new cv::Mat(img.clone()));

            std::lock_guard<std::mutex> lock(mutex_);
            imageBuf_ = std::move(frame);
            notEmpty_.notify_one();
        }
    });
    device_->start();
    return true;
}

void VisionEventLoop::loop()
{
    while (!quit_)
    {
        std::unique_ptr<cv::Mat> img;
        std::vector<Functor> functors;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            notEmpty_.wait(lock, [this] { return imageBuf_ || !pendingFunctors_.empty() || quit_; });
            img = std::move(imageBuf_);
            functors.swap(pendingFunctors_);
        }
        for (const Functor& functor : functors) functor();
        if (!img) continue;

        std::vector<uint8_t> dataEncode;
        cv::imencode(".jpg", *img, dataEncode, qualityOption); // 将图像编码
        if (sendImageCallback_) sendImageCallback_(dataEncode);

        auto start = util::Timestamp::now();
        track(*img);
        LOG(DEBUG) << "track time: " << (util::Timestamp::now() - start).toMsec() << " ms";
        if (enableTrack_ && sendTrackResultCallback_) sendTrackResultCallback_(trackResult_);
    }
}

void VisionEventLoop::enableMultiScale(bool enable)
{
    tracker_->enableMultiScale(enable);
    enableMultiScale_ = enable;
}

void VisionEventLoop::track(const cv::Mat& frame)
{
    cv::Mat grayImage;
    cv::cvtColor(frame, grayImage, CV_BGR2GRAY);
    grayImage.convertTo(grayImage, CV_32F, 1 / 255.f);
    grayImage -= 0.5f;

    if (resetTracker_)
    {
        resetTracker_ = false;
        int xstart = trackResult_.xpos - trackResult_.width / 2;
        int ystart = trackResult_.ypos - trackResult_.height / 2;
        LOG(INFO) << "pos: " << trackResult_.xpos << "," << trackResult_.ypos << " "
                  << "size: " << trackResult_.width << "," << trackResult_.height;
        tracker_->reset(cv::Rect(xstart, ystart, trackResult_.width, trackResult_.height), grayImage);
    }
    else if (enableTrack_)
    {
        cv::Rect result = tracker_->update(grayImage);
        int xcenter = result.x + result.width / 2;
        int ycenter = result.y + result.height / 2;

        trackResult_ = {xcenter, ycenter, result.width, result.height};
    }

}

} // namespace evt
