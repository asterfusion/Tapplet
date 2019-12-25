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




int main()
{
    int ret ;
    ret = get_and_link_all_shm();
    if(ret != 0)
    {
        perror("get shm:");
        return -1;
    }
    ret = unlink_all_shm();
    if(ret != 0)
    {
        perror("unlink shm:");
        return -1;
    }
    ret = delete_all_shm();
    if(ret != 0)
    {
        perror("delete shm:");
        printf("try 'ipcrm -M 0x%x' instead.\n" , main_shm_mgr.shm_key);
        return -1;
    }

    return 0;
}