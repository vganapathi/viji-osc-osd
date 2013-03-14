#include <string.h>
#include <pan/osdfs/osd.h>
#include "cap.h"
#include "attr.h"

#define NSEC_PER_SEC                1000000000
#define NSEC_PER_MSEC               1000000
#define MSECS_PER_SEC               1000
static inline void
TIMESPEC_TO_MSECS(struct osd_timespec *tsp, uint64_t *msp)
{
      *msp = (uint64_t)tsp->tv_sec * (uint64_t)NSEC_PER_SEC +
               (uint64_t)tsp->tv_nsec;
      *msp = (*msp)/NSEC_PER_MSEC;
}

/**
 * @doc
 *   Check if create time and capability versions match.
 *   Skipping version checking for now as it's always a
 */
int
cap_check_time_version(struct osd_device *osd, struct osd_v2_capability *caps, uint64_t pid, uint64_t oid) 
{
    uint32_t page = 0, id = 0, used_outlen = 1000;
    int i, ret;
    void *handle = osd->handle;
    uint64_t caps_time, create_time; //in milliseconds

    osd_debug("%s: checking ..", __func__);

    cap_scope_t cap_scope = cap_scope_object;
    if(pid == 0)
        cap_scope = cap_scope_dco;
    else if(oid == 0)
        cap_scope = cap_scope_gco;

    if (cap_scope != cap_scope_device) {
        void *outdata1 = Malloc(16);    //, *outdata2 = Malloc(16), *outdata3 = Malloc(16);
        uint16_t outlen1 = 0;           //, outlen2 = 0, outlen3 = 0;
        if (cap_scope == cap_scope_dco) {
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_Created);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_Created);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen1, outdata1, 0, &used_outlen);
#if 0
            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_CapabilityVersionA);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_CapabilityVersionA);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen2, outdata2, 0, &used_outlen);

            page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_CapabilityVersionB);
            id = GET_ATTRIBUTE_ID(ATTR_DEVICE_CapabilityVersionB);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen3, outdata3, 0, &used_outlen);
#endif
        }
        else if (cap_scope == cap_scope_gco ) {
            page = GET_ATTRIBUTE_PAGE(ATTR_GROUP_Created);
            id = GET_ATTRIBUTE_ID(ATTR_GROUP_Created);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen1, outdata1, 0, &used_outlen);
#if 0
            page = GET_ATTRIBUTE_PAGE(ATTR_GROUP_CapabilityVersionA);
            id = GET_ATTRIBUTE_ID(ATTR_GROUP_CapabilityVersionA);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen2, outdata2, 0, &used_outlen);
            page = GET_ATTRIBUTE_PAGE(ATTR_GROUP_CapabilityVersionB);
            id = GET_ATTRIBUTE_ID(ATTR_GROUP_CapabilityVersionB);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen3, outdata3, 0, &used_outlen);
#endif
        }
        else if (cap_scope == cap_scope_object) {
            page = GET_ATTRIBUTE_PAGE(ATTR_OBJECT_VirtualCreated);
            id = GET_ATTRIBUTE_ID(ATTR_OBJECT_VirtualCreated);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen1, outdata1, 0, &used_outlen);
#if 0
            page = GET_ATTRIBUTE_PAGE(ATTR_OBJECT_CapabilityVersionA);
            id = GET_ATTRIBUTE_ID(ATTR_OBJECT_CapabilityVersionA);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen2, outdata2, 0, &used_outlen);
            page = GET_ATTRIBUTE_PAGE(ATTR_OBJECT_CapabilityVersionB);
            id = GET_ATTRIBUTE_ID(ATTR_OBJECT_CapabilityVersionB);
            ret = _attr_get_attr(handle, pid, oid, page, id, page,
                                id, outlen3, outdata3, 0, &used_outlen);
