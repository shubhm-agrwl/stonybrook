#include <sys/syscall.h>
#include <linux/kernel.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void main (int argc, char const *argv[])
{
	// initializing the char string length to be 50
	char str[50];
	// initalizing the key and option variables
	int key, option = 1;
	// initializing the input flag
	int valid_input = 0;
	// initalizing the encrypt file descriptor
	int encrypt_fd = -1;

	// Printing the message for options
	printf ("Usage: \n -s : String value \n -k : Key value \n");

	// Loop which runs through the arguments
	while ((option = getopt(argc, argv, "s:k:"))!= -1){
		switch (option){
			// Case to handle k Argument
			case 'k':
				key = atoi(optarg);
				valid_input = 1;
				break;
			// Handle S Argument scenario
			case 's':
				strcpy(str, optarg);
				valid_input = 1;
				break;
			// Handle when none of the above cases are handled
			default:
				printf("Unknown arguments: %c\n", optopt);
				valid_input = 0;
			}
		}
	
	// To check if the input is a valid input or not
	if (valid_input == 1){
		// Call System call which was newly added
		encrypt_fd = syscall (440, str, key);
		// Printing the return value of the system call
		printf("Return value of sys_s2_encrypt: %d\n",encrypt_fd);
	} else {
		printf("Invalid inputs for sys_s2_encrypt\n");
	}
}
