/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parmlist.h"

#define PL_NAMEOFFSET 0
#define PL_SIZEOFFSET 1
#define PL_TYPEOFFSET 2
#define PL_DATAOFFSET 3

ParmList
allocParm(char *name, int type, int nelems)
{
    ParmList rtn;
    char *pname;

    pname = strdup(name);
    if (!pname) return 0;
    rtn = (ParmList)malloc((nelems+PL_DATAOFFSET)*sizeof(ParmItem));
    if (rtn){
	rtn[PL_NAMEOFFSET].sval = pname;
	rtn[PL_SIZEOFFSET].ival = nelems;
	rtn[PL_TYPEOFFSET].ival = type;
    }
    return rtn;
}

ParmList
appendFloatParm(ParmList p, float value)
{
    ParmList rtn = NULL;
    int n;

    if (p[PL_TYPEOFFSET].ival == PL_FLOAT){
	n = countParm(p);
	rtn = (ParmList)realloc(p, (n+1+PL_DATAOFFSET)*sizeof(ParmItem));
	if (rtn){
	    rtn[PL_SIZEOFFSET].ival = n+1;
	    setFloatParm(rtn, value, n);
	}
    }
    return rtn;
}

ParmList
appendIntParm(ParmList p, int value)
{
    ParmList rtn = NULL;
    int n;

    if (p[PL_TYPEOFFSET].ival == PL_INT){
	n = countParm(p);
	rtn = (ParmList)realloc(p, (n+1+PL_DATAOFFSET)*sizeof(ParmItem));
	if (rtn){
	    rtn[PL_SIZEOFFSET].ival = n+1;
	    setIntParm(rtn, value, n);
	}
    }
    return rtn;
}

ParmList
appendParmParm(ParmList p, ParmList value)
{
    ParmList rtn = NULL;
    int n;

    if (p[PL_TYPEOFFSET].ival == PL_PARM){
	n = countParm(p);
	rtn = (ParmList)realloc(p, (n+1+PL_DATAOFFSET)*sizeof(ParmItem));
	if (rtn){
	    rtn[PL_SIZEOFFSET].ival = n+1;
	    setParmParm(rtn, value, n);
	}
    }
    return rtn;
}

ParmList
appendPtrParm(ParmList p, void *value)
{
    ParmList rtn = NULL;
    int n;

    if (p[PL_TYPEOFFSET].ival == PL_PTR){
	n = countParm(p);
	rtn = (ParmList)realloc(p, (n+1+PL_DATAOFFSET)*sizeof(ParmItem));
	if (rtn){
	    rtn[PL_SIZEOFFSET].ival = n+1;
	    setPtrParm(rtn, value, n);
	}
    }
    return rtn;
}

ParmList
appendStrParm(ParmList p, char *value)
{
    ParmList rtn = NULL;
    int n;

    if (p[PL_TYPEOFFSET].ival == PL_STRING){
	n = countParm(p);
	rtn = (ParmList)realloc(p, (n+1+PL_DATAOFFSET)*sizeof(ParmItem));
	if (rtn){
	    rtn[PL_SIZEOFFSET].ival = n+1;
	    setStrParm(rtn, value, n);
	}
    }
    return rtn;
}

int
countParm(ParmList p)
{
    int rtn = 0;
    if (p){
	rtn = p[PL_SIZEOFFSET].ival;
    }
    return rtn;
}

ParmList
findParm(ParmList p, char *name)
{
    int i;
    int n;
    ParmList q;
    ParmList rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_PARM){
	n = countParm(p);
	for (i=PL_DATAOFFSET; i<PL_DATAOFFSET+n; i++){
	    q = (ParmList)p[i].pval;
	    if (strcmp(q[PL_NAMEOFFSET].sval, name) == 0){
		rtn = q;
		break;
	    }
	}
    }
    return rtn;
}

void
freeParms(ParmList p)
{
    int n;
    int i;
    int type;
    char *s;
    ParmList p2;

    if (p){
	n = countParm(p);
	if ((type=typeofParm(p)) == PL_PARM) {
	    for (i=0; i<n; i++){
		getParmParm(p, &p2, i);
		freeParms(p2);
	    }
	}else if (type == PL_STRING){
	    for (i=0; i<n; i++){
		getStrParm(p, &s, i);
		if (s){
		    free(s);
		}
	    }
	}
	free(p);
    }
}

int
getFloatParm(ParmList p, float *value, int elem)
{
    int rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_FLOAT
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	*value = p[PL_DATAOFFSET+elem].fval;
	rtn++;
    }
    return rtn;
}

