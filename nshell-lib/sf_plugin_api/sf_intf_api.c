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

#include "sf_intf_api.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "sf_string.h"

SF_CONF_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_CONF

SF_STAT_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_STAT                                               

// GET_STAT_PTR_FUNC_DEFINE
// CLEAN_STAT_FUNC_DEFINE

#define GET_INTERFACE_FUNC_DEFINE(type, member_name)             \
    int get_interface_##member_name(int index)                   \
    {                                                            \
        if (SF_CONF_PTR == NULL)                                 \
        {                                                        \
            return -1;                                           \
        }                                                        \
        if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)        \
        {                                                        \
            return -1;                                           \
        }                                                        \
        return SF_CONF_PTR->interfaces[index - 1].member_name; \
    }

#define SET_INTERFACE_FUNC_DEFINE(type, member_name)              \
    int set_interface_##member_name(int index, type value)        \
    {                                                             \
        if (SF_CONF_PTR == NULL)                                  \
        {                                                         \
            return -1;                                            \
        }                                                         \
        if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)         \
        {                                                         \
            return -1;                                            \
        }                                                         \
        SF_CONF_PTR->interfaces[index - 1].member_name = value; \
        return 0;                                                 \
    }

#define FUNC(a, b, c, d, e) GET_INTERFACE_FUNC_DEFINE(a, b)
foreach_member
#undef FUNC

#define FUNC(a, b, c, d, e) SET_INTERFACE_FUNC_DEFINE(a, b)
foreach_member
#undef FUNC

/*************** admin status ****************/
int get_interface_admin_status(int index)                   
{                                                           
    if (SF_CONF_PTR == NULL)                                
    {                                                       
        return -1;                                          
    }                                                       
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)       
    {                                                       
        return -1;                                          
    }                                                       
    return SF_CONF_PTR->admin_status[index - 1] ; 
}

int set_interface_admin_status(int index, int value)        
{                                                             
    if (SF_CONF_PTR == NULL)                                  
    {                                                         
        return -1;                                            
    }                                                         
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)         
    {                                                         
        return -1;                                            
    }                                                         
    SF_CONF_PTR->admin_status[index - 1]  = value ; 
    return 0;                                                 
}

/****** ingress *******/
#define GET_INGRESS_FUNC_DEFINE(type, member_name)             \
    int get_ingress_##member_name(int index)                   \
    {                                                            \
        if (SF_CONF_PTR == NULL)                                 \
        {                                                        \
            return -1;                                           \
        }                                                        \
        if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)        \
        {                                                        \
            return -1;                                           \
        }                                                        \
        return SF_CONF_PTR->interfaces[index - 1].ingress_config.member_name; \
    }

#define SET_INGRESS_FUNC_DEFINE(type, member_name)              \
    int set_ingress_##member_name(int index, type value)        \
    {                                                             \
        if (SF_CONF_PTR == NULL)                                  \
        {                                                         \
            return -1;                                            \
        }                                                         \
        if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)         \
        {                                                         \
            return -1;                                            \
        }                                                         \
        SF_CONF_PTR->interfaces[index - 1].ingress_config.member_name = value; \
        return 0;                                                 \
    }

#define FUNC(a, b, c, d, e) GET_INGRESS_FUNC_DEFINE(a, b)
foreach_member_ingress
#undef FUNC

#define FUNC(a, b, c, d, e) SET_INGRESS_FUNC_DEFINE(a, b)
foreach_member_ingress
#undef FUNC




/** alias name **/
int  get_interface_alias_name(int index, char *alias , int max_str_len)
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }
    if(alias == NULL || max_str_len < INTERFACE_ALIAS_MAX_LEN)
    {
        return -1;
    }

    char *alias_name = SF_CONF_PTR->interfaces[index - 1 ].alias_name;

    strncpy(alias , alias_name , INTERFACE_ALIAS_MAX_LEN);


    return 0;

}
int set_interface_alias_name(int index, char *alias )
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }
    if(alias == NULL)
    {
        return -1;
    }

    char *alias_name = SF_CONF_PTR->interfaces[index - 1 ].alias_name;

    strncpy(alias_name , alias , INTERFACE_ALIAS_MAX_LEN);

    return 0;
}


/** mac addr **/
int  get_interface_mac_addr(int index, char *mac_str , int max_str_len)
{
    uint8_t *mac_array;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(mac_str == NULL || max_str_len < 20)
    {
        return -1;
    }

    memset(mac_str , 0 ,max_str_len );

    mac_array = SF_CONF_PTR->interfaces[index - 1].mac;

    sprintf(mac_str , "%02x:%02x:%02x:%02x:%02x:%02x" , 
            mac_array[0] ,  mac_array[1] , mac_array[2] , 
            mac_array[3] ,  mac_array[4] , mac_array[5] );

    return 0;
}

