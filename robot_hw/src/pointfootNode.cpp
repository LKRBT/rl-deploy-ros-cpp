// Copyright information
//
// © [2024] LimX Dynamics Technology Co., Ltd. All rights reserved.

#include "ros/ros.h"
#include "robot_hw/RobotHWLoop.h"
#include "robot_hw/PointfootHW.h"
#include "limxsdk/pointfoot.h"

// Main function
int main(int argc, char **argv) {
  ros::init(argc, argv, "pointfoot_hw");

  std::string robot_ip = "127.0.0.1"; // Default robot IP address
  if (argc > 1) {
    robot_ip = argv[1]; // Use command-line argument as robot IP address if provided
  }

  // Initialize PointFoot instance with robot IP
  if (!limxsdk::PointFoot::getInstance()->init(robot_ip)) {
    ROS_ERROR("Failed to connect to the robot: %s", robot_ip.c_str());
    abort();
  } else {
    ROS_WARN("Connect to the robot: %s", robot_ip.c_str());
  }

  try {
    // PointfootHW는 ROS Client 역할로 Service 호출 가능
    // NodeHandle을 갖고 있음 (robot_hw_nh)
    ros::NodeHandle nh;
    ros::NodeHandle robot_hw_nh("~");

    // Create and initialize PointfootHW instance
    std::shared_ptr<hw::PointfootHW> hw = std::make_shared<hw::PointfootHW>();
    // ros::spin()으로 인해 조이스틱 콜백은 언제든지 비동기적으로 지속 수신됩니다.
    hw->init(nh, robot_hw_nh);

    // Create and initialize RobotHWLoop instance
    hw::RobotHWLoop loop(nh, robot_hw_nh, hw);
    loop.StartControlLoop(robot_hw_nh);

    // Check if Gazebo should be used
    bool use_gazebo;
    if (!nh.getParam("use_gazebo", use_gazebo)) {
        ROS_WARN("Failed to get param 'use_gazebo', defaulting to false");
        use_gazebo = false;
    }

    // If Gazebo is being used, start the Biped controller in a detached thread
    // → 실행 시점에 컨트롤러가 비동기적으로 실행될 수 있음 → 이로 인해 서비스가 아직 advertise 되지 않았을 가능성 존재
    // → 이를 방지하기 위해 controller 실행 후 service client는 retry or wait 구조가 필요
    if (use_gazebo) {
      std::thread controller_thread([hw]() {
        std::this_thread::sleep_for(std::chrono::nanoseconds(static_cast<int64_t>(3.0 * 1e9)));
        hw->startBipedController();
      });
      controller_thread.detach();
    }

    // Start ROS event loop
    ros::spin();
  } catch (const std::exception &e) {
    ROS_ERROR("Error in the hardware interface: %s", e.what());
    return 1;
  }

  return 0;
}
