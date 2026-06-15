# UnitreeSdkCtrl

Subscribe to ROS2 topics over Unitree SDK2 CycloneDDS channels and control a real Unitree B2 robot via `SportClient` and `MotionSwitcherClient`.

This node does **not** use `rclcpp`. It uses SDK2 `ChannelSubscriber` with ROS2-compatible IDL types (`geometry_msgs/msg/Twist`, `std_msgs/msg/Int32`), the same pattern as [SdkEventBridge](../SdkEventBridge).

## Architecture

```
ROS2 publisher (rclcpp)          UnitreeSdkCtrl (SDK2)           Real B2 robot
────────────────────────         ─────────────────────           ──────────────
/cmd_vel  ──► rt/cmd_vel  ──►  ChannelSubscriber<Twist>  ──►  SportClient::Move()
/cmd_ctl_sdk ─► rt/cmd_ctl_sdk ► ChannelSubscriber<Int32> ──►  Sport / MotionSwitcher APIs
/robot_mode_query ► rt/robot_mode_query ► CheckMode() ──► publish /robot_mode
```

ROS2 nodes can publish to these topics when both sides use CycloneDDS on the same domain (`ROS_DOMAIN_ID=0`).

## Features

- Subscribe `/cmd_vel` and forward velocity to `SportClient::Move(vx, vy, vyaw)`
- Subscribe `/cmd_ctl_sdk` and dispatch integer commands to sport / motion-switcher APIs
- Auto-stop on zero velocity or cmd_vel timeout (default 500 ms)
- Mode change with SDK state check: `CheckMode()` → skip if unchanged → `StandDown` → `SelectMode` → `BalanceStand`
- Speed level change with dedup; ignored in sport mode (fixed 6.0 m/s)
- Query current motion mode via `MotionSwitcherClient::CheckMode()` on `/robot_mode_query`, reply on `/robot_mode`

## Requirements

