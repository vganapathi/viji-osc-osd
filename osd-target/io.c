#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>


#include "io.h"
#include "db.h"
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
    ssize_t readlen;
    int ret, fd;
    char path[MAXNAMELEN];

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu", __func__,
            llu(pid), llu(oid), llu(len), llu(offset));

    assert(osd && osd->root && osd->handle && outdata && used_outlen 
            && sense);

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDONLY|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0) {
        osd_error("%s: open faild on [%s]", __func__, path);
        goto out_cdb_err;
    }

    readlen = pread(fd, outdata, len, offset);
    ret = close(fd);
    if ((readlen < 0) || (ret != 0))
        goto out_hw_err;

    /* valid, but return a sense code */
    if ((size_t) readlen < len) {
        memset(outdata + readlen, 0, len - readlen);
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);
    }

    *used_outlen = len;

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return ret;

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
    int ret, fd;
    char path[MAXNAMELEN];
    uint64_t inlen, pairs, offset_val, data_offset, length;
    unsigned int i;

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

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDONLY|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0)
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
        ret = pread(fd, outdata+data_offset, length, offset_val+offset);
        osd_debug("%s: return value is %d", __func__, ret);
        if (ret < 0)
            goto out_hw_err;
        if ((size_t) ret < length) {
            /* valid, fill with zeros */
            memset(outdata+data_offset+ret, 0, length - ret);
            length = ret;
        }
        data_offset += length;
        readlen += length;
    }

    ret = close(fd);
    if (ret != 0)
        goto out_hw_err;

    *used_outlen = readlen;

    /* valid, but return a sense code */
    if ((size_t) readlen < len)
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return ret;

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
    int ret, fd;
    char path[MAXNAMELEN];
    uint64_t inlen, bytes, hdr_offset, offset_val, data_offset, length, stride;
    unsigned int i;

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

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDONLY|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0)
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
        ret = pread(fd, outdata+data_offset, length, offset_val+offset);
        if (ret < 0 || (uint64_t)ret != length)
            goto out_hw_err;
        readlen += ret;
        data_offset += length;
        offset_val += stride;
        bytes -= length;
        if (bytes < length)
            length = bytes;
        osd_debug("%s: Total Bytes Left to read: %llu", __func__,
                llu(bytes));
    }

    ret = close(fd);
    if (ret != 0)
        goto out_hw_err;

    *used_outlen = readlen;

    /* valid, but return a sense code */
    if ((size_t) readlen < len)
        ret = sense_build_sdd_csi(sense, OSD_SSK_RECOVERED_ERROR,
                OSD_ASC_READ_PAST_END_OF_USER_OBJECT,
                pid, oid, readlen);

    fill_ccap(&osd->ccap, NULL, USEROBJECT, pid, oid, 0);
    return ret;

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
    char path[MAXNAMELEN];

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->dbc && dinbuf && sense);

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDWR|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0)
        goto out_cdb_err;

    ret = pwrite(fd, dinbuf, len, offset);
    if (ret < 0 || (uint64_t)ret != len)
        goto out_hw_err;
    ret = close(fd);
    if (ret != 0)
        goto out_hw_err;

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
        const struct sg_list *sglist, uint8_t *sense) 
{
    int ret;
    int fd;
    char path[MAXNAMELEN];
    uint64_t pairs, data_offset, offset_val, length;
    unsigned int i;

    osd_info("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->dbc && dinbuf && sense);

    pairs = sglist->num_entries;
    assert(pairs != 0);

    osd_info("%s: offset,len pairs %llu", __func__, llu(pairs));

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDWR|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0)
        goto out_cdb_err;

    data_offset = 0;

    for (i=0; i<pairs; i++) {
        /* offset into dest */
        offset_val = get_ntohll(&sglist->entries[i].offset);
        length = get_ntohll(&sglist->entries[i].bytes_to_transfer);

        osd_info("%s: Offset: %llu Length: %llu",
                __func__, llu(offset_val + offset), llu(length));

        osd_info("%s: Position in data buffer: %llu",
                __func__, llu(data_offset));

        osd_info("%s: ------------------------------", __func__);
        ret = pwrite(fd, dinbuf+data_offset, length, offset_val+offset);
        data_offset += length;
        osd_info("%s: return value is %d", __func__, ret);
        if (ret < 0 || (uint64_t)ret != length)
            goto out_hw_err;
    }

    ret = close(fd);
    if (ret != 0)
        goto out_hw_err;

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

    osd_debug("%s: pid %llu oid %llu len %llu offset %llu data %p",
            __func__, llu(pid), llu(oid), llu(len), llu(offset), dinbuf);

    assert(osd && osd->root && osd->handle->dbc && dinbuf && sense);

    stride = get_ntohll(dinbuf);
    hdr_offset = sizeof(uint64_t);
    length = get_ntohll(dinbuf + hdr_offset);

    osd_debug("%s: stride is %llu and len is %llu", __func__, llu(stride), llu(length));

    if (!(pid >= USEROBJECT_PID_LB && oid >= USEROBJECT_OID_LB))
        goto out_cdb_err;

    get_dfile_name(path, osd->root, pid, oid);
    fd = open(path, O_RDWR|O_LARGEFILE); /* fails on non-existent obj */
    if (fd < 0)
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
        ret = pwrite(fd, dinbuf+data_offset, length, offset_val+offset);
        if (ret < 0 || (uint64_t)ret != length)
            goto out_hw_err;
        data_offset += length;
        offset_val += stride;
        bytes -= length;
        if (bytes < length)
            length = bytes;
        osd_debug("%s: Total Bytes Left to write: %llu", __func__,
                llu(bytes));
    }
    ret = close(fd);
    if (ret != 0)
        goto out_hw_err;

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
setup_root_paths (const char* root, struct osd_device *osd) {                              

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

    /* to prevent fan-out create 256 subdirs under dfiles */
    for (i = 0; i < 256; i++) {
        sprintf(path, "%s/%s/%02x/", root, dfiles, i);
        ret = create_dir(path);
        if (ret != 0) {
            osd_error("!create_dir_256(%s)", path);
            goto out;
        }
    }

    /* create 'stranded-files' sub-directory */
    sprintf(path, "%s/%s/", root, stranded);
    ret = create_dir(path);
    if (ret != 0) {
        osd_error("!create_dir_stranded-files(%s)", path);
        goto out;
    }

    /* create 'md' sub-directory */
    sprintf(path, "%s/%s/", root, md);
    ret = create_dir(path);
    if (ret != 0) {
        osd_error("!create_dir_md(%s)", path);
        goto out;
    }

    osd->root = strdup(root);
    if (!osd->root) {
        osd_error("!strdup_root(%s)", root);
        ret = -ENOMEM;
        goto out;
    }

    osd->handle = malloc(sizeof(*osd->handle));

    /* auto-creates db if necessary, and sets osd->handle */
    get_dbname(path, root);
    ret = osd_db_open(path, osd);
    if (ret != 0 && ret != 1) {
        osd_error("!osd_db_open(%s)", path);
        goto out;
    }
    if (ret == 1) {
        ret = osd_initialize_db(osd);
        if (ret != 0) {
            osd_error("!osd_initialize_db");
            goto out;
        }
    }
    ret = db_exec_pragma(osd->handle->dbc);

out:
    return ret;
}


