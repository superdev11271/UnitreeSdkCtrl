#include "robot_mode_publisher.hpp"

#include <iostream>

#include "ros_dds_topic.hpp"

namespace unitree_sdk_ctrl
{

RobotModePublisher::RobotModePublisher(const std::string& topicName)
    : mPublisher(ToRosDdsTopicName(topicName)),
      m_channelInitialized(false)
{
    std::cout << "[robot_mode] publish DDS topic=" << mPublisher.GetChannelName() << std::endl;
}

RobotModePublisher::~RobotModePublisher()
{
    CloseChannel();
}

void RobotModePublisher::InitChannel()
{
    if (m_channelInitialized)
    {
        return;
    }

    mPublisher.InitChannel();
    m_channelInitialized = true;
}

void RobotModePublisher::CloseChannel()
{
    if (!m_channelInitialized)
    {
        return;
    }

    mPublisher.CloseChannel();
    m_channelInitialized = false;
}

bool RobotModePublisher::Publish(int32_t modeCode)
{
    if (!m_channelInitialized)
    {
        InitChannel();
    }

    std_msgs::msg::dds_::Int32_ message;
    message.data() = modeCode;
    const bool wrote = mPublisher.Write(message);

    std::cout << "[robot_mode] published code=" << modeCode << std::endl;
    if (!wrote)
    {
        std::cerr << "[robot_mode] publish failed" << std::endl;
    }
    return wrote;
}

const std::string& RobotModePublisher::GetChannelName() const
{
    return mPublisher.GetChannelName();
}

}  // namespace unitree_sdk_ctrl
