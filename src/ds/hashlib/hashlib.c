/*
** Copyright (C) 2001-2004 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
** 
** Use of the SILK system and related source code is subject to the terms 
** of the following licenses:
** 
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.225-7013
** 
** NO WARRANTY
** 
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER 
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY 
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN 
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY 
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT 
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE, 
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE 
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT, 
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY 
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF 
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES. 
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF 
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON 
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE 
** DELIVERABLES UNDER THIS LICENSE.
** 
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie 
** Mellon University, its trustees, officers, employees, and agents from 
** all claims or demands made against them (and any related losses, 
** expenses, or attorney's fees) arising out of, or relating to Licensee's 
** and/or its sub licensees' negligent use or willful misuse of or 
** negligent conduct or willful misconduct regarding the Software, 
** facilities, or other rights or assistance granted by Carnegie Mellon 
** University under this License, including, but not limited to, any 
** claims of product liability, personal injury, death, damage to 
** property, or violation of any laws or regulations.
** 
** Carnegie Mellon University Software Engineering Institute authored 
** documents are sponsored by the U.S. Department of Defense under 
** Contract F19628-00-C-0003. Carnegie Mellon University retains 
** copyrights in all material produced under this contract. The U.S. 
** Government retains a non-exclusive, royalty-free license to publish or 
** reproduce these documents, or allow others to do so, for U.S. 
** Government purposes only pursuant to the copyright license under the 
** contract clause at 252.227.7013.
** 
** @OPENSOURCE_HEADER_END@
*/

/*
** 1.13
** 2004/03/11 14:19:22
** thomasm
*/

/* File: hashlib.c: implements core hash library. */

#include "hashlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <strings.h>

typedef struct _blockheader {
    uint32_t block_size;
    uint32_t num_entries;
} BlockHeader;

/* Configuration */

/* Important: minimum block size cannot be less than 256 */
uint32_t MIN_BLOCK_SIZE = (1<<8);

/* Maximum size of a hash block. Should be tweaked for a particular
 * platform.  Default if not defined in hashlib_conf.h. */
#ifndef MAX_MEMORY_BLOCK
#define MAX_MEMORY_BLOCK (1<<27)
#endif 

/* Point at which a rehash is triggered (unless we're at max block size) */
uint32_t REHASH_BLOCK_COUNT = 4;

/* Secondary table size is (table_size >> SECONDARY_BLOCK_FRACTION).
 * May also be -1 or -2, where -1 means to keep halving, and -2 means
 * to keep halving starting at a secondary block size 1/4 of the main
 * block. */
int32_t SECONDARY_BLOCK_FRACTION=-2;

/* Compute the maximum number of entries per block */
#define HASH_GET_MAX_BLOCK_ENTRIES(entry_size) ((uint32_t)(MAX_MEMORY_BLOCK/entry_size))

/* Private functions for manipulating blocks */
HashBlock *hashlib_create_block(HashTable *table_ptr,
                                       uint32_t block_size);
static void hashlib_free_block(HashBlock *block_ptr);
static int hashlib_block_find_entry(HashBlock *block_ptr,
                                    uint8_t *key_ptr,
                                    uint8_t **entry_pptr);
static int hashlib_block_lookup(HashBlock *block_ptr,
                                uint8_t *key_ptr, uint8_t **value_pptr);
static uint32_t hashlib_block_count_nonempties(HashBlock *block_ptr);
static void hashlib_dump_block(FILE *fp, HashBlock *block_ptr);
static void hashlib_dump_block_header(FILE *fp, HashBlock *table_ptr);

#ifdef HASHLIB_RECORD_STATS
uint64_t stats_total_rehashes = 0;
uint64_t stats_total_rehash_inserts = 0;
uint64_t stats_total_inserts = 0;
uint64_t stats_total_lookups = 0;
uint32_t stats_total_blocks_allocated = 0;
#endif

/* ---------------------------------------------------------------------
 * Hash function taken from:
 * http://burtleburtle.net/bob/hash/evahash.html
 */

typedef  unsigned long int  u4;   /* unsigned 4-byte type */
typedef  unsigned     char  u1;   /* unsigned 1-byte type */

/* The mixing step */
#define mix(a,b,c) \
{ \
  a=a-b;  a=a-c;  a=a^(c>>13); \
  b=b-c;  b=b-a;  b=b^(a<<8);  \
  c=c-a;  c=c-b;  c=c^(b>>13); \
  a=a-b;  a=a-c;  a=a^(c>>12); \
  b=b-c;  b=b-a;  b=b^(a<<16); \
  c=c-a;  c=c-b;  c=c^(b>>5);  \
  a=a-b;  a=a-c;  a=a^(c>>3);  \
  b=b-c;  b=b-a;  b=b^(a<<10); \
  c=c-a;  c=c-b;  c=c^(b>>15); \
}

/* The whole new hash function */
u4 hash( k, length, initval)
register u1 *k;        /* the key */
u4           length;   /* the length of the key in bytes */
u4           initval;  /* the previous hash, or an arbitrary value */
{
   register u4 a,b,c;  /* the internal state */
   u4          len;    /* how many key bytes still need mixing */

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = initval;         /* variable initialization of internal state */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a=a+(k[0]+((u4)k[1]<<8)+((u4)k[2]<<16) +((u4)k[3]<<24));
      b=b+(k[4]+((u4)k[5]<<8)+((u4)k[6]<<16) +((u4)k[7]<<24));
      c=c+(k[8]+((u4)k[9]<<8)+((u4)k[10]<<16)+((u4)k[11]<<24));
      mix(a,b,c);
      k = k+12; len = len-12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c = c+length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c=c+((u4)k[10]<<24);
   case 10: c=c+((u4)k[9]<<16);
   case 9 : c=c+((u4)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b=b+((u4)k[7]<<24);
   case 7 : b=b+((u4)k[6]<<16);
   case 6 : b=b+((u4)k[5]<<8);
   case 5 : b=b+k[4];
   case 4 : a=a+((u4)k[3]<<24);
   case 3 : a=a+((u4)k[2]<<16);
   case 2 : a=a+((u4)k[1]<<8);
   case 1 : a=a+k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}

/* ---------------------------------------------------------------------
 * End of hash function. */

/* Get the size of a key, value, or entry in bytes */
#define HASH_GET_KEY_SIZE(table_ptr) (table_ptr->key_width)
#define HASH_GET_VALUE_SIZE(table_ptr) (table_ptr->value_width)
#define HASH_GET_ENTRY_SIZE(table_ptr) (table_ptr->key_width + table_ptr->value_width)
/* Get a pointer to the entry at index hash_index */
#define HASH_GET_ENTRY(table_ptr, hash_index) (table_ptr->_data_ptr + (HASH_GET_ENTRY_SIZE(table_ptr) * hash_index))
/* Get a pointer to a key or value in an entry */
#define HASH_GET_KEY(table_ptr, entry_ptr) (entry_ptr)
#define HASH_GET_VALUE(table_ptr, entry_ptr) (entry_ptr + HASH_GET_KEY_SIZE(table_ptr))

/* Return 1 in keymatch_b_ptr if all bytes in a pair of keys match,
 * otherwise 0 */
#define HASH_COMPARE_KEYS(keymatch_b_ptr, key1_ptr, key2_ptr, key_width) { \
    int i; \
    *keymatch_b_ptr = 1; \
    for (i=0;i<key_width;i++) { \
        if (key1_ptr[i] != key2_ptr[i]) { \
          *keymatch_b_ptr = 0; \
          break; \
        } \
    }\
}

