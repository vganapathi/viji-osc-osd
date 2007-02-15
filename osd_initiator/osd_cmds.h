#ifndef __OSD_CMDS_H
#define __OSD_CMDS_H

/*
 * Constants and structures for OSD commands.
 *
 */
/* varlen cdb service actions for OSD-2 (before SNIA changes 17 jan 07) */
#define INQUIRY				0x12
#define OSD_APPEND			0x8807
#define OSD_CREATE			0x8802
#define OSD_CREATE_AND_WRITE		0x8812
#define OSD_CREATE_COLLECTION		0x8815
#define OSD_CREATE_PARTITION		0x880b
#define OSD_FLUSH			0x8808
#define OSD_FLUSH_COLLECTION		0x881a
#define OSD_FLUSH_OSD			0x881c
#define OSD_FLUSH_PARTITION		0x881b
#define OSD_FORMAT_OSD			0x8801
#define OSD_GET_ATTRIBUTES		0x880e
#define OSD_GET_MEMBER_ATTRIBUTES	0x8822
#define OSD_LIST			0x8803
#define OSD_LIST_COLLECTION		0x8817
#define OSD_PERFORM_SCSI_COMMAND	0x8f7e
#define OSD_PERFORM_TASK_MGMT_FUNC	0x8f7f
#define OSD_QUERY			0x8820
#define OSD_READ			0x8805
#define OSD_REMOVE			0x880a
#define OSD_REMOVE_COLLECTION		0x8816
#define OSD_REMOVE_MEMBER_OBJECTS	0x8821
#define OSD_REMOVE_PARTITION		0x880c
#define OSD_SET_ATTRIBUTES		0x880f
#define OSD_SET_KEY			0x8818
#define OSD_SET_MASTER_KEY		0x8819
#define OSD_SET_MEMBER_ATTRIBUTES	0x8823
#define OSD_WRITE			0x8806

#endif /* __OSD_CMDS_H */
