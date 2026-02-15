add_executable(monitor_node
  src/main.cpp
)

target_link_libraries(monitor_node
  PRIVATE
    dds_core
)

target_compile_features(monitor_node PRIVATE cxx_std_17)

install(TARGETS monitor_node
  RUNTIME DESTINATION bin
)
