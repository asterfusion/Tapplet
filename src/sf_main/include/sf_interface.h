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
#ifndef __include_sf_interface_h__
#define __include_sf_interface_h__

#include <stdint.h>

#include "sf_shm_aligned.h"


#include "acl_defines.h"

#include "common_header.h"

#include "sf_intf_define.h"


#ifndef MAX_ACTION_COUNT  
#define MAX_ACTION_COUNT 128 
#endif
/* interface_type */
#define SF_INTF_TYPE_RX_TX 0
#define SF_INTF_TYPE_LOOPBACK 1
#define SF_INTF_TYPE_FORCE_TX 2
#define SF_INTF_TYPE_RX_TX_CHECK_SERVICE 3
#define SF_INTF_TYPE_RX_TX_TUNNEL 4



typedef uint8_t sf_action_index_t;

typedef struct{
    union{
        uint8_t cacheline_bytes[64]; // 
        struct{
            uint8_t acl_rule_group;
            sf_action_index_t default_action_id; 
        };
    };
    sf_action_index_t rule_to_action[MAX_RULE_PER_GROUP]; 
}sf_ingress_config_t;

#define INTERFACE_ALIAS_MAX_LEN 20

typedef struct{
    union{
        uint8_t cacheline_bytes[64];
        struct{
            uint8_t port_id; 
            uint8_t interface_type; 
            uint8_t mac[6];
            uint8_t ipaddr_mask;
            uint8_t ipv4_addr[4];
            ip_addr_t ipv6_addr;
            uint8_t ipv4_gateway[4];
            ip_addr_t ipv6_gateway;
            char alias_name[INTERFACE_ALIAS_MAX_LEN + 1];
        };
    }; // 64bytes


    sf_ingress_config_t ingress_config; // 64bytes + MAX_RULE_PER_GROUP + 4096/8

}SF_ALIGNED_CACHE sf_inferface_t;



/**********************************/

#endif
