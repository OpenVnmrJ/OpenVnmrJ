/* Copyright (c) Varian Assoc., Inc.  All Rights Reserved. */

/* 
 * Varian, Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

/*ARGSUSED*/
#ifdef __STDC__
static int P_delete (void *name, void *info)
#else
static int P_delete (name, info)
 void *name;
 void *info;
#endif
{
   return (P_free_param ((PARAM *)info));
}

/*ARGSUSED*/
#ifdef __STDC__
static int P_write (void *name, void *info)
#else
static int P_write (name, info)
 void *name;
 void *info;
#endif
{
   return (P_write_param (P_fileptr, (PARAM *)info));
}

#ifdef __STDC__
static int P_read_param (FILE *file_ptr, PARAM **pp)
#else
static int P_read_param (file_ptr, pp)
 FILE   *file_ptr;
 PARAM **pp;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg  The name of this function, for error messages.
   syn_msg  A message for missing lines in a parameter entry.
   buffer   A buffer for holding a line of the parameter entry.
   p_buff   A pointer into buffer, used while decoding the text line.
   p_info   A pointer to the VNMR parameter information structure.
   count    The number of parameter information fields read.
   */
   static char sub_msg[] = "P_read_param:";
   static char syn_msg[] = "%s missing %s line for parameter '%s' in file '%s'";
   char   buffer[MAXSTR+1];
   char  *p_buff;
   PARAM *p_info;
   int    count;

   /* read the first line of the parameter entry */
   if (P_get_param_line (file_ptr, buffer, sizeof(buffer)) != P_OK)
   {
      *pp = (PARAM *)NULL;
      return (P_error);
   }
   /* load the parameter name; this sets the global parameter name, used by
      error messages in this module */
   if ( (P_parmname = P_get_param_name (buffer, &p_buff)) == (char*)NULL)
   {
      *pp = (PARAM *)NULL;
      return (P_error);
   }

   /* get space for a parameter information structure */
   if ( (*pp = p_info = (PARAM *)calloc(1,sizeof(PARAM))) == (PARAM *)NULL)
   {
      SYS_ERROR(("%s calloc info struct for parameter '%s'",sub_msg,P_parmname));
      return (P_error = errno);
   }
   /* set the parameter name in the parameter information structure */
   p_info->name = P_parmname;

   /* load the parameter fields from the rest of this line */
   if ( (count = sscanf (p_buff, "%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
                         &p_info->subtype, &p_info->basictype,
                         &p_info->maxval, &p_info->minval, &p_info->step,
                         &p_info->Ggroup, &p_info->Dgroup, &p_info->prot,
                         &p_info->active, &p_info->intptr)) != 10)
   {
      error ("%s read %d fields for parameter '%s' in file '%s'; need 10",
             sub_msg, count, P_parmname, P_filename);
      P_error = SYN_ERR;
   }

   /* read the second line of the parameter entry */
   if (P_get_param_line (file_ptr, buffer, sizeof(buffer)) != P_OK)
   {
      /* if the second line is missing, print a message */
      if (P_error == EOF)
         error (syn_msg, sub_msg, "second", P_parmname, P_filename);

      (void)P_free_param (p_info);
      *pp = (PARAM *)NULL;

      return (P_error);
   }
   /* load the number of parameter values in this line */
   if ( (p_info->size = (short)strtol (buffer, &p_buff, 10)) > 0)
   {
      /* load the parameter values in this line */
      p_info->value = P_get_param_values (file_ptr, buffer, p_buff, 
                                         p_info->basictype, &(p_info->size));
      if (P_error != P_OK && P_error != SYN_ERR)
      {
         (void)P_free_param (p_info);
         *pp = (PARAM *)NULL;
         return (P_error);
      }
      P_error = P_OK;
   }

   /* read the third line of the parameter entry */
   if (P_get_param_line (file_ptr, buffer, sizeof(buffer)) != P_OK)
   {
      if (P_error == EOF)
         error (syn_msg, sub_msg, "third", P_parmname, P_filename);

      (void)P_free_param (p_info);
      *pp = (PARAM *)NULL;

      return (P_error);
   }
   /* load the number of parameter enumeral values in this line */
   if ( (p_info->enum_size = (short)strtol (buffer, &p_buff, 10)) > 0)
   {
      /* load the parameter enumeral values in this line */
      p_info->enum_value = P_get_param_values (file_ptr, buffer, p_buff,
                                 p_info->basictype, &(p_info->enum_size));
      if (P_error != P_OK && P_error != SYN_ERR)
      {
         (void)P_free_param (p_info);
         *pp = (PARAM *)NULL;
         return (P_error);
      }
      P_error = P_OK;
   }
   return (P_error);

}  /* end of function "P_read_param" */

