/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/* swrite                                               */
/* converts an svf file into a textfile and a datafile  */
/********************************************************/

/* ###		D A T A    T R A N S F E R   F O R M A T
		========================================

  In order to allow easy access to Varian NMR data on XL and VXR spectro-
meters and VAX/MicroVAX computers Varian will support the translation of
data stored in the Varian internal format into a simple standard format, 
which is described below in detail. This will allow access to free
induction decays in a simple and easy to understand way.

  The following two commands will be available on XL and VXR spectrometers
and as part of the VAX VNMR software:

  SWRITE(filename)	write data from current experiment into a file
			'filename' in the standard format.

  SREAD(filename)	read data from the standard format file 'filename'
			into the current experiment.


  A standard format file will actually consist of two individual files.
The first file is a text file containing information about the sample,
the experiment and the parameters used in the experiment. It also
contains details about the storage format of the data. The second file
is a binary file containing the actual data.

  In the VMS and UNIX environment, these two files are stored with the
following suffix:

  filename/stext	standard text file
  filename/sdata	standard data file

  In the Concurrent Pascal environment, the two files are stored as two
entries in the directory file 'filename'. Therefore, they can be accessed
with the syntax:

  FILENAME.STEXT and FILENAME.SDATA


1. Text file:
-------------

  The text file consists of the following three parts. First, a set of
essential parameters is listed. These parameters are required to process
the data and to properly define the format of the data file. Second, a
text describing the sample and/or experiment, and any other important
information. Third, a complete list of all Varian parameters used to
perform the experiment. Part two and three of this file are facultative.
The SREAD command will read files originating from non Varian software
and/or spectrometers, which contain only the essential set of parameters
in part one or any subset of part three.

  The syntax of the text file is as follows:

    parameter_name=parameter_value			(essential parameters)
    parameter_name=parameter_value
    #TEXT:
    any text describing the sample and experiment.
    #PARAMETERS:
    parameter_name"qfield"=parameter_value		(complete set of
    parameter_name"qfield"=parameter_value		 parameters)
	qfield is optional
	qfield consists of not more than 6 characters in quotes.	
	it may contain
		parameter specifications similar to those shown by DLIST
			if parameters are from a vxr system
		"y" or "n" showing if parameter is active
		the name of another parameter with the same characteristics
		"array#" indicating the parameter is an array
   
    if parameter is an array 
    parameter_name "array#"=				(#=1, 2, or 3);
    parameter_name "qfield"=parameter_value(1)	(param q-field in
    ...							quotes,optional)
    parameter_name          =parameter_value(n)
    paramater_value of DATE and RSTDIR are written out as 3 two digit integers
	with leading zeros in place of blanks.  In RSTDIR they are f.i2 ,f.i3
	and f.i4.
    entries in RSTDIR are written as pairs of numbers:
		integral_frequency=integral_value
    #ACQDAT:
    completed_transients=scale
    ...
    completed_transients=scale

In the text file, capital and small letters are treated as equivalent.
A new line is required after each individual parameter. 
The "=" is used to divide each line into 2 fields.  It has no other meaning.

2. List of the essential parameters:
------------------------------------

  DATATYPE		1 = Complex 1-D free induction decay(s)
			2 = Phased 1-D spectrum (spectra)
			3 = Complex 2-D free induction decays
			4 = Phased 2-D spectra

  SYSTEM		vxr, vnmr, ---

  DIM1			size of first dimension (see below)
  DIM2			size of second dimension (see below)
  DIM3			size of third dimension (see below)

  NP			number of points per fid 
			(twice the number of complex points in fid's)

  SW			spectral width in Hz

  SFRQ			spectrometer observe frequency in MHz

  BYTES			number of bytes per word in data file
			(2 for Varian single precision, 4 for Varian double
			precision data, 4 for all processed Varian spectra)

  ORDER			1   for most significant byte first
			    (all standard files generated on XL/VXR)
			0   for least significant byte first
			    (all standard files generated on VAX/MicroVAX)

  ACQTYP		0   for simultaneous acquisition of real and imaginary
			    points (Varian and GE data)
			1   for alternate acquisition of real and imaginary
			    points (Some or all of Bruker data)

  for 2D data only:

  SW1			spectral width in F1 direction in Hz
  DFRQ			decoupler observe frequency in MHz
  ABSTYP		0   for standard type 2-D experiments
			1   for pure absorption 2-D, using the hypercomplex
			    method
			2   for pure absorption 2-D, using the TPPI method
  

  DIM1,DIM2 and DIM3 is used as follows:

   - for standard 1-D data, one fid:
       DIM1=1, DIM2=1, DIM3=1
   - for single array 1-D data, such as T1 data set
       DIM1=number of fid's, DIM2=1, DIM3=1
   - for simple 2-D data sets:
      DIM1=number of fid's, DIM2=1, DIM3=1
   - for absorption 2-D data sets, measured with the method described by
     States, Haberkorn and Ruben (hypercomplex method):
      DIM1=2, DIM2=number of fid pairs.

3. Data file:
-------------

  The data file is always a binary file containing integer data. The number
of bytes per word and the order of the bytes is defined in the essential
parameter BYTES and ORDER. The number of points per fid or spectrum is always
defined in the essential parameter NP.  If more than 1 fid or spectrum is
contained in a data set, the data file contains all the individual fid's or
spectra immediately following each other. In case of multiple arrays,
DIM1 describes the fastest changing index.  
### */

