//
// Created by sebltm on 03/03/2020.
//

#include "main.h"

auto DESIRED_FMT = AV_SAMPLE_FMT_DBL;

decodedFile AudioDecoderMP3(const std::string &filename, double *&outBuffer, const std::string &outfilePath) {

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

    auto *tempBuffer = new std::vector<double>();

    int dataSize = 0, ret = 0, samplerate = 0;

    pkt = av_packet_alloc();
    if (!pkt) exit(-1);
    av_init_packet(pkt);

    format = avformat_alloc_context();
    if (!format) {
        std::cerr << "Could not allocate a format context." << std::endl;
        exit(-1);
    }

    if (avformat_open_input(&format, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open file : " << filename << std::endl;

        decodedFile empty = decodedFile();
        empty.num_samples = 0;

        avformat_free_context(format);
        av_packet_free(&pkt);

        return empty;
    }

    int stream_index = -1;
    for (int i = 0; i < format->nb_streams; i++) {
        if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        std::cerr << "Could not retrieve audio stream from file : " << filename.c_str() << std::endl;

        avformat_free_context(format);
        av_packet_free(&pkt);

        exit(-1);
    }

    stream = format->streams[stream_index];
    if (avformat_find_stream_info(format, nullptr) < 0) {
        std::cerr << "Could not find stream info." << std::endl;

        avformat_free_context(format);
        av_packet_free(&pkt);

        exit(-1);
    }

    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Could not find a codec." << std::endl;

        avformat_free_context(format);
        av_packet_free(&pkt);

        exit(-1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        std::cerr << "Could not find a parser for the codec." << std::endl;

        avformat_free_context(format);
        av_packet_free(&pkt);

        exit(-1);
    }

    context = avcodec_alloc_context3(codec);
    if (avcodec_open2(context, codec, nullptr) < 0) {
        std::cerr << "There was an error while initialising the AVCodexContext." << std::endl;

        avformat_free_context(format);
        av_parser_close(parser);
        av_packet_free(&pkt);

        exit(-1);
    }

    std::cout << "Music file is : " << filename << "." << std::endl;
    audioFile = fopen(filename.c_str(), "rb");
    if (!audioFile) {
        std::cerr << "Could not open file : " << filename << ", " << strerror(errno) << std::endl;

        avformat_free_context(format);
        avcodec_free_context(&context);
        av_parser_close(parser);
        av_packet_free(&pkt);

        exit(-1);
    }

    std::cout << "Noise file is : " << outfilePath << "." << std::endl;
    if (!outfilePath.empty()) {
        outfile = fopen(outfilePath.c_str(), "w++");
        if (!outfile) {
            std::cerr << "Could not open file : " << outfilePath << ", " << strerror(errno) << std::endl;

            fclose(audioFile);
            avformat_free_context(format);
            avcodec_free_context(&context);
            av_parser_close(parser);
            av_frame_free(&decoded_frame);
            av_packet_free(&pkt);

            exit(-1);
        }
    } else {
        outfile = nullptr;
    }

    data = inbuf;
    dataSize = fread(inbuf, 1, AUDIO_INBUF_SIZE, audioFile);

    decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        std::cerr << "Could not allocate audio frame." << std::endl;
        exit(-1);
    }

    samplerate = 44100;
    SwrContext *swr = swr_alloc_set_opts(
            nullptr,
            AV_CH_LAYOUT_MONO,
            AV_SAMPLE_FMT_DBL,
            44100,
            stream->codecpar->channel_layout,
            AVSampleFormat(stream->codecpar->format),
            stream->codecpar->sample_rate,
            0,
            nullptr);

    swr_init(swr);
    if (!swr_is_initialized(swr)) {
        std::cerr << "Resampler has not been properly initialized." << std::endl;
        exit(-1);
    }

    while (dataSize > 0) {
        ret = av_parser_parse2(parser, context, &pkt->data, &pkt->size, data, dataSize, AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE,
                               0);

        if (ret < 0) {
            std::cerr << "Error while parsing." << std::endl;
            exit(1);
        }

        data += ret;
        dataSize -= ret;

        if (pkt->size) {
            decode(context, pkt, decoded_frame, swr, tempBuffer, outfile);
        }

        if (dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if (len > 0) dataSize += len;
        }
    }

    outBuffer = (double *) malloc(tempBuffer->size() * sizeof(double));
    for (int i = 0; i < tempBuffer->size(); i++) {
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

    if (outfile) {
        fclose(outfile);
    }

    delete(tempBuffer);
    avformat_close_input(&format);
    avformat_free_context(format);
    avcodec_free_context(&context);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    return file;
}

void decode(AVCodecContext *context, AVPacket *pkt, AVFrame *frame, SwrContext *swr, std::vector<double> *buffer, FILE
*outfile) {
    int ret, dataSize;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(context, pkt);
    if (ret == AVERROR(EAGAIN)) {
        std::cerr << "Error while submitting the packet to the decoder: input is not accepted in the current state" << std::endl;
        // exit(-1);
    } else if (ret == AVERROR_EOF) {
        std::cerr << "Error while submitting the packet to the decoder: decoder has been flushed" << std::endl;
        // exit(-1);
    } else if (ret == AVERROR(EINVAL)) {
        std::cerr << "Error while submitting the packet to the decoder: coded not opened, it is an encoder" << std::endl;
        // exit(-1);
    } else if (ret == AVERROR(ENOMEM)) {
        std::cerr << "Error while submitting the packet to the decoder: failed to add packet to queue - decoding "
                        "error" << std::endl;
        // exit(-1);
    } else if (ret < 0) {
        std::cerr << "Error while submitting the packet to the decoder: %d" << ret << std::endl;
        // exit(-1);
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(context, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            std::cerr << "Error during decoding." << std::endl;
            exit(1);
        }
        dataSize = av_get_bytes_per_sample(context->sample_fmt);
        if (dataSize < 0) {
            std::cerr << "Failed to calculate data size." << std::endl;
            exit(1);
        }

        double *resampBuffer = nullptr;
        av_samples_alloc((uint8_t **) &resampBuffer, frame->linesize, context->channels, frame->nb_samples,
                         DESIRED_FMT, 0);
        swr_convert(swr, (uint8_t **) &resampBuffer, frame->nb_samples, (const uint8_t **) frame->data,
                    frame->nb_samples);

        int i = 0;
        for (i = 0; i < frame->nb_samples; i++) {

            if (outfile) {
                fwrite(&(resampBuffer[i]), 1, sizeof(double), outfile);
            }
            buffer->push_back(resampBuffer[i]);

        }

        av_free(resampBuffer);
    }
}