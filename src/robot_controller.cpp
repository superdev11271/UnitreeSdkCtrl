#include "robot_controller.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace unitree_sdk_ctrl
{

namespace
{

constexpr int32_t kSpeedLevelSlow = -1;
constexpr int32_t kSpeedLevelFast = 1;

}  // namespace

bool RobotController::IsAiModeName(const std::string& modeName)
{
    return modeName == "ai";
}

bool RobotController::IsSportModeName(const std::string& modeName)
{
    return modeName == "normal" || modeName == "normal-w";
}

void RobotController::Init()
{
    mSportClient.SetTimeout(10.0f);
    mSportClient.Init();

    mMotionSwitcherClient.SetTimeout(10.0f);
    mMotionSwitcherClient.Init();

    const int32_t moveModeRet = mSportClient.SwitchMoveMode(true);
    if (moveModeRet != 0)
    {
        std::cout << "[warn] SwitchMoveMode(true) failed, code=" << moveModeRet << std::endl;
    }
}

const char* RobotController::CmdToString(int32_t command)
{
    switch (static_cast<CmdCtlSdk>(command))
    {
    case CmdCtlSdk::Damp:
        return "Damp";
    case CmdCtlSdk::BalanceStand:
        return "BalanceStand";
    case CmdCtlSdk::StandDown:
        return "StandDown";
    case CmdCtlSdk::LockMotors:
        return "LockMotors";
    case CmdCtlSdk::UnlockMotors:
        return "UnlockMotors";
    case CmdCtlSdk::AiMode:
        return "AiMode";
    case CmdCtlSdk::SportMode:
        return "SportMode";
    case CmdCtlSdk::SpeedFast:
        return "SpeedFast";
    case CmdCtlSdk::SpeedNormal:
        return "SpeedNormal";
    default:
        return "Unknown";
    }
}

int32_t RobotController::HandleCmdCtl(int32_t command)
{
    std::lock_guard<std::mutex> lock(mMutex);

    std::cout << "[cmd_ctl_sdk] rx " << command << " (" << CmdToString(command) << ")" << std::endl;

    switch (static_cast<CmdCtlSdk>(command))
    {
    case CmdCtlSdk::Damp:
        return DampMotors();
    case CmdCtlSdk::BalanceStand:
        return BalanceStand();
    case CmdCtlSdk::StandDown:
        return StandDown();
    case CmdCtlSdk::LockMotors:
        return LockMotors();
    case CmdCtlSdk::UnlockMotors:
        return UnlockMotors();
    case CmdCtlSdk::AiMode:
        return SelectAiMode();
    case CmdCtlSdk::SportMode:
        return SelectSportMode();
    case CmdCtlSdk::SpeedFast:
        return SetSpeedFast();
    case CmdCtlSdk::SpeedNormal:
        return SetSpeedNormal();
    default:
        std::cout << "[cmd_ctl_sdk] unknown command " << command << std::endl;
        return -1;
    }
}

int32_t RobotController::HandleMove(float vx, float vy, float vyaw)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mSportClient.Move(vx, vy, vyaw);
}

int32_t RobotController::HandleStopMove()
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mSportClient.StopMove();
}

const char* RobotController::ModeCodeToString(int32_t modeCode)
{
    switch (modeCode)
    {
    case static_cast<int32_t>(CmdCtlSdk::AiMode):
        return "ai";
    case static_cast<int32_t>(CmdCtlSdk::SportMode):
        return "sport";
    case 0:
        return "none";
    default:
        if (modeCode < 0)
        {
            return "check_mode_error";
        }
        return "unknown";
    }
}

const char* RobotController::FormToString(const std::string& form)
{
    if (form == "0")
    {
        return "standard";
    }
    if (form == "1")
    {
        return "wheel";
    }
    return form.empty() ? "unknown" : form.c_str();
}

int32_t RobotController::ModeNameToCode(const std::string& name)
{
    if (IsAiModeName(name))
    {
        return static_cast<int32_t>(CmdCtlSdk::AiMode);
    }
    if (IsSportModeName(name))
    {
        return static_cast<int32_t>(CmdCtlSdk::SportMode);
    }
    return 0;
}

int32_t RobotController::CheckCurrentMode(std::string& form, std::string& name)
{
    return mMotionSwitcherClient.CheckMode(form, name);
}

int32_t RobotController::GetCurrentModeCode()
{
    std::lock_guard<std::mutex> lock(mMutex);

    std::string form;
    std::string name;
    const int32_t ret = CheckCurrentMode(form, name);
    if (ret != 0)
    {
        std::cout << "[robot_mode] CheckMode failed, code=" << ret << std::endl;
        return ret;
    }

    std::cout << "[robot_mode] CheckMode form=" << form << " (" << FormToString(form)
              << ") name=" << (name.empty() ? "(empty)" : name) << std::endl;

    return ModeNameToCode(name);
}

