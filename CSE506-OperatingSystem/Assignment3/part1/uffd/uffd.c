/* userfaultfd_demo.c

   Licensed under the GNU General Public License version 2 or later.
*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <poll.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);	\
	} while (0)

static int page_size;

static void *
fault_handler_thread(void *arg)
{
	static struct uffd_msg msg;   /* Data read from userfaultfd */
	static int fault_cnt = 0;     /* Number of faults so far handled */
	long uffd;                    /* userfaultfd file descriptor */
	static char *page = NULL;
	struct uffdio_copy uffdio_copy;
	ssize_t nread;

	uffd = (long) arg;

	/* [H1]
	 * Explain following in here.
	 * Char pointer to page is defined as NULL above. 
	 * Checking whether the page is NULL. It returns true and 
	 * creates mapping in virtual address space with length of a single
	 * page size so that data can be written when a page fault happens
	 * Arguments: 
	 * void *addr = NULL which allows kernel to choose the address
	 * size_t length = page_size i.e. size of the mapping to be allocated
	 * int prot = PROT_READ | PROT_WRITE which specifies the memory protection
	 * that the page being created can be read and written
	 * int flags = MAP_PRIVATE which specifies the properties
	 * that updates to this page/memory will not be accessible to other process
	 * flag = MAP_ANONYMOUS specifies that the page is not backed by any file
	 * and hence its associated file descriptor is set to -1 and
	 * off_t offset is set to 0
	 * if MAP_FAILED (void *) -1 is returned, it means that memory was not 
	 * allocated and the code exists since nothing can be done.
	 */
	if (page == NULL) {
		page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED)
			errExit("mmap");
	}

	/* [H2]
	 * Explain following in here.
	 * Loop to handle incomming events on the user fault file descriptor.
	 * Infinite loop as it constantly checks whether page fault is happening
	 * or not. This will loop until the thread which spawned this function
	 * is killed.
	 */
	for (;;) {

		/* See what poll() tells us about the userfaultfd */

		struct pollfd pollfd;
		int nready;

		/* [H3]
		 * Explain following in here.
		 * pollfd descriptor is used to monitor the user fault file 
		 * descriptor
		 * Since, uffd is a non blocking file decriptor, pollfd can poll
		 * without any condition or poller
		 * It sets its file descriptor to point to the userfault file 
		 * descriptor passed from the main function. 
		 * It sets the requested events as POLLIN which specifies 
		 * that it has data to read. revents are not set as it
		 * is not expecting any events to be returned
		 * nfds_t nfds is specified as 1 in poll function as there is
		 * only 1 user fault file descriptor to read from
		 * int timeout = -1 specifies the time (milliseconds) that
		 * poll() could wait for the file descriptor to be available
		 * to perform operations.
		 * -1 means that it can wait for infinite time.
		 * If the poll fails, it returns -1 and the program is exited
		 */
		pollfd.fd = uffd;
		pollfd.events = POLLIN;
		nready = poll(&pollfd, 1, -1);
		if (nready == -1)
			errExit("poll");

		printf("\nfault_handler_thread():\n");
		printf("    poll() returns: nready = %d; "
                       "POLLIN = %d; POLLERR = %d\n", nready,
                       (pollfd.revents & POLLIN) != 0,
                       (pollfd.revents & POLLERR) != 0);

		/* [H4]
		 * Explain following in here.
		 * Reads the message from the user fault file descriptor. 
		 * It returns the size in bytes of the message read. 
		 * If it reaches the end of file it returns 0.
		 */
		nread = read(uffd, &msg, sizeof(msg));
		if (nread == 0) {
			printf("EOF on userfaultfd!\n");
			exit(EXIT_FAILURE);
		}

		if (nread == -1)
			errExit("read");

		/* [H5]
		 * Explain following in here.
		 * uffd_msg has an attribute specifying various events. 
		 * In the below code, it checks whether the current message
		 * event is a page fault event or not.
		 */
		if (msg.event != UFFD_EVENT_PAGEFAULT) {
			fprintf(stderr, "Unexpected event on userfaultfd\n");
			exit(EXIT_FAILURE);
		}

		/* [H6]
		 * Explain following in here.
		 * Since, the control was not exited above, it prints that the page
		 * fault has happened. It also prints the flag value of page fault 
		 * and the address where the page fault has happened
		 */
		printf("    UFFD_EVENT_PAGEFAULT event: ");
		printf("flags = %llx; ", msg.arg.pagefault.flags);
		printf("address = %llx\n", msg.arg.pagefault.address);

		/* [H7]
		 * Explain following in here.
		 * sets the data to the address of page defined above to
		 * 'A' + number of times the page fault has happened.
		 * Since, we know that page can be of size page_size, 
		 * providing the same in the arguments which specifies the
		 * length to be copied. Next line increments the fault count,
		 * so that the next time, it sets to 'A' + 1 i.e. B 
		 * whenever a page fault happens
		 */
		memset(page, 'A' + fault_cnt % 20, page_size);
		fault_cnt++;

		/* [H8]
		 * Explain following in here.
		 * Once the page fault has happened, it copies the data to the page
		 * region/ address space. It sets the src argument to the page
		 * address allocated above. 
		 * The destination address is set to the page fault address.
		 * Since, we know that the length can be of a unit page size,
		 * round faulting address down to specify the page boundary. len
		 * specifies the length of page size which is to be copied. Mode
		 * specifies the flags which defines the behaviour of copy and 
		 * copy is set to zero which is an output field and not read by
		 * the copy operation
		 */
		uffdio_copy.src = (unsigned long) page;
		uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
			~(page_size - 1);
		uffdio_copy.len = page_size;
		uffdio_copy.mode = 0;
		uffdio_copy.copy = 0;

		/* [H9]
		 * Explain following in here.
		 * system call to copy with the parameters/ attributes defined
		 * above. It returns -1 if the copy fails
		 */
		if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
			errExit("ioctl-UFFDIO_COPY");

		/* [H10]
		 * Explain following in here.
		 * prints the output field copy from the uffdio_copy struct which
		 * gives the number of putes copied by the copy system call
		 * performed above. It prints that it has copied 4096 length.
		 */
		printf("        (uffdio_copy.copy returned %lld)\n",
                       uffdio_copy.copy);
	}
}

