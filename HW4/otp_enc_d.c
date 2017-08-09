#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
// FIXME: How to account for different size data in receiving buffer?

// Prints a usage statement
void print_usage() {
    printf("Usage:\notp_enc_d port\n");
	fflush(stdout);
    printf("port is an int on which to listen on\n");
	fflush(stdout);
}

void encrypt_key(char* plainText, char* key) {
	int i;
	int lenOfText = strlen(plainText);
	for (i = 0; i < lenOfText; i++) {
		if (plainText[i] == 32) {
			plainText[i] = 91;
		}
		if (key[i] == 32) {
			plainText[i] = 91;
		}
		int curKey = (plainText[i] - 65) + (key[i] - 65);
		curKey = curKey % 27;
		if (curKey == 26) {
			curKey = 32;
		}
		else {
			curKey += 65;
		}
		plainText[i] = curKey;
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Check that parameter given is a valid integer
	int i;
	for (i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            fprintf(stderr, "%s", "Error: parameter should be an integer\n");
			fflush(stdout);
            print_usage();
            exit(1);
        }
    }

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the listen socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) {
		fprintf(stderr, "%s", "Error: Unable to open listen socket\n");
		fflush(stdout);
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "%s", "Error: Unable to bind listen socket\n");
		fflush(stdout);
		exit(1);
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	if (establishedConnectionFD < 0) {
		fprintf(stderr, "%s", "Error: Unable to accept connection\n");
		exit(1);
	}

	// TODO: Send to client first
	// Send server type (encryption)

	// Get the message from the client
	char textToEncrypt[256];
	char keyText[256];
	memset(textToEncrypt, '\0', 256);
	memset(keyText, '\0', 256);

	charsRead = recv(establishedConnectionFD, textToEncrypt, 255, 0);
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		exit(1);
	}

	charsRead = recv(establishedConnectionFD, keyText, 255, 0); // Read the client's message from the socket
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		exit(1);
	}

	encrypt_key(textToEncrypt, keyText);

	// Send encrypted text back to client
	ssize_t charsWritten;
	charsWritten = send(establishedConnectionFD, textToEncrypt, strlen(textToEncrypt), 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		exit(1);
	}

	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
