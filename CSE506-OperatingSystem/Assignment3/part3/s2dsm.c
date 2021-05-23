#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFF_SIZE 4096

static int page_size;
char input_page[2];

/* Start of region handled by userfaultfd */
static char *addr;

enum msi_info {M, S, I};
enum msi_info msi_array[100];
int local_listen_fd, remote_send_fd, new_socket;

static void *
fault_handler_thread(void *arg)
{
    static struct uffd_msg msg;   /* Data read from userfaultfd */
    long uffd;                    /* userfaultfd file descriptor */
    static char *page = NULL;
    struct uffdio_copy uffdio_copy;
    ssize_t nread;

    uffd = (long) arg;

    if (page == NULL) {
        page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED)
            return;
    }

    for (;;) {

        /* See what poll() tells us about the userfaultfd */

        struct pollfd pollfd;
        int nready;

        pollfd.fd = uffd;
        pollfd.events = POLLIN;
        nready = poll(&pollfd, 1, -1);
        if (nready == -1)
            return;

       printf("[x] PAGEFAULT\n");

        nread = read(uffd, &msg, sizeof(msg));
        if (nread == 0) {
            printf("EOF on userfaultfd!\n");
            exit(EXIT_FAILURE);
        }

        if (nread == -1)
            return;

        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            fprintf(stderr, "Unexpected event on userfaultfd\n");
            exit(EXIT_FAILURE);
        }

        // Setting the page to null if a page fault happens
        memset (page, '\0', page_size);

        uffdio_copy.src = (unsigned long) page;
        uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
            ~(page_size - 1);
        uffdio_copy.len = page_size;
        uffdio_copy.mode = 0;
        uffdio_copy.copy = 0;

        if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
            return;
    }
}

// function to get the page number from user
int get_page(int pages)
{
    printf("> For which page? (0-%d, or -1 for all):\n", (pages-1));
    getchar();
    scanf("%s", input_page);
    return atoi(input_page);
}

// function to print a specific valid page
void handle_msi_print(int page_number, int write_flag)
{
    int i=0;
    char res[page_size];
    int msi_state;

    // Handling MSI State
    msi_state = msi_array[page_number];

    /*
    If write operation, send a socket message with write flag, to invalidate
    the other corresponding MSI Page. Change the MSI state of the current 
    process to "M".
    */
    if (write_flag == 1){
    
        char page_msg[2];
        char message[100];
        snprintf(page_msg, 2, "%lu", page_number);
        snprintf(message, 100, "%s%s", "w,", page_msg);
        sleep(1);
        send(remote_send_fd , message , 100 , 0 );
        msi_array[page_number] = M;
    
    } else if (write_flag == 0) {
      
        /*
        If it is a read operation, if the current MSI State is not in Shared: "S"
        state, send a socket message with Read - "r" flag to read the contents
        of the other process.
        */  
        if (msi_state != S) {
            char page_msg[2];
            char message[100];
            snprintf(page_msg, 2, "%lu", page_number);
            snprintf(message, 100, "%s%s", "r,", page_msg);
            sleep(1);
            send(remote_send_fd , message , 100 , 0 );      
        } 
    }

    // Printing the page if it is not in Invlid - "I" state
     if (msi_array[page_number] != I) {
        for (i=0;i<page_size; i++)
        {
            char c = addr[(page_number * page_size) + i];
            res[i] = c;
        }
        printf("[*] Page %d:\n%s\n", page_number, res);
     }
    
}

// function to print all the pages
void print_all_pages(int pages, int write_flag)
{
	int j = 0;
	// Looping through all the pages
    for (j=0; j < pages; j++)
    {
        handle_msi_print(j, write_flag);
    }
}

