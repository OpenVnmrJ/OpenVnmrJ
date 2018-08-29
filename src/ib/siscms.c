/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routines related to colormap.  The colormap is divided by three	*
*  groups.  The user can initialize these three group colormap  at	*
*  the start-up of the execution.  The size of each group colormap can	*
*  also be configurable.  At the start-up, it reads initalized colormap *
*  file.								*
*									*
*************************************************************************/
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <strings.h>
#include "stderr.h"
#include "siscms.h"

static int read_colormap(FILE *file,	/* filename */
        u_char *red,		/* red values */
	u_char *green,		/* green values */
	u_char *blue,		/* blue values */
        int size);		/* size of colormap */
static int init_get_cms(char *filename,    /* filename */
     	u_char *red,            /* red values */
     	u_char *green,          /* green values */
     	u_char *blue,           /* blue values */
     	char *cms_name,         /* cms name  */
     	int *cms_size);         /* cms size  */
static int default_colormap(char *defname,	/* colormap NAME */
	u_char *red,		/* red values */
	u_char *green,		/* green values */
	u_char *blue,		/* blue values */
	int size);		/* colormap size */

/************************************************************************
*                                                                       *
*  Create a colormap table.						*
*  The value stored in the colormap will be in order according to the	*
*  cmsname 1, 2, 3.  Typically,  cms name 1 is used for graphics 	*
*  drawing,  cms name 2 is used for gray-scale, and cms name 3 is	*
*  used for false-color.  The caller can specify NULL for the name if	*
*  he doesn't expect all three colormap to be loaded.			*
*  Return a pointer to object or NULL.					*
*									*/
Siscms *
siscms_create(char *filename,	/* File name containing colormap values */
	char *cmsname1,		/* cms NAME 1 */
	char *cmsname2,		/* cms NAME 2 */
	char *cmsname3)		/* cms NAME 3 */
{
   Siscms *siscms;

   WARNING_OFF(Sid);

   /* Allocate memory */
   if ((siscms = (Siscms *)calloc(1, sizeof(Siscms))) == NULL)
   {
      PERROR("siscms_create:calloc");
      return(NULL);
   }

   siscms->size_cms1 = siscms->size_cms2 = siscms->size_cms3 = 0;
   siscms->st_cms1 = siscms->st_cms2 = siscms->st_cms3 = 0;

   /* Get colormap NAME 1 */
   if (cmsname1)
   {
      if (init_get_cms(filename, siscms->r, siscms->g, siscms->b,
          cmsname1, &(siscms->size_cms1)) == NOT_OK)
      {
         STDERR("siscms_create:init_get_csm");
         (void)free((char *)siscms);
         return(NULL);
      }
      siscms->st_cms2 = siscms->st_cms3 = siscms->size_cms1;
   }

   /* Get colormap NAME 2 */
   if (cmsname2)
   {
      if (init_get_cms(filename, siscms->r + siscms->st_cms2,
				 siscms->g + siscms->st_cms2,
				 siscms->b + siscms->st_cms2,
          cmsname2, &(siscms->size_cms2)) == NOT_OK)
      {
         STDERR("siscms_create:init_get_csm");
         (void)free((char *)siscms);
         return(NULL);
      }
      siscms->st_cms3 += siscms->size_cms2;
   }

   /* Get colormap NAME 3 */
   if (cmsname3)
   {
      if (init_get_cms(filename, siscms->r + siscms->st_cms3,
				 siscms->g + siscms->st_cms3,
				 siscms->b + siscms->st_cms3,
          cmsname3, &(siscms->size_cms3)) == NOT_OK)
      {
         STDERR("siscms_create:init_get_csm");
         (void)free((char *)siscms);
         return(NULL);
      }
   }

   /* Check for upper and lower limit */
   if ((siscms->cms_size = siscms->size_cms1 + siscms->size_cms2 +
	siscms->size_cms3 + 1) > SIS_MAX_CMS)
   {
      STDERR_1("siscms_create:total colormap (%d) size exceed maximum limit",
	       siscms->cms_size)
      (void)free((char *)siscms);
      return(NULL);
   }
   if (siscms->cms_size < 1)
   {
      STDERR_1("siscms_create:total colormap (%d) size is less than 1",
	       siscms->cms_size)
      (void)free((char *)siscms);
      return(NULL);
   }

   return(siscms);
}

