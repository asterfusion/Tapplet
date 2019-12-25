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
#ifndef __include_sf_timer_enqueue_h__
#define __include_sf_timer_enqueue_h__

#include <vlib/vlib.h>

#include <stdint.h>

always_inline void
sf_u64_set_next_frame_buffer (vlib_main_t * vm,
			    vlib_node_runtime_t * node,
			    u32 next_index, uint64_t buffer_index)
{
  uint64_t *p;
  p = vlib_set_next_frame (vm, node, next_index, p);
  p[0] = buffer_index;
}

#define sf_u64_validate_buffer_enqueue_x2(vm,node,next_index,to_next,n_left_to_next,bi0,bi1,next0,next1) \
do {									\
  int enqueue_code = (next0 != next_index) + 2*(next1 != next_index);	\
									\
  if (PREDICT_FALSE (enqueue_code != 0))				\
    {									\
      switch (enqueue_code)						\
	{								\
	case 1:								\
	  /* A B A */							\
	  to_next[-2] = bi1;						\
	  to_next -= 1;							\
	  n_left_to_next += 1;						\
	  sf_u64_set_next_frame_buffer (vm, node, next0, bi0);		\
	  break;							\
									\
	case 2:								\
	  /* A A B */							\
	  to_next -= 1;							\
	  n_left_to_next += 1;						\
	  sf_u64_set_next_frame_buffer (vm, node, next1, bi1);		\
	  break;							\
									\
	case 3:								\
	  /* A B B or A B C */						\
	  to_next -= 2;							\
	  n_left_to_next += 2;						\
	  sf_u64_set_next_frame_buffer (vm, node, next0, bi0);		\
	  sf_u64_set_next_frame_buffer (vm, node, next1, bi1);		\
	  if (next0 == next1)						\
	    {								\
	      vlib_put_next_frame (vm, node, next_index,		\
				   n_left_to_next);			\
	      next_index = next1;					\
	      vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next); \
	    }								\
	}								\
    }									\
} while (0)







#endif