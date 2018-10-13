/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID

#ifndef lint
	static char STORAGE_H_ID[] = "@(#)storage.h 18.1 03/21/08 20:01:55 Copyright 1991 Spectroscopy Imaging Systems";
#endif

#else

#ifndef STORAGE_H_DEFINED
#define STORAGE_H_DEFINED

typedef void *Storage;

/* flags for the "Storage_insert" function */
#define REPLACE 1    /* replace previous data in duplicate key with new data */
#define REPORT  2    /* report existence of duplicate key, but don't replace */

/* error return codes for the storage functions */
#define S_OK     0  /* operation succeeded */
#define S_ACT   -1  /* invalid value for "action": not REPORT or REPLACE */
#define S_BAD   -2  /* the reference to the storage area is (Storage)NULL */
#define S_DEL   -3  /* no delete function has been supplied */
#define S_DERR  -4  /* user-supplied "delete" function returned an error */
#define S_EMT   -5  /* requested storage area is empty */
#define S_INS   -6  /* memory couldn't be allocated for storing the data */
#define S_REPL  -7  /* "action" is REPLACE and error occurred replacing data */
#define S_RPT   -8  /* "action" is REPORT and "key" is already in storage */
#define S_SHO   -9  /* no output function was specified */
#define S_SRCH -10  /* requested key not found in the storage area */

/* functions defined in the "Storage" module */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

Storage Storage_create (unsigned size, int (*compare)(const char *key1, const char *key2),
                        int (*delete)(void *key, void *data) );
int     Storage_insert (Storage store, void *key, void *data, int action);
void   *Storage_search (Storage store, void *key);
void   *Storage_first (Storage store);
void   *Storage_next (Storage store);
int     Storage_delete (Storage store, void *key);
int     Storage_clear  (Storage store);
int     Storage_release (Storage store);
int     Storage_output (Storage store, int (*output)(void *key, void *data) );
#ifdef DEBUG
int     Storage_display (Storage store);
#endif

#else

Storage Storage_create( /* unsigned size, int (*compare)(), int (*delete)() */ );
int     Storage_insert( /* Storage store, void *key, void *data, int action */ );
void   *Storage_search( /* Storage store, void *key */ );
void   *Storage_first( /* Storage store */ );
void   *Storage_next( /* Storage store */ );
int     Storage_delete( /* Storage store, void *key */ );
int     Storage_clear( /* Storage store */ );
int     Storage_release( /* Storage store */ );
int     Storage_output( /* Storage store, int (*output)() */ );
#ifdef DEBUG
int     Storage_display( /* Storage store */ );
#endif

#endif
#endif
#endif
