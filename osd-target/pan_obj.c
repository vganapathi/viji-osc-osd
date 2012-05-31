/*
 * Object table.
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <pan/osdfs/osd_ioctl.h>

#include "osd.h"
#include "osd-util/osd-util.h"
#include "obj.h"
#include "db.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* obj table tracks the presence of objects in the OSD */

static const char *obj_tab_name = "obj";
struct obj_tab {
	char *name;             /* name of the table */
	sqlite3_stmt *insert;   /* insert a row */
	sqlite3_stmt *delete;   /* delete a row */
	sqlite3_stmt *delpid;   /* delete all rows for pid */
	sqlite3_stmt *nextoid;  /* get next oid */
	sqlite3_stmt *nextpid;  /* get next pid */
	sqlite3_stmt *isprsnt;  /* is object present */
	sqlite3_stmt *emptypid; /* is partition empty */
	sqlite3_stmt *gettype;  /* get type of the object */
	sqlite3_stmt *getoids;  /* get oids in a pid */
	sqlite3_stmt *getcids;  /* get cids in pid */
	sqlite3_stmt *getpids;  /* get pids in db */
};


/*
 * returns:
 * -ENOMEM: out of memory
 * -EINVAL: invalid args
 * -EIO: if any prepare statement fails
 *  OSD_OK: success
 */
int obj_initialize(struct handle* ohandle)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


int obj_finalize(struct handle* ohandle)
{
  
  osd_debug("%s: \n", __func__);
	return 0;
}


