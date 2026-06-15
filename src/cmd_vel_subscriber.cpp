#include "cmd_vel_subscriber.hpp"

#include <iostream>

#include "ros_dds_topic.hpp"

namespace unitree_sdk_ctrl
{

CmdVelSubscriber::CmdVelSubscriber(const std::string& topicName)
    : mSubscriber(ToRosDdsTopicName(topicName)),
      m_channelInitialized(false)
{
    std::cout << "[cmd_vel] subscribe DDS topic=" << mSubscriber.GetChannelName() << std::endl;
}

CmdVelSubscriber::~CmdVelSubscriber()
{
    CloseChannel();
}

void CmdVelSubscriber::InitChannel(CmdVelHandler handler)
{
    if (m_channelInitialized || !handler)
    {
        return;
    }

    mSubscriber.InitChannel(
        [handler](const void* message) {
            const auto* twist = static_cast<const geometry_msgs::msg::dds_::Twist_*>(message);
            handler(*twist);
        },
        10);
    m_channelInitialized = true;
}

void CmdVelSubscriber::CloseChannel()
{
    if (!m_channelInitialized)
    {
        return;
    }

    mSubscriber.CloseChannel();
    m_channelInitialized = false;
}

const std::string& CmdVelSubscriber::GetChannelName() const
{
    return mSubscriber.GetChannelName();
}

}  // namespace unitree_sdk_ctrl
