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
}

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

class AudioDecoder {

private:
    AVCodec *codec;
    AVCodecContext *context;
    FILE *audioFile;
    std::string filename;
    AVFrame *decoded_frame;
    AVCodecParserContext *parser;
    AVPacket *pkt;

    uint8_t *inbuf;
    uint8_t *data;

    size_t dataSize;
    int ret;

    void decode();

public:
    std::vector<uint8_t> outBuffer;

    explicit AudioDecoder(const std::string& fileType);

    void decode(const std::string&);

    ~AudioDecoder() {
        pkt->data = nullptr;
        pkt->size = 0;
        decode();

        fclose(audioFile);

        avcodec_free_context(&context);
        av_parser_close(parser);
        av_frame_free(&decoded_frame);
        av_packet_free(&pkt);
    }
};

#endif //MUSICNOISECOMBINE_MAIN_H
