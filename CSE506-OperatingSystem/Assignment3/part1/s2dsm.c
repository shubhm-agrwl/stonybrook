#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/mman.h>

#define BUFF_SIZE 4096

static int page_size;

int main(int argc, const char *argv[])
{
    // initalizing socket file descriptors
    int local_listen_fd, remote_send_fd, new_socket;

    struct sockaddr_in address_listen;
    struct sockaddr_in address_send;
    int opt = 1;
    int addrlen = sizeof(address_listen);
    char buffer[BUFF_SIZE] = {0};

    // Assuming that the input is integer port number

    // Taking local port to listen
    int local_listen_port = atoi(argv[1]);

    // Taking remote port to send message
    int remote_send_port = atoi(argv[2]);
    unsigned long len;

    // Flag to check whether remote connection is made or not
    int is_connected = 0;
    char *addr;

    // Variable to store the number of pages input from the user
    char input_pages[10];
        
    // Printing the local and remote port being used
    printf("Local Port: %d\n", local_listen_port);
    printf("Remote Port: %d\n", remote_send_port);

    // Creating local socket file descriptor
    if ((local_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
        
    if (setsockopt(local_listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
        
    address_listen.sin_family = AF_INET;
    address_listen.sin_addr.s_addr = INADDR_ANY;

    // Assigning local port to listen to
    address_listen.sin_port = htons( local_listen_port );
    address_send.sin_family = AF_INET;
    address_send.sin_addr.s_addr = INADDR_ANY;

    // Assigning remote port to send address to
    address_send.sin_port = htons( remote_send_port );
        
    if (bind(local_listen_fd, (struct sockaddr *)&address_listen, sizeof(address_listen)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
        
    if (listen(local_listen_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
         
    // Creating remote socket file descriptor               
    if ((remote_send_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    if(inet_pton(AF_INET, "127.0.0.1", &address_send.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
                  
    /* 
    Process trying to connect to the server to send the message to
    If this is the first process, it will not able to connect since
    the port ii is trying to connect to is not opened
    If this is the second process, it will connect successfully as 
    the port is opened to be connected
    */
    if (connect(remote_send_fd, (struct sockaddr *)&address_send, sizeof(address_send)) < 0) {
        printf("\nConnection Failed as Remote process is not started yet. Please start the other process to continue..\n");
    } else {
        printf("\nConnection to remote process is established\n");
        is_connected = 1;
        printf("\nWaiting for messages from the remote process\n");
    }

    // Accepting connection to the local listen port
                  
    if ((new_socket = accept(local_listen_fd, (struct sockaddr *)&address_listen,(socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // If the client connection failed in the first attempt above. First process
    // will run this piece of code
    if (is_connected == 0) {

        if ((remote_send_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }

        // Trying to connect with the remote port again
        if (connect(remote_send_fd, (struct sockaddr *)&address_send, sizeof(address_send)) < 0) {
            printf("\nConnection Failed \n");
        } else {
            printf("Connection to remote process is established\n");
        }

        // User Input
        printf("How many pages you would like to allocate (greater than 0)?\n");
        int input = scanf("%s", input_pages);
        
        if (input == 0)
        {
                printf("Error while taking input from user");
                return -1;
        }

        // Converting input to integer value
        int pages = atoi(input_pages);

        // Getting the system page size
        page_size = sysconf(_SC_PAGE_SIZE);

        // Cacluating the page size to be allocated
        len = pages * page_size;

        // Converting len to string value to send the message across socket connection
        char string_len[len+1];
        snprintf(string_len, len+1, "%lu", len);

        // Printing the mmaped size
        printf("\nAllocating memory for mmapped size: %s\n", string_len);

        // Allocating MMap for the given size
        addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED)
            return -1;

        printf("Address returned by mmap() = %p\n", addr);

        char message[80];
        char string_addr[50];

        // Converting address of char pointer to String
        snprintf(string_addr,50,"%p",addr);

        // Concatenating message (size and address) to be sent
        snprintf(message, sizeof message, "%s%s%s", string_addr, ",", string_len);

        printf("\nMessage sent to Process 2: %s\n", message);

        // Sending Socket Message
        send(remote_send_fd , message , sizeof(message) , 0 );

    } else {

        // If the client was connected in the first attempt, 
        // it will read from the remote socket. i.e. Process 2
        if (read( new_socket , buffer, 1024) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("\nMessage from Process 1: %s\n",buffer );

        // Splitting incoming message based on delimeter - ','
        // Finding address
        char *address = strtok(buffer, ",");

        // Finding length
        char *length = strtok(NULL, "");

        printf("Process 1 Mmaped Address: %s\n", address);
        printf("Process 1 Mmaped length: %s\n", length);

        // Converting string address to char pointer
        sscanf(address, "%p", &addr);

        // Converting length to long
        len = strtoul(length, NULL, 0);

        // Allocating address with the same memory region and address length
        addr = mmap(addr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED)
            return -1;

        // Printing the Mmap region and size for Process 2
        printf("Address returned by process 2 mmap(): %p\n", addr);  
        printf("Length allocated for process 2 mmap: %ld\n", len);      
        
    }
    return 0;
}
