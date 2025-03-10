/*      $OpenBSD: db.c,v 1.13 2015/09/05 11:28:35 guenther Exp $        */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2022-2023 Jeffrey H. Johnson <trnsz@pobox.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../../include/compat.h"

#include <sys/file.h>
#include <sys/types.h>

#include <errno.h>
#include <bsd_fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <bsd_unistd.h>

#include <bsd_db.h>
#include <compat_bsd_db.h>

#undef open

static int __dberr(void);

DB *
dbopen(const char *fname, int flags, int mode, DBTYPE type,
    const void *openinfo)
{

#define DB_FLAGS        (DB_LOCK | DB_SHMEM | DB_TXN)
#define USE_OPEN_FLAGS                                                  \
        (O_CREAT | O_EXCL | O_EXLOCK | O_NOFOLLOW | O_NONBLOCK |        \
         O_ACCMODE | O_SHLOCK | O_SYNC | O_TRUNC)

        if (((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
            && (flags & ~(USE_OPEN_FLAGS | DB_FLAGS)) == 0)
                switch (type) {
                case DB_BTREE:
                        return (__bt_open(fname, flags & USE_OPEN_FLAGS,
                            mode, openinfo, flags & DB_FLAGS));
                case DB_HASH:
                        return (__hash_open(fname, flags & USE_OPEN_FLAGS,
                            mode, openinfo, flags & DB_FLAGS));
                case DB_RECNO:
                        return (__rec_open(fname, flags & USE_OPEN_FLAGS,
                            mode, openinfo, flags & DB_FLAGS));
                }
        errno = EINVAL;
        return (NULL);
}
DEF_WEAK(dbopen);

static int
__dberr(void)
{
        return (RET_ERROR);
}

/*
 * __DBPANIC -- Stop.
 *
 * Parameters:
 *      dbp:    pointer to the DB structure.
 */

void
__dbpanic(DB *dbp)
{
        /* The only thing that can succeed is a close. */
        dbp->del  = (int (*)(const struct __db *, const DBT*, unsigned int))__dberr;
        dbp->fd   = (int (*)(const struct __db *))__dberr;
        dbp->get  = (int (*)(const struct __db *, const DBT*, DBT *, unsigned int))__dberr;
        dbp->put  = (int (*)(const struct __db *, DBT *, const DBT *, unsigned int))__dberr;
        dbp->seq  = (int (*)(const struct __db *, DBT *, DBT *, unsigned int))__dberr;
        dbp->sync = (int (*)(const struct __db *, unsigned int))__dberr;
}
