/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <ctype.h>
#include <errno.h>
#ifndef __STDC__
#include <math.h>
#endif
#include <string.h>
#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif

/* these includes pick up the SID strings in the header files */
#define HEADER_ID
#include "boolean.h"
#include "error.h"
#include "params.h"
#include "storage.h"
#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif
#ifdef CLOCKTIME
#include "clocktime.h"
#endif
#undef HEADER_ID

#include "boolean.h"
#include "error.h"
#include "storage.h"
#include "params.h"
#include "p_struct.inc"

#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif

#ifdef CLOCKTIME
#include "clocktime.h"
#endif

/* global variables for this module */

static char *P_filename;             /* name of parameter file */
static FILE *P_fileptr;              /* file pointer for parameter file */
static char *P_parmname;             /* name of current parameter */
static char  P_noname[] = "(null)";  /* when "P_parmname" not available */

static int   P_error = P_OK;         /* error code for current operation */

#define REC_ERR BASE-50  /* Recoverable (record) error while reading entry */
#define SYN_ERR BASE-51  /* Recoverable (syntax) error while reading entry */
#define BAD_ERR BASE-52  /* Unrecoverable error while reading entry */

static char typ_msg[] =
            "%s unknown basic type %d for parameter '%s' in file '%s';";
#ifdef DEBUG
static char und_msg[] =
            "%s parameter '%s' in file '%s' has values of undefined type;";
#endif
static char not_msg[] = "parameter values were NOT %s";

/* functions local to this module */

#ifdef __STDC__

static int    P_delete (void *name, void *info);
static int    P_write (void *name, void *info);
static int    P_read_param (FILE *file_ptr, PARAM **pp);
static int    P_get_param_line (FILE *file_ptr, char *buffer, int size);
static char  *P_get_param_name (char *buffer, char **p_next);
static STRDBL P_get_param_values (FILE *file_ptr, char *buffer, char *p_buff,
						short type, short *size);
static char *P_get_value_string (FILE *file_ptr, char *buffer, char *p_buff, 
				char *o_buff, unsigned *length);
static int    P_write_param (FILE *file_ptr, PARAM *p);
static int    P_put_param_values (FILE *file_ptr, short size, short type,
                                  STRDBL value);
static int    P_free_param (PARAM *p);
static int    P_free_param_values (short basictype, short size, STRDBL value);

#else

static int    P_delete();
static int    P_write();
static int    P_read_param();
static int    P_get_param_line();
static char  *P_get_param_name();
static STRDBL P_get_param_values();
static char  *P_get_value_string();
static int    P_write_param();
static int    P_put_param_values();
static int    P_free_param();
static int    P_free_param_values();

#endif

#ifdef __STDC__
ParamSet PS_init (void)
#else
ParamSet PS_init()
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set to be created.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_init:";
#endif
   PARAM_SET *ps;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   /* allocate space for a parameter-set header */
   if ( (ps = (PARAM_SET *)calloc (1, sizeof (PARAM_SET))) == (PARAM_SET *)NULL)
   {
      SYS_ERROR(("%s calloc header for VNMR parameter set",sub_msg));
   }
   /* create the parameter-set storage area for this file */
   else if ( (ps->p_set = Storage_create (211,strcmp,P_delete)) == (Storage)NULL)
   {
      ERROR(("%s can't create storage area for VNMR parameter set",sub_msg));

      /* free the header */
      (void)free (ps);

      ps = (PARAM_SET *)NULL;
   }
   return ((ParamSet)ps);

}  /* end of function "PS_init" */

