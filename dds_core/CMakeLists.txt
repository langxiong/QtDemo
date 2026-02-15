cmake_minimum_required(VERSION 3.16)

add_library(dds_core
  DDSNode.cpp
  DDSNode.hpp
  QoSProfiles.hpp
  TopicRegistry.hpp
)

target_include_directories(dds_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dds_core PUBLIC messages fastdds)
target_compile_features(dds_core PUBLIC cxx_std_17)

install(TARGETS dds_core ARCHIVE DESTINATION lib)
