#ifndef __OBJECT_COLLECTION_H
#define __OBJECT_COLLECTION_H

#include <sqlite3.h>
#include "osd-types.h"

int oc_insert_row(sqlite3 *db, uint64_t pid, uint64_t cid, uint64_t oid, 
		  uint32_t number);

int oc_delete_row(sqlite3 *db, uint64_t pid, uint64_t cid, uint64_t oid);

int oc_delete_all_cid(sqlite3 *db, uint64_t pid, uint64_t cid);

int oc_delete_all_oid(sqlite3 *db, uint64_t pid, uint64_t oid);

int oc_isempty_cid(sqlite3 *db, uint64_t pid, uint64_t cid);

int oc_get_cid(sqlite3 *db, uint64_t pid, uint64_t oid, uint32_t number, 
	       uint64_t *cid);

int oc_get_cap(sqlite3 *db, uint64_t pid, uint64_t oid, void *outbuf, 
	       uint64_t outlen, uint8_t listfmt, uint32_t *used_outlen);

int oc_get_oids_in_cid(sqlite3 *db, uint64_t pid, uint64_t cid, void **buf);

#endif /* __OBJECT_COLLECTION_H */