const char *obj_getname(struct handle* ohandle)
{
	return "pan_obj_name";
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int obj_insert(struct handle* handle, uint64_t pid, uint64_t oid, 
	       uint32_t type)
{

  int ret = 0;

  osd_debug("%s: fd %d pid %llu oid %llu \n", __func__, handle->fd, llu(pid), llu(oid));

  if(oid){
    struct osd_ioctl_createobject iocco;
    iocco.group = pid;
    iocco.object = oid;
    ret = ioctl(handle->fd, OSD_IOCMD_CREATEOBJECT, &iocco);
  } else {
    struct osd_ioctl_createobjectgroup ioccog;
    ioccog.group = pid;
    ret = ioctl(handle->fd, OSD_IOCMD_CREATEOBJECTGROUP, &ioccog);
  }
	if (ret < 0) {
		osd_error("%s: create obj failed err %s\n", __func__, strerror(errno));
	}

	return ret;
}


/*
 * NOTE: If the object is not present, the function completes successfully.
 *
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int obj_delete(struct handle* handle, uint64_t pid, uint64_t oid)
{
  int ret = 0;

  osd_debug("%s: pid %llu oid %llu\n", __func__, llu(pid), llu(oid));

  if(oid){
    struct osd_ioctl_removeobject iocro;
    iocro.group = pid;
    iocro.object = oid;
    iocro.cdb_flags = 0;
    iocro.capacity_remaining = 0;
    iocro.capacity_freed = 0;
    ret = ioctl(handle->fd, OSD_IOCMD_REMOVEOBJECT, &iocro);
  } else {
    struct osd_ioctl_removeobjectgroup iocrog;
    iocrog.group = pid;
    iocrog.cdb_flags = OSD_REMOVE_NONEMPTY;
    ret = ioctl(handle->fd, OSD_IOCMD_REMOVEOBJECTGROUP, &iocrog);
  }
	if (ret < 0) {
		osd_error("%s: delete obj failed err %d %s\n", __func__, ret, strerror(errno));
	}

  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int obj_delete_pid(struct handle* ohandle, uint64_t pid)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/*
 * return values
 * -EINVAL: invalid args
 * OSD_ERROR: some other sqlite error
 * OSD_OK: success
 * 	oid = next oid if pid has some oids
 * 	oid = 1 if pid was empty or absent. caller must assign correct oid.
 */
int obj_get_nextoid(struct handle* ohandle, uint64_t pid, uint64_t *oid)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/*
 * return values
 * -EINVAL: invalid args
 * OSD_ERROR: some other sqlite error
 * OSD_OK: success
 * 	pid = next pid if OSD has some pids
 * 	pid = 1 if pid not in db. caller must assign correct pid.
 */
int obj_get_nextpid(struct handle* ohandle, uint64_t *pid)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/* 
 * NOTE: type not in arg, since USEROBJECT and COLLECTION share namespace 
 * and (pid, oid) is unique.
 *
 * returns:
 * -EINVAL: invalid arg, ignore value of *present
 * OSD_ERROR: some other error, ignore value of *present
 * OSD_OK: success, *present set to the following:
 * 	0: object is absent
 * 	1: object is present
 */
int obj_ispresent(struct handle* ohandle, uint64_t pid, uint64_t oid, 
		  int *present)
{
  struct stat sobj;
  char path[MAXPATHLEN];


  if(oid)
    sprintf(path, "/pandata/%llu/%llu", llu(pid), llu(oid));
  else
    sprintf(path, "/pandata/%llu", llu(pid));

  osd_debug("%s: path %s\n", __func__, path);

  if(stat(path, &sobj) != 0){
    osd_debug("%s: pid %llu oid %llu does not exist!", __func__, llu(pid), llu(oid));
    *present = 0;
  }else{
    osd_debug("%s: pid %llu oid %llu exists!", __func__, llu(pid), llu(oid));
    *present = 1;
  }
  
  return 0;
}


/*
 * tests whether partition is empty. 
 *
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: in case of other errors, ignore value of *isempty
 * OSD_OK: success, *isempty set to:
 * 	==1: if partition is empty or absent or in case of sqlite error
 * 	==0: if partition is not empty
 */
int obj_isempty_pid(struct handle* ohandle, uint64_t pid, int *isempty)
{
  osd_debug("%s: pid %llu ", __func__, llu(pid));

  int ret = 0;
	char path[MAXNAMELEN];
	DIR *dir = NULL;
	struct dirent *ent = NULL;
  *isempty = 0;

  sprintf(path, "/pandata/%llu", llu(pid)); 
  osd_debug("%s: path %s", __func__, path);

  if((dir = opendir(path)) == NULL){
    osd_error("%s: could not open dir!", __func__);
    return -1;
  }

  if((ent = readdir(dir)) == NULL) {
    osd_debug("%s: dir is empty", __func__);
    *isempty = 1;
  } else {
    osd_error("%s: dir is NOT empty", __func__);
  }

  return 0;
}

/*
 * return the type of the object
 *
 * returns:
 * -EINVAL: invalid arg, ignore value of obj_type
 * OSD_ERROR: any other error, ignore value of obj_type
 * OSD_OK: success in determining the type, either valid or invalid. 
 * 	obj_types set to the determined type.
 */
int obj_get_type(struct handle* ohandle, uint64_t pid, uint64_t oid, 
		 uint8_t *obj_type)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success. oids copied into outdata, contid, used_outlen and 
 * 	add_len are set accordingly
 */
int obj_get_oids_in_pid(struct handle* ohandle, uint64_t pid, 
			uint64_t initial_oid, uint64_t alloc_len, 
			uint8_t *outdata, uint64_t *used_outlen, 
			uint64_t *add_len, uint64_t *cont_id)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success. cids copied into outdata, contid, used_outlen and 
 * 	add_len are set accordingly
 */
int obj_get_cids_in_pid(struct handle* ohandle, uint64_t pid, 
			uint64_t initial_cid, uint64_t alloc_len, 
			uint8_t *outdata, uint64_t *used_outlen, 
			uint64_t *add_len, uint64_t *cont_id)
{
  osd_debug("%s: \n", __func__);
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success. pids copied into outdata, contid, used_outlen and 
 * 	add_len are set accordingly
 */
int obj_get_all_pids(struct handle* ohandle, uint64_t initial_pid, 
		     uint64_t alloc_len, uint8_t *outdata,
		     uint64_t *used_outlen, uint64_t *add_len,
		     uint64_t *cont_id)
{
  osd_debug("%s: \n", __func__);
  return 0;
}

