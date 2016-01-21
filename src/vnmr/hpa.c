/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/*							*/
/*  hpa.c	-plot parameter list on an HP plotter 	*/
/*							*/
/********************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "data.h"
#include "graphics.h"
#include "group.h"
#include "tools.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"

#ifdef UNIX
#define  DIRTERMCHAR	'/'
#else 
#define  DIRTERMCHAR	']'
#endif 

extern int  setplotter();
extern int getdstring(char *paramname, char *t,
                      int max, int digits, int tree0, int index);
extern int debug1;
extern int raster;

#define COMPLETE 	0
#define ERROR 		1

#define COL1  		23
#define COL2  		56
#define COL3  		104
#define COL4  		141
#define COL5  		180
#define COL6  		198
#define COL7  		222
#define COL8  		270
#define COL9  		380

#define ROW1  		-6
#define ROW2  		-13
#define ROW3  		-20
#define ROW4  		-27

/***************************************/
static void fixfilename(char file_name[])
/***************************************/
{
  int i,len;

  len = strlen(file_name);
  while ((len) && (file_name[len-1] != DIRTERMCHAR))
      len--;
  i = 0;
  while ((file_name[i] = file_name[len+i]) != '\0')
    i++;
}

/***************************************/
static void plotparam(char *paramname, int xpos, int ypos, int length, int digits)
/***************************************/
/* display parameter with paramname at specified x-y position */
{
   char t[MAXPATHL];
   int tree;

    xpos = (int) ( xpos * ppmm * wcmax / 400.0);
    tree = (strcmp(paramname,"h1freq")==0) ? SYSTEMGLOBAL : PROCESSED;
    getdstring(paramname,t,length,digits,tree,1);
    if ((strcmp(paramname,"file") == 0) &&
	(strrchr( &t[ 0 ], DIRTERMCHAR ) != NULL))
      fixfilename(t); 
    if (strcmp(t,"undefined")==0)
      strcpy(t,"---");
    if (xpos + ((int) strlen(t)+1) * xcharpixels > mnumxpnts)
      xpos = mnumxpnts - (strlen(t)+1) * xcharpixels;
    if (debug1)
	Wscrprintf("param %s is %s\n",paramname,t);
    amove(xpos,(int)(ypos*ppmm/ymultiplier));
    dstring(t);
}

/****************/
static int priv_getline(char str[], int length, FILE *fileid)
/****************/
{ int c,i;

  i = 0;
  while (--length > 0 && (c=getc(fileid)) != EOF && c != '\n')
    str[i++] = c;
  // check for windows end of line '\r\n'
  if (i && (str[i-1] == '\r') && (c == '\n'))
    i--;
  str[i] = '\0';
  return(i);
}

/****************/
static void plottext(int xpos, int ypos)
/****************/
{
  FILE *textfile;
  char tfilepath[MAXPATHL];
  int maxlines;
  char line[34];

  xpos = (int) (xpos * ppmm * wcmax / 400.0);
  ypos = (int) (ypos * ppmm / ymultiplier);

  strcpy(tfilepath,curexpdir);

#ifdef UNIX
  strcat(tfilepath,"/text");
#else 
  strcat(tfilepath,"text");
#endif 

  maxlines = 6;
  if ( (textfile=fopen(tfilepath,"r")) )
    {
      color( GREEN );
      while ((priv_getline(line,33,textfile) > 0) && maxlines--)
        {
          amove(xpos,ypos);
          ypos -= ycharpixels;
          dstring(line);
        }
      fclose(textfile);
    }
}

/*************/
int hpa(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  double plotchar = 0.6;

  (void) argc;
  (void) argv;
  (void) retc;
  (void) retv;
  if (setplotter()) return(ERROR);
  if (raster)
  {
    Werrprintf("hpa is not supported on raster plotters");
    return(COMPLETE);
  }
  charsize(plotchar);
  color(GREEN);
  if (debug1)
    Wscrprintf("ppmm= %g ymultiplier= %d plot= %d raster= %d, mnumxpnts= %d\n",
                ppmm,ymultiplier,plot,raster,mnumxpnts);

/*  Copy the display parameters from the current group to the
    processed group so the current values will be plotted.	*/

  P_copygroup( CURRENT, PROCESSED, G_DISPLAY );

  plotparam("tn",      COL1,   ROW1, 6,0);
  plotparam("sw",      COL1,   ROW2, 8,1);
  plotparam("at",      COL1,   ROW3, 8,3);
  plotparam("pw",      COL1,   ROW4, 8,1);
  plotparam("sfrq",    COL2,   ROW1, 8,4);
  plotparam("tof",     COL2,   ROW2, 8,1);
  plotparam("d1",      COL2,   ROW3, 8,3);
  plotparam("ct",      COL2+2, ROW4, 8,0);
  plotparam("dn",      COL3,   ROW1, 6,0);
  plotparam("dm",      COL3,   ROW2, 6,0);
  plotparam("dmm",     COL3+7, ROW3, 6,0);
  plotparam("pp",      COL3,   ROW4, 8,1);
  plotparam("dof",     COL4,   ROW1, 8,1);
  plotparam("dlp",     COL4,   ROW2, 8,0);
  plotparam("dmf",     COL4,   ROW3, 8,0);
  plotparam("dpwr",    COL4,   ROW4, 8,0);
  plotparam("fn",      COL5,   ROW1, 8,0);
  plotparam("lb",      COL5,   ROW2, 8,3);
  plotparam("wp",      COL5+2, ROW3, 8,1);
  plotparam("gf",      COL6,   ROW2, 8,3);
  plotparam("sp",      COL7-6, ROW3, 8,1);
  plotparam("seqfil",  COL8,   ROW1, 6,0);
  plotparam("temp",    COL8-2, ROW3, 8,1);
  plotparam("solvent", COL8,   ROW4, 8,0);
  plotparam("file",    COL9,   ROW2, 8,0);
  plotparam("date",    COL9,   ROW3, 8,0);
  plotparam("h1freq",  COL9,   ROW4, 6,0);
  plottext(295,ROW2);
  endgraphics();
  return(COMPLETE);
}
