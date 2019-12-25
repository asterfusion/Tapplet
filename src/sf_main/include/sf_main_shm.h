/* struct to class */
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
/** 1 **/
#ifndef __include_sf_main_shm_h__
#define __include_sf_main_shm_h__


#include <stdint.h>

#include "sf_shm_aligned.h"
#include "sf_api_macro.h"

#undef MOD_NAME

/** 2 **/
#define MOD_NAME sf_main


/** 3 **/
typedef struct
{
    uint8_t sf_vpp_status;
}SF_ALIGNED SF_STAT_T ;


#ifndef IN_NSHELL_LIB
SF_STAT_PTR_DECLARE_VPP
#endif


#endif
