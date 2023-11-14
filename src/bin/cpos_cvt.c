/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/

/****************************************************************/
/*								*/
/*  cpos_cvt.c - convert a CPOS fid file into a UNIX fid file	*/
/*								*/
/*	cpos_cvt filename					*/
/*								*/
/****************************************************************/ 

/*  Changed ``array_set'' and ``disparms'' for SUN4 compatibility  */
/*  The data structure `old_rentry' must be aligned on a long	   */
/*  word boundary so as to avoid bus errors when extracting the    */
/*  float value.  This is not the case when reading the data from  */
/*  the CPOS-style file, so the data is copied from the original   */
/*  buffer into a explicitly allocated ``old_rval'' data structure */
/*  and the pointer ``rvalp'' assigned to be the address of this   */
/*  data structure.    June 7, 1988				   */
/*								   */
/*  Several changes for VMS compatibility which should not affect  */
/*  the Sun version.  Also added call to D_init(); although this   */
/*  was discovered on VMS, it applies to the Sun version too.      */
/*  This call required because `cpos_cvt' is now a standalone      */
/*  program; no longer part of VNMR.			June 1990  */
/*								   */
/*  strlen is normally defined to be a function returning size_t.  */
/*  With the ANSI C compiler this created warnings if a size_t was */
/*  compared to a simple integer.  The warnings go away if the int */
/*  values are cast to ve size_t.  The size_t typedef is expected  */
/*  to be defined in /usr/include/string.h	October 1995	   */

#include "vnmrsys.h"

#include <stdio.h>
#include <ctype.h>

#ifdef UNIX
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>   /* contains open constants */

extern int	errno;
#else
#include <strings.h>
#include <file.h>
#include <errno.h>
#include  stat
#include "unix_io.h"	/* VMS:  Use routines that work right. */
#include "dirent.h"
#endif

#include "data.h"
#include "group.h"
#include "params.h"
#include "variables.h"

/*  These five variables are left-over from when this code was part of VNMR.	*/

/*extern int debug1,interuption;*/
int debug1 = 0;					/* set to 1 for debug */
int interuption = 0;
int Rflag = 0;
char curexpdir[ MAXPATH ] = "";
int last_line = 0;

/*  These two strings are also from VNMR; unlike the two previous
    variables, they are still active.					*/

char systemdir[ MAXPATH ] = "";	/* do not remove compile-time	*/
char userdir[ MAXPATH ] = "";		/* initialization; required for */
					/* ld to work correctly when	*/
#define  DIRECTORY_ID	1		/* searching VNMR libraries	*/
#define  DATA_ID	2
#define  PROG_ID	3
#define  ARRAY_ID	4

#define	LASTMNEM    35	/* last entry per page in xl type directory	*/ 
#define PAGELENGTH 512	/* number of bytes in xl type directory 	*/
#define OLD_EOM	    25	/* end of message character in xl text file 	*/
#define OLD_EOL	    10	/* end of line character in xl text file 	*/
#define MAXARRAY    18  /* maximun number of array elements             */

#ifdef SUN							/* 680xx */
#define ZERO	     0  /* 1 on VAX, 0 on 68020 */	   /* also SPARC */
#define ONE	     1  /* 0 on VAX, 1 on 68020 */
#define TWO	     2  /* 3 on VAX, 2 on 68020 */
#define THREE	     3  /* 2 on VAX, 3 on 68020 */
#define CMULT 	     1.0 /* 4.0 on VAX, 1.0 on 68020 */
#else								/* VAX */
#define ZERO	     1  /* 1 on VAX, 0 on 68020 */
#define ONE	     0  /* 0 on VAX, 1 on 68020 */
#define TWO	     3  /* 3 on VAX, 2 on 68020 */
#define THREE	     2  /* 2 on VAX, 3 on 68020 */
#define CMULT 	     4.0 /* 4.0 on VAX, 1.0 on 68020 */
#endif

#define APSIZE       128000.0  /* maximun number of data points */
#define MAXSWK       100.0     /* maximun sweep width in KHz    */
#define MAXFN        524288.0  /* maximun Fourier number        */
#define MAXTIM       8190.0    /* maximun timer word in secs    */
#define MINTIM       1e-7      /* minimun timer word in secs    */

struct old_fcb		/* xl type file control block 	*/
  { char  i1[2];
    short i2;
    short i3;
    short i4;
  };

struct old_fentry
  { char n[6];		/* xl type directory entry 	*/
    struct old_fcb f;
  };

struct  old_rentry
  { char q[4];		/* xl type parameter qualifier 	*/
    float r;
  };

struct old_mnempage	/* xl type directory page 	*/
  { short next,num;
    struct old_fentry mnemarray[LASTMNEM+1];
    short spare1,spare2;
  };

static struct old_mnempage dirpage;	/* buffer for one page of xl directory*/
static int curdirpage;			/* current page index in this buffer */
static char arraystring[64];
static char curfiddir[MAXPATH];	/* destination directory */
static int decplaces,mult;		/* parameter characteristics */
static float max,min,step;		/* parameter characteristics */
static double apsize,maxswk,maxfn,maxtim,mintim,cmult;
static double reval,re2val;
static char dir_with_cpos[MAXPATH];

int parlibpar,parlibfound;		/* used by sread.c */
         
void cposStrlcpy(char *p1, char *p2, int l );

/****************************************************************/
/*  Below are various symbols required by routines in magiclib.a or
    unmrlib.a which are needed by the `cpos_cvt' program.  They are
    defined here to prevent the ``ld'' program from bringing in the
    VNMR module which defines them, as the VNMR version includes
    references to SUN libraries which we would rather not load.    */
 
int     Dflag = 0;
int     Eflag = 0;
 
void unsetMagicVar(addr)
int addr;
{
}

/*  Under normal operation the window routines below should not
    be called.  At this time each routine just announces itself.  */
 
void WerrprintfWithPos()
{
        printf( "WerrprintfWithPos called\n" );
}
 
void Wscrprintf()
{
        printf( "Wscrprintf called\n" );
}
 
void Wprintfpos()
{
        printf( "Wprintfpos called\n" );
}
 
void Werrprintf()
{
        printf( "Werrprintf called\n" );
}
 
void Winfoprintf()
{
        printf( "Winfoprintf called\n" );
}

void saveFDAfiles_processed(char *dum1, char *dum2, char *dum3)
{
}

void initxposebuf() {}
void p11_saveFDAfiles_processed() {}
void transpose() {}

/****************************************************************/


/************************************/
static int SP_creatvar(tree,name,group)
int tree; char *name; int group;
/************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_creatvar(tree,name,group));
}

/**********************************************/
static int SP_Esetstring(tree,name,strptr,index)
int tree; char *name; char *strptr; int index;
/**********************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_Esetstring(tree,name,strptr,index));
}

/**********************************************/
static int SP_setlimits(tree,name,max,min,step)
int tree; char *name; double max,min,step;
/**********************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_setlimits(tree,name,max,min,step));
}

/*************************************/
static int SP_setgroup(tree,name,group)
int tree; char *name; int group;
/*************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_setgroup(tree,name,group));
}

/**************************************/
static int SP_setdgroup(tree,name,group)
int tree; char *name; int group;
/**************************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_setdgroup(tree,name,group));
}

/***********************************/
static int SP_setprot(tree,name,prot)
int tree; char *name; int prot;
/***********************************/
{
  if (parlibfound)
    return(0);
  else
    return(P_setprot(tree,name,prot));
}

/***************************/
static getdata(fd,buf,pos,nn)
/***************************/
/* read from file fd n xl size pages starting at pos into buf */
int fd;
struct old_mnempage *buf;
int pos,nn;
{ int x;
  lseek(fd,PAGELENGTH*pos,0);
  x=read(fd,buf,PAGELENGTH*nn);
  if (x==nn*PAGELENGTH) return 1;
  else 
    { perror("cpos_cvt:");
      return 0;
    }
}

/*********************/
static getbytes(fd,buf,pos,nn)
/*********************/
/* read from file fd nn bytes starting at pos pages into buf */
int fd;
struct old_mnempage *buf;
int pos,nn;
{ int x;
  lseek(fd,PAGELENGTH*pos,0);
  x=read(fd,buf,nn);
  if (x==nn) return 1;
  else 
    { perror("cpos_cvt:");
      return 0;
    }
}

/**********************************/
static cpos_to_vax(cpos_ptr,vax_ptr,strlen)
/**********************************/
/* convert a xl type string to unix */
char *cpos_ptr;
char *vax_ptr;
int  strlen;
{
  char	temp_chr;
  int   index;
  index=0;
  do
  { temp_chr = *cpos_ptr & 0x7F;	/* swab bytes and remove parity */
    *(vax_ptr+ONE) = *(cpos_ptr+1) & 0x7F;
    *(vax_ptr+ZERO) = temp_chr;
    vax_ptr += 2;
    cpos_ptr += 2;
    index += 2;
  } while ((index>= strlen) ? 0 :
     ((*(cpos_ptr-2) & 0x7F) && (*(cpos_ptr-1)& 0x7F)));
  *vax_ptr = '\000';			/* terminate unix string */
}

/************************/
static disdir(fx,firstsec,numsec)
/************************/
/* display a xl type directory at firstsec, numsec pages long */
int fx,firstsec,numsec;
{
  int i,sec;
  char name[8];
  for (sec=firstsec;sec<firstsec+numsec;sec++)
    { if (sec!=curdirpage)	/* get a page, if not there */
        { getdata(fx,&dirpage,sec,1);
	  curdirpage = sec;
        }
      for (i=0; i<=LASTMNEM; i++)
        { cpos_to_vax(dirpage.mnemarray[i].n,name,6);	/* convert the name */
          if (name[0]!=' ')	/* blank is used in unused entries */
            { printf("%8s %3d %3d %8d %8d %8d\n",name,
	      dirpage.mnemarray[i].f.i1[0]+128,
              dirpage.mnemarray[i].f.i1[1]+128,dirpage.mnemarray[i].f.i2,
	      dirpage.mnemarray[i].f.i3,dirpage.mnemarray[i].f.i4);
            }
         }
     }
}

/*************/
static distext(fx,sec)
/*************/
/* display the vxr text at page sec of file fx, store in current experiment */
int fx,sec;
{
  int i,j;
  char xlpage[PAGELENGTH];
  char upage[PAGELENGTH];
  FILE *fopen();
  FILE *textfile;		/* text file will be created as curexp/text */
  char tfilepath[MAXPATH];
  getdata(fx,xlpage,sec,1);
  /* convert to Unix format */
  cpos_to_vax(xlpage,upage,PAGELENGTH);
  /* strip off blanks at end */
  i = PAGELENGTH-1;
  while ((i>0) && (upage[i]==' ')) i--;
  if (upage[i]==OLD_EOM) i--; 
  if (i<(PAGELENGTH-2)) 
    { upage[i+2] = '\0';
      upage[i+1] = '\n';
    }
  /* convert backslash and eol to newline */
  for (j=0;j<=i;j++)
    {
      if ((upage[j]=='\\') || (upage[j]==OLD_EOL)) upage[j] = '\n';
    }
  /* display it */
  printf("%s",upage); 
  /* open "curfiddir/text"  writeonly, create if does not exist */
  strcpy(tfilepath,curfiddir);
#ifdef UNIX
  strcat(tfilepath,"/text");
#else
  strcat(tfilepath,"text");
#endif
  if(textfile=fopen(tfilepath,"w+"))
    { fprintf(textfile,"%s",upage);
      fclose(textfile);
      chmod(tfilepath,0666);
    }
  else
    { printf("unix system error\n");
      perror("cpos_cvt");
    }
}

