#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct room {
    int id;
    char* name;
    char* roomType;
    int connections;
    int connected[6];
};

struct room* createNewRoom(int newID, char* newName, char* newRoomType) {
    struct room* newRoom = malloc(sizeof(struct room));
    newRoom->id = newID;
    newRoom->name = newName;
    newRoom->connections = 0;
    newRoom->roomType = newRoomType;

    for (int i = 0; i < 6; i++) {
        newRoom->connected[i] = -1;
    }
    return newRoom;
}

char** createRoomNames() {
    /*
    Returns array of 10 room names, in a random order. To get 7 random room
    names, just pick the first 7 room names of the output array.
    */

    // Seed random number generator
    srand(time(NULL));

    // Hardcoded room names and room name order
    char** rooms = malloc(11 * sizeof(char*));
    char* room0 = "Red";
    char* room1 = "Green";
    char* room2 = "Yellow";
    char* room3 = "Blue";
    char* room4 = "White";
    char* room5 = "Black";
    char* room6 = "Purple";
    char* room7 = "Grey";
    char* room8 = "Magenta";
    char* room9 = "Party";
    rooms[0] = room0;
    rooms[1] = room1;
    rooms[2] = room2;
    rooms[3] = room3;
    rooms[4] = room4;
    rooms[5] = room5;
    rooms[6] = room6;
    rooms[7] = room7;
    rooms[8] = room8;
    rooms[9] = room9;
    rooms[10] = room0;          // Temporary location for swaps

    // Create new array to store output of randomized array order
    char** randRooms = malloc(10 * sizeof(char*));

    // For every element of randRooms, select a random index from rooms
    int i = 10;
    while (i > 0) {
        int curRand = rand() % i;
        randRooms[i - 1] = rooms[curRand];
        rooms[10] = rooms[curRand];
        // Need to move just-selected room to the end of the rooms array
        // This prevents the same room from being selected twice
        for (int j = curRand; j < 10; j++) {
            rooms[j] = rooms[j + 1];
        }
        i--;
    }

    free(rooms);
    return randRooms;
}

void freeRoomNames(char** rooms) {
    free(rooms);
}

int roomsConnectionsFull(struct room** rooms) {
    /*
    Returns 1 if all rooms in given array have between 3 and 6 connections,
    inclusive. Returns 0 otherwise
    */
    for (int i = 0; i < 7; i++) {
        if (rooms[i]->connections < 3 || rooms[i]->connections > 6) {
            return 0;
        }
    }
    return 1;
}

void addRoomConnection(struct room* room1, struct room* room2) {
    // Check to see if maximum number of room connections has been added
    if (room1->connections == 6 || room2->connections == 6) {
        return;
    }

    // Check to see if these rooms are already connected
    for (int i = 0; i < room1->connections; i++) {
        if (room1->connected[i] == room2->id) {
            return;
        }
    }
    for (int i = 0; i < room2->connections; i++) {
        if (room2->connected[i] == room1->id) {
            return;
        }
    }
    if (room1 == room2) {
        return;
    }

    // If these rooms can be connected, then connect them
    room1->connected[room1->connections] = room2->id;
    room2->connected[room2->connections] = room1->id;
    room1->connections++;
    room2->connections++;
}

void connectRooms(struct room** rooms) {
    while (roomsConnectionsFull(rooms) == 0) {
        addRoomConnection(rooms[rand() % 7], rooms[rand() % 7]);
    }
}

int main() {
    int curPID = getpid();

    char dirName[30] = "chenchar.rooms.";
    char curPIDChar[10];
    snprintf(curPIDChar, 10, "%d", curPID);

    // TODO: Need to check if successful completion of mkdir?
    /*
    strcat(dirName, curPIDChar);
    int newDir = mkdir(dirName, 0755);
    */

    char** roomNames = createRoomNames();
    for (int i = 0; i < 10; i++) {
        printf("%s\n", roomNames[i]);
    }

    struct room* rooms[7];
    for (int i = 0; i < 7; i++) {
        if (i == 0) {
            rooms[i] = createNewRoom(i, roomNames[i], "START_ROOM");
        }
        else if (i == 6) {
            rooms[i] = createNewRoom(i, roomNames[i], "END_ROOM");
        }
        else {
            rooms[i] = createNewRoom(i, roomNames[i], "MID_ROOM");
        }
    }

    connectRooms(rooms);
    for (int i = 0; i < 7; i++) {
        printf("ROOM NAME: %s\n", rooms[i]->name);
        for (int j = 1; j <= rooms[i]->connections; j++) {
            printf("CONNECTION %d: %s\n", j,
                rooms[rooms[i]->connected[j - 1]]->name);
        }
        printf("ROOM TYPE: %s\n", rooms[i]->roomType);
        printf("\n");
    }

    freeRoomNames(roomNames);
    return 0;
}
