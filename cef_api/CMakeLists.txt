cmake_minimum_required(VERSION 3.16)

# nlohmann/json: fetch single-header only to avoid json project CMake
find_package(nlohmann_json 3.10 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
  )
  FetchContent_MakeAvailable(json)
endif()

add_library(cef_api)

target_sources(cef_api
  PRIVATE
    src/dto/ApplicationData.cpp
    src/dto/StreamData.cpp
    src/dto/ApiEnvelope.cpp
    src/ApplicationHandler.cpp
    src/StreamHandler.cpp
    src/APIInterface.cpp
)

target_include_directories(cef_api
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cef_api
  PUBLIC
    nlohmann_json::nlohmann_json
  PRIVATE
    common
)

target_compile_features(cef_api PUBLIC cxx_std_17)
