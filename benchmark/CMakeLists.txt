set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/benchmarks)

set(benchbin "")

macro(add_benchmark source_name)
    add_executable(${source_name} ${source_name}.cpp utils.cpp)
    target_link_libraries(${source_name} Boost::system Boost::program_options nlohmann_json::nlohmann_json Asterisk Threads::Threads NTL GMP EMPTool)
    list(APPEND benchbin ${source_name})
endmacro()

add_benchmark(graphsc_mpc)
add_benchmark(mult)
add_benchmark(shuffle)
add_benchmark(chase_shuffle)
add_benchmark(scatter)
add_benchmark(gather)
add_benchmark(comparison)

add_custom_target(benchmarks)
add_dependencies(benchmarks ${benchbin})
