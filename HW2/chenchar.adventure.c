#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

struct room {
    char name[16];
    char roomType[16];
    int connections;
    char connected[6][32];
};

struct room* createNewRoom(char* newName, char* newRoomType) {
    struct room* newRoom = malloc(sizeof(struct room));

    memset(newRoom->name, '\0', sizeof(newRoom->name));
    strcpy(newRoom->name, newName);

    newRoom->connections = 0;

    memset(newRoom->roomType, '\0', sizeof(newRoom->roomType));
    strcpy(newRoom->roomType, newRoomType);

    int i = 0;
    for (i = 0; i < 6; i++) {
        memset(newRoom->connected[i], '\0', sizeof(newRoom->connected[i]));
        strcpy(newRoom->connected[i], "None");
    }
    return newRoom;
}

void addRoomConnection(struct room* room1, char* roomConnection) {
    strcpy(room1->connected[room1->connections], roomConnection);
    room1->connections++;
}

void setRoomType(struct room* room1, char* newRoomType) {
    memset(room1->roomType, '\0', sizeof(room1->roomType));
    strcpy(room1->roomType, newRoomType);
}

int findRoomInd(struct room** rooms, char* roomName) {
    // Given a room name, find the index it occupies in rooms array
    int i;
    for (i = 0; i < 7; i++) {
        if (strcmp(rooms[i]->name, roomName) == 0) {
            return i;
        }
    }

    // Returns -1 if not found
    return -1;
}

void printCurRoom(struct room** rooms, int curRoom) {
    printf("CURRENT LOCATION: %s\n", rooms[curRoom]->name);
    printf("POSSIBLE CONNECTIONS: ");
    int i = 0;
    for (i = 0; i < rooms[curRoom]->connections; i++) {
        printf("%s", rooms[curRoom]->connected[i]);
        if (i != (rooms[curRoom]->connections - 1)) {
            printf(", ");
        }
    }
    printf(".\n");
}

void getUserInput(struct room** rooms, int *curRoom) {
    size_t bufferSize = 0;
    char* userInput = NULL;
    char connectedRooms[128];
    int charsEntered;

    memset(connectedRooms, '\0', sizeof(connectedRooms));
    strcpy(connectedRooms, rooms[*curRoom]->connected[0]);

    int i;
    for (i = 1; i < rooms[*curRoom]->connections; i++) {
        strcat(connectedRooms, " ");
        strcat(connectedRooms, rooms[*curRoom]->connected[i]);
    }

    while(1) {
        printf("WHERE TO? >");
        charsEntered = getline(&userInput, &bufferSize, stdin);
        userInput[charsEntered - 1] = '\0';
        if ((strstr(connectedRooms, userInput) == NULL) &&
            (strcmp(userInput, "time") != 0)) {
            printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
            free(userInput);
            bufferSize = 0;
            userInput = NULL;
        }
        else if (strcmp(userInput, "time") == 0) {
            // Do time stuff
            free(userInput);
            return;
        }
        else {
            // Move player to the specified room
            *curRoom = findRoomInd(rooms, userInput);
            free(userInput);
            return;
        }
    }
}

int main() {
    DIR* rootDir = opendir(".");

    if (rootDir < 0) {
        fprintf(stderr, "Could not open directory\n");
        perror("Error opening directory");
        exit(1);
    }

    // Finds the directory that was most recently modified
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

    DIR* newestDir = opendir(newestDirName);

    struct room* rooms[7];
    FILE* curRoomFile;
    ssize_t nread;
    char readBuffer[512];
    char filenameBuffer[128];
    int i = 0;

    char curRoomName[32];
    char curConnectionName[32];
    char curRoomType[32];

    // Read in all files in rooms directory
    while ((curFile = readdir(newestDir)) != NULL) {
        // Opens the file if it is a rooms file
        if (curFile->d_type == DT_REG) {
            memset(readBuffer, '\0', sizeof(readBuffer));
            memset(filenameBuffer, '\0', sizeof(filenameBuffer));
            memset(curRoomName, '\0', sizeof(curRoomName));
            memset(curConnectionName, '\0', sizeof(curConnectionName));
            memset(curRoomType, '\0', sizeof(curRoomType));

            strcpy(filenameBuffer, newestDirName);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, curFile->d_name);
            curRoomFile = fopen(filenameBuffer, "r");
            if (!curRoomFile) {
                fprintf(stderr, "Could not open room file\n");
                perror("Error opening file");
                exit(1);
            }

            // Get roomName and strip trailing newline
            fgets(readBuffer, sizeof(readBuffer), curRoomFile);
            readBuffer[strlen(readBuffer) - 1] = '\0';
            strcpy(curRoomName, &readBuffer[11]);

            rooms[i] = createNewRoom(curRoomName, "MID_ROOM");

            // Get connections and room type
            while (fgets(readBuffer, sizeof(readBuffer), curRoomFile) != NULL) {
                if (strstr(readBuffer, "CONNECTION") != NULL) {
                    readBuffer[strlen(readBuffer) - 1] = '\0';
                    strcpy(curConnectionName, &readBuffer[14]);
                    addRoomConnection(rooms[i], curConnectionName);
                }
                else {
                    readBuffer[strlen(readBuffer) - 1] = '\0';
                    strcpy(curRoomType, &readBuffer[11]);
                    setRoomType(rooms[i], curRoomType);
                }
            }

            // Close current file
            fclose(curRoomFile);
            i++;
        }
    }
    closedir(newestDir);

    // Start the adventure game
    int curRoom = 0;
    while (strcmp(rooms[curRoom]->roomType, "END_ROOM") != 0) {
        printCurRoom(rooms, curRoom);
        getUserInput(rooms, &curRoom);
    }

    return 0;
}
