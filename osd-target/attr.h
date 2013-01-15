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
#ifndef __ATTR_H
#define __ATTR_H

#include "osd-types.h"

int attr_initialize(void *dbc);

int attr_finalize(void *dbc);

const char *attr_getname(void *o_handle);

int attr_set_conversion(void* o_handle, uint64_t pid, uint64_t oid, 
    uint32_t page, uint32_t number, const void *val, uint16_t len);

int attr_set_attr(struct osd_device *osd, uint64_t pid, uint64_t oid, 
		  uint32_t page, uint32_t number, const void *val, uint16_t len);

int _attr_set_attr(void* o_handle, uint64_t pid, uint64_t oid, 
    uint32_t page, uint32_t number, const void *val, uint16_t len);

int attr_delete_attr(void *o_handle, uint64_t pid, uint64_t oid, 
		     uint32_t page, uint32_t number);

int attr_delete_all(void *o_handle, uint64_t pid, uint64_t oid);

int attr_get_conversion(void *o_handle, uint64_t pid, uint64_t oid, uint32_t page,
		  uint32_t number, uint64_t outlen, void *outdata, uint8_t listfmt,
                  uint32_t *used_outlen);

int attr_get_attr(struct osd_device *osd, uint64_t pid, uint64_t oid, uint32_t page,
		  uint32_t number, uint64_t outlen, void *outdata, uint8_t listfmt,
                  uint32_t *used_outlen);

int _attr_get_attr(void *o_handle, uint64_t pid, uint64_t oid,
		  uint32_t orig_page, uint32_t orig_number,
		  uint32_t page, uint32_t number, uint64_t outlen,
		  void *outdata, uint8_t listfmt, uint32_t *used_outlen);

int attr_get_val(void *o_handle, uint64_t pid, uint64_t oid,
		 uint32_t page, uint32_t number, uint64_t outlen,
		 void *outdata, uint32_t *used_outlen);

int attr_get_page_as_list(void *o_handle, uint64_t pid, uint64_t oid,
			  uint32_t page, uint64_t outlen, void *outdata,
			  uint8_t listfmt, uint32_t *used_outlen);

int attr_get_for_all_pages(void *o_handle, uint64_t pid, uint64_t oid, 
			   uint32_t number, uint64_t outlen, void *outdata,
			   uint8_t listfmt, uint32_t *used_outlen);

int attr_get_all_attrs(void *o_handle, uint64_t pid, uint64_t oid,
		       uint64_t outlen, void *outdata, uint8_t listfmt, 
		       uint32_t *used_outlen);

int attr_get_dir_page(void *o_handle, uint64_t pid, uint64_t oid, 
		      uint32_t page, uint64_t outlen, void *outbuf,
		      uint8_t listfmt, uint32_t *used_outlen);

#endif /* __ATTR_H */
