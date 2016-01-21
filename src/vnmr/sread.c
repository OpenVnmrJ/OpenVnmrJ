/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* sread.c */
/* A program to convert data files from standard form to VXR5000 format.   */
/* sread('directorypath','<parlibpath>') 
   The directory contains stext and sdata. 
   parlibpath (optional) is path of standard parameters. */
/* On the vxr, SWRITE transforms a data file into a text file and a data file.*/
/* Format described in the source for SWRITE. */

/* ### SREAD program description:
       --------------------------
	This program uses input made by SWRITE on vxr/xl/gemini,
	vnmr(VAX), or VXR5000.

File 'stext' opened.
Output is currently placed in the current experiment.
Uses standard parameter files with default to parlib.s2pul.
Reads esssential parms. Places DIM1, DIM2, DIM3, ABSTYP and
	SYSTEM in global varibles.
Copies TEXT from 'stext' to 'text' of current experiment.
It then processes all Varian parameters:
	It looks for them in the output files and if not found creates
	new ones.  If a field in quotes
	is supplied with the parameter, it modifies the properties of
	the standard parameter. The values of standard parameters are
	all replaced by those of the new parameters.

	If a parameter is arrayed, its name is placed in the parameter 
        'array'.

ACQDAT is converted by first assigning memory for acqdat and then
	convert each string pair from ACQDAT into an integer pair
	in memory. File 'stext' is closed and 'sdata' is opened.
SDATA is transferred one fid at a time after the datahead for the
	fid file has been set properly using parameters from 'curpar'.
	In the loop the ctcount and scale from the memory are placed
	in the blockhead and it is written to the fid file of the
	current experiment just before the data from 'sdata'.
### */

#include "vnmrsys.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>   /* contains open constants */
#include <unistd.h>
#include <stdarg.h>
#include "data.h"
#include "group.h"
#include "params.h"
#include "tools.h" 
#include "variables.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"

int parlibpar,parlibfound;		/* moved here from CONVERT.C */

extern int debug1;
extern int interuption;
extern void     Wturnoff_buttons();
extern void clearGraphFunc();

#define CLEARS 12
#define TRUE   1
#define PAGELENGTH 512  /* number of bytes in xl directory          */
#define OLDLENGTH  256  /* number of words in xl directory sector   */
#define OLD_EOM     25  /* end of message character in xl text file */
#define OLD_EOL     10  /* end of line character in xl text file    */
#define MAXARRAY    18  /* maximun number of array elements         */
#define G_ESSPAR     5

#define APSIZE       128000.0  /* maximun number of data points */
#define ZERO         0  /* 1 on VAX, 0 on 68020 */
#define MAXSWK       100.0     /* maximun sweep width in KHz    */
#define MAXFN        524288.0  /* maximun Fourier number        */
#define MAXTIM       8190.0    /* maximun timer word in secs    */
#define MINTIM       1e-7      /* minimun timer word in secs    */
#define digit(c)     (('0' <= c) && (c <= '9'))
#define letter(c)  (((c>='a') && (c<='z')) || (c=='$') || (c=='#') || (c==' '))

static char arraystring[STR128];
static int decplaces = 0,mult;            /* parameter characteristics */
static float max,min,step;            /* parameter characteristics */
         
static char filename[STR128];
static char savet16[STR128];
static char savearg1[STR128];
static char arrayn[3][STR128];
static int arrayflag;
static int end;       /* if end of a text file section is found end = 1 */
static int getnonb2;
static int dim1,dim2,dim3,dima;
static int bruflag = 0;
static int cmxflag = 0;
static int errcount = 0;
static int d2flag;
static int *acodes;
static int vnmr;
static int g_realflag;
static int g_partype;
static int g_egroup;
static int g_active;

static float lbval = 0.0;

static FILE *fr;

/*  Adapted from CONVERT.C; subroutines `dgparm',
    `setdgparms' and `storeparams' use these variables.  */

static double	apsize;
static double	reval;
static double	re2val;
static int set_nodata();
static int findend();
static int convertline(int grident);
static int rstdir();
static void rfband(char t16b[], char name[], int *partype);
static void plotmode(char t16b[], char name[], int *partype);
         
/************************************/
static int SP_creatvar(int tree, char *name, int group)
/************************************/
/* creates a varible if not found in standard parameter file */
{
  if (parlibfound)
    return(0);
  else
    return(P_creatvar(tree,name,group));
}

/**********************************************/
static int SP_setlimits(int tree, char *name, double max, double min, double step)
/**********************************************/
/* sets limits for a variable not found in standard paramater file */
{
  if (parlibfound)
    return(0);
  else
    return(P_setlimits(tree,name,max,min,step));
}

/*************************************/
static int SP_setgroup(int tree, char *name, int group)
/*************************************/
/* sets group for a variable not found in standard paramater file */
{
  if (parlibfound)
    return(0);
  else
    return(P_setgroup(tree,name,group));
}

/***********************************/
static int SP_setprot(int tree, char *name, int prot)
/***********************************/
/* sets protedtion for a variable not found in standard paramater file */
{
  if (parlibfound)
    return(0);
  else
    return(P_setprot(tree,name,prot));
}


/****************************/
static int getbytes(int fd, short *buf, int pos, int nn)
/****************************/
/* read from file fd nn bytes starting at pos pages into buf */
/* used to read fid data int current experiment              */
{ int x;

  lseek(fd,PAGELENGTH*pos,0);
  x=read(fd,buf,(int) nn);
  if (x == nn) return 1;
  else 
    perror("convert:");
  return 0;
}

/****************/
static char GetnextChar()
/****************/
/* reads, sets bit 8, and changes to lower case one character from STEXT file */
{
    char ch;

    ch = getc(fr) & ~128;
    if (ch == '"') getnonb2=0;
    if ( (ch >= 'A' && ch <= 'Z')  && ! cmxflag)
        return(ch + 'a' -'A');
    else 
        return ch;
}

/****************/
static char GetNonBlank()
/****************/
/* reads characters from STEXT until one is not a blank or a double quote */
{ char v;

    do
    { v = GetnextChar();
    } while ((v==' ') || (v=='"'));
    return(v);
}

/****************/
static void noblanks(char s[])
/****************/
/* remove all blanks at end of string */
{ int i;
  i = strlen(s)-1;
  while ((i>=0)&&(s[i]==' '))
    {  s[i]='\000';
       i--;
    }
}

/********************/
static int distext()
/********************/
/* display the vxr text at page sec of file fx, store in current experiment */
{
  char  ch;
  char  tfilepath[MAXPATH];
  char  xlpage[PAGELENGTH];
  FILE *textfile;               /* text file will be created as curexp/text */
  int   i;
  int   j;
  int   savei;

  /* getdata(fx,xlpage,sec,1); */
  end = 0;
  i = 0;
    while ((!end) && (i<2*PAGELENGTH))
    {   ch = getc(fr) & ~128;
        if ( ch == '#') 
            end = findend();
        if (end) 
            ch=' ';
        if (i<PAGELENGTH) 
            xlpage[i] = ch;
        i++;
    }
  
  if ((i<PAGELENGTH) && ((i & 2) == 1))
    { xlpage[i] = ' ';
      i++;
    }

  if (i>PAGELENGTH) i = PAGELENGTH;
  i--; /* was i = PAGELENGTH-1; */
  while ((i>0) && (xlpage[i]==' ')) i--;
  savei =i;
  if (xlpage[i]==OLD_EOM) i--; 
  if (i<(PAGELENGTH-2)) 
    { xlpage[i+2] = '\0';
      xlpage[i+1] = '\n';
    }
  /* convert backslash and eol to newline */
  for (j=0;j<=i;j++)
    {
      if ((xlpage[j]=='\\') || (xlpage[j]==OLD_EOL)) xlpage[j] = '\n';
    }
  /* display it */
  i = savei;
  do {savei--;} 
  while (savei>0 && (xlpage[savei] == ' ' || xlpage[savei] == '\n'));
  for (i=0; i<=savei; i++) Wscrprintf("%c",xlpage[i]); 
  Wscrprintf("\n"); 
  /* open "curexpdir/text"  writeonly, create if does not exist */
  strcpy(tfilepath,curexpdir);
  strcat(tfilepath,"/text");
  if ( (textfile=fopen(tfilepath,"w+")) )
    { fprintf(textfile,"%s",xlpage);
      fclose(textfile);
    }
  else
    { Werrprintf("Unable to open file");
      return(1);
    }  
  return(0);
}

/**************************/
static void checkresult(int res, char *info)
/**************************/
/* check the result of a parameter function and display error, if necessary */
{ if (res)
    if ((res <= -6) || (res >= -2))
      { Wscrprintf("Error %d in parameter routine %s",res,info);
      }
}

/***********************/
static void set_egroup(char egroup)
/***********************/
/* sets max, min, step and mult for a varible */
{
   decplaces=0;
   mult=1;
   if (egroup >='A' && egroup <='Z') egroup = egroup - 'A' + 'a';
   switch (egroup)      /* now set up according to entry group */
   { 
     case 'a': decplaces=1; max=1e5;   min=0;      step=1e-7;mult=1e6;break;
     case 'b': decplaces=3; max=1e5;   min=0;      step=1e-7;         break;
     case 'c': decplaces=1; max=1e5;   min=100;    step=1;            break;
     case 'd': decplaces=1; max=100;   min=0.001;  step=0.001;        break;
     case 'e': decplaces=0; max=1.28e5;min=0;      step=32;   mult=2; break;
     case 'f': decplaces=0; max=49900; min=0;      step=100;          break;
     case 'g': decplaces=1; max=100;   min= -200;  step=0.1;          break;
     case 'h': decplaces=1; max=255;   min= 0;     step=1;            break;
     case 'i': decplaces=0; max=1e5;   min= -1e5;  step=100;          break;
     case 'j': decplaces=1; max=9.9e4; min= -9.9e4;step=0.1;          break;
     case 'k': decplaces=0; max=1e9;   min=1;      step=1;            break;
     case 'l': decplaces=0; max=1e9;   min=0;      step=1;            break;
     case 'm': decplaces=0; max=900;   min=1;      step=1;            break;
     case 'n': decplaces=3; max=1e4;   min=1e-5;   step=1e-7;         break;
     case 'o': decplaces=0; max=6.55e4;min=64;     step= -2;  mult=2; break;
     case 'p': decplaces=1; max=3.2e7; min= -3.2e7;step=0.1;          break;
     case 'q': decplaces=0; max=500;   min= -500;  step=1;            break;
     case 'r': decplaces=0; max=200;   min= -200;  step=1;            break;
     case 's': decplaces=0; max=1e9;   min= -1e9;  step=1;            break;
     case 't': decplaces=0; max=10;    min=1;      step=1;            break;
     case 'u': decplaces=1; max=3.2e7; min=0;      step=0.1;          break;
     case 'v': decplaces=1; max=3.6e3; min= -3.6e3;step=0.1;          break;
     case 'w': decplaces=3; max=100;   min=0;      step=0.001;        break;
     case 'x': decplaces=0; max=39;    min=0;      step=1;            break;
     case 'y': decplaces=4; max=1e18;  min= -1e18; step=1;            break;
     case 'z': decplaces=0; max=63;    min=0;      step=1;            break;
     case '#': decplaces=0; max=1e9;   min=1e-6;   step=1e-6;         break;
     case '0': decplaces=0; max=0; min=0; step=0; break;
     case '1': decplaces=2; max=1; min=0; step=0; break;
     case '2': decplaces=0; max=2; min=0; step=0; break;
     case '3': decplaces=2; max=3; min=0; step=0; break;
     case '4': decplaces=0; max=4; min=0; step=0; break;
     case '5': decplaces=2; max=0; min=0; step=0; break;
     case '6': decplaces=0; max=0; min=0; step=0; break;
     case '7': decplaces=2; max=0; min=0; step=0; break;
     case '8': decplaces=0; max=0; min=0; step=0; break;
     case '9': decplaces=2; max=0; min=0; step=0; break;
     case ' ': decplaces=2; max=0; min=0; step=0; break;
     default:  decplaces=1; max=1e5; min=0; step=1e-7;      mult=1e6;break;
   }
}

