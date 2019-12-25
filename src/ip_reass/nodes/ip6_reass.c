/*
 *------------------------------------------------------------------
 * Copyright (c) 2019 Asterfusion.
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
 *------------------------------------------------------------------
 */
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>

#include <stdio.h>

#include "sf_main.h"
#include "common_header.h"
#include "pkt_wdata.h"


#include "sf_huge_pkt_tools.h"
#include "sf_thread_tools.h"

#include "sf_ip_reass.h"
#include "ip_reass_timer.h"
#include "sf_ip_reass_shm.h"

#include "ip_reass_stat_tools.h"

#include "ip_reass_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"





typedef enum
{
    SF_NEXT_NODE_SF_ACL_ENTRY,
    SF_NEXT_NODE_UDP_INPUT , 
    SF_NEXT_NODE_TCP_INPUT,        
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

VLIB_REGISTER_NODE(sf_ip6_reass_node) = {
    .name = "sf_ip6_reass",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",
        [SF_NEXT_NODE_UDP_INPUT] = "sf_udp_input",
        [SF_NEXT_NODE_TCP_INPUT] = "sf_tcp_input",        
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};


/***
 * @return : 0 means didn't find , so keep next_index(0) as a default next node
 */
always_inline uint32_t sf_ip_reass_get_next_index(uint8_t  ip_protocol )
{
    if(ip_protocol == IP_PRO_TCP)
    {
        return SF_NEXT_NODE_TCP_INPUT;
    }
    else if(ip_protocol == IP_PRO_UDP)
    {
        return SF_NEXT_NODE_UDP_INPUT;
    }
    return SF_NEXT_NODE_SF_ACL_ENTRY;
}


static_always_inline void ip6_frag_build_pkt(vlib_main_t *vm , vlib_buffer_t **ret_buffer , int32_t *ret_status)
{
    uint16_t frag_offset;
    uint16_t frag_size;
    uint16_t ip_hdr_len;
    uint16_t ip_hdr_len_tmp;
    ipv6_frag_t *ip6_frag;
    // ipv6_header_t *ip6_hdr;

    sf_pool_per_thread_t *huge_16k_pool = get_huge_pkt_16k_pool(sf_get_current_worker_index());

    uint8_t *data_ptr = alloc_huge_pkt_16k(huge_16k_pool);
    if(PREDICT_FALSE(data_ptr == NULL))
    {
        return ;
    }
    // sf_debug("data_ptr : %p\n" , data_ptr);

    vlib_buffer_t *ip_reass_b0 = alloc_one_pkt_from_vpp(vm); 
    if( PREDICT_FALSE(ip_reass_b0 == NULL) )
    {
        free_huge_pkt_16k(huge_16k_pool , data_ptr);

        return ;
    }

    sf_wdata_t *new_wdata = sf_wdata(ip_reass_b0);
    vlib_buffer_t *tmp = *ret_buffer;

    // copy sf_wdata
    memcpy( sf_wdata(ip_reass_b0) , sf_wdata(tmp)  , SF_WDATA_MAIN_LEN);
    sf_wdata(ip_reass_b0)->unused8 |= UNUSED8_FLAG_IP_REASS_PKT;
    sf_wdata(ip_reass_b0)->unused8 |= UNUSED8_PKT_HUGE_PKT_WITH_WDATA;
    

    // set data ptr 
    sf_wdata(ip_reass_b0)->data_ptr = data_ptr;


    //copy data before ip
    // sf_debug("from %p to %p , len %d\n" , sf_wdata(tmp)->data_ptr , data_ptr , sf_wqe_get_current_offset(tmp));
    // sf_debug("eth offset : %d\n" , sf_wdata(tmp)->l2_hdr_offset);
    memcpy(data_ptr , sf_wdata(tmp)->data_ptr , sf_wqe_get_current_offset(tmp) );

    //copy ip6 head
    ip_hdr_len = ip_reass_record(tmp)->ip_hdr_len;
    // sf_debug("from %p to %p , len %d\n" , sf_wqe_get_current(tmp) , sf_wqe_get_current(ip_reass_b0) , ip_hdr_len);
    memcpy(sf_wqe_get_current(ip_reass_b0) , sf_wqe_get_current(tmp) , ip_hdr_len);
    // ip6_hdr = (ipv6_header_t *)sf_wqe_get_current(ip_reass_b0);
    ip6_frag = (ipv6_frag_t *)(sf_wqe_get_current(ip_reass_b0) + ip_hdr_len - IPV6_FRAG_HEADER_LEN);
    // sf_debug("hdr %p \n" , ip6_hdr);
    // sf_debug("frg %p \n" , ip6_frag);
    ip6_frag->ip6f_flags = 0;
#ifdef CPU_BIG_ENDIAN
    ip6_frag->offset = 0;
#else
    ip6_frag->ip6f_offset_5bit = 0;
    ip6_frag->ip6f_offset_8bit = 0;
#endif
    sf_wqe_advance(ip_reass_b0 , ip_hdr_len );

    data_ptr = sf_wqe_get_current(ip_reass_b0);


    sf_wqe_get_current_len(ip_reass_b0) = 0;


    // sf_debug("payload start : %p\n" ,data_ptr );

    ip_reass_record(ip_reass_b0)->followed_nodes_cnt = 0;

    while(tmp!= NULL)
    {
        ip_hdr_len_tmp = ip_reass_record(tmp)->ip_hdr_len;
        frag_offset = ip_reass_record(tmp)->frag_offset;
        frag_size = ip_reass_record(tmp)->frag_size;

        sf_wqe_get_current_len(ip_reass_b0) += frag_size;

        // sf_debug("offset : %d\n" , frag_offset);
        // sf_debug("frag_size : %d\n" , frag_size);

        // sf_debug("from %p to %p , len %d\n" , sf_wqe_get_current(tmp) + ip_hdr_len_tmp , data_ptr + frag_offset , frag_size);
        memcpy(data_ptr + frag_offset , sf_wqe_get_current(tmp) + ip_hdr_len_tmp , frag_size );


        ip_reass_record(ip_reass_b0)->followed_nodes_cnt += ip_reass_record(tmp)->followed_nodes_cnt + 1;

        tmp = ip_reass_record(tmp)->next_frag;

    }

    //TODO : sort


    //update ip wdata
    get_current_ip_wdata(new_wdata)->payload_len = sf_wqe_get_current_len(ip_reass_b0);
    get_current_ip_wdata(new_wdata)->reass_over = 1;

    //update  ip/tcp/udp data
    ip_reass_record(ip_reass_b0)->ip_reass_output = ip_reass_record(*ret_buffer)->ip_reass_output;
    ip_reass_record(ip_reass_b0)->interface_id = ip_reass_record(*ret_buffer)->interface_id;
    
    if(PREDICT_FALSE( ip_reass_record(ip_reass_b0)->ip_reass_output ))
    {
        sf_wqe_advance(ip_reass_b0 , -ip_hdr_len);
        update_ip_reass_pkt(ip_reass_b0);
        sf_wqe_advance(ip_reass_b0 , ip_hdr_len);
    }

    //  // reset to ip hdr
    // sf_wqe_advance(ip_reass_b0 , -ip_hdr_len);
    // new_wdata->ip_wdata_cnt --; // we will go to ip6_input later

    // sf_debug("reset ip reass : %p \n" , sf_wqe_get_current(ip_reass_b0));


    ip_reass_record(ip_reass_b0)->child_frag = *ret_buffer;
    ip_reass_record(ip_reass_b0)->next_frag = NULL;

    *ret_buffer = ip_reass_b0;
    *ret_status = 0;


}



// ret_status: >0 : reass fail , handle N pkt
//              0 : ret_buffer == NULL  : wait for more pkts
//                  ret_buffer != NULL  : reass over 
static_always_inline void ip6_frag_proc(
    vlib_main_t *vm,
    vlib_buffer_t **ret_buffer,
    int32_t *ret_status,
    uint32_t worker_index
)
{
    vlib_buffer_t *b0 = *ret_buffer;
    uint8_t is_last_frag = 0 ;
    key_ip_reass_t key;
    uint32_t hash_index;
    generic_hash_t *ip_reass_table = &(sf_ip_reass_main.table_ip_reass);
    generic_pnode_t pnode = NULL;
    node_ip_reass_t *ip_reass_data;

    sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

    ip_wdata_t *ip_wdata = get_current_ip_wdata(pkt_wdata0);
    ipv6_frag_t *ip6_frag;
    ip6_frag = (ipv6_frag_t *)( sf_wqe_get_current(b0)  + ip_wdata->header_len  - IPV6_FRAG_HEADER_LEN  );

    ip_reass_record_t *ip_reass_rec = ip_reass_record(b0);

    *ret_buffer = NULL;
    *ret_status = 0;


    // ip_reass_rec->child_frag =  NULL;
    ip_reass_rec->next_frag =  NULL;
    ip_reass_rec->frag_offset =  get_ip6_frag_offset(ip6_frag);
    ip_reass_rec->frag_offset <<= 3;
    ip_reass_rec->frag_size =  ip_wdata->payload_len;
    ip_reass_rec->ip_hdr_len =  ip_wdata->header_len;
    ip_reass_rec->interface_id = pkt_wdata0->interface_id & 0xFF; // 32bit to 8bit
    ip_reass_rec->ip_reass_output = ip_reass_config->ip_reass_output_enable[(pkt_wdata0->interface_id) - 1];

    // leaf node
    if((sf_wdata(b0)->unused8 & UNUSED8_FLAG_IP_REASS_PKT) == 0)
    {
        ip_reass_rec->followed_nodes_cnt = 0;
    }

    if( ip_reass_config->ip_reass_layers[(pkt_wdata0->interface_id) - 1]  <  (pkt_wdata0->ip_wdata_cnt)   )
    {
        *ret_buffer = b0;
        *ret_status = -1;
                sf_debug("layer overflow\n");

        return;
    }



    sf_memset_zero(&key , sizeof(key_ip_reass_t));

    key.sip.addr_v6_lower  = ip_wdata->sip.addr_v6_lower;
    key.sip.addr_v6_upper  = ip_wdata->sip.addr_v6_upper;
    key.dip.addr_v6_lower  = ip_wdata->dip.addr_v6_lower;
    key.dip.addr_v6_upper  = ip_wdata->dip.addr_v6_upper;

    key.ip_id   = ip6_frag->ip6f_ident;

    key.ip_layer   = pkt_wdata0->ip_wdata_cnt;
// sf_debug("ip_layer : %d\n" , pkt_wdata0->ip_wdata_cnt );

is_last_frag = !(ip6_frag->ip6f_flags);


    // {
    //         int i;
    //         uint8_t *data = (uint8_t *)&key;
    //         for(i=0;i<sizeof(key_ip_reass_t) ; i++)
    //         {
    //             sf_debug("%d : key  %02x\n" , i , data[i]);
    //         }
    // }

    hash_index = getHashIndex ( (uint8_t *)&key, sizeof(key_ip_reass_t));

// sf_debug("hash_index : %x\n" , hash_index);
    hashLock( ip_reass_table, hash_index); 
    pnode = hashFindNode_memcmp( ip_reass_table, hash_index, (void *)&key, sizeof(key_ip_reass_t) );
    if ( pnode )
    {
        // sf_debug("find pnode\n");
        ip_reass_data  =  (node_ip_reass_t *)node_data(pnode);

#if 1 
        {
            // duplicate pkt 
            vlib_buffer_t *tmp = ip_reass_data->head;
            while(tmp!= NULL)
            {
                if( ip_reass_record(tmp)->frag_offset  ==  ip_reass_rec->frag_offset)
                {
                    hashUnlock( ip_reass_table, hash_index ); 
                    *ret_buffer = b0;
                    *ret_status = 1;
                    return;
                }
                tmp = ip_reass_record(tmp)->next_frag;
            }
        }
#endif

        
        //append to pkt list
        ip_reass_record(ip_reass_data->tail)->next_frag = b0;
        ip_reass_data->tail = b0;

        ip_reass_data->cache_pkt_num ++;
        if(ip_reass_data->cache_pkt_num >= IP_REASS_MAX_CACHE_PKTS)
        {
                //stop timer 
                ip_reass_delete_timer(pnode);
            
                hashFreeNode(ip_reass_table, pnode, hash_index);

                hashUnlock( ip_reass_table, hash_index ); 

                *ret_buffer = ip_reass_data->head;
                *ret_status = ip_reass_data->cache_pkt_num;

                free_ip_reass_hash_node(pnode);

                 update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_timer_delete);
            update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_fail_too_many);
            sf_debug("!!!!!!!!!!!!!!!!!!! too many !!!!\n");
                return;
        }

        if( is_last_frag )
        {
            ip_reass_data->recv_last_frag = 1;
            ip_reass_data->total_len = ip_reass_rec->frag_offset + ip_reass_rec->frag_size;

            //total len too long
            if(ip_reass_data->total_len + sf_wqe_get_current_offset(b0) > SF_HUGE_PKT_MAX_DATA_SIZE)
            {
                //stop timer 
                ip_reass_delete_timer(pnode);
            
                hashFreeNode(ip_reass_table, pnode, hash_index);

                hashUnlock( ip_reass_table, hash_index ); 

                *ret_buffer = ip_reass_data->head;
                *ret_status = ip_reass_data->cache_pkt_num;

                free_ip_reass_hash_node(pnode);

                 update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_timer_delete);
                update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_fail_too_large);
                            sf_debug("!!!!!!!!!!!!!!!!!!! too large !!!!\n");

                return;
            }
        }

        ip_reass_data->meet_len += ip_reass_rec->frag_size;

        if(ip_reass_data->meet_len == ip_reass_data->total_len)
        {
            sf_debug("reass over\n");
            //stop timer 
            ip_reass_delete_timer(pnode);
            
            hashFreeNode(ip_reass_table, pnode, hash_index);

            hashUnlock( ip_reass_table, hash_index ); 


            *ret_buffer = ip_reass_data->head;
            *ret_status = ip_reass_data->cache_pkt_num;

            ip6_frag_build_pkt(vm , ret_buffer , ret_status);

            free_ip_reass_hash_node(pnode);


            update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_timer_delete);
            return;

        }
        ip_reass_data->recv_new_pkt_in_interval = 1;

        hashUnlock( ip_reass_table, hash_index ); 
        return;
    }
    else
    {


                // sf_debug("alloc pnode\n");

        pnode = alloc_ip_reass_hash_node ();
        if(pnode == NULL)
        {
            hashUnlock( ip_reass_table, hash_index ); 

            *ret_buffer = b0;
            *ret_status = 1;
            return;
        }

        ip_reass_data  =  (node_ip_reass_t *)node_data(pnode);

        sf_memset_zero(ip_reass_data , sizeof(node_ip_reass_t));
        memcpy( &(ip_reass_data->key)  , &key , sizeof(key_ip_reass_t));

        if (PREDICT_FALSE(hashInsertNode(ip_reass_table, hash_index, pnode) != 0)) 
        {
            free_ip_reass_hash_node (pnode );
            hashUnlock( ip_reass_table, hash_index ); 

            *ret_buffer = b0;
            *ret_status = 1;
            return;
        }

        //submit timer
        if(ip_reass_submit_timer( pnode,  hash_index ,  ip_reass_config->timeout_sec ) !=  0)
        {
            hashFreeNode(ip_reass_table, pnode, hash_index);
            free_ip_reass_hash_node (pnode );
            hashUnlock( ip_reass_table, hash_index ); 

            *ret_buffer = b0;
            *ret_status = 1;
            return;
        }

        update_ip_reass_stat(worker_index , ip_reass_rec->interface_id , ip_reass_timer_submit);


        //append to pkt list
        ip_reass_data->head = b0;
        ip_reass_data->tail = b0;
        ip_reass_rec->next_frag = NULL;

        ip_reass_data->interface_id = ip_reass_rec->interface_id;

        if( is_last_frag  )
        {
            ip_reass_data->recv_last_frag = 1;
            ip_reass_data->total_len = ip_reass_rec->frag_offset + ip_reass_rec->frag_size;
        }


        ip_reass_data->meet_len += ip_reass_rec->frag_size;


        hashUnlock( ip_reass_table, hash_index ); 
        return;
    }
    
}



