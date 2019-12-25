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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <syslog.h>
#include <errno.h>

#include "sf_shm_main.h"

// // only name and callback_ptr  ; for control plane
// extern sf_shm_map_registration_t sf_shm_map_reg_main_in_lib;
// // name , len , callback_ptr , init function ; for data plane
// extern sf_shm_map_registration_t sf_shm_map_reg_main_outside;

int mudole_count = 0;



uint32_t get_shm_map_count_outside(int re_count)
{
    uint32_t count = 0;
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_outside;
    sf_shm_map_registration_t *reg_ptr;

    if( !(  re_count || mudole_count == 0  ))
    {
        return mudole_count;
    }

    reg_ptr = head->next;
    while(reg_ptr != NULL)
    {
        count ++;
        reg_ptr = reg_ptr->next;
    }

    mudole_count = count;

    return count;
}

int module_shm_total_len = 0;

uint32_t get_shm_map_total_len_outside(int re_count)
{
    uint32_t count = 0;
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_outside;
    sf_shm_map_registration_t *reg_ptr;

    if( !(  re_count || module_shm_total_len == 0  ))
    {
        return module_shm_total_len;
    }

    reg_ptr = head->next;
    while(reg_ptr != NULL)
    {
        count += reg_ptr->len ;
        reg_ptr = reg_ptr->next;
    }

    module_shm_total_len = count;

    return count;
}



/**** save shm config to  file ****/

int save_shm_config_to_file(char *dst_file)
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    uint8_t *shm_data_ptr;
    uint32_t total_len = 0;
    uint32_t block_len = SHM_COPY_BYTES_EACH_TIME;
    uint32_t pre_offset = 0;
    int ret;
    FILE *dst_fp = NULL;
    int total_copy_bytes = 0;

    if(shm_main == NULL)
    {
        return -1;
    }

    if(dst_file == NULL)
    {
        return -1;
    }

    //delete old file , make "save" faster
    remove(dst_file);

    shm_data_ptr = (uint8_t *)shm_main;

    //from start of shm to config mem end 
    total_len = shm_main->config_mem_end + 1;

    // copy directly to file
    openlog("libnshell", LOG_CONS | LOG_PID, 0);   

	//open target file
    dst_fp = fopen( dst_file , "w");
    
	if (dst_fp == NULL) {
		syslog(LOG_USER | LOG_INFO, "Can't open config file '%s': %s\n",
            dst_file,
			strerror(errno));
        
        closelog();
		return -1;
	}


    while(total_len >= block_len)
    {
        ret = fwrite( shm_data_ptr + pre_offset ,  1 , block_len ,  dst_fp );
        total_copy_bytes += ret;
        if(ret <= 0 )
        {
            syslog(LOG_USER | LOG_INFO, "[%d]Can't write config file '%s': %s\n",
                __LINE__,
                dst_file,
                strerror(errno));

            fclose(dst_fp);
            closelog();
            return -1;
        }

        pre_offset +=  block_len;
        total_len -= block_len;
    }

    if(total_len > 0)
    {
        ret = fwrite( shm_data_ptr + pre_offset ,  1 , total_len ,  dst_fp );
        total_copy_bytes += ret;
        if(ret <= 0 )
        {
            syslog(LOG_USER | LOG_INFO, "[%d]Can't write config file '%s': %s\n",
                __LINE__,
                dst_file,
                strerror(errno));

            fclose(dst_fp);
            closelog();
            return -1;
        }
    }

    syslog(LOG_USER | LOG_INFO, "save config file['%s'] total_bytes: %d\n",
                dst_file,
                total_copy_bytes);

	//close file
	fclose(dst_fp);
    closelog();   

    return 0;
}

int save_shm_config_to_default_file()
{
    return save_shm_config_to_file(SHM_DEFAULT_CONFIG_FILE);
}

/**** load shm config from file ****/

static int check_sf_shm_main(sf_shm_main_t *a, sf_shm_main_t *b)
{
    if(strncmp(a->version , b->version , SF_SHM_MEM_VERSION_LEN) != 0 )
    {
        return -1;
    }

    if(a->config_mem_start != b->config_mem_start
        ||  a->config_mem_end != b->config_mem_end
        || a->map_count != b->map_count
    )
    {
        return -1;
    }

    return 0;
}

