/**
 * Copyright (c) 2016, Autonomous Networks Research Group. All rights reserved.
 * Developed by:
 * Autonomous Networks Research Group (ANRG)
 * University of Southern California
 * http://anrg.usc.edu/
 *
 * Contributors:
 * Jason A. Tran
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
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
 * @file
 * @brief       Application to act as a radio for the m3pi-mbed-os code.
 *
 * @author      Jason A. Tran <jasontra@usc.edu>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "board.h"
#include "thread.h"
#include "mutex.h"
#include "msg.h"
#include "random.h"
#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "periph/uart.h"
#include "hdlc.h"
#include "dispatcher.h"
#include "uart_pkt.h"
#include "main-conf.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef UART_STDIO_DEV
#define UART_STDIO_DEV          (UART_UNDEF)
#endif

#define HDLC_PRIO               (THREAD_PRIORITY_MAIN - 1)
#define DISPATCHER_PRIO         (THREAD_PRIORITY_MAIN - 1)

static msg_t main_msg_queue[8];
static msg_t rssi_dump_msg_queue[16];

static char hdlc_stack[THREAD_STACKSIZE_MAIN];
static char dispatcher_stack[512];
static char rssi_dump_stack[THREAD_STACKSIZE_MAIN];

/* TODO: need to program leader-follower IP discovery */

static void *_rssi_dump(void *arg) 
{
    kernel_pid_t hdlc_thread_pid = (kernel_pid_t)arg;
    kernel_pid_t *sender_pid;
    msg_t *msg;

    msg_init_queue(rssi_dump_msg_queue, 16);

    dispatcher_entry_t rssi_dump_thr = { .dispatcher_entry = NULL, 
        .port = RSSI_DUMP_PORT, .pid = thread_getpid() };
    dispatcher_register(&rssi_dump_thr);

    /* set short hwaddr */
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    gnrc_netif_get(ifs);
    uint8_t target_hwaddr_short[2];
    uint8_t target_hwaddr_short_len;
    char target_hwaddr_str[6] = ARREST_LEADER_SHORT_HWADDR;
    target_hwaddr_short_len = gnrc_netif_addr_from_str(target_hwaddr_short,
                                sizeof(target_hwaddr_short), target_hwaddr_str);
    gnrc_netapi_set(ifs[0], NETOPT_ADDRESS, 0, target_hwaddr_short, target_hwaddr_short_len);

    gnrc_netreg_entry_t rssi_dump_server = {NULL, RSSI_DUMP_PORT, thread_getpid()};

    char send_data[UART_PKT_HDR_LEN + 1];

    mutex_t send_pkt_mtx;
    hdlc_pkt_t send_pkt = { .data = send_data, .length = UART_PKT_HDR_LEN + 1 };
    uart_pkt_hdr_t uart_hdr = {
        .src_port = RSSI_DUMP_PORT,
        .dst_port = 0,               /* no dst_port needed for mbed's dispatcher */
        .pkt_type = RSSI_DATA_PKT
    };
    /* always the same header */
    uart_pkt_insert_hdr(send_pkt.data, UART_PKT_HDR_LEN + 1, &uart_hdr); 

    while(1)
    {
        msg_receive(&msg);

        switch (msg.type)
        {
            case HDLC_PKT_RDY:
                hdlc_pkt_t *recv_pkt;
                uart_pkt_hdr_t uart_hdr;
                uart_pkt_parse_hdr(&uart_hdr, recv_pkt->data, recv_pkt->length);
                switch (uart_hdr.pkt_type) 
                {
                    case RSSI_DUMP_START:
                        gnrc_netreg_register(1, &rssi_dump_server);
                        break;
                    case RSSI_DUMP_STOP:
                        gnrc_netreg_unregister(1, &rssi_dump_server);
                        break;
                    default:
                        DEBUG("rssi_dump: invalid packet!\n");
                        break;
                }
                break;
            case HDLC_RESP_RETRY_W_TIMEO:
                /* don't bother retrying */
                mutex_unlock(&send_pkt_mtx);
                break;
            case HDLC_RESP_SND_SUCC:
                mutex_unlock(&send_pkt_mtx);
                break;
            case GNRC_NETAPI_MSG_TYPE_RCV:
                /* if you are receiving messages, you are registered to the 
                specific port so you need to dump RSSI via hdlc */
                gnrc_netif_hdr_t *netif_hdr = ((gnrc_pktsnip_t *)msg.content.ptr)->data;
                uint8_t *dst_addr = gnrc_netif_hdr_get_dst_addr(netif_hdr);
                /* need to subtract 73 from raw RSSI (do on mbed side) to get dBm value */
                uint8_t raw_rssi = netif_hdr->rssi;

                if (netif_hdr->dst_l2addr_len == 2 && !memcmp(dst_addr, target_hwaddr_short, 2)) {
                    if (mutex_trylock(&send_pkt_mtx)) {
                        send_pkt.length = uart_pkt_cpy_data(send_pkt.data, 
                            UART_PKT_HDR_LEN + 1, &raw_rssi, 1); 

                        msg.type = HDLC_MSG_SND;
                        msg.content.ptr = &send_pkt;
                        msg_send(&msg, hdlc_pid);
                    } /* else { do nothing, skip this rssi reading } */
                }

                gnrc_pktbuf_release((gnrc_pktsnip_t *)msg.content.ptr);
            default:
                /* error */
                DEBUG("Dispatcher: invalid packet\n");
                break;
        }    
    }

    DEBUG("Error: Reached Exit!");
    /* should be never reached */

}

