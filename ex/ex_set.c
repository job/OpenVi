/*      $OpenBSD: ex_set.c,v 1.6 2014/11/12 04:28:41 bentley Exp $      */

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
 * ex_set -- :set
 *      Ex set option.
 *
 * PUBLIC: int ex_set(SCR *, EXCMD *);
 */
int
ex_set(SCR *sp, EXCMD *cmdp)
{
        switch(cmdp->argc) {
        case 0:
                opts_dump(sp, CHANGED_DISPLAY);
                break;
        default:
                if (opts_set(sp, cmdp->argv, cmdp->cmd->usage))
                        return (1);
                break;
        }
        return (0);
}