/* Return 1 in flag_ptr if the bytes match the empty value,
 * otherwise 0.*/
#define HASH_ISEMPTY(flag_ptr, table_ptr, value_ptr) { \
    int i; \
    *flag_ptr = 1; \
    for (i=0;i<table_ptr->value_width;i++) { \
      if (value_ptr[i] != table_ptr->no_value_ptr[i]) { \
            *flag_ptr = 0; \
            break; \
      } \
    } \
}

/* HASH_COMPUTE_HASH - A simple wrapper around hash() that
 * encapsulates the probing algorithm and takes care of scaling the
 * index to the block size. Before calling this function, 
 * hash_probe_increment should be set to 0. Each time it is called,
 * the probe increment is added to to the hash_value and scaled by
 * the block size (note: we don't use modulo since this is too
 * expensive, instead since sizes our powers of 2 we can mask).
 *
 * Args:
 *  index      Variable to set with the computed index
 *  key_ptr    Pointer to the key value to hash
 *  key_width  Number of bytes in the key
 *  block_size Number of bytes in the block (must be a power of 2)
 *  hash_value Variable (uint32_t) to hold the hash value
 *  hash_probe_increment Variable to hold the current probe increment
 
 * NOTE: This function is not thread safe, nor may a particular thread
 * interleave calls that are logically associated with
 * different tables. */

#define HASH_COMPUTE_HASH(index_ptr, key_ptr, key_width, block_size, hash_value, hash_probe_increment) { \
    if (hash_probe_increment == 0) {\
      hash_value = hash(key_ptr, key_width, 0);\
      hash_probe_increment = hash_value | 0x01; /* must be odd */ \
    } else {\
      hash_value = hash_value + hash_probe_increment;\
    }\
    *index_ptr = hash_value & ((uint32_t) block_size-1);\
    }

/* Choose the size for initial block. This will be a power of 2 with at
 * least MIN_BLOCK_SIZE entries that accomodates the data at a load 
 * less than the given load factor. */
uint32_t hashlib_calculate_block_size(uint32_t entry_width, uint8_t load_factor,
                                    uint32_t estimated_count) {
    uint32_t initial_size;
    uint32_t max_size = HASH_GET_MAX_BLOCK_ENTRIES(entry_width);

    if (estimated_count < MIN_BLOCK_SIZE) {
      initial_size = MIN_BLOCK_SIZE;
    } else {
      initial_size = MIN_BLOCK_SIZE<<1;
      while (initial_size <= max_size) {
        if (initial_size >= (((uint64_t) estimated_count)<<8)/load_factor)
          break; /* big enough */
        initial_size = initial_size << 1; /* double it */
      }
    }
    return initial_size;
}

HashTable *hashlib_create_table(uint8_t key_width,
                                uint8_t value_width,
                                uint8_t value_type,
                                uint8_t *no_value_ptr,
                                uint8_t *appdata_ptr,
                                uint32_t appdata_size,
                                uint32_t estimated_count,
                                uint8_t load_factor) {
    HashTable *table_ptr = NULL;
    HashBlock *block_ptr = NULL;
    uint32_t initial_size;

    /* Validate arguments */
    assert(value_type==HTT_INPLACE||value_type==HTT_BYREFERENCE);
    if ((value_type!=HTT_INPLACE) && (value_type!=HTT_BYREFERENCE)) {
      LOG0(stderr, "hashlib_create_table: invalid value_type argument.\n");
      assert(0);
      return NULL;
    }

    /* Calculate initial block size */
    initial_size = hashlib_calculate_block_size(key_width + value_width,
                                                load_factor, estimated_count);

    /* Allocate memory for the table and initialize attributes.  */
    table_ptr = (HashTable*) malloc(sizeof(HashTable));
    assert(table_ptr);
    if (table_ptr == NULL) {
      return NULL;
    }

    /* Initialize the table structure */
    table_ptr->value_type = value_type;
    table_ptr->key_width = key_width;
    table_ptr->value_width = value_width;
    table_ptr->load_factor = load_factor;
    /* Application data */
    table_ptr->appdata_ptr = appdata_ptr;
    table_ptr->appdata_size = appdata_size;
    /* Initialize zero value ptr to string of zero-valued bytes if NULL */
    if (no_value_ptr == NULL) {
        table_ptr->no_value_ptr = (uint8_t*) malloc(table_ptr->value_width);
        bzero(table_ptr->no_value_ptr, table_ptr->value_width);
    } else {
        table_ptr->no_value_ptr = (uint8_t*) malloc(table_ptr->value_width);
        memcpy(table_ptr->no_value_ptr, no_value_ptr, table_ptr->value_width);
    }

    /* Start with one block */
    table_ptr->num_blocks = 1;
    block_ptr = hashlib_create_block(table_ptr, initial_size);
    if (block_ptr == NULL) {
      return NULL;
    }
    table_ptr->block_ptrs[0] = block_ptr;

    return table_ptr;
}