/************************************************************************
*                                                                       *
*  Destroy the colormap table.						*
*									*/
void
siscms_destroy(Siscms *siscms)
{
   (void)free((char *)siscms);
}

/************************************************************************
*                                                                       *
*  Read colormap value from the file.                                   *
*                                                                       *
*  Return OK or NOT_OK.                                                 *
*                                                                       */
static int
init_get_cms(char *filename,    /* filename containing colormap values */
     u_char *red,               /* red values */
     u_char *green,             /* green values */
     u_char *blue,              /* blue values */
     char *cms_name,            /* cms name  */
     int *cms_size)             /* cms size  */
{
#define RETURN_ERR      (void)fclose(file); return(NOT_OK)
#define RETURN_OK      (void)fclose(file); return(OK)
   FILE *file;          /* pointer to the open file */
   char buf[256];       /* buffer to store a line of data */
   char *ptr;           /* pointer position of the buffer */
   int ret_value;       /* return value from scanf */
   char defname[80];    /* default buffer name */
      
   /* Open initialization file */
   if ((file = fopen(filename, "r")) == NULL)
   {  
      STDERR_1("init_get_cms:fopen:cannot open colormap file: %s.", filename);
      return(NOT_OK);
   }

   /* Initialize sizes to be zero */
   *cms_size = 0;

   /* Read all colormap values */
   while (fgets(buf, 256, file))
   {
      ptr = buf;

      while ((*ptr == ' ') || (*ptr == '\t'))
         ptr++;

      if ((*ptr == NULL) || (*ptr == '#') || (*ptr == '\n'))/* STOP */
         continue;

      if (strncmp(ptr,cms_name,strlen(cms_name)) == 0)
      {
	 /* Skip the cms name */
	 while ((*ptr == ' ') || (*ptr == '\t'))
	   ptr++;
	 while ((*ptr != NULL) && ((*ptr != ' ') && (*ptr != '\t')))
	   ptr++;

         ret_value = sscanf(ptr, "%d%s", cms_size, defname);
         if (ret_value == 0)
         {
            STDERR_2("init_get_cms:sscanf:cannot get %s value from %s",
                     cms_name, filename);
            RETURN_ERR;
         }
         else if ((ret_value == 1) && (*cms_size))
         {
            /* Read the colormap entry values */
            if (read_colormap(file, red, green, blue, *cms_size) == NOT_OK)
            {
               STDERR_2("init_get_cms:read_colormap:Error in %s at %s",
                        filename, cms_name);
               RETURN_ERR;
            }
	    //(void)fclose(file);
         }   
         else if (ret_value == 2)
         {   
            if(default_colormap(defname, red, green, blue, *cms_size) == NOT_OK)
	    {
	       STDERR("init_get_cms:default_colormap");
	       RETURN_ERR;
	    }
         }
	 RETURN_OK;
      }
   }
   STDERR_2("init_get_cms:cannot find %s in %s", cms_name, filename);
   RETURN_ERR;
#undef RETURN_ERR
#undef RETURN_OK
}

