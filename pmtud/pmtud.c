/***************************************************************************
* File name:     pmtud.c
* Author:        renleilei
* Description:   
* Others: 
* Last modified: 2015-07-16 12:09
***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include "pmtud.h"


#define IFNAME_WAN "eth0"
#define PPPOE_WAN "pppoe-wan"
#define PPP_HDR_LEN 8

char *g_website[] = {"www.baidu.com", "www.qq.com", "www.taobao.com", "www.microsoft.com", NULL};
int g_mtu_try[]={1492, 1488, 1484, 1480, 1476, 1460, 1450, 1430, 1390, 1300, 1200, 1100, 1000, 800, 0}; //order: desc
int g_nextmtu = 0;
int g_mtu = 0;

void init_daemon(void)
{
	int pid;
	int i;

	if(0 != (pid=fork()))
		exit(0);
	else if(pid < 0)
		exit(1);

	setsid();

	if(0 != (pid=fork()))
		exit(0);
	else if(pid < 0)
		exit(1);

	for(i = 0; i < 3; ++i)
		close(i);
	chdir("/tmp");
	umask(0);

	return;
}

int adjust_mtu(int mtu)
{
	int ret = 0;
	char cmd[256] = {0};

	if(mtu < MTU_MIN || mtu > MTU_MAX){
		printf("Error: arg mtu error: %d\n", mtu);
		return -1;
	}

	//snprintf(cmd, sizeof(cmd), "ifconfig %s mtu %d;ifconfig %s mtu %d;uci set network.wan.mtu=%d;uci set network.wan.mru=%d;uci commit network", 
	//								PPPOE_WAN, mtu, IFNAME_WAN, mtu, mtu, mtu);
	snprintf(cmd, sizeof(cmd), "uci set network.wan.mtu=%d;uci set network.wan.mru=%d;uci commit network;/etc/init.d/network restart &", 
									mtu, mtu);
	printf("exec: %s\n", cmd);
	system(cmd);

	return ret;	
}

void clear_crlf(char *str)
{
	if(!str)
		return;
	
	while(*str++ != '\0'){
		if(*str == '\n')
			*str ='\0';
	}

	return;
}

int get_uci_opt_value(char *filename, char *section, char *option, char *value, int val_len)
{
	FILE *fp = NULL;
	char cmd[256] = {0};
	char buf[128] = {0};
	
	if(!filename || !section || !option || !value) {
		printf("get uci opt v: arg error.");
		return 1;
	}
	
	snprintf(cmd, sizeof(cmd), "uci get %s.%s.%s >/dev/null 2>&1 "
		"&& uci get %s.%s.%s", filename, section, option,
		filename, section, option);
	
	fp = popen(cmd, "r");
	if(NULL != fp){
		fgets(buf, sizeof(buf), fp);	
		clear_crlf(buf);
		pclose(fp);
		fp = NULL;
		printf("get uci %s.%s.%s=%s.", filename, section, option, buf);
		strncpy(value, buf, val_len < sizeof(buf)?val_len:sizeof(buf));
	}else{
		printf("read uci error.");
	}
	
	return 0;
}

int main(int argc, char **argv)
{
	struct sockaddr *dst = NULL;
	int i = 0;
	int ret = 0;
	int mtu1 = 0, mtu2 = 0;
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	char mtu_mode[8] = {0};
	char mtu_config[8] = {0};
	
	if(get_uci_opt_value("network", "wan", "mtu", mtu_config, sizeof(mtu_config))) {
		printf("get mtu error.");
	}

	/*if(get_uci_opt_value("openwrtapi", "wan", "mtu_mode", mtu_mode, sizeof(mtu_mode))) {
		printf("get mtu_mode error.");
		return 1;
	}

	if(0 != strcmp(mtu_mode, "auto")){
		printf("mtu not auto detect, exit.\n");
		return 0;	
	}*/

	sleep(60); //wait for pppoe-wan up

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;	

	if(argc >= 2 && !strcmp(argv[1], "-d")){
		init_daemon();	
	}
	
	i = 0;
	while(g_website[i]){
		printf("Target: %s\n", g_website[i]);
		ret = getaddrinfo(g_website[i], 0, &hints, &result);	
		if(ret == 0){
			printf("resolve domain name success");
			for(ptr = result; ptr != NULL; ptr = ptr->ai_next){
				switch(ptr->ai_family){
					case AF_UNSPEC:
						printf("Unspecified\n");
						break;
					case AF_INET:
						printf("AF_INET (IPv4)\n");	
						dst = ptr->ai_addr;
						//dst->sa_family = AF_INET;
						printf("\tIPv4 address %s\n", inet_ntoa(((struct sockaddr_in *)dst)->sin_addr));
						break;
					default:
						break;
				}	
				
				if(dst)break;
			}

			break;	
		}else{
			if(2 == i){
				printf("resolve domain name failed");
				exit(1);
			}
		}
		sleep(3);
		i++;
	}

	for(i = 0; i < sizeof(g_mtu_try) && g_mtu_try[i] != 0; i++){
		printf("%d: %d\n", i, g_mtu_try[i]);
		ret = send_icmp_echo(dst, g_mtu_try[i] - sizeof(struct iphdr));
		if(g_nextmtu >= MTU_MIN && g_nextmtu <= MTU_MAX){
			printf("***got nextmtu: [ %d ]***\n", g_nextmtu);
			mtu1 = g_nextmtu;
			break;
		}
	
		if(RESP_OK == ret){
			printf("***got mtu: [ %d ]***\n", g_mtu_try[i]);
			mtu1 = g_mtu_try[i];
			break;	
		}
		sleep(1);
	}

	g_nextmtu = 0;
	/*repeat*/
	for(i = 0; i < sizeof(g_mtu_try) && g_mtu_try[i] != 0; i++){
		printf("%d: %d\n", i, g_mtu_try[i]);
		ret = send_icmp_echo(dst, g_mtu_try[i] - sizeof(struct iphdr));
		if(g_nextmtu >= MTU_MIN && g_nextmtu <= MTU_MAX){
			printf("***got nextmtu: [ %d ]***\n", g_nextmtu);
			mtu2 = g_nextmtu;
			break;
		}
	
		if(RESP_OK == ret){
			printf("***got mtu: [ %d ]***\n", g_mtu_try[i]);
			mtu2 = g_mtu_try[i];
			break;	
		}
		sleep(1);
	}
	/*repeat end*/

	if(mtu1 && mtu2){
		g_mtu = mtu1 > mtu2 ? mtu1 : mtu2;
	}

	if(g_mtu && (g_mtu != (atoi(mtu_config) - PPP_HDR_LEN)) && (g_mtu != 1492)){
		int is_mtk7628 = 1;
		if(is_mtk7628){
			g_mtu += PPP_HDR_LEN;
			if(g_mtu > 1500){
				g_mtu = 1500;
			}
		}
		adjust_mtu(g_mtu);	
	}

	return 0;
}

