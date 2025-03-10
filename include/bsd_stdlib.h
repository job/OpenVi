/*      $OpenBSD: stdlib.h,v 1.67 2016/09/20 21:10:22 fcambus Exp $     */
/*      $NetBSD:  stdlib.h,v 1.25 1995/12/27 21:19:08 jtc Exp $         */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1990 The Regents of the University of California.
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
 *
 *      @(#)stdlib.h    5.13 (Berkeley) 6/4/91
 */

#ifndef _COMPAT_STDLIB_H_
# define _COMPAT_STDLIB_H_

# ifdef __solaris__
#  undef _TIMESPEC_UTIL_H
#  define _TIMESPEC_UTIL_H 1
# endif /* ifdef __solaris__ */

# include <sys/types.h>
# include <stdint.h>

extern char  *__progname;
const  char  *bsd_getprogname(void);

void *openbsd_reallocarray(void *, size_t, size_t);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
long long strtonum(const char *, long long, long long, const char **);

#endif /* _COMPAT_STDLIB_H_ */

#include_next <stdlib.h>
