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
** 1.8
** 2004/03/11 14:19:22
** thomasm
*/
/* File: hashwrap.c: implements useful wrappers around hash functions. */

#include "hashlib.h"

#include <string.h>
#include <assert.h>
#include "hashwrap.h"

const uint32_t ctr_empty_value = 0xFFFFFFFF;

/* constants */
#define USE_MACROS

/* static prototypes */
#ifndef USE_MACROS
void tuple_encode_key (uint8_t key[], uint8_t fields, uint32_t src_ip,
                       uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
		       uint8_t protocol);
void tuple_decode_key (uint8_t key[], uint8_t fields, uint32_t *src_ip_ptr,
                       uint32_t *dest_ip_ptr, uint16_t *src_port_ptr,
		       uint16_t *dest_port_ptr, uint8_t *protocol_ptr);
static void hcn_encode_key (uint8_t key[], uint32_t src_ip, uint32_t dest_net);
static void hcn_decode_key (uint8_t key[], uint32_t *src_ip_ptr,
			    uint32_t *dest_net_ptr);
#endif

/***************************************************************************
 * Tuple [counter/pointer] Common Support
***************************************************************************/

/* AJK -- Moved macros to header for reporting applicaitons. Need to confirm
 * this is really what we want. */

#if 0
/* Encode a 5-tuple into a 13-byte padded hash key */
void tuple_encode_key (uint8_t key[], uint8_t fields, uint32_t src_ip,
		uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t protocol) {
	/* run over fields we care about copying them into the key */
	int i = 0;
	if (fields & FIELD_SRC_IP) {
		memcpy(&key[i],&src_ip,sizeof(uint32_t));
		i += sizeof(uint32_t);
	}
	if (fields & FIELD_DEST_IP) {
		memcpy(&key[i],&dest_ip,sizeof(uint32_t));
		i += sizeof(uint32_t);
	}
	if (fields & FIELD_SRC_PORT) {
		memcpy(&key[i],&src_port,sizeof(uint16_t));
		i += sizeof(uint16_t);
	}
	if (fields & FIELD_DEST_PORT) {
		memcpy(&key[i],&dest_port,sizeof(uint16_t));
		i += sizeof(uint16_t);
	}
	if (fields & FIELD_PROTOCOL) {
		memcpy(&key[i],&protocol,sizeof(uint8_t));
		i += sizeof(uint8_t);
	}
	/* zero-pad the key */
	for (; i < TUPLE_KEYWIDTH; i++) {
		key[i] = 0;
	}
}

/* Decode a 13-byte padded hash key into a 5-tuple */
void tuple_decode_key (uint8_t key[], uint8_t fields, uint32_t *src_ip_ptr,
		       uint32_t *dest_ip_ptr, uint16_t *src_port_ptr,
		       uint16_t *dest_port_ptr, uint8_t *protocol_ptr) {

	/* run over fields we care about copying them out of the key */
	int i = 0;
	if (fields & FIELD_SRC_IP) {
		memcpy(src_ip_ptr,&key[i],sizeof(uint32_t));
		i += sizeof(uint32_t);
	}
	if (fields & FIELD_DEST_IP) {
		memcpy(dest_ip_ptr,&key[i],sizeof(uint32_t));
		i += sizeof(uint32_t);
	}
	if (fields & FIELD_SRC_PORT) {
		memcpy(src_port_ptr,&key[i],sizeof(uint16_t));
		i += sizeof(uint16_t);
	}
	if (fields & FIELD_DEST_PORT) {
		memcpy(dest_port_ptr,&key[i],sizeof(uint16_t));
		i += sizeof(uint16_t);
	}
	if (fields & FIELD_PROTOCOL) {
		memcpy(protocol_ptr,&key[i],sizeof(uint8_t));
		i += sizeof(uint8_t);
	}
}
#endif /* USE_MACROS */

/***************************************************************************
 * Tuple Counter (13 byte key => 4 byte value)
***************************************************************************/

