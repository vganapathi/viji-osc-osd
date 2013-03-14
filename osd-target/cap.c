/*
 * Capability checking
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cap.h"

perm_table_t cap_perm_table[40];

/**
 * @doc
 *   Setup permissions table
 */
void
cap_initialize_tables() 
{

    bzero(&cap_perm_table, sizeof(cap_perm_table));

    //command, object type, caps, descriptor name
#define _perm(c, t, p, d)       \
    cap_perm_table[c].type = t; \
    cap_perm_table[c].perm = p; \
    cap_perm_table[c].desc = d

    _perm(OSD_APPEND, OSD_SEC_OBJ_USER, OSD_SEC_CAP_APPEND, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_CLEAR, OSD_SEC_OBJ_USER, OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_OBJ);
    //FIXME: same as clone
    _perm(OSD_COPY_USER_OBJECTS,  OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_CREATE & OSD_SEC_CAP_READ & OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_PAR); 
    _perm(OSD_CREATE, OSD_SEC_OBJ_USER, OSD_SEC_CAP_CREATE, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_CREATE_AND_WRITE, OSD_SEC_OBJ_USER, OSD_SEC_CAP_CREATE & OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_CREATE_CLONE, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_CREATE & OSD_SEC_CAP_READ & OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_PAR);                        
    _perm(OSD_CREATE_COLLECTION, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_CREATE, OSD_SEC_OBJ_DESC_COL);		        
    _perm(OSD_CREATE_PARTITION, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_CREATE, OSD_SEC_OBJ_DESC_PAR);		        
    _perm(OSD_CREATE_SNAPSHOT, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_CREATE & OSD_SEC_CAP_READ & OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_PAR);                     
    _perm(OSD_CREATE_USER_TRACKING_COLLECTION, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_CREATE & OSD_SEC_CAP_READ & OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_COL);     
    _perm(OSD_DETACH_CLONE, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_PAR);                        
    _perm(OSD_FLUSH, OSD_SEC_OBJ_USER, OSD_SEC_CAP_OBJ_MGMT, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_FLUSH_COLLECTION, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_OBJ_MGMT,  OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_FLUSH_OSD, OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_OBJ_MGMT, OSD_SEC_OBJ_DESC_PAR);		        	
    _perm(OSD_FLUSH_PARTITION, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_OBJ_MGMT, OSD_SEC_OBJ_DESC_PAR);	        	
    _perm(OSD_FORMAT_OSD, OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_OBJ_MGMT | OSD_SEC_CAP_GLOBAL, OSD_SEC_OBJ_DESC_PAR);
    //FIXME: per table 26
    _perm(OSD_GET_ATTRIBUTES, OSD_SEC_OBJ_ROOT | OSD_SEC_OBJ_PARTITION | OSD_SEC_OBJ_COLLECTION|OSD_SEC_OBJ_USER, OSD_SEC_CAP_GET_ATTR, OSD_SEC_OBJ_DESC_OBJ|OSD_SEC_OBJ_DESC_PAR|OSD_SEC_OBJ_DESC_COL); 
    //FIXME: per table 26
    _perm(OSD_GET_MEMBER_ATTRIBUTES, OSD_SEC_OBJ_ROOT | OSD_SEC_OBJ_PARTITION | OSD_SEC_OBJ_COLLECTION|OSD_SEC_OBJ_USER, OSD_SEC_CAP_GET_ATTR, OSD_SEC_OBJ_DESC_OBJ|OSD_SEC_OBJ_DESC_PAR|OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_LIST, OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_READ|OSD_SEC_CAP_M_OBJECT, OSD_SEC_OBJ_DESC_PAR);
    //FIXME:
    _perm(OSD_LIST_COLLECTION, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_READ, OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_OBJECT_STRUCTURE_CHECK, OSD_SEC_OBJ_ROOT|OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_DEV_MGMT, OSD_SEC_OBJ_DESC_PAR);              
    _perm(OSD_PERFORM_SCSI_COMMAND, OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_DEV_MGMT|OSD_SEC_CAP_GLOBAL, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_PERFORM_TASK_MGMT_FUNC,  OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_DEV_MGMT|OSD_SEC_CAP_GLOBAL, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_PUNCH, OSD_SEC_OBJ_USER, OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_QUERY, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_WRITE|OSD_SEC_CAP_QUERY, OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_READ, OSD_SEC_OBJ_USER, OSD_SEC_CAP_READ, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_READ_MAP, OSD_SEC_OBJ_USER, OSD_SEC_CAP_DEV_MGMT, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_READ_MAPS_AND_COMPARE, OSD_SEC_OBJ_USER, OSD_SEC_CAP_DEV_MGMT, OSD_SEC_OBJ_DESC_OBJ);
    _perm(OSD_REFRESH_SNAPSHOT_OR_CLONE, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_READ|OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_REMOVE, OSD_SEC_OBJ_USER, OSD_SEC_CAP_REMOVE, OSD_SEC_OBJ_USER);
    _perm(OSD_REMOVE_COLLECTION, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_REMOVE, OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_REMOVE_MEMBER_OBJECTS, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_REMOVE|OSD_SEC_CAP_M_OBJECT, OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_REMOVE_PARTITION, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_REMOVE|OSD_SEC_GBL_REM, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_RESTORE_PARTITION_FROM_SNAPSHOT, OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_WRITE|OSD_SEC_CAP_READ, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_SET_ATTRIBUTES,  OSD_SEC_OBJ_ROOT | OSD_SEC_OBJ_PARTITION | OSD_SEC_OBJ_COLLECTION|OSD_SEC_OBJ_USER, OSD_SEC_CAP_SET_ATTR, OSD_SEC_OBJ_DESC_OBJ|OSD_SEC_OBJ_DESC_PAR|OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_SET_KEY, OSD_SEC_OBJ_ROOT|OSD_SEC_OBJ_PARTITION, OSD_SEC_CAP_DEV_MGMT|OSD_SEC_CAP_POL_SEC, OSD_SEC_OBJ_DESC_PAR);
    _perm(OSD_SET_MASTER_KEY, OSD_SEC_OBJ_ROOT, OSD_SEC_CAP_DEV_MGMT|OSD_SEC_CAP_POL_SEC|OSD_SEC_CAP_GLOBAL, OSD_SEC_OBJ_DESC_PAR);	        	
    _perm(OSD_SET_MEMBER_ATTRIBUTES, OSD_SEC_OBJ_COLLECTION, OSD_SEC_CAP_SET_ATTR, OSD_SEC_OBJ_DESC_COL);
    _perm(OSD_WRITE, OSD_SEC_OBJ_USER, OSD_SEC_CAP_WRITE, OSD_SEC_OBJ_DESC_OBJ);

}