#ifdef __STDC__
ParamSet PS_read (char *filename, ParamSet params)
#else
ParamSet PS_read (filename, params)
 char    *filename;
 ParamSet params;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg  The name of this function, for error messages.
   fp       A file pointer for the input file.
   ps       A pointer to the "parameter set".
   p        A pointer to a parameter information structure, returned by the
            file reader function.
   res      The result of the load operation.
   */
   static char sub_msg[] = "PS_read:";
   FILE       *fp;
   PARAM_SET  *ps;
   PARAM      *p;
   int         res;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   /* set the pointer to the parameter set */
   ps = (PARAM_SET *)params;

   /* set a name for the VNMR parameter input file */
   if (filename == (char *)NULL || !isprint(*filename) )
   {
      filename = (ps != (PARAM_SET *)NULL && ps->p_name != (char *)NULL) ?
                 ps->p_name : "stdin";
   }
   /* open the VNMR parameter file for reading */
   if (strcmp (filename, "stdin") == 0)
   {
      fp = stdin;
   }
   else if ( (fp = fopen (filename, "r")) == (FILE *)NULL)
   {
      SYS_ERROR(("%s fopen VNMR parameter file '%s'",sub_msg,filename));
      return ((ParamSet)NULL);
   }
   /* if no "parameter set" was specified, create one */
   if (ps == (PARAM_SET *)NULL)
   {
      if ( (ps = (PARAM_SET *)PS_init()) == (PARAM_SET *)NULL)
      {
         (void)fclose (fp);
         return ((ParamSet)NULL);
      }
   }
   /* update the file name in the "parameter set" */
   if (ps->p_name == (char *)NULL)
   {
      ps->p_name = (char *)malloc (strlen (filename) + 1);
#ifdef DEBUG
      if (ps->p_name == (char *)NULL)
         sys_error ("%s malloc space for VNMR parameter file name '%s'",
                    sub_msg,filename);
#endif
   }
   else if (strlen(filename) > strlen(ps->p_name))
   {
      ps->p_name = (char *)realloc (ps->p_name, strlen (filename) + 1);
#ifdef DEBUG
      if (ps->p_name == (char *)NULL)
         sys_error ("%s realloc space for VNMR parameter file name '%s'",
                    sub_msg,filename);
#endif
   }
   if (ps->p_name != (char *)NULL)
      (void)strcpy (ps->p_name, filename);

   /* set the file name for error messages in this module */
   P_filename = ps->p_name;

#ifdef CLOCKTIME
  if (init_timer (1) != 0)
  {
     fprintf (stderr, "'init_timer (1)' failed\n");
     exit (1);
  }
  /* Turn on a clocktime timer */
  (void)start_timer ( 0 );
#endif

   /* load the parameters into the storage area */
   while ( (res = P_read_param (fp, &p)) != EOF)
   {
      /* continue reading on minor (recoverable) errors */
      if (res == REC_ERR)
         continue;

      /* stop reading if a serious error occurred */
      else if (res != P_OK && res != SYN_ERR)
      {
         error ("%s not all parameters read from file '%s'", sub_msg, filename);
         break;
      }
      /* if a parameter entry was read, put it into the parameter set */
      if (p != (PARAM *)NULL)
      {
#ifdef FUBAR
         ERROR(("%s: basictype %d, subtype %d, size %d, enum_size %d",
                p->name, p->basictype, p->subtype, p->size, p->enum_size));
#endif
         if (Storage_insert (ps->p_set, (void *)p->name, (void *)p, REPLACE) !=
             S_OK)
         {
            error ("%s can't store VNMR parameter '%s' from file '%s'",
                   sub_msg, p->name, filename);
         }
      }
   }
#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer (0);

  if (report_timer ("PS_read.out", filename) != 0)
     fprintf (stderr, "\"report_timer (\"PS_read.out\", \"%s\")\" failed\n",
              filename);
#endif

   /* close the parameter file */
   if (fp != stdin)
      (void)fclose (fp);

   /* return the pointer to the header */
   return ((ParamSet)ps);

}  /* end of function "PS_read" */

#ifdef __STDC__
int PS_insert (ParamSet params, char *pname, int type)
#else
int PS_insert (params, pname, type)
 ParamSet params;
 char    *pname;
 int      type;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to parameter set which contains parameter to be created.
   p        A pointer to parameter to be created.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_insert:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if (type != T_UNDEF && type != T_REAL && type != T_STRING)
   {
      ERROR(("%s illegal type %d for parameter '%s'",sub_msg,type,pname));
      P_error = P_TYPE;
   }
   /* get space for a parameter information structure */
   else if ( (p = (PARAM *)calloc (1, sizeof(PARAM))) == (PARAM *)NULL)
   {
      SYS_ERROR(("%s calloc info struct for parameter '%s'",sub_msg,pname));
      P_error = errno;
   }
   /* get memory to store the name string */
   else if ( (p->name = (char *)malloc (strlen(pname) + 1)) == (char*)NULL)
   {
      SYS_ERROR(("%s malloc memory for parameter '%s' name",sub_msg,pname));
      P_error = errno;
   }
   else
   {
      /* copy the name string into storage */
      (void)strcpy (p->name, pname);

      /* set the parameter type */
      p->basictype = (short)type;

      /* put this parameter into the storage area */
      if (Storage_insert (ps->p_set, (void *)pname, (void *)p, REPLACE) != S_OK)
      {
         ERROR(("%s can't store VNMR parameter '%s'",sub_msg,pname));
         P_error = P_INS;
      }
   }
   return (P_error);

}  /* end of function "PS_insert" */

