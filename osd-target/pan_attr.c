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
#include <sys/ioctl.h>
#include <pan/osdfs/osd_ioctl.h>
#include <pan/osdfs/osd.h>

#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "osd-types.h"
#include "attr.h"
#include "osd.h"
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

const char *attr_getname(void* ohandle)
{
    osd_debug("%s:", __func__); 
    return "Attr_Name";
}

int attr_set_attr(struct osd_device *osd, uint64_t pid, uint64_t oid, uint32_t page, uint32_t number,
        const void *val, uint16_t len )
{
    uint32_t id = number;
    char osdname[64];
    char path[MAXPATHLEN];
    int ret;
    void *handle = osd->handle;

    osd_debug("%s: trying to set attr for number %llu with val %p len %llu ", __func__, llu(number), val,
            llu(len));

if (page == USER_INFO_PG) {
	switch (number) {
	case UIAP_USERNAME:
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
            /* osd-target extension: We let in a system_ID set on LUN
             * format command.
             */
	default:
            osd_debug("%s: READ ONLY attr %llu!", __func__, llu(number));
            return OSD_OK; 
	}
} else if (page == ROOT_INFO_PG ) {
    switch (number) {
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

            /* read only */
        case RIAP_CLOCK:
        case RIAP_VENDOR_IDENTIFICATION:
        case RIAP_PRODUCT_IDENTIFICATION:
        case RIAP_PRODUCT_MODEL:
        case RIAP_PRODUCT_REVISION_LEVEL:
        case RIAP_PRODUCT_SERIAL_NUMBER:
        case RIAP_TOTAL_CAPACITY:
        case RIAP_USED_CAPACITY:
        case RIAP_NUMBER_OF_PARTITIONS:
            osd_debug("%s: READ ONLY attr %llu!", __func__, llu(number));
            return OSD_OK; 
        default:
            osd_debug("%s: don't know how to set number %llu", __func__, llu(number));
            break;
    }
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
int _attr_set_attr(void* ohandle, uint64_t pid, uint64_t oid, 
        uint32_t page, uint32_t number, const void *val, 
        uint16_t len)
{
    struct handle* o_handle = ohandle;
    osd_debug("%s: page %llu num %llu pid %llu oid %llu val %p", 
            __func__, llu(page), llu(number), llu(pid), llu(oid), val); 

    int ret = 0;
    // Format = num attrs + (page, attr) +  length + data
    int tlen = sizeof(uint32_t) + len + (2* sizeof(uint16_t))
        + sizeof (uint16_t);
    uint8_t tmp[tlen];
    uint8_t *p = tmp;
    uint32_t num_attrs = 1;
    uint32_t id = MAKE_ATTRIBUTE(page, number);
    memcpy (p, &num_attrs, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &id, sizeof(id));
    p += sizeof(id);
    memcpy(p, &len, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy(p, val, len);

    struct osd_ioctl_setgetattribute iocsa;

    bzero(&iocsa, sizeof(struct osd_ioctl_setgetattribute));
    iocsa.hdr.cdb_flags = 0;
    iocsa.group = pid;
    iocsa.object = oid;

    struct osd_ioctl_attrs tmpattrs;
    bzero(&tmpattrs, sizeof(tmpattrs));
    tmpattrs.setattr_length = tlen;
    tmpattrs.setattr = (char *)tmp;;
    iocsa.attr = &tmpattrs; 

    ret = ioctl(o_handle->fd, OSD_IOCMD_SETATTRIBUTE, &iocsa);
    if (ret < 0) {
        osd_error("%s: setattr failed with %d err %s", __FUNCTION__, ret, strerror(errno));
    }

    return ret;
}

/*
 * returns:
 * -EINVAL: invalid arg
 * OSD_ERROR: some other error
 * OSD_OK: success
 */
int attr_delete_attr(void* ohandle, uint64_t pid, uint64_t oid, 
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
int attr_delete_all(void *ohandle, uint64_t pid, uint64_t oid)
{
    osd_debug("%s:", __func__); 
    return 0;
}

int attr_get_attr(struct osd_device *osd, uint64_t pid, uint64_t oid,
        uint32_t orig_page, uint32_t orig_number, uint64_t outlen, void *outdata, uint8_t listfmt, 
        uint32_t *used_outlen)
{
    uint32_t page = 0, id = 0;
    int ret;
    char path[MAXNAMELEN];
    char name[ATTR_PAGE_ID_LEN];
    char line[256];
    uint64_t len, tempval;
    const void *val = NULL;
    void *temp;
    struct stat sb;
    struct statfs sfs;
    uint8_t ll[8];
    off_t sz = 0;

    osd_debug("&&&---&&&%s: page %llu num %llu", __func__, llu(orig_page), llu(orig_number));
if (orig_page == USER_INFO_PG) {
	switch (orig_number) {
	case 0:
		len = ATTR_PAGE_ID_LEN;
		sprintf(name, "INCITS  T10 User Object Information");
		val = name;
		goto done;
	case UIAP_PID:
		set_htonll(ll, pid);
		len = UIAP_PID_LEN;
		val = ll;
		goto done;
	case UIAP_OID:
		set_htonll(ll, pid);
		len = UIAP_OID_LEN;
		val = ll;
		goto done;
	case UIAP_USED_CAPACITY:
		len = UIAP_USED_CAPACITY_LEN;
		get_dfile_name(path, osd->root, pid, oid);
		if (!oid) {
			ret = statfs(path, &sfs);
			if (ret != 0)
				return OSD_ERROR;

			sz = (sfs.f_blocks - sfs.f_bfree) * BLOCK_SZ;
		} else {
			ret = stat(path, &sb);
			if (ret != 0)
				return OSD_ERROR;

			sz = sb.st_blocks*BLOCK_SZ;
		}
		set_htonll(ll, sz);
		val = ll;
		goto done;
	case UIAP_LOGICAL_LEN:
		len = UIAP_LOGICAL_LEN_LEN;
		get_dfile_name(path, osd->root, pid, oid);
		ret = stat(path, &sb);
		if (ret != 0)
			return OSD_ERROR;
		set_htonll(ll, sb.st_size);
		val = ll;
		goto done;
	case PARTITION_CAPACITY_QUOTA:
		len = UIAP_USED_CAPACITY_LEN;
		get_dfile_name(path, osd->root, pid, oid);
		ret = statfs(path, &sfs);
		osd_debug("PARTITION_CAPACITY_QUOTA statfs(%s)=>%d size=0x%llx\n",
			path, ret, llu(sfs.f_blocks));
		if (ret != 0)
			return OSD_ERROR;
		sz = sfs.f_blocks * BLOCK_SZ;
		set_htonll(ll, sz);
		val = ll;
		goto done;
	case UIAP_USERNAME:
                page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_UserName);
                id = GET_ATTRIBUTE_ID(ATTR_DEVICE_UserName);
                goto get_attr;
	default:
		return OSD_ERROR;
	}
} else if(orig_page == ROOT_INFO_PG) {
    switch (orig_number) {
        case 0:
            /*{ROOT_PG + 1, 0, "INCITS  T10 Root Information"},*/
            sprintf(name, "INCITS  T10 Root Information");
            val = name;
            len = ATTR_PAGE_ID_LEN;
            goto done;
        case RIAP_OSD_SYSTEM_ID:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ID);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ID);
            goto pan_default;

        case RIAP_VENDOR_IDENTIFICATION:
            //outlen = RIAP_VENDOR_IDENTIFICATION_LEN;
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Vendor);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Vendor);
            goto pan_default;

        case RIAP_PRODUCT_IDENTIFICATION:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ProductID);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ProductID);
            goto pan_default;

        case RIAP_PRODUCT_MODEL:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ModelNumber);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ModelNumber);
            goto pan_default;

        case RIAP_PRODUCT_REVISION_LEVEL:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_ProductRev);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_ProductRev);
            goto pan_default;

        case RIAP_PRODUCT_SERIAL_NUMBER:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_SerialNumber);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_SerialNumber);
            goto pan_default;

        case RIAP_TOTAL_CAPACITY:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_TotalCapacity);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_TotalCapacity);
            goto get_attr;

        case RIAP_USED_CAPACITY:
            /*FIXME: return used capacity of osd->root device*/
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_TotalCapacity);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_TotalCapacity);
            ret = _attr_get_attr(osd->handle, pid, oid, page, id, page,
                    id, outlen, outdata, 0, used_outlen);
            //temp = &outdata[LE_VAL_OFF];
            int total_cap = *((int *)outdata);
            osd_debug("%s: TotalCapacity outdata %s cap %d page %u num %u", __func__, (char *)outdata, total_cap, page, id);

            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_RemainingCapacity);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_RemainingCapacity);
            ret = _attr_get_attr(osd->handle, pid, oid, page, id, page,
                    id, outlen, outdata, 0, used_outlen);
            total_cap -= *((int *)outdata);
            osd_debug("%s: Remain outdata %s cap %d page %u num %u", __func__, (char *)outdata, total_cap, page, id);
            set_htonll(ll, total_cap);
            val = ll;
            len = RIAP_USED_CAPACITY_LEN;
            goto done;

        case RIAP_NUMBER_OF_PARTITIONS:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_GroupCount);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_GroupCount);
            goto get_attr;

        case RIAP_CLOCK:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Clock);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Clock);
            goto get_attr;

        case RIAP_OSD_NAME:
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_UserName);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_UserName);
            goto get_attr;

        default:
            osd_debug("%s: FIX_ME: cannot recognize num %d\n", __func__, orig_number);
            break;

    }
}

