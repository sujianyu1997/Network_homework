#include "sender.h"
/*
	modified by Sujianyu 202034071005
*/
void init_sender(Sender * sender, int receivers, int id)
{
    //TODO: You should fill in this function as necessary
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
	sender->receivers = receivers;
	sender->swp = (SlidingWindow *) malloc(receivers * sizeof(SlidingWindow));
    sender->expiring_timeval = (struct timeval(*)[MAX_WINDOW_SIZE]) malloc(sizeof(struct timeval) * receivers * MAX_WINDOW_SIZE);
    sender->ack_flag = (unsigned char(*)[MAX_WINDOW_SIZE]) malloc(sizeof(unsigned char) * receivers * MAX_WINDOW_SIZE);
    int i;
    int j;
    for (i = 0; i < receivers; i++) {
        for (j = 0; j < MAX_WINDOW_SIZE; j++) {
            sender->expiring_timeval[i][j].tv_sec = 0;
            sender->expiring_timeval[i][j].tv_usec = 0;
        }
    }
    for (i = 0; i < sender->receivers; i++) {
        sender->swp[i].left_frame_no = 0;
        sender->swp[i].right_frame_no = 7;
        memset(sender->swp[i].window_flag, 0, MAX_WINDOW_SIZE * sizeof(unsigned char));
        memset(sender->swp[i].buffer, 0, MAX_WINDOW_SIZE * sizeof(Frame));
    }
}

//重新设置某个帧的超时时间
void reset_timer(Sender * sender, int bias1, int bias2, long int sec, unsigned long int usec) {
    sender->expiring_timeval[bias1][bias2].tv_sec = sec;
    sender->expiring_timeval[bias1][bias2].tv_usec = usec;
}

//获取该发送者的最早超时的时间
struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    int i, j;
    int index1 = 0, index2 = 0;
    for (i = 0; i < sender->receivers; i++){
        for (j = 0; j < MAX_WINDOW_SIZE; j++) {
            if (sender->expiring_timeval[i][j].tv_sec < sender->expiring_timeval[index1][index2].tv_sec && sender->expiring_timeval[i][j].tv_usec < sender->expiring_timeval[index1][index2].tv_usec) {
                index1 = i;
                index2 = j;
            }
        }
    }
    if (sender->expiring_timeval[index1][index2].tv_sec == 0 && sender->expiring_timeval[index1][index2].tv_usec == 0) {
        return NULL;
    }
    else {
        return &sender->expiring_timeval[index1][index2];
    }
}
void sender_window_move(Sender * sender, int rec_id)
{
    char tmp[1024];
    sprintf(tmp, "Sender_%d-Receiver_%d(before): ", sender->send_id, rec_id);
    sender_print_window(sender, rec_id, tmp);
    int i;
    for (i = 0; i < 8; i++) {
        if (sender->ack_flag[rec_id][i] == 0) {
            break;
        }
    }
    sender->swp[rec_id].left_frame_no = (sender->swp[rec_id].left_frame_no + i) % SEQ_MAX;
    sender->swp[rec_id].right_frame_no = (sender->swp[rec_id].right_frame_no + i) % SEQ_MAX;
    unsigned char zero = 0;
    
    left_loop(sender->swp[rec_id].window_flag, &zero, MAX_WINDOW_SIZE, i, sizeof(unsigned char));
    // for (int i = 0; i < 8; i++) {
    //     fprintf(stderr, "%d ", sender->swp[rec_id].window_flag[i]);
    // }
    // fprintf(stderr, "\n");
    struct timeval time_z;
    time_z.tv_sec = 0;
    time_z.tv_usec = 0;
    left_loop(sender->ack_flag[rec_id], &zero, MAX_WINDOW_SIZE, i, sizeof(unsigned char));
    Frame tmp_f;
    left_loop(sender->swp[rec_id].buffer, &tmp_f, MAX_WINDOW_SIZE, i, sizeof(Frame));

    left_loop(sender->expiring_timeval[rec_id], &time_z, MAX_WINDOW_SIZE, i, sizeof(struct timeval));
    
    sprintf(tmp, "Sender_%d-Receiver_%d(after): ", sender->send_id, rec_id);
    sender_print_window(sender, rec_id, tmp);
}


