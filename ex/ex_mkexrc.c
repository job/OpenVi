/*      $OpenBSD: ex_mkexrc.c,v 1.7 2016/01/06 22:28:52 millert Exp $   */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1992, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *      Keith Bostic.  All rights reserved.
 * Copyright (c) 2022-2023 Jeffrey H. Johnson <trnsz@pobox.com>
 *
 * See the LICENSE.md file for redistribution information.
 */

#include "../include/compat.h"

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include <bitstring.h>
#include <errno.h>
#include <bsd_fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <bsd_stdlib.h>
#include <bsd_string.h>
#include <bsd_unistd.h>

#include "../common/common.h"
#include "pathnames.h"

#undef open

/*
 * ex_mkexrc -- :mkexrc[!] [file]
 *
 * Create (or overwrite) a .exrc file with the current info.
 *
 * PUBLIC: int ex_mkexrc(SCR *, EXCMD *);
 */
int
ex_mkexrc(SCR *sp, EXCMD *cmdp)
{
        struct stat sb;
        FILE *fp;
        int fd, sverrno;
        char *fname;

        switch (cmdp->argc) {
        case 0:
                fname = _PATH_EXRC;
                break;
        case 1:
                fname = cmdp->argv[0]->bp;
                set_alt_name(sp, fname);
                break;
        default:
                abort();
        }

        if (!FL_ISSET(cmdp->iflags, E_C_FORCE) && !stat(fname, &sb)) {
                msgq_str(sp, M_ERR, fname,
                    "%s exists, not written; use ! to override");
                return (1);
        }

        /* Create with max permissions of rw-r--r--. */
        if ((fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
                msgq_str(sp, M_SYSERR, fname, "%s");
                return (1);
        }

        if ((fp = fdopen(fd, "w")) == NULL) {
                sverrno = errno;
                (void)close(fd);
                goto e2;
        }

        if (seq_save(sp, fp, "abbreviate ", SEQ_ABBREV) || ferror(fp))
                goto e1;
        if (seq_save(sp, fp, "map ", SEQ_COMMAND) || ferror(fp))
                goto e1;
        if (seq_save(sp, fp, "map! ", SEQ_INPUT) || ferror(fp))
                goto e1;
        if (opts_save(sp, fp) || ferror(fp))
                goto e1;
        if (fclose(fp)) {
                sverrno = errno;
                goto e2;
        }

        msgq_str(sp, M_INFO, fname, "New exrc file: %s");
        return (0);

e1:     sverrno = errno;
        (void)fclose(fp);
e2:     errno = sverrno;
        msgq_str(sp, M_SYSERR, fname, "%s");
        return (1);
}