/* ###  SWRITE.C Program Description:
	-----------------------------
 
It creates a directory for the formatted files.
It opens the file 'stext' in the output directory.
It starts by writing out the the essential parameters by finding
	them in the current experiment 'curpar'.
It writes out the file divider #TEXT:
It writes out TEXT with the file divider #PARAMETERS: at the end.
It writes out all the parameters from 'curpar'.
	'array' is dropped but first 3 names are saved
	and later used to set up "array#" in quotes for the arrayed 
	parameter.
	If a parameter can be turned on and off a "y" for active or "n"
	for not active is placed after the parameter name. 
	The parameter value follows an equals sign.  If the value is
	a floating point number is converted to a text string first.
	The date written out as 3 two digit integers
	with leading zeros in place of blanks.

It writes out the file divider #ACQDAT:

The output file 'sdata' is now opened.  ACQDAT values are written out
	to 'stext' in the same loop which writes out the fid data
	to 'sdata'.  The ctcount and scale from the fid header
	are written out as string pairs seperated by an equal sign.
	The fid data file is extended with enough zero's to make its
	length a multiple of the vxr pagelength(256).
The files 'stext' and 'sdata' are closed.
### */

#include "vnmrsys.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "data.h"
#include "group.h"
#include "init2d.h"
#include "symtab.h"
#include "tools.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"

#define BUFSIZE     1024	/* maximum size of text to be passed back */
#define MAXTEXTLINE 80
#define PAGELENGTH  512
#define OLD_EOM     25
/* parameter protection definitions using a bit field */
/* if the bit is set, the comment is true */
#define P_ACT	2	/* bit 1  - cannot change active/not active status */

extern int      debug1;
extern int      Dflag;
extern char    *W_getInput(char *prompt, char *s, int n);
extern void     Wturnoff_buttons();

static char     arrayname[3][9];
static int      dim[3];
static FILE    *fr;
static FILE    *fdata;
static vInfo    info;

static int checkarray(char arrn[]);
static void copytodisk2(symbol **root, FILE *stream);

/****************************/
static int putbytes(short *buf, int nn, int blockcount)
/****************************/
/* write to file fd nn bytes at end of file */
{ int x;

  x=fwrite(buf,sizeof(char),nn,fdata);
  if (3 <= Dflag)
     if (blockcount == 0) 
        fprintf(stderr,"putbytes: just wrote %d bytes \n",x);
  if (x==nn) return 1;
  else 
    perror("convert:");
  return(0);
}

