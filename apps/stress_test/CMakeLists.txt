add_executable(stress_test
  src/main.cpp
)

target_link_libraries(stress_test
  PRIVATE
    common
)

target_compile_features(stress_test PRIVATE cxx_std_17)
