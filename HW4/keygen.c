#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

// Prints a usage statement
void print_usage() {
    printf("Usage:\nkeygen keylength\n");
    printf("keylength is an int representing the length of the requested key\n");
}

int main(int argc, char** argv) {
    // Check that keygen is called with the appropriate argument count (1)
    if (argc != 2) {
        fprintf(stderr, "%s", "Error: keygen should be used with only one argument\n");
        print_usage();
        exit(1);
    }

    // Check that parameter given is a valid integer
    int i;
    for (i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            fprintf(stderr, "%s", "Error: parameter should be an integer\n");
            print_usage();
            exit(1);
        }
    }

    // Converts input from char to int
    int lenOfKey = atoi(argv[1]);

    // Ensure that key length is valid
    if (lenOfKey <= 0) {
        fprintf(stderr, "%s", "Error: parameter should be a positive integer\n");
        print_usage();
        exit(1);
    }

    // Seed pseudo-random number generator
    srand(time(NULL));

    char* key = malloc(sizeof(key) * (lenOfKey + 1));
    for (i = 0; i < lenOfKey + 1; i++) {
        // Returns random int from 65 to 91
        // 65 is ASCII character A, and 90 is ASCII character Z
        int curKey = (rand() % 27) + 65;

        if (curKey == 91) {
            // If the random int is 91, set to 32 (ASCII whitespace)
            curKey = 32;
        }
        key[i] = curKey;
    }

    // Tags a newline on the end of the key array
    key[lenOfKey] = '\n';

    // Writes key to stdout
    write(1, key, lenOfKey + 1);

    free(key);

    return 0;
}