int load_shm_config_from_file(char *source_file)
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    sf_shm_main_t shm_main_buffer;
    sf_shm_map_t shm_map_buffer;
    uint8_t *shm_data_ptr;
    uint32_t start_offset = 0;
    uint32_t end_offset = 0;
    uint32_t block_len = SHM_COPY_BYTES_EACH_TIME;
    uint32_t pre_offset = 0;
    
    int ret;
    FILE *source_fp = NULL;
    long source_total_len;
    int total_copy_len = 0;

    uint32_t i;


    if(shm_main == NULL)
    {
        return -1;
    }

    if(source_file == NULL)
    {
        return -1;
    }

    shm_data_ptr = (uint8_t *)shm_main;

    //from start of config mem  to end of config mem 
    start_offset = shm_main->config_mem_start;
    end_offset = shm_main->config_mem_end;

    // copy directly to file
    openlog("libnshell", LOG_CONS | LOG_PID, 0);   

	//open target file
    source_fp = fopen( source_file , "r");
    
	if (source_fp == NULL) {
		syslog(LOG_USER | LOG_INFO, "Can't open config file '%s': %s\n",
            source_file,
			strerror(errno));
        closelog();
		return -1;
	}

    fseek(source_fp , 0L , SEEK_END);
    source_total_len = ftell(source_fp);


    //check total len
    if(source_total_len < (end_offset + 1))
    {
        fclose(source_fp);
        closelog();   
        return -2;
    }

    fseek(source_fp , 0L , SEEK_SET);
    ret = fread( &shm_main_buffer ,  1 , sizeof(sf_shm_main_t) , source_fp);
    if(ret <= 0)
    {
        syslog(LOG_USER | LOG_INFO, "[%d]Can't read config file '%s': %s\n",
            __LINE__,
            source_file,
            strerror(errno));

        fclose(source_fp);
        closelog();   
        return -1;
    }

    if(ret < sizeof(sf_shm_main_t))
    {
        fclose(source_fp);
        closelog();
        return -2;
    }

    //check sf_shm_main
    if(check_sf_shm_main(shm_main , &shm_main_buffer ) != 0)
    {
        syslog(LOG_USER | LOG_INFO, "[%d]Wrong config file head '%s'\n",
            __LINE__,
            source_file);

        fclose(source_fp);
        closelog();
        return -2;
    }

    //check mem map
    for(i=0; i < shm_main->map_count ; i++)
    {
        //check config map only
        if(shm_main->shm_maps[i].offset > end_offset)
        {
            break;
        }
        ret = fread(&shm_map_buffer , sizeof(sf_shm_map_t) , 1 , source_fp);
        if(ret <= 0)
        {
            syslog(LOG_USER | LOG_INFO, "[%d]Can't read config file '%s': %s\n",
                __LINE__,
                source_file,
                strerror(errno));

            fclose(source_fp);
            closelog();   
            return -1;
        }

        if(memcmp( &shm_map_buffer , &(shm_main->shm_maps[i]) , sizeof(sf_shm_map_t) ) != 0)
        {
            syslog(LOG_USER | LOG_INFO, "[%d]Wrong config file head map '%s'\n",
            __LINE__,
            source_file);

            fclose(source_fp);
            closelog();
            return -2;
        }
    }



    //start copy config
    fseek(source_fp , (long)(start_offset) , SEEK_SET);
    pre_offset = start_offset;
    while(pre_offset <= end_offset)
    {
        ret = fread(shm_data_ptr + pre_offset , 1 , block_len , source_fp);
        total_copy_len += ret;
        if(ret <= 0)
        {
            syslog(LOG_USER | LOG_INFO, "[%d]Can't read config file '%s': %s\n",
                __LINE__,
                source_file,
                strerror(errno));

            fclose(source_fp);
            closelog();   
            return -1;
        }

        pre_offset += block_len;
    }

    syslog(LOG_USER | LOG_INFO, "load config file['%s'] head len : %d total_copy_len : %d\n",
                source_file ,
                start_offset , 
                total_copy_len );

	//close file
	fclose(source_fp);
    closelog();   

    return 0;
}



int load_shm_config_from_default_file()
{
    return load_shm_config_from_file(SHM_DEFAULT_CONFIG_FILE);
}