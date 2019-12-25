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
#include <vppinfra/format.h>
#include <vlib/unix/cj.h>
#include <assert.h>

#include <vnet/ethernet/ethernet.h>
#include <dpdk/device/dpdk.h>

#include <dpdk/device/dpdk_priv.h>
#include <vppinfra/error.h>


// #define SF_DEBUG
#include "sf_debug.h"
#include "pkt_wdata.h"


#define foreach_dpdk_tx_func_error			\
  _(BAD_RETVAL, "DPDK tx function returned an error")	\
  _(PKT_DROP, "Tx packet drops (dpdk tx failure)")

typedef enum
{
#define _(f,s) DPDK_TX_FUNC_ERROR_##f,
  foreach_dpdk_tx_func_error
#undef _
    DPDK_TX_FUNC_N_ERROR,
} dpdk_tx_func_error_t;

static char *dpdk_tx_func_error_strings[] = {
#define _(n,s) s,
  foreach_dpdk_tx_func_error
#undef _
};

static clib_error_t *
dpdk_set_mac_address (vnet_hw_interface_t * hi,
		      const u8 * old_address, const u8 * address)
{
  int error;
  dpdk_main_t *dm = &dpdk_main;
  dpdk_device_t *xd = vec_elt_at_index (dm->devices, hi->dev_instance);

  error = rte_eth_dev_default_mac_addr_set (xd->port_id,
					    (struct ether_addr *) address);

  if (error)
    {
      return clib_error_return (0, "mac address set failed: %d", error);
    }
  else
    {
      vec_reset_length (xd->default_mac_address);
      vec_add (xd->default_mac_address, address, sizeof (address));
      return NULL;
    }
}



static_always_inline void
sf_dpdk_validate_rte_mbuf (vlib_main_t * vm, vlib_buffer_t* b ,sf_wdata_t * pkt_wdata,
			int maybe_multiseg , uint32_t *total_bytes_ptr)
{
  int i;
  struct rte_mbuf *mb, *first_mb, *last_mb;
  sf_hugepkt_helper_t *hugepkt_helper;
  sf_hugepkt_sub_t *pre_sub_buffer;

  /* buffer is coming from non-dpdk source so we need to init
     rte_mbuf header */
  // if (PREDICT_FALSE ((b->flags & VLIB_BUFFER_EXT_HDR_VALID) == 0))
  //   {
  //   }

  last_mb = first_mb = mb = rte_mbuf_from_vlib_buffer (b);
  first_mb->nb_segs = 1;

  *total_bytes_ptr += pkt_wdata->current_length;

  mb->data_len = pkt_wdata->current_length;
  mb->pkt_len = pkt_wdata->current_length;
  mb->data_off = VLIB_BUFFER_PRE_DATA_SIZE + pkt_wdata->current_data;

sf_debug("data %d len %d\n" ,pkt_wdata->current_data ,  pkt_wdata->current_length);

  if( maybe_multiseg && (pkt_wdata->unused8 & UNUSED8_FLAG_HUGE_PKT))
  {
      last_mb->next = NULL;
    hugepkt_helper = sf_hugepkt_helper(pkt_wdata);

        mb->data_len = hugepkt_helper->sub_buffer[0].current_length;
        
    for(i=1 ; i<hugepkt_helper->sub_buffer_cnt ; i++)
    {
      pre_sub_buffer = hugepkt_helper->sub_buffer + i;

      b = vlib_get_buffer (vm, pre_sub_buffer->buffer_index);
      mb = rte_mbuf_from_vlib_buffer (b);

      last_mb->next = mb;
      last_mb = mb;
      last_mb->next = NULL;
      mb->data_len = pre_sub_buffer->current_length;
      mb->pkt_len = pre_sub_buffer->current_length;
      mb->data_off = VLIB_BUFFER_PRE_DATA_SIZE + pre_sub_buffer->current_data;
      first_mb->nb_segs++;
    }
  }

}

/*
 * This function calls the dpdk's tx_burst function to transmit the packets.
 * It manages a lock per-device if the device does not
 * support multiple queues. It returns the number of packets untransmitted
 * If all packets are transmitted (the normal case), the function returns 0.
 */
