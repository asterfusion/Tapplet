#ifndef __include_sf_stat_tools_h__
#define __include_sf_stat_tools_h__

#include <vppinfra/smp.h>


// #define VALIDATE_STAT_NULL_POINTER 1

#if VALIDATE_STAT_NULL_POINTER


#define update_global_stat(ptr,member) \
if((ptr) != NULL){clib_smp_atomic_add(&((ptr)->member),1);}

// if increment is negative , then it means decrement
#define update_global_stat_multi(ptr,member,increment) \
if((ptr) != NULL){clib_smp_atomic_add(&((ptr)->member),increment);}

#define decrease_global_stat(ptr,member) \
if((ptr) != NULL){clib_smp_atomic_add(&((ptr)->member),-1);}


#else // #if VALIDATE_STAT_NULL_POINTER

#define update_global_stat(ptr,member) \
clib_smp_atomic_add(&((ptr)->member),1)

// if increment is negative , then it means decrement
#define update_global_stat_multi(ptr,member,increment) \
clib_smp_atomic_add(&((ptr)->member),increment)

#define decrease_global_stat(ptr,member) \
clib_smp_atomic_add(&((ptr)->member),-1)




#endif // #if VALIDATE_STAT_NULL_POINTER
#endif //#ifndef __include_sf_stat_tools_h__
