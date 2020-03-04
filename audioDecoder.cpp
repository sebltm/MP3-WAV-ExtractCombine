//
// Created by sebltm on 03/03/2020.
//

#include "main.h"

void AudioDecoder::decode(AVCodecContext *dec_ctx, AVPacket *dec_pkt, AVFrame *frame, std::vector<uint8_t>
        *dec_buffer) {
/* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, dec_pkt);
    if (ret == AVERROR(EAGAIN)) {
        fprintf(stderr, "Error while submitting the packet to the decoder: input is not accepted in the current state");
        // exit(-1);
    } else if (ret == AVERROR_EOF) {
        fprintf(stderr, "Error while submitting the packet to the decoder: decoder has been flushed");
        // exit(-1);
    } else if (ret == AVERROR(EINVAL)) {
        fprintf(stderr, "Error while submitting the packet to the decoder: coded not opened, it is an encoder");
        // exit(-1);
    } else if (ret == AVERROR(ENOMEM)) {
        fprintf(stderr, "Error while submitting the packet to the decoder: failed to add packet to queue - decoding "
                        "error");
        // exit(-1);
    } else if (ret < 0) {
        fprintf(stderr, "Error while submitting the packet to the decoder: %d\n", ret);
        // exit(-1);
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        dataSize = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (dataSize < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }

        fprintf(stdout, "Frame has %d channels and %d samples\n", dec_ctx->channels, frame->nb_samples);

        dec_buffer->insert(outBuffer->end(),
                         *frame->data[0],
                         *(frame->data[dec_ctx->channels - 1] + dataSize * (frame->nb_samples - 1)));
    }
}

void AudioDecoder::decode()  {

    audioFile = fopen(filename.c_str(), "rb");
    if(!audioFile) {
        fprintf(stderr, "Could not open file.");
        exit(-1);
    }

    data = inbuf;
    dataSize = fread(inbuf, 1, AUDIO_INBUF_SIZE, audioFile);

    decoded_frame = av_frame_alloc();
    if(!decoded_frame) {
        fprintf(stderr, "Could not allocate audio frame");
        exit(-1);
    }

    while(dataSize > 0) {
        ret = av_parser_parse2(parser, context, &pkt->data, &pkt->size, data, dataSize, AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE,
                               0);

        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }

        data += ret;
        dataSize -= ret;

        if(pkt->size) {
            decode(context, pkt, decoded_frame, outBuffer);
        }

        if(dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if(len > 0) dataSize += len;
        }
    }
}

AVCodec* AudioDecoder::findCodec() {
    codec = format->audio_codec;
    if (codec)
        return codec;
    codec = avcodec_find_decoder(format->audio_codec_id);
    if (codec)
        return codec;
    AVOutputFormat* fmt = av_guess_format(nullptr, //const char *short_name,
                                          filename.c_str(), nullptr); // const char *mime_type);;
    codec = fmt ? avcodec_find_decoder(fmt->audio_codec) : nullptr;
    if (codec)
        return codec;

    return nullptr;
}

AudioDecoder::AudioDecoder(const std::string &fileName) {
    filename = fileName;
    ret = 0;

    pkt = av_packet_alloc();
    if(!pkt) exit(-1);
    av_init_packet(pkt);

    codec = findCodec();

    parser = av_parser_init(codec->id);

    context = avcodec_alloc_context3(codec);
    if(avcodec_open2(context, codec, nullptr) < 0) {
        fprintf(stderr, "There was an error while initialising the AVCodexContext.");
        exit(-1);
    }

    decoded_frame = av_frame_alloc();
    if(!decoded_frame) {
        fprintf(stderr, "Could not allocate audio frame");
        exit(-1);
    }

    outBuffer = new std::vector<uint8_t>();
    inbuf = (uint8_t  *)malloc(AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE * sizeof(uint8_t));

    audioFile = nullptr;
    data = nullptr;
    dataSize = 0;
}
