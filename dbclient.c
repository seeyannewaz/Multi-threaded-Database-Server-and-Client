#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define BUF 256
#define MAX_NAME_LENGTH 128

void Usage(char *progname);
int LookupName(char *name, unsigned short port, struct sockaddr_storage *ret_addr, size_t *ret_addrlen);
int Connect(const struct sockaddr_storage *addr, const size_t addrlen, int *ret_fd);
void ProcessPut(int socket_fd);
void ProcessGet(int socket_fd);

int main(int argc, char **argv) {
    if (argc != 3) {
        Usage(argv[0]);
    }

    unsigned short port = 0;
    if (sscanf(argv[2], "%hu", &port) != 1) {
        Usage(argv[0]);
    }

    // Get an appropriate sockaddr structure.
    struct sockaddr_storage addr;
    size_t addrlen;
    if (!LookupName(argv[1], port, &addr, &addrlen)) {
        Usage(argv[0]);
    }

    // Connect to the remote host.
    int socket_fd;
    if (!Connect(&addr, addrlen, &socket_fd)) {
        Usage(argv[0]);
    }

    // Start client menu
    int menu_choice;
    do {
        // Print menu
        printf("\nMenu:\n");
        printf("1. PUT\n");
        printf("2. GET\n");
        printf("0. QUIT\n");
        printf("Enter your choice: ");
        fflush(stdout);
	
	// Get menu choice from user
	if(scanf("%d", &menu_choice) != 1) {
		printf("Invalid input.\n");
		continue;
	}
        
	// Clear newline from buffer
        while (getchar() != '\n');

	// Call function associated with menu choice
        switch (menu_choice) {
            case 1:
                ProcessPut(socket_fd);
                break;
            case 2:
                ProcessGet(socket_fd);
                break;
            case 0:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice [%d]. Please try again.\n", menu_choice);
        }
    } while (menu_choice != 0);

    // Clean up.
    close(socket_fd);
    return EXIT_SUCCESS;
}

void Usage(char *progname) {
    printf("usage: %s hostname port\n", progname);
    exit(EXIT_FAILURE);
}

// Function looks up hostname and port [from client.c code]
int LookupName(char *name, unsigned short port, struct sockaddr_storage *ret_addr, size_t *ret_addrlen) {
    struct addrinfo hints, *results;
    int retval;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Do the lookup by invoking getaddrinfo().
    if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(retval));
        return 0;
    }

    // Set the port in the first result.
    if (results->ai_family == AF_INET) {
        struct sockaddr_in *v4addr = (struct sockaddr_in *) (results->ai_addr);
        v4addr->sin_port = htons(port);
    } else if (results->ai_family == AF_INET6) {
        struct sockaddr_in6 *v6addr = (struct sockaddr_in6 *)(results->ai_addr);
        v6addr->sin6_port = htons(port);
    } else {
        printf("getaddrinfo failed to provide an IPv4 or IPv6 address\n");
        freeaddrinfo(results);
        return 0;
    }

    // Return the first result.
    assert(results != NULL);
    memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
    *ret_addrlen = results->ai_addrlen;

    // Clean up.
    freeaddrinfo(results);
    return 1;
}

// Function connect client to server [from client.c code]
int Connect(const struct sockaddr_storage *addr, const size_t addrlen, int *ret_fd) {
    // Create the socket.
    int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("socket() failed: %s\n", strerror(errno));
        return 0;
    }

    // Connect the socket to the remote host.
    int res = connect(socket_fd, (const struct sockaddr *)(addr), addrlen);
    if (res == -1) {
        printf("connect() failed: %s\n", strerror(errno));
        return 0;
    }

    *ret_fd = socket_fd;
    return 1;
}

// Function handles PUT process
void ProcessPut(int socket_fd) {
    struct msg put_msg;
    put_msg.type = PUT;

    // Ask user for Name
    printf("Enter name: ");
    fflush(stdout);
    // Scan user's reponse
    fgets(put_msg.rd.name, MAX_NAME_LENGTH, stdin);
    // Remove newline
    strtok(put_msg.rd.name, "\n");

    // Ask user for ID
    printf("Enter id: ");
    fflush(stdout);
    // Scan user's response
    if(scanf("%u", &put_msg.rd.id) != 1) {
	printf("Invalid input.\n");
	return;
    }
 
    // Clear any newlines that may be present in the buffer
    int c;
    while((c = getchar()) != '\n' && c != EOF);
    
    // Send PUT message to server
    if (write(socket_fd, &put_msg, sizeof(struct msg)) <= 0) {
        printf("Failed to send PUT message.\n");
        return;
    }

    // Wait for response from server
    struct msg response;
    if (read(socket_fd, &response, sizeof(struct msg)) <= 0) {
        printf("Failed to receive response.\n");
        return;
    }

    // Print appropiate message based on response type
    if (response.type == SUCCESS) {
        printf("Put success.\n");
    } else {
        printf("Put failed.\n");
    }
}

// Function handles GET process
void ProcessGet(int socket_fd) {
    struct msg get_msg;
    get_msg.type = GET;
    
    uint32_t requested_id;

    // Ask user for ID
    printf("Enter id: ");
    if(scanf("%u", &requested_id) != 1) {
	printf("Invalid input.\n");
	return;
    }

    get_msg.rd.id = requested_id;

    // Send GET message to server
    if (write(socket_fd, &get_msg, sizeof(struct msg)) <= 0) {
        printf("Failed to send GET message.\n");
        return;
    }

    // Wait for response from server
    struct msg response;
    ssize_t num_bytes = read(socket_fd, &response, sizeof(struct msg));   
    // If the client does not recieve response, print error
    if(num_bytes != sizeof(struct msg)) {
	printf("Failed to receive response (%zd bytes received).\n", num_bytes);
	return;
    }

    // Print appropirate message based on response type
    if (response.type == SUCCESS) {
        printf("Record found.\nName: %s\nID: %u\n", response.rd.name, response.rd.id);
    }
    else if (response.type == FAIL){
        printf("Get failed.\n");
    }
    else {
	printf("Unexpected response type: %d\n", response.type);	
    }
}
