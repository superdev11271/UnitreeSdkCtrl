#ifndef ROBOT_MODE_PUBLISHER_HPP
#define ROBOT_MODE_PUBLISHER_HPP

#include <cstdint>
#include <string>

#include <unitree/idl/ros2/Int32_.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>

namespace unitree_sdk_ctrl
{

class RobotModePublisher
{
public:
    explicit RobotModePublisher(const std::string& topicName = "/robot_mode");
    ~RobotModePublisher();

    RobotModePublisher(const RobotModePublisher&) = delete;
    RobotModePublisher& operator=(const RobotModePublisher&) = delete;

    void InitChannel();
    void CloseChannel();
    bool Publish(int32_t modeCode);

    const std::string& GetChannelName() const;

private:
    unitree::robot::ChannelPublisher<std_msgs::msg::dds_::Int32_> mPublisher;
    bool m_channelInitialized;
};

}  // namespace unitree_sdk_ctrl

#endif  // ROBOT_MODE_PUBLISHER_HPP