void user_interaction(int pages)
{
    char input_command[1];
	char input_message[4096];
	char buffer[BUFF_SIZE] = {0};
    int remote_process_msi_state;
    int page_input = 0;
    int i =0;
    int j=0;
    
	// Infinite loop to ask for User Input
    for (;;)
    {
        sleep(1);
        // User Input to ask for the command
        printf("> Which command should I run? (r:read, w:write, v:view msi array):\n");
        scanf("%s", input_command);

        // If read operation
        if (input_command[0] == 'r')
        {

            page_input = get_page(pages);

            // If -1 is entered by the user
            if (page_input == -1)
            {
			     // Printing all the pages
                 print_all_pages(pages, 0);
            
            } else if (page_input > -1 && page_input < pages)
            {
                // If valid page range
                handle_msi_print(page_input, 0); 	
            
            } else if (page_input <-1 || page_input >= pages)
            {
                // If invalid page
                printf("Invalid page\n");
                
            }

        } else if (input_command[0] == 'w')
        {
            // If write operation
            // Getting the page number
            page_input = get_page(pages);

            // If page input is -1, write to all the pages 
            if (page_input == -1)
            {
                printf("> Type your new message:\n");
                getchar();
                fgets(input_message, page_size, stdin);

                // Looping through all the pages
                for (j=0; j < pages; j++)
                {
                    i=0;
                    while (input_message[i] != '\0')
                    {
                        addr[((j*page_size)+i)] = input_message[i];
                        i++;
                    }
                }
            	print_all_pages(pages, 1);
                
            } else if (page_input > -1 && page_input < pages)
            {
                // If valid page
                printf("> Type your new message:\n");
                getchar();
                fgets(input_message, page_size, stdin);
                i =0;
                while (input_message[i] != '\0')
                {
                    addr[((page_input*page_size)+i)] = input_message[i];
                    i++;
                }
                    handle_msi_print(page_input, 1);
                
            } else if (page_input <-1 || page_input >= pages)
            {
                // if invalid page is entered by the user
                printf("Invalid page\n");
                
            }
        } else if (input_command[0] == 'v')
        {
            // Printing the contents when "v" is pressed
            char ch;

            page_input = get_page(pages);

            // If -1 is entered by the user
            if (page_input == -1)
            {
                for (i=0;i<pages;i++){
                    if (msi_array[i] == 0){
                        ch = 'M';
                    } else if (msi_array[i] == 1) {
                        ch = 'S';
                    } else {
                        ch = 'I';
                    }
                    printf("[*] Page %d:\n%c\n", i, ch);           
                }
            } else if (page_input > -1 && page_input < pages)
            {
                if (msi_array[page_input] == 0){
                    ch = 'M';
                } else if (msi_array[page_input] == 1) {
                    ch = 'S';
                } else {
                    ch = 'I';
                }
                printf("[*] Page %d:\n%c\n", page_input, ch);  
            } else if (page_input <-1 || page_input >= pages)
            {
                // If invalid page
                printf("Invalid page\n");
                
            }
        }
    }
}

