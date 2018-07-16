/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/statvfs.h>

#include "process.h"
#include "struct3d.h"
#include "constant.h"

#define MAXPATHL	128
#define MAXSTR		256

#define PLEXTRACT
#include "command.h"
#include "data.h"
#ifdef LINUX
#include "datac.h"
#endif
#undef PLEXTRACT

#include "fileio.h"


#define ERROR		-1
#define COMPLETE	0
#define FALSE		0
#define TRUE		1

#define F1F3		0
#define F2F3		1
#define F1F2		2
#define MAX_PLTYPES	3

struct _planeInfo
{
   int	npltypes;
   int	f1f2;
   int	f1f3;
   int	f2f3;
};

typedef struct _planeInfo planeInfo;

struct _filepar
{
   int	nptspertrace;
   int	nF1traces;
   int	nF2traces;
   int	nF3points;
   int	ndatafiles;
   int	datatype;
   int	*fdlist;
};

typedef struct _filepar filepar;

struct _info3D
{
   datafileheader	*filehead;
   filedesc		*datalist;
   filepar		*fileinfo;
};

typedef struct _info3D info3D;

struct _hdrInfo
{
   dfilehead	*filehead;
   dblockhead	*blockhead;
};

typedef struct _hdrInfo hdrInfo;

datafileheader *readDATAheader(int fd);

/*---------------------------------------
|                                       |
|             parseplane()/2            |
|                                       |
+--------------------------------------*/
void parseplane(planeInfo *plinfo, char *plane_argv)
{
   int	nplanetypes,
	f1,
	f2,
	f3,
	dirflag,
	lblflag,
	notdone;


   nplanetypes = 0;
   f1 = f2 = f3 = lblflag = FALSE;
   dirflag = notdone = TRUE;

   plinfo->npltypes = 0;
   plinfo->f1f2 = FALSE;
   plinfo->f1f3 = FALSE;
   plinfo->f2f3 = FALSE;


   while (notdone)
   {
      switch (*plane_argv)
      {
         case 'f':
         case 'F':
         {
            if (lblflag)
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }

            lblflag = TRUE;
            dirflag = FALSE;
            break;
         }

         case '1':
         {
            if (dirflag || f1)
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }

            lblflag = FALSE;
            dirflag = TRUE;
            f1 = TRUE;
            break;
         }

         case '2':
         {
            if (dirflag || f2)
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }

            lblflag = FALSE;
            dirflag = TRUE;
            f2 = TRUE;
            break;
         }

         case '3':
         {
            if (dirflag || f3)
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }

            lblflag = FALSE;
            dirflag = TRUE;
            f3 = TRUE;
            break;
         }

         case ':':
         case '\0':
         {
            if ( lblflag || (!dirflag) )
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }
            else if (f1 && f2)
            {
               if (plinfo->f1f2)
               {
                  (void) printf("\nparseplane():  format error\n");
                  return;
               }

               plinfo->f1f2 = TRUE;
            }
            else if (f1 && f3)
            {
               if (plinfo->f1f3)
               {
                  (void) printf("\nparseplane():  format error\n");
                  return;
               }    

               plinfo->f1f3 = TRUE;
            }
            else if (f2 && f3) 
            { 
               if (plinfo->f2f3) 
               { 
                  (void) printf("\nparseplane():  format error\n"); 
                  return; 
               }     

               plinfo->f2f3 = TRUE;
            }
            else
            {
               (void) printf("\nparseplane():  format error\n");
               return;
            }

            nplanetypes += 1;
            f1 = f2 = f3 = FALSE;
            if ( (*plane_argv) == '\0' )
               notdone = FALSE;
            break;
         }

         default:
         {
            (void) printf("\nparseplane():  format error\n");
            return;
         }
      }

      plane_argv += 1;
   }


   plinfo->npltypes = nplanetypes;
   if (plinfo->npltypes > MAX_PLTYPES)
   {
      plinfo->npltypes = 0;
      (void) printf("\nparseplane():  format error\n");
      return;
   }
}

