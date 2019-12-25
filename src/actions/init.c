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

#include "sf_main.h"
#include "sf_tools.h"

#include "action.h"
#include "action_main.h"

#include "sf_intf_define.h"

#include "action_path_tools.h"

#define ACTION_POOL_SIZE 128

/********************************************************/
/***********init action pool and hash table****************/

sf_action_main_t  sf_action_main;
static clib_error_t *sf_action_init(vlib_main_t *vm)
{
    sf_action_main_t *sf_action = &sf_action_main;
    clib_error_t  *error  = 0;
    clib_memset_u8(sf_action, 0 , sizeof(sf_action_main_t));
    sf_action->cpu_clock_pre_second = vm->clib_time.clocks_per_second;
    
    fix_sf_main_link_error();


    return error;
}
VLIB_INIT_FUNCTION(sf_action_init);

