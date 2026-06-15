#include "cmd_ctl_sdk_subscriber.hpp"

#include <iostream>

#include "ros_dds_topic.hpp"

namespace unitree_sdk_ctrl
{

CmdCtlSdkSubscriber::CmdCtlSdkSubscriber(const std::string& topicName)
    : mSubscriber(ToRosDdsTopicName(topicName)),
      m_channelInitialized(false)
{
    std::cout << "[cmd_ctl_sdk] subscribe DDS topic=" << mSubscriber.GetChannelName() << std::endl;
}

CmdCtlSdkSubscriber::~CmdCtlSdkSubscriber()
{
    CloseChannel();
}

void CmdCtlSdkSubscriber::InitChannel(CmdCtlHandler handler)
{
    if (m_channelInitialized || !handler)
    {
        return;
    }

    mSubscriber.InitChannel(
        [handler](const void* message) {
            const auto* cmd = static_cast<const std_msgs::msg::dds_::Int32_*>(message);
            handler(cmd->data());
        },
        10);
    m_channelInitialized = true;
}

void CmdCtlSdkSubscriber::CloseChannel()
{
    if (!m_channelInitialized)
    {
        return;
    }

    mSubscriber.CloseChannel();
    m_channelInitialized = false;
}

const std::string& CmdCtlSdkSubscriber::GetChannelName() const
{
    return mSubscriber.GetChannelName();
}

}  // namespace unitree_sdk_ctrl
