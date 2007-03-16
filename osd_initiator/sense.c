/* 
 * ASCII values for a number of symbolic constants, printing functions,
 * etc.
 * Additions for SCSI 2 and Linux 2.2.x by D. Gilbert (990422)
 * Additions for SCSI 3+ (SPC-3 T10/1416-D Rev 07 3 May 2002)
 *   by D. Gilbert and aeb (20020609)
 * Additions for SPC-3 T10/1416-D Rev 21 22 Sept 2004, D. Gilbert 20041025
 * Update to SPC-4 T10/1713-D Rev 5a, 14 June 2006, D. Gilbert 20060702
 *
 * Copied from linux-2.6.20 and edited for userspace use.  Printing is
 * done into a string rather than with printk.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "util/util.h"
#include "sense.h"

#if 0
static const char *scsi_status_string(unsigned char scsi_status)
{
	const char *ccp;

	switch (scsi_status) {
	case 0:    ccp = "Good"; break;
	case 0x2:  ccp = "Check Condition"; break;
	case 0x4:  ccp = "Condition Met"; break;
	case 0x8:  ccp = "Busy"; break;
	case 0x10: ccp = "Intermediate"; break;
	case 0x14: ccp = "Intermediate-Condition Met"; break;
	case 0x18: ccp = "Reservation Conflict"; break;
	case 0x22: ccp = "Command Terminated"; break;	/* obsolete */
	case 0x28: ccp = "Task set Full"; break;	/* was: Queue Full */
	case 0x30: ccp = "ACA Active"; break;
	case 0x40: ccp = "Task Aborted"; break;
	default:   ccp = "Unknown status";
	}
	return ccp;
}
#endif

/* description of the sense key values */
static const char *const snstext[] = {
	"No Sense",	    /* 0: There is no sense information */
	"Recovered Error",  /* 1: The last command completed successfully
				  but used error correction */
	"Not Ready",	    /* 2: The addressed target is not ready */
	"Medium Error",	    /* 3: Data error detected on the medium */
	"Hardware Error",   /* 4: Controller or device failure */
	"Illegal Request",  /* 5: Error in request */
	"Unit Attention",   /* 6: Removable medium was changed, or
				  the target has been reset, or ... */
	"Data Protect",	    /* 7: Access to the data is blocked */
	"Blank Check",	    /* 8: Reached unexpected written or unwritten
				  region of the medium */
	"Vendor Specific(9)",
	"Copy Aborted",	    /* A: COPY or COMPARE was aborted */
	"Aborted Command",  /* B: The target aborted the command */
	"Equal",	    /* C: A SEARCH DATA command found data equal */
	"Volume Overflow",  /* D: Medium full with still data to be written */
	"Miscompare",	    /* E: Source data and data on the medium
				  do not agree */
};

static const char *sense_key_string(unsigned char key)
{
	if (key <= 0xE)
		return snstext[key];
	return NULL;
}

struct error_info {
	unsigned short code12;	/* 0x0302 looks better than 0x03,0x02 */
	const char *text;
};

static struct error_info additional[] =
{
	{0x0000, "No additional sense information"},
	{0x0001, "Filemark detected"},
	{0x0002, "End-of-partition/medium detected"},
	{0x0003, "Setmark detected"},
	{0x0004, "Beginning-of-partition/medium detected"},
	{0x0005, "End-of-data detected"},
	{0x0006, "I/O process terminated"},
	{0x0011, "Audio play operation in progress"},
	{0x0012, "Audio play operation paused"},
	{0x0013, "Audio play operation successfully completed"},
	{0x0014, "Audio play operation stopped due to error"},
	{0x0015, "No current audio status to return"},
	{0x0016, "Operation in progress"},
	{0x0017, "Cleaning requested"},
	{0x0018, "Erase operation in progress"},
	{0x0019, "Locate operation in progress"},
	{0x001A, "Rewind operation in progress"},
	{0x001B, "Set capacity operation in progress"},
	{0x001C, "Verify operation in progress"},
	{0x001D, "ATA pass through information available"},

	{0x0100, "No index/sector signal"},

	{0x0200, "No seek complete"},

	{0x0300, "Peripheral device write fault"},
	{0x0301, "No write current"},
	{0x0302, "Excessive write errors"},

	{0x0400, "Logical unit not ready, cause not reportable"},
	{0x0401, "Logical unit is in process of becoming ready"},
	{0x0402, "Logical unit not ready, initializing command required"},
	{0x0403, "Logical unit not ready, manual intervention required"},
	{0x0404, "Logical unit not ready, format in progress"},
	{0x0405, "Logical unit not ready, rebuild in progress"},
	{0x0406, "Logical unit not ready, recalculation in progress"},
	{0x0407, "Logical unit not ready, operation in progress"},
	{0x0408, "Logical unit not ready, long write in progress"},
	{0x0409, "Logical unit not ready, self-test in progress"},
	{0x040A, "Logical unit not accessible, asymmetric access state "
	 "transition"},
	{0x040B, "Logical unit not accessible, target port in standby state"},
	{0x040C, "Logical unit not accessible, target port in unavailable "
	 "state"},
	{0x0410, "Logical unit not ready, auxiliary memory not accessible"},
	{0x0411, "Logical unit not ready, notify (enable spinup) required"},
	{0x0412, "Logical unit not ready, offline"},

