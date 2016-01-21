/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPPARMLIST_H
#define AIPPARMLIST_H

#define PL_INT 1
#define PL_FLOAT 2
#define PL_STRING 3
#define PL_PARM 4
#define PL_PTR 5

typedef union {
    int ival;
    float fval;
    char *sval;
    void *pval;
} ParmItem;
typedef ParmItem *ParmList;

#ifdef __cplusplus
extern "C" {
#endif

extern ParmList allocParm(const char *name, int type, int nelems);

extern ParmList appendFloatParm(ParmList, float value);
extern ParmList appendIntParm(ParmList, int value);
extern ParmList appendParmParm(ParmList, ParmList value);
extern ParmList appendPtrParm(ParmList, void *value);
extern ParmList appendStrParm(ParmList, const char *value);

extern int countParm(ParmList);
extern ParmList findParm(ParmList, const char *name);
extern void freeParms(ParmList);

extern int getFloatParm(ParmList, float *value, int elem);
extern int getFloatParms(ParmList, float *array, int nelems);
extern int getIntParm(ParmList, int *value, int elem);
extern int getIntParms(ParmList, int *array, int nelems);
extern int getParmParm(ParmList, ParmList *value, int elem);
extern int getParmParms(ParmList, ParmList *array, int nelems);
extern int getPtrParm(ParmList, void **value, int elem);
extern int getPtrParms(ParmList, void **array, int nelems);
extern int getStrParm(ParmList, char **value, int elem);
extern int getStrParms(ParmList, char **array, int nelems);

extern void printParm(ParmList);

extern int setFloatParm(ParmList, float value, int elem);
extern int setFloatParms(ParmList, float *array, int nelems);
extern int setIntParm(ParmList, int value, int elem);
extern int setIntParms(ParmList, int *array, int nelems);
extern int setParmParm(ParmList, ParmList value, int elem);
extern int setParmParms(ParmList, ParmList *array, int nelems);
extern int setPtrParm(ParmList, void *value, int elem);
extern int setPtrParms(ParmList, void **array, int nelems);
extern int setStrParm(ParmList, const char *value, int elem);
extern int setStrParms(ParmList, const char **array, int nelems);

extern int typeofParm(ParmList p);

#ifdef __cplusplus
}
#endif

#endif /* AIPPARMLIST_H */
