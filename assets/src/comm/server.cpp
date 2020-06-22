
#include "server.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>

#include "../vision/image_packet.h"
#include "../vision/locate.h"
#include "../control/field_control.h"
#include "../control/motion_control.h"
#include "../control/control_macro.hpp"


#define BACKLOG 10
#define MAX_FD_NUM 50
#define MAX_BUF_SIZE 1024

#define ALLOCATED_MIN_ID 20000
#define ALLOCATED_MAX_ID 65000


namespace EVTrack
{

namespace vision
{

extern ImagePacket imagePacket;

extern bool trackFlag;
extern bool setTargetFlag;
extern bool setMultiScaleFlag;

extern bool useMultiScale;

extern int targetXstart;
extern int targetYstart;
extern int targetWidth;
extern int targetHeight;

}

using namespace vision;

int checkAccountByName(uint8 type, char* name, char* pwd, uint16* userID);
int checkAccountById(int id, char* pwd, char* name);

void writeSysCmdResult(Packet& p, const char* cmd);


static struct epoll_event events[MAX_FD_NUM];
static uint8 recvBuf[MAX_BUF_SIZE];
static uint8 sendBuf[MAX_BUF_SIZE];


void setnonblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
    {
        perror("get fcntl flag");
        return;
    }
    int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (ret == -1)
    {
        perror("set fcntl non-blocking");
        return;
    }
}



Server::Server():
    runFlag_(true),
    allocatedID_(ALLOCATED_MIN_ID),
    packet_(sendBuf),
    pr_(recvBuf),
    clientCnt_(0)
{}

void Server::startup()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);//服务端监听套接字
    if (listenfd_ < 0)
    {
        perror("create socket");
        return;
    }

    struct sockaddr_in  serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port_);

    int on = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt failed");
        return;
    }
    //绑定端口
    if (bind(listenfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind");
        return;
    }
    //监听
    if (listen(listenfd_, BACKLOG) < 0)
    {
        perror("listen");
        return;
    }

    //内核中创建事件表
    epollfd_ = epoll_create(MAX_FD_NUM);
    if (epollfd_ < 0)
    {
        perror("epoll create");
        return;
    }

    EpollData* data = new EpollData();
    data->fd = listenfd_;
    data->userID = 0xffff;

    addEpollEvent(data);//监听事件


    data = new EpollData();
    data->fd = STDIN_FILENO;
    data->userID = 0xffff;

    addEpollEvent(data);//标准输入


    std::cout << "waiting for client's request..." << std::endl;

    epollEventLoop();
}


inline void Server::writeCmd(Command cmd)
{
    packet_.writeHeader(cmd);
}

inline void Server::writeCmd(Command cmd, std::string s)
{
    packet_.writeHeader(cmd);
    packet_.writeString(s);
}


inline void Server::sendMessage(int sockfd)
{
    packet_.pack();
    send(sockfd, sendBuf, packet_.getLength(), 0);
}



void Server::addEpollEvent(EpollData* data)
{
    struct epoll_event ev;

    setnonblock(data->fd);//非阻塞

    ev.data.ptr = data;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, data->fd, &ev);

    dataPtrList_.push_back(std::shared_ptr<EpollData>(data));

    if (data->userID != 0xffff)
    {
        clientCnt_++;
    }

    if (clientCnt_ <= 0) return;

    writeCmd(Command::update_client_cnt);
    packet_.writeInt32(clientCnt_);
    packet_.pack();

    sendToAllClients(packet_);

}


void Server::removeEpollEvent(EpollData* data)
{
    close(data->fd);
    //epoll_ctl(epollfd_, EPOLL_CTL_DEL, data->fd, &ev);
    std::cout << "close fd " << data->fd << std::endl;

    //data->flag |= 0x3;

    if (data->userID >= ALLOCATED_MIN_ID)
    {
        if (data->userID == allocatedID_ - 1)
        {
            allocatedID_--;
        }
        else
        {
            releaseIDList_.push_back(data->userID);
        }
    }


    resetDataPtr(data);

    clientCnt_--;


    if (clientCnt_ <= 0) return;

    writeCmd(Command::update_client_cnt);
    packet_.writeInt32(clientCnt_);
    packet_.pack();

    sendToAllClients(packet_);


}

