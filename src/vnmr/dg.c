/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*--------------------------------------------------------------
|							
|	dg.c
|  
|	This command displays parameter groups.  This command
|	can be called with any alias name and will retrieve 
|	a format string with that alias name from the current 
|	variables tree.  The format string describes the format
|	to be followed when displaying a parameter group.
|
|	If names of variables are passed as arguments to this
|	command and a previous display is still on the screen, 
|	only the passed variables will be updated and display  
|	in reverse video on the screen.  Certain terminals
|	such as the Wyse-50 and televideo can not use
|	reverse video (the old one character storage space
|	for attribute problem) without leaving a one character
|	space on the screen.  Thus, some terminals will just
|	update the variable.
|
|	the command da('name') displays the elements of the
|	arrayed variable ('name'). 
|							
|	the ap/pap commands use the same type of format string,
|	but produce output on the printer or plotter.  They also,
|	in effect, run da with output to the printer or plotter.
|	The printer output for arrays continues at the end of 
|	the 2-column output while the plotter output goes to
|	a 3rd and 4th (if needed) column.
|
|	Major additions to template capability  GMB 5/25/90
|
+------------------------------------------------------------*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "allocate.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "graphics.h"
#include "buttons.h"
#include "pvars.h"
#include "wjunk.h"
#include "tools.h"

#define COLUMNLENGTH 	20 
#define ESC 		27
#define MAXCOLUMNS 	4
#define SCRMAXLINES 	18
#define PMAXLINES	64
#define NAMELENGTH 	18
#define EXPLENGTH 	1024
#define NORMAL		0
#define REVERSE		1
#define TEMPLMAX 	1024
#define MAXTEXTLINE	40
#define MAXARRAYS        8
#define ARRAYSTRINGMAX 1024
#define MIN_PAP_START   120

#define isalpha_(c)  (isalpha(c) || ((c) == '_')) 
/* #define isalnum_(c)  (isalnum(c) || ((c) == '_'))  */

#define isalnum_(c)  (isalnum(c) || ((c) == '_') || ((c) == '\047')) 
#define isdbnum_(c)  (isdigit(c) || ((c) == '.')) 

#ifdef  DEBUG
#define EBUG
#endif 

#ifdef  EBUG
extern  int Eflag;
#define EPRINT(str) \
	if (Eflag) Wstprintf(str)
#define EPRINT1(str, arg1) \
	if (Eflag) Wstprintf(str,arg1)
#define EPRINT2(str, arg1, arg2) \
	if (Eflag) Wstprintf(str,arg1,arg2)
#define EPRINT3(str, arg1, arg2, arg3) \
	if (Eflag) Wstprintf(str,arg1,arg2,arg3)
#define EPRINT4(str, arg1, arg2, arg3, arg4) \
	if (Eflag) Wstprintf(str,arg1,arg2,arg3,arg4)
#define EPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (Eflag) Wstprintf(str,arg1,arg2,arg3,arg4,arg5)
#else 
#define EPRINT(str) 
#define EPRINT1(str, arg2) 
#define EPRINT2(str, arg1, arg2) 
#define EPRINT3(str, arg1, arg2, arg3) 
#define EPRINT4(str, arg1, arg2, arg3, arg4) 
#define EPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif 

static int  everything = 1;
static int  match;
static int  state;
static int  row[MAXCOLUMNS];
static char specialstring[COLUMNLENGTH];
static int  screenflag,plotterflag;
static int  maxlines,ccolumn,tree;
static char *psave[PMAXLINES];
static int  xstart,ystart;
static int  arraycount;
static int  ystartsave;
static int  dlaflag = 0;
static int  firstdg, firstarray;
static int  arrayfieldlen = 0;
static int  ap_color_set = 0;
static int  title_color_set = 0;
static int  ap_raster_pl_color;
static int  title_raster_pl_color;
static FILE *outfile = NULL;
static int  default_plot = 0;

int getdstring(char *par, char *t, int max, int digits, int tree, int index);
int dgmain(int argc, char *argv[], int retc, char *retv[]);

extern int select_init(int get_rev, int dis_setup, int fdimname, int doheaders,
              int docheck2d, int dospecpars, int doblockpars, int dophasefile);
extern int execName(char *var, char *strval, int maxlen);
extern int execExp(char *buffer);
extern int disp_current_seq();
extern int setplotter();
extern int currentindex();
extern int expdir_to_expnum(char *expdir);
extern int run_calcdim();
extern int colorindex(char *colorname, int *index);

/*******************/
static void addblanks(int num)
/*******************/
{ int i;

  if (default_plot)
  {
     fprintf(outfile," ");
     return;
  }
  for(i=0;i<num;i++)
    strncat(psave[ccolumn]," ",79-strlen(psave[ccolumn]));
}

/*****************************/
static void paramupdate(char *n, int argc, char *argv[])
/*****************************/
{ int i;
  for (i=1;i<argc;i++)
    { if ( ! strcmp(n,argv[i]) )
        { match = 1;
          return;
        }
    }
}

/*****************/
static void dg_move(int y, int x)
/*****************/
{
  if (default_plot)
     return;
  ccolumn = y;
  if (x<79)
    while ( (int) strlen(psave[y])<x) strcat(psave[y]," ");
}

/*****************/
static void dg_addstr(char *s)
/*****************/
{ int tmplen;
  if (default_plot)
     fprintf(outfile,"%s",s);
  else
  {
     tmplen = strlen(psave[ccolumn]);
     if (strlen(s)+tmplen<79)
        strcat(psave[ccolumn],s);
  }
}

/******************************************************/
int displayarray(int *columnp, int n, char *arrayname[],
                 int arrayfield[], int arraydigits[])