/* Note: doesn't free app data.  This is the responsibility of the
 * caller (e.g., the wrapper) */
void hashlib_free_table(HashTable *table_ptr) {
    int i;
    /* Free all the blocks in the table */
    for (i=0;i<table_ptr->num_blocks;i++) {
      hashlib_free_block(table_ptr->block_ptrs[i]);
    }

    /* Free the empty pointer memory */
    free(table_ptr->no_value_ptr);
    
    /* Free the table structure itself */
    free(table_ptr);
}

/* NOTE: Assumes block_size is a power of 2.  Very important! */
HashBlock *hashlib_create_block(HashTable *table_ptr,
                                       uint32_t block_size) {
    HashBlock *block_ptr;
    uint32_t entry_i;

#ifdef HASHLIB_RECORD_STATS
    stats_total_blocks_allocated++;
#endif

    LOG1(stderr, "Create block.  Requesting size = %d\n", block_size);

    /* Allocate memory for the block and initialize attributes.  */
    block_ptr = (HashBlock*) malloc(sizeof(HashBlock));
    assert(block_ptr);
    if (block_ptr == NULL) return NULL;

    block_ptr->value_type = table_ptr->value_type;
    block_ptr->num_entries = 0;
    block_ptr->key_width = table_ptr->key_width;
    block_ptr->value_width = table_ptr->value_width;
    block_ptr->load_factor = table_ptr->load_factor;
    block_ptr->block_size = block_size;

    /* Initialize zero value ptr to string of zero-valued bytes if NULL */
    if (table_ptr->no_value_ptr == NULL) {
        block_ptr->no_value_ptr = (uint8_t*) malloc(table_ptr->value_width);
        bzero(block_ptr->no_value_ptr, table_ptr->value_width);
    } else {
        block_ptr->no_value_ptr = table_ptr->no_value_ptr;
    }

    LOG3(stderr,
            "Allocating %d data records of %d bytes each (%d total bytes)\n",
            block_size,
            HASH_GET_ENTRY_SIZE(block_ptr),
            HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->block_size);

    /* Allocate memory for the data */
    block_ptr->_data_ptr =
        (uint8_t*) malloc(HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->block_size);
    assert(block_ptr->_data_ptr);
    if (block_ptr->_data_ptr == NULL) return NULL;


    /* Copy "empty" value to each entry.  Garbage key values are
     * ignored, so we don't bother writing to the keys.  When the
     * application overestimates the amount of memory needed, this can
     * be bottleneck.  */
      {
        uint8_t *data_ptr = block_ptr->_data_ptr;
        uint8_t entry_width = HASH_GET_ENTRY_SIZE(block_ptr);
        /* Point to first value */
        data_ptr += block_ptr->key_width;
        for (entry_i=0;entry_i<block_ptr->block_size;entry_i++) {
          memcpy(data_ptr, block_ptr->no_value_ptr, block_ptr->value_width);
          /* Advance to next */
          data_ptr += entry_width;
        }
      }

    return block_ptr;
}

static void hashlib_free_block(HashBlock *block_ptr) {
    /* Free the data and the table itself */
    free(block_ptr->_data_ptr);
    free(block_ptr);
}

/* Rehash entire table into a single block */
int hashlib_rehash(HashTable *table_ptr) {
    HashBlock *new_block_ptr = NULL;
    HashBlock *block_ptr = NULL;
    uint32_t num_entries = 0;
    uint32_t initial_size;
    uint8_t *key_ref;
    uint8_t *val_ref;
    uint8_t *entry_ptr;
    uint8_t isempty_b;
    uint8_t *new_entry_ptr;
    int rv;
    int i;
    uint32_t block_index, index;
    
#ifdef HASHLIB_RECORD_STATS
    stats_total_rehashes++;
#endif

    LOG0(stderr, "Rehashing table.\n");

    /* Count the total number of entries so we know what we need to
     * allocate. We base this on the actual size of the blocks, and
     * use the power of 2 that's double the smallest power of 2 bigger
     * than the sum of block sizes. It's justified by the intuition
     * that once we reach this point, we've decided that we're going
     * to explore an order of magnitude larger table. This particular
     * scheme seems to work well in practice although it's difficult
     * to justify theoretically--this is a rather arbitrary definition
     * of "order of magnitude". */
    for (i=0;i<table_ptr->num_blocks;i++) {
      num_entries += table_ptr->block_ptrs[i]->block_size;
    }
    /* .. and add the padding we need. */
    num_entries = (uint32_t) ((((uint64_t) num_entries) * 255)
                            /table_ptr->load_factor);
    /* Choose the size for the initial block. */
    initial_size = MIN_BLOCK_SIZE; /* Minimum size */
    for (i=0;i<24;i++) {
      if (initial_size >= num_entries) break; /* big enough */
      initial_size = initial_size << 1; /* double it */
    }
    initial_size = initial_size << 1;
    
    /* Create the new block */
    new_block_ptr = hashlib_create_block(table_ptr, initial_size);
    
    /* Walk through the table looking for non-empty entries, and
     * insert them into the new block. */
    for (block_index=0;block_index<table_ptr->num_blocks;block_index++) {
      block_ptr = table_ptr->block_ptrs[block_index];
      entry_ptr = HASH_GET_ENTRY(block_ptr, 0);
      for (index=0;index<block_ptr->block_size;index++) {
        key_ref = HASH_GET_KEY(block_ptr, entry_ptr);
        val_ref = HASH_GET_VALUE(block_ptr, entry_ptr);
        HASH_ISEMPTY(&isempty_b, block_ptr, val_ref);

        /* If not empty, then copy the entry into the new block */
        if (!isempty_b) {
            rv = hashlib_block_find_entry(new_block_ptr,
                                          key_ref, &new_entry_ptr);
          
            assert(rv == ERR_NOTFOUND);
            if (rv != ERR_NOTFOUND) {
              return ERR_INTERNALERROR;
            }
            
            /* Copy the key and value */
            memcpy(HASH_GET_KEY(new_block_ptr, new_entry_ptr),
                   key_ref, block_ptr->key_width);
            memcpy(HASH_GET_VALUE(new_block_ptr, new_entry_ptr),
                   val_ref, block_ptr->value_width);
            new_block_ptr->num_entries++;

#ifdef HASHLIB_RECORD_STATS
            stats_total_rehash_inserts++;
#endif
        } /* !isempty_b */
        entry_ptr += HASH_GET_ENTRY_SIZE(block_ptr);
      } /* entries */
      /* Free the block */
      hashlib_free_block(block_ptr);
    } /* blocks */
    
    /* Associate the new block with the table */
    table_ptr->num_blocks = 1;
    table_ptr->block_ptrs[0] = new_block_ptr;

    LOG0(stderr, "Rehashed table.\n");

    return OK;
}

