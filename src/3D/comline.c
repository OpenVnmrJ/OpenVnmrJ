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
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>

#include "constant.h"
#include "process.h"
#include "struct3d.h"

#define FT3D
#include "command.h"
#undef FT3D


extern char	*userdir;
extern void	Werrprintf();


/*---------------------------------------
|                                       |
|           parseprocmode()/2           |
|                                       |
+--------------------------------------*/
static int parseprocmode(pinfo, proctmp)
comInfo		*pinfo;
procMode	*proctmp;
{
   char		procmode[MAXPATHL];


   proctmp->procf3 = FALSE;
   proctmp->procf2 = FALSE;
   proctmp->procf1 = FALSE;

   if (pinfo->procf3acq.ival)
   {
      proctmp->procf3 = TRUE;
      return(COMPLETE);
   }

   (void) strcpy(procmode, pinfo->procmode.sval);

   if ( (strcmp(procmode, "f3f2f1") == 0) ||
	(strcmp(procmode, "f3f1f2") == 0) ||
	(strcmp(procmode, "f2f3f1") == 0) ||
	(strcmp(procmode, "f2f1f3") == 0) ||
	(strcmp(procmode, "f1f2f3") == 0) ||
	(strcmp(procmode, "f1f3f2") == 0) )
   {
      proctmp->procf3 = TRUE;
      proctmp->procf2 = TRUE;
      proctmp->procf1 = TRUE;
   }
   else if ( (strcmp(procmode, "f3f2") == 0) ||
	     (strcmp(procmode, "f2f3") == 0) )
   {
      proctmp->procf3 = TRUE; 
      proctmp->procf2 = TRUE;
   }
   else if ( (strcmp(procmode, "f2f1") == 0) ||
	     (strcmp(procmode, "f1f2") == 0) )
   {
      proctmp->procf2 = TRUE;
      proctmp->procf1= TRUE;
   }
   else if ( strcmp(procmode, "f3") == 0 )
   {
      proctmp->procf3 = TRUE;
   }
   else if ( strcmp(procmode, "f2") == 0 )
   {
      proctmp->procf2 = TRUE;
   }
   else if ( strcmp(procmode, "f1") == 0 )
   {
      proctmp->procf1 = TRUE;
   }
   else
   {
      Werrprintf("\nparseprocmode():  invalid processing mode");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            readselect()/2             |
|                                       |
+--------------------------------------*/
static ipar readselect(argstr, pinfo)
char    *argstr;
comInfo	*pinfo;
{
   ipar	tmp;


   if (*argstr++ == '-')
   {
      switch (*argstr)
      {
         case 'a':   tmp.pmode = pinfo->autofilepath.pmode;
		     pinfo->autofilepath.vset = TRUE;
		     tmp.ival = AUTO_FILE;
		     return(tmp);
         case 'c':   tmp.pmode = pinfo->coeffilepath.pmode;
		     pinfo->coeffilepath.vset = TRUE;
		     tmp.ival = INP_COEF_FILE;
		     return(tmp);
         case 'e':   tmp.pmode = pinfo->curexp.pmode;
		     tmp.ival = EXP_NUM;
		     return(tmp);
         case 'f':   tmp.pmode = pinfo->overwrite.pmode;
		     pinfo->overwrite.vset = TRUE;
		     pinfo->overwrite.ival = TRUE;
		     tmp.ival = OVER_WRITE;
		     return(tmp);
         case 'F':   tmp.pmode = pinfo->fdfheaderpath.pmode;
		     tmp.ival = FDF_OUTPUT;
		     return(tmp);
         case 'h':   tmp.pmode = pinfo->xrpc.pmode;
#ifndef LINUX
		     pinfo->xrpc.vset = TRUE;
		     pinfo->xrpc.ival = TRUE;
#endif
		     tmp.ival = X_RPC;
		     return(tmp);
         case 'H':   tmp.pmode = pinfo->hostAddr.pmode;
		     tmp.ival = IOHOSTADDR;
		     return(tmp);
         case 'i':   tmp.pmode = pinfo->fiddirpath.pmode;
		     tmp.ival = INP_FID_DIR;
		     return(tmp);
         case 'l':   tmp.pmode = pinfo->logfile.pmode;
		     pinfo->logfile.vset = TRUE;
		     pinfo->logfile.ival = TRUE;
		     tmp.vset = TRUE;
		     tmp.ival = LOG_FILE;
		     return(tmp);
         case 'm':   tmp.pmode = pinfo->multifile.pmode;
		     pinfo->multifile.vset = TRUE;
		     tmp.ival = MULTI_FILE;
		     return(tmp);
         case 'n':   tmp.pmode = pinfo->info3Ddirpath.pmode;
		     pinfo->info3Ddirpath.vset = TRUE;
		     tmp.ival = INP_INFO_DIR;
		     return(tmp);
         case 'o':   tmp.pmode = pinfo->datadirpath.pmode;
		     tmp.ival = OUT_DATA_DIR;
		     return(tmp);
         case 'p':   tmp.pmode = pinfo->procmode.pmode;
		     tmp.ival = PROC_MODE;
		     return(tmp);
         case 'r':   tmp.pmode = pinfo->reduce.pmode;
		     pinfo->reduce.vset = TRUE;
		     pinfo->reduce.ival = TRUE;
		     tmp.ival = REDUCE;
		     return(tmp);
         case 's':   tmp.pmode = pinfo->procf3acq.pmode;
                     pinfo->procf3acq.vset = TRUE;
                     pinfo->procf3acq.ival = TRUE;
		     tmp.ival = PROCF3_ACQ;
		     return(tmp);
         case 't':   tmp.pmode = pinfo->fidmapfilepath.pmode;
		     pinfo->fidmapfilepath.vset = TRUE;
		     tmp.ival = FID_MAP;
		     return(tmp);
         case 'x':   tmp.pmode = pinfo->getplane.pmode;
		     tmp.ival = GET_PLANE;
                     pinfo->getplane.vset = TRUE;
		     return(tmp);
         default:    Werrprintf("\nreadselect():  invalid option");
		     tmp.pmode = EITHER;
		     tmp.ival = ERROR;
                     return(tmp);
      }
   }          
   else
   {
      Werrprintf("\nreadselect():  improper input format");
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
static int setselect(argstr, selection, pinfo)
char    *argstr;
int     selection;
comInfo *pinfo;
{      
   switch (selection)
   {
      case AUTO_FILE:	   (void) strcpy(pinfo->autofilepath.sval, argstr);
			   pinfo->autofilepath.vset = TRUE;
			   break;
      case INP_COEF_FILE:  (void) strcpy(pinfo->coeffilepath.sval, argstr);
			   pinfo->coeffilepath.vset = TRUE;
                           break;
      case EXP_NUM:        pinfo->curexp.ival = atoi(argstr);
                           if ( (pinfo->curexp.ival < FIRST_EXP) ||
				(pinfo->curexp.ival > LAST_EXP) )
                           {
                              Werrprintf("\nsetselect():  invalid experiment");
                              return(ERROR);
                           }
			   pinfo->curexp.vset = TRUE;
                           break;
      case INP_FID_DIR:    (void) strcpy(pinfo->fiddirpath.sval, argstr);
			   pinfo->fiddirpath.vset = TRUE;
                           break;
      case INP_INFO_DIR:  (void) strcpy(pinfo->info3Ddirpath.sval, argstr);
			   pinfo->info3Ddirpath.vset = TRUE;
                           break;
      case MULTI_FILE:     pinfo->multifile.ival = atoi(argstr);
			   break;
      case OUT_DATA_DIR:   (void) strcpy(pinfo->datadirpath.sval, argstr);
			   pinfo->datadirpath.vset = TRUE;
                           break;
      case PROC_MODE:	   (void) strcpy(pinfo->procmode.sval, argstr);
			   pinfo->procmode.vset = TRUE;
			   break;
      case FID_MAP:	   (void) strcpy(pinfo->fidmapfilepath.sval, argstr);
			   pinfo->fidmapfilepath.vset = TRUE;
			   break;
      case GET_PLANE:      (void) strcpy(pinfo->getplane.sval, argstr);
                           break;
      case PROCF3_ACQ:	   pinfo->procf3acq.ival = atoi(argstr);
			   break;
      case FDF_OUTPUT:	   (void) strcpy(pinfo->fdfheaderpath.sval, argstr);
			   pinfo->fdfheaderpath.vset = TRUE;
			   break;
      case IOHOSTADDR:	   (void) strcpy(pinfo->hostAddr.sval, argstr);
			   pinfo->hostAddr.vset = TRUE;
			   break;
      default:             break;
   }
 
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            init3Dinfo()/1             |
|                                       |
+--------------------------------------*/
static void init3Dinfo(pinfo)
comInfo	*pinfo;
{
   pinfo->curexp.pmode = SET;
   pinfo->reduce.pmode = SELECT;
   pinfo->overwrite.pmode = SELECT;
   pinfo->fiddirpath.pmode = SET;
   pinfo->datadirpath.pmode = SET;
   pinfo->logfile.pmode = SELECT;
   pinfo->autofilepath.pmode = EITHER;
   pinfo->coeffilepath.pmode = EITHER;
   pinfo->info3Ddirpath.pmode = EITHER;
   pinfo->procmode.pmode = SET;
   pinfo->procf3acq.pmode = EITHER;
   pinfo->multifile.pmode = SET;
   pinfo->fidmapfilepath.pmode = EITHER;
   pinfo->xrpc.pmode = SELECT;
   pinfo->getplane.pmode = EITHER;
   pinfo->fdfheaderpath.pmode = EITHER;
   pinfo->hostAddr.pmode = SET;

   pinfo->curexp.vset = FALSE;
   pinfo->reduce.vset = FALSE;
   pinfo->overwrite.vset = FALSE;
   pinfo->fiddirpath.vset = FALSE;
   pinfo->datadirpath.vset = FALSE;
   pinfo->logfile.vset = FALSE;
   pinfo->autofilepath.vset = FALSE;
   pinfo->coeffilepath.vset = FALSE;
   pinfo->info3Ddirpath.vset = FALSE;
   pinfo->procmode.vset = FALSE;
   pinfo->procf3acq.vset = FALSE;
   pinfo->multifile.vset = FALSE;
   pinfo->fidmapfilepath.vset = FALSE;
   pinfo->xrpc.vset = FALSE;
   pinfo->getplane.vset = FALSE;
   pinfo->fdfheaderpath.vset = FALSE;
   pinfo->hostAddr.vset = FALSE;

   pinfo->curexp.ival = 0;
   pinfo->reduce.ival = FALSE;
   pinfo->overwrite.ival = FALSE;
   pinfo->multifile.ival = FALSE;
   pinfo->xrpc.ival = FALSE;
   pinfo->logfile.ival = FALSE;
   pinfo->procf3acq.ival = FALSE;
   (void) strcpy(pinfo->fiddirpath.sval, "");
   (void) strcpy(pinfo->datadirpath.sval, "");
   (void) strcpy(pinfo->autofilepath.sval, "");
   (void) strcpy(pinfo->coeffilepath.sval, "");
   (void) strcpy(pinfo->info3Ddirpath.sval, "");
   (void) strcpy(pinfo->fidmapfilepath.sval, "nomap");
   (void) strcpy(pinfo->procmode.sval, "");
   (void) strcpy(pinfo->getplane.sval, "noplanes");
   (void) strcpy(pinfo->fdfheaderpath.sval, "");
   (void) strcpy(pinfo->hostAddr.sval, "");
}


/*---------------------------------------
|                                       |
|            parseinput()/4             |
|                                       |
+--------------------------------------*/
comInfo *parseinput(argc, argv, prcinfo, ft3dmaster)
char   		*argv[];
int    		argc,
		ft3dmaster;
procMode	*prcinfo;
{
   char         expstr[10],
		tmpfiddirpath[MAXPATHL],
		tmpdatadirpath[MAXPATHL],
		tmpcoeffilepath[MAXPATHL],
		tmpinfo3Ddirpath[MAXPATHL],
		tmpprocmode[MAXPATHL],
		tmpmapfilepath[MAXPATHL],
		tmpgetplane[MAXPATHL],
		expfilepath[MAXPATHL],
		workingdirpath[ MAXPATHL ],
		*pwdir;
   int          argno = 1,
                mode = SELECT,
		tmpexpnum,
		tmpreduce,
		tmpoverwrite,
		tmpprocf3acq,
		tmpmultifile,
		tmplogfile;
   ipar		selection;
   comInfo      *pinfo;
   void		pathadj();
   FILE		*fa;
 
 
/*****************************************
*  Allocate memory for the command line  *
*  information structure.                *
*****************************************/

   if ( (pinfo = (comInfo *) malloc( (unsigned) sizeof(comInfo) ))
                == NULL )
   {
      Werrprintf("\nparseinput():  cannot allocate memory for command data");
      return(NULL);
   }


/******************************************
*  Initialize the pointer to the command  *
*  information structure.                 *
******************************************/
 
   init3Dinfo(pinfo);
 

/**********************************
*  Parse the input command line.  *
**********************************/

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
      Werrprintf("\nparseinput():  improper input format");
      return(NULL);
   }


/*********************************************
*  Set experiment directory path if 3D data  *
*  processing it to occur within a VNMR      *
*  experiment.                               *
*********************************************/

   if (pinfo->curexp.vset)
   {
      if ( (userdir = getenv("vnmruser")) == NULL)
      {
         Werrprintf("\nparseinput():  no `$vnmruser` environmental variable");
         return(NULL);
      }

      (void) sprintf(expstr, "%d", pinfo->curexp.ival);
      (void) strcpy(expfilepath, userdir);
      (void) strcat(expfilepath, "/exp");
      (void) strcat(expfilepath, expstr);
   }


/**************************************
*  Read in input parameters from the  *
*  automation text file.              *
**************************************/

   if (pinfo->autofilepath.vset)
   {
      if ( strcmp(pinfo->autofilepath.sval, "") == 0 )
      {
         if (pinfo->curexp.vset)
         {
            (void) strcpy(pinfo->autofilepath.sval, expfilepath);
            (void) strcat(pinfo->autofilepath.sval, "/auto");
         }
         else
         {
            (void) strcpy(pinfo->autofilepath.sval, "auto");
         }
      }

      if ( (fa = fopen(pinfo->autofilepath.sval, "r")) == NULL )
      {
         pinfo->autofilepath.vset = FALSE;
         (void) strcpy(pinfo->autofilepath.sval, "");
      }
      else
      {
         if ( fscanf(fa, "%d\n", &tmpoverwrite) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%d\n", &tmpreduce) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%d\n", &tmpexpnum) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%d\n", &tmpprocf3acq) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%d\n", &tmpmultifile) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%d\n", &tmplogfile) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpfiddirpath) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpdatadirpath) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpcoeffilepath) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpinfo3Ddirpath) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpprocmode) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpmapfilepath) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         if ( fscanf(fa, "%s\n", tmpgetplane) == EOF )
         {
            Werrprintf("\nparseinput():  improper automation file format");
            (void) fclose(fa);
            return(NULL);
         }

         (void) fclose(fa);
      }
   }


/*******************************************
*  Resolve whether the FID data is to be   *
*  transformed within a VNMR experimant    *
*  or outside of a VNMR experiment.  Also  *
*  sort out the `reduce` and `overwrite`   *
*  selections.                             *
*******************************************/

   if ( (pinfo->autofilepath.vset) && (!pinfo->curexp.vset) )
   {
      pinfo->curexp.vset = ( (tmpexpnum >= FIRST_EXP) &&
					(tmpexpnum <= LAST_EXP) );
      if (pinfo->curexp.vset)
         pinfo->curexp.ival = tmpexpnum;
   }

   if (pinfo->autofilepath.vset)
   {
      if (!pinfo->overwrite.vset)
         pinfo->overwrite.ival = tmpoverwrite;

      if (!pinfo->reduce.vset)
         pinfo->reduce.ival = tmpreduce;

      if (!pinfo->multifile.vset)
         pinfo->multifile.ival = tmpmultifile;

      if (!pinfo->procf3acq.vset)
         pinfo->procf3acq.ival = tmpprocf3acq;

      if (!pinfo->getplane.vset)
      {
         (void) strcpy(pinfo->getplane.sval, tmpgetplane);
         pinfo->getplane.vset = ( strcmp(tmpgetplane, "noplanes") != 0 );
      }

      if (!pinfo->logfile.vset)
         pinfo->logfile.ival = tmplogfile;
   }

/* overridden for remote processes */
   if (!ft3dmaster)
      pinfo->procf3acq.ival = FALSE;

   if ( (pinfo->procf3acq.ival) && (!pinfo->curexp.vset) )
   {
      Werrprintf("\nparseinput():  experiment no. required with -s option");
      return(NULL);
   }

   if (pinfo->multifile.vset)
   {
      if (pinfo->multifile.ival > MAXFILES)
         pinfo->multifile.ival = MAXFILES;
      if (pinfo->multifile.ival < 0)
         pinfo->multifile.ival = 0;
   }

   if ( pinfo->getplane.vset &&
	(strcmp(pinfo->getplane.sval, "noplanes") == 0) )
   {
      (void) strcpy(pinfo->getplane.sval, "f1f3:f2f3");
   }

/**********************************************
*  The FID data is to be found outside of a   *
*  VNMR experiment.  The automation file      *
*  stores the complete filepath for the FID   *
*  directory; otherwise, the .fid file ex-    *
*  tension must be appended.  If no filepath  *
*  is provided by either direct entry or an   *
*  automation file, an error results.         *
**********************************************/

   if (!pinfo->fiddirpath.vset)
   {
      if (pinfo->autofilepath.vset)
      {
         (void) strcpy(pinfo->fiddirpath.sval, tmpfiddirpath);
      }
      else if (pinfo->curexp.vset)
      {
         (void) strcpy(pinfo->fiddirpath.sval, expfilepath);
         (void) strcat(pinfo->fiddirpath.sval, "/acqfil");
      }
      else
      {
         Werrprintf("\nparseinput():  FID file path undefined");
         return(NULL);
      }
   }
   else
   {
      (void) strcat(pinfo->fiddirpath.sval, ".fid");
   }
 
/************************************************
*  The DATA3D data is to be found outside of    *
*  a VNMR experiment.  The automation file      *
*  stores the complete filepath for the DATA3D  *
*  directory; otherwise, the .data file exten-  *
*  sion must be appended.  If no filepath is    *
*  provided by either direct entry or an auto-  *
*  mation file, an error results.               *
************************************************/

   if (!pinfo->datadirpath.vset)
   {
      if (pinfo->autofilepath.vset)
      {
         (void) strcpy(pinfo->datadirpath.sval, tmpdatadirpath);
      }
      else if (pinfo->curexp.vset)
      {
         (void) strcpy(pinfo->datadirpath.sval, expfilepath);
         (void) strcat(pinfo->datadirpath.sval, "/datadir3d");

         if ( mkdir(pinfo->datadirpath.sval, 0777) )
         {
            if (errno != EEXIST)
            {
               Werrprintf("\nparseinput():  cannot create directory `%s`",
                              pinfo->datadirpath.sval);
               return(NULL);
            }
         }

         (void) strcat(pinfo->datadirpath.sval, "/data");
      }
      else
      {
         Werrprintf("\nparseinput():  DATA file path undefined");
         return(NULL);
      }
   }
   else
   {
      if ( mkdir(pinfo->datadirpath.sval, 0777) )
      {
         if (errno != EEXIST)
         {
            Werrprintf("\nparseinput():  cannot create directory `%s`",
                           pinfo->datadirpath.sval);
            return(NULL);
         }
      }

      (void) strcat(pinfo->datadirpath.sval, "/data");
   }

/*************************************************
*  Set the filepath for the coefficient file.    *
*  If it has already been set by direct entry,   *
*  do nothing.  Otherwise, use either the value  *
*  stored in the automation file (-a option) or  *
*  the default value ('coef').                   *
*************************************************/

   if (!pinfo->coeffilepath.vset)
   {
      if (pinfo->autofilepath.vset)
      {
         (void) strcpy(pinfo->coeffilepath.sval, tmpcoeffilepath);
      }
      else if (pinfo->curexp.vset)
      {
         (void) strcpy(pinfo->coeffilepath.sval, expfilepath);
         (void) strcat(pinfo->coeffilepath.sval, "/coef");
      }
      else
      {
         (void) strcpy(pinfo->coeffilepath.sval, "coef");
      }
   }

/*************************************************
*  Set the filepath for the FID mapping file.	 *
*  If it has already been set by direct entry,	 *
*  do nothing.  Otherwise, use either the value  *
*  stored in the automaton file (-a option) or   *
*  the default value ('map').                    *
*************************************************/

   if (pinfo->fidmapfilepath.vset)
   {
      if ( strcmp(pinfo->fidmapfilepath.sval, "nomap") == 0 )
      {
         (void) strcpy(pinfo->fidmapfilepath.sval, expfilepath);
         (void) strcat(pinfo->fidmapfilepath.sval, "/map");
      }
      else
      {
         (void) strcpy(pinfo->fidmapfilepath.sval, "map");
      }
   }
   else if (pinfo->autofilepath.vset)
   {
      if ( strcmp(tmpmapfilepath, "nomap") != 0 )
      {
         (void) strcpy(pinfo->fidmapfilepath.sval, tmpmapfilepath);
         pinfo->fidmapfilepath.vset = TRUE;
      }
   }

/*************************************************
*  Set the dirpath for the information file.     *
*  if it has already been set by direct entry,   *
*  do nothing.  Otherwise, use either the value  *
*  stored in the automation file (-a option) or  *
*  the default value ('info').                   *
*************************************************/

   if (!pinfo->info3Ddirpath.vset)
   {
      if (pinfo->autofilepath.vset)
      {
         (void) strcpy(pinfo->info3Ddirpath.sval, tmpinfo3Ddirpath);
      }
      else if (pinfo->curexp.vset)
      {
         (void) strcpy(pinfo->info3Ddirpath.sval, expfilepath);
         (void) strcat(pinfo->info3Ddirpath.sval, "/info");
      }
      else
      {
         (void) strcpy(pinfo->info3Ddirpath.sval, "info");
      }
   }

/************************************************
*  Set the processing mode.  If it has already  *
*  been set by direct entry, do nothing.  If    *
*  it has not been set, use either the value    *
*  stored in the automation file (-a option)    *
*  or the default value ('f3f2f1').             *
************************************************/

   if (!pinfo->procmode.vset)
   {
      if (pinfo->autofilepath.vset)
      {
         (void) strcpy(pinfo->procmode.sval, tmpprocmode);
      }
      else if (ft3dmaster)
      {   
         (void) strcpy(pinfo->procmode.sval, "f3f2f1");
      }
      else
      {
         (void) strcpy(pinfo->procmode.sval, "f2f1");
      }
   }

/********************************************
*  Initialize the procMode structure based  *
*  on the string value for 'procmode'.      *
********************************************/

   if ( parseprocmode(pinfo, prcinfo) )
      return(NULL);

   if (!ft3dmaster)
      prcinfo->procf3 = FALSE;

/***************************************************
*  If the '-s #' mode has been invoked and if the  *
*  argument # is > 1, return to the calling func-  *
*  tion at this time.                              *
***************************************************/

   if (pinfo->procf3acq.ival > 1)
      return(pinfo);

/***********************************************************************
*  If F3 processing is being done, attempt to create the DATA3D/data   *
*  directory.  If it cannot be created because it already exists and   *
*  the -f option has not been selected, FT3D exits with an error.  If  *
*  F3 processing is not being done, do not attempt to create the data  *
*  directory.  If the -f option has been selected and F3 processing    *
*  is being started, delete the DATA3D/extr directory as well.  Only   *
*  check that the FID mapping file exists if F3 processing is being    *
*  performed under the -t option.                                      *
***********************************************************************/

   if (prcinfo->procf3 && pinfo->overwrite.ival)
   {
      char	planedir[MAXPATHL],
         	syscmd[MAXPATHL]; 
      int	n;

      (void) strcpy(planedir, pinfo->datadirpath.sval);
      n = strlen(planedir);
      planedir[n-5] = '\0';
      (void) strcat(planedir, "/extr");
      sprintf(syscmd, "rm -rf \"%s\"\n", planedir); 
      (void) system(syscmd); 
   }

   if (prcinfo->procf3)
   {
      if ( mkdir(pinfo->datadirpath.sval, 0777) )
      {
         if (errno == EEXIST)
         {
            if (!pinfo->overwrite.ival)
            {
               Werrprintf("\nparseinput():  directory `%s` already exists",
                              pinfo->datadirpath.sval);
               return(NULL);
            }
            else
            {   
               char syscmd[MAXPATHL]; 
 
               sprintf(syscmd, "rm -rf \"%s\"\n", pinfo->datadirpath.sval); 
               (void) system(syscmd); 
 
               if ( mkdir(pinfo->datadirpath.sval, 0777) )
               {
                  (void) printf(
			"\nparseinput():  cannot create directory `%s`\n",
                              pinfo->datadirpath.sval);
                  return(NULL);
               }
            }
         }
         else
         {
            Werrprintf("\nparseinput():  cannot create directory `%s`",
                           pinfo->datadirpath.sval);
            return(NULL);
         }
      }

      if (pinfo->fidmapfilepath.vset)
      { /* only necessary for F3 processing */
         if ( access(pinfo->fidmapfilepath.sval, R_OK) )
         {
            Werrprintf("\nparseinput():  cannot access FID mapping file");
            return(NULL);
         }
      }
   }
   else
   {
      (void) strcpy(tmpinfo3Ddirpath, pinfo->datadirpath.sval);
      (void) strcat(tmpinfo3Ddirpath, "/info");
      (void) strcpy(tmpcoeffilepath, tmpinfo3Ddirpath);
      (void) strcat(tmpcoeffilepath, "/coef");

      if ( access(tmpinfo3Ddirpath, F_OK) )
      {
         Werrprintf("\nparseinput():  cannot access the directory `%s`",
			tmpinfo3Ddirpath);
         return(NULL);
      }
      else
      {
         char	fpath[MAXPATHL];


         (void) strcpy(fpath, tmpinfo3Ddirpath);
         (void) strcat(fpath, "/procdat");

         if ( access(fpath, R_OK) )
         {
            Werrprintf("\nparseinput():  cannot access the file `%s`", fpath);
            return(NULL);
         }

         (void) strcpy(fpath, tmpinfo3Ddirpath);
         (void) strcat(fpath, "/procpar3d"); 
 
         if ( access(fpath, R_OK) )
         { 
            Werrprintf("\nparseinput():  cannot access the file `%s`", fpath);
            return(NULL);
         }

         if ( access(tmpcoeffilepath, R_OK) )
         {
            Werrprintf("\nparseinput():  cannot access the file `%s`",
			   tmpcoeffilepath);
            return(NULL);
         }
      }
   }
 
/*****************************************
*  Write out the automation file if the  *
*  Ft3d is the master program.           *
*****************************************/

   if (ft3dmaster)
   {
      if ( strcmp(pinfo->autofilepath.sval, "") == 0 )
      {
         if (pinfo->curexp.vset)
         {
            (void) strcpy(pinfo->autofilepath.sval, expfilepath);
            (void) strcat(pinfo->autofilepath.sval, "/auto");
         }
         else
         {
            (void) strcpy(pinfo->autofilepath.sval, "auto");
         }
      }

      if ( (pwdir = getcwd( &workingdirpath[ 0 ], sizeof( workingdirpath ) )) != NULL )
      {
         if ( (fa = fopen(pinfo->autofilepath.sval, "w")) != NULL )
         {
            char	newpath[MAXPATHL];

   
            (void) fprintf(fa, "%d\n", pinfo->overwrite.ival);
            (void) fprintf(fa, "%d\n", pinfo->reduce.ival);
            (void) fprintf(fa, "%d\n", pinfo->curexp.ival);
            (void) fprintf(fa, "%d\n", pinfo->procf3acq.ival);
            (void) fprintf(fa, "%d\n", pinfo->multifile.ival);
            (void) fprintf(fa, "%d\n", pinfo->logfile.ival);

            pathadj(pinfo->fiddirpath.sval, pwdir, newpath);
            (void) fprintf(fa, "%s\n", newpath);
   
            pathadj(pinfo->datadirpath.sval, pwdir, newpath);
            (void) fprintf(fa, "%s\n", newpath);

            pathadj(pinfo->coeffilepath.sval, pwdir, newpath);
            (void) fprintf(fa, "%s\n", newpath);

            pathadj(pinfo->info3Ddirpath.sval, pwdir, newpath);
            (void) fprintf(fa, "%s\n", newpath);

            (void) fprintf(fa, "%s\n", pinfo->procmode.sval);

            if ( strcmp(pinfo->fidmapfilepath.sval, "nomap") == 0 )
            {
               (void) fprintf(fa, "%s\n", pinfo->fidmapfilepath.sval);
            }
            else
            {
               pathadj(pinfo->fidmapfilepath.sval, pwdir, newpath);
               (void) fprintf(fa, "%s\n", newpath);
            }

            (void) fprintf(fa, "%s\n", pinfo->getplane.sval);
            (void) fclose(fa);
         }
      }
   }


/************************************************
*  FID mapping is not allowed if F3 processing  *
*  is being performed concurrent with data      *
*  acquisition (-s option).                     *
************************************************/

   if (pinfo->procf3acq.ival && pinfo->fidmapfilepath.vset)
   {
      Werrprintf("\nparseinput():  FID mapping not allowed with -s option");
      return(NULL);
   }

   if (!prcinfo->procf3)
   {
      (void) strcpy(pinfo->info3Ddirpath.sval, tmpinfo3Ddirpath);
      (void) strcpy(pinfo->coeffilepath.sval, tmpcoeffilepath);
   }

   return(pinfo);
}


/*---------------------------------------
|					|
|	       pathadj()/3		|
|					|
+--------------------------------------*/
void pathadj(pathname, pwdir, newpath)
char	*pathname,
	*pwdir,
	*newpath;
{
   if (pathname[0] == '/')	/* absolute path name */
   {
      (void) strcpy(newpath, pathname);
      return;
   }

   (void) strcpy(newpath, pwdir);
   (void) strcat(newpath, "/");
   (void) strcat(newpath, pathname);
}