int32_t RobotController::BalanceStand()
{
    const int32_t ret = mSportClient.BalanceStand();
    if (ret == 0)
    {
        std::cout << "[cmd_ctl_sdk] BalanceStand ok" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] BalanceStand failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::DampMotors()
{
    mSportClient.StopMove();
    const int32_t ret = mSportClient.Damp();
    if (ret == 0)
    {
        std::cout << "[cmd_ctl_sdk] Damp ok" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] Damp failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::StandDown()
{
    mSportClient.StopMove();
    const int32_t ret = mSportClient.StandDown();
    if (ret == 0)
    {
        std::cout << "[cmd_ctl_sdk] StandDown ok" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] StandDown failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::LockMotors()
{
    mSportClient.StopMove();
    const int32_t ret = mSportClient.SwitchGait(0);
    if (ret == 0)
    {
        std::cout << "[cmd_ctl_sdk] SwitchGait(0) ok, joints locked" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] SwitchGait(0) failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::UnlockMotors()
{
    const int32_t ret = mSportClient.SwitchGait(1);
    if (ret == 0)
    {
        std::cout << "[cmd_ctl_sdk] SwitchGait(1) ok, joints unlocked" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] SwitchGait(1) failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::QueryCurrentMode(std::string& modeName)
{
    std::string form;
    const int32_t ret = CheckCurrentMode(form, modeName);
    if (ret != 0)
    {
        std::cout << "[cmd_ctl_sdk] CheckMode failed, code=" << ret << std::endl;
        return ret;
    }

    std::cout << "[cmd_ctl_sdk] CheckMode form=" << form << " (" << FormToString(form)
              << ") name=" << (modeName.empty() ? "(empty)" : modeName) << std::endl;
    return 0;
}

int32_t RobotController::ChangeMotionMode(const std::string& targetMode)
{
    std::string currentMode;
    const int32_t checkRet = QueryCurrentMode(currentMode);
    if (checkRet == 0)
    {
        const bool sameMode = (targetMode == "ai") ? IsAiModeName(currentMode)
                                                   : IsSportModeName(currentMode);
        if (sameMode)
        {
            std::cout << "[cmd_ctl_sdk] already in " << targetMode << " mode, ignoring" << std::endl;
            return 0;
        }
    }

    mSportClient.StopMove();

    const int32_t standDownRet = mSportClient.StandDown();
    if (standDownRet != 0)
    {
        std::cout << "[cmd_ctl_sdk] StandDown failed, code=" << standDownRet << std::endl;
        return standDownRet;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    const int32_t selectRet = mMotionSwitcherClient.SelectMode(targetMode);
    if (selectRet != 0)
    {
        std::cout << "[cmd_ctl_sdk] SelectMode(" << targetMode << ") failed, code=" << selectRet
                  << std::endl;
        return selectRet;
    }

    const int32_t balanceRet = mSportClient.BalanceStand();
    if (balanceRet != 0)
    {
        std::cout << "[cmd_ctl_sdk] BalanceStand failed, code=" << balanceRet << std::endl;
        return balanceRet;
    }

    mCurrentSpeedLevel.reset();
    std::cout << "[cmd_ctl_sdk] mode changed to " << targetMode << std::endl;
    return 0;
}

int32_t RobotController::SetSpeedLevelIfNeeded(int level)
{
    std::string currentMode;
    const int32_t checkRet = QueryCurrentMode(currentMode);
    if (checkRet == 0 && IsSportModeName(currentMode))
    {
        std::cout << "[cmd_ctl_sdk] SpeedLevel ignored in sport mode" << std::endl;
        return 0;
    }

    if (mCurrentSpeedLevel.has_value() && *mCurrentSpeedLevel == level)
    {
        std::cout << "[cmd_ctl_sdk] already at SpeedLevel(" << level << "), ignoring" << std::endl;
        return 0;
    }

    const int32_t ret = mSportClient.SpeedLevel(level);
    if (ret == 0)
    {
        mCurrentSpeedLevel = level;
        std::cout << "[cmd_ctl_sdk] SpeedLevel(" << level << ") ok" << std::endl;
    }
    else
    {
        std::cout << "[cmd_ctl_sdk] SpeedLevel(" << level << ") failed, code=" << ret << std::endl;
    }
    return ret;
}

int32_t RobotController::SelectAiMode()
{
    return ChangeMotionMode("ai");
}

int32_t RobotController::SelectSportMode()
{
    return ChangeMotionMode("normal");
}

int32_t RobotController::SetSpeedFast()
{
    return SetSpeedLevelIfNeeded(kSpeedLevelFast);
}

int32_t RobotController::SetSpeedNormal()
{
    return SetSpeedLevelIfNeeded(kSpeedLevelSlow);
}

}  // namespace unitree_sdk_ctrl
