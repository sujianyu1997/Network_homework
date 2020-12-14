#ifndef __INPUT_H__
#define __INPUT_H__

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
#define DEFAULT_INPUT_BUFFER_SIZE 1024

void * run_stdinthread(void *);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif

