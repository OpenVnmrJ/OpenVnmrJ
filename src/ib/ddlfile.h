/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
// @(#)ddlfile.h 18.1 03/21/08 (c)1992 SISCO

extern int ValidMagicNumber(char* filename, char *magic_string_list[]);
Imginfo *ddldata_load(char *filename, char *errmsg);

extern char *magic_string_list[];
