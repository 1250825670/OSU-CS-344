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

	// Opens files for reading
	FILE* plaintextFile = fopen(argv[1], "r");
	FILE* keyFile = fopen(argv[2], "r");

	if (!plaintextFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[1]);
		fflush(stdout);
		exit(1);
	}
	if (!keyFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[2]);
		fflush(stdout);
		exit(1);
	}

	char* plaintextBuffer = NULL;
	size_t plaintextBufferSize = 0;
	int plaintextBufferEntered = 0;

	char* keyBuffer = NULL;
	size_t keySize = 0;
	int keyBufferEntered = 0;

	// Size of the string array
	// Last element of the string array is \n
	plaintextBufferEntered = getline(&plaintextBuffer, &plaintextBufferSize, plaintextFile);
	keyBufferEntered = getline(&keyBuffer, &keySize, keyFile);

	fclose(plaintextFile);
	fclose(keyFile);

	// Check that key is long enough
	// Key must be at least as long as plaintext
	if (keyBufferEntered < plaintextBufferEntered) {
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		fflush(stdout);
		exit(1);
	}

	// Check for invalid characters in plaintext
	for (i = 0; i < plaintextBufferEntered - 1; i++) {
		if ( !(plaintextBuffer[i] >= 65 && plaintextBuffer[i] <= 90) && plaintextBuffer[i] != 32 ) {
			fprintf(stderr, "%s", "\nError: input contains bad characters\n");
			fflush(stdout);
			exit(1);
		}
	}

	// TODO: Remove trailing newline from plaintext and keybuffer before outputting

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
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting otp_enc socket");
	
	// TODO: Receive from server first
	// Receive the type of server - encrypt or decrypt

	// Get input message from user
	/*
	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	*/

	// Send message to server
	/*
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	*/

	// Removes trailing \n from input text files
	plaintextBuffer[plaintextBufferEntered - 1] = '\0';
	keyBuffer[keyBufferEntered - 1] = '\0';

	// Send plaintext to server
	charsWritten = send(socketFD, plaintextBuffer, strlen(plaintextBuffer), 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		exit(1);
	}

	// TODO: Handle this contingency
	if (charsWritten < strlen(plaintextBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Send the key to server
	charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		exit(1);
	}

	// TODO: Handle this contingency
	if (charsWritten < strlen(keyBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	char encryptedText[256];
	memset(encryptedText, '\0', sizeof(encryptedText));
	charsRead = recv(socketFD, encryptedText, 255, 0); // Read the client's message from the socket
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		exit(1);
	}

	close(socketFD); // Close the socket

	fprintf(stdout, "%s\n", encryptedText);

	free(plaintextBuffer);
	free(keyBuffer);
	return 0;
}
