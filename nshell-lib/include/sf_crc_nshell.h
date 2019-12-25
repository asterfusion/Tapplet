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
#ifndef __include_sf_crc_nshell_h__
#define __include_sf_crc_nshell_h__




#include <stdint.h>
#include <vlib/vlib.h>
#include <vppinfra/clib.h>
#include <vppinfra/lock.h>
#include <vppinfra/crc32.h>

typedef uint_fast32_t crc_t;

extern const crc_t sf_crc32_table[4][256];

static_always_inline
uint32_t sf_crc32_gen(const void *data, size_t data_len)
{
    crc_t crc = 0xffffffff;
    // crc_t * sf_crc32_table = get_sf_crc32_table();
    const unsigned char *d = (const unsigned char *)data;
    unsigned int tbl_idx;

    const uint32_t *d32 = (const uint32_t *)d;
    while (data_len >= 4)
    {
#if __BYTE_ORDER == __BIG_ENDIAN
        crc_t d1 = *d32++ ^ le16toh(crc);
        crc  =
            sf_crc32_table[0][d1 & 0xffu] ^
            sf_crc32_table[1][(d1 >> 8) & 0xffu] ^
            sf_crc32_table[2][(d1 >> 16) & 0xffu] ^
            sf_crc32_table[3][(d1 >> 24) & 0xffu];
#else
        crc_t d1 = *d32++ ^ crc;
        crc  =
            sf_crc32_table[0][(d1 >> 24) & 0xffu] ^
            sf_crc32_table[1][(d1 >> 16) & 0xffu] ^
            sf_crc32_table[2][(d1 >> 8) & 0xffu] ^
            sf_crc32_table[3][d1 & 0xffu];
#endif

        data_len -= 4;
    }

    /* Remaining bytes with the standard algorithm */
    d = (const unsigned char *)d32;
    while (data_len--) {
        tbl_idx = (crc ^ *d) & 0xff;
        crc = (sf_crc32_table[0][tbl_idx] ^ (crc >> 8)) & 0xffffffff;
        d++;
    }

    // crc = crc & 0xffffffff;

    return crc ^ 0xffffffff;
}




static_always_inline uint32_t 
get_hash_index_nshell(void *key, int keysize)
{
#ifdef clib_crc32c_uses_intrinsics
    return clib_crc32c(key , keysize);
#else
    return sf_crc32_gen((void *)key , keysize);
#endif
}









#endif