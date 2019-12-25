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
#ifndef  __ACTIONS_SHM_H__          
#define  __ACTIONS_SHM_H__


#include "sf_shm_aligned.h"
#include "sf_api_macro.h"

#include "action_define.h"
#include "action.h"

#undef  MOD_NAME
#define MOD_NAME action


#define MAX_ACTION_NUM         MAX_ACTION_COUNT

typedef struct
{
    sf_action_t                    actions[MAX_ACTION_NUM];

} SF_ALIGNED SF_CONF_T; 

#ifndef IN_NSHELL_LIB
SF_CONF_PTR_DECLARE_VPP
#endif

INIT_CONFIG_FUNC_DECLARE
#endif