#ifdef __STDC__
int PS_exist (ParamSet params, char *pname)
#else
int PS_exist (params, pname)
 ParamSet params;
 char    *pname;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   res   The result of the search operation.
   */
   PARAM_SET *ps;
   int        res = FALSE;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if (Storage_search (ps->p_set, (void *)pname) != (void *)NULL)
   {
      res = TRUE;
   }
   return (res);

}  /* end of function "PS_exist" */

#ifdef __STDC__
int PS_size (ParamSet params, char *pname, int *size)
#else
int PS_size (params, pname, size)
 ParamSet params;
 char    *pname;
 int     *size;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *size = (int)p->size;
   }
   return (P_error);

}  /* end of function "PS_size" */

#ifdef __STDC__
int PS_enum_size (ParamSet params, char *pname, int *enum_size)
#else
int PS_enum_size (params, pname, enum_size)
 ParamSet params;
 char    *pname;
 int     *enum_size;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *enum_size = (int)p->enum_size;
   }
   return (P_error);

}  /* end of function "PS_enum_size" */

#ifdef __STDC__
int PS_type (ParamSet params, char *pname, int *type)
#else
int PS_type (params, pname, type)
 ParamSet params;
 char    *pname;
 int     *type;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *type = (int)p->basictype;
   }
   return (P_error);

}  /* end of function "PS_type" */

#ifdef __STDC__
int PS_set_subtype (ParamSet params, char *pname, int subtype)
#else
int PS_set_subtype (params, pname, subtype)
 ParamSet params;
 char    *pname;
 int      subtype;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      switch (subtype)
      {
         case ST_UNDEF:
         case ST_REAL:
         case ST_STRING:
         case ST_DELAY:
         case ST_FLAG:
         case ST_FREQUENCY:
         case ST_PULSE:
         case ST_INTEGER:
            p->subtype = (short)subtype;
            break;
         default:
            ERROR(("PS_set_subtype: invalid subtype value %d",subtype));
            P_error = P_SUBT;
            break;
      }
   }
   return (P_error);

}  /* end of function "PS_set_subtype" */

#ifdef __STDC__
int PS_set_active (ParamSet params, char *pname, int active)
#else
int PS_set_active (params, pname, active)
 ParamSet params;
 char    *pname;
 int      active;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      p->active = (short)active;
   }
   return (P_error);

}  /* end of function "PS_set_active" */

#ifdef __STDC__
int PS_set_protect (ParamSet params, char *pname, int protect)
#else
int PS_set_protect (params, pname, protect)
 ParamSet params;
 char    *pname;
 int      protect;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      p->prot = protect;
   }
   return (P_error);

}  /* end of function "PS_set_protect" */

#ifdef __STDC__
int PS_set_real_value (ParamSet params, char *pname, int index, double value)
#else
int PS_set_real_value (params, pname, index, value)
 ParamSet params;
 char    *pname;
 int      index;
 double   value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_set_real_value:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   /* check the requested value index */
   else if (index < 1)
   {
      P_error = P_INDX;
   }
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter is not a string type */
   else if (p->basictype == T_STRING)
   {
      P_error = P_TYPE;
   }
   else
   {
      /* for an undefined type, set the type to real */
      if (p->basictype == T_UNDEF)
      {
#ifdef DEBUG
         if (p->value.dbl != (double *)NULL)
            error ("%s parameter '%s' value type undefined",sub_msg,pname);
#endif
         p->basictype = T_REAL;
         p->subtype   = ST_REAL;
      }
      if (index > p->size)
      {
         int i;

         /* if there is no array of values, allocate one */
         if (p->value.dbl == (double *)NULL)
         {
            if ( (p->value.dbl = (double *)malloc (
                       (unsigned)(index) * sizeof(double))) == (double *)NULL)
            {
               SYS_ERROR(("%s malloc space for %d values",sub_msg,index));
               return (P_error = P_MEM);
            }
         }
         /* otherwise, resize the array (realloc preserves the contents) */
         else if ( (p->value.dbl = (double *)realloc (p->value.dbl,
                       (unsigned)(index * sizeof(double)))) == (double *)NULL)
         {
            SYS_ERROR(("%s realloc value size from %d to %d",
                       sub_msg,p->size,index));
            return (P_error = P_MEM);
         }
         /* clear all the new values and set the new size */
         for (i = p->size; i < index - 1; ++i)
            p->value.dbl[i] = 0.0;
         p->size = index;
      }
      p->value.dbl [--index] = value;
   }
   return (P_error);

}  /* end of function "PS_set_real_value" */

