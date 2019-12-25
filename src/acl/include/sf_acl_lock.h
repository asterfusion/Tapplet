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
#ifndef __include_sf_acl_sync_h__
#define __include_sf_acl_sync_h__


#include <vnet/vnet.h>
#include <vlib/vlib.h>
#include <vppinfra/lock.h>

extern clib_spinlock_t *acl_sync_locks;

#define sf_acl_sync_lock(worker_index) \
clib_spinlock_lock(acl_sync_locks + worker_index)


#define sf_acl_sync_unlock(worker_index) \
clib_spinlock_unlock(acl_sync_locks + worker_index)



#endif