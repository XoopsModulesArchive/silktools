#ifndef _HASHLIB_H
#define _HASHLIB_H
/*
**  Copyright (C) 2001,2002,2003 by Carnegie Mellon University.
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
**  1.9
**  2003/12/10 22:00:42
** thomasm
*/

/*@unused@*/ static char rcsID_HASHLIB_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "hashlib.h,v 1.9 2003/12/10 22:00:42 thomasm Exp";


/* File: hashlib.h: defines interface to core hash library
 * functions. */


#include "silk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define integer type aliases if needed */
#ifdef DEFINE_INT_TYPES
typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

/* Global configuration */
#include "hashlib_conf.h"

/* Some generic utilities */
#include "hashlib_utils.h"

/* Return codes. Note that >= 0 are success codes. */
#define	OK 0
#define OK_DUPLICATE 1            /* entry already exists */
#define ERR_NOTFOUND -1           /* couldn't find entry */
#define ERR_NOMOREENTRIES -2      /* no more entries available */
#define ERR_INDEXOUTOFBOUNDS -3		/* no longer in use */
#define ERR_FILEOPENERROR -4      /* couldn't open a file */
#define ERR_BADARGUMENT -5        /* illegal argument value */
#define ERR_INTERNALERROR -6      /* corrupt internal state */
#define ERR_NOTSUPPORTED -7       /* operation not supported for this table */
#define ERR_FILEREADERROR -8      /* read error (corrupt data file) */
#define ERR_FILEWRITEERROR -9     /* write error */
#define ERR_NOMOREBLOCKS -254     /* attempted to aloc > max. # of blocks */
#define ERR_OUTOFMEMORY -255      /* a call to malloc failed */

/* Types of hash tables */

/* Let the library manage the memory used to store values. */
#define HTT_INPLACE 0

/* Application-managed memory. Indicates the hash table will manage
 * pointer storage, _not_ data storage. */
#define HTT_BYREFERENCE 1

/* Default load is 192 (75%). Generally, this is the value that should
 * be passed to hashlib_create_table for the load factor. */
#define DEFAULT_LOAD_FACTOR 192

/* Maximum number of blocks ever allocated. Once the primary block
 * reaches the maximum block size, new blocks will be allocated
 * until this maximum is reached. */
#define MAX_BLOCKS 8

/* Data types: a HashTable maintains a set of HashBlocks.  Note that
 * these data structures should generally be regarded as a read-only
 * by hash table users. Attriutes in HashTable duplicated in HashBlock
 * are copied upon block creation.*/
typedef struct _hashblock {
    uint8_t value_type;    /* HTT_INPLACE or HTT_BYREFERENCE */
    uint32_t block_size;   /* Total number of entries in the block */
    uint32_t num_entries;  /* Number of occupied entries in the block */
    uint8_t key_width;     /* Size of a key in bytes */
    uint8_t value_width;   /* Size of a value in bytes */
    uint8_t load_factor;   /* Point at which to resize (fraction of 255) */
    uint8_t *no_value_ptr; /* Pointer to representation of an empty value */
    uint8_t *_data_ptr;    /* Pointer to an array of variable-sized entries */
} HashBlock;

typedef struct _hashtable {
    uint8_t value_type;
    uint8_t key_width;
    uint8_t value_width;
    uint8_t load_factor;
    uint8_t *no_value_ptr;
    uint32_t appdata_size; /* Size of application data block */
    void *appdata_ptr;   /* Number of bytes of application data */
    uint8_t num_blocks;                  /* Number of blocks */
    HashBlock *block_ptrs[MAX_BLOCKS]; /* The blocks */
} HashTable;

/* Iteration */
typedef struct _hashiter {
    int block;      /* Current block. Negative value is beginning or end */
    uint32_t index;   /* Current index into block */
    uint8_t *current_entry_ptr; /* Pointer to the entry */
} HASH_ITER;

/* Distinguished values for block */
#define HASH_ITER_BEGIN -1
#define HASH_ITER_END -2

HashTable *hashlib_create_table(uint8_t key_width,
                                uint8_t value_width,
                                uint8_t data_type,
                                uint8_t *no_value_ptr,
                                uint8_t *app_data_ptr,
                                uint32_t app_data_size,
                                uint32_t estimated_size,
                                uint8_t load_factor);

int hashlib_insert(HashTable *table_ptr, uint8_t *key_ptr, uint8_t **value_pptr);
int hashlib_lookup(HashTable *table_ptr, uint8_t *key_ptr, uint8_t **value_pptr);
HASH_ITER hashlib_create_iterator(HashTable *table_ptr);
int hashlib_iterate(HashTable *table_ptr, HASH_ITER *iter_ptr,
                    uint8_t **key_pptr, uint8_t **val_pptr);
void hashlib_free_table(HashTable *table_ptr);
int hashlib_rehash(HashTable *table_ptr);
uint32_t hashlib_count_nonempties(HashTable *table_ptr);
uint32_t hashlib_count_entries(HashTable *table_ptr);
void hashlib_dump_table_header(FILE *fp, HashTable *table_ptr);
void hashlib_dump_table(FILE *fp, HashTable *table_ptr);
void hashlib_dump_bytes(FILE *fp, uint8_t *data_ptr, uint32_t data_size);

/* Serialization */
int hashlib_save_table(HashTable *table_ptr, char *filename_str,
                       uint8_t *header_ptr, uint8_t header_length);
int hashlib_restore_table(HashTable **table_pptr, char *filename_str,
                          uint8_t *header_ptr, uint8_t header_length);
int hashlib_serialize_table(HashTable *table_ptr, FILE *output_fp,
                            uint8_t *header_ptr, uint8_t header_length);
int hashlib_deserialize_table(HashTable **table_pptr, FILE *input_fp,
                              char *header_ptr, int header_length);

/* #define HASHLIB_RECORD_STATS */

/* Statistics */
#ifdef HASHLIB_RECORD_STATS
extern void hashlib_clear_stats();
extern void hashlib_dump_stats(FILE *fp);
extern uint64_t stats_total_rehashes;
extern uint64_t stats_total_rehash_inserts;
extern uint64_t stats_total_inserts;
extern uint64_t stats_total_lookups;
extern uint32_t stats_total_blocks_allocated;
#endif


#endif /* _HASHLIB_H */
