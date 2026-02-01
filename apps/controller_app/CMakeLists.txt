add_executable(controller_app
  src/main.cpp
)

target_link_libraries(controller_app
  PRIVATE
    common
    Qt5::Widgets
    Qt5::Charts
)

target_compile_features(controller_app PRIVATE cxx_std_17)