/******************************************************/
/* n = arrayindex + 1  */
{ int i,j,l,more_to_do;
  char *t[MAXARRAYS];
  char ti[16];
  int columnlen = COLUMNLENGTH;
  int claflag = 0;

  claflag = (strcmp(arrayname[0],"cla") == 0);
  firstarray = 0;
  for (i=0; i<n; i++)
    if (arrayfield[i]<0)
      arrayfield[i] = (COLUMNLENGTH-5)/n;
  if (row[*columnp]>=maxlines)
  {
    (*columnp)++;
    row[*columnp] = 1;
  }
  if (*columnp>MAXCOLUMNS-1) return 1;
  dg_move(row[*columnp],*columnp * columnlen);
  if (default_plot)
     dg_addstr("Tableheader: i ");
  else
     dg_addstr(" i ");
  for (i=0; i<n; i++)
    { l = strlen(arrayname[i]);
      if (strcmp(arrayname[i],"clfreq") == 0)
        addblanks(2);
      else 
        addblanks(arrayfield[i]-l);
      dg_addstr(arrayname[i]);
      if (default_plot)
         dg_addstr("\n");
    }
  arrayfieldlen=3;
  for (i=0; i<n; i++) arrayfieldlen += arrayfield[i];
  /* Winfoprintf("n=%d arrayfieldlen = %d",n,arrayfieldlen); */
  if (arrayfieldlen >= COLUMNLENGTH)
	columnlen = arrayfieldlen + 2;
  if (row[*columnp]>=maxlines)
    { if (*columnp>MAXCOLUMNS-1)
         return 1;
      (*columnp)++;
      row[*columnp] = 1;
    }
  else
    row[*columnp]++;
  for (i=0; i<n; i++)
    t[i] = (char *)allocateWithId(256,"dg");
  more_to_do = 1;
  j = 1;
  while (more_to_do)
    { more_to_do = 0;
      for (i=0; i<n; i++) 
        { if (getdstring(arrayname[i],t[i],arrayfield[i],
            arraydigits[i],tree,j)==0)
            more_to_do = 1;
        }
      if (more_to_do)
        { dg_move(row[*columnp],*columnp * columnlen);
	  /* if (*columnp > 0 && dlaflag) sprintf(ti,"      %2d ",j); else */
	  if (row[*columnp] >= maxlines - 1 && *columnp == MAXCOLUMNS-1)
	  {   dg_addstr("incomplete      ");
	  }
	  else
	  {   if (!claflag) sprintf(ti,"%2d ",j);
	      else sprintf(ti,"   ");
              dg_addstr(ti);
              for (i=0; i<n; i++)
                dg_addstr(t[i]);
              if (default_plot)
                 dg_addstr("\n");
	  }
	  /* if (dlaflag && i != 1) dg_addstr("      "); */
          if ( ! default_plot)
             row[*columnp]++;
          if (row[*columnp]>=maxlines)
            { if (*columnp>MAXCOLUMNS-1)
                return 1;
              (*columnp)++;
              row[*columnp] = 1;
                  dg_move(1,(*columnp) * columnlen);
	      /* if (dlaflag && i != 1) dg_addstr("            "); */
  	      dg_addstr(" i ");
  	      for (i=0; i<n; i++)
    	        { l = strlen(arrayname[i]);
      	          addblanks(arrayfield[i]-l);
      	          dg_addstr(arrayname[i]);
    	        }
              if (default_plot)
                 dg_addstr("\n");
              row[*columnp]++;
            }
        }
      j++;
    }
  return 0;
}

/*****************/
static void dg_initscr()
/*****************/
{ int i;
  for (i=0; i<PMAXLINES; i++) psave[i][0]='\0';
}

/****************************/
static void temperr(int pos, char *s, char *tmplt)
/****************************/
/* print out template error position */
{ int i;
  Werrprintf("error %s in template, state=%d, pos=%d",s,state,pos);
  printf("COMPLETE STRING: %s\n",tmplt);
  if ((pos>=0) && (pos< (int) strlen(tmplt)))
    { for (i=0; i<pos; i++) tmplt[i]='.';
      printf("ERROR POSITION:  %s\n",tmplt);
    }
  state=0;
}

int dla_maxlines() 
{
    vInfo  info;
    int maxsize;
 
    if (P_getVarInfo(GLOBAL,"clfreq",&info))
    {	
      return 1;
    }
    else
    {
      maxsize=info.size;
      if (P_getVarInfo(GLOBAL,"slfreq",&info))
      {	
        return 1;
      }
      else
      {
        if (info.size>maxsize) maxsize=info.size;
        maxlines = PMAXLINES - 1;
        if ((maxsize+5)/2 < SCRMAXLINES)
           maxlines = SCRMAXLINES;
        else if ((maxsize+6)/2 < maxlines)
           maxlines = (maxsize + 6)/2;
        return(0);
      }
    }
}

/********************************************/
int getdstring(char *paramname, char *t, int max, int digits, int tree0, int index)
/********************************************/
{  
    char   buf[1024];
    char   t1[8];
    double value;
    vInfo  info;

    if ( ! strcmp(paramname,"arraydim") )
    {
       if ( run_calcdim() )
          ABORT;
    }
    EPRINT3("getdstring:par=\"%s\" max=%d dig=%d\n",paramname,max,digits);
    if (P_getVarInfo(tree0,paramname,&info))
    {
       tree0 = GLOBAL;   /* Try GLOBAL tree before giving up */
       if (P_getVarInfo(tree0,paramname,&info))
       {
          sprintf(t1,"%%%ds",max);
          sprintf(t,t1,"undefined");
          EPRINT2("getdstring: par=\"%s\" t=\"%s\"\n",paramname,t);
          ABORT;
       }
    }
    if (index>info.size)
    {  sprintf(t1,"%%%ds",max);
       sprintf(t,t1," ");
       ABORT;
    }
    if ((info.size>1) && (index==0))
    {  sprintf(t1,"%%%ds",max);
       sprintf(t,t1,"arrayed");
       arraycount += info.size + 2;
    }
    else if (!info.active) 
    {  sprintf(t1,"%%%ds",max);
       sprintf(t,t1,"not used");
    }
    else
    {  if (index==0)
         index = 1;  
       switch (info.basicType)
       {   case T_REAL:     if (P_getreal(tree0,paramname,&value,index))
                            { sprintf(t1,"%%%ds",max);
                              sprintf(t,t1,"ERROR");
                              return 1; 
                            }
			    if (value == 0.0)
			    {
			        sprintf(t1,"%%%d.0f",max);
			        if (strcmp(paramname,"clindex"))
			            sprintf(t,t1,value);
			        else
			            sprintf(t,"    ");
			    }
			    else 
			    {
			        if ((digits>=0)&&(fabs(value)<1e6))
			    	   sprintf(t1,"%%%d.%df",max,digits);
			        else 
			    	   sprintf(t1,"%%%dg",max);
			        sprintf(t,t1,value);
			    }
			    break;
	   case T_STRING:   if (P_getstring(tree0,paramname,buf,index,1024))
                            { sprintf(t1,"%%%ds",max);
                               sprintf(t,t1,"ERROR");
                               return 1; 
                            }
			    sprintf(t1,"%%%ds",max);
			    sprintf(t,t1,buf);
			    EPRINT2("getdstring: name=\"%s\" string = \"%s\"\n",paramname,t);
			    break;
       }
    }
    return 0;
}

