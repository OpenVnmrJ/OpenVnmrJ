/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* tp.c: test driver for the Storage module */

#ifndef lint
	static char Sid[] = "@(#)tp.c 18.1 03/21/08 20:01:57 Dave Woodworth's test driver for the Storage module";
#endif

#include <ctype.h>
#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif
#include <string.h>
#ifndef MSDOS
#include <unistd.h>
#endif

#include "boolean.h"
#include "error.h"
#include "params.h"
#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif
#include "tp.h"

static char file_in[512];
static char file_out[512];

#ifdef __STDC__

static void help (void);
static void mainloop (void);
static void show_error (int p_error, char *p_name);
static void show_prot (int protect, char *p_name);
static void show_groups (int Ggroup, int Dgroup);
static void show_subtype (int subtype);

extern int  get_reply (char *prompt, char *reply, int max, int required);
extern void setbuf (FILE *stream, char *buf);
extern int  strcmp (char *s1, char *s2);

#else

static void help( /* void */ );
static void mainloop( /* void */ );
static void show_error( /* int p_error, char *p_name */ );
static void show_prot( /* int protect, char *p_name */ );
static void show_groups( /* int Ggroup, int Dgroup */ );
static void show_subtype( /* int subtype */ );

extern int  get_reply( /* char *prompt, char *reply, int max, int required */ );
extern void setbuf( /* FILE *stream, char *buf */ );
extern int  strcmp( /* char *s1, char *s2 */ );

#endif

#ifdef __STDC__
int main (int argc, char **argv)
#else
int main (argc, argv)
 int    argc;
 char **argv;
#endif
{
   (void)setbuf (stdout, (char *)NULL);
   (void)setbuf (stderr, (char *)NULL);

   help();
   mainloop();

   return 0;

}  /* end of function "main" */

#ifdef __STDC__
static void help (void)
#else
static void help()
#endif
{
   int i;

   for (i = 0; i < help_size; ++i)
      error ("%s", help_msg[i]);

}  /* end of function "help" */

