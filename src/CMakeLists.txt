add_library(Asterisk
    utils/circuit.cpp
    utils/types.cpp
    utils/helpers.cpp
    graphsc/sharing.cpp
    graphsc/rand_gen_pool.cpp
    graphsc/offline_evaluator.cpp
    graphsc/online_evaluator_load_balanced.cpp)



target_include_directories(Asterisk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Asterisk PUBLIC Boost::system EMPTool NTL GMP)
