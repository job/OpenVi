/*      $OpenBSD: key.c,v 1.19 2022/04/21 17:50:50 millert Exp $        */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 1991, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1991, 1993, 1994, 1995, 1996
 *      Keith Bostic.  All rights reserved.
 * Copyright (c) 2022-2023 Jeffrey H. Johnson <trnsz@pobox.com>
 *
 * See the LICENSE.md file for redistribution information.
 */

#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <bsd_stdlib.h>
#include <bsd_string.h>
#include <bsd_unistd.h>

#include "common.h"
#include "../vi/vi.h"

#define MAXIMUM(a, b)   (((a) > (b)) ? (a) : (b))

static int      v_event_append(SCR *, EVENT *);
static int      v_event_grow(SCR *, int);
static int      v_key_cmp(const void *, const void *);
static void     v_keyval(SCR *, int, scr_keyval_t);
static void     v_sync(SCR *, int);

/*
 * !!!
 * Historic vi always used:
 *
 *      ^D: autoindent deletion
 *      ^H: last character deletion
 *      ^W: last word deletion
 *      ^Q: quote the next character (if not used in flow control).
 *      ^V: quote the next character
 *
 * regardless of the user's choices for these characters.  The user's erase
 * and kill characters worked in addition to these characters.  Nvi wires
 * down the above characters, but in addition permits the VEOF, VERASE, VKILL
 * and VWERASE characters described by the user's termios structure.
 *
 * Ex was not consistent with this scheme, as it historically ran in tty
 * cooked mode.  This meant that the scroll command and autoindent erase
 * characters were mapped to the user's EOF character, and the character
 * and word deletion characters were the user's tty character and word
 * deletion characters.  This implementation makes it all consistent, as
 * described above for vi.
 *
 * !!!
 * This means that all screens share a special key set.
 */
KEYLIST keylist[] = {
        {K_BACKSLASH,     '\\'},        /*  \ */
        {K_CARAT,          '^'},        /*  ^ */
        {K_CNTRLD,      '\004'},        /* ^D */
        {K_CNTRLR,      '\022'},        /* ^R */
        {K_CNTRLT,      '\024'},        /* ^T */
        {K_CNTRLZ,      '\032'},        /* ^Z */
        {K_COLON,          ':'},        /*  : */
        {K_CR,            '\r'},        /* \r */
        {K_ESCAPE,      '\033'},        /* ^[ */
        {K_FORMFEED,      '\f'},        /* \f */
        {K_HEXCHAR,     '\030'},        /* ^X */
        {K_NL,            '\n'},        /* \n */
        {K_RIGHTBRACE,     '}'},        /*  } */
        {K_RIGHTPAREN,     ')'},        /*  ) */
        {K_TAB,           '\t'},        /* \t */
        {K_VERASE,        '\b'},        /* \b */
        {K_VKILL,       '\025'},        /* ^U */
        {K_VLNEXT,      '\021'},        /* ^Q */
        {K_VLNEXT,      '\026'},        /* ^V */
        {K_VWERASE,     '\027'},        /* ^W */
        {K_ZERO,           '0'},        /*  0 */

#define ADDITIONAL_CHARACTERS   4
        {K_NOTUSED, 0},                 /* VEOF, VERASE, VKILL, VWERASE */
        {K_NOTUSED, 0},
        {K_NOTUSED, 0},
        {K_NOTUSED, 0},
};
static int nkeylist =
    (sizeof(keylist) / sizeof(keylist[0])) - ADDITIONAL_CHARACTERS;

/*
 * v_key_init --
 *      Initialize the special key lookup table.
 *
 * PUBLIC: int v_key_init(SCR *);
 */
