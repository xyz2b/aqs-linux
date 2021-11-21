cmake_minimum_required(VERSION 3.16.5)
project(aqs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(CMAKE_CXX_STANDARD 14)

add_executable(linux main.cpp include/common.h thread/JavaThread.cpp thread/JavaThread.h thread/Atomic.cpp thread/Atomic.h thread/ObjectWaiter.cpp thread/ObjectWaiter.h thread/ObjectMonitor.cpp thread/ObjectMonitor.h thread/ParkEvent.cpp thread/ParkEvent.h)