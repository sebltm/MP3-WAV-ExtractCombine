cmake_minimum_required(VERSION 3.13)
project(MusicNoiseCombine)

set(CMAKE_CXX_STANDARD 14)

add_executable(MusicNoiseCombine main.cpp main.h audioDecoder.cpp)

target_link_libraries(MusicNoiseCombine portaudio)
target_link_libraries(MusicNoiseCombine avformat)
target_link_libraries(MusicNoiseCombine avcodec)
target_link_libraries(MusicNoiseCombine avutil)
target_link_libraries(MusicNoiseCombine IO)