TupleCounter *tuplectr_create_counter(uint8_t fields,
                                      uint32_t initial_size) {
    /* make a durable copy of the field specifier */
	uint8_t *fields_cpy = malloc(sizeof(uint8_t));
	*fields_cpy = fields;

	/* create the hash table */
	return (TupleCounter*) hashlib_create_table(TUPLE_KEYWIDTH,
                                             sizeof(uint32_t),
                                             HTT_INPLACE, 
                                             (uint8_t*) &ctr_empty_value, 
                                             fields_cpy,
                                             sizeof(uint8_t),
                                             initial_size,
                                             DEFAULT_LOAD_FACTOR);
}

int tuplectr_set(TupleCounter *counter_ptr,
                        uint32_t src_ip,
                        uint32_t dest_ip,
                        uint16_t src_port,
                        uint16_t dest_port,
                        uint8_t protocol,
                        uint32_t value) {

	uint8_t key[TUPLE_KEYWIDTH];
    int rv;
    uint32_t *val_ptr;
    HashTable *table_ptr = &(counter_ptr->_data);

    /* Make sure that they haven't supplied the empty value */
    if (value == ctr_empty_value) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        assert(0);
        exit(1);
    }
	
	/* generate a key */
	tuple_encode_key(key, *((uint8_t*)(table_ptr->appdata_ptr)), src_ip,
		dest_ip, src_port, dest_port, protocol);

    /* Do an insert */
    rv = hashlib_insert(table_ptr, key, (uint8_t**) &val_ptr);
    if (rv < 0) {
        assert(0);
        return rv;
    }
    *val_ptr = value;
    return rv;

}

int tuplectr_get(TupleCounter *counter_ptr, uint32_t src_ip,
                            uint32_t dest_ip, uint16_t src_port,
                            uint16_t dest_port, uint8_t protocol,
                            uint32_t *value_ptr) {
	uint8_t key[TUPLE_KEYWIDTH];
    uint32_t *valref_ptr;
    int rv;
    HashTable *table_ptr = &(counter_ptr->_data);
	
	/* generate a key */
	tuple_encode_key(key, *((uint8_t*)(table_ptr->appdata_ptr)), src_ip,
		dest_ip, src_port, dest_port, protocol);

	/* do the lookup */
	rv = hashlib_lookup(table_ptr, key, (uint8_t**) &valref_ptr);
    if (rv < 0) return rv;
    *value_ptr = *valref_ptr;
    return rv;
}


int tuplectr_add(TupleCounter *counter_ptr, uint32_t src_ip,
                 uint32_t dest_ip, uint16_t src_port,
                 uint16_t dest_port, uint8_t protocol,
                 uint32_t addend) {
	uint8_t key[TUPLE_KEYWIDTH];
    int rv;
    uint32_t *val_ptr;
    uint32_t new_value;
    HashTable *table_ptr = &(counter_ptr->_data);
	
	/* generate a key */
	tuple_encode_key(key, *((uint8_t*)(table_ptr->appdata_ptr)),
			 src_ip, dest_ip, src_port, dest_port, protocol);

    rv = hashlib_insert(table_ptr, key, (uint8_t**) &val_ptr);
    /* First insert -- start at one */
    if (rv == OK) {
        *val_ptr = 1;
        return rv;
    }
    
    if (rv < 0) {
        assert(0);
        return rv;
    }

    /* Do the sum */
    new_value = (*val_ptr) + addend;

    /* Check for overflow */
    if ((new_value == ctr_empty_value) ||
        (new_value < *val_ptr)) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        exit(1);
    }

    /* Set the value */
    *val_ptr = new_value;
    return rv;
}


