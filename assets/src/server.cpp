
#include "server.h"
#include <unordered_map>
#include "util/singleton.hpp"
#include "util/logger.h"
#include "util/sys_cmd.h"
#include "vision/vision_event_loop.h"
#include "control/control_event_loop.h"
#include "data_stream.h"
#include "command.h"

using namespace util;
using namespace net;

namespace evt
{

const int kHeaderLen = sizeof(uint32_t);
const int kMinMessageLen = 2;
const int kMaxMessageLen = 64 * 1024 * 1024;

std::unordered_map<uint32_t, std::weak_ptr<TcpConnection>> userMap;

Server::Server(net::EventLoop* loop,
               const net::InetAddress& listenAddr)
    : server_(loop, listenAddr),
      broadcastService_(loop)
{
    server_.setConnectionCallback([this](const net::TcpConnectionPtr & conn)
    {
        LOG(INFO) << "Server - " << conn->peerAddr().toIpPort() << " -> "
                  << conn->localAddr().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN");
        if (conn->connected())
        {
            broadcastService_.addConnection(conn);
        }
        else
        {
            broadcastService_.removeConnection(conn);
        }
    });
    server_.setMessageCallback([this](const net::TcpConnectionPtr & conn,
                                      net::Buffer * buf,
                                      net::Timestamp)
    {
        while (buf->readableBytes() >= kHeaderLen + kMinMessageLen)
        {
            uint32_t len = buf->peekUInt32();
            if (len > kMaxMessageLen || len < kMinMessageLen)
            {
                LOG(ERROR) << "message length invalid";
                break;
            }
            else if (buf->readableBytes() >= kHeaderLen + len)
            {
                parseMessage(conn, buf->peek() + kHeaderLen, len);
                buf->retrieve(kHeaderLen + len);
            }
            else
            {
                break;
            }
        }

    });
}


inline Buffer createBuffer(Command cmd, int initialSize = 0)
{
    Buffer buf(kHeaderLen + kMinMessageLen + initialSize, kHeaderLen);
    buf.appendUInt16(static_cast<uint16_t>(cmd));
    return buf;
}

inline void packBuffer(Buffer* buf)
{
    buf->prependUInt32(buf->readableBytes());
}


void Server::parseMessage(const net::TcpConnectionPtr& conn, const char* data, int len)
{
    static auto& vision = Singleton<VisionEventLoop>::instance();
    static auto& control = Singleton<ControlEventLoop>::instance();
    static auto& motionControl = control.motionControl();
    static auto& fieldControl = control.fieldControl();

    DataStreamReader in(data, data + len);
    Command cmd = static_cast<Command>(in.readFixed16<uint16_t>());
    switch (cmd)
    {
        case Command::INIT:
            control.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd, 512);

                DataStreamWriter out(buf.beginWrite());

                out.writeFixed8(vision.trackEnabled());
                out.writeFixed8(vision.multiScaleEnabled());

                out.writeFixed8(motionControl.isAutoMode());
                out.writeFixed8(fieldControl.isAutoMode());

                out.writeFixed32(motionControl.leftCtlVal());
                out.writeFixed32(motionControl.rightCtlVal());

                out.writeFixed32(fieldControl.angleHDefault());
                out.writeFixed32(fieldControl.angleVDefault());

                out.writeFixed32(fieldControl.angleH());
                out.writeFixed32(fieldControl.angleV());

                // PID params
                auto pdParams = fieldControl.angleHPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                pdParams = fieldControl.angleVPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                pdParams = motionControl.directionPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                auto piParams = motionControl.distancePIParams();
                out.writeFixed32(piParams.first);
                out.writeFixed32(piParams.second);

                buf.hasWritten(out.size());
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::UPDATE_PID:
            control.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd, 128);

                DataStreamWriter out(buf.beginWrite());
                auto pdParams = fieldControl.angleHPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                pdParams = fieldControl.angleVPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                pdParams = motionControl.directionPDParams();
                out.writeFixed32(pdParams.first);
                out.writeFixed32(pdParams.second);

                auto piParams = motionControl.distancePIParams();
                out.writeFixed32(piParams.first);
                out.writeFixed32(piParams.second);