/**************************/
static void setdims(int arraynum)
/**************************/
/* set the global varible dim giving the dimensions of the current experinment*/
{
    char na[9];
    int  i;
    int  ni2d;
    double value;

    for (i=0; i<3; ++i) 
    {   dim[i] = 1;
	arrayname[i][0] = '\0';
    }
    if (P_getreal(CURRENT,"ni",&value,1) == 0)
	ni2d = (int) (value+0.01);
    else
	ni2d=0;
    if (ni2d == 1) ni2d=0;

    if (ni2d)
    {   dim[0] = ni2d;
	Wscrprintf("ni = %d\n", ni2d);
	P_getstring(CURRENT,"array",na,1,8);
	if (strcmp(na,"phase") == 0)
	{   
	    dim[0] = 2;
	    dim[1] = ni2d ; /* was div 2 */
	    strcpy(arrayname[0],na);
	}
    }
    else
	for (i=0; i < arraynum; ++i) 
	{
	    P_getstring(CURRENT,"array",na,i+1,8);
	    strcpy(arrayname[i],na);
	    P_getVarInfo(CURRENT,na,&info);
	    dim[i] = info.size;
	}
 }

/**************************/
static void savereal(int tree, char *name)
/**************************/
/* write name and valuess of a real variable to STEXT */
{
   int    pres;
   double value;

   pres = P_getreal(tree,name,&value,1);
   fprintf(fr,"%s      = %s\n",name,realString(value));
}

/**************************/
static int saveesspar(char *stextpath)
/**************************/
/* get essential parameters from current experiment and write them to STEXT */
{
    char dpflag[4];
    int datatype;
    int pres;
    double value;

    fr = NULL;
    if ((fr = fopen(stextpath,"w")) == NULL)
    {   Werrprintf("unable to open file \"%s\"",stextpath);
	return(1);
    }  

    if (P_getstring(CURRENT,"dp",dpflag,1,1))
    {   Werrprintf("dp not found");
	return(1);
    }
    if (dpflag[0] == 'y') datatype=2;
    else datatype=1; 
    fprintf(fr,"datatype= %d\n",datatype);
    fprintf(fr,"system  = vnmr\n");

    P_getVarInfo(CURRENT,"array",&info);
    if (info.size > 3.01)
    {   Werrprintf("only 3 arrays allowed in transferred data");
	return(1);
    }
    setdims(info.size);
    fprintf(fr,"dim1    = %d\ndim2    = %d\ndim3    = %d\n",dim[0],dim[1],dim[2]);
    /* if (strcmp(arrayname[0],"phase") == 0) dim[1] *= 2; */

    savereal(CURRENT,"np");
    savereal(CURRENT,"sw");
    savereal(CURRENT,"sfrq");

    fprintf(fr,"bytes   = 2  \n");
    fprintf(fr,"order   = 1  \n");
    fprintf(fr,"acqtyp  = 0  \n");

    if ( (pres = P_getreal(CURRENT,"sw1",&value,1)) == 0)
	fprintf(fr,"sw1     = %s\n", realString(value));
    else
	fprintf(fr,"sw1     =      \n");

    if ( (pres = P_getreal(CURRENT,"dfrq",&value,1)) == 0)
	fprintf(fr,"dfrq    = %s\n", realString(value));
    else
	fprintf(fr,"dfrq    =      \n");

    if ( (pres = P_getreal(CURRENT,"ni",&value,1)) == 0)
	if (value>1.5) fprintf(fr,"abstyp  = 0  \n");
	else           fprintf(fr,"abstyp  =    \n");
    else               fprintf(fr,"abstyp  =    \n");
    
    fprintf(fr,"#TEXT:\n");
    return(0);
}