/*******************/
static checkresult(res,info)
/*******************/
/* check the result of a parameter function and display error, if necessary */
int res;
char *info;
{ if (res)
    if ((res <= -6) || (res >= -2))
      { printf("Error %d in parameter routine %s\n",res,info);
      }
}

/*******************/
static set_egroup(egroup)
/*******************/
char egroup;
{
   decplaces=0;
   mult=1;
   switch (egroup) 	/* now set up according to entry group */
   { 
     case 'A': decplaces=1; mult=1e6; max=maxtim; min=0; step=mintim; break;
     case 'B': decplaces=3; max=maxtim; min=0; step=mintim; break;
     case 'C': decplaces=1; max=maxswk; min=100; step = -mintim; break;
     case 'D': decplaces=1; max=100; min=0.001; step=0.001; break;
     case 'E': decplaces=0; mult=2; max=apsize; min=64; step=64; break;
     case 'F': decplaces=0; max=5e4; min=0; step=100; break;
     case 'G': decplaces=1; max=100; min= -200; step=0.1; break;
     case 'H': decplaces=1; max=255; min=0; step=1; break;
     case 'I': decplaces=0; max=1e5; min= -1e5; step=100; break;
     case 'J': decplaces=1; max=9.9e4; min= -9.9e4; step=0.1; break;
     case 'K': decplaces=0; max=1e9; min=1; step=1; break;
     case 'L': decplaces=0; max=1e9; min=0; step=1; break;
     case 'M': decplaces=0; max=900; min=1; step=1; break;
     case 'N': decplaces=3; max=maxswk; min=0; step=0; break;
     case 'O': decplaces=0; mult=2; max=maxfn; min=64; step= -2; break;
     case 'P': decplaces=1; max=1e9; min= -1e9; step=0.0; break;
     case 'Q': decplaces=0; max=500; min= 0; step=0.1; break;
     case 'R': decplaces=0; max=500; min= -500; step=0.1; break;
     case 'S': decplaces=0; max=2047; min= -2048; step=1; break;
     case 'T': decplaces=1; max=1e9; min=0; step=0; break;
     case 'U': decplaces=1; max=3.2e7; min=0; step=0.1; break;
     case 'V': decplaces=1; max=3.6e3; min= -3.6e3; step=0.1; break;
     case 'W': decplaces=3; max=maxtim; min=0; step=mintim; break;
     case 'X': decplaces=0; max=39; min=0; step=1; break;
     case 'Y': decplaces=4; max=1e18; min= -1e18; step=0; break;
     case 'Z': decplaces=0; max=63; min=0; step=1; break;
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
     default:  decplaces=1; mult=1e6; max=1e5; min=0; step=1e-7; break;
   }
}

/***************************/
static string_var(name,name1,element)
/***************************/
char name[],name1[];
int  element;
{ int pres;

  if (debug1) printf(" ...%s..., max=%g\n",name1,max);
  /* create the variable */
  pres=SP_creatvar(TEMPORARY,name,ST_STRING);
  checkresult(pres,"creatvar");
  /* set the limits */
  pres=SP_setlimits(TEMPORARY,name,max,0.0,0.0);
  checkresult(pres,"setlimits");
  /* set the value */
  pres=P_setstring(TEMPORARY,name,name1,element);
  checkresult(pres,"setstring");
  if (pres) printf("name='%s' ...%s..., max=%g\n",name,name1,max);
}

/*************************/
static par1_set(name,name1,element,active)
/*************************/
char name[],name1[];
int  element,active;
{ int pres;
  int protection;

  if (debug1) printf(" string ");
  max = 8;
  string_lower_case(name1);
  noblanks(name1);
  string_var(name,name1,element);
  pres=SP_setgroup(TEMPORARY,name,G_ACQUISITION);
  checkresult(pres,"setgroup");
  protection = (active == 'X') ? P_ACT : 0;
  protection |= P_ACT;
  pres = SP_setprot(TEMPORARY,name,protection);
  checkresult(pres,"setprotection");
}

/*************************/
static par18_set(name,rvalp,element)
/*************************/
char name[];
struct old_rentry *rvalp;
int  element;
{ char name1[8];

  if (debug1) printf(" string ");
  cpos_to_vax(&(rvalp->q[2]),name1,6);
  string_lower_case(name1);
  noblanks(name1);
  max =17;
  if (strcmp(name1,"aceton") == 0) strcpy(name1,"acetone");
  string_var(name,name1,element);
}

/*************************/
static par19_set(name,rvalp,element)
/*************************/
char name[];
struct old_rentry *rvalp;
int  element;
{ char name1[80];

  if (debug1) printf(" string ");
  sprintf(name1,"%2d-%2d-%2d",rvalp->q[2+ONE],rvalp->q[4+ONE],
    rvalp->q[6+ONE]);
  if (name1[3]==' ') name1[3]='0';
  if (name1[6]==' ') name1[6]='0';
  max=9;
  string_var(name,name1,element);
}

/************************/
static set_enum(name,val1,pos)
/************************/
char name[],val1[]; int pos;
{
  int pres;

  pres=SP_Esetstring(TEMPORARY,name,val1,pos);
  checkresult(pres,"Esetstring");
}

/************************/
static set_2enum(name,val1,val2)
/************************/
char name[],val1[],val2[];
{
  int pres;

  pres=SP_Esetstring(TEMPORARY,name,val1,1); /* possible values */
  checkresult(pres,"Esetstring");
  pres=SP_Esetstring(TEMPORARY,name,val2,2);
  checkresult(pres,"Esetstring");
}

/************************/
static par_set(name,rvalp,element,d2flag)
/************************/
char name[];
struct old_rentry *rvalp;
int  element,d2flag;
{ char name1[6];
  int xx;
  int pres;

  if (debug1) printf(" string\n");
  if ((max<1)||(max>4)) max=1;
  xx=max;
  cpos_to_vax(&(rvalp->q[4]),name1,xx);
  name1[(int)max]='\000'; /* if max is odd */
  string_lower_case(name1);
  noblanks(name1);
  if ((strcmp(name,"math")==0) && (d2flag))
    name1[0] = 'i';
  string_var(name,name1,element);
  if ((strcmp(name,"cp")==0) || (strcmp(name,"il")==0) ||
      (strcmp(name,"homo")==0) || (strcmp(name,"load")==0) ||
      (strcmp(name,"tin")==0) ||
      (strcmp(name,"in")==0) || (strcmp(name,"dp")==0))
  {
    set_2enum(name,"y","n");
  }
/*else if (strcmp(name,"syn")==0) 
  {
    set_2enum(name,"yy","yn");
    set_enum(name, "ny",3);
    set_enum(name, "nn",4);
    if ((strcmp(name1,"y")==0)|| (strcmp(name1,"n")==0))
    {
        if (strcmp(name1,"y")==0)
            pres=P_setstring(TEMPORARY,name,"yn",1);
        else
            pres=P_setstring(TEMPORARY,name,"nn",1);
        checkresult(pres,"setstring syn");
    }
  }   */
  else if (strcmp(name,"rfband")==0) 
  {
    set_2enum(name,"hh","hl");
    set_enum(name, "lh",3);
    set_enum(name, "ll",4);
    if ((strcmp(name1,"a")==0)|| (strcmp(name1,"b")==0))
    {
        if (strcmp(name1,"b")==0)
            pres=P_setstring(TEMPORARY,name,"hh",1);
        else
            pres=P_setstring(TEMPORARY,name,"ll",1);
        checkresult(pres,"setstring syn");
    }
  }
  else if (strcmp(name,"math")==0)
  {
    set_2enum(name,"d","f");
    pres=SP_Esetstring(TEMPORARY,name,"i",3);
    checkresult(pres,"Esetstring");
  }
}

/********************************/
static real_set(name,rvalp,egroup,element)
/********************************/
char name[],egroup;
struct old_rentry *rvalp;
int  element;
{ int *pntr;
  int pres;
  char name1[80];

  pntr = (int *) &(rvalp->r);
  /* fix special floating point pattern illegal on VAX */
  if ( ZERO && (*pntr == 32768)) *pntr = 0;
  if (debug1)
  {
    printf(" real\n");
    sprintf(name1," %%8.%1df\n",decplaces);
    printf(name1,mult*cmult*rvalp->r);
    printf(",mx=%8.2e,mn=%8.2e,st=%8.2e\n",max,min,step);
  }
  /* first create the variable */
  if      (egroup=='A')  pres=SP_creatvar(TEMPORARY,name,ST_PULSE);
  else if (egroup=='B')  pres=SP_creatvar(TEMPORARY,name,ST_DELAY);
  else if (egroup=='I')  pres=SP_creatvar(TEMPORARY,name,ST_FREQUENCY);
  else if (egroup=='J')  pres=SP_creatvar(TEMPORARY,name,ST_FREQUENCY);
  else if (decplaces==0) pres=SP_creatvar(TEMPORARY,name,ST_INTEGER);
  else                   pres=SP_creatvar(TEMPORARY,name,ST_REAL);
  checkresult(pres,"creatvar");
  /* set the limits */
  pres=SP_setlimits(TEMPORARY,name,max,min,step);
  checkresult(pres,"setlimits");
  /* set the value */
  pres=P_setreal(TEMPORARY,name,(double)mult*cmult*rvalp->r,element);
  checkresult(pres,"setreal");
}

/************************************************************/
static qualifiers(rvalp,realflag,partype,dgroup,active,egroup)
/************************************************************/
struct old_rentry *rvalp;
int  *realflag,*partype,*dgroup;
char *active,*egroup;
{
   *partype  = rvalp->q[ZERO]+128;	/* xl parameter type */
   *realflag = ((rvalp->q[THREE] & 32) && (*partype != 18));
					/* real or string? */
   *dgroup   = rvalp->q[THREE] & 7;	/* xl display group */
   *active   = rvalp->q[ONE]+128;	/* active flag */
   if (*active<32) *active=' ';		/* some params have bad char */
   *egroup   = rvalp->q[TWO]+128;	/* entry group */
}