int
v_key_init(SCR *sp)
{
        unsigned int ch;
        GS *gp;
        KEYLIST *kp;
        int cnt;

        gp = sp->gp;

        /*
         * XXX
         * 8-bit only, for now.  Recompilation should get you any 8-bit
         * character set, as long as NULL isn't a character.
         */
        (void)setlocale(LC_ALL, "");
        (void)setlocale(LC_NUMERIC, "");
        v_key_ilookup(sp);

        v_keyval(sp, K_CNTRLD, KEY_VEOF);
        v_keyval(sp, K_VERASE, KEY_VERASE);
        v_keyval(sp, K_VKILL, KEY_VKILL);
        v_keyval(sp, K_VWERASE, KEY_VWERASE);

        /* Sort the special key list. */
        qsort(keylist, nkeylist, sizeof(keylist[0]), v_key_cmp);

        /* Initialize the fast lookup table. */
        for (gp->max_special = 0, kp = keylist, cnt = nkeylist; cnt--; ++kp) {
                if (gp->max_special < kp->value)
                        gp->max_special = kp->value;
                if (kp->ch <= MAX_FAST_KEY)
                        gp->special_key[kp->ch] = kp->value;
        }

        /* Find a non-printable character to use as a message separator. */
        for (ch = 1; ch <= MAX_CHAR_T; ++ch)
                if (!isprint(ch)) {
                        gp->noprint = ch;
                        break;
                }
        if (ch != gp->noprint) {
                msgq(sp, M_ERR, "No non-printable character found");
                return (1);
        }
        return (0);
}

/*
 * v_keyval --
 *      Set key values.
 *
 * We've left some open slots in the keylist table, and if these values exist,
 * we put them into place.  Note, they may reset (or duplicate) values already
 * in the table, so we check for that first.
 */
static void
v_keyval(SCR *sp, int val, scr_keyval_t name)
{
        KEYLIST *kp;
        CHAR_T ch;
        int dne;

        /* Get the key's value from the screen. */
        if (sp->gp->scr_keyval(sp, name, &ch, &dne))
                return;
        if (dne)
                return;

        /* Check for duplication. */
        for (kp = keylist; kp->value != K_NOTUSED; ++kp)
                if (kp->ch == ch) {
                        kp->value = val;
                        return;
                }

        /* Add a new entry. */
        if (kp->value == K_NOTUSED) {
                keylist[nkeylist].ch = ch;
                keylist[nkeylist].value = val;
                ++nkeylist;
        }
}

/*
 * v_key_ilookup --
 *      Build the fast-lookup key display array.
 *
 * PUBLIC: void v_key_ilookup(SCR *);
 */
void
v_key_ilookup(SCR *sp)
{
        CHAR_T ch, *p, *t;
        GS *gp;
        size_t len;

        for (gp = sp->gp, ch = 0; ch <= MAX_FAST_KEY; ++ch)
                for (p = gp->cname[ch].name, t = v_key_name(sp, ch),
                    len = gp->cname[ch].len = sp->clen; len--;)
                        *p++ = *t++;
}

/*
 * v_key_len --
 *      Return the length of the string that will display the key.
 *      This routine is the backup for the KEY_LEN() macro.
 *
 * PUBLIC: size_t v_key_len(SCR *, CHAR_T);
 */
size_t
v_key_len(SCR *sp, CHAR_T ch)
{
        (void)v_key_name(sp, ch);
        return (sp->clen);
}

/*
 * v_key_name --
 *      Return the string that will display the key.  This routine
 *      is the backup for the KEY_NAME() macro.
 *
 * PUBLIC: CHAR_T *v_key_name(SCR *, CHAR_T);
 */