#ifdef __STDC__
int PS_set_string_value (ParamSet params, char *pname, int index, char *value)
#else
int PS_set_string_value (params, pname, index, value)
 ParamSet params;
 char    *pname;
 int      index;
 char    *value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   len      The length of the input string value, for storing it.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_set_string_value:";
#endif
   PARAM_SET *ps;
   PARAM     *p;
   unsigned   len;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   /* check the requested value index */
   else if (index < 1)
   {
      P_error = P_INDX;
   }
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter is not a real type */
   else if (p->basictype == T_REAL)
   {
      P_error = P_TYPE;
   }
   else
   {
      /* for an undefined type, set the type to string */
      if (p->basictype == T_UNDEF)
      {
#ifdef DEBUG
         if (p->value.str != (char **)NULL)
            error ("%s parameter '%s' value type undefined",sub_msg,pname);
#endif
         p->basictype = T_STRING;
         p->subtype   = ST_STRING;
      }
      if (index > p->size)
      {
         int i;

         /* if there is no array of values, allocate one */
         if (p->value.str == (char **)NULL)
         {
            if ( (p->value.str = (char **)malloc (
                       (unsigned)(index * sizeof(char *)))) == (char **)NULL)
            {
               SYS_ERROR(("%s malloc space for %d values",sub_msg,index));
               return (P_error = P_MEM);
            }
         }
         /* otherwise, resize the array (realloc preserves the contents) */
         else if ( (p->value.str = (char **)realloc (p->value.str,
                       (unsigned)(index * sizeof(char *)))) == (char **)NULL)
         {
            SYS_ERROR(("%s realloc value size from %d to %d",
                       sub_msg,p->size,index));
            return (P_error = P_MEM);
         }
         /* clear all the new values and set the new size */
         for (i = p->size; i < index; ++i)
            p->value.str[i] = (char *)NULL;
         p->size = index;
      }
      /* set the length of the value, to prepare for storing it */
      len = strlen (value);

      /* store the string */
      if (p->value.str [--index] == (char *)NULL)
      {
         if ( (p->value.str[index] = (char *)malloc (len+1)) == (char *)NULL)
         {
            SYS_ERROR(("%s malloc space for value '%s'",sub_msg,value));
            return (P_error = P_MEM);
         }
      }
      else if (len > strlen (p->value.str[index]) )
      {
         if ( (p->value.str[index] =
               (char *)realloc (p->value.str[index], len+1)) == (char *)NULL)
         {
            SYS_ERROR(("%s realloc space for value '%s'",sub_msg,value));
            return (P_error = P_MEM);
         }
      }
      (void)strcpy (p->value.str[index], value);
   }
   return (P_error);

}  /* end of function "PS_set_string_value" */

#ifdef __STDC__
int PS_set_real_evalue (ParamSet params, char *pname, int index, double evalue)
#else
int PS_set_real_evalue (params, pname, index, evalue)
 ParamSet params;
 char    *pname;
 int      index;
 double   evalue;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_set_real_evalue:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   /* check the requested value index */
   else if (index < 1)
   {
      P_error = P_INDX;
   }
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter is not a string type */
   else if (p->basictype == T_STRING)
   {
      P_error = P_TYPE;
   }
   else
   {
      /* for an undefined type, set the type to real */
      if (p->basictype == T_UNDEF)
      {
#ifdef DEBUG
         if (p->enum_value.dbl != (double *)NULL)
            error ("%s parameter '%s' value type undefined",sub_msg,pname);
#endif
         p->basictype = T_REAL;
         p->subtype   = ST_REAL;
      }
      if (index > p->enum_size)
      {
         int i;

         /* if there is no array of values, allocate one */
         if (p->enum_value.dbl == (double *)NULL)
         {
            if ( (p->enum_value.dbl = (double *)malloc (
                       (unsigned)(index) * sizeof(double))) == (double *)NULL)
            {
               SYS_ERROR(("%s malloc space for %d values",sub_msg,index));
               return (P_error = P_MEM);
            }
         }
         /* otherwise, resize the array (realloc preserves the contents) */
         else if ( (p->enum_value.dbl = (double *)realloc (p->enum_value.dbl,
                       (unsigned)(index * sizeof(double)))) == (double *)NULL)
         {
            SYS_ERROR(("%s realloc value size from %d to %d",
                       sub_msg,p->size,index));
            return (P_error = P_MEM);
         }
         /* clear all the new values and set the new size */
         for (i = p->size; i < index - 1; ++i)
            p->enum_value.dbl[i] = 0.0;
         p->size = index;
      }
      p->enum_value.dbl [--index] = evalue;
   }
   return (P_error);

}  /* end of function "PS_set_real_evalue" */

