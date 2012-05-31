/*
 * Collections.
 *
 * Copyright (C) 2007 OSD Team <pvfs-osd@osc.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "osd-types.h"
#include "coll.h"
#include "osd-util/osd-util.h"
#include "list-entry.h"

/*
 * coll table stores many-to-many relationship between userobjects and
 * collections. Using this table members of a collection and collections to
 * which an object belongs can be computed efficiently.
 */

static const char *coll_tab_name = "coll";
struct coll_tab {
	char *name;             /* name of the table */
	sqlite3_stmt *insert;   /* insert a row */
	sqlite3_stmt *delete;   /* delete a row */
	sqlite3_stmt *delcid;   /* delete collection cid */
	sqlite3_stmt *deloid;   /* delete oid from all collections */
	sqlite3_stmt *emptycid; /* is collection empty? */
	sqlite3_stmt *getcid;   /* get collection */
	sqlite3_stmt *getoids;  /* get objects in a collection */
	sqlite3_stmt *copyoids; /* copy oids from one collection to another */
};


/*
 * returns:
 * -ENOMEM: out of memory
 * -EINVAL: invalid args
 * -EIO: if any prepare statement fails
 *  OSD_OK: success
 */
int coll_initialize(void *handle)
{
  osd_debug("%s: ", __func__);
  return 0;
}


int coll_finalize(void *handle)
{
  osd_debug("%s: ", __func__);
  return 0;
}


const char *coll_getname(void *handle)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/* 
 * @pid: partition id 
 * @cid: collection id
 * @oid: userobject id
 * @number: attribute number of cid in CAP of oid
 *
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int coll_insert(void *handle, uint64_t pid, uint64_t cid,
		uint64_t oid, uint32_t number)
{
  osd_debug("%s: ", __func__);
  return 0;
}

/* 
 * @pid: partition id 
 * @dest_cid: destination collection id (typically user tracking collection)
 * @source_cid: source collection id (whose objects will be tracked)
 *
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int coll_copyoids(void *handle, uint64_t pid, uint64_t dest_cid,
		  uint64_t source_cid)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/* 
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int coll_delete(void *handle, uint64_t pid, uint64_t cid, 
		uint64_t oid)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/* 
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int coll_delete_cid(void *handle, uint64_t pid, uint64_t cid)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/* 
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int coll_delete_oid(void *handle, uint64_t pid, uint64_t oid)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/*
 * tests whether collection is empty. 
 *
 * returns:
 * -EINVAL: invalid arg, ignore value of isempty
 * OSD_ERROR: in case of other errors, ignore value of isempty
 * OSD_OK: success, isempty is set to:
 * 	==1: if collection is empty or absent 
 * 	==0: if not empty
 */
int coll_isempty_cid(void *handle, uint64_t pid, uint64_t cid,
		     int *isempty)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg, cid is not set
 * OSD_ERROR: in case of any error, cid is not set
 * OSD_OK: success, cid is set to proper collection id
 */
int coll_get_cid(void *handle, uint64_t pid, uint64_t oid, 
		 uint32_t number, uint64_t *cid)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: other errors
 * OSD_OK: success, oids copied into outbuf, cont_id set if necessary
 */
int coll_get_oids_in_cid(void *handle, uint64_t pid, uint64_t cid, 
		       uint64_t initial_oid, uint64_t alloc_len, 
		       uint8_t *outdata, uint64_t *used_outlen,
		       uint64_t *add_len, uint64_t *cont_id)
{
  osd_debug("%s: ", __func__);
  return 0;
}

/*
 * Collection Attributes Page (CAP) of a userobject stores its membership in 
 * collections osd2r01 Sec 7.1.2.19.
 */
int coll_get_cap(sqlite3 *db, uint64_t pid, uint64_t oid, void *outbuf, 
		 uint64_t outlen, uint8_t listfmt, uint32_t *used_outlen)
{
  osd_debug("%s: ", __func__);
	return OSD_ERROR;
}

