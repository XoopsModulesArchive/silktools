#ifndef _HASHWRAP_H
#define _HASHWRAP_H
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

**  1.8
**  2003/12/10 22:00:43
** thomasm
*/

/*@unused@*/ static char rcsID_HASHWRAP_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "hashwrap.h,v 1.8 2003/12/10 22:00:43 thomasm Exp";


/* File: hashwrap.h: header for hash table wrappers */

#include "hashlib.h"

/* keywidth constants */
/* Note that these must be word-aligned on some platforms. We opted for the simplest
 * solution and padded them out for _all_ platforms, sidestepping the issue
 * of platform-specific serialization. */
#define	TUPLE_KEYWIDTH 16
#define	HCN_KEYWIDTH 8    /* Must be word-aligned on some platforms */

/***************************************************************************
 * Tuple Counter (13 byte key => 4 byte value)
***************************************************************************/

/* Flags used by tuplectr and tuplestats */
#define FIELD_SRC_IP      1		/* hash on Source IP tuple field */
#define FIELD_DEST_IP     (1<<1)	/* hash on Dest IP tuple field */
#define FIELD_SRC_PORT    (1<<2)    	/* hash on Source Port tuple field */
#define FIELD_DEST_PORT   (1<<3)	/* hash on Dest Port tuple field */
#define FIELD_PROTOCOL    (1<<4)	/* hash on Protocol tuple field */
#define FIELD_ALL         0x1f		/* hash on all five tuple fields */

/* Tuple counter data structure */
typedef struct _tuplecounter {
    HashTable _data;
} TupleCounter;

/* tuplectr_create_counter(fields, initial_size):
 *
 * Creates a new hash table for counting 5-tuples.
 *
 * Parameters:
 * 	fields: a group of fields to hash on (FIELD_* constants)
 * 	ORd together; the special constant FIELD_ALL indicates hashing on
 * 	all fields.
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 	the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created TupleCounter, or NULL if the tuple counter
 * 	could not be created.
*/
TupleCounter *tuplectr_create_counter(uint8_t fields, uint32_t initial_size);

/* tuplectr_create_iterator(counter_ptr):
 *
 * Creates a new iterator for iterating a given TupleCounter.
 *
 * Parameters:
 * 	counter_ptr: the TupleCounter on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with tuplectr_iterate.
*/
#define tuplectr_create_iterator(counter_ptr)	\
    hashlib_create_iterator(&((counter_ptr)->_data))

/* tuplectr_inc(counter_ptr,src_ip,dest_ip,src_port,dest_port,protocol):
 *
 * Increment the count associated with a given tuple. Only the fields
 * selected at counter creation time are considered. Inserts the tuple
 * into the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the TupleCounter on which to increment.
 * 	src_ip, src_port, dest_ip, dest_port, protocol: the 5-tuple itself.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/

#define tuplectr_inc(counter_ptr,src_ip,dest_ip,src_port,dest_port,protocol) \
   tuplectr_add(counter_ptr, src_ip, dest_ip, src_port, dest_port, protocol, 1)

int tuplectr_add(TupleCounter *counter_ptr, uint32_t src_ip,
                 uint32_t dest_ip, uint16_t src_port,
                 uint16_t dest_port, uint8_t protocol,
                 uint32_t addend);


/* tuplectr_set(counter_ptr,src_ip,dest_ip,src_port,dest_port,protocol,value):
 *
 * Set the count associated with a given tuple. Only the fields
 * selected at counter creation time are considered. If the value is set
 * to 0, the tuple is removed from the counter. Inserts the tuple into
 * the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the TupleCounter in which to set the value.
 * 	src_ip, dest_ip, src_port, dest_port, protocol: the 5-tuple itself.
 * 	value: the count to associate with the given tuple
 *
 * Returns:
 * 		A hashlib result code (see hashlib.h)
*/
int tuplectr_set(TupleCounter *counter_ptr, uint32_t src_ip,
                        uint32_t dest_ip, uint16_t src_port,
                        uint16_t dest_port, uint8_t protocol,
                        uint32_t value);