	{0x0500, "Logical unit does not respond to selection"},

	{0x0600, "No reference position found"},

	{0x0700, "Multiple peripheral devices selected"},

	{0x0800, "Logical unit communication failure"},
	{0x0801, "Logical unit communication time-out"},
	{0x0802, "Logical unit communication parity error"},
	{0x0803, "Logical unit communication CRC error (Ultra-DMA/32)"},
	{0x0804, "Unreachable copy target"},

	{0x0900, "Track following error"},
	{0x0901, "Tracking servo failure"},
	{0x0902, "Focus servo failure"},
	{0x0903, "Spindle servo failure"},
	{0x0904, "Head select fault"},

	{0x0A00, "Error log overflow"},

	{0x0B00, "Warning"},
	{0x0B01, "Warning - specified temperature exceeded"},
	{0x0B02, "Warning - enclosure degraded"},
	{0x0B03, "Warning - background self-test failed"},
	{0x0B04, "Warning - background pre-scan detected medium error"},
	{0x0B05, "Warning - background medium scan detected medium error"},

	{0x0C00, "Write error"},
	{0x0C01, "Write error - recovered with auto reallocation"},
	{0x0C02, "Write error - auto reallocation failed"},
	{0x0C03, "Write error - recommend reassignment"},
	{0x0C04, "Compression check miscompare error"},
	{0x0C05, "Data expansion occurred during compression"},
	{0x0C06, "Block not compressible"},
	{0x0C07, "Write error - recovery needed"},
	{0x0C08, "Write error - recovery failed"},
	{0x0C09, "Write error - loss of streaming"},
	{0x0C0A, "Write error - padding blocks added"},
	{0x0C0B, "Auxiliary memory write error"},
	{0x0C0C, "Write error - unexpected unsolicited data"},
	{0x0C0D, "Write error - not enough unsolicited data"},
	{0x0C0F, "Defects in error window"},

	{0x0D00, "Error detected by third party temporary initiator"},
	{0x0D01, "Third party device failure"},
	{0x0D02, "Copy target device not reachable"},
	{0x0D03, "Incorrect copy target device type"},
	{0x0D04, "Copy target device data underrun"},
	{0x0D05, "Copy target device data overrun"},

	{0x0E00, "Invalid information unit"},
	{0x0E01, "Information unit too short"},
	{0x0E02, "Information unit too long"},
	{0x0E03, "Invalid field in command information unit"},

	{0x1000, "Id CRC or ECC error"},
	{0x1001, "Logical block guard check failed"},
	{0x1002, "Logical block application tag check failed"},
	{0x1003, "Logical block reference tag check failed"},

	{0x1100, "Unrecovered read error"},
	{0x1101, "Read retries exhausted"},
	{0x1102, "Error too long to correct"},
	{0x1103, "Multiple read errors"},
	{0x1104, "Unrecovered read error - auto reallocate failed"},
	{0x1105, "L-EC uncorrectable error"},
	{0x1106, "CIRC unrecovered error"},
	{0x1107, "Data re-synchronization error"},
	{0x1108, "Incomplete block read"},
	{0x1109, "No gap found"},
	{0x110A, "Miscorrected error"},
	{0x110B, "Unrecovered read error - recommend reassignment"},
	{0x110C, "Unrecovered read error - recommend rewrite the data"},
	{0x110D, "De-compression CRC error"},
	{0x110E, "Cannot decompress using declared algorithm"},
	{0x110F, "Error reading UPC/EAN number"},
	{0x1110, "Error reading ISRC number"},
	{0x1111, "Read error - loss of streaming"},
	{0x1112, "Auxiliary memory read error"},
	{0x1113, "Read error - failed retransmission request"},
	{0x1114, "Read error - lba marked bad by application client"},

	{0x1200, "Address mark not found for id field"},

	{0x1300, "Address mark not found for data field"},

	{0x1400, "Recorded entity not found"},
	{0x1401, "Record not found"},
	{0x1402, "Filemark or setmark not found"},
	{0x1403, "End-of-data not found"},
	{0x1404, "Block sequence error"},
	{0x1405, "Record not found - recommend reassignment"},
	{0x1406, "Record not found - data auto-reallocated"},
	{0x1407, "Locate operation failure"},

	{0x1500, "Random positioning error"},
	{0x1501, "Mechanical positioning error"},
	{0x1502, "Positioning error detected by read of medium"},

	{0x1600, "Data synchronization mark error"},
	{0x1601, "Data sync error - data rewritten"},
	{0x1602, "Data sync error - recommend rewrite"},
	{0x1603, "Data sync error - data auto-reallocated"},
	{0x1604, "Data sync error - recommend reassignment"},

