/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*******************************************************************************
* savretphf.c:  Contains code to save and return an alternate phase file.      *
* -----------   The Vnmr callable routines are svphf and rtphf.  These         *
*               routines are just interfaces which call routines               *
*               save_phasefile and return_phasefile.  These routines return    *
*               error codes from data.h.  Eventually, these routines may be    *
*               merged into data.c.		                               *
*******************************************************************************/

#include "vnmrsys.h"

#ifdef UNIX
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#else 
#include <file.h>
#define F_OK	0
#endif 
#include <errno.h>

extern int start_from_ft;

#include "data.h"
#include "disp.h"
#include "group.h"
#include "init2d.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

/* Full path to the default phasefile */
static char default_phasefile[MAXPATHL];
static char default_datafile[MAXPATHL];
#define TRUE 1
#define FALSE 0
#define ERROR		1

static void build_default_phasefile();
static void build_default_datafile();
static int filecopy (int f1, int f2 );
static int save_phasefile (char *outname );
static int return_phasefile (char *path );

/******************************************************************************/
int svphf ( argc, argv, retc, retv )
/* 
Purpose:
-------
     Routine svphf is the Vnmr interface for calling save_phasefile.  Its only
argument is the name for the new phasefile, which will be placed in the /planes
subdirectory of the current experiment.  Return arguments are not set.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Number of passed arguments, that is, the dimension of the
		following argument.
argv  :  (I  )  (Pointer to) array of input arguments.
retc  :  (I  )  Number of returned arguments, that is, the dimension of the
		following argument.
retv  :  (   )  (Pointer to) array of output arguments.  Not used in this case.
*/
int argc, retc;
char *argv[], *retv[];
{ /* Begin function svphf */
   /*
   Local Variables:
   ---------------
   err  :  Error number returned by subroutine.
   */
   int err;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Verify that the number of arguments is correct */
   if ( argc != 2 )
   {
      Werrprintf ( "Usage:  svphf('file-name')" );
      ABORT;
   }

   /* Call the service routine to do all the work */
   disp_status ( "SVPHF" );
   err = save_phasefile ( argv[1] );
   disp_status ( "    " );

   /* Error return, if necessary */
   if ( err != 0 )
      ABORT;

   /* Normal, successful return */
   RETURN;

} /* End function svphf */

