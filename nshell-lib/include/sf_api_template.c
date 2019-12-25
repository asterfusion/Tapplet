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
/**** notice : this file cannot be compiled directly ****/
/**** useage : #include "sf_api_template.c" ****/

SF_CONF_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_CONF

SF_STAT_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_STAT



#undef FUNC
#define FUNC(type , name , default_value , min , max) GET_SET_API_FUNC_CONF_DEFINE(type , name , min , max)
    foreach_member
#undef FUNC


RESET_CONFIG_FUNC_DEFINITION
GET_STAT_PTR_FUNC_DEFINE
CLEAN_STAT_FUNC_DEFINE

INIT_CONFIG_FUNC_DEFINITION()
{
    if(SF_CONF_PTR_VPP == NULL)
    {
        return -1;
    }

    sf_memset_zero(SF_CONF_PTR_VPP , sizeof(SF_CONF_T));

    #undef FUNC
    #undef __func
    #undef _func

    #define __func(ptr , member , default_value) ptr->member = default_value;
    #define _func(ptr , member , default_value)  __func(ptr , member , default_value)
    #define FUNC(type , name , default_value , min , max) _func(SF_CONF_PTR_VPP , name , default_value)
        foreach_member
    
    #undef FUNC
    #undef __func
    #undef _func

    return 0;
}