	{0x1700, "Recovered data with no error correction applied"},
	{0x1701, "Recovered data with retries"},
	{0x1702, "Recovered data with positive head offset"},
	{0x1703, "Recovered data with negative head offset"},
	{0x1704, "Recovered data with retries and/or circ applied"},
	{0x1705, "Recovered data using previous sector id"},
	{0x1706, "Recovered data without ECC - data auto-reallocated"},
	{0x1707, "Recovered data without ECC - recommend reassignment"},
	{0x1708, "Recovered data without ECC - recommend rewrite"},
	{0x1709, "Recovered data without ECC - data rewritten"},

	{0x1800, "Recovered data with error correction applied"},
	{0x1801, "Recovered data with error corr. & retries applied"},
	{0x1802, "Recovered data - data auto-reallocated"},
	{0x1803, "Recovered data with CIRC"},
	{0x1804, "Recovered data with L-EC"},
	{0x1805, "Recovered data - recommend reassignment"},
	{0x1806, "Recovered data - recommend rewrite"},
	{0x1807, "Recovered data with ECC - data rewritten"},
	{0x1808, "Recovered data with linking"},

	{0x1900, "Defect list error"},
	{0x1901, "Defect list not available"},
	{0x1902, "Defect list error in primary list"},
	{0x1903, "Defect list error in grown list"},

	{0x1A00, "Parameter list length error"},

	{0x1B00, "Synchronous data transfer error"},

	{0x1C00, "Defect list not found"},
	{0x1C01, "Primary defect list not found"},
	{0x1C02, "Grown defect list not found"},

	{0x1D00, "Miscompare during verify operation"},

	{0x1E00, "Recovered id with ECC correction"},

	{0x1F00, "Partial defect list transfer"},

	{0x2000, "Invalid command operation code"},
	{0x2001, "Access denied - initiator pending-enrolled"},
	{0x2002, "Access denied - no access rights"},
	{0x2003, "Access denied - invalid mgmt id key"},
	{0x2004, "Illegal command while in write capable state"},
	{0x2005, "Obsolete"},
	{0x2006, "Illegal command while in explicit address mode"},
	{0x2007, "Illegal command while in implicit address mode"},
	{0x2008, "Access denied - enrollment conflict"},
	{0x2009, "Access denied - invalid LU identifier"},
	{0x200A, "Access denied - invalid proxy token"},
	{0x200B, "Access denied - ACL LUN conflict"},

	{0x2100, "Logical block address out of range"},
	{0x2101, "Invalid element address"},
	{0x2102, "Invalid address for write"},
	{0x2103, "Invalid write crossing layer jump"},

	{0x2200, "Illegal function (use 20 00, 24 00, or 26 00)"},

	{0x2400, "Invalid field in cdb"},
	{0x2401, "CDB decryption error"},
	{0x2402, "Obsolete"},
	{0x2403, "Obsolete"},
	{0x2404, "Security audit value frozen"},
	{0x2405, "Security working key frozen"},
	{0x2406, "Nonce not unique"},
	{0x2407, "Nonce timestamp out of range"},

	{0x2500, "Logical unit not supported"},

	{0x2600, "Invalid field in parameter list"},
	{0x2601, "Parameter not supported"},
	{0x2602, "Parameter value invalid"},
	{0x2603, "Threshold parameters not supported"},
	{0x2604, "Invalid release of persistent reservation"},
	{0x2605, "Data decryption error"},
	{0x2606, "Too many target descriptors"},
	{0x2607, "Unsupported target descriptor type code"},
	{0x2608, "Too many segment descriptors"},
	{0x2609, "Unsupported segment descriptor type code"},
	{0x260A, "Unexpected inexact segment"},
	{0x260B, "Inline data length exceeded"},
	{0x260C, "Invalid operation for copy source or destination"},
	{0x260D, "Copy segment granularity violation"},
	{0x260E, "Invalid parameter while port is enabled"},
	{0x260F, "Invalid data-out buffer integrity check value"},
	{0x2610, "Data decryption key fail limit reached"},
	{0x2611, "Incomplete key-associated data set"},
	{0x2612, "Vendor specific key reference not found"},

	{0x2700, "Write protected"},
	{0x2701, "Hardware write protected"},
	{0x2702, "Logical unit software write protected"},
	{0x2703, "Associated write protect"},
	{0x2704, "Persistent write protect"},
	{0x2705, "Permanent write protect"},
	{0x2706, "Conditional write protect"},

	{0x2800, "Not ready to ready change, medium may have changed"},
	{0x2801, "Import or export element accessed"},
	{0x2802, "Format-layer may have changed"},

	{0x2900, "Power on, reset, or bus device reset occurred"},
	{0x2901, "Power on occurred"},
	{0x2902, "Scsi bus reset occurred"},
	{0x2903, "Bus device reset function occurred"},
	{0x2904, "Device internal reset"},
	{0x2905, "Transceiver mode changed to single-ended"},
	{0x2906, "Transceiver mode changed to lvd"},
	{0x2907, "I_T nexus loss occurred"},