/**********************************************/
static void disparam(char *paramname, int column, int special, int digits)
/**********************************************/
/* display parameter with paramname in specified column */
{ char t[512];
  int save,field;
  char *t1;

  EPRINT5("disparam: par \"%s\" col=%d spe=%d dig=%d pos=%d\n",
			paramname,column,special,digits,row[column]);
  if ((column<0)||(column>MAXCOLUMNS-1)) 
    return;
  if (row[column]>maxlines) 
    return;
  if (special)
    { /* for special strings, just save value in special string */
      if (digits>0)
        P_getstring(tree,paramname,t,1,digits);
      else
        P_getstring(tree,paramname,t,1,COLUMNLENGTH-4);
      if (strlen(specialstring)+strlen(t)>COLUMNLENGTH-4)
	{ /* display special string, if too long */
          dg_move(row[column],column*COLUMNLENGTH);
	  dg_addstr(specialstring);
	  row[column]++;
	  specialstring[0] = '\0';
	}
      strncat(specialstring,t,COLUMNLENGTH-strlen(specialstring));
      strcat(specialstring,"  ");
    }
  else
    { dg_move(row[column],column*COLUMNLENGTH);
      dg_addstr(paramname);
      getdstring(paramname,t,COLUMNLENGTH-3-strlen(paramname),digits,tree,0);
      field = strlen(t) + strlen(paramname);
      if ( default_plot)
      {
          addblanks(1);
          dg_addstr(t);
          dg_addstr("\n");
      }
      else if ( field > COLUMNLENGTH-2 )
        { t1 = t;
          addblanks(1);
          field = COLUMNLENGTH-strlen(paramname)-3;
          while ((strlen(t1)>0) && (row[column]<=maxlines))
            { if ( (int) strlen(t1)>field)
                { save = t1[field];
                  t1[field] = 0;
                }
              else
                save = 0;
              addblanks(field-strlen(t1));
              dg_addstr(t1);
              t1 += strlen(t1);
              t1[0] = save;
              if (strlen(t1)>0)
                { dg_addstr("~");
                  row[column]++;
                  if (row[column]>maxlines) 
                    return;
                  field = COLUMNLENGTH - 2;
                  dg_move(row[column],column*COLUMNLENGTH);
                }
            }
        }
      else if ( field == COLUMNLENGTH-2 )
        {
          addblanks(1);
          dg_addstr(t);
        }
      else
        {
          addblanks(COLUMNLENGTH-2 - field);
          dg_addstr(t);
        }
      row[column]++;
    }
}

/*******************************/
static void distitle(char *titlename, int column)
/*******************************/
/* display title name in specified column */
{
   char t[COLUMNLENGTH];
   char t1[8];

   if ((column<0)||(column>MAXCOLUMNS-1)) 
	return;
   if (row[column]>maxlines) 
	return;
   
   if (default_plot)
   {
      fprintf(outfile,"Label: %s\n", titlename);
      column=0;
      row[0] = 0;
   }
   else
   {
      dg_move(row[column],column*COLUMNLENGTH);
      sprintf(t1,"%%%ds",(COLUMNLENGTH-2+ (int) strlen(titlename))/2);
      sprintf(t,t1,titlename);
      dg_addstr(t);
      row[column]++;
   }
}

/*****************/
static void writeline(char *s)
/*****************/
{ 

  if (outfile)
  {
      fprintf(outfile,"%s\n",s);
  }
  else if (screenflag)
    { /* screen output */
      Wscrprintf("%s\n",s);
    }
  else if (plotterflag)
    { if (ystart>0)
      { amove(xstart,ystart);
        dstring(s);
        ystart -= ycharpixels;
      }
    }
  else
    { /* printer output, not yet implemented */
      Wscrprintf("%s\n",s);
    }
}

/*****************/
void writetitle(int expnum)
/*****************/
{ char pslabel[33];
  char t[40];
  char estring[5];

  sprintf(estring, "%d", expnum);
  if (P_getstring(PROCESSED,"pslabel",pslabel,1,32)==0)
    { sprintf(t,"exp%s  %s",estring,pslabel);
      writeline(t);
      writeline("");
    }
}

