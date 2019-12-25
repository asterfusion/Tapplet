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
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>
#include <vppinfra/os.h>

#include <stdio.h>

#include "pkt_wdata.h"
#include "sf_main.h"

#include "generic_hash.h"

#include "sf_ip_reass.h"

#include "sf_ip_reass_shm.h"

#include "sf_thread_tools.h"

static clib_error_t * ip_reass_hash_stat_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
  // sf_main_t * sm = &sf_main;
    vlib_cli_output(vm , "%U\n" , format_hash_table , sf_ip_reass_main.table_ip_reass);

  
  return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND (ip_reass_hash_stat_cmd, static) = {
    .path = "sf ip-reass hash",
    .short_help = 
    "sf ip-reass hash",
    .function = ip_reass_hash_stat_fn,
    .is_mp_safe = 1,
};

static clib_error_t * show_ip_reass_stat_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
  
  uint32_t worker_cnt = sf_vlib_num_workers();
  uint32_t i;
  uint32_t j;
  for(i=0; i<worker_cnt ; i++)
  {
    for(j=0; j<MAX_INTERFACE_CNT ; j++)
    {

vlib_cli_output(vm , "\n-- thread %d -- interface %d -- \n" , i , j);
#undef foreach_member
#define foreach_member                                               \
    FUNC(ip_reass_pkt_success)                             \
    FUNC(ip_reass_pkt_fail)                           \
    FUNC(ip_reass_success)                            \
    FUNC(ip_reass_fail)       \
    FUNC(ip_reass_fail_too_large)                             \
    FUNC(ip_reass_fail_too_many)                             \
    FUNC(ip_reass_timer_submit)                           \
    FUNC(ip_reass_timer_delete)                            \
    FUNC(ip_reass_timer_timeout)                                        
#undef FUNC

#define FUNC(a) vlib_cli_output(vm , "%30s : %lu\n" , #a , ip_reass_stat->local_stat[i][j].a);
foreach_member
#undef FUNC

    }
  }
  
  return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND (show_ip_reass_stat_cmd, static) = {
    .path = "show sf-ip-reass stat",
    .short_help = 
    "show sf-ip-reass stat",
    .function = show_ip_reass_stat_fn,
    .is_mp_safe = 1,
};


static clib_error_t * clean_ip_reass_stat_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
  
  uint32_t worker_cnt = sf_vlib_num_workers();
  uint32_t i;
  uint32_t j;
  for(i=0; i<worker_cnt ; i++)
  {
    for(j=0; j<MAX_INTERFACE_CNT ; j++)
    {

#undef foreach_member
#define foreach_member                                               \
    FUNC(ip_reass_pkt_success)                             \
    FUNC(ip_reass_pkt_fail)                           \
    FUNC(ip_reass_success)                            \
    FUNC(ip_reass_fail)       \
    FUNC(ip_reass_fail_too_large)                             \
    FUNC(ip_reass_fail_too_many)                             \
    FUNC(ip_reass_timer_submit)                           \
    FUNC(ip_reass_timer_delete)                            \
    FUNC(ip_reass_timer_timeout)                                        
#undef FUNC

#define FUNC(a) ip_reass_stat->local_stat[i][j].a = 0;
foreach_member
#undef FUNC

    }
  }
  
  return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND (clean_ip_reass_stat_cmd, static) = {
    .path = "clear sf-ip-reass stat",
    .short_help = 
    "clear sf-ip-reass stat",
    .function = clean_ip_reass_stat_fn,
    .is_mp_safe = 1,
};
