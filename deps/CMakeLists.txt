# Superbuild for demo dependencies: foonathan_memory, Fast-CDR, Fast-DDS, Poco
# Build order: foonathan_memory -> Fast-CDR -> Fast-DDS -> Poco
# Debug and Release install to separate dirs: install/Debug, install/Release
cmake_minimum_required(VERSION 3.16)

project(deps_build NONE)

include(ExternalProject)

set(DEPS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEPS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DEPS_INSTALL_BASE ${DEPS_SOURCE_DIR}/install CACHE PATH "Deps install root")

# Install prefix: caller can set DEPS_INSTALL_PREFIX directly (e.g. for merged multi-config install)
# Otherwise: install/Debug or install/Release from DEPS_INSTALL_CONFIG or CMAKE_BUILD_TYPE
if(DEFINED DEPS_INSTALL_PREFIX AND NOT "${DEPS_INSTALL_PREFIX}" STREQUAL "")
  # Use caller-provided prefix as-is
elseif(DEFINED DEPS_INSTALL_CONFIG)
  set(DEPS_INSTALL_PREFIX ${DEPS_INSTALL_BASE}/${DEPS_INSTALL_CONFIG})
elseif(CMAKE_BUILD_TYPE)
  set(DEPS_INSTALL_PREFIX ${DEPS_INSTALL_BASE}/${CMAKE_BUILD_TYPE})
else()
  set(DEPS_INSTALL_PREFIX ${DEPS_INSTALL_BASE}/Release)
endif()

# Propagate build type and config for multi-config generators
set(_ep_common_args
  -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_PREFIX}
)
if(CMAKE_BUILD_TYPE)
  list(APPEND _ep_common_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
elseif(DEFINED DEPS_INSTALL_CONFIG)
  list(APPEND _ep_common_args -DCMAKE_BUILD_TYPE=${DEPS_INSTALL_CONFIG})
endif()
if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND _ep_common_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

# Fast-DDS: /utf-8 /WX- applied directly in fastdds/CMakeLists.txt for C4819 on CP936

# 1. foonathan_memory (via foonathan_memory_vendor - uses EP internally to fetch memory)
ExternalProject_Add(dep_foonathan_memory
  SOURCE_DIR ${DEPS_SOURCE_DIR}/foonathan_memory_vendor
  BINARY_DIR ${DEPS_BINARY_DIR}/foonathan_memory_vendor-build
  CMAKE_ARGS
    ${_ep_common_args}
    -DFOONATHAN_MEMORY_FORCE_VENDORED_BUILD=ON
  INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
  BUILD_ALWAYS OFF
)

# 2. Fast-CDR (no deps)
ExternalProject_Add(dep_fastcdr
  SOURCE_DIR ${DEPS_SOURCE_DIR}/fastcdr
  BINARY_DIR ${DEPS_BINARY_DIR}/fastcdr-build
  CMAKE_ARGS
    ${_ep_common_args}
    -DBUILD_TESTING=OFF
  INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
  BUILD_ALWAYS OFF
)

# 3. Fast-DDS (depends on fastcdr, foonathan_memory; asio/tinyxml2 from thirdparty)
# THIRDPARTY=ON uses thirdparty when package not found; avoid EPROSIMA_BUILD to skip tests (GTest)
ExternalProject_Add(dep_fastdds
  SOURCE_DIR ${DEPS_SOURCE_DIR}/fastdds
  BINARY_DIR ${DEPS_BINARY_DIR}/fastdds-build
  DEPENDS dep_foonathan_memory dep_fastcdr
  CMAKE_ARGS
    ${_ep_common_args}
    -DCMAKE_PREFIX_PATH=${DEPS_INSTALL_PREFIX}
    -DTHIRDPARTY=ON
  INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
  BUILD_ALWAYS OFF
)

# 4. Poco (Foundation, Util, Net only) - no deps on DDS, can build in parallel
# /FS for MSVC: avoid C1041 when parallel build writes to same PDB
set(_ep_poco_extra "")
if(MSVC)
  set(_ep_poco_extra -DCMAKE_CXX_FLAGS:STRING="${CMAKE_CXX_FLAGS} /FS")
endif()
ExternalProject_Add(dep_poco
  SOURCE_DIR ${DEPS_SOURCE_DIR}/poco
  BINARY_DIR ${DEPS_BINARY_DIR}/poco-build
  CMAKE_ARGS
    ${_ep_common_args}
    ${_ep_poco_extra}
    -DPOCO_MINIMAL_BUILD=ON
    -DENABLE_FOUNDATION=ON
    -DENABLE_UTIL=ON
    -DENABLE_NET=ON
  INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
  BUILD_ALWAYS OFF
)

# 5. DuiLib (Windows only) - CEF parent window shell
if(WIN32)
  ExternalProject_Add(dep_duilib
    SOURCE_DIR ${DEPS_SOURCE_DIR}/duilib_build
    BINARY_DIR ${DEPS_BINARY_DIR}/duilib-build
    CMAKE_ARGS
      ${_ep_common_args}
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
    BUILD_ALWAYS OFF
  )
endif()
