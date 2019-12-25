/*
 * Copyright (c) 2015 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <vnet/vnet.h>
#include <vppinfra/vec.h>
#include <vppinfra/error.h>
#include <vppinfra/format.h>
#include <vppinfra/xxhash.h>

#include <vnet/ethernet/ethernet.h>
#include <dpdk/device/dpdk.h>
#include <vnet/classify/vnet_classify.h>
#include <vnet/mpls/packet.h>
#include <vnet/handoff.h>
#include <vnet/devices/devices.h>
#include <vnet/feature/feature.h>

#include <dpdk/device/dpdk_priv.h>


#include "pkt_wdata.h"

#include "sf_feature.h"


#include "sf_interface_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"

static char *dpdk_error_strings[] = {
#define _(n, s) s,
    foreach_dpdk_error
#undef _
};

STATIC_ASSERT(VNET_DEVICE_INPUT_NEXT_IP4_INPUT - 1 ==
                  VNET_DEVICE_INPUT_NEXT_IP4_NCS_INPUT,
              "IP4_INPUT must follow IP4_NCS_INPUT");

enum
{
    DPDK_RX_F_CKSUM_GOOD = 7,
    DPDK_RX_F_CKSUM_BAD = 4,
    DPDK_RX_F_FDIR = 2,
};

/* currently we are just copying bit positions from DPDK, but that
   might change in future, in case we start to be interested in something
   stored in upper bytes. Currently we store only lower byte for perf reasons */
STATIC_ASSERT(1 << DPDK_RX_F_CKSUM_GOOD == PKT_RX_IP_CKSUM_GOOD, "");
STATIC_ASSERT(1 << DPDK_RX_F_CKSUM_BAD == PKT_RX_IP_CKSUM_BAD, "");
STATIC_ASSERT(1 << DPDK_RX_F_FDIR == PKT_RX_FDIR, "");
STATIC_ASSERT((PKT_RX_IP_CKSUM_GOOD | PKT_RX_IP_CKSUM_BAD | PKT_RX_FDIR) <
                  256,
              "dpdk flags not un lower byte, fix needed");


static_always_inline uword
sf_dpdk_process_subseq_segs(vlib_main_t *vm, vlib_buffer_t *b,
                         struct rte_mbuf *mb , uint16_t *next , uint16_t hugepkt_next_index)
{
    u8 nb_seg = 1;
    struct rte_mbuf *mb_seg = 0;
    vlib_buffer_t *b_seg = 0;
    mb_seg = mb->next;

    sf_wdata_t *wdata0 = sf_wdata(b);
    sf_hugepkt_helper_t *hugepkt_helper = sf_hugepkt_helper(wdata0);
    sf_hugepkt_sub_t *pre_sub_buffer = hugepkt_helper->sub_buffer;

    if (mb->nb_segs < 2)
        return 0;

    *next = hugepkt_next_index;
    sf_debug("nb_segs : %d\n" , mb->nb_segs );

    wdata0->unused8 |= UNUSED8_FLAG_HUGE_PKT;
    
    hugepkt_helper->sub_total_len_no_first = 0;
    hugepkt_helper->sub_buffer_cnt = 1;

    pre_sub_buffer->buffer_index = vlib_get_buffer_index(vm, b);
    pre_sub_buffer->current_data = wdata0->current_data;
    pre_sub_buffer->current_length = wdata0->current_length;
    pre_sub_buffer++;

    while (nb_seg < mb->nb_segs)
    {
        ASSERT(mb_seg != 0);

        b_seg = vlib_buffer_from_rte_mbuf(mb_seg);
        /*
       * The driver (e.g. virtio) may not put the packet data at the start
       * of the segment, so don't assume b_seg->current_data == 0 is correct.
       */
        pre_sub_buffer->current_data = 
            (mb_seg->buf_addr + mb_seg->data_off) - (void *)b_seg->data;

        pre_sub_buffer->current_length =  mb_seg->data_len;

        hugepkt_helper->sub_total_len_no_first += mb_seg->data_len;
        hugepkt_helper->sub_buffer_cnt ++;

        pre_sub_buffer->buffer_index = vlib_get_buffer_index(vm, b_seg);

        sf_debug("middle : data %d  len %d\n" ,
             pre_sub_buffer->current_data , pre_sub_buffer->current_length);

        pre_sub_buffer++;
        mb_seg = mb_seg->next;
        nb_seg++;
    }

