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

#include "ip_reass_api.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>


#include "sf_string.h"

SF_CONF_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_CONF

SF_STAT_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_STAT


RESET_CONFIG_FUNC_DEFINITION

INIT_CONFIG_FUNC_DEFINITION()
{
    int i;
    if (SF_CONF_PTR_VPP == NULL)
    {
        return -1;
    }

    sf_memset_zero(SF_CONF_PTR_VPP, sizeof(SF_CONF_T));
    SF_CONF_PTR_VPP->hashsize_ip_reass = IP_REASS_DEFAULT_HASH_SIZE;
    SF_CONF_PTR_VPP->maxnum_ip_reass = IP_REASS_DEFAULT_HASH_NODE_NUM;
    SF_CONF_PTR_VPP->pool_node_init_size = IP_REASS_NODE_INIT_SIZE;
    SF_CONF_PTR_VPP->timeout_sec = IP_REASS_DEFAULT_TIMEOUT_SEC;
    // uint8_t ip_reass_layers[MAX_INTERFACE_CNT];
    // uint8_t ip_reass_output_enable[MAX_INTERFACE_CNT];

    for( i=0 ; i< MAX_INTERFACE_CNT ; i++)
    {
        SF_CONF_PTR_VPP->ip_reass_layers[i] = IP_REASS_DEFAULT_MAX_LAYERS;
        SF_CONF_PTR_VPP->ip_reass_output_enable[i] = IP_REASS_DEFAULT_OUTPUT_DISABLE;
    }


    return 0;
}



int ip_reass_set_max_layer(int interface_id , int layers)
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    SF_CONF_PTR->ip_reass_layers[interface_id - 1] = layers & 0xFF;

    return 0;
}

int ip_reass_get_max_layer(int interface_id )
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    return SF_CONF_PTR->ip_reass_layers[interface_id - 1];
}



int ip_reass_set_reass_output(int interface_id , int enable)
{
    uint8_t enable_disable;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if(enable)
    {
        enable_disable = 1;
    }
    else
    {
        enable_disable = 0;
    }
    
    
    SF_CONF_PTR->ip_reass_output_enable[interface_id - 1] = enable_disable;
    return 0;
}

int ip_reass_get_reass_output(int interface_id )
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    return SF_CONF_PTR->ip_reass_output_enable[interface_id - 1];
}