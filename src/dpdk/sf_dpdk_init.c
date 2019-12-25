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
#include <vppinfra/error.h>

#include <dpdk/device/dpdk.h>
#include <dpdk/device/dpdk_priv.h>

#include "sf_intf_phy_stat.h"

int dpdk_get_sw_if_statics_func(int sw_if_index, sf_port_statics_t *results)
{

    vnet_main_t *vnm = vnet_get_main();

    vnet_sw_interface_t *sw = vnet_get_sw_interface_safe(vnm , sw_if_index);

    if(sw == NULL)
    {
        return -1;
    }

    vnet_hw_interface_t *hi = vnet_get_hw_interface_safe(vnm, sw->hw_if_index);


    if(hi == NULL)
    {
        return -1;
    }


    dpdk_main_t *dm = &dpdk_main;
    dpdk_device_t *xd = vec_elt_at_index(dm->devices, hi->dev_instance);

    f64 now = vlib_time_now(dm->vlib_main);

    dpdk_update_counters(xd, now);

#if 0
#define foreach_dpdk_counter  \
    _(tx_frames_ok, opackets) \
    _(tx_bytes_ok, obytes)    \
    _(tx_errors, oerrors)     \
    _(rx_frames_ok, ipackets) \
    _(rx_bytes_ok, ibytes)    \
    _(rx_errors, ierrors)     \
    _(rx_missed, imissed)     \
    _(rx_no_bufs, rx_nombuf)

  /* $$$ MIB counters  */
  {
#define _(N, V)                                             \
    if ((xd->stats.V - xd->last_cleared_stats.V) != 0)      \
    {                                                       \
        s = format(s, "\n%U%-40U%16Lu",                     \
                   format_white_space, indent + 2,          \
                   format_c_identifier, #N,                 \
                   xd->stats.V - xd->last_cleared_stats.V); \
    }                                                          \

    foreach_dpdk_counter
#undef _
  }
#endif
#undef FUNC
#define FUNC(name) (xd->stats.name)

    results->rx_packets = FUNC(ipackets);
    results->rx_octets = FUNC(ibytes);
    results->rx_error = FUNC(ierrors);
    results->rx_miss = FUNC(imissed) + FUNC(rx_nombuf);

    results->tx_packets = FUNC(opackets);
    results->tx_octets = FUNC(obytes);
    results->tx_error = FUNC(oerrors);

    results->mask = (uint64_t)(-1);

    return 0;
}

int dpdk_get_sw_if_phy_stat(int sw_if_index , intf_phy_status_t *results)
{
    vnet_main_t *vnm = vnet_get_main();

    vnet_sw_interface_t *sw = vnet_get_sw_interface_safe(vnm , sw_if_index);

    if(sw == NULL)
    {
        return -1;
    }

    vnet_hw_interface_t *hi = vnet_get_hw_interface_safe(vnm, sw->hw_if_index);


    if(hi == NULL)
    {
        return -1;
    }


    dpdk_main_t *dm = &dpdk_main;
    dpdk_device_t *xd = vec_elt_at_index(dm->devices, hi->dev_instance);

    f64 now = vlib_time_now(dm->vlib_main);

    dpdk_update_link_state(xd, now);

    results->link = xd->link.link_status ? 1 : 0;
    results->speed = xd->link.link_speed;

    switch (xd->link.link_duplex)
	{
	case ETH_LINK_HALF_DUPLEX:
	  results->duplex = SF_INTF_LINK_HALF_DUPLEX;
	  break;
	case ETH_LINK_FULL_DUPLEX:
	  results->duplex = SF_INTF_LINK_FULL_DUPLEX;
	  break;
	default:
	  break;
	}

    results->mtu = hi->max_packet_bytes;

    return 0;
}

static clib_error_t *dpdk_intf_stat_fn_init(vlib_main_t *vm)
{
    sf_intf_dpdk_reg_statics_fn(dpdk_get_sw_if_statics_func);
    sf_intf_dpdk_reg_phy_stat_fn(dpdk_get_sw_if_phy_stat);
    return 0;
}
VLIB_INIT_FUNCTION(dpdk_intf_stat_fn_init);



#if 0
static clib_error_t *
test_dpdk_statics_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{

    sf_port_statics_t temp;

    dpdk_get_sw_if_statics(2 , &temp);


    vlib_cli_output (vm, "%lu\n",
                temp.rx_packets);

    vlib_cli_output (vm, "%U\n",
                format_dpdk_device, (u32)2, (int) 0);



    dpdk_get_sw_if_statics(3 , &temp);


    vlib_cli_output (vm, "\n\n%lu\n",
                temp.rx_packets);

    vlib_cli_output (vm, "%U\n",
                format_dpdk_device, (u32)3, (int) 0);

    return 0;
}


VLIB_CLI_COMMAND (test_dpdk_statics, static) = {
    .path = "sf test dpdk statics",
    .short_help = "sf test dpdk statics",
    .function = test_dpdk_statics_fn,
};
#endif