int
main(int argc, char *argv[])
{
	long uffd;          /* userfaultfd file descriptor */
	char *addr;         /* Start of region handled by userfaultfd */
	unsigned long len;  /* Length of region handled by userfaultfd */
	pthread_t thr;      /* ID of thread that handles page faults */
	struct uffdio_api uffdio_api;
	struct uffdio_register uffdio_register;
	int s;
	int l;

	/* [M1]
	 * Explain following in here.
	 * Checks for the count of the arguments. If The number of arguments
	 * including the binary file is not 2, then it will print the usage log
	 * "Usage: ./uffd num-pages". It will print this line if no or more than
	 * one parameter after the binary file is provided to run
	 */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s num-pages\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* [M2]
	 * Explain following in here.
	 * sysconf is used to get the configuration information at run-time
	 * first line is used to get the memory page size at run-time and stores
	 * the value in static variable page_size since this value will not change
	 * len is used to store the total size required in bytes by multiplying 
	 * indiviual page size found before and the number of pages inputed by 
	 * the user. stroul function returns a long int value of the argument 
	 * provided above. If the conversion fails, it returns a base value 0.
	 */
	page_size = sysconf(_SC_PAGE_SIZE);
	len = strtoul(argv[1], NULL, 0) * page_size;

	/* [M3]
	 * Explain following in here.
	 * System Call to create the userfault file descriptor with flags
	 * O_CLOEXEC enables close-on-exec flag. Generally used in multithreaded
	 * problems where we avoid using additional close-exec commands which can
	 * lead to race conditions.
	 * O_NONBLOCK ensures that the operations on the open file descriptor will
	 * not cause the calling process to wait for any kind of operations
	 * If the system call returns -1, there was an error creating the user
	 * fault file descriptor 
	 */
	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1)
		errExit("userfaultfd");

	/* [M4]
	 * Explain following in here.
	 * ioctl is used to define the characterestics of the file descriptor
	 * Here, ioctl is used to define the characterestics of the user fault file
	 * descriptor created above. uffdio_api is a struct which is defined to use
	 * UFFDIO_API which acts as a confirmation between the user and the kernel
	 * space with supported features.
	 * This operation is important before any other operation and returns -1
	 * if it fails. If -1 is returned, we are exiting the code
	 */
	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
		errExit("ioctl-UFFDIO_API");

	/* [M5]
	 * Explain following in here.
	 * mmap creates mapping in virtual address space
	 * Arguments: 
	 * void *addr = NULL which allows kernel to choose the address
	 * size_t length = len specifies the size of the mapping to be allocated
	 * int prot = PROT_READ | PROT_WRITE which specifies the memory protection
	 * that the page being created can be read and written
	 * int flags = MAP_PRIVATE | MAP_ANONYMOUS which specifies the properties
	 * that updates to this page/memory will not be accessible to other process
	 * and is not backed by any file
	 * int fd = -1
	 * off_t offset = 0 which specifies that memory can be used from the first
	 * from whatever memory space has been allocated
	 * if MAP_FAILED (void *) -1 is returned, it means that memory was not 
	 * allocated and the code exists since nothing can be done.
	 */
	addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED)
		errExit("mmap");

	printf("Address returned by mmap() = %p\n", addr);

	/* [M6]
	 * Explain following in here.
	 * uffdio_register struct is used to store the properties of the
	 * UFFDIO_REGISTER. UFFDIO_REGISTER operation allows page fault in the 
	 * kernel space to be forwarded to the user space. 
	 * Properties: It assigns the start location from the address of the 
	 * mmap allocated above. Defines the length of the pages which is equal
	 * to the length of the total memory allocated. Also assigns the mode as
	 * UFFDIO_REGISTER_MODE_MISSING which tracks the missing pages in the
	 * kernel space. If the registration does not happen successfully i.e.
	 * returns -1, the program is exited.
	 */
	uffdio_register.range.start = (unsigned long) addr;
	uffdio_register.range.len = len;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		errExit("ioctl-UFFDIO_REGISTER");

	/* [M7]
	 * Explain following in here.
	 * Creates a new independent thread with the pthread_t created above.
	 * It takes the start routine and routine's arguments. In this case, 
	 * fault_handler_thread is the routine and it is passing the user fault
	 * file descriptor created above
	 * pthread_create returns 0 on success, if the thread is not created, we
	 * are exiting the program
	 */
	s = pthread_create(&thr, NULL, fault_handler_thread, (void *) uffd);
	if (s != 0) {
		errno = s;
		errExit("pthread_create");
	}

	/*
	 * [U1]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * assigns integer l with hex value 0, runs a loop till the end of the 
	 * memory allocated (size len) increasing 1024 everytime. For the first
	 * time when this loop runs, it creates a page fault. It copies 'A' to 
	 * address space for 4096 length. Now, when it tries to fetch the adress,
	 * it fetches 'A' and prints it. 'A' is fetched as it depends on the number
	 * of times the page fault has happened. In this case, this is the 
	 * first time. This loop runs for 3 more times, as the
	 * value of len is 4096 and we are incrementing 1024 everytime. Now, since,
	 * all the 4096 has value 'A', there is no more page fault and it fetches
	 * 'A' for all subsequent times. Therefore, after one page fault, it prints
	 * 'A' for all 4 times until the value of l is less than len i.e. 4096
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#1. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	/*
	 * [U2]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * Since, we are again initializing l to 0, we are again looping from 0 to
	 * 1024 for 4 times until we reach 4096. Since, we know that page fault had
	 * already happened before and value 'A' is there in all locations till 4096
	 * length, it will return 'A' for all the loops. Hence, 'A' is printed
	 * everytime for all the 4 times without a page fault.
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#2. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	/*
	 * [U3]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * madvise is a system call to advice the kernel on the memory aspects
	 * starting from the addr i.e. address specified during the time of the
	 * memory allocation till the length till where it is allocated.
	 * MADV_DONTNEED advice or flag is used when it advices the kernel that
	 * the memory range specified will no longer be used in the near future.
	 * This advice indicates that kernel can free the resources associated with
	 * the given address space and hence deallocates the data from the pages.
	 * 
	 * Now, since, no more data is present in the page, again the page fault
	 * happens during the first iteration. Now, it fetches 'B' since the count
	 * of number of page faults has increased by 1 to 2, and hence the page
	 * fault logic sets the memory space of 4096 length to 2. Now, for all
	 * subsequent iterations, no page fault happens and it fetches and prints
	 * 'B' everytime. Therefore after 1 page fault, it prints 'B' for 4 times
	 */
	printf("-----------------------------------------------------\n");
	if (madvise(addr, len, MADV_DONTNEED)) {
		errExit("fail to madvise");
	}
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#3. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	/*
	 * [U4]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * Since, we are again initializing l to 0, we are again looping from 0 to
	 * 1024 for 4 times until we reach 4096. Since, we know that page fault had
	 * already happened before and value 'B' is there in all locations till 4096
	 * length, it will return 'B' for all the loops. Hence, 'B' is printed
	 * everytime for all the 4 times without a page fault.
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#4. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	/*
	 * [U5]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * madvise is a system call to advice the kernel on the memory aspects
	 * starting from the addr i.e. address specified during the time of the
	 * memory allocation till the length till where it is allocated.
	 * MADV_DONTNEED advice or flag is used when it advices the kernel that
	 * the memory range specified will no longer be used in the near future.
	 * This advice indicates that kernel can free the resources associated with
	 * the given address space and hence deallocates the data from the pages.
	 *
	 * A loop runs from 0 to 1024 till it reaches 4096. During the first 
	 * iteration, since the memory is deallocated due to madvice, memset
	 * will undergo a pagefault. As per the page fault function, memory
	 * will be allocated and data will be set to '@' and copied to page.
	 * Now, everytime the loop runs, '@' will be set to the specified 1024
	 * bytes but page fault will not happen as the memory is allocated to
	 * all the 4096 bytes. Hence, page fault happens once during the first
	 * iteration and prints '@' for 4 times till the loop ends.
	 */
	printf("-----------------------------------------------------\n");
	if (madvise(addr, len, MADV_DONTNEED)) {
		errExit("fail to madvise");
	}
	l = 0x0;
	while (l < len) {
		memset(addr+l, '@', 1024);
		printf("#5. write address %p in main(): ", addr + l);
		printf("%c\n", addr[l]);
		l += 1024;
	}

	/*
	 * [U6]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * Since, we are again initializing l to 0, we are again looping from 0 to
	 * 1024 for 4 times until we reach 4096. Since, we know that page fault had
	 * already happened before and value '@' is there in all locations till 4096
	 * length, it will return '@' for all the loops. Hence, '@' is printed
	 * everytime for all the 4 times without a page fault.
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#6. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	/*
	 * [U7]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * In this case, there is no madvise. The memory is still allocated and 
	 * has data '@'. Now, in every iteration with a step of 1024, we are 
	 * setting the address space with '^' - overriding the data. Hence, it
	 * overrides the data to '^' from '@' for all the address space and prints
	 * with no page fault for 4 times from 0 to 4096.
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		memset(addr+l, '^', 1024);
		printf("#7. write address %p in main(): ", addr + l);
		printf("%c\n", addr[l]);
		l += 1024;
	}

	/*
	 * [U8]
	 * Briefly explain the behavior of the output that corresponds with below section.
	 * Since, we are again initializing l to 0, we are again looping from 0 to
	 * 1024 for 4 times until we reach 4096. Since, we know 
	 * that the value '^' is there in all locations till 4096
	 * length, it will return '^' for all the loops. Hence, '^' is printed
	 * everytime for all the 4 times without a page fault.
	 */
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#8. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}

	exit(EXIT_SUCCESS);
}
