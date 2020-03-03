#include "main.h"

int main(int argc, char **argv) {
    auto audioDecoder = AudioDecoder("MP3");
    audioDecoder.decode("/home/sebltm/OneDrive/Documents/Exeter/BSx_Dissertation/Sounds/fma_small/000/000002.mp3");
}

void AudioDecoder::decode() {

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

        outBuffer.insert(outBuffer.end(),
                *decoded_frame->data[0],
                *(decoded_frame->data[context->channels - 1] + dataSize * (decoded_frame->nb_samples - 1)));
    }
}

void AudioDecoder::decode(const std::string& fileToDecode) {
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
            decode();
        }

        if(dataSize < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, dataSize);
            data = inbuf;
            int len = fread(data + dataSize, 1, AUDIO_INBUF_SIZE - dataSize, audioFile);

            if(len > 0) dataSize += len;
        }
    }
}

AudioDecoder::AudioDecoder(const std::string &fileType) {
    ret = 0;

    pkt = av_packet_alloc();
    if(!pkt) exit(-1);
    av_init_packet(pkt);

    AVCodecID id = AV_CODEC_ID_NONE;
    if(fileType == "MP3") {
        id = AV_CODEC_ID_MP3;
    } else if(fileType == "wav") {
        id = AV_CODEC_ID_WAVPACK;
    }
    codec = avcodec_find_decoder(id);

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

    decoded_frame = av_frame_alloc();
    if(!decoded_frame) {
        fprintf(stderr, "Could not allocate audio frame");
        exit(-1);
    }

    outBuffer = std::vector<uint8_t>();
    inbuf = (uint8_t  *)malloc(AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE * sizeof(uint8_t));

    data = nullptr;
    dataSize = 0;
}
