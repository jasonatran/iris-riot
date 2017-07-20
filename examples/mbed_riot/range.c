/**
 * Copyright (c) 2017, Autonomous Networks Research Group. All rights reserved.
 * Developed by:
 * Autonomous Networks Research Group (ANRG)
 * University of Southern California
 * http://anrg.usc.edu/
 *
 * Contributors:
 * Yutong Gu
 * Richard Kim
 *
 * Permission is here
 * by granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * - Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimers in the 
 *     documentation and/or other materials provided with the distribution.
 * - Neither the names of Autonomous Networks Research Group, nor University of 
 *     Southern California, nor the names of its contributors may be used to 
 *     endorse or promote products derived from this Software without specific 
 *     prior written permission.
 * - A citation to the Autonomous Networks Research Group must be included in 
 *     any publications benefiting from the use of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH 
 * THE SOFTWARE.
 */
/**
 * @ingroup     examples
 * @{
 *
 * @file        range.c
 * @brief       Ultrasound ranging library for localization
 *
 * @author      Yutong Gu <yutonggu@usc.edu>
 *
 * @}
 */

#include "range.h"
#define ENABLE_DEBUG (1)
#include "debug.h"

#define MAXSAMPLES_ONE_PIN            18000
#define MAXSAMPLES_TWO_PIN            18000

#define RX_ONE_PIN                    GPIO_PIN(3, 3) //aka GPIO_PD3 - maps to DIO0
#define RX_TWO_PIN                    GPIO_PIN(3, 2) //aka GPIO_PD2 - maps to DIO1
#define RX_XOR_PIN                    GPIO_PIN(3, 1) //aka GPIO_PD1 - maps to DIO2

#define TX_PIN                        GPIO_PIN(3, 2) //aka GPIO_PD2 - maps to DIO1 //for usb openmote
// #define TX_PIN                        GPIO_PIN(3, 0) //aka GPIO_PD0 - maps to DIO3 

