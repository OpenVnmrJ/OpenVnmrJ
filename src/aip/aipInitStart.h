/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef _INITSTART_H
#define _INITSTART_H

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

/************************************************************************
*                                                                       *
*  Get the object initialize value from a startup file.                 *
*  The following shows the example of the usage:			*
*  	window.init							*
*		UI 0 0 100 100						*
*		LOAD 100 20 300 400					*
*  The calling routine should be:					*
* 	init_get_val(".../window.init","UI","dddd",&v1,&v2,&v3,&v4)	*  
* 	init_get_val(".../window.init","LOAD","dddd",&v1,&v2,&v3,&v4)	*  
*									*
*  Note that it supports format:					*
*  	d  	int							*
*	D	long							*
*	u	unsigned int						*
*	U	unsigned long						*
*	f	float							*
*	F	double							*
*	o	octal							*
*	x	hexadecimal						*
*	c	char							*
*	s	char *							*
*									*
*  Return OK or NOT_OK.                                                 *
*                                                                       */
extern int
init_get_val(char *filename,	/* start-up file name */
	char *objname,     	/* object name in start-up file */
	char *format,      	/* value formats */
 	 ...);

/************************************************************************
*                                                                       *
*  Initialize the environment name you want to use.                     *
*                                                                       */
extern void
init_set_env_name(char *envame);	/* Environment name */

/************************************************************************
*                                                                       *
*  Get environment name for initialiazation.                            *
*  If the environment is not initialized, use the default name INITDIR. *
*  Always return a pointer to the filename as:                          *
*       $USER_ENV   ---> if you have called init_set_env_name()         *
*  or   $INITDIR    ---> if you didn't call init_set_env_name()         *
*                                                                       */
extern char *
init_get_env_name(char *filename);	/* user supplied buffer */

/************************************************************************
*                                                                       *
*  Get window initialization filename.                                  *
*  If the environment is not initialized, use the default name INITDIR. *
*  Always return pointer to the filename as:                            *
*      $USER_ENV/window.init --> if you have called init_set_env_name() *
*  or  $INITDIR/window.init  --> if you didn't call init_set_env_name() *
*                                                                       */
extern char *
init_get_win_filename(char *filename);	/* user supplied buffer */

/************************************************************************
*                                                                       *
*  Get colormap initialization filename.                                *
*  If the environment is not initialized, use the default name INITDIR. *
*  Always return pointer to the filename as:                            *
*     $USER_ENV/colormap.init --> if you have called init_set_env_name()*
*  or $INITDIR/colormap.init  --> if you didn't call init_set_env_name()*
*                                                                       */
extern char *
init_get_cmp_filename(char *filename);	/* user supplied buffer */


/************************************************************************
*									*
*  Get CSI parameter initialization filename.				*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*     $USER_ENV/csiparam.init --> if you have called init_set_env_name()*
*  or $INITDIR/csiparam.init  --> if you didn't call init_set_env_name()*
*									*/
extern char *
init_get_csiparam_filename(char *filename);	/* user supplied buffer */


/************************************************************************
*									*
*  Get initialization filename with given filename 			*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*     $USER_ENV/xxxx.init --> if you have called init_set_env_name()	*
*  or $INITDIR/xxxx.init  --> if you didn't call init_set_env_name()	*
*									*/
extern char *
init_get_filename(char *name, char *fullname);


#endif 
