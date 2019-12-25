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
#ifndef __include_sf_shm_main_h__
#define __include_sf_shm_main_h__

#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>


#include "sf_shm_map.h"

#include "sf_shm_aligned.h"

// #define SHM_KEY 0x2233
// #define SHM_MEM_LEN 1024
// #define MAP_MAX_COUNT 100

// man ftok : proj_id "still only 8 bits are used"
#define MAIN_SHM_KEY (ftok("/usr/lib/vpp_plugins/libnshell.so" , 0x2233))
#define MAIN_SHM_LEN (sizeof(sf_shm_main_t) +  \
sizeof(sf_shm_map_t) * get_shm_map_count_outside(0) + \
PADDING_AFTER_SHM_MAPS +  get_shm_map_total_len_outside(0))

// #define MODULE_SHM_KEY (ftok("/usr/lib/vpp_plugins/libnshell.so" , 0x3322))
// #define MODULE_SHM_LEN (get_shm_map_total_len_outside(0))


// extern uint8_t *shm_start_ptr;

#if  !defined(SF_VERSION)
#error SF_VERSION Undefined
#endif //!defined(SF_VERSION)

#if  !defined(TARGET_DEVICE)
#error TARGET_DEVICE Undefined
#endif //!defined(TARGET_DEVICE)

#define _get_macro_str(raw) #raw
#define get_macro_str(macro)  _get_macro_str(macro)
#define SF_BUILD_VERSION_STR get_macro_str(SF_VERSION)

#define SF_TARGET_DEVICE_STR get_macro_str(TARGET_DEVICE)


#define SF_SHM_MEM_VERSION_LEN 32
#define SF_SHM_MEM_VERSION_MIN_LEN 11

#define SF_SHM_MEM_DEVICE_TYPE_LEN 31

typedef struct {
    char version[SF_SHM_MEM_VERSION_LEN];
    char device_type_name[SF_SHM_MEM_DEVICE_TYPE_LEN];
    uint8_t sync_lock;
#define MAP_NOT_READY 0
#define MAP_MEM_FINISH 1
// #define MAP_IN_LIB_FINISH 2
#define SAVED_CONFIG_LOAD_FINISH 2

    int32_t status;


    int32_t used_mem;
    int32_t free_mem;


    uint32_t config_mem_start;
    uint32_t config_mem_end;
    uint32_t map_count;
    sf_shm_map_t shm_maps[0];
}SF_ALIGNED sf_shm_main_t ;


typedef struct __sf_shm_manager {
    uint8_t *start_ptr;
    int len;
    int shmid;
    key_t shm_key;
}SF_ALIGNED sf_shm_mgr_t ;

extern sf_shm_mgr_t main_shm_mgr;
// extern sf_shm_mgr_t module_shm_mgr;

#define GET_OR_CREATE_MEM_FAIL  -2
#define LINK_MEM_FAIL -1

#define get_sf_shm_main_ptr() ((sf_shm_main_t *)main_shm_mgr.start_ptr)

//for data-plane
int create_and_link_all_shm();
//for control-plane
int get_and_link_all_shm();

int unlink_all_shm();
int delete_all_shm();
//for data plane
int init_shm_main();
//check if the old shm size is smaller than we need
int check_shm_main();

#endif