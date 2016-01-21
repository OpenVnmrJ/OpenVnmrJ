/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define S_MAGNITUDE	0x40	/* 0 = phased data   1 = magnitude data	*/
#define S_FIRST_D	0x80	/* 0 = no FT(t3)     1 = FT(t3)		*/
#define S_SECOND_D	0x100	/* 0 = no FT(t2)     1 = FT(t2)		*/
#define S_THIRD_D	0x200	/* 0 = no FT(t1)     1 = FT(t1)		*/
#define PROCSTART	0x8000	/* 1 = processing has been started	*/

#define FILE_CLOSED	-1
#define READ		0
#define WRITE		1
#define LASTFID		10
#define ALL		-1

#define CHECK		0x1
#define READONLY	0x2


struct _f3blockpar
{
   int  hcptspertrace;
   int  bytesperfid;
   int  dpflag;
};
 
typedef struct _f3blockpar f3blockpar;

struct _phasepar
{
   float	rp;
   float	lp;
   float	rp1;
   float	lp1;
   float	rp2;
   float	lp2;
};

typedef struct _phasepar phasepar;

struct _filedesc
{
   int	ndatafd;
   int  dataexists;
   int	result;
   int	*dlklist;
   int	*dfdlist;
};

typedef struct _filedesc filedesc;

struct _datafileheader
{
   dfilehead		Vfilehead;	/* in "data.h" */
   f3blockpar		f3blockinfo;
   phasepar		phaseinfo;
   float		*coefvals;
   float		maxval;		/* signed	*/
   float		minval;		/* signed	*/
   int			mode;		/* display mode	*/
   int			version3d;
   int			ncoefbytes;
   int			lastfid;
   int			ndatafiles;
   int			nheadbytes;
};

typedef struct _datafileheader datafileheader;