#ifdef __STDC__
static void mainloop (void)
#else
static void mainloop()
#endif
{
   char    *p;
   ParamSet ps = (ParamSet)NULL;
   char     cmd [8];
   char     pname [64];
   char     buffer[128];
   double   value;
   int      int_val;
   int      size;
   int      index;
   int      res;

   for (;;)
   {
      cmd[0] = '\0';
      get_reply ("Enter command:", cmd, sizeof(cmd), FALSE);

      if ( (p = strtok (cmd, " \t")) == (char *)NULL)
         exit (0);

      /* force the message "Unknown command" for responses longer
         than one character */
      if (strlen (p) > 1)
         *p = '\1';

      switch (*p)
      {
         case '?':
            help();
            break;

         case 'q':
         case '\0':
#ifdef DEBUG_ALLOC
            get_reply ("Check dynamic memory? [Y/N]", cmd, sizeof(cmd), FALSE);
            if (cmd[0] == 'y' || cmd[0] == 'Y')
               check_list();
#endif
            exit (0);

         case 'a':
            if (get_reply ("Enter name of parameter to check activity state of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_get_active (ps, pname, &int_val)) == P_OK)
                  error ("Parameter '%s' is%s active", pname,
                         (int_val == 0 ? " not" : "") );
               else
                  show_error (res, pname);
            }
            break;

         case 'A':
            if (get_reply ("Enter name of parameter to set activity state of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if (get_reply("Enter new state:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               int_val = (int)strtol (buffer, &p, 10);
               if (p > buffer)
               {
                  if ((res = PS_set_active (ps, pname, int_val)) != P_OK)
                     show_error (res, pname);
               }
            }
            break;

         case 'g':
            if (get_reply ("Enter name of parameter to check groups of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               int Ggroup, Dgroup;

               if ( (res = PS_get_G_group (ps, pname, &Ggroup)) != P_OK ||
                    (res = PS_get_D_group (ps, pname, &Dgroup)) != P_OK   )
                  show_error (res, pname);
               else
                  show_groups (Ggroup, Dgroup);
            }
            break;

         case 'G':
            if (get_reply ("Enter name of parameter to set groups of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               int Ggroup, Dgroup;

               if (get_reply("Enter new Ggroup:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               Ggroup = (int)strtol (buffer, &p, 10);
               if (p <= buffer)
                  break;
               if (get_reply("Enter new Dgroup:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               Dgroup = (int)strtol (buffer, &p, 10);
               if (p <= buffer)
                  break;
               if ( (res = PS_set_G_group (ps, pname, Ggroup)) != P_OK ||
                    (res = PS_set_D_group (ps, pname, Dgroup)) != P_OK   )
                  show_error (res, pname);
            }
            break;

         case 'x':
            if (get_reply ("Enter name of parameter to get min/max/step of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               double min, max, step;

               if ( (res = PS_get_minval (ps, pname, &min)) != P_OK ||
                    (res = PS_get_maxval (ps, pname, &max)) != P_OK ||
                    (res = PS_get_step (ps, pname, &step))  != P_OK  )
                  show_error (res, pname);
               else
                  error ("min=%g, max=%g, step=%g", min, max, step);
            }
            break;

         case 'X':
            if (get_reply ("Enter name of parameter to set min/max/step of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               double min, max, step;

               if (get_reply("Enter min:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               min = strtod (buffer, &p);
               if (p <= buffer)
                  break;

               if (get_reply("Enter max:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               max = strtod (buffer, &p);
               if (p <= buffer)
                  break;

               if (get_reply("Enter step:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               step = strtod (buffer, &p);
               if (p <= buffer)
                  break;

               if ( (res = PS_set_minval (ps, pname, min)) != P_OK ||
                    (res = PS_set_maxval (ps, pname, max)) != P_OK ||
                    (res = PS_set_step (ps, pname, step))  != P_OK  )
                  show_error (res, pname);
            }
            break;

         case 'p':
            if (get_reply ("Enter name of parameter to check protection of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_get_protect (ps, pname, &int_val)) == P_OK)
                  show_prot (int_val, pname);
               else
                  show_error (res, pname);
            }
            break;

         case 'P':
            if (get_reply ("Enter name of parameter to set protection of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if (get_reply("Enter new protection bits:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               int_val = (int)strtol (buffer, &p, 10);
               if (p > buffer)
               {
                  if ((res = PS_set_protect (ps, pname, int_val)) != P_OK)
                     show_error (res, pname);
               }
            }
            break;

         case 'i':
            get_reply ("Enter name for input parameter file:", file_in,
                       sizeof(file_in), TRUE);
            break;

         case 'l':
            if ( (ps = PS_read (file_in, ps)) == (ParamSet)NULL)
               error ("Error reading parameter file '%s'", file_in);
            break;

         case 'e':
            if (get_reply ("Enter name of parameter to check:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if (PS_exist (ps, pname) == TRUE)
                  error ("Parameter '%s' exists", pname);
               else
                  error ("Parameter '%s' does not exist", pname);
            }
            break;

         case 'b':
            if (get_reply ("Enter name of parameter to check type of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_type (ps, pname, &int_val)) == P_OK)
               {
                  switch (int_val)
                  {
                     case T_UNDEF:  error ("undefined");   break;
                     case T_REAL:   error ("real");        break;
                     case T_STRING: error ("string");      break;
                     default:       error ("unknown");     break;
                  }
               }
               else
                  show_error (res, pname);
            }
            break;

         case 's':
            if (get_reply ("Enter name of parameter to check subtype of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_get_subtype (ps, pname, &int_val)) == P_OK)
                  show_subtype (int_val);
               else
                  show_error (res, pname);
            }
            break;

         case 'S':
            if (get_reply ("Enter name of parameter to set subtype of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if (get_reply("Enter new subtype:",buffer,sizeof(buffer),FALSE)==0)
                  break;
               int_val = (int)strtol (buffer, &p, 10);
               if (p > buffer)
               {
                  if ((res = PS_set_subtype (ps, pname, int_val)) != P_OK)
                     show_error (res, pname);
               }
            }
            break;

         case 'V':
            if (get_reply ("Enter name of parameter to set value of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_type (ps, pname, &int_val)) == P_OK)
               {
                  if (get_reply("Enter index:",buffer,sizeof(buffer),FALSE)==0)
                     break;

                  index = (int)strtol (buffer,&p,10);
                  if (p == buffer)
                     break;

                  switch (int_val)
                  {
                     case T_REAL:
                        if (get_reply("Enter number:",buffer,sizeof(buffer),0)>0)
                        {
                           value = strtod (buffer, &p);
                           if (p == buffer)
                              break;
                           if ((res=PS_set_real_value(ps,pname,index,value))!=P_OK)
                              show_error(res,pname);
                        }
                        break;
                     case T_STRING:
                        if (get_reply("Enter string:",buffer,sizeof(buffer),0)>0)
                        {
                           if ((res=PS_set_string_value(ps,pname,index,buffer))!=P_OK)
                              show_error(res,pname);
                        }
                        break;
                     case T_UNDEF:
                        if (get_reply("Enter value:",buffer,sizeof(buffer),0)>0)
                        {
                           value = strtod (buffer, &p);
                           if (p > buffer)
                           {
                              res=PS_set_real_value(ps,pname,index,value);
                           }
                           else
                           {
                              res=PS_set_string_value(ps,pname,index,buffer);
                           }
                           if (res != P_OK)
                              show_error(res,pname);
                        }
                        break;
                     default:
                        error ("Type %d of param '%s' unknown",int_val,pname);
                        break;
                  }
               }
               else
                  show_error (res, pname);
            }
            break;

         case 'v':
            if (get_reply ("Enter name of parameter to get value of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_type (ps, pname, &int_val)) == P_OK)
               {
                  if (get_reply("Enter index:",buffer,sizeof(buffer),FALSE)==0)
                     break;

                  index = (int)strtol (buffer,&p,10);
                  if (p == buffer)
                     break;

                  switch (int_val)
                  {
                     case T_REAL:
                        if ((res=PS_get_real_value(ps,pname,index,&value))!=P_OK)
                           show_error(res,pname);
                        else
                           error ("'%s'=%g",pname,value);
                        break;
                     case T_STRING:
                        if ((res=PS_get_string_value(ps,pname,index,&p))!=P_OK)
                           show_error(res,pname);
                        else
                           error ("'%s'=%s",pname,p);
                        break;
                     case T_UNDEF:
                     default:
                        error ("Type of parameter '%s' is un%s", pname,
                               (int_val == T_UNDEF ? "defined" : "known") );
                        break;
                  }
               }
               else
                  show_error (res, pname);
            }
            break;

         case 'B':
            if (get_reply ("Enter name of parameter to get values of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_size (ps, pname, &size))    != P_OK ||
                    (res = PS_type (ps, pname, &int_val)) != P_OK   )
               {
                  show_error (res, pname);
                  break;
               }
               else if (int_val != T_REAL && int_val != T_STRING)
               {
                  error ("Type of parameter '%s' is un%s", pname,
                         (int_val == T_UNDEF ? "defined" : "known") );
                  break;
               }
               fprintf (stderr, "%s:",pname);
               for (index = 1; index <= size; ++index)
               {
                  switch (int_val)
                  {
                     case T_REAL:
                        if ((res=PS_get_real_value(ps,pname,index,&value))!=P_OK)
                        {
                           show_error(res,pname);
                           index = size + 1;
                        }
                        else
                           fprintf (stderr, " %g", value);
                        break;
                     case T_STRING:
                        if ((res=PS_get_string_value(ps,pname,index,&p))!=P_OK)
                        {
                           show_error(res,pname);
                           index = size + 1;
                        }
                        else
                           fprintf (stderr, " \"%s\"",p);
                        break;
                  }
               }
               fprintf (stderr, "\n");
            }
            break;

         case 'E':
            if (get_reply("Enter name of parameter to get enumerated values of:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_enum_size (ps, pname, &size)) != P_OK ||
                    (res = PS_type (ps, pname, &int_val))   != P_OK   )
               {
                  show_error (res, pname);
                  break;
               }
               else if (int_val != T_REAL && int_val != T_STRING)
               {
                  error ("Type of parameter '%s' is un%s", pname,
                         (int_val == T_UNDEF ? "defined" : "known") );
                  break;
               }
               fprintf (stderr, "%s:",pname);
               for (index = 1; index <= size; ++index)
               {
                  switch (int_val)
                  {
                     case T_REAL:
                        if ((res=PS_get_real_evalue(ps,pname,index,&value))!=P_OK)
                        {
                           show_error(res,pname);
                           index = size + 1;
                        }
                        else
                           fprintf (stderr, " %g", value);
                        break;
                     case T_STRING:
                        if ((res=PS_get_string_evalue(ps,pname,index,&p))!=P_OK)
                        {
                           show_error(res,pname);
                           index = size + 1;
                        }
                        else
                           fprintf (stderr, " \"%s\"",p);
                        break;
                  }
               }
               fprintf (stderr, "\n");
            }
            break;

         case 'n':
            if (get_reply ("Enter name of parameter to check:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_size (ps, pname, &int_val)) == P_OK)
                  error ("'%s' has %d values", pname, int_val);
               else
                  show_error (res, pname);
            }
            break;

         case 'N':
            if (get_reply ("Enter name of parameter to check:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_enum_size (ps, pname, &int_val)) == P_OK)
                  error ("'%s' has %d enumerated values", pname, int_val);
               else
                  show_error (res, pname);
            }
            break;

         case 'd':
            if (get_reply ("Enter name of parameter to delete:",
                           pname, sizeof(pname), FALSE) > 0)
            {
               if ( (res = PS_delete (ps, pname)) != P_OK)
                  show_error (res, pname);
            }
            break;

         case 'o':
            get_reply ("Enter name for output parameter file:", file_out,
                       sizeof(file_out), TRUE);
            break;

         case 'w':
            if ( (res = PS_write (file_out, ps)) != P_OK)
               show_error (res, file_out);
            else
               error ("Output parameter set is '%s'", file_out);
            break;

         case 'c':
            if ( (res = PS_clear (ps)) != P_OK)
               show_error (res, "PS_clear");
            break;

         case 'r':
            if ( (res = PS_release (ps)) != P_OK)
               show_error (res, "PS_release");
            else
               ps = (ParamSet)NULL;
            break;

#ifdef DEBUG_ALLOC
         case 'm':
            check_list();
            break;
#endif
         default:
            error ("Unknown command - use '?' for help");
            break;
      }
      pname[0] ='\0';
   }
}  /* end of function "mainloop" */

#ifdef __STDC__
static void show_error (int p_error, char *p_name)
#else
static void show_error (p_error, p_name)
 int   p_error;
 char *p_name;
#endif
{
   char *p;

   fprintf (stderr, "'%s': ", p_name);

   switch (p_error)
   {
      case P_BAD:
         p = "pointer to parameter set is (ParamSet)NULL";
         break;
      case P_EMT:
         p = "requested parameter set is empty";
         break;
      case P_READ:
         p = "error occurred while reading parameter file";
         break;
      case P_WRITE:
         p = "error occurred while writing parameter file";
         break;
      case P_DERR:
         p = "error occurred while deleting an entry";
         break;
      case P_PARM:
         p = "pointer to parameter structure is (PARAM *)NULL";
         break;
      case P_SRCH:
         p = "requested parameter is not in the parameter set";
         break;
      case P_MEM:
         p = "can't allocate memory for parameter structures";
         break;
      case P_TYPE:
         p = "requested parameter has illegal type";
         break;
      case P_INS:
         p = "couldn't save parameter info struct in storage";
         break;
      case P_INDX:
         p = "value array index out of range [1...size]";
         break;
      case P_GGRP:
         p = "invalid Ggroup value";
         break;
      case P_DGRP:
         p = "invalid Dgroup value";
         break;
      default:
         error ("unknown error %d", p_error);
         return;
   }
   error ("%s", p);

}  /* end of function "show_error" */

static char *prot_msg[] = {
   "P_ARR",  /*    1 - cannot array the parameter */
   "P_ACT",  /*    2 - cannot change active/not active status */
   "P_VAL",  /*    4 - cannot change the parameter value */
   "P_MAC",  /*    8 - causes macro to be executed */
   "P_REX",  /*   16 - avoids automatic re-display */
   "P_DEL",  /*   32 - cannot delete parameter */
   "P_CRT",  /*   64 - cannot recreate parameter */
   "P_CPY",  /*  128 - cannot copy parameter from tree to tree */
   "P_LIM",  /*  256 - cannot set parameter max, min, or step */
   "P_ENU",  /*  512 - cannot set parameter enumeral values */
   "P_GRP",  /* 1024 - cannot change the parameter's group */
   "P_PRO",  /* 2048 - cannot change protection bits */
   "P_DGP",  /* 4096 - cannot change the display group */
   "P_MMS"   /* 8192 - lookup min, max, step values in table */
};

static int prot_count = sizeof(prot_msg) / sizeof(prot_msg[0]);

#ifdef __STDC__
static void show_prot (int protect, char *p_name)
#else
static void show_prot (protect, p_name)
 int   protect;
 char *p_name;
#endif
{
   int mask;
   int i;

   fprintf (stderr, "'%s':", p_name);

   for (mask = 1, i = 0; i < prot_count; mask <<= 1, ++i)
   {
      if (mask & protect)
         fprintf (stderr, " %s", prot_msg[i]);
   }
   fprintf(stderr,"\n");

}  /* end of function "show_prot" */

#ifdef __STDC__
static void show_groups (int Ggroup, int Dgroup)
#else
static void show_groups (Ggroup, Dgroup)
 int Ggroup;
 int Dgroup;
#endif
{
   char *pG = "unknown";
   char *pD = "unknown";

   switch (Ggroup)
   {
      case G_ALL:
         pG = "G_ALL";
         break;
      case G_SAMPLE:
         pG = "G_SAMPLE";
         break;
      case G_ACQUISITION:
         pG = "G_ACQUISITION";
         break;
      case G_PROCESSING:
         pG = "G_PROCESSING";
         break;
      case G_DISPLAY:
         pG = "G_DISPLAY";
         break;
      case G_SPIN:
         pG = "G_SPIN";
         break;
   }
   switch (Dgroup)
   {
      case D_ALL:
         pD = "D_ALL";
         break;
      case D_ACQUISITION:
         pD = "D_ACQUISITION";
         break;
      case D_2DACQUISITION:
         pD = "D_2DACQUISITION";
         break;
      case D_SAMPLE:
         pD = "D_SAMPLE";
         break;
      case D_DECOUPLING:
         pD = "D_DECOUPLING";
         break;
      case D_AFLAGS:
         pD = "D_AFLAGS";
         break;
      case D_PROCESSING:
         pD = "D_PROCESSING";
         break;
      case D_SPECIAL:
         pD = "D_SPECIAL";
         break;
      case D_DISPLAY:
         pD = "D_DISPLAY";
         break;
      case D_REFERENCE:
         pD = "D_REFERENCE";
         break;
      case D_PHASE:
         pD = "D_PHASE";
         break;
      case D_CHART:
         pD = "D_CHART";
         break;
      case D_2DDISPLAY:
         pD = "D_2DDISPLAY";
         break;
      case D_INTEGRAL:
         pD = "D_INTEGRAL";
         break;
      case D_DFLAGS:
         pD = "D_DFLAGS";
         break;
      case D_FID:
         pD = "D_FID";
         break;
      case D_SHIMCOILS:
         pD = "D_SHIMCOILS";
         break;
      case D_AUTOMATION:
         pD = "D_AUTOMATION";
         break;
      case D_NUMBERS:
         pD = "D_NUMBERS";
         break;
      case D_STRINGS:
         pD = "D_STRINGS";
         break;
   }
   error ("Group is %s (%d); display group is %s (%d)",pG,Ggroup,pD,Dgroup);

}  /* end of function "show_groups" */

#ifdef __STDC__
static void show_subtype (int subtype)
#else
static void show_subtype (subtype)
 int subtype;
#endif
{
   char *p = "unknown";

   switch (subtype)
   {
      case ST_UNDEF:
         p = "ST_UNDEF";
         break;
      case ST_REAL:
         p = "ST_REAL";
         break;
      case ST_STRING:
         p = "ST_STRING";
         break;
      case ST_DELAY:
         p = "ST_DELAY";
         break;
      case ST_FLAG:
         p = "ST_FLAG";
         break;
      case ST_FREQUENCY:
         p = "ST_FREQUENCY";
         break;
      case ST_PULSE:
         p = "ST_PULSE";
         break;
      case ST_INTEGER:
         p = "ST_INTEGER";
         break;
   }
   error ("Sub-type is %s (%d)", p, subtype);

}  /* end of function "show_subtype" */
