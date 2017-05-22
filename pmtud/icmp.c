#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "pmtud.h"

extern int g_nextmtu;

static int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return ans;
}


int send_icmp_echo(struct sockaddr *dst, int mtu)
{
	struct icmp *pkt;
	char buf[2048] = {0};
	int bflag = IP_PMTUDISC_DO;
	ssize_t c;
	int sock;
	int rand = 0;

	if(!dst || mtu < 600 || mtu > 1500){
		printf("arg error\n");
		return -1;
	}

	printf("icmp len=%d\n", mtu);
	pkt = (struct icmp *)buf;
	pkt->icmp_type = ICMP_ECHO;
	srand(time(NULL));
	rand = random()/4096;
	pkt->icmp_id = rand;
	pkt->icmp_seq = htons(1);
	pkt->icmp_otime = time(NULL);

	sock = socket(AF_INET, SOCK_RAW, 1);

	pkt->icmp_cksum = in_cksum((unsigned short *)pkt, sizeof(buf));
	setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &bflag, sizeof(bflag));
	c = sendto(sock, buf, mtu, 0, (struct sockaddr *)dst, sizeof(struct sockaddr));
	if(c < 0){
		perror("sendto");
	}
		
	while(1) {
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		struct timeval tv;
		fd_set fd;

		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		select(sock+1, &fd, 0, 0, &tv);
	
		fcntl(sock, F_SETFL, O_NONBLOCK);
		c = recvfrom(sock, buf, sizeof(buf), 0,(struct sockaddr *) &from, &fromlen);	
		if (c < 0) {
			if (errno != EINTR)
				perror("recvfrom");
			//continue;
			//break;
			return RESP_NORESP;
		}

		if(c >= 76){   /* ip + icmp */
			struct iphdr *iphdr = (struct iphdr *) buf;
			pkt = (struct icmp *) (buf + (iphdr->ihl << 2));
			if (pkt->icmp_type == ICMP_ECHOREPLY){
				printf("Got reply: type=%d, code=%d\n", pkt->icmp_type,pkt->icmp_code);
				if((pkt->icmp_type == ICMP_ECHOREPLY) && (pkt->icmp_code == 0)){
					
					return RESP_OK;
				}
				break;
			}else{
				printf("icmp_type=%d, code=%d\n", pkt->icmp_type, pkt->icmp_code);
				if((pkt->icmp_type == ICMP_DEST_UNREACH) && (pkt->icmp_code == ICMP_FRAG_NEEDED)){
					printf("nextmtu=%d\n", ntohs(pkt->icmp_nextmtu));
					g_nextmtu = ntohs(pkt->icmp_nextmtu);
					return RESP_NEED_FRAG;
				}
				break;
			}
		}
	}

	return -1;
}

