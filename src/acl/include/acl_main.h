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
#ifndef __include_acl_main_h__
#define __include_acl_main_h__

#include "acl_rte_def.h"

#include "acl_defines.h"



typedef struct{
    struct rte_acl_ctx *v46_context[MAX_RULE_GROUP_CNT] ;
}sf_acl_main_t;

extern sf_acl_main_t sf_acl_main;

#endif