/**
 * @doc
 *   Parse caps. see osdv2 or cap.h for cdb field order
 */
int 
cap_parse (uint8_t *caps_cdb, struct osd_v2_capability *caps) 
{
        caps->h.format                                  = caps_cdb[0]; 
        caps->h.integrity_algorithm__key_version        = caps_cdb[1];
        caps->h.security_method                         = caps_cdb[2];
        caps->h.reserved1                               = caps_cdb[3];
        memcpy(&caps->h.expiration_time.time, &caps_cdb[4], sizeof(uint8_t)*6);
        caps->h.audit[0]                                = caps_cdb[10];
        memcpy(&caps->h.discriminator, &caps_cdb[30], sizeof(uint8_t)*12);
        memcpy(&caps->h.object_created_time.time, &caps_cdb[42], sizeof(uint8_t)*6);
        caps->h.object_type                             = caps_cdb[48]; 
        memcpy(&caps->h.permissions_bit_mask, &caps_cdb[49], sizeof(uint8_t)*5);
        caps->h.reserved2                               = caps_cdb[54];
        caps->h.object_descriptor_type                  = caps_cdb[55]; /* high nibble */     

        //convert object_descriptor_type
        caps->od.obj_desc.allowed_attributes_access     = get_ntohl(&caps_cdb[56]);
        caps->od.obj_desc.policy_access_tag             = get_ntohl(&caps_cdb[60]);
        caps->od.obj_desc.boot_epoch                    = get_ntohs(&caps_cdb[64]);
        memcpy(&caps->od.obj_desc.reserved, &caps_cdb[66], sizeof(uint8_t)*6);
        caps->od.obj_desc.allowed_partition_id          = get_ntohll(&caps_cdb[72]);
        caps->od.obj_desc.allowed_object_id             = get_ntohll(&caps_cdb[80]);
        caps->od.obj_desc.allowed_range_length          = get_ntohll(&caps_cdb[88]);
        caps->od.obj_desc.allowed_range_start           = get_ntohll(&caps_cdb[96]);

        osd_debug("%s: done parsing caps!", __func__);

        return 1;
}

