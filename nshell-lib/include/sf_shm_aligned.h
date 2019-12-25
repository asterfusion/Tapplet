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
#ifndef __inlucde_sf_shm_aligned_h__
#define __inlucde_sf_shm_aligned_h__

#include <vppinfra/cache.h>


#ifndef SF_ALIGNED_4
#define SF_ALIGNED __attribute__((aligned(8)))
#else
#define SF_ALIGNED __attribute__((aligned(4)))
#endif


#define SF_ALIGNED_CACHE  __attribute__((aligned (CLIB_CACHE_LINE_BYTES)))




#endif