static_always_inline
  u32 tx_burst_vector_internal (vlib_main_t * vm,
				dpdk_device_t * xd,
				struct rte_mbuf **mb, u32 n_left)
{
  dpdk_main_t *dm = &dpdk_main;
  // u32 n_retry;
  int n_sent = 0;
  int queue_id;

  // n_retry = 16;
  queue_id = vm->thread_index;

  do
    {
      /*
       * This device only supports one TX queue,
       * and we're running multi-threaded...
       */
      if (PREDICT_FALSE (xd->lockp != 0))
	{
	  queue_id = queue_id % xd->tx_q_used;
	  while (__sync_lock_test_and_set (xd->lockp[queue_id], 1))
	    /* zzzz */
	    queue_id = (queue_id + 1) % xd->tx_q_used;
	}

      if (PREDICT_FALSE (xd->flags & DPDK_DEVICE_FLAG_HQOS))	/* HQoS ON */
	{
	  /* no wrap, transmit in one burst */
	  dpdk_device_hqos_per_worker_thread_t *hqos =
	    &xd->hqos_wt[vm->thread_index];

	  ASSERT (hqos->swq != NULL);

	  dpdk_hqos_metadata_set (hqos, mb, n_left);
	  n_sent = rte_ring_sp_enqueue_burst (hqos->swq, (void **) mb,
					      n_left, 0);
	}
      else if (PREDICT_TRUE (xd->flags & DPDK_DEVICE_FLAG_PMD))
	{
	  /* no wrap, transmit in one burst */
	  n_sent = rte_eth_tx_burst (xd->port_id, queue_id, mb, n_left);
	}
      else
	{
	  ASSERT (0);
	  n_sent = 0;
	}

      if (PREDICT_FALSE (xd->lockp != 0))
	*xd->lockp[queue_id] = 0;

      if (PREDICT_FALSE (n_sent < 0))
	{
	  // emit non-fatal message, bump counter
	  vnet_main_t *vnm = dm->vnet_main;
	  vnet_interface_main_t *im = &vnm->interface_main;
	  u32 node_index;

	  node_index = vec_elt_at_index (im->hw_interfaces,
					 xd->hw_if_index)->tx_node_index;

	  vlib_error_count (vm, node_index, DPDK_TX_FUNC_ERROR_BAD_RETVAL, 1);
	  clib_warning ("rte_eth_tx_burst[%d]: error %d",
			xd->port_id, n_sent);
	  return n_left;	// untransmitted packets
	}
      n_left -= n_sent;
      mb += n_sent;
    }
  // while (n_sent && n_left && (n_retry > 0));
  while (n_left);

  return n_left;
}

static_always_inline void
sf_dpdk_prefetch_buffer_wdata (vlib_main_t * vm, struct rte_mbuf *mb)
{
  vlib_buffer_t *b = vlib_buffer_from_rte_mbuf (mb);
  CLIB_PREFETCH (mb, 2 * CLIB_CACHE_LINE_BYTES, STORE);
  CLIB_PREFETCH (sf_wdata(b) , CLIB_CACHE_LINE_BYTES, LOAD);
}

static_always_inline void
sf_dpdk_prefetch_pkt_data (vlib_main_t * vm, struct rte_mbuf *mb)
{
  vlib_buffer_t *b = vlib_buffer_from_rte_mbuf (mb);
  CLIB_PREFETCH (b->data - CLIB_CACHE_LINE_BYTES , 2 * CLIB_CACHE_LINE_BYTES, STORE);
}

/*
 * Transmits the packets on the frame to the interface associated with the
 * node. It first copies packets on the frame to a per-thread arrays
 * containing the rte_mbuf pointers.
 */
