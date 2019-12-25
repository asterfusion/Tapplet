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
#ifndef __include_action_path_tools_h__
#define __include_action_path_tools_h__

#include "action.h"

#undef FUNC

#undef foreach_additional_next_node
#define foreach_additional_next_node                                        \
    FUNC(SF_NEXT_NODE_ACTION_GRE_ENCAPSULATION, "node_action_encap_gre")    \
    FUNC(SF_NEXT_NODE_ACTION_ERSPAN_ENCAPSULATION, "node_action_encap_erspan")    \
    FUNC(SF_NEXT_NODE_ACTION_VXLAN_ENCAPSULATION, "node_action_encap_vxlan")

#undef foreach_forward_next_node
#define foreach_forward_next_node                                                                                          \
    FUNC(ACTION_TYPE_DROP, SF_NEXT_NODE_SF_PKT_DROP, "sf_pkt_drop")                                                        \
    FUNC(ACTION_TYPE_FORWARD_INTERFACE, SF_NEXT_NODE_ACTION_FORWARD_INTERFACE, "sf_pkt_output")                   \
    FUNC(ACTION_TYPE_LOAD_BALANCE_INTERFACES, SF_NEXT_NODE_ACTION_LOAD_BALANCE_INTERFACES, "node_load_balance_interfaces") \

typedef enum
{
#define FUNC(a, b) a,
    foreach_additional_next_node
#undef FUNC

#define FUNC(a, b, c) b,
        foreach_forward_next_node
#undef FUNC
            SF_ACTION_NEXT_NODE_N_COUNT,
} sf_action_next_index_t;

// just to get the total count
typedef enum
{
#define FUNC(a, b) TYPE_##a,
    foreach_additional_next_node
#undef FUNC
        ADDITIONAL_TYPE_COUNT
} sf_additional_type_t;

typedef enum
{
    ACTION_TYPE_INVALID = 0,
#define FUNC(a, b, c) a,
    foreach_forward_next_node
#undef FUNC
        ACTION_TYPE_COUNT
} sf_action_type_t;

#endif