/* tuplectr_get(counter_ptr,src_ip,dest_ip,src_port,dest_port,protocol,
 *   val_ptr):
 *
 * Retrieve the count associated with a given tuple. Only the fields
 * selected at counter creation time are considered.
 *
 * Parameters:
 * 	counter_ptr: the TupleCounter from which to get the value.
 * 	src_ip, dest_ip, src_port, dest_port, protocol: the 5-tuple itself.
 * 	val_ptr: a pointer to a uint32_t in which to place the count.
 *
 * Returns:
 * 		A hashlib result code (see hashlib.h)
*/
int tuplectr_get(TupleCounter *counter_ptr, uint32_t src_ip,
	uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t protocol,
	uint32_t *val_ptr);

/* tuplectr_iterate(counter,ptr, iter_ptr, src_ip_ptr, dest_ip_ptr,
 *  src_port_ptr, dest_port_ptr, protocol_ptr, count_ptr):
 *
 * Retrieves the next counted tuple and the associated count while iterating
 * over a given TupleCounter. Call this function repeatedly with a HASH_ITER
 * returned by tuplectr_create_iterator to iterate over all the entries
 * in the counter. This function makes no guarantees about the order of
 * retrieved tuples. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	counter_ptr: the TupleCounter over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by tuplectr_create_iterator.
 * 	src_ip_ptr, dest_ip_ptr, src_port_ptr, dest_port_ptr, protocol_ptr:
 * 	Pointers to unsigned ints in which to place the next tuple.
 * 	count_ptr: pointer to a uint32_t in which to place the associated count.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int tuplectr_iterate(TupleCounter *counter_ptr, HASH_ITER *iter_ptr,
	uint32_t *src_ip_ptr, uint32_t *dest_ip_ptr, uint16_t *src_port_ptr,
	uint16_t *dest_port_ptr, uint8_t *protocol_ptr, uint32_t *count_ptr);

/* tuplectr_free(counter_ptr):
 *
 * Frees the storage used by a TupleCounter.
 *
 * Parameters:
 *	counter_ptr: the TupleCounter to free.
 *
 * Returns:
 * 	void
*/
#define tuplectr_free(counter_ptr) ({free((counter_ptr)->_data.appdata_ptr);\
    hashlib_free_table(&((counter_ptr)->_data));})

