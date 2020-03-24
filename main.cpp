#include "main.h"

int main(int argc, char **argv) {
    const int maxNoises = 50, maxMusic = 20, desiredDuration = 57330;
    int noiseOffset = 0, musicOffset = 0;

    auto **noises = (double **) malloc(maxNoises * sizeof(double *));
    if (!noises) {
        std::cerr << "Could not allocate memory for noise tracks" << std::endl;
        exit(-1);
    }

//    auto **musics = (double **) malloc(maxMusic * sizeof(double *));
//    if(!musics) {
//        std::cerr << "Could not allocate memory for music tracks" << std::endl;
//        free(noises);
//        exit(-1);
//    }

    auto musics = std::vector<double *>();

    std::string pathNoise("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/UrbanSound8K/audio/");
    std::string extNoise(".wav");

    std::string pathMusic("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/fma_small/");
    std::string extMusic(".mp3");

    std::vector<std::string> noiseFiles = SortFiles(pathNoise, extNoise);
    std::vector<std::string> musicFiles = SortFiles(pathMusic, extMusic);

    std::cout << std::to_string(musicFiles.size()) << " music files" << std::endl;

    decodedFile wavfile{};
    decodedFile mp3file{};

    for (int fold = 0; fold < 10; fold++) {

        int noiseNum = 0, musicNum = 0;
        std::string folder("/home/sebltm/OneDrive/Documents/Exeter/BSc_Dissertation/Sounds/Processed/fold" +
                           std::to_string(fold) + "/");

        std::string metadata(folder + "metadata.txt");

        if (mkdir(folder.c_str(), 0777) == -1) {
            std::cerr << "Error : " << strerror(errno) << std::endl;
        }

        FILE *noiseNames = fopen(metadata.c_str(), "w++");
        if (!noiseNames) {
            fprintf(stderr, "Could not open the metadata file: %s\n", metadata.c_str());
        }

        // Load all the files in the directory
        while (noiseNum < maxNoises) {

            printf("Processing sound %d of %d\n", noiseNum + 1, maxNoises);

            double *noiseBuffer = nullptr;
            double *formattedNoise = nullptr;
            const std::string &fileInWAV = noiseFiles[fold * maxNoises + noiseNum + noiseOffset];
            std::string fileOutWAV;

            wavfile = AudioDecoderWAV(fileInWAV, noiseBuffer, fileOutWAV);
            std::cout << "Length of noise sample : " << std::to_string(wavfile.num_samples / wavfile.channels) <<
                      std::endl;

            if ((wavfile.num_samples / wavfile.channels) < desiredDuration) {
                free(noiseBuffer);
                noiseOffset++;
                continue;
            }

            formattedNoise = MonoAndShorten(noiseBuffer, wavfile.channels, desiredDuration);

            fprintf(noiseNames, "%s\n", fileInWAV.c_str());
            noises[noiseNum] = formattedNoise;

            noiseNum++;
        }

        fclose(noiseNames);

        // Load all the files in the directory
        while (musicNum < maxMusic) {
            printf("Music num : %d\n", fold * maxMusic + musicNum);

            // Extract the PCM data
            double *musicBuffer = nullptr;
            const std::string &fileInMP3 = musicFiles[fold * maxMusic + musicNum + musicOffset];
            std::string fileOutMP3;
            mp3file = AudioDecoderMP3(fileInMP3, musicBuffer, fileOutMP3);

            if (mp3file.num_samples < desiredDuration) {
                free(musicBuffer);
                musicOffset++;
                continue;
            }

            musics.push_back(musicBuffer);

            musicNum++;
        }

        int currentNum = 0;
        for (noiseNum = 0; noiseNum < maxNoises; noiseNum++) {
            for (musicNum = 0; musicNum < maxMusic; musicNum++) {

                auto *combinedTracks = (double *) malloc(desiredDuration * sizeof(double));
                if (!combinedTracks) {
                    std::cerr << "Could not allocate memory to combine tracks : " << strerror(errno) << std::endl;

                    goto end;
                }

                std::string outPath(folder + std::to_string(currentNum) + ".RAW");

                FILE *outfile = fopen(outPath.c_str(), "w++");
                if (!outfile) {
                    std::cerr << "Could not open file : " << outPath << ", " << strerror(errno) << std::endl;
                    exit(-1);
                }

                for (int durationIndex = 0; durationIndex < desiredDuration; durationIndex++) {

                    combinedTracks[durationIndex] =
                            (musics[musicNum][durationIndex] + noises[noiseNum][durationIndex]) / 2.0;

                    fwrite(&(combinedTracks[durationIndex]), 1, sizeof(double), outfile);
                }

                fflush(outfile);
                fclose(outfile);

                free(combinedTracks);
                currentNum++;
            }
        }
    }

    end:
    for (int i = 0; i < maxNoises; i++) {
        free(noises[i]);
    }
    free(noises);

    for (int i = 0; i < maxMusic; i++) {
        free(musics[i]);
    }

}

double *MonoAndShorten(double *buffer, int channels, int duration) {
    int i = 0, ch = 0;

    auto *newbuffer = (double *) malloc(duration * sizeof(double));

    for (i = 0; i < duration; i++) {
        for (ch = 0; ch < channels; ch++) {
            newbuffer[i] += buffer[i + ch];
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