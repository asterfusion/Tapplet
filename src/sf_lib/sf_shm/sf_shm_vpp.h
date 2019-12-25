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
#ifndef __include__sf_shm_reg_h__
#define __include__sf_shm_reg_h__

#include "sf_shm_main.h"

#include "sf_api_macro.h"

#define __SHM_MAP_REGISTION_IN_VPP(x, type, init_function, ptr, shm_mem_type) \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x;                 \
    static void __sf_shm_map_registration_##x(void)                       \
        __attribute__((__constructor__));                                 \
    static void __sf_shm_map_registration_##x(void)                       \
    {                                                                     \
        sf_shm_map_registration_t *head = &(sf_shm_map_reg_main_outside); \
        _sf_shm_mag_reg_##x.next = head->next;                            \
        head->next = &_sf_shm_mag_reg_##x;                                \
    }                                                                     \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x = {              \
        .name = #x,                                                       \
        .mem_type = shm_mem_type,                                         \
        .len = sizeof(type),                                              \
        .callback_ptr = (void **)(&ptr),                                  \
        .init_func = init_function,                                       \
    };

//config
#define __SHM_MAP_REGISTION_IN_VPP_CONF(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_VPP(name##_config, type, init_func, callback_ptr,  SF_SHM_MEM_TYPE_CONFIG)

#define _SHM_MAP_REGISTION_IN_VPP_CONF(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_VPP_CONF(name, type, init_func, callback_ptr)

//stat
#define __SHM_MAP_REGISTION_IN_VPP_STAT(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_VPP(name##_stat, type, init_func, callback_ptr,  SF_SHM_MEM_TYPE_STAT)

#define _SHM_MAP_REGISTION_IN_VPP_STAT(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_VPP_STAT(name, type, init_func, callback_ptr)

#define SHM_MAP_REGISTION_IN_VPP_CONF \
    _SHM_MAP_REGISTION_IN_VPP_CONF(MOD_NAME, SF_CONF_T, INIT_CONFIG_FUNC_NAME, SF_CONF_PTR_VPP)

#define SHM_MAP_REGISTION_IN_VPP_STAT \
    _SHM_MAP_REGISTION_IN_VPP_STAT(MOD_NAME, SF_STAT_T, NULL, SF_STAT_PTR_VPP)

int upload_all_shm_reg();
int update_all_callback_ptr();

#endif
