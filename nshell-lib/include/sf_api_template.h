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
#ifndef __include_sf_api_template_h__
#define __include_sf_api_template_h__

#undef FUNC
#define FUNC(a, b , c, d , e) GET_SET_API_DECLARE_CONF(a, b)
foreach_member
#undef FUNC

// GET_SET_API_DECLARE_CONF(FUNC(uint32_t , test)
RESET_CONFIG_FUNC_DECLARE
GET_STAT_PTR_FUNC_DECLARE
CLEAN_STAT_FUNC_DECLARE


#include "sf_string.h"

#endif