/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>


#define MAXPATH		256
#define ERROR		-1
#define COMPLETE	0
#define FALSE		0
#define TRUE		1

#define S_DATA		0x1
#define S_32		0x4
#define S_FLOAT         0x8
#define MAX_16INT	32767
#define FIRST_EXP	1
#define LAST_EXP	9999

#define SELECT          0
#define SET             1
#define EITHER		2
#define INPUT_FILE	10
#define OUTPUT_FILE	11
#define EXP_NUM         12
#define OVER_WRITE      13

#define READ_ERR	-2
#define WRITE_ERR	-3
#define LAST_FID	-4


struct _ipar
{
   int	pmode;
   int	vset;
   int	ival;
};

typedef struct _ipar ipar;

struct _spar 
{ 
   int  pmode;
   int	vset;
   char	sval[MAXPATH];
}; 

typedef struct _spar spar;
 
struct _comInfo
{
   ipar overwrite;
   ipar	curexp;
   spar	inputdirpath;
   spar outputdirpath;
};
 
typedef struct _comInfo comInfo;

struct _fileheader
{
   long		nblocks;
   long		ntraces;
   long		np;
   long		ebytes;
   long		tbytes;
   long		bbytes;
   short	vers_id;
   short	status;
   long		nbheaders;
};

typedef struct _fileheader fileheader;

struct _blockheader
{
   short	scale;
   short	status;
   short	index;
   short	mode;
   long		ctcount;
   float	lpval;
   float	rpval;
   float	lvl;
   float	tlt;
};

typedef struct _blockheader blockheader;

struct _datapointer
{
   blockheader	blockhead;
   char		*data;
};

typedef struct _datapointer datapointer;

struct _fdInfo
{
   int	inp;
   int	out;
};

typedef struct _fdInfo fdInfo;

#if defined(__INTERIX) || defined(MACOS)
#include <arpa/inet.h>
#elif LINUX
#include <netinet/in.h>
#include <inttypes.h>
#endif

/* mirrors datafilehead to allow little to big endian swapping */
struct datafileheadSwapByte
{
   long    l1;       /* number of blocks in file			*/
   long    l2;       /* number of traces per block			*/
   long    l3;            /* number of elements per trace		*/
   long    l4;        /* number of bytes per element		*/
   long    l5;        /* number of bytes per trace			*/
   long    l6;        /* number of bytes per block			*/
   short   s1;       /* software version and file_id status bits	*/
   short   s2;        /* status of whole file			*/
   long	   l7;	  /* number of block headers			*/
};

typedef union
{
   fileheader *in1;
   struct datafileheadSwapByte *out;
} datafileheadSwapUnion;

/* really these should use LITTLE_ENDIAN as the conditional compile, but the choice 
   was already made, GMB */

#ifdef LINUX

#define DATAFILEHEADER_SET_HTON(pfid2,pfid1) \
{ \
          datafileheadSwapUnion hU1, hU2;	\
          hU1.in1 = pfid1;	\
          hU2.in1 = pfid2;	\
          hU2.out->s1 = htons(hU1.out->s1);	\
          hU2.out->s2 = htons(hU1.out->s2);	\
          hU2.out->l1 = htonl(hU1.out->l1);	\
          hU2.out->l2 = htonl(hU1.out->l2);	\
          hU2.out->l3 = htonl(hU1.out->l3);	\
          hU2.out->l4 = htonl(hU1.out->l4);	\
          hU2.out->l5 = htonl(hU1.out->l5);	\
          hU2.out->l6 = htonl(hU1.out->l6);	\
          hU2.out->l7 = htonl(hU1.out->l7);	\
}

#define DATAFILEHEADER_CONVERT_NTOH(pfidfileheader) \
{ \
          datafileheadSwapUnion hU;	\
          hU.in1 = pfidfileheader;	\
          hU.out->s1 = ntohs(hU.out->s1);	\
          hU.out->s2 = ntohs(hU.out->s2);	\
          hU.out->l1 = ntohl(hU.out->l1);	\
          hU.out->l2 = ntohl(hU.out->l2);	\
          hU.out->l3 = ntohl(hU.out->l3);	\
          hU.out->l4 = ntohl(hU.out->l4);	\
          hU.out->l5 = ntohl(hU.out->l5);	\
          hU.out->l6 = ntohl(hU.out->l6);	\
          hU.out->l7 = ntohl(hU.out->l7);	\
}