/***********************************/
static void esetstring(char *name, int count, ...)
/***********************************/
{
    int pres;
    va_list vargs;
    char *s;

    va_start(vargs, count);
    if (count>=1)
    {
        s = va_arg(vargs, char *);
        pres=P_Esetstring(CURRENT,name,s,1); /* possible values */
        checkresult(pres,"Esetstring");
    }
    if (count>=2)
    {
        s = va_arg(vargs, char *);
        pres=P_Esetstring(CURRENT,name,s,2); /* possible values */
        checkresult(pres,"Esetstring");
    }
    if (count>=3)
    {
        s = va_arg(vargs, char *);
        pres=P_Esetstring(CURRENT,name,s,3); /* possible values */
        checkresult(pres,"Esetstring");
    }
    if (count>=4)
    {
        s = va_arg(vargs, char *);
        pres=P_Esetstring(CURRENT,name,s,4); /* possible values */
        checkresult(pres,"Esetstring");
    }
    if (count>=5)
    {
        s = va_arg(vargs, char *);
        pres=P_Esetstring(CURRENT,name,s,5); /* possible values */
        checkresult(pres,"Esetstring");
    }
    va_end(vargs);
}

/***********************************/
static void string_var(char *name,char *name1, int element)
/***********************************/
/* creates a string varible (if it does not exist) and sets its value */
{ int pres;

  /* if (debug1) Wscrprintf(" ...%s..., max=%g",name,max); */
  /* create the variable */
  if (!parlibfound)
  {
      pres=SP_creatvar(CURRENT,name,ST_STRING);
      checkresult(pres,"creatvar");
  }
  /* set the limits */
  pres=SP_setlimits(CURRENT,name,max,0.0,0.0);
  checkresult(pres,"setlimits");
  /* set the value */
  /* if (max!=6 || name1[1] != '\0') */
  {
      pres=P_setstring(CURRENT,name,name1,element);
      checkresult(pres, name);
  }
}

/*************************/
static void par1_set(char *name,char *name1, int element)
/*************************/
/* sets SEQFIL and PSLABEL */
{ int pres;

  /* if (debug1) Wscrprintf(" string "); */
  max = 8;
  noblanks(name1);
  string_var(name,name1,element);
  pres=SP_setgroup(CURRENT,name,G_ACQUISITION);
  checkresult(pres,"setgroup"); 
}

/******************************/
static void par18_set(char *name, char *name1, int element)
/******************************/
/* used to set pname type parameters from the VXR */
{
  noblanks(name1);
  max = 16;
  string_var(name,name1,element); 
}

/******************************/
static void par19_set(char *name, char q[], int element)
/******************************/
/* sets DATE */
{

  max=9;
  if (!digit(q[0]))
  {
      string_var(name,q,element);
      if (debug1) Wscrprintf(" string='%s'\n ",q);
  }
  else 
  {
      char name1[16];
      sprintf(name1,"%c%c-%c%c-%c%c",q[0],q[1],q[2],q[3],q[4],q[5]);
      string_var(name,name1,element);
      if (debug1) Wscrprintf(" string='%s'\n ",name1);
  }
}

/************************/
static void par_set(char *name, char name1[], int element)
/************************/
/* sets a char type parameter from VXR */
{
  if (!parlibfound && (max<1||max>4)) 
	{ max=4;
  	  name1[(int)max]='\000';
	}
  noblanks(name1);
  if (debug1) Wscrprintf(" par_set name = %s string=%s\n",name,name1);
  string_var(name,name1,element); 
}

/********************************/
static void real_set(char *name, char *sreal, char egroup, int element)
/********************************/
/* sets a real type parameter from VXR */
{
  int pres;
  float rval;

  rval = stringReal(sreal);
  if (debug1) Wscrprintf(" real=%f\n",mult*rval);
  /* first create the variable */
  if (!parlibfound)
  {
  if      (egroup=='a')  pres=SP_creatvar(CURRENT,name,ST_PULSE);
  else if (egroup=='b')  pres=SP_creatvar(CURRENT,name,ST_DELAY);
  else if (egroup=='i')  pres=SP_creatvar(CURRENT,name,ST_FREQUENCY);
  else if (egroup=='j')  pres=SP_creatvar(CURRENT,name,ST_FREQUENCY);
  else if (decplaces==0) pres=SP_creatvar(CURRENT,name,ST_INTEGER);
  else                   pres=SP_creatvar(CURRENT,name,ST_REAL);
  checkresult(pres,"creatvar");
  }
  /* set the limits */
  if (strcmp(name,"vs")==0 || strcmp(name,"is")==0 ||strcmp(name,"fb")==0)
    pres= P_setlimits(CURRENT,name,max,min,step);
  else 
    pres=SP_setlimits(CURRENT,name,max,min,step);
  checkresult(pres,"setlimits");
  /* set the value */
  pres=P_setreal(CURRENT,name,(double)mult*rval,element);
  checkresult(pres,"setreal");
  if (pres<0)  Wscrprintf(" '%s' error=%d\n",name,pres);
}

/************************************************************/
static int qualifiers(char t6[], int *realflag, int *partype, int *dgroup,
                      char *active, char *egroup)
/************************************************************/
/* sets variable qualifiers based on field in double quotes */
{
   char t2[4];
   int  q3;
   int  i;

   if (t6[0] == ' ' || t6[0] == '\0' || 
	strcmp(t6,"y") == 0 || strcmp(t6,"n") == 0)
   {   *realflag = 1;
       *partype  = 22;
       *dgroup   = 0;
       *active   = 'y';
       if (strcmp(t6,"n")==0 || t6[2]=='n') *active = 'n';
       *egroup   = 'd';
       return 1;
   }
   else 
       *active   = t6[2];   /* active flag */
   t2[0]=t6[0];t2[1]=t6[1]; t2[2]='\0';
   *partype  = (int)stringReal(t2);
   if (*partype == 18 || *partype == 19 || *partype == 4)
   {
       q3      = 0;
       *egroup = '4';
   }
   else
   {
       for (i=0; i<2; ++i) t2[i]=t6[i+4];
       q3      = (int)stringReal(t2);
       *egroup = t6[3];    /* entry group */
   }
   *realflag = q3 & 32; /* real or string? */
   *dgroup   = q3 & 7;  /* xl display group */
   *active   = t6[2];   /* active flag */
   if (*active<32) *active=' ';         /* some params have bad char */
   return 1;
}

/***************************/
static int getstr(char key[], int *end, char ch)
/***************************/
/* reads half of a line from STEXT, see definition of line in FORMAT */
{
  int i       = 0;
  int getnonb = 0;
  char v;

  key[0]='\0';
  getnonb2= 1;
  do{
      if ((i==0) || ((getnonb) && (getnonb2))) 
           v=GetNonBlank(); 
      else v=GetnextChar();
      if ((v=='"') && !cmxflag) v=' ';
      if ((v==' ') && (ch=='=')) getnonb=1;
      if (v=='#') *end=findend();
      if ((!*end) && (i<STR128-1))
      {
        if (v==OLD_EOL || (v=='\n') || (v=='=') )
          key[i]='\000';
        else
          key[i]=v;
      }
      ++i;
    } while ((v!=OLD_EOL) && (v!='\n') && (v!='=') && (!*end) && (i<222));
  if (cmxflag)
  {
    key[i] = '\0';
  }
  else
  {
  while (i < 16) 
    { key[i]='\000';
      ++i;
    }
  key[16]='\000';
  }

  if (key[0] != '\000')
  {
   if (debug1) Wscrprintf("\nKEY=%s\n",key);

  if (debug1) if (i>16)
     Wscrprintf("i=%d, String='%s'\n",i,key);
  if (v!=ch && !*end && errcount < 20)
     {
	 if (strlen(key) != 0) 
         {   if (ch == '=' ) 
	          Wscrprintf("String '%s' does not end with '=' \n",key);
             else Wscrprintf("Bad String '%s' \n", key);
	     errcount++;
	 }
	 if (errcount >= 20) return 1;
         getstr(key,end,ch);
     }
     
  } /* key[0] != '\000' */
  return 0;
}

