/*      $OpenBSD: ex_mark.c,v 1.7 2016/01/06 22:28:52 millert Exp $     */

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
 * ex_mark -- :mark char
 *            :k char
 *      Mark lines.
 *
 *
 * PUBLIC: int ex_mark(SCR *, EXCMD *);
 */
int
ex_mark(SCR *sp, EXCMD *cmdp)
{
        NEEDFILE(sp, cmdp);

        if (cmdp->argv[0]->len != 1) {
                msgq(sp, M_ERR, "Mark names must be a single character");
                return (1);
        }
        return (mark_set(sp, cmdp->argv[0]->bp[0], &cmdp->addr1, 1));
}
