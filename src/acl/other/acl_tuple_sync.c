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

#include <rte_acl.h>


#include <vlib/vlib.h>


#include "acl_main.h"


#include "sf_acl_shm.h"

#include "sf_acl_lock.h"

#include "sf_thread_tools.h"


#define SF_DEBUG
#include "sf_debug.h"

/*****************************************************/
struct rte_acl_field_def acl_rte_ipv46_defs[NUM_FIELDS_IPV46] = {
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint8_t),
		.field_index = PROTO_FIELD_IPV46,
		.input_index = PROTO_FIELD_IPV46,
		.offset = PROTO_FIELD_IPV46_offset,
	},
    /**** SIP ***/
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SIP1_FIELD_IPV46,
		.input_index = SIP1_FIELD_IPV46,
		.offset = SIP1_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SIP2_FIELD_IPV46,
		.input_index = SIP2_FIELD_IPV46,
		.offset = SIP2_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SIP3_FIELD_IPV46,
		.input_index = SIP3_FIELD_IPV46,
		.offset = SIP3_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SIP4_FIELD_IPV46,
		.input_index = SIP4_FIELD_IPV46,
		.offset = SIP4_FIELD_IPV46_offset,
	},
    /**** DIP ***/
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DIP1_FIELD_IPV46,
		.input_index = DIP1_FIELD_IPV46,
		.offset = DIP1_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DIP2_FIELD_IPV46,
		.input_index = DIP2_FIELD_IPV46,
		.offset = DIP2_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DIP3_FIELD_IPV46,
		.input_index = DIP3_FIELD_IPV46,
		.offset = DIP3_FIELD_IPV46_offset,
	},
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DIP4_FIELD_IPV46,
		.input_index = DIP4_FIELD_IPV46,
		.offset = DIP4_FIELD_IPV46_offset,
	},
    /**** PORT ****/
    {
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = SPORT_FIELD_IPV46,
		.input_index = SPORT_FIELD_IPV46,
		.offset = SPORT_FIELD_IPV46_offset,
	},    
    {
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = DPORT_FIELD_IPV46,
		.input_index = SPORT_FIELD_IPV46,
		.offset = DPORT_FIELD_IPV46_offset,
	}, 
    
};


RTE_ACL_RULE_DEF(acl46_rule_cfg, RTE_DIM(acl_rte_ipv46_defs));



/******************************************************/
static char *table_name_model[] = {
    "v46s1_g%d",
    "v46s2_g%d"
};

int pre_table_name_model[MAX_RULE_GROUP_CNT] = {0};


void format_rte_acl_table_name(char *dst, char *model , int group_id)
{
    sprintf(dst , model , group_id);
}

