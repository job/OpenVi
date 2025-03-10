/*      $OpenBSD: ex_usage.c,v 1.10 2018/07/13 09:02:07 krw Exp $       */

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
#include <sys/time.h>

#include <bitstring.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <bsd_stdlib.h>
#include <bsd_string.h>

#include "../common/common.h"
#include "../vi/vi.h"

/*
 * ex_help -- :help
 *      Display help message.
 *
 * PUBLIC: int ex_help(SCR *, EXCMD *);
 */
int
ex_help(SCR *sp, EXCMD *cmdp)
{
        (void)ex_puts(sp,
            "To see the list of vi commands, enter \":viusage<CR>\"\n");
        (void)ex_puts(sp,
            "To see the list of ex commands, enter \":exusage<CR>\"\n");
        (void)ex_puts(sp,
            "For an ex command usage statement enter \":exusage [cmd]<CR>\"\n");
        (void)ex_puts(sp,
            "For a vi key usage statement enter \":viusage [key]<CR>\"\n");
        (void)ex_puts(sp, "To exit, enter \":q!\"\n");
        return (0);
}

/*
 * ex_usage -- :exusage [cmd]
 *      Display ex usage strings.
 *
 * PUBLIC: int ex_usage(SCR *, EXCMD *);
 */
int
ex_usage(SCR *sp, EXCMD *cmdp)
{
        ARGS *ap;
        EXCMDLIST const *cp;
        int newscreen;

        switch (cmdp->argc) {
        case 1:
                ap = cmdp->argv[0];
                if (isupper(ap->bp[0])) {
                        newscreen = 1;
                        ap->bp[0] = tolower(ap->bp[0]);
                } else
                        newscreen = 0;
                for (cp = cmds; cp->name != NULL &&
                    memcmp(ap->bp, cp->name, ap->len); ++cp);
                if (cp->name == NULL ||
                    (newscreen && !F_ISSET(cp, E_NEWSCREEN))) {
                        if (newscreen)
                                ap->bp[0] = toupper(ap->bp[0]);
                        (void)ex_printf(sp, "The %.*s command is unknown\n",
                            (int)ap->len, ap->bp);
                } else {
                        (void)ex_printf(sp,
                            "Command: %s\n  Usage: %s\n", cp->help, cp->usage);
                        /*
                         * !!!
                         * The "visual" command has two modes, one from ex,
                         * one from the vi colon line.  Don't ask.
                         */
                        if (cp != &cmds[C_VISUAL_EX] &&
                            cp != &cmds[C_VISUAL_VI])
                                break;
                        if (cp == &cmds[C_VISUAL_EX])
                                cp = &cmds[C_VISUAL_VI];
                        else
                                cp = &cmds[C_VISUAL_EX];
                        (void)ex_printf(sp,
                            "Command: %s\n  Usage: %s\n", cp->help, cp->usage);
                }
                break;
        case 0:
                for (cp = cmds; cp->name != NULL && !INTERRUPTED(sp); ++cp)
                        (void)ex_printf(sp, "%*s: %s\n", MAXCMDNAMELEN,
                            /* The ^D command has an unprintable name. */
                            cp == &cmds[C_SCROLL] ? "^D" : cp->name,
                            cp->help);
                break;
        default:
                abort();
        }
        return (0);
}

/*
 * ex_viusage -- :viusage [key]
 *      Display vi usage strings.
 *
 * PUBLIC: int ex_viusage(SCR *, EXCMD *);
 */
int
ex_viusage(SCR *sp, EXCMD *cmdp)
{
        VIKEYS const *kp;
        int key;

        switch (cmdp->argc) {
        case 1:
                if (cmdp->argv[0]->len != 1) {
                        ex_emsg(sp, cmdp->cmd->usage, EXM_USAGE);
                        return (1);
                }
                key = cmdp->argv[0]->bp[0];
                if (key > MAXVIKEY)
                        goto nokey;

                /* Special case: '[' and ']' commands. */
                if ((key == '[' || key == ']') && cmdp->argv[0]->bp[1] != key)
                        goto nokey;

                /* Special case: ~ command. */
                if (key == '~' && O_ISSET(sp, O_TILDEOP))
                        kp = &tmotion;
                else
                        kp = &vikeys[key];

                if (kp->usage == NULL)
nokey:                  (void)ex_printf(sp,
                            "The %s key has no current meaning\n",
                            KEY_NAME(sp, key));
                else
                        (void)ex_printf(sp,
                            "  Key:%s%s\nUsage: %s\n",
                            isblank(*kp->help) ? "" : " ", kp->help, kp->usage);
                break;
        case 0:
                for (key = 0; key <= MAXVIKEY && !INTERRUPTED(sp); ++key) {
                        /* Special case: ~ command. */
                        if (key == '~' && O_ISSET(sp, O_TILDEOP))
                                kp = &tmotion;
                        else
                                kp = &vikeys[key];
                        if (kp->help != NULL)
                                (void)ex_printf(sp, "%s\n", kp->help);
                }
                break;
        default:
                abort();
        }
        return (0);
}
