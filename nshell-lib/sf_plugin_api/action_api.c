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
#include "action_api.h"

#include <netinet/in.h>
//#include <endian.h>

SF_CONF_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_CONF


//reset config
RESET_CONFIG_FUNC_DEFINITION

INIT_CONFIG_FUNC_DEFINITION()
{
    if(SF_CONF_PTR_VPP == NULL)
    {
        return -1;
    }
    sf_memset_zero(SF_CONF_PTR_VPP , sizeof(SF_CONF_T));

    return 0;
}

int action_get_max_cnt()
{
    return MAX_ACTION_NUM;
}

/* put & post */
// uint32_t put_action_config(sf_action_t *action, uint32_t action_id)
// {
//     if(action_config_ptr == NULL)
//         return (uint32_t)-1;

//     if(!is_valid_action_id(action_id))
//         return (uint32_t)-1;
//     uint32_t  weight_count = 0;
//     int i;
//     int flag = 0;
//     memcpy(&(action_config_ptr->actions[action_id-1]), action, sizeof(sf_action_t));
//     action_config_ptr->actions[action_id-1].ref_counter++;

//     for(i = 0 ; i < MAX_LOAD_BALANCE_NODE ; i++)
//     {
//         if(action_config_ptr->actions[action_id-1].load_balance_weight[i] != 0)
//             flag += 1;
//     }

//     if(action_config_ptr->actions[action_id-1].type == ACTION_TYPE_LOAD_BALANCE_INTERFACES)
//         weight_count = action_config_ptr->actions[action_id-1].load_balance_interfaces_num;


//     if(flag == weight_count)
//     {
//         action_config_ptr->actions[action_id-1].weight_data.cw = 0;
//         action_config_ptr->actions[action_id-1].weight_data.gcd = weight_calc_max(
//                 action_config_ptr->actions[action_id-1].load_balance_weight, 
//                 weight_count,   
//                 &(action_config_ptr->actions[action_id-1].weight_data.mw));
//     }


//     //Host order to network order
//     action_config_ptr->actions[action_id-1].additional_actions.vxlan_dport = htons(action_config_ptr->actions[action_id-1].additional_actions.vxlan_dport);
//     action_config_ptr->actions[action_id-1].additional_actions.vxlan_vni = htonl(action_config_ptr->actions[action_id-1].additional_actions.vxlan_vni << 8);

//     return 0;
// }


#if 0

