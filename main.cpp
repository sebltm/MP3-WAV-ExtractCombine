#include "main.h"

int main(int argc, char** argv) {
    std::string filename = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/000/000002.mp3";
    AudioDecoder audioDecoder = AudioDecoder(filename);
    audioDecoder.decode();

    filename = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/UrbanSound8K/audio/fold1/7061-6-0-0.wav";
    audioDecoder = AudioDecoder(filename);
    audioDecoder.decode();
}