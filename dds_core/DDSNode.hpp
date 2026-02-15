#pragma once

#include "QoSProfiles.hpp"
#include "TopicRegistry.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <memory>
#include <string>

// Forward-declare generated types
struct JointState;
class JointStatePubSubType;
struct ControlCommand;
class ControlCommandPubSubType;

namespace dds_core {

class DDSNode {
public:
  explicit DDSNode(int domain_id);
  ~DDSNode();

  DDSNode(const DDSNode&) = delete;
  DDSNode& operator=(const DDSNode&) = delete;

  eprosima::fastdds::dds::DomainParticipant* participant() const {
    return participant_;
  }

  int domainId() const { return domain_id_; }

  /** Create a publisher for JointState. Caller owns the returned pointers. */
  eprosima::fastdds::dds::Publisher* createPublisher();
  eprosima::fastdds::dds::DataWriter* createJointStateWriter(
      eprosima::fastdds::dds::Publisher* pub);
  eprosima::fastdds::dds::DataWriter* createControlCommandWriter(
      eprosima::fastdds::dds::Publisher* pub);

  /** Create a subscriber for topics. Caller owns the returned pointers. */
  eprosima::fastdds::dds::Subscriber* createSubscriber();
  eprosima::fastdds::dds::DataReader* createJointStateReader(
      eprosima::fastdds::dds::Subscriber* sub);
  eprosima::fastdds::dds::DataReader* createControlCommandReader(
      eprosima::fastdds::dds::Subscriber* sub);

  /** Graceful shutdown: delete participant and contained entities */
  void shutdown();

private:
  int domain_id_;
  eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
  bool shutdown_{false};
};

}  // namespace dds_core
