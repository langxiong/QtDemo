#include "DDSNode.hpp"
#include "ControlCommand.hpp"
#include "ControlCommandPubSubTypes.hpp"
#include "JointState.hpp"
#include "JointStatePubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <stdexcept>

namespace dds_core {

DDSNode::DDSNode(int domain_id) : domain_id_(domain_id) {
  auto factory = eprosima::fastdds::dds::DomainParticipantFactory::get_instance();
  participant_ = factory->create_participant(
      static_cast<eprosima::fastdds::dds::DomainId_t>(domain_id),
      eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
  if (!participant_) {
    throw std::runtime_error("Failed to create DDS DomainParticipant");
  }

  participant_->register_type(eprosima::fastdds::dds::TypeSupport(new JointStatePubSubType()));
  participant_->register_type(eprosima::fastdds::dds::TypeSupport(new ControlCommandPubSubType()));
}

DDSNode::~DDSNode() {
  shutdown();
}

void DDSNode::shutdown() {
  if (shutdown_) return;
  shutdown_ = true;
  if (participant_) {
    participant_->delete_contained_entities();
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
    participant_ = nullptr;
  }
}

eprosima::fastdds::dds::Publisher* DDSNode::createPublisher() {
  if (!participant_) return nullptr;
  return participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT);
}

eprosima::fastdds::dds::DataWriter* DDSNode::createJointStateWriter(
    eprosima::fastdds::dds::Publisher* pub) {
  if (!pub || !participant_) return nullptr;
  auto* topic = participant_->create_topic(
      TopicRegistry::joint_state(),
      "JointState",
      eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
  if (!topic) return nullptr;
  auto qos = writerQosJointState();
  return pub->create_datawriter(topic, qos);
}

eprosima::fastdds::dds::DataWriter* DDSNode::createControlCommandWriter(
    eprosima::fastdds::dds::Publisher* pub) {
  if (!pub || !participant_) return nullptr;
  auto* topic = participant_->create_topic(
      TopicRegistry::control_command(), "ControlCommand",
      eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
  if (!topic) return nullptr;
  auto qos = writerQosControlCommand();
  return pub->create_datawriter(topic, qos);
}

eprosima::fastdds::dds::Subscriber* DDSNode::createSubscriber() {
  if (!participant_) return nullptr;
  return participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT);
}

eprosima::fastdds::dds::DataReader* DDSNode::createJointStateReader(
    eprosima::fastdds::dds::Subscriber* sub) {
  if (!sub || !participant_) return nullptr;
  auto* topic = participant_->create_topic(
      TopicRegistry::joint_state(), "JointState",
      eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
  if (!topic) return nullptr;
  auto qos = readerQosJointState();
  return sub->create_datareader(topic, qos);
}

eprosima::fastdds::dds::DataReader* DDSNode::createControlCommandReader(
    eprosima::fastdds::dds::Subscriber* sub) {
  if (!sub || !participant_) return nullptr;
  auto* topic = participant_->create_topic(
      TopicRegistry::control_command(), "ControlCommand",
      eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
  if (!topic) return nullptr;
  auto qos = readerQosControlCommand();
  return sub->create_datareader(topic, qos);
}

}  // namespace dds_core