                buf.hasWritten(out.size());
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::RESET:
        {
            uint32_t type = in.readFixed32<uint32_t>();
            if (type & 0x1)
            {
                vision.enableTrack(false);
                control.queueInLoop([]()
                {
                    motionControl.enableAutoMode(false);
                    fieldControl.enableAutoMode(false);
                });
            }
            if (type & 0x2)
            {
                control.queueInLoop([]()
                {
                    fieldControl.resetAngleH();
                    fieldControl.resetAngleV();
                    fieldControl.enableAutoMode(false);
                });
            }
            if (type & 0x4)
            {
                control.queueInLoop([]()
                {
                    motionControl.stopMotor();
                    motionControl.enableAutoMode(false);
                });
            }
            Buffer buf = createBuffer(cmd);
            buf.appendUInt32(type);
            packBuffer(&buf);
            broadcastService_.broadcast(std::move(buf));
            break;
        }
        case Command::MESSAGE:
        {
            control.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd, 128);

                DataStreamWriter out(buf.beginWrite());
                out.writeFixed32(fieldControl.angleH());
                out.writeFixed32(fieldControl.angleV());
                out.writeFixed32(motionControl.leftCtlVal());
                out.writeFixed32(motionControl.rightCtlVal());

                buf.hasWritten(out.size());
                packBuffer(&buf);
                conn->send(std::move(buf));
            });

            vision.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd, 128);

                DataStreamWriter out(buf.beginWrite());
                Rect result = vision.trackResult();
                out.writeFixed32(result.xpos);
                out.writeFixed32(result.ypos);
                out.writeFixed32(result.width);
                out.writeFixed32(result.height);

                buf.hasWritten(out.size());
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        }
        case Command::IMAGE:
            vision.queueInLoop([this]()
            {
                vision.setSendImageCallback([this](const std::vector<uint8_t>& image)
                {
                    Buffer buf = createBuffer(Command::IMAGE, image.size());
                    buf.append(image.data(), image.size());
                    packBuffer(&buf);
                    broadcastService_.broadcast(std::move(buf));
                });
            });
            break;
        case Command::START_TRACK:
            vision.queueInLoop([]()
            {
                vision.enableTrack(true);
            });
            break;
        case Command::STOP_TRACK:
            vision.queueInLoop([]()
            {
                vision.enableTrack(false);
            });
            break;
        case Command::SET_TARGET:
        {
            Rect locate;
            in.readFixed32(&locate.xpos);
            in.readFixed32(&locate.ypos);
            in.readFixed32(&locate.width);
            in.readFixed32(&locate.height);

            vision.queueInLoop([conn, cmd, locate]()
            {
                vision.resetTarget(locate);

                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        }
        case Command::ENABLE_MULTI_SCALE:
        {
            vision.queueInLoop([this, conn, cmd]()
            {
                vision.enableMultiScale(true);
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        }
        case Command::DISABLE_MULTI_SCALE:
        {
            vision.queueInLoop([this, conn, cmd]()
            {
                vision.enableMultiScale(false);
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        }
        case Command::ENABLE_MOTION_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                motionControl.enableAutoMode(true);
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        case Command::DISABLE_MOTION_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                motionControl.enableAutoMode(false);
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        case Command::STOP_MOTOR:
            control.queueInLoop([conn, cmd]()
            {
                motionControl.stopMotor();
                motionControl.enableAutoMode(false);

                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::START_MOTOR:
            control.queueInLoop([conn, cmd]()
            {
                motionControl.startMotor();

                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::SET_MOTOR_VAL:
        {
            int leftCtlVal, rightCtlVal;
            in.readFixed32(&leftCtlVal);
            in.readFixed32(&rightCtlVal);
            control.queueInLoop([conn, cmd, leftCtlVal, rightCtlVal]()
            {
                motionControl.setControlValue(leftCtlVal, rightCtlVal);

                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        }
        case Command::GET_MOTOR_VAL:
            control.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd);
                buf.appendUInt32(motionControl.leftCtlVal());
                buf.appendUInt32(motionControl.rightCtlVal());
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::ENABLE_FIELD_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                fieldControl.enableAutoMode(true);
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        case Command::DISABLE_FIELD_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                fieldControl.enableAutoMode(false);

                Buffer buf = createBuffer(cmd, 128);

                DataStreamWriter out(buf.beginWrite());
                out.writeFixed32(fieldControl.angleH());
                out.writeFixed32(fieldControl.angleV());
                buf.hasWritten(out.size());

                packBuffer(&buf);
                conn->send(std::move(buf));

                buf = createBuffer(cmd);

                out = DataStreamWriter(buf.beginWrite());
                out.writeFixed32(fieldControl.angleH());
                out.writeFixed32(fieldControl.angleV());
                buf.hasWritten(out.size());

                packBuffer(&buf);
                broadcastService_.broadcastExcept(std::move(buf), conn);
            });
            break;
        case Command::SET_SERVO_VAL:
        {
            float angleH, angleV;
            in.readFixed32(&angleH);
            in.readFixed32(&angleV);
            control.queueInLoop([conn, cmd, angleH, angleV]()
            {
                fieldControl.setAngleH(angleH);
                fieldControl.setAngleV(angleV);

                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        }
        case Command::GET_SERVO_VAL:
            control.queueInLoop([conn, cmd]()
            {
                Buffer buf = createBuffer(cmd, 128);

                DataStreamWriter out(buf.beginWrite());
                out.writeFixed32(fieldControl.angleH());
                out.writeFixed32(fieldControl.angleV());
                buf.hasWritten(out.size());

                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        case Command::SET_MOTION_CTL_PID:
        {
            int index = in.readFixed32<int>();
            float param = in.readFixed32<float>();

            control.queueInLoop([this, conn, cmd, index, param]()
            {
                switch (index)
                {
                    case 0:
                        motionControl.setDirectionKp(param);
                        break;
                    case 1:
                        motionControl.setDirectionKd(param);
                        break;
                    case 2:
                        motionControl.setDistanceKp(param);
                        break;
                    case 3:
                        motionControl.setDistanceKi(param);
                        break;
                    default:
                        break;
                }
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcast(std::move(buf));
            });
            break;
        }
        case Command::SET_FIELD_CTL_PID:
        {
            int index = in.readFixed32<int>();
            float param = in.readFixed32<float>();

            control.queueInLoop([this, conn, cmd, index, param]()
            {
                switch (index)
                {
                    case 0:
                        fieldControl.setAngleHKp(param);
                        break;
                    case 1:
                        fieldControl.setAngleHKd(param);
                        break;
                    case 2:
                        fieldControl.setAngleVKp(param);
                        break;
                    case 3:
                        fieldControl.setAngleVKd(param);
                        break;
                    default:
                        break;
                }
                Buffer buf = createBuffer(cmd);
                packBuffer(&buf);
                broadcastService_.broadcast(std::move(buf));
            });
            break;
        }
        case Command::SEND_TO_CLIENT:
        {
            uint32_t userId = in.readFixed32<uint32_t>();
            std::string str = in.readString();
            auto it = userMap.find(userId);
            if (it != userMap.end())
            {
                Buffer buf = createBuffer(Command::NOTICE);
                buf.append(str.data(), str.size());
                packBuffer(&buf);
                TcpConnectionPtr targetConn = it->second.lock();
                if (targetConn) targetConn->send(std::move(buf));
            }
            break;
        }
        case Command::SEND_TO_ALL_CLIENTS:
        {
            std::string str = in.readString();

            Buffer buf = createBuffer(Command::SEND_TO_ALL_CLIENTS);
            packBuffer(&buf);
            conn->send(std::move(buf));

            buf = createBuffer(Command::NOTICE);
            buf.append(str.data(), str.size());
            packBuffer(&buf);
            broadcastService_.broadcastExcept(std::move(buf), conn);
            break;
        }
        case Command::SYSTEM_CMD:
        {
            std::string sysCmd = in.readString();
            std::string res = util::system(sysCmd);

            Buffer buf = createBuffer(cmd);
            buf.append(res.data(), res.size());
            packBuffer(&buf);
            conn->send(std::move(buf));
            break;
        }
        case Command::HALT:
            // util::system("halt");
            break;
        case Command::REBOOT:
            // util::system("reboot");
            break;
        default:
            break;
    }
}

} // namespace evt
