#ifndef __include_sf_pkt_tools_h__
#define __include_sf_pkt_tools_h__

#include <vlib/vlib.h>
#include <stdint.h>


always_inline u32
sf_vlib_alloc_no_fail (vlib_main_t * vm, u32 * buffers, u32 n_buffers)
{
  vlib_buffer_main_t *bm = &buffer_main;
  vlib_buffer_free_list_t *fl;
  u32 *src;
  uword len;

  ASSERT (bm->cb.vlib_buffer_fill_free_list_cb);

  fl = pool_elt_at_index (vm->buffer_free_list_pool, VLIB_BUFFER_DEFAULT_FREE_LIST_INDEX);

  len = vec_len (fl->buffers);

  if (PREDICT_FALSE (len < n_buffers))
    {
      bm->cb.vlib_buffer_fill_free_list_cb (vm, fl, n_buffers);

      //!!! Notice !!!!
      if (PREDICT_FALSE ( (len = vec_len (fl->buffers)) < n_buffers))
	        return 0;

      /* following code is intentionaly duplicated to allow compiler
         to optimize fast path when n_buffers is constant value */

      src = fl->buffers + len - n_buffers;
      clib_memcpy (buffers, src, n_buffers * sizeof (u32));
      _vec_len (fl->buffers) -= n_buffers;

      /* Verify that buffers are known free. */
      vlib_buffer_validate_alloc_free (vm, buffers, n_buffers,
				       VLIB_BUFFER_KNOWN_FREE);

      return n_buffers;
    }

  src = fl->buffers + len - n_buffers;
  clib_memcpy (buffers, src, n_buffers * sizeof (u32));
  _vec_len (fl->buffers) -= n_buffers;

  /* Verify that buffers are known free. */
  vlib_buffer_validate_alloc_free (vm, buffers, n_buffers,
				   VLIB_BUFFER_KNOWN_FREE);

  return n_buffers;
}

static_always_inline vlib_buffer_t *
alloc_one_pkt_from_vpp(vlib_main_t *vm )
{
    uint32_t index = 0;
    uint32_t ret = vlib_buffer_alloc(vm , &index  , 1);
    if(PREDICT_FALSE(ret != 1))
    {
        return NULL;
    }

    return vlib_get_buffer(vm , index);
}

/**
 * Notice  : !!! make sure the index_list is uint32_t ** !!!
 * @return uint32_t : alloced cnt , maybe 0 or smaller than asked
 */
static_always_inline uint32_t
alloc_n_pkt_from_vpp(vlib_main_t *vm , uint32_t *index_list , uint32_t asked_cnt )
{
    if(PREDICT_FALSE(asked_cnt == 0))
    {
        return 0;
    }

    return vlib_buffer_alloc(vm , index_list , asked_cnt);
}


static_always_inline void 
sf_push_pkt_to_next(vlib_main_t *vm , vlib_node_runtime_t *node , 
            u32 next_index , u32 *from , u32 n_left_from )
{
     u32 n_left_to_next;
    u32 *to_next;
  while(n_left_from > 0)
  {
    u32 n_copy = 0;
    vlib_get_next_frame(vm, node, next_index,
                        to_next, n_left_to_next);

    n_copy = n_left_to_next < n_left_from ? n_left_to_next : n_left_from;

    memcpy(to_next , from , n_copy * sizeof(u32));

    n_left_from -= n_copy;
    n_left_to_next -= n_copy;
    
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }
  
}


static_always_inline void 
sf_push_pkt_to_next_u64(vlib_main_t *vm , vlib_node_runtime_t *node , 
            u32 next_index , u64 *from , u32 n_left_from )
{
    u32 n_left_to_next;
    u64 *to_next;
  while(n_left_from > 0)
  {
    u32 n_copy = 0;
    vlib_get_next_frame(vm, node, next_index,
                        to_next, n_left_to_next);

    n_copy = n_left_to_next < n_left_from ? n_left_to_next : n_left_from;

    memcpy(to_next , from , n_copy * sizeof(u64));

    n_left_from -= n_copy;
    n_left_to_next -= n_copy;
    
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }
  
}

static_always_inline void
sf_vlib_buffer_free_no_next (vlib_main_t * vm,
		  /* pointer to first buffer */
		  u32 * buffers,
		  /* number of buffers to free */
		  u32 n_buffers)
{
  vlib_buffer_main_t *bm = &buffer_main;

  return bm->cb.vlib_buffer_free_no_next_cb (vm, buffers, n_buffers);
}


#endif