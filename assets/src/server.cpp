
#include "server.h"

#include "util/logger.h"
#include "util/singleton.hpp"
#include "net/event_loop.h"
#include "vision/vision_event_loop.h"
#include "session.h"
#include "data_stream.h"
#include "command.h"
using namespace net;

namespace evt
{

Server::Server(net::EventLoop* loop,
               const net::InetAddress& listenAddr)
    : server_(loop, listenAddr),
      broadcastService_(&server_)
{
    server_.setConnectionCallback([this](const net::TcpConnectionPtr & conn)
    {
        LOG(INFO) << "Server - " << conn->peerAddr().toIpPort() << " -> "
                  << conn->localAddr().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN");
        server_.getLoop()->runInLoop([this]()
        {
            Buffer buf = Session::createBuffer(Command::UPDATE_CLIENT_CNT, sizeof(uint32_t));
            buf.appendUInt32(server_.connections().size());
            Session::packBuffer(&buf);
            broadcastService_.broadcast(std::move(buf));
        });
        if (conn->connected())
        {
            SessionPtr session(new Session(this, conn));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                assert(sessions_.find(conn->id()) == sessions_.end());
                sessions_[conn->id()] = std::move(session);
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(conn->id());
            assert(it != sessions_.end());
            auto& session = it->second;
            if (session->login()) users_.erase(session->userId());
            sessions_.erase(it);
        }
    });
    static auto& vision = util::Singleton<VisionEventLoop>::instance();
    vision.queueInLoop([this]()
    {
        vision.setSendImageCallback([this](const std::vector<uint8_t>& image)
        {
            Buffer buf = Session::createBuffer(Command::IMAGE, sizeof(uint32_t) + image.size());
            LOG(TRACE) << "imageEncodedSize: " << image.size();
            buf.appendUInt32(image.size());
            buf.append(image.data(), image.size());
            Session::packBuffer(&buf);
            broadcastService_.broadcast(std::move(buf));
        });
        vision.setTrackResultCallback([this](const Rect & result)
        {
            Buffer buf = Session::createBuffer(Command::LOCATE, 16);

            DataStreamWriter out(buf.beginWrite());
            out.writeFixed32(result.xpos);
            out.writeFixed32(result.ypos);
            out.writeFixed32(result.width);
            out.writeFixed32(result.height);

            buf.hasWritten(out.size());
            Session::packBuffer(&buf);

            broadcastService_.broadcast(std::move(buf));
        });
    });
}

} // namespace evt
