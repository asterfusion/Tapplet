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
#ifndef __include_acl_defines_h__
#define __include_acl_defines_h__

/*********************************/
/****  acl init config ***********/
/*********************************/


#define MAX_TUPLE4_RULE_PER_GROUP 2048
#define MAX_TUPLE6_RULE_PER_GROUP 2048

#define TUPLE4_RULE_BITMAP_CNT (MAX_TUPLE4_RULE_PER_GROUP/64)
#define TUPLE6_RULE_BITMAP_CNT (MAX_TUPLE6_RULE_PER_GROUP/64)

#define TUPLE4_RULE_HASH_BUCKET_CNT (MAX_TUPLE4_RULE_PER_GROUP/8)
#define TUPLE6_RULE_HASH_BUCKET_CNT (MAX_TUPLE6_RULE_PER_GROUP/8)




/********************************/


#define MAX_RULE_GROUP_CNT 1

#define MAX_RULE_PER_GROUP  (MAX_TUPLE4_RULE_PER_GROUP + MAX_TUPLE6_RULE_PER_GROUP)

#define MAX_ACL_RULE_COUNT (MAX_RULE_PER_GROUP * MAX_RULE_GROUP_CNT)

#define acl_get_rule_id(result) ((result) & 0xFFFFFFFF)

#endif