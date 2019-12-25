
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
// avoid include this file in other [.h] files
#undef sf_debug
#undef SF_ASSERT


#if  defined(SF_DEBUG)  && !defined(BUILD_RELEASE)

#include <vppinfra/error.h>

#define sf_debug(fmt, ...) clib_warning(fmt,## __VA_ARGS__)
#else
#define sf_debug(fmt, ...) ;
#endif



#if  defined(SF_DEBUG)  && !defined(BUILD_RELEASE)

#undef CLIB_ASSERT_ENABLE 
#define CLIB_ASSERT_ENABLE 1
#include <vppinfra/error_bootstrap.h>

#define SF_ASSERT(truth) ASSERT(truth)

#else // SF_DEBUG

#define SF_ASSERT(truth) 

#endif


