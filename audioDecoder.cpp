//
// Created by sebltm on 03/03/2020.
//

#include "main.h"

void AudioDecoder::decodePacket() {
    int ret, dataSize;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(context, pkt);
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
        ret = avcodec_receive_frame(context, decoded_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        dataSize = av_get_bytes_per_sample(context->sample_fmt);
        if (dataSize < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }

        fprintf(stdout, "Frame has %d channels and %d samples\n", context->channels, decoded_frame->nb_samples);

        int i, ch;

        for(i = 0; i < decoded_frame->nb_samples; i++) {
            for(ch = 0; ch < context->channels; ch++) {
                // outBuffer->push_back(*(frame->data[ch] + dataSize * i));
                fwrite(decoded_frame->data[ch] + dataSize * i, 1, dataSize, outfile);
            }
        }

        outBuffer.insert(outBuffer.end(),
                          *decoded_frame->data[0],
                          *(decoded_frame->data[context->channels - 1] + dataSize * (decoded_frame->nb_samples - 1)));
    }
}

AudioDecoder::AudioDecoder(const std::string& filename) : filename(filename) {

    inbuf = (uint8_t *)malloc((AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE) * sizeof(uint8_t));
    outBuffer = std::vector<uint8_t>();

    pkt = av_packet_alloc();
    if(!pkt) exit(-1);
    av_init_packet(pkt);

    format = avformat_alloc_context();
    if(!format) {
        fprintf(stderr, "Could not find a context for AVFormat.\n");
        exit(-1);
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    if(!codec) {
        fprintf(stderr, "Could not find a codec.\n");
        exit(-1);
    }

    parser = av_parser_init(codec->id);
    if(!parser) {
        fprintf(stderr, "Could not find a parser for the codec.\n");
        exit(-1);
    }

    context = avcodec_alloc_context3(codec);
    if(avcodec_open2(context, codec, nullptr) < 0) {
        fprintf(stderr, "There was an error while initialising the AVCodexContext.");
        exit(-1);
    }

    audioFile = fopen(filename.c_str(), "rb");
    if(!audioFile) {
        fprintf(stderr, "Could not open file.");
        exit(-1);
    }

    outfile = fopen("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/outputConv.RAW", "w++");
    if(!outfile) {
        fprintf(stderr, "Could not open file.");
        exit(-1);
    }
}

AudioDecoder::~AudioDecoder() {
    pkt->data = nullptr;
    pkt->size = 0;
    decodePacket();

    fclose(audioFile);
    fclose(outfile);

    avcodec_free_context(&context);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);
}

void AudioDecoder::decode() {
    int dataSize, ret;

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
            decodePacket();
        }

        if(dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if(len > 0) dataSize += len;
        }
    }

    printf("Finished decoding, size of the output buffer is %zu\n", outBuffer.size());
}

AVCodec *AudioDecoder::findCodec() {
    codec = format->audio_codec;
    if (codec)
        return codec;
    codec = avcodec_find_decoder(format->audio_codec_id);
    if (codec)
        return codec;
    AVOutputFormat* fmt = av_guess_format(nullptr, filename.c_str(), nullptr); // const char *mime_type);;
    codec = fmt ? avcodec_find_decoder(fmt->audio_codec) : nullptr;
    if (codec)
        return codec;
    return nullptr;
}
