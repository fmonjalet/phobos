/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
/*
 * Copyright 2014-2015 CEA/DAM. All Rights Reserved.
 */
/**
 * \brief  Phobos Local Device Manager.
 *
 * This modules implements low level device control on local host.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pho_ldm.h"
#include "pho_cfg.h"
#include "pho_type_utils.h"
#include "pho_common.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

/**
 * Build a command to query drive info.
 * The result must be released by the caller using free(3).
 */
static char *drive_query_cmd(enum dev_family type, const char *device)
{
    const char *cmd_cfg = NULL;
    char *cmd_out;

    if (pho_cfg_get(PHO_CFG_LDM_cmd_drive_query, &cmd_cfg))
        return NULL;

    if (asprintf(&cmd_out, cmd_cfg, device) < 0)
        return NULL;

    return cmd_out;
}

/**
 * Build a command to load a drive.
 * The result must be released by the caller using free(3).
 */
static char *drive_load_cmd(enum dev_family type, const char *device,
                            const char *media_id)
{
    const char *cmd_cfg = NULL;
    char *cmd_out;

    if (pho_cfg_get(PHO_CFG_LDM_cmd_drive_load, &cmd_cfg))
        return NULL;

    if (asprintf(&cmd_out, cmd_cfg, device, media_id) < 0)
        return NULL;

    return cmd_out;
}

/**
 * Build a command to unload a drive.
 * The result must be released by the caller using free(3).
 */
static char *drive_unload_cmd(enum dev_family type, const char *device,
                              const char *media_id)
{
    const char *cmd_cfg = NULL;
    char *cmd_out;

    if (pho_cfg_get(PHO_CFG_LDM_cmd_drive_unload, &cmd_cfg))
        return NULL;

    if (asprintf(&cmd_out, cmd_cfg, device, media_id) < 0)
        return NULL;

    return cmd_out;
}

/**
 * Build a command to mount a filesystem at a given path.
 * The result must be released by the caller using free(3).
 */
static char *mount_cmd(enum fs_type fs, const char *device, const char *path)
{
    const char *cmd_cfg = NULL;
    char *cmd_out;
    enum pho_cfg_params param;

    if (fs == PHO_FS_LTFS)
        param = PHO_CFG_LDM_cmd_mount_ltfs;
    else
        /* not supported */
        return NULL;

    if (pho_cfg_get(param, &cmd_cfg))
        return NULL;

    if (asprintf(&cmd_out, cmd_cfg, device, path) < 0)
        return NULL;

    return cmd_out;
}

/**
 * Build a command to unmount a filesystem at a given path.
 * The result must be released by the caller using free(3).
 */
static char *umount_cmd(enum fs_type fs, const char *device, const char *path)
{
    const char *cmd_cfg = NULL;
    char *cmd_out;
    enum pho_cfg_params param;

    if (fs == PHO_FS_LTFS)
        param = PHO_CFG_LDM_cmd_umount_ltfs;
    else
        /* not supported */
        return NULL;

    if (pho_cfg_get(param, &cmd_cfg))
        return NULL;

    if (asprintf(&cmd_out, cmd_cfg, device, path) < 0)
        return NULL;

    return cmd_out;
}

/** concatenate a command output */
static int collect_output(void *cb_arg, char *line, size_t size)
{
    g_string_append_len((GString *)cb_arg, line, size);
    return 0;
}

/**
 * Try to determine the type of device.
 */
static int guess_dev_type(const char *path, enum dev_family *dev_type)
{
    return -ENOTSUP;
}

int ldm_device_query(enum dev_family dev_type, const char *dev_path,
                     struct dev_state *dev_st)
{
    char    *cmd = NULL;
    GString *cmd_out = NULL;
    int      rc;

    if (dev_path == NULL || dev_st == NULL)
        LOG_RETURN(-EINVAL, "Unexpected NULL argument");

    if (dev_type == PHO_DEV_UNSPEC) {
        rc = guess_dev_type(dev_path, &dev_type);
        if (rc)
            return rc;
    }

    cmd = drive_query_cmd(dev_type, dev_path);
    if (!cmd)
        LOG_GOTO(out, rc = -ENOMEM, "Failed to build drive info command");

    cmd_out = g_string_new("");

    /* @TODO skip a step by reading JSON directly from a stream */

    /* retrieve physical device state */
    rc = command_call(cmd, collect_output, cmd_out);
    if (rc)
        LOG_GOTO(out, rc, "Query command failed: '%s'", cmd);