int check_incoming_acks(LLnode ** outgoing_frames_head_ptr,
			   Frame * inframe, Sender * sender)
{
    unsigned short int crc = inframe->crc;
    inframe->crc = 0;
    //检测损坏
    if (crc != crc16((unsigned char *)inframe, MAX_FRAME_SIZE))
    {
        //不是自己的直接返回0
        if (inframe->header.dst_id != sender->send_id)
        {
            return 0;
        }
        char tmp[1024];
        sprintf(tmp, "Sender_%d-Corrupt reply message: ", sender->send_id);
        print_frame(inframe, tmp);
        return 0;
    }
    //不是自己的直接返回0
    if (inframe->header.dst_id != sender->send_id)
    {
        return 0;
    }
    //落入窗口内
    int bias = (inframe->header.number - sender->swp[inframe->header.src_id].left_frame_no + SEQ_MAX) % SEQ_MAX;
    if ((inframe->header.number - sender->swp[inframe->header.src_id].left_frame_no + SEQ_MAX) % SEQ_MAX <= MAX_WINDOW_SIZE - 1 && sender->swp[inframe->header.src_id].window_flag[bias])
    {
        //如果是ack则把对应的帧的flag置为1
        if (inframe->header.flag == 1) {
            sender->ack_flag[inframe->header.src_id][bias] = 1;
            return 1;
        }
        //如果收到nak帧则设置一个超时的帧，例如(1, 1)
        else if (inframe->header.flag == 2) {
            reset_timer(sender, inframe->header.src_id, bias, 1, 1);
            return 0;
        }
        
    }
    return 0;
}
void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair
    int incoming_acks_length = ll_get_length(sender->input_framelist_head);
    while (incoming_acks_length > 0)                                    //此函数会从有消息一直干到没消息
    {
        LLnode * ll_inmsg_node = ll_pop_node(&sender->input_framelist_head);

        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);

        if (!check_incoming_acks(outgoing_frames_head_ptr, inframe, sender))
        {
            free(raw_char_buf);
            free(inframe);
            free(ll_inmsg_node);
            incoming_acks_length = ll_get_length(sender->input_framelist_head);
            continue;
        }
        sender_window_move(sender, inframe->header.src_id);
        incoming_acks_length = ll_get_length(sender->input_framelist_head);
    }
}
//返回某接收者窗口的可用发送空间，index = -1表示没有空间，index表示偏移left_frame_no的偏移量
int locate_available_window(Sender * sender, int rec_id) {
    int i, index = -1;
    for (i = 0; i < MAX_WINDOW_SIZE; i++) {
        if (sender->swp[rec_id].window_flag[i] == 0) {
            index = i;
            return index;
        }
    }
    return index;
}

void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    
        
    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        //Pop a node off and update the input_cmd_length
        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);
        
        int index = locate_available_window(sender, outgoing_cmd->dst_id);
        //说明发送窗口没位置了，把命令重新入队
        if (index == -1) {
            ll_append_node(&sender->input_cmdlist_head, (void *) outgoing_cmd);
            continue;
        }

        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = strlen(outgoing_cmd->message);
        if (msg_length > FRAME_PAYLOAD_SIZE)
        {
            //Do something about messages that exceed the frame size

            printf("<SEND_%d>: sending messages of length greater than %d is not implemented\n", sender->send_id, MAX_FRAME_SIZE);
        }
        else
        {
            //This is probably ONLY one step you want
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            memset(outgoing_frame, 0, MAX_FRAME_SIZE);//初始化
            strcpy(outgoing_frame->data, outgoing_cmd->message);

            outgoing_frame->header.data_length = msg_length;
            outgoing_frame->header.dst_id = outgoing_cmd->dst_id;
            outgoing_frame->header.src_id = outgoing_cmd->src_id;
            outgoing_frame->header.number = (sender->swp[outgoing_cmd->dst_id].left_frame_no + index) % SEQ_MAX;
            outgoing_frame->header.flag = 0;
            outgoing_frame->crc = crc16((unsigned char *)outgoing_frame, MAX_FRAME_SIZE);
            memcpy(&sender->swp[outgoing_cmd->dst_id].buffer[index], outgoing_frame, sizeof(Frame));
            //sender->swp[outgoing_cmd->dst_id].buffer[index] = *outgoing_frame;
            struct timeval    curr_timeval;
            gettimeofday(&curr_timeval, 
                     NULL);
            sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_sec = curr_timeval.tv_sec;
            sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_usec = curr_timeval.tv_usec + 100000;
            if (sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_usec >= 1000000) {
                sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_usec -= 1000000;
                sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_sec++;
            }
            //fprintf(stderr, "%.2f\n", sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_usec /1000 + sender->expiring_timeval[outgoing_cmd->dst_id][index].tv_sec * 1000);
            sender->swp[outgoing_cmd->dst_id].window_flag[index] = 1;
            //At this point, we don't need the outgoing_cmd
            free(outgoing_cmd->message);
            free(outgoing_cmd);
            char tmp[1024];
            sprintf(tmp, "Sender_%d-Receiver_%d-SEQ: ", outgoing_frame->header.src_id, outgoing_frame->header.dst_id);
            print_frame(outgoing_frame, tmp);
            //Convert the message to the outgoing_charbuf
            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            ll_append_node(outgoing_frames_head_ptr,
                           outgoing_charbuf);
            free(outgoing_frame);
        }
    }   
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    int i, j;
    struct timeval curr_timeval;
    for (i = 0; i < sender->receivers; i++) {
        for (j = 0; j < MAX_WINDOW_SIZE; j++) {
            if (sender->expiring_timeval[i][j].tv_sec == 0 && sender->expiring_timeval[i][j].tv_usec == 0) {
                continue;
            }
            else {
                gettimeofday(&curr_timeval,  NULL);
                //如果过时了，那么重新发送
                if (timeval_usecdiff(&sender->expiring_timeval[i][j], &curr_timeval)) {
                    ll_append_node(outgoing_frames_head_ptr, convert_frame_to_char(&sender->swp[i].buffer[j]));
                    sender->expiring_timeval[i][j].tv_sec = curr_timeval.tv_sec;
                    sender->expiring_timeval[i][j].tv_usec = curr_timeval.tv_usec + 100000;
                    if (sender->expiring_timeval[i][j].tv_usec >= 1000000) {
                        sender->expiring_timeval[i][j].tv_usec -= 1000000;
                        sender->expiring_timeval[i][j].tv_sec++;
                    }
                    char tmp[1024];
                    sprintf(tmp, "Sender_%d_Receiver_%d-SEQ-time_out: ", sender->send_id, i);
                    print_frame(&sender->swp[i].buffer[j], tmp);
                }
            }
        }
    }
}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        //等待0.1s
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            //最新的事件还没到，至少需要等sleep_sec_time的时间
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }
            //不然的话，不需要等，直接检测有啥事件   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