#endif
        }
        else {
            osd_error("%s: cap_scope is something else!! %d", __func__, cap_scope);
            ret = 0;
        }

        if (ret != 0) {
            osd_error("%s: attr_get_attr returned %d!", __func__, ret);
            return 0;
        }

        // check that create times match. 
        //

        //bzero as caps_time is only 48 bits
        bzero(&caps_time, sizeof(caps_time));
        memcpy(&caps_time, &caps->h.object_created_time.time, sizeof(uint8_t) * 6);

        osd_debug("%s: outdata %llx capstime %llx time %llx \n", __func__, 
                  llu(*(uint64_t *)outdata1),
                  llu(caps_time), 
                  llu(*(uint64_t *)caps->h.object_created_time.time));

        uint64_t  create_time; //in milliseconds
        if (cap_scope == cap_scope_object) {
            create_time = *(uint64_t *)outdata1;
            create_time = create_time/NSEC_PER_MSEC;
        }
        else {
            TIMESPEC_TO_MSECS((struct osd_timespec *)outdata1,&create_time);
        }
        if (caps_time && caps_time != create_time) {
            osd_error ("%s: times don't match\n"
                    "obj create time: %llu, cap create time: %llu \n", 
                    __func__, llu(create_time), llu(caps_time));
            return 0;
        }

#if 0
//CAP version is always a
        uint64_t cap_version_a = 0;
        cap_version_a = *(uint64_t *)outdata2;
        if (m_cap->obj_sel.spec.o.cap_version_a != cap_version_a) {
            OSD_DEBUG(mod_osd, "Task::isCreateTimeAndCapVerValid - cap "
                    "version a does not match\n"
                    "obj cap_version_a: %llu, cap cap_version_a: %llu\n", 
                    cap_version_a, m_cap->obj_sel.spec.o.cap_version_a);
            return 0;
        }

        uint32_t cap_version_b = 0;
        cap_version_b = *(pan_uint32_t *)outdata3;
        if (m_cap->obj_sel.spec.o.cap_version_b != cap_version_b) {
            OSD_DEBUG(mod_osd, "Task::isCreateTimeAndCapVerValid - cap "
                    "version b does not match\n"
                    "obj cap_version_b: %lu, cap cap_version_b: %lu\n", 
                    cap_version_b, m_cap->obj_sel.spec.o.cap_version_b);
            return 0;
        }
#endif
        free(outdata1);
        //free(outdata2);
        //free(outdata3);
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
    //in seconds
    time_t now = time(0);

    osd_debug("%s: checking basic fields!", __func__);

    now = now * MSECS_PER_SEC;

    // Check cap hasn't expired
    time_t exp_time;
    memcpy(&exp_time, &caps->h.expiration_time.time, sizeof(uint8_t) *6);
    osd_debug("%s:  now: %llu \t exp_time: %llu",
            __func__, llu(now), llu(exp_time));
    if (now > exp_time) {
        osd_error ( "%s: Cap has expired. diff (%llu)\n",
                __func__, llu((uint64_t)now - (uint64_t)caps->h.expiration_time.time));
        return 0;
    }

    // Check boot epoch match
    if (caps->od.obj_desc.boot_epoch) {
        void *outdata = Malloc(16);
        uint16_t outlen = 0;
        uint32_t used_outlen = 100;
        uint32_t page = GET_ATTRIBUTE_PAGE(ATTR_DEVICE_BootEpoch);
        uint32_t id = GET_ATTRIBUTE_ID(ATTR_DEVICE_BootEpoch);
        osd_debug("%s: page %llu id %llu \n", __func__, llu(page), llu(id));

        ret = _attr_get_attr(handle, 0, 0, page, id, page,
                id, outlen, outdata, 0, &used_outlen);
        if (caps->od.obj_desc.boot_epoch != *(uint16_t*)outdata) {
            osd_error("%s: Boot epoch sent(%llu) device(%llu)\n",
                    __func__, llu(caps->od.obj_desc.boot_epoch), llu(*(uint16_t*)outdata));
            free(outdata);
            return 0;
        }
        free(outdata);
    }

    return 1;
}
