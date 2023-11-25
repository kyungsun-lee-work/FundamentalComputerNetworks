#include <deque>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../include/simulator.h"

using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define RTT 20

deque<pkt> msg_buffer;
unique_ptr<bool> a_alt_seq = make_unique<bool>(false);
unique_ptr<bool> b_alt_seq = make_unique<bool>(false);
unique_ptr<bool> is_wait_for_ack = make_unique<bool>(false);

void alternate_seq(std::unique_ptr<bool>& current_seq)
{
    *current_seq = !(*current_seq);
}

int get_checksum(struct pkt *packet)
{
    int checksum = 0;
    checksum += packet->seqnum;
    checksum += packet->acknum;
    for (int i = 0; i < 20; ++i)
        checksum += packet->payload[i];
    return checksum;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{  
    printf("A_output: push msg to buffer! %s\n", message.data); 
    struct pkt packet_to_send;
    packet_to_send.seqnum = 0;
    memmove(packet_to_send.payload, message.data, 20);
    packet_to_send.checksum = get_checksum(&packet_to_send);
    msg_buffer.push_back(packet_to_send);

    if(*is_wait_for_ack == false)
    {
        printf("A_ouput: Send msg: %s, a_seq: %d\n", 
            msg_buffer.begin()->payload, *a_alt_seq);
        msg_buffer.begin()->seqnum = *a_alt_seq;
        msg_buffer.begin()->checksum = get_checksum(&(msg_buffer.front()));
        starttimer (0, RTT);
        tolayer3 (0, msg_buffer.front());
        *is_wait_for_ack = true;   
    }  	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{     
    if(packet.acknum == *a_alt_seq &&
        (get_checksum(&packet) == packet.checksum))
    {
        printf("A_input: received correct ack[%d] \n", packet.acknum);
        stoptimer(0);
        *is_wait_for_ack = false;
        alternate_seq(a_alt_seq);        
        if(!msg_buffer.empty())
        {
            printf("A_input: delete acked msg from buffer.");
            msg_buffer.erase(msg_buffer.begin());
        }        
        if(!msg_buffer.empty())
        {
            printf("A_input: Send msg: %s, a_seq: %d\n",
                msg_buffer.begin()->payload, *a_alt_seq);
            msg_buffer.begin()->seqnum = *a_alt_seq;
            msg_buffer.begin()->checksum = get_checksum(&msg_buffer.front());
            tolayer3 (0, msg_buffer.front());
            starttimer (0, RTT);
            *is_wait_for_ack = true;            
        }
    }
    else
    {
        printf("A_input: msg corrupted! Send msg again: %s , a_seq: %d\n",
	    msg_buffer.begin()->payload, *a_alt_seq);
	msg_buffer.begin()->seqnum = *a_alt_seq;
	msg_buffer.begin()->checksum = get_checksum(&msg_buffer.front());
	tolayer3 (0, msg_buffer.front());
	stoptimer(0);
	starttimer (0, RTT);
	*is_wait_for_ack = true;		
    }	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    if(!msg_buffer.empty())
    {
        printf("A_timerinterrupt: Send msg %s, a_seq: %d\n",
            msg_buffer.begin()->payload, *a_alt_seq);
        msg_buffer.begin()->seqnum = *a_alt_seq;
        msg_buffer.begin()->checksum = get_checksum(&msg_buffer.front());
        tolayer3(0, msg_buffer.front());
        starttimer(0, RTT);
        *is_wait_for_ack = true;
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it tseo do any initialization */
void A_init()
{
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    printf("B_input: Packet checksun[%d], seqnum [%d], b_state[%d] \n ",
        packet.checksum, packet.seqnum, *b_alt_seq);
    if(get_checksum(&packet) == packet.checksum &&
        packet.seqnum == *b_alt_seq)
    {	
        struct pkt packet_acked; 
        packet_acked.acknum = *b_alt_seq;
        packet_acked.seqnum = packet.seqnum;
        memmove(packet_acked.payload, packet.payload, 20);
        packet_acked.checksum = get_checksum(&packet_acked);
        
        printf("B_input: packet acknowledged! \n");
        tolayer5 (1, packet.payload);
        alternate_seq(b_alt_seq);
        tolayer3 (1, packet_acked);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
}