#ifdef __STDC__
int PS_set_string_evalue (ParamSet params, char *pname, int index, char *evalue)
#else
int PS_set_string_evalue (params, pname, index, evalue)
 ParamSet params;
 char    *pname;
 int      index;
 char    *evalue;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   len      The length of the input string value, for storing it.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_set_string_evalue:";
#endif
   PARAM_SET *ps;
   PARAM     *p;
   unsigned   len;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   /* check the requested value index */
   else if (index < 1)
   {
      P_error = P_INDX;
   }
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter is not a real type */
   else if (p->basictype == T_REAL)
   {
      P_error = P_TYPE;
   }
   else
   {
      /* for an undefined type, set the type to string */
      if (p->basictype == T_UNDEF)
      {
#ifdef DEBUG
         if (p->enum_value.str != (char **)NULL)
            error ("%s parameter '%s' value type undefined",sub_msg,pname);
#endif
         p->basictype = T_STRING;
         p->subtype   = ST_STRING;
      }
      if (index > p->enum_size)
      {
         int i;

         /* if there is no array of values, allocate one */
         if (p->enum_value.str == (char **)NULL)
         {
            if ( (p->enum_value.str = (char **)malloc (
                       (unsigned)(index * sizeof(char *)))) == (char **)NULL)
            {
               SYS_ERROR(("%s malloc space for %d values",sub_msg,index));
               return (P_error = P_MEM);
            }
         }
         /* otherwise, resize the array (realloc preserves the contents) */
         else if ( (p->enum_value.str = (char **)realloc (p->enum_value.str,
                       (unsigned)(index * sizeof(char *)))) == (char **)NULL)
         {
            SYS_ERROR(("%s realloc value size from %d to %d",
                       sub_msg,p->size,index));
            return (P_error = P_MEM);
         }
         /* clear all the new values and set the new size */
         for (i = p->size; i < index; ++i)
            p->enum_value.str[i] = (char *)NULL;
         p->size = index;
      }
      /* set the length of the value, to prepare for storing it */
      len = strlen (evalue);

      /* store the string */
      if (p->enum_value.str [--index] == (char *)NULL)
      {
         if ( (p->enum_value.str[index] = (char *)malloc(len+1)) == (char *)NULL)
         {
            SYS_ERROR(("%s malloc space for value '%s'",sub_msg,evalue));
            return (P_error = P_MEM);
         }
      }
      else if (len > strlen (p->enum_value.str[index]) )
      {
         if ( (p->enum_value.str[index] =
             (char *)realloc (p->enum_value.str[index], len+1)) == (char *)NULL)
         {
            SYS_ERROR(("%s realloc space for value '%s'",sub_msg,evalue));
            return (P_error = P_MEM);
         }
      }
      (void)strcpy (p->enum_value.str[index], evalue);
   }
   return (P_error);

}  /* end of function "PS_set_string_evalue" */

#ifdef __STDC__
int PS_set_G_group (ParamSet params, char *pname, int G_group)
#else
int PS_set_G_group (params, pname, G_group)
 ParamSet params;
 char    *pname;
 int      G_group;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      switch (G_group)
      {
         case G_ALL:
         case G_SAMPLE:
         case G_ACQUISITION:
         case G_PROCESSING:
         case G_DISPLAY:
         case G_SPIN:
            p->Ggroup = (short)G_group;
            break;
         default:
            ERROR(("PS_set_G_group: invalid G_group value %d",G_group));
            P_error = P_GGRP;
            break;
      }
   }
   return (P_error);

}  /* end of function "PS_set_G_group" */