	{0x2A00, "Parameters changed"},
	{0x2A01, "Mode parameters changed"},
	{0x2A02, "Log parameters changed"},
	{0x2A03, "Reservations preempted"},
	{0x2A04, "Reservations released"},
	{0x2A05, "Registrations preempted"},
	{0x2A06, "Asymmetric access state changed"},
	{0x2A07, "Implicit asymmetric access state transition failed"},
	{0x2A08, "Priority changed"},
	{0x2A09, "Capacity data has changed"},
	{0x2A10, "Timestamp changed"},
	{0x2A11, "Data encryption parameters changed by another i_t nexus"},
	{0x2A12, "Data encryption parameters changed by vendor specific "
		 "event"},
	{0x2A13, "Data encryption key instance counter has changed"},

	{0x2B00, "Copy cannot execute since host cannot disconnect"},

	{0x2C00, "Command sequence error"},
	{0x2C01, "Too many windows specified"},
	{0x2C02, "Invalid combination of windows specified"},
	{0x2C03, "Current program area is not empty"},
	{0x2C04, "Current program area is empty"},
	{0x2C05, "Illegal power condition request"},
	{0x2C06, "Persistent prevent conflict"},
	{0x2C07, "Previous busy status"},
	{0x2C08, "Previous task set full status"},
	{0x2C09, "Previous reservation conflict status"},
	{0x2C0A, "Partition or collection contains user objects"},
	{0x2C0B, "Not reserved"},

	{0x2D00, "Overwrite error on update in place"},

	{0x2E00, "Insufficient time for operation"},

	{0x2F00, "Commands cleared by another initiator"},
	{0x2F01, "Commands cleared by power loss notification"},

	{0x3000, "Incompatible medium installed"},
	{0x3001, "Cannot read medium - unknown format"},
	{0x3002, "Cannot read medium - incompatible format"},
	{0x3003, "Cleaning cartridge installed"},
	{0x3004, "Cannot write medium - unknown format"},
	{0x3005, "Cannot write medium - incompatible format"},
	{0x3006, "Cannot format medium - incompatible medium"},
	{0x3007, "Cleaning failure"},
	{0x3008, "Cannot write - application code mismatch"},
	{0x3009, "Current session not fixated for append"},
	{0x300A, "Cleaning request rejected"},
	{0x300C, "WORM medium - overwrite attempted"},
	{0x300D, "WORM medium - integrity check"},
	{0x3010, "Medium not formatted"},

	{0x3100, "Medium format corrupted"},
	{0x3101, "Format command failed"},
	{0x3102, "Zoned formatting failed due to spare linking"},

	{0x3200, "No defect spare location available"},
	{0x3201, "Defect list update failure"},

	{0x3300, "Tape length error"},

	{0x3400, "Enclosure failure"},

	{0x3500, "Enclosure services failure"},
	{0x3501, "Unsupported enclosure function"},
	{0x3502, "Enclosure services unavailable"},
	{0x3503, "Enclosure services transfer failure"},
	{0x3504, "Enclosure services transfer refused"},
	{0x3505, "Enclosure services checksum error"},

	{0x3600, "Ribbon, ink, or toner failure"},

	{0x3700, "Rounded parameter"},

	{0x3800, "Event status notification"},
	{0x3802, "Esn - power management class event"},
	{0x3804, "Esn - media class event"},
	{0x3806, "Esn - device busy class event"},

	{0x3900, "Saving parameters not supported"},

	{0x3A00, "Medium not present"},
	{0x3A01, "Medium not present - tray closed"},
	{0x3A02, "Medium not present - tray open"},
	{0x3A03, "Medium not present - loadable"},
	{0x3A04, "Medium not present - medium auxiliary memory accessible"},

	{0x3B00, "Sequential positioning error"},
	{0x3B01, "Tape position error at beginning-of-medium"},
	{0x3B02, "Tape position error at end-of-medium"},
	{0x3B03, "Tape or electronic vertical forms unit not ready"},
	{0x3B04, "Slew failure"},
	{0x3B05, "Paper jam"},
	{0x3B06, "Failed to sense top-of-form"},
	{0x3B07, "Failed to sense bottom-of-form"},
	{0x3B08, "Reposition error"},
	{0x3B09, "Read past end of medium"},
	{0x3B0A, "Read past beginning of medium"},
	{0x3B0B, "Position past end of medium"},
	{0x3B0C, "Position past beginning of medium"},
	{0x3B0D, "Medium destination element full"},
	{0x3B0E, "Medium source element empty"},
	{0x3B0F, "End of medium reached"},
	{0x3B11, "Medium magazine not accessible"},
	{0x3B12, "Medium magazine removed"},
	{0x3B13, "Medium magazine inserted"},
	{0x3B14, "Medium magazine locked"},
	{0x3B15, "Medium magazine unlocked"},
	{0x3B16, "Mechanical positioning or changer error"},
	{0x3B17, "Read past end of user object"},

	{0x3D00, "Invalid bits in identify message"},

	{0x3E00, "Logical unit has not self-configured yet"},
	{0x3E01, "Logical unit failure"},
	{0x3E02, "Timeout on logical unit"},
	{0x3E03, "Logical unit failed self-test"},
	{0x3E04, "Logical unit unable to update self-test log"},