#else

#define DATAFILEHEADER_CONVERT_NTOH(pfidfileheader) 

#endif

/* mirrors datablockhead to allow little to big endian swapping */
struct datablockheadSwapByte
{
   short s1;
   short s2;
   short s3;
   short s4;
   long  l1;
   long  l2;
   long  l3;
   long  l4;
   long  l5;
};

typedef union
{
   blockheader *in1;
   struct datablockheadSwapByte *out;
} datablockheadSwapUnion;

/* really these should use LITTLE_ENDIAN as the conditional compile, but the choice 
   was already made, GMB */

#ifdef LINUX

#define DATABLOCKHEADER_SET_HTON(pfid2,pfid1) \
{ \
          datablockheadSwapUnion hU1, hU2;	\
          hU1.in1 = pfid1;	\
          hU2.in1 = pfid2;	\
          hU2.out->s1 = htons(hU1.out->s1);	\
          hU2.out->s2 = htons(hU1.out->s2);	\
          hU2.out->s3 = htons(hU1.out->s3);	\
          hU2.out->s4 = htons(hU1.out->s4);	\
          hU2.out->l1 = htonl(hU1.out->l1);	\
          hU2.out->l2 = htonl(hU1.out->l2);	\
          hU2.out->l3 = htonl(hU1.out->l3);	\
          hU2.out->l4 = htonl(hU1.out->l4);	\
          hU2.out->l5 = htonl(hU1.out->l5);	\
}

#define DATABLOCKHEADER_CONVERT_NTOH(pfidblockheader) \
{ \
          datablockheadSwapUnion hU;	\
          hU.in1 = pfidblockheader;	\
          hU.out->s1 = ntohs(hU.out->s1);	\
          hU.out->s2 = ntohs(hU.out->s2);	\
          hU.out->s3 = ntohs(hU.out->s3);	\
          hU.out->s4 = ntohs(hU.out->s4);	\
          hU.out->l1 = ntohl(hU.out->l1);	\
          hU.out->l2 = ntohl(hU.out->l2);	\
          hU.out->l3 = ntohl(hU.out->l3);	\
          hU.out->l4 = ntohl(hU.out->l4);	\
          hU.out->l5 = ntohl(hU.out->l5);	\
}

#else

#define DATABLOCKHEADER_CONVERT_NTOH(pfidblockheader)

#endif

int getVNMRexppath( comInfo *pinfo);

/*---------------------------------------
|                                       |
|            readselect()/2             |
|                                       |
+--------------------------------------*/
ipar readselect(argstr, pinfo)
char    *argstr;
comInfo	*pinfo;
{
   ipar	tmp;


   tmp.vset = FALSE;
   if (*argstr++ == '-')
   {
      switch (*argstr)
      {
         case 'e':   tmp.pmode = pinfo->curexp.pmode;
                     tmp.ival = EXP_NUM;
                     return(tmp);
         case 'f':   tmp.pmode = pinfo->overwrite.pmode;
		     pinfo->overwrite.vset = TRUE;
		     pinfo->overwrite.ival = TRUE;
		     tmp.ival = OVER_WRITE;
		     return(tmp);
         case 'i':   tmp.pmode = pinfo->inputdirpath.pmode;
		     tmp.ival = INPUT_FILE;
		     return(tmp);
         case 'o':   tmp.pmode = pinfo->outputdirpath.pmode;
		     tmp.ival = OUTPUT_FILE;
		     return(tmp);
         default:    printf("\ncompressfid:  invalid option\n\n");
		     tmp.pmode = EITHER;
		     tmp.ival = ERROR;
                     return(tmp);
      }
   }          
   else
   {
      printf("\ncompressfid:  improper input format\n\n");
      tmp.pmode = EITHER;
      tmp.ival = ERROR;
      return(tmp);
   }
}