int tuplectr_iterate(TupleCounter *counter_ptr, HASH_ITER *iter,
	uint32_t *src_ip_ptr, uint32_t *dest_ip_ptr, uint16_t *src_port_ptr,
	uint16_t *dest_port_ptr, uint8_t *protocol_ptr, uint32_t *count_ptr) {

	uint8_t *key_ptr;
	uint8_t *val_ptr;

	int rv;
	
	/* first, iterate on the hashtable */
	rv = hashlib_iterate(&(counter_ptr->_data), iter, &key_ptr, &val_ptr);
	if (rv < 0) return rv;

	/* decode the key */
	tuple_decode_key(key_ptr, *((uint8_t*)(counter_ptr->_data.appdata_ptr)),
			src_ip_ptr, dest_ip_ptr, src_port_ptr, dest_port_ptr,
			protocol_ptr);
	/* fill in the count */
	*count_ptr = *((uint32_t*)val_ptr);

	/* all done here */
	return OK;
}

/***************************************************************************
 * Tuple Statistics (13 byte key => pointer)
***************************************************************************/

TupleStats *tuplestats_create_table(uint8_t fields, uint32_t initial_size) {
    /* make a durable copy of the field specifier */
	uint8_t *fields_cpy = malloc(sizeof(uint8_t));
	*fields_cpy = fields;

	/* create the hash table */
	return (TupleStats*) hashlib_create_table(TUPLE_KEYWIDTH,
                                             sizeof(void*),
                                             HTT_BYREFERENCE, 
                                             NULL,
                                             fields_cpy,
                                             sizeof(uint8_t),
                                             initial_size,
                                             DEFAULT_LOAD_FACTOR);
}

int tuplestats_insert(TupleStats *stats_ptr, uint32_t src_ip,
		uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t protocol,
		void ***stats_obj_ppptr) {

	uint8_t key[TUPLE_KEYWIDTH];
    uint8_t *val_ptr;
	int rv;

	/* generate a key */
	tuple_encode_key(key, *((uint8_t*)(stats_ptr->_data.appdata_ptr)), src_ip,
		dest_ip, src_port, dest_port, protocol);
	
	/* do the insert */
    rv = hashlib_insert(&(stats_ptr->_data), (uint8_t*) &key, &val_ptr);
    if ((rv != OK) && (rv != OK_DUPLICATE))
      return rv;
    (*stats_obj_ppptr) = (void**) val_ptr;
    return rv;
}

int tuplestats_lookup(TupleStats *stats_ptr, uint32_t src_ip,
		      uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
		      uint8_t protocol, void **stats_obj_pptr) {

	uint8_t key[TUPLE_KEYWIDTH];
    void **valref_ptr;
	int rv;

	/* generate a key */
	tuple_encode_key(key, *((uint8_t*)(stats_ptr->_data.appdata_ptr)),
			 src_ip, dest_ip, src_port, dest_port, protocol);

	/* do the lookup */
    rv = hashlib_lookup(&(stats_ptr->_data), (uint8_t*) &key,
                            (uint8_t**) &valref_ptr);
    if (rv != OK) return rv;
    *stats_obj_pptr = (void*) *valref_ptr;
    return rv;
}

int tuplestats_iterate(TupleStats *stats_ptr, HASH_ITER *iter,
		       uint32_t *src_ip_ptr, uint32_t *dest_ip_ptr,
		       uint16_t *src_port_ptr, uint16_t *dest_port_ptr,
		       uint8_t *protocol_ptr, void **stats_obj_pptr) {

    uint8_t *key_ptr;
    uint8_t *valref_ptr;
    int rv;
    rv = hashlib_iterate(&(stats_ptr->_data), iter, &key_ptr, &valref_ptr);
    if (rv != OK) return rv;
	/* decode the key */
	tuple_decode_key(key_ptr, *((uint8_t*)stats_ptr->_data.appdata_ptr),
			 src_ip_ptr, dest_ip_ptr, src_port_ptr,
			 dest_port_ptr, protocol_ptr);
	/* hand over the value */
    *stats_obj_pptr = (void*) (*(void**)valref_ptr);
    return OK;
}

/***************************************************************************
 * IP counter (4 byte key => 4 byte value)
***************************************************************************/

