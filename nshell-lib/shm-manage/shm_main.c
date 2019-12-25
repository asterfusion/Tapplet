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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>


#include "sf_shm_main.h"

sf_shm_mgr_t main_shm_mgr;
// sf_shm_mgr_t module_shm_mgr;

int link_shm(sf_shm_mgr_t *shm_mgr)
{
    void *link_ptr;
    link_ptr =   shmat(shm_mgr->shmid, NULL, 0);
    if(link_ptr == (void *) -1 )
    {
        perror("link shm");
        return LINK_MEM_FAIL;
    }

    shm_mgr->start_ptr = (uint8_t *)link_ptr;

    return 0;
}

int get_and_link_shm_inline(sf_shm_mgr_t *shm_mgr , key_t key , uint32_t len , int is_create)
{
    int shmid ;
    if (is_create)
    {
        shmid = shmget(key , len  , IPC_CREAT | IPC_EXCL | 0666); 
    }
    else
    {
        shmid = shmget(key , 0  , 0); 
    }
    
    shm_mgr->shm_key = key ;
    shm_mgr->len = len ;

    if(shmid < 0 )
    {
        // perror("get main shm");
        return GET_OR_CREATE_MEM_FAIL;
    }

    shm_mgr->shmid = shmid;

    return link_shm(shm_mgr);
}

int init_all_shm_inline(int is_create)
{
    int ret ;
    memset( &main_shm_mgr  , 0  , sizeof(sf_shm_mgr_t)  );
    // memset( &module_shm_mgr  , 0  , sizeof(sf_shm_mgr_t)  );

    ret = get_and_link_shm_inline(&main_shm_mgr , MAIN_SHM_KEY , MAIN_SHM_LEN , is_create );
    if(ret != 0)
    {
        return ret;
    }

    // ret = get_and_link_shm_inline(&module_shm_mgr , MODULE_SHM_KEY , MODULE_SHM_LEN , is_create);
    // if(ret != 0)
    // {
    //     return ret;
    // }

    return 0;
}


//for data-plane
int create_and_link_all_shm()
{
    return init_all_shm_inline(1);
}

//for control-plane
int get_and_link_all_shm()
{
    return init_all_shm_inline(0);
}



int unlink_shm(sf_shm_mgr_t *shm_mgr)
{
    int ret = shmdt(shm_mgr->start_ptr);
    if(ret < 0 )
    {
        return -1;
    }
    return 0;
}
int unlink_all_shm()
{
    int ret ;
    ret = unlink_shm( &main_shm_mgr );
    if(ret != 0)
    {
        return ret;
    }

    // ret = unlink_shm( &module_shm_mgr );
    // if(ret != 0)
    // {
    //     return ret;
    // }
    return 0;
}


int delete_shm(sf_shm_mgr_t *shm_mgr)
{
    int ret = shmctl(shm_mgr->shmid, IPC_RMID, NULL);
    if(ret <0 )
    {
        return -1;
    }

    return 0;
}

int delete_all_shm()
{
    int ret ;
    ret = delete_shm( &main_shm_mgr );
    if(ret != 0)
    {
        return ret;
    }

    // ret = delete_shm( &module_shm_mgr );
    // if(ret != 0)
    // {
    //     return ret;
    // }
    return 0;
}

/******************************/
//for data plane
int init_shm_main()
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();

    //check if shm init over ?
    if(shm_main == NULL)
    {
        return -1;
    }

    shm_main->status = MAP_NOT_READY;
    shm_main->used_mem = sizeof(sf_shm_main_t);
    shm_main->free_mem = MAIN_SHM_LEN - sizeof(sf_shm_main_t);
    shm_main->map_count = 0;

    return 0;
}

//check if the old shm size is smaller than we need
int check_shm_main()
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    if(MAIN_SHM_LEN != (shm_main->used_mem + shm_main->free_mem) )
    {
        return MAIN_SHM_LEN - (shm_main->used_mem + shm_main->free_mem);
    }

    return 0;
}