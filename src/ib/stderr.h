/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _STDERR_H
#define	_STDERR_H
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

#include <stdio.h>
#include <assert.h>

/* To shut off warning "variable not used ......" */
#ifndef WARNING_OFF
#define	WARNING_OFF(v)	(v) = (v)
#endif /* WARNING_OFF */

/* Return values */
#ifndef OK
#define	OK	0
#define	NOT_OK	-1
#endif /* OK */

#ifndef SUCCESS
#define	SUCCESS	0
#define	ERROR	-1
#endif /* SUCCESS */

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

/* Errors */
#define	PERROR(str)\
	{  char err_buf[256]; \
	   (void)sprintf(err_buf,"%s: %d: %s",__FILE__, __LINE__, str);\
	   perror(err_buf);\
	}
#define	PERROR_1(str,arg1)\
	{  char err_buf[256]; \
	   (void)sprintf(err_buf,"%s: %d: "str,__FILE__, __LINE__, arg1);\
	   perror(err_buf);\
	}
#define	PERROR_2(str,arg1,arg2)\
	{  char err_buf[256]; \
	   (void)sprintf(err_buf,"%s: %d: "str,__FILE__, __LINE__, arg1, arg2);\
	   perror(err_buf);\
	}
#define	PERROR_3(str,arg1,arg2,arg3)\
	{  char err_buf[256]; \
	   (void)sprintf(err_buf,"%s: %d: "str,__FILE__, __LINE__, arg1, arg2, arg3);\
	   perror(err_buf);\
	}

#define	STDERR(str)\
	   (void)fprintf(stderr,"%s: %d: %s\n",__FILE__, __LINE__, str);

#define	STDERR_1(str,arg1)\
	   (void)fprintf(stderr,"%s: %d: "str"\n",__FILE__, __LINE__, arg1);

#define	STDERR_2(str,arg1,arg2)\
	   (void)fprintf(stderr,"%s: %d: "str"\n",__FILE__, __LINE__, arg1, arg2);

#define	STDERR_3(str,arg1,arg2,arg3)\
	   (void)fprintf(stderr,"%s: %d: "str"\n",__FILE__, __LINE__, arg1, arg2, arg3);
#endif /* (not) _STDERR_H */
