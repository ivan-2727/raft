cmake_minimum_required(VERSION 3.10)
project(RaftServer)
set(CMAKE_CXX_STANDARD 17)
find_package(Boost 1.65 REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(RaftServer src/main.cpp)
    target_link_libraries(RaftServer ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Could not find Boost libraries")
endif()