CHAR_T *
v_key_name(SCR *sp, CHAR_T ch)
{
        static const CHAR_T hexdigit[] = "0123456789abcdef";
        static const CHAR_T octdigit[] = "01234567";
        CHAR_T *chp, mask;
        size_t len;
        int cnt, shift;

        /* See if the character was explicitly declared printable or not. */
        if ((chp = O_STR(sp, O_PRINT)) != NULL)
                for (; *chp != '\0'; ++chp)
                        if (*chp == ch)
                                goto pr;
        if ((chp = O_STR(sp, O_NOPRINT)) != NULL)
                for (; *chp != '\0'; ++chp)
                        if (*chp == ch)
                                goto nopr;

        /*
         * Historical (ARPA standard) mappings.  Printable characters are left
         * alone.  Control characters less than 0x20 are represented as '^'
         * followed by the character offset from the '@' character in the ASCII
         * character set.  Del (0x7f) is represented as '^' followed by '?'.
         *
         * If set O_ALTNOTATION, most control characters less than 0x20 are
         * displayed using <C-char> notation.  Carriage feed, escape, and
         * delete are displayed as <Ret>, <Esc>, and <Del>, respectively.
         *
         * XXX
         * The following code depends on the current locale being identical to
         * the ASCII map from 0x40 to 0x5f (since 0x1f + 0x40 == 0x5f).  I'm
         * told that this is a reasonable assumption...
         *
         * XXX
         * This code will only work with CHAR_T's that are multiples of 8-bit
         * bytes.
         *
         * XXX
         * NB: There's an assumption here that all printable characters take
         * up a single column on the screen.  This is not always correct.
         */
        if (isprint(ch)) {
pr:             sp->cname[0] = ch;
                len = 1;
                goto done;
        }
nopr:   if (iscntrl(ch) && (ch < 0x20 || ch == 0x7f)) {
            if (O_ISSET(sp, O_ALTNOTATION)) {
                if (ch == 0x00) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = '@';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x01) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'a';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x02) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'b';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x03) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'c';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x04) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'd';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x05) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'e';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x06) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'f';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x07) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'g';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x08) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'h';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x0A) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'N';
                    sp->cname[2] = 'L';
                    sp->cname[3] = '>';
                    len = 4;
                } else if (ch == 0x0B) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'k';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x0C) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'l';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x0D) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'R';
                    sp->cname[2] = 'e';
                    sp->cname[3] = 't';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x0E) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'n';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x0F) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'o';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x10) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'p';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x11) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'q';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x12) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'r';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x13) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 's';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x14) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 't';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x15) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'u';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x16) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'v';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x17) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'w';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x18) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'x';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x19) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'y';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1A) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = 'z';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1B) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'E';
                    sp->cname[2] = 's';
                    sp->cname[3] = 'c';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1C) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = '\\';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1D) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = ']';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1E) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = '^';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x1F) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'C';
                    sp->cname[2] = '-';
                    sp->cname[3] = '_';
                    sp->cname[4] = '>';
                    len = 5;
                } else if (ch == 0x7F) {
                    sp->cname[0] = '<';
                    sp->cname[1] = 'D';
                    sp->cname[2] = 'e';
                    sp->cname[3] = 'l';
                    sp->cname[4] = '>';
                    len = 5;
                } else {
                    sp->cname[0] = '^';
                    sp->cname[1] = ch == 0x7f ? '?' : '@' + ch;
                    len = 2;
		}
            } else {
                sp->cname[0] = '^';
                sp->cname[1] = ch == 0x7f ? '?' : '@' + ch;
                len = 2;
            }
        } else if (O_ISSET(sp, O_OCTAL)) {
#define BITS    (sizeof(CHAR_T) * 8)
#define SHIFT   (BITS - BITS % 3)
#define TOPMASK (BITS % 3 == 2 ? 3 : 1) << (BITS - BITS % 3)
                sp->cname[0] = '\\';
                sp->cname[1] = octdigit[(ch & TOPMASK) >> SHIFT];
                shift = SHIFT - 3;
                for (len = 2, mask = 7 << (SHIFT - 3),
                    cnt = BITS / 3; cnt-- > 0; mask >>= 3, shift -= 3)
                        sp->cname[len++] = octdigit[(ch & mask) >> shift];
        } else {
                sp->cname[0] = '\\';
                sp->cname[1] = 'x';
                for (len = 2, chp = (u_int8_t *)&ch,
                    cnt = sizeof(CHAR_T); cnt-- > 0; ++chp) {
                        sp->cname[len++] = hexdigit[(*chp & 0xf0) >> 4];
                        sp->cname[len++] = hexdigit[*chp & 0x0f];
                }
        }
