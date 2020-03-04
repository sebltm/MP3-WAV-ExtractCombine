#include "main.h"

int main(int argc, char** argv) {
    AudioDecoder audioDecoder = AudioDecoder();
    audioDecoder.filename = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/000/000002.mp3";

    audioDecoder.decode();
}