/* Add a new block to a table. */
static int hashlib_add_block(HashTable *table_ptr, uint32_t new_block_size) {
    HashBlock *block_ptr = NULL;

    assert(table_ptr->num_blocks < MAX_BLOCKS);
    if (table_ptr->num_blocks > MAX_BLOCKS) {
      LOG0(stderr, "FATAL ERROR: Cannot allocate another block.\n");
      return ERR_OUTOFMEMORY;
    }

    /* Create the new block */
    LOG1(stderr, "Adding block of size %u\n", new_block_size);
    block_ptr = hashlib_create_block(table_ptr, new_block_size);
    assert(block_ptr);
    if (block_ptr==NULL) return ERR_OUTOFMEMORY;

    /* Add it to the table */
    table_ptr->block_ptrs[table_ptr->num_blocks] = block_ptr;
    table_ptr->num_blocks++;
    LOG0(stderr, "Added block.\n");

    return OK;
}

/* See what size the next hash block should be. */
uint32_t hashlib_compute_next_block_size(HashTable *table_ptr) {
    uint32_t block_size = 0;

    /* This condition will only be true when the primary block has
     * reached the maximum block size. */
    if (table_ptr->num_blocks >= REHASH_BLOCK_COUNT) {
        return table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size;
    }

    /* Otherwise, it depends on current parameters */
    if (SECONDARY_BLOCK_FRACTION >= 0) {
      block_size = (table_ptr->block_ptrs[0]->block_size)
          >>SECONDARY_BLOCK_FRACTION;
    } else {
      if (SECONDARY_BLOCK_FRACTION == -1) {
        /* Keep halving blocks */
        block_size =
            (table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size)>>1;
      } else if (SECONDARY_BLOCK_FRACTION == -2) {
        if (table_ptr->num_blocks==1) {
          /* First secondary block is 1/4 size of main block */
          block_size =
              (table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size)>>2;
        } else {
          /* Other secondary blocks are halved */
          block_size =
              (table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size)>>1;
        }
      }
    }

    return block_size;
}

/* Algorithm:
   - If the primary block is at its maximum, never rehash, only add
     new blocks.
   - If we have a small table, then don't bother creating
     secondary tables.  Simply rehash into a new block.
   - If we've exceeded the maximum number of blocks, rehash
     into a new block.
   - Otherwise, create a new block
*/

static int hashlib_resize_table(HashTable *table_ptr) {

    /* Compute the (potential) size of the new block */
    uint32_t new_block_size = hashlib_compute_next_block_size(table_ptr);

    /* If we're at the maximum number of blocks (which implies that
     * the first block is at its max, and we can't resize, then that's
     * it. */
    if (table_ptr->num_blocks == MAX_BLOCKS) {
        return ERR_NOMOREBLOCKS;
    }

    /* If the first block is at its maximum size, then add a new block.
    * Once we reach the maximum block size, we don't rehash.  Instead
    * we keep adding blocks until we reach the maximum. */
    if (table_ptr->block_ptrs[0]->block_size ==
        HASH_GET_MAX_BLOCK_ENTRIES(HASH_GET_ENTRY_SIZE(table_ptr))) {
      assert (new_block_size > MIN_BLOCK_SIZE);
      return hashlib_add_block(table_ptr, new_block_size);
    }

    /* If we have REHASH_BLOCK_COUNT blocks, or the new block would be
     * too small, we simply rehash. */
    if ((new_block_size < MIN_BLOCK_SIZE) ||
        (table_ptr->num_blocks >= REHASH_BLOCK_COUNT)) {
      return hashlib_rehash(table_ptr);
    }

    /* Assert several global invariants */
    assert(new_block_size >= MIN_BLOCK_SIZE);
    assert(new_block_size <=
           HASH_GET_MAX_BLOCK_ENTRIES(HASH_GET_ENTRY_SIZE(table_ptr)));
    assert(table_ptr->num_blocks < MAX_BLOCKS);
    
    /* Otherwise, add new a new block */
    return hashlib_add_block(table_ptr, new_block_size);    
}


void assert_not_already_there(HashTable *table_ptr, uint8_t *key_ptr) {
    uint8_t *entry_ptr;
    HashBlock *block_ptr;
    int i;
    int rv;
    
    for (i=0;i<(table_ptr->num_blocks-1);i++) {
      block_ptr = table_ptr->block_ptrs[i];
      rv = hashlib_block_find_entry(block_ptr, key_ptr, &entry_ptr);
      if (rv == OK) {
        getc(stdin);
      }
    }
}

#define HASHLIB_BLOCK_IS_FULL(table_ptr, block_ptr) (((block_ptr->num_entries+1)/((block_ptr->block_size)>>8))> block_ptr->load_factor)

int hashlib_insert(HashTable *table_ptr, uint8_t *key_ptr, uint8_t **value_pptr) {
    uint32_t i;
    int rv;
    HashBlock *block_ptr;
    uint8_t *current_entry_ptr;

#ifdef HASHLIB_RECORD_STATS    
    stats_total_inserts++;
#endif

    /* Before doing anything else, see if we're ready to do a resize
     * by either adding a block or rehashing. */
    if (HASHLIB_BLOCK_IS_FULL(table_ptr,
                              table_ptr->block_ptrs[table_ptr->num_blocks-1])) {
        rv = hashlib_resize_table(table_ptr);
        if (rv != OK) return rv;
    }

    /* Look for the key in all the blocks but the last block */
    for (i=0;i<(uint8_t)(table_ptr->num_blocks-1);i++) {
      block_ptr = table_ptr->block_ptrs[i];
      rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
      if (rv == OK) {
        /* Found entry, use it */
        *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
        return OK_DUPLICATE;
      } else if (rv != ERR_NOTFOUND) {
        assert(0);
        /* An error of some sort occurred */
        return rv; 
      }
    }

    /* It wasn't found in the first n-1 blocks. Now, deal with the
     * last block */
    block_ptr = table_ptr->block_ptrs[i];
    
    /* See if it's in the last block.  If so, set the value pointer
     * and return. */
    rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
    if (rv == OK) {
      /* Found entry, use it */
      *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
      return OK_DUPLICATE;
    } else if (rv != ERR_NOTFOUND) {
      /* An error occurred during lookup */
      assert(0);
      return rv; 
    }

    /* Not in the last block, so do an insert into the last block by
     * setting the key AND increasing the count. The caller will set
     * the value. */
    *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
    memcpy(HASH_GET_KEY(block_ptr, current_entry_ptr),
           key_ptr, block_ptr->key_width);
    block_ptr->num_entries++;

    return OK;
}



