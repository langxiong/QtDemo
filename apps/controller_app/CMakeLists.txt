set(controller_app_sources
  src/main.cpp
  src/ControllerApp.cpp
  src/ControllerApp.h
  src/DeviceSimulator.cpp
  src/DeviceSimulator.h
  src/IpcServer.cpp
  src/IpcServer.h
  src/ControllerRuntimeDds.cpp
  src/ControllerRuntimeDds.h
)
if(MRCD_DUILIB_ROOT AND WIN32)
  list(APPEND controller_app_sources
    ui/MainWindow.cpp
    ui/UISubsystem.cpp
  )
endif()
add_executable(controller_app ${controller_app_sources})
if(MRCD_DUILIB_ROOT AND WIN32)
  target_compile_definitions(controller_app PRIVATE CONTROLLER_HAVE_DUILIB=1)
endif()

target_link_libraries(controller_app
  PRIVATE
    common
    dds_core
    cef_api
    Poco::Util
    Poco::Net
    Poco::Foundation
)
if(MRCD_DUILIB_ROOT AND WIN32)
  if(NOT TARGET DuiLib)
    add_library(DuiLib SHARED IMPORTED GLOBAL)
    set_target_properties(DuiLib PROPERTIES
      IMPORTED_LOCATION "${MRCD_DUILIB_ROOT}/bin/DuiLib.dll"
      IMPORTED_IMPLIB "${MRCD_DUILIB_ROOT}/lib/DuiLib.lib"
    )
  endif()
  target_include_directories(controller_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    "${MRCD_DUILIB_ROOT}/include"
  )
  target_link_libraries(controller_app PRIVATE DuiLib)
  add_custom_command(TARGET controller_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/resources/mainwindow.xml"
      "$<TARGET_FILE_DIR:controller_app>"
    COMMENT "Copy mainwindow.xml to controller_app output"
  )
  if(EXISTS "${MRCD_DUILIB_ROOT}/bin/DuiLib.dll")
    add_custom_command(TARGET controller_app POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        "${MRCD_DUILIB_ROOT}/bin/DuiLib.dll"
        "$<TARGET_FILE_DIR:controller_app>"
      COMMENT "Copy DuiLib.dll to controller_app output"
    )
  endif()
  message(STATUS "controller_app: DuiLib GUI enabled (MRCD_DUILIB_ROOT=${MRCD_DUILIB_ROOT})")
endif()

target_compile_features(controller_app PRIVATE cxx_std_17)

install(TARGETS controller_app
  RUNTIME DESTINATION bin
)
