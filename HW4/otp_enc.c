#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

// Prints a usage statement
void print_usage() {
    printf("Usage:\notp_enc plaintext key port\n");
	fflush(stdout);
    printf("plaintext is a file containing the text to encrypt\n");
	fflush(stdout);
	printf("key is a file containing the key used to encrypt the file\n");
	fflush(stdout);
	printf("port is an int representing the port to send the data to\n");
	fflush(stdout);
}

int main(int argc, char *argv[])
{    
	// Check for required parameters
	if (argc < 4) {
		fprintf(stderr, "%s", "Error: Incorrect number of parameters\n");
		fflush(stdout);
		print_usage();
		exit(1);
	}

	// Check that port given is a valid integer
	int i;
	for (i = 0; i < strlen(argv[3]); i++) {
        if (!isdigit(argv[3][i])) {
            fprintf(stderr, "%s", "Error: port parameter should be an integer\n");
			fflush(stdout);
            print_usage();
            exit(1);
        }
    }

	char* plaintextBuffer = NULL;
	size_t plaintextBufferSize = 0;
	int plaintextBufferEntered = 0;

	// Opens files for reading
	FILE* plaintextFile = fopen(argv[1], "r");
	if (!plaintextFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[1]);
		fflush(stdout);
		exit(1);
	}

	// Size of the string array
	// Last element of the string array is \n
	plaintextBufferEntered = getline(&plaintextBuffer, &plaintextBufferSize, plaintextFile);
	fclose(plaintextFile);

	// Check for invalid characters in plaintext
	for (i = 0; i < plaintextBufferEntered - 1; i++) {
		if ( !(plaintextBuffer[i] >= 65 && plaintextBuffer[i] <= 90) && plaintextBuffer[i] != 32 ) {
			fprintf(stderr, "%s", "\nError: input contains bad characters\n");
			free(plaintextBuffer);
			exit(1);
		}
	}

	char* keyBuffer = NULL;
	size_t keySize = 0;
	int keyBufferEntered = 0;

	FILE* keyFile = fopen(argv[2], "r");
	if (!keyFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[2]);
		exit(1);
	}
	keyBufferEntered = getline(&keyBuffer, &keySize, keyFile);
	fclose(keyFile);

	// Check that key is long enough
	// Key must be at least as long as plaintext
	if (keyBufferEntered < plaintextBufferEntered) {
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		fflush(stdout);
		free(plaintextBuffer);
		free(keyBuffer);
		exit(1);
	}

	int socketFD, portNumber;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	ssize_t charsWritten, charsRead;

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(1); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket in otp_enc");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		free(plaintextBuffer);
		free(keyBuffer);
		exit(2);
	}
	
	// First check that the client connected to the correct daemon
	char daemonType[2];
	memset(daemonType, '\0', sizeof(daemonType));
	charsRead = recv(socketFD, daemonType, 2, 0);
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		free(plaintextBuffer);
		free(keyBuffer);
		exit(1);
	}

	char clientType[2];
	clientType[0] = 'e';
	clientType[1] = '\0';
	// Also, send the client type to the daemon
	charsWritten = send(socketFD, clientType, 2, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(plaintextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// If connected to the incorrect daemon, need to exit with error
	if (strcmp(daemonType, "e") != 0) {
		fprintf(stderr, "%s", "Error: Attempting to connect decryption daemon\n");
		free(plaintextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// Removes trailing \n from input text files
	plaintextBuffer[plaintextBufferEntered - 1] = '\0';
	keyBuffer[keyBufferEntered - 1] = '\0';

	// First, sends a number containing the length of the plaintext buffer
	char bufferSizeChar[8];
	memset(bufferSizeChar, '\0', sizeof(bufferSizeChar));
	snprintf(bufferSizeChar, 8, "%d", plaintextBufferEntered - 1);
	charsWritten = send(socketFD, bufferSizeChar, 8, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(plaintextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// Send plaintext to server
	int curBuffer = 0;
	while (curBuffer < plaintextBufferEntered - 1) {
		charsWritten = send(socketFD, plaintextBuffer, plaintextBufferEntered - 1, 0);
		if (charsWritten < 0) {
			fprintf(stderr, "Error: Unable to write to socket\n");
			free(plaintextBuffer);
			free(keyBuffer);
			exit(1);
		}
		curBuffer += charsWritten;
	}

	// Send the key to server
	// If the key is longer than the plaintext, only enough characters to encrypt the plaintext
	// will be sent
	curBuffer = 0;
	while (curBuffer < plaintextBufferEntered - 1) {
		charsWritten = send(socketFD, keyBuffer, plaintextBufferEntered - 1, 0);
		if (charsWritten < 0) {
			fprintf(stderr, "Error: Unable to write to socket\n");
			free(plaintextBuffer);
			free(keyBuffer);
			exit(1);
		}
		curBuffer += charsWritten;
	}

	curBuffer = 0;
	char encryptedText[plaintextBufferEntered];
	memset(encryptedText, '\0', sizeof(encryptedText));
	while (curBuffer < plaintextBufferEntered - 1) {
		charsRead = recv(socketFD, encryptedText, plaintextBufferEntered - 1, 0); // Read the client's message from the socket
		if (charsRead < 0) {
			fprintf(stderr, "%s", "Error: unable to read from socket");
			free(plaintextBuffer);
			free(keyBuffer);
			exit(1);
		}
		curBuffer += charsRead;
	}

	close(socketFD); // Close the socket

	// Output the encrypted text onto stdout
	fprintf(stdout, "%s\n", encryptedText);

	free(plaintextBuffer);
	free(keyBuffer);
	return 0;
}