	{0x3F00, "Target operating conditions have changed"},
	{0x3F01, "Microcode has been changed"},
	{0x3F02, "Changed operating definition"},
	{0x3F03, "Inquiry data has changed"},
	{0x3F04, "Component device attached"},
	{0x3F05, "Device identifier changed"},
	{0x3F06, "Redundancy group created or modified"},
	{0x3F07, "Redundancy group deleted"},
	{0x3F08, "Spare created or modified"},
	{0x3F09, "Spare deleted"},
	{0x3F0A, "Volume set created or modified"},
	{0x3F0B, "Volume set deleted"},
	{0x3F0C, "Volume set deassigned"},
	{0x3F0D, "Volume set reassigned"},
	{0x3F0E, "Reported luns data has changed"},
	{0x3F0F, "Echo buffer overwritten"},
	{0x3F10, "Medium loadable"},
	{0x3F11, "Medium auxiliary memory accessible"},
	{0x3F12, "iSCSI IP address added"},
	{0x3F13, "iSCSI IP address removed"},
	{0x3F14, "iSCSI IP address changed"},
/*
 *	{0x40NN, "Ram failure"},
 *	{0x40NN, "Diagnostic failure on component nn"},
 *	{0x41NN, "Data path failure"},
 *	{0x42NN, "Power-on or self-test failure"},
 */
	{0x4300, "Message error"},

	{0x4400, "Internal target failure"},
	{0x4471, "ATA device failed set features"},

	{0x4500, "Select or reselect failure"},

	{0x4600, "Unsuccessful soft reset"},

	{0x4700, "Scsi parity error"},
	{0x4701, "Data phase CRC error detected"},
	{0x4702, "Scsi parity error detected during st data phase"},
	{0x4703, "Information unit iuCRC error detected"},
	{0x4704, "Asynchronous information protection error detected"},
	{0x4705, "Protocol service CRC error"},
	{0x4706, "Phy test function in progress"},
	{0x477f, "Some commands cleared by iSCSI Protocol event"},

	{0x4800, "Initiator detected error message received"},

	{0x4900, "Invalid message error"},

	{0x4A00, "Command phase error"},

	{0x4B00, "Data phase error"},
	{0x4B01, "Invalid target port transfer tag received"},
	{0x4B02, "Too much write data"},
	{0x4B03, "Ack/nak timeout"},
	{0x4B04, "Nak received"},
	{0x4B05, "Data offset error"},
	{0x4B06, "Initiator response timeout"},

	{0x4C00, "Logical unit failed self-configuration"},
/*
 *	{0x4DNN, "Tagged overlapped commands (nn = queue tag)"},
 */
	{0x4E00, "Overlapped commands attempted"},

	{0x5000, "Write append error"},
	{0x5001, "Write append position error"},
	{0x5002, "Position error related to timing"},

	{0x5100, "Erase failure"},
	{0x5101, "Erase failure - incomplete erase operation detected"},

	{0x5200, "Cartridge fault"},

	{0x5300, "Media load or eject failed"},
	{0x5301, "Unload tape failure"},
	{0x5302, "Medium removal prevented"},
	{0x5303, "Medium removal prevented by data transfer element"},
	{0x5304, "Medium thread or unthread failure"},

	{0x5400, "Scsi to host system interface failure"},

	{0x5500, "System resource failure"},
	{0x5501, "System buffer full"},
	{0x5502, "Insufficient reservation resources"},
	{0x5503, "Insufficient resources"},
	{0x5504, "Insufficient registration resources"},
	{0x5505, "Insufficient access control resources"},
	{0x5506, "Auxiliary memory out of space"},
	{0x5507, "Quota error"},
	{0x5508, "Maximum number of supplemental decryption keys exceeded"},

	{0x5700, "Unable to recover table-of-contents"},

	{0x5800, "Generation does not exist"},

	{0x5900, "Updated block read"},

	{0x5A00, "Operator request or state change input"},
	{0x5A01, "Operator medium removal request"},
	{0x5A02, "Operator selected write protect"},
	{0x5A03, "Operator selected write permit"},

	{0x5B00, "Log exception"},
	{0x5B01, "Threshold condition met"},
	{0x5B02, "Log counter at maximum"},
	{0x5B03, "Log list codes exhausted"},

	{0x5C00, "Rpl status change"},
	{0x5C01, "Spindles synchronized"},
	{0x5C02, "Spindles not synchronized"},

