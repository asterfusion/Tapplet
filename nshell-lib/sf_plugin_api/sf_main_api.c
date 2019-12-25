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
#include <stdio.h>


#include "sf_shm_main.h"


#include "sf_main_api.h"

SF_STAT_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_STAT


uint8_t get_sf_main_config_sf_vpp_status()
{
    if(sf_main_stat_ptr == NULL)
        return (uint8_t)-1;
    return sf_main_stat_ptr->sf_vpp_status;
}
