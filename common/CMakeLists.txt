add_library(common)

target_sources(common
  PRIVATE
    src/common/log/Log.cpp
    src/common/config/Config.cpp
    src/common/sensor/SensorSimulator.cpp
    src/common/sensor/SensorPipeline.cpp
    src/common/ipc/IpcClient.cpp
    src/common/ipc/IpcServer.cpp
    src/common/heartbeat/HeartbeatMonitor.cpp
    src/common/algo/AlgoProcessManager.cpp
    src/common/controller/ControllerRuntime.cpp
    src/common/fault/FaultInjector.cpp
    src/common/control/ActuatorSimulator.cpp
    src/common/control/ControlLoop.cpp
)

target_include_directories(common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
  PUBLIC
    Poco::Foundation
    Poco::Util
    Poco::Net
)

target_compile_features(common PUBLIC cxx_std_17)

install(TARGETS common
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
