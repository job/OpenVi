/*      $OpenBSD: ex_display.c,v 1.13 2016/05/27 09:18:12 martijn Exp $ */

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
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <bsd_string.h>

#include "../common/common.h"
#include "tag.h"

static int      bdisplay(SCR *);
static void     db(SCR *, CB *, CHAR_T *);

/*
 * ex_display -- :display b[uffers] | s[creens] | t[ags]
 *
 *      Display buffers, tags or screens.
 *
 * PUBLIC: int ex_display(SCR *, EXCMD *);
 */
int
ex_display(SCR *sp, EXCMD *cmdp)
{
        switch (cmdp->argv[0]->bp[0]) {
        case 'b':
#undef  ARG
#define ARG     "buffers"
                if (cmdp->argv[0]->len >= sizeof(ARG) ||
                    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
                        break;
                return (bdisplay(sp));
        case 's':
#undef  ARG
#define ARG     "screens"
                if (cmdp->argv[0]->len >= sizeof(ARG) ||
                    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
                        break;
                return (ex_sdisplay(sp));
        case 't':
#undef  ARG
#define ARG     "tags"
                if (cmdp->argv[0]->len >= sizeof(ARG) ||
                    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
                        break;
                return (ex_tag_display(sp));
        }
        ex_emsg(sp, cmdp->cmd->usage, EXM_USAGE);
        return (1);
}

/*
 * bdisplay --
 *
 *      Display buffers.
 */
static int
bdisplay(SCR *sp)
{
        CB *cbp;

        if (LIST_FIRST(&sp->gp->cutq) == NULL && sp->gp->dcbp == NULL) {
                msgq(sp, M_INFO, "No cut buffers to display");
                return (0);
        }

        /* Display regular cut buffers. */
        LIST_FOREACH(cbp, &sp->gp->cutq, q) {
                if (isdigit(cbp->name))
                        continue;
                if (!TAILQ_EMPTY(&cbp->textq))
                        db(sp, cbp, NULL);
                if (INTERRUPTED(sp))
                        return (0);
        }
        /* Display numbered buffers. */
        LIST_FOREACH(cbp, &sp->gp->cutq, q) {
                if (!isdigit(cbp->name))
                        continue;
                if (!TAILQ_EMPTY(&cbp->textq))
                        db(sp, cbp, NULL);
                if (INTERRUPTED(sp))
                        return (0);
        }
        /* Display default buffer. */
        if ((cbp = sp->gp->dcbp) != NULL)
                db(sp, cbp, "default buffer");
        return (0);
}

/*
 * db --
 *      Display a buffer.
 */
static void
db(SCR *sp, CB *cbp, CHAR_T *name)
{
        CHAR_T *p;
        TEXT *tp;
        size_t len;

        (void)ex_printf(sp, "********** %s%s\n",
            name == NULL ? KEY_NAME(sp, cbp->name) : name,
            F_ISSET(cbp, CB_LMODE) ? " (line mode)" : " (character mode)");
        TAILQ_FOREACH(tp, &cbp->textq, q) {
                for (len = tp->len, p = tp->lb; len--; ++p) {
                        (void)ex_puts(sp, KEY_NAME(sp, *p));
                        if (INTERRUPTED(sp))
                                return;
                }
                (void)ex_puts(sp, "\n");
        }
}
