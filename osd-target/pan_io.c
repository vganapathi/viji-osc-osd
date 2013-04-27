#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pan/osdfs/osd_ioctl.h>


#include "io.h"
#include "osd.h"
#include "osd-sense.h"
#include "osd-types.h"
#include "target-sense.h"
#include "osd-util/osd-util.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/*
 * @offset: offset from byte zero of the object where data will be read
 * @len: length of data to be read
 * @outdata: pointer to start of the data-out-buffer: destination of read
 *
 * returns:
 * ==0: success, used_outlen is set
 * > 0: error, sense is set
 */

int contig_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
        uint64_t len, uint64_t offset, uint8_t *outdata, 
        uint64_t *used_outlen, uint8_t *sense)
{
    size_t readlen;
    int ret, fd;
    char path[MAXNAMELEN];
    struct osd_ioctl_openclose ioco;
    struct osd_ioctl_rw iocr;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu", __func__,
            llu(pid), llu(oid), llu(len), llu(offset));

    assert(osd && osd->root && osd->handle && outdata && used_outlen 
            && sense);

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    bzero(&iocr, sizeof(struct osd_ioctl_rw));
    iocr.group = pid;
    iocr.object = oid;
    iocr.offset = offset;
    iocr.length = len;
    iocr.ret.rval = (char *)outdata;
    iocr.hdr.cdb_flags = 0;

    ret = ioctl(osd->handle->fd, OSD_IOCMD_READ, &iocr);
    if ((ret < 0) || iocr.ret.rval_length != len ) {
        osd_error("%s: len %llu rval_len %llu", __FUNCTION__, llu(len), llu(iocr.ret.rval_length));
        goto out_hw_err;
    }

    readlen = iocr.ret.rval_length;
    /* valid, but return a sense code */
    if ((size_t) readlen < len) {
        memset(outdata + readlen, 0, len - readlen);
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);
    }

    *used_outlen = len;

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK;

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

}

int sgl_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
        uint64_t len, uint64_t offset, const struct sg_list *sglist,
        uint8_t *outdata, uint64_t *used_outlen, uint8_t *sense)
{
    ssize_t readlen;
    int ret = 0, fd;
    char path[MAXNAMELEN];
    uint64_t inlen, pairs, offset_val, data_offset, length;
    unsigned int i;
    struct osd_ioctl_rw iocr;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu", __func__,
            llu(pid), llu(oid), llu(len), llu(offset));

    assert(osd && osd->root && osd->handle && outdata && used_outlen 
            && sense);

    pairs = sglist->num_entries;
    inlen = (pairs * sizeof(uint64_t) * 2) + sizeof(uint64_t);
    assert(pairs * sizeof(uint64_t) * 2 == inlen - sizeof(uint64_t));

    osd_debug("%s: offset,len pairs %llu", __func__, llu(pairs));
    osd_debug("%s: data offset %llu", __func__, llu(inlen));


    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    data_offset = 0;
    readlen = 0;

    for (i = 0; i < pairs; i++) {
        /* offset into dest */
        offset_val = get_ntohll(&sglist->entries[i].offset);
        length = get_ntohll(&sglist->entries[i].bytes_to_transfer);

        osd_debug("%s: Offset: %llu Length: %llu", __func__, llu(offset_val + offset),
                llu(length));

        osd_debug("%s: Position in data buffer: %llu master offset %llu", __func__, llu(data_offset), llu(offset));

        osd_debug("%s: ------------------------------", __func__);
        bzero(&iocr, sizeof(struct osd_ioctl_rw));
        iocr.group = pid;
        iocr.object = oid;
        iocr.offset = offset + offset_val;
        iocr.length = length;
        iocr.ret.rval = (char *)(outdata + data_offset);
        iocr.hdr.cdb_flags = 0;

        ret = ioctl(osd->handle->fd, OSD_IOCMD_READ, &iocr);
        osd_debug("%s: return value is %d", __func__, ret);
        if (ret < 0)
            goto out_hw_err;
        ret = iocr.ret.rval_length;
        if ((size_t) ret < length) {
            /* valid, fill with zeros */
            memset(outdata+data_offset+ret, 0, length - ret);
            length = ret;
        }
        data_offset += length;
        readlen += length;
    }

    *used_outlen = readlen;

    /* valid, but return a sense code */
    if ((size_t) readlen < len)
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK;

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;


}

