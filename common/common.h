/*      $OpenBSD: common.h,v 1.10 2021/01/26 18:19:43 deraadt Exp $     */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1991, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1991, 1993, 1994, 1995, 1996
 *      Keith Bostic.  All rights reserved.
 * Copyright (c) 2022-2023 Jeffrey H. Johnson <trnsz@pobox.com>
 *
 * See the LICENSE.md file for redistribution information.
 *
 *      @(#)common.h    10.13 (Berkeley) 9/25/96
 */

#include "../include/compat.h"

#include <sys/time.h>
#include <bsd_db.h>
#include <bsd_regex.h>

/*
 * Forward structure declarations.  Not pretty, but the include files
 * are far too interrelated for a clean solution.
 */

typedef struct _cb              CB;
typedef struct _event           EVENT;
typedef struct _excmd           EXCMD;
typedef struct _exf             EXF;
typedef struct _fref            FREF;
typedef struct _gs              GS;
typedef struct _lmark           LMARK;
typedef struct _mark            MARK;
typedef struct _msg             MSGS;
typedef struct _option          OPTION;
typedef struct _optlist         OPTLIST;
typedef struct _scr             SCR;
typedef struct _script          SCRIPT;
typedef struct _seq             SEQ;
typedef struct _tag             TAG;
typedef struct _tagf            TAGF;
typedef struct _tagq            TAGQ;
typedef struct _text            TEXT;

/* Autoindent state. */
typedef enum { C_NOTSET, C_CARATSET, C_ZEROSET } carat_t;

/* Busy message types. */
typedef enum { BUSY_ON = 1, BUSY_OFF, BUSY_UPDATE } busy_t;

/*
 * Routines that return a confirmation return:
 *
 *      CONF_NO         User answered no.
 *      CONF_QUIT       User answered quit, eof or an error.
 *      CONF_YES        User answered yes.
 */

typedef enum { CONF_NO, CONF_QUIT, CONF_YES } conf_t;

/* Directions. */
typedef enum { NOTSET, FORWARD, BACKWARD } dir_t;

/* Line operations. */
typedef enum { LINE_APPEND, LINE_DELETE, LINE_INSERT, LINE_RESET } lnop_t;

/* Lock return values. */
typedef enum { LOCK_FAILED, LOCK_SUCCESS, LOCK_UNAVAIL } lockr_t;

/* Sequence types. */
typedef enum { SEQ_ABBREV, SEQ_COMMAND, SEQ_INPUT } seq_t;

/* Program modes. */
extern enum pmode { MODE_EX, MODE_VI, MODE_VIEW } pmode;

/*
 * Extensions to POSIX.
 */

#ifndef ACCESSPERMS
# define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO)
#endif /* ifndef ACCESSPERMS */

#ifndef ALLPERMS
# define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif /* ifndef ALLPERMS */

/*
 * Local includes.
 */

#include "key.h"                /* Required by args.h. */
#include "args.h"               /* Required by options.h. */
#include "options.h"            /* Required by screen.h. */
#include "msg.h"                /* Required by gs.h. */
#include "cut.h"                /* Required by gs.h. */
#include "seq.h"                /* Required by screen.h. */
#include "util.h"               /* Required by ex.h. */
#include "mark.h"               /* Required by gs.h. */
#include "../ex/ex.h"           /* Required by gs.h. */
#include "gs.h"                 /* Required by screen.h. */
#include "screen.h"             /* Required by exf.h. */
#include "exf.h"
#include "log.h"
#include "mem.h"

#include "com_extern.h"