    sf_debug("first : %d len_no_first : %d sub_buffer_cnt : %d\n" , wdata0->current_length , 
        hugepkt_helper->sub_total_len_no_first , hugepkt_helper->sub_buffer_cnt);

    return hugepkt_helper->sub_total_len_no_first ;
}

static_always_inline void
dpdk_prefetch_mbuf_x4(struct rte_mbuf *mb[])
{
    CLIB_PREFETCH(mb[0], CLIB_CACHE_LINE_BYTES, LOAD);
    CLIB_PREFETCH(mb[1], CLIB_CACHE_LINE_BYTES, LOAD);
    CLIB_PREFETCH(mb[2], CLIB_CACHE_LINE_BYTES, LOAD);
    CLIB_PREFETCH(mb[3], CLIB_CACHE_LINE_BYTES, LOAD);
}



static_always_inline void
sf_dpdk_prefetch_wdata_x4(struct rte_mbuf *mb[])
{
    vlib_buffer_t *b;
    b = vlib_buffer_from_rte_mbuf(mb[0]);
    CLIB_PREFETCH(sf_wdata(b), CLIB_CACHE_LINE_BYTES , STORE);
    b = vlib_buffer_from_rte_mbuf(mb[1]);
    CLIB_PREFETCH(sf_wdata(b), CLIB_CACHE_LINE_BYTES , STORE);
    b = vlib_buffer_from_rte_mbuf(mb[2]);
    CLIB_PREFETCH(sf_wdata(b), CLIB_CACHE_LINE_BYTES , STORE);
    b = vlib_buffer_from_rte_mbuf(mb[3]);
    CLIB_PREFETCH(sf_wdata(b), CLIB_CACHE_LINE_BYTES , STORE);
}

static_always_inline void
sf_dpdk_prefetch_ex_data_x4(struct rte_mbuf *mb[])
{
    vlib_buffer_t *b;
    b = vlib_buffer_from_rte_mbuf(mb[0]);
    CLIB_PREFETCH(sf_wdata(b)->recv_for_opaque, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(sf_wdata(b)->recv_pre_data, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);

    b = vlib_buffer_from_rte_mbuf(mb[1]);
    CLIB_PREFETCH(sf_wdata(b)->recv_for_opaque, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(sf_wdata(b)->recv_pre_data, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);

    b = vlib_buffer_from_rte_mbuf(mb[2]);
    CLIB_PREFETCH(sf_wdata(b)->recv_for_opaque, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(sf_wdata(b)->recv_pre_data, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);


    b = vlib_buffer_from_rte_mbuf(mb[3]);
    CLIB_PREFETCH(sf_wdata(b)->recv_for_opaque, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(sf_wdata(b)->recv_pre_data, CLIB_CACHE_LINE_BYTES , STORE);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);
}

static_always_inline void
dpdk_prefetch_buffer_data_x4(struct rte_mbuf *mb[])
{
    vlib_buffer_t *b;
    b = vlib_buffer_from_rte_mbuf(mb[0]);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);
    b = vlib_buffer_from_rte_mbuf(mb[1]);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);
    b = vlib_buffer_from_rte_mbuf(mb[2]);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);
    b = vlib_buffer_from_rte_mbuf(mb[3]);
    CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);
}

static_always_inline uword
dpdk_prefetch_ex_data_all(vlib_main_t *vm, dpdk_per_thread_data_t *ptd, uword n_rx_packets)
{
    struct rte_mbuf **mb = ptd->mbufs;
    vlib_buffer_t *b;
    while(n_rx_packets > 0)
    {
        b = vlib_buffer_from_rte_mbuf(mb[0]);
        CLIB_PREFETCH(sf_wdata(b)->recv_for_opaque, CLIB_CACHE_LINE_BYTES , STORE);
        CLIB_PREFETCH(sf_wdata(b)->recv_pre_data, CLIB_CACHE_LINE_BYTES , STORE);
        CLIB_PREFETCH(b->data, CLIB_CACHE_LINE_BYTES, LOAD);

        mb++;
        n_rx_packets --;
    }

    return 0;
}