/*****************************************/
static array_set(name,fcbt,fx,firstsector,active,partype,d2flag)
/*****************************************/
char name[];
struct old_fcb *fcbt;
int fx,firstsector;
char *active;
int  *partype,d2flag;
{ struct old_mnempage arraypage;  /* buffer for one page of xl directory*/
  int pindex;
  struct old_rentry *rvalp;
  struct old_rentry  rvalbuf;
  int  realflag,dgroup;
  char egroup;
  char *tptr;
  int  done;
  char name1[6];

  getdata(fx,&arraypage,firstsector + fcbt->i3 ,1);
  pindex = 0;
/*  rvalp = (struct old_rentry *)&arraypage.mnemarray[pindex].f; */
  tptr = (char *)&arraypage.mnemarray[pindex].f;
  rvalp = &rvalbuf;
  /*bcopy( tptr, rvalp, sizeof( rvalbuf) );*/
  memcpy( rvalp, tptr, sizeof( rvalbuf) );

  qualifiers(rvalp,&realflag,partype,&dgroup,active,&egroup);
  set_egroup(egroup);
  done = 0;
  while ((pindex<MAXARRAY) && (!done))      /* go through all entries */
  {
    cpos_to_vax(arraypage.mnemarray[pindex].n,name1,6);
    done = (name1[0] == ' ');
    if (!done)
    {
/*      rvalp = (struct old_rentry *)&arraypage.mnemarray[pindex].f; */
      tptr = (char *)&arraypage.mnemarray[pindex].f;
      rvalp = &rvalbuf;
      /*bcopy( tptr, rvalp, sizeof( rvalbuf) );*/
      memcpy( rvalp, tptr, sizeof( rvalbuf) );

      pindex++;
      if (*partype==18)
        par18_set(name,rvalp,pindex);
      else if (*partype==19)
        par19_set(name,rvalp,pindex);
      else if (realflag)
        real_set(name,rvalp,egroup,pindex);
      else 
        par_set(name,rvalp,pindex,d2flag);
    }
  }
}

/***********************************/
static disparms(fx,entries,firstsec,grident,d2flag)
/***********************************/
/* display a xl type parameter directory of specified no of entries
   starting at specified page and puts variables into the TEMPORARY tree */
