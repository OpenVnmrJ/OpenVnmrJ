/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _SISCMS_H
#define	_SISCMS_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

#include <sys/types.h>

#define	SIS_MAX_CMS	256

typedef enum
{
   SISCMS_1,
   SISCMS_2,
   SISCMS_3,
   SISCMS_4
} Siscms_type;

/* Note that this colormap contains 3 types of color values so that the */
/* user can refer to each type of colormap when he/she wants to use     */
/* the colormap.							*/
typedef struct _siscms
{
   u_char r[SIS_MAX_CMS];	/* Red values */
   u_char g[SIS_MAX_CMS];	/* Green values */
   u_char b[SIS_MAX_CMS];	/* Blue values */
   int st_cms1;			/* starting index of colormap 1 */
   int st_cms2;			/* starting index of colormap 2 */
   int st_cms3;			/* starting index of colormap 3 */
   int size_cms1;		/* size of colormap 1 */
   int size_cms2;		/* size of colormap 2 */
   int size_cms3;		/* size of colormap 3 */
   int cms_size;		/* total size of all colormaps */
} Siscms;

/************************************************************************
*                                                                       *
*  Create a colormap table.                                             *
*  The value stored in the colormap will be in order according to the   *
*  cmsname 1, 2, 3.  Typically,  cms name 1 is used for graphics        *
*  drawing,  cms name 2 is used for gray-scale, and cms name 3 is       *
*  used for false-color.  The caller can specify NULL for the name if   *
*  he doesn't expect all three colormaps to be loaded.                  *
*  									*
*  The function will read in an initialized colormap file from a	*
*  directory specified in 'filename', and colormap name specified by	*
*  cmsname<x>.  The format of the colormap file will look like the 	*
*  following:								*
*									*
*-----------------------------------------------------------------------*
*									*
*  mark-color	8							*
*	<red>	<green>	<blue>						*
*	<red>	<green>	<blue>						*
*	. . .								*
*  gray-color	128 DEFAULT_GRAY_COLOR					*
*  false-color	31							*
*	<red>	<green>	<blue>						*
*	<red>	<green>	<blue>						*
*	. . .								*
*									*
*-----------------------------------------------------------------------*
*									*
*  Explantion:								*
*  -----------								*
*  The arguments cmsname1 is "mark-color", cmsname2 is "gray_color"	*
*  and cmsname3 is "false-color".  After the line 'mark-color   8',	*
*  it should be followed by 8 lines of red/green/blue values.  Since	*
*  'gray-color' specifies the default value with 128 gray-level, the	*
*  function will generates the red/green/blue for it.  Then, it follows	*
*  by 31 lines of 'false-color'.					*
*  									*
*  You can specify the default color for those three cmsname:		*
*  	mark-color	8	DEFAULT_MARK_COLOR			*
*  	gray-color	128	DEFAULT_GRAY_COLOR			*
*  	false-color	31	DEFAULT_FALSE_COLOR			*
*  However, it is not recommended to specify 'mark-color' and 		*
*  'false-color' to be default colors.					*
*									*
*  After the function reads in the initialized file, it will store all	*
*  colormap values, starting indexes, and sizes of each colormap in	*
*  structure Siscms.							*
*									*
*  The purpose to use initailzied file is to enable the user to 	*
*  configure his colormap.  For example, he may configures his colormap	*
*  to be:								*
*	mark-color	8						*
*		...							*
*	gray-color	100	DEFAULT_GRAY_COLOR			*
*	false-color	64						*
*		...							*
*-----------------------------------------------------------------------*
*									*
*  Return a pointer to object or NULL.                                  *
*                                                                       */
extern Siscms *
siscms_create(char *filename,   /* file containing colormap values */
        char *cmsname1,          	/* cms NAME 1 */ 
        char *cmsname2,          	/* cms NAME 2 */
        char *cmsname);         	/* cms NAME 3 */


/************************************************************************
*                                                                       *
*  Destroy the colormap table.                                          *
*                                                                       */
extern void
siscms_destroy(Siscms *siscms);

#endif (not) _SISCMS_H
