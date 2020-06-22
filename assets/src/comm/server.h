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

struct EpollData
{
    int fd;
    uint16 userID;
    uint16 flag;
    void* object;
};

class Server
{
public:
    Server();

    void startup();

    bool isRunning()
    {
        return runFlag_;
    }


    void addEpollEvent(EpollData* data);
    void removeEpollEvent(EpollData* data);

    void resetDataPtr(EpollData* data);

    bool checkAccount(int sockfd);
    void acceptEvent();

    void recvMessage(EpollData* data, Command& cmd, uint16& len);

    void epollEventLoop();

    void imageEvent(EpollData* data);

    std::shared_ptr<EpollData> getClientByUserID(uint16 userID);

    int enrollOutEvent();
    int enrollOutEvent(EpollData* data);



    void writeCmd(Command cmd);
    void writeCmd(Command cmd, std::string s);
    void sendMessage(int sockfd);


    void sendToAllClients(Packet& packet);

    void sendToOtherClients(Packet& packet, uint16 userID);


private:
    const int port_ = 18000;
    bool runFlag_;
    uint16 allocatedID_;
    int listenfd_, epollfd_;
    Packet packet_;
    PacketReader pr_;
    int clientCnt_;
    EpollData* outEpollData_;
    std::list<uint16> releaseIDList_;
    std::list<std::shared_ptr<EpollData>> dataPtrList_;
    //std::list<int>::iterator it_;

};

}

#endif //EVT_SERVER_H
