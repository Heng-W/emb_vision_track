
#include "inet_address.h"

#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../util/logger.h"
#include "endian.h"

namespace net
{

constexpr in_addr_t kInaddrAny = INADDR_ANY;
constexpr in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = hostToNetwork32(ip);
    addr_.sin_port = hostToNetwork16(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
    {
        SYSLOG(ERROR) << "InetAddress::InetAddress";
    }
}

std::string InetAddress::toIpPort() const
{
    char buf[64];
    size_t size = sizeof(buf);
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, static_cast<socklen_t>(size));
    size_t end = ::strlen(buf);
    uint16_t port = networkToHost16(addr_.sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
    return buf;
}

std::string InetAddress::toIp() const
{
    char buf[64];
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, static_cast<socklen_t>(sizeof(buf)));
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return networkToHost16(addr_.sin_port);
}

bool InetAddress::resolve(const std::string& hostname, InetAddress* out)
{
    thread_local char t_resolveBuffer[64 * 1024];

    assert(out != nullptr);
    struct hostent hent;
    struct hostent* he = nullptr;
    int herrno = 0;
    memset(&hent, 0, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
    if (ret == 0 && he != nullptr)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }
    else
    {
        if (ret)
        {
            SYSLOG(ERROR) << "InetAddress::resolve";
        }
        return false;
    }
}

} // namespace net