get_attr:
    return _attr_get_attr(osd->handle, pid, oid, orig_page, orig_number, page, id, outlen, outdata, listfmt, used_outlen);

pan_default:
    len = sizeof("PANASAS-OSC_OSD");
    val = "PANASAS-OSC_OSD";

done:
    if (listfmt== RTRVD_SET_ATTR_LIST)
        ret = le_pack_attr(outdata, outlen, orig_page, orig_number, len, val);
    else if (listfmt == RTRVD_CREATE_MULTIOBJ_LIST)
        ret = le_multiobj_pack_attr(outdata, outlen, oid, orig_page, orig_number, len, val);
    else
        return OSD_ERROR;

    assert(ret == -EINVAL || ret == -EOVERFLOW || ret > 0);
    if (ret == -EOVERFLOW)
        *used_outlen = 0;
    else if (ret > 0)
        *used_outlen = ret;
    else
        return ret;

    return OSD_OK;
}


/*
 * get one attribute in list format.
 *
 * -EINVAL: invalid arg, ignore used_len 
 * -ENOENT: error, attribute not found
 * OSD_ERROR: some other error
 * OSD_OK: success, used_outlen modified
 */
int _attr_get_attr(void* ohandle, uint64_t pid, uint64_t oid,
        uint32_t orig_page, uint32_t orig_num, 
        uint32_t page, uint32_t number, uint64_t outlen,
        void *outdata, uint8_t listfmt, uint32_t *used_outlen)
{

    struct handle* o_handle = ohandle;
    uint8_t *tempbuf = outdata;
    int ret = 0;
    osd_debug("+++++%s: outdata %s, outlen %llu, used_outlen %llu page %llu num %llu pid %llu oid %llu listfmt %llu",  
            __func__, (char *)outdata, llu(outlen), llu(*used_outlen), 
            llu(page), llu(number), llu(pid), llu(oid), llu(listfmt)); 

    //num_attrs + id
    int tlen = sizeof(uint32_t) + sizeof(uint32_t);
    uint8_t tmp[tlen];
    uint8_t *p = tmp;
    uint32_t num_attrs = 1;
    uint32_t id = MAKE_ATTRIBUTE(page, number);
    memcpy (p, &num_attrs, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &id, sizeof(id));

    struct osd_attr_t out_attr;

    struct osd_ioctl_setgetattribute iocga;
    bzero(&iocga, sizeof(struct osd_ioctl_setgetattribute));
    iocga.group = pid;
    iocga.object = oid;
    iocga.hdr.cdb_flags = 0;

    struct osd_ioctl_attrs tmpattrs;
    bzero(&tmpattrs, sizeof(tmpattrs));
    tmpattrs.getattr_length = tlen;
    tmpattrs.getattr = (char *)tmp;;

    iocga.attr = &tmpattrs; 
    iocga.ret.rval_length = sizeof(struct osd_attr_t);
    iocga.ret.rval = (char *)&out_attr;

    osd_debug("%s: fd is %d", __func__, o_handle->fd);
    ret = ioctl(o_handle->fd, OSD_IOCMD_GETATTRIBUTE, &iocga);
    if (ret < 0) {
        osd_error("%s: getattr failed with %d %s", 
                __FUNCTION__, ret, strerror(errno));
        return ret;
    }
    osd_debug("++++++++%s: attr.val %p, attr.len %u", 
            __func__, out_attr.value, out_attr.length);
    if(listfmt == 0) {
        //Don't format the values
        outdata = out_attr.value;
        outlen = out_attr.length;
        osd_debug("%s: returning without formatting!", __func__);
        return 0;
    }
    ret = le_pack_attr(outdata, outlen, orig_page, orig_num, out_attr.length, out_attr.value);

    outlen -= ret;
    tempbuf += ret;
    outdata = tempbuf;
    assert(ret == -EINVAL || ret == -EOVERFLOW || ret > 0);
    if (ret == -EOVERFLOW)
        *used_outlen = 0;
    else if (ret > 0)
        *used_outlen = ret;
    else
        return ret;
    osd_debug("+++++%s: outdata %s, outlen %llu, used_outlen %llu page %llu num %llu pid %llu oid %llu listfmt %llu",  
            __func__, (char *)outdata, llu(outlen), llu(*used_outlen), llu(page), llu(number), llu(pid), llu(oid), llu(listfmt)); 

    return OSD_OK;

}