#ifdef __STDC__
static int P_get_param_line (FILE *file_ptr, char *buffer, int size)
#else
static int P_get_param_line (file_ptr, buffer, size)
 FILE *file_ptr;
 char *buffer;
 int   size;
#endif
{
#ifdef DEBUG
   static char sub_msg[] = "P_get_param_line:";
#endif

   /* read a line from the parameter file */
   if (fgets (buffer, size, file_ptr) == (char*)NULL)
   {
      if (feof (file_ptr))
      {
         P_error = EOF;
      }
      else if (ferror (file_ptr))
      {
         SYS_ERROR(("%s fgets 1 line from parameter file '%s'",
                    sub_msg,P_filename));
         P_error = errno;
      }
      else
      {
         ERROR(("%s unknown error reading parameter file '%s'",
                 sub_msg,P_filename));
         P_error = BAD_ERR;
      }
      /* clear the error indication for the file */
      (void)clearerr (file_ptr);
   }
   return (P_error);

}  /* end of function "P_get_param_line" */

#ifdef __STDC__
static char *P_get_param_name (char *buffer, char **p_next)
#else
static char *P_get_param_name (buffer, p_next)
 char  *buffer;
 char **p_next;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_buff   A pointer into the text string, for finding the parameter name.
   p_name   A pointer to the (stored) parameter name.
   */
   char *p_buff = buffer;
   char *p_name = (char *)NULL;

   /* skip any white space ahead of the name */
   while (isspace (*p_buff))
      ++p_buff;

   /* check for the end of the line of text  */
   if (*p_buff != '\0')
   {
      /* save the start of the name */
      buffer = p_buff;

      /* skip past the name to white space, then terminate the name */
      while (!isspace (*p_buff) && *p_buff != '\0')
         ++p_buff;
      *p_buff++ = '\0';

      /* get memory to store the name string */
      if ( (p_name = (char *)malloc ((unsigned)(p_buff-buffer))) != (char*)NULL)
      {
         /* copy the name string into storage */
         (void)strcpy (p_name, buffer);
      }
      else
      {
         SYS_ERROR(("P_get_param_name: malloc space for parameter '%s' in file '%s'",
                    buffer,P_filename));
         P_error = errno;
      }
   }
   else
   {
      /* indicate a minor error (blank line) */
      P_error = REC_ERR;
   }
   /* report the current position in the buffer */
   *p_next = p_buff;

   /* return the pointer to the name */
   return (p_name);

}  /* end of function "P_get_param_name" */

#ifdef __STDC__
static STRDBL P_get_param_values (FILE *file_ptr, char *buffer, char *p_buff,
						short type, short *size)
