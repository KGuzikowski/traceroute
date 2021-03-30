#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "icmp_send.h"

#define RESPONSE_TIME 1
#define PACKETS_TO_SEND 3
#define MAX_TTL 30

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Please provide exactly one argument (IP).\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest;
    
    // Let's convert program argument to a proper IP:
    if (!inet_pton(AF_INET, argv[1], &dest)) {
        fprintf(stderr, "Error: Provided IP address is not correct.\n");
        return EXIT_FAILURE;
    }

    // sockfd will hold file descriptor that refers to a communication endpoint.
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		fprintf(stderr, "Socket error: %s\n", strerror(errno)); 
		return EXIT_FAILURE;
	}

    int pid = getpid();
    for (int i = 1; i <= MAX_TTL; i++) {
        
    }

    return 0;
}
