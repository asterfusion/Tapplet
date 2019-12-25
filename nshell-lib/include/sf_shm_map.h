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
#ifndef __include_sf_shm_map_h__
#define __include_sf_shm_map_h__

#include <stdint.h>

#include "sf_shm_aligned.h"

#define SF_SHM_MAP_NAME_LEN 32

#define PADDING_AFTER_SHM_MAPS (sizeof(sf_shm_map_t))

typedef struct
{
    char name[SF_SHM_MAP_NAME_LEN];
    uint32_t offset;
    uint32_t len;
} SF_ALIGNED sf_shm_map_t;

typedef int (*shm_config_init_t)(void *ptr , int is_reset);

typedef enum {
    SF_SHM_MEM_TYPE_CONFIG = 0,
    SF_SHM_MEM_TYPE_STAT = 1 , 
    SF_SHM_MEM_TYPE_COUNT,
} sf_shm_mem_type_t;

typedef struct _sf_shm_map_registration_t
{
    char name[SF_SHM_MAP_NAME_LEN];
    sf_shm_mem_type_t mem_type;
    uint32_t len; // 0  : only callback_ptr needed to be set
    void **callback_ptr;
    shm_config_init_t init_func;

    struct _sf_shm_map_registration_t *next;
} sf_shm_map_registration_t;

#define SHM_MAP_REGISTION_IN_LIB(x)                                    \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x;              \
    static void __sf_shm_map_registration_##x(void)                    \
        __attribute__((__constructor__));                              \
    static void __sf_shm_map_registration_##x(void)                    \
    {                                                                  \
        sf_shm_map_registration_t *head = &sf_shm_map_reg_main_in_lib; \
        _sf_shm_mag_reg_##x.next = head->next;                         \
        head->next = &_sf_shm_mag_reg_##x;                             \
    }                                                                  \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x

#define __SHM_MAP_REGISTION_IN_LIB(x, type, init_function, ptr)        \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x;              \
    static void __sf_shm_map_registration_##x(void)                    \
        __attribute__((__constructor__));                              \
    static void __sf_shm_map_registration_##x(void)                    \
    {                                                                  \
        sf_shm_map_registration_t *head = &sf_shm_map_reg_main_in_lib; \
        _sf_shm_mag_reg_##x.next = head->next;                         \
        head->next = &_sf_shm_mag_reg_##x;                             \
    }                                                                  \
    static sf_shm_map_registration_t _sf_shm_mag_reg_##x = {           \
        .name = #x,                                                    \
        .callback_ptr = (void **)(&ptr),                               \
        .init_func = init_function,                                    \
    };

//config
#define __SHM_MAP_REGISTION_IN_LIB_CONF(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_LIB(name##_config, type, init_func, callback_ptr)

#define _SHM_MAP_REGISTION_IN_LIB_CONF(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_LIB_CONF(name, type, init_func, callback_ptr)

//stat
#define __SHM_MAP_REGISTION_IN_LIB_STAT(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_LIB(name##_stat, type, init_func, callback_ptr)

#define _SHM_MAP_REGISTION_IN_LIB_STAT(name, type, init_func, callback_ptr) \
    __SHM_MAP_REGISTION_IN_LIB_STAT(name, type, init_func, callback_ptr)

#define SHM_MAP_REGISTION_IN_LIB_CONF \
    _SHM_MAP_REGISTION_IN_LIB_CONF(MOD_NAME, SF_CONF_T, INIT_CONFIG_FUNC_NAME, SF_CONF_PTR)

#define SHM_MAP_REGISTION_IN_LIB_STAT \
    _SHM_MAP_REGISTION_IN_LIB_STAT(MOD_NAME, SF_STAT_T, NULL, SF_STAT_PTR)

extern sf_shm_map_registration_t sf_shm_map_reg_main_in_lib;
extern sf_shm_map_registration_t sf_shm_map_reg_main_outside;

//for control-plane
//Notice : if shm init fail , return -1
int get_shm_map_status();
//control-plane
int update_shm_map_map_inlib();
//for data-plane
int update_shm_map_map_outside();
//for data plane ; usually , update_shm_map_map = 1
int start_shm_map_malloc(int update_map_in_lib);

//utils
// sf_shm_main_t *get_sf_shm_main_ptr();
uint32_t get_shm_map_count_outside(int re_count);
uint32_t get_shm_map_total_len_outside(int re_count);


//save & load config file
#define SHM_COPY_BYTES_EACH_TIME 2048
#define SHM_DEFAULT_CONFIG_FILE "/opt/sfconfig/shm_config.bin"
int save_shm_config_to_file(char *dst_file);
int load_shm_config_from_file(char *source_file);
int save_shm_config_to_default_file();
int load_shm_config_from_default_file();

#endif

// !!!!!  don't delete code below
// static void __vlib_rm_node_registration_##x (void)
//     __attribute__((__destructor__)) ;
// static void __vlib_rm_node_registration_##x (void)
// {
//     vlib_main_t * vm = vlib_get_main();
//     VLIB_REMOVE_FROM_LINKED_LIST (vm->node_main.node_registrations,
//                                   &x, next_registration);
// }
