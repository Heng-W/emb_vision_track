#ifndef EVT_VISION_EVENT_LOOP_H
#define EVT_VISION_EVENT_LOOP_H

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "../util/common.h"


namespace cv
{
class Mat;

} // namespace cv


namespace evt
{

struct Rect
{
    int xpos, ypos;
    int width, height;
};

class VideoDevice;
class KCFTracker;


class VisionEventLoop
{
public:
    using Functor = std::function<void()>;
    using SendImageCallback = std::function<void(const std::vector<uint8_t>&)>;
    using SendTrackResultCallback = std::function<void(const Rect&)>;

    VisionEventLoop();
    ~VisionEventLoop();

    DISALLOW_COPY_AND_ASSIGN(VisionEventLoop);

    bool openDevice(const char* deviceName = "/dev/video0");

    void loop();

    void quit()
    {
        quit_ = true;
        notEmpty_.notify_one();
    }

    template <class Fn, class... Args>
    void queueInLoop(Fn&& fn, Args&& ... args)
    {
        auto cb = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.emplace_back(std::move(cb));
        }
        notEmpty_.notify_one();
    }

    void setSendImageCallback(const SendImageCallback& cb) { sendImageCallback_ = cb; }

    void setTrackResultCallback(const SendTrackResultCallback& cb) { sendTrackResultCallback_ = cb; }

    void enableTrack(bool enable) { enableTrack_ = enable; }
    bool trackEnabled() const { return enableTrack_; }

    void resetTrack() { resetTracker_ = true; }

    void enableMultiScale(bool enable);
    bool multiScaleEnabled() const { return enableMultiScale_; }

    void resetTarget(const Rect& where)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        trackResult_ = where;
        resetTracker_ = true;
    }

    Rect trackResult() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return trackResult_;
    }

    int imageWidth() const { return imageWidth_; }
    int imageHeight() const { return imageHeight_; }

private:

    void track(const cv::Mat&);


    std::unique_ptr<VideoDevice> device_;
    std::unique_ptr<KCFTracker> tracker_;
    Rect trackResult_;
    std::vector<Functor> pendingFunctors_;
    SendImageCallback sendImageCallback_;
    SendTrackResultCallback sendTrackResultCallback_;

    bool enableTrack_;
    bool resetTracker_;
    bool enableMultiScale_;
    std::atomic<bool> quit_;

    int imageWidth_, imageHeight_;

    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
};

} // namespace evt

#endif // EVT_VISION_EVENT_LOOP_H
