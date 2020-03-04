#include "main.h"

void decode(AVCodecContext *context, AVPacket *packet, AVFrame *frame, std::vector<uint8_t> *outBuffer, FILE *outfile) {
    int ret, dataSize;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(context, packet);
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
        ret = avcodec_receive_frame(context, frame);
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

        fprintf(stdout, "Frame has %d channels and %d samples\n", context->channels, frame->nb_samples);

        int i, ch;
        for(i = 0; i < frame->nb_samples; i++) {
            for(ch = 0; ch < context->channels; ch++) {
                // outBuffer->push_back(*(frame->data[ch] + dataSize * i));
                fwrite(frame->data[ch] + dataSize*i, 1, dataSize, outfile);
            }
        }

        outBuffer->insert(outBuffer->end(),
                          *frame->data[0],
                          *(frame->data[context->channels - 1] + dataSize * (frame->nb_samples - 1)));
    }
}

int main(int argc, char **argv) {
    AVCodec *codec;
    AVCodecContext *context;
    FILE *audioFile;
    std::string filename;
    AVFrame *decoded_frame;
    AVCodecParserContext *parser;
    AVPacket *pkt;

    uint8_t  inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;

    size_t dataSize;
    int ret;
    auto outBuffer = std::vector<uint8_t>();

    filename = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/000/000002.mp3";

    pkt = av_packet_alloc();
    if(!pkt) exit(-1);
    av_init_packet(pkt);

    codec = avcodec_find_decoder(AV_CODEC_ID_MP3);

    parser = av_parser_init(codec->id);

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

    FILE *outfile = fopen("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/outputConv.RAW", "w++");

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
            decode(context, pkt, decoded_frame, &outBuffer, outfile);
        }

        if(dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if(len > 0) dataSize += len;
        }
    }

    printf("Finished decoding, size of the output buffer is %zu\n", outBuffer.size());

    pkt->data = nullptr;
    pkt->size = 0;
    decode(context, pkt, decoded_frame, &outBuffer, outfile);

    fclose(audioFile);
    fclose(outfile);

    avcodec_free_context(&context);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    return 0;
}