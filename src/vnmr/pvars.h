/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef PVARS_H
#define PVARS_H

#ifdef __cplusplus
extern "C" {
#endif
int P_setreal(int tree, const char *name, double value, int index);
int P_setstring(int tree, const char *name, const char *value, int index);
int P_Esetreal(int tree, const char *name, double value, int index);
int P_Esetstring(int tree, const char *name, char *strptr, int index);
int P_setlimits(int tree, const char *name, double max, double min, double step);
int P_setgroup(int tree, const char *name, int group);
int P_getstring(int tree, const char *name, char *buf, int index, int maxbuf);
int P_isString(int tree, const char *name);
int P_getreal(int tree, const char *name, double *value, int index) ;
int P_creatvar(int tree, const char *name, int type);
int P_setdgroup(int tree, const char *name, int value);
int P_setprot(int tree, const char *name, int value);
int P_setactive(int tree, const char *name, int activeFlag);
int P_getactive(int tree, const char *name);
int P_getsubtype(int tree, const char *name);
int P_getsize(int tree, const char *name, int *val);
int P_pruneTree(int tree1, int tree2);
int P_exch(int tree1, int tree2);
int P_copy(int fromtree, int totree);
int P_copygroup(int fromtree, int totree, int group);
int P_copyvar(int fromtree, int totree, const char *name, const char *nname);
int P_deleteVar(int tree, const char *name);
int P_deleteVarIndex(int tree, const char *name, int index);
int P_treereset(int tree);
int P_save(int tree, const char *filename);
int P_saveUnsharedGlobal(const char *filename);
int P_rtx(int tree, const char *filename, int key1IsRt, int key2IsClear);
int P_read(int tree, const char *filename);
int P_testread(int tree);
int P_ispar(const char *filename);
int P_readnames(const char *filename, char **names);
int P_getparinfo(int tree, const char *pname, double *pval, int *pstatus);
int P_getmax(int tree, const char *pname, double *pval);
int P_getmin(int tree, const char *pname, double *pval);
void P_err(int res, const char *s, const char *t);
double P_getval(const char *name);
#ifdef __cplusplus
}
#endif

#endif 
