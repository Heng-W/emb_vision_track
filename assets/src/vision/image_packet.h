#ifndef EVT_IMAGE_PACKET_H
#define EVT_IMAGE_PACKET_H

//#include "opencv2/opencv.hpp"
#include <vector>
#include "../comm/packet.h"

namespace cv
{
class Mat;
}

namespace EVTrack
{
class Server;


class ImagePacket
{
public:
    ImagePacket();

    void makePacket(Server& server);

    void encodeImage(cv::Mat& frame);
    int update();

    void makeLocatePacket();

    bool isOver()
    {
        return packetIdx_ + 1 == packetCnt_;
    }

    int getPacketLen()
    {
        return packet_.getLength();
    }

    uint8* getPacketMsg()
    {
        return packet_.getMessage();
    }

    int getImageLen()
    {
        return imageLen_;
    }

    uint8* getImageAddr()
    {
        return imageAddr_;
    }

    void setSendFlag(bool flag)
    {
        sendFlag_ = flag;
    }


private:

    std::vector<int> quality_;
    const int JPEG_QUALITY_DEFAULT_VALUE = 75;

    Packet packet_;
    //  bool frameUpdateFlag;
    int packetCnt_;
    int packetIdx_;
    uint8* imageAddr_;
    int imageLen_;
    bool sendFlag_;


};


}


#endif  //EVT_IMAGE_PACKET_H
