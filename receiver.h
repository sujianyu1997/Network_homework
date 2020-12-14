#ifndef __RECEIVER_H__
#define __RECEIVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include "common.h"
#include "util.h"
#include "communicate.h"

void init_receiver(Receiver *, int, int);
//交付给网络层
void to_network_layer(char * str, int id);
void fill_outgoing_frames(LLnode ** outgoing_frames_head_ptr, Frame * inframe,  
		     unsigned char flag);
void window_move(Receiver * receiver, int src_id);
int check_incoming_msgs(LLnode ** outgoing_frames_head_ptr,
			   Frame * inframe, Receiver * receiver);
void * run_receiver(void *);

#endif
