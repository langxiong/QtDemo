#pragma once

#include <string>

namespace dds_core {

struct TopicRegistry {
  static const char* joint_state() { return "JointState"; }
  static const char* control_command() { return "ControlCommand"; }
};

}  // namespace dds_core
