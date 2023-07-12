/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	tools.h
|
|	These are the declartions for the tools.c module
|
+-----------------------------------------------------------------------------*/
#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <sys/stat.h>

#define NO_FIRST_FILE	-4
#define NO_SECOND_FILE	-2
#define SIZE_MISMATCH	-3

extern const char   *msg(const char *m);
extern char   *newString(const char *p);
extern char   *newStringId(const char *p, const char *id);
extern char   *newCat(char *a, const char *b);
extern char   *newCatId(char *a, const char *b, const char *id);
extern char   *newCatDontTouch(const char *a, const char *b);
extern char   *newCatIdDontTouch(const char *a, const char *b, const char *id);
extern char   *intString(int i);
extern char   *rtoa(double d, char *buf, int size);
extern char   *realString(double d);
extern double  stringReal(char *s);
extern int     isReal(char *s);
extern int     isFinite(char *s);
extern void    space(FILE *f, int n);
extern int     verify_fname(char *fnptr);
extern int     verify_fname2(char *fnptr);

extern int     isHardLink(char *lptr);
extern int     isSymLink(char *lptr);
extern int     copyFile(const char *fromFile, const char *toFile, mode_t mode);
extern int     copy_file_verify(char *file_a, char *file_b );
extern int     make_copy_fidfile(char *prog, char *dir, char *msg);
extern char *strwrd(char *s, char *buf, size_t len, char *delim);

#endif
