#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "msg.h"

#define BUF 256
#define DB_FILENAME "database.txt"

void Usage(char *progname);
void *HandleClient(void *arg);

// Arguments passed into HandleClient function
struct thread_arg {
    int c_fd;
};

void Usage(char *progname) {
    printf("usage: %s port \n", progname);
    exit(EXIT_FAILURE);
}

int Listen(char *portnum);

int main(int argc, char **argv) {
    if (argc != 2) {
        Usage(argv[0]);
    }

    // Bind port and Listen for client
    int listen_fd = Listen(argv[1]);
    if (listen_fd <= 0) {
        printf("Couldn't bind to any addresses.\n");
        return EXIT_FAILURE;
    }

    while (1) {
        struct sockaddr_storage caddr;
        socklen_t caddr_len = sizeof(caddr);
     
        // Accept new client
        int client_fd = accept(listen_fd, (struct sockaddr *)(&caddr), &caddr_len);
        if (client_fd < 0) {
            if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
                continue;
            printf("Failure on accept:%s \n ", strerror(errno));
            break;
        }

        // Create a new thread to handle the client
        pthread_t tid;
        struct thread_arg *arg = malloc(sizeof(struct thread_arg));
        arg->c_fd = client_fd;

        if (pthread_create(&tid, NULL, HandleClient, (void *)arg) != 0) {
            printf("Failed to create thread \n");
            close(client_fd);
            free(arg);
            continue;
        }
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}

// Function listens to port and queues clients [from server.c]
int Listen(char *portnum) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
    hints.ai_socktype = SOCK_STREAM;  // stream
    hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
    hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
    hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    struct addrinfo *result;
    int res = getaddrinfo(NULL, portnum, &hints, &result);

    if (res != 0) {
        printf("getaddrinfo failed: %s", gai_strerror(res));
        return -1;
    }

    int listen_fd = -1;
    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1) {
            printf("socket() failed:%s \n ", strerror(errno));
            listen_fd = -1;
            continue;
        }

        int optval = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(result);

    if (listen_fd == -1)
        return listen_fd;

    if (listen(listen_fd, SOMAXCONN) != 0) {
        printf("Failed to mark socket as listening:%s \n ", strerror(errno));
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

// Thread function envoked when a client has connected
void *HandleClient(void *arg) {
    int c_fd = ((struct thread_arg *)arg)->c_fd;
    free(arg);

    while(1) {
    	struct msg request;
 	// Read message sent from client
 	ssize_t num_bytes = read(c_fd, &request, sizeof(struct msg));
    	if(num_bytes == 0) {
		break;
    	}
    	else if (num_bytes != sizeof(struct msg)) {
        	close(c_fd);
        	pthread_exit(NULL);
    	}

    
    	// Process PUT request
    	if (request.type == PUT) {
        	// Open file
		FILE *db_file = fopen(DB_FILENAME, "a+");
        	if (db_file == NULL) {
        	    perror("Error opening database file");
        	    close(c_fd);
        	    pthread_exit(NULL);
        	}
	
		// Write to file
        	fprintf(db_file, "%s|%u\n", request.rd.name, request.rd.id);
        	fflush(db_file);
        	fclose(db_file);

		// Set put as successful
        	struct msg response = {.type = SUCCESS};

		// Write back to client
        	write(c_fd, &response, sizeof(struct msg));
    	}
    	// Process GET request
    	else if (request.type == GET) {
        	fflush(stdout);
		// Open file
		FILE *db_file = fopen(DB_FILENAME, "r");
        	if (db_file == NULL) {
        	    	perror("Error opening database file");
            		close(c_fd);
            		pthread_exit(NULL);
        	}
    
        	struct record rec;
        	struct msg response = { .type = FAIL};
		// Search the file for ID
		while(fscanf(db_file, "%[^|]|%u\n", rec.name, &rec.id) == 2) {	    
	    		// If matching ID is found, make response message success
			if (request.rd.id == rec.id) {
				response.type = SUCCESS;
                		response.rd = rec;
				break;
            		}
        	}

        	fclose(db_file);
        	// Write result to client
		write(c_fd, &response, sizeof(struct msg));
    	} else {
        	// Invalid request type
        	struct msg response = { .type = FAIL }; //should be fail
        	printf("sent fail.\n");
		write(c_fd, &response, sizeof(struct msg));
    	}
    }
    close(c_fd);
    pthread_exit(NULL);
}
