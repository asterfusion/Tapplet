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
#ifndef __include_sf_api_macro_h__
#define __include_sf_api_macro_h__

#include <string.h>

/******** stat /conf struct name ********/
#define __SF_T(name, type) name##_##type##_t

#define _SF_T(name, type) __SF_T(name, type)

#define SF_CONF_T _SF_T(MOD_NAME, global_config)

#define SF_STAT_T _SF_T(MOD_NAME, global_stat)

/******** stat /conf ptr name ********/
#define __SF_PTR(name, type) name##_##type##_ptr

#define _SF_PTR(name, type) __SF_PTR(name, type)

#define SF_STAT_PTR _SF_PTR(MOD_NAME, stat)

#define SF_CONF_PTR _SF_PTR(MOD_NAME, config)

/******** stat /conf ptr extern  declare ********/
#define __SF_PTR_DECLARE(type, name) extern type *name;

#define _SF_PTR_DECLARE(type, name) __SF_PTR_DECLARE(type, name)

#define SF_CONF_PTR_DECLARE _SF_PTR_DECLARE(SF_CONF_T, SF_CONF_PTR)

#define SF_STAT_PTR_DECLARE _SF_PTR_DECLARE(SF_STAT_T, SF_STAT_PTR)

/******** stat /conf ptr definition ********/

#define __SF_PTR_DEFINE(type, name) type *name;

#define _SF_PTR_DEFINE(type, name) __SF_PTR_DEFINE(type, name)

#define SF_CONF_PTR_DEFINE _SF_PTR_DEFINE(SF_CONF_T, SF_CONF_PTR)

#define SF_STAT_PTR_DEFINE _SF_PTR_DEFINE(SF_STAT_T, SF_STAT_PTR)

/******** config get/set declare ********/

#define _GET_SET_API_DECLARE(name, type, member) \
	int get_##name##_config_##member();         \
	int set_##name##_config_##member(type value);

#define GET_SET_API_DECLARE(name, type, member) \
	_GET_SET_API_DECLARE(name, type, member)

#define GET_SET_API_DECLARE_CONF(type, member) \
	GET_SET_API_DECLARE(MOD_NAME, type, member)

/******** config get/set function implement********/
#define __GET_SET_API_FUNC(name, ptr, type, member, minnum, maxnum) \
	int get_##name##_config_##member()                             \
	{                                                               \
		if (ptr != NULL)                                            \
			return ptr->member;                                     \
		return -1;                                                  \
	}                                                               \
	int set_##name##_config_##member(type value)                    \
	{                                                               \
		if (ptr != NULL)                                            \
		{                                                           \
			if (value >= minnum && value <= maxnum)                 \
			{                                                       \
				ptr->member = value;                                \
				return 0;                                           \
			}                                                       \
			return -2;                                              \
		}                                                           \
		return -1;                                                  \
	}

#define GET_SET_API_FUNC(name, ptr, type, member, minnum, maxnum) \
	__GET_SET_API_FUNC(name, ptr, type, member, minnum, maxnum)

#define GET_SET_API_FUNC_CONF_DEFINE(type, member, minnum, maxnum) \
	GET_SET_API_FUNC(MOD_NAME, SF_CONF_PTR, type, member, minnum, maxnum)

/******** stat ptr get function declare********/
#define __GET_STAT_PTR_DECLARE(type, name) \
	type *get_##name##_stat_ptr();

#define _GET_STAT_PTR_DECLARE(type, name) \
	__GET_STAT_PTR_DECLARE(type, name)

#define GET_STAT_PTR_FUNC_DECLARE \
	_GET_STAT_PTR_DECLARE(SF_STAT_T, MOD_NAME)

/******** stat ptr get function implement********/

#define __GET_STAT_PTR_FUNC(type, name, ptr) \
	type *get_##name##_stat_ptr() { return ptr; }

#define _GET_STAT_PTR_FUNC(type, name, ptr) \
	__GET_STAT_PTR_FUNC(type, name, ptr)

#define GET_STAT_PTR_FUNC_DEFINE \
	_GET_STAT_PTR_FUNC(SF_STAT_T, MOD_NAME, SF_STAT_PTR)

/******** stat ptr clean function declare********/
#define __CLEAN_STAT_DECLARE(name) \
	int clean_##name##_stat();

#define _CLEAN_STAT_DECLARE(name) \
	__CLEAN_STAT_DECLARE(name)

#define CLEAN_STAT_FUNC_DECLARE \
	_CLEAN_STAT_DECLARE(MOD_NAME)

/******** stat ptr get function implement********/

#define __CLEAN_STAT_FUNC(type, name, ptr) \
	int clean_##name##_stat()              \
	{                                      \
		if (ptr != NULL)                   \
		{                                  \
			memset(ptr, 0, sizeof(type));  \
			return 0;                      \
		}                                  \
		return -1;                         \
	}

