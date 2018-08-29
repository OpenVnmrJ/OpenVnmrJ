/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef WJUNK_H
#define WJUNK_H

extern void Werrprintf(const char *format, ...) __attribute__((format(printf,1,2)));
extern void WerrprintfWithPos(const char *format, ...) __attribute__((format(printf,1,2)));
extern void WerrprintfLine3Only(const char *format, ...) __attribute__((format(printf,1,2)));
extern void Winfoprintf(const char *format, ...) __attribute__((format(printf,1,2)));
extern void Wprintfpos(int window, int line, int column, const char *format, ...) __attribute__((format(printf,4,5)));
extern void Wscrprintf(const char *format, ...) __attribute__((format(printf,1,2)));
extern char *Wgetgraphicsdisplay(char *buf, int max1);
extern char *W_getInputCR(char *prompt, char *s, int n);
extern char *W_getInput(char *prompt, char *s, int n);
extern void Wsetactive(int window);
extern void Wclear(int window);
extern void Wclear_text();
extern void Wclear_graphics();
extern void Wsettextdisplay(char *n);
extern void Wsetgraphicsdisplay(char *n);
extern void disp_acq(char *t);
extern void disp_status(char *t);
extern void disp_index(int i);
extern void disp_specIndex(int i);
extern void disp_seq(char *t);
extern void disp_exp(int i);
extern void Wshow_text();
extern int  WscreenSize();
extern int  Wissun();
extern int  WisSunColor();
extern int  Wistek();
extern void setTtyInputFocus();
extern void Wresterm(char *mes);
extern void Wsetupterm();
extern void setTextAtBottom();
extern void Wseterrorkey(char *str);
extern void Woverlap();
extern void Wshow_graphics();
extern void Wgmode();
extern int WgraphicsdisplayValid(char *n);
extern void Walpha(int window);
extern void Wstprintf(char *format, ...) __attribute__((format(printf,1,2)));
extern int WtextdisplayValid(char *n);
extern void Wexecgraphicsdisplay();
extern void WstoreMessageOff();
extern void WstoreMessageOn();
extern void Wturnoff_message();
extern void Wturnon_message();
extern char storeMessage[];

#endif 