int
getFloatParms(ParmList p, float *array, int nelems)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<nelems; i++){
	if (ok=getFloatParm(p, &array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
getIntParm(ParmList p, int *value, int elem)
{
    int rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_INT
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	*value = p[PL_DATAOFFSET+elem].ival;
	rtn++;
    }
    return rtn;
}

int
getIntParms(ParmList p, int *array, int nelems)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<nelems; i++){
	if (ok=getIntParm(p, &array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
getParmParm(ParmList p, ParmList *value, int elem)
{
    int rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_PARM
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	*value = (ParmList)p[PL_DATAOFFSET+elem].pval;
	rtn++;
    }
    return rtn;
}

int
getParmParms(ParmList p, ParmList *array, int nelems)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<nelems; i++){
	if (ok=getParmParm(p, &array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
getPtrParm(ParmList p, void **value, int elem)
{
    int rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_PTR
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	*value = p[PL_DATAOFFSET+elem].pval;
	rtn++;
    }
    return rtn;
}

int
getPtrParms(ParmList p, void **array, int nelems)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<nelems; i++){
	if (ok=getPtrParm(p, &array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
getStrParm(ParmList p, char **value, int elem)
{
    int rtn = 0;
    if (p[PL_TYPEOFFSET].ival == PL_STRING
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	*value = p[PL_DATAOFFSET+elem].sval;
	rtn++;
    }
    return rtn;
}

int
getStrParms(ParmList p, char **array, int nelems)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<nelems; i++){
	if (ok=getStrParm(p, &array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

void
printParm(ParmList p)
{
    int i;
    int n;
    int type;
    int pi;
    float pf;
    void *pp;
    char *ps;
    ParmList parm;

    if (p){
	fprintf(stderr,"%s = [", p[PL_NAMEOFFSET].sval);
	n = countParm(p);
	type = typeofParm(p);
	for (i=0; i<n; i++){
	    switch (type){
	      case PL_INT:
		getIntParm(p, &pi, i);
		fprintf(stderr,"%d ", pi);
		break;
	      case PL_FLOAT:
		getFloatParm(p, &pf, i);
		fprintf(stderr,"%g ", pf);
		break;
	      case PL_STRING:
		getStrParm(p, &ps, i);
		if (ps){
		    fprintf(stderr,"\"%s\" ", ps);
		}else{
		    fprintf(stderr,"NULL ");
		}
		break;
	      case PL_PTR:
		getPtrParm(p, &pp, i);
		fprintf(stderr,"%p ", pp);
		break;
	      case PL_PARM:
		getParmParm(p, &parm, i);
		fprintf(stderr,"%p ", parm);
		break;
	    }
	}
	fprintf(stderr,"]\n");
    }
}

int
setFloatParm(ParmList p, float value, int elem)
{
    int rtn = 0;
    if (p
	&& p[PL_TYPEOFFSET].ival == PL_FLOAT
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	p[PL_DATAOFFSET+elem].fval = value;
	rtn = 1;
    }
    return rtn;
}

int
setFloatParms(ParmList p, float *array, int n)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<n; i++){
	if (ok=setFloatParm(p, array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
setIntParm(ParmList p, int value, int elem)
{
    int rtn = 0;
    if (p
	&& p[PL_TYPEOFFSET].ival == PL_INT
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	p[PL_DATAOFFSET+elem].ival = value;
	rtn = 1;
    }
    return rtn;
}

int
setIntParms(ParmList p, int *array, int n)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<n; i++){
	if (ok=setIntParm(p, array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
setParmParm(ParmList p, ParmList value, int elem)
{
    int rtn = 0;
    if (p
	&& p[PL_TYPEOFFSET].ival == PL_PARM
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	p[PL_DATAOFFSET+elem].pval = value;
	rtn = 1;
    }
    return rtn;
}

int
setParmParms(ParmList p, ParmList *array, int n)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<n; i++){
	if (ok=setParmParm(p, array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
setPtrParm(ParmList p, void *value, int elem)
{
    int rtn = 0;
    if (p
	&& p[PL_TYPEOFFSET].ival == PL_PTR
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	p[PL_DATAOFFSET+elem].pval = value;
	rtn = 1;
    }
    return rtn;
}

int
setPtrParms(ParmList p, void **array, int n)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<n; i++){
	if (ok=setPtrParm(p, array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

/*
 * Duplicates string into new memory
 */
int
setStrParm(ParmList p, char *value, int elem)
{
    char *s;
    int rtn = 0;
    if (p
	&& p[PL_TYPEOFFSET].ival == PL_STRING
	&& p[PL_SIZEOFFSET].ival > elem
	&& elem >= 0)
    {
	s = strdup(value);
	if (s){
	    p[PL_DATAOFFSET+elem].pval = s;
	    rtn = 1;
	}
    }
    return rtn;
}

int
setStrParms(ParmList p, char **array, int n)
{
    int rtn = 0;
    int ok = 1;
    int i;

    for (i=0; ok && i<n; i++){
	if (ok=setStrParm(p, array[i], i)){
	    rtn++;
	}
    }
    return rtn;
}

int
typeofParm(ParmList p)
{
    int rtn = 0;
    if (p){
	rtn = p[PL_TYPEOFFSET].ival;
    }
    return rtn;
}