void format_action_config(sf_action_t *action)
{
    int i;
    printf("%20s : %d\n" , "ref_counter" , action->ref_counter);
    printf("%20s : %d\n" , "type" , action->type);
    printf("%20s : %d\n" , "forward_interface_id" , action->forward_interface_id);
    printf("%20s : %d\n" , "load_balance_interfaces_num" , action->load_balance_interfaces_num);
    printf("load_balance_interface_array : ");
    for(i=0;i<MAX_INTERFACE_CNT;i++)
    {
        printf("%d " , action->load_balance_interface_array[i]);
    }
    printf("\n");

    printf("%20s : %d\n" , "load_balance_mode" , action->load_balance_mode);

    printf("load_balance_weight : ");
    for(i=0;i<MAX_LOAD_BALANCE_NODE;i++)
    {
        printf("%d " , action->load_balance_weight[i]);
    }
    printf("\n");

    printf("-------------\n");

    printf("-- gre encap --\n");
    printf("switch : %d\n" , action->additional_actions.additional_switch.flag_gre_encapsulation);
    printf("mac: ");
    for(i=0;i<6;i++)
    {
        printf("%02x " , action->additional_actions.gre_dmac[i]);
    }
    printf("\n"); 

    printf("dip type : %d\n" , action->additional_actions.gre_dip_type);
    printf("ip: ");
    for(i=0;i<16;i++)
    {
        printf("%02x " , action->additional_actions.gre_dip.ipv6_byte[i]);
    }
    printf("\n"); 

    printf("gre_dscp : %d\n" ,action->additional_actions.gre_dscp ) ;

    printf("-- vxlan encap --\n");
    printf("switch : %d\n" , action->additional_actions.additional_switch.flag_vxlan_encapsulation);
    printf("mac: ");
    for(i=0;i<6;i++)
    {
        printf("%02x " , action->additional_actions.vxlan_dmac[i]);
    }
    printf("\n"); 

    printf("dip type : %d\n" , action->additional_actions.vxlan_dip_type);
    printf("ip: ");
    for(i=0;i<16;i++)
    {
        printf("%02x " , action->additional_actions.vxlan_dip.ipv6_byte[i]);
    }
    printf("\n"); 

    printf("vxlan_dscp : %d\n" ,action->additional_actions.vxlan_dscp ) ;
    printf("vxlan_dport : %d\n" ,action->additional_actions.vxlan_dport ) ;
    printf("vxlan_vni : %d\n" ,action->additional_actions.vxlan_vni ) ;

    printf("-- erspan encap --\n");
    printf("switch : %d\n" , action->additional_actions.additional_switch.flag_erspan_encapsulation);
    printf("mac: ");
    for(i=0;i<6;i++)
    {
        printf("%02x " , action->additional_actions.erspan_dmac[i]);
    }
    printf("\n"); 

    printf("dip type : %d\n" , action->additional_actions.erspan_dip_type);
    printf("ip: ");
    for(i=0;i<16;i++)
    {
        printf("%02x " , action->additional_actions.erspan_dip.ipv6_byte[i]);
    }
    printf("\n"); 

    printf("erspan_dscp : %d\n" ,action->additional_actions.erspan_dscp ) ;
    printf("erspan_type : %d\n" ,action->additional_actions.erspan_type ) ;
    printf("erspan_session_id : %d\n" ,action->additional_actions.erspan_session_id ) ;
}
#endif

int get_action_config(sf_action_t *action, uint32_t action_id)
{
    if(action_config_ptr == NULL)
        return -1;

    if(!is_valid_action_id(action_id))
        return -1;    

    if(action == NULL)
    {
        return -1;
    }
    memcpy(action , &(action_config_ptr->actions[action_id-1]) , sizeof(sf_action_t) );
    
    // format_action_config(action);

    return 0;
}


int put_action_config(sf_action_t *action, uint32_t action_id)
{
    if(action_config_ptr == NULL)
        return -1;

    if(!is_valid_action_id(action_id))
        return -1;


    memcpy(&(action_config_ptr->actions[action_id-1]), action, sizeof(sf_action_t));


    if(action->load_balance_mode == LOAD_BALANCE_MODE_WRR)
    {
        action_config_ptr->actions[action_id-1].weight_data.cw = 0;
        action_config_ptr->actions[action_id-1].weight_data.gcd = weight_calc_max(
                action_config_ptr->actions[action_id-1].load_balance_weight, 
                action->load_balance_interfaces_num ,   
                &(action_config_ptr->actions[action_id-1].weight_data.mw));
    }


    //Host order to network order
    action_config_ptr->actions[action_id-1].additional_actions.vxlan_dport = htons(action_config_ptr->actions[action_id-1].additional_actions.vxlan_dport);
    action_config_ptr->actions[action_id-1].additional_actions.vxlan_vni = htonl(action_config_ptr->actions[action_id-1].additional_actions.vxlan_vni << 8);


    action_config_ptr->actions[action_id-1].ref_counter++;

    return 0;
}

/*DELETE*/
int delete_action_config(int action_id)
{
    if(action_config_ptr == NULL)
        return -1;

    if(!is_valid_action_id(action_id))
        return -1;
    memset(&(action_config_ptr->actions[action_id-1]), 0 , sizeof(sf_action_t));
    return 0;
}




int check_action_if_exist(int action_id)
{
    if (action_config_ptr == NULL)
        return -1;
    
    if(!is_valid_action_id(action_id))
        return -1;

    if(action_config_ptr->actions[action_id - 1].ref_counter > 0 )
    {
        return 1;
    }

    return 0;
} 



int reset_action_config_struct(sf_action_t *action)
{
    memset(action , 0 , sizeof(sf_action_t));
    return 0;
}