int fx,entries,firstsec,grident,*d2flag;
{
  int sec,pindex,index,partype,ix,dgroup,pres;
  int realflag,protection;
  int foundtype = 0;
  char name[8],name1[8];
  char active,egroup;
  struct old_rentry *rvalp;
  struct old_rentry  rvalbuf;
  char *tptr;
  vInfo dummy;
  char parlibpath[MAXPATH];
  double rval;

  index  = 0;			/* index through all entries */
  pindex = 0;			/* index within a page */
  sec    = firstsec;		/* current page */
  while (index<entries)		/* go through all entries */
    { if (pindex==0)
        { if (sec!=curdirpage)	/* get page, if not there */
          { getdata(fx,&dirpage,sec,1);
	    curdirpage = sec;
          }
        }
      cpos_to_vax(dirpage.mnemarray[pindex].n,name,6); /* convert the name */
      if ((name[0]!=' ') && (strcmp(name,"XX    \000"))) /* unused entry? */
	{  /* construct a pointer pointing to the parameter qualifier */
/*	   rvalp = (struct old_rentry *)&dirpage.mnemarray[pindex].f; */
	   tptr = (char *)&dirpage.mnemarray[pindex].f;
	   rvalp = &rvalbuf;
	   /*bcopy( tptr, rvalp, sizeof( rvalbuf) );*/
	   memcpy( rvalp, tptr, sizeof( rvalbuf) );

           qualifiers(rvalp,&realflag,&partype,&dgroup,&active,&egroup);
	   if ((grident==G_ACQUISITION)&&(index<2))
	     {	/* treat PSLABEL and SEQFIL as exceptions */
	     strcpy(name1,name);
	     if (index==0)
               { strcpy(name,"PSLABEL");
                 strcpy(parlibpath,userdir);
#ifdef UNIX
                 strcat(parlibpath,"/parlib/");
#else
		 vms_fname_cat(parlibpath,"[.parlib]");
#endif
                 string_lower_case(name1);
                 noblanks(name1);
#ifdef UNIX
                 strcat(parlibpath,name1);
                 strcat(parlibpath,".par/procpar");
#else
		 strcat(parlibpath,name1);
		 strcat(parlibpath,"_par");
		 make_vmstree(parlibpath,parlibpath,MAXPATHL);
		 strcat(parlibpath,"procpar");
#endif	
                 parlibpar = (P_read(TEMPORARY,parlibpath) == 0);
                 if (parlibpar==0)
                   { strcpy(parlibpath,systemdir);
#ifdef UNIX
                     strcat(parlibpath,"/parlib/");
                     strcat(parlibpath,name1);
                     strcat(parlibpath,".par/procpar");
#else
		     vms_fname_cat(parlibpath,"[.parlib]");
		     strcat(parlibpath,name1);
		     strcat(parlibpath,"_par");
		     make_vmstree(parlibpath,parlibpath,MAXPATHL);
		     strcat(parlibpath,"procpar");
#endif
                     parlibpar = (P_read(TEMPORARY,parlibpath) == 0);
                   }
                 if (parlibpar==0)
                   { strcpy(parlibpath,systemdir);
#ifdef UNIX
                     strcat(parlibpath,"/parlib/s2pul.par/procpar");
#else
		     vms_fname_cat(parlibpath,"[.parlib.s2pul_par]procpar");
#endif
                     parlibpar = (P_read(TEMPORARY,parlibpath) == 0);
                   }
                 if (parlibpar)
                   printf("using parameter file %s\n",parlibpath);
                 else 
                   printf("not using parameter file %s\n",parlibpath);
               }
	     else          strcpy(name,"SEQFIL");
	     partype = 1;	/* to mark these special params */
	     }
           string_lower_case(name);
	   noblanks(name);
           /* display name, display group, active flag, and param type*/
	   if (debug1)
             printf("%s,  dg%1d, act=%1c, tp=%3d, eg=%1c\n",
                         name,dgroup,active,partype,egroup);
	   /* change names of certain variables */
	   if      (strcmp(name,"sp2"   )==0) strcpy(name,"sp1");
	   else if (strcmp(name,"wp2"   )==0) strcpy(name,"wp1");
	   else if (strcmp(name,"sw2"   )==0) strcpy(name,"sw1");
	   else if (strcmp(name,"rfl2"  )==0) strcpy(name,"rfl1");
	   else if (strcmp(name,"rfp2"  )==0) strcpy(name,"rfp1");
	   else if (strcmp(name,"fn2"   )==0) strcpy(name,"fn1");
	   else if (strcmp(name,"lb2"   )==0) strcpy(name,"lb1");
	   else if (strcmp(name,"af"    )==0) strcpy(name,"gf");
	   else if (strcmp(name,"af2"   )==0) strcpy(name,"gf1");
	   else if (strcmp(name,"solvnt")==0) 
		{active='Y'; strcpy(name,"solvent");
      		}
	   else if (strcmp(name,"to"    )==0) strcpy(name,"tof");
	   else if (strcmp(name,"do"    )==0) strcpy(name,"dof");
	   else if (strcmp(name,"opos"  )==0)
		{active='Y'; strcpy(name,"rfband");
      		}
/*	   else if (strcmp(name,"synt"  )==0) 
	   	{active='Y'; strcpy(name,"syn");
		}				*/
	   else if (strcmp(name,"array1")==0) strcpy(name,"lifrq");
	   else if (strcmp(name,"array2")==0) strcpy(name,"liamp");
	   else if (strcmp(name,"array3")==0) strcpy(name,"llfrq");
	   else if (strcmp(name,"rstdir")==0) strcpy(name,"llamp");
	   else if (strcmp(name,"mxcnst")==0) strcpy(name,"intmod");
           else if (strcmp(name,"lcr"   )==0) strcpy(name,"delta");
           if (parlibpar)
           {  parlibfound = (P_getVarInfo(TEMPORARY,name,&dummy)==0);
	      if (parlibfound) foundtype = dummy.basicType;
	   }
           else
             parlibfound = 0;
           if (parlibfound) /* do nothing */ ;
           /* else reset egroup of certain parameters  */
	   else if (strcmp(name,"sfrq"  )==0) egroup = 'T';
	   else if (strcmp(name,"dfrq"  )==0) egroup = 'T';
	   else if (strcmp(name,"vtwait")==0) egroup = 'B';
	   else if (strcmp(name,"vs"    )==0) egroup = 'T';
	   else if (strcmp(name,"is"    )==0) egroup = 'T';
	   else if (strcmp(name,"th"    )==0) egroup = 'T';
	   else if (strcmp(name,"vf"    )==0) egroup = 'T';
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
           /*  special case for aig and dcg */
	        if (strcmp(name,"aig"   )==0) egroup = '2';
	   else if (strcmp(name,"dcg"   )==0) egroup = '3';
	   if (strcmp(name,"ni")==0)
             *d2flag = (rvalp->r > 1.5);
	   if (debug1)
             printf("%s,  dg%1d, act=%1c, tp=%3d, eg=%1c\n",
                         name,dgroup,active,partype,egroup);
           set_egroup(egroup);


	   if     ((strcmp(name,"vs")==0)|| 
	           (strcmp(name,"is")==0)) min=1e-6;
	   else if (strcmp(name,"fb")==0)  max=49900;

	   /* omit certain parameters */
	   if      (strcmp(name,"mftl"  )==0) partype = 0;
	   else if (strcmp(name,"domain")==0) partype = 0;
	   else if (strcmp(name,"pt"    )==0) partype = 0;
	   else if (strcmp(name,"heg"   )==0) partype = 0;
	   else if (strcmp(name,"hzcm"  )==0) partype = 0;
	   else if (strcmp(name,"lflag1")==0) partype = 0;
	   else if (strcmp(name,"lflag2")==0) partype = 0;
	   else if (strcmp(name,"tonorm")==0) partype = 0;
	   else if (strcmp(name,"donorm")==0) partype = 0;
	   else if (strcmp(name,"dsynt" )==0) partype = 0;
	   else if (strcmp(name,"adcflg")==0) partype = 0;
	   else if (strcmp(name,"lh"    )==0) partype = 0;
	   else if (strcmp(name,"imax"  )==0) partype = 0;
	   else if (strcmp(name,"mnps"  )==0) partype = 0;
	   else if (strcmp(name,"imav"  )==0) partype = 0;
	   else if (strcmp(name,"reav"  )==0) partype = 0;
	   else if (strcmp(name,"scale" )==0) partype = 0;
	   else if (strcmp(name,"se"    )==0) partype = 0;
	   else if (strcmp(name,"se2"   )==0) partype = 0;
	   else if (strcmp(name,"re"    )==0) partype = 0;
	   else if (strcmp(name,"re2"   )==0) partype = 0;
	   else if (strcmp(name,"cd"    )==0) partype = 0;
	   else if (strcmp(name,"cd2"   )==0) partype = 0;
	   else if (strcmp(name,"ccd"   )==0) partype = 0;
	   else if (strcmp(name,"ccd2"  )==0) partype = 0;
	   else if (strcmp(name,"npoint")==0) partype = 0;
	   else if (strcmp(name,"qp"    )==0) partype = 0;
	   else if (strcmp(name,"syn"   )==0) partype = 0;
	   else if (strcmp(name,"synt"  )==0) partype = 0;
           /* change type of certain parameters */
	   else
             if ((strcmp(name,"tn")==0) || (strcmp(name,"dn")==0))
	     { pres=SP_creatvar(TEMPORARY,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(TEMPORARY,name,4.0,0.0,0.0);
	       checkresult(pres,"setlimits");
 	       rval= mult*cmult*rvalp->r;
 	       if (rval >=1.0 && rval<=1.999)
	         pres=P_setstring(TEMPORARY,name,"H1",0);
	       else if (rval >=2.0 && rval<=2.999)
	         pres=P_setstring(TEMPORARY,name,"H2",0);
	       else if (rval >=3.0 && rval<=3.999)
	         pres=P_setstring(TEMPORARY,name,"H3",0);
	       else if (rval >=13.0 && rval<=13.999)
	         pres=P_setstring(TEMPORARY,name,"C13",0);
	       else if (rval >=14.0 && rval<=14.999) 
	         pres=P_setstring(TEMPORARY,name,"N14",0);
	       else if (rval >=15.0 && rval<=15.999) 
	         pres=P_setstring(TEMPORARY,name,"N15",0);
	       else if (rval >=17.0 && rval<=17.999)
	         pres=P_setstring(TEMPORARY,name,"O17",0);
	       else if (rval >=19.0 && rval<=19.999)
	         pres=P_setstring(TEMPORARY,name,"F19",0);
	       else if (rval >=23.0 && rval<=23.999)
	         pres=P_setstring(TEMPORARY,name,"Na23",0);
	       else if (rval >=31.0 && rval<=31.999)
	         pres=P_setstring(TEMPORARY,name,"P31",0);
	       else if (rval >=39.0 && rval<=39.999)
	         pres=P_setstring(TEMPORARY,name,"K39",0);
	       else
	         pres=P_setstring(TEMPORARY,name,"???",0);
	       checkresult(pres,"setstring");
	       pres=SP_setgroup(TEMPORARY,name,G_ACQUISITION);
	       checkresult(pres,"setgroup");
               protection = (active == 'X') ? P_ACT : 0;
 	       protection |= P_ACT;
               pres = SP_setprot(TEMPORARY,name,protection);
	       checkresult(pres,"setprotection");
	       partype = 0; /* nothing further to do */
	     }
	   else
             if ((strcmp(name,"lifrq")==0) || (strcmp(name,"llfrq")==0) ||
                 (strcmp(name,"liamp")==0) || (strcmp(name,"llamp")==0))
             {
               realflag = 1;
               dgroup   = 1;
               active   = 'N';
	       rvalp->r = 0.0;
               real_set(name,rvalp,egroup,0);
	       pres=P_setactive(TEMPORARY,name,ACT_OFF);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(TEMPORARY,name,G_DISPLAY);
	       checkresult(pres,"setgroup");
	       partype = 0; /* nothing further to do */
             }
	   else
             if (strcmp(name,"intmod")==0)
	     { pres=SP_creatvar(TEMPORARY,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(TEMPORARY,name,8.0,0.0,0.0);
	       checkresult(pres,"setlimits");
	       pres=SP_Esetstring(TEMPORARY,name,INT_OFF,1); /* possible values */
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,INT_PARTIAL,2);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,INT_FULL,3);
	       checkresult(pres,"Esetstring");
	       pres=P_setstring(TEMPORARY,name,INT_OFF,0);
	       checkresult(pres,"setstring");
	       pres=P_setactive(TEMPORARY,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(TEMPORARY,name,G_DISPLAY);
	       checkresult(pres,"setgroup");
	       partype = 0; /* nothing further to do */
             }
	   else
             if (strcmp(name,"pltmod")==0)
	     { int pltmod;

               pres=SP_creatvar(TEMPORARY,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(TEMPORARY,name,8.0,0.0,0.0);
	       checkresult(pres,"setlimits");
	       pres=SP_Esetstring(TEMPORARY,name,"off",1); /* possible values */
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,"fixed",2);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,"full",3);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,"variable",4);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(TEMPORARY,name,"user",5);
	       checkresult(pres,"Esetstring");
	       pltmod = (int) rvalp->r;
               switch (pltmod)
               {
                 case 0:  pres=P_setstring(TEMPORARY,name,"off",0);      break;
                 case 1:  pres=P_setstring(TEMPORARY,name,"fixed",0);    break;
                 case 2:  pres=P_setstring(TEMPORARY,name,"full",0);     break;
                 case 3:  pres=P_setstring(TEMPORARY,name,"variable",0); break;
                 default: pres=P_setstring(TEMPORARY,name,"off",0);      break;
               }
	       checkresult(pres,"setstring");
	       pres=P_setactive(TEMPORARY,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(TEMPORARY,name,G_DISPLAY);
	       checkresult(pres,"setgroup");
	       partype = 0; /* nothing further to do */
             }

	   /* if conflict between parameter as defined in the standard
	      paramter file nad parameter as read from the old-style
	      FID file, skip the old-style parameter.			*/

	   if (parlibfound && partype != 4 && dummy.size == 1)
	     if (( realflag && foundtype != ST_REAL) ||
		 (!realflag && foundtype == ST_REAL)) partype = 0;

	   /* display info according to parameter type */
	   if (partype==7)
	     { 
	       cpos_to_vax(&(rvalp->q[2]),name1,6);
	       string_lower_case(name1);
	       noblanks(name1);
	       if (strlen(name1))
		 { if (strlen(arraystring)) strcat(arraystring,",");
	           strcat(arraystring,name1);
                 }
	     }
	   else if (partype==0)
	     { if (debug1) printf("omitted parameter\n");
               if ((strcmp(name,"re")==0) && (active == 'Y'))
	         reval = rvalp->r;
               else if ((strcmp(name,"re2")==0) && (active == 'Y'))
	         re2val = rvalp->r;
	     }
           else if (partype==1)
             par1_set(name,name1,0,active);
	   else if (partype==4)
	     { if (debug1) printf("array identifier, name=%s\n",name);
               array_set(name,rvalp,fx,firstsec,&active,&partype,*d2flag);
	     }
	   else if (partype==18)
             par18_set(name,rvalp,0);
	   else if (partype==19)
             par19_set(name,rvalp,0);
	   else if (realflag)
             real_set(name,rvalp,egroup,0);
	   else 
             par_set(name,rvalp,0,*d2flag);
           if ((partype) && (strcmp(name,"dim1")) && (strcmp(name,"dim2")) &&
                            (strcmp(name,"dim3")))
           {
             protection = (active == 'X') ? P_ACT : 0;
             if ((strcmp(name,"at")==0) || (strcmp(name,"np")==0) ||
                 (strcmp(name,"sw")==0) || (strcmp(name,"solvent") == 0))
               protection |= P_MAC;
             if (((!(partype == 20) && !(partype == 21) &&
                 !(partype == 22))) || (grident != G_ACQUISITION) ||
                 (strcmp(name,"sfrq")==0) || (strcmp(name,"dfrq")==0))
               protection |= P_ARR;
             if (strcmp(name,"ct")==0)
               protection |= P_VAL;
/*           if (strcmp(name,"syn")==0)
               protection  |= P_ACT;       */
             pres = SP_setprot(TEMPORARY,name,protection);
	     checkresult(pres,"setprotection");
           }
           if (partype>9)
	     {
	       /* set the active flag */
	       if (active=='N')      pres=P_setactive(TEMPORARY,name,ACT_OFF);
	       else		     pres=P_setactive(TEMPORARY,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(TEMPORARY,name,grident);
	       checkresult(pres,"setgroup");
	     }
	}
      /* go to next parameter */
      index++;
      pindex++;
      if (pindex>LASTMNEM)	/* go to next page */
	{ pindex = 0;
	  sec += 1;
	}
    }
  if (debug1) printf("done\n");
}

/*****************/
dgparm(name,type,group,max,min,step,protection)
/*****************/
char *name;
int   type,group,protection;
double max,min,step;
{
  int pres;
  vInfo dummy;

  if (parlibpar)
    parlibfound = (P_getVarInfo(TEMPORARY,name,&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_creatvar(TEMPORARY,name,type);
    checkresult(pres,"creatvar");
    pres=P_setgroup(TEMPORARY,name,group);
    checkresult(pres,"setgroup");
    pres=P_setlimits(TEMPORARY,name,max,min,step);
    checkresult(pres,"setlimits");
    pres = P_setprot(TEMPORARY,name,protection);
    checkresult(pres,"setprotection");
  }
}

/*****************/
setdgparms(d2flag)
/*****************/
int d2flag;
/* first create new parameters for Unix software, then setup DG templates */
/* save tree finally in current experiment */
/* also create some new parameters: hzmm,rplot,trace,array,arraydim */
{
  int pres,nfflag;
  char t[1024];
  double wc,wp,rnf,phase;
  vInfo dummy;

  nfflag = ((P_getreal(TEMPORARY,"nf",&rnf,1)==0) && (rnf>1.5));

  pres = P_setprot(TEMPORARY,"seqfil", P_ARR | P_ACT | P_MAC);
  if (!pres) pres = P_setprot(TEMPORARY,"pslabel",P_ARR | P_ACT | P_MAC);
  if (!pres) pres = P_setprot(TEMPORARY,"tn",P_MAC);
  if (!pres) pres = P_setprot(TEMPORARY,"dn",P_MAC);
  checkresult(pres,"setprotection: setdgparms");

  /* new parameters */
/* cr1 */
  dgparm("cr1",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
/* delta1 */
  dgparm("delta1",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
/* hzmm */
  dgparm("hzmm",ST_REAL,G_DISPLAY,1e9,-1e9,0.0,P_ARR | P_ACT);
  if ((P_getreal(TEMPORARY,"wc",&wc,1)==0)&&(P_getreal(TEMPORARY,"wp",&wp,1)==0))
    P_setreal(TEMPORARY,"hzmm",wp/wc,0);
  else printf("wc or wp not found\n");
/* rplot */
  dgparm("rplot",ST_STRING,G_DISPLAY,1.0,0.0,0.0,P_ARR | P_ACT);
  pres=P_setstring(TEMPORARY,"rplot","n",0);
  checkresult(pres,"setstring");
  set_2enum("rplot","y","n");
/* trace */
  dgparm("trace",ST_STRING,G_DISPLAY,2.0,0.0,0.0,P_ARR | P_ACT);
  pres=P_setstring(TEMPORARY,"trace","f1",0);
  checkresult(pres,"setstring");
  set_2enum("trace","f1","f2");
/* array */
  dgparm("array",ST_STRING,G_ACQUISITION,256.0,0.0,0.0,P_ARR);
  pres=P_setstring(TEMPORARY,"array",arraystring,0);
  checkresult(pres,"setstring");
/* arraydim */
  dgparm("arraydim",ST_INTEGER,G_ACQUISITION,32768.0,1.0,1.0,P_VAL | P_ARR);
  pres=P_setreal(TEMPORARY,"arraydim",1.0,0);
  checkresult(pres,"setreal");
/* cf */
  dgparm("cf",ST_INTEGER,G_PROCESSING,32767.0,0.0,1.0,P_ARR);
  pres=P_setreal(TEMPORARY,"cf",1.0,0);
  checkresult(pres,"setreal");
  if (!nfflag)
    { pres=P_setactive(TEMPORARY,"cf",ACT_OFF);
      checkresult(pres,"setactive");
    }
/* lsfid */
  dgparm("lsfid",ST_INTEGER,G_PROCESSING,apsize / 2.0,0.0,1.0,P_ARR);
  pres=P_setreal(TEMPORARY,"lsfid",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"lsfid",0);
  checkresult(pres,"setactive");
/* phfid */
  dgparm("phfid",ST_REAL,G_PROCESSING,3600.0,-3600.0,0.1,P_ARR);
  pres=P_setreal(TEMPORARY,"phfid",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"phfid",0);
  checkresult(pres,"setactive");
/* sb */
  dgparm("sb",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,0,P_ARR);
  pres=P_setreal(TEMPORARY,"sb",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"sb",0);
/* sb1 */
  dgparm("sb1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"sb1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"sb1",0);
/* sbs */
  dgparm("sbs",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"sbs",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"sbs",0);
/* sbs1 */
  dgparm("sbs1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"sbs1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"sbs1",0);
/* gfs */
  dgparm("gfs",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"gfs",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"gfs",0);
/* gfs1 */
  dgparm("gfs1",ST_REAL,G_PROCESSING,1000.0,-1000.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"gfs1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"gfs1",0);
/* awc */
  dgparm("awc",ST_REAL,G_PROCESSING,1.0,-1.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"awc",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"awc",0);
/* awc1 */
  dgparm("awc1",ST_REAL,G_PROCESSING,1.0,-1.0,0.001,P_ARR);
  pres=P_setreal(TEMPORARY,"awc1",0.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"awc1",0);
/* fpmult */
  dgparm("fpmult",ST_REAL,G_PROCESSING,1.0e9,0.0,0.0,P_ARR);
  pres=P_setreal(TEMPORARY,"fpmult",1.0,0);
  checkresult(pres,"setreal");
  pres=P_setactive(TEMPORARY,"fpmult",ACT_OFF);
/* priority */
  dgparm("priority",ST_INTEGER,G_ACQUISITION,32768.0,0.0,1.0,P_ARR);
  pres=P_setreal(TEMPORARY,"priority",5.0,0);
  checkresult(pres,"setreal");
  /* now set aig and dcg to defaults */
  if (parlibpar)
    parlibfound = (P_getVarInfo(TEMPORARY,"aig",&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_setlimits(TEMPORARY,"aig",2.0,0.0,0.0);
    checkresult(pres,"setlimits");
    pres=P_setstring(TEMPORARY,"aig","nm",0);
    checkresult(pres,"setstring");
    set_2enum("aig","nm","ai");
  }
  if (parlibpar)
    parlibfound = (P_getVarInfo(TEMPORARY,"dcg",&dummy)==0);
  else
    parlibfound = 0;
  if (!parlibfound)
  {
    pres=P_setlimits(TEMPORARY,"dcg",3.0,0.0,0.0);
    checkresult(pres,"setlimits");
    pres=P_setstring(TEMPORARY,"dcg","cdc",0);
    checkresult(pres,"setstring");
    set_2enum("dcg","dc","cdc");
  }
  if ((reval > 0.0) && (P_getVarInfo(TEMPORARY,"lb",&dummy)==0))
    if (dummy.active == ACT_OFF)
    {
      pres=P_setreal(TEMPORARY,"lb",-1.0 /(3.14159265358979323846 * reval),0);
      checkresult(pres,"setreal");
      pres=P_setactive(TEMPORARY,"lb",ACT_ON);
    }
  if ((re2val > 0.0) && (P_getVarInfo(TEMPORARY,"lb1",&dummy)==0))
    if (dummy.active == ACT_OFF)
    {
      pres=P_setreal(TEMPORARY,"lb1",-1.0 /(3.14159265358979323846 * re2val),0);
      checkresult(pres,"setreal");
      pres=P_setactive(TEMPORARY,"lb1",ACT_ON);
    }
  /* now make dg templates */
  /* create the variable */
  dgparm("dg",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) printf("creating dg string\n");
    strcpy(t,"1:ACQUISITION:sfrq:3,tn,at:3,np:0,sw:1,fb:0,bs:0,ss:0,pw:1,p1:1,d1:3,d2:3,tof:1,nt:0,ct:0;");
    strcat(t,"2:SAMPLE:date,solvent,file;");
    strcat(t,"2:DECOUPLING:dn,dof:1,dm,dmm,dmf,dhp:0,dlp:0;");
    strcat(t,"2(ni):2D ACQUISITION:sw1:1,ni:0;");
    strcat(t,"3:PROCESSING:cf(nf):0,lb:2,sb:3,sbs(sb):3,gf:3,gfs(gf):3,awc:3,lsfid:0,phfid:1,fn:0,math,,werr,wexp,wbs,wnt;");
    strcat(t,"4(ni):2D PROCESSING:lb1:2,sb1:3,sbs1(sb1):3,gf1:3,gfs1(gf1):3,awc1:3,fn1:0;");
    strcat(t,"4:FLAGS:il,in,dp,hs;");
    strcat(t,"4:SPECIAL:temp:1,priority:0;");
    if (debug1) printf("%s\n",t);
    /* set the value */
    pres=P_setstring(TEMPORARY,"dg",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("dg1",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) printf("creating dg1 string\n");
    strcpy(t,"1:DISPLAY:sp:1,wp:1,vs:0;");
    strcat(t,"1:REFERENCE:rfl:1,rfp:1,cr:1,delta:1;");
    strcat(t,"1:PHASE:lp:1,rp:1,rp1(ni):1,lp1(ni):1;");
    strcat(t,"2:CHART:sc:0,wc:0,hzmm:2,vp:0,axis,pltmod,,th:0,,ho:2,vo:2,,rplot,trace:2;");
    strcat(t,"3(ni):2D:sp1:1,wp1:1,sc2:0,wc2:0,rfl1:1,rfp1:1;");
    strcat(t,"3:FLAGS:aig*,dcg*,dmg*;");
    strcat(t,"3:FID:sf:3,wf:3,vf:0;");
    strcat(t,"4:INTEGRAL:intmod,is:2,ins:3,io:0,,lvl:3,tlt:3;");
    if (debug1) printf("%s\n",t);
    /* set the value */
    pres=P_setstring(TEMPORARY,"dg1",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("dgs",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) printf("creating dgs string\n");
    strcpy(t,"1:AXIAL SHIMS:z1:0,z1c:0,z2:0,z2c:0,z3:0,z4:0,z5(z5flag):0;");
    strcat(t,"2:NON AXIAL SHIMS:x1:0,y1:0,xz:0,yz:0,xy:0,x2y2:0,x3:0,y3:0,xz2(z5flag):0,yz2(z5flag):0,zxy(z5flag):0,zx2y2(z5flag):0;");
    strcat(t,"3:AUTOMATION:method,wshim,load,,spin:0,gain:0,loc:0;");
    strcat(t,"3:SPECIAL:temp;");
    strcat(t,"4:MACROS:r1,r2,r3,r4,r5,r6,r7,n1,n2,n3,n4;");
    if (debug1) printf("%s\n",t);
    /* set the value */
    pres=P_setstring(TEMPORARY,"dgs",t,0);
    checkresult(pres,"setstring");
  }
  /* create the variable */
  dgparm("ap",ST_STRING,G_DISPLAY,1023.0,0.0,0.0,P_ARR | P_VAL | P_ACT);
  if (!parlibfound)
  {
    if (debug1) printf("creating ap string\n");
    strcpy(t,"1:SAMPLE:date,solvent,file;");
    strcat(t,"1:ACQUISITION:sfrq:3,tn:2,at:3,np:0,sw:1,fb:0,bs(bs):0,ss(ss):0,pw:1,p1(p1):1,d1:3,d2(d2):3,tof:1,nt:0,ct:0,alock,gain:0;");
    strcat(t,"1:FLAGS:il,in,dp,hs;");
    strcat(t,"1(ni):2D ACQUISITION:sw1:1,ni:0;");
    strcat(t,"1(ni):2D DISPLAY:sp1:1,wp1:1,sc2:0,wc2:0,rfl1:1,rfp1:1;");
    strcat(t,"2:DEC. & VT:dn:2,dof:1,dm,dmm,dmf,dhp(dhp):0,dlp(dlp):0,temp(temp):1;");
    strcat(t,"2:PROCESSING:cf(cf):0,lb(lb):2,sb(sb):3,sbs(sb):3,gf(gf):3,gfs(gf):3,awc(awc):3,lsfid(lsfid):0,phfid(phfid):1,fn:0,math,,werr,wexp,wbs,wnt;");
    strcat(t,"2(ni):2D PROCESSING:lb1(lb1):2,sb1(sb1):3,sbs1(sb1):3,gf1(gf1):3,gfs1(gf1):3,awc1(awc1):3,fn1:0;");
    strcat(t,"2:DISPLAY:sp:1,wp:1,vs:0,sc:0,wc:0,hzmm:2,is:2,rfl:1,rfp:1,th:0,ins:3,aig*,dcg*,dmg*;");
    if (debug1) printf("%s\n",t);
    /* set the value */
    pres=P_setstring(TEMPORARY,"ap",t,0);
    checkresult(pres,"setstring");
  }
}

/******************/
storeparams()
/******************/
{
  char parampath[MAXPATH];
  int  pres;

/* save parameters in procpar file */

  strcpy(parampath,curfiddir);
#ifdef UNIX
  strcat(parampath,"/procpar");
#else
  strcat(parampath,"procpar");
#endif
  pres=P_save(TEMPORARY,parampath);
  checkresult(pres,"save");
}

/********/
static string_lower_case(s)
/********/
/* convert all characters in string to lower case */
char *s;
{ int i;
  i=0;
  while (s[i])
    {  if ((s[i]>='A')&&(s[i]<='Z')) s[i]=s[i]+'a'-'A';
       i++;
    }
}

/*********/
static noblanks(s)
/*********/
/* remove all blanks at end of string */
char *s;
{ int i;
  i = strlen(s)-1;
  while ((i>=0)&&(s[i]==' '))
    {  s[i]='\000';
       i--;
    }
}

/*****************************/
static find(fx,firstsec,numsec,name,f)
/*****************************/
/* find file entry with specified name in xl type directory */
int fx,firstsec,numsec;
char *name;
struct old_fcb *f;
{
  int sec,found,index,i1,i2,i3;
  char name1[64];
  char *n;
  found = 0;
  sec = firstsec;
  index = 0;
  while ((found==0) && (sec<firstsec+numsec))	/* go through the dir */
    { if (index==0)
        { if (sec!=curdirpage)
          { getdata(fx,&dirpage,sec,1);
	    curdirpage = sec;
          }
        }
      if ((dirpage.mnemarray[index].n[ZERO] & 0x80) == 0 )
	{ /* convert a trn field */
	  n=dirpage.mnemarray[index].n;
	  if (n[ONE]<0) i1 = 256*n[ZERO] + n[ONE] + 256;
	         else i1 = 256*n[ZERO] + n[ONE];
	  if (n[2+ONE]<0) i2 = 256*n[2+ZERO] + n[2+ONE] + 256;
	         else i2 = 256*n[2+ZERO] + n[2+ONE];
	  if (n[4+ONE]<0) i3 = 256*n[4+ZERO] + n[4+ONE] + 256;
	         else i3 = 256*n[4+ZERO] + n[4+ONE];
	  sprintf(name1,"%4d.%4d.%4d",i1,i2,i3);
	}
      else /* convert a name */
          cpos_to_vax(dirpage.mnemarray[index].n,name1,6);
      if (strcmp(name,name1)==0) 
	{
	  found = 1;					/* found it */
	  *f = dirpage.mnemarray[index].f;		/* give back fcb */
	}
      index++;
      if (index>LASTMNEM)
	{ index = 0;
	  sec++;
	}
    }
  return(found);
}

/*************/
static storedata(fr,f)
/*************/
int fr;
struct old_fcb *f;
#define CTLOW  18	/* position of CT counter in fid */
#define CTHIGH 17
#define NPLOW  14	/* position of NP counter in fid */
#define NPHIGH 13
#define DPFLAG 20	/* position of double precision flag in fid */
#define SCALE  25	/* position of scale in fid */
{ int dim1,dim2,dim3,dima,i1,i2,i3,ii,r;
  int dpf,compressed,fid_sectors;
  int codeok = 1;
  struct old_fcb f1,acqdat;
  char fidname[64];
  char dpflag[4];
  short *allocateWithId();
  short *acodes;
  short *wptr;
  register int tmp,i;
  char filepath[MAXPATH];
  int bi,nf;
  struct datafilehead datahead;
  struct datapointers bpntrs;
  double rnp,rnf;

  r=P_getreal(TEMPORARY,"nf",&rnf,1);
  if (r==0)
    nf = (int) (rnf + 0.001);
  else
    nf = 1;
/*set_nodata();*/
  /* open fid file */
  D_close(D_USERFILE); 
  strcpy(filepath,curfiddir);
#ifdef UNIX
  strcat(filepath,"/fid");
#else
  strcat(filepath,"fid");
#endif
  /* find DIMS */
  if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"DIMS  \000",&f1))
    if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"DIMS C\000",&f1))
    {   printf("DIMS not found\n"); 
        return( -1 ); 
    }
  dim1 = f1.i2;
  dim2 = f1.i3;
  dim3 = f1.i4;
  dima = dim1 * dim2 * dim3;
  if (debug1) printf("fid dimensions = 1:%d, 2:%d, 3:%d, total=%d\n",
	      dim1,dim2,dim3,dima);
  r=P_setreal(TEMPORARY,"arraydim",(float)dima,0);
  checkresult(r,"set arraydim");
  /* is the data set compressed? */
  compressed =  find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"ACQDAT\000",&acqdat);
  if (compressed)
  {
    printf("compressed data set\n");
    /* find the fid file */
    if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"FIDFIL\000",&f1))
    { printf("FIDFIL not found\n"); 
      return( -1 );
    }
    fid_sectors = f1.i4 / dima;
    /* allocate memory for acqdat */
    if (debug1)
      printf("fid length is %d sectors.  acqdat is %d sectors\n",
                  fid_sectors,acqdat.i4);
    if (!(acodes = allocateWithId(acqdat.i4*PAGELENGTH,"convert")))
    { printf("cannot allocate buffer for acqdat\n");
      return( -1 );
    }
    if (getdata(fr,acodes,(acqdat.i3+f->i3),acqdat.i4)==0)
    { printf("cannot read acqdat of old fid\n");
      release(acodes); return( -1 );
    }
  }
  else /* allocate memory for one acode */
    if (!(acodes = allocateWithId(3*PAGELENGTH,"convert")))
    { printf("cannot allocate buffer for code\n");
      return( -1 );
    }
  /* set up datahead */
  datahead.nblocks = dima;
  datahead.ntraces = nf;
  if (nf>1)
    printf("multiple fid traces: nf=%d\n",nf);
  if (P_getstring(TEMPORARY,"dp",dpflag,1,3))
  { printf("dp not found\n");
    release(acodes); return( -1 );
  }
  else dpf = (dpflag[0]=='y');
  if (dpf) datahead.ebytes = 4;
      else datahead.ebytes = 2;
  datahead.nbheaders = 1;
  if (dpf) datahead.status = S_DATA | S_32 | S_COMPLEX;
      else datahead.status = S_DATA | S_COMPLEX;
  datahead.vers_id = VERSION + FID_FILE;
  if (compressed)
    if (P_getreal(TEMPORARY,"np",&rnp,1))
    { printf("np not found\n");
      release(acodes); return( -1 );
    }
    else datahead.np = (int)rnp;
  bi = -1;
  for (i3=1; i3<=dim3; i3++)
    for (i2=1; i2<=dim2; i2++)
      for (i1=1; i1<=dim1; i1++)
        { bi += 1;
          if (interuption)
	    { D_close(D_USERFILE); release(acodes); return( -1 );
            }
 
          if (!compressed)
          {
            sprintf(fidname,"%4d.%4d.%4d",i1,i2,i3);
            if (find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,fidname,&f1)==0)
            { printf("cannot find old fid fcb\n");
              D_close(D_USERFILE); return( -1 );
	    }
            if (getdata(fr,acodes,(f1.i3+f->i3),3)==0)
	    { printf("cannot read code file of old fid\n");
	      D_close(D_USERFILE); release(acodes); return( -1 );
	    }
          }
          if ((i1==1)&&(i2==1)&&(i3==1))
	  { /* set up datahead */
            if (!compressed)
            {
	      datahead.np = acodes[NPLOW] + 65536 * acodes[NPHIGH];
	      if (acodes[NPLOW]<0) datahead.np += 65536;
	      if (datahead.np <32 || datahead.np >65536
	          || acodes[CTHIGH] <0 || acodes[SCALE] >16 || acodes[SCALE]<0)
	         codeok=0;
	      if (!codeok)       
	      { printf("np not stored in first fid, using parameter\n");
	        if (P_getreal(TEMPORARY,"np",&rnp,1))
	        { printf("np not found\n");
	          D_close(D_USERFILE); release(acodes); return( -1 );
	        }
                else datahead.np = (int)rnp * nf;
	      }
              datahead.np /= nf;
            }
            datahead.tbytes = datahead.ebytes * datahead.np;
	    datahead.bbytes = datahead.tbytes * datahead.ntraces +
				     sizeof(struct datablockhead);
	    if (r=D_newhead(D_USERFILE,filepath,&datahead))
	    { D_error(r);
	      D_close(D_USERFILE);
              release(acodes);
	      return( -1 );
            }
	  }
              /* now prepare storage for fid in data */
          if (r=D_allocbuf(D_USERFILE,bi,&bpntrs))
	  { D_error(r);
	    D_close(D_USERFILE);
            release(acodes);
            return( -1 );
	  }
          if (compressed)
          {
	    bpntrs.head->scale  = acodes[bi*4 + 3];
	    bpntrs.head->ctcount = acodes[bi*4 + 1] + 65536*acodes[bi*4];
	    if (acodes[bi*4 + 1]<0) bpntrs.head->ctcount += 65536;
          }
          else
          {
	    if (codeok)
	    {   bpntrs.head->scale  = acodes[SCALE];
	        bpntrs.head->ctcount = acodes[CTLOW] + 65536*acodes[CTHIGH];
	        if (acodes[CTLOW]<0) bpntrs.head->ctcount += 65536;
 	    }
	    else 
	    {   bpntrs.head->scale   = 0;
	        bpntrs.head->ctcount = 1;
	    }
          }
	  if ((!compressed && f1.i2) || (compressed && bpntrs.head->ctcount))
	  { if (dpf) bpntrs.head->status = S_DATA | S_32 | S_COMPLEX;
	        else bpntrs.head->status = S_DATA | S_COMPLEX;
	  }
	  else bpntrs.head->status = 0;
	  bpntrs.head->mode = 0;
	  bpntrs.head->rpval = 0.0;
	  bpntrs.head->lpval = 0.0;
	  bpntrs.head->lvl = 0.0;
	  bpntrs.head->tlt = 0.0;
          if (compressed)
            r = (getbytes(fr,bpntrs.data,f1.i3+f->i3+bi*fid_sectors,
                          datahead.tbytes*nf)==0);
          else
            r = (getbytes(fr,bpntrs.data,f1.i3+f->i3+3,datahead.tbytes*nf)==0);
          if (r)
	  { printf("cannot read data file of old fid\n");
	    D_close(D_USERFILE); release(acodes); return( -1 );
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
	  if (r=D_markupdated(D_USERFILE,bi))
	  { D_error(r);
	    D_close(D_USERFILE); release(acodes); return( -1 );
	  }
          if (r=D_release(D_USERFILE,bi))
	  { D_error(r);
	    D_close(D_USERFILE); release(acodes); return( -1 );
	  }
	}
  D_close(D_USERFILE);
  release(acodes);
  RETURN;
}


