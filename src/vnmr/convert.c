/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/*								*/
/*  convert.c - convert a VXR fid file into a UNIX fid file	*/
/*								*/
/*	convert(filename)					*/
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

#include "vnmrsys.h"
#ifdef UNIX
#include <sys/file.h>   /* contains open constants */
#else
#include <file.h>
#include "unix_io.h"	/* VMS:  Use routines that work right. */
#endif
#include <strings.h>
#include <stdio.h>
#include "data.h"
#include "group.h"
#include "params.h"
#include "variables.h"

extern int debug1,interuption;
extern int specIndex;

#define	LASTMNEM    35	/* last entry per page in xl type directory	*/ 
#define PAGELENGTH 512	/* number of bytes in xl type directory 	*/
#define OLD_EOM	    25	/* end of message character in xl text file 	*/
#define OLD_EOL	    10	/* end of line character in xl text file 	*/
#define MAXARRAY    18  /* maximun number of array elements             */

#ifdef SUN							/* 680xx */
#define ZERO	     0  /* 1 on VAX, 0 on 68020 */
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
static int decplaces,mult;		/* parameter characteristics */
static float max,min,step;		/* parameter characteristics */
static double apsize,maxswk,maxfn,maxtim,mintim,cmult;
static double reval,re2val;

extern int parlibpar,parlibfound;	/* moved to SREAD.C */
         
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
    { perror("convert:");
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
    { perror("convert:");
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
            { Wscrprintf("%8s %3d %3d %8d %8d %8d\n",name,
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
  char tfilepath[MAXPATHL];
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
  Wscrprintf("%s",upage); 
  /* open "curexpdir/text"  writeonly, create if does not exist */
  strcpy(tfilepath,curexpdir);
#ifdef UNIX
  strcat(tfilepath,"/text");
#else
  strcat(tfilepath,"text");
#endif
  if(textfile=fopen(tfilepath,"w+"))
    { fprintf(textfile,"%s",upage);
      fclose(textfile);
      chmod(tfilepath,0660);
    }
  else
    { Werrprintf("unix system error");
      perror("convert");
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
      { Werrprintf("Error %d in parameter routine %s",res,info);
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

  if (debug1) Wscrprintf(" ...%s..., max=%g\n",name1,max);
  /* create the variable */
  pres=SP_creatvar(CURRENT,name,ST_STRING);
  checkresult(pres,"creatvar");
  /* set the limits */
  pres=SP_setlimits(CURRENT,name,max,0.0,0.0);
  checkresult(pres,"setlimits");
  /* set the value */
  pres=P_setstring(CURRENT,name,name1,element);
  checkresult(pres,"setstring");
  if (pres) Wscrprintf("name='%s' ...%s..., max=%g\n",name,name1,max);
}

/*************************/
static par1_set(name,name1,element,active)
/*************************/
char name[],name1[];
int  element,active;
{ int pres;
  int protection;

  if (debug1) Wscrprintf(" string ");
  max = 8;
  tolower(name1);
  noblanks(name1);
  string_var(name,name1,element);
  pres=SP_setgroup(CURRENT,name,G_ACQUISITION);
  checkresult(pres,"setgroup");
  protection = (active == 'X') ? P_ACT : 0;
  protection |= P_ACT;
  pres = SP_setprot(CURRENT,name,protection);
  checkresult(pres,"setprotection");
}

/*************************/
static par18_set(name,rvalp,element)
/*************************/
char name[];
struct old_rentry *rvalp;
int  element;
{ char name1[8];

  if (debug1) Wscrprintf(" string ");
  cpos_to_vax(&(rvalp->q[2]),name1,6);
  tolower(name1);
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

  if (debug1) Wscrprintf(" string ");
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

  pres=SP_Esetstring(CURRENT,name,val1,pos);
  checkresult(pres,"Esetstring");
}

/************************/
static set_2enum(name,val1,val2)
/************************/
char name[],val1[],val2[];
{
  int pres;

  pres=SP_Esetstring(CURRENT,name,val1,1); /* possible values */
  checkresult(pres,"Esetstring");
  pres=SP_Esetstring(CURRENT,name,val2,2);
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

  if (debug1) Wscrprintf(" string\n");
  if ((max<1)||(max>4)) max=1;
  xx=max;
  cpos_to_vax(&(rvalp->q[4]),name1,xx);
  name1[(int)max]='\000'; /* if max is odd */
  tolower(name1);
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
            pres=P_setstring(CURRENT,name,"yn",1);
        else
            pres=P_setstring(CURRENT,name,"nn",1);
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
            pres=P_setstring(CURRENT,name,"hh",1);
        else
            pres=P_setstring(CURRENT,name,"ll",1);
        checkresult(pres,"setstring syn");
    }
  }
  else if (strcmp(name,"math")==0)
  {
    set_2enum(name,"d","f");
    pres=SP_Esetstring(CURRENT,name,"i",3);
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
    Wscrprintf(" real\n");
    sprintf(name1," %%8.%1df\n",decplaces);
    Wscrprintf(name1,mult*cmult*rvalp->r);
    Wscrprintf(",mx=%8.2e,mn=%8.2e,st=%8.2e\n",max,min,step);
  }
  /* first create the variable */
  if      (egroup=='A')  pres=SP_creatvar(CURRENT,name,ST_PULSE);
  else if (egroup=='B')  pres=SP_creatvar(CURRENT,name,ST_DELAY);
  else if (egroup=='I')  pres=SP_creatvar(CURRENT,name,ST_FREQUENCY);
  else if (egroup=='J')  pres=SP_creatvar(CURRENT,name,ST_FREQUENCY);
  else if (decplaces==0) pres=SP_creatvar(CURRENT,name,ST_INTEGER);
  else                   pres=SP_creatvar(CURRENT,name,ST_REAL);
  checkresult(pres,"creatvar");
  /* set the limits */
  pres=SP_setlimits(CURRENT,name,max,min,step);
  checkresult(pres,"setlimits");
  /* set the value */
  pres=P_setreal(CURRENT,name,(double)mult*cmult*rvalp->r,element);
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
  bcopy( tptr, rvalp, sizeof( rvalbuf) );

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
      bcopy( tptr, rvalp, sizeof( rvalbuf) );

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
   starting at specified page and puts variables into the CURRENT tree */
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
  char parlibpath[MAXPATHL];
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
	   bcopy( tptr, rvalp, sizeof( rvalbuf) );

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
                 tolower(name1);
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
                 parlibpar = (P_read(CURRENT,parlibpath) == 0);
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
                     parlibpar = (P_read(CURRENT,parlibpath) == 0);
                   }
                 if (parlibpar==0)
                   { strcpy(parlibpath,systemdir);
#ifdef UNIX
                     strcat(parlibpath,"/parlib/s2pul.par/procpar");
#else
		     vms_fname_cat(parlibpath,"[.parlib.s2pul_par]procpar");
#endif
                     parlibpar = (P_read(CURRENT,parlibpath) == 0);
                   }
                 if (parlibpar)
                   Wscrprintf("using parameter file %s\n",parlibpath);
                 else 
                   Wscrprintf("not using parameter file %s\n",parlibpath);
               }
	     else          strcpy(name,"SEQFIL");
	     partype = 1;	/* to mark these special params */
	     }
           tolower(name);
	   noblanks(name);
           /* display name, display group, active flag, and param type*/
	   if (debug1)
             Wscrprintf("%s,  dg%1d, act=%1c, tp=%3d, eg=%1c\n",
                         name,dgroup,active,partype,egroup);
	   /* change names of certain variables */
	   if      (strcmp(name,"sp2"   )==0) strcpy(name,"sp1");
	   else if (strcmp(name,"wp2"   )==0) strcpy(name,"wp1");
	   else if (strcmp(name,"sw2"   )==0) strcpy(name,"sw1");
	   else if (strcmp(name,"rfl2"  )==0) strcpy(name,"rfl1");
	   else if (strcmp(name,"rfp2"  )==0) strcpy(name,"rfp1");
	   else if (strcmp(name,"lp2"   )==0) strcpy(name,"lp1");
	   else if (strcmp(name,"rp2"   )==0) strcpy(name,"rp1");
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
           {  parlibfound = (P_getVarInfo(CURRENT,name,&dummy)==0);
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
	   if (strcmp(name,"vp")==0)
             rvalp->r = 0.0;
	   if (debug1)
             Wscrprintf("%s,  dg%1d, act=%1c, tp=%3d, eg=%1c\n",
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
	     { pres=SP_creatvar(CURRENT,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(CURRENT,name,4.0,0.0,0.0);
	       checkresult(pres,"setlimits");
 	       rval= mult*cmult*rvalp->r;
 	       if (rval >=1.0 && rval<=1.999)
	         pres=P_setstring(CURRENT,name,"H1",0);
	       else if (rval >=2.0 && rval<=2.999)
	         pres=P_setstring(CURRENT,name,"H2",0);
	       else if (rval >=3.0 && rval<=3.999)
	         pres=P_setstring(CURRENT,name,"H3",0);
	       else if (rval >=13.0 && rval<=13.999)
	         pres=P_setstring(CURRENT,name,"C13",0);
	       else if (rval >=14.0 && rval<=14.999) 
	         pres=P_setstring(CURRENT,name,"N14",0);
	       else if (rval >=15.0 && rval<=15.999) 
	         pres=P_setstring(CURRENT,name,"N15",0);
	       else if (rval >=17.0 && rval<=17.999)
	         pres=P_setstring(CURRENT,name,"O17",0);
	       else if (rval >=19.0 && rval<=19.999)
	         pres=P_setstring(CURRENT,name,"F19",0);
	       else if (rval >=23.0 && rval<=23.999)
	         pres=P_setstring(CURRENT,name,"Na23",0);
	       else if (rval >=31.0 && rval<=31.999)
	         pres=P_setstring(CURRENT,name,"P31",0);
	       else if (rval >=39.0 && rval<=39.999)
	         pres=P_setstring(CURRENT,name,"K39",0);
	       else
	         pres=P_setstring(CURRENT,name,"???",0);
	       checkresult(pres,"setstring");
	       pres=SP_setgroup(CURRENT,name,G_ACQUISITION);
	       checkresult(pres,"setgroup");
               protection = (active == 'X') ? P_ACT : 0;
 	       protection |= P_ACT;
               pres = SP_setprot(CURRENT,name,protection);
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
	       pres=P_setactive(CURRENT,name,ACT_OFF);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(CURRENT,name,G_DISPLAY);
	       checkresult(pres,"setgroup");
	       partype = 0; /* nothing further to do */
             }
	   else
             if (strcmp(name,"intmod")==0)
	     { pres=SP_creatvar(CURRENT,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(CURRENT,name,8.0,0.0,0.0);
	       checkresult(pres,"setlimits");
	       pres=SP_Esetstring(CURRENT,name,INT_OFF,1); /* possible values */
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,INT_PARTIAL,2);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,INT_FULL,3);
	       checkresult(pres,"Esetstring");
	       pres=P_setstring(CURRENT,name,INT_OFF,0);
	       checkresult(pres,"setstring");
	       pres=P_setactive(CURRENT,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(CURRENT,name,G_DISPLAY);
	       checkresult(pres,"setgroup");
	       partype = 0; /* nothing further to do */
             }
	   else
             if (strcmp(name,"pltmod")==0)
	     { int pltmod;

               pres=SP_creatvar(CURRENT,name,ST_STRING);
	       checkresult(pres,"creatvar");
	       pres=SP_setlimits(CURRENT,name,8.0,0.0,0.0);
	       checkresult(pres,"setlimits");
	       pres=SP_Esetstring(CURRENT,name,"off",1); /* possible values */
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,"fixed",2);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,"full",3);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,"variable",4);
	       checkresult(pres,"Esetstring");
	       pres=SP_Esetstring(CURRENT,name,"user",5);
	       checkresult(pres,"Esetstring");
	       pltmod = (int) rvalp->r;
               switch (pltmod)
               {
                 case 0:  pres=P_setstring(CURRENT,name,"off",0);      break;
                 case 1:  pres=P_setstring(CURRENT,name,"fixed",0);    break;
                 case 2:  pres=P_setstring(CURRENT,name,"full",0);     break;
                 case 3:  pres=P_setstring(CURRENT,name,"variable",0); break;
                 default: pres=P_setstring(CURRENT,name,"off",0);      break;
               }
	       checkresult(pres,"setstring");
	       pres=P_setactive(CURRENT,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(CURRENT,name,G_DISPLAY);
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
	       tolower(name1);
	       noblanks(name1);
	       if (strlen(name1))
		 { if (strlen(arraystring)) strcat(arraystring,",");
	           strcat(arraystring,name1);
                 }
	     }
	   else if (partype==0)
	     { if (debug1) Wscrprintf("omitted parameter\n");
               if ((strcmp(name,"re")==0) && (active == 'Y'))
	         reval = rvalp->r;
               else if ((strcmp(name,"re2")==0) && (active == 'Y'))
	         re2val = rvalp->r;
	     }
           else if (partype==1)
             par1_set(name,name1,0,active);
	   else if (partype==4)
	     { if (debug1) Wscrprintf("array identifier, name=%s\n",name);
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
             pres = SP_setprot(CURRENT,name,protection);
	     checkresult(pres,"setprotection");
           }
           if (partype>9)
	     {
	       /* set the active flag */
	       if (active=='N')      pres=P_setactive(CURRENT,name,ACT_OFF);
	       else		     pres=P_setactive(CURRENT,name,ACT_ON);
	       checkresult(pres,"setactive");
	       /* set the group identifier */
	       pres=SP_setgroup(CURRENT,name,grident);
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
  if (debug1) Wscrprintf("done\n");
}

/********/
static tolower(s)
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
      if (dirpage.mnemarray[index].n[ZERO]>=0)
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
  char filepath[MAXPATHL];
  int bi,nf;
  struct datafilehead datahead;
  struct datapointers bpntrs;
  double rnp,rnf;

  r=P_getreal(PROCESSED,"nf",&rnf,1);
  if (r==0)
    nf = (int) (rnf + 0.001);
  else
    nf = 1;
  set_nodata();
  /* open curexp/acqfil/fid file */
  D_close(D_USERFILE); 
  strcpy(filepath,curexpdir);
#ifdef UNIX
  strcat(filepath,"/acqfil/fid");
#else
  vms_fname_cat(filepath,"[.acqfil]fid");
#endif
  /* find DIMS */
  if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"DIMS  \000",&f1))
    if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"DIMS C\000",&f1))
    {   Werrprintf("DIMS not found"); 
        ABORT; 
    }
  dim1 = f1.i2;
  dim2 = f1.i3;
  dim3 = f1.i4;
  dima = dim1 * dim2 * dim3;
  if (debug1) printf("fid dimensions = 1:%d, 2:%d, 3:%d, total=%d\n",
	      dim1,dim2,dim3,dima);
  r=P_setreal(CURRENT,"arraydim",(float)dima,0);
  checkresult(r,"set arraydim");
  r=P_setreal(PROCESSED,"arraydim",(float)dima,0);
  checkresult(r,"set arraydim");
  /* is the data set compressed? */
  compressed =  find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"ACQDAT\000",&acqdat);
  if (compressed)
  {
    Wscrprintf("compressed data set\n");
    /* find the fid file */
    if (!find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,"FIDFIL\000",&f1))
    { Werrprintf("FIDFIL not found"); 
      ABORT;
    }
    fid_sectors = f1.i4 / dima;
    /* allocate memory for acqdat */
    if (debug1)
      Wscrprintf("fid length is %d sectors.  acqdat is %d sectors\n",
                  fid_sectors,acqdat.i4);
    if (!(acodes = allocateWithId(acqdat.i4*PAGELENGTH,"convert")))
    { Werrprintf("cannot allocate buffer for acqdat");
      ABORT;
    }
    if (getdata(fr,acodes,(acqdat.i3+f->i3),acqdat.i4)==0)
    { Werrprintf("cannot read acqdat of old fid");
      release(acodes); ABORT;
    }
  }
  else /* allocate memory for one acode */
    if (!(acodes = allocateWithId(3*PAGELENGTH,"convert")))
    { Werrprintf("cannot allocate buffer for code");
      ABORT;
    }
  /* set up datahead */
  datahead.nblocks = dima;
  datahead.ntraces = nf;
  if (nf>1)
    Wscrprintf("multiple fid traces: nf=%d\n",nf);
  if (P_getstring(CURRENT,"dp",dpflag,1,3))
  { Werrprintf("dp not found");
    release(acodes); ABORT;
  }
  else dpf = (dpflag[0]=='y');
  if (dpf) datahead.ebytes = 4;
      else datahead.ebytes = 2;
  if (dpf) datahead.status = S_DATA | S_32 | S_COMPLEX;
      else datahead.status = S_DATA | S_COMPLEX;


  datahead.nbheaders = 1;
  datahead.vers_id = VERSION;
  datahead.vers_id += FID_FILE;

  if (compressed)
    if (P_getreal(CURRENT,"np",&rnp,1))
    { Werrprintf("np not found");
      release(acodes); ABORT;
    }
    else datahead.np = (int)rnp;
  bi = -1;
  for (i3=1; i3<=dim3; i3++)
    for (i2=1; i2<=dim2; i2++)
      for (i1=1; i1<=dim1; i1++)
        { bi += 1;
          if (interuption)
	    { D_close(D_USERFILE); release(acodes); ABORT;
            }
 
          if (!compressed)
          {
            sprintf(fidname,"%4d.%4d.%4d",i1,i2,i3);
            if (find(fr,f->i3,(f->i2-1)/(LASTMNEM+1)+1,fidname,&f1)==0)
            { Werrprintf("cannot find old fid fcb");
              D_close(D_USERFILE); ABORT;
	    }
            if (getdata(fr,acodes,(f1.i3+f->i3),3)==0)
	    { Werrprintf("cannot read code file of old fid");
	      D_close(D_USERFILE); release(acodes); ABORT;
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
	      { Wscrprintf("np not stored in first fid, using parameter\n");
	        if (P_getreal(CURRENT,"np",&rnp,1))
	        { Werrprintf("np not found");
	          D_close(D_USERFILE); release(acodes); ABORT;
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
	      ABORT;
            }
	  }
              /* now prepare storage for fid in data */
          if (r=D_allocbuf(D_USERFILE,bi,&bpntrs))
	  { D_error(r);
	    D_close(D_USERFILE);
            release(acodes);
            ABORT;
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
	  { Werrprintf("cannot read data file of old fid");
	    D_close(D_USERFILE); release(acodes); ABORT;
	  }
	      /* convert double precision to VAX format if neccessary */
	      /* corrected for multitrace data (nf>1)  01/26/88 */
          if ((ZERO) && (dpf))
          {
            wptr = (short *)bpntrs.data;
            for (i=0; i<datahead.tbytes*nf/4; i++)
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
	    D_close(D_USERFILE); release(acodes); ABORT;
	  }
          if (r=D_release(D_USERFILE,bi))
	  { D_error(r);
	    D_close(D_USERFILE); release(acodes); ABORT;
	  }
	}
  D_close(D_USERFILE);
  release(acodes);
  RETURN;
}

/****************/
int convert(argc,argv,retc,retv) int argc; char *argv[]; int retc; char *retv[];
/****************/
{    
     int  fr;    /* from file descriptor */
     struct old_fcb f;
     char t[80];
     int d2flag;

     Wturnoff_buttons();
     Wshow_text();
     D_allrelease();
     if (argc!=2) 
       { Werrprintf("usage - convert(filename)");
	 ABORT;
       }

     /* open "fromfile"   readonly  */  
     if((fr = open(argv[1],O_RDONLY)) == -1)
     {   Werrprintf("unable to open file \"%s\"",argv[1]);
         perror("convert");
         ABORT;
     }  
     disp_status("CONVERT");
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
     if (find(fr,0,1,"TEXT  \000",&f))	/* then find the text file */
       { if (debug1) Wscrprintf("\nTEXT:\n");
	 distext(fr,f.i3);		/* and display it */
       }
     else
       { Werrprintf("not a good data file");
         disp_status("       ");
	 ABORT;
       }
     specIndex = 1;
     P_treereset(PROCESSED);		/* clear the tree first */
     P_treereset(CURRENT);		/* clear the tree first */
     disp_status("PARAMS ");
#ifdef VMS
     if (find(fr,0,1,"VAXFMT\000",&f))
       cmult = 1.0;			/* values already converted */
     else
       cmult = CMULT;
#else
     cmult = CMULT;
#endif
     if (find(fr,0,1,"ACQPAR\000",&f))	/* find and display ACQPAR */
       { if (debug1) Wscrprintf("\nACQPAR:\n");
         parlibfound = 0;
	 disparms(fr,f.i2,f.i3,G_ACQUISITION,&d2flag);
       }
     if (find(fr,0,1,"PRCPAR\000",&f))	/* find and display PRCPAR */
       { if (debug1) Wscrprintf("\nPRCPAR:\n");
         parlibfound = 0;
	 disparms(fr,f.i2,f.i3,G_PROCESSING,&d2flag);
       }
     if (find(fr,0,1,"DISPAR\000",&f))	/* find and display DISPAR */
       { if (debug1) Wscrprintf("\nDISPAR:\n");
         parlibfound = 0;
	 disparms(fr,f.i2,f.i3,G_DISPLAY,&d2flag);
       }
     Wscrprintf("\n");
     setdgparms(d2flag);
     if (debug1) Wscrprintf("setdgparms done\n");
     storeparams();

     if (find(fr,0,1,"FID   \000",&f))	/* find and display FID */
       { if (debug1) Wscrprintf("FID:\n");
         disp_status("DATA   ");
	 if (debug1) disdir(fr,f.i3,(f.i2-1)/(LASTMNEM+1)+1);
	 storedata(fr,&f);
       }
     close(fr);  /* close the files */
     disp_status("       ");
     releaseAllWithId("convert");
     Wsettextdisplay("convert");
     execString("fixpar\n");
     RETURN;
}