void trans_shm_to_rte_tuple4(sf_tuple4_rule_cfg_t *raw_cfg , struct acl46_rule_cfg *rule_cfg)
{
    uint32_t *sip = (uint32_t *)raw_cfg->sip;
    uint32_t *dip = (uint32_t *)raw_cfg->dip;

    memset(rule_cfg , 0 ,sizeof(struct acl46_rule_cfg));

#define FUNC(a , b , c , d) rule_cfg->field[a].value.b  = raw_cfg->c; \
rule_cfg->field[a].mask_range.b  = raw_cfg->d;


FUNC(PROTO_FIELD_IPV46 , u8 , proto_min , proto_max)
FUNC(SPORT_FIELD_IPV46 , u16 , sport_min , sport_max)
FUNC(DPORT_FIELD_IPV46 , u16 , dport_min , dport_max)


#undef FUNC


    rule_cfg->field[SIP1_FIELD_IPV46].value.u32  = clib_net_to_host_u32(sip[0]); 
    rule_cfg->field[SIP1_FIELD_IPV46].mask_range.u32 = raw_cfg->sip_mask;


    rule_cfg->field[DIP1_FIELD_IPV46].value.u32  = clib_net_to_host_u32(dip[0]); 
    rule_cfg->field[DIP1_FIELD_IPV46].mask_range.u32 = raw_cfg->dip_mask;


    rule_cfg->field[SIP2_FIELD_IPV46].value.u32  = 0; 
    rule_cfg->field[SIP3_FIELD_IPV46].value.u32  = 0; 
    rule_cfg->field[SIP4_FIELD_IPV46].value.u32  = 0; 

    rule_cfg->field[SIP2_FIELD_IPV46].mask_range.u32 = 32;
    rule_cfg->field[SIP3_FIELD_IPV46].mask_range.u32 = 32;
    rule_cfg->field[SIP4_FIELD_IPV46].mask_range.u32 = 32;

    rule_cfg->field[DIP2_FIELD_IPV46].value.u32  = 0; 
    rule_cfg->field[DIP3_FIELD_IPV46].value.u32  = 0; 
    rule_cfg->field[DIP4_FIELD_IPV46].value.u32  = 0; 

    rule_cfg->field[DIP2_FIELD_IPV46].mask_range.u32 = 32;
    rule_cfg->field[DIP3_FIELD_IPV46].mask_range.u32 = 32;
    rule_cfg->field[DIP4_FIELD_IPV46].mask_range.u32 = 32;

}
void trans_shm_to_rte_tuple6(sf_tuple6_rule_cfg_t *raw_cfg , struct acl46_rule_cfg *rule_cfg)
{
    uint32_t *sip = (uint32_t *)raw_cfg->sip;
    uint32_t *dip = (uint32_t *)raw_cfg->dip;

    uint8_t sip_mask = raw_cfg->sip_mask;
    uint8_t dip_mask = raw_cfg->dip_mask;

    memset(rule_cfg , 0 ,sizeof(struct acl46_rule_cfg));

#define FUNC(a , b , c , d) rule_cfg->field[a].value.b  = raw_cfg->c; \
rule_cfg->field[a].mask_range.b  = raw_cfg->d;

FUNC(PROTO_FIELD_IPV46 , u8 , proto_min , proto_max)
FUNC(SPORT_FIELD_IPV46 , u16 , sport_min , sport_max)
FUNC(DPORT_FIELD_IPV46 , u16 , dport_min , dport_max)

#undef FUNC


#define mask_sub(mask) \
{\
    mask = (mask) > 32 ? (mask - 32) : 0;\
}

#define mask_get(mask) (mask) > 32 ? 32 : (mask)

    rule_cfg->field[SIP1_FIELD_IPV46].value.u32  = clib_net_to_host_u32(sip[0]); 
    rule_cfg->field[SIP2_FIELD_IPV46].value.u32  = clib_net_to_host_u32(sip[1]); 
    rule_cfg->field[SIP3_FIELD_IPV46].value.u32  = clib_net_to_host_u32(sip[2]); 
    rule_cfg->field[SIP4_FIELD_IPV46].value.u32  = clib_net_to_host_u32(sip[3]); 

    rule_cfg->field[SIP1_FIELD_IPV46].mask_range.u32 = mask_get(sip_mask); mask_sub(sip_mask);
    rule_cfg->field[SIP2_FIELD_IPV46].mask_range.u32 = mask_get(sip_mask); mask_sub(sip_mask);
    rule_cfg->field[SIP3_FIELD_IPV46].mask_range.u32 = mask_get(sip_mask); mask_sub(sip_mask);
    rule_cfg->field[SIP4_FIELD_IPV46].mask_range.u32 = mask_get(sip_mask);

    rule_cfg->field[DIP1_FIELD_IPV46].value.u32  = clib_net_to_host_u32(dip[0]); 
    rule_cfg->field[DIP2_FIELD_IPV46].value.u32  = clib_net_to_host_u32(dip[1]); 
    rule_cfg->field[DIP3_FIELD_IPV46].value.u32  = clib_net_to_host_u32(dip[2]); 
    rule_cfg->field[DIP4_FIELD_IPV46].value.u32  = clib_net_to_host_u32(dip[3]); 

    rule_cfg->field[DIP1_FIELD_IPV46].mask_range.u32 = mask_get(dip_mask); mask_sub(dip_mask);
    rule_cfg->field[DIP2_FIELD_IPV46].mask_range.u32 = mask_get(dip_mask); mask_sub(dip_mask);
    rule_cfg->field[DIP3_FIELD_IPV46].mask_range.u32 = mask_get(dip_mask); mask_sub(dip_mask);
    rule_cfg->field[DIP4_FIELD_IPV46].mask_range.u32 = mask_get(dip_mask);
}

