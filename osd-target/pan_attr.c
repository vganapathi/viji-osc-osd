/*
 * Attributes.
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
#include <unistd.h>
#include <sys/stat.h>

#include <pan/osdfs/osd.h>

#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "osd-types.h"
#include "attr.h"
#include "osd-util/osd-util.h"
#include "list-entry.h"

#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x < _y ? _x : _y; })


/* attr table stores all the attributes of all the objects in the OSD */

/* 40 bytes including terminating NUL */
static const char unid_page[ATTR_PAGE_ID_LEN] = 
"        unidentified attributes page   ";

const char *attr_getname(struct handle* ohandle)
{
		osd_debug("%s:", __func__); 
	return "Attr_Name";
}

int attr_set_attr(struct handle *handle, uint64_t pid, uint64_t oid, uint32_t number,
       const void *val, uint16_t len )
{
  uint32_t page;
  uint32_t id;
  char osdname[64];
  char path[MAXPATHLEN];
  int ret;

  osd_debug("%s: trying to set attr for number %llu with val %p", __func__, llu(number), val);

  switch (number) {
    /* osd-target extension: We let in a system_ID set on LUN
     * format command.
     */
    case RIAP_OSD_SYSTEM_ID:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ID);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ID);
      len = RIAP_OSD_SYSTEM_ID_LEN;
      break;
    case RIAP_OSD_NAME: 

      snprintf(osdname, min((uint16_t)64U, len), "%s",
          (const char *)val);
      osd_info("RIAP_OSD_NAME [%s]\n", osdname);

      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_UserName);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_UserName);
      break;
    case UIAP_LOGICAL_LEN: 
      len = get_ntohll((const uint8_t *)val);
      sprintf(path, "/root/dfiles/%llu", llu(pid));
      if(oid)
        sprintf(path, "%s/%llu/data", path, llu(oid));

      osd_debug("%s: %s %llu\n", __func__, path, llu(len));
      ret = truncate(path, len);
      if (ret < 0)
        return OSD_ERROR;
      else
        return OSD_OK;
      break;


    /* read only */
    case RIAP_VENDOR_IDENTIFICATION:
    case RIAP_PRODUCT_IDENTIFICATION:
    case RIAP_PRODUCT_MODEL:
    case RIAP_PRODUCT_REVISION_LEVEL:
    case RIAP_PRODUCT_SERIAL_NUMBER:
    case RIAP_TOTAL_CAPACITY:
    case RIAP_USED_CAPACITY:
    case RIAP_NUMBER_OF_PARTITIONS:
    default:
      osd_debug("%s: don't know how to set number %llu", __func__, llu(number));
      return OSD_ERROR;
  }

  return _attr_set_attr(handle, pid, oid, page, id, val, len);
}

