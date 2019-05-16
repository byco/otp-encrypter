// This program should run in the background like a daemon and will decrypt a ciphertext file, 
// given the file name and another file with a key. It will decrypt and return plaintext
// when requested by a client program (otp_dec).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

// function to decrypt ciphertext using a key
// characters in original will be altered
void decrypt(char original[], char key[])
{
		// set up variables for decryption
        int length = strlen(original);
        int cNum;
        int keyNum;
        int newNum;
        // these are valid characters in the plaintext
        char okChars[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

        // go through the ciphertext, character, by character
        int i;
        for (i = 0; i < length; i++)
        {
        		// compare ciphertext and okChars characters
        		// find the corresponding okChars character
        		// assign the index number of okChars
                int j;
                for(j = 0; j < 27; j++)
                {
                        if(original[i] == okChars[j])
                        {
                                cNum = j;
                        }
                }

                // compare ciphertext and key characters
                // find the corresponding okChars character
                // assign the index number of okChars
                int k;
                for(k = 0; k < 27; k++)
                {
                        if(key[i] == okChars[k])
                        {
                                keyNum = k;
                        }
                }

                // decrypt by subtracting the key number equivalent from ciphertext number equivalent
                newNum = cNum - keyNum;

                // it's possible that the key number is bigger than the ciphertext number
                // so add 27 if the plaintext number is currently negative
                // this ensures that the plaintext number is in the valid 0-26 range
                while(newNum < 0)
                {
                        newNum = newNum + 27;
                }


                //printf("plainNum: %i\n", plainNum);
                //printf("plainChar: %c\n", okChars[plainNum]);
                //printf("keyNum: %i\n", keyNum);
                //printf("keyChar: %c\n", okChars[keyNum]);
                //printf("newNum: %i\n", newNum);
                //printf("newChar: %c\n", okChars[newNum]);

                // assign the correct character to the original array
                original[i] = okChars[newNum];


        }

        // make sure to add end of string character to original array
        original[i] = '\0';
}

// this function will be called when a otp_dec requests the server to decrypt via a child fork
void childProcess(int port)
{
	// varibales needed for receiving data from client
	int written = 0;

	int bufferTotal = 0;	
	int bufferLength = 0;

	int recvChar = 0;
	int recvChar2 = 0;

	char buffer[200000];
	memset(buffer, '\0', sizeof(buffer));

	char buffer2[200000];
	memset(buffer2, '\0', sizeof(buffer2));	

	char cBuffer[200000];
	memset(cBuffer, '\0', sizeof(cBuffer));

	char keyBuffer[200000];
	memset(keyBuffer, '\0', sizeof(keyBuffer));

	while(1) {
		// receive characters from client via socket
		recvChar = recv(port, &buffer[bufferTotal], 200000, 0);
		bufferLength = strlen(buffer); // need to check for terminating character in while loop

		// TESTING
		//printf("bufferLength: %i\n", bufferLength);
		//fflush(stdout);
		//printf("recvChar: %i\n", recvChar);
		//fflush(stdout);

		bufferTotal += recvChar;
	
		// TESTING
		//printf("buffer so far: %s\n", buffer);
		//fflush(stdout);

		// print message if receiving via socket error 
		if (recvChar < 0)
		{
			fprintf(stderr,"ERROR receiving from socket\n");
			fflush(stdout);
			//exit(1);
		}
		
		// break if buffer is relatively short - may speed up process
		if(bufferLength < 1000)
		{
			break;
		}
	}

	// TESTING
	//printf("bufferTotal: %i\n", bufferTotal);
	//printf("here 11f bufferTotal: %i\n", bufferTotal);
	//fflush(stdout);	

	// now go through the buffer and save each character as either ciphertext or keytext
	int saveToC = 0;
	int a = 0;
	int b = 0;
	int c = 0;
	for(a = 0; a < bufferTotal; a++)
	{
		// % signals the end of ciphertext
		// if % is found, then will switch to saving to the key buffer
		if(buffer[a] == '%' && saveToC == 0)
		{
			saveToC = 1;
		}

		// if % hasn't been found yet, save character to cipher buffer
		else if(saveToC == 0)
		{
			cBuffer[b] = buffer[a];
			b++;
		}

		// if % has been found, save character to key buffer
		else if(saveToC == 1)
		{
			keyBuffer[c] = buffer[a];
			c++;
		}

		// if ^ is found, that signals the end of the key characters
		else if(buffer[a] == '^')
		{
			break;
		}
	}
	
	// TESTING
	//printf("plainBuffer: %s\n", plainBuffer);
	//fflush(stdout);
	//printf("keyBuffer: %s\n", keyBuffer);
	//fflush(stdout);

	// decrypt the ciphertext using the keytext in the decrypt function
	decrypt(cBuffer, keyBuffer);

	// TESTING TO MAKE SURE THAT CORRECT TEXT IS PASSED BACK
	//printf("plainBuffer: %s\n", plainBuffer);
	//fflush(stdout);

	// now send back to client so client can print out the decrypted text
	int sent;
	sent = send(port, cBuffer, strlen(cBuffer), 0);

	// if ciphertext not successfuly sent, print error message
	if(sent < 0)
	{
		fprintf(stderr,"ERROR writing to socket\n");
		fflush(stdout);
		exit(1);
	}

	// close socket
	close(port);
}

void error(const char *msg) 
{ 
	perror(msg); 
	exit(1); 
} // Error function used for reporting issues


int main(int argc, char *argv[])
{
	// setting up variables for sockets, listening, reading, forking
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[200000];
	struct sockaddr_in serverAddress, clientAddress;
	pid_t pid;
	int childNum = 0;
	int childExit = -5;

	// Check usage & args - see if there's two arguments
	if (argc != 2) 
	{ 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		fflush(stdout);
		exit(1); 
	}

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string

	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);

	// error message if socketing up socket didn't work
	if (listenSocketFD < 0) 
	{
		fprintf(stderr,"ERROR opening socket\n");
		fflush(stdout);
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	{	
		fprintf(stderr, "ERROR binding socket\n");
		fflush(stdout);
		exit(1);
	}

	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// remember that this program will be running in background until stopped
	while(1)
	{

	// while loop here because 5 children at a time
	// different from number of processes queuing on listening socket ***

	// lower number of children if any have died since	

		while(( pid = waitpid(-1, &childExit, WNOHANG)) > 0)
		{
			childNum = childNum -1;
		}

		// double check that less than five children have been forked
		if(childNum < 5)
		{
			// Accept a connection, blocking if one is not available until one connects
			// call accept() to generate socket used for communication
			sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
			establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept

			// print error message if accept() didn't work
			if (establishedConnectionFD < 0) 
			{
				fprintf(stderr, "ERROR accepting connection\n");
				fflush(stdout);
				exit(1);
			}

		// accept and THEN fork - handle the rest of transaction on newly accepted socket
		// error fork, child fork (call function), parent fork (default, count child processes)

			pid_t newPid = -5;
			newPid = fork();

			switch(newPid)
			{
				// error fork, print message
				case -1:
				{
					fprintf(stderr, "ERROR with forking\n");
					fflush(stdout);
					exit(1);
				}

				case 0:
				{
					// the child fork does the decrpytion work
					childProcess(establishedConnectionFD);
				}

				default:
				{
					// parent fork will increment children counter
					childNum = childNum + 1;
				}
			}

		}
		
		else
		{
			// call waitpid there are more than five children
			if ((pid = waitpid(-1, &childExit, 0)) > 0)
			{
				childNum = childNum + 1;
			}
		}	
	}		

		close(listenSocketFD); // Close the listening socket	

	return 0;
}