VNET_DEVICE_CLASS_TX_FN (dpdk_device_class) (vlib_main_t * vm,
					     vlib_node_runtime_t * node,
					     vlib_frame_t * f)
{
  dpdk_main_t *dm = &dpdk_main;
  vnet_interface_output_runtime_t *rd = (void *) node->runtime_data;
  dpdk_device_t *xd = vec_elt_at_index (dm->devices, rd->dev_instance);
  u32 n_packets = f->n_vectors;
  u32 n_left;
  // u32 *from;
  u32 thread_index = vm->thread_index;
  vnet_main_t *vnm = vnet_get_main ();
  vnet_interface_main_t *im = &vnm->interface_main;
  // int queue_id = thread_index;
  u32 tx_pkts = 0;
  dpdk_per_thread_data_t *ptd = vec_elt_at_index (dm->per_thread_data,
						  thread_index);
  struct rte_mbuf **mb;
  vlib_buffer_t *b[4];
  sf_wdata_t *pkt_wdata[4];

  uint32_t total_bytes = 0;

  // from = vlib_frame_vector_args (f);

  ASSERT (n_packets <= VLIB_FRAME_SIZE);

  sf_debug("recv %d pkts\n", f->n_vectors);



  /* TX PCAP tracing */
  if (PREDICT_FALSE (dm->pcap[VLIB_TX].pcap_enable))
    {

    }


  /* calculate rte_mbuf pointers out of buffer indices */
  vlib_get_buffers_with_offset (vm, vlib_frame_vector_args (f),
				(void **) ptd->mbufs, n_packets,
				-(i32) sizeof (struct rte_mbuf));

  // from = vlib_frame_vector_args (f);
  n_left = n_packets;
  mb = ptd->mbufs;

  while (n_left >= 8)
    {
      u32 or_flags;

      sf_dpdk_prefetch_buffer_wdata (vm, mb[4]);
      sf_dpdk_prefetch_buffer_wdata (vm, mb[5]);
      sf_dpdk_prefetch_buffer_wdata (vm, mb[6]);
      sf_dpdk_prefetch_buffer_wdata (vm, mb[7]);


      b[0] = vlib_buffer_from_rte_mbuf (mb[0]);
      b[1] = vlib_buffer_from_rte_mbuf (mb[1]);
      b[2] = vlib_buffer_from_rte_mbuf (mb[2]);
      b[3] = vlib_buffer_from_rte_mbuf (mb[3]);

      pkt_wdata[0] = sf_wdata(b[0]);
      pkt_wdata[1] = sf_wdata(b[1]);
      pkt_wdata[2] = sf_wdata(b[2]);
      pkt_wdata[3] = sf_wdata(b[3]);

      or_flags = pkt_wdata[0]->unused8 | pkt_wdata[1]->unused8 
        | pkt_wdata[2]->unused8 | pkt_wdata[3]->unused8 ;


      if (or_flags & UNUSED8_FLAG_HUGE_PKT)
	{
	  sf_dpdk_validate_rte_mbuf (vm, b[0] , pkt_wdata[0], 1 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[1] , pkt_wdata[1], 1 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[2] , pkt_wdata[2], 1 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[3] , pkt_wdata[3], 1 , &total_bytes);
	}
      else
	{
    sf_dpdk_validate_rte_mbuf (vm, b[0] , pkt_wdata[0], 0 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[1] , pkt_wdata[1], 0 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[2] , pkt_wdata[2], 0 , &total_bytes);
	  sf_dpdk_validate_rte_mbuf (vm, b[3] , pkt_wdata[3], 0 , &total_bytes);
	}

      if (PREDICT_FALSE (node->flags & VLIB_NODE_FLAG_TRACE))
	{
	}

      mb += 4;
      n_left -= 4;
    }
  while (n_left > 0)
    {
      b[0] = vlib_buffer_from_rte_mbuf (mb[0]);
      pkt_wdata[0] = sf_wdata(b[0]);

      sf_dpdk_validate_rte_mbuf (vm, b[0] , pkt_wdata[0], 1 , &total_bytes);
      
      if (PREDICT_FALSE (node->flags & VLIB_NODE_FLAG_TRACE))
      {
        	if (b[0]->flags & VLIB_BUFFER_IS_TRACED)
        {

        }
      }

      mb++;
      n_left--;
    }



    vlib_increment_combined_counter (im->combined_sw_if_counters +
            VNET_INTERFACE_COUNTER_TX,
            thread_index,  xd->sw_if_index, n_packets,
            total_bytes);

/******************************************************/
/******************************************************/

  /* transmit as many packets as possible */
  tx_pkts = n_packets = mb - ptd->mbufs;
  n_left = tx_burst_vector_internal (vm, xd, ptd->mbufs, n_packets);

  {
    /* If there is no callback then drop any non-transmitted packets */
    if (PREDICT_FALSE (n_left))
      {
	tx_pkts -= n_left;
	vlib_simple_counter_main_t *cm;
	vnet_main_t *vnm = vnet_get_main ();

	cm = vec_elt_at_index (vnm->interface_main.sw_if_counters,
			       VNET_INTERFACE_COUNTER_TX_ERROR);

	vlib_increment_simple_counter (cm, thread_index, xd->sw_if_index,
				       n_left);

	vlib_error_count (vm, node->node_index, DPDK_TX_FUNC_ERROR_PKT_DROP,
			  n_left);

	while (n_left--)
	  rte_pktmbuf_free (ptd->mbufs[n_packets - n_left - 1]);
      }
  }

  return tx_pkts;
}