/** \brief Main DPDK input node
    @node dpdk-input

    This is the main DPDK input node: across each assigned interface,
    call rte_eth_rx_burst(...) or similar to obtain a vector of
    packets to process. Derive @c vlib_buffer_t metadata from
    <code>struct rte_mbuf</code> metadata,
    Depending on the resulting metadata: adjust <code>b->current_data,
    b->current_length </code> and dispatch directly to
    ip4-input-no-checksum, or ip6-input. Trace the packet if required.

    @param vm   vlib_main_t corresponding to the current thread
    @param node vlib_node_runtime_t
    @param f    vlib_frame_t input-node, not used.

    @par Graph mechanics: buffer metadata, next index usage

    @em Uses:
    - <code>struct rte_mbuf mb->ol_flags</code>
        - PKT_RX_IP_CKSUM_BAD

    @em Sets:
    - <code>b->error</code> if the packet is to be dropped immediately
    - <code>b->current_data, b->current_length</code>
        - adjusted as needed to skip the L2 header in  direct-dispatch cases
    - <code>vnet_buffer(b)->sw_if_index[VLIB_RX]</code>
        - rx interface sw_if_index
    - <code>vnet_buffer(b)->sw_if_index[VLIB_TX] = ~0</code>
        - required by ipX-lookup
    - <code>b->flags</code>
        - to indicate multi-segment pkts (VLIB_BUFFER_NEXT_PRESENT), etc.

    <em>Next Nodes:</em>
    - Static arcs to: error-drop, ethernet-input,
      ip4-input-no-checksum, ip6-input, mpls-input
    - per-interface redirection, controlled by
      <code>xd->per_interface_next_index</code>
*/



typedef enum
{
    SF_NEXT_NODE_SF_ETHERNET_INUT,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(dpdk_input_node) = {
    .type = VLIB_NODE_TYPE_INPUT,
    .name = "sf-dpdk-input",
    .sibling_of = "device-input",

    /* Will be enabled if/when hardware is detected. */
    .state = VLIB_NODE_STATE_DISABLED,

    .format_buffer = format_ethernet_header_with_length,
    .format_trace = format_dpdk_rx_trace,

    .n_errors = DPDK_N_ERROR,
    .error_strings = dpdk_error_strings,

};
/* *INDENT-ON* */

static_always_inline u8
dpdk_ol_flags_extract(struct rte_mbuf **mb, u8 *flags, int count)
{
    u8 rv = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        /* all flags we are interested in are in lower 8 bits but
         that might change */
        flags[i] = (u8)mb[i]->ol_flags;
        rv |= flags[i];
    }
    return rv;
}

