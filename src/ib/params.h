/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID


#else (not) HEADER_ID

#ifndef PARAMS_H_DEFINED
#define PARAMS_H_DEFINED

/* DEFINITIONS */

typedef void *ParamSet; /* Pointer to a "parameter set" */

#define MAXSTR  1024    /* Maximum length of string values, including EOS. */

/* parameter basic types */
#define T_UNDEF    0
#define T_REAL     1    /* Indicates parameter basictype is double */
#define T_STRING   2    /* Indicates parameter basictype is string */

/* parameter sub-types */
#define ST_UNDEF     0
#define ST_REAL      1
#define ST_STRING    2
#define ST_DELAY     3
#define ST_FLAG      4
#define ST_FREQUENCY 5
#define ST_PULSE     6
#define ST_INTEGER   7

/*  group definitions */
#define G_ALL           0
#define G_SAMPLE        1
#define G_ACQUISITION   2
#define G_PROCESSING    3
#define G_DISPLAY       4
#define G_SPIN          5

/* display groups */
#define D_ALL           0
#define D_ACQUISITION   1
#define D_2DACQUISITION 2
#define D_SAMPLE        3
#define D_DECOUPLING    4
#define D_AFLAGS        5
#define D_PROCESSING    6
#define D_SPECIAL       7
#define D_DISPLAY       8
#define D_REFERENCE     9
#define D_PHASE        10
#define D_CHART        11
#define D_2DDISPLAY    12
#define D_INTEGRAL     13
#define D_DFLAGS       14
#define D_FID          15
#define D_SHIMCOILS    16
#define D_AUTOMATION   17

#define D_NUMBERS      24
#define D_STRINGS      25

#define ACT_OFF         0
#define ACT_ON          1

/* parameter protection bits */
#define P_ARR     1   /* bit 0  - cannot array the parameter */
#define P_ACT     2   /* bit 1  - cannot change active/not active status */
#define P_VAL     4   /* bit 2  - cannot change the parameter value */
#define P_MAC     8   /* bit 3  - causes macro to be executed */
#define P_REX    16   /* bit 4  - avoids automatic re-display */
#define P_DEL    32   /* bit 5  - cannot delete parameter */
#define P_CRT    64   /* bit 6  - cannot recreate parameter */
#define P_CPY   128   /* bit 7  - cannot copy parameter from tree to tree */
#define P_LIM   256   /* bit 8  - cannot set parameter max, min, or step */
#define P_ENU   512   /* bit 9  - cannot set parameter enumeral values */
#define P_GRP  1024   /* bit 10 - cannot change the parameter's group */
#define P_PRO  2048   /* bit 11 - cannot change protection bits */
#define P_DGP  4096   /* bit 12 - cannot change the display group */
#define P_MMS  8192   /* bit 13 - lookup min, max, step values in table */

/* error return codes for the parameter functions */
#define P_OK    0     /* operation succeeded */

#define BASE    EOF-1    /* base for error codes: must not conflict with EOF */

#define P_BAD   BASE-0   /* pointer to parameter set is (ParamSet)NULL */
#define P_EMT   BASE-1   /* requested parameter set is empty */
#define P_READ  BASE-2   /* error occurred while reading parameter file */
#define P_WRITE BASE-3   /* error occurred while writing parameter file */
#define P_DERR  BASE-4   /* error occurred while deleting an entry */
#define P_PARM  BASE-5   /* pointer to parameter structure is (PARAM *)NULL */
#define P_SRCH  BASE-6   /* requested parameter is not in the parameter set */
#define P_MEM   BASE-7   /* can't allocate memory for parameter structures */
#define P_TYPE  BASE-8   /* requested parameter has illegal type */
#define P_SUBT  BASE-9   /* invalid subtype value */
#define P_INS   BASE-10  /* couldn't save parameter info struct in storage */
#define P_INDX  BASE-11  /* value array index out of range [1...size] */
#define P_GGRP  BASE-12  /* invalid Ggroup value */
#define P_DGRP  BASE-13  /* invalid Dgroup value */

