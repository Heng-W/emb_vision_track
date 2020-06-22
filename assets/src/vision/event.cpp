#include "event.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include <string>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"

#include "video_frame.h"
#include "kcf_tracker/kcf_tracker.h"
#include "image_packet.h"
#include "yuyv2bgr.h"
#include "locate.h"



namespace EVTrack
{

using namespace kcf;

namespace vision
{

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex;

static bool exitFlag = false;
static bool produceOverFlag = false;

static bool disposeOverFlag = true;
static bool disposethreadSleepFlag = false;

static bool encodeOverFlag = true;
static bool packetthreadSleepFlag = false;


bool trackFlag = false;
bool setTargetFlag = false;
bool setMultiScaleFlag = false;

bool useMultiScale = false;

const int TARGET_DEF_W = 100;

int targetXstart = VideoFrame::IMAGE_W / 2 - TARGET_DEF_W / 2;
int targetYstart = VideoFrame::IMAGE_H / 2 - TARGET_DEF_W / 2;
int targetWidth = TARGET_DEF_W;
int targetHeight = TARGET_DEF_W;


VideoFrame videoFrame;
KCFTracker tracker(useMultiScale);
ImagePacket imagePacket;

//cv::VideoCapture cap(0);
cv::Mat& frame = videoFrame.getFrameRef();
static cv::Rect result = cv::Rect(targetXstart, targetYstart, targetWidth, targetHeight);

static void* pickThread(void* pVoid)
{
    Event* p = (Event*)pVoid;
    p->pickEventLoop();
    return p;
}

static void* disposeThread(void* pVoid)
{
    Event* p = (Event*)pVoid;
    p->disposeEventLoop();
    return p;
}

static void* packetThread(void* pVoid)
{
    Event* p = (Event*)pVoid;
    p->packetEventLoop();
    return p;
}


static void sleepMs(unsigned long ms)
{
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}


Event::Event(Server& server):
    server_(server)
{
    videoFrame.dispFormat();

    int ret = pthread_create(&pickThread_, NULL, pickThread, this);
    if (ret != 0)
    {
        perror("pickThread");
    }

    ret = pthread_create(&disposeThread_, NULL, disposeThread, this);
    if (ret != 0)
    {
        perror("disposeThread");
    }

    ret = pthread_create(&packetThread_, NULL, packetThread, this);
    if (ret != 0)
    {
        perror("packetThread");
    }

}


//cv::Mat frame;
clock_t begin, end;
clock_t tmp1, tmp2, tmp3;
static int tim_cnt;
int pickDelay = 100;
int pickTimeOutDelay = 50;
cv::Mat grayImage;

static void disposeRawImage(void* rawImage)
{
    if (disposeOverFlag && encodeOverFlag)
    {
        videoFrame.convertImage(rawImage);

        produceOverFlag = 1;

    }

}



void Event::pickEventLoop()
{
    videoFrame.setDisposeFunction(&disposeRawImage);
    sleepMs(1000);

    while (true)
    {
        if (videoFrame.update() == -2)
        {
            sleepMs(pickTimeOutDelay);
            continue;
        }


        if (produceOverFlag && disposethreadSleepFlag && packetthreadSleepFlag)
        {

            produceOverFlag = false;

            disposeOverFlag = false;
            encodeOverFlag = false;

            disposethreadSleepFlag = false;
            packetthreadSleepFlag = false;

            if (!server_.isRunning())
                exitFlag = true;


            pthread_mutex_lock(&mutex);
            pthread_cond_broadcast(&cond);
            // pthread_cond_signal(&cond);//唤醒
            pthread_mutex_unlock(&mutex);

            if (exitFlag)
                break;
        }

        sleepMs(pickDelay);
    }
}



void Event::disposeEventLoop()
{
    pthread_mutex_lock(&mutex);
    disposethreadSleepFlag = true;
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    cv::cvtColor(frame, grayImage, CV_BGR2GRAY);
    grayImage.convertTo(grayImage, CV_32F, 1 / 255.f);
    grayImage -= (float) 0.5;

    tracker.init(cv::Rect(targetXstart, targetYstart, targetWidth, targetHeight), grayImage);

    while (true)
    {
        cv::cvtColor(frame, grayImage, CV_BGR2GRAY);
        grayImage.convertTo(grayImage, CV_32F, 1 / 255.f);
        grayImage -= (float) 0.5;

        if (setMultiScaleFlag)
        {
            setMultiScaleFlag = false;
            tracker.setTrackMode(useMultiScale);
        }
        if (setTargetFlag)
        {
            setTargetFlag = false;
            tracker.reset(cv::Rect(targetXstart, targetYstart, targetWidth, targetHeight), grayImage);
        }
        else if (trackFlag)
        {
            begin = clock();
            result = tracker.update(grayImage);
            end = clock();
            std::cout << "track: " << (end - begin) << " / " << CLOCKS_PER_SEC << std::endl;
        }

        disposeOverFlag = true;

        int xcenter = result.x + result.width / 2;
        int ycenter = result.y + result.height / 2;

        pthread_mutex_lock(&mutex);

        Locate::xpos = xcenter;
        Locate::ypos = ycenter;
        Locate::width = result.width;
        Locate::height = result.height;

        disposethreadSleepFlag = true;
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);

        if (exitFlag)
        {
            break;
        }

    }
}

void Event::packetEventLoop()
{
    pthread_mutex_lock(&mutex);
    packetthreadSleepFlag = true;
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    while (true)
    {
        imagePacket.encodeImage(frame);
        encodeOverFlag = true;

        imagePacket.makePacket(server_);


        pthread_mutex_lock(&mutex);
        packetthreadSleepFlag = true;
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);

        if (exitFlag)
            break;
    }
}

}
}
