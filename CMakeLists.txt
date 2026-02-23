cmake_minimum_required(VERSION 3.16)

# Initialize git submodules (deps) so they are available for building
find_package(Git QUIET)
if(GIT_FOUND)
  get_filename_component(_git_root "${CMAKE_SOURCE_DIR}" DIRECTORY)
  set(_submod_dir "")
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(_submod_dir "${CMAKE_SOURCE_DIR}")
  elseif(EXISTS "${_git_root}/.git")
    set(_submod_dir "${_git_root}")
  endif()
  if(_submod_dir)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${_submod_dir}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(WARNING "git submodule update failed (code ${GIT_SUBMOD_RESULT}), deps may be missing")
    endif()
  endif()
endif()

project(medical_robot_control_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8)  # Avoid C4819 on CP936 (Chinese) systems
  # Force the correct multithreaded-DLL runtime library for all targets.
  # This ensures consistency with pre-built dependencies like Qt and Poco.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  # CEF libcef_dll_wrapper uses CEF_RUNTIME_LIBRARY_FLAG; keep in sync with project.
  set(CEF_RUNTIME_LIBRARY_FLAG "/MD" CACHE STRING "CEF runtime (matches project)")
  if(NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
    set(CEF_RUNTIME_LIBRARY_FLAG "/MT" CACHE STRING "CEF runtime (matches project)" FORCE)
  endif()
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

# Option: use deps from submodules (demo/deps) or system/vcpkg
option(MRCD_USE_DEPS_SUBMODULES "Build and use Poco/Fast-DDS from demo/deps submodules" OFF)
option(MRCD_FETCH_CEF "Fetch CEF prebuilt when bootstrapping deps (requires Python 3)" ON)
set(MRCD_FASTDDS_ROOT "" CACHE PATH "Fast DDS install prefix. Required unless MRCD_USE_DEPS_SUBMODULES=ON.")

set(DEPS_INSTALL_BASE "${CMAKE_SOURCE_DIR}/deps/install")

if(MRCD_USE_DEPS_SUBMODULES)
  # Single-config only: build deps for current CMAKE_BUILD_TYPE, install to install/Debug or install/Release
  # Use build_debug/build_release at project root for compatibility with vcpkg/Conan
  set(_deps_build "${CMAKE_BINARY_DIR}/deps_build")
  set(_cfg ${CMAKE_BUILD_TYPE})
  if(NOT _cfg)
    set(_cfg "Release")
  endif()
  set(_deps_install_prefix "${DEPS_INSTALL_BASE}/${_cfg}")
  set(_deps_marker "${_deps_install_prefix}/.deps_built")
  if(NOT EXISTS "${_deps_marker}")
    message(STATUS "Building deps for ${_cfg} (first-time setup)...")
    set(_deps_cfg_args
      -B ${_deps_build}_${_cfg} -S ${CMAKE_SOURCE_DIR}/deps
      -DCMAKE_INSTALL_PREFIX=${_deps_install_prefix}
      -DDEPS_INSTALL_CONFIG=${_cfg}
      -DCMAKE_BUILD_TYPE=${_cfg}
    )
    if(CMAKE_TOOLCHAIN_FILE)
      list(APPEND _deps_cfg_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} ${_deps_cfg_args} RESULT_VARIABLE _rc1)
    if(NOT _rc1 EQUAL 0)
      message(FATAL_ERROR "Deps configure for ${_cfg} failed. Run: git submodule update --init --recursive")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build ${_deps_build}_${_cfg} --config ${_cfg} RESULT_VARIABLE _rc2)
    if(NOT _rc2 EQUAL 0)
      message(FATAL_ERROR "Deps build for ${_cfg} failed")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --install ${_deps_build}_${_cfg} --config ${_cfg} --prefix ${_deps_install_prefix} RESULT_VARIABLE _rc3)
    if(NOT _rc3 EQUAL 0)
      message(FATAL_ERROR "Deps install for ${_cfg} failed")
    endif()
    file(WRITE "${_deps_marker}" "")
    message(STATUS "Deps built and installed to ${_deps_install_prefix}")
  endif()
  if(MRCD_FETCH_CEF AND NOT MRCD_CEF_ROOT)
    file(GLOB _cef_existing "${CMAKE_SOURCE_DIR}/prebuilt/cef/cef_binary_*_windows64")
    if(NOT _cef_existing)
      find_package(Python3 QUIET COMPONENTS Interpreter)
      if(Python3_Interpreter_FOUND)
        execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/fetch_cef.py"
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          RESULT_VARIABLE _cef_rc)
        if(NOT _cef_rc EQUAL 0)
          message(WARNING "CEF fetch failed (code ${_cef_rc}). Run: python demo/scripts/fetch_cef.py")
        endif()
      else()
        message(STATUS "Python3 not found; skip CEF fetch. Run: python demo/scripts/fetch_cef.py")
      endif()
    endif()
  endif()
  list(PREPEND CMAKE_PREFIX_PATH ${_deps_install_prefix})
  set(MRCD_FASTDDS_ROOT "${_deps_install_prefix}" CACHE PATH "Fast DDS from deps" FORCE)
  if(WIN32)
    set(MRCD_DUILIB_ROOT "${_deps_install_prefix}" CACHE PATH "DuiLib from deps" FORCE)
  endif()
