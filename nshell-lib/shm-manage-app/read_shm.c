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

void print_single_map(sf_shm_map_t *shm_map)
{
    printf("name : %s\n" , shm_map->name);
    printf("range: %d %d\n" , shm_map->offset , shm_map->offset + shm_map->len - 1 );
}

void print_all_map()
{
    sf_shm_main_t *shm_main;
    sf_shm_map_t *shm_map;
    uint32_t map_count;
    uint32_t i;
    shm_main = (sf_shm_main_t *)main_shm_mgr.start_ptr;
    map_count  = shm_main->map_count;

    shm_map = shm_main->shm_maps;
printf("locked : %s\n" , shm_main->sync_lock ? "yes" : "no"  );
printf("used_mem : %d\n" , shm_main->used_mem  );
printf("free_mem : %d\n" , shm_main->free_mem);
printf("total len : %d\n" , shm_main->used_mem + shm_main->free_mem);
printf("config_mem_start : %d\n" , shm_main->config_mem_start);
printf("config_mem_end : %d\n" , shm_main->config_mem_end);
printf("-----------------------------\n");
    for(i=0;i<map_count ; i++)
    {
        print_single_map(shm_map);
        shm_map++;
    }
printf("-----------------------------\n");

}


int main()
{
    int ret ;
    ret = get_and_link_all_shm();
    if(ret != 0)
    {
        perror("get shm:");
        return -1;
    }


    print_all_map();



    ret = unlink_all_shm();
    if(ret != 0)
    {
        perror("unlink shm:");
        return -1;
    }

    return 0;
}