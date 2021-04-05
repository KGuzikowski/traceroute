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

bool more_than_one_ip(char ip_str[][20], int last_idx) {
	for (int i = last_idx; i >= 0; i--)
		if (strcmp(ip_str[i], ip_str[last_idx]))
			return true;
	return false;
}

int receive(int pid, int sockfd, int max_resp_time, int TTL, int packets_no, struct timeval* start_time) {
	struct timeval time[packets_no];
	struct timeval timeout, curr_time;
	timeout.tv_sec = max_resp_time; timeout.tv_usec = 0;
	bool more_than_one = false;
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

	while (packets < packets_no && (timeout.tv_sec || timeout.tv_usec)) {
		int ready_fd = select(sockfd + 1, &descriptors, NULL, NULL, &timeout);

		if (ready_fd < 0) return ready_fd;
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
		
		struct icmp* org_icmp_header;
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

		if (!more_than_one) more_than_one = more_than_one_ip(ip_str, packets);

		gettimeofday(&curr_time, NULL);
		timersub(&curr_time, start_time, &time[packets]);
		packets++;

		if (icmp_header->icmp_type == ICMP_ECHOREPLY) {
			got_reply = true;
			break;
		}
	}

	printf("%d. ", TTL);
	if (packets == 0) printf("*\n");
	else if (more_than_one) {
		for (int i = 0; i < packets; i++)
			printf("%s ", ip_str[i]);
		if (packets == packets_no)
			print_time(time, packets);
		else printf("???\n");
	} else {
		printf("%s ", ip_str[0]);
		if (got_reply || packets == packets_no)
			print_time(time, packets);
		else printf("???\n");
	}

	return got_reply;
}