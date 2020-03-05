#include "main.h"

int main(int argc, char** argv) {

    float *musicBuffer = nullptr;
    std::string fileInMP3 = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/000/000002.mp3";
    std::string fileOutMP3 = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/outfileMusic.RAW";
    auto mp3file = AudioDecoderMP3(fileInMP3, musicBuffer, fileOutMP3);
    auto durationmp3 = (float)mp3file.num_samples / mp3file.samplerate;

    float *noiseBuffer = nullptr;
    std::string fileInWAV = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/UrbanSound8K/audio/fold1/7061-6-0-0.wav";
    std::string fileOutWAV = "/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/outfileNoise.RAW";
    auto wwavfile = AudioDecoderWAV(fileInWAV, noiseBuffer, fileOutWAV);
    auto durationwav = (float)wwavfile.num_samples / wwavfile.samplerate;

    printf("Duration of music : %f, duration of noise : %f", durationmp3, durationwav);

    int minduration = (int)fmin(durationmp3, durationwav);

    float *formattedNoise = MonoAndShorten(noiseBuffer, 2, 44100, minduration);
    auto *combinedTracks = (float *)malloc(wwavfile.samplerate * minduration * sizeof(float));

    FILE *outfile = fopen("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/outfileCombined.RAW", "w++");
    for(int i = 0; i < minduration * mp3file.samplerate; i++) {
        combinedTracks[i] = (musicBuffer[i] + formattedNoise[i]) / 2.0f;
        fwrite(&(combinedTracks[i]), 1, sizeof(float), outfile);
    }

    fclose(outfile);
    free(combinedTracks);
    free(formattedNoise);
    free(musicBuffer);
}

float * MonoAndShorten(float *buffer, int channels, int sampleRate, int duration) {
    int i = 0, srSeek = 0, ch = 0;

    auto *newbuffer = (float *)malloc(duration * sampleRate * sizeof(float));

    for(i = 0; i < duration; i++) {
        for(srSeek = 0; srSeek < sampleRate; srSeek++) {
            newbuffer[i + srSeek + ch] = 0;
            for(ch = 0; ch < channels; ch++) {
                newbuffer[i + srSeek] += buffer[i + srSeek + ch];
            }
        }
    }

    free(buffer);
    return newbuffer;
}