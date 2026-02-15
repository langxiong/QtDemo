cmake_minimum_required(VERSION 3.16)

set(MESSAGES_IDL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MESSAGES_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Find fastddsgen or use Docker (Fast-DDS-Gen is separate from Fast DDS)
set(FASTDDSGEN_USE_DOCKER FALSE)
if(MRCD_FASTDDSGEN_DOCKER_IMAGE)
  find_program(FASTDDSGEN_DOCKER docker)
  if(FASTDDSGEN_DOCKER)
    set(FASTDDSGEN_USE_DOCKER TRUE)
  else()
    message(FATAL_ERROR "MRCD_FASTDDSGEN_DOCKER_IMAGE=${MRCD_FASTDDSGEN_DOCKER_IMAGE} but docker not found in PATH")
  endif()
elseif(MRCD_FASTDDSGEN AND EXISTS "${MRCD_FASTDDSGEN}")
  set(FASTDDSGEN "${MRCD_FASTDDSGEN}")
else()
  set(_fastddsgen_paths "")
  if(MRCD_FASTDDS_ROOT)
    list(APPEND _fastddsgen_paths ${MRCD_FASTDDS_ROOT}/bin ${MRCD_FASTDDS_ROOT}/scripts)
  endif()
  find_program(FASTDDSGEN fastddsgen fastddsgen.bat
    HINTS ${_fastddsgen_paths}
    PATHS ${_fastddsgen_paths}
  )
endif()
if(NOT FASTDDSGEN AND NOT FASTDDSGEN_USE_DOCKER)
  message(FATAL_ERROR
    "fastddsgen not found. Options:\n"
    "  - Use Docker: -DMRCD_FASTDDSGEN_DOCKER_IMAGE=fastddsgen-arch (see FASTDDS_SETUP.md)\n"
    "  - Install from https://github.com/eProsima/Fast-DDS-Gen and set -DMRCD_FASTDDSGEN=/path/to/fastddsgen\n"
    "  - Or add fastddsgen to PATH")
endif()

# Generate C++ from IDL
file(MAKE_DIRECTORY ${MESSAGES_GEN_DIR})
set(IDL_FILES JointState.idl ControlCommand.idl)

# Docker: -v idl_dir:/idl -v gen_dir:/out, fastddsgen -d /out -replace /idl/file.idl
set(MESSAGES_STAMPS "")
foreach(_idl ${IDL_FILES})
  set(_idl_path ${MESSAGES_IDL_DIR}/${_idl})
  if(FASTDDSGEN_USE_DOCKER)
    add_custom_command(
      OUTPUT
        ${MESSAGES_GEN_DIR}/${_idl}-generated.stamp
      COMMAND ${FASTDDSGEN_DOCKER} run --rm
        -v "${MESSAGES_IDL_DIR}:/idl"
        -v "${MESSAGES_GEN_DIR}:/out"
        ${MRCD_FASTDDSGEN_DOCKER_IMAGE}
        -ppDisable -d /out -replace /idl/${_idl}
      COMMAND ${CMAKE_COMMAND} -E touch ${MESSAGES_GEN_DIR}/${_idl}-generated.stamp
      DEPENDS ${_idl_path}
      COMMENT "Generating C++ from ${_idl} (Docker)"
    )
  else()
    add_custom_command(
      OUTPUT
        ${MESSAGES_GEN_DIR}/${_idl}-generated.stamp
      COMMAND ${FASTDDSGEN}
        -ppDisable
        -d ${MESSAGES_GEN_DIR}
        -replace
        ${_idl_path}
      COMMAND ${CMAKE_COMMAND} -E touch ${MESSAGES_GEN_DIR}/${_idl}-generated.stamp
      DEPENDS ${_idl_path}
      COMMENT "Generating C++ from ${_idl}"
    )
  endif()
  list(APPEND MESSAGES_STAMPS ${MESSAGES_GEN_DIR}/${_idl}-generated.stamp)
endforeach()

# Collect generated sources (naming depends on fastddsgen output)
add_custom_target(messages_generate ALL DEPENDS ${MESSAGES_STAMPS})

# List expected outputs (Fast DDS-Gen: *PubSubTypes.cxx + *TypeObjectSupport.cxx; no *Type.cxx)
set(MESSAGES_SOURCES
  ${MESSAGES_GEN_DIR}/JointStatePubSubTypes.cxx
  ${MESSAGES_GEN_DIR}/JointStateTypeObjectSupport.cxx
  ${MESSAGES_GEN_DIR}/ControlCommandPubSubTypes.cxx
  ${MESSAGES_GEN_DIR}/ControlCommandTypeObjectSupport.cxx
)
# Mark as generated so CMake does not require them at configure time
set_source_files_properties(${MESSAGES_SOURCES} PROPERTIES GENERATED TRUE)

add_library(messages ${MESSAGES_SOURCES})
add_dependencies(messages messages_generate)
set_target_properties(messages PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(messages PUBLIC ${MESSAGES_GEN_DIR})
target_link_libraries(messages PUBLIC fastcdr fastdds)
target_compile_features(messages PUBLIC cxx_std_17)

install(TARGETS messages ARCHIVE DESTINATION lib)
