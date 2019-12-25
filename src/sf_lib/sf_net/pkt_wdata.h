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
#ifndef __pkt_wdata_h__
#define __pkt_wdata_h__

#include <vlib/vlib.h>

#include <stdint.h>

#include "sf_huge_pkt.h"
#include "common_header.h"

//check if the <vlib/buffer.h> is right
#ifndef VLIB_SF_WDATA_SIZE

#error no sf_wdata_area in vlib_buffer_t

#endif


/***** !!!!notice!!!! *****/
#include "new_pkt_wdata.h"

#define set_packet_type(pkt_wdata, value) (pkt_wdata)->packet_type |= (1ULL << (value))
#define has_packet_type(pkt_wdata, value) ( (pkt_wdata)->packet_type & (1ULL << (value)) )
/***  !!!!!! Notice !!!!!
 *  with wdata : this pkt owns a wdata , and needs to be freed in the future
 *  without wdata : this pkt shares a wdata with other pkt , but no need to be freed
 */
#define UNUSED8_FLAG_WDATA (1 << 0)
#define UNUSED8_FLAG_IS_PKT (1 << 1)
#define UNUSED8_FLAG_HUGE_PKT (1 << 2)
#define UNUSED8_FLAG_IP_REASS_PKT (1 << 3)
#define UNUSED8_FLAG_PKT_TRACED (1 << 4)

#define UNUSED8_FLAG_PKT_RECORD_DROP UNUSED8_FLAG_WDATA

// 111
#define UNUSED8_PKT_HUGE_PKT_WITH_WDATA 7

#define sf_wdata(b0) ((sf_wdata_t *)((b0)->sf_wdata_area))

#define sf_wdata_prefetch(b0) CLIB_PREFETCH((b0)->sf_wdata_area , 128 , STORE)
#define sf_wdata_prefetch_full(b0) CLIB_PREFETCH((b0)->sf_wdata_area , 256 , STORE)
#define sf_wdata_prefetch_cache(b0) CLIB_PREFETCH((b0)->sf_wdata_area , CLIB_CACHE_LINE_BYTES , STORE)

#define sf_prefetch_data(b0)  CLIB_PREFETCH(b0->data, CLIB_CACHE_LINE_BYTES, LOAD);
#define sf_prefetch_data_store(b0)  CLIB_PREFETCH(b0->data, CLIB_CACHE_LINE_BYTES, STORE);

#define sf_prefetch_pre_data(b0) CLIB_PREFETCH(b0->pre_data, CLIB_CACHE_LINE_BYTES, STORE);

#define is_vlib_b_traced(b0) (sf_wdata(b0)->unused8 & UNUSED8_FLAG_PKT_TRACED )

/************************************/
#define sf_wqe_get_data_ptr(b0) (sf_wdata(b0)->data_ptr)

#define sf_wqe_get_current(b0) \
    (sf_wdata(b0)->data_ptr + sf_wdata(b0)->current_data)

#define sf_wqe_get_tail(b0) \
    (sf_wdata(b0)->data_ptr + sf_wdata(b0)->current_data + sf_wdata(b0)->current_length)

#define sf_wqe_get_current_len(b0) (sf_wdata(b0)->current_length)
#define sf_wqe_get_current_offset(b0) (sf_wdata(b0)->current_data)

/* l can be negative */
#define sf_wqe_advance(b0, l)      \
    {                              \
        sf_wdata(b0)->current_data += l;   \
        sf_wdata(b0)->current_length -= l; \
    }

/* !!!! Notice !!! , don't use it if current_data is negative or less than  sf_opaque(b0)->l2_hdr_offset*/
#define sf_wqe_reset(b0)                                                           \
    {                                                                              \
        sf_wdata(b0)->current_length += sf_wdata(b0)->current_data - sf_wdata(b0)->l2_hdr_offset; \
        sf_wdata(b0)->current_data = sf_wdata(b0)->l2_hdr_offset;                         \
    }



#define sf_wqe_get_data_ptr_wdata(wdata) ((wdata)->data_ptr)

#define sf_wqe_get_current_wdata(wdata) \
    ((wdata)->data_ptr + (wdata)->current_data)

#define sf_wqe_get_cur_len_wdata(wdata) ((wdata)->current_length)
#define sf_wqe_get_cur_offset_wdata(wdata) ((wdata)->current_data)

/* l can be negative */
#define sf_wqe_advance_wdata(wdata, l)      \
    {                              \
        (wdata)->current_data += l;   \
        (wdata)->current_length -= l; \
    }




/* !!!! Notice !!! , don't use it if current_data is negative or less than (wdata)->l2_hdr_offset*/
#define sf_wqe_reset_wdata(wdata)                                                           \
    {                                                                              \
        (wdata)->current_length += (wdata)->current_data - (wdata)->l2_hdr_offset; \
        (wdata)->current_data = (wdata)->l2_hdr_offset;                         \
    }

/************************************/
typedef struct{
    uint32_t buffer_index;
    int16_t current_data;
    uint16_t current_length;
}sf_hugepkt_sub_t;

typedef struct{ 
    uint32_t sub_total_len_no_first; // !!! not include first buffer
    uint32_t sub_buffer_cnt;
    sf_hugepkt_sub_t sub_buffer[0];
}sf_hugepkt_helper_t;

// using pre data , max : ( 128 - 8 ) / 8 ; also notice dsa tag
#define SF_HUFEPKT_MAX_SUB_BUFFER_CNT (10)

#define sf_hugepkt_helper(wdata) ( (sf_hugepkt_helper_t *)((wdata)->recv_pre_data) )

#endif /* __pkt_wdata_h__ */