static __thread uint32_t *vlib_buffer_array = NULL;
static __thread uint16_t *nexts = NULL;
VLIB_NODE_FN(sf_ip6_reass_node)
(vlib_main_t *vm,
           vlib_node_runtime_t *node,
           vlib_frame_t *frame)
{
    u32 n_left_from, *from;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

        sf_debug("recv %d pkts\n", n_left_from);

    // vlib_buffer_t *ret_buffer;
    int32_t ret_status;

    vec_validate(vlib_buffer_array, VLIB_FRAME_SIZE);
    vec_validate(nexts, VLIB_FRAME_SIZE);
    uint32_t pre_index = 0;
    uint32_t count = 0;

    uint32_t worker_index = sf_get_current_worker_index();
    
    while (n_left_from > 0)
    {
       
        u32 bi0;
        vlib_buffer_t *b0;
        
        bi0 = from[0];
        from += 1;
        n_left_from -= 1;

        b0 = vlib_get_buffer(vm, bi0);
        /*******************  start handle pkt  ***************/
        uint8_t protocol;

        // sf_debug("current offset: %d \n" , sf_wqe_get_current_offset(b0));

        ip6_frag_proc(vm , &b0 ,  &ret_status , worker_index);

        if(ret_status == 0 && b0 != NULL)
        {
            vlib_buffer_array[pre_index] = vlib_get_buffer_index(vm ,b0);
            protocol = (get_current_ip_wdata(sf_wdata(b0)))->protocol ;
            nexts[pre_index] = sf_ip_reass_get_next_index(protocol);
            pre_index ++;
        }
        else if(ret_status > 0 )
        {
            // Notice : not accurate 
            if( PREDICT_FALSE( (pre_index + ret_status * IP_REASS_MAX_CACHE_PKTS ) >= VLIB_FRAME_SIZE) )
            {
                vlib_buffer_enqueue_to_next(vm , node , vlib_buffer_array , nexts , (uint64_t)pre_index);
                pre_index = 0;
            }

            count = split_frag_buffer_on_fail( vm , b0 , 
                vlib_buffer_array + pre_index , 
                nexts + pre_index , 
                worker_index ,
                ip_reass_record(b0)->interface_id ,
                SF_NEXT_NODE_SF_ACL_ENTRY , 
                SF_NEXT_NODE_SF_PKT_DROP );


            pre_index += count;
        }
        else if(ret_status == -1 )
        // reass layer config is smaller
        {
            vlib_buffer_array[pre_index] = vlib_get_buffer_index(vm ,b0);
            nexts[pre_index] = SF_NEXT_NODE_SF_ACL_ENTRY;
            pre_index ++;
        }
        

        if(PREDICT_FALSE(pre_index >= VLIB_FRAME_SIZE))
        {
            vlib_buffer_enqueue_to_next(vm , node , vlib_buffer_array , nexts , (uint64_t)pre_index);
            pre_index = 0;
        }
        
    }

    vlib_buffer_enqueue_to_next(vm , node , vlib_buffer_array , nexts , (uint64_t)pre_index);


    return frame->n_vectors;
}

