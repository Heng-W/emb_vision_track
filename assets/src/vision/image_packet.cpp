
#include "image_packet.h"

#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <vector>

#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/imgcodecs/legacy/constants_c.h"
#include "opencv2/videoio/legacy/constants_c.h"

#include "locate.h"
#include "../comm/server.h"


namespace EVTrack
{

#define PACKET_MAX_SIZE 4096


std::vector<uchar> dataEncode;

uint8 msg[50];
//Packet packet(msg);


ImagePacket::ImagePacket():
    packet_(msg),
    sendFlag_(false)
{
    quality_.push_back(CV_IMWRITE_JPEG_QUALITY);
    quality_.push_back(JPEG_QUALITY_DEFAULT_VALUE);//压缩
}


void ImagePacket::encodeImage(cv::Mat& frame)
{
    imencode(".jpg", frame, dataEncode, quality_); //将图像编码
}


void ImagePacket::makePacket(Server& server)
{
    if (!sendFlag_) return;
    sendFlag_ = false;

    packetIdx_ = 0;
    packetCnt_ = (dataEncode.size() - 1) / PACKET_MAX_SIZE + 1;
    //   std::cout << packetCnt << std::endl;
    //   std::cout << dataEncode.size() << std::endl;


    packet_.writeHeader(Command::image);
    packet_.writeUint32(dataEncode.size());//图像总大小
    unsigned int nsize = PACKET_MAX_SIZE;
    if (packetIdx_ == packetCnt_ - 1)
        nsize = dataEncode.size() - packetIdx_ * PACKET_MAX_SIZE;
    packet_.writeUint32(nsize);//当前大小
    packet_.writeUint32(packetIdx_ * PACKET_MAX_SIZE); //位置偏移
    packet_.pack();


    imageAddr_ = &dataEncode[packetIdx_ * PACKET_MAX_SIZE];
    imageLen_ = nsize;


    server.enrollOutEvent();

    /*
    std::shared_ptr<EpollData> epollData = server.getClientByUserID(0);
    if (epollData == NULL)
    {
        return;
    }
    //epollData->object = this;
    server.enrollOutEvent(epollData.get());
    epollData.reset();
    //   std::cout << epollData.use_count() << std::endl;
    */

}


int ImagePacket::update()
{
    packetIdx_++;
    if (packetIdx_ == packetCnt_) return -1;
    packet_.writeHeader(Command::image);
    packet_.writeUint32(dataEncode.size());//图像总大小
    unsigned int nsize = PACKET_MAX_SIZE;
    if (packetIdx_ == packetCnt_ - 1)
        nsize = dataEncode.size() - packetIdx_ * PACKET_MAX_SIZE;
    packet_.writeUint32(nsize);//当前大小
    packet_.writeUint32(packetIdx_ * PACKET_MAX_SIZE); //位置偏移
    packet_.pack();
    imageAddr_ = &dataEncode[packetIdx_ * PACKET_MAX_SIZE];
    imageLen_ = nsize;
    return 0;
}

void ImagePacket::makeLocatePacket()
{
    packet_.writeHeader(Command::locate);
    packet_.writeInt32(Locate::xpos);
    packet_.writeInt32(Locate::ypos);
    packet_.writeInt32(Locate::width);
    packet_.writeInt32(Locate::height);
    packet_.pack();
}

}
