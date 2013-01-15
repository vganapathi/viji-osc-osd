/*
 * OSD commands.
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
#ifndef __IO_H
#define __IO_H

#include "osd-types.h"

int contig_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
		       uint64_t len, uint64_t offset, uint8_t *outdata, 
		       uint64_t *used_outlen, uint8_t *sense);

int sgl_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
		    uint64_t len, uint64_t offset, const struct sg_list *sglist,
		    uint8_t *outdata, uint64_t *used_outlen, uint8_t *sense);

int vec_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
		    uint64_t len, uint64_t offset, const uint8_t *indata,
		    uint8_t *outdata, uint64_t *used_outlen, uint8_t *sense);

int contig_write(struct osd_device *osd, uint64_t pid, uint64_t oid, 
			uint64_t len, uint64_t offset, const uint8_t *dinbuf, 
			uint8_t *sense);

int sgl_write(struct osd_device *osd, uint64_t pid, uint64_t oid, 
		     uint64_t len, uint64_t offset, const uint8_t *dinbuf,
		     const struct sg_list *sglist,
		     uint8_t *sense); 

int vec_write(struct osd_device *osd, uint64_t pid, uint64_t oid,
		     uint64_t len, uint64_t offset, const uint8_t *dinbuf,
		     uint8_t *sense);

int setup_root_paths (const char* root, struct osd_device *osd);

int osd_create_datafile(struct osd_device *osd, uint64_t pid, uint64_t oid); 

int format_osd(struct osd_device *osd, uint64_t capacity, uint32_t cdb_cont_len, uint8_t *sense);



#endif /* __IO_H */
