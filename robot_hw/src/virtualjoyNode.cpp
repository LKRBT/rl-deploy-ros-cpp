// 수정(전체)된 항목: 버튼 18개 전체 매핑 + 축(좌/우 조이스틱) 반영 (Arrow Key/Numpad)
#include <ros/ros.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_map>
#include <set>
#include <sensor_msgs/Joy.h>

char getKeyNonBlocking() {
  struct termios oldt, newt;
  char c;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  c = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (c != EOF) return c;
  return 0;
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "virtual_joystick_node");
  ros::NodeHandle nh;

  ros::Publisher joy_pub = nh.advertise<sensor_msgs::Joy>("/virtual_joystick", 10);

  // 버튼 매핑 (joystick_buttons 기준)
  std::unordered_map<char, int> key_to_button = {
    {'k', 0},   // A (ok) 엑스
    {'l', 1},   // B (ok) 동그라미
    {'j', 2},   // X (ok) 네모
    {'i', 3},   // Y (ok) 세모
    {'q', 4},   // L1 (ok)
    {'o', 5},   // R2 (ok)
    {'e', 6},   // L2 (ok)
    {'u', 7},   // R1 (ok)
    {'z', 8},   // SELECT (ok)
    {'x', 9},   // START (ok)
    {'w', 12},  // D-Pad Up
    {'s', 13},  // D-Pad Down
    {'a', 14},  // D-Pad Left
    {'d', 15},  // D-Pad Right
    {'n', 16},  // MENU
    {'m', 17}   // BACK
  };

  // 축 매핑 키 (Arrow/Numpad)
  std::unordered_map<char, std::pair<int, float>> key_to_axis = {
    {'A', {0, -1.0}},  // ← Left
    {'D', {0, +1.0}},  // → Right
    {'W', {1, +1.0}},  // ↑ Up
    {'S', {1, -1.0}},  // ↓ Down

    {'4', {2, -1.0}},  // → 오른쪽 조이스틱 좌
    {'6', {2, +1.0}},  // → 오른쪽 조이스틱 우
    {'8', {3, +1.0}},  // → 오른쪽 조이스틱 위
    {'5', {3, -1.0}}   // → 오른쪽 조이스틱 아래
  };

  std::vector<int32_t> buttons(18, 0);
  std::vector<float> axes(4, 0.0f);

  std::set<char> pressed_buttons;
  std::set<char> pressed_axes;

  ROS_INFO("가상 조이스틱 노드 시작됨. 키 입력 대기 중...");
  ros::Rate rate(10);

  while (ros::ok()) {
    char c = getKeyNonBlocking();
    if (c != 0) {
      // 버튼 토글
      if (key_to_button.count(c)) {
        if (pressed_buttons.count(c)) pressed_buttons.erase(c);
        else pressed_buttons.insert(c);
      }

      // 축 반영 (즉시 반응, toggle 아님)
      if (key_to_axis.count(c)) {
        pressed_axes.insert(c);
      }

      // 버튼 상태 설정
      std::fill(buttons.begin(), buttons.end(), 0);
      for (const char& k : pressed_buttons) {
        buttons[key_to_button[k]] = 1;
      }

      // 축 상태 설정
      std::fill(axes.begin(), axes.end(), 0.0f);
      for (const char& k : pressed_axes) {
        auto axis = key_to_axis[k];
        axes[axis.first] = axis.second;
      }

      // Joy 메시지 전송
      sensor_msgs::Joy joy_msg;
      joy_msg.header.stamp = ros::Time::now();
      joy_msg.axes = axes;
      joy_msg.buttons = buttons;
      joy_pub.publish(joy_msg);

      // 로그 출력
      std::string b_str(pressed_buttons.begin(), pressed_buttons.end());
      std::string a_str(pressed_axes.begin(), pressed_axes.end());
      ROS_INFO_STREAM("눌린 버튼: [" << b_str << "], 눌린 축 키: [" << a_str << "]");
    }

    pressed_axes.clear();  // 축은 매 loop 마다 초기화 (즉시 반응 방식)

    ros::spinOnce();
    rate.sleep();
  }

  return 0;
}
