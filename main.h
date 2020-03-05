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
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#define AUDIO_INBUF_SIZE 30720
#define AUDIO_REFILL_THRESH 4096

struct decodedFile {
    int samplerate;
    int num_samples;
};

decodedFile AudioDecoderMP3(const std::string& filename, float *& outBuffer, const std::string& outfilePath);
void decode(AVCodecContext *context, AVPacket *pkt, AVFrame *frame, SwrContext  *swr, std::vector<float> *buffer,
        FILE *outfile);
int resample(AVCodecContext *context, AVFrame *frame, uint8_t*& buffer);

decodedFile AudioDecoderWAV(const std::string& filename, float*& outBuffer, const std::string& outfilePath);

float * MonoAndShorten(float *buffer, int channels, int sampleRate, int duration);

#endif //MUSICNOISECOMBINE_MAIN_H