#define NUMBER_FID_OPTIONS	2

int cpos_convert( cpos_fnptr, unix_fnptr )
char *cpos_fnptr;
char *unix_fnptr;
{    
    int fr;    			/* from file descriptor */
    int ival;
    struct old_fcb f;
    char fidfile[ MAXPATH ];
    int d2flag;

/* open "fromfile" readonly  */  

    if (unix_fnptr == NULL) {
        fr = verify_open_CPOSfile( cpos_fnptr, &curfiddir[ 0 ], MAXPATH );
    }
    else {
        fr = verify_open_CPOSfile( cpos_fnptr, NULL, 0 );
	cposStrlcpy( unix_fnptr, &curfiddir[ 0 ], MAXPATH );
    }

    if (fr < 0) {
        return( -1 );
    }

#ifdef VMS
    make_vmstree( &curfiddir[ 0 ], &curfiddir[ 0 ], MAXPATHL );
#endif

    ival = mkdir( &curfiddir[ 0 ], 0777 );
    if (ival != 0) {
	fprintf( stderr, "cannot create %s directory\n", &curfiddir[ 0 ] );
	perror( "Unix error" );
	return( -1 );
    }

    apsize = APSIZE;
    maxswk = MAXSWK * 1e3;
    maxfn  = MAXFN;
    maxtim = MAXTIM;
    mintim = MINTIM;
    d2flag = 0;
    reval  = -1.0;
    re2val = -1.0;
    curdirpage = -1;	/* no page currently in buffer */
    arraystring[0] = '\0';
    parlibfound = 0;
    if (debug1) disdir(fr,0,1);	/* first display the fid directory */

    if (find(fr,0,1,"TEXT  \000",&f)) {	/* then find the text file */
	if (debug1) printf("\nTEXT:\n");
	distext(fr,f.i3);		/* and display it */
    }
    else {
	printf("not a good data file\n");
	return( -1 );
    }
    P_treereset(TEMPORARY);		/* clear the tree first */
#ifdef VMS
    if (find(fr,0,1,"VAXFMT\000",&f))
      cmult = 1.0;			/* values already converted */
    else
      cmult = CMULT;
#else
    cmult = CMULT;
#endif
    if (find(fr,0,1,"ACQPAR\000",&f)) {	/* find and display ACQPAR */
	if (debug1) printf("\nACQPAR:\n");
        parlibfound = 0;
	disparms(fr,f.i2,f.i3,G_ACQUISITION,&d2flag);
    }
    if (find(fr,0,1,"PRCPAR\000",&f)) {	/* find and display PRCPAR */
        if (debug1) printf("\nPRCPAR:\n");
        parlibfound = 0;
	disparms(fr,f.i2,f.i3,G_PROCESSING,&d2flag);
    }
    if (find(fr,0,1,"DISPAR\000",&f)) {	/* find and display DISPAR */
        if (debug1) printf("\nDISPAR:\n");
        parlibfound = 0;
	disparms(fr,f.i2,f.i3,G_DISPLAY,&d2flag);
    }
    printf("Convering parameters\n");
    setdgparms(d2flag);
    if (debug1) printf("setdgparms done\n");

/*  storeparams();	Storing parameters operation follows storing	*/
/*			data.  The parameter `arraydim' is calculated	*/
/*			as the data is stored.				*/

    if (find(fr,0,1,"FID   \000",&f)) {	/* find and display FID */
        if (debug1) printf("FID:\n");
        printf("Converting data\n" );
	if (debug1) disdir(fr,f.i3,(f.i2-1)/(LASTMNEM+1)+1);
	storedata(fr,&f);

    }

    storeparams();				/* See note above.	*/

    close(fr);  /* close the files */
    releaseAllWithId("convert");
/*  execString("fixpar\n");	done by the RT command  */
    return( 0 );
}

