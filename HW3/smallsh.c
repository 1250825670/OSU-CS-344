// CS-344 HW3
// Programmer: Charles Chen

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>

// Global variable to keep track of if we are in foreground-only mode
// set to this type so it can be modified within a signal handler
volatile sig_atomic_t FOREGROUND_ONLY = 0;

struct background_children {
    int *children_pids;
    int n;
};
struct background_children* childProcesses;

struct background_children* initBGChildren();
void clearChildren(struct background_children*);
void addChild(struct background_children*, int);
void removeChild(struct background_children*, int);
int childrenLeft(struct background_children*);
int getLastChild(struct background_children*);
void popLastChild(struct background_children*);
int pidInChildren(struct background_children*, pid_t);
int* initArr();
void clearArr(int*);
void setArr(int*, int);
int findChar(char**, char*, int);
void catchSIGTSTP(int);
void checkBackgroundProcesses(struct background_children*);

// Initializes a background_children struct
// Allocates space for 11 integers in children_pids array and sets
// n to 0
struct background_children* initBGChildren() {
    // Maximum of 10 children
    // Using last index as a garbage can
    int* arr = malloc(sizeof(int) * 11);
    struct background_children* childrenStruct = malloc(sizeof(struct background_children));
    childrenStruct->children_pids = arr;

    // n attribute represents the number of children in the children_pids array
    childrenStruct->n = 0;
    return childrenStruct;
}

// Frees memory allocated for struct
void clearChildren(struct background_children* children) {
    free(children->children_pids);
    free(children);
    return;
}

// Adds the given pid to the children_pids array
void addChild(struct background_children* children, int child_pid) {
    // Only adds new value if there is space left in the array
    if (children->n < 10) {
        children->children_pids[children->n] = child_pid;
        children->n++;
    }
    return;
}

// Removes the given pid from the children_pids array
void removeChild(struct background_children* children, int child_pid) {
    if (children->n == 0) {
        return;
    }

    // Find the index of the array in which the pid canbe found
    int i;
    int found = 0;
    for (i = 0; i < children->n; i++) {
        if (children->children_pids[i] == child_pid) {
            found = 1;
            break;
        }
    }

    // Remove the given pid from the children_pids array, and shift following
    // elements up
    if (found == 1) {
        children->children_pids[10] = children->children_pids[i];
        int j;
        for (j = i; j < children->n; j++) {
            children->children_pids[j] = children->children_pids[j + 1];
        }
        children->n--;
    }
}

// Returns the number of children left
int childrenLeft(struct background_children* children) {
    return children->n;
}

// Gets the value of the last child's pid
int getLastChild(struct background_children* children) {
    return children->children_pids[children->n-1];
}

// Removes the last child's pid
void popLastChild(struct background_children* children) {
    children->n--;
}

// Returns 1 if the given pid is in children_pids, 0 if not
int pidInChildren(struct background_children* children, pid_t pidToCheck) {
    if (children->n == 0) {
        return 0;
    }

    int i;
    for (i = 0; i < children->n; i++) {
        if (children->children_pids[i] == (int)pidToCheck) {
            return 1;
        }
    }
    return 0;
}

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

// Catches SIGTSTP signal and switches foreground-only mode on and off
void catchSIGTSTP(int signo) {
    if (FOREGROUND_ONLY == 0) {
        write(1, "\nEntering foreground-only mode (& is now ignored)\n: ", 52);
        fflush(stdout);
        FOREGROUND_ONLY = 1;
    }
    else if (FOREGROUND_ONLY == 1) {
        write(1, "\nExiting foreground-only mode\n: ", 32);
        fflush(stdout);
        FOREGROUND_ONLY = 0;
    }
}

