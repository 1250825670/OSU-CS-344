#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

int main() {
    DIR* rootDir = opendir(".");

    if (rootDir < 0) {
        fprintf(stderr, "Could not open directory\n");
        perror("Error opening directory");
        exit(1);
    }

    struct dirent *curFile;
    char newestDirName[256];
    memset(newestDirName, '\0', sizeof(newestDirName));
    struct stat fileStats;
    long int modifiedTime = -1;
    int dirExists = 0;
    while ((curFile = readdir(rootDir)) != NULL) {
        // Search for directories with chenchar.rooms prefix
        if (strstr(curFile->d_name, "chenchar.rooms.") != NULL) {
            dirExists = 1;
            stat(curFile->d_name, &fileStats);
            if ((long int)(fileStats.st_mtime) > modifiedTime) {
                // Finds most recently modified rooms directory
                modifiedTime = (long int)fileStats.st_mtime;
                memset(newestDirName, '\0', sizeof(newestDirName));
                strcpy(newestDirName, curFile->d_name);
            }
        }
    }
    closedir(rootDir);

    if (dirExists == 0) {
        fprintf(stderr, "No rooms directories found\n");
        perror("No rooms directories found");
        exit(1);
    }

    printf("%s\n", newestDirName);

    DIR* newestDir = opendir(newestDirName);

    // Read in all files in directory
    while ((curFile = readdir(newestDir)) != NULL) {
        printf("%s\n", curFile->d_name);
        if (curFile->d_type == DT_REG) {
            printf("This is a file\n");
        }
    }
    closedir(newestDir);

    return 0;
}