#ifdef __STDC__
int PS_set_D_group (ParamSet params, char *pname, int D_group)
#else
int PS_set_D_group (params, pname, D_group)
 ParamSet params;
 char    *pname;
 int      D_group;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      switch (D_group)
      {
         case D_ALL:
         case D_ACQUISITION:
         case D_2DACQUISITION:
         case D_SAMPLE:
         case D_DECOUPLING:
         case D_AFLAGS:
         case D_PROCESSING:
         case D_SPECIAL:
         case D_DISPLAY:
         case D_REFERENCE:
         case D_PHASE:
         case D_CHART:
         case D_2DDISPLAY:
         case D_INTEGRAL:
         case D_DFLAGS:
         case D_FID:
         case D_SHIMCOILS:
         case D_AUTOMATION:
         case D_NUMBERS:
         case D_STRINGS:
            p->Dgroup = (short)D_group;
            break;
         default:
            ERROR(("PS_set_D_group: invalid D_group value %d",D_group));
            P_error = P_DGRP;
            break;
      }
   }
   return (P_error);

}  /* end of function "PS_set_D_group" */

#ifdef __STDC__
int PS_set_minval (ParamSet params, char *pname, double minval)
#else
int PS_set_minval (params, pname, minval)
 ParamSet params;
 char    *pname;
 double   minval;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      p->minval = minval;
   }
   return (P_error);

}  /* end of function "PS_set_minval" */

#ifdef __STDC__
int PS_set_maxval (ParamSet params, char *pname, double maxval)
#else
int PS_set_maxval (params, pname, maxval)
 ParamSet params;
 char    *pname;
 double   maxval;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      p->maxval = maxval;
   }
   return (P_error);

}  /* end of function "PS_set_maxval" */

#ifdef __STDC__
int PS_set_step (ParamSet params, char *pname, double step)
#else
int PS_set_step (params, pname, step)
 ParamSet params;
 char    *pname;
 double   step;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to set.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      p->step = step;
   }
   return (P_error);

}  /* end of function "PS_set_step" */

#ifdef __STDC__
int PS_get_subtype (ParamSet params, char *pname, int *subtype)
#else
int PS_get_subtype (params, pname, subtype)
 ParamSet params;
 char    *pname;
 int     *subtype;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *subtype = (int)p->subtype;
   }
   return (P_error);

}  /* end of function "PS_get_subtype" */

#ifdef __STDC__
int PS_get_active (ParamSet params, char *pname, int *active)
#else
int PS_get_active (params, pname, active)
 ParamSet params;
 char    *pname;
 int     *active;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *active = (int)p->active;
   }
   return (P_error);

}  /* end of function "PS_get_active" */

#ifdef __STDC__
int PS_get_protect (ParamSet params, char *pname, int *protect)
#else
int PS_get_protect (params, pname, protect)
 ParamSet params;
 char    *pname;
 int     *protect;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *protect = p->prot;
   }
   return (P_error);

}  /* end of function "PS_get_protect" */

#ifdef __STDC__
int PS_get_real_value (ParamSet params, char *pname, int index, double *value)
#else
int PS_get_real_value (params, pname, index, value)
 ParamSet params;
 char    *pname;
 int      index;
 double  *value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_get_real_value:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
#ifdef DEBUG
   else if (value == (double *)NULL)
   {
      error ("%s NULL value pointer",sub_msg);
      P_error = P_BAD;
   }
#endif
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set,(void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter has the correct type */
   else if (p->basictype != T_REAL)
   {
      P_error = P_TYPE;
   }
   /* check the requested value index */
   else if (index < 1 || index > p->size)
   {
      P_error = P_INDX;
   }
   else
   {
      *value = p->value.dbl [--index];
   }
   return (P_error);

}  /* end of function "PS_get_real_value" */

#ifdef __STDC__
int PS_get_string_value (ParamSet params, char *pname, int index, char **value)
#else
int PS_get_string_value (params, pname, index, value)
 ParamSet params;
 char    *pname;
 int      index;
 char   **value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_get_string_value:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
#ifdef DEBUG
   else if (value == (char **)NULL)
   {
      error ("%s NULL value pointer",sub_msg);
      P_error = P_BAD;
   }
#endif
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set,(void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter has the correct type */
   else if (p->basictype != T_STRING)
   {
      P_error = P_TYPE;
   }
   /* check the requested value index */
   else if (index < 1 || index > p->size)
   {
      P_error = P_INDX;
   }
   else
   {
      *value = p->value.str [--index];
   }
   return (P_error);

}  /* end of function "PS_get_string_value" */