- C++17 compiler
- [Unitree SDK2](https://github.com/unitreerobotics/unitree_sdk2) installed (`unitree_sdk2`, CycloneDDS)
- Unitree B2 robot reachable on the chosen network interface (e.g. `eth0`)
- ROS2 Humble (optional, for testing publishers) with `rmw_cyclonedds_cpp`

## Build

```bash
cd UnitreeSdkCtrl
mkdir -p build && cd build
cmake ..
cmake --build .
```

## Run

```bash
./build/unitree_sdk_ctrl <networkInterface> [--timeout-ms N]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `networkInterface` | — | DDS network interface (`eth0`, `lo`, …) |
| `--timeout-ms N` | `500` | Stop move if no `/cmd_vel` for N ms |

Example:

```bash
./build/unitree_sdk_ctrl eth0
./build/unitree_sdk_ctrl eth0 --timeout-ms 1500
```

## ROS2 environment

Publishers must use the same DDS stack and domain as the robot:

```bash
source /opt/ros/humble/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=0
```

Without `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`, ROS2 may use FastDDS and will not see SDK2 topics.

## Subscribed topics

| ROS2 topic | DDS topic | Message | Action |
|------------|-----------|---------|--------|
| `/cmd_vel` | `rt/cmd_vel` | `geometry_msgs/msg/Twist` | `Move(linear.x, linear.y, angular.z)` |
| `/cmd_ctl_sdk` | `rt/cmd_ctl_sdk` | `std_msgs/msg/Int32` | Robot state / mode commands (see below) |
| `/robot_mode_query` | `rt/robot_mode_query` | `std_msgs/msg/Int32` | Trigger mode query (any value) |

## Published topics

| ROS2 topic | DDS topic | Message | Description |
|------------|-----------|---------|-------------|
| `/robot_mode` | `rt/robot_mode` | `std_msgs/msg/Int32` | Current motion mode code (see below) |

### `/robot_mode` state codes

Queried via SDK `MotionSwitcherClient::CheckMode(form, name)`:

| `CheckMode` output | Meaning |
|--------------------|---------|
| `form == "0"` | Standard form |
| `form == "1"` | Wheel mode |
| `name == "ai"` | AI motion mode |
| `name == "normal"` / `"normal-w"` | Sport motion mode |
| `name` empty | No active mode selected |

Published `/robot_mode` codes (from `name`):

| Code | Mode |
|------|------|
| `1005` | AI (`name == "ai"`) |
| `1006` | Sport (`name == "normal"` or `"normal-w"`) |
| `0` | No active mode / unknown `name` |
| `< 0` | `CheckMode` SDK error (returned as-is) |

**Query current mode:**

```bash
# Terminal 1: unitree_sdk_ctrl running

# Terminal 2
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=0
ros2 topic echo /robot_mode std_msgs/msg/Int32 &
ros2 topic pub /robot_mode_query std_msgs/msg/Int32 "{data: 0}" --once
```

Example response: `data: 1005` (AI mode).

### `/cmd_vel` mapping

| Twist field | SportClient |
|-------------|-------------|
| `linear.x` | `vx` (m/s) |
| `linear.y` | `vy` (m/s) |
| `angular.z` | `vyaw` (rad/s) |

- Zero velocity → `StopMove()`
- No message within timeout → `StopMove()` (watchdog)
- Joint lock is enforced by the robot firmware (`SwitchGait(0)`); this node always calls `Move()` and the robot rejects when locked

Publish at **≥ 20 Hz** (or increase `--timeout-ms`) so messages arrive faster than the 500 ms watchdog:

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}" -r 20
```

## `/cmd_ctl_sdk` commands

| Code | Name | SDK action |
|------|------|------------|
| `1000` | Damp | `StopMove()` + `Damp()` |
| `1001` | Balance stand | `BalanceStand()` |
| `1002` | Stand down | `StopMove()` + `StandDown()` |
| `1003` | Lock joints | `StopMove()` + `SwitchGait(0)` |
| `1004` | Unlock joints | `SwitchGait(1)` |
| `1005` | AI mode | Mode change to `ai` (see below) |
| `1006` | Sport mode | Mode change to `normal` (see below) |
| `1007` | Speed fast | `SpeedLevel(1)` — AI mode only |
| `1008` | Speed slow | `SpeedLevel(-1)` — AI mode only |

Example:

```bash
ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 "{data: 1001}" --once
```

### Typical startup (AI mode + move)

```bash
# Terminal 1
./build/unitree_sdk_ctrl eth0

# Terminal 2
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=0

ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 "{data: 1002}" --once   # stand down
ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 "{data: 1005}" --once   # ai mode
ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 "{data: 1001}" --once   # balance stand
ros2 topic pub /cmd_ctl_sdk std_msgs/msg/Int32 "{data: 1004}" --once   # unlock gait
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.3, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}" -r 20
```

### Mode change (1005 / 1006)

1. `CheckMode()` — if already in target mode, ignore
2. `StopMove()` → `StandDown()` → wait 3 s
3. `SelectMode("ai")` or `SelectMode("normal")`
4. `BalanceStand()`

Sport mode names from SDK: `ai`, `normal`, `normal-w`.

### Speed level (1007 / 1008)

| Level | Command | Max linear speed (AI mode) |
|-------|---------|----------------------------|
| `-1` | 1008 | ~1.5 m/s (slow) |
| `1` | 1007 | ~3.5 m/s (fast) |

- Skipped if already at the requested level (tracked from last successful SDK call)
- **Ignored in sport mode** — sport profile uses a fixed 6.0 m/s cap

## Error codes

| Code | Meaning |
|------|---------|
| `3104` | Sport API timeout — robot unreachable or not in locomotion-ready state |
| `7002` | Motion switcher busy — stand down required before mode change |

## Project layout

```
UnitreeSdkCtrl/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── channel_factory_init.hpp   # ChannelFactory init helper
│   ├── ros_dds_topic.hpp          # /foo → rt/foo mapping
│   ├── cmd_vel_subscriber.hpp
│   ├── cmd_ctl_sdk_subscriber.hpp
│   ├── robot_mode_publisher.hpp
│   ├── robot_mode_query_subscriber.hpp
│   ├── robot_controller.hpp       # cmd_ctl_sdk → SDK dispatch
│   └── unitree/idl/ros2/Int32_.hpp
└── src/
    ├── main.cpp
    ├── cmd_vel_subscriber.cpp
    ├── cmd_ctl_sdk_subscriber.cpp
    ├── robot_mode_publisher.cpp
    ├── robot_mode_query_subscriber.cpp
    ├── robot_controller.cpp
    └── int32_.cpp                 # CycloneDDS type registration for Int32
```

## Related projects

| Project | Role |
|---------|------|
| [SdkEventBridge](../SdkEventBridge) | Inverse bridge: sport client → `/cmd_vel_origin` (simulation) |
| [UnitreeSdkTest](../UnitreeSdkTest) | Minimal sport + motion-switcher init example |
| [unitree_sdk2](../unitree_sdk2) | Upstream Unitree SDK2 |

## See also

- SdkEventBridge README — DDS topic mapping, joint lock / `SwitchGait`, speed level behavior
- `UnitreeSdkTest/motion_switcher_test.cpp` — stand down → select mode → balance stand sequence
