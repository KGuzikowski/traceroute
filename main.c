#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>
#include "icmp_send.h"
#include "icmp_receive.h"

#define MAX_RESPONSE_TIME 1
#define PACKETS_TO_SEND 3
#define MAX_TTL 30

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments. Please provide exactly one argument (IP).\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    
    // Let's convert program argument to a proper IP:
    if (inet_pton(AF_INET, argv[1], &(dest.sin_addr)) != 1) {
        fprintf(stderr, "Error: Provided IP address is not correct.\n");
        return EXIT_FAILURE;
    }

    // sockfd will hold file descriptor that refers to a communication endpoint.
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		fprintf(stderr, "Socket error: %s\n", strerror(errno)); 
		return EXIT_FAILURE;
	}

    // pid is a unique identifier that will help
    // distinguish packages (because raw socket is used here).
    int pid = getpid();

    for (int i = 1; i <= MAX_TTL; i++) {
        struct timeval start_time;
        gettimeofday(&start_time, NULL);

        for (int j = 0; j < PACKETS_TO_SEND; j++)
            if (send_one(sockfd, i, &dest, pid) < 0) {
                fprintf(stderr, "Error when sending packets: %s\n", strerror(errno));
                return EXIT_FAILURE;
            }

        int res = receive(pid, sockfd, MAX_RESPONSE_TIME, i, PACKETS_TO_SEND, &start_time);

        if (res < 0) {
            fprintf(stderr, "Error when receiving packets: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        if (res == 1) break;
    }

    close(sockfd);
    return EXIT_SUCCESS;
}