int vec_read(struct osd_device *osd, uint64_t pid, uint64_t oid, 
        uint64_t len, uint64_t offset, const uint8_t *indata,
        uint8_t *outdata, uint64_t *used_outlen, uint8_t *sense)
{
    ssize_t readlen;
    int ret = 0, fd;
    char path[MAXNAMELEN];
    uint64_t inlen, bytes, hdr_offset, offset_val, data_offset, length, stride;
    unsigned int i;
    struct osd_ioctl_rw iocr;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu", __func__,
            llu(pid), llu(oid), llu(len), llu(offset));

    assert(osd && osd->root && osd->handle && outdata && used_outlen 
            && sense);

    stride = get_ntohll(indata);
    hdr_offset = sizeof(uint64_t);
    length = get_ntohll(indata + hdr_offset);

    osd_debug("%s: stride is %llu and len is %llu", __func__, llu(stride), 
            llu(length));

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    data_offset = 0;
    bytes = len;
    readlen = 0;
    osd_debug("%s: bytes to read is %llu", __func__, llu(bytes));
    offset_val = 0;
    while (bytes > 0) {
        osd_debug("%s: Position in data buffer: %llu", __func__,
                llu(data_offset));
        osd_debug("%s: Offset: %llu", __func__, llu(offset_val + offset));
        osd_debug("%s: ------------------------------", __func__);
        bzero(&iocr, sizeof(struct osd_ioctl_rw));
        iocr.group = pid;
        iocr.object = oid;
        iocr.offset = offset + offset_val;
        iocr.length = length;
        iocr.hdr.cdb_flags = 0;
        iocr.ret.rval = (char *)(outdata + data_offset);

        ret = ioctl(osd->handle->fd, OSD_IOCMD_READ, &iocr);
        osd_debug("%s: return value is %d", __func__, ret);
        if (ret < 0 || iocr.ret.rval_length != length)
            goto out_hw_err;
        ret = iocr.ret.rval_length;

        readlen += ret;
        data_offset += length;
        offset_val += stride;
        bytes -= length;
        if (bytes < length)
            length = bytes;
        osd_debug("%s: Total Bytes Left to read: %llu", __func__,
                llu(bytes));
    }

    *used_outlen = readlen;

    /* valid, but return a sense code */
    if ((size_t) readlen < len)
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK;

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

}

/*
 * TODO: We did not implement length as an attribute in attr table, hence
 * relying on underlying ext3 fs to get length of the object. Also since we
 * are not implementing quotas, we ignore maximum length attribute of an
 * object and the partition which stores the object.
 *
 * @offset: offset from byte zero of the object where data will be written
 * @len: length of data to be written
 * @dinbuf: pointer to start of the Data-in-buffer: source of data
 */
int contig_write(struct osd_device *osd, uint64_t pid, uint64_t oid, 
        uint64_t len, uint64_t offset, const uint8_t *dinbuf, 
        uint8_t *sense)
{
    int ret;
    int fd;
    struct osd_ioctl_rw iocw;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->fd && dinbuf && sense);

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    bzero(&iocw, sizeof(struct osd_ioctl_rw));
    iocw.group = pid;
    iocw.object = oid;
    iocw.offset = offset;
    iocw.length = len;
    iocw.obj_data = dinbuf;
    iocw.hdr.cdb_flags = 0;

    ret = ioctl(osd->handle->fd, OSD_IOCMD_WRITE, &iocw);
    osd_info("%s: return value is %d", __func__, ret);
    if (ret < 0 || (uint64_t)iocw.ret.rval_length != len)
    {
        osd_error("%s: len %llu rval_len %llu", __FUNCTION__, llu(len), llu(iocw.ret.rval_length));
        goto out_hw_err;
    }

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK; /* success */

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;


}

int sgl_write(struct osd_device *osd, uint64_t pid, uint64_t oid, 
        uint64_t len, uint64_t offset, const uint8_t *dinbuf,
        const struct sg_list *sglist,
        uint8_t *sense) 
{
    int ret;
    int fd;
    char path[MAXNAMELEN];
    uint64_t pairs, data_offset, offset_val, length;
    unsigned int i;
    struct osd_ioctl_rw iocw;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->fd && dinbuf && sense);

    pairs = sglist->num_entries;
    assert(pairs != 0);

    osd_debug("%s: offset,len pairs %llu", __func__, llu(pairs));

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    data_offset = 0;

    for (i=0; i<pairs; i++) {
        /* offset into dest */
        offset_val = get_ntohll(&sglist->entries[i].offset);
        length = get_ntohll(&sglist->entries[i].bytes_to_transfer);

        osd_debug("%s: Offset: %llu Length: %llu",
                __func__, llu(offset_val + offset), llu(length));

        osd_debug("%s: Position in data buffer: %llu",
                __func__, llu(data_offset));

        osd_debug("%s: ------------------------------", __func__);
        bzero(&iocw, sizeof(struct osd_ioctl_rw));
        iocw.group = pid;
        iocw.object = oid;
        iocw.offset = offset_val+offset;
        iocw.length = length;
        iocw.obj_data = dinbuf+data_offset;
        iocw.hdr.cdb_flags = 0;

        ret = ioctl(osd->handle->fd, OSD_IOCMD_WRITE, &iocw);
        osd_info("%s: return value is %d", __func__, ret);
        if (ret < 0 || (uint64_t)iocw.ret.rval_length != length)
            goto out_hw_err;

        data_offset += length;
    }

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK; /* success */

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;


}


