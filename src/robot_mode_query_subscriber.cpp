#include "robot_mode_query_subscriber.hpp"

#include <iostream>

#include "ros_dds_topic.hpp"

namespace unitree_sdk_ctrl
{

RobotModeQuerySubscriber::RobotModeQuerySubscriber(const std::string& topicName)
    : mSubscriber(ToRosDdsTopicName(topicName)),
      m_channelInitialized(false)
{
    std::cout << "[robot_mode] subscribe query DDS topic=" << mSubscriber.GetChannelName()
              << std::endl;
}

RobotModeQuerySubscriber::~RobotModeQuerySubscriber()
{
    CloseChannel();
}

void RobotModeQuerySubscriber::InitChannel(QueryHandler handler)
{
    if (m_channelInitialized || !handler)
    {
        return;
    }

    mSubscriber.InitChannel(
        [handler](const void* message) {
            const auto* request = static_cast<const std_msgs::msg::dds_::Int32_*>(message);
            std::cout << "[robot_mode] query rx data=" << request->data() << std::endl;
            handler();
        },
        10);
    m_channelInitialized = true;
}

void RobotModeQuerySubscriber::CloseChannel()
{
    if (!m_channelInitialized)
    {
        return;
    }

    mSubscriber.CloseChannel();
    m_channelInitialized = false;
}

const std::string& RobotModeQuerySubscriber::GetChannelName() const
{
    return mSubscriber.GetChannelName();
}

}  // namespace unitree_sdk_ctrl