IpCounter *ipctr_create_counter(uint32_t initial_size) {
    return (IpCounter*) hashlib_create_table(sizeof(uint32_t), sizeof(uint32_t),
                                             HTT_INPLACE,
                                             (uint8_t*) &ctr_empty_value,
                                             NULL, 0,
                                             initial_size,
                                             DEFAULT_LOAD_FACTOR);
}
    
int ipctr_add(IpCounter *counter_ptr, uint32_t key, uint32_t addend) {
    uint32_t *val_ptr = NULL;
    uint32_t new_value;
    
    int rv = hashlib_insert(&(counter_ptr->_data), (uint8_t*) &key,
			(uint8_t**) &val_ptr);
    /* First insert -- start at one */
    if (rv == OK) {
        *val_ptr = 1;
        return rv;
    }
    if (rv < 0) return rv;

    /* Do the sum */
    new_value = (*val_ptr) + addend;

    /* Check for overflow */
    if ((new_value == ctr_empty_value) ||
        (new_value < *val_ptr)) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        exit(1);
    }

    (*val_ptr) = new_value;
    return rv;
}


int ipctr_set(IpCounter *counter_ptr, uint32_t key, uint32_t value) {
    uint32_t *val_ptr;
	int rv;

    /* Make sure that they haven't supplied the empty value */
    if (value == ctr_empty_value) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        assert(0);
        exit(1);
    }

	/* do the insert */
    rv = hashlib_insert(&(counter_ptr->_data), (uint8_t*) &key,
			(uint8_t**) &val_ptr);
    if (rv < 0) return rv;
    (*val_ptr) = value;
    return rv;
}

int ipctr_get(IpCounter *counter_ptr, uint32_t key, uint32_t *value_ptr) {
    uint32_t *valref_ptr;
    int rv = hashlib_lookup(&(counter_ptr->_data), (uint8_t*) &key,
			(uint8_t**) &valref_ptr);
    if (rv < 0) return rv;
    *value_ptr = *valref_ptr;
    return OK;
}

int ipctr_iterate(IpCounter *counter_ptr, HASH_ITER *iter_ptr,
                      uint32_t *key_ptr, uint32_t *value_ptr) {
	/* first, iterate on the hashtable */
    uint32_t *keyref_ptr, *valref_ptr;
    int rv;
    rv = hashlib_iterate(&(counter_ptr->_data), iter_ptr,
                         (uint8_t**) &keyref_ptr,
                         (uint8_t**) &valref_ptr);
    if (rv < 0) return rv;
    *key_ptr = *keyref_ptr;
    *value_ptr = *valref_ptr;
    return OK;
}

/***************************************************************************
 * IP Statistics (4 byte key => pointer)
***************************************************************************/

IpStats* ipstats_create_table(uint32_t initial_size) {
    return (IpStats*) hashlib_create_table(sizeof(uint32_t),
                                           sizeof(void*),
                                           HTT_BYREFERENCE,
                                           NULL,
                                           NULL, 0,
                                           initial_size,
                                           DEFAULT_LOAD_FACTOR);
}

int ipstats_insert(IpStats *stats_ptr, uint32_t key,
                         void ***stats_obj_ppptr) {
    uint8_t *val_ptr;
    int rv = hashlib_insert(&(stats_ptr->_data),
                            (uint8_t*) &key, &val_ptr);
    if ((rv != OK) && (rv != OK_DUPLICATE))
      return rv;
    (*stats_obj_ppptr) = (void**) val_ptr;
    return rv;
}

int ipstats_lookup(IpStats *stats_ptr, uint32_t key, 
                      void **stats_obj_pptr) {
    void **valref_ptr;
    int rv = hashlib_lookup(&(stats_ptr->_data), (uint8_t*) &key,
                            (uint8_t**) &valref_ptr);
    if (rv != OK) return rv;
    *stats_obj_pptr = (void*) *valref_ptr;
    return rv;
}

