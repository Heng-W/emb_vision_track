
#include "session.h"

#include "util/singleton.hpp"
#include "util/logger.h"
#include "util/sys_cmd.h"
#include "net/event_loop.h"
#include "vision/vision_event_loop.h"
#include "control/control_event_loop.h"
#include "data_stream.h"
#include "command.h"
#include "server.h"
#include "account.h"

using namespace util;
using namespace net;

namespace evt
{

const int kHeaderLen = sizeof(uint32_t);
const int kMinMessageLen = 2;
const int kMaxMessageLen = 64 * 1024 * 1024;


Session::Session(Server* server, const TcpConnectionPtr& conn)
    : server_(server),
      conn_(conn),
      broadcastService_(server->broadcastService()),
      login_(false),
      userId_(65535),
      type_(2)
{
    conn->setMessageCallback([this](const net::TcpConnectionPtr & conn,
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


Session::~Session() = default;

Buffer Session::createBuffer(Command cmd, int initialSize)
{
    Buffer buf(kHeaderLen + kMinMessageLen + initialSize, kHeaderLen);
    buf.appendUInt16(static_cast<uint16_t>(cmd));
    return buf;
}

void Session::packBuffer(Buffer* buf)
{
    buf->prependUInt32(buf->readableBytes());
}

void Session::logout()
{
    if (login_)
    {
        login_ = false;
        server_->removeUser(userId_);
        Buffer buf = createBuffer(Command::LOGOUT);
        packBuffer(&buf);
        conn_->send(std::move(buf));
        conn_->shutdown();
    }
}

void Session::parseMessage(const net::TcpConnectionPtr& conn, const char* data, int len)
{
    DataStreamReader in(data, data + len);
    Command cmd = static_cast<Command>(in.readFixed16<uint16_t>());

    if (!login_)
    {
        if (cmd != Command::LOGIN)
        {
            conn->shutdown();
            return;
        }
        std::string userName = in.readString();
        std::string pwd = in.readString();
        int errorCode = 0;
        if (userName != "guest")
        {
            errorCode = checkAccountByUserName(userName, pwd, &userId_, &type_);
        }

        Buffer buf = createBuffer(cmd);
        buf.appendUInt32(errorCode);
        if (errorCode == 0)
        {
            buf.appendUInt32(userId_);
            buf.appendUInt8(type_);
        }
        packBuffer(&buf);
        conn->send(std::move(buf));
        if (errorCode != 0)
        {
            conn->shutdown();
            return;
        }
        auto session = server_->findUser(userId_);
        if (session) // 已登录，发出下线通知
        {
            session->logout();
        }
        server_->addUser(userId_, shared_from_this());
        login_ = true;
        return;
    }

    static auto& vision = Singleton<VisionEventLoop>::instance();
    static auto& control = Singleton<ControlEventLoop>::instance();
    static auto& motionControl = control.motionControl();
    static auto& fieldControl = control.fieldControl();

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
            Buffer buf2 = buf;

            buf.appendUInt8(0);
            packBuffer(&buf);
            conn->send(std::move(buf));

            buf2.appendUInt8(1);
            packBuffer(&buf2);
            broadcastService_->broadcastExcept(std::move(buf2), conn->id());
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
            break;
        }
        case Command::IMAGE:
            break;
        case Command::START_TRACK:
        {
            vision.queueInLoop([]()
            {
                vision.enableTrack(true);
            });
            Buffer buf = createBuffer(cmd);
            Buffer buf2 = buf;

            buf.appendUInt8(0);
            packBuffer(&buf);
            conn->send(std::move(buf));

            buf2.appendUInt8(1);
            packBuffer(&buf2);
            broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            break;
        }
        case Command::STOP_TRACK:
        {
            vision.queueInLoop([]()
            {
                vision.enableTrack(false);
            });
            Buffer buf = createBuffer(cmd);
            Buffer buf2 = buf;

            buf.appendUInt8(0);
            packBuffer(&buf);
            conn->send(std::move(buf));

            buf2.appendUInt8(1);
            packBuffer(&buf2);
            broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            break;
        }
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
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            });
            break;
        }
        case Command::DISABLE_MULTI_SCALE:
        {
            vision.queueInLoop([this, conn, cmd]()
            {
                vision.enableMultiScale(false);

                Buffer buf = createBuffer(cmd);
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            });
            break;
        }
        case Command::ENABLE_MOTION_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                motionControl.enableAutoMode(true);

                Buffer buf = createBuffer(cmd);
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            });
            break;
        case Command::DISABLE_MOTION_AUTOCTL:
            control.queueInLoop([this, conn, cmd]()
            {
                motionControl.enableAutoMode(false);

                Buffer buf = createBuffer(cmd);
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
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
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
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
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
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
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
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
                Buffer buf2 = buf;

                buf.appendUInt8(0);
                packBuffer(&buf);
                conn->send(std::move(buf));

                buf2.appendUInt8(1);
                packBuffer(&buf2);
                broadcastService_->broadcastExcept(std::move(buf2), conn->id());
            });
            break;
        }
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
        case Command::SEND_TO_CLIENT:
        {
            uint32_t userId = in.readFixed32<uint32_t>();
            std::string str = in.readString();
            auto session = server_->findUser(userId);
            if (session)
            {
                Buffer buf = createBuffer(Command::NOTICE);
                buf.appendUInt32(str.size());
                buf.append(str.data(), str.size());
                packBuffer(&buf);
                session->connection()->send(std::move(buf));
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
            buf.appendUInt32(str.size());
            buf.append(str.data(), str.size());
            packBuffer(&buf);
            broadcastService_->broadcastExcept(std::move(buf), conn->id());
            break;
        }
        case Command::SYSTEM_CMD:
        {
            std::string sysCmd = in.readString();
            if (userId_ == 0)
            {
                std::string res = util::system(sysCmd);

                Buffer buf = createBuffer(cmd);
                buf.appendUInt32(res.size());
                buf.append(res.data(), res.size());
                packBuffer(&buf);
                conn->send(std::move(buf));
            }
            break;
        }
        case Command::UPDATE_CLIENT_CNT:
        {
            server_->getLoop()->runInLoop([this, conn, cmd]
            {
                int clientCnt = server_->connections().size();
                Buffer buf = createBuffer(cmd);
                buf.appendUInt32(clientCnt);
                packBuffer(&buf);
                conn->send(std::move(buf));
            });
            break;
        }
        case Command::HALT:
            if (userId_ == 0)
            {
                util::system("halt");
            }
            break;
        case Command::REBOOT:
            if (userId_ == 0)
            {
                util::system("reboot");
            }
            break;
        default:
            break;
    }
}

} // namespace evt