/****************/
static void writetext()
/****************/
{
  FILE *textfile;
  char tfilepath[MAXPATH];
  char tline[MAXTEXTLINE+8];
  int i,c;

/*  Changed to draw text using pen 4 on multi-pen plotters.
    Since pen 5 was selected earlier, reselct it when complete.  */

  strcpy(tfilepath,curexpdir);
#ifdef UNIX
  strcat(tfilepath,"/text");
#else 
  strcat(tfilepath,"text");
#endif 
  i = 0;
  if ( (textfile=fopen(tfilepath,"r")) )
    {
      if (plotterflag)
      {  
           if(ap_color_set)
           {
              if(title_color_set)            /* color for title is selected by user */
                 color(title_raster_pl_color);
              else
                 color(RED);                    /* RED is a default color for title */
           }
           else
              color(PARAM_COLOR);
      }  
 
      c = getc(textfile);
      while (c != EOF)
        { if (c == '\n')
            { tline[i] = '\0';
              writeline(tline);
              i = 0;
            }
          else
            { if (i>=MAXTEXTLINE)
                { tline[i] = '\0';
                  writeline(tline);
                  i = 0;
                }
              tline[i++] = c;
            }
          c = getc(textfile);
        }
      if (i)
      {
        tline[i] = '\0';
        writeline(tline);
      }
      writeline("");
      fclose(textfile);
      if (plotterflag)
        {
          if(ap_color_set)         /* color selected , ex: pap('green') page */
             color(ap_raster_pl_color);
          else
            color(PARAM_COLOR);
        }
    }
    else
       Werrprintf("Unable to open text file");
}

/**********/
void disp_expno()
/**********/
{
   int	expnum;

   expnum = expdir_to_expnum(curexpdir);
   disp_exp(expnum); 
   disp_specIndex(currentindex());
}

/*************************/
int dg(int argc, char *argv[], int retc, char *retv[])
/*************************/
{
   arraycount=3;
   firstdg = 1;
   firstarray = 1;
   outfile = NULL;
   dgmain(argc,argv,retc,retv);
   if (arraycount == 3)
   {
      if (outfile)
         fclose(outfile);
      RETURN;
   }
   if (strcmp(argv[0],"ap") == 0 || strcmp(argv[0],"pap") == 0)
   {					/* print/plot arrays */
       int limit = 9;
       firstdg = 0;
       if (strcmp(argv[0],"ap") == 0)
       {   strcpy(argv[0],"pr");
       }
       else 
       {   
	   strcpy(argv[0],"pl");
  	   limit = 37;
	   if (arraycount > limit)
	   {
	      int tempmax;
              select_init(1,2,0,0,0,0,0,0);
	      tempmax = mnumypnts / ycharpixels;
	      if (maxlines > tempmax) maxlines = tempmax;
	   }
       }
       if (arraycount <= limit)
           maxlines = limit + 1;
       else if ((arraycount + 1) / 2 < maxlines)
	   maxlines = (arraycount +1 )/2;
       else if ((arraycount + 1) / 3 < maxlines)
	   maxlines = (arraycount +1 )/3;
       else if ((arraycount + 1) / 4 < maxlines)
	   maxlines = (arraycount +1 )/4;
       dgmain(argc,argv,retc,retv);
   }
   if (outfile)
      fclose(outfile);
   RETURN;
}

int dg_raster_color(char *name)
{
  if( strcmp(name,"red")    == 0 ||
      strcmp(name,"green")  == 0 ||
      strcmp(name,"blue")   == 0 ||
      strcmp(name,"cyan")   == 0 ||
      strcmp(name,"magenta")== 0 ||
      strcmp(name,"yellow") == 0 ||
      strcmp(name,"black")  == 0 ||
      strcmp(name,"white")  == 0   )
  {
     return(1);
  }
  else
  {
     return(0);
  }
}

static char	old_template_name[ MAXPATH ];		/* store the template  */
							/* parameter name here */
