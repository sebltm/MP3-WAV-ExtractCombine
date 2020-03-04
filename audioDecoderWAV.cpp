//
// Created by sebltm on 04/03/2020.
//

#include "main.h"

int AudioDecoderWAV(const std::string& filename) {
    auto *sfInfo = new SF_INFO();

    SNDFILE *sndfile = sf_open(filename.c_str(), SFM_READ, sfInfo);
    if(!sndfile) {
        fprintf(stderr, "Could not open the request file.\n");
        exit(-1);
    }

    float outBuffer[sfInfo->frames * sfInfo->channels];
    long read = sf_read_float(sndfile, outBuffer, sfInfo->channels * sfInfo->frames);
    if(read < sfInfo->channels * sfInfo->frames) {
        fprintf(stdout, "Did not read all elements, read %ld of %ld", read, sfInfo->channels * sfInfo->frames);
    } else {
        fprintf(stdout, "Read entire file, %ld samples", read);
    }

    FILE *outfile = fopen("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/outputConvWAV.RAW", "w++");
    if(!outfile) {
        fprintf(stderr, "Could not open output file.");
        exit(-1);
    }

    for(int i = 0; i < sfInfo->frames; i++) {
        for(int ch = 0; ch < sfInfo->channels; ch++) {
            fwrite(&outBuffer[i * sfInfo->channels + ch], sizeof(float), 1, outfile);
        }
    }

    fclose(outfile);
    sf_close(sndfile);
    return 0;
}