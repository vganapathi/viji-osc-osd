/*
 * Test the chardev transport to the kernel.
 */

#ifndef __KERNEL__
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#define BITS_PER_LONG 32 

#endif

#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <libosd/interface.h>
#include <libosd/util.h>
#include <libosd/osd_hdr.h>

#define VARLEN_CDB_SIZE 200
#define VARLEN_CDB 0x7f

/*
 * Initializes a new varlen cdb.
 */
static void varlen_cdb_init(uint8_t *cdb, uint16_t action)
{
	memset(cdb, 0, VARLEN_CDB_SIZE);
	cdb[0] = VARLEN_CDB;
	cdb[7] = 192;
	cdb[8] = action >> 8;
	cdb[9] = action & 0xff;
	cdb[11] = 2 << 4;  /* get/set attributes page format */
}

static void cdb_build_inquiry(uint8_t *cdb)
{
	memset(cdb, 0, 6);
	cdb[0] = INQUIRY;
	cdb[4] = 80;
}

static void hexdump(uint8_t *d, size_t len)
{
	size_t offset = 0;

	while (offset < len) {
		int i, range;

		range = 8;
		if (range > len-offset)
			range = len-offset;
		printf("%4lx:", offset);
		for (i=0; i<range; i++)
			printf(" %02x", d[offset+i]);
		printf("\n");
		offset += range;
	}
}

int main(int argc, char *argv[])
{
	uint8_t cdb[200];
	uint8_t inquiry_rsp[80];

	set_progname(argc, argv);

	interface_init("/dev/sua");

	info("inquiry");
	cdb_build_inquiry(cdb);
	memset(inquiry_rsp, 0, sizeof(inquiry_rsp));
	cdb_read_cmd(cdb, 6, inquiry_rsp, sizeof(inquiry_rsp));
	hexdump(inquiry_rsp, sizeof(inquiry_rsp));

	info("osd flush osd");
	varlen_cdb_init(cdb, OSD_FLUSH_OSD);
	cdb[10] = 2;  /* flush everything */
	cdb_nodata_cmd(cdb, 200);

	interface_exit();
	return 0;
}