int ipstats_iterate(IpStats *stats_ptr, HASH_ITER *iter_ptr,
                    uint32_t *key_ptr, void **stats_obj_pptr) {
    uint8_t *keyref_ptr;
    uint8_t *valref_ptr;
    int rv;
    rv = hashlib_iterate(&(stats_ptr->_data), iter_ptr, &keyref_ptr, &valref_ptr);
    if (rv != OK) return rv;
    *key_ptr = *((uint32_t*)keyref_ptr);
    *stats_obj_pptr = (void*) (*(void**)valref_ptr);
    return OK;

}

void ipstats_dump_table(IpStats *stats_ptr, void (* dump_object)(void *)) {
    HASH_ITER iter;
    uint32_t ip_addr;
    void *object_ptr;
    
    iter = ipstats_create_iterator(stats_ptr);
    while (ipstats_iterate(stats_ptr, &iter, &ip_addr,
                           (void **) &object_ptr) != ERR_NOMOREENTRIES) {
        printf("%d.%d.%d.%d --> ",
               ((uint8_t*)(&ip_addr))[3],
               ((uint8_t*)(&ip_addr))[2],
               ((uint8_t*)(&ip_addr))[1],
               ((uint8_t*)(&ip_addr))[0]);
        dump_object(object_ptr);
    }
    
}

/***************************************************************************
 * Host/CNetwork Common Support
***************************************************************************/

/* Encode and decode keys. Store both in native byte order, ip address
 * followed by the network address (with a zero LSB) (two 32-bit
 * values). */

#define hcn_encode_key(key, src_ip, dest_net) { \
    uint32_t masked_net = dest_net & 0xFFFFFF00; /* mask out host */ \
    memcpy(key,&src_ip,sizeof(uint32_t)); /* src ip */ \
    memcpy(key+sizeof(uint32_t),&masked_net,sizeof(uint32_t)); /* c-net */	\
}

#define hcn_decode_key(key, src_ip_ptr, dest_net_ptr) { \
	memcpy((uint8_t*) src_ip_ptr, key, sizeof(uint32_t)); \
	memcpy((uint8_t*) dest_net_ptr, key+sizeof(uint32_t), sizeof(uint32_t)); /* c-net */ \
}

/***************************************************************************
 * Host/CNetwork Counter (7 byte key -> 4 byte value)
***************************************************************************/

HcnCounter *hcnctr_create_counter(uint32_t initial_size) {
	return (HcnCounter*) hashlib_create_table(HCN_KEYWIDTH,
                                             sizeof(uint32_t),
                                             HTT_INPLACE, 
                                             (uint8_t*) &ctr_empty_value,
                                             NULL,
                                             0,
                                             initial_size,
                                             DEFAULT_LOAD_FACTOR);
}

int hcnctr_add(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net, uint32_t addend) {
	uint8_t key[HCN_KEYWIDTH];
    int rv;
    uint32_t *val_ptr;
    uint32_t new_value;
	
	/* generate a key */
	hcn_encode_key(key, src_ip, dest_net);

    /* Increment value */
    rv = hashlib_insert(&(counter_ptr->_data), key, (uint8_t**) &val_ptr);
    /* First insert -- start at one */
    if (rv == OK) {
        *val_ptr = 1;
        return rv;
    }
    if (rv < 0) return rv;
    
    /* Do the sum */
    new_value = (*val_ptr) + addend;

    /* Check for overflow */
    if ((new_value == ctr_empty_value) ||
        (new_value < *val_ptr)) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        exit(1);
    }

    /* Set the value */
    *val_ptr = new_value;

    return rv;
}


int hcnctr_set(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net,
		uint32_t value) {
	uint8_t key[HCN_KEYWIDTH];
    int rv;
    uint32_t *val_ptr;

    /* Make sure that they haven't supplied the empty value */
    if (value == ctr_empty_value) {
        fprintf(stderr, "INTERNAL ERROR: counter overflow\n");
        assert(0);
        exit(1);
    }

	/* generate a key */
	hcn_encode_key(key, src_ip, dest_net);

    /* do insert */
    rv = hashlib_insert(&(counter_ptr->_data), key, (uint8_t**) &val_ptr);
    *val_ptr = value;
    return rv;
}

