#include "receiver.h"

/*
	modified by Sujianyu 202034071005
*/
void init_receiver(Receiver * receiver, int senders, int id)
{
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
	receiver->senders = senders;
	receiver->swp = (SlidingWindow *) malloc(senders * sizeof(SlidingWindow));
    int i;
    for (i = 0; i < receiver->senders; i++) {
        receiver->swp[i].left_frame_no = 0;
        receiver->swp[i].right_frame_no = 7;
        memset(receiver->swp[i].window_flag, 0, MAX_WINDOW_SIZE * sizeof(unsigned char));
        memset(receiver->swp[i].buffer, 0, MAX_WINDOW_SIZE * sizeof(Frame));
    }
}
//向outgoing_frames_head_ptr双向列表填充回答报文，run_receiver会不断从这里提取出帧发到senders, 1:ack , 2: nak
void fill_outgoing_frames(LLnode ** outgoing_frames_head_ptr, Frame * inframe, unsigned char flag)
{
	//对inframe发出回答帧
	Frame * outgoingframe = (Frame *) malloc (sizeof(Frame));
	memset(outgoingframe, 0, MAX_FRAME_SIZE);//初始化
	outgoingframe->header.src_id = inframe->header.dst_id;//交换地址，原发送方变接收方
	outgoingframe->header.dst_id = inframe->header.src_id;
	outgoingframe->header.number = inframe->header.number;
	outgoingframe->header.flag = flag;
    outgoingframe->crc = 0;
	//添加crc校验码
	outgoingframe->crc = crc16((unsigned char *)outgoingframe, MAX_FRAME_SIZE);

	ll_append_node(outgoing_frames_head_ptr, convert_frame_to_char(outgoingframe));

    //接收者发送ACK帧
	if(flag == 1)
	{	
		char tmp[1024];
		sprintf(tmp,"Receiver_%d-Sender_%d-ACK", outgoingframe->header.src_id, outgoingframe->header.dst_id);
		print_frame(outgoingframe, tmp);
	}
    //接收者发送NAK帧
	else if(flag == 2)
	{	
		char tmp[1024];
		sprintf(tmp,"Receiver_%d-Sender_%d-NAK", outgoingframe->header.src_id, outgoingframe->header.dst_id);
		print_frame(outgoingframe, tmp);
	}
	free(outgoingframe);
}
void receiver_window_move(Receiver * receiver, int src_id)
{
    char tmp[1024];
    sprintf(tmp, "Receiver_%d-Sender_%d(before): ", receiver->recv_id, src_id);
    receiver_print_window(receiver, src_id, tmp);
    int i;
    for (i = 0; i < 8; i++) {
        if (receiver->swp[src_id].window_flag[i] == 0) {
            break;
        }
        else {
            to_network_layer(receiver->swp[src_id].buffer[i].data, receiver->recv_id);
        }
    }
    receiver->swp[src_id].left_frame_no = (receiver->swp[src_id].left_frame_no + i) % SEQ_MAX;
    receiver->swp[src_id].right_frame_no = (receiver->swp[src_id].right_frame_no + i) % SEQ_MAX;
    unsigned char zero = 0;
    left_loop(receiver->swp[src_id].window_flag, &zero, MAX_WINDOW_SIZE, i, sizeof(unsigned char));
    Frame tmp_f;
    left_loop(receiver->swp[src_id].buffer, &tmp_f, MAX_WINDOW_SIZE, i, sizeof(Frame));
    sprintf(tmp, "Receiver_%d-Sender_%d(after): ", receiver->recv_id, src_id);
    receiver_print_window(receiver, src_id, tmp);
}
void to_network_layer(char * str, int id)
{
    printf("<RECV_%d>:[%s]\n", id, str);
}
int check_incoming_msgs(LLnode ** outgoing_frames_head_ptr, Frame * inframe, Receiver * receiver)
{

    unsigned short int crc = inframe->crc;
    inframe->crc = 0;
    //损坏发NAK帧
    if (crc != crc16((unsigned char *)inframe, MAX_FRAME_SIZE))
    {
        //不是自己的直接返回0
        if (inframe->header.dst_id != receiver->recv_id)
        {
            return 0;
        }
        //目的地址和起始地址损坏，不发NAK
        if (inframe->header.src_id < 0 || inframe->header.src_id >= receiver->senders || inframe->header.dst_id < 0) {
            return 0;
        }
        char tmp[1024];
        sprintf(tmp, "Rec_%d-Corrupted message", receiver->recv_id);
        print_frame(inframe, tmp);
        fill_outgoing_frames(outgoing_frames_head_ptr, inframe, 2);
        return 0;
    }
    //不是自己的直接返回0
    if (inframe->header.dst_id != receiver->recv_id)
    {
        return 0;
    }
    //在窗口内的发ACK帧
    if ((inframe->header.number - receiver->swp[inframe->header.src_id].left_frame_no + SEQ_MAX) % SEQ_MAX <= MAX_WINDOW_SIZE - 1)
    {
        int biass = (inframe->header.number - receiver->swp[inframe->header.src_id].left_frame_no + SEQ_MAX) % SEQ_MAX;
        receiver->swp[inframe->header.src_id].window_flag[biass] = 1;
        memcpy(&receiver->swp[inframe->header.src_id].buffer[biass], inframe, sizeof(Frame));
        //receiver->swp[inframe->header.src_id].buffer[biass] = *inframe;
        fill_outgoing_frames(outgoing_frames_head_ptr, inframe, 1);
        return 1;
    }
    //不在窗口内的发ACK帧
    else
    {
        fill_outgoing_frames(outgoing_frames_head_ptr, inframe, 1);
        return 0;
    }
}
void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the receiver->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        
        if (!check_incoming_msgs(outgoing_frames_head_ptr, inframe, receiver))
        {
            free(raw_char_buf);
            free(inframe);
            free(ll_inmsg_node);
            incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
            continue;
        }
        receiver_window_move(receiver, inframe->header.src_id);
        //Free raw_char_buf
        free(raw_char_buf);
        

        //printf("<RECV_%d>:[%s]\n", receiver->recv_id, inframe->data);

        free(inframe);
        free(ll_inmsg_node);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    }
}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);
    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);
        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
}