static int _handle_pkt(hdlc_pkt_t *send_pkt, uint8_t *data, size_t length) 
{
    uart_pkt_hdr_t rcv_hdr;
    uart_pkt_parse_hdr(&rcv_hdr, data, length);

    switch (rcv_hdr.pkt_type)
    {
        /* TODO: free any radio packets received */
        case RADIO_SET_CHAN:
            uart_pkt_hdr_t hdr;
            hdr.dst_port = rcv_hdr.src_port; 
            hdr.src_port = rcv_hdr.dst_port; 
            uart_pkt_insert_hdr(send_pkt->data, HDLC_MAX_PKT_SIZE, &hdr);
            uint16_t channel = data[UART_PKT_DATA_FIELD];
            kernel_pid_t ifs[GNRC_NETIF_NUMOF];
            gnrc_netif_get(ifs);

            if(gnrc_netapi_set(ifs[0], NETOPT_CHANNEL, 0, &channel, sizeof(uint16_t)) < 0) {
                send_pkt->data[UART_PKT_TYPE_FIELD] = RADIO_SET_CHAN_FAIL;
            } else {
                send_pkt->data[UART_PKT_TYPE_FIELD] = RADIO_SET_CHAN_SUCCESS;
            }

            send_pkt->data[UART_PKT_DATA_FIELD] = (uint8_t)channel;
            send_pkt->length = UART_PKT_HDR_LEN + 1;
            return 1; /* need to send packet */
        case RADIO_SET_POWER:
            /* TODO */
            break;
        case SOUND_RANGE_REQ:
            ultrasound_range_recv();
            /* initialize sound ranging */
            /* do sound ranging */
            /* once done, send packet containing TDoA to mbed */
            break;
        case SOUND_RANGE_X10_REQ:
            /* TODO */
            break;
        default:
            DEBUG("Unknown packet type.\n");
            return 1;
    }
    return 0
}

int main(void)
{
    kernel_pid_t hdlc_pid = hdlc_init(hdlc_stack, sizeof(hdlc_stack), HDLC_PRIO, 
        "hdlc", UART_DEV(1));
    kernel_pid_t dispatcher_pid = dispacher_init(dispatcher_stack, 
        sizeof(dispatcher_stack), DISPATCHER_PRIO, "dispatcher", hdlc_pid);
    kernel_pid_t rssi_dump_pid = thread_create(rssi_dump_stack, 
        sizeof(rssi_dump_stack), RSSI_DUMP_PRIO, "rssi_dump", hdlc_pid);

    msg_init_queue(main_msg_queue, 8);

    uint8_t

    msg_t msg, msg_snd_pkt;
    char send_data[HDLC_MAX_PKT_SIZE];
    hdlc_pkt_t hdlc_pkt = { .data = send_data, .length = HDLC_MAX_PKT_SIZE };

    dispatcher_entry_t main_thr = { .next = NULL, .port = 8080, 
        .pid = thread_getpid() };
    dispatcher_register(&main_thr);

    send_hdlc_lock = 0;

    /* this thread handles set tx power, set channel, and ranging requests */
    while(1)
    {

        while(!send_hdlc_lock)
        {
            msg_receive(&msg);

            switch (msg.type)
            {
                case HDLC_RESP_SND_SUCC:
                    send_hdlc_lock = 1;
                    break;
                case HDLC_RESP_RETRY_W_TIMEO:
                    xtimer_usleep(msg.content.value);
                    msg_send(&msg_snd_pkt, hdlc_pid);
                    break;
                case HDLC_PKT_RDY:
                    recv_pkt = (hdlc_pkt_t *)msg_resp.content.ptr;
                    _handle_pkt = (&send_pkt, recv_pkt->data, recv_pkt->length);
                    /* insert flag to send a packet */
                    hdlc_pkt_release(recv_pkt);
                    break;
                case GNRC_NETAPI_MSG_TYPE_RCV:
                    gnrc_pktsnip_t *gnrc_pkt = msg.content.ptr;
                    gnrc_pktbuf_release(gnrc_pkt);
                    break;

                default:
                    /* error */
                    LED3_ON;
                    break;
            }
        }

        /* fill pkt buffer */

        /* send needed packet */
        msg_snd_pkt.type = HDLC_MSG_SND;
        msg_send_pkt.content.ptr = &hdlc_pkt;
        msg_send(&msg_snd_pkt, hdlc_pid);


    }

    /* should be never reached */
    return 0;
}


