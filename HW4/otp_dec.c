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
    printf("Usage:\notp_dec ciphertext key port\n");
	fflush(stdout);
    printf("cipertext is a file containing the text to decrypt\n");
	fflush(stdout);
	printf("key is a file containing the key used to decrypt the file\n");
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
	FILE* ciphertextFile = fopen(argv[1], "r");
	FILE* keyFile = fopen(argv[2], "r");

	if (!ciphertextFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[1]);
		fflush(stdout);
		exit(1);
	}
	if (!keyFile) {
		fprintf(stderr, "%s%s\n", "Error: could not open file ", argv[2]);
		fflush(stdout);
		exit(1);
	}

	char* ciphertextBuffer = NULL;
	size_t ciphertextBufferSize = 0;
	int ciphertextBufferEntered = 0;

	char* keyBuffer = NULL;
	size_t keySize = 0;
	int keyBufferEntered = 0;

	// Size of the string array
	// Last element of the string array is \n
	ciphertextBufferEntered = getline(&ciphertextBuffer, &ciphertextBufferSize, ciphertextFile);
	keyBufferEntered = getline(&keyBuffer, &keySize, keyFile);

	fclose(ciphertextFile);
	fclose(keyFile);

	// Check that key is long enough
	// Key must be at least as long as plaintext
	if (keyBufferEntered < ciphertextBufferEntered) {
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		fflush(stdout);
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// Check for invalid characters in plaintext
	for (i = 0; i < ciphertextBufferEntered - 1; i++) {
		if ( !(ciphertextBuffer[i] >= 65 && ciphertextBuffer[i] <= 90) && ciphertextBuffer[i] != 32 ) {
			fprintf(stderr, "%s", "\nError: input contains bad characters\n");
			fflush(stdout);
			free(ciphertextBuffer);
			free(keyBuffer);
			exit(1);
		}
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
	if (socketFD < 0) error("CLIENT: ERROR opening socket in otp_dec");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(2);
	}
	
	// First check that the client connected to the correct daemon
	char daemonType[2];
	memset(daemonType, '\0', sizeof(daemonType));
	charsRead = recv(socketFD, daemonType, 2, 0);
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	char clientType[2];
	clientType[0] = 'd';
	clientType[1] = '\0';
	// Also, send the client type to the daemon
	charsWritten = send(socketFD, clientType, 2, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// If connected to the incorrect daemon, need to exit with error
	if (strcmp(daemonType, "d") != 0) {
		fprintf(stderr, "%s", "Error: Attempting to connect encryption daemon\n");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// Removes trailing \n from input text files
	ciphertextBuffer[ciphertextBufferEntered - 1] = '\0';
	keyBuffer[keyBufferEntered - 1] = '\0';

	// First, sends a number containing the length of the plaintext buffer
	char bufferSizeChar[8];
	memset(bufferSizeChar, '\0', sizeof(bufferSizeChar));
	snprintf(bufferSizeChar, 8, "%d", ciphertextBufferEntered - 1);
	charsWritten = send(socketFD, bufferSizeChar, 8, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	// Send ciphertext to server
	charsWritten = send(socketFD, ciphertextBuffer, ciphertextBufferEntered - 1, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}
	if (charsWritten < ciphertextBufferEntered - 1) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Send the key to server
	// If the key is longer than the ciphertext, only enough characters to encrypt the plaintext
	// will be sent
	charsWritten = send(socketFD, keyBuffer, ciphertextBufferEntered - 1, 0);
	if (charsWritten < 0) {
		fprintf(stderr, "Error: Unable to write to socket\n");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	if (charsWritten < ciphertextBufferEntered - 1) printf("CLIENT: WARNING: Not all data written to socket!\n");

	char decryptedText[ciphertextBufferEntered];
	memset(decryptedText, '\0', sizeof(decryptedText));
	charsRead = recv(socketFD, decryptedText, ciphertextBufferEntered - 1, 0); // Read the client's message from the socket
	if (charsRead < 0) {
		fprintf(stderr, "%s", "Error: unable to read from socket");
		free(ciphertextBuffer);
		free(keyBuffer);
		exit(1);
	}

	close(socketFD); // Close the socket

	// Output the encrypted text onto stdout
	fprintf(stdout, "%s\n", decryptedText);

	free(ciphertextBuffer);
	free(keyBuffer);
	return 0;
}