int set_interface_mac_addr(int index, char *mac_str)
{
    uint8_t *mac_array;
    uint32_t temp[6];
    int ret;
    int i;

    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    mac_array = SF_CONF_PTR->interfaces[index - 1].mac;


    ret = sscanf(mac_str , "%02x:%02x:%02x:%02x:%02x:%02x" , 
            &temp[0] ,  &temp[1] , &temp[2] , 
            &temp[3] ,  &temp[4] , &temp[5]);

    // memcpy(mac_array , temp , 6);

    for(i=0 ; i<6 ; i++)
    {
        mac_array[i] = temp[i] & 0xff;
    }


    if ( ret != 6)
        return -1;

    // SF_CONF_PTR->interfaces[index - 1].mac[0] = mac_array[0];
    // SF_CONF_PTR->interfaces[index - 1].mac[1] = mac_array[1];
    // SF_CONF_PTR->interfaces[index - 1].mac[2] = mac_array[2];
    // SF_CONF_PTR->interfaces[index - 1].mac[3] = mac_array[3];
    // SF_CONF_PTR->interfaces[index - 1].mac[4] = mac_array[4];
    // SF_CONF_PTR->interfaces[index - 1].mac[5] = mac_array[5];

    return 0;
}

/** ip **/

int  get_interface_ipv4(int index, char *ip_str , int str_max_len)
{
    uint8_t *ipv4;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL || str_max_len < 20)
    {
        return -1;
    }

    memset(ip_str , 0 ,str_max_len );

    ipv4 = SF_CONF_PTR->interfaces[index - 1].ipv4_addr;

    sprintf(ip_str , "%d.%d.%d.%d" , ipv4[0] , ipv4[1] , ipv4[2] , ipv4[3]);

    return 0;
}

int  set_interface_ipv4(int index, char *ip_str)
{
    uint8_t *ipv4;
    int ret;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL)
    {
        return -1;
    }


    ipv4 = SF_CONF_PTR->interfaces[index - 1].ipv4_addr;

    ret = inet_pton(AF_INET, ip_str , ipv4); 

    if(ret <= 0)
    {
        return -1;
    }

    return 0;
}

int  get_interface_ipv6(int index, char *ip_str , int str_max_len)
{
    uint8_t *ipv6;
    const char *ret = NULL;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL || str_max_len < 40)
    {
        return -1;
    }

    memset(ip_str , 0 ,str_max_len );

    ipv6 = SF_CONF_PTR->interfaces[index - 1].ipv6_addr.ipv6_byte;
        
//    sprintf(ip_str , "%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x" , 
//            ipv6[0] , ipv6[1] , ipv6[2] , ipv6[3] , ipv6[4] , ipv6[5] , ipv6[6] , ipv6[7] ,
//            ipv6[8] , ipv6[9] , ipv6[10], ipv6[11], ipv6[12] ,ipv6[13] ,ipv6[14] ,ipv6[15]);
    
    ret = inet_ntop(AF_INET6 , ipv6 , ip_str , 40);

    if (ret == NULL)
    {
        return -1;
    }

    return 0;
}

int  set_interface_ipv6(int index, char *ip_str)
{
    uint8_t *ipv6;
    int ret;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL)
    {
        return -1;
    }


    ipv6 = SF_CONF_PTR->interfaces[index - 1].ipv6_addr.ipv6_byte;

    ret = inet_pton(AF_INET6, ip_str , ipv6); 

    if(ret <= 0)
    {
        return -1;
    }

    return 0;
}

/** gateway **/

int  get_interface_ipv4_gateway(int index, char *ip_str , int str_max_len)
{
    uint8_t *gateway;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL || str_max_len < 20)
    {
        return -1;
    }
    memset(ip_str , 0 ,str_max_len );


    gateway = SF_CONF_PTR->interfaces[index - 1].ipv4_gateway;

    sprintf(ip_str , "%d.%d.%d.%d" , gateway[0] , gateway[1] , gateway[2] , gateway[3]);

    return 0;
}

int  set_interface_ipv4_gateway(int index, char *ip_str)
{
    uint8_t *gateway;
    int ret;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL)
    {
        return -1;
    }



    gateway = SF_CONF_PTR->interfaces[index - 1].ipv4_gateway;

    ret = inet_pton(AF_INET, ip_str , gateway); 

    if(ret <= 0)
    {
        return -1;
    }

    return 0;
}

