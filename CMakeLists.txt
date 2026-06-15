cmake_minimum_required(VERSION 3.10)
project(UnitreeSdkCtrl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(unitree_sdk2 REQUIRED)

add_executable(unitree_sdk_ctrl
    src/main.cpp
    src/cmd_vel_subscriber.cpp
    src/cmd_ctl_sdk_subscriber.cpp
    src/robot_controller.cpp
    src/int32_.cpp
)
target_include_directories(unitree_sdk_ctrl PRIVATE include)
target_link_libraries(unitree_sdk_ctrl PRIVATE unitree_sdk2)
