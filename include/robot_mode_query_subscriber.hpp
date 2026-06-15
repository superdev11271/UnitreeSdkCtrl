#ifndef ROBOT_MODE_QUERY_SUBSCRIBER_HPP
#define ROBOT_MODE_QUERY_SUBSCRIBER_HPP

#include <functional>
#include <string>

#include <unitree/idl/ros2/Int32_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace unitree_sdk_ctrl
{

class RobotModeQuerySubscriber
{
public:
    using QueryHandler = std::function<void()>;

    explicit RobotModeQuerySubscriber(const std::string& topicName = "/robot_mode_query");
    ~RobotModeQuerySubscriber();

    RobotModeQuerySubscriber(const RobotModeQuerySubscriber&) = delete;
    RobotModeQuerySubscriber& operator=(const RobotModeQuerySubscriber&) = delete;

    void InitChannel(QueryHandler handler);
    void CloseChannel();

    const std::string& GetChannelName() const;

private:
    unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::Int32_> mSubscriber;
    bool m_channelInitialized;
};

}  // namespace unitree_sdk_ctrl

#endif  // ROBOT_MODE_QUERY_SUBSCRIBER_HPP
