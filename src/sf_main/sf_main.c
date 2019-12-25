
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
#include <vnet/ethernet/ethernet.h>

#include "sf_main.h"

#include "sf_interface.h"

int fix_sf_main_link_error()
{
    return 0;
}


#if defined(RUN_ON_VM)
#define SKIP_SOME_SW_IF                  \
    if (sw_if_index > MAX_INTERFACE_CNT) \
    {                                    \
        break;                           \
    }
#else
#error Unknown Device
#endif 

static clib_error_t *sf_up_interfaces_all_fn(vlib_main_t *vm,
                                             unformat_input_t *input,
                                             vlib_cli_command_t *cmd)
{
    clib_error_t *error = 0;
    vnet_main_t *vnm = vnet_get_main();
    vnet_interface_main_t *im = &vnm->interface_main;
    // vlib_cli_output(vm , "total : %d\n"  ,  pool_elts (im->sw_interfaces));
    u32 flags = ETHERNET_INTERFACE_FLAG_ACCEPT_ALL;       
    ethernet_main_t *em = &ethernet_main;
    ethernet_interface_t *eif;


    int sw_if_count = pool_elts(im->sw_interfaces);
    int sw_if_index = 0;
    for (sw_if_index = 1; sw_if_index < sw_if_count; sw_if_index++)
    {
        SKIP_SOME_SW_IF
        
        error = vnet_sw_interface_set_flags(vnm, sw_if_index, VNET_SW_INTERFACE_FLAG_ADMIN_UP);
        if (error)
            goto done;

        eif = ethernet_get_interface (em, im->sw_interfaces[sw_if_index].hw_if_index);
        if (!eif)
            return clib_error_return (0, "sw_if %d not ethernet interface" , sw_if_index);

        ethernet_set_flags (vnm, im->sw_interfaces[sw_if_index].hw_if_index, flags);
    }

done:
    return error;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_enable_interfaces_all_command, static) = {
    .path = "sf up interfaces all",
    .short_help =
        "sf up interfaces all",
    .function = sf_up_interfaces_all_fn,
};