/**
 * @doc
 *   Parse caps, check validity of request, and permissions 
 */
int
cap_check(struct osd_device *osd, uint8_t *cdb, uint64_t pid, uint64_t oid, uint16_t action) 
{

        struct osd_v2_capability caps;
        uint8_t *caps_cdb = &cdb[80];
        int ret = 0;
        perm_table_t perms;
        void *handle = osd->handle;

        osd_debug("%s: checking caps....", __func__);
        ret = cap_parse(caps_cdb, &caps);
        if(!ret)
            return ret;

        ret = cap_passes_basic_tests(handle, &caps, pid, oid);
        if(!ret)
            return ret;

        osd_debug ("%s: action 0x%04x ", __func__, action); 
        if(action != OSD_CREATE) {
            // Can't check create time attributes as object won't exist
            ret = cap_check_allowed_range(osd, &caps, pid, oid);
            if(!ret)
                return ret;
        }

        //command, object type, caps, descriptor name
        perms = cap_perm_table[action];
        if ((perms.type & caps.h.object_type) == 0 ||
            (perms.perm & caps.h.permissions_bit_mask[0]) == 0 ||
            (perms.desc & caps.h.object_descriptor_type) == 0) {
            osd_error("%s: caps mismatch! capt 0x%llx type 0x%llx cap_p 0x%llx perm 0x%llx cap_d 0x%llx descr 0x%llx", __func__, 
                        llu(perms.type), llu(caps.h.object_type), 
                        llu(perms.perm), llu(caps.h.permissions_bit_mask[0]), 
                        llu(perms.desc), llu(caps.h.object_descriptor_type));
            return 0;
        }

        osd_debug("%s: done checking caps!", __func__);

        return 1;

}

/**
 * @doc
 *   Check timestamps on allowed range
 */
int
cap_check_allowed_range(struct osd_device *osd, struct osd_v2_capability *caps, uint64_t pid, uint64_t oid)
{
    osd_debug("%s: allowed_range_len (%llu) \n",
                    __func__, llu(caps->od.obj_desc.allowed_range_length));
    if (caps->od.obj_desc.allowed_range_length == 0) {
        if (!cap_check_time_version(osd, caps, pid, oid)) {
            osd_debug("%s: time_version check failed!", __func__);
            return 0;
        }
    } 

    return 1;
}

/**
 * @doc
 *   Check if the data range supplied in the CDB is accessible
 */
int    
cap_check_data_range(struct osd_v2_capability *caps, uint64_t start_addr,
        uint64_t data_length, uint64_t pid, uint64_t oid)
{
    // Check the data range in the command is allowed by the capability.
    // Data range is ignored for caps on the DCO or any GCO.
    //
    if (!oid && caps->od.obj_desc.allowed_range_length) {
        if (!cap_permits_data_range(caps->od.obj_desc.allowed_range_start,
                    caps->od.obj_desc.allowed_range_length,
                    start_addr,
                    data_length)) {
            osd_debug("%s: data range out of bounds in the capability\n", __func__);
            return 0;
        }
    }

    return 1;
}
