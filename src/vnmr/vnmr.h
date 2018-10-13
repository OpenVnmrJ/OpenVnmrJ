/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

extern void Wprintfpos(int, int, int, char *, ...);
extern void Wprintf(char *, ...);
extern int  Wsetgraphicsdisplay(char *);
extern int  Wsettextdisplay(char *);
extern int  P_setreal(int tree, char *name, double value, int index);
extern int  P_setstring(int tree, char *name, char *strptr, int index);
extern int  P_Esetreal(int tree, char *name, double value, int index);
extern int  P_Esetstring(int tree, char *name, char *strptr, int index);
extern int  P_setlimits(int tree, char *name, double max, double min, double step);
extern int  P_setgroup(int tree, char *name, int group);
extern int  P_setdgroup(int tree, char *name, int group);
extern int  P_setprot(int tree, char *name, int prot);
extern int  P_getactive(int tree, char *name);
extern int  P_setactive(int tree, char *name, int act);
extern int  P_pruneTree(int tree1, int tree2);
extern int  P_deleteGroupVar(int tree, int group);
extern int  P_copy(int fromtree, int totree);
extern int  P_copygroup(int fromtree, int totree, int group);
extern int  P_copyvar(int fromtree, int totree, char *name, char *nname);
extern int  P_deleteVar(int tree, char *name);
extern int  P_creatvar(int tree, char *name, int type);
extern int  P_treereset(int tree);
extern varInfo *P_getVarInfoAddr(int tree, char *name);
extern int  P_getVarInfo(int tree, char *name, vInfo *info);
extern int  P_getreal(int tree, char *name, double *value, int index);
extern int  P_getstring(int tree, char *name, char *buf, int index, int maxbuf);
extern int  P_Egetreal(int tree, char *name, double *value, int index);
extern int  P_Egetstring(int tree, char *name, char *buf, int index, int maxbuf);
extern int  P_listnames(int tree, char *list);
extern int  P_save(int tree, char *filename);
extern int  P_rtx(int tree, char *filename, int key1IsRt, int key2IsClear);
extern int  P_read(int tree, char *filename);
extern int  P_getparinfo(int tree, char *pname, double *pval, int *pstatus);
extern int  P_getmax(int tree, char *pname, double *pval);
extern void P_err(int res, char *s, char *t);
extern int  InitRecverAddr();
extern int  SendAsyncInova();
extern int  set_comm_port();
extern int  wait_for_select();
extern int  prepare_reply_socket();
extern void Wgmode();		// not set in wjunk.c
extern int  xormode();
extern void repaintCanvas();