extern uint16_t hugepkt_next_index;
extern uint16_t eth_input_next_index;
static_always_inline uword
dpdk_process_rx_burst(vlib_main_t *vm, int sw_if_index , dpdk_per_thread_data_t *ptd,
                      uword n_rx_packets, int maybe_multiseg, u8 *or_flagsp)
{
    u32 n_left = n_rx_packets;
    vlib_buffer_t *b[4];
    sf_wdata_t *pkt_wdata[4];
    struct rte_mbuf **mb = ptd->mbufs;
    uword n_bytes = 0;
    i16 off;
    u8 *flags, or_flags = 0;
    u16 *next;

    uint8_t global_interface_id = sf_sw_if_to_interface(sw_if_index);
    uint16_t hugepkt_next = hugepkt_next_index; 
    uint16_t global_next_index = eth_input_next_index;


    if(sf_intf_get_handoff_status(global_interface_id))
    {
        hugepkt_next = sf_intf_handoff_main.dpdk_next_index_for_handoff;
        global_next_index = sf_intf_handoff_main.dpdk_next_index_for_handoff;
    }


    uint64_t global_cpu_time_now = clib_cpu_time_now();


    mb = ptd->mbufs;
    flags = ptd->flags;
    next = ptd->next;

    while (n_left >= 8)
    {
        CLIB_PREFETCH(mb + 8, CLIB_CACHE_LINE_BYTES, LOAD);

        sf_dpdk_prefetch_wdata_x4(mb + 4);
        
        // sf_dpdk_prefetch_ex_data_x4(mb + 4);

        b[0] = vlib_buffer_from_rte_mbuf(mb[0]);
        b[1] = vlib_buffer_from_rte_mbuf(mb[1]);
        b[2] = vlib_buffer_from_rte_mbuf(mb[2]);
        b[3] = vlib_buffer_from_rte_mbuf(mb[3]);

        pkt_wdata[0] = sf_wdata(b[0]);
        pkt_wdata[1] = sf_wdata(b[1]);
        pkt_wdata[2] = sf_wdata(b[2]);
        pkt_wdata[3] = sf_wdata(b[3]);

        reset_pkt_wdata(pkt_wdata[0]);
        reset_pkt_wdata(pkt_wdata[1]);
        reset_pkt_wdata(pkt_wdata[2]);
        reset_pkt_wdata(pkt_wdata[3]);
    
        dpdk_prefetch_mbuf_x4(mb + 4);

        or_flags |= dpdk_ol_flags_extract(mb, flags, 4);
        flags += 4;

        pkt_wdata[0]->unused8 = UNUSED8_FLAG_PKT_RECORD_DROP;
        pkt_wdata[1]->unused8 = UNUSED8_FLAG_PKT_RECORD_DROP;
        pkt_wdata[2]->unused8 = UNUSED8_FLAG_PKT_RECORD_DROP;
        pkt_wdata[3]->unused8 = UNUSED8_FLAG_PKT_RECORD_DROP;

        off = mb[0]->data_off;
        off -= RTE_PKTMBUF_HEADROOM;
        pkt_wdata[0]->l2_hdr_offset = off;
        pkt_wdata[0]->current_data = off;

        off = mb[1]->data_off;
        off -= RTE_PKTMBUF_HEADROOM;
        pkt_wdata[1]->l2_hdr_offset = off;
        pkt_wdata[1]->current_data = off;

        off = mb[2]->data_off;
        off -= RTE_PKTMBUF_HEADROOM;
        pkt_wdata[2]->l2_hdr_offset = off;
        pkt_wdata[2]->current_data = off;

        off = mb[3]->data_off;
        off -= RTE_PKTMBUF_HEADROOM;
        pkt_wdata[3]->l2_hdr_offset = off;
        pkt_wdata[3]->current_data = off;

        pkt_wdata[0]->current_length = mb[0]->data_len;
        pkt_wdata[1]->current_length = mb[1]->data_len;
        pkt_wdata[2]->current_length = mb[2]->data_len;
        pkt_wdata[3]->current_length = mb[3]->data_len;

        pkt_wdata[0]->interface_id = global_interface_id;
        pkt_wdata[1]->interface_id = global_interface_id;
        pkt_wdata[2]->interface_id = global_interface_id;
        pkt_wdata[3]->interface_id = global_interface_id;

        pkt_wdata[0]->timestamp = global_cpu_time_now;
        pkt_wdata[1]->timestamp = global_cpu_time_now;
        pkt_wdata[2]->timestamp = global_cpu_time_now;
        pkt_wdata[3]->timestamp = global_cpu_time_now;

        pkt_wdata[0]->data_ptr = b[0]->data;
        pkt_wdata[1]->data_ptr = b[1]->data;
        pkt_wdata[2]->data_ptr = b[2]->data;
        pkt_wdata[3]->data_ptr = b[3]->data;

        n_bytes += mb[0]->data_len;
        n_bytes += mb[1]->data_len;
        n_bytes += mb[2]->data_len;
        n_bytes += mb[3]->data_len;

        next[0] = global_next_index;
        next[1] = global_next_index;
        next[2] = global_next_index;
        next[3] = global_next_index;

        if (maybe_multiseg)
        {
            n_bytes += sf_dpdk_process_subseq_segs(vm, b[0], mb[0] , next     , hugepkt_next);
            n_bytes += sf_dpdk_process_subseq_segs(vm, b[1], mb[1] , next + 1 , hugepkt_next);
            n_bytes += sf_dpdk_process_subseq_segs(vm, b[2], mb[2] , next + 2 , hugepkt_next);
            n_bytes += sf_dpdk_process_subseq_segs(vm, b[3], mb[3] , next + 3 , hugepkt_next);
        }

        /* next */
        mb += 4;
        n_left -= 4;
        next += 4;
    }

    while (n_left)
    {
        b[0] = vlib_buffer_from_rte_mbuf(mb[0]);
        pkt_wdata[0] = sf_wdata(b[0]);

        memset( pkt_wdata[0] , 0 , 256);

        or_flags |= dpdk_ol_flags_extract(mb, flags, 1);
        flags += 1;
        
        pkt_wdata[0]->unused8 = UNUSED8_FLAG_PKT_RECORD_DROP;

        off = mb[0]->data_off;
        off -= RTE_PKTMBUF_HEADROOM;
        pkt_wdata[0]->l2_hdr_offset = off;
        pkt_wdata[0]->current_data = off;

        pkt_wdata[0]->current_length = mb[0]->data_len;
        pkt_wdata[0]->interface_id = global_interface_id;
        pkt_wdata[0]->timestamp = global_cpu_time_now;
        pkt_wdata[0]->data_ptr = b[0]->data;

        n_bytes += mb[0]->data_len;

        next[0] = global_next_index;

        if (maybe_multiseg)
            n_bytes += sf_dpdk_process_subseq_segs(vm, b[0], mb[0] , next , hugepkt_next);

        /* next */
        mb += 1;
        n_left -= 1;
        next += 1;
    }

    *or_flagsp = or_flags;
    return n_bytes ;
}

