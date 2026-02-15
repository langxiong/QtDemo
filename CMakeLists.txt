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

# Fast DDS (not vcpkg: use install path; see docs/FASTDDS_SETUP.md)
set(MRCD_FASTDDS_ROOT "" CACHE PATH "Fast DDS install prefix (e.g. C:/Opt/fastdds). Required.")
set(MRCD_FASTDDSGEN "" CACHE FILEPATH "Path to fastddsgen (or fastddsgen.bat). Set if not under MRCD_FASTDDS_ROOT/bin or in PATH.")
set(MRCD_FASTDDSGEN_DOCKER_IMAGE "" CACHE STRING "Docker image for fastddsgen (e.g. fastddsgen-arch). When set, IDL generation uses 'docker run' instead of native fastddsgen.")
if(MRCD_FASTDDS_ROOT)
  list(APPEND CMAKE_PREFIX_PATH ${MRCD_FASTDDS_ROOT})
  set(fastdds_DIR "${MRCD_FASTDDS_ROOT}/share/fastdds/cmake" CACHE PATH "Fast DDS CMake config dir")
  set(foonathan_memory_DIR "${MRCD_FASTDDS_ROOT}/share/foonathan_memory/cmake" CACHE PATH "")
  find_package(fastcdr REQUIRED)
  find_package(fastdds REQUIRED)
  add_subdirectory(messages)
  add_subdirectory(dds_core)
  add_subdirectory(apps/monitor_node)
endif()
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
install(FILES config/monitor_node.ini DESTINATION bin)

# Windows: deploy Qt runtime + Poco runtime into the install bin folder
if(WIN32)
  set(MRCD_WINDEPLOYQT "" CACHE FILEPATH "Path to windeployqt.exe (Qt6).")
  if(NOT MRCD_WINDEPLOYQT)
    find_program(MRCD_WINDEPLOYQT NAMES windeployqt windeployqt.exe)
  endif()
  if(NOT MRCD_WINDEPLOYQT AND Qt6_DIR)
    get_filename_component(_qt_cmake "${Qt6_DIR}" DIRECTORY)
    get_filename_component(_qt_lib "${_qt_cmake}" DIRECTORY)
    get_filename_component(_qt_prefix "${_qt_lib}" DIRECTORY)
    set(_qt_windeployqt "${_qt_prefix}/bin/windeployqt.exe")
    if(EXISTS "${_qt_windeployqt}")
      set(MRCD_WINDEPLOYQT "${_qt_windeployqt}" CACHE FILEPATH "Path to windeployqt.exe (auto from Qt6)" FORCE)
    endif()
  endif()

  set(MRCD_POCO_PREFIX "" CACHE PATH "Poco install prefix (e.g. C:/Users/Zoom/Opt/Poco).")
  if(NOT MRCD_POCO_PREFIX AND Poco_DIR)
    get_filename_component(_poco_parent "${Poco_DIR}" DIRECTORY)
    get_filename_component(_poco_grand "${_poco_parent}" DIRECTORY)
    get_filename_component(_poco_prefix3 "${_poco_grand}" DIRECTORY)
    foreach(_poco_p ${_poco_prefix3} ${_poco_parent})
      if(EXISTS "${_poco_p}/bin/PocoFoundation.dll")
        set(MRCD_POCO_PREFIX "${_poco_p}" CACHE PATH "Poco install prefix (auto from Poco)" FORCE)
        break()
      endif()
    endforeach()
  endif()

  # Bake cache values at configure time (install script cannot read $CACHE{} at install time)
  set(_install_windeployqt "${MRCD_WINDEPLOYQT}")
  set(_install_poco_prefix "${MRCD_POCO_PREFIX}")
  set(_install_fastdds_root "${MRCD_FASTDDS_ROOT}")
  set(_install_prefix "${CMAKE_INSTALL_PREFIX}")
  install(CODE "
set(MRCD_WINDEPLOYQT \"${_install_windeployqt}\")
set(MRCD_POCO_PREFIX \"${_install_poco_prefix}\")
set(MRCD_FASTDDS_ROOT \"${_install_fastdds_root}\")
if(NOT MRCD_WINDEPLOYQT)
  message(WARNING \"MRCD_WINDEPLOYQT not found; Qt DLLs/plugins will not be deployed. Set -DMRCD_WINDEPLOYQT=.../windeployqt.exe\")
else()
  message(STATUS \"Running windeployqt: \${MRCD_WINDEPLOYQT}\")
  execute_process(
    COMMAND \"\${MRCD_WINDEPLOYQT}\" --no-translations --no-compiler-runtime \"${_install_prefix}/bin/controller_app.exe\"
    RESULT_VARIABLE _windeployqt_rc
  )
  if(NOT _windeployqt_rc EQUAL 0)
    message(FATAL_ERROR \"windeployqt failed with code \${_windeployqt_rc}\")
  endif()
endif()
if(MRCD_POCO_PREFIX)
  set(_poco_bin \"\${MRCD_POCO_PREFIX}/bin\")
  message(STATUS \"Deploying Poco DLLs from: \${_poco_bin}\")
  file(GLOB _poco_dlls \"\${_poco_bin}/Poco*.dll\")
  if(_poco_dlls)
    file(INSTALL DESTINATION \"${_install_prefix}/bin\" TYPE FILE FILES \${_poco_dlls})
  else()
    message(WARNING \"No Poco*.dll found in \${_poco_bin}\")
  endif()
else()
  message(STATUS \"MRCD_POCO_PREFIX not set; skipping Poco DLL deployment\")
endif()
if(MRCD_FASTDDS_ROOT)
  set(_fastdds_bin \"\${MRCD_FASTDDS_ROOT}/bin\")
  if(EXISTS \"\${_fastdds_bin}\")
    message(STATUS \"Deploying Fast DDS DLLs from: \${_fastdds_bin}\")
    file(GLOB _fastdds_dlls \"\${_fastdds_bin}/*fastcdr*.dll\" \"\${_fastdds_bin}/*fastdds*.dll\" \"\${_fastdds_bin}/*fastrtps*.dll\" \"\${_fastdds_bin}/*foonathan*.dll\")
    if(_fastdds_dlls)
      file(INSTALL DESTINATION \"${_install_prefix}/bin\" TYPE FILE FILES \${_fastdds_dlls})
    else()
      message(WARNING \"No Fast DDS DLLs found in \${_fastdds_bin}\")
    endif()
  else()
    message(STATUS \"MRCD_FASTDDS_ROOT/bin not found; skipping Fast DDS DLL deployment\")
  endif()
else()
  message(STATUS \"MRCD_FASTDDS_ROOT not set; skipping Fast DDS DLL deployment\")
endif()
")
endif()
