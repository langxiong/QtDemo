cmake_minimum_required(VERSION 3.16)

project(medical_robot_control_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MRCD_ENABLE_WARNINGS "Enable compiler warnings" ON)

if(MRCD_ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W4 /permissive-)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)
qt_standard_project_setup()

# Poco
find_package(Poco REQUIRED COMPONENTS Foundation Util Net)

add_subdirectory(common)
add_subdirectory(apps/controller_app)
add_subdirectory(apps/algo_worker)
add_subdirectory(apps/stress_test)

# Install layout:
#   <prefix>/bin   -> executables + runtime DLLs
#   <prefix>/lib   -> libraries
#   <prefix>/config -> runtime config
install(DIRECTORY config DESTINATION .)

# Windows: deploy Qt runtime + Poco runtime into the install bin folder
if(WIN32)
  set(MRCD_WINDEPLOYQT "" CACHE FILEPATH "Path to windeployqt.exe (Qt6).")
  if(NOT MRCD_WINDEPLOYQT)
    find_program(MRCD_WINDEPLOYQT NAMES windeployqt windeployqt.exe
      PATHS ${QT6_DIR}/../../bin
      ${QT6_DIR}/bin
      ${Qt6_DIR}/../../bin
      ${Qt6_DIR}/bin
      NO_DEFAULT_PATH
    )
  endif()

  set(MRCD_POCO_PREFIX "" CACHE PATH "Poco install prefix (e.g. C:/Users/Zoom/Opt/Poco).")

  # 将变量传递给 install 脚本
  set(WINDEPLOYQT_EXECUTABLE "${MRCD_WINDEPLOYQT}")
  set(POCO_INSTALL_PREFIX "${MRCD_POCO_PREFIX}")

  # 创建部署脚本
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy_win.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deploy_win.cmake
    @ONLY
  )

  install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/deploy_win.cmake)
endif()