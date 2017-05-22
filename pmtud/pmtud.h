/***************************************************************************
* File name:     pmtud.h
* Author:        renleilei
* Description:   
* Others: 
* Last modified: 2015-07-20 17:43
***************************************************************************/

#ifndef __PMTUD_H__
#define __PMTUD_H__

#include <stdio.h>

enum {
	RESP_OK,
	RESP_NEED_FRAG,
	RESP_NORESP
}; 

#define MTU_MIN 596

#define MTU_MAX 1500

int send_icmp_echo(struct sockaddr *dst, int mtu);

#endif

