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

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define AUDIO_INBUF_SIZE 30720
#define AUDIO_REFILL_THRESH 4096

class AudioDecoder {

private:
    AVCodec *codec;
    AVCodecContext *context;
    FILE *audioFile, *outfile;
    AVFrame *decoded_frame;
    AVCodecParserContext *parser;
    AVFormatContext *format;
    AVPacket *pkt;

    std::string filename;

    uint8_t *data;
    uint8_t *inbuf;

    void decodePacket();

    AVCodec* findCodec();

public:
    std::vector<uint8_t> outBuffer;

    explicit AudioDecoder(const std::string& filename);

    void decode();

    ~AudioDecoder();
};

#endif //MUSICNOISECOMBINE_MAIN_H
