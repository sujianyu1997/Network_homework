#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

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

void send_msg_to_receivers(char * );
void send_msg_to_senders(char * );
void send_frame(char * ,
                enum SendFrame_DstType);

#endif