#define _CLEAN_STAT_FUNC(type, name, ptr) \
	__CLEAN_STAT_FUNC(type, name, ptr)

#define CLEAN_STAT_FUNC_DEFINE \
	_CLEAN_STAT_FUNC(SF_STAT_T, MOD_NAME, SF_STAT_PTR)

/*********************for vpp ******************************/
/******** stat /conf ptr name ********/
#define __SF_PTR_VPP(name, type) name##_##type

#define _SF_PTR_VPP(name, type) __SF_PTR_VPP(name, type)

#define SF_STAT_PTR_VPP _SF_PTR_VPP(MOD_NAME, stat)

#define SF_CONF_PTR_VPP _SF_PTR_VPP(MOD_NAME, config)

/******** stat /conf ptr extern  declare ********/
#define __SF_PTR_DECLARE_VPP(type, name) extern type *name;

#define _SF_PTR_DECLARE_VPP(type, name) __SF_PTR_DECLARE_VPP(type, name)

#define SF_CONF_PTR_DECLARE_VPP _SF_PTR_DECLARE_VPP(SF_CONF_T, SF_CONF_PTR_VPP)

#define SF_STAT_PTR_DECLARE_VPP _SF_PTR_DECLARE_VPP(SF_STAT_T, SF_STAT_PTR_VPP)

/******** stat /conf ptr definition ********/

#define __SF_PTR_DEFINE_VPP(type, name) type *name;

#define _SF_PTR_DEFINE_VPP(type, name) __SF_PTR_DEFINE_VPP(type, name)

#define SF_CONF_PTR_DEFINE_VPP _SF_PTR_DEFINE_VPP(SF_CONF_T, SF_CONF_PTR_VPP)

#define SF_STAT_PTR_DEFINE_VPP _SF_PTR_DEFINE_VPP(SF_STAT_T, SF_STAT_PTR_VPP)

/******** conf init function name (!!!!!)********/
#define __INIT_CONFIG_FUNC_NAME(name) init_##name##_global_config
#define _INIT_CONFIG_FUNC_NAME(name) __INIT_CONFIG_FUNC_NAME(name)
#define INIT_CONFIG_FUNC_NAME _INIT_CONFIG_FUNC_NAME(MOD_NAME)

/******** conf init function declare ********/
#define __INIT_CONFIG_FUNC_DECLARE(modname) int init_##modname##_global_config();
#define _INIT_CONFIG_FUNC_DECLARE(modname) __INIT_CONFIG_FUNC_DECLARE(modname)
#define INIT_CONFIG_FUNC_DECLARE _INIT_CONFIG_FUNC_DECLARE(MOD_NAME)

/******** conf init function definition ********/
#define __INIT_CONFIG_FUNC_DEFINITION(modname, type, ptr_name) \
	int __init_##modname##_global_config(type *ptr_name , int is_reset);      \
	int init_##modname##_global_config(void *ptr, int is_reset)              \
	{                                                          \
		type *ptr_name = (type *)ptr;                          \
		return __init_##modname##_global_config(ptr_name , is_reset);     \
	}                                                          \
	int __init_##modname##_global_config(type *ptr_name , int is_reset)

#define _INIT_CONFIG_FUNC_DEFINITION(modname, type, ptr_name) __INIT_CONFIG_FUNC_DEFINITION(modname, type, ptr_name)
#define INIT_CONFIG_FUNC_DEFINITION() _INIT_CONFIG_FUNC_DEFINITION(MOD_NAME, SF_CONF_T, SF_CONF_PTR_VPP)

/******** reset config declare ********/
#define __RESET_CONFIG_FUNC_DECLARE(modname) int reset_##modname##_global_config();
#define _RESET_CONFIG_FUNC_DECLARE(modname) __RESET_CONFIG_FUNC_DECLARE(modname)
#define RESET_CONFIG_FUNC_DECLARE _RESET_CONFIG_FUNC_DECLARE(MOD_NAME)
/******** reset config definition ********/
#define __RESET_CONFIG_FUNC_DEFINITION(modname, init_function, ptr) \
	int reset_##modname##_global_config()                           \
	{                                                               \
		return init_function(ptr , 1);                              \
	}
#define _RESET_CONFIG_FUNC_DEFINITION(modname, init_function, ptr) __RESET_CONFIG_FUNC_DEFINITION(modname, init_function, ptr)
#define RESET_CONFIG_FUNC_DEFINITION _RESET_CONFIG_FUNC_DEFINITION(MOD_NAME, INIT_CONFIG_FUNC_NAME, SF_CONF_PTR)

#endif