int hcnctr_get(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net,
		uint32_t *value_ptr) {
	uint8_t key[HCN_KEYWIDTH];
    uint32_t *valref_ptr;
    int rv;
	
	/* generate a key */
	hcn_encode_key(key, src_ip, dest_net);

	/* do the lookup */
	rv = hashlib_lookup(&(counter_ptr->_data), key, (uint8_t**) &valref_ptr);
    if (rv != OK) return rv;
    *value_ptr = *valref_ptr;
    return rv;
}

int hcnctr_iterate(HcnCounter *counter_ptr, HASH_ITER *iter,
	uint32_t *src_ip_ptr, uint32_t *dest_net_ptr, uint32_t *count_ptr) {
	uint8_t *key_ptr;
	uint8_t *val_ptr;

	int rv;
	
	/* first, iterate on the hashtable */
	rv = hashlib_iterate(&(counter_ptr->_data), iter, &key_ptr, &val_ptr);
	if (rv != OK) return rv;

	/* decode the key */
	hcn_decode_key(key_ptr, src_ip_ptr, dest_net_ptr);
	/* fill in the count */
	*count_ptr = *((uint32_t*)val_ptr);

	/* all done here */
	return OK;
}

/***************************************************************************
 * Host/CNetwork Statistics (7 byte key -> pointer)
***************************************************************************/
HcnStats *hcnstats_create_table(uint32_t initial_size) {
	return (HcnStats*) hashlib_create_table(HCN_KEYWIDTH,
                                             sizeof(void*),
                                             HTT_BYREFERENCE, 
                                             NULL,
                                             NULL,
                                             0,
                                             initial_size,
                                             DEFAULT_LOAD_FACTOR);
}

int hcnstats_insert(HcnStats *stats_ptr, uint32_t src_ip, uint32_t dest_net,
		void ***stats_obj_ppptr) {

	uint8_t key[HCN_KEYWIDTH];
    uint8_t *val_ptr;
	int rv;

	/* generate a key */
	hcn_encode_key(key, src_ip, dest_net);
	
	/* do the insert */
    rv = hashlib_insert(&(stats_ptr->_data), (uint8_t*) &key, &val_ptr);
    if ((rv != OK) && (rv != OK_DUPLICATE)) return rv;
    (*stats_obj_ppptr) = (void**) val_ptr;
    return rv;
	
}

int hcnstats_lookup(HcnStats *stats_ptr, uint32_t src_ip, int32_t dest_net,
		void **stats_obj_pptr) {
	uint8_t key[HCN_KEYWIDTH];
    void **valref_ptr;
	int rv;

	/* generate a key */
	hcn_encode_key(key, src_ip, dest_net);

	/* do the lookup */
    rv = hashlib_lookup(&(stats_ptr->_data), (uint8_t*) &key,
                            (uint8_t**) &valref_ptr);
    if (rv != OK) return rv;
    *stats_obj_pptr = (void*) *valref_ptr;
    return rv;
}

int hcnstats_iterate(HcnStats *stats_ptr, HASH_ITER *iter,
		uint32_t *src_ip_ptr, uint32_t *dest_net_ptr, void **stats_obj_pptr) {
    uint8_t *key_ptr;
    uint8_t *valref_ptr;
    int rv;
    rv = hashlib_iterate((HashTable*) stats_ptr,
                         iter,
                         &key_ptr,
                         &valref_ptr);
    if (rv != OK) return rv;
	/* decode the key */
	hcn_decode_key(key_ptr, src_ip_ptr, dest_net_ptr);
	/* hand over the value */
    *stats_obj_pptr = (void*) (*(void**)valref_ptr);
    return OK;
}
