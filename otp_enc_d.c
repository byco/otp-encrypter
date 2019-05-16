// This program should run in the background like a daemon and will encrypt a plaintext file, 
// given the file name and another file with a key. It will encrypt and return ciphertext
// when requested by a client program (otp_enc).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

// function to ecrypt original message using a key
// characters in original will be altered into ciphertext
void encrypt(char original[], char key[])
{
        int length = strlen(original);
        int plainNum;
        int keyNum;
        int newNum;
        // the following chars are acceptable in ciphertext: uppercase alphabetical characters and space
        char okChars[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

        // go over each letter in the original and key arrays
        // figure out its equivalent in okChars
        int i;
        for (i = 0; i < length; i++)
        {
                int j;
                for(j = 0; j < 27; j++)
                {
                        if(original[i] == okChars[j])
                        {
                                plainNum = j;
                        }
                }

                int k;
                for(k = 0; k < 27; k++)
                {
                        if(key[i] == okChars[k])
                        {
                                keyNum = k;
                        }
                }

                // newNum is the letter # for the ciphertext
                // get the ciphertext number by adding the number equivalents of plaintext and key
                newNum = plainNum + keyNum;

                // newNum may be over 26 so subtract to make sure it's in the range of 0-26
                // this allows it to be assigned the correct letter or space from okChars
                while(newNum > 26)
                {
                        newNum = newNum - 27;
                }

                // TESTING
                //printf("plainNum: %i\n", plainNum);
                //printf("plainChar: %c\n", okChars[plainNum]);
                //printf("keyNum: %i\n", keyNum);
                //printf("keyChar: %c\n", okChars[keyNum]);
                //printf("newNum: %i\n", newNum);
                //printf("newChar: %c\n", okChars[newNum]);

                // now assign new letter to original array
                original[i] = okChars[newNum];


        }

        // make sure end of string character to original array
        original[i] = '\0';
}

// this function will be called when otp_enc connects with otp_enc_d and requests ciphertext
void childProcess(int port)
{
	// variables needed to receive characters from otp_enc
	int written = 0;

	int bufferTotal = 0;	
	int bufferLength = 0;

	int recvChar = 0;
	int recvChar2 = 0;

	// create and clear buffers for data from otp_enc
	char buffer[200000];
	memset(buffer, '\0', sizeof(buffer));

	char buffer2[200000];
	memset(buffer2, '\0', sizeof(buffer2));	

	char plainBuffer[200000];
	memset(plainBuffer, '\0', sizeof(plainBuffer));

	char keyBuffer[200000];
	memset(keyBuffer, '\0', sizeof(keyBuffer));


	while(1) {

		recvChar = recv(port, &buffer[bufferTotal], 200000, 0);
		bufferLength = strlen(buffer); // need to check for terminating character in while loop

		bufferTotal += recvChar;
	
		// TESTING
		//printf("buffer so far: %s\n", buffer);
		//fflush(stdout);

		// if nothing is received from the receiving port, print error message
		if (recvChar < 0)
		{
			fprintf(stderr,"ERROR receiving from socket\n");
			fflush(stdout);
			//exit(1);
		}

		// break if next buffer iteration is less than 1000. speeds up process.
		if(bufferLength < 1000)
		{
			break;
		}
	}

	int saveToPlain = 0;

	// now go over the buffer text and save each character to either plaintext or keytext
	int a = 0;
	int b = 0;
	int c = 0;
	for(a = 0; a < bufferTotal; a++)
	{

		// the % character is used in otp_enc to denote the end of plaintext
		// if % found, then switch to saving to keytext instead
		if(buffer[a] == '%' && saveToPlain == 0)
		{
			saveToPlain = 1;
		}

		// if % hasn't been found yet, then keeping saving characters to plaintext
		else if(saveToPlain == 0)
		{
			plainBuffer[b] = buffer[a];
			b++;
		}

		// if % has been found, then save characters as keytext
		else if(saveToPlain == 1)
		{
			keyBuffer[c] = buffer[a];
			c++;
		}

		// if ^ is found, that denotes the end of keytext
		else if(buffer[a] == '^')
		{
			break;
		}
	}

	// encrypt the plaintext in separate function
	encrypt(plainBuffer, keyBuffer);

	// TESTING TO SEE IF CORRECT BUFFER HAS BEEN ALTERED
	//printf("plainBuffer: %s\n", plainBuffer);
	//fflush(stdout);

	// now send back ciphertext to client. note that keytext is not sent as well.
	int sent;
	sent = send(port, plainBuffer, strlen(plainBuffer), 0);

	// print error message if nothing is sent back to client
	if(sent < 0)
	{
		fprintf(stderr,"ERROR writing to socket\n");
		fflush(stdout);
		exit(1);
	}

	// close current socket
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

	// print error message if socket cannot be opened
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
				// error case
				case -1:
				{
					fprintf(stderr, "ERROR with forking\n");
					fflush(stdout);
					exit(1);
				}

				// child fork - use new socket to do the encryption and sending work
				case 0:
				{
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