/*---------------------------------------
|                                       |
|            setselect()/3              |
|                                       |
+--------------------------------------*/
int setselect(argstr, selection, pinfo)
char    *argstr;
int     selection;
comInfo *pinfo;
{      
   switch (selection)
   {
      case INPUT_FILE:    (void) strcpy(pinfo->inputdirpath.sval, argstr);
		 	  pinfo->inputdirpath.vset = TRUE;
                          break;
      case OUTPUT_FILE:   (void) strcpy(pinfo->outputdirpath.sval, argstr);
			  pinfo->outputdirpath.vset = TRUE;
                          break;
      case EXP_NUM:       pinfo->curexp.ival = atoi(argstr);
			  if ( (pinfo->curexp.ival < FIRST_EXP) ||
                               (pinfo->curexp.ival > LAST_EXP) )
                          {
                             printf("\ncompressfid:  invalid experiment\n\n");
                             return(ERROR);
                          }
			  pinfo->curexp.vset = TRUE;
			  break;
      default:            break;
   }
 
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|             initinfo()/1              |
|                                       |
+--------------------------------------*/
static void initinfo(pinfo)
comInfo	*pinfo;
{
   pinfo->overwrite.pmode = SELECT;
   pinfo->curexp.pmode = SET;
   pinfo->inputdirpath.pmode = SET;
   pinfo->outputdirpath.pmode = SET;

   pinfo->overwrite.vset = FALSE;
   pinfo->curexp.vset = FALSE;
   pinfo->inputdirpath.vset = FALSE;
   pinfo->outputdirpath.vset = FALSE;

   pinfo->overwrite.ival = FALSE;
   pinfo->curexp.ival = 0;
   (void) strcpy(pinfo->inputdirpath.sval, "");
   (void) strcpy(pinfo->outputdirpath.sval, "");
}


/*---------------------------------------
|                                       |
|            parseinput()/2             |
|                                       |
+--------------------------------------*/
comInfo *parseinput(argc, argv)
char   		*argv[];
int    		argc;
{
   int          argno = 1,
                mode = SELECT;
   ipar		selection,
		readselect();
   comInfo      *pinfo;
   void		initinfo();
 
 
/*****************************************
*  Allocate memory for the command line  *
*  information structure.                *
*****************************************/

   if ( (pinfo = (comInfo *) malloc( (unsigned) sizeof(comInfo) ))
                == NULL )
   {
      printf("\ncompressfid:  cannot allocate memory for command data\n\n");
      return(NULL);
   }


/******************************************
*  Initialize the pointer to the command  *
*  information structure.                 *
******************************************/
 
   initinfo(pinfo);
 

/**********************************
*  Parse the input command line.  *
**********************************/

   selection.ival = ERROR;
   while (argno < argc)
   {
      switch (mode)
      {
         case SELECT:	selection = readselect(argv[argno++], pinfo);
			if (selection.ival == ERROR)
			   return(NULL);
			mode = selection.pmode;
                        break;
         case SET:      if ( setselect(argv[argno++], selection.ival, pinfo) )
                           return(NULL);
                        mode = SELECT;
                        break;
         case EITHER:	mode = (  ( *argv[argno] == '-' ) ? SELECT : SET );
			break;
         default:       break;
      }
   }

 
   if (mode == SET)
   {
      printf("\ncompressfid:  improper input format\n\n");
      return(NULL);
   }


/*******************************************
*  Check to see that the input and output  *
*  file paths have been specified on the   *
*  command line.                           *
*******************************************/

   if (pinfo->inputdirpath.vset)
   {
      (void) strcat(pinfo->inputdirpath.sval, ".fid");
   }
   else
   {
      if (pinfo->curexp.vset)
      {
         if ( getVNMRexppath(pinfo) )
            return(NULL);

         (void) strcat(pinfo->inputdirpath.sval, "/acqfil");
         pinfo->inputdirpath.vset = TRUE;
      }
      else
      {
         printf("\ncompressfid:  input directory path undefined\n\n");
         return(NULL);
      }
   }
 
   if (pinfo->outputdirpath.vset)
   {
      (void) strcat(pinfo->outputdirpath.sval, ".fid");
   }
   else
   {
      printf("\ncompressfid:  output directory path undefined\n\n");
      return(NULL);
   }

   if ( strcmp(pinfo->inputdirpath.sval, pinfo->outputdirpath.sval) == 0 )
   {
      printf(
	 "\ncompressfid:  input and ouput directories must be different\n\n");
      return(NULL);
   }

   return(pinfo);
}


/*---------------------------------------
|                                       |
|          getVNMRexppath()/1           |
|                                       |
+--------------------------------------*/
int getVNMRexppath( comInfo *pinfo)
{
   char		expstr[5],
		*userdir;
   extern char	*getenv();


   if ( (userdir = getenv("vnmruser")) == NULL)
   {
      printf("\ncompressfid:  no `$vnmruser` environmental variable\n\n");
      return(ERROR);
   }

   (void) sprintf(expstr, "%d", pinfo->curexp.ival);
   (void) strcpy(pinfo->inputdirpath.sval, userdir);
   (void) strcat(pinfo->inputdirpath.sval, "/exp");
   (void) strcat(pinfo->inputdirpath.sval, expstr);

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|           openFIDfiles()/1            |
|                                       |
+--------------------------------------*/
fdInfo *openFIDfiles(pinfo)
comInfo	*pinfo;
{
   char		filepath[MAXPATH];
   fdInfo	*fdinfo;


   if ( (fdinfo = (fdInfo *) malloc( (unsigned) sizeof(fdInfo) )) == NULL )
   {
      printf("\ncompressfid:  insufficient memory\n\n");
      return(NULL);
   }

   (void) strcpy(filepath, pinfo->inputdirpath.sval);
   (void) strcat(filepath, "/fid");

   if ( access(filepath, R_OK) )
   {
      printf("\ncompressfid:  cannot read input file `%s`\n\n", filepath);
      return(NULL);
   }

   if ( (fdinfo->inp = open(filepath, O_RDONLY, 0440)) < 0 )
   {
      printf("\ncompressfid:  cannot open input file `%s`\n\n", filepath);
      return(NULL);
   }
   return(fdinfo);
}


/*---------------------------------------
|                                       |
|             getheader()/1             |
|                                       |
+--------------------------------------*/
fileheader *getheader(fd)
int	fd;
{
   fileheader	*filehead;


   if ( (filehead = (fileheader *) malloc( (unsigned) sizeof(fileheader) ))
	    == NULL )
   {
      printf("\ncompressfid:  insufficient memory\n\n");
      return(NULL);
   }

   if ( read(fd, (char *)filehead, sizeof(fileheader)) != sizeof(fileheader) )
   {
      printf("\ncompressfid:  error in reading input file header\n\n");
      return(NULL);
   }
   DATAFILEHEADER_CONVERT_NTOH(filehead);

   if ((filehead->status & S_FLOAT) != 0)
   {
      printf("\ncompressfid: Original FID file is floating point\n\n");
      return(NULL);
   }
   if ( (~filehead->status) & S_32 )
   {
      printf("\ncompressfid: Original FID file is not double-precision\n\n");
      return(NULL);
   }

   return(filehead);
}


/*---------------------------------------
|                                       |
|           modifyheader()/1            |
|                                       |
+--------------------------------------*/
fileheader modifyheader(inputhead)
fileheader	*inputhead;
{
   fileheader	outputhead;


   outputhead.nblocks   = inputhead->nblocks;
   outputhead.ntraces   = inputhead->ntraces;
   outputhead.np        = inputhead->np;
   outputhead.vers_id   = inputhead->vers_id;
   outputhead.nbheaders = inputhead->nbheaders;

   outputhead.status    = inputhead->status & (~S_32);
   outputhead.ebytes    = sizeof(short);
   outputhead.tbytes    = outputhead.ebytes * outputhead.np;
   outputhead.bbytes    = outputhead.tbytes * outputhead.ntraces +
				sizeof(blockheader);
	/*
	   Not set up for multiple block headers
	   in FID data.
	*/

   return(outputhead);
}


/*---------------------------------------
|                                       |
|            writeheader()/1            |
|                                       |
+--------------------------------------*/
int writeheader(fd, filehead)
int		fd;
fileheader	*filehead;
{
#ifdef LINUX
   fileheader tmphead;
   DATAFILEHEADER_SET_HTON( &tmphead, filehead);
   if ( write(fd, (char *) &tmphead, sizeof(fileheader)) != sizeof(fileheader) )
   {
      printf("\ncompressfid:  error in writing output file header\n\n");
      return(ERROR);
  }
#else
   if ( write(fd, (char *)filehead, sizeof(fileheader)) != sizeof(fileheader) )
   {
      printf("\ncompressfid:  error in writing output file header\n\n");
      return(ERROR);
  }
#endif

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|           closeFIDfiles()/1           |
|                                       |
+--------------------------------------*/
void closeFIDfiles(fdinfo)
fdInfo	*fdinfo;
{
   (void) close(fdinfo->inp);
   (void) close(fdinfo->out);
}


/*---------------------------------------
|                                       |
|             readdata()/3              |
|                                       |
+--------------------------------------*/
int readdata(fd, datapntr, nfidwords)
int		fd,
		nfidwords;
datapointer	*datapntr;
{
   if ( read(fd, (char *) (&(datapntr->blockhead)), sizeof(blockheader))
		!= sizeof(blockheader) )
   {
      return(LAST_FID);
   }

   DATABLOCKHEADER_CONVERT_NTOH( &(datapntr->blockhead) );
   if ( !(datapntr->blockhead.status & S_DATA) )
      return(LAST_FID);

   if ( read(fd, datapntr->data, nfidwords*sizeof(int)) !=
		nfidwords*sizeof(int) )
   {
      return(READ_ERR);
   }
		
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            modifydata()/3             |
|                                       |
+--------------------------------------*/
void modifydata(inpdatapntr, outdatapntr, npts)
int		npts;
datapointer	*inpdatapntr,
		*outdatapntr;
{
   register short	scale = 0,
			*out;
   register int		i,
			value,
			divisor = 1,
			max = 0,
			*inp;


   inp = (int *) (inpdatapntr->data);
   out = (short *) (outdatapntr->data);

   for (i = 0; i < npts; i++)
   {
#ifdef LINUX
      *inp = ntohl( *inp );
#endif
      value = *inp++;
      if (value < 0)
         value *= -1;
      if (value > max)
         max = value;
   }

   while (max > MAX_16INT)
   {
      scale += 1;
      divisor *= 2;
      max /= 2;
   }
         
   outdatapntr->blockhead = inpdatapntr->blockhead;
   outdatapntr->blockhead.scale = scale;
   outdatapntr->blockhead.status = inpdatapntr->blockhead.status & (~S_32);
   inp = (int *) (inpdatapntr->data);

   if (divisor == 1)
   {
      for (i = 0; i < npts; i++)
         *out++ = (short) (*inp++);
   }
   else
   {
      for (i = 0; i < npts; i++)
         *out++ = (short) ( (*inp++) / divisor );
   }
}


/*---------------------------------------
|                                       |
|             writedata()/3             |
|                                       |
+--------------------------------------*/
int writedata(fd, datapntr, nfidwords)
int		fd,
		nfidwords;
datapointer	*datapntr;
{
#ifdef LINUX
   blockheader	tmpblockhead;
   int count;
   short *ptr;

   DATABLOCKHEADER_SET_HTON( &tmpblockhead, &(datapntr->blockhead));
   
   if ( write(fd, (char *) &tmpblockhead, sizeof(blockheader))
		!= sizeof(blockheader) )
   {
      return(WRITE_ERR);
   }
#else
   if ( write(fd, (char *) (&(datapntr->blockhead)), sizeof(blockheader))
		!= sizeof(blockheader) )
   {
      return(WRITE_ERR);
   }
#endif

   if ( !(datapntr->blockhead.status & S_DATA) )
      return(COMPLETE);

#ifdef LINUX
   ptr = (short *) datapntr->data;
   for (count=0; count < nfidwords; count++)
   {
       *ptr = htons( *ptr );
       ptr++;
   }
#endif
   if ( write(fd, datapntr->data, nfidwords*sizeof(short)) !=
		nfidwords*sizeof(short) )
   {
      return(WRITE_ERR);
   }
		
   return(COMPLETE);
}

/*---------------------------------------
|                                       |
|               main()/2                |
|                                       |
+--------------------------------------*/
int main(int argc, char *argv[])
{
   int		i,
		r,
		done = FALSE,
		nfidwords;
   void		closeFIDfiles(),
		modifydata();
   fdInfo	*fdinfo,
		*openFIDfiles();
   comInfo	*pinfo,
		*parseinput();
   fileheader	*inputhead,
		*getheader(),
		outputhead,
		modifyheader();
   datapointer	*inpdatapntr,
		*outdatapntr;
   char		filepath[MAXPATH];

#ifdef DBXTOOL
   sleep(30);
#endif


   if ( (pinfo = parseinput(argc, argv)) == NULL )
      return ERROR;

   if ( (fdinfo = openFIDfiles(pinfo)) == NULL )
      return ERROR;

   if ( (inputhead = getheader(fdinfo->inp)) == NULL )
   {
      closeFIDfiles(fdinfo);
      return ERROR;
   }


   (void) strcpy(filepath, pinfo->outputdirpath.sval);
   (void) strcat(filepath, "/fid");

   if ( (fdinfo->out = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, 0666)) < 0 )
   {
      printf("\ncompressfid:  cannot open output file `%s`\n\n", filepath);
      (void) close(fdinfo->inp);
      return ERROR;
   }

   outputhead = modifyheader(inputhead); 

   if ( writeheader(fdinfo->out, &outputhead) )
   {
      closeFIDfiles(fdinfo);
      return ERROR;
   }


/*****************************************
*  Allocate memory for input and output  *
*  data structures.                      *
*****************************************/

   if ( (inpdatapntr = (datapointer *) malloc( (unsigned)
		sizeof(datapointer) )) == NULL )
   {
      printf("\ncompressfid:  insufficient memory in main\n\n");
      closeFIDfiles(fdinfo);
      return ERROR;
   }
   else
   {
      if ( (inpdatapntr->data = malloc( (unsigned) (inputhead->tbytes *
		inputhead->ntraces) )) == NULL )
      {
         printf("\ncompressfid main:  insufficient memory\n\n");
         closeFIDfiles(fdinfo);
         return ERROR;
      }

   }

   if ( (outdatapntr = (datapointer *) malloc( (unsigned)
		sizeof(datapointer) )) == NULL )
   {
      printf("\ncompressfid:  insufficient memory\n\n");
      closeFIDfiles(fdinfo);
      return ERROR;
   }
   else
   {
      if ( (outdatapntr->data = malloc( (unsigned) (outputhead.tbytes *
		outputhead.ntraces) )) == NULL )
      {
         printf("\ncompressfid:  insufficient memory\n\n");
         closeFIDfiles(fdinfo);
         return ERROR;
      }
   }


/************************************
*  Begin modification of the input  *
*  FID data.                        *
************************************/

   nfidwords = inputhead->ntraces*inputhead->np;

   for (i = 0; i < inputhead->nblocks; i++)
   {
      if ( (r = readdata(fdinfo->inp, inpdatapntr, nfidwords)) )
      {
         switch (r)
         {
            case READ_ERR:  printf("\ncompressfid:  read error\n\n");
			    closeFIDfiles(fdinfo);
			    return ERROR;
            case LAST_FID:  i = inputhead->nblocks;
			    done = TRUE;
			    break;
            default:	    printf("\ncompressfid:  internal error\n\n");
			    closeFIDfiles(fdinfo);
			    return ERROR;
         }
      }

      if (!done)
      {
         modifydata(inpdatapntr, outdatapntr, nfidwords);
         if ( (r = writedata(fdinfo->out, outdatapntr, nfidwords)) )
         {
            switch (r)
            {
               case WRITE_ERR:  printf("\ncompressfid:  write error\n\n");
			        closeFIDfiles(fdinfo);
			        return ERROR;
               default:	        printf("\ncompressfid:  internal error\n\n");
			        closeFIDfiles(fdinfo);
			        return ERROR;
            }
         }
      }
   }
   closeFIDfiles(fdinfo);
   printf("\nFID compression done!\n\n");
   return COMPLETE;
}