int osd_create_datafile(struct osd_device *osd, uint64_t pid,
        uint64_t oid)
{
    int ret = 0;
    char path[MAXNAMELEN];
    struct stat sb;


    get_dfile_name(path, osd->root, pid, oid);
    ret = stat(path, &sb);
    if (ret == 0 && S_ISREG(sb.st_mode)) {
        osd_debug("%s: path %s exists!", __func__, path);
        return -EEXIST;
    } else if (ret == -1 && errno == ENOENT) {
#ifdef __PANASAS_OSDSIM__
        char *smoog;
        smoog = strrchr(path, '/');
        *smoog = '\0';
        create_dir(path);
        osd_error("%s: panasas create %s directory %m", __func__,path);
        *smoog = '/';
#endif
        ret = creat(path, 0666);
        osd_debug("%s: path %s creat failed ret %d!", __func__, path, ret);
        if (ret <= 0)
            return ret;
        close(ret);
    } else {
        return ret;
    }

    return 0;
}


int format_osd(struct osd_device *osd, uint64_t capacity, uint32_t cdb_cont_len, uint8_t *sense)
{
    int ret;
    char *root = NULL;
    char path[MAXNAMELEN];
    struct stat sb;

    osd_debug("%s: capacity %llu MB", __func__, llu(capacity >> 20));

    assert(osd && osd->root && osd->handle && sense);

    root = strdup(osd->root);

    get_dbname(path, root);
    if (stat(path, &sb) != 0) {
        osd_error_errno("%s: DB %s does not exist, creating it",
                __func__, path);
        goto create;
    }

    sprintf(path, "%s/%s/", root, md);
    ret = empty_dir(path);
    if (ret) {
        osd_error("%s: empty_dir %s failed", __func__, path);
        goto out_sense;
    }

    sprintf(path, "%s/%s", root, stranded);
    ret = empty_dir(path);
    if (ret) {
        osd_error("%s: empty_dir %s failed", __func__, path);
        goto out_sense;
    }

    sprintf(path, "%s/%s", root, dfiles);
    ret = empty_dir(path);
    if (ret) {
        osd_error("%s: empty_dir %s failed", __func__, path);
        goto out_sense;
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
    int ret = 0;
    ret = db_begin_txn(osd->handle->dbc);
    return ret;
}

int osd_end_txn(struct osd_device *osd)
{
    int ret = 0;
    ret = db_end_txn(osd->handle->dbc);
    return ret;

}

int osd_close(struct osd_device *osd)
{
    int ret = 0;

    ret = osd_db_close(osd);
    if (ret != 0)
        osd_error("%s: osd_db_close", __func__);
    free(osd->root);
    osd->root = NULL;
    return ret;
}
