#ifndef EVT_SERVER_H
#define EVT_SERVER_H


#include "common.h"

#include <string>
#include <list>
#include <iterator>
#include <memory>
#include "packet.h"


namespace EVTrack
{

//用户信息结构体
struct EpollData
{
    int fd;//描述符
    uint16 userID;//用户ID值(0-65535)
    uint16 flag;//用户状态标志
    void* object;
};

class Server
{
public:
    Server();

    //启动服务器
    void startup();

    //获取运行状态
    bool isRunning()
    {
        return runFlag_;
    }

    //对用户添加或移除EPOLL监听
    void addEpollEvent(EpollData* data);
    void removeEpollEvent(EpollData* data);

    //用户连接断开后，解除内存引用并从用户列表中移除
    void resetDataPtr(EpollData* data);

    //验证账户
    bool checkAccount(int sockfd);
    
    //新用户连接事件
	void acceptEvent();

    //接收消息处理
    void recvMessage(EpollData* data, Command& cmd, uint16& len);

    //事件主循环，监听触发的EPOLL事件
    void epollEventLoop();

    //图像数据包群发
    void imageEvent(EpollData* data);

    //通过ID获取用户信息
    std::shared_ptr<EpollData> getClientByUserID(uint16 userID);

    //注册EPOLLOUT事件
    int enrollOutEvent();
    int enrollOutEvent(EpollData* data);

    //写消息
    void writeCmd(Command cmd);
    void writeCmd(Command cmd, std::string s);
    void sendMessage(int sockfd);

    //向所有用户发送数据包
    void sendToAllClients(Packet& packet);

    //向除自己以外的其它用户发送数据包
    void sendToOtherClients(Packet& packet, uint16 userID);


private:
    const int port_ = 18000;//连接监听端口号
    bool runFlag_;
    uint16 allocatedID_;//当前已分配的最大ID值
    int listenfd_, epollfd_;
    Packet packet_;
    PacketReader pr_;
    int clientCnt_;//用户连接数量
    EpollData* outEpollData_;
    std::list<uint16> releaseIDList_;//已释放、可重新分配的ID列表
    std::list<std::shared_ptr<EpollData>> dataPtrList_;//用户列表
    //std::list<int>::iterator it_;

};

}

#endif //EVT_SERVER_H
