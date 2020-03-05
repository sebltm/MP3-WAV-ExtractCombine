cmake_minimum_required(VERSION 3.13)
project(MusicNoiseCombine)

set(CMAKE_CXX_STANDARD 14)

add_executable(MusicNoiseCombine main.cpp main.h audioDecoderMP3.cpp audioDecoderWAV.cpp)

target_link_libraries(MusicNoiseCombine avformat avcodec avutil swresample sndfile)