void Server::resetDataPtr(EpollData* data)
{
    std::list<std::shared_ptr<EpollData>>::iterator it;
    for (it = dataPtrList_.begin(); it != dataPtrList_.end();)
    {
        if ((*it).get() == data)
        {
            std::cout << "reduce reference fd " << (*it)->fd << std::endl;
            (*it).reset();//解除引用
            it = dataPtrList_.erase(it);//指向erase元素的后一个元素
            // std::cout << "total of clients: " << clientCnt_ << std::endl;
        }
        else
        {
            it++;
        }
    }
}



bool Server::checkAccount(int sockfd)
{
    int ret = recv(sockfd, recvBuf, 3, 0);
    if (ret <= 0) return false;
    uint8 type = recvBuf[0];
    uint16 userID = recvBuf[1] | recvBuf[2] << 8;
    int retCode = 0;

    char userName[16];
    memset(userName, 0, 16);

    if (type != 2)
    {
        if (userID == 0x1)
        {
            ret = recv(sockfd, recvBuf, 32, 0);
            if (ret <= 0) return false;

            char* buf = (char*)recvBuf;

            memcpy(userName, buf, 16);


            retCode = checkAccountByName(type, buf, buf + 16, &userID);

        }
        else
        {
            ret = recv(sockfd, recvBuf, 16, 0);
            if (ret <= 0) return false;


            retCode = checkAccountById(userID, (char*)recvBuf, userName);
        }
    }
    else //guest用户自动分配ID
    {
        if (!releaseIDList_.empty())
        {
            userID = releaseIDList_.front();
            releaseIDList_.pop_front();
        }
        else if (allocatedID_ <= ALLOCATED_MAX_ID)
        {
            userID = allocatedID_++;
        }
        else
        {
            retCode = 10;
        }
        strcpy(userName, "guest");
    }


    send(sockfd, &retCode, 4, 0);

    if (retCode == 0)
    {
        send(sockfd, &userID, 2, 0);
        send(sockfd, userName, 16, 0);
        std::cout << "User " << userID << " login successful" << std::endl;
        std::cout << "create sockfd " << sockfd << std::endl;

        //防止重复登录
        if (userID < ALLOCATED_MIN_ID)
        {
            std::shared_ptr<EpollData> oldUserData = getClientByUserID(userID);
            if (oldUserData != NULL)
            {
                writeCmd(Command::logout);
                packet_.writeInt32(1);
                sendMessage(oldUserData->fd);

                removeEpollEvent(oldUserData.get());

            }
        }

        EpollData* data = new EpollData();
        data->fd = sockfd;
        data->userID = userID;

        addEpollEvent(data);


        //std::cout << "total of clients: " << getClientCnt() << std::endl;

    }
    else
    {
        std::cout << "User " << userID << " login failed" << std::endl;
        std::cout << "retCode " << retCode << std::endl;
        close(sockfd);
    }
    return (retCode == 0) ? true : false;
}


void Server::acceptEvent()
{
    int sockfd = accept(listenfd_, (struct sockaddr*)NULL, NULL);
    if (sockfd < 0)
    {
        perror("accept socket");
        return;
    }
    //验证账户
    checkAccount(sockfd);

}


