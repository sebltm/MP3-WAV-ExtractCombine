//
// Created by sebltm on 02/03/2020.
//

#ifndef MUSICNOISECOMBINE_MAIN_H
#define MUSICNOISECOMBINE_MAIN_H

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include <IO.h>
#include <sndfile.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define AUDIO_INBUF_SIZE 30720
#define AUDIO_REFILL_THRESH 4096

int AudioDecoderMP3(const std::string& filename);
void decode(AVCodecContext *context, AVPacket *pkt, AVFrame *frame, std::vector<uint8_t> *buffer, FILE *outfile);

int AudioDecoderWAV(const std::string& filename);

#endif //MUSICNOISECOMBINE_MAIN_H