#if 0
/* NOTE: Since we're returning a reference to the value, the user
 * could mistakenly set the value to "empty" (e.g., put a null pointer
 * in an allocated entry).  This is problematic, since the internal
 * count will have been incremented even though in essence no entry
 * has been added.  I don't know the proper solution. */
static int hashlib_block_insert(HashBlock *block_ptr,
                                uint8_t *key_ptr, uint8_t **value_pptr) {
    uint8_t *current_entry_ptr = NULL;
    int rv;

    /* Assert block won't exceed load factor */
    assert(!((block_ptr->num_entries/((block_ptr->block_size)>>8)) >
             block_ptr->load_factor));

    /* Find a slot with the specified key, or a free slot in the hash
     * table */
    rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
    if (rv == OK) {
      /* Return pointer to the value in the entry structure */
      *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
      return OK_DUPLICATE;
    } else if (rv == ERR_NOTFOUND) {
      /* Got an empty entry back -- fill it the key */
      memcpy(HASH_GET_KEY(block_ptr, current_entry_ptr),
             key_ptr, block_ptr->key_width); 
      /* Return pointer to the value in the entry structure */
      *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
      /* Update the number of entries */
      (block_ptr->num_entries)++;
      return OK;
    } else {
      return rv;
    }
}

#endif


int hashlib_lookup(HashTable *table_ptr, uint8_t *key_ptr, uint8_t **value_pptr) {
    uint8_t i;

#ifdef HASHLIB_RECORD_STATS
    stats_total_lookups++;
#endif
    
    /* Look in each block for the entry */
    for (i=0;i<table_ptr->num_blocks;i++) {
      if (hashlib_block_lookup(table_ptr->block_ptrs[i],
                               key_ptr, value_pptr)==OK) return OK;
    }
    return ERR_NOTFOUND;
}

static int hashlib_block_lookup(HashBlock *block_ptr,
                                uint8_t *key_ptr,
                                uint8_t **value_pptr) {
    uint8_t *current_entry_ptr = NULL;
    int rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
    if (rv == OK) {
      /* Return pointer to the value in the entry structure */
      *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
      return OK;
    } else {
      return ERR_NOTFOUND;
    }

}

/* If not found, points to insertion point */
static int hashlib_block_find_entry(HashBlock *block_ptr,
                                    uint8_t *key_ptr,
                                    uint8_t **entry_pptr) {
    uint32_t num_tries = 0;
    uint32_t hash_index = 0;
    uint8_t *current_entry_ptr = NULL;
    uint8_t isempty_b;
    uint8_t keymatch_b;
    uint32_t hash_value = 0;
    uint32_t hash_probe_increment = 0;

    /* Loop until we find the correct entry in this hash sequence, or
     * encounter an empty entry. */
    do {
        HASH_COMPUTE_HASH(&hash_index, key_ptr, block_ptr->key_width,
                        block_ptr->block_size,
                        hash_value, hash_probe_increment);
        
        current_entry_ptr = HASH_GET_ENTRY(block_ptr, hash_index);

        /* Hit an empty entry, we're done. */
        HASH_ISEMPTY(&isempty_b, block_ptr,
                     HASH_GET_VALUE(block_ptr, current_entry_ptr));
        if (isempty_b) {
            *entry_pptr = current_entry_ptr;
            return ERR_NOTFOUND;
        }

        assert(++num_tries < block_ptr->block_size);

        HASH_COMPARE_KEYS(&keymatch_b,
                          HASH_GET_KEY(block_ptr, current_entry_ptr),
                          key_ptr, block_ptr->key_width);
    } while(!keymatch_b);

    /* Found it. */
    *entry_pptr = current_entry_ptr; 
    return OK;
}

static uint32_t hashlib_block_count_nonempties(HashBlock *block_ptr) {
    uint32_t i;
    uint32_t count = 0;
    uint8_t *entry_ptr;
    uint8_t isempty_b;
    for (i=0;i<block_ptr->block_size;i++) {
        entry_ptr = HASH_GET_ENTRY(block_ptr, i);
        HASH_ISEMPTY(&isempty_b, block_ptr,
                     HASH_GET_VALUE(block_ptr, entry_ptr));
        if (!isempty_b) 
          count++;
    }
    return count;
}

void hashlib_dump_bytes(FILE *fp, uint8_t *data_ptr, uint32_t data_size) {
    uint32_t j;
    for (j=0;j<data_size;j++) {
        fprintf(fp, "%02x ", data_ptr[j]);
    }
}

void hashlib_dump_block_header(FILE *fp, HashBlock *block_ptr) {
    /* Dump header info */
    fprintf(fp, "Table size: \t %u\n", block_ptr->block_size);
    fprintf(fp, "Num entries:\t %u (%2.0f%% full)\n", block_ptr->num_entries,
           100*(float)block_ptr->num_entries/block_ptr->block_size);
    fprintf(fp, "Key width:\t %d bytes\n", block_ptr->key_width);
    fprintf(fp, "Value width:\t %d bytes\n", block_ptr->key_width);
    fprintf(fp, "Load factor:\t %d = %2.0f%%\n",
           block_ptr->load_factor,
           100*(float)block_ptr->load_factor/255);
    fprintf(fp, "Empty value representation: ");
    hashlib_dump_bytes(fp, block_ptr->no_value_ptr, block_ptr->value_width);
    fprintf(fp, "\n");
}

