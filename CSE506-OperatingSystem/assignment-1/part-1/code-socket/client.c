#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 5984
#define BUFF_SIZE 4096

int main(int argc, const char *argv[])
{
	int sock = 0;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	char buffer[BUFF_SIZE] = {0};

	/* [C1]
	 * Explaint the following here.
	 * This creates a socket file descriptor which returns a negative number if the socket connection is not created properly.
	 * It calls a socket method which takes 3 arguments: domain - AF_INET ensures an IPV4 protocol, type - SOCK_STREAM ensures a TCP (handshake) communication,
	 * third parameter is a protocol value which is visible in the IP packet header 
	 */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	/* [C2]
	 * Explaint the following here.
	 * Defines the properties of the address. memset assigns the size of address to serv_address starting from 0
	 * assigns IPV4 protocol and assigns the port defined in line 9 of the same program.
	 */
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	/* [C3]
	 * Explaint the following here.
	 * Convert the address mentioned to binary form. It returns 0 if the address is invalid or -1 if any other error in the arguments.
	 */
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	/* [C4]
	 * Explaint the following here.
	 * Connect is used by the client to connect with the server given the file descriptor and correct address.
	 * Returns -1 if unsuccessful or 0 if successful
	 */
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}


	/* [C5]
	 * Explaint the following here.
	 * C code to print a message to press any key. getChar() is used to expect some input data to proceed forward.
	 * It is not assigned to any variable as the program is not dependent on the key which the user is pressing.
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [C6]
	 * Explaint the following here.
	 * Send method is used to send message to the server using the socket created above, along with the size and the starting address
	 */
	send(sock , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");

	/* [C7]
	 * Explaint the following here.
	 * Read method is invoked anytime a message is received from the server with a pre-defined buffer size of 1024
	 * Returns -1 if unsuccessful read happens
	 */
	if (read( sock , buffer, 1024) < 0) {
		printf("\nRead Failed \n");
		return -1;
    }
	printf("Message from a server: %s\n",buffer );
	return 0;
}
