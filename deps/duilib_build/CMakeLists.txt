# Build only DuiLib from DuiLib_Ultimate (no Demos).
# Source from demo/deps/duilib (local in deps); apply duilib-no-flash.patch to exclude UIFlash.
cmake_minimum_required(VERSION 3.16)
project(duilib_build LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

# Use local duilib in deps directory
set(DUILIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../duilib)
if(NOT EXISTS "${DUILIB_SOURCE_DIR}/DuiLib/CMakeLists.txt")
  message(FATAL_ERROR "DuiLib source not found at ${DUILIB_SOURCE_DIR}. Clone: git clone https://github.com/qdtroy/DuiLib_Ultimate.git demo/deps/duilib")
endif()

# Exclude UIFlash (requires flash11.tlh, Flash deprecated)
set(_duilib_noflash "${CMAKE_CURRENT_SOURCE_DIR}/../patches/duilib-no-flash.patch")
if(EXISTS "${_duilib_noflash}" AND EXISTS "${DUILIB_SOURCE_DIR}/DuiLib/UIlib.h")
  find_program(PATCH_EXECUTABLE patch PATHS ENV PATH)
  if(PATCH_EXECUTABLE)
    execute_process(
      COMMAND ${PATCH_EXECUTABLE} -p1 -i "${_duilib_noflash}"
      WORKING_DIRECTORY ${DUILIB_SOURCE_DIR}
      RESULT_VARIABLE _pf_rc
    )
    if(_pf_rc EQUAL 0)
      message(STATUS "duilib: applied duilib-no-flash.patch (exclude UIFlash)")
    endif()
  endif()
endif()

add_subdirectory(${DUILIB_SOURCE_DIR}/DuiLib ${CMAKE_BINARY_DIR}/DuiLib)

# DuiLib has no install; add our own (Windows: RUNTIME=dll, ARCHIVE=lib)
install(TARGETS DuiLib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY ${DUILIB_SOURCE_DIR}/DuiLib/
  DESTINATION include/duilib
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
