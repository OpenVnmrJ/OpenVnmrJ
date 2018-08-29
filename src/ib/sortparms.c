/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "boolean.h"
#include "error.h"
#include "params.h"
#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif

#ifdef __STDC__
int main (int argc, char **argv)
#else
int main (argc, argv)
 int    argc;
 char **argv;
#endif
{
   char *prog_name;
   char *p_in  = (char *)NULL;
   char *p_out = (char *)NULL;
   ParamSet ps;

   /* get the name of this program */
   if ( (prog_name = strrchr (*argv, '/')) == (char *)NULL)
      prog_name = *argv;
   else
      ++prog_name;

   if (argc > 3)
   {
      error ("USAGE: %s [input_file [output_file] ]", prog_name);
      error_exit (1);
   }
   /* check for I/O files on the command line */
   if (argc >= 2)
   {
      p_in = *(argv + 1);
      if (argc > 2)
         p_in = *(argv + 2);
   }
   if (p_in == (char *)NULL)
      error ("reading parameters from stdin");
   else
      error ("reading input parameter file '%s'", p_in);

   if ( (ps = PS_read (p_in, (ParamSet)NULL)) == (ParamSet)NULL)
   {
      if (p_in == (char *)NULL)
         error ("error reading parameters from stdin");
      else
         error ("error reading input parameter file '%s'", p_in);
   }
   else
   {
      if (p_out == (char *)NULL)
         error ("writing parameters to stdout");
      else
         error ("writing output parameter file '%s'", p_out);

      if (PS_write (p_out, ps) != P_OK)
      {
         if (p_out == (char *)NULL)
            error ("error writing parameters to stdout");
         else
            error ("error writing output parameter file '%s'", p_out);
      }
   }
   /* free the parameter set and check for memory leaks */
   (void)PS_release (ps);

#ifdef DEBUG_ALLOC
   (void)check_list();
#endif

   return 0;
}
