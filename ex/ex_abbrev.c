/*      $OpenBSD: ex_abbrev.c,v 1.9 2016/05/27 09:18:12 martijn Exp $   */

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
 * ex_abbr -- :abbreviate [key replacement]
 *      Create an abbreviation or display abbreviations.
 *
 * PUBLIC: int ex_abbr(SCR *, EXCMD *);
 */
int
ex_abbr(SCR *sp, EXCMD *cmdp)
{
        CHAR_T *p;
        size_t len;

        switch (cmdp->argc) {
        case 0:
                if (seq_dump(sp, SEQ_ABBREV, 0) == 0)
                        msgq(sp, M_INFO, "No abbreviations to display");
                return (0);
        case 2:
                break;
        default:
                abort();
        }

        /*
         * Check for illegal characters.
         *
         * !!!
         * Another fun one, historically.  See vi/v_ntext.c:txt_abbrev() for
         * details.  The bottom line is that all abbreviations have to end
         * with a "word" character, because it's the transition from word to
         * non-word characters that triggers the test for an abbreviation.  In
         * addition, because of the way the test is done, there can't be any
         * transitions from word to non-word character (or vice-versa) other
         * than between the next-to-last and last characters of the string,
         * and there can't be any <blank> characters.  Warn the user.
         */
        if (!inword(cmdp->argv[0]->bp[cmdp->argv[0]->len - 1])) {
                msgq(sp, M_ERR,
                    "Abbreviations must end with a \"word\" character");
                        return (1);
        }
        for (p = cmdp->argv[0]->bp; *p != '\0'; ++p)
                if (isblank(p[0])) {
                        msgq(sp, M_ERR,
                            "Abbreviations may not contain tabs or spaces");
                        return (1);
                }
        if (cmdp->argv[0]->len > 2)
                for (p = cmdp->argv[0]->bp,
                    len = cmdp->argv[0]->len - 2; len; --len, ++p)
                        if (inword(p[0]) != inword(p[1])) {
                                msgq(sp, M_ERR,
"Abbreviations may not mix word/non-word characters, except at the end");
                                return (1);
                        }

        if (seq_set(sp, NULL, 0, cmdp->argv[0]->bp, cmdp->argv[0]->len,
            cmdp->argv[1]->bp, cmdp->argv[1]->len, SEQ_ABBREV, SEQ_USERDEF))
                return (1);

        F_SET(sp->gp, G_ABBREV);
        return (0);
}

/*
 * ex_unabbr -- :unabbreviate key
 *      Delete an abbreviation.
 *
 * PUBLIC: int ex_unabbr(SCR *, EXCMD *);
 */
int
ex_unabbr(SCR *sp, EXCMD *cmdp)
{
        ARGS *ap;

        ap = cmdp->argv[0];
        if (!F_ISSET(sp->gp, G_ABBREV) ||
            seq_delete(sp, ap->bp, ap->len, SEQ_ABBREV)) {
                msgq_str(sp, M_ERR, ap->bp,
                    "\"%s\" is not an abbreviation");
                return (1);
        }
        return (0);
}
