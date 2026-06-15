#ifndef ROBOT_CONTROLLER_HPP
#define ROBOT_CONTROLLER_HPP

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

#include <unitree/robot/b2/motion_switcher/motion_switcher_client.hpp>
#include <unitree/robot/b2/sport/sport_client.hpp>

namespace unitree_sdk_ctrl
{

enum class CmdCtlSdk : int32_t
{
    Damp = 1000,
    BalanceStand = 1001,
    StandDown = 1002,
    LockMotors = 1003,
    UnlockMotors = 1004,
    AiMode = 1005,
    SportMode = 1006,
    SpeedFast = 1007,
    SpeedNormal = 1008,
};

class RobotController
{
public:
    void Init();

    int32_t HandleCmdCtl(int32_t command);
    int32_t HandleMove(float vx, float vy, float vyaw);
    int32_t HandleStopMove();
    int32_t GetCurrentModeCode();

    static const char* CmdToString(int32_t command);
    static const char* ModeCodeToString(int32_t modeCode);
    static const char* FormToString(const std::string& form);

private:
    static bool IsAiModeName(const std::string& modeName);
    static bool IsSportModeName(const std::string& modeName);
    static int32_t ModeNameToCode(const std::string& name);

    // MotionSwitcherClient::CheckMode(form, name) — form: "0"=standard, "1"=wheel; name: "ai", "normal", …
    int32_t CheckCurrentMode(std::string& form, std::string& name);
    int32_t QueryCurrentMode(std::string& modeName);
    int32_t ChangeMotionMode(const std::string& targetMode);
    int32_t SetSpeedLevelIfNeeded(int level);

    int32_t DampMotors();
    int32_t BalanceStand();
    int32_t StandDown();
    int32_t LockMotors();
    int32_t UnlockMotors();
    int32_t SelectAiMode();
    int32_t SelectSportMode();
    int32_t SetSpeedFast();
    int32_t SetSpeedNormal();

    unitree::robot::b2::SportClient mSportClient;
    unitree::robot::b2::MotionSwitcherClient mMotionSwitcherClient;
    mutable std::mutex mMutex;
    std::optional<int32_t> mCurrentSpeedLevel;
};

}  // namespace unitree_sdk_ctrl

#endif  // ROBOT_CONTROLLER_HPP