#if defined ( __STDC__ ) && ! defined ( __cplusplus )
#define PROTO
#define EXTRN extern
#endif

#ifdef __cplusplus
#define PROTO
#define EXTRN extern "C"
#endif

#ifdef PROTO

EXTRN ParamSet PS_init (void);
EXTRN ParamSet PS_read (char *filename, ParamSet params);
EXTRN int      PS_insert (ParamSet params, char *pname, int type);
EXTRN int      PS_exist (ParamSet params, char *pname);
EXTRN int      PS_size (ParamSet params, char *pname, int *size);
EXTRN int      PS_enum_size (ParamSet params, char *pname, int *enum_size);
EXTRN int      PS_type (ParamSet params, char *pname, int *type);
EXTRN int      PS_set_subtype (ParamSet params, char *pname, int subtype);
EXTRN int      PS_set_active (ParamSet params, char *pname, int active);
EXTRN int      PS_set_protect (ParamSet params, char *pname, int protect);
EXTRN int      PS_set_real_value (ParamSet params, char *pname, int index,
                                  double value);
EXTRN int      PS_set_string_value (ParamSet params, char *pname, int index,
                                    char *value);
EXTRN int      PS_set_real_evalue (ParamSet params, char *pname, int index,
                                   double evalue);
EXTRN int      PS_set_string_evalue (ParamSet params, char *pname, int index,
                                     char *evalue);
EXTRN int      PS_set_G_group (ParamSet params, char *pname, int G_group);
EXTRN int      PS_set_D_group (ParamSet params, char *pname, int D_group);
EXTRN int      PS_set_minval (ParamSet params, char *pname, double minval);
EXTRN int      PS_set_maxval (ParamSet params, char *pname, double maxval);
EXTRN int      PS_set_step (ParamSet params, char *pname, double step);
EXTRN int      PS_get_subtype (ParamSet params, char *pname, int *subtype);
EXTRN int      PS_get_active (ParamSet params, char *pname, int *active);
EXTRN int      PS_get_protect (ParamSet params, char *pname, int *protect);
EXTRN int      PS_get_real_value (ParamSet params, char *pname, int index,
                                  double *value);
EXTRN int      PS_get_string_value (ParamSet params, char *pname, int index,
                                    char **value);
EXTRN int      PS_get_real_evalue (ParamSet params, char *pname, int index,
                                   double *evalue);
EXTRN int      PS_get_string_evalue (ParamSet params, char *pname, int index,
                                     char **evalue);
EXTRN int      PS_get_G_group (ParamSet params, char *pname, int *G_group);
EXTRN int      PS_get_D_group (ParamSet params, char *pname, int *D_group);
EXTRN int      PS_get_minval (ParamSet params, char *pname, double *minval);
EXTRN int      PS_get_maxval (ParamSet params, char *pname, double *maxval);
EXTRN int      PS_get_step (ParamSet params, char *pname, double *step);
EXTRN int      PS_delete (ParamSet params, char *pname);
EXTRN int      PS_write (char *filename, ParamSet params);
EXTRN int      PS_clear (ParamSet params);
EXTRN int      PS_release (ParamSet params);

#else

ParamSet PS_init();
ParamSet PS_read();
int      PS_insert();
int      PS_exist();
int      PS_size();
int      PS_enum_size();
int      PS_type();
int      PS_set_subtype();
int      PS_set_active();
int      PS_set_protect();
int      PS_set_real_value();
int      PS_set_string_value();
int      PS_set_real_evalue();
int      PS_set_string_evalue();
int      PS_set_G_group();
int      PS_set_D_group();
int      PS_set_minval();
int      PS_set_maxval();
int      PS_set_step();
int      PS_get_subtype();
int      PS_get_active();
int      PS_get_protect();
int      PS_get_real_value();
int      PS_get_string_value();
int      PS_get_real_evalue();
int      PS_get_string_evalue();
int      PS_get_G_group();
int      PS_get_D_group();
int      PS_get_minval();
int      PS_get_maxval();
int      PS_get_step();
int      PS_delete();
int      PS_write();
int      PS_clear();
int      PS_release();

#endif
#endif
#endif HEADER_ID
