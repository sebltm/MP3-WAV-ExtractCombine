#include <random>
#include "main.h"

int main(int argc, char** argv) {

    int maxSound = 1, maxDuration = 57330;
    auto **noises = (double **)malloc(maxSound * sizeof(double *));

    std::string pathNoise("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/UrbanSound8K/audio/");
    std::string extNoise(".wav");

    FILE *noiseNames = fopen("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/order.txt", "w++");

    std::vector<std::string> noiseFiles = SortFiles(pathNoise, extNoise);
    decodedFile wavfile = decodedFile();

    int soundNum = 0;
    // Load all the files in the directory
    for(auto & noiseFile : noiseFiles) {
        wavfile.num_samples = 0;
        wavfile.samplerate = 44100;

        if (soundNum == maxSound) {
            break;
        }

        printf("Processing sound %d of %d\n", soundNum+1, maxSound);

        double *noiseBuffer = nullptr;
        const std::string& fileInWAV = noiseFile;
        std::string fileOutWAV;

        while( ( (float)wavfile.num_samples / (float)wavfile.samplerate ) < 1.30) {
            wavfile = AudioDecoderWAV(fileInWAV, noiseBuffer, fileOutWAV);
            printf("%f\n", (float)wavfile.num_samples / (float)wavfile.samplerate);
        }

        double *formattedNoise = MonoAndShorten(noiseBuffer, 2, 44100, maxDuration);
        fprintf(noiseNames, "%s\n", noiseFile.c_str());
        noises[soundNum] = formattedNoise;
        soundNum++;
    }

    fclose(noiseNames);

    std::string pathMusic("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/");
    std::string extMusic(".mp3");

    std::vector<std::string> musicFiles = SortFiles(pathMusic, extMusic);

    auto *combinedTracks = (double *) malloc(maxDuration * sizeof(double));
    if (!combinedTracks) {
        std::cerr << "Could not allocate memory to combine tracks : " << strerror(errno) << std::endl;

        for(int i = 0; i < maxSound; i++) {
            free(noises[i]);
        }
        free(noises);

        exit(-1);
    }

    int musicNum = 0;
    // Load all the files in the directory
    for(const auto &musicFile : musicFiles) {
        printf("Music num : %d\n", musicNum);

        if(musicNum == 1) {
            free(combinedTracks);

            for(int i = 0; i < maxSound; i++) {
                free(noises[i]);
            }
            free(noises);

            exit(0);
        }

        std::string folder("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/Processed/" +
                           std::to_string(musicNum));

        if (mkdir(folder.c_str(), 0777) == -1) {
            std::cerr << "Error : " << strerror(errno) << std::endl;
        }

        // Extract the PCM data
        double *musicBuffer = nullptr;
        const std::string& fileInMP3 = musicFile;
        std::string fileOutMP3(folder + "/ogMusic.RAW");
        decodedFile mp3file = AudioDecoderMP3(fileInMP3, musicBuffer, fileOutMP3);
        if (mp3file.num_samples == 0) {
            free(musicBuffer);
            continue;
        }

        for (int currentNoise = 0; currentNoise < maxSound; currentNoise++) {

            std::string outPath(folder + "/" + std::to_string(currentNoise) + ".RAW");

            FILE *outfile = fopen(outPath.c_str(), "w++");
            if (!outfile) {
                std::cerr << "Could not open file : " << outPath << ", " << strerror(errno) << std::endl;
            }

            for (int i = 0; i < maxDuration; i++) {
                combinedTracks[i] = (musicBuffer[i] + noises[currentNoise][i]) / 2.0f;
                fwrite(&(combinedTracks[i]), 1, sizeof(double), outfile);
            }

            fclose(outfile);
        }

        free(musicBuffer);
        musicNum++;
    }

    free(combinedTracks);

    for(int i = 0; i < maxSound; i++) {
        free(noises[i]);
    }
    free(noises);

}

double * MonoAndShorten(double *buffer, int channels, int duration) {
    int i = 0, srSeek = 0, ch = 0;

    auto *newbuffer = (double *)malloc(duration * sizeof(double));

    for(i = 0; i < duration; i++) {
        for(ch = 0; ch < channels; ch++) {
            newbuffer[i + srSeek] += buffer[i + srSeek + ch];
        }
    }

    free(buffer);
    return newbuffer;
}

std::vector<std::string> SortFiles(const std::string &path, const std::string &extension) {

    std::vector<std::string> fileList = std::vector<std::string>();

    for (const auto &p : std::filesystem::recursive_directory_iterator(path)) {

        if (p.path().extension() == extension) {
            fileList.push_back(p.path().string());
        }
    }

    std::shuffle(fileList.begin(), fileList.end(), std::mt19937(std::random_device()()));
    return fileList;
}