	{0x5D00, "Failure prediction threshold exceeded"},
	{0x5D01, "Media failure prediction threshold exceeded"},
	{0x5D02, "Logical unit failure prediction threshold exceeded"},
	{0x5D03, "Spare area exhaustion prediction threshold exceeded"},
	{0x5D10, "Hardware impending failure general hard drive failure"},
	{0x5D11, "Hardware impending failure drive error rate too high"},
	{0x5D12, "Hardware impending failure data error rate too high"},
	{0x5D13, "Hardware impending failure seek error rate too high"},
	{0x5D14, "Hardware impending failure too many block reassigns"},
	{0x5D15, "Hardware impending failure access times too high"},
	{0x5D16, "Hardware impending failure start unit times too high"},
	{0x5D17, "Hardware impending failure channel parametrics"},
	{0x5D18, "Hardware impending failure controller detected"},
	{0x5D19, "Hardware impending failure throughput performance"},
	{0x5D1A, "Hardware impending failure seek time performance"},
	{0x5D1B, "Hardware impending failure spin-up retry count"},
	{0x5D1C, "Hardware impending failure drive calibration retry count"},
	{0x5D20, "Controller impending failure general hard drive failure"},
	{0x5D21, "Controller impending failure drive error rate too high"},
	{0x5D22, "Controller impending failure data error rate too high"},
	{0x5D23, "Controller impending failure seek error rate too high"},
	{0x5D24, "Controller impending failure too many block reassigns"},
	{0x5D25, "Controller impending failure access times too high"},
	{0x5D26, "Controller impending failure start unit times too high"},
	{0x5D27, "Controller impending failure channel parametrics"},
	{0x5D28, "Controller impending failure controller detected"},
	{0x5D29, "Controller impending failure throughput performance"},
	{0x5D2A, "Controller impending failure seek time performance"},
	{0x5D2B, "Controller impending failure spin-up retry count"},
	{0x5D2C, "Controller impending failure drive calibration retry count"},
	{0x5D30, "Data channel impending failure general hard drive failure"},
	{0x5D31, "Data channel impending failure drive error rate too high"},
	{0x5D32, "Data channel impending failure data error rate too high"},
	{0x5D33, "Data channel impending failure seek error rate too high"},
	{0x5D34, "Data channel impending failure too many block reassigns"},
	{0x5D35, "Data channel impending failure access times too high"},
	{0x5D36, "Data channel impending failure start unit times too high"},
	{0x5D37, "Data channel impending failure channel parametrics"},
	{0x5D38, "Data channel impending failure controller detected"},
	{0x5D39, "Data channel impending failure throughput performance"},
	{0x5D3A, "Data channel impending failure seek time performance"},
	{0x5D3B, "Data channel impending failure spin-up retry count"},
	{0x5D3C, "Data channel impending failure drive calibration retry "
	 "count"},
	{0x5D40, "Servo impending failure general hard drive failure"},
	{0x5D41, "Servo impending failure drive error rate too high"},
	{0x5D42, "Servo impending failure data error rate too high"},
	{0x5D43, "Servo impending failure seek error rate too high"},
	{0x5D44, "Servo impending failure too many block reassigns"},
	{0x5D45, "Servo impending failure access times too high"},
	{0x5D46, "Servo impending failure start unit times too high"},
	{0x5D47, "Servo impending failure channel parametrics"},
	{0x5D48, "Servo impending failure controller detected"},
	{0x5D49, "Servo impending failure throughput performance"},
	{0x5D4A, "Servo impending failure seek time performance"},
	{0x5D4B, "Servo impending failure spin-up retry count"},
	{0x5D4C, "Servo impending failure drive calibration retry count"},
	{0x5D50, "Spindle impending failure general hard drive failure"},
	{0x5D51, "Spindle impending failure drive error rate too high"},
	{0x5D52, "Spindle impending failure data error rate too high"},
	{0x5D53, "Spindle impending failure seek error rate too high"},
	{0x5D54, "Spindle impending failure too many block reassigns"},
	{0x5D55, "Spindle impending failure access times too high"},
	{0x5D56, "Spindle impending failure start unit times too high"},
	{0x5D57, "Spindle impending failure channel parametrics"},
	{0x5D58, "Spindle impending failure controller detected"},
	{0x5D59, "Spindle impending failure throughput performance"},
	{0x5D5A, "Spindle impending failure seek time performance"},
	{0x5D5B, "Spindle impending failure spin-up retry count"},
	{0x5D5C, "Spindle impending failure drive calibration retry count"},
	{0x5D60, "Firmware impending failure general hard drive failure"},
	{0x5D61, "Firmware impending failure drive error rate too high"},
	{0x5D62, "Firmware impending failure data error rate too high"},
	{0x5D63, "Firmware impending failure seek error rate too high"},
	{0x5D64, "Firmware impending failure too many block reassigns"},
	{0x5D65, "Firmware impending failure access times too high"},
	{0x5D66, "Firmware impending failure start unit times too high"},
	{0x5D67, "Firmware impending failure channel parametrics"},
	{0x5D68, "Firmware impending failure controller detected"},
	{0x5D69, "Firmware impending failure throughput performance"},
	{0x5D6A, "Firmware impending failure seek time performance"},
	{0x5D6B, "Firmware impending failure spin-up retry count"},
	{0x5D6C, "Firmware impending failure drive calibration retry count"},
	{0x5DFF, "Failure prediction threshold exceeded (false)"},

	{0x5E00, "Low power condition on"},
	{0x5E01, "Idle condition activated by timer"},
	{0x5E02, "Standby condition activated by timer"},
	{0x5E03, "Idle condition activated by command"},
	{0x5E04, "Standby condition activated by command"},
	{0x5E41, "Power state change to active"},
	{0x5E42, "Power state change to idle"},
	{0x5E43, "Power state change to standby"},
	{0x5E45, "Power state change to sleep"},
	{0x5E47, "Power state change to device control"},

	{0x6000, "Lamp failure"},

	{0x6100, "Video acquisition error"},
	{0x6101, "Unable to acquire video"},
	{0x6102, "Out of focus"},

