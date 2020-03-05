//
// Created by sebltm on 03/03/2020.
//

#include "main.h"

auto DESIRED_FMT = AV_SAMPLE_FMT_FLT;

decodedFile AudioDecoderMP3(const std::string& filename, float*& outBuffer, const std::string& outfilePath) {

    AVCodec *codec = nullptr;
    AVCodecContext *context = nullptr;
    FILE *audioFile = nullptr, *outfile = nullptr;
    AVFrame *decoded_frame = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVFormatContext *format = nullptr;
    AVPacket *pkt = nullptr;
    AVStream *stream = nullptr;

    uint8_t *data = nullptr;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];

    auto *tempBuffer = new std::vector<float>();

    int dataSize = 0, ret = 0, samplerate = 0;

    pkt = av_packet_alloc();
    if(!pkt) exit(-1);
    av_init_packet(pkt);

    format = avformat_alloc_context();
    if(!format) {
        fprintf(stderr, "Could not allocate a format context.");
        exit(-1);
    }

    if(avformat_open_input(&format, filename.c_str(), nullptr, nullptr) != 0) {
        fprintf(stderr, "Could not open desired file.");
        exit(-1);
    }

    int stream_index = -1;
    for(int i = 0; i < format->nb_streams; i++) {
        if(format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        fprintf(stderr, "Could not retrieve audio stream from file '%s'\n", filename.c_str());
        exit(-1);
    }

    stream = format->streams[stream_index];
    if(avformat_find_stream_info(format, nullptr) < 0) {
        fprintf(stderr, "Could not find stream info.\n");
        exit(-1);
    }

    codec = avcodec_find_decoder(stream->codecpar->codec_id);
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

    if(!outfilePath.empty()) {
        outfile = fopen(outfilePath.c_str(), "w++");
        if (!outfile) {
            fprintf(stderr, "Could not open file.");
            exit(-1);
        }
    } else {
        outfile = nullptr;
    }

    data = inbuf;
    dataSize = fread(inbuf, 1, AUDIO_INBUF_SIZE, audioFile);

    decoded_frame = av_frame_alloc();
    if(!decoded_frame) {
        fprintf(stderr, "Could not allocate audio frame");
        exit(-1);
    }

    samplerate = 44100;
    SwrContext *swr = swr_alloc_set_opts(
            nullptr,
            AV_CH_LAYOUT_MONO,
            DESIRED_FMT,
            samplerate,
            stream->codecpar->channel_layout,
            AVSampleFormat(stream->codecpar->format),
            stream->codecpar->sample_rate,
            0,
            nullptr);

    swr_init(swr);
    if(!swr_is_initialized(swr)) {
        fprintf(stderr, "Resampler has not been properly initialized\n");
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
            decode(context, pkt, decoded_frame, swr, tempBuffer, outfile);
        }

        if(dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if(len > 0) dataSize += len;
        }
    }

    outBuffer = (float *)malloc(tempBuffer->size() * sizeof(float));
    for(int i = 0; i < tempBuffer->size(); i++) {
        outBuffer[i] = (*tempBuffer)[i];
    }

    swr_free(&swr);
    pkt->data = nullptr;
    pkt->size = 0;
    decode(context, pkt, decoded_frame, swr, tempBuffer, outfile);

    auto file = decodedFile();
    file.num_samples = tempBuffer->size();
    file.samplerate = stream->codecpar->sample_rate;

    fclose(audioFile);

    if(outfile) {
        fclose(outfile);
    }

    avcodec_free_context(&context);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    return file;
}

void decode(AVCodecContext *context, AVPacket *pkt, AVFrame *frame, SwrContext  *swr, std::vector<float> *buffer, FILE
*outfile) {
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
        ret = avcodec_receive_frame(context, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        dataSize = av_get_bytes_per_sample(context->sample_fmt);
        if (dataSize < 0) {
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }

        float *resampBuffer = nullptr;
        av_samples_alloc((uint8_t **) &resampBuffer, frame->linesize, context->channels, frame->nb_samples,
                         DESIRED_FMT, 0);
        swr_convert(swr, (uint8_t **) &resampBuffer, frame->nb_samples, (const uint8_t **) frame->data,
                                                       frame->nb_samples);

        int i;
        if(outfile) {
            for(i = 0; i < frame->nb_samples; i++) {
                fwrite(&(resampBuffer[i]), 1, sizeof(float), outfile);
                buffer->push_back(resampBuffer[i]);
            }
        }

    }
}