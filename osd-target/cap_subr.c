#include <stdio.h>
#include <stdlib.h>

#include "cap.h"
#include "attr.h"

/**
 * @doc
 *   Check if create time and capability versions match.
 *   Skipping version checking for now as it's always a
 */
int
cap_check_time_version(struct osd_device *osd, struct osd_v2_capability *caps, uint64_t pid, uint64_t oid) 
{
        int ret = 0;
        uint16_t len = 0;
        void *val = NULL;
        uint64_t time = 0;
        char name[ATTR_PAGE_ID_LEN] = {'\0'};
        char path[MAXNAMELEN];
        struct stat sb;

        int ret;

        osd_debug("%s: checking ..", __func__);

        get_dfile_name(path, osd->root, pid, oid);
        memset(&sb, 0, sizeof(sb));
        ret = stat(path, &sb);
        if (ret != 0)
                return OSD_ERROR;
        len = 6;
        val = &time;
        if (number == UTSAP_CTIME)
                set_hton_time(val, sb.st_ctime, sb.st_ctim.tv_nsec);

        // check that create times match. 
        //
        uint8_t caps_time = caps->h.object_created_time.time[0];

        osd_debug("%s: val %llu time[0] %llu time %llu \n", __func__, llu(val), 
        llu(caps_time), llu(caps->h.object_created_time.time));

        uint64_t  create_time = time;
        if (caps_time && caps_time != create_time) {
            osd_debug ("%s: times don't match\n"
                    "obj create time: %llu, cap create time: %llu\n", 
                    __func__, llu(create_time), llu(caps->h.object_created_time.time));
            return 0;
        }

    }

    osd_debug ("%s: done checking ..", __func__);


    return 1;
}

/**
 * @doc
 *   Check if the capabilty that's passed with the request is valid
 */
int
cap_passes_basic_tests(void *handle, struct osd_v2_capability *caps, uint64_t pid, uint64_t oid)
{
    int ret = 0;

    osd_debug("%s: checking basic fields!", __func__);

    // Check cap hasn't expired
    time_t now = time(0);
    osd_debug("%s:  now\t\t%llu\n exp_time\t%llu",
            __func__, llu(now), llu(caps->h.expiration_time.time));
    if (now > (time_t)caps->h.expiration_time.time) {
        osd_debug ( "%s: Cap has expired. diff (%llu)\n",
                __func__, llu((uint64_t)now - (uint64_t)caps->h.expiration_time.time));
        //this->setResponseCodes(ASCQ_CAPABILITY_EXPIRED);
        return 0;
    }

    return 1;
}