/*---------------------------------------
|                                       |
|            readselect()/2             |
|                                       |
+--------------------------------------*/
ipar readselect(char *argstr, comInfo *pinfo)
{
   ipar	tmp;


   tmp.vset = ERROR;
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
         case 'i':   tmp.pmode = pinfo->indirpath.pmode;
		     tmp.ival = INP_DATA_DIR;
		     return(tmp);
         case 'o':   tmp.pmode = pinfo->outdirpath.pmode;
		     tmp.ival = OUT_DATA_DIR;
		     return(tmp);
         case 'p':   tmp.pmode = pinfo->planeselect.pmode;
		     tmp.ival = PLANE_SELECT;
		     return(tmp);
         default:    (void) printf("\nreadselect():  invalid option\n");
		     tmp.pmode = EITHER;
		     tmp.ival = ERROR;
                     return(tmp);
      }
   }          
   else
   {
      (void) printf("\nreadselect():  improper input format\n");
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
static int setselect(char *argstr, int selection, comInfo *pinfo)
{      
   switch (selection)
   {
      case EXP_NUM:	   pinfo->curexp.ival = atoi(argstr);
                           if ( (pinfo->curexp.ival < FIRST_EXP) ||
                                (pinfo->curexp.ival > LAST_EXP) )
                           {
                              printf("\nsetselect():  invalid experiment");
                              return(ERROR);
                           }
                           pinfo->curexp.vset = TRUE;
                           break;
      case INP_DATA_DIR:   (void) strcpy(pinfo->indirpath.sval, argstr);
			   pinfo->indirpath.vset = TRUE;
                           break;
      case OUT_DATA_DIR:   (void) strcpy(pinfo->outdirpath.sval, argstr);
			   pinfo->outdirpath.vset = TRUE;
                           break;
      case PLANE_SELECT:   (void) strcpy(pinfo->planeselect.sval, argstr);
			   pinfo->planeselect.vset = TRUE;
			   break;
      default:             break;
   }
 
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            initPLinfo()/1             |
|                                       |
+--------------------------------------*/
void initPLinfo(comInfo *pinfo)
{
   pinfo->overwrite.pmode = SELECT;
   pinfo->curexp.pmode = SET;
   pinfo->indirpath.pmode = SET;
   pinfo->outdirpath.pmode = SET;
   pinfo->planeselect.pmode = SET;

   pinfo->overwrite.vset = FALSE;
   pinfo->curexp.vset = FALSE;
   pinfo->indirpath.vset = FALSE;
   pinfo->outdirpath.vset = FALSE;
   pinfo->planeselect.vset = FALSE;

   pinfo->overwrite.ival = FALSE;
   pinfo->curexp.ival = FALSE;
   (void) strcpy(pinfo->indirpath.sval, "");
   (void) strcpy(pinfo->outdirpath.sval, "");
   (void) strcpy(pinfo->planeselect.sval, "");
}


/*---------------------------------------
|                                       |
|            parseinput()/3             |
|                                       |
+--------------------------------------*/
comInfo *parseinput(int argc, char *argv[], planeInfo *plinfo)
{
   char		*vnmruser;
   int          argno = 1,
                mode = SELECT;
   ipar		selection;
   comInfo      *pinfo;
 
 
/*****************************************
*  Allocate memory for the command line  *
*  information structure.                *
*****************************************/

   if ( (pinfo = (comInfo *) malloc( (unsigned) sizeof(comInfo) ))
                == NULL )
   {
      (void) printf(
		"\nparseinput():  cannot allocate memory for command data\n");
      return(NULL);
   }


/******************************************
*  Initialize the pointer to the command  *
*  information structure.                 *
******************************************/
 
   initPLinfo(pinfo);
 

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
      (void) printf("\nparseinput():  improper input format\n");
      return(NULL);
   }


/********************************************
*  Check that input 3D data directory has   *
*  been set and that the directory exists.  *
********************************************/

   if (pinfo->curexp.vset)
   {
      char	tmppath[MAXPATHL],
		fext[25];

/**********************************************
*  Get environment variable if the 2D planes  *
*  are to be extracted into an experiment.    *
**********************************************/

      if ( (vnmruser = getenv("vnmruser")) == NULL )
      {
         (void) printf(
	   "\nparseinput():  cannot get `vnmruser` environmental variable\n");
         return(NULL);
      }

      (void) strcpy(tmppath, vnmruser);
      (void) strcat(tmppath, "/exp");
      (void) sprintf(fext, "%d/datadir3d", pinfo->curexp.ival);
      (void) strcat(tmppath, fext);

      if (!pinfo->indirpath.vset)
         (void) strcpy(pinfo->indirpath.sval, tmppath);

      if (!pinfo->outdirpath.vset)
         (void) strcpy(pinfo->outdirpath.sval, tmppath);

      (void) strcat(pinfo->indirpath.sval, "/data");

      if ( mkdir(pinfo->outdirpath.sval, 0777) )
      {
         if (errno != EEXIST)
         {
            (void) printf("\nparseinput():  cannot create directory `%s`\n",
			      pinfo->outdirpath.sval);
            return(NULL);
         }
      }

      (void) strcat(pinfo->outdirpath.sval, "/extr");
   }
   else
   {
      if (!pinfo->indirpath.vset)
      {
         (void) printf("\nparseinput():  input 3D data directory required\n");
         return(NULL);
      }

      if (!pinfo->outdirpath.vset)
         (void) strcpy(pinfo->outdirpath.sval, pinfo->indirpath.sval);

      (void) strcat(pinfo->indirpath.sval, "/data");

      if ( mkdir(pinfo->outdirpath.sval, 0777) )
      {
         if (errno != EEXIST)
         {
            (void) printf("\nparseinput():  cannot create directory `%s`\n",
			      pinfo->outdirpath.sval);
            return(NULL);
         }
      }

      (void) strcat(pinfo->outdirpath.sval, "/extr");
   }

   if ( access(pinfo->indirpath.sval, F_OK) )
   {
      (void) printf(
	  "\nparseinput():  cannot access input 3D data directory\n");
      return(NULL);
   }


/************************************************
*  Check that output extraction data directory  *
*  has been set and that the directory can be   *
*  properly created.                            *
************************************************/

   if ( mkdir(pinfo->outdirpath.sval, 0777) )
   {
      if (errno == EEXIST)
      {
         if (pinfo->overwrite.ival)
         {
            char syscmd[MAXPATHL];

            sprintf(syscmd, "rm -r \"%s\"\n", pinfo->outdirpath.sval);
            (void) system(syscmd);

            if ( mkdir(pinfo->outdirpath.sval, 0777) )
            {
               (void) printf("\nparseinput():  cannot create directory `%s`\n",
			      pinfo->outdirpath.sval);
               return(NULL);
            }
         }
      }
      else
      {
         (void) printf("\nparseinput():  cannot create directory `%s`\n",
                        pinfo->outdirpath.sval);
         return(NULL);
      }
   }

 
/***********************************************
*  If "planeselect" has been specified, parse  *
*  the input string to decide how many and     *
*  which planes should be extracted.  If it    *
*  has not been specfied, take the internal    *
*  default specification.                      *
***********************************************/

   if (!pinfo->planeselect.vset)
   {
      plinfo->npltypes = 2;
      plinfo->f1f2 = FALSE;
      plinfo->f1f3 = TRUE;
      plinfo->f2f3 = TRUE;
   }
   else
   {
      parseplane(plinfo, pinfo->planeselect.sval);
      if (!plinfo->npltypes)
      {
         (void) printf("\nparseinput():  invalid plane selection was made\n");
         return(NULL);
      }
   }

   return(pinfo);
}


/*---------------------------------------
|                                       |
|         createDATAheader()/3          |
|                                       |
+--------------------------------------*/
hdrInfo *createDATAheader(filepar *fileinfo, datafileheader *datahead, int pltype)
{
   int		nbheaders;
   hdrInfo	*hdrinfo;


   switch (fileinfo->datatype)
   {
      case REAL:
      case COMPLEX:       nbheaders = 1;
                          break;
      case HYPERCOMPLEX:  nbheaders = 2;
                          break;
      default:            (void) printf("\ncreateDATAheader():  internal error\n");
                          return(NULL);
   }

   if ( (hdrinfo = (hdrInfo *) malloc( (unsigned) sizeof(hdrInfo) ))
	    == NULL )
   {
      (void) printf("\ncreateDATAheader():  insufficient memory\n");
      return(NULL);
   }
   else if ( (hdrinfo->filehead = (dfilehead *) malloc( (unsigned)
	    (sizeof(dfilehead) + nbheaders*sizeof(dblockhead)) ))
	    == NULL )
   {
      (void) printf("\ncreateDATAheader():  insufficient memory\n"); 
      return(NULL); 
   }

   hdrinfo->blockhead = (dblockhead *) (hdrinfo->filehead + 1);

   hdrinfo->filehead->nblocks = 1;
   hdrinfo->filehead->np      = ( (pltype == F2F3) ? fileinfo->nF2traces :
					fileinfo->nF1traces )
					* fileinfo->datatype;

   hdrinfo->filehead->ntraces = ( (pltype == F1F2) ? fileinfo->nF2traces :
					(fileinfo->nF3points / 
						fileinfo->datatype) );
   hdrinfo->filehead->ebytes  = 4;
   hdrinfo->filehead->tbytes  = (hdrinfo->filehead->ebytes *
					hdrinfo->filehead->np);
   hdrinfo->filehead->bbytes  = (hdrinfo->filehead->tbytes *
					hdrinfo->filehead->ntraces) +
					nbheaders*sizeof(dblockhead);

   hdrinfo->filehead->vers_id = (VERSION + FT3D_VERSION) | DATA_FILE;
   hdrinfo->filehead->nbheaders = nbheaders;

   hdrinfo->filehead->status = (S_DATA|S_SPEC|S_FLOAT|S_SECND|S_TRANSF|S_3D);
   if (fileinfo->datatype > REAL)
   {
      hdrinfo->filehead->status |= S_COMPLEX;
      if (fileinfo->datatype > COMPLEX)
         hdrinfo->filehead->status |= S_HYPERCOMPLEX;
   }

   switch (pltype)
   {
      case F1F2:  hdrinfo->filehead->status |= (S_NI|S_NI2);
		  hdrinfo->blockhead->mode = datahead->mode &
						(NI_DSPLY|NI2_DSPLY);
                  break;
      case F1F3:  hdrinfo->filehead->status |= (S_NP|S_NI);
                  hdrinfo->blockhead->mode = datahead->mode,
						(NI_DSPLY|NP_DSPLY);
                  break;
      case F2F3:  hdrinfo->filehead->status |= (S_NP|S_NI2);
                  hdrinfo->blockhead->mode = datahead->mode,
						(NI2_DSPLY|NP_DSPLY);
                  break;
      default:    (void) printf("\ncreateDATAheader():  internal error\n");
                  return(NULL);
   }

   hdrinfo->blockhead->lvl     = 0.0;
   hdrinfo->blockhead->tlt     = 0.0;
   hdrinfo->blockhead->rpval   = datahead->phaseinfo.rp1;
   hdrinfo->blockhead->lpval   = datahead->phaseinfo.lp1;
   hdrinfo->blockhead->ctcount = 0;
   hdrinfo->blockhead->scale   = 0;
   hdrinfo->blockhead->index   = 0;
   hdrinfo->blockhead->status |= (S_DATA|S_SPEC|S_FLOAT);
   if (fileinfo->datatype > REAL)
   {
      hdrinfo->blockhead->status |= (S_COMPLEX|NI_CMPLX);
      if (fileinfo->datatype > COMPLEX)
         hdrinfo->blockhead->status |= (S_HYPERCOMPLEX|MORE_BLOCKS|NI2_CMPLX);
   }

   if (hdrinfo->blockhead->status & MORE_BLOCKS)
   {
      hycmplxhead	*tmpblockhead;

      tmpblockhead = (hycmplxhead *) (hdrinfo->blockhead + 1);
      tmpblockhead->status   = U_HYPERCOMPLEX;
      tmpblockhead->s_spare1 = 0;
      tmpblockhead->s_spare2 = 0;
      tmpblockhead->s_spare3 = 0;
      tmpblockhead->l_spare1 = 0;
      tmpblockhead->rpval1   = datahead->phaseinfo.rp2;
      tmpblockhead->lpval1   = datahead->phaseinfo.lp2;
      tmpblockhead->f_spare1 = 0.0;
      tmpblockhead->f_spare2 = 0.0;
   }

   return(hdrinfo);
}


/*---------------------------------------
|					|
|	      initDLinfo()/1		|
|					|
+--------------------------------------*/
void initDLinfo(filedesc *dlist)
{
   dlist->ndatafd    = 0;
   dlist->dataexists = FALSE;
   dlist->result     = 0;
   dlist->dlklist    = NULL;
   dlist->dfdlist    = NULL;
}


/*---------------------------------------
|					|
|	      initFHinfo()/1		|
|					|
+--------------------------------------*/
void initFHinfo(datafileheader *dhead)
{
   dhead->Vfilehead.nblocks   = 0;
   dhead->Vfilehead.ntraces   = 0;
   dhead->Vfilehead.np        = 0;
   dhead->Vfilehead.ebytes    = 0;
   dhead->Vfilehead.tbytes    = 0;
   dhead->Vfilehead.bbytes    = 0;
   dhead->Vfilehead.vers_id   = 0;
   dhead->Vfilehead.status    = 0;
   dhead->Vfilehead.nbheaders = 0;

   dhead->f3blockinfo.hcptspertrace = 0;
   dhead->f3blockinfo.bytesperfid   = 0;
   dhead->f3blockinfo.dpflag        = 0;

   dhead->coefvals   = NULL;
   dhead->maxval     = 0;
   dhead->minval     = 0;
   dhead->mode	     = 0;
   dhead->version3d  = 0;
   dhead->ncoefbytes = 0;
   dhead->lastfid    = 0;
   dhead->ndatafiles = 0;
   dhead->nheadbytes = 0;
}  


/*---------------------------------------
|					|
|	      initFPinfo()/1		|
|					|
+--------------------------------------*/
void initFPinfo(info3D *info)
{
   info->fileinfo->nF3points    = info->filehead->Vfilehead.np;
   info->fileinfo->ndatafiles   = info->filehead->ndatafiles;
   info->fileinfo->nptspertrace = info->fileinfo->nF3points /
					info->fileinfo->ndatafiles;
   info->fileinfo->nF1traces    = info->filehead->Vfilehead.ntraces;
   info->fileinfo->nF2traces    = info->filehead->Vfilehead.nblocks;
   info->fileinfo->fdlist       = info->datalist->dfdlist;

   info->fileinfo->datatype     = info->fileinfo->nF3points /
			(info->filehead->f3blockinfo.hcptspertrace *
			 info->fileinfo->ndatafiles);
}


/*---------------------------------------
|					|
|	    closeDATAfiles()/1		|
|					|
+--------------------------------------*/
void closeDATAfiles(filedesc *dlist)
{
   int	i;


   for (i = 0; i < dlist->ndatafd; i++)
   {
      if (*(dlist->dfdlist) != FILE_CLOSED)
      {
         (void) close( *(dlist->dfdlist) );
         *(dlist->dfdlist) = FILE_CLOSED;
      }

      dlist->dfdlist += 1;
   }
}


/*---------------------------------------
|					|
|	     openDATAfiles()/1		|
|					|
+--------------------------------------*/
info3D *openDATAfiles(char *indirpath)
{
   char			basedatapath[MAXPATHL],
			datapath[MAXPATHL],
			fext[10];
   int			i,
			fd,
			*fdlist;
   info3D		*info;


   (void) strcpy(basedatapath, indirpath);
   (void) strcat(basedatapath, "/data");

   if ( (info = (info3D *) malloc( (unsigned) sizeof(info3D) ))
		== NULL )
   {
      (void) printf("\nopenDATAfiles():  insufficient memory\n");
      return(NULL);
   }
   else if ( (info->datalist = (filedesc *) malloc( (unsigned)
		sizeof(filedesc) )) == NULL )
   {
      (void) printf("\nopenDATAfiles():  insufficient memory\n");
      return(NULL);
   }
   else if ( (info->filehead = (datafileheader *) malloc( (unsigned)
		sizeof(datafileheader) )) == NULL )
   {   
      (void) printf("\nopenDATAfiles():  insufficient memory\n");
      return(NULL);
   }
   else if ( (info->fileinfo = (filepar *) malloc( (unsigned)
		sizeof(filepar) )) == NULL )
   {
      (void) printf("\nopenDATAfiles():  insufficient memory\n");
      return(NULL);
   }

   initDLinfo(info->datalist);
   initFHinfo(info->filehead);

   (void) strcpy(datapath, basedatapath);
   (void) strcat(datapath, "1");

   if ( (fd = open(datapath, O_RDONLY, 0444)) < 0 )
   {
      (void) printf("\nopenDATAfiles():  cannot open file '%s'\n", datapath);
      return(NULL);
   }
   else if ( (info->filehead = readDATAheader(fd)) == NULL )
   {
      (void) close(fd);
      return(NULL);
   }

   if ( (~info->filehead->Vfilehead.status) & S_3D )
   {
      (void) printf("\n`%s` is not a fully 3D transformed data set\n",
		datapath);
      (void) close(fd);
      return(NULL);
   }

   info->datalist->ndatafd = info->filehead->ndatafiles;

   if ( (info->datalist->dfdlist = (int *) malloc( (unsigned)
		(info->datalist->ndatafd * sizeof(int)) ))
		== NULL )
   {
      (void) printf("\nopenDATAfiles():  insufficient memory\n");
      (void) close(fd);
      return(NULL);
   }
   else
   {
      initFPinfo(info);
   }

   *(info->datalist->dfdlist) = fd;
   fdlist = info->datalist->dfdlist + 1;
   for (i = 1; i < info->datalist->ndatafd; i++)
      *fdlist++ = FILE_CLOSED;

   fdlist = info->datalist->dfdlist + 1;

   for (i = 1; i < info->datalist->ndatafd; i++)
   {
      (void) sprintf(fext, "%1d", i+1);
      (void) strcpy(datapath, basedatapath);
      (void) strcat(datapath, fext);

      if ( (*fdlist = open(datapath, O_RDONLY, 0444)) < 0 )
      {
         (void) printf("\nopenDATAfiles():  cannot open file '%s'\n", datapath);
         closeDATAfiles(info->datalist);
         return(NULL);
      }

      fdlist += 1;
   }

   return(info);
}


/*---------------------------------------
|                                       |
|          readDATAheader()/1           |
|                                       |
+--------------------------------------*/
datafileheader *readDATAheader(int fd)
{
   int                  nbytes;
   datafileheader       *datahead;


   if ( (datahead = (datafileheader *) malloc( (unsigned)
                (sizeof(datafileheader)) ))
                == NULL )
   {
      (void) printf("\nreadDATAheader():  insufficient memory\n");
      return(NULL);
   }
 
   nbytes = sizeof(dfilehead) + sizeof(f3blockpar) + sizeof(phasepar);
   if ( read(fd, (char *)datahead, nbytes) != nbytes )
   {
      (void) printf("\nreadDATAheader():  read error 1\n");
      return(NULL);
   }
 
   nbytes = sizeof(datafileheader) - nbytes - sizeof(float);
   if ( read(fd, (char *) (&(datahead->maxval)), nbytes)
                != nbytes )
   {
      (void) printf("\nreadDATAheader():  read error 2\n");
      return(NULL);
   }
 
   return(datahead);
}


/*---------------------------------------
|					|
|	       copypar()/2		|
|					|
+--------------------------------------*/
int copypar(char *indirpath, char *outdirpath)
{
   char	infilepath[MAXPATHL],
	outfilepath[MAXPATHL],
	syscmd[MAXSTR];


   (void) strcpy(infilepath, indirpath);
   (void) strcat(infilepath, "/info/procpar3d");

   if ( access(infilepath, R_OK|F_OK) )
   {
      (void) printf("\ncopypar():  cannot find file `%s`\n", infilepath);
      return(ERROR);
   }

   (void) strcpy(outfilepath, outdirpath);
   (void) strcat(outfilepath, "/procpar3d");

   (void) sprintf(syscmd, "cp \"%s\" \"%s\"\n", infilepath, outfilepath);
   (void) system(syscmd);

   if ( access(outfilepath, R_OK) )
   { 
      (void) printf("\ncopypar():  copy failed; cannot find file `%s`\n",
		outfilepath); 
      return(ERROR); 
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	    readFiF3plane()/7		|
|					|
+--------------------------------------*/
int readFiF3plane(filepar *finfo, float *data, float *wspace, int Fjtrace,
                  int F3file, int nfheadbytes, int plane)
{
   int			ntrbytes,
			nFitraces,
			fd;
   off_t		Fi_startbytes,
			f3skip,
			Fi_skipbytes;
   register int		i,
			j;
   register float	*tmpdata,
			*tmpwspace;


/*************************************
*  Initialize I/O and transposition  *
*  parameters for reading the Fi-F3  *
*  2D data planes.                   *
*************************************/

   fd = *(finfo->fdlist + F3file);
   ntrbytes = finfo->nptspertrace * sizeof(float);
   f3skip = (off_t) finfo->nptspertrace * (off_t) (finfo->ndatafiles - 1);

   if (plane == F1F3)
   {
      nFitraces = finfo->nF1traces;
      Fi_startbytes = (off_t) Fjtrace * (off_t) finfo->nF1traces * (off_t) ntrbytes;
      Fi_startbytes += nfheadbytes;
      Fi_skipbytes = 0;
   }
   else
   { /* plane is F2F3 or F1F2 */
      nFitraces = finfo->nF2traces;
      Fi_startbytes = (off_t) Fjtrace * (off_t) ntrbytes;
      Fi_startbytes += nfheadbytes;
      Fi_skipbytes = (finfo->nF1traces - 1);
      Fi_skipbytes *= (off_t) ntrbytes;
   }

   if ( lseek(fd, Fi_startbytes, SEEK_SET) < 0L )
   {
      (void) printf("\nreadFiF3plane():  seek error occurred on data file\n");
      return(ERROR);
   }

/****************************************
*  Read each Fi trace for the specific  *
*  F3 subsection.  Transpose the data   *
*  after each Fi trace is read.         *
****************************************/

   tmpdata = data + (finfo->nptspertrace * F3file);
   tmpwspace = wspace;


   for (i = 0; i < nFitraces; i++)
   {
      if ( read(fd, (char *)wspace, (size_t)ntrbytes) != ntrbytes )
      {
         (void) printf(
		  "\nreadFiF3plane():  read error occurred on data file\n");
         return(ERROR);
      }

      for (j = 0; j < finfo->nptspertrace; j++)
         *tmpdata++ = *tmpwspace++;

      if ( (plane == F2F3) || (plane == F1F2) )
      {
         if ( lseek(fd, Fi_skipbytes, SEEK_CUR) < 0L )
         {
            (void) printf(
		     "\nreadFiF3plane():  seek error occurred on data file\n");
            return(ERROR);
         }
      }

      tmpdata += f3skip;
      tmpwspace = wspace;
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     skylineproj()/5		|
|					|
+--------------------------------------*/
void skylineproj(float *destpntr, float *srcpntr, int npnts,
                 int plane_no, int datatype)
{
   register int		i,
			j;
   register float	*spntr,
			*dpntr,
			tmpnew,
			tmpold;


   dpntr = destpntr;
   spntr = srcpntr;

   if (plane_no == 0)
   {
      for (i = 0; i < npnts; i++)
         *dpntr++ = *spntr++;

      return;
   }

   for (i = 0; i < (npnts/datatype); i++)
   {
      tmpold = tmpnew = 0.0;

      for (j = 0; j < datatype; j++)
      {
         tmpold += (*dpntr) * (*dpntr);
         tmpnew += (*spntr) * (*spntr);
         spntr += 1;
         dpntr += 1;
      }

      if (tmpnew > tmpold)
      {
         for (j = 0; j < datatype; j++)
            *(--dpntr) = *(--spntr);

         dpntr += datatype;
         spntr += datatype;
      }
   }
}


/*---------------------------------------
|					|
|	     getF2trace()/6		|
|					|
+--------------------------------------*/
void getF2trace(float *data, float *wspace, int F2traceno, int nF3pts,
                int nF2traces, int datatype)
{
   register int		i,
			j,
			f2skip;
   register float	*tmpdata,
			*tmpwspace;


   tmpwspace = wspace;
   tmpdata = data + F2traceno*datatype;
   f2skip = nF3pts - datatype;

   for (i = 0; i < nF2traces; i++)
   {
      for (j = 0; j < datatype; j++)
         *tmpwspace++ = *tmpdata++;

      tmpdata += f2skip;
   }
}

#ifdef LINUX
static void convertData(int *buffer2, int *buffer, int np)
{
   int index;

   for (index=0; index < np; index++)
   {
      *buffer2++ = htonl( *buffer );
      buffer++;
   }
}

static void convertProj(int *tmpprojbuffer, int np)
{
   int index;

   for (index=0; index < np; index++)
   {
      *tmpprojbuffer = htonl( *tmpprojbuffer );
      tmpprojbuffer++;
   }
}
#endif

/*---------------------------------------
|					|
|	     writeplanes()/4		|
|					|
+--------------------------------------*/
int writeplanes(info3D *info, char *outdirpath, int plane, int f1f2flag)
{
   char		basefilepath[MAXPATHL],
		basefilepathF1F2[MAXPATHL],
		filepath[MAXPATHL],
		fext[10];
   int		i,
		j,
		fdw,
		nFj_traces,
		nbytes,
		nF1F2files = 0,
		wrF1F2bytes = 0,
		wrF1F2words = 0,
		nbheaderbytes,
		nheaderbytes,
		worksize;
   off_t        fOffset;
   float	*buffer,
		*workspace,
		*projbuffer = NULL,
		*projf1f2buffer = NULL;
   hdrInfo	*hdrinfo,
		*hdrinfof1f2 = NULL;
   dfilehead    *pFilehead;
   dblockhead   *pBlockhead;
   char         *pBuffer;
#ifdef LINUX
   int          *buffer2;
   dfilehead    tmp;
   dblockhead   tmp2;
#endif


/*************************************
*  Create the template for the VNMR  *
*  data file and block headers.      *
*************************************/

   if ( (hdrinfo = createDATAheader( info->fileinfo, info->filehead,
	     ((plane == F1F2) ? F1F3 : plane) )) == NULL )
   {
      closeDATAfiles(info->datalist);
      return(ERROR);
   }

   if ( ((plane == F2F3) && f1f2flag) || (plane == F1F2) )
   {
      if ( (hdrinfof1f2 = createDATAheader( info->fileinfo, info->filehead,
		F1F2)) == NULL )
      {
         closeDATAfiles(info->datalist);
         return(ERROR);
      }
   }

/************************************
*  Initialize filename and certain  *
*  file parameters.                 *
************************************/

   (void) strcpy(basefilepath, outdirpath);
   (void) strcpy(basefilepathF1F2, outdirpath);

   if (plane == F1F3)
   {
      nFj_traces = info->fileinfo->nF2traces;
      (void) strcat(basefilepath, "/dataf1f3.");
      worksize = info->fileinfo->nptspertrace;
   }
   else
   { /* plane F2F3 or F1F2 */
      nFj_traces = info->fileinfo->nF1traces;
      (void) strcat(basefilepath, "/dataf2f3.");
      (void) strcat(basefilepathF1F2, "/dataf1f2.");

      nF1F2files = info->fileinfo->nF3points / info->fileinfo->datatype;
      wrF1F2words = info->fileinfo->nF2traces * info->fileinfo->datatype;
      wrF1F2bytes = wrF1F2words * sizeof(float);

      worksize = ( (plane == F1F2) ? wrF1F2words
			: info->fileinfo->nptspertrace );
      if (f1f2flag)
      {
         worksize = ( (worksize < wrF1F2words) ? wrF1F2words
			: info->fileinfo->nptspertrace );
      }
   }

/***************************************
*  Allocate memory for the data block  *
*  and the scratch read buffer.        *
***************************************/

   nbheaderbytes = hdrinfo->filehead->nbheaders * sizeof(dblockhead);
   nheaderbytes = nbheaderbytes + sizeof(dfilehead);
   nbytes = hdrinfo->filehead->bbytes - nbheaderbytes +
		(worksize * sizeof(float));

   if (plane == F1F2)
   {
      nbytes += hdrinfof1f2->filehead->bbytes - nbheaderbytes;
   }
   else
   {
      nbytes += hdrinfo->filehead->bbytes - nbheaderbytes;
      if (f1f2flag)
         nbytes += hdrinfof1f2->filehead->bbytes - nbheaderbytes;
   }

   if ( (buffer = (float *) malloc( (unsigned)nbytes )) == NULL )
   {
      (void) printf("\nwriteplanes():  insufficient memory\n");
      closeDATAfiles(info->datalist);
      return(ERROR);
   }
#ifdef LINUX
   if ( (buffer2 = (int *) malloc( (unsigned)nbytes )) == NULL )
   {
      (void) printf("\nwriteplanes():  insufficient memory\n");
      closeDATAfiles(info->datalist);
      return(ERROR);
   }
#endif

   workspace = buffer + (hdrinfo->filehead->bbytes - nbheaderbytes) /
		  sizeof(float);

   if (plane == F1F2)
   {
      projf1f2buffer = workspace + worksize;
   }
   else
   {
      projbuffer = workspace + worksize;
      if (f1f2flag)
      {
         projf1f2buffer = (float *) ( (char *)projbuffer +
		   hdrinfo->filehead->bbytes - nbheaderbytes );
      }
   }

/****************************************
*  Start extracting planes and writing  *
*  them to separate files comensurate   *
*  with the VNMR format.                *
****************************************/

   for (i = 0; i < nFj_traces; i++)
   {
      for (j = 0; j < info->fileinfo->ndatafiles; j++)
      {
         if ( readFiF3plane(info->fileinfo, buffer, workspace,
			i, j, info->filehead->nheadbytes, plane) )
         {
            closeDATAfiles(info->datalist);
            return(ERROR);
         }
      }

      if ( (plane == F1F3) || (plane == F2F3) )
      {
         (void) strcpy(filepath, basefilepath);
         (void) sprintf(fext, "%d", i+1);
         (void) strcat(filepath, fext);

         if ( (fdw = open(filepath, (O_WRONLY|O_CREAT|O_TRUNC), 0666))
			< 0 )
         {
            (void) printf("\nwriteplanes():  error in opening file `%s`\n",
			filepath);
            closeDATAfiles(info->datalist);
            return(ERROR);
         }

#ifdef LINUX
         DFH_CONVERT3(tmp, hdrinfo->filehead);
         pFilehead = &tmp;
         DBH_CONVERT3(tmp2, hdrinfo->blockhead);
         pBlockhead = &tmp2;
         //convertData(buffer2, (int *)buffer, (hdrinfo->filehead->bbytes - nbheaderbytes) / hdrinfo->filehead->bbytes);
         convertData(buffer2, (int *)buffer,
                (hdrinfo->filehead->bbytes - nbheaderbytes) / sizeof(float) );
         pBuffer = (char *) buffer2;
#else
         pFilehead = hdrinfo->filehead;
         pBlockhead = hdrinfo->blockhead;
         pBuffer = (char *) buffer;
#endif
         if ( write(fdw, (char *)  pFilehead,
			(unsigned) (sizeof(dfilehead)))
			!= sizeof(dfilehead) )
         {
            (void) printf(
	       "\nwriteplanes():  error in writing file header to file `%s`\n",
		    filepath);
            closeDATAfiles(info->datalist); 
            (void) close(fdw);
            return(ERROR); 
         }
         if ( write(fdw, (char *) (pBlockhead),
			(unsigned)nbheaderbytes)
			!= nbheaderbytes )
         {
            (void) printf(
	       "\nwriteplanes():  error in writing block header to file `%s`\n",
                    filepath);
            closeDATAfiles(info->datalist); 
            (void) close(fdw);
            return(ERROR);   
         }


         if ( write(fdw, pBuffer, (unsigned) (hdrinfo->filehead->bbytes
			- nbheaderbytes)) != (hdrinfo->filehead->bbytes
			- nbheaderbytes) )
         { 
            (void) printf(
	       "\nwriteplanes():  error in writing data to file `%s`\n", 
                    filepath); 
            closeDATAfiles(info->datalist);  
            (void) close(fdw);
            return(ERROR);  
         }

         (void) close(fdw);

         skylineproj(projbuffer, buffer, (hdrinfo->filehead->bbytes -
		nbheaderbytes)/sizeof(float), i, info->fileinfo->datatype);
      }

      if ( ((plane == F2F3) && f1f2flag) || (plane == F1F2) )
      {
         for (j = 0; j < nF1F2files; j++)
         {
            (void) strcpy(filepath, basefilepathF1F2);
            (void) sprintf(fext, "%d", j+1);
            (void) strcat(filepath, fext);

            if (i == 0)
            { /* Create the file and write out the file and block headers */
               if ( (fdw = open(filepath, (O_WRONLY|O_CREAT|O_TRUNC), 0666))
			< 0 )
               {
                  (void) printf(
			   "\nwriteplanes():  error in creating file `%s`\n",
				filepath);
                  closeDATAfiles(info->datalist);
                  return(ERROR);
               }

#ifdef LINUX
               DFH_CONVERT3(tmp, hdrinfof1f2->filehead);
               pFilehead = &tmp;
               DBH_CONVERT3(tmp2, hdrinfof1f2->blockhead);
               pBlockhead = &tmp2;
#else
               pFilehead = hdrinfof1f2->filehead;
               pBlockhead = hdrinfof1f2->blockhead;
#endif
               if ( write(fdw, (char *) pFilehead,
			    (unsigned) (sizeof(dfilehead)))
			    != sizeof(dfilehead) )
               {
                  (void) printf(
			"\nerror in writing file header to file `%s`\n",
			      filepath);
                  closeDATAfiles(info->datalist);
                  (void) close(fdw);
                  return(ERROR);
               }

               if ( write(fdw, (char *) pBlockhead,
			    (unsigned)nbheaderbytes)
			    != nbheaderbytes )
               {
                  (void) printf(
			"\nerror in writing block header to file `%s`\n",
			      filepath);
                  closeDATAfiles(info->datalist);
                  (void) close(fdw);
                  return(ERROR);
               }
            }
            else
            { /* Open the file and seek to the proper place */
               if ( (fdw = open(filepath, O_WRONLY, 0666)) < 0 )
               {
                  (void) printf(
			   "\nwriteplanes():  error in opening file `%s`\n",
				filepath);
                  closeDATAfiles(info->datalist);
                  return(ERROR);
               }

               fOffset = (off_t) wrF1F2bytes * (off_t) i;
               fOffset += nheaderbytes;
               if ( lseek(fdw, fOffset, SEEK_SET) < 0L )
               {
                  (void) printf("\nwriteplanes():  seek error on file `%s`\n",
			   filepath);
                  closeDATAfiles(info->datalist);
                  (void) close(fdw);
                  return(ERROR);
               }
            }


            getF2trace(buffer, workspace, j, info->fileinfo->nF3points,
			info->fileinfo->nF2traces, info->fileinfo->datatype);
#ifdef LINUX
            convertData(buffer2, (int *)workspace, wrF1F2words);
            pBuffer = (char *) buffer2;
#else
            pBuffer = (char *) workspace;
#endif

            if ( write(fdw, pBuffer, (unsigned)wrF1F2bytes)
			 != wrF1F2bytes )
            {
               (void) printf(
			"\nwriteplanes():  error in writing to file `%s`\n",
				filepath);
               closeDATAfiles(info->datalist);
               (void) close(fdw);
               return(ERROR);
            }

            (void) close(fdw);

            skylineproj(projf1f2buffer + i*wrF1F2words, workspace,
			   wrF1F2words, j, info->fileinfo->datatype);
         }
      }
   }

/***********************************************
*  Write out 2D projection data if requested.  *
***********************************************/

   for (i = 0; i < 2; i++)
   {
      int	projflag;
      float	*tmpprojbuffer;
      hdrInfo	*tmphdrinfo;

      switch (i)
      {
         case 0:   tmphdrinfo = hdrinfo;
                   tmpprojbuffer = projbuffer;
		   projflag = ( (plane == F1F3) || (plane == F2F3) );
                   (void) strcpy(filepath, basefilepath);
                   break;
         case 1:   tmphdrinfo = hdrinfof1f2;
                   tmpprojbuffer = projf1f2buffer; 
		   projflag = ( f1f2flag || (plane == F1F2) );
                   (void) strcpy(filepath, basefilepathF1F2);
                   break;
         default:  break;
      }

      if (projflag)
      {
         tmphdrinfo->blockhead->index = 0;
         (void) strcat(filepath, "proj");

         if ( (fdw = open(filepath, (O_WRONLY|O_CREAT|O_TRUNC), 0666))
                           < 0 )
         {
            (void) printf("\nwriteplanes():  error in opening file `%s`\n",
                           filepath);
            closeDATAfiles(info->datalist);
            return(ERROR);
         }

#ifdef LINUX
         DFH_CONVERT3(tmp, tmphdrinfo->filehead);
         pFilehead = &tmp;
         DBH_CONVERT3(tmp2, tmphdrinfo->blockhead);
         pBlockhead = &tmp2;
         convertProj( (void *) tmpprojbuffer, (tmphdrinfo->filehead->bbytes - nbheaderbytes) / tmphdrinfo->filehead->ebytes );
#else
         pFilehead = tmphdrinfo->filehead;
         pBlockhead = tmphdrinfo->blockhead;
#endif
         if ( write(fdw, (char *) pFilehead,
                        (unsigned) (sizeof(dfilehead)))
                        != sizeof(dfilehead) )
         {
            (void) printf(
               "\nwriteplanes():  error in writing file header to file `%s`\n",
		      filepath);
            closeDATAfiles(info->datalist);
            (void) close(fdw);    
            return(ERROR);
         }

         if ( write(fdw, (char *) pBlockhead,
                        (unsigned)nbheaderbytes)
                        != nbheaderbytes )
         {
            (void) printf(
               "\nwriteplanes():  error in writing block header to file `%s`\n",
                     filepath);
            closeDATAfiles(info->datalist);
            (void) close(fdw);
            return(ERROR);
         }

         if ( write(fdw, (char *)tmpprojbuffer,
		   (unsigned) (tmphdrinfo->filehead->bbytes - nbheaderbytes))
		   != (tmphdrinfo->filehead->bbytes - nbheaderbytes) )
         {
            (void) printf(
                "\nwriteplanes():  error in writing data to file `%s`\n",
                      filepath);
            closeDATAfiles(info->datalist);
            (void) close(fdw);
            return(ERROR);
         }
 
         (void) close(fdw);
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	    checkdiskspace()/3		|
|					|
+--------------------------------------*/
int checkdiskspace(filepar *finfo, char *outputdir, planeInfo plselect)
{
   int			req_kbytes = 100,
			free_kbytes,
			sizeofplanes;
   struct statvfs	freeblocks_buf;


   statvfs(outputdir, &freeblocks_buf);
   if (freeblocks_buf.f_frsize >= 1024) {
       free_kbytes = freeblocks_buf.f_bavail * (freeblocks_buf.f_frsize / 1024);
   } else {
       free_kbytes = freeblocks_buf.f_bavail / (1024 / freeblocks_buf.f_frsize);
   }

   sizeofplanes = (finfo->nF2traces * finfo->nF1traces *
			finfo->nF3points) / 1000;

   if (plselect.f1f3)
   {
      req_kbytes += sizeofplanes;
      req_kbytes += (finfo->nF1traces * finfo->nF3points) / 1000;
   }

   if (plselect.f2f3)
   {
      req_kbytes += sizeofplanes;
      req_kbytes += (finfo->nF2traces * finfo->nF3points) / 1000;
   }

   if (plselect.f1f2)
   {
      req_kbytes += sizeofplanes;
      req_kbytes += (finfo->nF2traces * finfo->nF1traces *
				finfo->datatype) / 1000;
   }

   req_kbytes *= sizeof(float);
   return( (free_kbytes < req_kbytes) );
}


/*---------------------------------------
|					|
|		 main()/2		|
|					|
+--------------------------------------*/
int main(int argc, char *argv[])
{
   comInfo	*pinfo;
   planeInfo	plselect;
   info3D	*datainfo;


   if ( (pinfo = parseinput(argc, argv, &plselect)) == NULL )
      return ERROR;

   if ( (datainfo = openDATAfiles(pinfo->indirpath.sval)) == NULL )
      return ERROR;

   if ( checkdiskspace(datainfo->fileinfo, pinfo->outdirpath.sval, plselect) )
   {
      (void) printf("\ngetplane():  insufficient disk space available\n");
      return ERROR;
   }

   if (plselect.f1f3)
   { /* fast extraction */
      if ( writeplanes(datainfo, pinfo->outdirpath.sval, F1F3, FALSE) )
         return ERROR;
   }

   if (plselect.f2f3) 
   { /* medium extraction */
      if ( writeplanes(datainfo, pinfo->outdirpath.sval, F2F3, plselect.f1f2) )
         return ERROR; 
   }

   if ( plselect.f1f2 && (!plselect.f2f3) )
   { /* slow extraction */
      if ( writeplanes(datainfo, pinfo->outdirpath.sval, F1F2, FALSE) )
         return ERROR;  
   }

   if ( copypar(pinfo->indirpath.sval, pinfo->outdirpath.sval) )
      return ERROR;

   if (pinfo->curexp.vset)
      printf("\ngetplane done!\n");	/* mainly for VNMR */

   return COMPLETE;
}
