cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(MultiCent VERSION 0.1)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-march=native -maes -mrdseed -lntl -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Path to custom cmake files for external libs
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Find external dependencies
find_package(GMP REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost 1.72.0 COMPONENTS unit_test_framework program_options system REQUIRED)
find_package(emp-tool REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(NTL 11.0.0 REQUIRED)

if(EMP-TOOL_FOUND)
    add_library(EMPTool INTERFACE)
    target_link_libraries(EMPTool INTERFACE ${EMP-TOOL_LIBRARIES})
    target_include_directories(EMPTool INTERFACE ${EMP-TOOL_INCLUDE_DIRS})
endif()

add_subdirectory(src)
add_subdirectory(benchmark)