done:   sp->cname[sp->clen = len] = '\0';
        return (sp->cname);
}

/*
 * v_key_val --
 *      Fill in the value for a key.  This routine is the backup
 *      for the KEY_VAL() macro.
 *
 * PUBLIC: int v_key_val(SCR *, CHAR_T);
 */
int
v_key_val(SCR *sp, CHAR_T ch)
{
        KEYLIST k, *kp;

        k.ch = ch;
        kp = bsearch(&k, keylist, nkeylist, sizeof(keylist[0]), v_key_cmp);
        return (kp == NULL ? K_NOTUSED : kp->value);
}

/*
 * v_event_push --
 *      Push events/keys onto the front of the buffer.
 *
 * There is a single input buffer in ex/vi.  Characters are put onto the
 * end of the buffer by the terminal input routines, and pushed onto the
 * front of the buffer by various other functions in ex/vi.  Each key has
 * an associated flag value, which indicates if it has already been quoted,
 * and if it is the result of a mapping or an abbreviation.
 *
 * PUBLIC: int v_event_push(SCR *, EVENT *, CHAR_T *, size_t, unsigned int);
 */
int
v_event_push(SCR *sp, EVENT *p_evp, CHAR_T *p_s, size_t nitems, unsigned int flags)
{
        EVENT *evp;
        GS *gp;
        size_t total;

        /* If we have room, stuff the items into the buffer. */
        gp = sp->gp;
        if (nitems <= gp->i_next ||
            (gp->i_event != NULL && gp->i_cnt == 0 && nitems <= gp->i_nelem)) {
                if (gp->i_cnt != 0)
                        gp->i_next -= nitems;
                goto copy;
        }

        /*
         * If there are currently items in the queue, shift them up,
         * leaving some extra room.  Get enough space plus a little
         * extra.
         */
#define TERM_PUSH_SHIFT 30
        total = gp->i_cnt + gp->i_next + nitems + TERM_PUSH_SHIFT;
        if (total >= gp->i_nelem && v_event_grow(sp, MAXIMUM(total, 64)))
                return (1);
        if (gp == NULL)
                return (1);
        if (gp->i_cnt)
                MEMMOVE(gp->i_event + TERM_PUSH_SHIFT + nitems,
                    gp->i_event + gp->i_next, gp->i_cnt);
        gp->i_next = TERM_PUSH_SHIFT;

        /* Put the new items into the queue. */
copy:   gp->i_cnt += nitems;
        for (evp = gp->i_event + gp->i_next; nitems--; ++evp) {
                if (p_evp != NULL)
                        *evp = *p_evp++;
                else {
                        if (evp == NULL)
                            return (1);
                        evp->e_event = E_CHARACTER;
                        evp->e_c = *p_s++;
                        evp->e_value = KEY_VAL(sp, evp->e_c);
                        F_INIT(&evp->e_ch, flags);
                }
        }
        return (0);
}

/*
 * v_event_append --
 *      Append events onto the tail of the buffer.
 */
static int
v_event_append(SCR *sp, EVENT *argp)
{
        CHAR_T *s;                      /* Characters. */
        EVENT *evp;
        GS *gp;
        size_t nevents;                 /* Number of events. */

        /* Grow the buffer as necessary. */
        nevents = argp->e_event == E_STRING ? argp->e_len : 1;
        gp = sp->gp;
        if (gp->i_event == NULL ||
            nevents > gp->i_nelem - (gp->i_next + gp->i_cnt))
                v_event_grow(sp, MAXIMUM(nevents, 64));
        evp = gp->i_event + gp->i_next + gp->i_cnt;
        gp->i_cnt += nevents;

        /* Transform strings of characters into single events. */
        if (argp == NULL || evp == NULL)
                return (1);
        if (argp->e_event == E_STRING)
                for (s = argp->e_csp; nevents--; ++evp) {
                        evp->e_event = E_CHARACTER;
                        evp->e_c = *s++;
                        evp->e_value = KEY_VAL(sp, evp->e_c);
                        evp->e_flags = 0;
                }
        else
                *evp = *argp;
        return (0);
}