#ifdef __STDC__
int PS_get_real_evalue (ParamSet params, char *pname, int index, double *evalue)
#else
int PS_get_real_evalue (params, pname, index, evalue)
 ParamSet params;
 char    *pname;
 int      index;
 double  *evalue;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_get_real_evalue:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
#ifdef DEBUG
   else if (evalue == (double *)NULL)
   {
      error ("%s NULL value pointer",sub_msg);
      P_error = P_BAD;
   }
#endif
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set,(void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter has the correct type */
   else if (p->basictype != T_REAL)
   {
      P_error = P_TYPE;
   }
   /* check the requested value index */
   else if (index < 1 || index > p->enum_size)
   {
      P_error = P_INDX;
   }
   else
   {
      *evalue = p->enum_value.dbl [--index];
   }
   return (P_error);

}  /* end of function "PS_get_real_evalue" */

#ifdef __STDC__
int PS_get_string_evalue (ParamSet params, char *pname, int index, char **evalue)
#else
int PS_get_string_evalue (params, pname, index, evalue)
 ParamSet params;
 char    *pname;
 int      index;
 char   **evalue;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter for
            which the value is to be set.
   p        A pointer to the parameter for which the value is to be set.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_get_string_evalue:";
#endif
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
#ifdef DEBUG
   else if (evalue == (char **)NULL)
   {
      error ("%s NULL value pointer",sub_msg);
      P_error = P_BAD;
   }
#endif
   /* get the parameter from storage */
   else if ( (p = (PARAM *)Storage_search (ps->p_set,(void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   /* make sure that this parameter has the correct type */
   else if (p->basictype != T_STRING)
   {
      P_error = P_TYPE;
   }
   /* check the requested value index */
   else if (index < 1 || index > p->enum_size)
   {
      P_error = P_INDX;
   }
   else
   {
      *evalue = p->enum_value.str [--index];
   }
   return (P_error);

}  /* end of function "PS_get_string_evalue" */

#ifdef __STDC__
int PS_get_G_group (ParamSet params, char *pname, int *G_group)
#else
int PS_get_G_group (params, pname, G_group)
 ParamSet params;
 char    *pname;
 int     *G_group;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *G_group = (int)p->Ggroup;
   }
   return (P_error);

}  /* end of function "PS_get_G_group" */

#ifdef __STDC__
int PS_get_D_group (ParamSet params, char *pname, int *D_group)
#else
int PS_get_D_group (params, pname, D_group)
 ParamSet params;
 char    *pname;
 int     *D_group;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *D_group = (int)p->Dgroup;
   }
   return (P_error);

}  /* end of function "PS_get_D_group" */

#ifdef __STDC__
int PS_get_minval (ParamSet params, char *pname, double *minval)
#else
int PS_get_minval (params, pname, minval)
 ParamSet params;
 char    *pname;
 double  *minval;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *minval = p->minval;
   }
   return (P_error);

}  /* end of function "PS_get_minval" */

#ifdef __STDC__
int PS_get_maxval (ParamSet params, char *pname, double *maxval)
#else
int PS_get_maxval (params, pname, maxval)
 ParamSet params;
 char    *pname;
 double  *maxval;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *maxval = p->maxval;
   }
   return (P_error);

}  /* end of function "PS_get_maxval" */

#ifdef __STDC__
int PS_get_step (ParamSet params, char *pname, double *step)
#else
int PS_get_step (params, pname, step)
 ParamSet params;
 char    *pname;
 double  *step;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set which contains the parameter to get.
   p     A pointer to the parameter information structure.
   */
   PARAM_SET *ps;
   PARAM     *p;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else if ( (p = (PARAM *)Storage_search (ps->p_set, (void *)pname)) ==
            (PARAM *)NULL)
   {
      P_error = P_SRCH;
   }
   else
   {
      *step = p->step;
   }
   return (P_error);

}  /* end of function "PS_get_step" */

#ifdef __STDC__
int PS_delete (ParamSet params, char *pname)
#else
int PS_delete (params, pname)
 ParamSet params;
 char    *pname;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps       A pointer to the parameter set which contains the parameter to be
            deleted.
   res      The result of the write operation.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_delete:";
