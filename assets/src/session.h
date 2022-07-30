#ifndef EVT_SESSION_H
#define EVT_SESSION_H

#include "net/tcp_connection.h"
#include "broadcast_service.h"

namespace evt
{

class Server;
enum class Command;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(Server* server, const net::TcpConnectionPtr& conn);
    ~Session();

    net::TcpConnectionPtr connection() const { return conn_; }

    void logout();

    uint32_t userId() const { return userId_; }
    bool login() const { return login_; }

    static net::Buffer createBuffer(Command cmd, int initialSize = 0);
    static void packBuffer(net::Buffer* buf);

private:
    void parseMessage(const net::TcpConnectionPtr& conn, const char* data, int len);

    Server* server_;
    net::TcpConnectionPtr conn_;
    net::BroadcastService* broadcastService_;
    bool login_;
    uint32_t userId_;
    uint8_t type_;
};

using SessionPtr = std::shared_ptr<Session>;

} // namespace evt

#endif // EVT_SESSION_H