int vec_write(struct osd_device *osd, uint64_t pid, uint64_t oid,
        uint64_t len, uint64_t offset, const uint8_t *dinbuf,
        uint8_t *sense)
{
    int ret;
    int fd;
    char path[MAXNAMELEN];
    uint64_t data_offset, offset_val, hdr_offset, length, stride, bytes;
    unsigned int i;
    struct osd_ioctl_rw iocw;

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->fd && dinbuf && sense);

    stride = get_ntohll(dinbuf);
    hdr_offset = sizeof(uint64_t);
    length = get_ntohll(dinbuf + hdr_offset);

    osd_debug("%s: stride is %llu and len is %llu", __func__, llu(stride), llu(length));

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    data_offset = hdr_offset + sizeof(uint64_t);

    bytes = len - (2*sizeof(uint64_t));

    osd_debug("%s: bytes to write is %llu", __func__, llu(bytes));
    offset_val = 0;
    while (bytes > 0) {
        osd_debug("%s: Position in data buffer: %llu", __func__,
                llu(data_offset));
        osd_debug("%s: Offset: %llu", __func__, llu(offset_val + offset));
        osd_debug("%s: ------------------------------", __func__);
        bzero(&iocw, sizeof(struct osd_ioctl_rw));
        iocw.group = pid;
        iocw.object = oid;
        iocw.offset = offset_val + offset;
        iocw.length = length;
        iocw.obj_data = dinbuf + data_offset;
        iocw.hdr.cdb_flags = 0;

        ret = ioctl(osd->handle->fd, OSD_IOCMD_WRITE, &iocw);
        if (ret < 0 || (uint64_t)iocw.ret.rval_length != length)
            goto out_hw_err;
        data_offset += length;
        offset_val += stride;
        bytes -= length;
        if (bytes < length)
            length = bytes;
        osd_debug("%s: Total Bytes Left to write: %llu", __func__,
                llu(bytes));
    }

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return OSD_OK; /* success */

out_hw_err:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

out_cdb_err:
    ret = sense_build_sdd(sense, OSD_SSK_ILLEGAL_REQUEST,
            OSD_ASC_INVALID_FIELD_IN_CDB, pid, oid);
    return ret;

}

int 
setup_root_paths (const char* root, struct osd_device *osd) 
{

    int i = 0;
    int ret = 0;
    char path[MAXNAMELEN];
    char *argv[] = { strdup("osd-target"), NULL };

    osd_set_progname(1, argv);  /* for debug messages from libosdutil */
    mhz = get_mhz(); /* XXX: find a better way of profiling */

    if (strlen(root) > MAXROOTLEN) {
        osd_error("strlen(%s) > MAXROOTLEN", root);
        ret = -ENAMETOOLONG;
        goto out;
    }

    memset(osd, 0, sizeof(*osd));

    /* test if root exists and is a directory */
    ret = create_dir(root);
    if (ret != 0) {
        osd_error("!create_dir_root(%s)", root);
        goto out;
    }

    /* test create 'data/dfiles' sub-directory */
    sprintf(path, "%s/%s/", root, dfiles);
    ret = create_dir(path);
    if (ret != 0) {
        osd_error("!create_dir_data/dfiles(%s)", path);
        goto out;
    }

    osd->root = strdup(root);
    if (!osd->root) {
        osd_error("!strdup_root(%s)", root);
        ret = -ENOMEM;
        goto out;
    }

    osd_debug("%s: root %s", __func__, osd->root);

    osd->handle = malloc(sizeof(*osd->handle));

    sprintf(path, "%s/%s", root, dfiles);
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        osd_error("could not open osd->handle %d, %s", fd, strerror(errno));
    }
    osd->handle->fd = fd;
    osd_debug("%s: fd %d", __func__, fd);

out:
    return ret;
}

int osd_create_datafile(struct osd_device *osd, uint64_t pid,
        uint64_t oid)
{
    osd_debug("%s: called but not needed!", __func__);
    return 0;
}

int format_osd(struct osd_device *osd, uint64_t capacity, uint32_t cdb_cont_len, uint8_t *sense)
{
    int ret = 0;
    char *root = NULL;
    char path[MAXNAMELEN];
    struct stat sb;


    assert(osd && osd->root && osd->handle && sense);

    root = strdup(osd->root);

    osd_debug("%s: osd root %s capacity %llu MB", __func__, osd->root, llu(capacity >> 20));

    if(osd->handle->fd != -1) {
        goto out;
    }

    ret = osd_close(osd);
    if (ret) {
        osd_error("%s: osd close failed, ret %d", __func__, ret);
        goto out_sense;
    }
create:
    ret = osd_open(root, osd); /* will create files/dirs under root */
    if (ret != 0) {
        osd_error("%s: osd_open %s failed", __func__, root);
        goto out_sense;
    }
    memset(&osd->ccap, 0, sizeof(osd->ccap)); /* reset ccap */
    ret = OSD_OK;
    goto out;

out_sense:
    ret = sense_build_sdd(sense, OSD_SSK_HARDWARE_ERROR,
            OSD_ASC_SYSTEM_RESOURCE_FAILURE, 0, 0);

out:
    return ret;
}


int osd_begin_txn(struct osd_device *osd)
{
    return 0;
}

int osd_end_txn(struct osd_device *osd)
{
    return 0;

}

int osd_close(struct osd_device *osd)
{
    int ret = 0;

#ifdef __DBUS_STATS__
    gsh_dbus_pkgshutdown();
#endif

    free(osd->root);
    osd->root = NULL;
    return ret;
}