int  get_interface_ipv6_gateway(int index, char *ip_str , int str_max_len)
{
    uint8_t *gateway;
    const char *ret = NULL;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL || str_max_len < 40)
    {
        return -1;
    }
    memset(ip_str , 0 ,str_max_len );


    gateway = SF_CONF_PTR->interfaces[index - 1].ipv6_gateway.ipv6_byte;

    ret = inet_ntop(AF_INET6 , gateway , ip_str , 40);
    
    if (ret == NULL)
    {
        return -1;
    }

    return 0;
}

int  set_interface_ipv6_gateway(int index, char *ip_str)
{
    uint8_t *gateway;
    int ret;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(ip_str == NULL)
    {
        return -1;
    }



    gateway = SF_CONF_PTR->interfaces[index - 1].ipv6_gateway.ipv6_byte;

    ret = inet_pton(AF_INET6, ip_str , gateway); 

    if(ret <= 0)
    {
        return -1;
    }

    return 0;
}


/*** rule to action ***/
int get_ingress_rule_to_action(int index , int rule_id)
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(rule_id <=0 || rule_id > MAX_ACL_RULE_COUNT)
    {
        return -1;
    }


    return SF_CONF_PTR->interfaces[index - 1].ingress_config.rule_to_action[rule_id - 1 ];

}

int set_ingress_rule_to_action(int index , int rule_id , sf_action_index_t action_id)
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (index <= 0 || index >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    if(rule_id <=0 || rule_id > MAX_ACL_RULE_COUNT)
    {
        return -1;
    }

    if(action_id < 0 || action_id > MAX_ACTION_COUNT)
    {
        return -1;
    }

    SF_CONF_PTR->interfaces[index - 1].ingress_config.rule_to_action[rule_id - 1 ] = action_id;


    return 0;

}


/**** init & reset ****/
int init_single_interface_config(sf_inferface_t *sf_intf, int index , int is_reset)
{
	uint8_t mac_old[6];
    if (sf_intf == NULL)
    {
        return -1;
    }

    int i;

    if(is_reset)
    {
        memcpy(mac_old , sf_intf->mac , 6);
    }

    sf_memset_zero(sf_intf, sizeof(sf_inferface_t));


    sf_intf->port_id = index + 1;
    sf_intf->ingress_config.acl_rule_group = 1;

    if(is_reset)
    {
        memcpy(sf_intf->mac , mac_old , 6);
    }
}


RESET_CONFIG_FUNC_DEFINITION

INIT_CONFIG_FUNC_DEFINITION()
{
    if (SF_CONF_PTR_VPP == NULL)
    {
        return -1;
    }

    // sf_memset_zero(SF_CONF_PTR_VPP, sizeof(SF_CONF_T));

    int i;
    for (i = 0; i < MAX_INTERFACE_CNT; i++)
    {
        SF_CONF_PTR_VPP->admin_status[i] = 1;
        
        init_single_interface_config(SF_CONF_PTR_VPP->interfaces + i, i , is_reset);
    }

    return 0;
}



/*****  stat ******/
/* get */

single_interface_stat_t *get_interface_stat_ptr(int intf_id)
{
    if (SF_STAT_PTR == NULL)
    {
        return NULL;
    }

    if (intf_id <= 0 || intf_id >= MAX_INTERFACE_CNT + 1)
    {
        return NULL;
    }

    // calculate_single_interface_in_lib(index - 1);

    return SF_STAT_PTR->interfaces + intf_id - 1;
}

/* clean */
int clean_interface_stat(int intf_id)
{
    if (SF_STAT_PTR == NULL)
    {
        return -1;
    }

    if (intf_id <= 0 || intf_id >= MAX_INTERFACE_CNT + 1)
    {
        return -1;
    }

    sf_memset_zero(SF_STAT_PTR->interfaces + intf_id - 1 , sizeof(single_interface_stat_t) ); 
    
    return 0;
}

/* GET interface status */


int get_interfaces_real_cnt()
{
    if (SF_STAT_PTR == NULL)                                 
    {                                                        
        return -1;                                           
    }                                                        

    return SF_STAT_PTR->interfaces_cnt;
}

int get_interfaces_max_cnt()
{
    return MAX_INTERFACE_CNT;
}

/**** intf phy info ****/



#define GET_INTF_PHY_INFO_FUNC_DEFINE(member_name)             \
    int get_interfaces_phy_info_##member_name(int intf_id)        \
    {                                                            \
        if (SF_STAT_PTR == NULL)                                 \
        {                                                        \
            return -1;                                           \
        }                                                        \
        if (intf_id <= 0 || intf_id >= MAX_INTERFACE_CNT + 1)        \
        {                                                        \
            return -1;                                           \
        }                                                        \
        return SF_STAT_PTR->intf_phy_status[intf_id - 1].member_name; \
    }



#undef FUNC
#define FUNC(member_name) GET_INTF_PHY_INFO_FUNC_DEFINE(member_name)
foreach_phy_info_member