/************************************************************************
*                                                                       *
*  Read colormap entry from a file.                                     *
*  Return OK or NOT_OK.                                                 *
*                                                                       */
static int
read_colormap(FILE *file,	/* filename */
        u_char *red,		/* red values */
	u_char *green,		/* green values */
	u_char *blue,		/* blue values */
        int size)		/* size of colormap */
{
   char buf[256];       /* buffer to store a line of data */
   char *ptr;           /* pointer position of the buffer */
   int i;               /* loop counter */
   int r,g,b;           /* red-green-blue values */

   /* Read the colormap entry values */
   i = 0;
   while (i < size)
   {
      if (fgets(buf, 256, file) == NULL)
      {
         STDERR("read_colormap:doesn't have enough values")
         return(NOT_OK);
      }

      ptr = buf;
      while ((*ptr == ' ') || (*ptr == '\t'))
      ptr++;

      if ((*ptr == NULL) || (*ptr == '#') || (*ptr == '\n'))/* STOP */
         continue;

      if (sscanf(ptr,"%d %d %d", &r, &g, &b) != 3)
      {
         STDERR_1("read_colormap:sscanf:missing values at index %d", i)
         return(NOT_OK);
      }
      red[i] = (u_char)r;
      green[i] = (u_char)g;
      blue[i] = (u_char)b;

      i++;
   }
   return(OK);
}

/************************************************************************
*                                                                       *
*  Set-up default colormap.						*
*  Return OK or NOT_OK.							*
*									*/
static int
default_colormap(char *defname,		/* colormap NAME */
	u_char *red,			/* red values */
	u_char *green,			/* green values */
	u_char *blue,			/* blue values */
	int size)			/* colormap size */
{
   int i;	/* loop counter */
   if (strcmp("DEFAULT_GRAY_COLOR", defname) == 0)
   {
      int i;
      for (i=0; i<size; i++)
      {
	 red[i] = green[i] = blue[i] = (i * 255) / (size -1);
      }
      return(OK);
   }
   else if (strcmp("DEFAULT_MARK_COLOR", defname) == 0)
   {
      for (i=0; i<size; i++)
      {
	 switch (i % 8)
	 {
	    case 0: red[i] = 0;   green[i] = 0;   blue[i] = 0;   break;
	    case 1: red[i] = 255; green[i] = 0;   blue[i] = 0;   break;
	    case 2: red[i] = 0;   green[i] = 255; blue[i] = 0;   break;
	    case 3: red[i] = 0;   green[i] = 0;   blue[i] = 255; break;
	    case 4: red[i] = 255; green[i] = 0;   blue[i] = 255; break;
	    case 5: red[i] = 255; green[i] = 255; blue[i] = 0;   break;
	    case 6: red[i] = 0;   green[i] = 255; blue[i] = 255; break;
	    case 7: red[i] = 255; green[i] = 255; blue[i] = 255; break;
	 }
      }

      return (OK);
   }
   else if (strcmp("RGB_GRADIENT_COLOR", defname) == 0)
   {
      int    j, k, unit_size, val ;
 
      unit_size = (int)( (float)size / 3.0 ) ;
      k = 0 ;

      for (j=0; j<unit_size; j++) {
	 val = (int)( (float)(j * 255) / (float)(unit_size -1) ) ;
	 red[k]   = (u_char)val ;
	 green[k] = (u_char)0 ;
	 blue[k]  = (u_char)0 ;
	 ++k ;
      }
      for (j=0; j<unit_size; j++) {
	 val = (int)( (float)(j * 255) / (float)(unit_size -1) ) ;
	 red[k]   = (u_char)0 ;
	 green[k] = (u_char)val ;
	 blue[k]  = (u_char)0 ;
	 ++k ;
      }
      for (j=0; j<unit_size; j++) {
	 val = (int)( (float)(j * 255) / (float)(unit_size -1) ) ;
	 red[k]   = (u_char)0 ; 
	 green[k] = (u_char)0 ; 
	 blue[k]  = (u_char)val ;
	 ++k ;
      }
      return(OK);
   }
   else if (strcmp("DEFAULT_FALSE_COLOR", defname) == 0)
   {
      STDERR("DEFAULT_FALSE_COLOR is not yet supported");
      return (NOT_OK);
   }

   STDERR_1("default_colormap:No such default %s", defname);
   return(NOT_OK);
}