	{0x6200, "Scan head positioning error"},

	{0x6300, "End of user area encountered on this track"},
	{0x6301, "Packet does not fit in available space"},

	{0x6400, "Illegal mode for this track"},
	{0x6401, "Invalid packet size"},

	{0x6500, "Voltage fault"},

	{0x6600, "Automatic document feeder cover up"},
	{0x6601, "Automatic document feeder lift up"},
	{0x6602, "Document jam in automatic document feeder"},
	{0x6603, "Document miss feed automatic in document feeder"},

	{0x6700, "Configuration failure"},
	{0x6701, "Configuration of incapable logical units failed"},
	{0x6702, "Add logical unit failed"},
	{0x6703, "Modification of logical unit failed"},
	{0x6704, "Exchange of logical unit failed"},
	{0x6705, "Remove of logical unit failed"},
	{0x6706, "Attachment of logical unit failed"},
	{0x6707, "Creation of logical unit failed"},
	{0x6708, "Assign failure occurred"},
	{0x6709, "Multiply assigned logical unit"},
	{0x670A, "Set target port groups command failed"},
	{0x670B, "ATA device feature not enabled"},

	{0x6800, "Logical unit not configured"},

	{0x6900, "Data loss on logical unit"},
	{0x6901, "Multiple logical unit failures"},
	{0x6902, "Parity/data mismatch"},

	{0x6A00, "Informational, refer to log"},

	{0x6B00, "State change has occurred"},
	{0x6B01, "Redundancy level got better"},
	{0x6B02, "Redundancy level got worse"},

	{0x6C00, "Rebuild failure occurred"},

	{0x6D00, "Recalculate failure occurred"},

	{0x6E00, "Command to logical unit failed"},

	{0x6F00, "Copy protection key exchange failure - authentication "
	 "failure"},
	{0x6F01, "Copy protection key exchange failure - key not present"},
	{0x6F02, "Copy protection key exchange failure - key not established"},
	{0x6F03, "Read of scrambled sector without authentication"},
	{0x6F04, "Media region code is mismatched to logical unit region"},
	{0x6F05, "Drive region must be permanent/region reset count error"},
	{0x6F06, "Insufficient block count for binding nonce recording"},
	{0x6F07, "Conflict in binding nonce recording"},
/*
 *	{0x70NN, "Decompression exception short algorithm id of nn"},
 */
	{0x7100, "Decompression exception long algorithm id"},

	{0x7200, "Session fixation error"},
	{0x7201, "Session fixation error writing lead-in"},
	{0x7202, "Session fixation error writing lead-out"},
	{0x7203, "Session fixation error - incomplete track in session"},
	{0x7204, "Empty or partially written reserved track"},
	{0x7205, "No more track reservations allowed"},
	{0x7206, "RMZ extension is not allowed"},
	{0x7207, "No more test zone extensions are allowed"},

	{0x7300, "Cd control error"},
	{0x7301, "Power calibration area almost full"},
	{0x7302, "Power calibration area is full"},
	{0x7303, "Power calibration area error"},
	{0x7304, "Program memory area update failure"},
	{0x7305, "Program memory area is full"},
	{0x7306, "RMA/PMA is almost full"},
	{0x7310, "Current power calibration area almost full"},
	{0x7311, "Current power calibration area is full"},
	{0x7317, "RDZ is full"},

	{0x7400, "Security error"},
	{0x7401, "Unable to decrypt data"},
	{0x7402, "Unencrypted data encountered while decrypting"},
	{0x7403, "Incorrect data encryption key"},
	{0x7404, "Cryptographic integrity validation failed"},
	{0x7405, "Error decrypting data"},
	{0x7471, "Logical unit access not authorized"},

	{0, NULL}
};

struct error_info2 {
	unsigned char code1, code2_min, code2_max;
	const char *fmt;
};

static const struct error_info2 additional2[] =
{
	{0x40,0x00,0x7f,"Ram failure"},
	{0x40,0x80,0xff,"Diagnostic failure on component"},
	{0x41,0x00,0xff,"Data path failure"},
	{0x42,0x00,0xff,"Power-on or self-test failure"},
	{0x4D,0x00,0xff,"Tagged overlapped commands"},
	{0x70,0x00,0xff,"Decompression exception short algorithm"},
	{0, 0, 0, NULL}
};

/*
 * Get additional sense code string or NULL if not available.
 * Returns 1 if it should be printed with ascq as arg.
 */
static const char *asc_ascq_string(unsigned char asc, unsigned char ascq)
{
	int i;
	unsigned short code = ((asc << 8) | ascq);

	for (i=0; additional[i].text; i++)
		if (additional[i].code12 == code)
			return additional[i].text;
	for (i=0; additional2[i].fmt; i++)
		if (additional2[i].code1 == asc &&
		    additional2[i].code2_min >= ascq &&
		    additional2[i].code2_max <= ascq)
			return additional2[i].fmt;
	return NULL;
}

/*
 * Additional sense descriptors, see 4.14.2.1 page 62 and thereabouts.
 */