/******************/
static int findend()
/******************/
/* checks for end of a section in STEXT, see format definition */
{
/* char *t1; t1="se   \0"; */
  char ch;
  char *t;
  int  end;
  int  cntr;
  int  limit;
  ch=GetnextChar();
  end=0;
  if ((ch=='a') || (ch=='p') || (ch=='t') ||
      (ch=='A') || (ch=='P') || (ch=='T') ) end=1;
  else
    { end = 0;
      if (debug1) Wscrprintf("findend char =%c\n",ch);
      ungetc(ch,fr);
      return(end);
    }
  if (end){
      switch(ch) {
        case 'a': {
                   t = "acqdat:        \0"; 
                   limit=8;
                   break;}
        case 'A': {
                   t = "ACQDAT:        \0"; 
                   limit=8;
                   break;}
        case 'p': {t = "parameters:\n  \0"; 
                   limit=12;
                   break;}
        case 'P': {t = "PARAMETERS:\n  \0"; 
                   limit=12;
                   break;}
        case 't': {t = "text:\n        \0"; 
                   limit=6;
                   break;}
        case 'T': {t = "TEXT:\n        \0"; 
                   limit=6;
                   break;}
        default : {t = "text:\n        \0"; 
                   limit=6;
                   break;}
        }
        cntr=0;
        while ((ch!=':') && (cntr<limit) && (t[cntr]==ch)) {
            ++cntr;
            ch=GetnextChar();
          }
      if (ch == ':') end=1;
       ch=GetnextChar();
    }
  return(end);
}

/*************************************/
static void getCmxName(char t16[], char na[], char t6[])
/*************************************/
/* gets variable name and field in quotes the input string */
{
  int  cntr,j,done;
  char ch;
  if ((t16[0]!='\n'))
    {
      cntr = -1;
      do
        { ++cntr;
          ch=t16[cntr];
          na[cntr] = ch;
        } while ((cntr<STR128) && (ch!=' ') &&
                 (ch != '=') && (ch != '"') && (ch != '\0') );
      na[cntr] = '\0';                			   /* STM:  terminate na */
      if (debug1) Wscrprintf("na=%s\n",na);
      while ((cntr<STR128-1) && ((t16[cntr]==' ') || (t16[cntr]=='"')) ) ++cntr;
      j=0;
      done=0;
      do
        { ch=t16[cntr];
          if (ch == '=' || ch == ' ' || ( !letter(ch) && !digit(ch) )) done++;
          if (!done)
          {
             t6[j]=ch;
             ++j;
          }
          ++cntr;
        } while ((!done) && (cntr<STR128-1) && (j<STR64));
      t6[j]='\0';
      if (debug1) Wscrprintf("T6=%s\n",t6);
    }
}

/*************************************/
static void getname(char t16[], char na[], char t6[])
/*************************************/
/* gets variable name and field in quotes the input string */
{
  int  cntr,j,done;
  int  err;
  char ch;
 
  if (cmxflag)
  {
     getCmxName(t16, na, t6);
     return;
  }
  for (cntr=0; cntr<8; ++cntr) na[cntr]='\0';
  for (cntr=0; cntr<6;  ++cntr) t6[cntr]='\0';
  err=0;
  if ((t16[0]!='\n'))
    {
      if (!letter(t16[0])) ++err;
      cntr = -1;
      do
        { ++cntr;
          ch=t16[cntr];
          na[cntr] = ch;
          if ((!digit(ch)) && (!letter(ch)) && (ch!=' ')) err *= err;
        } while ((cntr<8) && (ch!=' '));
      na[cntr] = '\0';                			   /* STM:  terminate na */
      while  ((t16[cntr]==' ') && (cntr<15)) ++cntr;
      while (((t16[cntr]==' ') || (t16[cntr]=='"')) && (cntr<15)) ++cntr;
      j=0;
      done=0;
      do
        { ch=t16[cntr];
          if (ch == '=' || ch == ' ' || ( !letter(ch) && !digit(ch) )) done++;
          if (!done) t6[j]=ch;
          ++cntr;
          ++j;
        } while ((!done) && (cntr<16) && (j<6));
      t6[j]='\0';
      if (debug1) Wscrprintf("T6=%s\n",t6);
    }
}
    
/*********************************************/
static int array_set(char *oldname, int grident)
/*********************************************/
/* sets up an array reads all its values from STEXT */
{
  char name[STR128];
  char t6[STR64];
  char t16b[STR128];
  int  done;
  int  pindex;
  int  pres;

  arrayflag=0;
  convertline(grident);
  arrayflag=1;

  done = 0;
  pindex = 1;
  while (!done)
  {
      getstr(savet16,&end,'=');
      getname(savet16,name,t6);
      noblanks(name);
      done = strcmp(oldname,name);
      if (!done)
      {
        getstr(t16b,&end,'\n');
        pindex++;
        if (g_partype==18)
          par18_set(name,t16b,pindex);
        else if (g_partype==19)
          par19_set(name,t16b,pindex);
        else if (g_realflag)
          real_set(name,t16b,g_egroup,pindex);
        else 
          par_set(name,t16b,pindex);
        if (pindex == 1 && g_partype>9)
    	  {
            if (g_active=='N') pres=P_setactive(CURRENT,name,ACT_OFF);
            else               pres=P_setactive(CURRENT,name,ACT_ON);
            checkresult(pres,"setactive");
	    pres=SP_setgroup(CURRENT,name,G_ACQUISITION);
            checkresult(pres,"setgroup array");
	  }
      }
  }
  return(0);
}

/***************************************************************/
static int convertline(int grident)
/***************************************************************/
/* line converter from STEXT standard format to VXR5000 variables */
/* handles special cases where VXR5000 is different from VXR4000  */
  
