/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPSTDERR_H
#define	AIPSTDERR_H

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

int aipSetDebug(int argc, char *argv[], int retc, char *retv[]);
int aipGetDebug();
void aipDprint(int bit, const char *format);
bool isDebugBit(int bit);

#define DEBUGBIT_0 0            /* Con/de-structors from shared_ptr */
#define DEBUGBIT_1 1            /* Track current RefCount */
#define DEBUGBIT_2 2            /* Con/de-structors of ROI / CoordLists */
#define DEBUGBIT_3 3            /* Orientation matrices */
#define DEBUGBIT_4 4            /* Print "redisplay" commands */
#define DEBUGBIT_5 5            /* Time image drawing */
#define DEBUGBIT_6 6            /* DDL symbol table memory management */
#define DEBUGBIT_7 7            /* Data loading / deleting */
#define DEBUGBIT_8 8            /* Image Math */

/*
 * Use Vnmr command aipSetDebug(bitNbr) to turn on debug mode.
 */

#endif /* (not) AIPSTDERR_H */
