#include "communicate.h"

//*********************************************************************
//NOTE: We will overwrite this file, so whatever changes you put here
//      WILL NOT persist
//*********************************************************************

/*
	modified by Sujianyu 202034071005
*/
void send_frame(char * char_buffer,
                enum SendFrame_DstType dst_type)
{
    int i = 0, j;
    char * per_recv_char_buffer;

    //Multiply out the probabilities to some degree of precision
    int prob_prec = 1000;
    int drop_prob = (int) prob_prec * glb_sysconfig.drop_prob;
    int corrupt_prob = (int) prob_prec * glb_sysconfig.corrupt_prob;
    int num_corrupt_bits = CORRUPTION_BITS;
    int corrupt_indices[num_corrupt_bits];

    //Pick a random number
    int random_num = rand() % prob_prec;
    int random_index;

    //Drop the packet on the floor
    if (random_num < drop_prob)
    {
        free(char_buffer);
        return;
    }
    
    //Determine whether to corrupt bits
    random_num = rand() % prob_prec;
    if (random_num < corrupt_prob)
    {
        //Pick random indices to corrupt
        for (i=0;
             i < num_corrupt_bits;
             i++)
        {
            random_index = rand() % MAX_FRAME_SIZE;
            corrupt_indices[i] = random_index;
        }
    }

    //Determine the array size of the destination objects
    int array_length = 0;
    if (dst_type == ReceiverDst)
    {
        array_length = glb_receivers_array_length;
    }
    else if (dst_type == SenderDst)
    {
        array_length = glb_senders_array_length;
    }

    //Go through the dst array and add the packet to their receive queues
    for (i=0;
         i < array_length;
         i++)
    {
        //Allocate a per receiver char buffer for the message
        per_recv_char_buffer = (char *) malloc(sizeof(char) * MAX_FRAME_SIZE);
        memcpy(per_recv_char_buffer,
               char_buffer,
               MAX_FRAME_SIZE);
        
        //Corrupt the bits (inefficient, should just corrupt one copy and memcpy it
        if (random_num < corrupt_prob)
        {
            //Corrupt bits at the chosen random indices
            for (j=0;
                 j < num_corrupt_bits;
                 j++)
            {
                random_index = corrupt_indices[j];
                per_recv_char_buffer[random_index] = ~per_recv_char_buffer[random_index];
            }            
        }

        if (dst_type == ReceiverDst)
        {
            Receiver * dst = &glb_receivers_array[i];
            pthread_mutex_lock(&dst->buffer_mutex);
            ll_append_node(&dst->input_framelist_head,
                           (void *) per_recv_char_buffer);
            pthread_cond_signal(&dst->buffer_cv);
            pthread_mutex_unlock(&dst->buffer_mutex);
        }
        else if (dst_type == SenderDst)
        {
            Sender * dst = &glb_senders_array[i];
            pthread_mutex_lock(&dst->buffer_mutex);
            ll_append_node(&dst->input_framelist_head,
                           (void *) per_recv_char_buffer);
            pthread_cond_signal(&dst->buffer_cv);
            pthread_mutex_unlock(&dst->buffer_mutex);
        }
    }
    
    free(char_buffer);
    return;
}

//NOTE: You should use the following method to transmit messages from senders to receivers
void send_msg_to_receivers(char * char_buffer)
{
    send_frame(char_buffer,
               ReceiverDst);
    return;
}

//NOTE: You should use the following method to transmit messages from receivers to senders
void send_msg_to_senders(char * char_buffer)
{
    send_frame(char_buffer,
               SenderDst);
    return;
}