/*----------------------------------------------------------------------------*/
int range_tx( void )
{
// SETUP
    // Radio setup.
    // TODO: adjust channel.
    //------------------------------------------------------------------------//
    /* for sending L2 pkt */
    kernel_pid_t dev;
    uint8_t hw_addr[MAX_ADDR_LEN];
    size_t hw_addr_len;

    gnrc_pktsnip_t *pkt, *hdr;
    gnrc_netif_hdr_t *nethdr;
    uint8_t flags = 0x00;    
    uint8_t buf[3] = {0x00, 0x00, 0x00}; 

    int16_t tx_power = TX_POWER;

    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    size_t numof = gnrc_netif_get(ifs); 

    /* there should be only one network interface on the board */
    if (numof == 1) {
        gnrc_netapi_set(ifs[0], NETOPT_TX_POWER, 0, &tx_power, sizeof(int16_t));
    }

    // Broadcasting flag setup.
    flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
    //------------------------------------------------------------------------//

    // Ultrasonic sensor setup.
    //------------------------------------------------------------------------//
    /* enable output on Port D pin 3 */
    if(gpio_init(TX_PIN, GPIO_OUT) < 0) {
        DEBUG("Error initializing GPIO_PIN.\n");
        return 1;
    }
    // Clearing output for the ultrasonic sensor.
    gpio_clear(TX_PIN);

    range_tx_init(TX_PIN);
    //------------------------------------------------------------------------//
    
    // Miscellaneous setup.
    //------------------------------------------------------------------------//
    
    // Ranking of the node.
    char ranking;
    
    // Total number of nodes.
    char total_number_of_nodes;

    //bool no_signal;
    //xtimer_ticks32_t rstx_start_time;
    //uint8_t *hw_addr_nodes[NUM_OF_NODES]; // array of hw_addr of the nodes
    // NETOPT_ADDRESS; // this device's hw_addr 
    //char place_on_list = 0;
    // for processing the time from the leader node.
    // uint32_t time_processing = 0x00000000;
    // xtimer_ticks32_t offset_time;
    //------------------------------------------------------------------------//

// NODE sends REQ packet to LEADER. - put in loop
    /** Send L2 Packet **/
    /* network interface */
    dev = ifs[0];
    hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), LEADER_HW_ADDR);
    
    /* put packet together */
    // pkt structure: {NODE_HW_ADDR_LAST_4_DIGITS[2], FOLLOW_REQ[1]}
    buf[0] = 0x00; //set to flag laster
    buf[1] = 0x00;
    buf[2] = 0x00;
    pkt = gnrc_pktbuf_add(NULL, buf, 3, GNRC_NETTYPE_UNDEF);
    if (pkt == NULL) {
        DEBUG("error: packet buffer full\n");
        return 1;
    }
   
    hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
    if (hdr == NULL) {
        DEBUG("error: packet buffer full\n");
        gnrc_pktbuf_release(pkt);
        return 1;
    }
    LL_PREPEND(pkt, hdr);
    nethdr = (gnrc_netif_hdr_t *)hdr->data;
    nethdr->flags = flags;
    /* ready to send */
    
    //make sure no packets are to be sent!!
    if (gnrc_netapi_send(dev, pkt) < 1) { // check we don't receive our own packets, dump if that's the case
        DEBUG("error: unable to send\n");
        gnrc_pktbuf_release(pkt);
        return 1;
    }   

    while(true)
    {
        msg_receive(&msg);
        pkt = msg.content.ptr;
 

 // xtimer receive message with timeout
 // 200 ms timeout???
 // find gnrc_netapi_receive
 // then parse as l2 packet
 
        /* get snip containing packet data where we put the packet number */
        snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);

        char indicator = ((uint8_t *)snip->data)[2]; //strncpy?

        switch(indicator)
        // switch (msg.type)
        {
            // NODE receives FOLLOW_ASSIGN packet from LEADER.
            // NODE records its ranking.
            case FOLLOW_ASSIGN:
                pkt = msg.content.ptr;
 
                /* get snip containing packet data where we put the packet number */
                snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);
                
                // Record ranking. 
                ranking = (int)((uint8_t *)snip->data)[2];
 
                // Record total number of nodes in the field.
                total_number_of_nodes = (int)((uint8_t *)snip->data)[3];
                
                gnrc_pktbuf_release(pkt);

                // Send ACK packet.
                /** Send L2 Packet **/
                /* network interface */
                dev = ifs[0];
                hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), LEADER_HW_ADDR);
                
                /* put packet together */
                // pkt structure: {NODE_HW_ADDR_LAST_4_DIGITS[2], FOLLOW_REQ[1]}
                buf[0] = (uint8_t)(NETOPT_ADDRESS >> 8);
                buf[1] = (uint8_t)(NETOPT_ADDRESS);
                buf[2] = LEAD_ACK;
                pkt = gnrc_pktbuf_add(NULL, buf, 3, GNRC_NETTYPE_UNDEF);
                if (pkt == NULL) {
                    DEBUG("error: packet buffer full\n");
                    return 1;
                }
               
                hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
                if (hdr == NULL) {
                    DEBUG("error: packet buffer full\n");
                    gnrc_pktbuf_release(pkt);
                    return 1;
                }
                LL_PREPEND(pkt, hdr);
                nethdr = (gnrc_netif_hdr_t *)hdr->data;
                nethdr->flags = flags;
                /* ready to send */
                
                //make sure no packets are to be sent!!
                if (gnrc_netapi_send(dev, pkt) < 1) {
                    DEBUG("error: unable to send\n");
                    gnrc_pktbuf_release(pkt);
                    return 1;
                }   
                break;
            case FOLLOW_SYNC:
            // - All nodes sync to timestamp.
                // Pull time from FOLLOW_SYNC from the leader node.
                pkt = msg.content.ptr;
 
                /* get snip containing packet data where we put the packet number */
                snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);
 
                // uint8_t = 0x00; // == char
                // uint32_t = 0x00000000;
                // data from leader with this signal will run from 0 to 4, 1-4 holds the 
                // address. 
                // sloppy, see if this works
                for(int x = 0; x < 4; x++)
                {
                    part_of_time = ((uint8_t *) snip->data)[x+1];
                    time_processing = (time_processing << 8) | part_of_time;
                }
                gnrc_pktbuf_release(pkt);
                // syncing...
                offset_time = sync_time( (xtimer_ticks32_t)time_processing);
 
                // - All nodes randomly send hw_addr.
                // - If successful, node will listen for ASSIGN signal.
                // - If failed (collision), node will wait (# of nodes) * time_window
                xtimer_usleep(r * TIME_WINDOW);
                /** Send L2 Packet **/
                /* network interface */
                dev = ifs[0];
                hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), LEADER_HW_ADDR);

                /* put packet together */

                // add in appropriate flags, collision checking
                buf[0] = LEAD_INFO;
                buf[1] = tx_node_id1;
                buf[2] = tx_node_id2;
                pkt = gnrc_pktbuf_add(NULL, &buf, sizeof(buf), GNRC_NETTYPE_UNDEF);
                if (pkt == NULL) {
                    DEBUG("error: packet buffer full");
                    return 1;
                }   
                break;
            case FOLLOW_GO:
            // syncing...//
            // - All nodes sync to timestamp.
 
                // Pull time from FOLLOW_SYNC from the leader node.
                pkt = msg.content.ptr;
 
                /* get snip containing packet data where we put the packet number */
                snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);
 
                // uint8_t = 0x00; // == char
                // uint32_t = 0x00000000;
                // data from leader with this signal will run from 0 to 4, 1-4 holds the 
                // address. 
                // sloppy, see if this works
                for(int x = 0; x < 4; x++)
                {
                    part_of_time = ((uint8_t *) snip->data)[x+1];
                    time_processing = (time_processing << 8) | part_of_time;
                }
                gnrc_pktbuf_release(pkt);
 
                // syncing...
                offset_time = sync_time( (xtimer_ticks32_t)time_processing);
                 
                //ACUTALLY SENDING THE SIGNAL GOD FINALLY
 
                // TODO: Modify to not block the interrupts
                xtimer_usleep(place_on_list * TIME_WINDOW);
                /** Send L2 Packet **/
                /* network interface */
                dev = ifs[0];
                // hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), RANGE_RX_HW_ADDR);
                hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), LEADER_HW_ADDR);

                /* put packet together */
                buf[0] = RANGE_FLAG_BYTE0;
                buf[1] = RANGE_FLAG_BYTE1;
                buf[2] = TX_NODE_ID;
                pkt = gnrc_pktbuf_add(NULL, &buf, 3, GNRC_NETTYPE_UNDEF);
                if (pkt == NULL) {
                    DEBUG("error: packet buffer full\n");
                    return 1;
                }
   
                hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
                if (hdr == NULL) {
                    DEBUG("error: packet buffer full\n");
                    gnrc_pktbuf_release(pkt);
                    return 1;
                }
                LL_PREPEND(pkt, hdr);
                nethdr = (gnrc_netif_hdr_t *)hdr->data;
                nethdr->flags = flags;
                /* ready to send */
                
                //make sure no packets are to be sent!!
                if (gnrc_netapi_send(dev, pkt) < 1) {
                    DEBUG("error: unable to send\n");
                    gnrc_pktbuf_release(pkt);
                    return 1;
                }   
                //gnrc_pktbuf_release(pkt);
                range_tx_off(); //turn off just in case
                DEBUG("RF and ultrasound pings sent\n");  
                break;
            default:
                // pkt received didn't match any of these.
                DEBUG("PKT was none of these.");
                break;
        }
    }
    //end of while loop, code should never reach here etc.
    return 0;
}
/*----------------------------------------------------------------------------*/









    // TODO: finalize syncing
    // syncing:
    // xtimer_ticks32_t current_time, current_lead_time, sync_offset;
    // bool sync_offset_pos;

    // current_time = xtimer_now();
    // if(xtimer_less(current_time, current_lead_time))
    // {
    //     sync_offset = xtimer_diff(current_lead_time, current_time);
    //     sync_offset_pos = true;
    // }
    // else
    // {
    //     sync_offset = xtimer_diff(current_time, current_lead_time);
    //     sync_offset_pos = false;
    // }
    // 
    // if xtimer_ticks32_t can be negative:
    // sync_offset = current_lead_time = xtimer_now