/*
 * Note: Current SQLITE INSERT syntax does not support bulk inserts in a
 * single INSERT SQL statement. Therefore this function needs to be called
 * for each table insert.
 *
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int _attr_set_attr(struct handle* ohandle, uint64_t pid, uint64_t oid, 
    uint32_t page, uint32_t number, const void *val, 
    uint16_t len)
{
  struct osd_attr_t attr;
  int fd = 1;
  memset(&attr, 0, sizeof(attr));
  attr.page = page;
  attr.attr = number;
  attr.length = len;

  if (val) {
    memcpy(attr.value, val, attr.length);
  }

  int hand = ohandle->fd;
  int rc = syscall(SYS_setosdattr, ohandle->fd, &attr);
  if (rc) {
    osd_error("%s: Error in SYS_setosdattr: "
        "%s", __func__, strerror(errno));
    return 1;
  }

  return 0;
}

/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int attr_delete_attr(struct handle* ohandle, uint64_t pid, uint64_t oid, 
    uint32_t page, uint32_t number)
{
  osd_debug("%s:", __func__); 
  return 0;
}


/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int attr_delete_all(struct handle *ohandle, uint64_t pid, uint64_t oid)
{
  osd_debug("%s:", __func__); 
  return 0;
}

int attr_get_attr(struct handle *ohandle, uint64_t pid, uint64_t oid,
    uint32_t number, uint64_t outlen, void *outdata, uint8_t listfmt, 
    uint32_t *used_outlen)
{
  uint32_t page = 0, id = 0;
  void *val;
  int ret;
  char path[MAXNAMELEN];
  uint64_t len, tempval;
  char name[MAXNAMELEN];
  struct stat sb;
	struct statfs sfs;
	uint8_t ll[8];
	off_t sz = 0;

  osd_debug("%s: getting attributes for num %llu\n", __func__, llu(number));
  switch (number) {
    case 0:
      /*{ROOT_PG + 1, 0, "INCITS  T10 Root Information"},*/
      len = ATTR_PAGE_ID_LEN;
      sprintf(name, "INCITS  T10 Root Information");
      val = name;
      break;
    case RIAP_OSD_SYSTEM_ID:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ID);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ID);
      break;
    case RIAP_VENDOR_IDENTIFICATION:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Vendor);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Vendor);
      break;
    case RIAP_PRODUCT_IDENTIFICATION:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ProductID);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ProductID);
      break;
    case RIAP_PRODUCT_MODEL:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ModelNumber);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ModelNumber);
      break;
    case RIAP_PRODUCT_REVISION_LEVEL:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ProductRev);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ProductRev);
      break;
    case RIAP_PRODUCT_SERIAL_NUMBER:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_SerialNumber);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_SerialNumber);
      break;
    case RIAP_TOTAL_CAPACITY:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_TotalCapacity);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_TotalCapacity);
      break;
    case RIAP_USED_CAPACITY:
      /*FIXME: return used capacity of osd->root device*/
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_TotalCapacity);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_TotalCapacity);
      ret = _attr_get_attr(ohandle, pid, oid, page,
          id, outlen, outdata, listfmt, used_outlen);
      osd_error("%s: TotalCapacity id attr_get_attr return %d", __func__, ret);
      tempval = (int)(*((int *)outdata));
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_RemainingCapacity);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_RemainingCapacity);
      ret = _attr_get_attr(ohandle, pid, oid, page,
          id, outlen, outdata, listfmt, used_outlen);
      tempval -= (int)(*((int *)outdata)); 
      val = &tempval;
      osd_error("%s: RemainingCapacity id attr_get_attr return %d", __func__, ret);
      break;
    case RIAP_NUMBER_OF_PARTITIONS:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_GroupCount);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_GroupCount);
      break;
    case RIAP_CLOCK:
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Clock);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Clock);
      break;
    case RIAP_OSD_NAME:
      osd_error("%s: calling osdname attr_get_attr", __func__);
      page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Name);
      id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Name);
      break;
    case UIAP_LOGICAL_LEN:
      len = UIAP_LOGICAL_LEN_LEN;
      sprintf(path, "/root/dfiles/%llu", llu(pid));
      if(oid)
        sprintf(path, "%s/%llu/data", path, llu(oid));
      ret = stat(path, &sb);
      if (ret != 0)
        return OSD_ERROR;
      set_htonll(ll, sb.st_size);
      val = ll;
      break;
    case PARTITION_CAPACITY_QUOTA:
      len = UIAP_USED_CAPACITY_LEN;
      sprintf(path, "/root/dfiles/%llu", llu(pid));
      if(oid)
        sprintf(path, "%s/%llu/data", path, llu(oid));
      ret = statfs(path, &sfs);
      osd_debug("PARTITION_CAPACITY_QUOTA statfs(%s)=>%d size=0x%llx\n",
			path, ret, llu(sfs.f_blocks));
      if (ret != 0)
        return OSD_ERROR;
      sz = sfs.f_blocks * BLOCK_SZ;
      set_htonll(ll, sz);
      val = ll;
      break;

    default:
      osd_debug("%s: cannot recognize num %d\n", __func__, number);
      return OSD_ERROR;

  }
  return _attr_get_attr(ohandle, pid, oid, page, id, outlen, outdata, listfmt, used_outlen);
}


  /*
   * get one attribute in list format.
   *
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int _attr_get_attr(struct handle* ohandle, uint64_t pid, uint64_t oid,
      uint32_t page, uint32_t number, uint64_t outlen,
      void *outdata, uint8_t listfmt, uint32_t *used_outlen)
  {

    struct osd_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.page = page;
    attr.attr = number;
    int rc = syscall(SYS_getosdattr, ohandle->fd, &attr);
    if (rc == -1) {
      osd_error("%s: Error in SYS_getosdattr: "
          "%s.\n", __func__, strerror(errno));
      return 1;
    }
    *used_outlen = 0;
    outlen = 0;
    if (attr.length != 0) {
      *used_outlen = attr.length;
      memset(outdata, 0, *used_outlen);
      memcpy(outdata, attr.value, *used_outlen);
      outlen = 0;
    }
    osd_debug("%s: outdata svalue is %s, outlen %llu, used_outlen %llu, max %llu", __func__, (char *)outdata, 
        llu(outlen), llu(used_outlen), llu(ATTR_MAX_LENGTH));
    return rc;

  }


  /*
   * get one attribute value.
   *
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int attr_get_val(struct handle* ohandle, uint64_t pid, uint64_t oid,
      uint32_t page, uint32_t number, uint64_t outlen,
      void *outdata, uint32_t *used_outlen)
  {
    osd_debug("%s: attr (%llu %llu %u %u) not found!", __func__, 
        llu(pid), llu(oid), page, number);

    return 0;
  }

  /*
   * get one page in list format
   *
   * XXX:SD If the page is defined and we don't have name for the page (attr
   * num == 0), then we don't return its name. That is a bug
   *
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int attr_get_page_as_list(struct handle* ohandle, uint64_t pid, uint64_t oid,
      uint32_t page, uint64_t outlen, void *outdata,
      uint8_t listfmt, uint32_t *used_outlen)
  {
    osd_debug("%s: attr (%llu %llu %u *) not found!", __func__, 
        llu(pid), llu(oid), page);

    return 0;
  }


  /*
   * for each defined page of an object get attribute with specified number
   *
   * XXX:SD If the page is defined and we don't have name for the page (attr
   * num == 0), then we don't return its name. That is a bug
   *
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int attr_get_for_all_pages(struct handle* ohandle, uint64_t pid, uint64_t oid, 
      uint32_t number, uint64_t outlen, void *outdata,
      uint8_t listfmt, uint32_t *used_outlen)
  {
    int max_defined_pages = 6;
    int ret = 0;
    int p;

    osd_debug("%s: attr (%llu %llu * %u) not found!", __func__, 
        llu(pid), llu(oid), number);

    for ( p = 0; p < max_defined_pages; p++){
      ret = _attr_get_attr(ohandle, pid, oid, p, number, outlen, outdata, listfmt, used_outlen);
    }

    return ret;
  }


  /*
   * get all attributes for an object in a list format
   *
   * XXX:SD If the page is defined and we don't have name for the page (attr
   * num == 0), then we don't return its name. That is a bug
   *
   * returns: 
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int attr_get_all_attrs(struct handle* ohandle, uint64_t pid, uint64_t oid,
      uint64_t outlen, void *outdata, uint8_t listfmt, 
      uint32_t *used_outlen)
  {

    osd_debug("%s: attr (%llu %llu * *) not found!", __func__, 
        llu(pid), llu(oid));

    return 0;
  }


  /*
   * get the directory page of the object.
   *
   * returns:
   * -EINVAL: invalid arg, ignore used_len 
   * -ENOENT: error, attribute not found
   * OSD_ERROR: some other error
   * OSD_OK: success, used_outlen modified
   */
  int attr_get_dir_page(struct handle* ohandle, uint64_t pid, uint64_t oid, 
      uint32_t page, uint64_t outlen, void *outdata,
      uint8_t listfmt, uint32_t *used_outlen)
  {
    osd_debug("%s: dir page not found!", __func__);

    return 0;
  }

