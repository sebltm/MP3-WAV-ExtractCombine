cmake_minimum_required(VERSION 3.16)
project(MusicNoiseCombine)

set(CMAKE_CXX_STANDARD 14)

add_executable(MusicNoiseCombine main.cpp main.h)
target_link_libraries(MusicNoiseCombine avformat)
target_link_libraries(MusicNoiseCombine avcodec)
target_link_libraries(MusicNoiseCombine avutil)
target_link_libraries(MusicNoiseCombine avdevice)
target_link_libraries(MusicNoiseCombine postproc)
target_link_libraries(MusicNoiseCombine IO)
