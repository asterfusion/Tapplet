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


#ifndef __include_generic_hash_h__
#define __include_generic_hash_h__

#include <stdint.h>
#include <vlib/vlib.h>
#include <vppinfra/clib.h>
#include <vppinfra/lock.h>
#include <vppinfra/crc32.h>
#include "sf_crc32.h"

#include "sf_lock.h"
#include "sf_string.h"

typedef struct _generic_node_t generic_node_t, *generic_pnode_t; 
typedef struct _generic_buck_t generic_buck_t, *generic_pbuck_t; 


struct _generic_node_t {	
    generic_pnode_t prev;	
    generic_pnode_t next;	
    uint8_t node_data[0];
};

struct _generic_buck_t {	
    sf_spinlock_t   lock; 
    uint32_t   ncount;  //node count
    generic_pnode_t head;	
};


typedef union {
    struct{
        //1*8
        uint32_t   hashsize; //hash size ; bucket total count
        uint32_t   hashmax; //hash max ; node max cnt
        //1*8
        uint32_t   bcount;  //buckets count in use
        uint32_t   ncount;  //node count in use
        //1*8
        uint32_t   lmax;    //max bucket length in history
        uint32_t   nmax;    //max node in history
        //1*8
        uint32_t   bmax;    //max buckets in history
        uint32_t   reserved;
        //2*8
        uint64_t weight;  //total weight
        generic_pbuck_t buckets;
    };
    uint64_t data64[6];
} generic_hash_t;

// int getHashIndex(uint8_t *key, uint32_t keysize);
int hashInit(generic_hash_t *h, uint32_t hashsize, uint32_t hashmax);
// generic_pnode_t hashFindNode(generic_hash_t *h, int hash, void *key, int (*cmp)(void *, generic_pnode_t));
generic_pnode_t hashFindNodeByPtr(generic_hash_t *h, uint32_t hash, generic_pnode_t c_node);
generic_pnode_t hashNodeGet(generic_hash_t *h, uint32_t hash, generic_pnode_t c_node);
int hashNodeCountGet(generic_hash_t *h, uint32_t hash);
// int hashInsertNode(generic_hash_t *h, int hash, generic_pnode_t node);
void hashTraverse(void *context, generic_hash_t *h, int (*handler)(void *, generic_pnode_t));
// void hashFreeNode(generic_hash_t *h, generic_pnode_t node, int hash);
// void hashReset(generic_hash_t *h);
void hashFree(generic_hash_t *h);
// void *node_data(generic_pnode_t node);
// void hashLock(generic_hash_t *h, int hash);
// void hashUnlock(generic_hash_t *h, int hash);

u8 * format_hash_table (u8 * s, va_list * args);

static_always_inline uint32_t 
getHashIndex(uint8_t *key, int keysize)
{
#ifdef clib_crc32c_uses_intrinsics
    return clib_crc32c(key , keysize);
#else
    return sf_crc32_gen((void *)key , keysize);
#endif
}


static_always_inline void 
hashLock(generic_hash_t *h, uint32_t hash)
{
    generic_pbuck_t buck;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    sf_spinlock_lock(&(buck->lock));
}

static_always_inline void 
hashUnlock(generic_hash_t *h, uint32_t hash)
{
    generic_pbuck_t buck;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    sf_spinlock_unlock(&(buck->lock));
}

static_always_inline void* 
node_data(generic_pnode_t node) 
{ 
    return (void*)(node->node_data); 
}

static_always_inline generic_pnode_t 
hashFindNode(generic_hash_t *h, uint32_t hash, void *key, 
                int (*cmp)(void *, generic_pnode_t))
{
    generic_pbuck_t buck;
    generic_pnode_t node;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    for(node=buck->head ; node ; node=node->next) 
    {
        if((*cmp)(key, node) == 0) 
            return node; 
    }
    return NULL;
}

/****
 *  Notice : if you want to use this function , make sure that "node_data(node)" points to the key of the node
 */
static_always_inline generic_pnode_t 
hashFindNode_memcmp(generic_hash_t *h, uint32_t hash, void *key, 
                int key_len)
{
    generic_pbuck_t buck;
    generic_pnode_t node;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    for(node=buck->head ; node ; node=node->next) 
    {
        if(sf_memcmp(key , node_data(node) , key_len) == 0) 
            return node; 
    }
    return NULL;
}


static_always_inline int 
hashInsertNode(generic_hash_t *h, uint32_t hash, 
                generic_pnode_t node)
{
    generic_pbuck_t buck;
    // if(h->ncount >= h->hashmax)
    //     return -1;

    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;
    node->next = NULL;
    node->prev = NULL;
    if(buck->head)
    {
        node->next = buck->head;
        (buck->head)->prev = node;
    }
    buck->head = node;
    buck->ncount++;

    // clib_smp_atomic_add(&h->ncount, 1);
    // clib_smp_atomic_add(&h->weight, buck->ncount);
    // not accurate
    h->ncount += 1;
    h->weight += buck->ncount;

    if(buck->ncount==1) 
    { 
        // clib_smp_atomic_add(&h->bcount, 1);
        // not accurate
        h->bcount += 1;
        if(h->bcount>h->bmax) 
            h->bmax = h->bcount;
    }
    
    if(h->ncount>h->nmax) 
        h->nmax = h->ncount;
    
    if(buck->ncount>h->lmax) 
        h->lmax = buck->ncount;
    
    //h->ncount++;
    //h->weight+=buck->ncount;
    //if(buck->ncount==1) { h->bcount++; if(h->bcount>h->bmax) h->bmax=h->bcount; }
    //if(h->ncount>h->nmax) h->nmax = h->ncount;
    //if(buck->ncount>h->lmax) h->lmax=buck->ncount;
    return 0;
}


/*
 * BE AWARE: user should make sure that current node is in the bucket
 * or this action DOES cause mistake!!
 * */
static_always_inline void 
hashFreeNode(generic_hash_t *h, generic_pnode_t node , uint32_t hash  )
{
    generic_pbuck_t buck;
    hash = hash & (h->hashsize-1);
    buck = h->buckets + hash;

    if(node->prev == NULL && buck->head == NULL)
    {
        return;
    }

    if(node == buck->head)
    {
        buck->head = node->next;
        if(node->next)
            node->next->prev = NULL;
    }
    else
    {
        (node->prev)->next = node->next;
        if(node->next)
            (node->next)->prev = node->prev;
    }
    buck->ncount--;
    // clib_smp_atomic_add(&h->weight, -(buck->ncount+1));
    // clib_smp_atomic_add(&h->ncount, -1);
    // //h->weight-=buck->ncount+1;
    // //h->ncount--;
    // if(buck->ncount==0) //h->bcount--;
    //    clib_smp_atomic_add(&h->bcount, -1);


    //not accurate
    h->weight -= buck->ncount+1;
    h->ncount -= 1;
    if(buck->ncount==0) //h->bcount--;
       h->bcount -= 1;

}

#endif
