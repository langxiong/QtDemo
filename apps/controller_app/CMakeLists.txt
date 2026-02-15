set(controller_app_sources
  src/main.cpp
  src/ControllerApp.cpp
  src/ControllerApp.h
  src/QtSubsystem.cpp
  src/QtSubsystem.h
  src/DeviceSimulator.cpp
  src/DeviceSimulator.h
  src/DemoController.cpp
  src/DemoController.h
  src/MainWindow.cpp
  src/MainWindow.h
  src/ui/ControlPanel.cpp
  src/ui/ControlPanel.h
  src/ui/LogPanel.cpp
  src/ui/LogPanel.h
  src/ui/ReadoutStrip.cpp
  src/ui/ReadoutStrip.h
  src/ui/StatusStrip.cpp
  src/ui/StatusStrip.h
  src/ui/StyleHelper.h
  src/ui/TelemetryChart.cpp
  src/ui/TelemetryChart.h
)
list(APPEND controller_app_sources
  src/ControllerRuntimeDds.cpp
  src/ControllerRuntimeDds.h
)
add_executable(controller_app ${controller_app_sources})

target_link_libraries(controller_app
  PRIVATE
    common
    dds_core
    Poco::Util
    Qt6::Widgets
    Qt6::Charts
)

target_compile_features(controller_app PRIVATE cxx_std_17)

install(TARGETS controller_app
  RUNTIME DESTINATION bin
)