{
  char  active;
  char  egroup;
  char  name1[STR128];
  char  name[STR128];
  char  stdname[STR128];
  char  t16[STR128];
  char  t16b[STR128];
  char  t6[STR64];
  int dgroup;
  int i;
  int pindex;
  int partype = 1;
  int pres;
  int protection;
  int realflag = 0;
  int foundtype = 0;
  int copyflag = 0;
  int rval;
  vInfo dummy;
  
  for (i=0; i<STR128; i++)
    t16[i]='\0';
  if ( arrayflag) 
    { strcpy(t16,savet16);
      arrayflag = 0;
    }
  else
    {
      getstr(t16,&end,'=');
      strcpy(savet16,t16);
    }
  if (end) 
    { 
      if (debug1) Wscrprintf("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
      return 1;
    }
  for (i=0; i<STR128; ++i) name[i]= '\0';
  for (i=0; i<STR64; ++i) t6[i]= '\0';
  getname(t16,name,t6);
  noblanks(name);
  if (debug1) Wscrprintf("name=%s t6=%s\n",name,t6);

  if (parlibpar)
      {
      parlibfound = (P_getVarInfo(CURRENT,name,&dummy)==0);
      if (parlibfound) foundtype = dummy.basicType;
      }
  else
      parlibfound = 0;

  if ( (strncmp(t6,"array",5) == 0) && (sscanf(t6,"array%d", &pindex) == 1) )
  {   arrayflag=1;
      getstr(t16b,&end,'\n');
      if (debug1) Wscrprintf("array identifier, name=%s\n",name);

      if (array_set(name,grident))
      {  
          Wscrprintf("ARRAY IDENTIFIER, NAME=%s\n",name);
	  return 1;
      }
      strcpy(arrayn[pindex-1],name);
      sprintf(arraystring,"%s %d %s",arraystring,pindex,name);

      return 1;
  }

  if (end) return 1;
  getstr(t16b,&end,'\n');
  if (name[1] == '\0' || strcmp(name,"syn") == 0 ) return(1);

  /* omit certain parameters */
           if      (strcmp(name,"mftl"  )==0) partype = 0;
           else if (strcmp(name,"array ")==0) partype = 0;
           else if (strcmp(name,"domain")==0) partype = 0;
           else if (strcmp(name,"pt"    )==0) partype = 0;
           else if (strcmp(name,"heg"   )==0) partype = 0;
           else if (strcmp(name,"hzcm"  )==0) partype = 0;
           else if (strcmp(name,"lflag1")==0) partype = 0;
           else if (strcmp(name,"lflag2")==0) partype = 0;
           else if (strcmp(name,"tonorm")==0) partype = 0;
           else if (strcmp(name,"donorm")==0) partype = 0;
           else if (strcmp(name,"syt"   )==0) partype = 0;
           else if (strcmp(name,"synt"  )==0) partype = 0;
           else if (strcmp(name,"dsynt" )==0) partype = 0;
           else if (strcmp(name,"adcflg")==0) partype = 0;
           else if (strcmp(name,"lh"    )==0) partype = 0;
           else if (strcmp(name,"imax"  )==0) partype = 0;
           else if (strcmp(name,"mnps"  )==0) partype = 0;
           else if (strcmp(name,"imav"  )==0) partype = 0;
           else if (strcmp(name,"reav"  )==0) partype = 0;
           else if (strcmp(name,"scale" )==0) partype = 0;
           else if (strcmp(name,"se"    )==0) partype = 0;
           else if (strcmp(name,"com$st")==0) partype = 0;
           else if (strcmp(name,"se2"   )==0) partype = 0;
           else if (strcmp(name,"re"    )==0)
		{   partype = 0;
		    if (!vnmr && t6[2]=='y')
                    {
                       lbval = -0.318/stringReal(t16b);
                    }
		}
           else if (strcmp(name,"re2"   )==0) partype = 0;
           else if (strcmp(name,"cd"    )==0) partype = 0;
           else if (strcmp(name,"cd2"   )==0) partype = 0;
           else if (strcmp(name,"ccd"   )==0) partype = 0;
           else if (strcmp(name,"ccd2"  )==0) partype = 0;
           else if (strcmp(name,"npoint")==0) partype = 0;
           else if (strcmp(name,"qp"    )==0) partype = 0;
           else if (strcmp(name,"datatype")==0) partype = 0;
           else if (strcmp(name,"bytes" )==0) partype = 0;
           else if (strcmp(name,"order" )==0) partype = 0;
           else if (strcmp(name,"dim1") == 0) 
		{   dim1 = (int)stringReal(t16b);
	            partype = 0;
		}
           else if (strcmp(name,"dim2") == 0) 
		{   dim2 = (int)stringReal(t16b);
	            partype = 0;
		}
           else if (strcmp(name,"dim3") == 0) 
		{   dim3 = (int)stringReal(t16b);
	            partype = 0;
		}
           else if (strcmp(name,"system") == 0) 
		{   if (t16b[0] == 'v' || t16b[0] == 'j' || t16b[3] == '5') 
		    {
		       vnmr =1;
          	       Wscrprintf("VXR5000 DATA  \n");
		    }
		    else
		       if (t16b[0] == 'b')
		       {
			  vnmr = 1;
			  bruflag  = 1;
          	          Wscrprintf("BRUKER DATA  \n");
		       }
		    partype = 0;
                }
           else if (strcmp(name,"file") == 0) 
		{   strcpy(t16b,savearg1);
                }
           else if (strcmp(name,"abstyp")==0) 
		{  
		    if (digit(t16b[0]))
		    {   d2flag  = 1;
		        strcpy(name,"ni");
		    }
		    else partype = 0;
		}
           else if (strcmp(name,"sw1" ) == 0 ||
                    strcmp(name,"dfrq") == 0) 
		{   if (t16b[0] == '\0') partype=0;
	        }
               

           /* change names of certain variables */
           else if (strcmp(name,"sp2"   )==0) strcpy(name,"sp1");
           else if (strcmp(name,"wp2"   )==0) strcpy(name,"wp1");
           else if (strcmp(name,"sw2"   )==0) strcpy(name,"sw1");
           else if (strcmp(name,"rfl2"  )==0) strcpy(name,"rfl1");
           else if (strcmp(name,"rfp2"  )==0) strcpy(name,"rfp1");
           else if (strcmp(name,"fn2"   )==0) strcpy(name,"fn1");
           else if (strcmp(name,"lb2"   )==0) strcpy(name,"lb1");
           else if (strcmp(name,"af"    )==0) strcpy(name,"gf");
           else if (strcmp(name,"af2"   )==0) strcpy(name,"gf1");
           else if (strcmp(name,"solvnt")==0) strcpy(name,"solvent");
           else if (strcmp(name,"arraye")==0) strcpy(name,"arrayelemts");
           else if (strcmp(name,"pslabl")==0) strcpy(name,"pslabel");
           else if (strcmp(name,"exppat")==0) strcpy(name,"exppath");
           else if (strcmp(name,"arrayd")==0) strcpy(name,"arraydim");
           else if (strcmp(name,"priori")==0) strcpy(name,"priority");
           else if (strcmp(name,"to"    )==0) strcpy(name,"tof");
           else if (strcmp(name,"do"    )==0) strcpy(name,"dof");
           else if (strcmp(name,"opos"  )==0) strcpy(name,"rfband");
           else if (strcmp(name,"array1")==0) strcpy(name,"lifrq");
           else if (strcmp(name,"array2")==0) strcpy(name,"liamp");
           else if (strcmp(name,"array3")==0) strcpy(name,"llfrq");
           else if (strcmp(name,"rstdir")==0){strcpy(name,"llamp"); rstdir();}
           else if (strcmp(name,"mxcnst")==0) strcpy(name,"intmod");
           else if (strcmp(name,"lcr"   )==0) strcpy(name,"delta");

	   if (letter(t16b[0]) && (t6[1] == ' ' || t6[1] == '\0'))
	   {
	     t6[2] = t6[0];
	     t6[0] = '1';
	     t6[1] = '8';
	   }
           if (partype)
	       qualifiers(t6,&realflag,&partype,&dgroup,&active,&egroup); 

 	   if (!partype)
           {    if (debug1) Wscrprintf("omitted parameter: %s\n",name);
		return 0;
	   }


  if (strcmp(name,"file") == 0 || strcmp(name,"solvnt")== 0 || 
	   strcmp(name,"goid")  == 0 || strcmp(name,"exppath")== 0 ||
	   strcmp(name,"user")  == 0 || strcmp(name,"probe") == 0 ||
	   strcmp(name,"method")== 0 || strcmp(name,"wexp")  == 0 ||
	   strcmp(name,"wbs")   == 0 || strcmp(name,"wnt")   == 0 ||
	   strcmp(name,"werr")  == 0 || strcmp(name,"n1")    == 0 ||
	   strcmp(name,"n2")    == 0 || strcmp(name,"n3")    == 0 ||
 	   strcmp(name,"n4")    == 0)  partype=18;

  if (!parlibfound)
  {
     if (t16b[0] == '\000') partype = 0;
     else
     {
     if      (strcmp(name,"ni")   == 0) strcpy(stdname,"np");
     else if (strcmp(name,"dfrq") == 0) strcpy(stdname,"sfrq");
     else if (strcmp(name,"sw1")  == 0) strcpy(stdname,"sw");
     else if (strcmp(name,"goid") == 0) strcpy(stdname,"file");
     else if (strcmp(name,"user") == 0) strcpy(stdname,"file");
     else if (strcmp(name,"exppath") == 0) strcpy(stdname,"file");
     else if (partype == 18) strcpy(stdname,"n1");
     else if (name[0] == 'p') strcpy(stdname,"pw");
     else if (name[0] == 'd') strcpy(stdname,"d1");
     else if (letter(t16b[0])) strcpy(stdname,"n2");
     else strcpy(stdname,"filter");
     parlibfound = (P_getVarInfo(CURRENT,stdname,&dummy)==0);
     copyflag = 0;
     if (parlibfound) 
 	{
	copyflag = (P_copyvar(CURRENT,CURRENT,stdname,name) >= 0);
	max = dummy.maxVal;
        min = dummy.minVal;
	step = dummy.step;
        if (dummy.basicType != T_STRING)  decplaces = 10;
        foundtype = dummy.basicType;
	}
     }
  }
   
  if (parlibfound)
  {   if (dummy.basicType == T_STRING)  
      {   realflag = 0;
	  egroup = '4';
	  if (partype != 18)
          {
	      if (strcmp(name,"date") == 0) partype=19;
	      else partype = 20;
          }
      }
      else 
      {   realflag = 1;
	  partype  = 22;
	  egroup   = 'l';
      }
  }

  /* egroup of certain parameters  */
           if      (strcmp(name,"sfrq"  )==0) egroup = 'T';
           else if (strcmp(name,"dfrq"  )==0) egroup = 'T';
           else if (strcmp(name,"vtwait")==0) egroup = 'B';
           else if (strcmp(name,"vs"    )==0) egroup = '#';
           else if (strcmp(name,"is"    )==0) egroup = '#';
           else if (strcmp(name,"fb"    )==0) egroup = 'f';
           else if (strcmp(name,"th"    )==0) egroup = 'K';
           else if (strcmp(name,"vf"    )==0) egroup = 'S';
           else if (strcmp(name,"wp"    )==0) egroup = 'N';
           else if (strcmp(name,"wp1"   )==0) egroup = 'N';
           else if (strcmp(name,"rfl"   )==0) egroup = 'P';
           else if (strcmp(name,"rfp"   )==0) egroup = 'P';
           else if (strcmp(name,"lvl"   )==0) egroup = 'P';
           else if (strcmp(name,"tlt"   )==0) egroup = 'P';
           else if (strcmp(name,"cr"    )==0) egroup = 'P';
           else if (strcmp(name,"delta" )==0) egroup = 'P';
           else if (strcmp(name,"rfl1"  )==0) egroup = 'P';
           else if (strcmp(name,"rfp1"  )==0) egroup = 'P';
           else if (strcmp(name,"ho"    )==0) egroup = 'R';
           else if (strcmp(name,"llfrq" )==0) egroup = 'P';
           else if (strcmp(name,"llamp" )==0) egroup = 'T';
           else if (strcmp(name,"lifrq" )==0) egroup = 'P';
           else if (strcmp(name,"liamp" )==0) egroup = 'T';
           else if (strcmp(name,"aig"   )==0) egroup = '2';
           else if (strcmp(name,"dcg"   )==0) egroup = '3';

  if (!copyflag) set_egroup(egroup);
  if (vnmr) mult = 1.0;
  if ( (strcmp(name,"pslabl")==0) || (strcmp(name,"pslabel")==0)
        || (strcmp(name,"seqfil")==0))
  {   /* treat PSLABEL and SEQFIL as exceptions */
      strcpy(name1,t16b);
      if (strcmp(name,"seqfil")==0) strcpy(name,"seqfil");
      else strcpy(name,"pslabel");
      partype = 1;    /* to mark these special params */
      if (debug1) Wscrprintf(" seqfil/pslabl partype=%d",partype);
  }
  if (debug1) Wscrprintf("%s, ",name);
  if (debug1) Wscrprintf("partype=%d\n ",partype);
  /*if (debug1) Wscrprintf(" dgroup%1d, active=%1c ",dgroup,active);*/

  if ((strcmp(name,"tn")==0) || (strcmp(name,"dn")==0))
  {      pres=SP_creatvar(CURRENT,name,ST_STRING);
         checkresult(pres,"creatvar");
         pres=SP_setlimits(CURRENT,name,4.0,0.0,0.0);
         checkresult(pres,"setlimits");
  	 if ((digit(t16b[0])) || (t16b[0] == '-')) realflag=1;
  	 else realflag=0;
         if (!realflag) 
	     for (i=0; i<16; ++i)
         	if (t16b[i] >= 'a' && t16b[i] <= 'z') t16b[i] += 'A' - 'a';
         if (realflag)	
         {   rval = stringReal(t16b);
             if      ((rval>=1.0) && (rval<=1.999))
                 pres=P_setstring(CURRENT,name,"H1",0);
             else if ((rval>=2.0) && (rval<=2.999))
                 pres=P_setstring(CURRENT,name,"D2",0);
	     else if (rval >=3.0 && rval<=3.999)
	         pres=P_setstring(CURRENT,name,"H3",0);
	     else if (rval >=13.0 && rval<=13.999)
	         pres=P_setstring(CURRENT,name,"C13",0);
             else if ((rval>=14.0) && (rval<=14.999))
                 pres=P_setstring(CURRENT,name,"N14",0);
             else if ((rval>=15.0) && (rval<=15.999))
                 pres=P_setstring(CURRENT,name,"N15",0);
	     else if (rval >=17.0 && rval<=17.999)
	         pres=P_setstring(CURRENT,name,"O17",0);
             else if ((rval>=18.99) && (rval<19.999))
                 pres=P_setstring(CURRENT,name,"F19",0);
	     else if (rval >=23.0 && rval<=23.999)
	         pres=P_setstring(CURRENT,name,"NA23",0);
	     else if (rval >=31.0 && rval<=31.999)
	         pres=P_setstring(CURRENT,name,"P31",0);
             else
                 pres=P_setstring(CURRENT,name,"???",0);
             checkresult(pres,"setstring");
             pres=SP_setgroup(CURRENT,name,G_ACQUISITION);
             checkresult(pres,"setgroup");
             partype = 0; /* nothing further to do */
         }
  }
  else
             if ((strcmp(name,"lifrq")==0) || (strcmp(name,"llfrq")==0) ||
                 (strcmp(name,"liamp")==0) || (strcmp(name,"llamp")==0))
             {
               dgroup   = 1;
               if (t16b[0] != '\0') real_set(name,t16b,egroup,0);
               pres=P_setactive(CURRENT,name,ACT_OFF);
               checkresult(pres,"setactive");
               pres=SP_setgroup(CURRENT,name,G_DISPLAY);
               checkresult(pres,"setgroup");
               partype = 0;
             }
  else
           if (strcmp(name,"intmod")==0)
             { pres=SP_creatvar(CURRENT,name,ST_STRING);
               checkresult(pres,"creatvar");
               pres=SP_setlimits(CURRENT,name,8.0,0.0,0.0);
               checkresult(pres,"setlimits");
	   /*  esetstring(name,3,INT_OFF,INT_PARTIAL,INT_FULL); */
	       esetstring(name,3,"off","partial","full");
               pres=P_setstring(CURRENT,name,INT_OFF,0);
               checkresult(pres,"setstring");
               pres=P_setactive(CURRENT,name,ACT_ON);
               checkresult(pres,"setactive");
               pres=SP_setgroup(CURRENT,name,G_DISPLAY);
               checkresult(pres,"setgroup");
               partype = 0; /* nothing further to do */
             }
  else
            if (d2flag && strcmp(name,"axis")==0)
	       {if (t16b[1] == ' ') t16b[1] = t16b[0];
	       }
  else
            if (strcmp(name,"pltmod")==0)
	       {  plotmode(t16b,name,&partype);
		  if (!partype) return 1;
	       } 
  else
            if (strcmp(name,"rfband")==0)
		  rfband(t16b,name,&partype);
  
  if (t16b[0] != '\0' || parlibfound)
  {
  		if ((digit(t16b[0]) || (t16b[0] == '-')) && (partype!=18) &&
							    (partype!=19)) 
		   {   realflag=1;
		       if (parlibfound && foundtype != ST_REAL) partype=0;
		   }
  		else 
		    {    realflag=0;
		         if (parlibfound && foundtype == ST_REAL) partype=0;
		    }
  }

  /* display info according to parameter type */
            if (partype==0)
              { if (debug1) Wscrprintf("omitted parameter\n");
	        return(1);
              }
            else if (partype==1)
              par1_set(name,t16b,0);
            else if (partype==18)
              par18_set(name,t16b,0);
            else if (partype==19)
              par19_set(name,t16b,0);
            else if (realflag)
              {  if (t16b[0] != '\0') real_set(name,t16b,egroup,0);
	      }
            else 
              par_set(name,t16b,0);
	    g_partype = partype;
	    g_realflag= realflag;
	    g_egroup  = egroup;
	    g_active  = active;

           if (strcmp(name,"ni") == 0)
           { 
             d2flag = ((float)stringReal(t16b) > 1.5);
           }
           if (strcmp(name,"vp")==0)
             pres=P_setreal(CURRENT,name,(double)mult*0.0,0);

           if ((partype) && (strcmp(name,"dim1")) && (strcmp(name,"dim2")) &&
                            (strcmp(name,"dim3")))
             {
               protection = (active == 'X') ? P_ACT : 0;
               if ((strcmp(name,"at")==0) || (strcmp(name,"np")==0) ||
                   (strcmp(name,"sw")==0))
                 protection |= P_MAC;
               if (((!(partype == 20) && !(partype == 21) &&
                   !(partype == 22))) || (grident != G_ACQUISITION) ||
                   (strcmp(name,"sfrq")==0) || (strcmp(name,"dfrq")==0))
                 protection |= P_ARR;
               if (strcmp(name,"ct")==0)
                 protection |= P_VAL;
               pres = SP_setprot(CURRENT,name,protection);
               checkresult(pres,"setprotection");
           }

           if (partype>9)
             {
               /* set the active flag */
               if (active=='n')      pres=P_setactive(CURRENT,name,ACT_OFF);
               else                  pres=P_setactive(CURRENT,name,ACT_ON);
               checkresult(pres,"setactive");
               /* set the group identifier */
               pres=SP_setgroup(CURRENT,name,grident);
               checkresult(pres,"setgroup");
             }

	    if (!vnmr && strcmp(name,"llamp") == 0) rstdir();
	    return 1;

}

/******************************************/
static int rstdir()
/******************************************/
/* reads RSTDIR values from VXR */
{
    char t16b[STR128];
    int  cntr;

    cntr = 0;
    while (!end && cntr<200)
    {
        getstr(savet16,&end,'='); 
        if (end) 
	    return 1;
        getstr(t16b,&end,'\n');
        ++cntr;
    }
    if (cntr >= 200) Werrprintf("ACQDAT not found"); 
    return 0;
}


/******************************/
/* static set_protect(name,protect)  char name[]; int protect; */
/******************************/
/*{   char    *tree;
    symbol **root;
    varInfo *v;

    tree = "current";
    if ((root = getTreeRoot(tree)) == NULL)
    {	Wscrprintf("bad tree name: CURRENT\n");
	RETURN;
    }
    if ((v = rfindVar(name,root)) == NULL)
    {	Wscrprintf("variable \"%s\" doesn't exist\n",name);
	RETURN;
    }
    v->prot |= protect;
    RETURN;
}*/


/*****************/
static void set_table(char *name, double pos)
/*****************/
{
   vInfo info;
   int pres;
   pres = P_getVarInfo(CURRENT,name,&info);
   if (pres == 0)
   {   pres = P_setprot(CURRENT,name,P_MMS | info.prot);
       pres = P_setlimits(CURRENT,name,pos,pos,pos);
   }
   checkresult(pres,"set_table ");
}

/*****************/
static void fill_table()
/*****************/
{
set_table("sc",1.0);
set_table("wc",2.0);
set_table("sc2",3.0);
set_table("wc2",4.0);
set_table("sw",5.0);
set_table("fb",6.0);
set_table("tof",7.0);
set_table("dof",8.0);
set_table("dhp",9.0);
set_table("dlp",10.0);
set_table("dmf",11.0);
set_table("loc",12.0);
}

/******************************************/
static void disparms(int grident)
/******************************************/
/* reads a VXR type parameter file in standard format 
   and puts variables into the CURRENT tree           */
{
    char name[STR128];
    char name1[STR128];
    int pres;
    int r;
    double cr,delta;

    if (grident == G_ACQUISITION)
    {
        strcpy(name,"array");
        strcpy(name1,"      ");
        par18_set(name,name1,0);
        set_egroup(' ');
        pres=P_setactive(CURRENT,name,ACT_ON);
        pres=SP_setgroup(CURRENT,name,grident);
        checkresult(pres,"setgroup");
    }
    while (!end)
    {
        convertline(grident);
        if (end) if (debug1)
	  Wscrprintf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    }
    if (lbval!=0.0) 
    {   pres=P_setreal(CURRENT,"lb",(double)lbval,1);
        checkresult(pres,"setlb");
    }

    if (!vnmr) /* setcursor */
    {   r=P_getreal(CURRENT,"delta",&delta,1);
        if (r==0)
        {   r=P_getreal(CURRENT,"cr",&cr,1);
            pres=P_setreal(CURRENT,"cr",delta,1);
            checkresult(pres,"setcr");
	    delta = delta - cr;
            pres=P_setreal(CURRENT,"delta",delta,1);
            checkresult(pres,"setdelta");
        }
    }
    P_setprot(CURRENT,"seqfil", P_ARR | P_ACT | P_MAC);
    P_setprot(CURRENT,"pslabel",P_ARR | P_ACT | P_MAC);
    P_setprot(CURRENT,"tn",P_MAC);
    P_setprot(CURRENT,"dn",P_MAC);
    fill_table();
}


/******************************/
/* static syn(t16b,name,partype) */
/******************************/
/*char t16b[],name[]; int *partype;
{
    int pres;

    pres=SP_creatvar(CURRENT,name,ST_STRING);
    checkresult(pres,"creatvar syn");
    pres=SP_setlimits(CURRENT,name,8.0,0.0,0.0);
    checkresult(pres,"setlimits syn");
    pres=P_setactive(CURRENT,name,ACT_ON);
    checkresult(pres,"setactive syn");
    esetstring("syn",4,"yy","yn","ny","nn");
    if (strcmp(t16b,"y")) strcpy(t16b,"yy");
    else if (strcmp(t16b,"n")) strcpy(t16b,"nn");
    pres=P_setstring(CURRENT,name,t16b,0); 
    checkresult(pres,"setstring syn");
    pres = P_setprot(CURRENT,"syn",P_ARR,P_ACT);
    checkresult(pres,"setprotection syn");
    partype = 0;
}*/

/******************************/
static void rfband(char t16b[], char name[], int *partype) 
/******************************/
{
    int pres;

    pres=SP_creatvar(CURRENT,name,ST_STRING);
    checkresult(pres,"creatvar rfband");
    pres=P_setlimits(CURRENT,name,2.0,0.0,0.0);
    checkresult(pres,"setlimits rfband");
    pres=P_setactive(CURRENT,name,ACT_ON);
    checkresult(pres,"setactive rfband");
    esetstring(name,4,"hh","hl","lh","ll");
    if      (t16b[0]=='a') pres=P_setstring(CURRENT,name,"ll",0); 
    else if (t16b[0]=='b') pres=P_setstring(CURRENT,name,"hh",0); 
    else 		   pres=P_setstring(CURRENT,name, t16b,0); 
    checkresult(pres,"setstring rfband");
    pres = P_setprot(CURRENT,name,P_ARR | P_ACT);
    checkresult(pres,"setprotection rfband");
    *partype = 0;
}

/******************************/
static void plotmode(char t16b[], char name[], int *partype) 
/******************************/
{   int pltmod = 0;
    int pres   = 0;

    pres=SP_creatvar(CURRENT,name,ST_STRING);
    checkresult(pres,"creatvar");
    pres=SP_setlimits(CURRENT,name,8.0,0.0,0.0);
    checkresult(pres,"setlimits");
    esetstring("pltmod",5,"off","fixed","full","variable","user");
    pres=P_setactive(CURRENT,name,ACT_ON);
    checkresult(pres,"setactive");
    /* set the group identifier */
    pres=SP_setgroup(CURRENT,name,G_DISPLAY);
    checkresult(pres,"setgroup");
    if (!vnmr || strcmp(t16b,"0") == 0) 
    {  
         *partype = 0;
         if ((digit(t16b[0])) || (t16b[0] == '-')) 
         {
             pltmod  = (int)stringReal(t16b);
         }
         switch (pltmod)
         {
                 case 0:  pres=P_setstring(CURRENT,name,"off",0);      break;
                 case 1:  pres=P_setstring(CURRENT,name,"fixed",0);    break;
                 case 2:  pres=P_setstring(CURRENT,name,"full",0);     break;
                 case 3:  pres=P_setstring(CURRENT,name,"variable",0); break;
                 default: pres=P_setstring(CURRENT,name,"off",0);      break;
         }
         checkresult(pres,"setstring");
    }
}

/******************************/
static int essparms()
/******************************/
/* reads essential parameters from the STEXT standard format and places them
   in the current tree or in global variables */
{
  int  pres;

  dim1 = -1;
  dim2 = -1;
  dim3 = -1;
  end =  0;
  errcount = 0;
  while (!end && errcount < 20)
  {
      convertline(G_ACQUISITION);
  }
  if (errcount >= 20 ) 
  {
      Werrprintf("Problem reading 'stext' file");
      return 1;
  }
  if (end && dim3 == -1)
  {
      Werrprintf("DIM3 not found"); 
      return 1;
  }
  else 
  {   dima = dim1*dim2*dim3;
      if (d2flag)
      {   pres=P_setreal(CURRENT,"ni",(double)dim1,0);
  	  checkresult(pres,"set ni");
      }
  }
  if (debug1) 
      Wscrprintf("fid dimensions = 1:%d, 2:%d, 3:%d\n",dim1,dim2,dim3);
  return 0;
}

/******************************/
static int acqdat()
/******************************/
/* allocates memory for ct and scale of each fid and reads values from STEXT */
{
    char t16b[STR128];
    char v;
    int  i;

    do
    { v = GetnextChar();
    } while (v == ' ' || v == OLD_EOL || v == '\n');
    ungetc(v,fr);
    /* allocate memory for acqdat */
    if (debug1)
        Wscrprintf("acqdat is %d entries\n",dima);
    if (!(acodes = (int *) allocateWithId(sizeof(int)*2*dima,"sread")))
    {   Werrprintf("cannot allocate buffer for acqdat");
        return 1;
    }
    for (i = 0; i<dima; ++i)
    {   getstr(savet16,&end,'=');
        getstr(t16b,&end,'\n');
        acodes[i*2] = (int)stringReal(savet16);
        acodes[i*2+1] = (int)stringReal(t16b);
        if (debug1) if (i == 0) Wscrprintf("ct=%d\n",acodes[0]);
    }
    return 1;

}

/**********************************************/
static int storedata(char *filepath) /* for compressed fids */
/**********************************************/
/* sets up dataheads of current experiment and reads in fid data form SDATA */
{
  char dpflag[4];
  char newfilepath[MAXPATH];
  double rnf;
  int fr;
  int i1,i2,i3,r;
  int bi,nf;
  int dpf,fidlength,fid_sectors;
  short *wptr;
  register int tmp,i;
  struct datafilehead datahead;
  struct datapointers bpntrs;

  r=P_getreal(PROCESSED,"nf",&rnf,1);
  if (r==0)
    nf = (int) (rnf + 0.001);
  else
    nf = 1;

  if ( (r = D_getfilepath(D_USERFILE, newfilepath, curexpdir)) )
  {
     D_error(r);
     release(acodes);
     return 1;
  }

  set_nodata();
  
  r=P_setreal(CURRENT,"arraydim",(float)dima,0);
  checkresult(r,"set arraydim");
  r=P_setreal(PROCESSED,"arraydim",(float)dima,0);
  checkresult(r,"set arraydim");
  
    /* find the fid file */
  r=P_getreal(CURRENT,"np",&rnf,1);
  if (r==0)
    datahead.np = (int) (rnf + 0.001);
  else
  {   Werrprintf("np not found");
      release(acodes); return 1;
  }
  fidlength = datahead.np;
  if (P_getstring(CURRENT,"dp",dpflag,1,3))
  {   Werrprintf("dp not found");
      release(acodes); return 1;
  }
  else dpf = (dpflag[0]=='y');
  if (dpf) fidlength = 2*fidlength;
  fid_sectors = (fidlength +OLDLENGTH -1)/OLDLENGTH;

  if (debug1)
      Wscrprintf("fid length is %d short integers\n",fidlength);

  /* set up datahead */
  datahead.nblocks = dima;
  datahead.ntraces = nf;
  if (nf>1)
      Wscrprintf("multiple fid traces: nf=%d\n",nf);
  if (dpf) datahead.ebytes = 4;
      else datahead.ebytes = 2;
  if (dpf) datahead.status = S_DATA | S_32 | S_COMPLEX;
      else datahead.status = S_DATA | S_COMPLEX;

  datahead.nbheaders = 1;
  datahead.vers_id = VERSION;
  datahead.vers_id += FID_FILE;
  if (bruflag)
     datahead.vers_id |= S_BRU;

  strcat(filepath,"/sdata"); 
  if((fr = open(filepath,O_RDONLY)) == -1)
     {   Werrprintf("unable to open file \"%s\"",filepath);
         perror("convert");
         return 1 ;
     }  
 else if (debug1) Wscrprintf("file opened \"%s\"\n",filepath);

  bi = -1;
  for (i3=1; i3<=dim3; i3++)
    for (i2=1; i2<=dim2; i2++)
      for (i1=1; i1<=dim1; i1++)
        { bi += 1;
          if (interuption)
            { D_close(D_USERFILE); release(acodes);
              close(fr);
              return 1;
            }
 
	  if (bi == 63)
	       Wscrprintf("  Block = %d ",bi+1);
	  else if (bi % 64 == 63)
	       Wscrprintf("  %d",bi+1);
          if ((i1==1)&&(i2==1)&&(i3==1))
          { /* set up datahead */
            datahead.tbytes = datahead.ebytes * datahead.np;
            datahead.bbytes = datahead.tbytes * datahead.ntraces +
                                     sizeof(struct datablockhead);
            if ( (r=D_newhead(D_USERFILE,newfilepath,&datahead)) )
            { D_error(r);
              D_close(D_USERFILE);
              release(acodes);
              close(fr);
              return 1;
            }
          }
              /* now prepare storage for fid in data */
          if ( (r=D_allocbuf(D_USERFILE,bi,&bpntrs)) )
          { D_error(r);
            D_close(D_USERFILE);
            release(acodes);
            close(fr);
            return 1;
          }
          
          bpntrs.head->ctcount = acodes[bi*2];
          bpntrs.head->scale  = (short)acodes[bi*2 + 1];
           
          if (bpntrs.head->ctcount)
          { if (dpf) bpntrs.head->status = S_DATA | S_32 | S_COMPLEX;
            else     bpntrs.head->status = S_DATA | S_COMPLEX;
          }
          else bpntrs.head->status = 0;
          bpntrs.head->index = bi + 1;

          bpntrs.head->mode = 0;
          bpntrs.head->rpval = 0.0;
          bpntrs.head->lpval = 0.0;
          bpntrs.head->lvl   = 0.0;
          bpntrs.head->tlt   = 0.0;
          
          r = (getbytes(fr,(short *)bpntrs.data,(bi)*fid_sectors,
                          datahead.tbytes*nf)==0);
          if (r)
          { Werrprintf("cannot read data file of old fid");
            D_close(D_USERFILE); release(acodes);
            close(fr);
            return 1;
          }
              /* convert double precision to VAX format if neccessary */
          if ((ZERO) && (dpf))
          {
            wptr = (short *)bpntrs.data;
            for (i=0; i<datahead.tbytes/4; i++)
            {
              tmp     = *(wptr+1);
              *(wptr+1) = *wptr;
              *wptr   = tmp;
              wptr += 2;
            }
          }
              /* now mark block as updated and release it */
          if ( (r=D_markupdated(D_USERFILE,bi)) )
          { D_error(r);
            D_close(D_USERFILE); release(acodes);
            close(fr);
            return 1;
          }
          if ( (r=D_release(D_USERFILE,bi)) )
          { D_error(r);
            D_close(D_USERFILE); release(acodes);
            close(fr);
            return 1;
          }
        }
  close(fr);
  D_close(D_USERFILE);
  release(acodes);
  return 1;
}

/******************************************/
static int getseqname(char *seqfil)
/******************************************/
{
    char ch;
    char name[STR128];
    char t16b[STR128];
    char t6[STR64];
    int  cntr = 0;
    int  i    = 0;

    while (!end && cntr<200)
    {
        getstr(savet16,&end,'='); 
        for (i=0; i<16; ++i) name[i]= '\0';
	getname(savet16,name,t6);
        getstr(t16b,&end,'\n');
        if (strncmp(name,"system",6) == 0)
        {  if (t16b[0] =='j' || t16b[3] =='5')
	   return(1);
	} /* return if system=VXR5000 */
        ++cntr;
    }
    if (cntr >= 200)
    {   Werrprintf("end of ESSPAR not found"); 
	return(0);
    }
    
    end = 0;
    while ((!end) && (i<2*PAGELENGTH))
    {   ch = getc(fr) & ~128;
        if ( ch == '#') 
            end = findend();
        i++;
    }
    end = 0;
    getstr(savet16,&end,'=');
    for (i=0; i<16; ++i) name[i]= '\0';
    getname(savet16,name,t6);
    getstr(t16b,&end,'\n');
    noblanks(name);
    if (strcmp(name,"seqfil")==0)
    {	strcpy(seqfil,t16b);
        return(1);
    }
    return(0);
}

/******************************************/
static int getsysname(char sysfil[])
/******************************************/
{
    char name[STR128];
    char t16b[STR128];
    char t6[STR64];
    int  cntr = 0;
    int  i    = 0;

    sysfil[0] = '\0';
    while (!end && cntr<200)
    {
        getstr(savet16,&end,'='); 
        for (i=0; i<16; ++i) name[i]= '\0';
	getname(savet16,name,t6);
        getstr(t16b,&end,'\n');
        if (strncmp(name,"system",6) == 0)
        {  strcpy(sysfil,t16b);
	   return(1);
	}
        ++cntr;
    }
    if (cntr >= 200)
    {   Werrprintf("end of ESSPAR not found"); 
	return(0);
    }
    return(0);
}

/******************************/
static void setarray()
/******************************/
{						      /* Note: array_is_set is equiv. */
    int  array_is_set, pres;			           /* to strlen( arrayn ) > 0 */
    char valofarray[ STR128 ];			/* size must be 3x each row of arrayn */

    valofarray[ 0 ] = '\0';
    array_is_set = 0;

    if (cmxflag)
    {
       if ((dima > 1) && strlen(arraystring))
       {
          int a1, a2, a3;
          int ret;

          char val1[STR64], val2[STR64], val3[STR64];
          ret = sscanf(arraystring," %d %s %d %s %d %s", &a1, val1, &a2, val2, &a3, val3);
          if (ret == 2)
          {
             strcpy(valofarray, val1);
          }
          else if (ret == 4)
          {
             if (a1 == a2)
                sprintf(valofarray,"(%s,%s)", val1, val2);
             else
                sprintf(valofarray,"%s,%s", val1, val2);
          }
          else if (ret == 6)
          {
             if ((a1 == a2) && (a1 == a3))
                sprintf(valofarray,"(%s,%s,%s)", val1, val2, val3);
             else if (a1 == a2)
                sprintf(valofarray,"(%s,%s),%s", val1, val2, val3);
             else if (a1 == a3)
                sprintf(valofarray,"(%s,%s),%s", val1, val3, val2);
             else if (a2 == a3)
                sprintf(valofarray,"%s,(%s,%s)", val1, val2, val3);
             else
                sprintf(valofarray,"%s,%s,%s", val1, val2, val3);
          }
       }
    }
    else
    {
       if (dim1>1 && strcmp(arrayn[0],"array1") != 0 && strlen(arrayn[0]) > 0) {
        strcpy( &valofarray[ 0 ], arrayn[0] );
        array_is_set = 131071;
       }
       if (dim2>1 && strcmp(arrayn[1],"array2") != 0 && strlen(arrayn[1]) > 0) {   
        if (array_is_set)
          strcat( &valofarray[ 0 ], "," );
        strcat( &valofarray[ 0 ], arrayn[1] );
        array_is_set = 131071;
       }
       if (dim3>1 && strcmp(arrayn[2],"array3") != 0 && strlen(arrayn[2]) > 0) {
        if (array_is_set)
          strcat( &valofarray[ 0 ], "," );
        strcat( &valofarray[ 0 ], arrayn[2] );
       }
    }

    pres=P_setstring(CURRENT,"array",&valofarray[0],1);
    checkresult(pres,"setstring array");
}


/**********/
static int set_nodata()
/**********/
/* set data and phasefile to empty */
{
  /* remove any phasefile */
  D_remove(D_PHASFILE);
  /* remove any data file */
  D_remove(D_DATAFILE);
  /* open curexp/acqfil/fid file */
  D_close(D_USERFILE); 
  RETURN;
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
    {   for (i=0; i<j && i<16; i++) txt[i] = txt[cntr+i+1];
	txt[j] = '\0';
    }
}

/*  The following 5 subroutines were taken from CONVERT.C  */

/**********************************************/
static int SP_Esetstring(int tree, char *name, char *strptr, int index)
/**********************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_Esetstring(tree,name,strptr,index));
}

/************************/
static void set_2enum(char *name, char *val1, char *val2)
/************************/
{
  int pres;

  pres=SP_Esetstring(CURRENT,name,val1,1); /* possible values */
  checkresult(pres,"Esetstring");
  pres=SP_Esetstring(CURRENT,name,val2,2);
  checkresult(pres,"Esetstring");
}

/*****************/
void dgparm(char *name, int type, int group, double max,
            double min, double step, int protection)
/*****************/
{
  int pres;
  vInfo dummy;

  if (parlibpar)
    parlibfound = (P_getVarInfo(CURRENT,name,&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_creatvar(CURRENT,name,type);
    checkresult(pres,"creatvar");
    pres=P_setgroup(CURRENT,name,group);
    checkresult(pres,"setgroup");
    pres=P_setlimits(CURRENT,name,max,min,step);
    checkresult(pres,"setlimits");
    pres = P_setprot(CURRENT,name,protection);
    checkresult(pres,"setprotection");
  }
}

/*****************/
void setdgparms(int d2flag)
/*****************/
/* first create new parameters for Unix software, then setup DG templates */
/* save tree finally in current experiment */
/* also create some new parameters: hzmm,trace,array,arraydim */
{
  int pres,nfflag;
  char t[1024];
  double wc,wp,rnf,phase;
  vInfo dummy;

  nfflag = ((P_getreal(CURRENT,"nf",&rnf,1)==0) && (rnf>1.5));

  pres = P_setprot(CURRENT,"seqfil", P_ARR | P_ACT | P_MAC);
  if (!pres) pres = P_setprot(CURRENT,"pslabel",P_ARR | P_ACT | P_MAC);
  if (!pres) pres = P_setprot(CURRENT,"tn",P_MAC);
  if (!pres) pres = P_setprot(CURRENT,"dn",P_MAC);
  checkresult(pres,"setprotection: setdgparms");

  /* new parameters */
/* lp1 */
  if (P_getreal(CURRENT,"lp1",&phase,1)==0)
  {
    if (d2flag)
    {
      P_setreal(CURRENT,"lp",phase,0);
      P_setreal(CURRENT,"lp1",0.0,0);
    }
  }
  else
    dgparm("lp1",ST_REAL,G_DISPLAY,3600.0,-3600.0,0.1,P_ARR | P_ACT);
/* rp1 */
  if (P_getreal(CURRENT,"rp1",&phase,1)==0)
  {
    if (d2flag)
    {
      P_setreal(CURRENT,"rp",phase,0);
      P_setreal(CURRENT,"rp1",0.0,0);
    }
  }
  else
    dgparm("rp1",ST_REAL,G_DISPLAY,3600.0,-3600.0,0.1,P_ARR | P_ACT);
/* cr1 */
  dgparm("cr1",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
/* delta1 */
  dgparm("delta1",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
/* hzmm */
  dgparm("hzmm",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
  if ((P_getreal(CURRENT,"wc",&wc,1)==0)&&(P_getreal(CURRENT,"wp",&wp,1)==0))
    P_setreal(CURRENT,"hzmm",wp/wc,0);
  else Wscrprintf("wc or wp not found\n");
/* trace */
  dgparm("trace",ST_STRING,G_DISPLAY,2.0,0.0,0.0,P_ARR | P_ACT);
  pres=P_setstring(CURRENT,"trace","f1",0);
  checkresult(pres,"setstring");
  set_2enum("trace","f1","f2");
/* array */
  dgparm("array",ST_STRING,G_ACQUISITION,256.0,0.0,0.0,P_ARR);
  pres=P_setstring(CURRENT,"array",arraystring,0);
  checkresult(pres,"setstring");
/* arraydim */
  dgparm("arraydim",ST_INTEGER,G_ACQUISITION,32768.0,1.0,1.0,P_VAL | P_ARR);
  pres=P_setreal(CURRENT,"arraydim",1.0,0);
  checkresult(pres,"setreal");
/* cf */
  dgparm("cf",ST_INTEGER,G_PROCESSING,32767.0,0.0,1.0,P_ARR);
  pres=P_setreal(CURRENT,"cf",1.0,0);
  checkresult(pres,"setreal");
  if (!nfflag)
    { pres=P_setactive(CURRENT,"cf",ACT_OFF);
      checkresult(pres,"setactive");
    }
/* lsfid */
  dgparm("lsfid",ST_INTEGER,G_PROCESSING,apsize / 2.0,0.0,1.0,P_ARR);
  pres=P_setreal(CURRENT,"lsfid",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"lsfid",0);
  checkresult(pres,"setactive");
/* phfid */
  dgparm("phfid",ST_REAL,G_PROCESSING,3600.0,-3600.0,0.1,P_ARR);
  pres=P_setreal(CURRENT,"phfid",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"phfid",0);
  checkresult(pres,"setactive");
/* sb */
  dgparm("sb",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"sb",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"sb",0);
/* sb1 */
  dgparm("sb1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"sb1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"sb1",0);
/* sbs */
  dgparm("sbs",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"sbs",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"sbs",0);
/* sbs1 */
  dgparm("sbs1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"sbs1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"sbs1",0);
/* gfs */
  dgparm("gfs",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"gfs",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"gfs",0);
/* gfs1 */
  dgparm("gfs1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"gfs1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"gfs1",0);
/* awc */
  dgparm("awc",ST_REAL,G_PROCESSING,1.0,-1.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"awc",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"awc",0);
/* awc1 */
  dgparm("awc1",ST_REAL,G_PROCESSING,1.0,-1.0,0.001,P_ARR);
  pres=P_setreal(CURRENT,"awc1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"awc1",0);
/* fpmult */
  dgparm("fpmult",ST_REAL,G_PROCESSING,1.0e9,0.0,0.0,P_ARR);
  pres=P_setreal(CURRENT,"fpmult",1.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(CURRENT,"fpmult",ACT_OFF);
/* priority */
  dgparm("priority",ST_INTEGER,G_ACQUISITION,32768.0,0.0,1.0,P_ARR);
  pres=P_setreal(CURRENT,"priority",5.0,0);
  checkresult(pres,"setreal");
  /* now set aig and dcg to defaults */
  if (parlibpar)
    parlibfound = (P_getVarInfo(CURRENT,"aig",&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_setlimits(CURRENT,"aig",2.0,0.0,0.0);
    checkresult(pres,"setlimits");
    pres=P_setstring(CURRENT,"aig","nm",0);
    checkresult(pres,"setstring");
    set_2enum("aig","nm","ai");
  }
  if (parlibpar)
    parlibfound = (P_getVarInfo(CURRENT,"dcg",&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_setlimits(CURRENT,"dcg",3.0,0.0,0.0);
    checkresult(pres,"setlimits");
    pres=P_setstring(CURRENT,"dcg","cdc",0);
    checkresult(pres,"setstring");
    set_2enum("dcg","dc","cdc");
  }
  if ((reval > 0.0) && (P_getVarInfo(CURRENT,"lb",&dummy)==0))
    if (dummy.active == ACT_OFF)
    {
      pres=P_setreal(CURRENT,"lb",-reval*3.14159,0);
      checkresult(pres,"setreal");
      pres=P_setactive(CURRENT,"lb",ACT_ON);
    }
  if ((re2val > 0.0) && (P_getVarInfo(CURRENT,"lb1",&dummy)==0))
    if (dummy.active == ACT_OFF)
    {
      pres=P_setreal(CURRENT,"lb1",-re2val*3.14159,0);
      checkresult(pres,"setreal");
      pres=P_setactive(CURRENT,"lb1",ACT_ON);
    }
  /* now make dg templates */
  /* create the variable */
  dgparm("dg",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) Wscrprintf("creating dg string\n");
    strcpy(t,"1:ACQUISITION:sfrq:3,tn,at:3,np:0,sw:1,fb:0,bs:0,ss:0,pw:1,p1:1,d1:3,d2:3,tof:1,nt:0,ct:0;");
    strcat(t,"2:SAMPLE:date,solvent,file;");
    strcat(t,"2:DECOUPLING:dn,dof:1,dm,dmm,dmf,dhp:0,dlp:0;");
    strcat(t,"2(ni):2D ACQUISITION:sw1:1,ni:0;");
    strcat(t,"3:PROCESSING:cf(nf):0,lb:2,sb:3,sbs(sb):3,gf:3,gfs(gf):3,awc:3,lsfid:0,phfid:1,fn:0,math,,werr,wexp,wbs,wnt;");
    strcat(t,"4(ni):2D PROCESSING:lb1:2,sb1:3,sbs1(sb1):3,gf1:3,gfs1(gf1):3,awc1:3,fn1:0;");
    strcat(t,"4:FLAGS:il,in,dp,hs;");
    strcat(t,"4:SPECIAL:temp:1;");
    if (debug1) Wscrprintf("%s\n",t);
    /* set the value */
    pres=P_setstring(CURRENT,"dg",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("dg1",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) Wscrprintf("creating dg1 string\n");
    strcpy(t,"1:DISPLAY:sp:1,wp:1,vs:0;");
    strcat(t,"1:REFERENCE:rfl:1,rfp:1,cr:1,delta:1;");
    strcat(t,"1:PHASE:lp:1,rp:1,rp1(ni):1,lp1(ni):1;");
    strcat(t,"2:CHART:sc:0,wc:0,hzmm:2,vp:0,axis,pltmod,,th:0,,ho:2,vo:2,trace:2;");
    strcat(t,"3(ni):2D:sp1:1,wp1:1,sc2:0,wc2:0,rfl1:1,rfp1:1;");
    strcat(t,"3:FLAGS:aig*,dcg*,dmg*;");
    strcat(t,"3:FID:sf:3,wf:3,vf:0;");
    strcat(t,"4:INTEGRAL:intmod,is:2,ins:3,io:0,,lvl:3,tlt:3;");
    if (debug1) Wscrprintf("%s\n",t);
    /* set the value */
    pres=P_setstring(CURRENT,"dg1",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("dgs",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) Wscrprintf("creating dgs string\n");
    strcpy(t,"1:AXIAL SHIMS:z1:0,z1c:0,z2:0,z2c:0,z3:0,z4:0,z5(z5flag):0;");
    strcat(t,"2:NON AXIAL SHIMS:x1:0,y1:0,xz:0,yz:0,xy:0,x2y2:0,x3:0,y3:0,xz2(z5flag):0,yz2(z5flag):0,zxy(z5flag):0,zx2y2(z5flag):0;");
    strcat(t,"3:AUTOMATION:method,wshim,load,,spin:0,gain:0,loc:0;");
    strcat(t,"3:SPECIAL:temp;");
    strcat(t,"4:MACROS:r1,r2,r3,r4,r5,r6,r7,n1,n2,n3,n4;");
    if (debug1) Wscrprintf("%s\n",t);
    /* set the value */
    pres=P_setstring(CURRENT,"dgs",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("ap",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) Wscrprintf("creating ap string\n");
    strcpy(t,"1:SAMPLE:date,solvent,file;");
    strcat(t,"1:ACQUISITION:sfrq:3,tn:2,at:3,np:0,sw:1,fb:0,bs(bs):0,ss(ss):0,pw:1,p1(p1):1,d1:3,d2(d2):3,tof:1,nt:0,ct:0,alock,gain:0;");
    strcat(t,"1:FLAGS:il,in,dp,hs;");
    strcat(t,"1(ni):2D ACQUISITION:sw1:1,ni:0;");
    strcat(t,"1(ni):2D DISPLAY:sp1:1,wp1:1,sc2:0,wc2:0,rfl1:1,rfp1:1;");
    strcat(t,"2:DEC. & VT:dn:2,dof:1,dm,dmm,dmf,dhp(dhp):0,dlp(dlp):0,temp(temp):1;");
    strcat(t,"2:PROCESSING:cf(cf):0,lb(lb):2,sb(sb):3,sbs(sb):3,gf(gf):3,gfs(gf):3,awc(awc):3,lsfid(lsfid):0,phfid(phfid):1,fn:0,math,,werr,wexp,wbs,wnt;");
    strcat(t,"2(ni):2D PROCESSING:lb1(lb1):2,sb1(sb1):3,sbs1(sb1):3,gf1(gf1):3,gfs1(gf1):3,awc1(awc1):3,fn1:0;");
    strcat(t,"2:DISPLAY:sp:1,wp:1,vs:0,sc:0,wc:0,hzmm:2,is:2,rfl:1,rfp:1,th:0,ins:3,aig*,dcg*,dmg*;");
    if (debug1) Wscrprintf("%s\n",t);
    /* set the value */
    pres=P_setstring(CURRENT,"ap",t,0);
    checkresult(pres,"setstring");
  }
}

/******************/
void storeparams()
/******************/
{
  char parampath[MAXPATH];
  int  pres;

  /* save parameters in curexp/curpar file */
  D_getparfilepath(CURRENT, parampath, curexpdir);
  pres=P_save(CURRENT,parampath);
  checkresult(pres,"save");
  /* copy parameters to PROCESSED */
  pres=P_copy(CURRENT,PROCESSED);
  checkresult(pres,"copy");
  /* save parameters in curexp/procpar file */
  D_getparfilepath(PROCESSED, parampath, curexpdir);
  pres=P_save(PROCESSED,parampath);
  checkresult(pres,"save");
}

/******************************/
int sread(int argc, char *argv[], int retc, char *retv[])
/******************************/
/* main of SREAD, see description at top of program */
{    
    char seqfil[STR128], sysfil[STR128];
    char filepath[MAXPATH];
    char stextpath[MAXPATH];
    char stdparpath[MAXPATH];
    int  r = 1;

    (void) retc;
    (void) retv;
    bruflag = 0;
    cmxflag = 0;
    if (argc!=2 && argc!=3) 
    {   Werrprintf("usage - sread('filename')");
        return 1 ;
    }

    D_close(D_USERFILE); 
    Wturnoff_buttons();
    Wshow_text();
    D_allrelease();

    strcpy(filepath,argv[1]);
    strcpy(stextpath,filepath);
    strcat(stextpath,"/stext");
    if((fr = fopen(stextpath,"r")) == NULL)
    {   Werrprintf("unable to open file \"%s\"",stextpath);
        return 1 ;
    }  
    fixtxt(argv[1]);
    strcpy(savearg1,argv[1]);

    apsize = APSIZE;			/* adapted from CONVERT.C */
    d2flag = 0;
    vnmr   = 0;
    arraystring[0] = '\0';
    arrayn[2][0] = '\0';
    arrayn[1][0] = '\0';
    arrayn[2][0] = '\0';
    parlibfound = 0;
    reval  = -1.0;			/* adapted from CONVERT.C */
    re2val = -1.0;			/* adapted from CONVERT.C */

    P_treereset(CURRENT);               /* clear the tree first */
    P_treereset(PROCESSED);             /* clear the tree first */
    /* strcpy(stdparpath,systemdir); */
    if ( (r = getsysname(sysfil)) )
    {   noblanks(sysfil);
        if (strcmp(sysfil,"bruker") == 0) bruflag=1;
        if (strcmp(sysfil,"cmx") == 0) cmxflag=1;
    }
    if ( (r = getseqname(seqfil)) )
      noblanks(seqfil);
    rewind(fr);

    if (argc == 3)
    {   strcpy(stdparpath,argv[2]);
        strcat(stdparpath,".par/procpar");
        parlibpar = (P_read(CURRENT,stdparpath) == 0);
        if (!parlibpar) 
	{
	   Wscrprintf("Standard parameter file, %s, not found, exiting.\n",
		argv[2]);
           fclose(fr);
           return(1);
	}
    }
    else
    {  
        if (debug1) Wscrprintf("r=%d,seqname=%s\n",r,seqfil);

	if (!r && !bruflag) strcpy(seqfil,"s2pul");
        strcpy(stdparpath,userdir);
        strcat(stdparpath,"/parlib/");
	if (bruflag)
          strcat(stdparpath,sysfil);
	else
          strcat(stdparpath,seqfil);
        strcat(stdparpath,".par/procpar");
        parlibpar = (P_read(CURRENT,stdparpath) == 0);
 	 
	if (!parlibpar)
	{
            strcpy(stdparpath,systemdir);
            strcat(stdparpath,"/parlib/");
	    if (bruflag)
              strcat(stdparpath,sysfil);
	    else
              strcat(stdparpath,seqfil);
            strcat(stdparpath,".par/procpar");
            parlibpar = (P_read(CURRENT,stdparpath) == 0);
	}
    }
    if (!parlibpar && !bruflag)
    {
       strcpy(stdparpath,systemdir);
       strcat(stdparpath,"/parlib/s2pul.par/procpar");
       parlibpar = (P_read(CURRENT,stdparpath) == 0);
       if (parlibpar) strcpy(seqfil,"s2pul");
    }
    if (parlibpar) 
    {  if (!bruflag || (strcmp(seqfil,"s2pul") == 0))
	    Wscrprintf("Using Standard Parameter File: %s\n",seqfil);
    } 
    else
    {   
       if (bruflag)
           Wscrprintf(
		"Standard parameter file '%s.par' not found, exiting.\n",sysfil);
       else
           Wscrprintf("Standard parameter file not found, exiting.\n");
       fclose(fr);
       return(1);
    }
    if (debug1) Wscrprintf("\nESSPAR:\n");
    strcpy(filename,"esspar");
    if (essparms())
    {
       fclose(fr);
       return 0;
    }
    
    if (debug1) Wscrprintf("\nSTEXT:\n");
    if (distext())
    {
       fclose(fr);
       return 1;              /*  and display it */
    }
     
    end = 0;

    strcpy(filename,"acqpar");
    if (debug1) Wscrprintf("\nPARAMS:\n");
    disparms(G_ACQUISITION);

    if (end)
    {
 	end = 0;
        if (debug1) Wscrprintf("\nACQDAT:\n");
        strcpy(filename,"acqdat");
        acqdat();
    }
    fclose(fr);  /* close the files */

    setdgparms(0); /* 0 prevents swapping of lp & lp1, rp & rp1 */
    if (debug1) Wscrprintf("setdgparms done\n");

    setarray();

    storeparams();

    set_nodata();
    storedata(filepath);

    P_treereset(PROCESSED);
    P_copy(CURRENT,PROCESSED);
    Wsetgraphicsdisplay("");
    clearGraphFunc();  /* prevent auto redraw of data */

    releaseAllWithId("sread");
    RETURN;
}