static void
dpdk_clear_hw_interface_counters (u32 instance)
{
  dpdk_main_t *dm = &dpdk_main;
  dpdk_device_t *xd = vec_elt_at_index (dm->devices, instance);

  /*
   * Set the "last_cleared_stats" to the current stats, so that
   * things appear to clear from a display perspective.
   */
  dpdk_update_counters (xd, vlib_time_now (dm->vlib_main));

  clib_memcpy (&xd->last_cleared_stats, &xd->stats, sizeof (xd->stats));
  clib_memcpy (xd->last_cleared_xstats, xd->xstats,
	       vec_len (xd->last_cleared_xstats) *
	       sizeof (xd->last_cleared_xstats[0]));

}

static clib_error_t *
dpdk_interface_admin_up_down (vnet_main_t * vnm, u32 hw_if_index, u32 flags)
{
  vnet_hw_interface_t *hif = vnet_get_hw_interface (vnm, hw_if_index);
  uword is_up = (flags & VNET_SW_INTERFACE_FLAG_ADMIN_UP) != 0;
  dpdk_main_t *dm = &dpdk_main;
  dpdk_device_t *xd = vec_elt_at_index (dm->devices, hif->dev_instance);

  if (xd->flags & DPDK_DEVICE_FLAG_PMD_INIT_FAIL)
    return clib_error_return (0, "Interface not initialized");

  if (is_up)
    {
      if ((xd->flags & DPDK_DEVICE_FLAG_ADMIN_UP) == 0)
	dpdk_device_start (xd);
      xd->flags |= DPDK_DEVICE_FLAG_ADMIN_UP;
      f64 now = vlib_time_now (dm->vlib_main);
      dpdk_update_counters (xd, now);
      dpdk_update_link_state (xd, now);
    }
  else
    {
      vnet_hw_interface_set_flags (vnm, xd->hw_if_index, 0);
      if ((xd->flags & DPDK_DEVICE_FLAG_ADMIN_UP) != 0)
	dpdk_device_stop (xd);
      xd->flags &= ~DPDK_DEVICE_FLAG_ADMIN_UP;
    }

  return /* no error */ 0;
}

/*
 * Dynamically redirect all pkts from a specific interface
 * to the specified node
 */
static void
dpdk_set_interface_next_node (vnet_main_t * vnm, u32 hw_if_index,
			      u32 node_index)
{
  dpdk_main_t *xm = &dpdk_main;
  vnet_hw_interface_t *hw = vnet_get_hw_interface (vnm, hw_if_index);
  dpdk_device_t *xd = vec_elt_at_index (xm->devices, hw->dev_instance);

  /* Shut off redirection */
  if (node_index == ~0)
    {
      xd->per_interface_next_index = node_index;
      return;
    }

  xd->per_interface_next_index =
    vlib_node_add_next (xm->vlib_main, dpdk_input_node.index, node_index);
}


static clib_error_t *
dpdk_subif_add_del_function (vnet_main_t * vnm,
			     u32 hw_if_index,
			     struct vnet_sw_interface_t *st, int is_add)
{
  dpdk_main_t *xm = &dpdk_main;
  vnet_hw_interface_t *hw = vnet_get_hw_interface (vnm, hw_if_index);
  dpdk_device_t *xd = vec_elt_at_index (xm->devices, hw->dev_instance);
  vnet_sw_interface_t *t = (vnet_sw_interface_t *) st;
  int r, vlan_offload;
  u32 prev_subifs = xd->num_subifs;
  clib_error_t *err = 0;