void Server::recvMessage(EpollData* data, Command& cmd, uint16& len)
{
    int sockfd = data->fd;

    if (cmd != Command::image)
    {

        std::cout << "sockfd " << sockfd << ": recvfrom user " << data->userID;
        std::cout << " cmd:" << static_cast<uint16>(cmd) << " len:" << len << std::endl;
    }

    switch (cmd)
    {
        case Command::init:
            writeCmd(cmd);
            packet_.writeInt8((int8)trackFlag);

            packet_.writeInt8((int8)MotionControl::autoCtlFlag);

            packet_.writeInt8((int8)FieldControl::autoCtlFlag);

            packet_.writeInt8((int8)useMultiScale);


            packet_.writeInt32(MotionControl::leftCtlVal);
            packet_.writeInt32(MotionControl::rightCtlVal);

            packet_.writeFloat(FieldControl::angleHDef);
            packet_.writeFloat(FieldControl::angleVDef);

            packet_.writeFloat(FieldControl::angleH);
            packet_.writeFloat(FieldControl::angleV);

            for (int i = 0; i < 4; i++)
            {
                packet_.writeFloat(FieldControl::pidParams[i]);
            }
            for (int i = 0; i < 4; i++)
            {
                packet_.writeFloat(MotionControl::pidParams[i]);
            }

            sendMessage(sockfd);
            break;

        case Command::update_pid:
            writeCmd(cmd);

            for (int i = 0; i < 4; i++)
            {
                packet_.writeFloat(FieldControl::pidParams[i]);
            }
            for (int i = 0; i < 4; i++)
            {
                packet_.writeFloat(MotionControl::pidParams[i]);
            }
            sendMessage(sockfd);
            break;

        case Command::reset:
        {
            int type = pr_.readInt32();
            if (type & 0x1)
            {
                trackFlag = false;
                MotionControl::autoCtlFlag = false;
                FieldControl::autoCtlFlag = false;
                // MotionControl::stopSignal = true;
            }
            if (type & 0x2)
            {
                FieldControl::angleHSet = FieldControl::angleHDef * 100;
                FieldControl::angleVSet =  FieldControl::angleVDef * 100;
                FieldControl::autoCtlFlag = false;
                FieldControl::valSetFlag = true;
            }
            if (type & 0x4)
            {
                MotionControl::stopSignal = true;
                MotionControl::autoCtlFlag = false;

            }
            writeCmd(cmd);
            packet_.writeInt32(type);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);
            break;
        }
        case Command::message:

            writeCmd(cmd);
            packet_.writeFloat(FieldControl::angleH);
            packet_.writeFloat(FieldControl::angleV);
            packet_.writeInt32(MotionControl::leftCtlVal);
            packet_.writeInt32(MotionControl::rightCtlVal);

            packet_.writeInt32(Locate::xpos);
            packet_.writeInt32(Locate::ypos);
            packet_.writeInt32(Locate::width);
            packet_.writeInt32(Locate::height);

            sendMessage(sockfd);
            break;

        case Command::image:
            data->flag |= 0x80;

            imagePacket.setSendFlag(true);
            break;
        case Command::start_track:
            trackFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::stop_track:
            trackFlag = false;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::set_target:
            if (len < 8)
                break;
            Locate::xpos = targetXstart = (int)pr_.readUint16();
            Locate::ypos = targetYstart = (int)pr_.readUint16();
            Locate::width = targetWidth = (int)pr_.readUint16();
            Locate::height = targetHeight = (int)pr_.readUint16();
            setTargetFlag = true;
            writeCmd(cmd);
            sendMessage(sockfd);
            break;
        case Command::enable_motion_autoctl:
            MotionControl::autoCtlFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::disable_motion_autoctl:
            MotionControl::autoCtlFlag = false;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::stop_motor:

            MotionControl::stopSignal = true;
            MotionControl::autoCtlFlag = false;

            writeCmd(cmd);
            sendMessage(sockfd);
            break;
        case Command::start_motor:
            MotionControl::startSignal = true;
            writeCmd(cmd);
            sendMessage(sockfd);
            break;
        case Command::set_motor_val:
            MotionControl::leftValSet = pr_.readInt32();
            MotionControl::rightValSet = pr_.readInt32();
            MotionControl::valSetFlag = true;
            writeCmd(cmd);
            sendMessage(sockfd);
            break;
        case Command::get_motor_val:
            writeCmd(cmd);
            packet_.writeInt32(MotionControl::leftCtlVal);
            packet_.writeInt32(MotionControl::rightCtlVal);
            sendMessage(sockfd);
            break;

        case Command::enable_field_autoctl:
            FieldControl::autoCtlFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::disable_field_autoctl:
            FieldControl::autoCtlFlag = false;
            writeCmd(cmd);
            packet_.writeFloat(FieldControl::angleH);
            packet_.writeFloat(FieldControl::angleV);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        case Command::set_servo_val:
            FieldControl::angleHSet = pr_.readInt32();
            FieldControl::angleVSet = pr_.readInt32();
            FieldControl::valSetFlag = true;
            writeCmd(cmd);
            sendMessage(sockfd);
            break;
        case Command::get_servo_val:
            writeCmd(cmd);
            packet_.writeFloat(FieldControl::angleH);
            packet_.writeFloat(FieldControl::angleV);
            sendMessage(sockfd);
            break;

        case Command::set_motion_ctl_pid:
        {
            int motionParamNum = pr_.readInt32();
            MotionControl::pidParams[motionParamNum] = pr_.readFloat();
            std::cout << MotionControl::pidParams[motionParamNum] << std::endl;
            MotionControl::pidSetFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        }
        case Command::set_field_ctl_pid:
        {
            int fieldParamNum = pr_.readInt32();
            FieldControl::pidParams[fieldParamNum] = pr_.readFloat();
            std::cout << FieldControl::pidParams[fieldParamNum] << std::endl;
            FieldControl::pidSetFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;
        }

        case Command::send_to_client:
        {
            uint16 userID = pr_.readUint16();
            std::string str = pr_.readString();
            std::shared_ptr<EpollData> targetClient = getClientByUserID(userID);
            if (targetClient != NULL)
            {
                writeCmd(Command::notice, str);
                sendMessage(targetClient->fd);
            }
            break;
        }


        case Command::send_to_all_clients:
        {
            std::string str = pr_.readString();

            writeCmd(Command::send_to_all_clients);
            sendMessage(sockfd);


            writeCmd(Command::notice, str);
            packet_.pack();

            sendToOtherClients(packet_, data->userID);
            break;
        }
        case Command::system_cmd:
        {

            std::string str = pr_.readString();
            writeCmd(cmd);
            writeSysCmdResult(packet_, str.c_str());
            sendMessage(sockfd);
            break;
        }
        case Command::enable_multi_scale:
            useMultiScale = true;
            setMultiScaleFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;

        case Command::disable_multi_scale:
            useMultiScale = false;
            setMultiScaleFlag = true;
            writeCmd(cmd);
            packet_.writeUint16(data->userID);
            packet_.pack();
            sendToAllClients(packet_);

            break;

        case Command::halt:
            system("halt");
            break;
        case Command::reboot:
            system("reboot");
            break;
        case Command::exit:
            writeCmd(cmd);
            sendMessage(sockfd);
            removeEpollEvent(data);
            break;
        default:
            break;

    }
}