#endif
   PARAM_SET *ps;
   int        res;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      ERROR(("%s NULL parameter set pointer",sub_msg));
      P_error = P_BAD;
   }
   else
   {
      switch (res = Storage_delete (ps->p_set, (void *)pname))
      {
         case S_OK:
            break;
         case S_EMT:
            P_error = P_EMT;
            break;
         case S_DERR:
            P_error = P_DERR;
            break;
         case S_SRCH:
            P_error = P_SRCH;
            break;
         default:
            ERROR(("%s Storage_delete returned error %d",sub_msg,res));
         case S_BAD:
            P_error = P_BAD;
            break;
      }
   }
   return (P_error);

}  /* end of function "PS_delete" */

#ifdef __STDC__
int PS_write (char *filename, ParamSet params)
#else
int PS_write (filename, params)
 char    *filename;
 ParamSet params;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set to be written.
   fp    The file pointer for the output file.
   res   The result of the write operation.
   */
#ifdef DEBUG
   static char sub_msg[] = "PS_write:";
#endif
   PARAM_SET *ps;
   FILE      *fp = (FILE *)NULL;
   int        res;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      ERROR(("%s NULL parameter set pointer",sub_msg));
      P_error = P_BAD;
   }
   else
   {
      /* set a name for the VNMR parameter output file */
      if (filename == (char *)NULL || !isprint(*filename) )
      {
         filename = (ps->p_name!=(char *)NULL && strcmp(ps->p_name,"stdin")!=0) ?
                    ps->p_name : "stdout";
      }
      /* open the VNMR parameter file for writing */
      if (strcmp (filename, "stdout") == 0)
         fp = stdout;
      else if ( (fp = fopen (filename, "w")) == NULL)
      {
         SYS_ERROR(("%s can't open output VNMR param file '%s'",
                    sub_msg,filename));
         return (P_error = errno);
      }
      /* set the name and code for error messages in this module */
      P_error    = P_OK;
      P_filename = ps->p_name;
      P_fileptr  = fp;

      /* write the contents of this parameter set to the output file */
      switch (res = Storage_output (ps->p_set, P_write))
      {
         case S_OK:
            break;
         case S_EMT:
            P_error = P_EMT;
            break;
         default:
            ERROR(("PS_write: Storage_output returned error %d",res));
         case S_BAD:
            P_error = P_BAD;
            break;
      }
      /* close the output file */
      if (fp != stdout)
         (void)fclose (fp);
   }
   return (P_error);

}  /* end of function "PS_write" */

#ifdef __STDC__
int PS_clear (ParamSet params)
#else
int PS_clear (params)
 ParamSet params;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set to be cleared.
   res   The result of the clear operation.
   */
   PARAM_SET *ps;
   int        res;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else
   {
      switch (res = Storage_clear (ps->p_set))
      {
         case S_OK:
            break;
         case S_EMT:
            P_error = P_EMT;
            break;
         case S_DERR:
            P_error = P_DERR;
            break;
         default:
            ERROR(("PS_clear: Storage_clear returned error %d",res));
         case S_BAD:
            P_error = P_BAD;
            break;
      }
      /* free the memory for the file name and clear the pointer */
      if (ps->p_name != (char *)NULL)
      {
         (void)free (ps->p_name);
         ps->p_name = (char *)NULL;
      }
   }
   return (P_error);

}  /* end of function "PS_clear" */

#ifdef __STDC__
int PS_release (ParamSet params)
#else
int PS_release (params)
 ParamSet params;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   ps    A pointer to the parameter set to be released.
   res   The result of the release operation.
   */
   PARAM_SET *ps;
   int        res;

   /* clear the return code for errors in this module */
   P_error = P_OK;

   if ( (ps = (PARAM_SET *)params) == (PARAM_SET *)NULL ||
       ps->p_set == (Storage)NULL)
   {
      P_error = P_BAD;
   }
   else
   {
      switch (res = Storage_release (ps->p_set))
      {
         case S_BAD:
            P_error = P_BAD;
            break;
         case S_EMT:
            P_error = P_EMT;
            break;
         case S_DERR:
            P_error = P_DERR;
            break;
         case S_OK:
            break;
         default:
            ERROR(("PS_release: Storage_release returned error %d",res));
            break;
      }
      /* free the memory for the file name */
      if (ps->p_name != (char *)NULL)
         (void)free (ps->p_name);

      /* free the memory for the parameter set header */
      (void)free (ps);
   }
   return (P_error);

}  /* end of function "PS_release" */

#include "p_static.inc"