/* Remove events from the queue. */
#define QREM(len) {                                                     \
        if ((gp->i_cnt -= (len)) == 0)                                  \
                gp->i_next = 0;                                         \
        else                                                            \
                gp->i_next += (len);                                    \
}

/*
 * v_event_get --
 *      Return the next event.
 *
 * !!!
 * The flag EC_NODIGIT probably needs some explanation.  First, the idea of
 * mapping keys is that one or more keystrokes act like a function key.
 * What's going on is that vi is reading a number, and the character following
 * the number may or may not be mapped (EC_MAPCOMMAND).  For example, if the
 * user is entering the z command, a valid command is "z40+", and we don't want
 * to map the '+', i.e. if '+' is mapped to "xxx", we don't want to change it
 * into "z40xxx".  However, if the user enters "35x", we want to put all of the
 * characters through the mapping code.
 *
 * Historical practice is a bit muddled here.  (Surprise!)  It always permitted
 * mapping digits as long as they weren't the first character of the map, e.g.
 * ":map ^A1 xxx" was okay.  It also permitted the mapping of the digits 1-9
 * (the digit 0 was a special case as it doesn't indicate the start of a count)
 * as the first character of the map, but then ignored those mappings.  While
 * it's probably stupid to map digits, vi isn't your mother.
 *
 * The way this works is that the EC_MAPNODIGIT causes term_key to return the
 * end-of-digit without "looking" at the next character, i.e. leaving it as the
 * user entered it.  Presumably, the next term_key call will tell us how the
 * user wants it handled.
 *
 * There is one more complication.  Users might map keys to digits, and, as
 * it's described above, the commands:
 *
 *      :map g 1G
 *      d2g
 *
 * would return the keys "d2<end-of-digits>1G", when the user probably wanted
 * "d21<end-of-digits>G".  So, if a map starts off with a digit we continue as
 * before, otherwise, we pretend we haven't mapped the character, and return
 * <end-of-digits>.
 *
 * Now that that's out of the way, let's talk about Energizer Bunny macros.
 * It's easy to create macros that expand to a loop, e.g. map x 3x.  It's
 * fairly easy to detect this example, because it's all internal to term_key.
 * If we're expanding a macro and it gets big enough, at some point we can
 * assume it's looping and kill it.  The examples that are tough are the ones
 * where the parser is involved, e.g. map x "ayyx"byy.  We do an expansion
 * on 'x', and get "ayyx"byy.  We then return the first 4 characters, and then
 * find the looping macro again.  There is no way that we can detect this
 * without doing a full parse of the command, because the character that might
 * cause the loop (in this case 'x') may be a literal character, e.g. the map
 * map x "ayy"xyy"byy is perfectly legal and won't cause a loop.
 *
 * Historic vi tried to detect looping macros by disallowing obvious cases in
 * the map command, maps that that ended with the same letter as they started
 * (which wrongly disallowed "map x 'x"), and detecting macros that expanded
 * too many times before keys were returned to the command parser.  It didn't
 * get many (most?) of the tricky cases right, however, and it was certainly
 * possible to create macros that ran forever.  And, even if it did figure out
 * what was going on, the user was usually tossed into ex mode.  Finally, any
 * changes made before vi realized that the macro was recursing were left in
 * place.  We recover gracefully, but the only recourse the user has in an
 * infinite macro loop is to interrupt.
 *
 * !!!
 * It is historic practice that mapping characters to themselves as the first
 * part of the mapped string was legal, and did not cause infinite loops, i.e.
 * ":map! { {^M^T" and ":map n nz." were known to work.  The initial, matching
 * characters were returned instead of being remapped.
 *
 * !!!
 * It is also historic practice that the macro "map ] ]]^" caused a single ]
 * keypress to behave as the command ]] (the ^ got the map past the vi check
 * for "tail recursion").  Conversely, the mapping "map n nn^" went recursive.
 * What happened was that, in the historic vi, maps were expanded as the keys
 * were retrieved, but not all at once and not centrally.  So, the keypress ]
 * pushed ]]^ on the stack, and then the first ] from the stack was passed to
 * the ]] command code.  The ]] command then retrieved a key without entering
 * the mapping code.  This could bite us anytime a user has a map that depends
 * on secondary keys NOT being mapped.  I can't see any possible way to make
 * this work in here without the complete abandonment of Rationality Itself.
 *
 * XXX
 * The final issue is recovery.  It would be possible to undo all of the work
 * that was done by the macro if we entered a record into the log so that we
 * knew when the macro started, and, in fact, this might be worth doing at some
 * point.  Given that this might make the log grow unacceptably (consider that
 * cursor keys are done with maps), for now we leave any changes made in place.
 *
 * PUBLIC: int v_event_get(SCR *, EVENT *, int, u_int32_t);
 */