static_always_inline u32
dpdk_device_input(vlib_main_t *vm, dpdk_main_t *dm, dpdk_device_t *xd,
                  vlib_node_runtime_t *node, u32 thread_index, u16 queue_id)
{
    uword n_rx_packets = 0, n_rx_bytes;
    u32 n_trace;
    u8 or_flags;
    u32 n;
    volatile  uint32_t interface_id = sf_sw_if_to_interface(xd->sw_if_index);

    dpdk_per_thread_data_t *ptd = vec_elt_at_index(dm->per_thread_data,
                                                   thread_index);

    //for vpp
    if ((xd->flags & DPDK_DEVICE_FLAG_ADMIN_UP) == 0)
        return 0;

    //for sf
    if(interface_id > 0 && get_interface_admin_status(interface_id) == 0)
    {
        return 0;
    }

    /* get up to DPDK_RX_BURST_SZ buffers from PMD */
    while (n_rx_packets < DPDK_RX_BURST_SZ)
    {
        n = rte_eth_rx_burst(xd->port_id, queue_id,
                             ptd->mbufs + n_rx_packets,
                             DPDK_RX_BURST_SZ - n_rx_packets);
        n_rx_packets += n;

        if (n < 32)
            break;
    }

    if (n_rx_packets == 0)
        return 0;

    /* Update buffer template */
    // vnet_buffer(bt)->sw_if_index[VLIB_RX] = xd->sw_if_index;
    // bt->error = node->errors[DPDK_ERROR_NONE];
    // /* as DPDK is allocating empty buffers from mempool provided before interface
    //  start for each queue, it is safe to store this in the template */
    // bt->buffer_pool_index = xd->buffer_pool_for_queue[queue_id];


    // for ( n = 0; n < n_rx_packets; n++ )
    //     ptd->next[n] = eth_next_index;

    if (xd->flags & DPDK_DEVICE_FLAG_MAYBE_MULTISEG)
        n_rx_bytes = dpdk_process_rx_burst(vm, xd->sw_if_index , ptd, n_rx_packets, 1, &or_flags);
    else
        n_rx_bytes = dpdk_process_rx_burst(vm, xd->sw_if_index , ptd, n_rx_packets, 0, &or_flags);


    //prefetch rest wdata
    dpdk_prefetch_ex_data_all(vm , ptd , n_rx_packets);

    /* flow offload - process if rx flow offload enabled and at least one packet
     is marked */
    if (PREDICT_FALSE((xd->flags & DPDK_DEVICE_FLAG_RX_FLOW_OFFLOAD) &&
                      (or_flags & (1 << DPDK_RX_F_FDIR))))
    {
    }

    /* is at least one packet marked as ip4 checksum bad? */
    if (PREDICT_FALSE(or_flags & (1 << DPDK_RX_F_CKSUM_BAD)))
    {
    }

    /* enqueue buffers to the next node */
    vlib_get_buffer_indices_with_offset(vm, (void **)ptd->mbufs, ptd->buffers,
                                        n_rx_packets,
                                        sizeof(struct rte_mbuf));


    vlib_buffer_enqueue_to_next(vm, node, ptd->buffers, ptd->next,
                                n_rx_packets);

    /* packet trace if enabled */
    if (PREDICT_FALSE((n_trace = vlib_get_trace_count(vm, node))))
    {
    }

    /* rx pcap capture if enabled */
    if (PREDICT_FALSE(dm->pcap[VLIB_RX].pcap_enable))
    {
    }



    //for sf
    if(interface_id != 0)
    {
        update_interface_stat_multi(thread_index , interface_id , in_packets , n_rx_packets);
        update_interface_stat_multi(thread_index , interface_id , in_octets , n_rx_bytes);
    }

    //for vpp
    vlib_increment_combined_counter(vnet_get_main()->interface_main.combined_sw_if_counters + VNET_INTERFACE_COUNTER_RX, thread_index, xd->sw_if_index,
                                    n_rx_packets, n_rx_bytes);

    vnet_device_increment_rx_packets(thread_index, n_rx_packets);



    return n_rx_packets;
}

