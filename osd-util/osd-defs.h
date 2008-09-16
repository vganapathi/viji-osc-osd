/*
 * Symbols from the OSD and other specifications.
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
#ifndef __OSD_DEFS_H
#define __OSD_DEFS_H
/*
 * Shared defitions for OSD target and initiator.
 */
#define VARLEN_CDB 0x7f
#define OSD_CDB_SIZE 200
#define OSD_MAX_SENSE 252

/* varlen cdb service actions for osd2r01 */
#define OSD_APPEND			0x8887
#define OSD_CREATE			0x8882
#define OSD_CREATE_AND_WRITE		0x8892
#define OSD_CREATE_COLLECTION		0x8895
#define OSD_CREATE_PARTITION		0x888b
#define OSD_FLUSH			0x8888
#define OSD_FLUSH_COLLECTION		0x889a
#define OSD_FLUSH_OSD			0x889c
#define OSD_FLUSH_PARTITION		0x889b
#define OSD_FORMAT_OSD			0x8881
#define OSD_GET_ATTRIBUTES		0x888e
#define OSD_GET_MEMBER_ATTRIBUTES	0x88a2
#define OSD_LIST			0x8883
#define OSD_LIST_COLLECTION		0x8897
#define OSD_PERFORM_SCSI_COMMAND	0x8f7c
#define OSD_PERFORM_TASK_MGMT_FUNC	0x8f7d
#define OSD_QUERY			0x88a0
#define OSD_READ			0x8885
#define OSD_REMOVE			0x888a
#define OSD_REMOVE_COLLECTION		0x8896
#define OSD_REMOVE_MEMBER_OBJECTS	0x88a1
#define OSD_REMOVE_PARTITION		0x888c
#define OSD_SET_ATTRIBUTES		0x888f
#define OSD_SET_KEY			0x8898
#define OSD_SET_MASTER_KEY		0x8899
#define OSD_SET_MEMBER_ATTRIBUTES	0x88a3
#define OSD_WRITE			0x8886

/* custom definitions */
#define OSD_CAS				0x8889
#define OSD_FA				0x8890
#define OSD_COND_SETATTR		0x8891
#define OSD_GEN_CAS			0x88a5

/*Data Distribution Types*/
#define DDT_CONTIG	0x0
#define DDT_SGL		0x1
#define DDT_IOV		0x2
#define DDT_RES		0x3


#define SAM_STAT_GOOD            0x00
#define SAM_STAT_CHECK_CONDITION 0x02
#define SAM_STAT_CONDITION_MET   0x04
#define SAM_STAT_BUSY            0x08
#define SAM_STAT_INTERMEDIATE    0x10
#define SAM_STAT_INTERMEDIATE_CONDITION_MET 0x14
#define SAM_STAT_RESERVATION_CONFLICT 0x18
#define SAM_STAT_COMMAND_TERMINATED 0x22
#define SAM_STAT_TASK_SET_FULL   0x28
#define SAM_STAT_ACA_ACTIVE      0x30
#define SAM_STAT_TASK_ABORTED    0x40

/* OSD object constants, osd2r01 sec 4.6.2 */
#define ROOT_PID 0LLU
#define ROOT_OID 0LLU
#define PARTITION_PID_LB 0x10000LLU
#define PARTITION_OID 0x0LLU
#define OBJECT_PID_LB 0x10000LLU
#define COLLECTION_PID_LB OBJECT_PID_LB
#define USEROBJECT_PID_LB OBJECT_PID_LB
#define OBJECT_OID_LB OBJECT_PID_LB
#define COLLECTION_OID_LB COLLECTION_PID_LB
#define USEROBJECT_OID_LB USEROBJECT_PID_LB

/* object types, osd2r00 section 4.9.2.2.1 table 9 */
enum {
	ROOT = 0x01,
	PARTITION = 0x02,
	COLLECTION = 0x40,
	USEROBJECT = 0x80,
	ILLEGAL_OBJ = 0x00 /* XXX: this is not in standard */
};

/* attribute page ranges, osd2r01 sec 4.7.3 */
enum {
	USEROBJECT_PG = 0x0U,
	PARTITION_PG  = 0x30000000U,
	COLLECTION_PG = 0x60000000U,
	ROOT_PG       = 0x90000000U,
	RESERVED_PG   = 0xC0000000U,
	ANY_PG        = 0xF0000000U,
	CUR_CMD_ATTR_PG = 0xFFFFFFFEU,
	GETALLATTR_PG = 0xFFFFFFFFU,
};

/* attribute page sets, further subdivide the above, osd2r01 sec 4.7.3 */
enum {
	STD_PG_LB = 0x0,
	STD_PG_UB = 0x7F,
	RSRV_PG_LB = 0x80,
	RSRV_PG_UB = 0x7FFF,
	OSTD_PG_LB = 0x8000,
	OSTD_PG_UB = 0xEFFF,
	MSPC_PG_LB = 0xF000,
	MSPC_PG_UB = 0xFFFF,
	LUN_PG_LB = 0x10000,
	LUN_PG_UB = 0x1FFFFFFF,
	VEND_PG_LB = 0x20000000,
	VEND_PG_UB = 0x2FFFFFFF
};

