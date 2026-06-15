#ifndef CMD_VEL_SUBSCRIBER_HPP
#define CMD_VEL_SUBSCRIBER_HPP

#include <functional>
#include <string>

#include <unitree/idl/ros2/Twist_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace unitree_sdk_ctrl
{

class CmdVelSubscriber
{
public:
    using CmdVelHandler = std::function<void(const geometry_msgs::msg::dds_::Twist_& twist)>;

    explicit CmdVelSubscriber(const std::string& topicName = "/cmd_vel");
    ~CmdVelSubscriber();

    CmdVelSubscriber(const CmdVelSubscriber&) = delete;
    CmdVelSubscriber& operator=(const CmdVelSubscriber&) = delete;

    void InitChannel(CmdVelHandler handler);
    void CloseChannel();

    const std::string& GetChannelName() const;

private:
    unitree::robot::ChannelSubscriber<geometry_msgs::msg::dds_::Twist_> mSubscriber;
    bool m_channelInitialized;
};

}  // namespace unitree_sdk_ctrl

#endif  // CMD_VEL_SUBSCRIBER_HPP
