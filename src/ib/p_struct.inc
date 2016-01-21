/* Copyright (c) Varian Assoc., Inc.  All Rights Reserved. */

/* 
 * Varian, Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

/* union for holding string or double parameter values */
typedef union {
   double *dbl;
   char  **str;
} STRDBL;

/* all information about a Vnmr parameter */
typedef struct {
   double minval;
   double maxval;
   double step;
   char  *name;
   STRDBL value;
   STRDBL enum_value;
   int    prot;
   int    intptr;
   short  active;
   short  Dgroup;
   short  Ggroup;
   short  basictype;
   short  size;
   short  enum_size;
   short  subtype;
} PARAM;

/* header structure for a parameter set */
typedef struct {
   char   *p_name;
   Storage p_set;
} PARAM_SET;