/* osd2r01, Section 4.7.4 */
enum {
	ATTRNUM_LB = 0x0,
	ATTRNUM_UB = 0xFFFFFFFE,
	ATTRNUM_INFO = ATTRNUM_LB,
	ATTRNUM_UNMODIFIABLE = 0xFFFFFFFF,
	ATTRNUM_GETALL = ATTRNUM_UNMODIFIABLE
};

/* osd2r00, Table 5 Section 4.7.5 */
enum {
	USEROBJECT_DIR_PG = (USEROBJECT_PG + 0x0),
	PARTITION_DIR_PG = (PARTITION_PG + 0x0),
	COLLECTION_DIR_PG = (COLLECTION_PG + 0x0),
	ROOT_DIR_PG = (ROOT_PG + 0x0)
};

/* (selected) attribute pages defined by osd spec, osd2r01 sec 7.1.2.1 */
enum {
	USER_DIR_PG = 0x0,
	USER_INFO_PG = 0x1,
	USER_QUOTA_PG = 0x2,
	USER_TMSTMP_PG = 0x3,
	USER_COLL_PG = 0x4,
	USER_POLICY_PG = 0x5,
	USER_ATOMICS_PG = 0x6,
};

/* in all attribute pages, attribute number 0 is a 40-byte identification */
#define PAGE_ID (0x0)
#define ATTR_PAGE_ID_LEN (40)

/* current command attributes page constants, osd2r01 sec 7.1.2.24 */
enum {
	/* individiual attribute items (pageid always at 0) */
	CCAP_RICV = 0x1,
	CCAP_OBJT = 0x2,
	CCAP_PID = 0x3,
	CCAP_OID = 0x4,
	CCAP_APPADDR = 0x5,

	/* lengths of the items */
	CCAP_RICV_LEN = 20,
	CCAP_OBJT_LEN = 1,
	CCAP_PID_LEN = 8,
	CCAP_OID_LEN = 8,
	CCAP_APPADDR_LEN = 8,

	/* offsets when retrieved in page format (page num and len at 0) */
	CCAP_RICV_OFF = 8,
	CCAP_OBJT_OFF = 28,
	CCAP_PID_OFF = 32,
	CCAP_OID_OFF = 40,
	CCAP_APPADDR_OFF = 48,
	CCAP_TOTAL_LEN = 56,
};

/* userobject timestamp attribute page osd2r01 sec 7.1.2.18 */
enum {
	/* attributes */
	UTSAP_CTIME = 0x1,
	UTSAP_ATTR_ATIME = 0x2,
	UTSAP_ATTR_MTIME = 0x3,
	UTSAP_DATA_ATIME = 0x4,
	UTSAP_DATA_MTIME = 0x5,

	/* length of attributes */
	UTSAP_CTIME_LEN = 6,
	UTSAP_ATTR_ATIME_LEN = 6,
	UTSAP_ATTR_MTIME_LEN = 6,
	UTSAP_DATA_ATIME_LEN = 6,
	UTSAP_DATA_MTIME_LEN = 6,

	/* offsets */
	UTSAP_CTIME_OFF = 8,
	UTSAP_ATTR_ATIME_OFF = 14,
	UTSAP_ATTR_MTIME_OFF = 20,
	UTSAP_DATA_ATIME_OFF = 26,
	UTSAP_DATA_MTIME_OFF = 32,
	UTSAP_TOTAL_LEN = 38,
};

/* userobject information attribute page osd2r01 sec 7.1.2.11 */
enum {
	/* attributes */
	UIAP_PID = 0x1,
	UIAP_OID = 0x2,
	UIAP_USERNAME = 0x9,
	UIAP_USED_CAPACITY = 0x81,
	UIAP_LOGICAL_LEN = 0x82,

	/* lengths */
	UIAP_PID_LEN = 8,
	UIAP_OID_LEN = 8,
	UIAP_USED_CAPACITY_LEN = 8,
	UIAP_LOGICAL_LEN_LEN = 8,
};

/* userobject collections attribute page osd2r01 Sec 7.1.2.19 */
enum {
	UCAP_COLL_PTR_LB = 0x1,
	UCAP_COLL_PTR_UB = 0xFFFFFF00,
};

/* userobject atomics page. Only userobjects have atomics attribute page. */
enum {
	UAP_CAS = 0x1,
	UAP_FA = 0x2,
};

enum {
	GETPAGE_SETVALUE = 0x2,
	GETLIST_SETLIST = 0x3
};

#endif /* __OSD_DEFS_H */