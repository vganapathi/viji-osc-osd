/*
 * Multi-table queries.
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
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "osd-types.h"
#include "obj.h"
#include "attr.h"
#include "coll.h" 
#include "mtq.h"
#include "osd-util/osd-util.h"
#include "list-entry.h"

/*
 * mtq: multitable query, all queries dealing with >=2 tables are implemented
 * here
 */

/*
 * return values:
 * -EINVAL: invalid argument
 * -EIO: prepare or some other sqlite function failed
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int mtq_run_query(void *handle, uint64_t pid, uint64_t cid, 
		  struct query_criteria *qc, void *outdata, 
		  uint32_t alloc_len, uint64_t *used_outlen)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/*
 * returns list of objects along with requested attributes
 *
 * return values:
 * -EINVAL: invalid argument
 * -EIO: prepare or some other sqlite function failed
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int mtq_list_oids_attr(void *handle, uint64_t pid,
		       uint64_t initial_oid, struct getattr_list *get_attr,
		       uint64_t alloc_len, void *outdata, 
		       uint64_t *used_outlen, uint64_t *add_len, 
		       uint64_t *cont_id)
{
  osd_debug("%s: ", __func__);
  return 0;
}


/*
 * set attributes on members of the give collection
 *
 * return values:
 * -EINVAL: invalid argument
 * -EIO: prepare or some other sqlite function failed
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int mtq_set_member_attrs(void *handle, uint64_t pid, uint64_t cid, 
			 struct setattr_list *set_attr)
{
  osd_debug("%s: ", __func__);
  return 0;
}


