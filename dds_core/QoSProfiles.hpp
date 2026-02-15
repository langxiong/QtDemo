#pragma once

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

namespace dds_core {

/** JointState: BEST_EFFORT, KEEP_LAST(1), VOLATILE */
inline eprosima::fastdds::dds::DataWriterQos writerQosJointState() {
  eprosima::fastdds::dds::DataWriterQos qos;
  qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
  qos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
  qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
  qos.history().depth = 1;
  return qos;
}

inline eprosima::fastdds::dds::DataReaderQos readerQosJointState() {
  eprosima::fastdds::dds::DataReaderQos qos;
  qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
  qos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
  qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
  qos.history().depth = 1;
  return qos;
}

/** ControlCommand: RELIABLE, KEEP_LAST(1), deadline 2ms */
inline eprosima::fastdds::dds::DataWriterQos writerQosControlCommand() {
  eprosima::fastdds::dds::DataWriterQos qos;
  qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
  qos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
  qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
  qos.history().depth = 1;
  qos.deadline().period = {0, 2000000};  // 2 ms
  return qos;
}

inline eprosima::fastdds::dds::DataReaderQos readerQosControlCommand() {
  eprosima::fastdds::dds::DataReaderQos qos;
  qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
  qos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
  qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
  qos.history().depth = 1;
  qos.deadline().period = {0, 2000000};  // 2 ms
  return qos;
}

}  // namespace dds_core
