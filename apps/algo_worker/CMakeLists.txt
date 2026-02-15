add_executable(algo_worker src/main_dds.cpp)
target_link_libraries(algo_worker PRIVATE dds_core common)

target_compile_features(algo_worker PRIVATE cxx_std_17)

install(TARGETS algo_worker RUNTIME DESTINATION bin)
