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
#ifndef __included_sf_main_h__
#define __included_sf_main_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ethernet/ethernet.h>
#include <vlib/vlib.h>

#include <vppinfra/hash.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>

#include <vppinfra/time.h>

#include "pkt_wdata.h"
#include <stdio.h>
#include <time.h>

#include "sf_pool.h"
#include "sf_timer.h"

#include "sf_tools.h"


typedef struct
{
    /* convenience */
    vnet_main_t *vnet_main;
    vlib_main_t *vlib_main;

    uint64_t cpu_clock_pre_second;

    sf_pool_head_t huge_pkt_16k_pool;

    uint8_t *intf_output_next_index;
    uint8_t *hugepkt_output_next_index;

} sf_main_t;

extern sf_main_t sf_main;






/***** tools ******/
int fix_sf_main_link_error();


#define CALL_SHM_INIT_FUNC(vm, error)                 \
    error = vlib_call_init_function(vm, sf_shm_init); \
    if (error)                                        \
    {                                                 \
        clib_error_report(error);                     \
        return error;                                 \
    }




#endif /* __included_sf_main_h__ */
