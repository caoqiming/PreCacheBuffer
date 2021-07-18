cmake_minimum_required (VERSION 3.19.6)
add_definitions(-std=c++11)

set(Boost_INCLUDE_DIR "/Users/bytedance/Documents/boost/include")
set(Boost_LIBRARY_DIR "/Users/bytedance/Documents/boost/lib")



project (PreCachedBuffer)
find_package(Boost 1.76.0) 
include_directories(${Boost_INCLUDE_DIR})
include_directories("./include")
link_directories(${Boost_LIBRARY_DIR})

add_executable(test src/main.cpp)
