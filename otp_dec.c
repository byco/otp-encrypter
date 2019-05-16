// This program should act as a client and request the server (otp_dec_d) to decrypt a ciphertext
// file using a key file. It will send the ciphertext and key as bytes via sockets and then print
// the resulting plaintext as received from otp_dec_d.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h> 


void printDec(int socket)
{
	// set up variables to receive plaintext data from server
	char buffer[200000];
	memset(buffer, '\0', sizeof(buffer));

	int bufferLength;
	int bufferWritten = 0;
	int charRecv = 0;

	// receive data from server
	charRecv = recv(socket, &buffer[bufferWritten], 100, 0);

	// if nothing received from socket, then print message
	if(charRecv < 0)
	{
		fprintf(stderr,"ERROR receiving from socket\n");
		exit(0);
	}

	// print resulting ciphertext to stdout. remember to flush!
	printf("%s\n", buffer);
	fflush(stdout);
}


void error(const char *msg) 
{ 
	perror(msg); 
	exit(0); 
} // Error function used for reporting issues

// send ciphertext and key data to server for decryption
void sendData(int socket, char buffer[], int length)
{
	// send ciphertext and key via given socket/port number
	int sent;
	sent = send(socket, buffer, length, 0);

	// if unsuccessfully sent, then print error message
	if(sent < 0)
	{
		fprintf(stderr,"ERROR sending to socket\n");
		exit(0);
	}
}


int main(int argc, char *argv[])
{
	// variables needed for reading ciphertext and key files and sending to server
	int port;
	int charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	char cBuffer[200000];
	memset(cBuffer, '\0', sizeof(cBuffer));
	char keyBuffer[200000];
	memset(keyBuffer, '\0', sizeof(keyBuffer));
	
	// Check usage & args
	if (argc < 3)
	{
		fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]);
		exit(0);
	}

	// use localhost as host and start setting up socket
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent* serverHost = gethostbyname("localhost");

    // if socket unsuccessfully opened, then print error message
	if(socketFD < 0)
	{
		fprintf(stderr, "CLIENT: ERROR opening socket");
		exit(0);
	}

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	port = atoi(argv[3]); // Get the port number, convert to an integer from a string

	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(port); // Store the port number

	// if localhost not found, print message in error
	if (serverHost == NULL) 
	{ 
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
		exit(0); 
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHost->h_addr, serverHost->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	
	if (socketFD < 0) 
	{
		error("CLIENT: ERROR opening socket");
		exit(1);
	}
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
	{
		error("CLIENT: ERROR connecting");
		exit(1);
	}

// CHECK PLAINTEXT FILE ONLY HAS CAP LETTERS AND SPACES (should end in \0)
// note - open wasn't working for me so will try fopen

// open ciphertext file for reading only, then get file length with fseek
	FILE* cText = fopen(argv[1], "r");
	fseek(cText, 0, SEEK_END); 
	int cLength = ftell(cText);  
	fseek(cText, 0, SEEK_SET);

	// print error message if ciphertext file is empty or unable to open
	if(cText == 0)
	{
		fprintf(stderr, "CLIENT: ERROR cipher text file failed to open\n"); 
		exit(1);
	}

	// save ciphertext file into cBuffer
	while(
		fgets(cBuffer, sizeof(cBuffer) - 1, cText));

// do the same for the key file - open file for reading, then get file length
	FILE* keyText = fopen(argv[2], "r");	
	fseek(keyText, 0, SEEK_END); 

	int keyLength = ftell(keyText);  
	fseek(keyText, 0, SEEK_SET);

	// print error message if key file is empty or unable to open
	if(keyText == 0)
	{
		fprintf(stderr, "CLIENT: ERROR key text file failed to open\n"); 
		exit(1);
	}

	// save key file into keyBuffer
	while(
		fgets(keyBuffer, sizeof(keyBuffer) - 1, keyText));

	// close files now that we're done with them
	fclose(cText);

	fclose(keyText);

	// print error message if ciphertext is longer than the key
	// unable to decrypt
	if(keyLength < cLength)
	{
		fprintf(stderr, "CLIENT: ERROR, key shorter than cipher text\n"); 
		exit(1); 
	}

	// add % to end of ciphertext
	// add ^ to end of keytext
	// makes for easier reading in server
	cBuffer[strcspn(cBuffer, "\n")] = '%';
	keyBuffer[strcspn(keyBuffer, "\n")] = '^';

	// combine both ciphertext and key into a single buffer
	sprintf(cBuffer + strlen(cBuffer), keyBuffer);

	// TESTING
	//printf("new buffer: %s\n", plainBuffer);

	// send both ciphertext and key to server. note that if key is longer than ciphertext, only
	// part of it will be sent for decryption
	sendData(socketFD, cBuffer, cLength + cLength);


	// TESTING
	//printf("plain: %s\nkey: %s\n", plainBuffer, keyBuffer);
	//fflush(stdout);

	//print resulting plaintext from server
	printDec(socketFD);

	close(socketFD); // Close the socket

	return 0;
}