  if (is_add)
    xd->num_subifs++;
  else if (xd->num_subifs)
    xd->num_subifs--;

  if ((xd->flags & DPDK_DEVICE_FLAG_PMD) == 0)
    goto done;

  /* currently we program VLANS only for IXGBE VF and I40E VF */
  if ((xd->pmd != VNET_DPDK_PMD_IXGBEVF) && (xd->pmd != VNET_DPDK_PMD_I40EVF))
    goto done;

  if (t->sub.eth.flags.no_tags == 1)
    goto done;

  if ((t->sub.eth.flags.one_tag != 1) || (t->sub.eth.flags.exact_match != 1))
    {
      xd->num_subifs = prev_subifs;
      err = clib_error_return (0, "unsupported VLAN setup");
      goto done;
    }

  vlan_offload = rte_eth_dev_get_vlan_offload (xd->port_id);
  vlan_offload |= ETH_VLAN_FILTER_OFFLOAD;

  if ((r = rte_eth_dev_set_vlan_offload (xd->port_id, vlan_offload)))
    {
      xd->num_subifs = prev_subifs;
      err = clib_error_return (0, "rte_eth_dev_set_vlan_offload[%d]: err %d",
			       xd->port_id, r);
      goto done;
    }


  if ((r =
       rte_eth_dev_vlan_filter (xd->port_id,
				t->sub.eth.outer_vlan_id, is_add)))
    {
      xd->num_subifs = prev_subifs;
      err = clib_error_return (0, "rte_eth_dev_vlan_filter[%d]: err %d",
			       xd->port_id, r);
      goto done;
    }

done:
  if (xd->num_subifs)
    xd->flags |= DPDK_DEVICE_FLAG_HAVE_SUBIF;
  else
    xd->flags &= ~DPDK_DEVICE_FLAG_HAVE_SUBIF;

  return err;
}

/* *INDENT-OFF* */
VNET_DEVICE_CLASS (dpdk_device_class) = {
  .name = "dpdk",
  .tx_function_n_errors = DPDK_TX_FUNC_N_ERROR,
  .tx_function_error_strings = dpdk_tx_func_error_strings,
  .format_device_name = format_dpdk_device_name,
  .format_device = format_dpdk_device,
  .format_tx_trace = format_dpdk_tx_trace,
  .clear_counters = dpdk_clear_hw_interface_counters,
  .admin_up_down_function = dpdk_interface_admin_up_down,
  .subif_add_del_function = dpdk_subif_add_del_function,
  .rx_redirect_to_node = dpdk_set_interface_next_node,
  .mac_addr_change_function = dpdk_set_mac_address,
  .format_flow = format_dpdk_flow,
  .flow_ops_function = dpdk_flow_ops_fn,
};
/* *INDENT-ON* */

#define UP_DOWN_FLAG_EVENT 1

static uword
admin_up_down_process (vlib_main_t * vm,
		       vlib_node_runtime_t * rt, vlib_frame_t * f)
{
  clib_error_t *error = 0;
  uword event_type;
  uword *event_data = 0;
  u32 sw_if_index;
  u32 flags;

  while (1)
    {
      vlib_process_wait_for_event (vm);

      event_type = vlib_process_get_events (vm, &event_data);

      dpdk_main.admin_up_down_in_progress = 1;

      switch (event_type)
	{
	case UP_DOWN_FLAG_EVENT:
	  {
	    if (vec_len (event_data) == 2)
	      {
		sw_if_index = event_data[0];
		flags = event_data[1];
		error =
		  vnet_sw_interface_set_flags (vnet_get_main (), sw_if_index,
					       flags);
		clib_error_report (error);
	      }
	  }
	  break;
	}

      vec_reset_length (event_data);

      dpdk_main.admin_up_down_in_progress = 0;

    }
  return 0;			/* or not */
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE (admin_up_down_process_node) = {
    .function = admin_up_down_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "admin-up-down-process",
    .process_log2_n_stack_bytes = 17,  // 256KB
};
/* *INDENT-ON* */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
