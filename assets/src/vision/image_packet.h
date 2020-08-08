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

    //生成图像数据包
    void makePacket(Server& server);

    //编码压缩图像为JPEG格式
    void encodeImage(cv::Mat& frame);

    //图像数据包更新
    int update();

    //生成目标位置数据包
    void makeLocatePacket();

    //一帧图像数据是否发送完成
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
    const int JPEG_QUALITY_DEFAULT_VALUE = 75;//JPEG图像质量，默认95

    Packet packet_;
    //  bool frameUpdateFlag;
    int packetCnt_;//图像数据包数量
    int packetIdx_;//图像数据包当前索引
    uint8* imageAddr_;//图像数据实际地址
    int imageLen_;//图像数据长度
    bool sendFlag_;


};


}


#endif  //EVT_IMAGE_PACKET_H
