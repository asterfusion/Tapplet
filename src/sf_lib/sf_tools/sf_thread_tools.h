#ifndef __include_sf_thread_tools_h__
#define __include_sf_thread_tools_h__



#include <vlib/threads.h>

#include "sf_thread.h"


#define sf_get_current_thread_index() vlib_get_thread_index()

#define sf_get_current_worker_index() vlib_get_current_worker_index()

#define sf_vlib_num_workers()  vlib_num_workers()


#endif