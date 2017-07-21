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
    char newestDir[256];
    memset(newestDir, '\0', sizeof(newestDir));
    struct stat fileStats;
    long int modifiedTime = -1;
    int dirExists = 0;
    while ((curFile = readdir(rootDir)) != NULL) {
        // Search for directories with chenchar.rooms prefix
        if (strstr(curFile->d_name, "chenchar.rooms.") != NULL) {
            dirExists = 1;
            stat(curFile->d_name, &fileStats);
            if ((long int)(fileStats.st_mtime) > modifiedTime) {
                modifiedTime = (long int)fileStats.st_mtime;
                memset(newestDir, '\0', sizeof(newestDir));
                strcpy(newestDir, curFile->d_name);
            }
        }
    }
    closedir(rootDir);

    if (dirExists == 0) {
        fprintf(stderr, "No rooms directories found\n");
        perror("No rooms directories found");
        exit(1);
    }

    // Find most recently created files

    printf("%s\n", newestDir);

    // Read in all files

    return 0;
}
