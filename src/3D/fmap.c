/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <sys/file.h>
#include "constant.h"

#define FT3D
#include "command.h"
#undef FT3D


#define STD_ARRAY	0x1
#define NI_ARRAY	0x2
#define NI2_ARRAY	0x4

#define NORMAL		0x100
#define REVERSE		0x200
#define FIRSTLAST	0x400


struct _elemInfo
{
   float	mult;		/* combination multiplier	*/
   int		fd;		/* File descriptor		*/
   int		offset;		/* in bytes			*/
};

typedef struct _elemInfo elemInfo;

struct _sumInfo
{
   elemInfo	*eidata;	/* has "nei" elements		*/
   int		nei;
};

typedef struct _sumInfo sumInfo;

struct _arrayInfo
{
   int	status;			/* ni, ni2, or non-time array; acq. mode */
   int	arrayno;
   int	nelems;
};

typedef struct _arrayInfo arrayInfo;



/*-------------------------------------------------------
|							|
|  Map format:						|
|							|
|     [						  	|
|        fid11(1.0) fid21(1.0) fid31(1.0) fid41(1.0);	|
|        fid12(1.0) fid22(1.0) fid32(1.0);		|
|        fid13(1.0) fid23(1.0);				|
|        fid14(1.0);					|
|     ]							|
|     <							|
|        ni:r phase(2):n ni2:n phase(2):r;		|
|	 ni:n ni2:n phase(2):n phase2(2):n;		|
|	 ni phase2(2) phase(2) ni2;			|
|     >							|
|							|
|							|
|   Each row in the file description, enclosed in [],	|
|   specifies a set of FID's which span the complete	|
|   array space.  The order of the array elements in    |
|   the array space is given by the first line in the	|
|   section enclosed in <>.  How each array is done	|
|   can be specified by options that are delimited	|
|   from the array element name by a :, e.g., ni:r.	|
|   In this case, 'r' means that ni was done in reverse	|
|   order.  The default is 'n', which means that the	|
|   array was done in the normal order.			|
|							|
|   A set of FID directory names which spans the com-	|
|   plete array space is terminated by a ;.  Also,	|
|   the FID directory names are assumed to have a .fid	|
|   file extension.  Subsequent rows of FID directory	|
|   names enclosed in [] specify another complete	|
|   array space, an element of which is to be first	|
|   multiplied by the value enclosed in () before the	|
|   FID is added to the appropriate element from the	|
|   set of FID directories specified in the first row.  |
|   Subsequent rows within <> specify the array order	|
|   for their row counterpart within [].                |
|							|
+------------------------------------------------------*/



/*---------------------------------------
|                                       |
|           createFIDmap()/0            |
|                                       |
+--------------------------------------*/
int createFIDmap()
{
/*
   parseFIDinfo();
   parseARYinfo();
*/

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     mapFIDblock()/1		|
|					|
+--------------------------------------*/
int mapFIDblock(int block_no)
{
   return(block_no);	/* temporary addition */
}