// Check all background processes to see if they have terminated
void checkBackgroundProcesses(struct background_children* bgProcesses) {
    int childExitMethod;
    int i;

    // Loops through all stored background process PIDs stored in struct
    for (i = 0; i < bgProcesses->n; i++) {
        pid_t childEndedPID = waitpid(bgProcesses->children_pids[i],
                &childExitMethod, WNOHANG);
        
        // waitpid returns 0 if the given process has not completed
        if (childEndedPID == 0) {
            return;
        }

        if (WIFEXITED(childExitMethod) != 0) {
            // Background process ended normally - print exit value
            int exitStatus = WEXITSTATUS(childExitMethod);
            write(1, "background pid ", 16);
            fflush(stdout);

            // Converts the PID to char for printing
            char curPidChar[10];
            memset(curPidChar, '\0', sizeof(curPidChar));
            snprintf(curPidChar, 10, "%d", (int)childEndedPID);

            write(1, curPidChar, 10);
            fflush(stdout);
            write(1, " is done: exit value ", 21);
            fflush(stdout);

            char exitStatusChar[5];
            memset(exitStatusChar, '\0', sizeof(exitStatusChar));
            snprintf(exitStatusChar, 5, "%d", exitStatus);

            write(1, exitStatusChar, 5);
            fflush(stdout);
            write(1, "\n", 1);
            fflush(stdout);

            // Remove the process that was just terminated from our children ADT
            removeChild(bgProcesses, (int)childEndedPID);
        }
        else if (WIFSIGNALED(childExitMethod) != 0) {
            // Background process was terminated by a signal
            int termSig = WTERMSIG(childExitMethod);
            write(1, "\nbackground pid ", 16);
            fflush(stdout);

            // Converts the PID to char for printing
            char curPidChar[10];
            memset(curPidChar, '\0', sizeof(curPidChar));
            snprintf(curPidChar, 10, "%d", (int)childEndedPID);

            write(1, curPidChar, 10);
            fflush(stdout);

            write(1, " is done: terminated by signal ", 31);
            fflush(stdout);

            char termSigChar[5];
            memset(termSigChar, '\0', sizeof(termSigChar));
            snprintf(termSigChar, 5, "%d", termSig);

            write(1, termSigChar, 5);
            fflush(stdout);
            write(1, "\n", 1);
            fflush(stdout);
            
            // Remove the process that was just terminated from our children ADT
            removeChild(bgProcesses, (int)childEndedPID);
        }
    }
}