/*************************/
int dgmain(int argc, char *argv[], int retc, char *retv[])
/*************************/
{
    char *template_name;
    char  omitname[NAMELENGTH+1];
    char  paramname[NAMELENGTH+1];
    char  tmplt[TEMPLMAX];
    char  titlename[COLUMNLENGTH];
    int   digits = 0;
    int   field = 0;
    int   i,j;
    int   offset;
    int   column;
    int   omitpnt = 0;
    int   parampnt = 0;
    int   special = 0;
    int   titlepnt = 0;
    int   fixFontsize;
    int   showvar = 0;
    int   showgroup;
    int   array,arrayindex;
    char  *arrayname[MAXARRAYS];
    int   arrayfield[MAXARRAYS];
    int   arraydigits[MAXARRAYS];
    char  arraystring[ARRAYSTRINGMAX];
    int   templatesize,tindex;
    vInfo  varinfo;
    char expbuf[EXPLENGTH];
    int  expindx;
    int  chk4blankinname = 0;
    int  isliteral = 0;
    int  redisplay_param_flag;
    int  plaflag = 0;  
    double fs;
    char *command_name;

/*  Redisplay Parameters?  */

    (void) retc;
    (void) retv;
    redisplay_param_flag = 0;
    default_plot = 0;
    fixFontsize = 0;
    if (argc > 1)
      if (strcmp( argv[ argc-1 ], "redisplay parameters" ) == 0) {
            argc--;

/*  autoRedisplay (in lexjunk.c) is supposed
    to insure the next test returns TRUE.	*/

	    if (WtextdisplayValid( argv[ 0 ] ))
              redisplay_param_flag = 1;
      }
 
    dlaflag = 0;
    psave[0] = 0;
    match = 0;
    command_name = argv[ 0 ];
    template_name = NULL;

    if (strcmp(command_name,"ap")==0 || strcmp(command_name,"pr")==0)
      { screenflag  = 0;
	if (argc > 1)
	  template_name = argv[ 1 ];
	else
	  template_name = command_name;
	everything  = 1;
	plotterflag = 0;
	tree        = PROCESSED;
	if (strcmp(command_name,"ap") == 0) maxlines = PMAXLINES-1;
        if (argc>2) 
        {
           if (!outfile)
              if ( (outfile = fopen(argv[2],"w")) == NULL)
              {
                Werrprintf("ap: could not open output file %s", argv[2]);
	        ABORT;
              }
           
           if (argc == 4)
              default_plot = 1;
        }
      }
    else
      if (strcmp(command_name,"pap")==0 || strcmp(command_name,"pl")==0)
      {  
        ap_color_set = 0;
        title_color_set = 0;
        screenflag  = 0;
        if (strcmp(command_name,"pap") == 0) maxlines = PMAXLINES-1;
 
/*  Now "pap" can have arguments giving the character size and the
    position of the plot.  To allow the first argumet to be the name
    of the template, we emply a Trick, in that we advance the argument
    vector and decrease the argument count.  Now if the remainder of
    this program uses `command_name' to reference the name of the
    command, it can examine argv[ 1 ], argv[ 2 ], etc. just as it did
    before user-template-parameter-as-argument was introduced.          */
 
        template_name = "ap";
        if (argc > 1)
        {
           if( (!isReal(argv[1])) && (!dg_raster_color(argv[1])) )
           {
              template_name = argv[ 1 ];
              argc--;
              argv++;
           }
        }
           if (argc > 1)
           {
              if( dg_raster_color(argv[1]) )
              {  
                 colorindex( argv[1], &ap_raster_pl_color );
                 argc--;
                 argv++;
                 ap_color_set = 1;
                 if (argc > 1)
                 {
                    if( dg_raster_color(argv[1]) )
                    {
                       colorindex( argv[1], &title_raster_pl_color );
                       argc--;
                       argv++;
                       title_color_set = 1;
                    }
                 }
               } 
            }
         everything  = 1;
	 plotterflag = 1;
	 tree        = PROCESSED;
      }
      else if (strcmp(command_name,"da")==0)
           {
              screenflag  = 1;
	      everything  = 1;
	      plotterflag = 0;
	      firstdg     = 0;
	      maxlines    = SCRMAXLINES;
	      tree        = CURRENT;
            }
            else if (argc>1 && strcmp(argv[1],"dla")==0)
	            dlaflag = 1;

    if (strcmp( command_name, "ap" )==0 || strcmp( command_name, "pap" ) == 0)
      { screenflag  = 0;
      }
    else if (strcmp(command_name,"da")==0 || strcmp(command_name,"pl")==0 ||
             strcmp(command_name,"pr")==0)
      {
        if ( run_calcdim() )
           ABORT;
	template_name = "";
        if (argc>1 && strcmp(command_name,"da") == 0) 
          { i = 1;
            strcpy(tmplt,"1:ACQUISITION ARRAYS:array,arraydim;");
            while (i<argc)
              { strcat(tmplt,"1: :[");
	        strcat(tmplt,argv[i]);
	        strcat(tmplt,"];");
                i++;
              }
          }
        else
          { strcpy(tmplt,"1:ACQUISITION ARRAYS:array,arraydim;");
            if (P_getstring(tree,"array",arraystring,1,ARRAYSTRINGMAX))
              { Werrprintf("could not read parameter array");
	        releaseAllWithId("dg");
	        ABORT;
              }
            i=0;
            while(arraystring[i])
              {
                if (default_plot)
                   strcat(tmplt,"1 [");  /* skip the blank line */
                else
                   strcat(tmplt,"1: :[");
                j = strlen(tmplt);
                if (arraystring[i]=='(') i++;
                while(isalnum_(arraystring[i]))
                  { tmplt[j]=arraystring[i];
                    i++;
                    j++;
                  }
                tmplt[j]=0;
	        strcat(tmplt,"];");
                if (arraystring[i]==')') i++;
                if (arraystring[i]==',') i++;
              }
          }
	/* Wscrprintf(" %s \n",tmplt); */
      }
    else					/* command name is "dg */
      {
	screenflag  = 1;
	if (redisplay_param_flag) {
		everything = 0;
		if (strlen( &old_template_name[ 0 ] ) > 0)
		  template_name = &old_template_name[ 0 ];
		else
		  template_name = "dg";
	}
	else {
		everything = 1;
		if (argc > 1)
		  template_name = argv[ 1 ];
		else
		  template_name = "dg";
                if (argc>2 && strcmp(command_name,"dg") == 0) 
                {
	          screenflag  = 0;
                   if (!outfile)
                      outfile = fopen(argv[2],"w");
                }
	}
	plotterflag = 0;
	tree     = CURRENT;
        maxlines = SCRMAXLINES;
        if (argc>1 && strcmp(argv[1],"dla")==0)
	{
           if (dla_maxlines()) return(1);
	}
        else if (argc>1 && strcmp(argv[1],"pla")==0)
        {
	   /* Wscrprintf("Command name is dg\n"); */
	   strcpy(template_name,"dla");
           screenflag=0;
           plotterflag=1;
	   plaflag = 1; /* dg('pla') plots dla display, spin sim line assignments */
           maxlines = PMAXLINES;
           select_init(1,2,0,0,0,0,0,0);
	   if (maxlines > mnumypnts / ycharpixels) 
	      maxlines = mnumypnts / ycharpixels;
        }
      }

/*  Move the display group from the current tree to the processed
    tree if the processed tree was identified as the one to use.  */

    if (tree == PROCESSED)
      P_copygroup( CURRENT, PROCESSED, G_DISPLAY );

    if (screenflag)
      { disp_current_seq();
        disp_expno();
      }
    psave[0] = (char *)allocateWithId(PMAXLINES * 80,"dg");
    if (psave[0]==0) 
      { Werrprintf("cannot allocate buffer space"); ABORT; }
    for (i=1; i<PMAXLINES; i++) psave[i] = psave[i-1] + 80;
    if (!screenflag)
      {	if (plotterflag)
	  { setplotter();
	    if ((argc>4)||((argc==4)&&(!isReal(argv[3]))))
	      { Werrprintf("usage - ppa(x,y,charsize) x,y in mm\n");
	        releaseAllWithId("dg");
	        ABORT;
              }
            else if (argc==4) {
              fixFontsize = 1;
	      charsize((double)stringReal(argv[3]));
            }
            else
	      charsize((double)0.7);
	    color(BLACK);
 	    
	    if (strcmp(argv[0],"pl") == 0)
 	      {
	      }
 	    else if (plaflag)
	      xstart = mnumxpnts/50;
	    else if (argc>1)	/* xstart defined as argument */
	      { if (isReal(argv[1]))
	          { xstart = (int)(stringReal(argv[1])*ppmm);
		    if (xstart < mnumxpnts / MIN_PAP_START)
		      xstart = mnumxpnts / MIN_PAP_START;
		    else if (xstart > mnumxpnts - 40 * xcharpixels)
		      xstart = mnumxpnts - 40 * xcharpixels;
		  }
                else if (strcmp(template_name,"dla") == 0)
		  {
	            xstart = mnumxpnts/50;
		    Wscrprintf("argc=%d template_name=%s\n",argc,template_name);
		  }
		  else	
		  { Werrprintf("usage - ppa(x,y,charsize) x,y in mm\n");
	            releaseAllWithId("dg");
	            ABORT;
                  }
	      }
	    else
	      xstart = mnumxpnts/50;
 	    
	    if (strcmp(argv[0],"pl") == 0)
 	      {
	      }
 	    else if (plaflag)
	      ystart = mnumypnts - mnumypnts / 20;
	    else if (argc>2)	/* xstart defined as argument */
	      { if (isReal(argv[2]))
	          { ystart = (int)(stringReal(argv[2])*ppmm/ymultiplier)+ymin;
		    if (ystart < mnumypnts / 20)
		      ystart = mnumypnts / 20;
		    else if (ystart > mnumypnts - mnumypnts / 20)
		      ystart = mnumypnts - mnumypnts / 20;
		  }
                else
		  { Werrprintf("usage - ppa(x,y,charsize) x,y in mm\n");
	            releaseAllWithId("dg");
	            ABORT;
                  }
	      }
	    else
	      ystart = mnumypnts - mnumypnts / 20;
          }
        else if ( ! outfile) /* (!plotterflag) */
	  {
	    Walpha(W_SCROLL);
	    Wscrprintf("\n");
	  }
        if ( strcmp(argv[0],"pr") != 0 && strcmp(argv[0],"pl") != 0 && ! default_plot ) 
	  {
	    writetext();
            writetitle( expdir_to_expnum(curexpdir) );
          }
      }
  dg_initscr();
  for (i=0; i<MAXCOLUMNS; i++) 
    row[i] = 0;

  templatesize = 1;
  if (strlen(template_name))
  { 
   if (P_getVarInfo(tree, template_name, &varinfo))
   {
      tree = GLOBAL;
      if (P_getVarInfo(tree, template_name, &varinfo))
      {
         Werrprintf("could not find template parameter '%s'",template_name);
	 releaseAllWithId("dg");
	 ABORT;
      }

   }
   if (varinfo.basicType != T_STRING)
   {
      Werrprintf("template parameter '%s' is not a string variable.",template_name);
      releaseAllWithId("dg");
      ABORT;
   }
   templatesize = varinfo.size;		/* array size of template */
   EPRINT1("array size: %d\n",templatesize);
  }


  /* loop for each template element */
  for (tindex=1; tindex <= templatesize; tindex++)
  {
    if (strlen(template_name))
    { 
      if (P_getstring(tree,template_name,tmplt,tindex,TEMPLMAX))
      { 
	Werrprintf("could not get template parameter %s",template_name);
  	releaseAllWithId("dg");
  	ABORT;
      }
    }

  i=0;	/* start at beginning of template */
  array = arrayindex = showgroup = expindx = 0;
  state = 1;	/* expect a header first */
  while (tmplt[i])
    {
        switch (state)
	{ case 1:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*   ^                               */
		   { if (firstarray || firstdg) column = tmplt[i]-'1';
                     if (default_plot)
                     {
                        column = 0;
                        row[0] = 0;
                     }
		     while (row[column]>=maxlines)
                     {   if (column<MAXCOLUMNS-1)
                            column++;
			else
			{ 
			   /* Werrprintf("Display/plot incomplete: check dg str."); */
			   /* temperr(i,"too mangled",tmplt); */
			   tmplt[i+1] = 0;
			   break;
			}
                     }
	             state = 2;
                     array = 0;
		     omitname[0] = '\0';
		     specialstring[0] = '\0';
		     showgroup = 1;
		     if ((column<0) || (column>MAXCOLUMNS-1))
		       temperr(i,"illegal column",tmplt);
		     break;
	 	   }
	  case 2:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*    ^   ^                          */
		   { 
		     if (tmplt[i] == '(') 
		     { 
			expindx = 0;
			expbuf[0]='\0';
			state=3;
		     }
		     else if (tmplt[i]==':') 
		     {
		        state = 5;
		     }
		     else if (tmplt[i]==' ') 
		     {
		        state = 7;
		     }
		     else 
			temperr(i,"expecting '(' or ':' :",tmplt);
		     break;
		   }
	  case 3:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*     ^                             */
		   { 
		     if (tmplt[i] != ':') 
		     { 
			if (expindx>=EXPLENGTH-1) 
			   temperr(i,"Expression too long",tmplt);
		        else 
			{
			  expbuf[expindx]=tmplt[i];
			  expindx++; 
			}
		     }
		     else if (tmplt[i]==':') 
		     {
			expbuf[expindx-1] = '\0'; /* null out last ')' */
			/* now evaluate expression */
			showgroup = execExp(expbuf);
		        state = 5;
		        if (showgroup == -1)
		        {
			   temperr(i,"syntax",tmplt);
		        }
		     }
		     else temperr(i,"unknown char",tmplt);
		     break;
		   }

	  case 4:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
                     break;

	  case 5:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*         ^                         */
		   { 
		     /* must discover if 1. blank in name, 2. starts with number */
		     /* Since the parser will be very unhappy to see them. */
		     if (isalnum_(tmplt[i]))
		     {  state=6; 
			titlename[0]=tmplt[i]; 
			titlepnt=0;
			if ( isdigit(tmplt[i]) )
			{
			  chk4blankinname=2;
			  isliteral=1;
			}
			else
			{
			  chk4blankinname=1;
			  isliteral=0;
			}
		     } 
		     else if (tmplt[i]==' ')
		     {  state=6; 
			titlename[0]=tmplt[i]; 
			titlepnt=0;
			chk4blankinname=0;
		     }
		     else temperr(i,"expecting alpha",tmplt);
		     break;
		   }
	  case 6:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*          ^^^^^^^^^^^^^^           */
		   { if (isalnum_(tmplt[i])||(tmplt[i]==' ')||
			 (tmplt[i]=='&')||(tmplt[i]=='.') ||
			 (tmplt[i]=='[')||(tmplt[i]==']') )
		     { if (titlepnt>=COLUMNLENGTH-1) 
			  temperr(i,"name too long",tmplt);
		       else 
		       {
			  if ( (isalnum_(tmplt[i])) && (!chk4blankinname) )
			  {
			     chk4blankinname=1;
			     isliteral=0;
			  }
			  else if (chk4blankinname==1)
			  {
			     if (tmplt[i]==' ')
				isliteral=1;
			  }
			  titlepnt++; 
			  titlename[titlepnt]=tmplt[i];
		       }
		     }
		     else if (tmplt[i]==':') 
		       {
			  char buf[COLUMNLENGTH];
			  int stat;
			  titlename[titlepnt+1] = '\0'; 
			  /* now determine is title is a string variable or not */
			  /* execName returns string len in buf */
			  if ( !isliteral )
			  {
			    if ( (stat = execName(titlename,buf,COLUMNLENGTH)) > 0 )
			    {
			       strcpy(titlename,buf);
                            }
			    else if ( stat < 0 )	/* error? */
		            {
			       temperr(i,"syntax",tmplt);
		            }
			  }
			  state = 7;
			  if (showgroup)
				distitle(titlename,column);
		       }
		     else temperr(i,"expecting alpha",tmplt);
		     break;
		   }
	  case 7:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*                        ^          */
		   { omitname[0] = '\0';
		     digits = -1; /* default, use automatic display */
                     field  = -1;
		     showvar = 1;
                     if (tmplt[i]=='[')
                       { array = 1;
                         arrayindex = -1;
                       }
		     else if (isalnum_(tmplt[i])||(tmplt[i]==' '))
		     {  state=8; 
			paramname[0]=tmplt[i]; 
			parampnt=0;
			special=0;
		     }
		     else if (tmplt[i]==',')
                       { if (row[column]<maxlines) 
			   row[column]++;
                       }
		     else if (tmplt[i]==';')
                       { if (row[column]<maxlines) 
			   row[column]++;
			 state=1;
                       }
		     else temperr(i,"expecting letter",tmplt);
		     break;
		   }
	  case 8:  /*   1(ni):2D ACQUISITION:ni,sw1;    */
		   /*                         ^  ^^     */
		   { if (isalnum_(tmplt[i])||(tmplt[i]==' '))
		     { if (parampnt>=NAMELENGTH-1)
				     temperr(i,"name too long",tmplt);
		       else parampnt++; paramname[parampnt]=tmplt[i];
		     }
		     else if (tmplt[i]==',') 
		       { paramname[parampnt+1] = '\0'; 
			 state = 7;
                         /* showvar = checkomit(omitname); */
			 if (showgroup && showvar)
                           { if (array)
                               { arrayindex++;
                                 arrayname[arrayindex] =
                                   (char *)allocateWithId(strlen(paramname)+2,"dg");
                                 if (!arrayname[arrayindex])
                                   { Werrprintf("cannot allocate buffer space");
                                     releaseAllWithId("dg"); 
                                     ABORT;
                                   }
                                 strcpy(arrayname[arrayindex],paramname);
                                 arraydigits[arrayindex] = digits;
                                 arrayfield[arrayindex]  = field;
			         paramupdate(paramname,argc,argv);
                               }
                             else
                               { if (row[column]>=maxlines) 
                                   { if (column<MAXCOLUMNS-1)
                                       { column++;
                             /*            row[column] = 0; */
                                       }
				       else
			               { 
				     	   /* Werrprintf(
"Display/plot incomplete: check dg str."); */
				     	   tmplt[i+1] = 0;
				     	   break;
				       }
                                   }
                                 disparam(paramname,column,special,digits);
			         paramupdate(paramname,argc,argv);
                               }
                           }
		       }
		     else if ((tmplt[i]==';')||(tmplt[i]==']'))
                       { if (tmplt[i]==']')
		           { if (!array)
                               { temperr(i,"] without [ ",tmplt);
                                 break;
                               }
                             i++;
                             if ((tmplt[i]!=';') && (tmplt[i]!=','))
                               { temperr(i,"expecting ; or ,",tmplt);
                                 break;
                               }
                           }
		         else if (array)
                           { temperr(i,"expecting ] ",tmplt);
                             break;
                           }
		         paramname[parampnt+1] = '\0'; 
			 if (tmplt[i]==';')
			   state = 1;
			 else	/* tmplt[i]==',' */
			   state = 7;
                         /* showvar = checkomit(omitname); */
			 if (showgroup && showvar)
                           { if (array)
                               { arrayindex++;
                                 arrayname[arrayindex] =
                                   (char *)allocateWithId(strlen(paramname)+2,"dg");
                                 if (!arrayname[arrayindex])
                                   { Werrprintf("cannot allocate buffer space");
                                     releaseAllWithId("dg"); 
                                     ABORT;
                                   }
                                 strcpy(arrayname[arrayindex],paramname);
                                 arraydigits[arrayindex] = digits;
                                 arrayfield[arrayindex]  = field;
			         paramupdate(paramname,argc,argv);
                               }
                             else
                               { if (row[column]>=maxlines) 
                                   { if (column<MAXCOLUMNS-1)
                                       { column++;
                                         row[column] = 0;
                                       }
				       else
			               { 
				     	  /* Werrprintf(
"Display/plot incomplete: check dg str."); */
				     	   tmplt[i+1] = 0;
				     	   break;
				       }
                                   }
                                 disparam(paramname,column,special,digits);
			         paramupdate(paramname,argc,argv);
                               }
                           }
                         if (array)
                           { displayarray(&column,arrayindex+1,arrayname,
                               arrayfield,arraydigits);
                             array = 0;
 			     if (row[column]>=maxlines || column>MAXCOLUMNS-1)
			        { if (column>MAXCOLUMNS-1)
			          { 
				    /* Werrprintf(
"Display/plot incomplete: check dg str."); */
				     tmplt[i+1] = 0;
				     break;
				  }
				  else
				  {
			             (column)++;
			             row[column] = 1;
				  }
			        }
                           }
                         if (strlen(specialstring))
                           { /* display special string, if there */
                             dg_move(row[column],column*COLUMNLENGTH);
                             if (default_plot)
                               dg_addstr("Special: ");
                             dg_addstr(specialstring);
                             if (default_plot)
                               dg_addstr("\n");
			     row[column]++;
                           }
		       }
                     else if (tmplt[i]=='*')
		       { if (array)
                           { temperr(i,"expecting ] ",tmplt);
                             break;
                           }
                         special=1;
		       } 
                     else if (tmplt[i]=='(')
		       { state = 9;
			 expindx=0;
		       }
                     else if (tmplt[i]==':')
		       { state = 11;
		       }
		     else temperr(i,"expecting alpha",tmplt);
		     break;
		   }
	  case 9:  /*   1(ni):2D ACQUISITION:ni,sw1(ni);  */
		   /*                               ^     */
		   { 
		     if ( (tmplt[i] != ',') && (tmplt[i] != ';') &&
			  (tmplt[i] != ':') )
		     { 
			if (expindx>=EXPLENGTH-1) 
			   temperr(i,"Expression too long",tmplt);
		        else 
			{
			  expbuf[expindx]=tmplt[i];
			  expindx++; 
			}
		     }
		     else if ( (tmplt[i]==',') || (tmplt[i]==';') ||
			       (tmplt[i] == ':') )
		     {
			expbuf[expindx-1] = '\0'; /* null out last ')' */
			/* now evaluate expression */
			showvar = execExp(expbuf);
		        state = 8;
			i--;	/* back up past , or ; for state 8 */
		        if (showvar == -1)
		        {
			   temperr(i,"syntax",tmplt);
		        }
		     }
		     else temperr(i,"syntax error ",tmplt);
		     break;
		   }
	  case 10:  /*   1(ni):2D ACQUISITION:ni,sw1(ni); */
		   /*                                 ^^  */
		   { if (isalnum_(tmplt[i])) 
		       { if (omitpnt>=NAMELENGTH-1)
				      temperr(i,"name too long",tmplt);
		         else omitpnt++; omitname[omitpnt]=tmplt[i];
		       }
		     else if (tmplt[i]==')')
		       { omitname[omitpnt+1]='\000'; state = 8;
		         omitpnt = 0;
		       }
		     else temperr(i,"expecting alpha",tmplt);
                     break;
		   }
          case 11:  /*   1(ni):2D ACQUISITION:ni:0,sw1:3; */
		    /*                           ^      ^   */
                   { field = digits;
                     digits=tmplt[i]-'0';
		     state=8;
		     if ((digits<0)||(digits>9))
			 temperr(i,"illegal digit",tmplt);
                     if ((tmplt[i+1]>='0')&&(tmplt[i+1]<='9'))
                       { i++;
                         digits = 10 * digits + tmplt[i] - '0';
                       }
                     break;
		   }
          case 0:  /* error */
	           releaseAllWithId("dg");
		   ABORT;
		   break;
        }
      i++;
    } /* end of while */
  }  /* end of for */
  j = maxlines;
  while ((j>0)&&(strlen(psave[j])==0)) j--;

  if (screenflag && !everything && !match)
    { releaseAllWithId("dg"); 
      RETURN;
    }
    if (screenflag)
      { Wturnoff_buttons();
        Wshow_text();
#ifdef DEBUG
        if (!Eflag)
#endif 
        Wclear_text();
      }
  if (strcmp(argv[0],"pap") == 0 && arraycount > 3)
    {  if (arraycount > 37) offset = 80; else offset=60;
       if (xstart > mnumxpnts - offset * xcharpixels)
	   xstart = mnumxpnts - offset * xcharpixels;
    }
  if (strcmp(argv[0],"pl") == 0)
    {
      ystart = ystartsave;
      xstart += 40*xcharpixels;
    }
  else
    ystartsave = ystart;

  if (!screenflag && plotterflag) {
     if (!fixFontsize) {
         i = ystart - ycharpixels * j;
         fs = 0.7;
         while (i < 0) {
             if (fs < 0.5)
                 break;
             fs = fs - 0.05;
             charsize(fs); 
             i = ystart - ycharpixels * j;
         }
     }
  }
  if ( ! default_plot )
  {
     for (i=0; i<=j; i++)
       writeline(psave[i]);
  }
  if (plotterflag) {
    endgraphics();
    charsize(1.0); 
  }
  releaseAllWithId("dg");
  if (screenflag)
  {
    /* releasevarlist();    The addition of this command causes the
      LARGE graphics screen to be redrawn, even if not active, when-
      ever any parameter is changed within the currently displayed
      parameter group.  SF */

    if (template_name != NULL) /* Store template name for future reference */
      strcpy( &old_template_name[ 0 ], template_name );
    if (strcmp(template_name,"dla") == 0)
       Wsettextdisplay("dla"); /* store display name for future updates */
    else
       Wsettextdisplay(argv[0]); /* store display name for future updates */
  }

    /* Wscrprintf("command_name=%s dlaflag=%d\n",command_name,dlaflag); */
  RETURN;
}

