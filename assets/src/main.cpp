
#include "net/event_loop.h"
#include "vision/vision_event_loop.h"
#include "control/control_event_loop.h"
#include "util/config_file_reader.h"
#include "server.h"

using namespace evt;


int main()
{
    util::ConfigFileReader config("conf/var.conf");

    const char* portStr = config.get("port");
    uint16_t port = portStr ? static_cast<uint16_t>(atoi(portStr)) : 18825;

    const char* videoDev = config.get("video_device");
    if (!videoDev) videoDev = "/dev/video0";

    VisionEventLoop visionEventLoop;
    ControlEventLoop controlEventLoop;

    std::thread visionThread([&visionEventLoop]()
    {
        visionEventLoop.loop();
    });
    std::thread controlThread([&controlEventLoop]()
    {
        controlEventLoop.loop();
    });

    net::EventLoop loop;
    Server server(&loop, net::InetAddress(port));
    server.start();
    loop.loop();

    visionEventLoop.quit();
    controlEventLoop.quit();

    visionThread.join();
    controlThread.join();

    return 0;
}