/*----------------------------------------------------------------------------*/
// xtimer_ticks32_t sync_time(xtimer_ticks32_t leader_time)
// {
//     return leader_time - xtimer_now();
// }
/*----------------------------------------------------------------------------*/

// check for leader, leader processing
    // if(LEADER_HW_ADDR == NETOPT_ADDRESS) //or some matching
    // {
    //     int current_list_size = 1;
    //     uint16_t list_of_hw_addr[NUM_OF_NODES+1];
    //     xtimer_ticks32_t cutoff_timer = 0
    //     xtimer_ticks32_t current_time = xtimer_now();
        
    //     // prepare sync packet
    //     // wait for a little bit for everyone to settle down
    //     // send SYNC
        
    //     // Broadcasting flag setup.
    //     flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
        
    //     /** Send L2 Packet **/
    //     /* network interface */
    //     dev = ifs[0];
    //     hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), RANGE_RX_HW_ADDR);
        
    //     /* put packet together */
    //     buf2[0] = (uint8_t)current_time >> 24;
    //     buf2[1] = (uint8_t)current_time >> 16;
    //     buf2[2] = (uint8_t)current_time >> 8;
    //     buf2[3] = (uint8_t)current_time;
    //     buf2[4] = FOLLOW_SYNC;

    //     pkt = gnrc_pktbuf_add(NULL, &buf2, BUFFER_SIZE_OF_PACKET_LEADER, GNRC_NETTYPE_UNDEF);
    //     if (pkt == NULL) {
    //         DEBUG("error: packet buffer full");
    //         return 1;
    //     }
       
    //     hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
    //     if (hdr == NULL) {
    //         DEBUG("error: packet buffer full");
    //         gnrc_pktbuf_release(pkt);
    //         return 1;
    //     }
    //     LL_PREPEND(pkt, hdr);
    //     nethdr = (gnrc_netif_hdr_t *)hdr->data;
    //     nethdr->flags = flags;
    //     /* ready to send */

    //     // make sure no packets are to be sent!!
    //     if (gnrc_netapi_send(dev, pkt) < 1) {
    //         DEBUG("error: unable to send");
    //         gnrc_pktbuf_release(pkt);
    //         return 1;
    //     }   

    //     // prepare time (time is equal to NUM_OF_NODES)        
    //     current_time = xtimer_now();
    //     cutoff_timer = xtimer_now() - current_time;
    //     while(current_list_size < NUM_OF_NODES && cutoff_timer < (NUM_OF_NODES * TIME_WINDOW))
    //     {
    //         // set timer for 1 time window
    //         msg_receive(&msg);
    //         if (msg.type == LEAD_INFO) // ensure that they send an INFO signal or something
    //         {
    //             // unpack and get the hw_addr
    //             pkt = msg.content.ptr;
    //             snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);

    //             // getting the hw_addr
    //             uint16_t incoming_hw_addr = 0x0000;
    //             incoming_hw_addr | ((uint16_t)((uint8_t *)snip->data)[0]) << 8;
    //             incoming_hw_addr | ((uint8_t *)snip->data)[1];

    //             // add to the list: hw_addr_nodes
    //             hw_addr_nodes[current_list_size] = incoming_hw_addr;

    //             current_list_size++; // increment current_list_size
    //             cutoff_timer = 0; // reset the timer
    //         }
    //         // TODO: figure out how to get it to stop if nothing received.
    //         if(msg.type == NULL) // no message received or something
    //         {
    //             cutoff_timer = xtimer_now() - current_time;
    //         }
    //         cutoff_timer = xtimer_now() - current_time; // increment timer
    //     }

    //     // done with getting the list
    //     // now need to send out the list
    //     for(int x = 1; x < current_list_size + 1; x++)
    //     {
    //         // prepare and send packets with x and *(x + hw_addr_nodes) and ASSIGN flags.
    //         /** Send L2 Packet **/
    //         /* network interface */
    //         dev = ifs[0];
    //         // hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), RANGE_RX_HW_ADDR);
    //         hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), hw_addr_nodes[x]);

    //         /* put packet together */
    //         buf[0] = 0x00; // unused
    //         buf[1] = (uint8_t)x;
    //         buf[2] = FOLLOW_ASSIGN;
    //         pkt = gnrc_pktbuf_add(NULL, &buf, 3, GNRC_NETTYPE_UNDEF);
    //         if (pkt == NULL) {
    //             DEBUG("error: packet buffer full");
    //             return 1;
    //         }
           
    //         hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
    //         if (hdr == NULL) {
    //             DEBUG("error: packet buffer full");
    //             gnrc_pktbuf_release(pkt);
    //             return 1;
    //         }
    //         LL_PREPEND(pkt, hdr);
    //         nethdr = (gnrc_netif_hdr_t *)hdr->data;
    //         nethdr->flags = flags;
    //         /* ready to send */

    //         // make sure no packets are to be sent!!
    //         if (gnrc_netapi_send(dev, pkt) < 1) {
    //             DEBUG("error: unable to send");
    //             gnrc_pktbuf_release(pkt);
    //             return 1;
    //         }   
    //     }
    //     while(true)
    //     {
    //         // send GO signal with timestamp
    //         // send SYNC
            
    //         // Broadcasting flag setup.
    //         flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
            
    //         /** Send L2 Packet **/
    //         /* network interface */
    //         dev = ifs[0];
    //         hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), RANGE_RX_HW_ADDR);
            
    //         /* put packet together */
    //         buf2[0] = (uint8_t)current_time >> 24;
    //         buf2[1] = (uint8_t)current_time >> 16;
    //         buf2[2] = (uint8_t)current_time >> 8;
    //         buf2[3] = (uint8_t)current_time;
    //         buf2[0] = FOLLOW_GO;

    //         pkt = gnrc_pktbuf_add(NULL, &buf2, BUFFER_SIZE_OF_PACKET_LEADER, GNRC_NETTYPE_UNDEF);
    //         if (pkt == NULL) {
    //             DEBUG("error: packet buffer full");
    //             return 1;
    //         }
           
    //         hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
    //         if (hdr == NULL) {
    //             DEBUG("error: packet buffer full");
    //             gnrc_pktbuf_release(pkt);
    //             return 1;
    //         }
    //         LL_PREPEND(pkt, hdr);
    //         nethdr = (gnrc_netif_hdr_t *)hdr->data;
    //         nethdr->flags = flags;
    //         /* ready to send */

    //         // make sure no packets are to be sent!!
    //         if (gnrc_netapi_send(dev, pkt) < 1) {
    //             DEBUG("error: unable to send");
    //             gnrc_pktbuf_release(pkt);
    //             return 1;
    //         }   
    //         // wait current_list_size * TIME_WINDOW + 1
    //         xtimer_usleep( (current_list_size + 1) * TIME_WINDOW );
    //     }
    // }
    // end leader processing
    // leader should never go beyond this

