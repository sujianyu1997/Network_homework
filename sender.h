#ifndef __SENDER_H__
#define __SENDER_H__

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


void init_sender(Sender *, int, int);
void reset_timer(Sender * sender, int bias1, int bias2);
void sender_window_move(Sender * sender, int rec_id);
int check_incoming_acks(LLnode ** outgoing_frames_head_ptr,
			   Frame * inframe, Sender * sender);
int locate_available_window(Sender * sender, int rec_id);
void * run_sender(void *);

#endif
