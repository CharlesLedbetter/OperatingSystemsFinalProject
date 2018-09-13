/*********************************************************************
** Program name: otp_enc (HW4)
** Author: Charles Ledbetter
** Date: 3/14/2018
** Description: The client side of the one time pad encryption Program
This program takes a message and a key and sends it over a port to the
server side encryption program. The server then sends back encrypted
data. The data is then checked to make sure it is the same length as
the original message. If it is the encrypted message is printed to the
screen. If it is not, the process starts again sending the message
until a correctly encrypted message returns.
*********************************************************************/
#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/ioctl.h>
#include<netdb.h>

//main
int main(int argc, char *argv[]){
	/********Variable declarations and initiations********/
	//for loop control
	int i = 0;

	//holds integer addresses of port and socket
	int socketFD = 0;
	int portNumber = 0;

	//used for socket connections and communication
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;

	//used for insuring messages were sent over the network correctly
	int charsWritten = -5;
	int charsRead = -5;
	int mesError = 0;
	int readLoops = 0;
	int finalCharCount = 0;
	int connected = 0;

	//buffers used to parse input files
	char mesBuff[80000];
	char keyBuff[80000];
	int mesLength = 80000;
	int keyLength = 80000;
	char arg[10];
	strcpy(arg, argv[0]);

	//buffer used for combining other buffers to send over the network
	char sendBuff[160010];

	//buffer used for receiving return messages
	char retBuff[80000];

	//for tracking / reading into buffer
	char* insert = NULL;
	int left = 0;

	//for opening input files
	FILE* messageIn;
	FILE* keyIn;


	/********End variable declarations and initiations********/

	//********Verify input********//

	// Check usage & number of args
	if(argc < 4){
		fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
		exit(0);
	}

	//loop until message sent and received correctly
	do {
		mesError = 0;

		//open the files
		messageIn = fopen(argv[1], "r");
		keyIn = fopen(argv[2], "r");

		//insure files opened
		if(messageIn == NULL || keyIn == NULL){
			fprintf(stderr,"One or both files could not be opened\n");
			exit(1);
		}

		/*	I considered refactoring the next 2 forloops into the
		/		forloop in the following section, that is used to create
		/		the sendBuff. I decided against this to have better
		/		separation of concerns */

		//load the mesBuff from file while checking for invalid characters
		memset(mesBuff, '\0', sizeof(mesBuff));
		for(i = 0; i < sizeof(mesBuff); i++){
			mesBuff[i] = fgetc(messageIn);

			//when \n is reached capture length and break
			if(mesBuff[i] == '\n'){
				mesLength = i;
				break;
			}

			//check every character to see if it is in the valid list of
			//characters. If not throw error and exit
			if(mesBuff[i] > 90 || (mesBuff[i] < 65 && mesBuff[i] != 32)){
				fprintf(stderr,"Invalid data for encryption message\n");
				exit(1);
			}
		}

		//load keyBuff from file while checking for invalid characters
		memset(keyBuff, '\0', sizeof(keyBuff));
		for(i = 0; i < sizeof(keyBuff); i++){
			keyBuff[i] = fgetc(keyIn);

			//when \n is reached capture length and break
			if(keyBuff[i] == '\n'){
				keyLength = i;
				break;
			}

			//check every character to see if it is in the valid list of
			//characters. If not throw error and exit
			if(keyBuff[i] > 90 || (keyBuff[i] < 65 && keyBuff[i] != 32)){
				fprintf(stderr,"Invalid data for encryption key\n");
				exit(1);
			}
		}

		//insure that the input key was at least as long as the input message
		if(mesLength > keyLength){
			fprintf(stderr,"Error: key %s is too short\n", argv[2]);
			exit(1);
		}

		//********End verify input********//

		//********Parse data into buffers********//

		//remove /n from the three input buffers
		mesBuff[strcspn(mesBuff, "\n")] = '\0';
		keyBuff[strcspn(keyBuff, "\n")] = '\0';
		arg[strcspn(arg, "\n")] = '\0';

		//parse the input buffers into a buffer to send across the network
		//note that all 0 are parsed into \0 at the server
		memset(sendBuff, '\0', sizeof(sendBuff));
		for(i = 0; i < 160010; i++){

			//the first 10 chars are from arg
			//if the element is blank insert a 0
			//otherwise insert from arg
			if(i < 10){
				if(arg[i] == '\0'){
					sendBuff[i] = '0';
				}
				else{
					sendBuff[i] = arg[i];
				}
			}

			//the nest 80000 chars are from mesBuff
			//if the element is blank insert a 0
			//otherwise insert from mesBuff
			else if(i < 80010){
				if(mesBuff[i-10] == '\0'){
					sendBuff[i] = '0';
				}
				else{
					sendBuff[i] = mesBuff[i-10];
				}
			}

			//the nest 80000 chars are from keyBuff
			//if the element is blank insert a 0
			//otherwise insert from keyBuff
			else{
				if(keyBuff[i-80010] == '\0'){
					sendBuff[i] = '0';
				}
				else{
					sendBuff[i] = keyBuff[i-80010];
				}
			}
		}

		//********End parse data into buffer********//

		//close files
		fclose(messageIn);
		fclose(keyIn);



		//********Set up the server address struct********//

		// Clear out the address struct
		memset((char*)&serverAddress, '\0', sizeof(serverAddress));

		// Get the port number, convert to an integer from a string
		portNumber = atoi(argv[3]);

		// Create a network-capable socket
		serverAddress.sin_family = AF_INET;

		// Store the port number
		serverAddress.sin_port = htons(portNumber);

		// Convert the machine name into a special form of address
		serverHostInfo = gethostbyname("localhost");
		if(serverHostInfo == NULL){
			fprintf(stderr, "CLIENT: ERROR, no such host\n");
			exit(0);
		}

		// Copy in the address
		memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);


		// Set up and create the socket
		socketFD = socket(AF_INET, SOCK_STREAM, 0);
		if(socketFD < 0){
			fprintf(stderr,"CLIENT: ERROR opening socket %d\n", portNumber);
			exit(2);
		}

		// Connect to server
		do {
			connected = 0;
			if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
				fprintf(stderr,"CLIENT: ERROR connecting to socket %d. Trying again\n", portNumber);
				connected = 1;
				sleep(1);
			}
		} while(connected == 1);


	//********End set up the server address struct********//

	//********Contact server and send message********//

		// Send message to server
		charsWritten = 0;
		insert = &sendBuff[0];
		do {
			left = 160010 - charsWritten;
			insert += charsWritten;
			charsWritten += send(socketFD, insert, left, 0);

			//if an error occurred tell user
			if (charsWritten < 0){
				fprintf(stderr,"CLIENT: ERROR writing to socket %d\n", portNumber);
				exit(2);
			}
		}while(charsWritten < 160010);

		insert = NULL;

		//********End contact server and send message********//

		//********Receive message from server and print********//

		// Get return message
		memset(retBuff, '\0', sizeof(retBuff));
		charsRead = 0;
		insert = &retBuff[0];
		readLoops = 0;
		do {
			readLoops++;
			left = 80000 - charsRead;
			insert += charsRead;
			charsRead += recv(socketFD, insert, left, 0);

			// if error occurred tell user
			// if(charsRead < 0){
			// 	fprintf(stderr,"CLIENT: ERROR reading from socket %d\n", portNumber);
			// 	exit(2);
			// }

			//if the opt_enc_d server is not the server contacted throw an error
			if(retBuff[0] == '@'){
				fprintf(stderr,"ERROR opt_enc must not connect to opt_dec_d\n");
				exit(2);
			}

			//if the opt_enc_d server did not receive the message exit
			if(readLoops > 100 || retBuff[0] == '1' ||
			(retBuff[0] == '\0' && charsRead != 0)){
				mesError = 1;
				charsRead = 80001;
			}
		} while(charsRead < 80000);

		insert = NULL;

		//print the encrypted message to the console
		if(mesError == 0){
			finalCharCount = 0;
			for(i = 0; i < 80000; i++){
				if(retBuff[i] == '0' || retBuff[i] == '\0'){
					retBuff[i] = '\0';
				}
				else{
					finalCharCount++;
				}
			}

			if(finalCharCount >= mesLength){
				printf("%s\n", retBuff);
			}
		}

		if(finalCharCount < mesLength){
			mesError = 1;
			// Close the socket
			close(socketFD);
			sleep(1);
		}

	//********End receive message from server and print********//

	} while(mesError != 0);

	// Close the socket
	close(socketFD);

	return 0;
}
