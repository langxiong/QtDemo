# cef_host: CEF host for React UI.
# Set MRCD_CEF_ROOT to CEF binary distribution root (e.g. cef_binary_* from cef-builds.spotifycdn.com).
# See demo/docs/CEF_REACT_SETUP.md for CEF build and link instructions.
# Uses native popup window. DuiLib host window belongs to controller_app, not cef_host.

set(MRCD_CEF_ROOT "" CACHE PATH "CEF binary distribution root")

# Detect CEF and load CEF configuration (required for libcef_dll macros)
set(_CEF_LIBCEF_DLL "")
if(MRCD_CEF_ROOT)
  if(EXISTS "${MRCD_CEF_ROOT}/libcef_dll/CMakeLists.txt")
    set(_CEF_LIBCEF_DLL "${MRCD_CEF_ROOT}/libcef_dll")
  elseif(EXISTS "${MRCD_CEF_ROOT}/libcef_dll_wrapper/CMakeLists.txt")
    set(_CEF_LIBCEF_DLL "${MRCD_CEF_ROOT}/libcef_dll_wrapper")
  endif()
  if(_CEF_LIBCEF_DLL AND EXISTS "${MRCD_CEF_ROOT}/cmake/FindCEF.cmake")
    set(CEF_ROOT "${MRCD_CEF_ROOT}" CACHE PATH "CEF root" FORCE)
    list(APPEND CMAKE_MODULE_PATH "${MRCD_CEF_ROOT}/cmake")
    find_package(CEF REQUIRED)
    # CEF prebuilt from CDN is Release-only; fallback so libcef_lib works for Debug builds
    if((NOT CEF_LIB_DEBUG OR NOT EXISTS "${CEF_LIB_DEBUG}") AND CEF_LIB_RELEASE AND EXISTS "${CEF_LIB_RELEASE}")
      set(CEF_LIB_DEBUG "${CEF_LIB_RELEASE}")
    endif()
  endif()
endif()

set(CEF_HOST_SOURCES)
if(_CEF_LIBCEF_DLL)
  if(WIN32)
    list(APPEND CEF_HOST_SOURCES src/main_win.cc)
  else()
    list(APPEND CEF_HOST_SOURCES src/main.cpp)
  endif()
  list(APPEND CEF_HOST_SOURCES
    src/cef_host_app.cpp
    src/cef_render_handler.cpp
    src/api_v8_handler.cpp
    src/CefHostApplication.cpp
  )
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
  add_subdirectory(${_CEF_LIBCEF_DLL} ${CMAKE_BINARY_DIR}/cef_host_libcef_dll)
else()
  list(APPEND CEF_HOST_SOURCES src/main.cpp)
endif()

if(_CEF_LIBCEF_DLL)
  add_executable(cef_host WIN32 ${CEF_HOST_SOURCES})
else()
  add_executable(cef_host ${CEF_HOST_SOURCES})  # Console for stub mode
endif()

target_include_directories(cef_host
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(_CEF_LIBCEF_DLL)
  target_include_directories(cef_host PRIVATE "${MRCD_CEF_ROOT}")
  target_link_libraries(cef_host
    PRIVATE
      libcef_lib
      libcef_dll_wrapper
      cef_api
      common
      Poco::Foundation
      Poco::Util
  )
  target_compile_definitions(cef_host PRIVATE HAVE_CEF=1 NOMINMAX)
  target_compile_features(cef_host PRIVATE cxx_std_20)  # CEF 145+ requires C++20
  message(STATUS "cef_host: CEF enabled (MRCD_CEF_ROOT=${MRCD_CEF_ROOT})")

  # Deploy CEF DLLs and Resources to output dir for runtime.
  # CEF expects icudtl.dat, *.pak, locales/ in the same directory as libcef.dll (not in a Resources/ subdir).
  set(_CEF_RELEASE "${MRCD_CEF_ROOT}/Release")
  set(_CEF_RESOURCES "${MRCD_CEF_ROOT}/Resources")
  if(EXISTS "${_CEF_RELEASE}")
    add_custom_command(TARGET cef_host POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${_CEF_RELEASE}" "$<TARGET_FILE_DIR:cef_host>"
      COMMENT "Deploying CEF Release DLLs to cef_host output"
    )
  endif()
  if(EXISTS "${_CEF_RESOURCES}")
    add_custom_command(TARGET cef_host POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${_CEF_RESOURCES}" "$<TARGET_FILE_DIR:cef_host>"
      COMMENT "Deploying CEF Resources (icudtl.dat, *.pak, locales) to cef_host output"
    )
  endif()
else()
  target_link_libraries(cef_host
    PRIVATE
      cef_api
      common
      Poco::Foundation
  )
  target_compile_features(cef_host PRIVATE cxx_std_17)
  message(STATUS "cef_host: stub mode (set MRCD_CEF_ROOT for full CEF)")
endif()

install(TARGETS cef_host RUNTIME DESTINATION bin)
