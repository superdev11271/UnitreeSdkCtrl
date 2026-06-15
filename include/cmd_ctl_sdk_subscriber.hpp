#ifndef CMD_CTL_SDK_SUBSCRIBER_HPP
#define CMD_CTL_SDK_SUBSCRIBER_HPP

#include <functional>
#include <string>

#include <unitree/idl/ros2/Int32_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace unitree_sdk_ctrl
{

class CmdCtlSdkSubscriber
{
public:
    using CmdCtlHandler = std::function<void(int32_t command)>;

    explicit CmdCtlSdkSubscriber(const std::string& topicName = "/cmd_ctl_sdk");
    ~CmdCtlSdkSubscriber();

    CmdCtlSdkSubscriber(const CmdCtlSdkSubscriber&) = delete;
    CmdCtlSdkSubscriber& operator=(const CmdCtlSdkSubscriber&) = delete;

    void InitChannel(CmdCtlHandler handler);
    void CloseChannel();

    const std::string& GetChannelName() const;

private:
    unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::Int32_> mSubscriber;
    bool m_channelInitialized;
};

}  // namespace unitree_sdk_ctrl

#endif  // CMD_CTL_SDK_SUBSCRIBER_HPP