/**************************/
static void writetext(char *s, int max)
/**************************/
/* reads TEXT from current experiment and writes to #TEXT section of STEXT */
{
  FILE *textfile;
  char tfilepath[MAXPATHL];
  char tline[MAXTEXTLINE+1];
  int i,c,j;

  strcpy(tfilepath,curexpdir);
  strcat(tfilepath,"/text");
  i = 0;
  j = 0;
  if ( (textfile=fopen(tfilepath,"r")) )
    { c = getc(textfile);
      if (c == OLD_EOM) c=' ';
      while (c != EOF)
	{ if (j<max-1)
	    s[j++] = c;
	  if (c == '\n')
	    { tline[i] = '\0';
	      fprintf(fr,"%s\n",tline);
	      Wscrprintf("%s\n",tline);
	      i = 0;
	    }
	  else
	    { if (i>=MAXTEXTLINE)
		{ tline[i] = '\0';
		  fprintf(fr,"%s\n",tline);
		  Wscrprintf("%s\n",tline);
		  i = 0;
		}
	      tline[i++] = c;
	    }
	  c = getc(textfile);
	  if (c == OLD_EOM) c=' ';
	}
      fprintf(fr,"\n");
      fclose(textfile);
      s[j] = 0;
      fprintf(fr,"#PARAMETERS:\n");
    }
  else Werrprintf("Unable to open text file");
}

/*******************************/
static void fixtxt(char txt[])
/*******************************/
/* shorten a filepath to a single pascal name */
{
    int cntr;
    int i;
    int j = 0;

    cntr = strlen(txt) - 1;
    while (cntr>0 && txt[cntr] != '/') 
    {
	cntr -= 1;
	j    += 1;
    }
    if (txt[cntr] == '/') 
    {   for (i=0; i<j; i++) txt[i] = txt[cntr+i+1];
	txt[j] = '\0';
    }
}


/*******************************/
static void fixdate(char txt[])
/*******************************/
/* insure that value of date is written accoring to the format */
{
    int cntr;
    int i = 0;
    int j = 0;
    int k = 0;

    cntr = strlen(txt);
    while (j<cntr) 
    {
	if (txt[j] != '-')
	{
	    if (txt[j] == ' ') txt[i] = '0';
	    else txt[i] = txt[j];
	    ++i;
	    ++k;
	}
	else
	{   if (k == 1)
	    {   txt[i] = txt[i-1];
		txt[i-1] = '0';
		++i;
	    }
	    k = 0;
	}
	++j;
    }
    txt[i] = '\0';
}

/*------------------------------------------------------------------------------
|
|	RshowRvals
|
|	This function writes all Rvals to STEXT.
|
+-----------------------------------------------------------------------------*/

static void RshowRvals(FILE *f, char *na, int b, char ch, Rval *r)
{   if (r)
    {	int   i;
	char  txt[MAXPATHL];

	if (strlen(na) > 8) na[6] = '\000';
	i = 1;
	while (r)
	{ 
	    switch (b)
	    { case T_UNDEF:	fprintf(f,"%-8s     =     \n",na);
				break;
	      case T_REAL:	if (ch == ' ')
				    fprintf(f,"%-8s     =%g\n",na,r->v.r);
				else
				    fprintf(f,"%-8s\"%c\"  =%g\n",na,ch,r->v.r);
				break;
	      case T_STRING:    strcpy(txt,r->v.s);
				if (strcmp(na,"file"   )==0 ||
				    strcmp(na,"exppath")==0 ||
				    strcmp(na,"goid"   )==0) fixtxt(txt);
				else if (strcmp(na,"date")==0) fixdate(txt);
				if (ch == ' ')
				    fprintf(f,"%-8s     = %s \n",na,txt);
				else
				    fprintf(f,"%-8s\"%c\"  = %s \n",na,ch,txt);
				break;
	      default:		fprintf(f,"unknown (=%d)\n",b);
				break;
	    }
	    i += 1;
	    r  = r->next;
	}
    }
    else
	fprintf(f," \n");
}

/*------------------------------------------------------------------------------
|
|	SP_save
|
|	This function gets all variables from a tree by calling copytodisk2.
|
+-----------------------------------------------------------------------------*/

