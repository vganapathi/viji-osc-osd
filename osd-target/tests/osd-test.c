#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "osd-defs.h"
#include "osd-types.h"
#include "osd.h"
#include "db.h"
#include "attr.h"
#include "obj.h"
#include "util.h"

void test_osd_create(struct osd_device *osd);
void test_osd_set_attributes(struct osd_device *osd);

void test_osd_create(struct osd_device *osd)
{
	int ret = 0;
	void *sense = Calloc(1, 1024);

	ret = osd_create(osd, 0, 1, 0, sense);
	if (ret != 0)
		error("0, 1, 0 failed as expected");
	ret = osd_create(osd, USEROBJECT_PID_LB, 1, 0, sense);
	if (ret != 0)
		error("USEROBJECT_PID_LB, 1, 0 failed as expected");
	ret = osd_create(osd, USEROBJECT_PID_LB, USEROBJECT_OID_LB, 2, sense);
	if (ret != 0)
		error("USEROBJECT_PID_LB, USEROBJECT_OID_LB, 2 failed as "
		      "expected");
	ret = osd_create(osd, USEROBJECT_PID_LB, USEROBJECT_PID_LB, 0, sense);
	if (ret != 0)
		error_fatal("USEROBJECT_PID_LB, USEROBJECT_PID_LB, 0 failed");
	ret = osd_remove(osd, USEROBJECT_PID_LB, USEROBJECT_PID_LB, sense);
	if (ret != 0)
		error_fatal("USEROBJECT_PID_LB, USEROBJECT_PID_LB");

	free(sense);
}

void test_osd_set_attributes(struct osd_device *osd)
{
	int ret = 0;
	void *sense = Calloc(1, 1024);
	void *val = Calloc(1, 1024);

	ret = osd_create(osd, USEROBJECT_PID_LB, USEROBJECT_PID_LB, 0, sense);
	if (ret != 0)
		error_fatal("USEROBJECT_PID_LB, USEROBJECT_PID_LB, 0 failed");

	ret = osd_set_attributes(osd, ROOT_PID, ROOT_OID, 0, 0, 
				 NULL, 0, sense);
	if (ret != 0)
		error("osd_set_attributes root failed as expected");
	ret = osd_set_attributes(osd, PARTITION_PG_LB, PARTITION_OID, 0, 0, 
				 NULL, 0, sense);
	if (ret != 0)
		error("osd_set_attributes partition failed as expected");
	ret = osd_set_attributes(osd, COLLECTION_PID_LB, COLLECTION_OID_LB, 0, 
				 0, NULL, 0, sense);
	if (ret != 0)
		error("osd_set_attributes collection failed as expected");
	ret = osd_set_attributes(osd, USEROBJECT_PID_LB, USEROBJECT_OID_LB, 0,
				 0, NULL, 0, sense);
	if (ret != 0)
		error("osd_set_attributes userobject failed as expected");

	sprintf(val, "This is test, long test more than forty bytes");
	ret = osd_set_attributes(osd, USEROBJECT_PID_LB, USEROBJECT_OID_LB, 
				 USEROBJECT_PG_LB, 0, val, strlen(val)+1, 
				 sense);
	if (ret != 0)
		error("osd_set_attributes number failed as expected");

	sprintf(val, "Madhuri Dixit");
	ret = osd_set_attributes(osd, USEROBJECT_PID_LB, USEROBJECT_OID_LB, 
				 USEROBJECT_PG_LB, 1, val, strlen(val)+1, 
				 sense);
	if (ret != 0)
		error_fatal("osd_set_attributes failed for mad dix");

	ret = osd_remove(osd, USEROBJECT_PID_LB, USEROBJECT_PID_LB, sense);
	if (ret != 0)
		error_fatal("osd_remove USEROBJECT_PID_LB, USEROBJECT_PID_LB");

	free(sense);
	free(val);
}

int main()
{
	int ret = 0;
	const char *root = "/tmp/";
	struct osd_device osd;

	ret = osd_open(root, &osd);
	if (ret != 0)
		error_fatal("%s: osd_open", __func__);

	/*test_osd_create(&osd);*/
	test_osd_set_attributes(&osd);

	ret = osd_close(&osd);
	if (ret != 0)
		error_fatal("%s: osd_close", __func__);

	return 0;
}