void format_acl_rule(struct acl46_rule_cfg *rte_rule)
{
    int i;
    for(i=0; i<NUM_FIELDS_IPV46 ; i++)
    {
        sf_debug("------ %d size %d------\n" , i , acl_rte_ipv46_defs[i].size);
        switch (acl_rte_ipv46_defs[i].size)
        {
            case 1:
                sf_debug("min %d\n" , rte_rule->field[i].value.u8);
                sf_debug("max %d\n" , rte_rule->field[i].mask_range.u8);

                break;

            case 2:
                sf_debug("min %d\n" , rte_rule->field[i].value.u16);
                sf_debug("max %d\n" , rte_rule->field[i].mask_range.u16);            
                break;
            case 4:
                sf_debug("min %u\n" , rte_rule->field[i].value.u32);
                sf_debug("max %u\n" , rte_rule->field[i].mask_range.u32);                      
                break;
            default:
                break;
        }
    }
}

int rte_acl_table_add_rules(struct rte_acl_ctx *context , int group_id)
{
    struct acl46_rule_cfg rte_rule;

    sf_tuple4_rule_t *v4_rule;
    sf_tuple6_rule_t *v6_rule;

    int outer_rule_id;
    int inner_rule_index;

    int rule_cnt = 0;

    //ipv4
    for(inner_rule_index = 0 ; inner_rule_index < MAX_TUPLE4_RULE_PER_GROUP ; inner_rule_index++)
    {
        v4_rule = &( acl_config->tuple_rule_v4[group_id - 1][inner_rule_index] );
        outer_rule_id = v4_rule->map_rule.outer_rule_id;
        if(v4_rule->map_rule.outer_id_valid && outer_rule_id != 0)
        {
            trans_shm_to_rte_tuple4( &(v4_rule->rule_cfg) ,  &rte_rule);

format_acl_rule(&rte_rule);

            rte_rule.data.category_mask = 1;
            rte_rule.data.priority = sf_rte_rule_priority(outer_rule_id);
            rte_rule.data.userdata = outer_rule_id;

            if (rte_acl_add_rules(context, (struct rte_acl_rule *)&rte_rule, 1) < 0)
            {
                clib_warning("Failed to add rule (g%d r%d v4)\n" , group_id , outer_rule_id );
                return -1;
            }

            rule_cnt ++;
        }
    }
    //ipv6
    for(inner_rule_index = 0 ; inner_rule_index < MAX_TUPLE6_RULE_PER_GROUP ; inner_rule_index++)
    {
        v6_rule = &( acl_config->tuple_rule_v6[group_id - 1][inner_rule_index] );
        outer_rule_id = v6_rule->map_rule.outer_rule_id;
        if(v6_rule->map_rule.outer_id_valid && outer_rule_id != 0)
        {
            trans_shm_to_rte_tuple6( &(v6_rule->rule_cfg) ,  &rte_rule);


format_acl_rule(&rte_rule);

            rte_rule.data.category_mask = 1;
            rte_rule.data.priority = sf_rte_rule_priority(outer_rule_id);
            rte_rule.data.userdata = outer_rule_id;

            if (rte_acl_add_rules(context, (struct rte_acl_rule *)&rte_rule, 1) < 0)
            {
                clib_warning("Failed to add rule (g%d r%d v6)\n" , group_id , outer_rule_id );
                return -1;
            }

            rule_cnt ++;
        }
    }
    return rule_cnt;
}

void rte_acl_table_free(struct rte_acl_ctx *ctx)
{
    if(ctx == NULL) return;
    rte_acl_free(ctx);
}