void Server::epollEventLoop()
{
    int nfds;
    int ret, i;
    struct EpollData* data;
    while (runFlag_)
    {
        nfds = epoll_wait(epollfd_, events, MAX_FD_NUM, -1);//等待epoll事件
        if (nfds < 0)
        {
            perror("epoll failed");
            break;
        }
        for (i = 0; i < nfds; i++)
        {
            //fd = events[i].data.fd;
            data = (EpollData*)events[i].data.ptr;
            //accept new connection
            if (data->fd == listenfd_)
            {

                acceptEvent();

            }
            //std input
            else if (data->fd == STDIN_FILENO)
            {
                if (read(data->fd, recvBuf, 20) > 0)
                {
                    char* buf = (char*)recvBuf;
                    //buf[strlen(buf) - 1] = '\0';
                    strtok(buf, "\n");
                    //std::cout<<buf<<std::endl;
                    if (strcmp(buf, "exit") == 0)
                    {

                        runFlag_ = false;
                    }
                    else if (strcmp(buf, "send") == 0)
                    {

                        writeCmd(Command::notice, "hello");
                        packet_.pack();

                        sendToAllClients(packet_);

                    }
                }
            }
            //recv message
            else if (events[i].events & EPOLLIN)
            {
                if (data->fd < 0)
                    continue;
                ret = recv(data->fd, recvBuf, 6, 0);

                if (ret == 0 || (ret < 0 && errno != EAGAIN && errno != EINTR))
                {
                    removeEpollEvent(data);
                    continue;
                }
                if (ret < 6)
                {
                    continue;
                }
                pr_.reset();
                if (!pr_.isHeader())
                {
                    std::cout << "packet error!" << std::endl;
                    continue;
                }
                Command cmd = static_cast<Command>(pr_.readUint16());
                uint16 len = pr_.readUint16();
                if (len > 0)
                {
                    int recvLen = 0;
                    while (recvLen < len)
                    {
                        ret = recv(data->fd, recvBuf + recvLen, len - recvLen, 0);
                        if (ret <= 0)
                        {
                            if (ret < 0 && (errno == EAGAIN || errno == EINTR))
                            {
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        recvLen += ret;
                    }
                    if (recvLen != len)
                    {
                        removeEpollEvent(data);
                        continue;
                    }
                }
                pr_.reset();
                recvMessage(data, cmd, len);
            }
            //send message
            else if (events[i].events & EPOLLOUT)
            {
                imageEvent(data);
            }

            else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP)
            {
                std::cout << "epoll error" << std::endl;
                removeEpollEvent(data);
            }
            else
            {
                std::cout << "other event" << std::endl;
            }
        }
    }

    for (std::shared_ptr<EpollData> epollData : dataPtrList_)
    {
        close(epollData->fd);
        std::cout << "close fd " << epollData->fd << std::endl;
        epollData.reset();
    }
    dataPtrList_.clear();


    close(epollfd_);
    std::cout << "exit process" << std::endl;
}


void Server::imageEvent(EpollData* data)
{
    int ret;

    for (std::shared_ptr<EpollData> client : dataPtrList_)
    {
        if (client->userID == 0xffff) continue;
        if (((client->flag) & 0x80) == 0) continue;

        ret = send(client->fd, imagePacket.getPacketMsg(), imagePacket.getPacketLen(), 0);
        if (ret == 0 || (ret < 0 && errno != EAGAIN && errno != EINTR))
        {
            removeEpollEvent(client.get());
            continue;
        }

        if (imagePacket.getImageLen() <= 0) continue;

        ret = send(client->fd, imagePacket.getImageAddr(), imagePacket.getImageLen(), 0);
        if (ret == 0 || (ret < 0 && errno != EAGAIN && errno != EINTR))
        {
            removeEpollEvent(client.get());
            continue;
        }
    }

    if (imagePacket.update() != -1)
    {
        return;
    }

    imagePacket.makeLocatePacket();
    for (std::shared_ptr<EpollData> client : dataPtrList_)
    {
        if (client->userID == 0xffff) continue;
        if (((client->flag) & 0x80) == 0) continue;

        ret = send(client->fd, imagePacket.getPacketMsg(), imagePacket.getPacketLen(), 0);
        if (ret == 0 || (ret < 0 && errno != EAGAIN && errno != EINTR))
        {
            removeEpollEvent(client.get());
            continue;
        }
        client->flag &= ~0x80;
    }

    struct epoll_event ev;
    ev.data.ptr = data;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd_, EPOLL_CTL_MOD, data->fd, &ev);
    data->flag |= 1;
}