void hashlib_dump_block(FILE *fp, HashBlock *block_ptr) {
    uint32_t i; /* Index of into hash table */
    uint32_t entry_index = 0;
    uint8_t *entry_ptr;
    uint8_t isempty_b;

    hashlib_dump_block_header(fp, block_ptr);
    fprintf(fp, "Data Dump:\n");
    fprintf(fp, "----------\n");
    for (i=0;i<block_ptr->block_size;i++) {
        entry_ptr = HASH_GET_ENTRY(block_ptr, i);
        /* Don't dump empty entries */
        HASH_ISEMPTY(&isempty_b, block_ptr,
                     HASH_GET_VALUE(block_ptr, entry_ptr));
        if (isempty_b)
            continue;
        else
            entry_index++;
        
        /* Dump hash index in table, the key and the value */
        fprintf(fp, "%6d (%d). ", entry_index, i);
        hashlib_dump_bytes(fp, HASH_GET_KEY(block_ptr, entry_ptr),
                           block_ptr->key_width);
        fprintf(fp, " -- ");
        hashlib_dump_bytes(fp, HASH_GET_VALUE(block_ptr, entry_ptr),
                           block_ptr->value_width);
        fprintf(fp, "\n");
    }
}
#ifdef HASHLIB_RECORD_STATS
void hashlib_clear_stats() {
    stats_total_inserts = 0;
    stats_total_lookups = 0;
    stats_total_rehashes = 0;
    stats_total_rehash_inserts =0;
    stats_total_blocks_allocated =0;
}

void hashlib_dump_stats(FILE *fp) {
    fprintf(fp, "Accumulated statistics:\n");
    fprintf(fp, "  %llu total inserts.\n", stats_total_inserts);
    fprintf(fp, "  %llu total lookups.\n", stats_total_lookups);
    fprintf(fp, "  %llu total rehashes.\n", stats_total_rehashes);
    fprintf(fp, "  %llu inserts due to rehashing.\n",
            stats_total_rehash_inserts);
}
#endif

HASH_ITER hashlib_create_iterator(HashTable *table_ptr) {
    HASH_ITER iter;
    assert(table_ptr);
    iter.block = HASH_ITER_BEGIN;
    iter.index = 0;
    return iter;
}


int hashlib_iterate(HashTable *table_ptr, HASH_ITER *iter_ptr,
                    uint8_t **key_pptr, uint8_t **val_pptr) {
    HashBlock *block_ptr;
    uint8_t isempty_b;
    static uint32_t so_far = 0;
    
    if (iter_ptr->block == HASH_ITER_END) {
        return ERR_NOMOREENTRIES;
    }
    
    /* Start at the first entry in the first block or increment the
     * iterator to start looking at the next entry. */
    if (iter_ptr->block == HASH_ITER_BEGIN) { 
      iter_ptr->block = 0;
      iter_ptr->index = 0;
      iter_ptr->current_entry_ptr =
          HASH_GET_ENTRY(table_ptr->block_ptrs[0], 0);
    } else {
      (iter_ptr->index)++;
      (iter_ptr->current_entry_ptr) += HASH_GET_ENTRY_SIZE(table_ptr);
    }

    /* Walk through indices of current block until we find a
     * non-empty.  Once we reach the end of the block, move on to the
     * next block. */
    while (iter_ptr->block < table_ptr->num_blocks) {
    
      /* Select the current block */
      block_ptr = table_ptr->block_ptrs[iter_ptr->block];
      
      /* Find the next non-empty entry in the current block (if there
       * is one). */
      while (iter_ptr->index < block_ptr->block_size) {
        HASH_ISEMPTY(&isempty_b, block_ptr,
                     HASH_GET_VALUE(table_ptr, iter_ptr->current_entry_ptr));
        if (!isempty_b) {
          break;
        }
        /* Move on to the next entry */
        (iter_ptr->index)++;
        (iter_ptr->current_entry_ptr) += HASH_GET_ENTRY_SIZE(table_ptr);
      }

      /* At the end of the block. */
      if (iter_ptr->index >= block_ptr->block_size) {
        /* We're past the last entry of the last block, so we're
         * done. */
        if (iter_ptr->block == (table_ptr->num_blocks-1)) {
          *key_pptr = NULL;
          *val_pptr = NULL;
          iter_ptr->block = HASH_ITER_END;
          LOG1(stderr, "Iterate. No more entries. So far %u\n", so_far);
          return ERR_NOMOREENTRIES;
        } 
      } else {
        /* We found an entry, return it */
        *key_pptr = HASH_GET_KEY(table_ptr, iter_ptr->current_entry_ptr);
        *val_pptr = HASH_GET_VALUE(table_ptr, iter_ptr->current_entry_ptr);
        so_far++;
        return OK;
      }

      LOG1(stderr, "Iterate. Moving to next block. So far %u\n", so_far);
      so_far = 0;

      /* try the next block */
      iter_ptr->block++;
      iter_ptr->index = 0;
      iter_ptr->current_entry_ptr =
          HASH_GET_ENTRY(table_ptr->block_ptrs[iter_ptr->block], 0);

    } /* end loop through blocks */

    /* Should never happen.  The compiler doesn't have enough info. to
     * know this. */
    *key_pptr = NULL;
    *val_pptr = NULL;
    iter_ptr->block = HASH_ITER_END;
    return ERR_NOMOREENTRIES;
    
}

int32_t hashlib_count_buckets(HashTable *table_ptr) {
    int i;
    uint32_t total = 0;
    for (i=0;i<table_ptr->num_blocks;i++) {
      total = total + table_ptr->block_ptrs[i]->block_size;
    }
    return total;
}

uint32_t hashlib_count_entries(HashTable *table_ptr) {
    int i;
    uint32_t total = 0;
    for (i=0;i<table_ptr->num_blocks;i++) {
      total = total + table_ptr->block_ptrs[i]->num_entries;
    }
    return total;
}

uint32_t hashlib_count_nonempties(HashTable *table_ptr) {
    int i;
    uint32_t total = 0;
    for (i=0;i<table_ptr->num_blocks;i++) {
        uint32_t bc = hashlib_block_count_nonempties(table_ptr->block_ptrs[i]);
        total = total + bc;
        LOG2(stderr, "nonempty count for block %d is %u\n", i, bc);
    }
    return total;

}

