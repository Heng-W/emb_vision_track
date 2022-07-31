
#include "net/event_loop.h"
#include "vision/vision_event_loop.h"
#include "control/control_event_loop.h"
#include "util/logger.h"
#include "util/config_file_reader.h"
#include "util/singleton.hpp"
#include "server.h"

using namespace evt;


int main()
{
    util::ConfigFileReader config("conf/var.conf");

    VisionEventLoop& visionEventLoop = util::Singleton<VisionEventLoop>::instance();
    ControlEventLoop& controlEventLoop = util::Singleton<ControlEventLoop>::instance();

    const char* videoDevice = config.get("video_device");
    if (!videoDevice) videoDevice = "/dev/video0";
    visionEventLoop.openDevice(videoDevice);

    const char* servoDevice = config.get("servo_device");
    if (!servoDevice) servoDevice = "/dev/servo";
    controlEventLoop.fieldControl().openDevice(servoDevice);

    const char* motorDevice = config.get("motor_device");
    if (!motorDevice) motorDevice = "/dev/motor";
    controlEventLoop.motionControl().openDevice(motorDevice);

    std::thread visionThread([&visionEventLoop]
    {
        visionEventLoop.loop();
    });
    std::thread controlThread([&controlEventLoop]
    {
        controlEventLoop.loop();
    });

    net::EventLoop loop;

    const char* portStr = config.get("listen_port");
    uint16_t port = portStr ? static_cast<uint16_t>(atoi(portStr)) : 18825;
    LOG(INFO) << "listen port: " << port;

    Server server(&loop, net::InetAddress(port));
    server.start();
    loop.loop();

    visionEventLoop.quit();
    controlEventLoop.quit();

    visionThread.join();
    controlThread.join();

    return 0;
}

