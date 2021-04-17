#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>

void print_time(struct timeval* time, int n) {
	double avg = 0;
	for (int i = 0; i < n; i++)
		avg += time[i].tv_usec + 1000000 * time[i].tv_sec;
	avg /= n * 1000;
	printf("%.2fms\n", avg);
}

void print_route(int packets, char ip_str[][20], struct timeval *time) {
    if (packets == 0) {
        printf("*\n");
        return;
    }

    if (packets >= 1) printf("%s ", ip_str[0]);

    if (packets >= 2 && strcmp(ip_str[0], ip_str[1]) != 0)
        printf("%s ", ip_str[1]);

    if (
		packets >= 3 &&
		strcmp(ip_str[0], ip_str[1]) != 0 &&
		strcmp(ip_str[0], ip_str[2]) != 0
	) printf("%s ", ip_str[2]);

    if (packets == 3) print_time(time, packets);
    else printf("???\n");
}

int receive(int pid, int sockfd, int max_resp_time, int TTL, int packets_no, struct timeval* start_time) {
	struct timeval time[packets_no];
	struct timeval timeout, curr_time;
	timeout.tv_sec = max_resp_time; timeout.tv_usec = 0;
	bool got_reply = false;
	int packets = 0;
	char ip_str[packets_no][20];

	// file descriptors set that select will monitor
	fd_set descriptors;
	// zero the mask (file descriptor)
	FD_ZERO(&descriptors);
	// Sets the bit for the file descriptor 'sockfd'
	// in the file descriptor set 'descriptors/.
	FD_SET(sockfd, &descriptors);
	
	while (packets < packets_no && (timeout.tv_sec > 0 || timeout.tv_usec > 0)) {
		int ready_fd = select(sockfd + 1, &descriptors, NULL, NULL, &timeout);

		if (ready_fd < 0) return -1;
		else if (ready_fd == 0) break;

		struct sockaddr_in sender;
		socklen_t sender_len = sizeof(sender);
		u_int8_t buffer[IP_MAXPACKET];

		ssize_t packet_len = recvfrom(
			sockfd,
			buffer,
			IP_MAXPACKET,
			MSG_DONTWAIT,
			(struct sockaddr*) &sender,
			&sender_len
		);
		
		if (packet_len < 0) return -1;
		
		if (inet_ntop(AF_INET, &(sender.sin_addr), ip_str[packets], sizeof(ip_str[packets])) == NULL)
			return -1;

		struct ip* ip_header = (struct ip*) buffer;
		u_int8_t* icmp_packet = buffer + 4 * ip_header->ip_hl;
		struct icmp* icmp_header = (struct icmp*) icmp_packet;

		if (
			icmp_header->icmp_type != ICMP_ECHOREPLY &&
			icmp_header->icmp_type != ICMP_TIME_EXCEEDED
		) continue;
		
		struct icmp* org_icmp_header = icmp_header;
		// If the type is ICMP_TIME_EXCEEDED we need to move icmp 
		// by 64 bits (8 bytes) in order to get to the original datagram.
		// https://www.frozentux.net/iptables-tutorial/chunkyhtml/x281.html
		if (icmp_header->icmp_type == ICMP_TIME_EXCEEDED) {
			struct ip* org_ip_header = (void *) icmp_header + 8;
			org_icmp_header = (void *) org_ip_header + 4 * org_ip_header->ip_hl;
		}
		
		if (
			org_icmp_header->icmp_hun.ih_idseq.icd_id != pid &&
			org_icmp_header->icmp_hun.ih_idseq.icd_seq != TTL
		) continue;
		
		gettimeofday(&curr_time, NULL);
		timersub(&curr_time, start_time, &time[packets]);
		packets++;
		if (icmp_header->icmp_type == ICMP_ECHOREPLY && packets == packets_no) {
			got_reply = true;
			break;
		}
	}
	
	printf("%d. ", TTL);
	print_route(packets, ip_str, time);

	return got_reply;
}
