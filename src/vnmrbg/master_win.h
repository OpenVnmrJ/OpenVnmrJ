/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef MASTER_WIN_H
#define MASTER_WIN_H

extern void unlink_alphafile();
extern void show_vnmrj_alphadisplay();
extern void textprintf(char *fgReaderBuffer,int len);
extern void dispMessage(int item_no, char *msg, int loc_x, int loc_y, int width);
extern void register_child_exit_func(int signum, int pid,
                                     void (*func_exit)(), void (*func_kill)());
extern void but_interpos_handler_jfunc(char *jstr );
extern void removeButtons();
extern void setErrorwindow(char *tex);
extern void setTextwindow(char *tex);
extern void fileDump(char *file);
extern void buildButton(int bnum, char *label);
extern void buildMainButton(int num, char *label, void (*func)());
extern void ttyInput(char *tex, int len);
extern void ttyFocus(char *tex);
extern void kill_all_childs();
extern void raise_text_window();
extern void setLargeSmallButton(char *size);
extern void jNextPrev(int cmd );
extern void ignoreSigpipe();
extern void set_acqi_signal();
extern void nmr_quit(int signal);
extern void set_nmr_quit_signal();
extern void set_wait_child(int pid);
extern void unlockPanel();
extern void system_call(char *s);
extern FILE *popen_call(char *cmdstr, char *mode );
extern int pclose_call( FILE *pfile );
extern void MasterLoop();
extern void ring_bell();
extern void setCallbackTimer(int n);
extern void clearForground(int pipeSuspended);
extern void waitForgroundChild(int forgroundFdR, int pid);
extern void createWindows2(int tclType, int noUi);
extern void readyCatchers();
extern void readyJCatchers(int fd );
extern void removeJCatchers(int fd );
extern int listen_socket (int portNum);
extern int shelldebug(int t);
#endif