static int SP_save(int tree)
{
    symbol **root;
    /* uses global static fr */

    if ( (root = getTreeRoot(getRoot(tree))) )
    {
	copytodisk2(root,fr);
	fprintf(fr,"#ACQDAT:\n");
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	copytodisk2
|
|	This function copies all variables and its values from a tree
|	to a disk file.
|
+-----------------------------------------------------------------------------*/

static void copytodisk2(symbol **root, FILE *stream)
{  symbol  *p;
   varInfo *v;
   char     ch = ' ';
   char     arrn[9];
   char     na[9];
   int      type;
 
   if ( (p=(*root)) )   /* check if there is at least something in from tree */
   {  if (p->name)
      {  if (3 <= Dflag)
	    fprintf(stderr,"copytodisk:  working on var \"%s\"\n",p->name);
	 if ( (v=(varInfo *)(p->val)) )
	 {  type  = v->T.basicType;
	    na[0] = '\0';
	    strcpy(na,p->name);
	    if (type!=T_STRING  || v->maxVal < 16.0)
	    {  if ((v->prot & P_ACT) != P_ACT)
	       {  if (v->active == 0)
		     ch='n';
		  else
		     ch='y';
	       }
	       if (v->T.size<=1)
		  RshowRvals(fr,na,type,ch,v->R);
	       else 
	       {  arrn[0] = '\0';
		  strcpy(arrn,p->name);
		  Wscrprintf("Arrayed: %s , size=%d\n",p->name,v->T.size);
		  if (checkarray(arrn))
		  {  fprintf(fr,"%s\"%s\"=\n",na,arrn);
		     RshowRvals(fr,na,type,ch,v->R);
		  }
	       }
	    }
	 }  
      }
      if (p->left)
	 copytodisk2(&(p->left),stream);
      if (p->right)
	 copytodisk2(&(p->right),stream);
   }
}

/**************************/
static int checkarray(char arrn[])
/**************************/
/* determines if a varible is arrayed */
{
    int r=1;

    if      (strcmp(arrayname[0],arrn) == 0) strcpy(arrn,"array1");
    else if (strcmp(arrayname[1],arrn) == 0) strcpy(arrn,"array2");
    else if (strcmp(arrayname[2],arrn) == 0) strcpy(arrn,"array3");
    else r=0;
    return(r);
}

/**************************/
static int saveacqdat(char *sdatapath)
/**************************/
/* copies all fids, without dataheads, from current experiment to SDATA */
{
  char   outfidpath[MAXPATHL];
  char   ch;
  int    blocknumber = -1;
  int    e;
  int    fill;
  int    i;
  int    i1,i2,i3;
  int    nf;
  int    r;
  double rnf;
  struct datafilehead dhd;
  struct datapointers bpntr;

  r=P_getreal(PROCESSED,"nf",&rnf,1);
  if (r==0)
    nf = (int) (rnf + 0.001);
  else
    nf = 1;

  fdata = NULL;
  if ((fdata = fopen(sdatapath,"w")) == NULL)
  {   Werrprintf("unable to open file \"%s\"",sdatapath);
      return 1 ;
  }  

  Wscrprintf("DATA BLOCK HEADER:\n");

  if ( (e = D_gethead(D_USERFILE,&dhd)) )
  {   if (e==D_NOTOPEN)
      { 
          if ( (e = D_getfilepath(D_USERFILE, outfidpath, curexpdir)) )
          {
             D_error(e);
             return 1;
          }

	  e = D_open(D_USERFILE,outfidpath,&dhd); /* open the file */
      }
      if (e) 
      {   D_error(e); return 1;
      }
  }
  /* fill: number of bytes needed to extend a fid to the end of a VXR sector */
  fill = dhd.tbytes % PAGELENGTH;
  if (fill) fill = PAGELENGTH - fill;
  if (3 <= Dflag)
     Wscrprintf("  fill =   %d\n",fill);

  for (i3=1; i3<=dim[2]; i3++)
    for (i2=1; i2<=dim[1]; i2++)
      for (i1=1; i1<=dim[0]; i1++)
	{ blocknumber += 1;
	  if (interuption)
	  {   D_close(D_USERFILE); return 1;
	  }
 
	  if ( (e = D_getbuf(D_USERFILE,1,blocknumber,&bpntr)) )
	  {   D_error(e); return 1;
	  }
	  if (blocknumber == 0)
	  {    Wscrprintf("  First Block");
	       Wscrprintf("  ctcount   =%8d,   scale    =%8d\n",
		  bpntr.head->ctcount, bpntr.head->scale);
	  }
	  else if (blocknumber == 63)
	       Wscrprintf("\n  Block =  %d",blocknumber+1);
	  else if (blocknumber % 64 == 63)
	       Wscrprintf("  %d",blocknumber+1);
	  fprintf(fr,"%9d  =%5d\n", bpntr.head->ctcount, bpntr.head->scale);

	  r = (putbytes((short *)bpntr.data, dhd.tbytes*nf,blocknumber)==0);
	  for (i=0; i<fill; ++i) putc('\000',fdata);

	  if (i1==dim[0] && i2 == dim[1] && i3 == dim[2])
	      Wscrprintf("\nLast Block: %d Copied\n",blocknumber+1);

	  if ( (r=D_release(D_USERFILE,blocknumber)) )
	  {   D_error(r);
	      D_close(D_USERFILE);
	      return 1;
	  }
	}
   ch = EOF;
   fprintf(fr,"%c",ch);
   return 0;
}

/**************************/
int swrite(int argc, char *argv[], int retc, char *retv[])
/**************************/
{ char datafilepath[MAXPATHL],filepath[MAXPATHL],path[MAXPATHL];
  char *name;
  char answer[16];
  char s[BUFSIZE];
  int r,fidflag,tree;
  int svf_update;

  (void) retc;
  (void) retv;
  Wturnoff_buttons();
  D_allrelease();

  if (strcmp(argv[0],"swrite")==0) fidflag = 0;
  else fidflag=0;
  if (argc<2)
    { W_getInput("File name (enter name and <return>)? ",filepath,MAXPATHL-1);
      name = filepath;
      if (strlen(name)==0)
	{ Werrprintf("No file name given, command aborted");
	  return 1;
	}
    }
  else if (argc!=2)
    { Werrprintf("usage - %s(filename)",argv[0]);
      return 1;
    }
  else
    name = argv[1];
  if (strlen(name) >= (MAXPATHL-32))
    { Werrprintf("file path too long");
      return 1;
    }
  strcpy(filepath,name);
  disp_status("swrite  ");
  Wscrprintf("\n");
  svf_update = 0;
#ifdef SIS
  if (mkdir(filepath,0755))
#else 
  if (mkdir(filepath,0777))
#endif 
    { if (errno==EEXIST)
	{ W_getInput("File exists, overwrite (enter y or n <return>)? "
	    ,answer,15);
	  if ((strcmp(answer,"y")==0) || (strcmp(answer,"yes")==0))
	    svf_update = 1;
	  else
	    { Werrprintf("File '%s' exists, cannot update",filepath);
	      disp_status("        ");
	      return 1;
	    }
	}
      else
	{ Werrprintf("cannot create file %s",filepath);
	  disp_status("        ");
	  return 1;
	}
    }
  strcpy(path,filepath);
  strcat(path,"/stext");
  strcpy(datafilepath,filepath);
  strcat(datafilepath,"/sdata");
  if (fidflag)
    { tree = PROCESSED;
      P_copygroup(CURRENT,PROCESSED,G_DISPLAY);
      P_copygroup(CURRENT,PROCESSED,G_PROCESSING);
    }
  else tree = CURRENT;

  D_close(D_USERFILE);
  if ( (r=saveesspar(path) != 0) )
    { Werrprintf("problem storing parameters in %s",path);
      disp_status("        ");
      if (fr != NULL)
        fclose(fr);
      return(1);
    }

  writetext(s,BUFSIZE);
  SP_save(tree);
  saveacqdat(datafilepath);
  fclose(fr);
  if (fdata != NULL)
     fclose(fdata);

  if (!fidflag)
    { disp_status("        ");
      return 0;
    }

  disp_status("        ");
  return 0;
}