/*
 * get one attribute value.
 *
 * -EINVAL: invalid arg, ignore used_len 
 * -ENOENT: error, attribute not found
 * OSD_ERROR: some other error
 * OSD_OK: success, used_outlen modified
 */
int attr_get_val(void* ohandle, uint64_t pid, uint64_t oid,
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
int attr_get_page_as_list(void* ohandle, uint64_t pid, uint64_t oid,
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
int attr_get_for_all_pages(void* ohandle, uint64_t pid, uint64_t oid, 
        uint32_t number, uint64_t outlen, void *outdata,
        uint8_t listfmt, uint32_t *used_outlen)
{
    int max_defined_pages = 6;
    int ret = 0;
    int p;

    osd_debug("%s: attr (%llu %llu * %u) not found!", __func__, 
            llu(pid), llu(oid), number);

    for ( p = 0; p < max_defined_pages; p++){
        ret = _attr_get_attr(ohandle, pid, oid, p, number, p, number, outlen, outdata, listfmt, used_outlen);
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
int attr_get_all_attrs(void* ohandle, uint64_t pid, uint64_t oid,
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
int attr_get_dir_page(void* ohandle, uint64_t pid, uint64_t oid, 
        uint32_t page, uint64_t outlen, void *outdata,
        uint8_t listfmt, uint32_t *used_outlen)
{
    osd_debug("%s: dir page not found!", __func__);

    return 0;
}

int attr_get_conversion(void* ohandle, uint64_t pid, uint64_t oid,
        uint32_t page, uint32_t number, uint64_t outlen,
        void *outdata, uint8_t listfmt, uint32_t *used_outlen)
{

    uint32_t orig_page = page, orig_num = number;

    page &= 0xffff; 
    page += MAX_STD_ATTR_PAGE;
    page += 1;
    number &= 0xffff;
    return _attr_get_attr(ohandle, pid, oid, orig_page, orig_num, 
            page, number, outlen, outdata, listfmt, used_outlen);
}

int attr_set_conversion(void* ohandle, uint64_t pid, uint64_t oid, 
        uint32_t page, uint32_t number, const void *val, 
        uint16_t len)
{

    //User defined page
    page &= 0xffff; 
    page += MAX_STD_ATTR_PAGE;
    page += 1;
    number &= 0xffff;

    return _attr_set_attr(ohandle, pid, oid, page, number, val, len);
}