/*----------------------------------------------------------------------------*/
// void _send_message(uint8_t *buf[BUFFER_SIZE_OF_PACKET])
// {
//     /** Send L2 Packet **/
//     /* network interface */
//     dev = ifs[0];
//     hw_addr_len = gnrc_netif_addr_from_str(hw_addr, sizeof(hw_addr), RANGE_RX_HW_ADDR);
    
//     /* put packet together */
//     pkt = gnrc_pktbuf_add(NULL, buf, 3, GNRC_NETTYPE_UNDEF);
//     if (pkt == NULL) {
//         DEBUG("error: packet buffer full\n");
//         return 1;
//     }
   
//     hdr = gnrc_netif_hdr_build(NULL, 0, hw_addr, hw_addr_len);
//     if (hdr == NULL) {
//         DEBUG("error: packet buffer full\n");
//         gnrc_pktbuf_release(pkt);
//         return 1;
//     }
//     LL_PREPEND(pkt, hdr);
//     nethdr = (gnrc_netif_hdr_t *)hdr->data;
//     nethdr->flags = flags;
//     /* ready to send */
    
//     //make sure no packets are to be sent!!
//     if (gnrc_netapi_send(dev, pkt) < 1) {
//         DEBUG("error: unable to send\n");
//         gnrc_pktbuf_release(pkt);
//         return 1;
//     }   
// }
/*----------------------------------------------------------------------------*/
static range_data_t* time_diffs;