/* Convenience macros for dealing with rwRecords */
#define tuplectr_rec_set(counter_ptr, rec_ptr, value) \
	tuplectr_set((counter_ptr), (rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
        (rec_ptr)->sPort,(rec_ptr)->dPort, (rec_ptr)->proto, (value))
#define tuplectr_rec_get(counter_ptr, rec_ptr, val_ptr) \
	tuplectr_get((counter_ptr), (rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
	(rec_ptr)->sPort, (rec_ptr)->dPort, (rec_ptr)->proto, (val_ptr))
#define tuplectr_rec_inc(counter_ptr, rec_ptr) \
	tuplectr_inc((counter_ptr),(rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
	 (rec_ptr)->sPort,(rec_ptr)->dPort, (rec_ptr)->proto)
#define tuplectr_rec_add(counter_ptr, rec_ptr, addend) \
	tuplectr_inc((counter_ptr), (rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
	 (rec_ptr)->sPort,(rec_ptr)->dPort, (rec_ptr)->proto, addend)
#define tuplectr_rec_iterate(counter_ptr, iter_ptr, rec_ptr, count_ptr) \
	tuplectr_iterate((counter_ptr), (iter_ptr), \
	&((rec_ptr)->sIP.ipnum),&((rec_ptr)->dIP.ipnum),\
	 &((rec_ptr)->sPort), &((rec_ptr)->dPort), \
	&((rec_ptr)->proto), (count_ptr))
#define tuplectr_count_entries(counter_ptr) \
  hashlib_count_entries(&(counter_ptr->_data))

/***************************************************************************
 * Tuple Statistics (13 byte key => pointer)
***************************************************************************/

/* Tuple statistics data structure */
typedef struct _tuplestats {
    HashTable _data;
} TupleStats;

/* tuplestats_create_table(fields, initial_size):
 *
 * Creates a new hash table for indexing statistics objects by 5-tuples.
 *
 * Parameters:
 * 	fields: a group of fields to hash on (FIELD_* constants)
 * 	ORd together; the special constant FIELD_ALL indicates hashing on
 * 	all fields.
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 	the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created TupleStats, or NULL if the tuple stats table
 * 	could not be created.
*/
TupleStats *tuplestats_create_table(uint8_t fields, uint32_t initial_size);

/* tuplestats_create_iterator(stats_ptr):
 *
 * Creates a new iterator for iterating a given TupleStats.
 *
 * Parameters:
 * 	stats_ptr: the TupleStats on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with tuplestats_iterate.
*/
#define tuplestats_create_iterator(stats_ptr) \
    hashlib_create_iterator(&((stats_ptr)->_data))

/* tuplestats_insert(stats_ptr,src_ip,dest_ip,src_port,dest_port,protocol,
 * 					stats_obj_ppptr):
 *
 * Associates a tuple with a slot in the table used to store a pointer to
 * a statistics object, and returns a pointer to that slot. This rather
 * roundabout way of inserting allows you to use your own pointer
 * memory management scheme with the statistics table; see the documentation
 * for examples and more on this function's use.
 *
 * Parameters:
 * 	stats_ptr: the TupleStats into which to insert the record.
 * 	src_ip,dest_ip,src_port,dest_port,proto: the tuple to insert.
 * 	stats_obj_ppptr: an out-parameter in which a pointer to the slot where
 * 	the pointer to the statistics object to be stored will be placed
 * 	is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int tuplestats_insert(TupleStats *stats_ptr, uint32_t src_ip,
		      uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
		      uint8_t protocol, void ***stats_obj_ppptr);

/* tuplestats_lookup(stats_ptr,src_ip,dest_ip,src_port,dest_port,protocol,
 * 					stats_obj_pptr):
 *
 * Retrieves a pointer to the statistics object associated with a given
 * tuple.
 *
 * Parameters:
 * 	stats_ptr: the TupleStats in which to look up the record.
 * 	src_ip,dest_ip,src_port,dest_port,proto: the tuple to look up.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 	statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int tuplestats_lookup(TupleStats *stats_ptr, uint32_t src_ip,
		      uint32_t dest_ip, uint16_t src_port, uint16_t dest_port,
		      uint8_t protocol, void **stats_obj_pptr);

/* tuplestats_iterate(stats,ptr, iter_ptr, src_ip_ptr, dest_ip_ptr,
 *	src_port_ptr, dest_port_ptr, protocol_ptr, stats_obj_pptr):
 *
 * Retrieves the next counted tuple and the associated count while iterating
 * over a given TupleStats. Call this function repeatedly with a HASH_ITER
 * returned by tuplestats_create_iterator to iterate over all the entries
 * in the stats table. This function makes no guarantees about the order of
 * retrieved tuples. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	stats_ptr: the TupleStats over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by tuplestats_create_iterator.
 * 	src_ip_ptr, dest_ip_ptr, src_port_ptr, dest_port_ptr, protocol_ptr:
 * 	Pointers to unsigned ints in which to place the next tuple.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 	associated statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int tuplestats_iterate(TupleStats *stats_ptr, HASH_ITER *iter,
		       uint32_t *src_ip_ptr, uint32_t *dest_ip_ptr,
		       uint16_t *src_port_ptr, uint16_t *dest_port_ptr,
		       uint8_t *protocol_ptr, void **stats_obj_pptr);

/* tuplestats_free(stats_ptr):
 *
 * Frees the storage used by a TupleStats. Does NOT free any storage used
 * by the associated statistics objects.
 *
 * Parameters:
 * 	stats_ptr: the TupleStats to free.
 *
 * Returns:
 * 	void
*/
#define tuplestats_free(stats_ptr) ({ \
	free((stats_ptr)->_data.appdata_ptr); \
	hashlib_free_table(&((stats_ptr)->_data)); \
})

/* Convenience macros for dealing with rwRecords */
#define tuplestats_rec_insert(stats_ptr, rec_ptr, stats_obj_ppptr) \
	tuplestats_insert(stats_ptr,(rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
	 (rec_ptr)->sPort, (rec_ptr)->dPort, (rec_ptr)->proto, stats_obj_ppptr)
#define tuplestats_rec_lookup(stats_ptr, rec_ptr, stats_obj_pptr) \
	tuplestats_lookup(stats_ptr,(rec_ptr)->sIP.ipnum,(rec_ptr)->dIP.ipnum,\
	(rec_ptr)->sPort,(rec_ptr)->dPort, (rec_ptr)->proto, stats_obj_pptr)
#define tuplestats_rec_iterate(stats_ptr, iter_ptr, rec_ptr, stats_obj_ppptr)\
	tuplestats_iterate((stats_ptr), (iter_ptr), \
	&((rec_ptr)->sIP.ipnum),&((rec_ptr)->dIP.ipnum),\
	&((rec_ptr)->sPort), &((rec_ptr)->dPort), \
	&((rec_ptr)->proto), (stats_obj_pptr))
#define tuplestats_count_entries(stats_ptr) \
  hashlib_count_entries(&(stats_ptr->_data))

/***************************************************************************
 * IP counter (4 byte key => 4 byte value)
***************************************************************************/

/* IP counter data structure */
typedef struct _ipcounter {
    HashTable _data;
} IpCounter;

/* ipctr_create_counter(initial_size):
 *
 * Creates a new hash table for counting 32-bit IPv4 addresses.
 *
 * Parameters:
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 	the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created IpCounter, or NULL if the IP counter
 * 	could not be created.
*/
IpCounter *ipctr_create_counter(uint32_t initial_size);

/* ipctr_create_iterator(counter_ptr):
 *
 * Creates a new iterator for iterating a given IpCounter.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with ipctr_iterate.
*/
#define ipctr_create_iterator(counter_ptr)	\
     hashlib_create_iterator(&((counter_ptr)->_data))

/* ipctr_inc(counter_ptr,key):
 *
 * Increment the count associated with a given IP. Inserts the IP
 * into the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter on which to increment.
 * 	key: the IP address to increment.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/

#define ipctr_inc(counter_ptr, key) ipctr_add(counter_ptr, key, 1)

int ipctr_add(IpCounter *counter_ptr, uint32_t key, uint32_t addend);

/* ipctr_set(counter_ptr,key,value):
 *
 * Set the count associated with a given IP. If the value is set
 * to 0, the ip is removed from the counter. Inserts the IP into
 * the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter in which to set the value.
 * 	key: the IP address to set.
 * 	value: the count to associate with the given IP
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipctr_set(IpCounter *counter_ptr, uint32_t key, uint32_t value);

/* ipctr_get(counter_ptr,key,value_ptr):
 *
 * Retrieve the count associated with a given IP.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter from which to get the value.
 * 	key: the IP address to retrieve.
 * 	value_ptr: a pointer to a uint32_t in which to place the count.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipctr_get(IpCounter *counter_ptr, uint32_t key, uint32_t *value_ptr);

/* ipctr_iterate(counter,ptr, iter_ptr, key_ptr, value_ptr):
 *
 * Retrieves the next counted IP and the associated count while iterating
 * over a given IpCounter. Call this function repeatedly with a HASH_ITER
 * returned by ipctr_create_iterator to iterate over all the entries
 * in the counter. This function makes no guarantees about the order of
 * retrieved IPs. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by ipctr_create_iterator.
 * 	key_ptr: pointer to a uint32_t in which to place the IP address.
 * 	count_ptr: pointer to a uint32_t in which to place the associated count.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipctr_iterate(IpCounter *counter_ptr, HASH_ITER *iter_ptr,
                      uint32_t *key_ptr, uint32_t *value_ptr);

/* ipctr_free(counter_ptr):
 *
 * Frees the storage used by a IpCounter.
 *
 * Parameters:
 * 	counter_ptr: the IpCounter to free.
 *
 * Returns:
 * 	void
*/
#define ipctr_free(counter_ptr) \
    hashlib_free_table(&((counter_ptr)->_data));
#define ipctr_count_entries(counter_ptr) \
  hashlib_count_entries(&(counter_ptr->_data))


/***************************************************************************
 * IP Statistics (4 byte key => pointer)
***************************************************************************/

/* IP statistics table data structure */
typedef struct _ipstats {
    HashTable _data;
} IpStats;

/* ipstats_create_table(initial_size):
 *
 * Creates a new hash table for indexing statistics objects by IP address.
 *
 * Parameters:
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 		the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created IpStats, or NULL if the IP stats table
 * 	could not be created.
*/
IpStats *ipstats_create_table(uint32_t initial_size);

/* ipstats_create_iterator(stats_ptr):
 *
 * Creates a new iterator for iterating a given IpStats.
 *
 * Parameters:
 * 	stats_ptr: the IpStats on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with ipstats_iterate.
*/
#define ipstats_create_iterator(stats_ptr) \
     hashlib_create_iterator(&((stats_ptr)->_data))


/* ipstats_insert(stats_ptr,key,stats_obj_ppptr):
 *
 * Associates an IP with a slot in the table used to store a pointer to
 * a statistics object, and returns a pointer to that slot. This rather
 * roundabout way of inserting allows you to use your own pointer
 * memory management scheme with the statistics table; see the documentation
 * for examples and more on this function's use.
 *
 * Parameters:
 * 	stats_ptr: the IpStats into which to insert the record.
 * 	key: the ip to insert.
 * 	stats_obj_ppptr: an out-parameter in which a pointer to the slot where
 * 	the pointer to the statistics object to be stored will be placed
 * 	is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipstats_insert(IpStats *stats_ptr, uint32_t key,
                         void ***stats_obj_ppptr);

/* ipstats_lookup(stats_ptr,key,stats_obj_pptr):
 *
 * Retrieves a pointer to the statistics object associated with a given
 * IP.
 *
 * Parameters:
 * 	stats_ptr: the IpStats in which to look up the record.
 * 	key: the IP to look up.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 		statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipstats_lookup(IpStats *stats_ptr, uint32_t key, void **stats_obj_pptr);

/* ipstats_iterate(stats,ptr, iter_ptr, key,ptr, stats_obj_pptr):
 *
 * Retrieves the next counted IP and the associated count while iterating
 * over a given IpStats. Call this function repeatedly with a HASH_ITER
 * returned by ipstats_create_iterator to iterate over all the entries
 * in the stats table. This function makes no guarantees about the order of
 * retrieved IPs. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	stats_ptr: the IpStats over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by ipstats_create_iterator.
 * 	key_ptr: Pointer to uint32_t in which to place the next IP.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 	associated statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int ipstats_iterate(IpStats *stats_ptr, HASH_ITER *iter_ptr,
                    uint32_t *key_ptr, void **stats_obj_pptr);


/* ipstats_free(stats_ptr):
 *
 * Frees the storage used by a IpStats. Does NOT free any storage used
 * by the associated statistics objects.
 *
 * Parameters:
 * 	stats_ptr: the IpStats to free.
 *
 * Returns:
 * 	void
*/
#define ipstats_free(stats_ptr) \
    hashlib_free_table(&((stats_ptr)->_data));
/* ipstats_count_entries(stats_ptr) : counts entries in an IpStats. */
#define ipstats_count_entries(stats_ptr) \
	hashlib_count_entries(&(stats_ptr->_data))

/***************************************************************************
 * Host/CNetwork Counter (7 byte key -> 4 byte value)
***************************************************************************/

/* Host/C-Network counter data structure */
typedef struct _hcncount {
	HashTable _data;
} HcnCounter;

/* hcnctr_create_counter(initial_size):
 *
 * Creates a new hash table for counting host/C-network pairs.
 *
 * Parameters:
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 	the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created HcnCounter, or NULL if the HCN counter
 * 	could not be created.
*/
HcnCounter *hcnctr_create_counter(uint32_t initial_size);

/* hcnctr_create_iterator(counter_ptr):
 *
 * Creates a new iterator for iterating a given HcnCounter.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with hcnctr_iterate.
*/
#define hcnctr_create_iterator(counter_ptr) \
    hashlib_create_iterator(&((counter_ptr)->_data))

/* hcnctr_inc(counter_ptr,src_ip,dest_net):
 *
 * Increment the count associated with a given host/network pair.
 * Inserts the pair into the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter on which to increment.
 * 	src_ip, dest_net: the host/network pair.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/

#define hcnctr_inc(counter_ptr,src_ip,dest_net) hcnctr_add(counter_ptr,src_ip, dest_net, 1)

int hcnctr_add(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net, uint32_t addend);

/* hcnctr_set(counter_ptr,src_ip,dest_net,value):
 *
 * Set the count associated with a given host/net pair.. If the value
 * is set to 0, the pair is removed from the counter. Inserts the pair
 * into the counter if it has not yet been counted.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter in which to set the value
 * 	src_ip, dest_net: the host/network pair
 * 	value: the count to associate with the given pair
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnctr_set(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net,
		uint32_t value);

/* hcnctr_get(counter_ptr,src_ip,src_port,dest_ip,dest_port,protocol,val_ptr):
 *
 * Retrieve the count associated with a given pair.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter from which to get the value.
 * 	src_ip, dest_net: the host/net pair.
 * 	value_ptr: a pointer to a uint32_t in which to place the count.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnctr_get(HcnCounter *counter_ptr, uint32_t src_ip, uint32_t dest_net,
		uint32_t *value_ptr);

/* hcnctr_iterate(counter,ptr, iter_ptr, src_ip_ptr, src_port_ptr,
 * 		dest_ip_ptr, dest_port_ptr, protocol_ptr, count_ptr):
 *
 * Retrieves the next counted pair and the associated count while iterating
 * over a given HcnCounter. Call this function repeatedly with a HASH_ITER
 * returned by hcnctr_create_iterator to iterate over all the entries
 * in the counter. This function makes no guarantees about the order of
 * retrieved pairs. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by hcnctr_create_iterator.
 * 	src_ip_ptr, dest_net_ptr:
 * 	Pointers to unsigned ints in which to place the next pair.
 * 	count_ptr: pointer to a uint32_t in which to place the associated count.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnctr_iterate(HcnCounter *counter_ptr, HASH_ITER *iter,
	uint32_t *src_ip_ptr, uint32_t *dest_net_ptr, uint32_t *count_ptr);

/* Macros for HCN counter features not interesting enough to be functions */

/* hcnctr_free(counter_ptr):
 *
 * Frees the storage used by a HcnCounter.
 *
 * Parameters:
 * 	counter_ptr: the HcnCounter to free.
 *
 * Returns:
 * 	void
*/
#define hcnctr_free(counter_ptr) \
    hashlib_free_table(&((counter_ptr)->_data));

/***************************************************************************
 * Host/CNetwork Statistics (7 byte key -> pointer)
***************************************************************************/

/* Host/C-Network stats data structure */
typedef struct _hcnstats {
	HashTable _data;
} HcnStats;

/* hcnstats_create_table(initial_size):
 *
 * Creates a new hash table for indexing statistics objects by host/C-network
 * pairs.
 *
 * Parameters:
 * 	initial_size: initial table size, in entries; use a "best guess" for
 * 		the number of entries the table will hold.
 *
 * Returns:
 * 	a pointer to the created HcnStats, or NULL if the HCN stats table
 * 	could not be created.
*/
HcnStats *hcnstats_create_table(uint32_t initial_size);

/* hcnstats_create_iterator(stats_ptr):
 *
 * Creates a new iterator for iterating a given HcnStats.
 *
 * Parameters:
 * 	stats_ptr: the HcnStats on which to iterate.
 *
 * Returns:
 * 	a new HASH_ITER for use with hcnstats_iterate.
*/
#define hcnstats_create_iterator(stats_ptr) \
    hashlib_create_iterator(&((stats_ptr)->_data))

/* hcnstats_insert(stats_ptr,src_ip,dest_net,stats_obj_ppptr):
 *
 * Associates a pair with a slot in the table used to store a pointer to
 * a statistics object, and returns a pointer to that slot. This rather
 * roundabout way of inserting allows you to use your own pointer
 * memory management scheme with the statistics table; see the documentation
 * for examples and more on this function's use.
 *
 * Parameters:
 * 	stats_ptr: the HcnStats into which to insert the record.
 * 	src_ip,dest_net: the host/network pair to insert.
 * 	stats_obj_ppptr: an out-parameter in which a pointer to the slot where
 * 	the pointer to the statistics object to be stored will be placed
 * 	is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnstats_insert(HcnStats *stats_ptr, uint32_t src_ip, uint32_t dest_net,
		void ***stats_obj_ppptr);

/* hcnstats_lookup(stats_ptr,src_ip,dest_net,stats_obj_pptr):
 *
 * Retrieves a pointer to the statistics object associated with a given
 * host/network pair.
 *
 * Parameters:
 * 	stats_ptr: the HcnStats in which to look up the record.
 * 	src_ip,dest_net: the host/network pair to look up.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 	statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnstats_lookup(HcnStats *stats_ptr, uint32_t src_ip, int32_t dest_net,
		void **stats_obj_pptr);

/* hcnstats_iterate(stats,ptr, iter_ptr, src_ip_ptr, dest_net_ptr,
 * 				stats_obj_pptr):
 *
 * Retrieves the next counted pair and the associated count while iterating
 * over a given HcnStats. Call this function repeatedly with a HASH_ITER
 * returned by hcnstats_create_iterator to iterate over all the entries
 * in the stats table. This function makes no guarantees about the order of
 * retrieved pairs. See the documentation for an example of iteration.
 *
 * Parameters:
 * 	stats_ptr: the HcnStats over which to iterate.
 * 	iter_ptr: the HASH_ITER returned by hcnstats_create_iterator.
 * 	src_ip_ptr, dest_net_ptr:
 * 	Pointers to unsigned ints in which to place the next pair.
 * 	stats_obj_pptr: an out-parameter in which the pointer to the
 * 	associated statistics object is returned.
 *
 * Returns:
 * 	A hashlib result code (see hashlib.h)
*/
int hcnstats_iterate(HcnStats *stats_ptr, HASH_ITER *iter,
		     uint32_t *src_ip_ptr, uint32_t *dest_net_ptr,
		     void **stats_obj_pptr);

/* Macros for HCN stats features not interesting enough to be functions */

/* hcnstats_free(stats_ptr):
 *
 * Frees the storage used by a HcnStats. Does NOT free any storage used
 * by the associated statistics objects.
 *
 * Parameters:
 * 	stats_ptr: the HcnStats to free.
 *
 * Returns:
 * 	void
*/
#define hcnstats_free(stats_ptr) \
    hashlib_free_table(&((stats_ptr)->_data));

/* Debug utilities */
#define DEBUG_DUMP_TABLE(fp, table_ptr) hashlib_dump_table(fp, &(table_ptr->_data));
void ipstats_dump_table(IpStats *stats_ptr, void (* dump_object)(void *));

/* Regression testing */
void ipstats_test();
void ipctr_test();
void tuplectr_test();

/* Useful utilities for addrtype hashing */
/* These don't belong here. Move them somewhere else and nuke 'em - bht */

/* Simplify masks out rightmost num_host_bits bits */
/* uint32_t netaddr_from_ip(uint32_t ip_addr, uint8_t num_host_bits);  */
/* (add later) */

/* Creates a hashtable that includes entries for a set of network
 * addresses.  File format TBD. */
/* HashTable create_hashtable_from_netmask_file(FILE *netmasks_fp); */
/* (add later) */



#define tuple_encode_key(key, fields, sip, dip, sp, dp, proto) { \
	int i = 0; \
	if (fields & FIELD_SRC_IP) { \
		memcpy(&key[i],&sip,sizeof(uint32_t)); i += sizeof(uint32_t); \
	} \
	if (fields & FIELD_DEST_IP) { \
		memcpy(&key[i],&dip,sizeof(uint32_t)); i += sizeof(uint32_t); \
	} \
	if (fields & FIELD_SRC_PORT) { \
		memcpy(&key[i],&sp,sizeof(uint16_t)); i += sizeof(uint16_t); \
	} \
	if (fields & FIELD_DEST_PORT) { \
		memcpy(&key[i],&dp,sizeof(uint16_t)); i += sizeof(uint16_t); \
	} \
	if (fields & FIELD_PROTOCOL) { \
		memcpy(&key[i],&proto,sizeof(uint8_t)); i += sizeof(uint8_t); \
	} \
	for (; i < TUPLE_KEYWIDTH; i++) { \
		key[i] = 0; \
	} \
}

#define tuple_decode_key(key, fields, sipp, dipp, spp, dpp, protop) {	\
	int i = 0; \
	if (fields & FIELD_SRC_IP) { \
		memcpy(sipp,&key[i],sizeof(uint32_t)); i += sizeof(uint32_t); \
	} \
	if (fields & FIELD_DEST_IP) { \
		memcpy(dipp,&key[i],sizeof(uint32_t)); i += sizeof(uint32_t); \
	} \
	if (fields & FIELD_SRC_PORT) { \
		memcpy(spp,&key[i],sizeof(uint16_t)); i += sizeof(uint16_t); \
	} \
	if (fields & FIELD_DEST_PORT) { \
		memcpy(dpp,&key[i],sizeof(uint16_t)); i += sizeof(uint16_t); \
	} \
	if (fields & FIELD_PROTOCOL) { \
		memcpy(protop,&key[i],sizeof(uint8_t)); i += sizeof(uint8_t); \
	} \
}


#endif /* _HASHWRAP_H */
