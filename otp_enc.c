// This program should act as a client and request the server (otp_enc_d) to encrypt a plaintext
// file and key file. It will send the plaintext and key as bytes via sockets and then print
// the resulting ciphertext as received from otp_enc_d.

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

// this function will print the final resulting ciphertext as received from otp_enc_d
void printEnc(int socket)
{
	// buffer to receive data from server
	char buffer[200000];
	memset(buffer, '\0', sizeof(buffer));

	int bufferLength;
	int bufferWritten = 0;
	int charRecv = 0;

	// receive server data
	charRecv = recv(socket, &buffer[bufferWritten], 100, 0);

	// if server data not successfully received, print error message
	if(charRecv < 0)
	{
		fprintf(stderr,"ERROR receiving from socket\n");
		exit(0);
	}

	// print buffer (aka ciphertext). Remember to flush every time printf is used!
	printf("%s\n", buffer);
	fflush(stdout);
}


void error(const char *msg) 
{ 
	perror(msg); 
	exit(0); 
} // Error function used for reporting issues


// this function will send the plaintext and key files byte by byte (since open() isn't allowed)
void sendData(int socket, char buffer[], int length)
{
	int sent;

		sent = send(socket, buffer, length, 0);

		if(sent < 0)
		{
			fprintf(stderr,"ERROR sending to socket\n");
			exit(0);
		}
}


int main(int argc, char *argv[])
{
	// set up variables needed for setting up sockets and reading files
	int port;
	int charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	char plainBuffer[200000];
	memset(plainBuffer, '\0', sizeof(plainBuffer));
	char keyBuffer[200000];
	memset(keyBuffer, '\0', sizeof(keyBuffer));

	// Check usage & args
	if (argc < 3)
	{
		fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
		exit(0);
	}

	// localhost is set as the target IP address/host
	// set up localhost since we're not doing this via the command line as in server.c
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent* serverHost = gethostbyname("localhost");

    // print error message if socket isn't opening
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
	
	// print error message if localhost isn't found
	if (serverHost == NULL) 
	{ 
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
		exit(0); 
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHost->h_addr, serverHost->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	
	// print message if new socket isn't working
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

// CHECK PLAINTEXT FILE ONLY HAS CAP LETTERS AND SPACES (should end in \0?)
// note - open wasn't working for me so will try fopen

// open plaintext file for reading only
	FILE* plainText = fopen(argv[1], "r");
	fseek(plainText, 0, SEEK_END); 
	// now find the length of plaintext file - then go back to beginning for reading
	int plainLength = ftell(plainText);  
	fseek(plainText, 0, SEEK_SET);

	// if text file is empty or not found, print error message
	if(plainText == 0)
	{
		fprintf(stderr, "CLIENT: ERROR plain text file failed to open\n"); 
		exit(1);
	}

	// save file into plainBuffer
	while(
		fgets(plainBuffer, sizeof(plainBuffer) - 1, plainText));


// open key file for reading only
	FILE* keyText = fopen(argv[2], "r");
	fseek(keyText, 0, SEEK_END); 
// save the length of keytext file into keylength, go back to beginning for reading
	int keyLength = ftell(keyText);  
	fseek(keyText, 0, SEEK_SET);

	// if key file is empty or not found, print error message
	if(keyText == 0)
	{
		fprintf(stderr, "CLIENT: ERROR key text file failed to open\n"); 
		exit(1);
	}

	// save file into keyBuffer
	while(
		fgets(keyBuffer, sizeof(keyBuffer) - 1, keyText));


// close both plain and key files
	fclose(plainText);	
	fclose(keyText);


	// print error message if key isn't long enough to ecrypt plaintext
	if(keyLength < plainLength)
	{
		fprintf(stderr, "CLIENT: ERROR, key shorter than plaintext\n"); 
		exit(1); 
	}

	// add characters for easy reading in server
	// % is at the end of the plaintext characters
	// ^ is at the end of the key characters
	plainBuffer[strcspn(plainBuffer, "\n")] = '%';
	keyBuffer[strcspn(keyBuffer, "\n")] = '^';

	// now combine both plaintext and key into one buffer 
	sprintf(plainBuffer + strlen(plainBuffer), keyBuffer);

	// TESTING THAT NEW BUFFER IS COMBINED PLAINTEXT AND KEY
	//printf("new buffer: %s\n", plainBuffer);

	// send combined buffer to server for encryption
	// note that it has to be 2x as long as the plaintext file
	// it will cut off the key if the key is longer than the plaintext
	sendData(socketFD, plainBuffer, plainLength + plainLength);


	// print ecrypted string from server
	printEnc(socketFD);

	close(socketFD); // Close the socket

	return 0;
}
