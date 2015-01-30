/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
/*
 * Copyright 2014-2015 CEA/DAM. All Rights Reserved.
 */
/**
 * \brief test object store
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "phobos_store.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int rc;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s put <file>\n", argv[0]);
        fprintf(stderr, "       %s get <id> <dest>\n", argv[0]);
        exit(1);
    }

    if (!strcmp(argv[1], "put")) {
        struct pho_attrs attrs = {0};
        char fullp[PATH_MAX];

        rc = pho_attr_set(&attrs, "program", argv[0]);
        if (rc)
            return rc;
        rc = phobos_put(realpath(argv[2], fullp), argv[2], 0, &attrs);

        pho_attrs_free(&attrs);
    } else if (!strcmp(argv[1], "get")) {
        rc = phobos_get(argv[2], argv[3], 0);
    } else {
        fprintf(stderr, "verb put|get expected at '%s'\n", argv[1]);
        rc = -EINVAL;
    }
    return rc;
}