VLIB_NODE_FN(dpdk_input_node)
(vlib_main_t *vm, vlib_node_runtime_t *node,
 vlib_frame_t *f)
{
    dpdk_main_t *dm = &dpdk_main;
    dpdk_device_t *xd;
    uword n_rx_packets = 0;
    vnet_device_input_runtime_t *rt = (void *)node->runtime_data;
    vnet_device_and_queue_t *dq;
    u32 thread_index = node->thread_index;

    if(interfaces_stat->dpdk_init_status == 0)
    {
        return 0;
    }

    /*
   * Poll all devices on this cpu for input/interrupts.
   */
    /* *INDENT-OFF* */
    foreach_device_and_queue(dq, rt->devices_and_queues)
    {
        xd = vec_elt_at_index(dm->devices, dq->dev_instance);
        if (PREDICT_FALSE(xd->flags & DPDK_DEVICE_FLAG_BOND_SLAVE))
            continue; /* Do not poll slave to a bonded interface */
        n_rx_packets += dpdk_device_input(vm, dm, xd, node, thread_index,
                                          dq->queue_id);
    }
    /* *INDENT-ON* */
    return n_rx_packets;
}


#ifndef CLIB_MARCH_VARIANT

uint16_t hugepkt_next_index;
uint16_t eth_input_next_index;
static clib_error_t *sf_dpdk_next_node_init(vlib_main_t *vm)
{
    vlib_node_t *dpdk_input;
    vlib_node_t *hugepkt_input; 
    vlib_node_t *eth_input; 
    dpdk_input = vlib_get_node_by_name (vm, (u8 *) "sf-dpdk-input");
    hugepkt_input = vlib_get_node_by_name(vm , (u8 *)"sf_hugepkt_input");
    eth_input = vlib_get_node_by_name(vm , (u8 *)"sf_eth_input");

    hugepkt_next_index =
        vlib_node_add_next (vm, dpdk_input->index, hugepkt_input->index );

    eth_input_next_index = 
        vlib_node_add_next (vm, dpdk_input->index, eth_input->index );
    return 0;
}

VLIB_INIT_FUNCTION(sf_dpdk_next_node_init);


#endif