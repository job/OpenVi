/*      $OpenBSD: pwcache.c,v 1.16 2022/12/27 17:10:06 jmc Exp $         */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1992 Keith Muller
 * Copyright (c) 1992, 1993 The Regents of the University of California
 * Copyright (c) 2022-2023 Jeffrey H. Johnson <trnsz@pobox.com>
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
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

#include "../include/compat.h"

#include <sys/types.h>
#include <assert.h>

#if defined(__illumos__) && !defined(_POSIX_PTHREAD_SEMANTICS)
# define _POSIX_PTHREAD_SEMANTICS
#endif /* if defined(__illumos__) && !defined(_POSIX_PTHREAD_SEMANTICS) */


#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <bsd_stdlib.h>
#include <bsd_string.h>
#include <bsd_unistd.h>

/*
 * Constants and data structures used to implement group and password file
 * caches.  Name lengths have been chosen to be as large as those supported
 * by the passwd and group files as well as the standard archive formats.
 * CACHE SIZES MUST BE PRIME
 */
#define UNMLEN  32    /* >= user name found in any protocol  */
#define GNMLEN  32    /* >= group name found in any protocol */
#define UID_SZ 317    /* size of uid to user_name cache      */
#define UNM_SZ 317    /* size of user_name to uid cache      */
#define GID_SZ 251    /* size of gid to group_name cache     */
#define GNM_SZ 251    /* size of group_name to gid cache     */
#define VALID    1    /* entry and name are valid            */
#define INVALID  2    /* entry valid, name NOT valid         */

/*
 * Node structures used in the user, group, uid, and gid caches.
 */

typedef struct uidc
{
  int valid;         /* is this a valid or a miss entry */
  char name[UNMLEN]; /* uid name */
  uid_t uid;         /* cached uid */
} UIDC;

typedef struct gidc
{
  int valid;         /* is this a valid or a miss entry */
  char name[GNMLEN]; /* gid name */
  gid_t gid;         /* cached gid */
} GIDC;

/*
 * Routines that control user, group, uid and gid caches.
 * Traditional passwd/group cache routines perform quite poorly with
 * archives. The chances of hitting a valid lookup with an archive is quite a
 * bit worse than with files already resident on the file system. These misses
 * create a MAJOR performance cost. To address this problem, these routines
 * cache both hits and misses.
 */

static UIDC **usrtb; /* user name to uid cache */
static GIDC **grptb; /* group name to gid cache */

static unsigned int
st_hash(const char *name, size_t len, int tabsz)
{
  unsigned int key = 0;

  assert(name != NULL);

  while (len--)
    {
      key += *name++;
      key = ( key << 8 ) | ( key >> 24 );
    }

  return key % tabsz;
}

/*
 * usrtb_start
 *      creates an an empty usrtb
 * Return:
 *      0 if ok, -1 otherwise
 */
static int
usrtb_start(void)
{
  static int fail = 0;

  if (usrtb != NULL)
    {
      return 0;
    }

  if (fail)
    {
      return -1;
    }

  if (( usrtb = calloc(UNM_SZ, sizeof ( UIDC * ))) == NULL)
    {
      ++fail;
      return -1;
    }

  return 0;
}

/*
 * grptb_start
 *      creates an an empty grptb
 * Return:
 *      0 if ok, -1 otherwise
 */
static int
grptb_start(void)
{
  static int fail = 0;

  if (grptb != NULL)
    {
      return 0;
    }

  if (fail)
    {
      return -1;
    }

  if (( grptb = calloc(GNM_SZ, sizeof ( GIDC * ))) == NULL)
    {
      ++fail;
      return -1;
    }

  return 0;
}

/*
 * uid_from_user()
 *      caches the uid for a given user name. We use a simple hash table.
 * Return:
 *      0 if the user name is found (filling in uid), -1 otherwise
 */
int
openbsd_uid_from_user(const char *name, uid_t *uid)
{
  struct passwd pwstore, *pw = NULL;
  char pwbuf[_PW_BUF_LEN];
  UIDC **pptr, *ptr = NULL;
  size_t namelen;

  /*
   * return -1 for mangled names
   */
  if (name == NULL || (( namelen = strlen(name)) == 0 ))
    {
      return -1;
    }

  if (( usrtb != NULL ) || ( usrtb_start() == 0 ))
    {
      /*
       * look up in hash table, if found and valid return the uid,
       * if found and invalid, return a -1
       */
      pptr = usrtb + st_hash(name, namelen, UNM_SZ);
      ptr = *pptr;

      if (( ptr != NULL ) && ( ptr->valid > 0 ) && strcmp(name, ptr->name) == 0)
        {
          if (ptr->valid == INVALID)
            {
              return -1;
            }

          *uid = ptr->uid;
          return 0;
        }

      if (ptr == NULL)
        {
          *pptr = ptr = malloc(sizeof ( UIDC ));
        }
    }

  /*
   * no match, look it up, if no match store it as an invalid entry,
   * or store the matching uid
   */
  getpwnam_r(name, &pwstore, pwbuf, sizeof ( pwbuf ), &pw);
  if (ptr == NULL)
    {
      if (pw == NULL)
        {
          return -1;
        }

      *uid = pw->pw_uid;
      return 0;
    }

  (void)openbsd_strlcpy(ptr->name, name, sizeof ( ptr->name ));
  if (pw == NULL)
    {
      ptr->valid = INVALID;
      return -1;
    }

  ptr->valid = VALID;
  *uid = ptr->uid = pw->pw_uid;
  return 0;
}

/*
 * gid_from_group()
 *      caches the gid for a given group name. We use a simple hash table.
 * Return:
 *      0 if the group name is found (filling in gid), -1 otherwise
 */
int
openbsd_gid_from_group(const char *name, gid_t *gid)
{
  struct group grstore, *gr = NULL;
  char grbuf[_GR_BUF_LEN];
  GIDC **pptr, *ptr = NULL;
  size_t namelen;

  /*
   * return -1 for mangled names
   */
  if (name == NULL || (( namelen = strlen(name)) == 0 ))
    {
      return -1;
    }

  if (( grptb != NULL ) || ( grptb_start() == 0 ))
    {
      /*
       * look up in hash table, if found and valid return the uid,
       * if found and invalid, return a -1
       */
      pptr = grptb + st_hash(name, namelen, GID_SZ);
      ptr = *pptr;

      if (( ptr != NULL ) && ( ptr->valid > 0 ) && strcmp(name, ptr->name) == 0)
        {
          if (ptr->valid == INVALID)
            {
              return -1;
            }

          *gid = ptr->gid;
          return 0;
        }

      if (ptr == NULL)
        {
          *pptr = ptr = malloc(sizeof ( GIDC ));
        }
    }

  /*
   * no match, look it up, if no match store it as an invalid entry,
   * or store the matching gid
   */
  getgrnam_r(name, &grstore, grbuf, sizeof ( grbuf ), &gr);
  if (ptr == NULL)
    {
      if (gr == NULL)
        {
          return -1;
        }

      *gid = gr->gr_gid;
      return 0;
    }

  (void)openbsd_strlcpy(ptr->name, name, sizeof ( ptr->name ));
  if (gr == NULL)
    {
      ptr->valid = INVALID;
      return -1;
    }

  ptr->valid = VALID;
  *gid = ptr->gid = gr->gr_gid;
  return 0;
}
