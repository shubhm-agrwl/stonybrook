#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 5984
#define BUFF_SIZE 4096

int main(int argc, const char *argv[])
{
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[BUFF_SIZE] = {0};
	char *hello = "Hello from server";

	/* [S1]
	 * Explaint the following here.
	 * This creates a socket file descriptor which returns -1 if the socket connection is not created properly.
	 * It calls a socket method which takes 3 arguments: domain - AF_INET ensures an IPV4 protocol, type - SOCK_STREAM ensures a TCP (handshake) communication,
	 * third parameter is a protocol value which is visible in the IP packet header 
	 */
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* [S2]
	 * Explaint the following here.
	 * This is optional but handles cases where port in which the connection is to be established is already in use. 
	 * In such cases, it reuses the port, thus handling failures with respect to the socket descriptor created above.
	 */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		       &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* [S3]
	 * Explaint the following here.
	 * Assigns the properties to the address.
	 * Assigns IPV4 protocol, localhost address and port defined in line 8 of the same program
	 */
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	/* [S4]
	 * Explaint the following here.
	 * The socket descriptor created above gets bind to the address and port.  
	 * Returns -1 if the bind fails
	 */
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* [S5]
	 * Explaint the following here.
	 * After creating the socket connection, it listens to the port it has binded to. It keeps on listening for any incoming client requests.
	 * listen has 2 parameters - socket descriptor and backlog, which is the maximum number of pending socket requests which this server process can handle.
	 * Returns -1 if listen fails
	 */
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* [S6]
	 * Explaint the following here.
	 * As soon as the client sends request to the server, it accepts the request which is on top of the queue. 
	 * It returns a new file descriptor which is used to communicate and transfer data.
	 * Returns -1 if the accept method fails
	 */
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
				 (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* [S7]
	 * Explaint the following here.
	 * C code to print a message to press any key. getChar() is used to expect some input data to proceed forward.
	 * It is not assigned to any variable as the program is not dependent on the key which the user is pressing.
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [S8]
	 * Explaint the following here.
	 * As soon as the message is received from the client, the below method is called, which has a pre-defined buffer size of 1024. 
	 * Returns -1 if the read method fails
	 */
	if (read( new_socket , buffer, 1024) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("Message from a client: %s\n",buffer );

	/* [S9]
	 * Explaint the following here.
	 * In order to send message to the client, send method is used.
	 */
	send(new_socket , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");
	return 0;
}
