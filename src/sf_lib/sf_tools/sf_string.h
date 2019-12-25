#ifndef __include_sf_string_h__
#define __include_sf_string_h__


#include <vppinfra/clib.h>

#include <string.h>
#include <stdint.h>

#define sf_memset_zero(p , count) memset(p , 0 , count)

#define sf_memcmp(a , b, c) memcmp(a , b , c)

#define sf_strncmp(a,b,c) (memcmp(a , b , c ))

#endif