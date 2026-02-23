# CMake for DuiLib (added for tag 0.3 which lacks it)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} RootSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Control ControlSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Core CoreSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Layout LayoutSrc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils UtilsSrc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Control)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Layout)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Utils)

add_library(DuiLib SHARED ${ControlSrc} ${CoreSrc} ${LayoutSrc} ${UtilsSrc} ${RootSrc})

add_definitions(-DUILIB_EXPORTS)
target_link_libraries(DuiLib comctl32)
set_target_properties(DuiLib PROPERTIES
  DEBUG_POSTFIX ""
  OUTPUT_NAME "DuiLib"
)