range_data_t* range_rx(uint32_t timeout_usec, uint8_t range_mode, uint16_t num_samples){ 
    // Check correct argument usage.
    uint8_t mode = range_mode;
    uint32_t maxsamps; //number of iterations in the gpio polling loop before calling it a timeout
    gpio_rx_line_t lines = (gpio_rx_line_t){RX_ONE_PIN, RX_TWO_PIN, RX_XOR_PIN};
    
    if(mode == TWO_SENSOR_MODE){
        maxsamps = MAXSAMPLES_TWO_PIN;
    } else {
        maxsamps = MAXSAMPLES_ONE_PIN;
    }

    if(gpio_init(TX_PIN, GPIO_OUT) < 0) {
        DEBUG("Error initializing GPIO_PIN.\n");
        return 1;
    }
    // clearing output for the ultrasonic sensor
    gpio_clear(TX_PIN);

    time_diffs = malloc(sizeof(range_data_t)*num_samples);
    
    uint32_t timeout = timeout_usec;
    if(timeout <= 0){
        DEBUG("timeout must be greater than 0");
        return NULL;
    }

    msg_t msg; 
    msg_t msg_queue[QUEUE_SIZE];

    /* setup the message queue */
    msg_init_queue(msg_queue, QUEUE_SIZE);

   
    int i;
    for(i = 0; i < num_samples; i++){


        range_rx_init(TX_NODE_ID, thread_getpid(), lines, maxsamps, mode);

block:
        if(xtimer_msg_receive_timeout(&msg,timeout)<0){
            DEBUG("RF ping missed\n");
            return NULL;
        }   

        if(msg.type == RF_RCVD){
            if(xtimer_msg_receive_timeout(&msg,timeout)<0){
                DEBUG("Ultrsnd ping missed\n");
                return NULL;
            }
            if(msg.type == ULTRSND_RCVD){
                time_diffs[i] = *(range_data_t*) msg.content.ptr;
            } else{
                goto block;
            }

        }
        if(time_diffs[i].tdoa > 0){
            printf("range: TDoA = %d\n", time_diffs[i].tdoa);
            switch (range_mode){
                case ONE_SENSOR_MODE:
                    break;

                case TWO_SENSOR_MODE:
                    if(time_diffs[i].error!=0){
                        printf("range: Missed pin %d\n", time_diffs[i].error);
                    } else{
                        printf("range: OD = %d\n", time_diffs[i].orient_diff);
                    }
                    break;

                case XOR_SENSOR_MODE:
                    printf("range: OD = %d\n", time_diffs[i].orient_diff);
                    break;
            }
        }
        else{
            printf("Ultrsnd ping missed\n");
        }


    }

    return time_diffs;
}
/*----------------------------------------------------------------------------*/
static range_data_t* time_diffs;
range_data_t* range_rx(uint32_t timeout_usec, uint8_t range_mode, uint16_t num_samples){ 
    // Check correct argument usage.
    uint8_t mode = range_mode;
    uint32_t maxsamps; //number of iterations in the gpio polling loop before calling it a timeout
                       //
    gpio_rx_line_t lines = (gpio_rx_line_t){RX_ONE_PIN, RX_TWO_PIN, RX_XOR_PIN};
    
    if(mode == TWO_SENSOR_MODE){
        maxsamps = MAXSAMPLES_TWO_PIN;
    } else {
        maxsamps = MAXSAMPLES_ONE_PIN;
    }

    time_diffs = malloc(sizeof(range_data_t)*num_samples);
    
    uint32_t timeout = timeout_usec;
    if(timeout <= 0){
        DEBUG("timeout must be greater than 0");
        return NULL;
    }

    msg_t msg; 
    msg_t msg_queue[QUEUE_SIZE];

    /* setup the message queue */
    msg_init_queue(msg_queue, QUEUE_SIZE);

   
    int i;
    for(i = 0; i < num_samples; i++){


        range_rx_init(TX_NODE_ID, thread_getpid(), lines, maxsamps, mode);

block:
        if(xtimer_msg_receive_timeout(&msg,timeout)<0){
            DEBUG("RF ping missed\n");
            return NULL;
        }

        if(msg.type == RF_RCVD){
            if(xtimer_msg_receive_timeout(&msg,timeout)<0){
                DEBUG("Ultrsnd ping missed\n");
                return NULL;
            }
            if(msg.type == ULTRSND_RCVD){
                time_diffs[i] = *(range_data_t*) msg.content.ptr;
            } else{
                goto block;
            }

        }
        printf("range: tdoa = %d\n", time_diffs[i].tdoa);
        switch (range_mode){
            case ONE_SENSOR_MODE:
                break;

            case TWO_SENSOR_MODE:
                if(time_diffs[i].error!=0){
                    printf("range: Missed pin %d\n", time_diffs[i].error);
                } else{
                    printf("range: odelay = %d\n", time_diffs[i].odelay);
                }
                break;

            case XOR_SENSOR_MODE:
                printf("range: odelay = %d\n", time_diffs[i].odelay);
                break;
        }
        if(i == num_samples-1){
            time_diffs[i].error += 10;
        }


    }

    return time_diffs;
}
