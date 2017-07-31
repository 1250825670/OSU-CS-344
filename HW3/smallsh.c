#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>

// TODO: Add support for background processes
// TODO: Add signal trapping

int* initArr();
void clearArr(int*);
void setArr(int*, int);
int findChar(char**, char*, int);

// The following functions are helpers to work with an array that is used to
// keep track of strings that were allocated dynamically, so that they can be
// freed later
int* initArr() {
    // Returns a pointer to an array of 512 elements with all values set to 0
    int* arr = malloc(sizeof(int) * 512);
    clearArr(arr);
    return arr;
}

void clearArr(int* arr) {
    // Takes a pointer to an array of 512 elements and sets all values to 0
    int i;
    for (i = 0; i < 512; i++) {
        arr[i] = 0;
    }
}

void setArr(int* arr, int ind) {
    // Takes a pointer to an array, and sets the value of the given index to 1
    arr[ind] = 1;
}

int findChar(char** newArgv, char* charToFind, int newArgvSize) {
    // Given an array of strings, finds the index of the array where the
    // specified string can be found. Returns -1 if not found
    int i;
    if (newArgv == NULL || charToFind == NULL) {
        return -1;
    }

    for (i = 0; i < newArgvSize; i++) {
        if (strcmp(newArgv[i], charToFind) == 0) {
            return i;
        }
    }
    return -1;
}

