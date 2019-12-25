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
#include <vppinfra/smp.h>

#include <vppinfra/format.h>

#include "generic_hash.h"
#include "sf_string.h"


/*bcz the restriction of memory-allocation*/
#define GENERIC_MAX_HASH_SIZE 0x2000000

int hashInit(generic_hash_t *h, uint32_t hashsize, uint32_t hashmax)
{
    uint32_t temp1;
    uint32_t hashsize_admit = 0x80000000;
    int i;

    if((!h) || (!hashsize) || (!hashmax))
        return -1;

    //restrict hash size, bcz of the memory allocation
    if(hashsize>GENERIC_MAX_HASH_SIZE)
        hashsize = GENERIC_MAX_HASH_SIZE;

    //adjust hash size ; 2^log2(hashsize) ???
    temp1 = hashsize;
    while((temp1 & 0x80000000) == 0)
    {
        hashsize_admit = hashsize_admit >> 1;
        temp1 = temp1 << 1;
    }

    sf_memset_zero(h, sizeof(generic_hash_t) );
    h->buckets = vec_new_ha(generic_buck_t , hashsize_admit , 0 , CLIB_CACHE_LINE_BYTES);

    if(h->buckets == NULL) return -1;
    // memset(PHY2PTR(h->buckets), 0, hashsize_admit*sizeof(generic_buck_t));
    sf_memset_zero(h->buckets , hashsize_admit*sizeof(generic_buck_t));
    
    for(i=0;i<hashsize_admit;i++)
    {
        sf_spinlock_init( &( h->buckets[i].lock ));
    }

    h->hashsize = hashsize_admit;
    h->hashmax = hashmax;
    return 0;
}

/* BEWARE : only buckets are freed  */
/* BEWARE : also spinlocks need to be freed  */
void hashFree(generic_hash_t *h)
{
    vec_free(h->buckets);
    sf_memset_zero(h, sizeof(generic_hash_t) ); // ?? needed?
}

// void hashReset(generic_hash_t *h)
// {
//     int i;
//     h->bcount = h->ncount = 0;
//     h->weight = 0;
//     h->bmax = h->lmax = h->nmax = 0;
    
// }



generic_pnode_t hashNodeGet(generic_hash_t *h, uint32_t hash, generic_pnode_t c_node)
{
    generic_pbuck_t buck;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    return (c_node == NULL) ? buck->head:c_node->next;
}

int hashNodeCountGet(generic_hash_t *h, uint32_t hash)
{
    generic_pbuck_t buck;
    
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    
    return buck->ncount;
}


generic_pnode_t hashFindNodeByPtr(generic_hash_t *h, uint32_t hash, generic_pnode_t c_node)
{
    generic_pbuck_t buck;
    generic_pnode_t node;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    for(node=buck->head ; node ; node=node->next) {
        if(node == c_node) 
            return c_node; 
    }
    return NULL;
}



void hashTraverse(void *context, generic_hash_t *h, int (*handler)(void *,generic_pnode_t))
{
    uint32_t i;
    generic_pnode_t node;
    generic_pbuck_t buck;
    for(i=0 ; i<h->hashsize ; i++) 
    {
        buck = h->buckets + i;
        for(node=buck->head ; node ; node=node->next) 
        {
            if((*handler)(context, node)) 
                return; 
        }
    }
}

// static u8 * format_node(u8 * s, va_list * args)
// {
//    generic_pnode_t node = va_arg (*args, generic_pnode_t );
  
//   return format (s, "self : %p , prev: %p , next: %p", node ,  node->prev , node->next);
// }



u8 * format_hash_table (u8 * s, va_list * args)
{
  generic_hash_t *h = va_arg (*args, generic_hash_t *);
  u32 show_detail = va_arg (*args, u32);
  
  int bucket_index ;
//   generic_pnode_t node_ptr;
    // int ncount;

  s = format(s , "%10s : %d\n" , "hashsize" , h->hashsize);
  s = format(s , "%10s : %d\n" , "hashmax", h->hashmax);
  s = format(s , "%10s : %d\n" , "bcount", h->bcount);
  s = format(s , "%10s : %d\n" , "ncount", h->ncount);
  s = format(s , "%10s : %d\n" , "lmax", h->lmax);
  s = format(s , "%10s : %d\n" , "nmax", h->nmax);
  s = format(s , "%10s : %d\n" , "bmax", h->bmax);
  s = format(s , "%10s : %lu\n" , "weight", h->weight);

    if(!show_detail) 
        return s;

#if 1

    for(bucket_index=0; bucket_index<h->hashsize ; bucket_index++)
    {
        if(h->buckets[bucket_index].ncount == 0)
            continue;
        
        s = format(s , "buckt(%d) , node cnt(%d)\n" , 
                    bucket_index , h->buckets[bucket_index].ncount);
        // ncount = h->buckets[bucket_index].ncount;
#if 0
        node_ptr = h->buckets[bucket_index].head;
        while(node_ptr != NULL )
        {
            s = format(s, "%U\n" , format_node , node_ptr);
            node_ptr = node_ptr->next;
        }
#endif
    }
#endif
    return s;


}