endif()

set(MRCD_DUILIB_ROOT "" CACHE PATH "DuiLib install prefix. When set, controller_app uses DuiLib MainWindow.")

# Poco (from deps or system/vcpkg). Explicit version avoids ConfigVersion rejecting empty PACKAGE_FIND_VERSION.
find_package(Poco 1.10 REQUIRED COMPONENTS Foundation Util Net)

# Fast DDS (from deps or MRCD_FASTDDS_ROOT; see docs/FASTDDS_SETUP.md)
set(MRCD_FASTDDSGEN "" CACHE FILEPATH "Path to fastddsgen (or fastddsgen.bat). Set if not under MRCD_FASTDDS_ROOT/bin or in PATH.")
set(MRCD_FASTDDSGEN_DOCKER_IMAGE "" CACHE STRING "Docker image for fastddsgen (e.g. fastddsgen-mrcd). When set, IDL generation uses 'docker run' instead of native fastddsgen.")
# When using deps, default to Docker fastddsgen if neither native nor image specified
if(MRCD_USE_DEPS_SUBMODULES AND MRCD_FASTDDS_ROOT AND NOT MRCD_FASTDDSGEN AND NOT MRCD_FASTDDSGEN_DOCKER_IMAGE)
  set(MRCD_FASTDDSGEN_DOCKER_IMAGE "fastddsgen-mrcd" CACHE STRING "Docker image for fastddsgen" FORCE)
endif()
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

# CEF prebuilt: auto-detect from demo/prebuilt/cef when MRCD_CEF_ROOT not set
set(MRCD_PREBUILT_ROOT "${CMAKE_SOURCE_DIR}/prebuilt")
if(NOT MRCD_CEF_ROOT AND EXISTS "${MRCD_PREBUILT_ROOT}/cef")
  file(GLOB _cef_dirs "${MRCD_PREBUILT_ROOT}/cef/cef_binary_*_windows64")
  if(_cef_dirs)
    list(GET _cef_dirs 0 _cef_root)
    set(MRCD_CEF_ROOT "${_cef_root}" CACHE PATH "CEF from prebuilt" FORCE)
    message(STATUS "MRCD_CEF_ROOT auto-detected: ${MRCD_CEF_ROOT}")
  endif()
endif()

add_subdirectory(common)
add_subdirectory(cef_api)
add_subdirectory(apps/cef_host)
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

# Frontend build (npm run build) - run manually: cd frontend && npm run build
add_custom_target(frontend_build
  COMMAND npm run build
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/frontend
  COMMENT "Building React frontend"
)

# Windows: deploy Poco runtime (and Fast DDS) into the install bin folder
if(WIN32)
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
  set(_install_poco_prefix "${MRCD_POCO_PREFIX}")
  set(_install_fastdds_root "${MRCD_FASTDDS_ROOT}")
  set(_install_prefix "${CMAKE_INSTALL_PREFIX}")
  install(CODE "
set(MRCD_POCO_PREFIX \"${_install_poco_prefix}\")
set(MRCD_FASTDDS_ROOT \"${_install_fastdds_root}\")
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