#else
static STRDBL P_get_param_values (file_ptr, buffer, p_buff, type, size)
 FILE *file_ptr;
 char  *buffer;
 char  *p_buff;
 short  type;
 short *size;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   mem_msg     Error message for allocating memory for value array.
   val_msg     Error message for reading real value from input.
   str_msg     Error message for allocating memory for string storage.
   real        A character string for error messages.
   string      A character string for error messages.
   str_dbl     The structure for holding the values read.
   val_i       A counter for the values read from the text buffer.
   p_dbls      A pointer for the array of real values.
   p_strs      A pointer for the array of string values.
   p_str       A pointer for a string value.
   length      The length of a string, returned by "P_get_value_string()".
   */
   static char sub_msg[] = "P_get_param_values:";
   static char mem_msg[] =
               "%s calloc space for parameter '%s' %s values in file '%s'";
   static char val_msg[] =
               "%s can't read %s value %d for parameter '%s' in file '%s'";
   static char str_msg[] =
               "%s malloc space for parameter '%s' %s value '%s' in file '%s'";
   static char real[]    = "real";
   static char string[]  = "string";
   char   outbuffer[MAXSTR+1];
   STRDBL   str_dbl;
   short    val_i = 0;
   double  *p_dbls;
   char   **p_strs;
   char    *p_str;
   unsigned length;

   switch (type)
   {
      /* there should be no values for type "undefined" */
      case T_UNDEF:
         if (*size != 0)
         {
            ERROR((und_msg, sub_msg, P_parmname, P_filename));
            ERROR((not_msg, "read"));
            P_error = SYN_ERR;
         }
         str_dbl.dbl = (double *)NULL;
         break;

      /* store double-precision values */
      case T_REAL:

         /* allocate space for an array of doubles */
         if ( (p_dbls = (double *)calloc ((unsigned)(*size), sizeof(double))) !=
             (double *)NULL)
         {
            /* read and load the array of double values */
            for (; val_i < *size; ++val_i)
            {
               if (*p_buff == '\0')
               {
		  if (P_get_param_line (file_ptr, buffer, 
					MAXSTR+1) != P_OK)
		  {
                    error (val_msg, sub_msg, real, val_i, P_parmname, 
								P_filename);
                    P_error = SYN_ERR;
                    break;
		  }
	          p_buff = buffer;
               }
               *(p_dbls + val_i) = strtod (p_buff, &p_buff);
            }
         }
         else
         {
            (void)sys_error (mem_msg, sub_msg, P_parmname, real, P_filename);
            P_error = errno;
         }
         str_dbl.dbl = p_dbls;
         break;

      /* store string values */
      case T_STRING:

         /* allocate space for an array of character pointers */
         if ( (p_strs = (char **)calloc ((unsigned)(*size), sizeof(char *))) !=
             (char **)NULL)
         {
            /* read and load the array of character strings */
            for (; val_i < *size; ++val_i)
            {
               /* get a value string, without the delimiting quotes */
               p_buff = P_get_value_string (file_ptr, buffer, p_buff, 
			outbuffer, &length);
	       if ((p_str = p_buff) == (char*)NULL)
	       {
         	   P_error = SYN_ERR;
         	   str_dbl.str = (char **)NULL;
		   return (str_dbl);
	       }

               /* get space for saving the string */
               if ( (*(p_strs+val_i) = malloc (length)) == (char*)NULL)
               {
                  (void)sys_error (str_msg, sub_msg, P_parmname, string, 
					outbuffer, P_filename);
                  P_error = errno;
                  break;
               }
               /* save the value string */
               (void)strcpy (*(p_strs+val_i), outbuffer);
            }
         }
         else
         {
            (void)sys_error (mem_msg, sub_msg, P_parmname, string, P_filename);
            P_error = errno;
         }
         str_dbl.str = p_strs;
         break;

      /* handle error in VNMR basic type */
      default:
         error (typ_msg, sub_msg, type, P_parmname, P_filename);
         error (not_msg, "read");
         P_error = SYN_ERR;
         str_dbl.str = (char **)NULL;
         break;
   }
   /* set the number of values that were actually loaded */
   *size = val_i;

   return (str_dbl);

}  /* end of function "P_get_param_values" */

#ifdef __STDC__
static char *P_get_value_string (FILE *file_ptr, char *buffer, char *p_buff, 
				char *o_buff, unsigned *length)
