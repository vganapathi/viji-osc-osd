/*
 * Capabilities
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
#ifndef __CAP_H
#define __CAP_H

#include "osd-types.h"
#include "osd-util.h"

#define BIT(n)     (1 << n)
#define CREATE_MASK (OSD_CREATE|OSD_CREATE_AND_WRITE|OSD_CREATE_CLONE|OSD_CREATE_COLLECTION|OSD_CREATE_PARTITION|OSD_CREATE_USER_TRACKING_COLLECTION)

#define cap_permits_data_range(_cap_off_, _cap_len_, _dat_off_, \
  _dat_len_) \
  (((_cap_off_) <= (_dat_off_)) && \
   (((_cap_len_) == 0 && (_cap_off_) == 0) || \
    ((_cap_len_) == (uint64_t)(-1)) || \
    (((_cap_len_) >= (_dat_len_)) && \
     (((_cap_len_) - (_dat_len_)) >= ((_dat_off_) - (_cap_off_))))))

struct osd_timestamp {
            uint8_t time[6]; /* number of milliseconds since 1/1/1970 UT (big endian) */
} __packed;

/* v2 caps */
struct osd_capability_head {
        uint8_t format; /* low nibble */
        uint8_t integrity_algorithm__key_version; /* MAKE_BYTE(integ_alg, key_ver) */
        uint8_t security_method;
        uint8_t reserved1;
/*04*/  struct osd_timestamp expiration_time;
/*10*/  uint8_t audit[20];
/*30*/  uint8_t discriminator[12];
/*42*/  struct osd_timestamp object_created_time;
/*48*/  uint8_t object_type;
/*49*/  uint8_t permissions_bit_mask[5];
/*54*/  uint8_t reserved2;
/*55*/  uint8_t object_descriptor_type; /* high nibble */
} __packed;


/*56 v2*/
struct osd_cap_object_descriptor {
    union {
        struct {
        /*56*/    uint32_t allowed_attributes_access;
        /*60*/    uint32_t policy_access_tag;
        /*64*/    uint16_t boot_epoch;
        /*66*/    uint8_t reserved[6];
        /*72*/    uint64_t allowed_partition_id;
        /*80*/    uint64_t allowed_object_id;
        /*88*/    uint64_t allowed_range_length;
        /*96*/    uint64_t allowed_range_start;
        } __packed obj_desc;

        /*56*/    uint8_t object_descriptor[48];
    };
} __packed;
/*104 v2*/


struct osd_v2_capability {
    struct osd_capability_head h;
    struct osd_cap_object_descriptor od;
} __packed;


typedef enum {
	OSD_SEC_OBJ_ROOT = 0x1,
	OSD_SEC_OBJ_PARTITION = 0x2,
	OSD_SEC_OBJ_COLLECTION = 0x40,
	OSD_SEC_OBJ_USER = 0x80
} object_type_t;

typedef enum {
	OSD_SEC_CAP_APPEND	= BIT(0),
	OSD_SEC_CAP_OBJ_MGMT	= BIT(1),
	OSD_SEC_CAP_REMOVE	= BIT(2),
	OSD_SEC_CAP_CREATE	= BIT(3),
	OSD_SEC_CAP_SET_ATTR	= BIT(4),
	OSD_SEC_CAP_GET_ATTR	= BIT(5),
	OSD_SEC_CAP_WRITE	= BIT(6),
	OSD_SEC_CAP_READ	= BIT(7),

	OSD_SEC_CAP_NONE1	= BIT(8),
	OSD_SEC_CAP_NONE2	= BIT(9),
	OSD_SEC_GBL_REM 	= BIT(10), /*v2 only*/
	OSD_SEC_CAP_QUERY	= BIT(11), /*v2 only*/
	OSD_SEC_CAP_M_OBJECT	= BIT(12), /*v2 only*/
	OSD_SEC_CAP_POL_SEC	= BIT(13),
	OSD_SEC_CAP_GLOBAL	= BIT(14),
	OSD_SEC_CAP_DEV_MGMT	= BIT(15)
} osd_capability_bit_masks_t;

/* for object_descriptor_type (hi nibble used) */
typedef enum {
	OSD_SEC_OBJ_DESC_NONE = 0,     /* Not allowed */
	OSD_SEC_OBJ_DESC_OBJ = 1 << 4, /* v1: also collection */
	OSD_SEC_OBJ_DESC_PAR = 2 << 4, /* also root */
	OSD_SEC_OBJ_DESC_COL = 3 << 4  /* v2 only */
} descr_types_t;

typedef struct {
    object_type_t                 type;
    osd_capability_bit_masks_t    perm;
    descr_types_t                 desc;
} perm_table_t;

typedef enum {
    cap_scope_object,
    cap_scope_gco,
    cap_scope_group,
    cap_scope_dco,
    cap_scope_device
} cap_scope_t;

void cap_initialize_tables(void);
int cap_parse (uint8_t *, struct osd_v2_capability *);
int cap_check (struct osd_device *, uint8_t *, uint64_t , uint64_t , uint16_t);
int cap_check_time_version(struct osd_device*, struct osd_v2_capability *, uint64_t , uint64_t );
int cap_passes_basic_tests(void*, struct osd_v2_capability *, uint64_t , uint64_t );
int cap_check_allowed_range(struct osd_device*, struct osd_v2_capability *, uint64_t , uint64_t );
int cap_check_data_range(struct osd_v2_capability *, uint64_t , uint64_t , uint64_t , uint64_t );

#endif