    /* parse command output */
    rc = device_state_from_json(cmd_out->str, dev_st);

out:
    free(cmd);
    if (cmd_out != NULL)
        g_string_free(cmd_out, TRUE);
    return rc;
}

int ldm_device_load(enum dev_family dev_type, const char *dev_path,
                    const struct media_id *media_id)
{
    char    *cmd = NULL;
    GString *cmd_out = NULL;
    int      rc;

    if (dev_path == NULL || media_id == NULL)
        LOG_RETURN(-EINVAL, "Unexpected NULL argument");

    if (dev_type == PHO_DEV_UNSPEC) {
        rc = guess_dev_type(dev_path, &dev_type);
        if (rc)
            return rc;
    }

    if (dev_type != PHO_DEV_TAPE)
        LOG_RETURN(-ENOTSUP, "Cannot load device of type '%s'",
                   dev_family2str(dev_type));

    /* get load command for the given device type */
    cmd = drive_load_cmd(dev_type, dev_path, media_id_get(media_id));
    if (!cmd)
        LOG_RETURN(-ENOMEM, "Failed to build drive load command");

    cmd_out = g_string_new("");

    rc = command_call(cmd, collect_output, cmd_out);
    if (rc)
        pho_error(rc, "Load command failed: '%s'", cmd);

    free(cmd);
    if (cmd_out != NULL)
        g_string_free(cmd_out, TRUE);
    return rc;
}

int ldm_device_unload(enum dev_family dev_type, const char *dev_path,
                      const struct media_id *media_id)
{
    char    *cmd = NULL;
    GString *cmd_out = NULL;
    int      rc;

    if (dev_path == NULL || media_id == NULL)
        LOG_RETURN(-EINVAL, "Unexpected NULL argument");

    if (dev_type == PHO_DEV_UNSPEC) {
        rc = guess_dev_type(dev_path, &dev_type);
        if (rc)
            return rc;
    }

    if (dev_type != PHO_DEV_TAPE)
        LOG_RETURN(-ENOTSUP, "Cannot unload device of type '%s'",
                   dev_family2str(dev_type));

    /* get unload command for the given device type */
    cmd = drive_unload_cmd(dev_type, dev_path, media_id_get(media_id));
    if (!cmd)
        LOG_RETURN(-ENOMEM, "Failed to build drive unload command");

    cmd_out = g_string_new("");

    rc = command_call(cmd, collect_output, cmd_out);
    if (rc)
        pho_error(rc, "Unload command failed: '%s'", cmd);

    free(cmd);
    if (cmd_out != NULL)
        g_string_free(cmd_out, TRUE);
    return rc;
}

int ldm_fs_mount(enum fs_type fs, const char *dev_path, const char *mnt_point)
{
    char    *cmd = NULL;
    GString *cmd_out = NULL;
    int      rc;

    if (fs == PHO_FS_POSIX)
        /* the "device" is accessible as is and doesn't need to be mounted
         * (e.g. directory) */
        return 0;

    cmd = mount_cmd(fs, dev_path, mnt_point);
    if (!cmd)
        LOG_GOTO(out_free, rc = -ENOMEM, "Failed to build mount command");

    cmd_out = g_string_new("");

    /* create the mount point */
    if (mkdir(mnt_point, 640) != 0 && errno != EEXIST)
        LOG_GOTO(out_free, rc = -errno, "Failed to create mount point %s",
                 mnt_point);

    /* mount the filesystem */
    rc = command_call(cmd, collect_output, cmd_out);
    if (rc)
        LOG_GOTO(out_free, rc, "Mount command failed: '%s'", cmd);

out_free:
    free(cmd);
    if (cmd_out != NULL)
        g_string_free(cmd_out, TRUE);
    return rc;
}


int ldm_fs_umount(enum fs_type fs, const char *dev_path, const char *mnt_point)
{
    char    *cmd = NULL;
    GString *cmd_out = NULL;
    int      rc;

    if (fs == PHO_FS_POSIX)
        /* it is always accessible and can't be unmounted
         * (e.g. directory) */
        return 0;

    cmd = umount_cmd(fs, dev_path, mnt_point);
    if (!cmd)
        LOG_GOTO(out_free, rc = -ENOMEM, "Failed to build umount command");

    cmd_out = g_string_new("");

    /* unmount the filesystem */
    rc = command_call(cmd, collect_output, cmd_out);
    if (rc)
        LOG_GOTO(out_free, rc, "Unmount command failed: '%s'", cmd);

out_free:
    free(cmd);
    if (cmd_out != NULL)
        g_string_free(cmd_out, TRUE);
    return rc;
}