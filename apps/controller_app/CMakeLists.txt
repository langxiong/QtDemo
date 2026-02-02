add_executable(controller_app
  src/main.cpp
)

target_link_libraries(controller_app
  PRIVATE
    common
    Qt6::Widgets
    Qt6::Charts
)

target_compile_features(controller_app PRIVATE cxx_std_17)

install(TARGETS controller_app
  RUNTIME DESTINATION bin
)