#else
static char *P_get_value_string (file_ptr, buffer, p_buff, o_buff, length)
 FILE   *file_ptr;
 char     *buffer;
 char    *p_buff;
 char    *o_buff;
 unsigned *length;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   syn_msg     Error message for string syntax.
   p_beg       A pointer to the beginning of the string (without quotes).
   bad_quote   Which quote is missing: 1 => starting
                                       2 => ending
   */
   static char sub_msg[] = "P_get_value_string:";
   static char syn_msg[] =
               "%s %s in string value for parameter '%s' in file '%s'";
   char *p_beg;

   /* NULL the pointer to the start of the string, and clear the length */
   p_beg = (char *)NULL;
   *length = 0;

   /* Get first quote */
   while (*p_buff != '"')
   {
   	/* move past any leading white space */
   	while (isspace(*p_buff))
   	   ++p_buff;

   	/* if the end of the line was reached ... */
   	if ((*p_buff == '\0') || (*p_buff == '\n'))
   	{
   	   if (P_get_param_line (file_ptr, buffer, MAXSTR+1) != P_OK)
	   {
      	    error ("%s missing string value for parameter '%s' in file '%s'",
             	sub_msg, P_parmname, P_filename);
      		P_error = SYN_ERR;
	        return (p_beg);
	   }
	   p_buff = buffer;
       	}
	else if (*p_buff != '"')
	{
           error (syn_msg, sub_msg, "missing \"", P_parmname, P_filename);
           P_error = SYN_ERR;
	   return (p_beg);
	}

    }

    /* move the pointer past the quotes to the start of the text string */
    ++p_buff;

   /* get the string */
   while ( (*p_buff != '"') && (*length < MAXSTR) )
   {

   	/* if the end of the line was reached ... */
   	if ((*p_buff == '\0') || (*p_buff == '\n'))
   	{
   	   if (P_get_param_line (file_ptr, buffer, MAXSTR+1) != P_OK)
	   {
		/* clear the length */
      		*length = 0;
      	    error ("%s missing string value for parameter '%s' in file '%s'",
             	sub_msg, P_parmname, P_filename);
      		P_error = SYN_ERR;
		return (p_beg);
	   }
	   p_buff = buffer;
       	}
	else
	{
	   *o_buff = *p_buff;
	    ++o_buff;
	    ++p_buff;
	    *length = *length+1;
	}
   }
   if (*length == MAXSTR)
   {
	/* clear the length */
      	*length = 0;
	error (syn_msg, sub_msg, "missing \"", P_parmname, P_filename);
	P_error = SYN_ERR;
	return (p_beg);
   }
   *o_buff = '\0';
   *length = *length+1;
   ++p_buff;


   /* return a pointer to the start of the rest of the string */
   return (p_buff);

}  /* end of function "P_get_value_string" */

#ifdef __STDC__
static int P_write_param (FILE *file_ptr, PARAM *p)
#else
static int P_write_param(file_ptr, p)
 FILE  *file_ptr;
 PARAM *p;
#endif
{
   if (p == (PARAM *)NULL)
   {
      ERROR(("P_write_param: NULL PARAM pointer"));
      P_error = P_PARM;
   }
   else
   {
      /* set the name from the parameter information structure */
      P_parmname = (p->name == (char *)NULL ? P_noname : p->name);

      /* print the first line of the parameter entry */
      (void)fprintf (file_ptr, "%s ", P_parmname);

      (void)fprintf (file_ptr, "%d %d %.12g %.12g %.12g %d %d %d %d %x\n",
                     p->subtype, p->basictype, p->maxval, p->minval, p->step,
                     p->Ggroup, p->Dgroup, p->prot, p->active, p->intptr);

      /* print the 2nd line of the parameter entry: parameter values */
      (void)P_put_param_values (file_ptr, p->size, p->basictype, p->value);

      /* print the 3rd line of the parameter entry: enumeral values */
      (void)P_put_param_values (file_ptr, p->enum_size, p->basictype, p->enum_value);
      /* ignore output record errors */
      if (P_error == REC_ERR)
         P_error = P_OK;
   }
   return (P_error);

}  /* end of function "P_write_param" */

#ifdef __STDC__
static int P_put_param_values (FILE *file_ptr, short size, short type,
                               STRDBL value)
