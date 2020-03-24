cmake_minimum_required(VERSION 3.9)
project(MusicNoiseCombine)

set(CMAKE_CXX_STANDARD 17)

add_executable(MusicNoiseCombine main.cpp main.h audioDecoderMP3.cpp audioDecoderWAV.cpp)

target_link_libraries(MusicNoiseCombine avformat avcodec avutil swresample sndfile samplerate)