void hashlib_dump_table(FILE *fp, HashTable *table_ptr) {
    int i;
    hashlib_dump_table_header(fp, table_ptr);
    for (i=0;i<table_ptr->num_blocks;i++) {
      fprintf(fp, "Block %d:\n", i);
      hashlib_dump_block(fp, table_ptr->block_ptrs[i]);
    }
}

void hashlib_dump_table_header(FILE *fp, HashTable *table_ptr) {
    int i;
    HashBlock *block_ptr;
    uint32_t total_used_memory = 0;
    uint32_t total_data_memory = 0;
    
    /* Dump header info */
    fprintf(fp, "Key width:\t %d bytes\n", table_ptr->key_width);
    fprintf(fp, "Value width:\t %d bytes\n", table_ptr->value_width);
    if (table_ptr->value_type == HTT_INPLACE) {
      fprintf(fp, "Value type:\t in-place value\n");
    } else if (table_ptr->value_type == HTT_BYREFERENCE) {
      fprintf(fp, "Value type:\t reference\n");
    } else {
      fprintf(fp, "Value type:\t #ERROR\n");
    }
    fprintf(fp, "Empty value:\t" );
    hashlib_dump_bytes(fp, table_ptr->no_value_ptr, table_ptr->value_width);
    fprintf(fp, "\n");
    fprintf(fp, "App data size:\t %d bytes\n", table_ptr->appdata_size);
    fprintf(fp, "Load factor:\t %d = %2.0f%%\n",
          table_ptr->load_factor,
           100*(float)table_ptr->load_factor/255);
    fprintf(fp, "Table has %u blocks:\n", table_ptr->num_blocks);
    for (i=0;i<table_ptr->num_blocks;i++) {
      block_ptr = table_ptr->block_ptrs[i];
      total_data_memory +=
          HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->block_size;
      total_used_memory +=
          HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->num_entries;
      if (i != 0) fprintf(fp, ", ");
      fprintf(fp, "  Block #%d: %u/%u (%3.1f%%)",
             i,
             block_ptr->num_entries,
             block_ptr->block_size,
             100*((float) block_ptr->num_entries)/block_ptr->block_size);
    }
    fprintf(fp, "\n");
    fprintf(fp, "Total data memory:           %u bytes\n",
           total_data_memory);
    fprintf(fp, "Total allocated data memory: %u bytes\n",
           total_used_memory);
    fprintf(fp, "Excess data memory:          %u bytes\n",
           total_data_memory - total_used_memory);
    fprintf(fp, "\n");
}

