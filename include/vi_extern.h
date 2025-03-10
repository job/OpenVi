/* SPDX-License-Identifier: BSD-3-Clause */

/*
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
 * 3. Neither the names of the copyright holders nor the names of any
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

int cs_init(SCR *, VCS *);
int cs_next(SCR *, VCS *);
int cs_fspace(SCR *, VCS *);
int cs_fblank(SCR *, VCS *);
int cs_prev(SCR *, VCS *);
int cs_bblank(SCR *, VCS *);
int v_at(SCR *, VICMD *);
int v_chrepeat(SCR *, VICMD *);
int v_chrrepeat(SCR *, VICMD *);
int v_cht(SCR *, VICMD *);
int v_chf(SCR *, VICMD *);
int v_chT(SCR *, VICMD *);
int v_chF(SCR *, VICMD *);
int v_delete(SCR *, VICMD *);
int v_again(SCR *, VICMD *);
int v_exmode(SCR *, VICMD *);
int v_join(SCR *, VICMD *);
int v_shiftl(SCR *, VICMD *);
int v_shiftr(SCR *, VICMD *);
int v_suspend(SCR *, VICMD *);
int v_switch(SCR *, VICMD *);
int v_tagpush(SCR *, VICMD *);
int v_tagpop(SCR *, VICMD *);
int v_filter(SCR *, VICMD *);
int v_event_exec(SCR *, VICMD *);
int v_ex(SCR *, VICMD *);
int v_ecl_exec(SCR *);
int v_increment(SCR *, VICMD *);
int v_screen_copy(SCR *, SCR *);
int v_screen_end(SCR *);
int v_optchange(SCR *, int, char *, unsigned long *);
int v_iA(SCR *, VICMD *);
int v_ia(SCR *, VICMD *);
int v_iI(SCR *, VICMD *);
int v_ii(SCR *, VICMD *);
int v_iO(SCR *, VICMD *);
int v_io(SCR *, VICMD *);
int v_change(SCR *, VICMD *);
int v_Replace(SCR *, VICMD *);
int v_subst(SCR *, VICMD *);
int v_left(SCR *, VICMD *);
int v_cfirst(SCR *, VICMD *);
int v_first(SCR *, VICMD *);
int v_ncol(SCR *, VICMD *);
int v_zero(SCR *, VICMD *);
int v_mark(SCR *, VICMD *);
int v_bmark(SCR *, VICMD *);
int v_fmark(SCR *, VICMD *);
int v_match(SCR *, VICMD *);
int v_paragraphf(SCR *, VICMD *);
int v_paragraphb(SCR *, VICMD *);
int v_buildps(SCR *, char *, char *);
int v_Put(SCR *, VICMD *);
int v_put(SCR *, VICMD *);
int v_redraw(SCR *, VICMD *);
int v_replace(SCR *, VICMD *);
int v_right(SCR *, VICMD *);
int v_dollar(SCR *, VICMD *);
int v_screen(SCR *, VICMD *);
int v_lgoto(SCR *, VICMD *);
int v_home(SCR *, VICMD *);
int v_middle(SCR *, VICMD *);
int v_bottom(SCR *, VICMD *);
int v_up(SCR *, VICMD *);
int v_cr(SCR *, VICMD *);
int v_down(SCR *, VICMD *);
int v_hpageup(SCR *, VICMD *);
int v_hpagedown(SCR *, VICMD *);
int v_pagedown(SCR *, VICMD *);
int v_pageup(SCR *, VICMD *);
int v_lineup(SCR *, VICMD *);
int v_linedown(SCR *, VICMD *);
int v_searchb(SCR *, VICMD *);
int v_searchf(SCR *, VICMD *);
int v_searchN(SCR *, VICMD *);
int v_searchn(SCR *, VICMD *);
int v_searchw(SCR *, VICMD *);
int v_correct(SCR *, VICMD *, int);
int v_sectionf(SCR *, VICMD *);
int v_sectionb(SCR *, VICMD *);
int v_sentencef(SCR *, VICMD *);
int v_sentenceb(SCR *, VICMD *);
int v_status(SCR *, VICMD *);
int v_tcmd(SCR *, VICMD *, CHAR_T, unsigned int);
int v_txt(SCR *, VICMD *, MARK *,
const char *, size_t, CHAR_T, recno_t, unsigned long, u_int32_t);
int v_txt_auto(SCR *, recno_t, TEXT *, size_t, TEXT *);
int v_ulcase(SCR *, VICMD *);
int v_mulcase(SCR *, VICMD *);
int v_Undo(SCR *, VICMD *);
int v_undo(SCR *, VICMD *);
void v_eof(SCR *, MARK *);
void v_eol(SCR *, MARK *);
void v_nomove(SCR *);
void v_sof(SCR *, MARK *);
void v_sol(SCR *);
int v_isempty(char *, size_t);
void v_emsg(SCR *, char *, vim_t);
int v_wordW(SCR *, VICMD *);
int v_wordw(SCR *, VICMD *);
int v_wordE(SCR *, VICMD *);
int v_worde(SCR *, VICMD *);
int v_wordB(SCR *, VICMD *);
int v_wordb(SCR *, VICMD *);
int v_xchar(SCR *, VICMD *);
int v_Xchar(SCR *, VICMD *);
int v_yank(SCR *, VICMD *);
int v_z(SCR *, VICMD *);
int vs_crel(SCR *, long);
int v_zexit(SCR *, VICMD *);
int vi(SCR **);
int vs_line(SCR *, SMAP *, size_t *, size_t *);
int vs_number(SCR *);
void vs_busy(SCR *, const char *, busy_t);
void vs_home(SCR *);
void vs_update(SCR *, const char *, const char *);
void vs_msg(SCR *, mtype_t, char *, size_t);
int vs_ex_resolve(SCR *, int *);
int vs_resolve(SCR *, SCR *, int);
int vs_repaint(SCR *, EVENT *);
int vs_refresh(SCR *, int);
int vs_column(SCR *, size_t *);
size_t vs_screens(SCR *, recno_t, size_t *);
size_t vs_columns(SCR *, char *, recno_t, size_t *, size_t *);
size_t vs_rcm(SCR *, recno_t, int);
size_t vs_colpos(SCR *, recno_t, size_t);
int vs_change(SCR *, recno_t, lnop_t);
int vs_sm_fill(SCR *, recno_t, pos_t);
int vs_sm_scroll(SCR *, MARK *, recno_t, scroll_t);
int vs_sm_1up(SCR *);
int vs_sm_1down(SCR *);
int vs_sm_next(SCR *, SMAP *, SMAP *);
int vs_sm_prev(SCR *, SMAP *, SMAP *);
int vs_sm_cursor(SCR *, SMAP **);
int vs_sm_position(SCR *, MARK *, unsigned long, pos_t);
recno_t vs_sm_nlines(SCR *, SMAP *, recno_t, size_t);
int vs_split(SCR *, SCR *, int);
int vs_discard(SCR *, SCR **);
int vs_fg(SCR *, SCR **, CHAR_T *, int);
int vs_bg(SCR *);
int vs_swap(SCR *, SCR **, char *);
int vs_resize(SCR *, long, adj_t);
