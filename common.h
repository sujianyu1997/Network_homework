#ifndef __COMMON_H__
#define __COMMON_H__

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

#define MAX_COMMAND_LENGTH 16
#define AUTOMATED_FILENAME 512
#define MAX_WINDOW_SIZE 8
typedef unsigned char uchar_t;

/*
	modified by Sujianyu 202034071005
*/
//System configuration information 丢包率，误码率等信息
struct SysConfig_t
{
    float drop_prob;
    float corrupt_prob;
    unsigned char automated;
    char automated_file[AUTOMATED_FILENAME];
};
typedef struct SysConfig_t  SysConfig;

//Command line input information
//发送方，接收方，消息内容，uint16_t是windows上的unsigned short int
struct Cmd_t
{
    uint16_t src_id;
    uint16_t dst_id;
    char * message;
};
typedef struct Cmd_t Cmd;

//Linked list information
//链表的枚举信息
enum LLtype 
{
    llt_string, //只有这个是真正起作用的
    llt_frame,
    llt_integer,
    llt_head
} LLtype;

//双向循环链表
struct LLnode_t
{
    struct LLnode_t * prev;
    struct LLnode_t * next;
    enum LLtype type;
    void * value;
};
typedef struct LLnode_t LLnode;



//SendFrame格式
enum SendFrame_DstType 
{
    ReceiverDst,
    SenderDst
} SendFrame_DstType ;

typedef struct Sender_t Sender;
typedef struct Receiver_t Receiver;


#define MAX_FRAME_SIZE 48

//TODO: You should change this!
//Remember, your frame can be AT MOST 48 bytes!
#define FRAME_PAYLOAD_SIZE 41
#define SEQ_MAX 256

struct Frame_h			//帧头5B
{
    unsigned char src_id;			//帧发送者id
    unsigned char dst_id;			//帧接收者id
    unsigned char number; //编号
    unsigned char flag;	//标志，区分seq还是ack，0表示seq，1表示ACK(确认)，2表示NAK(否定应答)
    unsigned char data_length;		//数据长度小于48
};
typedef struct Frame_h FrameHeader;

struct Frame_t //帧头5B + payload41B + crc2B = 48B
{
	FrameHeader header;
    char data[FRAME_PAYLOAD_SIZE];
    unsigned short int crc;	//校验字段，使用CRC校验码
};
typedef struct Frame_t Frame;


struct SWP_t{  //滑动窗口结构体
	unsigned char left_frame_no; //窗口左边界的编号
	unsigned char right_frame_no; //窗口有边界的编号
    unsigned char window_flag[MAX_WINDOW_SIZE];       //8位的标志，window_flag[i] = 0 表示有帧存储，否则没有
	Frame buffer[MAX_WINDOW_SIZE];		    //缓存数据(发送的或者接收的)
};
typedef struct SWP_t SlidingWindow;

//Receiver and sender data structures
struct Receiver_t //接收者结构体
{
    //DO NOT CHANGE:
    // 1) buffer_mutex
    // 2) buffer_cv
    // 3) input_framelist_head
    // 4) recv_id
    pthread_mutex_t buffer_mutex; //linux线程互斥量
    pthread_cond_t buffer_cv; //线程条件变量
    LLnode * input_framelist_head;//待处理信息的队列
    int recv_id;
	int senders;//一个接受者需要处理的发送者数量，因为是广播式
	SlidingWindow * swp;//滑动窗口结构体数组
};

struct Sender_t //发送者结构体
{
    //DO NOT CHANGE:
    // 1) buffer_mutex
    // 2) buffer_cv
    // 3) input_cmdlist_head
    // 4) input_framelist_head
    // 5) send_id
    pthread_mutex_t buffer_mutex; //linux线程互斥量
    pthread_cond_t buffer_cv;    //线程条件变量
    LLnode * input_cmdlist_head; //命令双向链表
    LLnode * input_framelist_head; //待发消息双向链表
    int send_id;
	int receivers; //一个发送者需要处理的者数量，因为是广播式
	SlidingWindow * swp;//滑动窗口结构体数组
    struct timeval (*expiring_timeval)[MAX_WINDOW_SIZE];
    unsigned char (*ack_flag)[MAX_WINDOW_SIZE];
};


//Declare global variables here
//DO NOT CHANGE: 
//   1) glb_senders_array
//   2) glb_receivers_array
//   3) glb_senders_array_length
//   4) glb_receivers_array_length
//   5) glb_sysconfig
//   6) CORRUPTION_BITS
Sender * glb_senders_array;
Receiver * glb_receivers_array;
int glb_senders_array_length;
int glb_receivers_array_length;
SysConfig glb_sysconfig;
int CORRUPTION_BITS;
#endif 
