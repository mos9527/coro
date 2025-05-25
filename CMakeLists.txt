cmake_minimum_required(VERSION 3.30)
project(coroutines)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(exp_task examples/task.cpp)
add_executable(exp_event_loop examples/event_loop.cpp)
add_executable(exp_generator examples/generator.cpp)