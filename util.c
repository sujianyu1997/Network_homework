#include "util.h"

//Linked list functions
/*
	modified by Sujianyu 202034071005
*/
int ll_get_length(LLnode * head)
{
    LLnode * tmp;
    int count = 1;
    if (head == NULL)
        return 0;
    else
    {
        tmp = head->next;
        while (tmp != head)
        {
            count++;
            tmp = tmp->next;
        }
        return count;
    }
}

void ll_append_node(LLnode ** head_ptr, 
                    void * value)
{
    LLnode * prev_last_node;
    LLnode * new_node;
    LLnode * head;

    if (head_ptr == NULL)
    {
        return;
    }
    
    //Init the value pntr
    head = (*head_ptr);
    new_node = (LLnode *) malloc(sizeof(LLnode));
    new_node->value = value;

    //The list is empty, no node is currently present
    if (head == NULL)
    {
        (*head_ptr) = new_node;
        new_node->prev = new_node;
        new_node->next = new_node;
    }
    else
    {
        //Node exists by itself
        prev_last_node = head->prev;
        head->prev = new_node;
        prev_last_node->next = new_node;
        new_node->next = head;
        new_node->prev = prev_last_node;
    }
}


LLnode * ll_pop_node(LLnode ** head_ptr)
{
    LLnode * last_node;
    LLnode * new_head;
    LLnode * prev_head;

    prev_head = (*head_ptr);
    if (prev_head == NULL)
    {
        return NULL;
    }
    last_node = prev_head->prev;
    new_head = prev_head->next;

    //We are about to set the head ptr to nothing because there is only one thing in list
    if (last_node == prev_head)
    {
        (*head_ptr) = NULL;
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
    else
    {
        (*head_ptr) = new_head;
        last_node->next = new_head;
        new_head->prev = last_node;

        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
}

void ll_destroy_node(LLnode * node)
{
    if (node->type == llt_string)
    {
        free((char *) node->value);
    }
    free(node);
}

//Compute the difference in usec for two timeval objects
//返回区间范围的微秒
long timeval_usecdiff(struct timeval *start_time, 
                      struct timeval *finish_time)
{
  long usec;
  usec=(finish_time->tv_sec - start_time->tv_sec)*1000000;
  usec+=(finish_time->tv_usec- start_time->tv_usec);
  return usec;
}
//循环左移
void left_loop (void * p, void * tmp, int n, int k, int size)
{
    int i, j;
    for (j = 0; j < k; j++)
    {
        for (i = 0; i < n - 1; i++)
        {
            memcpy((p + i * size), (p + (i + 1) * size), size);
        }
    }
    for (i = n - k; i < n; i++)
    {
        memcpy((p + i * size), tmp, size);
    }
}
//Print out messages entered by the user
void print_cmd(Cmd * cmd)
{
    fprintf(stderr, "src=%d, dst=%d, message=%s\n", 
           cmd->src_id,
           cmd->dst_id,
           cmd->message);
}
//打印帧内容
void print_frame(Frame * frame, char * str)
{
	char * type;
	if (frame->header.flag == 0)
	{
		type = "SEQ";
	}
	else if (frame->header.flag == 1)
	{
		type = "ACK";
	}
	else
	{
		type = "NAK";
	}
	fprintf(stderr, "%-40s frame_content: number=%d,src_id=%d,dst_id=%d,type=%s,data_length=%d,data=%s,crc=%x\n",
          str,
          frame->header.number,
          frame->header.src_id,
          frame->header.dst_id,
          type,
          frame->header.data_length,
		  frame->data,
          frame->crc
           );
}
void sender_print_window(Sender * sender, int rec_id, char * str) {
    SlidingWindow * swp = &sender->swp[rec_id];
    fprintf(stderr, "%-40s window:[%d%s%s, %d%s%s, %d%s%s, %d%s%s, %d%s%s, %d%s%s, %d%s%s, %d%s%s] \t", 
    str,
    swp->left_frame_no, (swp->window_flag[0] ? "*" : " "), (sender->ack_flag[rec_id][0] ? "D" : " "), 
    (swp->left_frame_no + 1) % SEQ_MAX, (swp->window_flag[1] ? "*" : " "), (sender->ack_flag[rec_id][1] ? "D" : " "), 
    (swp->left_frame_no + 2) % SEQ_MAX, (swp->window_flag[2] ? "*" : " "), (sender->ack_flag[rec_id][2] ? "D" : " "), 
    (swp->left_frame_no + 3) % SEQ_MAX, (swp->window_flag[3] ? "*" : " "), (sender->ack_flag[rec_id][3] ? "D" : " "), 
    (swp->left_frame_no + 4) % SEQ_MAX, (swp->window_flag[4] ? "*" : " "), (sender->ack_flag[rec_id][4] ? "D" : " "), 
    (swp->left_frame_no + 5) % SEQ_MAX, (swp->window_flag[5] ? "*" : " "), (sender->ack_flag[rec_id][5] ? "D" : " "), 
    (swp->left_frame_no + 6) % SEQ_MAX, (swp->window_flag[6] ? "*" : " "), (sender->ack_flag[rec_id][6] ? "D" : " "), 
    (swp->left_frame_no + 7) % SEQ_MAX, (swp->window_flag[7] ? "*" : " "), (sender->ack_flag[rec_id][7] ? "D" : " "));

    struct timeval    curr_timeval;
    gettimeofday(&curr_timeval, 
                     NULL);
    fprintf(stderr, "window_left_time(ms):[%.2lf, %.2lf, %.2lf, %.2lf, %.2lf, %.2lf, %.2lf, %.2lf]\n", 
    sender->swp[rec_id].window_flag[0] == 0 ? 0 : (sender->expiring_timeval[rec_id][0].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][0].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[1] == 0 ? 0 : (sender->expiring_timeval[rec_id][1].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][1].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[2] == 0 ? 0 : (sender->expiring_timeval[rec_id][2].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][2].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[3] == 0 ? 0 : (sender->expiring_timeval[rec_id][3].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][3].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[4] == 0 ? 0 : (sender->expiring_timeval[rec_id][4].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][4].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[5] == 0 ? 0 : (sender->expiring_timeval[rec_id][5].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][5].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[6] == 0 ? 0 : (sender->expiring_timeval[rec_id][6].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][6].tv_usec - curr_timeval.tv_usec) / 1000,
    sender->swp[rec_id].window_flag[7] == 0 ? 0 : (sender->expiring_timeval[rec_id][7].tv_sec - curr_timeval.tv_sec) *1000 + (double)(sender->expiring_timeval[rec_id][7].tv_usec - curr_timeval.tv_usec) / 1000
    );
}

void receiver_print_window(Receiver * receiver, int send_id, char * str) {
    SlidingWindow * swp = &receiver->swp[send_id];
    fprintf(stderr, "%-40s window:[%d%s, %d%s, %d%s, %d%s, %d%s, %d%s, %d%s, %d%s] \n", 
    str,
    swp->left_frame_no, (swp->window_flag[0] ? "*" : " "),
    (swp->left_frame_no + 1) % SEQ_MAX, (swp->window_flag[1] ? "*" : " "),
    (swp->left_frame_no + 2) % SEQ_MAX, (swp->window_flag[2] ? "*" : " "),
    (swp->left_frame_no + 3) % SEQ_MAX, (swp->window_flag[3] ? "*" : " "),
    (swp->left_frame_no + 4) % SEQ_MAX, (swp->window_flag[4] ? "*" : " "),
    (swp->left_frame_no + 5) % SEQ_MAX, (swp->window_flag[5] ? "*" : " "),
    (swp->left_frame_no + 6) % SEQ_MAX, (swp->window_flag[6] ? "*" : " "),
    (swp->left_frame_no + 7) % SEQ_MAX, (swp->window_flag[7] ? "*" : " "));
}
char * convert_frame_to_char(Frame * frame)
{
    //TODO: You should implement this as necessary
    char * char_buffer = (char *) malloc(sizeof(Frame));
    memset(char_buffer,
           0,
           sizeof(Frame));
	//小改动，frame->data改成了frame，因为添加了新的变量
    memcpy(char_buffer, 
           frame,
           sizeof(Frame));
    return char_buffer;
}


Frame * convert_char_to_frame(char * char_buf)
{
    //TODO: You should implement this as necessary
    Frame * frame = (Frame *) malloc(sizeof(Frame));
	//初始化
    memset(frame,
           0,
           sizeof(Frame));
	//和上一个函数对应，直接用memcpy就行了
    memcpy(frame, 
           char_buf,
           sizeof(Frame));
    return frame;
}

//权威的crc16做法
unsigned short int crc16(unsigned char *ptr, unsigned int len) 
{
	unsigned short int crc = 0;
	unsigned char da = 0;
	unsigned short int crc_ta[256] = { /* CRC 余式表 */
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
	};
	crc = 0;
	while (len-- != 0) {
		da = (unsigned char)(crc / 256);/* 以8位二进制数的形式暂存CRC的高8位 */
		crc <<= 8; 		/* 左移8位,相当于CRC的低8位乘以28 */
		crc ^= crc_ta[ da ^ *ptr ]; /* 高8位和当前字节相加后再查表求CRC,再加上以前的CRC */
		ptr++;
	}
	return (crc);
}