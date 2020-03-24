//
// Created by sebltm on 04/03/2020.
//

#include "main.h"

decodedFile AudioDecoderWAV(const std::string& filename, double*& outBuffer, const std::string& outfilePath) {
    auto *sfInfo = new SF_INFO();

    SNDFILE *sndfile = sf_open(filename.c_str(), SFM_READ, sfInfo);
    if(!sndfile) {
        std::cerr << "Could not open the request file." << std::endl;
        exit(-1);
    }

    outBuffer = (double *) malloc(sfInfo->frames * sfInfo->channels * sizeof(*outBuffer));

    long read = sf_read_double(sndfile, outBuffer, sfInfo->channels * sfInfo->frames);
    if (read < sfInfo->channels * sfInfo->frames) {
        std::cout << "Did not read all elements, read " << read << " of " << sfInfo->channels * sfInfo->frames
                  << "." << std::endl;
    } else {
        std::cout << "Read entire file : " << read << " samples." << std::endl;
    }

    SRC_DATA srcData;
    srcData.data_in = (float *) outBuffer;
    srcData.input_frames = sfInfo->frames;
    srcData.data_out = (float *) outBuffer;
    srcData.src_ratio = 44100.0 / sfInfo->samplerate;

    resample(&srcData, sfInfo->channels);

    if (!outfilePath.empty()) {
        FILE *outfile = fopen(outfilePath.c_str(), "w++");
        if (!outfile) {
            std::cerr << "Could not open file : " << filename << ", " << strerror(errno) << std::endl;
            exit(-1);
        }

        for (int i = 0; i < sfInfo->frames; i++) {
            for (int ch = 0; ch < sfInfo->channels; ch++) {
                fwrite(&outBuffer[i * sfInfo->channels + ch], sizeof(double), 1, outfile);
            }
        }

        fclose(outfile);
    }

    auto file = decodedFile();
    file.samplerate = sfInfo->samplerate;
    file.num_samples = sfInfo->frames;
    file.channels = sfInfo->channels;

    sf_close(sndfile);
    delete (sfInfo);
    return file;
}

void resample(SRC_DATA *srcData, int channels) {
    int error;

    SRC_STATE *srcState = src_new(SRC_SINC_BEST_QUALITY, channels, &error);
    if (srcState == nullptr) {
        std::cerr << "Could not start resample." << std::endl;
        exit(-1);
    }

    src_process(srcState, srcData);
    src_delete(srcState);
}