int
v_event_get(SCR *sp, EVENT *argp, int timeout, u_int32_t flags)
{
        EVENT *evp, ev;
        GS *gp;
        SEQ *qp;
        int init_nomap, ispartial, istimeout, remap_cnt;

        gp = sp->gp;

        /* If simply checking for interrupts, argp may be NULL. */
        if (argp == NULL)
                argp = &ev;

retry:  istimeout = remap_cnt = 0;

        /*
         * If the queue isn't empty and we're timing out for characters,
         * return immediately.
         */
        if (gp->i_cnt != 0 && LF_ISSET(EC_TIMEOUT))
                return (0);

        /*
         * If the queue is empty, we're checking for interrupts, or we're
         * timing out for characters, get more events.
         */
        if (gp->i_cnt == 0 || LF_ISSET(EC_INTERRUPT | EC_TIMEOUT)) {
                /*
                 * If we're reading new characters, check any scripting
                 * windows for input.
                 */
                if (F_ISSET(gp, G_SCRWIN) && sscr_input(sp))
                        return (1);
loop:           if (gp->scr_event(sp, argp,
                    LF_ISSET(EC_INTERRUPT | EC_QUOTED | EC_RAW), timeout))
                        return (1);
                switch (argp->e_event) {
                case E_ERR:
                case E_SIGHUP:
                case E_SIGTERM:
                        /*
                         * Fatal conditions cause the file to be synced to
                         * disk immediately.
                         */
                        v_sync(sp, RCV_ENDSESSION | RCV_PRESERVE |
                            (argp->e_event == E_SIGTERM ? 0: RCV_EMAIL));
                        return (1);
                case E_TIMEOUT:
                        istimeout = 1;
                        break;
                case E_INTERRUPT:
                        /* Set the global interrupt flag. */
                        F_SET(sp->gp, G_INTERRUPTED);

                        /*
                         * If the caller was interested in interrupts, return
                         * immediately.
                         */
                        if (LF_ISSET(EC_INTERRUPT))
                                return (0);
                        goto append;
                default:
append:                 if (v_event_append(sp, argp))
                                return (1);
                        break;
                }
        }

        /*
         * If the caller was only interested in interrupts or timeouts, return
         * immediately.  (We may have gotten characters, and that's okay, they
         * were queued up for later use.)
         */
        if (LF_ISSET(EC_INTERRUPT | EC_TIMEOUT))
                return (0);

newmap: evp = &gp->i_event[gp->i_next];

        /*
         * If the next event in the queue isn't a character event, return
         * it, we're done.
         */
        if (evp->e_event != E_CHARACTER) {
                *argp = *evp;
                QREM(1);
                return (0);
        }

        /*
         * If the key isn't mappable because:
         *
         *      + ... the timeout has expired
         *      + ... it's not a mappable key
         *      + ... neither the command or input map flags are set
         *      + ... there are no maps that can apply to it
         *
         * return it forthwith.
         */
        if (istimeout || F_ISSET(&evp->e_ch, CH_NOMAP) ||
            !LF_ISSET(EC_MAPCOMMAND | EC_MAPINPUT) ||
            (evp->e_c < MAX_BIT_SEQ && !bit_test(gp->seqb, evp->e_c)))
                goto nomap;

        /* Search the map. */
        qp = seq_find(sp, NULL, evp, NULL, gp->i_cnt,
            LF_ISSET(EC_MAPCOMMAND) ? SEQ_COMMAND : SEQ_INPUT, &ispartial);

        /*
         * If get a partial match, get more characters and retry the map.
         * If time out without further characters, return the characters
         * unmapped.
         *
         * !!!
         * <escape> characters are a problem.  Cursor keys start with <escape>
         * characters, so there's almost always a map in place that begins with
         * an <escape> character.  If we timeout <escape> keys in the same way
         * that we timeout other keys, the user will get a noticeable pause as
         * they enter <escape> to terminate input mode.  If key timeout is set
         * for a slow link, users will get an even longer pause.  Nvi used to
         * simply timeout <escape> characters at 1/10th of a second, but this
         * loses over PPP links where the latency is greater than 100Ms.
         */
        if (ispartial) {
                if (O_ISSET(sp, O_TIMEOUT))
                        timeout = (evp->e_value == K_ESCAPE ?
                            O_VAL(sp, O_ESCAPETIME) :
                            O_VAL(sp, O_KEYTIME)) * 100;
                else
                        timeout = 0;
                goto loop;
        }

        /* If no map, return the character. */
        if (qp == NULL) {
nomap:          if (!isdigit(evp->e_c) && LF_ISSET(EC_MAPNODIGIT))
                        goto not_digit;
                *argp = *evp;
                QREM(1);
                return (0);
        }

        /*
         * If looking for the end of a digit string, and the first character
         * of the map is it, pretend we haven't seen the character.
         */
        if (LF_ISSET(EC_MAPNODIGIT) &&
            qp->output != NULL && !isdigit(qp->output[0])) {
not_digit:      argp->e_c = CH_NOT_DIGIT;
                argp->e_value = K_NOTUSED;
                argp->e_event = E_CHARACTER;
                F_INIT(&argp->e_ch, 0);
                return (0);
        }

        /* Find out if the initial segments are identical. */
        if (qp->output != NULL) {
                init_nomap =
                    !e_memcmp(qp->output, &gp->i_event[gp->i_next], qp->ilen);
        }

        /* Delete the mapped characters from the queue. */
        QREM(qp->ilen);

        /* If keys mapped to nothing, go get more. */
        if (qp->output == NULL)
                goto retry;

        /* If remapping characters... */
        if (O_ISSET(sp, O_REMAP)) {
                /*
                 * Periodically check for interrupts.  Always check the first
                 * time through, because it's possible to set up a map that
                 * will return a character every time, but will expand to more,
                 * e.g. "map! a aaaa" will always return a 'a', but we'll never
                 * get anywhere useful.
                 */
                if ((++remap_cnt == 1 || remap_cnt % 10 == 0) &&
                    (gp->scr_event(sp, &ev,
                    EC_INTERRUPT, 0) || ev.e_event == E_INTERRUPT)) {
                        F_SET(sp->gp, G_INTERRUPTED);
                        argp->e_event = E_INTERRUPT;
                        return (0);
                }

                /*
                 * If an initial part of the characters mapped, they are not
                 * further remapped -- return the first one.  Push the rest
                 * of the characters, or all of the characters if no initial
                 * part mapped, back on the queue.
                 */
                if (init_nomap) {
                        if (v_event_push(sp, NULL, qp->output + qp->ilen,
                            qp->olen - qp->ilen, CH_MAPPED))
                                return (1);
                        if (v_event_push(sp, NULL,
                            qp->output, qp->ilen, CH_NOMAP | CH_MAPPED))
                                return (1);
                        evp = &gp->i_event[gp->i_next];
                        goto nomap;
                }
                if (v_event_push(sp, NULL, qp->output, qp->olen, CH_MAPPED))
                        return (1);
                goto newmap;
        }

        /* Else, push the characters on the queue and return one. */
        if (v_event_push(sp, NULL, qp->output, qp->olen, CH_MAPPED | CH_NOMAP))
                return (1);

        goto nomap;
}