/*  The main routine for `cpos_cvt'	*/

main( argc, argv )
int argc;
char *argv[];
{
	char		*tptr, cpos_file[ MAXPATH ];
	int		 ival;
	extern char	*getenv();

	debug1 = 0;				/* set to non-zero for debug */
	D_init();
	if (argc != 2 && argc != 3) {
		fprintf( stderr, "usage: %s <CPOS FID>\n", argv[ 0 ] );
		exit();
	}

	tptr = getenv( "vnmrsystem" );
	if (tptr == NULL) {
		fprintf( stderr, "Environment variable 'vnmrsystem' not defined\n" );
		exit();
	}
	strcpy( &systemdir[ 0 ], tptr );

	tptr = getenv( "vnmruser" );
	if (tptr == NULL) {
		fprintf( stderr, "Environment variable 'vnmruser' not defined\n" );
		exit();
	}
	strcpy( &userdir[ 0 ], tptr );
	/*free( tptr );*/	/* do NOT free the address that getenv returns */
				          /* it is NOT allocated from the heap */
/*  Now use tptr as the 2nd argument to the cpos_convert routine.  */

	if (argc == 2)
	  tptr = NULL;
	else {
		if (strlen( argv[ 2 ] ) > (size_t) (MAXPATH-1)) {
			fprintf(
	    stderr, "Output file name has too many characters\n"
			);
			exit();
		}
		tptr = argv[ 2 ];
	}
	ival = cpos_convert( argv[ 1 ], tptr );
	if (ival == 0)
	  printf( "complete\n" );
}
 