int main() {
    int childExitMethod = -5;
    pid_t curPid;
    pid_t childPid = -5;
    int exitStatus = 0;
    char exitStatusChar[4];
    int i;

    int* charsToFree = initArr();
    char *home_var = getenv("HOME");
    childProcesses = initBGChildren();

    // Signal handler for CTRL+Z
    // Toggles foreground-only mode
    struct sigaction SIGTSTP_action = {0};
    memset(&SIGTSTP_action, 0, sizeof(SIGTSTP_action));
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigemptyset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;

    // Signal handler for CTRL+C in the shell
    struct sigaction ignore_SIGINT = {0};
    memset(&ignore_SIGINT, 0, sizeof(ignore_SIGINT));
    ignore_SIGINT.sa_handler = SIG_IGN;
    sigfillset(&ignore_SIGINT.sa_mask);
    sigdelset(&ignore_SIGINT.sa_mask, SIGCHLD);
    ignore_SIGINT.sa_flags = SA_RESTART;

    // Check for CTRL+Z signal
    // Will swap between foreground only and background allowed modes
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while (1) {
        char* inputBuffer = NULL;
        size_t bufferSize = 0;
        int charsEntered = 0;

        // Ignores SIGINT signal in the shell
        sigaction(SIGINT, &ignore_SIGINT, NULL);

        // Before prompt, check for completion of background processes
        checkBackgroundProcesses(childProcesses);

        // Writes prompt to screen
        if (childPid != 0) {
            write(1, ": ", 2);
            fflush(stdout);
        }

        // Prompts user for input
        charsEntered = getline(&inputBuffer, &bufferSize, stdin);

        if (charsEntered > 2048) {
            // If command is more than 2048 characters, prompt for next input
            write(1, "Can not have more than 2048 characters in input\n", 48);
            fflush(stdout);

            exitStatus = 1;     // Set exit status to a failure

            bufferSize = 0;
            free(inputBuffer);
            continue;
        }

        // If input reading unsuccessful, just prompt for input again
        if (charsEntered < 1) {
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
            write(1, "\n", 1);
            continue;
        }

        // Replaces last character of inputBuffer with null terminator instead
        // of newline
        inputBuffer[charsEntered - 1] = '\0';

        // Need to parse input into an array, delimited by spaces
        char *token;
        char *restOfInput = inputBuffer;
        char ** newArgv = NULL;
        int argCount = 0;

        // Splits up every argument in input and stores in newArgv array
        // Parses the argument on whitespaces
        while ((token = strtok_r(restOfInput, " ", &restOfInput))) {
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

        // Get prompt for next command if no arguments detected
        if (argCount <= 0) {
            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
            continue;
        }

        if (charsEntered == 0) {
            // Do nothing
            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
            continue;
        }

        if (*inputBuffer == '#') {
            // Do nothing - this line is a comment
            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
            continue;
        }

        if (*inputBuffer == '\n' && argCount == 1) {
            // Do nothing - user didn't enter anything
            free(newArgv);
            bufferSize = 0;
            free(inputBuffer);
            inputBuffer = NULL;
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

        // Checks the arguments to see if any argument contains $$
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

        // Add NULL to the argv array, as execvp command expects this
        newArgv = realloc(newArgv, sizeof(char*) * (argCount + 1));
        assert(newArgv != 0);
        newArgv[argCount] = NULL;

        if (strcmp(inputBuffer, "exit") == 0) {
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

            // Kills off any remaining background processes
            while (childrenLeft(childProcesses) > 0) {
                int lastChild = getLastChild(childProcesses);
                kill((pid_t)lastChild, SIGTERM);
                popLastChild(childProcesses);
            }

            // Clear children struct
            clearChildren(childProcesses);

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

        // Status builtin command
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

        // If the command is not a builtin, need to fork and exec
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

            // If we are in foreground-only mode, override & command
            if (FOREGROUND_ONLY == 1) {
                backgroundProcess = 0;
            }

            if (!(stdinRedir == -1 && stdoutRedir == -1)) {
                if (stdinRedir != -1) {
                    // First, check that stdin redirection argument is valid
                    if (stdinRedir == argCount - 1 || stdinRedir == 0 ||
                            stdoutRedir == stdinRedir + 1) {
                        fflush(stdout);
                        write(2, "Error: Invalid stdin redirection\n", 33);
                        fflush(stderr);

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
                        continue;
                    }

                    // Open stdin file for read
                    stdinFile = open(newArgv[stdinRedir + 1], O_RDONLY);

                    // Check that file was opened properly
                    if (stdinFile < 0) {
                        fflush(stdout);
                        write(2, "Error: Cannot open file for stdin redirection\n", 46);
                        fflush(stderr);

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
                        continue;
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
                        continue;
                    }

                    // Open stdout file for writing
                    stdoutFile = open(newArgv[stdoutRedir + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                    // Check that file was opened appropriately
                    if (stdoutFile < 0) {
                        fflush(stdout);
                        write(2, "Error: Cannot open file for stdout redirection\n", 47);
                        fflush(stderr);
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
                        continue;
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
                if (backgroundProcess == 0) {
                // If this is a foreground process, SIGINT should terminate
                // this process
                    struct sigaction terminate_FG = {0};
                    memset(&terminate_FG, 0, sizeof(terminate_FG));

                    terminate_FG.sa_handler = SIG_DFL;
                    sigemptyset(&terminate_FG.sa_mask);
                    terminate_FG.sa_flags = 0;

                    sigaction(SIGINT, &terminate_FG, NULL);
                }
                if (backgroundProcess == 1) {
                    // If this is a background process, ignore SIGINT
                    sigaction(SIGINT, &ignore_SIGINT, NULL);
                }
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

                if (backgroundProcess == 1 && stdinFile == 0) {
                    int devNull = open("/dev/null", O_RDONLY);
                    dup2(devNull, 0);
                    close(devNull);
                }

                if (backgroundProcess == 1 && stdoutFile == 0) {
                    int devNull = open("/dev/null", O_WRONLY);
                    dup2(devNull, 1);
                    close(devNull);
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
                    fflush(stdin);
                    fflush(stdout);

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

                    // If the process was killed by signal, print the signal
                    if (WIFSIGNALED(childExitMethod) != 0) {
                        int termSig = WTERMSIG(childExitMethod);
                        
                        write(1, "terminated by signal ", 21);
                        fflush(stdout);

                        char termSigChar[5];
                        memset(termSigChar, '\0', sizeof(termSigChar));
                        snprintf(termSigChar, 5, "%d", termSig);
                        fflush(stdout);

                        write(1, termSigChar, 5);
                        fflush(stdout);
                        write(1, "\n", 1);
                        fflush(stdout);
                        
                        exitStatus = 1;
                    }
                }
                else if (backgroundProcess == 1) {
                    // Add child process to list of background processes
                    // Will check for completion of background processes prior
                    // to returning prompt to user
                    addChild(childProcesses, childPid);
                    
                    // Run in background
                    char childPidChar[10];
                    memset(childPidChar, '\0', sizeof(childPidChar));
                    snprintf(childPidChar, 10, "%d", childPid);
                    write(1, "background pid is ", 18);
                    fflush(stdout);
                    write(1, childPidChar, 10);
                    fflush(stdout);
                    write(1, "\n", 1);
                    fflush(stdout);
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