/******************************************************************************/
static int save_phasefile (char *outname )
/* 
Purpose:
-------
     Routine save_phasefile shall save the contents of a phasefile.  The current
phasefile is copied to another file (name of which is an argument).
     In this version, the copy is done using unix I/O calls open, creat, read,
and write.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
outname  :  (I  )  (Pointer to) name of file to receive the phasefile data.
*/
{ /* Begin function save_phasefile */
   /*
   Local Variables:
   ---------------
   f1             :  Unix file pointer for current phasefile.
   f2             :  Unix file pointer for file to be created.
   newpath        :  Full path of the new phasefile.
   err            :  Error indicator returned by a function.
   string	  :  Used to get a response from the user.
   */
   int f1, f2, err;
   char newpath[MAXPATHL], string[3];
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Flush out the phasefile to guarantee that its contents are valid */
   if ( (err=D_flush(D_PHASFILE)) != 0 )
      if ( err != D_NOTOPEN )
      {
	 Werrprintf ( "Could not flush existing phasefile, error number %d",
		      err );
	 ABORT;
      }

   /* Compose path of existing phasefile */
   build_default_phasefile();

   /* Open the input file to copy */
   if ( (f1=open(default_phasefile,O_RDONLY)) == -1 )
   {
      Werrprintf ( "Could not open %s for reading", default_phasefile );
      ABORT;
   }

   /* Create the "planes" subdirectory, if necessary. */
   strcpy ( newpath, curexpdir );
#ifdef UNIX
   strcat ( newpath, "/planes" );
#else 
   strcat ( newpath, "planes.dir" );
#endif 
   if ( access ( newpath, F_OK ) != 0 )
   {
#ifdef VMS
      make_vmstree( newpath, newpath, MAXPATHL );
#endif 
#ifdef SIS
      if ( mkdir ( newpath, 0755 ) )
#else 
      if ( mkdir ( newpath, 0777 ) )
#endif 
      {  Werrprintf ( "cannot create directory %s", newpath );
	 close ( f1 );
	 ABORT;
      }
   }

   /* Obtain path to the output phase file */
#ifdef UNIX
   strcat ( newpath, "/" );
   strcat ( newpath, outname );
#else 
   strcpy ( newpath, curexpdir );
   vms_fname_cat ( newpath, "[.planes]" );
   strcat ( newpath, outname );
#endif 

   /* If file already exists, get confirmation from user */
   if ( access ( newpath, F_OK ) == 0 )
   {  
      W_getInput ( "File already exists.  Overwrite (Y/N)?", string, 
		   sizeof(string) );
      if ( string[0] != 'y' && string[0] != 'Y' )
      {  close ( f1 );
	 ABORT;
      }
   }

   /* Open the output file to copy */
   if ( (f2=open(newpath,O_WRONLY|O_CREAT,0666)) == -1 )
   {
      Werrprintf ( "Could not open %s for writing, unix error number %d", 
		   newpath, errno );
      close ( f1 );
      ABORT;
   }

   /* Do the file copy.  This also closes files. */
   if ( filecopy ( f1, f2 ) != 0 )
   {  unlink ( newpath );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function save_phasefile */

/******************************************************************************/
int rtphf ( argc, argv, retc, retv )
/* 
Purpose:
-------
     Routine rtphf is the Vnmr interface for calling return_phasefile.  Its only
argument is the name of the new phasefile.  The new phasefile is found in the
/planes subdirectory of the current experiment.  Return arguments are not set.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Number of passed arguments, that is, the dimension of the
		following argument.
argv  :  (I  )  (Pointer to) array of input arguments.
retc  :  (I  )  Number of returned arguments, that is, the dimension of the
		following argument.
retv  :  (   )  (Pointer to) array of output arguments.  Not used in this case.
*/
int argc, retc;
char *argv[], *retv[];
{ /* Begin function rtphf */
   /*
   Local Variables:
   ---------------
   newpath  :        Full path of the new phasefile.
   err            :  Error number returned by subroutine.
   */
   char newpath[MAXPATHL];
   char		ni0name[6];
   int		r;
   double	rni;
   int          dim0,dim1;
   int err;
   dfilehead	phasehead;	/* header information for phasfile	*/
   int	ndflag = ONE_D; /* number of Data dimensions            */
   char     normchar,revchar;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Verify that the number of arguments is correct,                   */
   /* and set path to new file.			                        */
   if ( argc != 2 )
   {  Werrprintf ( "Usage:  rtphf('file-path')" );
      ABORT;
   }
   /* Put path to new phasefile together */
   strcpy ( newpath, curexpdir );
#ifdef UNIX
   strcat ( newpath, "/planes/" );
#else 
   vms_fname_cat ( newpath, "[.planes]" );
#endif 
   strcat ( newpath, argv[1] );

   /* Call the service routine to do all the work */
   disp_status ( "RTPHF" );
   err = return_phasefile ( newpath );

   /* Error return, if necessary */
   if ( err != 0 )
   {
      strcpy( newpath, argv[1] );
      err = return_phasefile ( newpath );
      if ( err != 0 )
      {
	Werrprintf( "Could not open %s for reading in %s%s or current dir.",
				 newpath,curexpdir,"/planes/");
      	ABORT;
      }
   }
   disp_status ( "    " );

   err = D_gethead(D_PHASFILE, &phasehead);
   if (err != 0)
      ABORT;

/******************************************************
*  Determines whether the experiment contains 1D or   *
*  2D data; sets D2FLAG, REVFLAG, and NI; if get_rev  *
*  is FALSE, REVFLAG is assumed to be set by the      *
*  calling program.                                   *
******************************************************/

  /* Defaults for 1D data */
  ndflag = ONE_D;
  dim1 = dim0 = S_NP;
  ni = 1;
  normchar = ' ';
  revchar = '1';
  if (phasehead.status & S_TRANSF)	/* 2D data */
  {
     ndflag = TWO_D;
     normchar = '2';
     if (phasehead.status & S_3D)
        ndflag = THREE_D;

     if (ndflag == TWO_D)
     {
        if (phasehead.status & S_NF)
        {
           strcpy(ni0name, "nf");
        }
        else if (phasehead.status & S_NI)
        { 
           strcpy(ni0name, "ni"); 
        } 
        else if (phasehead.status & S_NI2)
        {
           strcpy(ni0name, "ni2");
        }
        else
        {
           Werrprintf("Invalid F1 dimension in first data block header");
           ABORT;
        }
     }

     if (ndflag == THREE_D)
     {
        if ((phasehead.status & (S_NP|S_NF)) == (S_NP|S_NF) )
        {
           strcpy(ni0name, "nf");
           normchar = '3';
           revchar = '1';
        }
        else if ((phasehead.status & (S_NP|S_NI)) == (S_NP|S_NI) )
        { 
           strcpy(ni0name, "ni"); 
           normchar = '3';
           revchar = '1';
        } 
        else if ((phasehead.status & (S_NP|S_NI2)) == (S_NP|S_NI2) )
        {
           strcpy(ni0name, "ni2");
           normchar = '3';
           revchar = '2';
        }
        else if ((phasehead.status & (S_NI|S_NI2)) == (S_NI|S_NI2) )
        {
           strcpy(ni0name, "ni");
           normchar = '2';
           revchar = '1';
        }
        else
        {
           Werrprintf("Invalid dimension in first data block header");
           ABORT;
        }
     }

     if ( (r = P_getreal(PROCESSED, ni0name, &rni, 1)) )
     {
        Werrprintf("Unable to get %s 2D parameter for 2D data matrix\n",
			ni0name);
        d2flag = FALSE;
        ABORT;
     }
     else if (rni < 1.5)
     {
        Werrprintf("Invalid %s value (%d) for 2D data matrix\n", ni0name,
			(int)rni);
        d2flag = FALSE;
        ABORT;
     }
     else
     {
	/*------------------------------------------------------*/
	/* If supposed to be 2D copy phasefile to datafile	*/
	/* to ensure init2d picks up second dimension		*/
	/*------------------------------------------------------*/
	if (!d2flag)
	{
	    /*------------------------------------------------------*/
	    /* If no 2D data in datafile abort since, currently,    */
	    /* the cp_phasefile_datafile() routine does not work.   */
	    /* It only copies the phasefile directly which consists */
	    /* of the real or absval data points.  It has to add    */
	    /* the complex data points.         mrh                 */
	    /*------------------------------------------------------*/
            Werrprintf("Must have 2D data in data file\n");
            ABORT;
	   /* cp_phasefile_datafile();*/
	}
	d2flag=TRUE;
     }
   }

   /* Display data, as necessary */
   if ( !Bnmr )
   {
      releasevarlist();
      if ( d2flag )
      {
	 /* execString("dconi('dcon','gray','linear')\n"); */
         releasevarlist();
	 appendvarlist("dcon");
         appendvarlist("gray"); /* default to linear gray scale if imager */
         appendvarlist("linear");
	 Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
      }
      else
      {
	 /* execString("ds('cr')\n"); */
	 releasevarlist();         
	 appendvarlist("cr");
	 Wsetgraphicsdisplay("ds");  /* activate the dconi program */
         start_from_ft = 1;
      }
   }
   disp_index ( 0 );

   /* Normal, successful return */
   RETURN;

} /* End function rtphf */

/******************************************************************************/
static int return_phasefile (char *path )
/* 
Purpose:
-------
     Routine return_phasefile changes the effective phasefile.  Its argument is
used as the path of the file to be used as the new phasefile.
     This routine closes the phasefile currently in use, opens the new phase-
file, and causes its contents to be displayed.


David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
path  :  (I  )  (Pointer to) full path of file to use as new phasefile.
*/
{ /* Begin function return_phasefile */
   /*
   Local Variables:
   ---------------
   f1		    :  UNIX file descriptor for saved phasefile.
   f2		    :  UNIX file descriptor for default phasefile.
   dummy_phasehead  :  A datafilehead struct.  Used to call D_open, its value is
		       not used.
   err		    :  Error indicator returned by function.
   */
   struct datafilehead dummy_phasehead;
   int f1, f2, err;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Open the input file to copy */
   if ( (f1=open(path,O_RDONLY)) == -1 )
   {  
      return(ERROR);
   }
   /* Delete the existing phasefile */
   if ( (err=D_remove(D_PHASFILE)) != 0 )
      if ( err != D_NOTOPEN )
      {  Werrprintf ( "Could not close existing phasefile" );
	 sleep ( 1 );
	 D_error ( err );
	 ABORT;
      }
   /* Open the new phasefile */
   build_default_phasefile();
   if ( (f2=open(default_phasefile,O_WRONLY)) == -1 )
   {  Werrprintf ( "Could not open %s for writing", default_phasefile );
      ABORT;
   }
   /* Copy files to create new phasefile.  Also closes files.  */
   if ( filecopy ( f1, f2 ) != 0 )
   {  unlink ( default_phasefile );
      ABORT;
   }

   /* Open the new phasefile */
   if ( (err=D_open(D_PHASFILE,default_phasefile,&dummy_phasehead)) != 0 )
   {  Werrprintf ( "Could not open file %s", default_phasefile );
      sleep ( 1 );
      if ( err == D_NOTOPEN )
         Werrprintf ( "Unix I/O error %d", errno );
      else
         D_error ( err );
      ABORT;  
   }

   /* Normal successful return */
   RETURN;

} /* End function return_phasefile */

/******************************************************************************/
int cp_phasefile_datafile( )
/* 
Purpose:
-------
     Routine copies phasefile to datafile.

Arguments:
---------
 void
*/

{ 
   struct datafilehead dummy_datahead;
   int f1, f2, err;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Open the input file to copy */
   if ( (f1=open(default_phasefile,O_RDONLY)) == -1 )
   {  Werrprintf ( "Could not open %s for reading", default_phasefile );
      ABORT;
   }
   /* Delete the existing phasefile */
   if ( (err=D_remove(D_DATAFILE)) != 0 )
      if ( err != D_NOTOPEN )
      {  Werrprintf ( "Could not close existing phasefile" );
	 sleep ( 1 );
	 D_error ( err );
	 ABORT;
      }
   /* Open the new phasefile */
   build_default_datafile();
   if ( (f2=open(default_datafile,O_WRONLY)) == -1 )
   {  Werrprintf ( "Could not open %s for writing", default_datafile );
      ABORT;
   }
   /* Copy files to create new datafile.  Also closes files.  */
   if ( filecopy ( f1, f2 ) != 0 )
   {  unlink ( default_datafile );
      ABORT;
   }

   /* Open the new phasefile */
   if ( (err=D_open(D_DATAFILE,default_datafile,&dummy_datahead)) != 0 )
   {  Werrprintf ( "Could not open file %s", default_datafile );
      sleep ( 1 );
      if ( err == D_NOTOPEN )
         Werrprintf ( "Unix I/O error %d", errno );
      else
         D_error ( err );
      ABORT;  
   }

   /* Normal successful return */
   RETURN;

}

/******************************************************************************/
static void build_default_phasefile()
/* 
Purpose:
-------
     Routine build_default_phasefile shall construct the full path to the 
default phasefile (curexpdir)/datdir/phasefile, placing the result in variable
default_phasefile.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:  None.
---------
*/
{ /* Begin function build_default_phasefile */
   /*
   Local Variables:  None.
   ---------------
   */
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Start with curexpdir */
   strcpy ( default_phasefile, curexpdir );

   /* Put in the rest of the path */
   D_getfilepath(D_PHASFILE, default_phasefile, curexpdir);

} /* End function build_default_phasefile */

/******************************************************************************/
static void build_default_datafile()
/* 
Purpose:
-------
     Routine build_default_datafile shall construct the full path to the 
default datafile (curexpdir)/datdir/data, placing the result in variable
default_datafile.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:  None.
---------
*/
{ /* Begin function build_default_datafile */
   /*
   Local Variables:  None.
   ---------------
   */
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Start with curexpdir */
   strcpy ( default_datafile, curexpdir );

   /* Put in the rest of the path */
   D_getfilepath(D_DATAFILE, default_datafile, curexpdir);

} /* End function build_default_phasefile */

/******************************************************************************/

static int filecopy (int f1, int f2 )
/* 
Purpose:
-------
     Routine filecopy shall copy one file onto another.  The files are accessed
through a pair of (open) UNIX file descriptors.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
f1  :  (I  )  File descriptor for the (open) input file.
f2  :  (I  )  File descriptor for the (open) output file.
*/
{ /* Begin function filecopy */
   /*
   Local Variables:
   ---------------
   buffer  :  Buffer for holding data during copy.
   n       :  Number of bytes read at a time.
   totbyt  :  Total number of bytes read.
   */
#define BUFSIZE 512
   char buffer[BUFSIZE];
   int n;
   long totbyt;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Do the file copy.  This code is stolen from the K&R example "cp." */
   totbyt = 0;
   while ( (n=read(f1,buffer,BUFSIZE)) > 0 )
   {  if ( write(f2,buffer,n) != n )
      {  Werrprintf ( "Unix I/O error number %d writing to file", errno );
         close ( f1 );
         close ( f2 );
         ABORT;
      }
      totbyt += n;
   }

   /* Close the input file */
   if ( close(f1) == -1 )
   {  Werrprintf ( "Could not close file, unix error %d", errno );
      close ( f2 );
      ABORT;
   }
   if ( close(f2) == -1 )
   {  Werrprintf ( "Could not close file, unix error %d", errno );
      ABORT;
   }
   /* If the phasefile was mostly empty, generate an error message */
   if ( totbyt <= sizeof(struct datafilehead) )
   {  Werrprintf ( "Error:  phasefile contained no data" );
      ABORT;
   }
   /* Normal successful return */
   RETURN;

} /* End function filecopy */

int readheader(int argc, char *argv[], int retc, char *retv[])
{
   int in_file;
   dfilehead datafilehead;
   int blks, traces, nps;

   if (argc<2)
   {
      Werrprintf("%s: Name of file was not passed", argv[0]);
      ABORT;
   }
   if ((in_file=open(argv[1],O_RDONLY)) == -1)
   {
      Werrprintf("%s: Cannot open file %s", argv[0], argv[1]);
      ABORT;
   }
   if (read(in_file, &datafilehead, sizeof(dfilehead)) <= 0)
   {
      Werrprintf ("%s: Error reading from file %s", argv[0], argv[1]);
      close(in_file);
      ABORT;
   }
   close(in_file);
   nps = ntohl(datafilehead.np);
   blks = ntohl(datafilehead.nblocks);
   traces = ntohl(datafilehead.ntraces);
   
   if (retc)
   {
      retv[0] = intString(nps);
      if (retc >= 2)
      {
         retv[1] = intString(blks);
         if (retc >= 3)
            retv[2] = intString(traces);
      }
   }
   else
   {
      Winfoprintf("%s: points:%d  blocks:%d  traces:%d",argv[0],nps,blks,traces);
   }

   RETURN;
}
