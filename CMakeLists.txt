cmake_minimum_required(VERSION 3.16.5)
project(aqs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(CMAKE_CXX_STANDARD 14)

add_executable(linux main.cpp include/common.h thread/oop/JavaThread.cpp thread/oop/JavaThread.h thread/oop/Atomic.cpp thread/oop/Atomic.h thread/oop/ObjectWaiter.cpp thread/oop/ObjectWaiter.h thread/oop/ObjectMonitor.cpp thread/oop/ObjectMonitor.h thread/oop/ParkEvent.cpp thread/oop/ParkEvent.h thread/oop/MarkOopDesc.cpp thread/oop/MarkOopDesc.h thread/oop/InstanceOopDesc.cpp thread/oop/InstanceOopDesc.h)