int main() {
    char* inputBuffer = NULL;
    size_t bufferSize = 0;
    int charsEntered = 0;
    char current_dir[100];
    int childExitMethod = -5;
    pid_t curPid;
    pid_t childPid = -5;
    int exitStatus = 0;
    char exitStatusChar[4];
    int termSignal = -5;

    char *home_var = getenv("HOME");
    int* charsToFree = initArr();

    while (1) {
        // Writes prompt to screen
        if (childPid != 0) {
            write(1, ": ", 2);
            fflush(stdout);
        }

        if (!(charsEntered = getline(&inputBuffer, &bufferSize, stdin))) {
            // For some reason, it wasn't possible to read the character input
            // Close out of the shell if this happens
            fflush(stdout);
            write(2, "Error: Unable to read character input\n", 38);
            fflush(stderr);
            exit(1);
        }

        if (charsEntered > 2048) {
            // If command is more than 2048 characters, prompt for next input
            write(1, "Can not have more than 2048 characters in input\n", 48);
            fflush(stdout);

            exitStatus = 1;     // Set exit status to a failure

            bufferSize = 0;
            free(inputBuffer);
            continue;
        }

        inputBuffer[charsEntered - 1] = '\0';

        // Need to parse input into an array, delimited by spaces
        char *token;
        char *restOfInput = inputBuffer;

        char ** newArgv = NULL;
        int argCount = 0;

        while ((token = strtok_r(restOfInput, " ", &restOfInput)))
        {
            // For each new argument, reallocate memory in our array
            argCount++;
            newArgv = realloc(newArgv, sizeof(char*) * argCount);

            // Ensure that memory reallocation was successful
            assert(newArgv != 0);

            newArgv[argCount - 1] = token;
        }

        if (argCount > 512) {
            // If command has more than 512 arguments, prompt for next input
            write(1, "Can't have more than 512 arguments in input\n", 44);
            fflush(stdout);

            exitStatus = 1;     // Set exit status to a failure

            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            continue;
        }

        // Expand out any instance of $$ to the current PID
        curPid = getpid();
        char curPidChar[10];
        memset(curPidChar, '\0', sizeof(curPidChar));
        snprintf(curPidChar, 10, "%d", curPid);
        int pidLen = strlen(curPidChar);
        char* startOfExpand;
        char* newStr;

        int i;
        for (i = 0; i < argCount; i++) {
            startOfExpand = strstr(newArgv[i], "$$");
            if (startOfExpand != NULL) {
                int lenOfOrig = strlen(newArgv[i]);

                // Construct a new string which is the expansion of $$
                newStr = malloc(sizeof(char) * (lenOfOrig - 2 + pidLen + 1));
                strncpy(newStr, newArgv[i], (startOfExpand - newArgv[i])/sizeof(char));
                strncpy(newStr + sizeof(char) * (startOfExpand - newArgv[i]), curPidChar, pidLen);
                strcat(newStr, newArgv[i] + sizeof(char)*(startOfExpand - newArgv[i] + 2));
                newArgv[i] = newStr;

                // Keep track of this index - malloc'd memory for this string,
                // so need to free it later
                setArr(charsToFree, i);
            }
        }

        // Add NULL to the argv array, as execv and execvp commands expect this
        newArgv = realloc(newArgv, sizeof(char*) * (argCount + 1));
        assert(newArgv != 0);
        newArgv[argCount] = NULL;

        if (charsEntered == 0) {
            // Do nothing
        }

        else if (*inputBuffer == '#') {
            // Do nothing - this line is a comment
        }

        else if (*inputBuffer == '\n' && argCount == 1) {
            // Do nothing - user didn't enter anything
        }

        else if (strcmp(inputBuffer, "exit") == 0) {
            // Allocated memory for some strings in argv if $$ expansion
            // was required. Need to free this memory
            for (i = 0; i < 512; i++) {
                if (charsToFree[i] == 1) {
                    free(newArgv[i]);
                }
            }
            clearArr(charsToFree);
            free(charsToFree);

            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
            // FIXME: Still need to kill off any remaining child processes
            exit(0);
        }

        // Need to have cd commands execute on parent process
        // If we fork and exec a cd command, it will change directories only on
        // the child process - after this terminates and returns to the parent,
        // we will still be in the same directory
        else if (strcmp(inputBuffer, "cd") == 0) {
            if (argCount == 1) {
                // Go to home directory if no arguments specified
                chdir(home_var);
                exitStatus = 0;
            }
            else if (argCount == 2) {
                chdir(newArgv[1]);
                exitStatus = 0;
            }
            else if (argCount >= 3) {
                // If more than 2 arguments specified, throw an error
                fflush(stdout);
                write(2, "cd: string not in pwd\n", 22);
                fflush(stderr);
                exitStatus = 1;
            }
        }

        else if (strcmp(inputBuffer, "status") == 0 && argCount == 1) {
            // Converts exit status to char
            memset(exitStatusChar, '\0', sizeof(exitStatusChar));
            snprintf(exitStatusChar, 4, "%d", exitStatus);

            // Writes exit value to stdout
            write(1, "exit value ", 11);
            fflush(stdout);
            write(1, exitStatusChar, 4);
            fflush(stdout);
            write(1, "\n", 1);
            fflush(stdout);
        }

        else {
            // Will return -1 if the redirect operators are not found
            // Otherwise, returns the index where they are found in argv
            int stdinRedir = findChar(newArgv, "<", argCount);
            int stdoutRedir = findChar(newArgv, ">", argCount);

            int stdinFile = 0;
            int stdoutFile = 0;

            // Keep track of whether or not this is a background process
            int backgroundProcess = 0;
            if (strcmp(newArgv[argCount - 1], "&") == 0) {
                // If this is meant to be initialized as a background process,
                // note this and remove the & from the argv array
                backgroundProcess = 1;
                newArgv[argCount - 1] = NULL;
                argCount--;
            }

            if (!(stdinRedir == -1 && stdoutRedir == -1)) {
                if (stdinRedir != -1) {
                    // First, check that stdin redirection argument is valid
                    if (stdinRedir == argCount - 1 || stdinRedir == 0 ||
                            stdoutRedir == stdinRedir + 1) {
                        fflush(stdout);
                        write(2, "Error: Invalid stdin redirection\n", 33);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }

                    // Open stdin file for read
                    stdinFile = open(newArgv[stdinRedir + 1], O_RDONLY);

                    // Check that file was opened properly
                    if (stdinFile < 0) {
                        fflush(stdout);
                        write(2, "Error: Cannot open file for stdin redirection\n", 46);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }

                    // When executing commands, don't want the redirection
                    // operators or arguments
                    newArgv[stdinRedir] = NULL;
                    newArgv[stdinRedir + 1] = NULL;
                }

                if (stdoutRedir != -1) {
                    // Check that stdout redirection argument is valid
                    if (stdoutRedir == argCount - 1 || stdoutRedir == 0 ||
                            stdinRedir == stdoutRedir + 1) {
                        fflush(stdout);
                        write(2, "Error: Invalid stdout redirection\n", 34);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }

                    // Open stdout file for writing
                    stdoutFile = open(newArgv[stdoutRedir + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                    // Check that file was opened appropriately
                    if (stdoutFile < 0) {
                        fflush(stdout);
                        write(2, "Error: Cannot open file for stdout redirection\n", 47);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }

                    // When executing commands, don't want the redirection
                    // operators or arguments
                    newArgv[stdoutRedir] = NULL;
                    newArgv[stdoutRedir + 1] = NULL;
                }
            }

            // Fork a child process to run this command
            childPid = fork();

            // Execute this command if we are in the child process
            if (childPid == 0) {
                // Redirects stdin to the open file
                if (stdinFile != 0) {
                    if (dup2(stdinFile, 0) < 0) {
                        // Check to make sure that redirection occurred properly
                        fflush(stdout);
                        write(2, "Error: Issue with stdin redirection\n", 36);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }
                }

                // Redirects stdout to the open file
                if (stdoutFile != 0) {
                    if (dup2(stdoutFile, 1) < 0) {
                        // Check to make sure that redirection occurred properly
                        fflush(stdout);
                        write(2, "Error: Issue with stdout redirection\n", 37);
                        fflush(stderr);
                        free(newArgv);
                        free(inputBuffer);
                        exit(1);
                    }
                }

                // Close files before execution          
                if (stdinFile != 0) {
                    close(stdinFile);
                }

                if (stdoutFile != 0) {
                    close(stdoutFile);
                }

                // Execute the command
                if (execvp(newArgv[0], newArgv) < 0) {
                    // Throw an error if the command did not work
                    fflush(stdout);
                    write(2, "Error: Command not found\n", 25);
                    fflush(stderr);
                    free(newArgv);
                    free(inputBuffer);
                    exit(1);
                }
                exit(0);
            }
            else {
                if (backgroundProcess == 0) {
                    // If command was run in foreground, then block until
                    // completion of child process
                    waitpid(childPid, &childExitMethod, 0);

                    if (stdinFile != 0) {
                        close(stdinFile);
                    }

                    if (stdoutFile != 0) {
                        close(stdoutFile);
                    }

                    // Get termination method
                    if (WIFEXITED(childExitMethod) != 0) {
                        exitStatus = WEXITSTATUS(childExitMethod);
                    }

                    else if (WIFSIGNALED(childExitMethod) != 0) {
                        termSignal = WTERMSIG(childExitMethod);
                    }
                }
                else if (backgroundProcess == 1) {
                    // Run in background
                }
            }
        }

        // Allocated memory for some strings in argv if $$ expansion
        // was required. Need to free this memory
        for (i = 0; i < 512; i++) {
            if (charsToFree[i] == 1) {
                free(newArgv[i]);
            }
        }
        clearArr(charsToFree);

        free(newArgv);
        bufferSize = 0;
        free(inputBuffer);
        inputBuffer = NULL;
    }
}