/*
 * v_sync --
 *      Walk the screen lists, sync'ing files to their backup copies.
 */
static void
v_sync(SCR *sp, int flags)
{
        GS *gp;

        gp = sp->gp;
        TAILQ_FOREACH(sp, &gp->dq, q)
                rcv_sync(sp, flags);
        TAILQ_FOREACH(sp, &gp->hq, q)
                rcv_sync(sp, flags);
}

/*
 * v_event_err --
 *      Unexpected event.
 *
 * PUBLIC: void v_event_err(SCR *, EVENT *);
 */
void
v_event_err(SCR *sp, EVENT *evp)
{
        switch (evp->e_event) {
        case E_CHARACTER:
                msgq(sp, M_ERR, "Unexpected character event");
                break;
        case E_EOF:
                msgq(sp, M_ERR, "Unexpected end-of-file event");
                break;
        case E_INTERRUPT:
                msgq(sp, M_ERR, "Unexpected interrupt event");
                break;
        case E_QUIT:
                msgq(sp, M_ERR, "Unexpected quit event");
                break;
        case E_REPAINT:
                msgq(sp, M_ERR, "Unexpected repaint event");
                break;
        case E_STRING:
                msgq(sp, M_ERR, "Unexpected string event");
                break;
        case E_TIMEOUT:
                msgq(sp, M_ERR, "Unexpected timeout event");
                break;
        case E_WRESIZE:
                msgq(sp, M_ERR, "Unexpected resize event");
                break;
        case E_WRITE:
                msgq(sp, M_ERR, "Unexpected write event");
                break;

        /*
         * Theoretically, none of these can occur, as they're handled at the
         * top editor level.
         */
        case E_ERR:
        case E_SIGHUP:
        case E_SIGTERM:
        default:
                abort();
        }

        /* Free any allocated memory. */
        free(evp->e_asp);
}