#else
static int P_put_param_values (file_ptr, size, type, value)
 FILE *file_ptr;
 short size;
 short type;
 STRDBL value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg  The name of this function, for error messages.
   val_i    A counter, for indexing the array of parameter values.
   */
   static char sub_msg[] = "P_put_param_values:";
   int val_i;

   /* print the (array) values for the parameter entry */
   switch (type)
   {
      /* there should be no values for type "undefined" */
      case T_UNDEF:
         if (value.dbl != (double *)NULL)
         {
            ERROR((und_msg, sub_msg, P_parmname, P_filename));
            ERROR((not_msg, "written"));
            P_error = REC_ERR;
         }
         (void)fprintf (file_ptr, "0\n");
         break;

      case T_REAL:
         /* print the number of values in the parameter entry */
         (void)fprintf (file_ptr, "%d", size);

         /* print the values for the parameter entry */
         if (value.dbl != (double *)NULL)
         {
            for (val_i = 0; val_i < size; ++val_i)
               (void)fprintf (file_ptr, " %1.12g", value.dbl[val_i] );
         }
         (void)fprintf (file_ptr, "\n");
         break;

      case T_STRING:
         /* print the number of values in the parameter entry */
         (void)fprintf (file_ptr, "%d", size);

         /* print the values for the parameter entry */
         if (value.str != (char **)NULL)
         {
            for (val_i = 0; val_i < size; ++val_i)
            {
               if (value.str[val_i] != (char *)NULL)
                  (void)fprintf (file_ptr, " \"%s\"", value.str[val_i] );
               else
                  (void)fprintf (file_ptr, " \"\"");
            }
         }
         (void)fprintf (file_ptr, "\n");
         break;

      default:
         error (typ_msg, sub_msg, type, P_parmname, P_filename);
         error (not_msg, "written");

         /* print zero for the number of values in the parameter entry */
         (void)fprintf (file_ptr, "0\n");

         P_error = REC_ERR;
         break;
   }
   return (P_error);

}  /* end of function "P_put_param_values" */

#ifdef __STDC__
static int P_free_param (PARAM *p)
#else
static int P_free_param (p)
 PARAM *p;
#endif
{
   /* make sure that there is something to free */
   if (p == (PARAM *)NULL)
   {
      ERROR(("P_free_param: NULL pointer to parameter info struct"));
      P_error = P_PARM;
   }
   else
   {
      /* set the parameter name for error messages */
      P_parmname = (p->name == (char *)NULL ? P_noname : p->name);

      /* free the memory used by the parameter values */
      if (P_free_param_values (p->basictype,p->size,p->value) == P_OK &&
          P_free_param_values (p->basictype,p->enum_size,p->enum_value) == P_OK)
      {
         /* free the parameter name */
         if (p->name != (char *)NULL)
            (void)free (p->name);

         /* free the parameter information structure */
         (void)free ( (char *)p );
      }
   }
   return (P_error);

}  /* end of function "P_free_param" */

#ifdef __STDC__
static int P_free_param_values (short type, short size, STRDBL value)
#else
static int P_free_param_values (type, size, value)
 short type;
 short size;
 STRDBL value;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   val_i       A counter for accessing the array of parameter values.
   */
   static char sub_msg[] = "P_free_param_values:";
   int val_i;

   switch (type)
   {
      /* there should be no values for type "undefined" */
      case T_UNDEF:
         if (value.dbl != (double *)NULL)
         {
            ERROR((und_msg, sub_msg, P_parmname, P_filename));
            ERROR((not_msg, "freed"));
            P_error = SYN_ERR;
         }
         break;

      /* free the array of doubles for a real-valued parameter */
      case T_REAL:
         if (value.dbl != (double *)NULL)
            (void)free ( (char *)value.dbl );
         break;

      /* free the array of strings for a string-valued parameter */
      case T_STRING:
         if (value.str != (char **)NULL)
         {
            /* free each string in the array */
            for (val_i = 0; val_i < size; ++val_i)
            {
               if (value.str[val_i] != (char *)NULL)
                  (void)free ( (char *)value.str[val_i] );
            }
            /* free the array */
            (void)free ( (char *)value.str );
         }
         break;

      default:
         error (typ_msg, sub_msg, type, P_parmname, P_filename);
         error (not_msg, "freed");
         P_error = SYN_ERR;
         break;
   }
   return (P_error);

}  /* end of function "P_free_param_values" */
