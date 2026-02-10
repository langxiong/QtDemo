cmake_minimum_required(VERSION 3.16)

project(medical_robot_control_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Force the correct multithreaded-DLL runtime library for all targets.
  # This ensures consistency with pre-built dependencies like Qt and Poco.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

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
#   <prefix>/bin   -> executables + runtime DLLs + configs
#   <prefix>/lib   -> libraries
install(FILES
  config/controller_app.ini
  config/algo_worker.ini
  config/stress_test.ini
  DESTINATION bin
)

# Windows: deploy Qt runtime + Poco runtime into the install bin folder
if(WIN32)
  set(MRCD_WINDEPLOYQT "" CACHE FILEPATH "Path to windeployqt.exe (Qt6).")
  if(NOT MRCD_WINDEPLOYQT)
    find_program(MRCD_WINDEPLOYQT NAMES windeployqt windeployqt.exe)
  endif()

  set(MRCD_POCO_PREFIX "" CACHE PATH "Poco install prefix (e.g. C:/Users/Zoom/Opt/Poco).")

  install(CODE [[
    # Pull values from the CMake cache (install script does not automatically see cache vars).
    set(MRCD_WINDEPLOYQT "$CACHE{MRCD_WINDEPLOYQT}")
    set(MRCD_POCO_PREFIX "$CACHE{MRCD_POCO_PREFIX}")

    # Qt deployment
    if(NOT MRCD_WINDEPLOYQT)
      message(WARNING "MRCD_WINDEPLOYQT not found; Qt DLLs/plugins will not be deployed. Set -DMRCD_WINDEPLOYQT=.../windeployqt.exe")
    else()
      message(STATUS "Running windeployqt: ${MRCD_WINDEPLOYQT}")
      execute_process(
        COMMAND "${MRCD_WINDEPLOYQT}" --no-translations --no-compiler-runtime "${CMAKE_INSTALL_PREFIX}/bin/controller_app.exe"
        RESULT_VARIABLE _windeployqt_rc
      )
      if(NOT _windeployqt_rc EQUAL 0)
        message(FATAL_ERROR "windeployqt failed with code ${_windeployqt_rc}")
      endif()
    endif()

    # Poco deployment from install prefix
    if(MRCD_POCO_PREFIX)
      set(_poco_bin "${MRCD_POCO_PREFIX}/bin")
      message(STATUS "Deploying Poco DLLs from: ${_poco_bin}")
      file(GLOB _poco_dlls "${_poco_bin}/Poco*.dll")
      if(_poco_dlls)
        file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE FILES ${_poco_dlls})
      else()
        message(WARNING "No Poco*.dll found in ${_poco_bin}")
      endif()
    else()
      message(STATUS "MRCD_POCO_PREFIX not set; skipping Poco DLL deployment")
    endif()
  ]])
endif()