int hashlib_write_block_header(HashBlock *block_ptr, FILE *output_fp) {
    size_t write_count;
    
    /* Write out the block attributes */
    write_count = fwrite(&(block_ptr->block_size),
                        sizeof(block_ptr->block_size), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;
    
    write_count = fwrite(&(block_ptr->num_entries),
                        sizeof(block_ptr->num_entries), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;
    return OK;
}

int hashlib_write_block(HashBlock *block_ptr, FILE *output_fp) {
    size_t write_count;
    /* Write out the actual data */
    write_count = fwrite(block_ptr->_data_ptr,
                         block_ptr->value_width + block_ptr->key_width,
                         block_ptr->block_size, output_fp);
    assert(write_count==block_ptr->block_size);
    if (write_count != block_ptr->block_size) return ERR_FILEWRITEERROR;

    return OK;
}

int hashlib_read_block_header(BlockHeader *header_ptr, FILE *input_fp) {
    size_t read_count;
    /* Read block attributes */
    read_count = fread(&(header_ptr->block_size), sizeof(header_ptr->block_size),
                       1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    read_count = fread(&(header_ptr->num_entries), sizeof(header_ptr->num_entries),
                       1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    return OK;
}

int hashlib_read_block(HashBlock **block_pptr, HashTable *table_ptr,
                       BlockHeader *block_header_ptr, FILE *input_fp) {

    HashBlock *block_ptr;
    size_t read_count;
    
    /* Create a new block to hold the data */
    block_ptr = hashlib_create_block(table_ptr, block_header_ptr->block_size); 
    block_ptr->num_entries = block_header_ptr->num_entries;
    assert(block_ptr != NULL);
    if (block_ptr == NULL) {
      return ERR_OUTOFMEMORY;
    }

    /* Read the table data right into the array directly */
    read_count = fread(block_ptr->_data_ptr,
                       table_ptr->key_width + table_ptr->value_width,
                       block_ptr->block_size,
                       input_fp);
    assert(read_count == block_ptr->block_size);
    if (read_count != block_ptr->block_size) return ERR_FILEREADERROR;
    
    /* Return the pointer to the block */
    *block_pptr = block_ptr;

    return OK;
}

int hashlib_save_table(HashTable *table_ptr, char *filename_str,
                       uint8_t *header_ptr, uint8_t header_length) {
    /* Open up a file for writing and serialize the data to the file */
    int rv;
    FILE *output_fp = fopen(filename_str, "w");
    if (output_fp == NULL) {
        return ERR_FILEOPENERROR;
    }
    rv = hashlib_serialize_table(table_ptr, output_fp,
                                 header_ptr, header_length);

    fclose(output_fp);
    return rv;
}

int hashlib_restore_table(HashTable **table_pptr, char *filename_str,
                          uint8_t *header_ptr, uint8_t header_length) {
    int rv;
    /* Open up a file for reading and deserialize the data from the file */
    FILE *input_fp = fopen(filename_str, "r");
    if (input_fp == NULL) {
        return ERR_FILEOPENERROR;
    }
    rv = hashlib_deserialize_table(table_pptr, input_fp,
                                   header_ptr, header_length);
    fclose(input_fp);
    return rv;
}

int hashlib_serialize_table(HashTable *table_ptr, FILE *output_fp,
                            uint8_t *header_ptr, uint8_t header_length) {
    int rv = OK;
    int block_index;
    size_t write_count;

    /* For the moment, we only support serialization of values, not
     * references. */
    assert(table_ptr->value_type == HTT_INPLACE);
    if (table_ptr->value_type != HTT_INPLACE) {
        LOG0(stderr,
                "FATAL ERROR.  Reference serialization not yet implemented.");
        return ERR_NOTSUPPORTED;
    }

    /* Write out the generic header (if any) */
    write_count = fwrite(header_ptr, 1, header_length, output_fp);
    assert(write_count==header_length); 
    if (write_count != header_length) return ERR_FILEWRITEERROR;

    /* Write out table attributes */
    write_count = fwrite(&(table_ptr->key_width),
                         sizeof(table_ptr->key_width), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;
    write_count = fwrite(&(table_ptr->value_width),
                         sizeof(table_ptr->value_width), 1, output_fp);
        assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;
    write_count = fwrite(&(table_ptr->load_factor),
                         sizeof(table_ptr->load_factor), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;

    /* Write out representation of no value */
    write_count = fwrite(table_ptr->no_value_ptr, table_ptr->value_width, 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;

    /* Write out app data length */
    write_count = fwrite(&(table_ptr->appdata_size),
                         sizeof(table_ptr->appdata_size), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;

    /* Write out the app data */
    write_count = fwrite(table_ptr->appdata_ptr,
                         1, table_ptr->appdata_size, output_fp);
    assert(write_count==table_ptr->appdata_size);
    if (write_count != table_ptr->appdata_size) return ERR_FILEWRITEERROR;

    /* Write out number of blocks */
    write_count = fwrite(&(table_ptr->num_blocks),
                         sizeof(table_ptr->num_blocks), 1, output_fp);
    assert(write_count==1);
    if (write_count != 1) return ERR_FILEWRITEERROR;

    /* Write the block headers */
    for (block_index=0;block_index<MAX_BLOCKS;block_index++) {
        if (block_index < table_ptr->num_blocks) {
            rv = hashlib_write_block_header(table_ptr->block_ptrs[block_index],
                                            output_fp);
            if (rv != OK) {
              return rv;
            }
        } else {
            BlockHeader hdr;
            bzero((uint8_t*) &hdr, sizeof(BlockHeader));
            write_count = fwrite(&hdr, sizeof(hdr), 1, output_fp);
            assert(write_count==1);
            if (write_count != 1) return ERR_FILEWRITEERROR;
        }
    }
        
    /* Write each of the blocks, one after the other */
    for (block_index=0;block_index<table_ptr->num_blocks;block_index++) {
        rv = hashlib_write_block(table_ptr->block_ptrs[block_index], output_fp);
        if (rv != OK) return rv;
    }

    return OK;
}

int hashlib_deserialize_table(HashTable **table_pptr, FILE *input_fp,
                              char *header_ptr, int header_length) {
    HashTable *table_ptr;
    uint8_t key_width;
    uint8_t value_width;
    uint8_t load_factor;
    uint32_t appdata_size;
    uint8_t *no_value_ptr;
    void *appdata_ptr;
    int block_index;
    BlockHeader block_headers[MAX_BLOCKS];
    size_t read_count;
    
    assert(input_fp);

    /* Read the header (the caller will interpret it) */
    read_count = fread(header_ptr, 1, header_length, input_fp);
    assert(read_count==(size_t) header_length);
    if (read_count != (size_t) header_length) return ERR_FILEREADERROR;

    /* Read the table attributes */
    read_count = fread(&(key_width), sizeof(key_width), 1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    read_count = fread(&(value_width), sizeof(value_width), 1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    read_count = fread(&(load_factor), sizeof(load_factor), 1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    
    /* Read representation of an "empty" value */
    no_value_ptr = (uint8_t*) malloc(value_width);
    assert(no_value_ptr);
    if (no_value_ptr == NULL) {
        return ERR_OUTOFMEMORY;
    }
    read_count = fread(no_value_ptr, value_width, 1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;
    
    /* Read size of app data */
    read_count = fread(&(appdata_size), sizeof(appdata_size), 1, input_fp);
    assert(read_count==1);
    if (read_count != 1) return ERR_FILEREADERROR;

    /* Allocate memory for the application data */
    appdata_ptr = (void *) malloc(appdata_size);
    if (appdata_ptr == NULL) {
      return ERR_OUTOFMEMORY;
    }

    /* Read application data */
    read_count = fread(appdata_ptr, 1, appdata_size, input_fp);
    assert(read_count==appdata_size);
    if (read_count != appdata_size) return ERR_FILEREADERROR;

    /* Create a table structure using the values we just read */
    /* Allocate memory for the table and initialize attributes.  */
    table_ptr = (HashTable*) malloc(sizeof(HashTable));
    assert(table_ptr);
    if (table_ptr == NULL) {
      return ERR_OUTOFMEMORY;
    }
    /* Initialize the table structure */
    table_ptr->value_type = HTT_INPLACE;
    table_ptr->key_width = key_width;
    table_ptr->value_width = value_width;
    table_ptr->load_factor = load_factor;
    /* Application data */
    table_ptr->appdata_ptr = appdata_ptr;
    table_ptr->appdata_size = appdata_size;
    /* Initialize zero value ptr to string of zero-valued bytes if NULL */
    if (no_value_ptr == NULL) {
        table_ptr->no_value_ptr = (uint8_t*) malloc(table_ptr->value_width);
        bzero(table_ptr->no_value_ptr, table_ptr->value_width);
    } else {
      table_ptr->no_value_ptr = no_value_ptr;
    }
    assert(table_ptr != NULL);

    /* Read the number of blocks */
    read_count = fread(&(table_ptr->num_blocks),
                       sizeof(table_ptr->num_blocks), 1, input_fp);
    if (read_count != 1) return ERR_FILEREADERROR;
    
    /* Read the block header array */
    read_count = fread(block_headers, sizeof(BlockHeader), MAX_BLOCKS, input_fp);
    if (read_count != MAX_BLOCKS) return ERR_FILEREADERROR;

    /* Read each of the blocks */
    for (block_index=0;block_index<table_ptr->num_blocks;block_index++) {
        HashBlock *block_ptr;
        int rv;

        /* Read block based on block header */
        rv = hashlib_read_block(&block_ptr, table_ptr,
                                &(block_headers[block_index]),
                                input_fp);

        if (rv!=OK) {
            assert(rv==OK);
            return rv;
        }

        table_ptr->block_ptrs[block_index] = block_ptr; 
    }

    *table_pptr = table_ptr;
    
    return OK;
}