/*
 * v_event_flush --
 *      Flush any flagged keys, returning if any keys were flushed.
 *
 * PUBLIC: int v_event_flush(SCR *, unsigned int);
 */
int
v_event_flush(SCR *sp, unsigned int flags)
{
        GS *gp;
        int rval;

        for (rval = 0, gp = sp->gp; gp->i_cnt != 0 &&
            F_ISSET(&gp->i_event[gp->i_next].e_ch, flags); rval = 1)
                QREM(1);
        return (rval);
}

/*
 * v_event_grow --
 *      Grow the terminal queue.
 */
static int
v_event_grow(SCR *sp, int add)
{
        GS *gp;
        size_t new_nelem, olen;

        gp = sp->gp;
        new_nelem = gp->i_nelem + add;
        olen = gp->i_nelem * sizeof(gp->i_event[0]);
        BINC_RET(sp, gp->i_event, olen, new_nelem * sizeof(gp->i_event[0]));
        gp->i_nelem = olen / sizeof(gp->i_event[0]);
        return (0);
}

/*
 * v_key_cmp --
 *      Compare two keys for sorting.
 */
static int
v_key_cmp(const void *ap, const void *bp)
{
        return (((KEYLIST *)ap)->ch - ((KEYLIST *)bp)->ch);
}
