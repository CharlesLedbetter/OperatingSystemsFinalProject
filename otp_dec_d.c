/*********************************************************************
** Program name: otp_dec_d (HW4)
** Author: Charles Ledbetter
** Date: 3/14/2018
** Description: The server side of the decryption program. This
program acts as a daemon that listens on a particular port for data
from the server side decryption program. It will only return decrypted
data to the client side partner of this program. This program takes
the message and decryption key and forks a child to decrypt the
message and send it back to the client. the child then terminates, but
the parent stays running until send a kill signal.
*********************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<netinet/in.h>

//main
int main(int argc, char *argv[]){
	/********Variable declarations and initiations********/
	//for loop control
	int i = 0;

	//used to hold the integer addresses of the port and socket
	int portNumber = 0;
	int listenSocketFD = 0;
	int establishedConnectionFD = 0;

	//used to open a socket for listening
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;

	//used to insure all chars read and written
	int charsRead = 0;
	int charsWritten = 0;
	int checkSend = -5;

	//the buffer used to capture the message from over the network
	char recBuff[160010];

	//used to check that opt_dec (not opt_enc) is the contacting client
	char cArg[10];
	char* decName = "otp_dec";

	//used to parse data from recBuff decode it and send it back
	char mesBuff[80000];
	char keyBuff[80000];

	//for tracking / reading into buffer
	char* insert = NULL;
	int left = 0;

	//used to decode mesBuff
	int decoded = 0;

	//pid for fork processes up to 5 forks can run at once
	int pidStatus;
	pid_t decPids[5];
	for(i = 0; i < 5; i++){
    decPids[i] = -5;
  }

	/********End variable declarations and initiations********/

	// Check usage & number of args
	if(argc < 2){
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		exit(0);
	}

	//********Set up server port and socket********//

	// Clear out the address struct
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));

	// Get the port number, convert to an integer from a string
	portNumber = atoi(argv[1]);

	// Create a network-capable socketand store the port number
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);

	//if there was an error tell the user
	if(listenSocketFD < 0){
		fprintf(stderr,"ERROR opening socket to port %d\n", portNumber);
		exit(1);
	}

	// Enable the socket to begin listening
	if(bind(listenSocketFD, (struct sockaddr *)&serverAddress,
	sizeof(serverAddress)) < 0){
		fprintf(stderr,"ERROR on binding port %d\n", portNumber);
		exit(1);
	}

	// Flip the socket on - it can now receive up to 5 connections
	listen(listenSocketFD, 5);

	//********End set up server port and socket********//

	//********Receive and parse data from sockets********//

	// Accept a connection, blocking if one is not available until one connects
	while(1){
		// check every background process to see if any have ended
		for(i = 0; i < 5; i++){

			//if the one being checked has ended call waitpid on it
			if(decPids[i] != -5){

				//if the child exited get code and set exited
				if(waitpid(decPids[i], &pidStatus, WNOHANG)){
					decPids[i] = -5;
				}
			}
		}

		// Get the size of the address for the client that will connect
		sizeOfClientInfo = sizeof(clientAddress);
		establishedConnectionFD = accept(listenSocketFD,
			(struct sockaddr *)&clientAddress, &sizeOfClientInfo);

		// find a decPid not in use and fork
		for(i = 0; i < 5; i++){
			if(decPids[i] == -5){
				decPids[i] = fork();

				//if the fork failed throw an error
        if(decPids[i] < 0 || establishedConnectionFD < 0){
					fprintf(stderr,"ERROR on accept\n");
					exit(1);
        }


				//child decrypts message
				else if(decPids[i] == 0){

					// clear the buffer
					memset(recBuff, '\0', sizeof(recBuff));


					// Read the client's message from the socket
					charsRead = 0;
					insert = &recBuff[0];
					do {
						left = 160010 - charsRead;
						insert += charsRead;
						charsRead += recv(establishedConnectionFD, insert, left, 0);

						//if an error occurred tell the user
						if (charsRead < 0){
							//send a message back to the client to terminate it
							memset(recBuff, '1', sizeof(mesBuff));
							send(establishedConnectionFD, mesBuff, 80000, 0);
							exit(2);
						}
					} while(charsRead < 160010);

					insert = NULL;

					//clear out buffers in order to load from socket
					memset(cArg, '\0', sizeof(cArg));
					memset(mesBuff, '\0', sizeof(mesBuff));
					memset(keyBuff, '\0', sizeof(keyBuff));

					//go through the incoming message one char at a time and parse
					for(i = 0; i < 160010; i++){

						//the first 10 chars from recBuff go to cArg
						//if the element is a '0' insert a \0
						//otherwise insert from recBuff
						if(i < 10){
							if(recBuff[i] == '0'){
								cArg[i] = '\0';
							}
							else{
								cArg[i] = recBuff[i];
							}
						}

						//the next 80000 chars from recBuff go to mesBuff
						//if the element is a '0' insert a \0
						//otherwise insert from recBuff
						else if(i < 80010){
							// if(recBuff[i] == '0'){
							// 	mesBuff[i-10] = '\0';
							// }
							// else{
								mesBuff[i-10] = recBuff[i];
							//}
						}

						//the next 80000 chars from recBuff go to keyBuff
						//if the element is a '0' insert a \0
						//otherwise insert from recBuff
						else{
							// if(recBuff[i] == '0'){
							// 	keyBuff[i-80010] = '\0';
							// }
							// else{
								keyBuff[i-80010] = recBuff[i];
							// }
						}
					}

					//if cArg is otp_dec tell the client
					if(strcmp(cArg, decName) == 0){
						for(i = 0; i < strlen(mesBuff); i++){

							// if the end was reached
							if(mesBuff[i] == '0'){
								mesBuff[i] = '0';
							}

							//decode the message
							else if(mesBuff[i] == 32 || (mesBuff[i] > 64 && mesBuff[i] < 91)){
								//if a space is found in mesBuff shift it
								if(mesBuff[i] == 32){
									mesBuff[i] = 91;
								}

								//if a space is found in keyBuff shift it
								if(keyBuff[i] == 32){
									keyBuff[i] = 91;
								}

								//do the one time pad math
								decoded = (((mesBuff[i] - 65) - (keyBuff[i] - 65)) % 27) + 65;

								//adjust for number under 0
								if(decoded < 65){
									decoded += 27;
								}

								//if new char is ']' shift to a space
								if(decoded == 91){
									decoded = 32;
								}

								//load the current element of mesBuff with the new char
								mesBuff[i] = (char)decoded;
							}

							//for debugging incorrect decryption
							else{
								mesBuff[i] = '#';
							}
						}
					}

					//if cArg is not otp_dec tell the client
					else{
						mesBuff[0] = '@';
					}

					//********End receive and parse data from sockets********//

					//********Send decrypted data back to client********//

					// Send message back to client
					charsWritten = 0;
					insert = &mesBuff[0];
					do {
						left = 80000 - charsWritten;
						insert += charsWritten;
						charsWritten += send(establishedConnectionFD, insert, left, 0);

						//if there was an error tell the user
						// if (charsWritten < 0){
						// 	fprintf(stderr,"SERVER: ERROR writing to socket %d\n", portNumber);
						// 	exit(2);
						// }
					}while(charsWritten < 80000);

					insert = NULL;

					// Loop forever until send buffer for this socket is empty
					checkSend = -5;
					do {
						// Check the send buffer for this socket
						ioctl(establishedConnectionFD, TIOCOUTQ, &checkSend);
					}
					while (checkSend > 0);

					//********End send decrypted data back to client********//

					// Close the existing socket which is connected to the client
					close(establishedConnectionFD);

					//exit the child program
					exit(0);
				}

				//the parent can return to the endless server loop
				//but should check for zombies first
				else if(decPids[i] > 0){
					// check every background process to see if any have ended
					for(i = 0; i < 5; i++){
						//if the one being checked has ended call waitpid on it
						if(decPids[i] != -5){

							//if the child exited get code and set exited
							if(waitpid(decPids[i], &pidStatus, WNOHANG)){
								decPids[i] = -5;
							}
						}
					}
					//if this is a parent to a forked child
					break;
        }
				//if a fork occurred the parent can break
				break;
			}
		}
	}

	// Close the listening socket
	close(listenSocketFD);
	return 0;
}
