#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "channel_factory_init.hpp"
#include "cmd_ctl_sdk_subscriber.hpp"
#include "cmd_vel_subscriber.hpp"
#include "robot_controller.hpp"

namespace
{

std::atomic<bool> g_running{true};

void SignalHandler(int)
{
    g_running = false;
}

bool IsZeroVelocity(const geometry_msgs::msg::dds_::Twist_& twist)
{
    constexpr double kEpsilon = 1e-6;
    return std::abs(twist.linear().x()) < kEpsilon &&
           std::abs(twist.linear().y()) < kEpsilon &&
           std::abs(twist.angular().z()) < kEpsilon;
}

void PrintUsage(const char* program)
{
    std::cout << "Usage: " << program
              << " <networkInterface> [--timeout-ms N]" << std::endl;
    std::cout << "  timeout-ms defaults to 500 (matches twist_mux; use >=1000 for ros2 topic pub -r 1)"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Subscribed topics:" << std::endl;
    std::cout << "  /cmd_vel      -> SportClient::Move()" << std::endl;
    std::cout << "  /cmd_ctl_sdk  -> robot state/mode commands (std_msgs/Int32)" << std::endl;
    std::cout << std::endl;
    std::cout << "cmd_ctl_sdk commands:" << std::endl;
    std::cout << "  1000 Damp            1001 BalanceStand   1002 StandDown" << std::endl;
    std::cout << "  1003 Lock (SwitchGait 0)   1004 Unlock (SwitchGait 1)" << std::endl;
    std::cout << "  1005 AiMode          1006 SportMode      1007 SpeedFast" << std::endl;
    std::cout << "  1008 SpeedNormal (slow, AI mode)" << std::endl;
    std::cout << std::endl;
    std::cout << "ROS2 publisher must use the same DDS stack and domain:" << std::endl;
    std::cout << "  export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" << std::endl;
    std::cout << "  export ROS_DOMAIN_ID=0" << std::endl;
    std::cout << "  ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "
                 "'{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}' -r 20"
              << std::endl;
    std::cout << "  ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 '{data: 1001}'" << std::endl;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string networkInterface = argv[1];
    int timeoutMs = 500;

    for (int i = 2; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--timeout-ms" && i + 1 < argc)
        {
            timeoutMs = std::atoi(argv[++i]);
            continue;
        }
        if (arg.rfind("--", 0) == 0)
        {
            std::cout << "Unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }

    if (timeoutMs <= 0)
    {
        std::cout << "timeout-ms must be positive" << std::endl;
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    unitree_sdk_ctrl::InitUnitreeChannelFactoryOnce(0, networkInterface);

    unitree_sdk_ctrl::RobotController robotController;
    robotController.Init();

    std::mutex moveMutex;
    std::chrono::steady_clock::time_point lastCmdTime = std::chrono::steady_clock::now();
    bool hasActiveMove = false;
    bool receivedCmdVel = false;

    unitree_sdk_ctrl::CmdCtlSdkSubscriber cmdCtlSubscriber("/cmd_ctl_sdk");
    cmdCtlSubscriber.InitChannel([&robotController](int32_t command) {
        robotController.HandleCmdCtl(command);
    });

    unitree_sdk_ctrl::CmdVelSubscriber cmdVelSubscriber("/cmd_vel");
    cmdVelSubscriber.InitChannel([&](const geometry_msgs::msg::dds_::Twist_& twist) {
        const float vx = static_cast<float>(twist.linear().x());
        const float vy = static_cast<float>(twist.linear().y());
        const float vyaw = static_cast<float>(twist.angular().z());

        std::lock_guard<std::mutex> lock(moveMutex);
        lastCmdTime = std::chrono::steady_clock::now();
        receivedCmdVel = true;

        std::cout << "[cmd_vel] rx vx=" << vx << " vy=" << vy << " vyaw=" << vyaw << std::endl;

        if (IsZeroVelocity(twist))
        {
            if (hasActiveMove)
            {
                const int32_t ret = robotController.HandleStopMove();
                if (ret != 0)
                {
                    std::cout << "[cmd_vel] StopMove failed, code=" << ret << std::endl;
                }
                else
                {
                    std::cout << "[cmd_vel] StopMove ok" << std::endl;
                }
                hasActiveMove = false;
            }
            return;
        }

        const int32_t ret = robotController.HandleMove(vx, vy, vyaw);
        if (ret != 0)
        {
            std::cout << "[cmd_vel] Move failed, code=" << ret
                      << " (3104 = sport API timeout; ensure robot is on "
                      << networkInterface << " and in BalanceStand)" << std::endl;
            return;
        }

        hasActiveMove = true;
        std::cout << "[cmd_vel] Move ok" << std::endl;
    });

    std::cout << "[info] listening on " << cmdVelSubscriber.GetChannelName()
              << " and " << cmdCtlSubscriber.GetChannelName() << std::endl;
    std::cout << "[info] cmd_vel watchdog timeout=" << timeoutMs << " ms" << std::endl;

    const auto cmdTimeout = std::chrono::milliseconds(timeoutMs);
    constexpr auto kLoopInterval = std::chrono::milliseconds(50);

    while (g_running)
    {
        {
            std::lock_guard<std::mutex> lock(moveMutex);
            if (receivedCmdVel && hasActiveMove &&
                std::chrono::steady_clock::now() - lastCmdTime > cmdTimeout)
            {
                const int32_t ret = robotController.HandleStopMove();
                if (ret != 0)
                {
                    std::cout << "[watchdog] StopMove failed, code=" << ret << std::endl;
                }
                else
                {
                    std::cout << "[watchdog] no cmd_vel for " << timeoutMs
                              << " ms, stopped (increase --timeout-ms or pub rate with -r 20)"
                              << std::endl;
                }
                hasActiveMove = false;
            }
        }

        std::this_thread::sleep_for(kLoopInterval);
    }

    {
        std::lock_guard<std::mutex> lock(moveMutex);
        if (hasActiveMove)
        {
            robotController.HandleStopMove();
        }
    }

    cmdVelSubscriber.CloseChannel();
    cmdCtlSubscriber.CloseChannel();
    std::cout << "[info] shutdown complete" << std::endl;
    return 0;
}
