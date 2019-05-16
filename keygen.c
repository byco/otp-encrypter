// This program works in conjunction with otp_enc_d, otp_enc, otp_dec_d, and otp_dec, in order
// to create a one-time pad system that encrypts and decrypts files. Keygen generates a key
// that can be used to decrypt or encrypt.

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
	// check argument length
	if(argc != 2)
	{
		printf("The syntax for keygen: keygen keylength. Please enter two arguments.\n");
		exit(0);
	}

	// note that syntax is keygen keylength in command line
	// this variable will store the keylength number passed as the 2nd command line argument
	int keylength = atoi(argv[1]);

	// need numbers to be randomly generated
	srand(time(0));
	int i;

	// generate valid characters to make up a key string as long as what the user specified
	for(i = 0; i < keylength; i++)
	{
		// there are 27 possible characters (uppercase alphabetical characters + a space)
		// add 65 so it can correlate to ascii table
		int newLetter = rand() % 27 + 65;
		
		// if number happens to be 91, we'll make it a space character
		if(newLetter == 91)
		{
			printf(" ");
		}

		// otherwise print the character equivalent of the number
		else
		{
			printf("%c", newLetter);
		}
	}

	// print end of string character (as specified in instructions)
	printf("\n");
		
	return 0;
}