std::shared_ptr<EpollData> Server::getClientByUserID(uint16 userID)
{
    for (std::shared_ptr<EpollData> data : dataPtrList_)
    {
        if (data->userID == userID)
        {
            return data;
        }
    }
    return NULL;

}

int Server::enrollOutEvent()
{

    std::shared_ptr<EpollData> data = dataPtrList_.back();

    if (clientCnt_ <= 0)
    {
        return 1;

    }

    struct epoll_event ev;

    ev.data.ptr = data.get();
    ev.events = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(epollfd_, EPOLL_CTL_MOD, data->fd, &ev) < 0)
    {
        perror("enroll out event");
        return -1;
    }
    return 0;

}

int Server::enrollOutEvent(EpollData* data)
{
    if (clientCnt_ <= 0) return 1;

    struct epoll_event ev;

    ev.data.ptr = data;
    ev.events = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(epollfd_, EPOLL_CTL_MOD, data->fd, &ev) < 0)
    {
        perror("enroll out event");
        data->flag |= 0x3;
        return -1;
    }
    return 0;

}


void Server::sendToAllClients(Packet& packet)
{
    if (clientCnt_ == 1)
    {
        send((dataPtrList_.back())->fd, sendBuf, packet.getLength(), 0);
        return;
    }
    for (std::shared_ptr<EpollData> client : dataPtrList_)
    {
        if (client->userID != 0xffff)
        {
            send(client->fd, sendBuf, packet.getLength(), 0);

        }
    }

}

void Server::sendToOtherClients(Packet& packet, uint16 userID)
{
    if (clientCnt_ <= 1) return;
    for (std::shared_ptr<EpollData> client : dataPtrList_)
    {
        if (client->userID != 0xffff && client->userID != userID)
        {
            send(client->fd, sendBuf, packet.getLength(), 0);
        }
    }

}



}