/*
This new thread is implemented which constantly reads socket messages
and performs actions based on the flags of read/Write operations.
The operations are happening based on the 3 flags:
1. "w" - Write Request: Process A sends a write request to Process B.
        Process B madvises the address region and sets the MSI state to I
        for that particular page number
2. "r" - Read Request: Process A sends a read request to Process B.
        Process B reads the incoming page number. Process B produces a page fault
        if the memory is not allocated. If the MSI State is not 'I', it sends
        the response back to Process A with flag 'y' with the current MSI State
        and data. After sending the message, process B changes the state to 'S'.
3. "y" - Read Response: This is the response which Process B sends to process A
            upon the request sent by Process A earlier. If the response MSI state
            is not "I", it reads the data sent, and assigns it to its own page 
            defined region and displays it. If MSI state in response is "I", it
            does a page fault and prints a null string as defined by the page 
            fault handler thread.
*/
void *socket_read_thread(void *arg) 
{ 
    char buffer[BUFF_SIZE] = {0};
    int page_number;
    int msi_state;

    for (;;)
    {

        if (read( new_socket , buffer, 1024) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if (buffer != '\0') {
            
            char *operation = strtok(buffer, ",");
            char *page_num = strtok(NULL, "");
            page_number = strtoul(page_num, NULL, 0);
            if (*operation == 'w') {

                msi_state = msi_array[page_number];
                madvise(addr + (page_number * page_size), page_size, MADV_DONTNEED);
                msi_array[page_number] = I;
            
            } else if (*operation == 'r') {
            
                char res[page_size];
                int i;
                msi_state = msi_array[page_number];    
                char message[100];
                char msistate[2];
                snprintf(msistate, 2, "%lu", msi_state);
                
                if (msi_state != 2) {

                    for (i=0;i<page_size; i++)
                    {
                        char c = addr[(page_number * page_size) + i];
                        res[i] = c;
                    }   
                    snprintf(message, 100, "%s%s%s%s%s%s", "y,", page_num, ",", msistate, ",", res);
                    send(remote_send_fd , message , 100 , 0 );
                    msi_array[page_number] = S;
               
                } else {
                    
                    snprintf(message, 100, "%s%s%s%s%s", "y,", page_num, ",", msistate, ",");
                    send(remote_send_fd , message , 100 , 0 );
                
                }
            
            } else if (*operation == 'y') {
                
                char *pagenum = strtok(page_num, ",");
                page_number = strtoul(pagenum, NULL, 0);
                char *msi = strtok(NULL, ","); 
                int msi_st = strtoul(msi, NULL, 0);
                
                if (msi_st != 2) {
                    
                    char *data = strtok(NULL, ""); 
                    int i=0;
                    while (data[i] != '\0')
                    {
                        addr[((page_number*page_size)+i)] = data[i];
                        i++;
                    }
                    printf("[*] Page %d:\n%s\n", page_number, data);
                    msi_array[page_number] = S;
                
                } else {
                    
                    if (msi_array[page_number] == I) {
                        char res[page_size];
                        int i;
                        for (i=0;i<page_size; i++)
                        {
                            char c = addr[(page_number * page_size) + i];
                            res[i] = c;
                        }
                        printf("[*] Page %d:\n%s\n", page_number, res);
                        madvise(addr + (page_number * page_size), page_size, MADV_DONTNEED);
                    }
                }
            
            }
        }
    }
} 

int main(int argc, const char *argv[])
{
    struct sockaddr_in address_listen;
    struct sockaddr_in address_send;
    int opt = 1;
    int addrlen = sizeof(address_listen);
    char buffer[BUFF_SIZE] = {0};

    long uffd;          /* userfaultfd file descriptor */
    unsigned long len;  /* Length of region handled by userfaultfd */
    pthread_t thr;      /* ID of thread that handles page faults */
    pthread_t read_thr;
    struct uffdio_api uffdio_api;
    struct uffdio_register uffdio_register;
    int s;

    // Getting the system page size
    page_size = sysconf(_SC_PAGE_SIZE);

    // Taking local port to listen
    int local_listen_port = atoi(argv[1]);

    // Taking remote port to send message
    int remote_send_port = atoi(argv[2]);

    // Flag to check whether remote connection is made or not
    int is_connected = 0;
    
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
        printf("\nConnection Failed as Remote process is not started yet\n");
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

    uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd == -1)
        return -1;

    uffdio_api.api = UFFD_API;
    uffdio_api.features = 0;
    if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
        return -1;

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

        // enum msi_info msi_array[pages];
        int i=0;
        for (i=0;i<pages;i++)
        {
            msi_array[i] = I;
        }

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

        uffdio_register.range.start = (unsigned long) addr;
        uffdio_register.range.len = len;
        uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
        if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
            return -1;

        // Initating the page fault thread
        s = pthread_create(&thr, NULL, fault_handler_thread, (void *) uffd);
        if (s != 0) {
            errno = s;
            return -1;
        }

	    char message[80];
        char string_addr[50];
        
        // Converting address of char pointer to String
        snprintf(string_addr,50,"%p",addr);

        // Concatenating message (size and address) to be sent
        snprintf(message, sizeof message, "%s%s%s", string_addr, ",", string_len);

        printf("\nMessage sent to Process 2: %s\n", message);

        // Sending Socket Message
        send(remote_send_fd , message , sizeof(message) , 0 );

        pthread_create(&read_thr, NULL, socket_read_thread, -1);

	    user_interaction(pages);


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

        uffdio_register.range.start = (unsigned long) addr;
        uffdio_register.range.len = len;
        uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
        if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
            return -1;

        // Initating the page fault thread
        s = pthread_create(&thr, NULL, fault_handler_thread, (void *) uffd);
        if (s != 0) {
            errno = s;
            return -1;
        }

        // Printing the Mmap region and size for Process 2
        printf("Address returned by process 2 mmap(): %p\n", addr);
        printf("Length allocated for process 2 mmap: %ld\n", len);
        
        int pages = len/page_size;
        // enum msi_info msi_array[pages];
        int i=0;
        for (i=0;i<pages;i++)
        {
            msi_array[i] = I;
        }

        // Initating the page fault thread
        pthread_create(&read_thr, NULL, socket_read_thread, -1);

        user_interaction(pages);
    }
    
    return 0;
}