int rte_acl_table_setup(int group_id , struct rte_acl_ctx **ret_ctx)
{
    struct rte_acl_ctx *context ;
	struct rte_acl_param acl_param;
    struct rte_acl_config acl_build_param;

	int dim = RTE_DIM(acl_rte_ipv46_defs);

sf_debug("dim : %d\n" , dim);

    char *name_model ;
    char table_name[32];
    int ret;
    if(group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -2;
    }

    name_model = table_name_model[ pre_table_name_model[group_id - 1] ];
    

    format_rte_acl_table_name(table_name , name_model , group_id);

    acl_param.name = table_name;
	acl_param.socket_id = SOCKET_ID_ANY; 
	acl_param.rule_size = RTE_ACL_RULE_SZ(dim);

sf_debug("rule_size : %d\n" , acl_param.rule_size);


	acl_param.max_rule_num = MAX_TUPLE6_RULE_PER_GROUP + MAX_TUPLE4_RULE_PER_GROUP ;


    context = rte_acl_create(&acl_param);


    if(context == NULL)
    {
        clib_warning("Failed to create ACL context (g%d)\n" , group_id );
        return -1;
    }

    // TODO : check cpu : sse3 avx ...
    if ( rte_acl_set_ctx_classify(context, RTE_ACL_CLASSIFY_SCALAR) != 0)
    {
        clib_warning("Failed to setup classify method for  ACL context (g%d)\n" , group_id );
        goto setup_on_error;
    }

    ret = rte_acl_table_add_rules(context , group_id );
    sf_debug("ret : %d\n" , ret);
    if( ret < 0)
    {
        clib_warning("Failed to add rules (g%d)\n" , group_id );
        goto setup_on_error;        
    }
    else if (ret == 0)
    {
        *ret_ctx = NULL;
        rte_acl_table_free(context);
        return 0;
    }
    

    memset(&acl_build_param, 0, sizeof(acl_build_param));
	acl_build_param.num_categories = 1;
	acl_build_param.num_fields = dim;

	memcpy(&acl_build_param.defs, acl_rte_ipv46_defs, sizeof(acl_rte_ipv46_defs));

	if (rte_acl_build(context, &acl_build_param) != 0)
    {
        clib_warning("Failed to build ACL trie (g%d)\n" , group_id );
        goto setup_on_error;    
    }


// setup_on_success:
    *ret_ctx = context;
    return 0;

setup_on_error:
    rte_acl_free(context);
    return -1;
}



void sf_rte_table_sync()
{
	uint32_t  worker_num = sf_vlib_num_workers();
	uint32_t i;

	for(i= 0 ; i<worker_num ; i++)
	{
		sf_acl_sync_lock(i);
		// do nothing , just make sure old table is not in use
		// Notice , unlock will call CLIB_MEMORY_BARRIER to make sure new ptr has been writen
		sf_acl_sync_unlock(i);
	}
}


int sync_rte_tuple_rules(int check_sync)
{
    struct rte_acl_ctx *v46_context;
    struct rte_acl_ctx *swap_context;

    int group_id = 0;
    int ret;
    int error_happened = 0;


{
    int i;
    for(i=0;i< NUM_FIELDS_IPV46; i++)
    {
		// .type = RTE_ACL_FIELD_TYPE_RANGE,
		// .size = sizeof(uint8_t),
		// .field_index = PROTO_FIELD_IPV46,
		// .input_index = PROTO_FIELD_IPV46,
		// .offset = PROTO_FIELD_IPV46_offset,
        sf_debug("--%d--\n" , i);
        sf_debug("type : %d\n" ,acl_rte_ipv46_defs[i].type);
        sf_debug("size : %d\n" ,acl_rte_ipv46_defs[i].size);

        sf_debug("field : %d\n" , acl_rte_ipv46_defs[i].field_index);
        sf_debug("input : %d\n" , acl_rte_ipv46_defs[i].input_index);
        sf_debug("offset : %d\n" , acl_rte_ipv46_defs[i].offset);
    }
}

    for(group_id = 1 ; group_id <= MAX_RULE_GROUP_CNT ; group_id++)
    {
        ret = rte_acl_table_setup(group_id , &v46_context);
        if( ret != 0 )
        {
            error_happened --;
            continue;
        }
        sf_debug("sync g%d over\n", group_id);

        swap_context = sf_acl_main.v46_context[group_id -1];

        sf_acl_main.v46_context[group_id -1] = v46_context;
        
        // make sure no worker thread is using old table any more
        if(check_sync)
        {
            sf_rte_table_sync();
        }

        if(v46_context != NULL)
        {
            pre_table_name_model[group_id -1] = !(pre_table_name_model[group_id -1]); // 0 or 1
        }

        rte_acl_table_free(swap_context);
    }

    return error_happened;
}
