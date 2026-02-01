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

# Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets Charts)

# Poco
find_package(Poco REQUIRED COMPONENTS Foundation Util Net)

add_subdirectory(common)
add_subdirectory(apps/controller_app)
add_subdirectory(apps/algo_worker)

install(DIRECTORY config DESTINATION .)
add_subdirectory(apps/stress_test)
