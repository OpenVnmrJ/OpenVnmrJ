/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *	Byte swapping for endian conversion	<byteswap_linux.h>
 */

#ifndef	_BYTESWAP_LINUX_H
#define	_BYTESWAP_LINUX_H	1

/*-----------------------------------*/
/*----  Varian data file handler ----*/
/*-----------------------------------*/

/*  Slightly different from Varian documentation : all 'long' values were replaced by    */
/*  'int' values, because 'long' values are 8 bytes long on Linux 64 bits, and           */
/*  4 bytes long on 32 bits systems. To preseve compatibility, Varian made the choice    */
/*  to always write data files in the SUN (big endian) format, and with 4 bytes 'long'   */
/*  values.                                                                              */



/*  The file headers are defines as follows:                     */

/*****************/
struct datafilehead
/*****************/
/* Used at the beginning of each data file (fid's, spectra, 2D)  */
{
   int    nblocks;       /* number of blocks in file			*/
   int    ntraces;       /* number of traces per block			*/
   int    np;            /* number of elements per trace		*/
   int    ebytes;        /* number of bytes per element		*/
   int    tbytes;        /* number of bytes per trace			*/
   int    bbytes;        /* number of bytes per block			*/
   short   vers_id;       /* software version and file_id status bits	*/
   short   status;        /* status of whole file			*/
   int	   nbheaders;	  /* number of block headers			*/
};



/*******************/
struct datablockhead
/*******************/
/* Each file block contains the following header        */
{
   short 	  scale;	/* scaling factor                   */
   short 	  status;	/* status of data in block          */
   short 	  index;	/* block index                      */
   short	  mode;		/* mode of data in block	    */
   int		  ctcount;	/* ct value for FID		    */
   float 	  lpval;	/* F2 left phase in phasefile       */
   float 	  rpval;	/* F2 right phase in phasefile      */
   float 	  lvl;		/* F2 level drift correction        */
   float 	  tlt;		/* F2 tilt drift correction         */
};


/*---------------------------*/
/*---- Check file header ----*/
/*---------------------------*/
int check_file_header(struct datafilehead *dfh);


/*-----------------------------*/
/*---- Reverse file header ----*/
/*-----------------------------*/
void reversedfh(struct datafilehead *dfh);

/*------------------------------*/
/*---- Reverse block header ----*/
/*------------------------------*/
void reversedbh(struct datablockhead *dbh);


void reverse2ByteOrder(int nele,char *ptr);
void reverse4ByteOrder(int nele,char *ptr);
void reverse8ByteOrder(int nele,char *ptr);


#endif /* byteswap_linux.h  */

