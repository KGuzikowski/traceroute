#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>

u_int16_t compute_icmp_checksum(const void *buff, int length) {
	u_int32_t sum;
	const u_int16_t* ptr = buff;
	assert (length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (u_int16_t)(~(sum + (sum >> 16)));
}


// On success, this function return the number
// of characters sent. On error, -1 is returned.
int send_one(int sockfd, int TTL, struct sockaddr_in *dest, int id) {
    // Below I set TTl for the socket.
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &TTL, sizeof(int)) < 0)
        return -1;

    struct icmp icmp_header;
    icmp_header.icmp_type = ICMP_ECHO;
    icmp_header.icmp_code = 0;
    icmp_header.icmp_hun.ih_idseq.icd_seq = TTL;
    icmp_header.icmp_hun.ih_idseq.icd_id = id;
    icmp_header.icmp_cksum = 0;
    icmp_header.icmp_cksum = compute_icmp_checksum((u_int16_t*) &icmp_header, sizeof(icmp_header));

    char a[20];
    inet_ntop(AF_INET, &(dest->sin_addr), a, sizeof(a));
    printf("IN send_one dest addr is %s\n", a);
    
    return sendto(
        sockfd,
        &icmp_header,
        sizeof(icmp_header),
        0, // flags
        (struct sockaddr*) dest,
        sizeof(*dest)
    );
}