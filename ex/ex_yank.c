/*      $OpenBSD: ex_yank.c,v 1.6 2014/11/12 04:28:41 bentley Exp $     */

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

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"

/*
 * ex_yank -- :[line [,line]] ya[nk] [buffer] [count]
 *      Yank the lines into a buffer.
 *
 * PUBLIC: int ex_yank(SCR *, EXCMD *);
 */
int
ex_yank(SCR *sp, EXCMD *cmdp)
{
        NEEDFILE(sp, cmdp);

        /*
         * !!!
         * Historically, yanking lines in ex didn't count toward the
         * number-of-lines-yanked report.
         */
        return (cut(sp,
            FL_ISSET(cmdp->iflags, E_C_BUFFER) ? &cmdp->buffer : NULL,
            &cmdp->addr1, &cmdp->addr2, CUT_LINEMODE));
}