static int sense_parse_descriptor(char *s, int *ppos, const uint8_t *info,
                                  int addl_len)
{
	int pos = *ppos;
	int len = 0;

	if (addl_len < 2)
		return 0;
	if (info[0] == 0x6) {
		/* object identification */
		uint64_t pid, oid;
		if (info[1]+2 != 32) {
			pos += sprintf(s+pos,
				       "invalid object ident length %d\n",
				       info[1]);
			goto out;
		}
		if (addl_len < 32) {
			pos += sprintf(s+pos,
			       "insufficient senselen %d for object ident\n",
				       addl_len);
			goto out;
		}
		len = 32;
		/* ignore not_init and completed funcs */
		pid = ntohll(&info[16]);
		oid = ntohll(&info[24]);
		pos += sprintf(s+pos, "  Offending pid 0x%016lx oid 0x%016lx\n",
			       pid, oid);
	}

	if (info[0] == 0x8) {
		/* attribute identification */
		int i;
		if (addl_len < info[1]+1) {
			pos += sprintf(s+pos,
			               "insufficient senselen %d for attr %d\n",
				       addl_len, info[1]+1);
			goto out;
		}
		if ((info[1]+1-4) % 8 != 0) {
			pos += sprintf(s+pos, "info len %d not 4 + N * 8\n",
				       info[1]+1);
			goto out;
		}
		len = info[1]+1;
		for (i=0; i<len; i+=8) {
			uint32_t page = ntohl(&info[i*8+0]);
			uint32_t number = ntohl(&info[i*8+4]);
			pos += sprintf(s+pos, "  Offending attribute"
			               " page 0x%08x number 0x%08x\n",
				       page, number);
		}
	}

	if (info[0] == 0x1) {
		/* spc3 4.5.2.2 information */
		uint64_t csi;
		if (info[1]+2 != 12) {
			pos += sprintf(s+pos, "invalid info length %d\n",
				       info[1]);
			goto out;
		}
		if (addl_len < 12) {
			pos += sprintf(s+pos,
			               "insufficient senselen %d for info\n",
				       addl_len);
			goto out;
		}
		len = 12;
		/* not sure it is wise to always convert this to int, but
		 * that is the use for read overflow at least */
		csi = ntohll(&info[4]);
		pos += sprintf(s+pos, "  Information 0x%016lx\n", csi);
	}

	/* consider adding attribute identification */
out:
	*ppos = pos;
	return len;
}

/*
 * Possibly debugging support.  Parse and print out interesting bits of the
 * sense (i.e. error) message returned from the target.  Returned in a new
 * string.
 *
 * OSD sense is always descriptor format.
 */
void osd_sense_extract(const uint8_t *sense, int len, int *key, int *asc_ascq)
{
	*key = 0;
	*asc_ascq = 0;
	if (len < 8)
		return;
	if ((sense[0] & 0x72) != 0x72)
		return;
	*key = sense[1];
	*asc_ascq = (sense[2] << 8) | sense[3];
}

char *osd_sense_as_string(const uint8_t *sense, int len)
{
	uint8_t code, key, asc, ascq, additional_len;
	int deferred;
	const char *keystr, *ascstr;
	const uint8_t *info;
	char s[1024];
	int pos;

	if (len < 8) {
		sprintf(s, "sense length too short\n");
		goto out;
	}
	code = sense[0];
	if ((code & 0x72) != 0x72) {
		sprintf(s, "code 0x%02x not expected 0x72 or 0x73\n", code);
		goto out;
	}
	deferred = code & 1;
	key = sense[1];
	asc = sense[2];
	ascq = sense[3];

	pos = 0;
	if (deferred)
		pos += sprintf(s+pos, "[deferred] ");

	keystr = sense_key_string(key);
	if (keystr)
		pos += sprintf(s+pos, "%s: ", keystr);
	else
		pos += sprintf(s+pos, "Unknown sense key 0x%02x: ", key);

	ascstr = asc_ascq_string(asc, ascq);
	if (ascstr)
	    pos += sprintf(s+pos, "%s\n", ascstr);
	else
	    pos += sprintf(s+pos, "Unknown asc/ascq\n");

	additional_len = sense[7];
	info = sense + 8;
	while (additional_len > 0) {
		int len = sense_parse_descriptor(s, &pos, info, additional_len);
		if (len == 0)
			break;
		info += len;
		additional_len -= len;
	}

	if (additional_len > 0)
		sprintf(s+pos, "Descriptor type 0x%02x len %d not shown",
		        info[0], additional_len);

out:
	return strdup(s);
}

/*
 * Return a pointer to the 8-byte CSI string.  Not necessarily an int.
 * Assumes proper descriptor format sense buffer.
 */
uint8_t *osd_sense_extract_csi(const uint8_t *sense, int len)
{
	uint8_t *s = NULL;

	/* skip header */
	sense += 8;
	len -= 8;
	while (len > 0) {
		int nextlen;
		if (len < 2)
			break;
		nextlen = sense[1] + 2;
		if (sense[0] == 0x1) {
			if (nextlen != 12)
				break;
			if (len < nextlen)
				break;
			s = sense + 4;
			break;
		}
		sense += nextlen;
		len -= nextlen;
	}
	return s;
}

