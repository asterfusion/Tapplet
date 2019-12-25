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
#ifndef __include_action_api_h__
#define __include_action_api_h__

#include <stdint.h>

#undef IN_NSHELL_LIB
#define IN_NSHELL_LIB

#include "sf_gcd.h"
#include "action.h"
#include "action_path_defines.h"
#include "action_shm.h"

#include "sf_string.h"
 

#define is_valid_action_id(id) (((id) >= 1) && ((id) <= MAX_ACTION_NUM))

#define swap(a,b,type)   \
do                  \
{                   \
    type tmp;       \
    tmp = a;        \
    a = b;          \
    b = tmp;        \
}                   \
while(0);

RESET_CONFIG_FUNC_DECLARE




#endif