/*  Computes name of the Unity-style FID, as well as opening the CPOS
    FID file.  Procedure is:

    1. Look for the original file name.  If successful, go to step 3.

    2. Append ".*".  If one file matches the wildcard specification,
       go to step 3 using that file name.  Otherwise, exit with a
       failure status.

    3. Examine the file name's suffix.  If it implys the CPOS file
       was an array, data, or a program, exit with a failure status.
       If it implies it was a directory, verify the number of entries
       is suitable for a FID.

    4. Eliminate the suffix, if present.

    5. Append .fid to complete the procedure.

    The Unity-style FID will thus be created in the same directory as
    the original CPOS FID.						*/

int verify_open_CPOSfile( sname, fname, flen )
char *sname;
char *fname;
int flen;
{
	char	fext[ 20 ], pname[ 256 ], tmpbuf[ MAXPATH ];
	int	ival, num_entries;

/*  Use find1file() to find the file implied by the argument.  */

	if (strlen( sname ) < (size_t) 1) {
		printf( "Null file name not allowed\n" );
		return( -1 );
	}

/*  `find1file' opens the CPOS file, returning the FD if successful.  */

	if ((ival = find1file( sname, &pname[ 0 ], 254 )) < 0)
	{
		if (ival == -3 || ival == -4)
		 printf( "File cannot be found\n" );
		else if (ival == -5)
		 printf( "Too many files match\n" );
		else
		 printf( "Someone made a big mistake, fd = %d\n", ival );

		return( -1 );
	}

	is_ext( &pname[ 0 ], &fext[ 0 ], 20 );
	num_entries = verifyExtension( &fext[ 0 ] );
	if (num_entries < 0) {
		printf( "UNIX file name implies an invalid CPOS file type\n" );
		return( -1 );
	}
	else if (num_entries > LASTMNEM + 1) {
		printf( "CPOS directory has too many entries to be a FID\n" );
		return( -1 );
	}

/*  Only compute the corresponding UNIX file name
    if the address for it is not NULL.			*/

	if (fname == NULL) {
		return( ival );
	}

	if (unix_fsplit( &pname[ 0 ],
			 &dir_with_cpos[ 0 ], MAXPATH-2,
			 &tmpbuf[ 0 ], flen-4			/* room for .fid */
	)) {
		printf( "programming error 1 while opening CPOS file\n" );
		return( -1 );
	}
	if (remove_extension( &tmpbuf[ 0 ], fname, flen-4 )) {	/* room for .fid */
		printf( "programming error 2 while opening CPOS file\n" );
		return( -1 );
	}
#ifdef VMS
	strcat( fname, "_fid" );
#else
	strcat( fname, ".fid" );
#endif
	return( ival );			/* The FD for the CPOS file */
}

int remove_extension( sname, fname, flen )
char *sname;
char *fname;
int flen;
{
	char	*slash_ptr, *dot_ptr;
	int	 xfer_len;

	if (sname == NULL)
	  return( -1 );
	if (fname == NULL)
	  return( -1 );
	if (strlen( sname ) < (size_t) 1)
	  return( -1 );

/*  Find last '/' character; then find last '.' character.  */

	slash_ptr = strrchr( sname, '/' );
	if (slash_ptr == NULL)
	  slash_ptr = sname;
	dot_ptr = strrchr( slash_ptr, '.' );
	if (dot_ptr == NULL)
	  xfer_len = strlen( sname );
	else
	  xfer_len = dot_ptr-sname;

	if (xfer_len >= flen)
	  return( -1 );

	strncpy( fname, sname, xfer_len );
	*(fname+xfer_len) = '\0';
	return( 0 );
}

/****************************************************************/
/*  What follows was taken from decomp.c, SCCS category BIN.	*/
/*  All subroutines work on both UNIX and VMS.  Some routines   */
/*  have UNIX in their name and many comments imply the code    */
/*  only works on a UNIX file system.  This is no longer true.	*/
/****************************************************************/

/*  Verifies the UNIX file extension.
    Returns -1 if unacceptable.
    Returns 0 if acceptable, but the number of directory entries
cannot be determined from the file extension.
    Otherwise, the file extension is the decimal representation of
the number of CPOS entries and the function returns this value.  Note
that a negative value will not be returned in I2 by EXTR_I1_I2().	*/

int verifyExtension( extp )
char *extp;
{
	short		i1, i2;
	int		iter;

/*  Extract the extension; convert to I1 and I2 values  */

	extr_i1_i2( extp, &i1, &i2 );
	if (i1 == ARRAY_ID || i1 == PROG_ID) return( -1 );
	else if (i1 == DIRECTORY_ID) return( i2 );
	else if (strlen( &extp[ 0 ] ) < (size_t) 3) return( 0 );

/*  DATA_ID is used as the default CPOS file type.  We want to reject
    the file if its extension starts with ".DAT" or some variation
    with lower-case letters.						*/

	else {
		for (iter = 0; iter < 3; iter++)
		 if ( (*(extp++) & ~0x20) != "DAT"[ iter ] )
		 return( 0 );
		return( -1 );
	}
}

/*  Following procedure finds exactly 1 file, using this procedure.

    The input name is searched for.  If successful, that file is opened.
    Otherwise, the wildcard specification ".*" is appended and a file
    matching that specification is searched for. Only ONE file can match
    the latter specification; if two or more match, an error is reported.  */

int find1file( input_pname, final_pname, final_plen )
char *input_pname, *final_pname;
int final_plen;
{
	char		local_pname[ 256 ];
	int		input_plen, ival, tfd, want_pname;

	if ( (input_plen = strlen( input_pname )) < 1 ) return( -3 );
	else if ( input_plen > 255 ) 			return( -1 );

	want_pname = (final_pname != NULL && final_plen > 0);

/*  Try to open the file.  If successful, return the file descriptor.  */

	tfd = open( input_pname, 0 );
	if (tfd >= 0) {
		if (want_pname)
		 cposStrlcpy( input_pname, final_pname, final_plen );
		return( tfd );
	}

/*  If the file was not found and the path name has an extension,
    return error code indicating not found.				*/

	if (is_ext( input_pname, NULL, 0 )) return( -3 );

/*  If no extension, append ".*" to the name and use UNIX_SEARCH.  */

	cposStrlcpy( input_pname, &local_pname[ 0 ], 256 );
	strcat( &local_pname[ input_plen ], ".*" );
	ival = unix_search( &local_pname[ 0 ], final_pname, final_plen );
	if (ival == 0)
	 return( open( final_pname, 0 ) );
	else
	 return( ival );
}

/*  Searches for a UNIX file.  If successful, returns 0.  Accepts ordinary
    UNIX path names and wild card specifications of the form "X.*", where
    X is a valid UNIX file name.

    Error returns:
	-1		Original path name too long
	-2 		Invalid wild card operation, e. g. "*.c"
	-3		File not found
	-4		Directory not found (wildcard operations only)
	-5		More than 1 file matches the wildcard spec	*/

