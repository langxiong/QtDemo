add_executable(algo_worker
  src/main.cpp
)

target_link_libraries(algo_worker
  PRIVATE
    common
)

target_compile_features(algo_worker PRIVATE cxx_std_17)
