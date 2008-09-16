#ifndef __COLL_H
#define __COLL_H

#include <sqlite3.h>
#include "osd-types.h"


int coll_initialize(struct db_context *dbc);

int coll_finalize(struct db_context *dbc);

const char *coll_getname(struct db_context *dbc);

int coll_insert(struct db_context *dbc, uint64_t pid, uint64_t cid,
		uint64_t oid, uint32_t number);

int coll_delete(struct db_context *dbc, uint64_t pid, uint64_t cid, 
		uint64_t oid);

int coll_delete_cid(struct db_context *dbc, uint64_t pid, uint64_t cid);

int coll_delete_oid(struct db_context *dbc, uint64_t pid, uint64_t oid);

int coll_isempty_cid(struct db_context *dbc, uint64_t pid, uint64_t cid,
		     int *isempty);

int coll_get_cid(struct db_context *dbc, uint64_t pid, uint64_t oid, 
		 uint32_t number, uint64_t *cid);

int coll_get_cap(sqlite3 *db, uint64_t pid, uint64_t oid, void *outbuf, 
		 uint64_t outlen, uint8_t listfmt, uint32_t *used_outlen);

int coll_get_oids_in_cid(struct db_context *dbc, uint64_t pid, uint64_t cid, 
			 uint64_t initial_oid, uint64_t alloc_len, 
			 uint8_t *outdata, uint64_t *used_outlen,
			 uint64_t *add_len, uint64_t *cont_id);

#endif /* __COLL_H */