int unix_search( input_ptr, output_ptr, output_len )
char *input_ptr;
char *output_ptr;
int output_len;
{
	char		dname[ 256 ], fname[ 80 ], res_name[ 80 ], *tptr;
	int		dlen, flen, ilen, ival, rval, star_present, tfd,
			want_output;
	DIR		*dstream_ptr;

	ilen = strlen( input_ptr );
	if (ilen < 1) return( -3 );		/*  No null file names  */
	else if (ilen > 255) return( -1 );

	want_output = (output_ptr != NULL && output_len > 0);
	ival = unix_fsplit( input_ptr, &dname[ 0 ], 256, &fname[ 0 ], 80 );

/*  Look for the wildcard charater.  If found, it must be the last
    character in the string, and the previous character must be a
    dot.  If either is not true, return an error condition.		*/

	flen = strlen( tptr = &fname[ 0 ] );
	star_present = 0;
	while (*tptr) {
		if (*tptr == '*') {
			if ( *(tptr+1) != '\000' )	return( -2 );
			else if ( tptr == &fname[ 0 ] ) return( -2 );
			else if ( *(tptr-1) != '.' )	return( -2 );
			else star_present = 131071;
		}

/*  If the wildcard character was found, the next character in the string
    has to be the null character.  Thus incrementing the pointer will
    cause the WHILE loop to exit, just by checking the current character.  */
	
		tptr++;
	}

/*  If no wildcard character was found, complete the search by attempting
    to open the original file specification.				*/

	if ( !star_present ) {
		tfd = open( input_ptr, 0 );
		if (tfd < 0) return( -3 );

		close( tfd );
		if (want_output)
		 cposStrlcpy( input_ptr, output_ptr, output_len );
		return( 0 );
	}

/*  Wildcard found, presumably valid.  */

/*  For UNIX, it is necessary to append a dot, to search the
    contents of the directory.  The VMS version of `opendir'
    and `readdir' understand that a bare directory spec
    implies that directory is to be searched.			*/

#ifdef UNIX
	strcat( &dname[ 0 ], "." );
#endif
	dstream_ptr = opendir( &dname[ 0 ] );
	if (dstream_ptr == NULL) return( -4 );

/*  Search the directory in a separate routine.  */

	rval = unix_dirsearch( dstream_ptr, &fname[ 0 ], &res_name[ 0 ], 80 );
	if (rval != 0 || !want_output) return( rval );

/*  Before copying the directory name, erase the
    "." we added earlier (UNIX version only)    */

	if ( (dlen = strlen( &dname[ 0 ] )) > 1 ) {
#ifdef UNIX					/* remove dot appended above */
		dname[ --dlen ] = '\000';
#endif
		cposStrlcpy( &dname[ 0 ], output_ptr, output_len );
		output_len = output_len - dlen;
		if (output_len < 1) return( rval );
		else output_ptr += dlen;
	}

	cposStrlcpy( &res_name[ 0 ], output_ptr, output_len );
	return( 0 );
}

int unix_dirsearch( ds_ptr, fs_ptr, fn_ptr, fn_len )
DIR *ds_ptr;
char *fs_ptr;
char *fn_ptr;
int fn_len;
{
	char		*iptr, *cptr, safe_place[ 256 ];
	int		fs_len, matched, ret_val, star_present, want_match;
	struct dirent	*cur_entry;

	fs_len = strlen( iptr = fs_ptr );
	if (fs_len < 1) return( -3 );
	else if (fs_len > 255) return( -1 );

/*  This check means we can call UNIX_DIRSEARCH from routines other
    than UNIX_SEARCH.  It verifies that if the file spec to be matched
    has a wild card, the wild card character is the last character.	*/

	star_present = 0;
	while (*iptr) {
		if (*iptr == '*') {
			if ( *(iptr+1) != '\000' ) return( -2 );
			else star_present = 131071;
		}
		iptr++;
	}

	want_match = (fn_ptr != NULL && fn_len > 0);
	ret_val = -3;					/*  not found  */
	do {
		cur_entry = readdir( ds_ptr );
		if (cur_entry == NULL) break;

		iptr = fs_ptr;
		cptr = &cur_entry->d_name[ 0 ];

/*  Compare the current entry with the input specification.
    The instructions below assume there is only one wild card
    character in the specification.				*/

		matched = 0;
		do {
			if (*iptr == '*')
			 matched = 131071;
			else if (*iptr == '\000' && *cptr == '\000')
			 matched = 131071;

/*  VMS (FILES-11) makes no distinction between
    upper and lower case characters.  UNIX does.  */

#ifdef VMS
			else {
				char		tchr_1, tchr_2;

				tchr_1 = *iptr & ~0x20;
				tchr_2 = *cptr & ~0x20;
				if (tchr_1 != tchr_2)
				  break;
			}
#else
			else if (*iptr != *cptr) break;
#endif
		} while (*(iptr++) && *(cptr++) && !matched);	

/*  More than one match?  Return -5 if so.  */

		if (matched) {
			if (ret_val == -3) ret_val = 0;
			else		   ret_val = -5;
			if (ret_val == 0 && want_match)
			 cposStrlcpy(
				&cur_entry->d_name[ 0 ],
				&safe_place[ 0 ], 256
			 );
		}

/*  CUR_ENTRY == NULL was checked earlier...  */

	} while (ret_val != -5);

	closedir( ds_ptr );
	if (ret_val == 0 && want_match)
	 cposStrlcpy( &safe_place[ 0 ], fn_ptr, fn_len );

	return( ret_val );
}

/*  Take the UNIX path name in PNAME and splits it into the directory
    and file names.  Length of the latter strings is governed by DLEN
    and FLEN.  Thus:

	Input:      /usr2/nmr/robert/test01.c
        directory:  /usr2/nmr/robert/
        file name:  test01.c

    This routine follows the UNIX convention by returning 0 if successful
    and a negative value if an error occurs.  Either DNAME or FNAME can
    be NULL, provided the corrsponding length value is 0.		*/

int unix_fsplit( pname, dname, dlen, fname, flen )
char *pname;
char *dname;
int dlen;
char *fname;
int flen;
{
	char		*tptr;
	int		finished, index, plen;

	plen = strlen( pname );
	if (plen < 1) return( 0 );
	else if (plen > 255) return( -1 );		/*  Too long !!  */

/*  Locate a slash, if it is present.  */

	tptr = pname + (plen-1);
	finished = 0;
	while (tptr >= pname && !finished)
#ifdef UNIX
	 if (*tptr == '/') finished = 131071;
#else
	 if (*tptr == ']') finished = 131071;
#endif
	 else tptr--;

/*  Copy the directory path, including the slash.  If no slash is present,
    a null character is placed in the directory name.			*/

	tptr++;
	if (dname != NULL && dlen > 0)			/*  I promised !!  */
	 if (finished) {
		index = 1;
		while (index < dlen && pname < tptr) {
			*(dname++) = *(pname++);
			index++;
		}
		*dname = '\000';
	 }
	 else *dname = '\000';

/*  Copy the file name.  */

	if (fname != NULL && flen > 0)
	 cposStrlcpy( tptr, fname, flen );

	return( 0 );
}

/*  UNIX version has to account for distinction between upper and lower
    case characters.  "prg", "Prg", "PRG", etc are all accepted as
    program files.  Avoid inconsistancy by skipping the "." if it is
    the 1st character in the extension.					*/

extr_i1_i2( ext_ptr, i1_addr, i2_addr )
char *ext_ptr;
short *i1_addr, *i2_addr;
{
	char		cur_char;
	int		array_flag, i1_val, iter, ival;

	if ( *ext_ptr == '.' ) ext_ptr++;

/*  Work around SUN's failure to provide a version of toupper()
    that functions correctly with non-lower case characters.		*/

	cur_char = *ext_ptr;
	if (cur_char >= 'a' && cur_char <= 'z') cur_char = cur_char - 0x20;
	if ( 'A' == cur_char ) {
		array_flag = 131071;
		ext_ptr++;
	}
	else
	 array_flag = 0;

	if (isdigit( *ext_ptr )) {
		ival = 0;
		do
		 ival = ival*10 + (int) (*(ext_ptr++) - '0');
		while (isdigit( *ext_ptr ) && ( *ext_ptr ));

		*i1_addr = (array_flag) ? ARRAY_ID : DIRECTORY_ID;
		*i2_addr = (short) ival;
	}

/*  If we make it here and ARRAY_FLAG was set, the extension is not
    supported and defaults to DATA.					*/

	else {
		*i2_addr = 0;
		if (array_flag) {
			*i1_addr = DATA_ID;
			return;
		}
		i1_val = PROG_ID;
		for (iter = 0; iter < 3; iter++) {
			cur_char = *ext_ptr;
			if (cur_char >= 'a' && cur_char <= 'z')
			 cur_char = cur_char - 0x20;
			if ( "PRG"[ iter ] == cur_char )
		 	 ext_ptr++;
			else {
				i1_val = DATA_ID;
				break;
			}
		}

		*i1_addr = (short) i1_val;
	}
}

int is_ext( fil_nptr, ext_ptr, ext_len )
char *fil_nptr, *ext_ptr;
int ext_len;
{
	char		*t_ptr;
	int		fil_nlen, finished, index;

/*  If the file name is not NULL, then T_PTR is set to the address of the
    the last non-null character in the name.  The routine then uses this
    pointer to scan the file name backwards until a '/' or a '.' is found,
    or until the process reaches the beginning of the string.		*/

	if ( (fil_nlen = strlen( fil_nptr )) > 0) {
		t_ptr = fil_nptr + (fil_nlen-1);
		finished = 0;
		while ( t_ptr >= fil_nptr && !finished )
#ifdef UNIX
		 if (*t_ptr == '/' || *t_ptr == '.') finished = 262143;
#else
		 if (*t_ptr == ']' || *t_ptr == '.') finished = 262143;
#endif
		 else t_ptr--;

/*  If a '.' was found, copy all the trailing characters in the file
    name to the extension, but do not overwrite the end of this buffer.

    Note:  "test01." has an extension; "test01" does not.		*/

		if (finished)
		 if ( *t_ptr == '.' ) {
			if (ext_len < 1 || ext_ptr == NULL) return( 1 );
			t_ptr++;		/*  Skip the dot  */
			index = 1;		/*  Leave room for null  */
			while ( index < ext_len && (*t_ptr) ) {
				*(ext_ptr++) = *(t_ptr++);
				index++;
			}
			*ext_ptr = '\000';	/* Terminate extension  */
			return( 1 );
		 }
	}

/*  Otherwise, place a null character in the extension buffer  */

	if (ext_len > 0) *ext_ptr = '\000';
	return( 0 );
}

#define  BYTES_PER_BLOCK	512

int get_fsize_fd( fd )
int fd;
{
	int		ival;
	struct stat	unix_fab;

	ival = fstat( fd, &unix_fab );
	if (ival != 0) {
		perror( "FSTAT call failed" );
		exit( 3 );
	}

	return( (unix_fab.st_size+BYTES_PER_BLOCK-1) >> 9 );
}

/*  STRLCPY -  String copy, limited.

    This subroutine is like STRNCPY, except that copying stops when
    either the output limit is reached or the entire input string is
    copied, whichever occurs first.  Since a a NUL character is always
    appended to the end of the output, a maximum of LEN-1 non-NUL
    characters can be copied.  The output is NOT padded with additional
    NUL characters, nor is any value returned.				*/

void cposStrlcpy(char *p1, char *p2, int l )
{
	int		index;

/*  Defensive programming...  */

	if (p1 == NULL) return;
	if (p2 == NULL) return;
	if (l < 1) return;

	index = 1;				    /*  Leave room for NUL  */
	while (index < l && *p1) {
		*(p2++) = *(p1++);
		index++;
	}
	*p2 = '\000';
}

