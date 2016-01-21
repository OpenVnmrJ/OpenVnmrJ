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
/*  dpcon	-   display a 2D contour map		*/
/*  dpconn	-   same, but do not erase screen	*/
/*  pcon	-   2D contour map on plotter		*/
/*							*/
/********************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "sky.h"
#include "vnmrsys.h"
#include "allocate.h"
#include "buttons.h"
#include "dscale.h"
#include "init_proc.h"
#include "pvars.h"
#include "wjunk.h"

extern int debug1;
extern void flush_draw();
extern void x_adraw(int x, int y);
extern void new_x_adraw(int start_flag);
extern int dcon_displayparms();
extern void setcolormap(int firstcolor, int numcolors, int th, int phcolor);
extern int currentindex();
extern void set_spectrum_thickness(char *min, char *max, double ratio);

static int b_colors;
static int colors,numcont;
static float contmult;
static int oldcontour,newcontour,colorflag,xval0,yval0,sort,movex,movey;
static float cont;

/**************************/
/* plot sorting variables */
/**************************/

#define PLOTBUFFARRAY	4096		/* number of links to store */
#define INDXBUFFARRAY	1024		/* number of indices to store */

struct plotbuffentry
  { int x1,y1,x2,y2,next;
  };

struct indxentry
  { int otherendoflink,plotbuffindx,xsave,ysave;
  };

static struct plotbuffentry *plbuff; 
static struct indxentry *indxbuff;
static int numberfilled,indxnumber,lastpbindx,lastindx,xpos,ypos,pbindx;

/****************/
static int zerobuffs()
/****************/
/* zero the sorting buffers */
{ register int i;
  if (debug1) Wscrprintf("zerobuffs\n");
  if ((plbuff = (struct plotbuffentry *) allocateWithId(PLOTBUFFARRAY *
			      sizeof(struct plotbuffentry),"dpcon"))==0)
    { Werrprintf("cannot allocate plot sorting buffer");
      ABORT;
    }
  if ((indxbuff = (struct indxentry *) allocateWithId(INDXBUFFARRAY *
		              sizeof(struct indxentry),"dpcon")) == 0)
    { Werrprintf("cannot allocate plot sorting buffer");
      ABORT;
    }
  for (i=0; i<PLOTBUFFARRAY; i++)
    plbuff[i].next = 0;
  for (i=0; i<INDXBUFFARRAY; i++)
    indxbuff[i].otherendoflink = indxbuff[i].xsave =indxbuff[i].ysave = 0;
  numberfilled = indxnumber = lastpbindx = lastindx = 0;
  RETURN;
}

/*******************/
static void plotlink(indx)
/*******************/
/* plot out one link */
int indx;
{ int base,temp;
  if (debug1) Wscrprintf("plotlink(%d)\n",indx);
  temp = indx;
  amove(xpos=indxbuff[temp].xsave,ypos=indxbuff[temp].ysave);
  base = indxbuff[temp].plotbuffindx;
  do
    { if ((plbuff[base].x1==xpos)&&(plbuff[base].y1==ypos))
	adraw(xpos=plbuff[base].x2,ypos=plbuff[base].y2);
      else
	adraw(xpos=plbuff[base].x1,ypos=plbuff[base].y1);
      temp = base;
      base = plbuff[temp].next;
      plbuff[temp].next = 0;
    }
    while (base>0);
  while ((plbuff[numberfilled].next==0) && (numberfilled>0))
    numberfilled--;
  temp = indxbuff[indx].otherendoflink;
  indxbuff[indx].otherendoflink=indxbuff[indx].xsave=indxbuff[indx].ysave=0;
  indxbuff[temp].otherendoflink=indxbuff[temp].xsave=indxbuff[temp].ysave=0;
  lastpbindx = lastindx = 0;
  while ((indxbuff[indxnumber].otherendoflink==0) && (indxnumber>1))
    indxnumber--;
}

/*****************/
static void dumpbuffer()
/*****************/
/* dump the sorting buffer completely */
{ int i,indx;
  if (debug1) Wscrprintf("dumpbuffer()\n");
  if (indxnumber==0) return;
  indx = indxnumber;
  for (i=1; i<=indx; i++)
    if (indxbuff[i].otherendoflink>0) plotlink(i);
  numberfilled = indxnumber = lastpbindx = lastindx = 0;
}

/*******************************/
static void checkforcycle(indx,linked)
/*******************************/
/* check, whether a complete cycle is in buffer, plot it, if complete */
int indx,*linked;
{ int temp;
  if (debug1) Wscrprintf("checkforcycle(%d)\n",indx);
  temp = indxbuff[indx].otherendoflink;
  if (temp<0) temp = -temp;
  if ((indxbuff[indx].xsave==indxbuff[temp].xsave) &&
      (indxbuff[indx].ysave==indxbuff[temp].ysave))
    { plotlink(indx);
      *linked = 1;
    }
}

/*****************************/
static int checkforconnection(indx)
/*****************************/
/* check, whether the specified link is connected with a different one */
int indx;
{ int i,indx1,indx2,indx3,temp1,temp2,linked;
  linked = 0;
  temp1 = indxbuff[indx].otherendoflink;
  if (debug1) Wscrprintf("checkforconnection(%d), temp1=%d\n",indx,temp1);
  if (temp1>0) checkforcycle(indx,&linked);
  else         checkforcycle(-temp1,&linked);
  if (linked) RETURN;
  i = 0;
  do
    { i += 1; if (i==indx) i++;
      if ((indxbuff[indx].xsave==indxbuff[i].xsave) &&
	  (indxbuff[indx].ysave==indxbuff[i].ysave))
        linked = 1;
    }
    while ((i<indxnumber) && (!linked));
  if (!linked) RETURN;
  lastindx = 0;
  temp2 = indxbuff[i].otherendoflink;
  if (((temp1<0)&&(temp2>0))||((temp1>0)&&(temp2<0)))
    { /* same direction, can just link them together */
      if (temp1<0)
	{ temp1 = -temp1;
	  indxbuff[temp1].otherendoflink = temp2;
	  indxbuff[temp2].otherendoflink = -temp1;
	  temp2 = indxbuff[indx].plotbuffindx;
	  plbuff[temp2].next = indxbuff[i].plotbuffindx;
	  indxbuff[indx].otherendoflink = indxbuff[indx].xsave
					 = indxbuff[indx].ysave = 0;
	  indxbuff[i].otherendoflink = indxbuff[i].xsave
					    = indxbuff[i].ysave = 0;
	  checkforcycle(temp1,&linked);
	  RETURN;
	}
      else
	{ temp2 = -temp2;
	  indxbuff[temp1].otherendoflink = -temp2;
	  indxbuff[temp2].otherendoflink = temp1;
	  temp1 = indxbuff[i].plotbuffindx;
	  plbuff[temp1].next = indxbuff[indx].plotbuffindx;
	  indxbuff[indx].otherendoflink = indxbuff[indx].xsave
					 = indxbuff[indx].ysave = 0;
	  indxbuff[i].otherendoflink = indxbuff[i].xsave
					    = indxbuff[i].ysave = 0;
	  checkforcycle(temp2,&linked);
	  RETURN;
	}
    }
  /* other direction, must turn one of them around */
  linked = 0;
  if (temp1>0)
    { indxbuff[temp1].otherendoflink = -temp2;
      indxbuff[temp2].otherendoflink = temp1;
      indx1 = indxbuff[indx].plotbuffindx;
      indx2 = indxbuff[i].plotbuffindx;
    }
  else
    { indxbuff[-temp1].otherendoflink = -temp2;
      indxbuff[-temp2].otherendoflink = temp1;
      indx1 = temp2;
      indx2 = indxbuff[-temp2].plotbuffindx;
    }
  do
    { indx3 = plbuff[indx2].next;
      plbuff[indx2].next = indx1;
      indx1 = indx2;
      indx2 = indx3;
    }
    while (indx3>=0);
  if (temp1<0)
    { indx3 = indxbuff[indx].plotbuffindx;
      plbuff[indx3].next = indx1;
    }
  indxbuff[indx].otherendoflink = indxbuff[indx].xsave
					 = indxbuff[indx].ysave = 0;
  indxbuff[i].otherendoflink = indxbuff[i].xsave
					    = indxbuff[i].ysave = 0;
  if (temp1<0) checkforcycle(-temp1,&linked);
  else         checkforcycle(temp2,&linked);
  RETURN;
}

/**********************************/
static void findindxes(startlink,endlink)
/**********************************/
int *startlink,*endlink;
{ if (debug1) Wscrprintf("findindxes, lastindx=%d\n",lastindx);
  *startlink = lastindx;
  do
     (*startlink)++;
  while ((*startlink<INDXBUFFARRAY-3) && (indxbuff[*startlink].otherendoflink));
  *endlink = *startlink;
  do
     (*endlink)++;
  while ((*endlink<INDXBUFFARRAY-2) && (indxbuff[*endlink].otherendoflink));
  if ( (*endlink >= INDXBUFFARRAY-1) ||
       (*startlink >= INDXBUFFARRAY-2) ||
        indxbuff[*startlink].otherendoflink ||
	indxbuff[*endlink].otherendoflink )
  {
      movmem((char *)plbuff+pbindx,(char *)plbuff,sizeof(struct plotbuffentry),1,1);
      plbuff[pbindx].next = 0;
      dumpbuffer();
      numberfilled += 1;
      movmem((char *)plbuff,(char *)plbuff+pbindx,sizeof(struct plotbuffentry),1,1);
      plbuff[0].next = 0;
      *startlink = indxnumber + 1;
      *endlink = *startlink + 1;
      indxnumber = *endlink;
  }
  else if (indxnumber < *endlink)
  {
      indxnumber = *endlink;
  }
  lastindx = *endlink;
}

/***********************/
static void findpbindx(dumped)
/***********************/
int *dumped;
{ if (debug1) Wscrprintf("findpbindx()\n");
  numberfilled -= 1;
  pbindx = lastpbindx;
  do pbindx++;
    while ((plbuff[pbindx].next) && (pbindx<PLOTBUFFARRAY-1));
  lastpbindx = pbindx;
  if (plbuff[pbindx].next != 0)
    { dumpbuffer();
      *dumped = 1;
    }
}

/*************************************/
static void fillbuff(x1,y1,x2,y2,dumped)
/*************************************/
int x1,y1,x2,y2,*dumped;
{
  if (debug1) Wscrprintf("fillbuff(%d,%d,%d,%d)\n",x1,y1,x2,y2);
  *dumped = 0;
  numberfilled += 1;
  if (numberfilled<PLOTBUFFARRAY)
    pbindx = numberfilled;
  else
    { findpbindx(dumped);
      if (*dumped) return;
    }
  plbuff[pbindx].x1 = x1;
  plbuff[pbindx].y1 = y1;
  plbuff[pbindx].x2 = x2;
  plbuff[pbindx].y2 = y2;
}

/******************************/
static void startnewlink(x1,y1,x2,y2)
/******************************/
int x1,y1,x2,y2;
{ int startlink,endlink,dumped;
  if (debug1) Wscrprintf("startnewlink(%d,%d,%d,%d)\n",x1,y1,x2,y2);
  fillbuff(x1,y1,x2,y2,&dumped);
  if (dumped) fillbuff(x1,y1,x2,y2,&dumped);
  findindxes(&startlink,&endlink);
  indxbuff[startlink].otherendoflink = endlink;
  indxbuff[startlink].plotbuffindx = pbindx;
  indxbuff[endlink].otherendoflink = -startlink;
  indxbuff[endlink].plotbuffindx = pbindx;
  plbuff[pbindx].next = -endlink;
  if ((x1<x2) || ((x1==x2) && (y2<y1)))
    { indxbuff[startlink].xsave = x1;
      indxbuff[startlink].ysave = y1;
      indxbuff[endlink].xsave = x2;
      indxbuff[endlink].ysave = y2;
    }
  else
    { indxbuff[startlink].xsave = x2;
      indxbuff[startlink].ysave = y2;
      indxbuff[endlink].xsave = x1;
      indxbuff[endlink].ysave = y1;
    }
}

/*************************/
static int addtolink(x1,y1,x2,y2,indx)
/*************************/
int x1,y1,x2,y2,indx;
{ int i,dumped;
  if (debug1) Wscrprintf("addtolink(%d,%d,%d,%d,%d)\n",x1,y1,x2,y2,indx);
  i = indxbuff[indx].plotbuffindx;
  if (((x1==plbuff[i].x1) && (y1==plbuff[i].y1) &&
       (x2==plbuff[i].x2) && (y2==plbuff[i].y2)) ||
      ((x1==plbuff[i].x2) && (y1==plbuff[i].y2) &&
       (x2==plbuff[i].x1) && (y2==plbuff[i].y1)))
      RETURN;
  fillbuff(x1,y1,x2,y2,&dumped);
  if (dumped)
    { startnewlink(x1,y1,x2,y2);
      RETURN;
    }
  if ((x1==indxbuff[indx].xsave) && (y1==indxbuff[indx].ysave))
    { indxbuff[indx].xsave = x2;
      indxbuff[indx].ysave = y2;
    }
  else
    { indxbuff[indx].xsave = x1;
      indxbuff[indx].ysave = y1;
    }
  if (indxbuff[indx].otherendoflink>0)
    { plbuff[pbindx].next = indxbuff[indx].plotbuffindx;
      indxbuff[indx].plotbuffindx = pbindx;
    }
  else
    { plbuff[pbindx].next = -indx;
      i = indxbuff[indx].plotbuffindx;
      plbuff[i].next = pbindx;
      indxbuff[indx].plotbuffindx = pbindx;
    }
  checkforconnection(indx);
      RETURN;
}

/****************/
static int s_adraw(x,y)
/****************/
/* add vector to sorting buffer */
int x,y;
{ int i,linked;
  if (debug1) Wscrprintf("s_adraw(%d,%d,%d,%d)\n",movex,movey,x,y);
  if ((x==movex) && (y==movey)) RETURN;
  if (numberfilled==0)
    { startnewlink(movex,movey,x,y);
      RETURN;
    }
  linked = 0;
  i = 1;
  do
    { i++;
      if ((movex==indxbuff[i].xsave) && (movey==indxbuff[i].ysave))
	  linked = 1;
      else if ((x==indxbuff[i].xsave) && (y==indxbuff[i].ysave))
	  linked = 1;
    }
    while ((i<indxnumber) && (!linked));
  if (linked) addtolink(movex,movey,x,y,i);
  else startnewlink(movex,movey,x,y);
      RETURN;
}

/**********************/
static int i_dpcon(plotflag)
/**********************/
/* initialize dpcon program */
int plotflag;
{

  if (init2d(1,plotflag)) ABORT;
  if (!d2flag)
    { Werrprintf("no 2D data in data file");
      ABORT;
    }

  sort = (plotflag==2); /* only sort for plotting */
  plbuff = 0;
  indxbuff = 0;
  if (sort)
    if (zerobuffs()) ABORT;
  RETURN;	/* successfull initialization */
}

/* Remember that the range of the function modulo N is [0..N-1]  */

extern int VnmrJViewId;
extern int getVpSpecColor();

int getVpNegColor() {
   if(VnmrJViewId == 1) return VP1_NCOLOR;
   else if(VnmrJViewId == 2) return VP2_NCOLOR;
   else if(VnmrJViewId == 3) return VP3_NCOLOR;
   else if(VnmrJViewId == 4) return VP4_NCOLOR;
   else if(VnmrJViewId == 5) return VP5_NCOLOR;
   else if(VnmrJViewId == 6) return VP6_NCOLOR;
   else if(VnmrJViewId == 7) return VP7_NCOLOR;
   else if(VnmrJViewId == 8) return VP8_NCOLOR;
   else if(VnmrJViewId == 9) return VP9_NCOLOR;
   else return VP1_NCOLOR;
}

/**************************/
static void changecolor(x,doingneg,phase_display)
/**************************/
int x;				/* value is 1 to number of contours */
int phase_display;
int doingneg;			/* set when drawing negative contours */
{				/* of phased display */

/*  New constant ZERO_PH_LEVEL lets us define the zero-level for
    2D phased displays.  The negative and positive contours each
    receive an allotment of colors which the program cycles through.	*/

  // if b_colors=1, draw mono color contours according to viewport color.
  // use viewport color for possitive contours, 
  // use ZERO_PH_LEVEL + VnmrJViewId for negative contours.
  if (phase_display) 
    { if (doingneg) {
	if(!b_colors)
           color( ZERO_PH_LEVEL -
		((x-1) % (ZERO_PH_LEVEL - FIRST_PH_CLR) + 1));
	else 
           color(getVpNegColor());
           //color( ZERO_PH_LEVEL - VnmrJViewId);

/*  You must exclude the zero phase level from the positive
    countour color set.						*/

      } else {
	if(!b_colors)
           color( ZERO_PH_LEVEL + ((x-1) %
		(NUM_PH_CLRS + FIRST_PH_CLR - ZERO_PH_LEVEL - 1) + 1));
	else
           color(getVpSpecColor());
      }
    }

/*  The number of AV colors includes color 17, which is black and
    should be omitted from the palatte used by the plotting commands.

    The program cycles through the non-black colors if the number
    of contours exceeds the number of AV colors.			*/

    else if(!b_colors)
      color( FIRST_AV_CLR + (x-1) % (NUM_AV_CLRS-1) + 1 );
    else
      color(getVpSpecColor());
}

/********************/
static int zcon3(buf,npnt)
/********************/
short *buf;
int npnt;
/* return result=1, if any value in buffer >=256, else 0 */
{ register int i;
  register short *bufr;
  i = npnt;
  bufr = buf;
  while (i--) if (*bufr++>=256) return 1;
  return 0;
}

/*****************************/
static void zcon4(b1,b2,size,val,ip)
/*****************************/
short *b1,*b2;
int size;
int *val,*ip;
{ register int i;
  register short *b1r,*b2r;
  i = *ip;
  b1r = b1 + i - 1;
  b2r = b2 + i - 1;
  while ((i<size)&&(*b1r<256)&&(*(b1r+1)<256)&&(*b2r<256)&&(*(b2r+1)<256))
    { i += 1;
      b1r += 1;
      b2r += 1;
    }
  if (i<size)
    { val[0] = *b1r;
      val[1] = *(b1r+1);
      val[2] = *b2r;
      val[3] = *(b2r+1);
    }
  *ip = i;
}

/*************************/
static void contour(yincr,b1,b2,exp_horiz,exp_vert)
/************************/
int yincr;
short *b1,*b2;
float exp_horiz,exp_vert;
{ int i,j;
  int val[4];
  float m;
  int drawx,drawy,diagx,diagy;

#ifdef VNMRJ
#define adraw(x, y)    x_adraw(x, y)
#endif

  i = 1;
  diagx = diagy = 0;
  while (i<npnt) 
    {
      zcon4(b1,b2,npnt,val,&i);
      if (i<npnt)
        {
	  for (j=0; j<4; j++) if (val[j]==256) val[j]=257;
	  if ((val[1]<256) && (val[2]>256))
	    { m = (float)(256-val[1])/(float)(val[2]-val[1]);
	      diagx = xval0 + (int)(((float)(i)-m) * exp_horiz);
	      diagy = yval0 + (int)(((float)(yincr)+m) * exp_vert);
	      if (val[0]<256)
		{ movex = xval0 + (int)((float)(i-1) * exp_horiz);
		  movey = yval0 +
		    (int)(((float)(yincr) + (float)(256-val[0]) /
		    (float)(val[2]-val[0])) * exp_vert);
                }
              else
		{ movex = xval0 +
		    (int)(((float)(i)-(float)(256-val[1]) /
		    (float)(val[0]-val[1])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr) * exp_vert);
                }
              if (sort)
	        { s_adraw(diagx,diagy);
		  movex = diagx; movey = diagy;
                }
              else
		{ amove(movex,movey);
	          adraw(diagx,diagy);
                }
              if (val[3]<256)
		{ drawx = xval0 +
		    (int)(((float)(i)-(float)(256-val[3]) /
		    (float)(val[2]-val[3])) * exp_horiz);
                  drawy = yval0 + (int)((float)(yincr+1) * exp_vert);
                }
               else 
		{ drawx = xval0 + (int)((float)(i) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr) + (float)(256-val[1]) /
		    (float)(val[3]-val[1])) * exp_vert);
		}
               if (sort) s_adraw(drawx,drawy);
               else adraw(drawx,drawy);
	    }
          else if ((val[1]>256) && (val[2]<256))
	    { m = (float)(256-val[2])/(float)(val[1]-val[2]);
	      diagx = xval0 + (int)(((float)(i-1)+m) * exp_horiz);
	      diagy = yval0 + (int)(((float)(yincr+1)-m) * exp_vert);
	      if (val[0]>256)
		{ movex = xval0 + (int)((float)(i-1) * exp_horiz);
		  movey = yval0 +
		    (int)(((float)(yincr+1) - (float)(256-val[2]) /
		    (float)(val[0]-val[2])) * exp_vert);
                }
              else
		{ movex = xval0 +
		    (int)(((float)(i-1)+(float)(256-val[0]) /
		    (float)(val[1]-val[0])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr) * exp_vert);
                }
              if (sort)
	        { s_adraw(diagx,diagy);
		  movex = diagx; movey = diagy;
                }
              else
	        { amove(movex,movey);
	          adraw(diagx,diagy);
                }
              if (val[3]>256)
		{ drawx = xval0 +
		    (int)(((float)(i-1)+(float)(256-val[2]) /
		    (float)(val[3]-val[2])) * exp_horiz);
                  drawy = yval0 + (int)((float)(yincr+1) * exp_vert);
                }
               else 
		{ drawx = xval0 + (int)((float)(i) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr+1) - (float)(256-val[3]) /
		    (float)(val[1]-val[3])) * exp_vert);
		}
               if (sort) s_adraw(drawx,drawy);
               else adraw(drawx,drawy);
	    }
          else if (val[1]>256) /* &&(val[2]>256) */
	    { 
	      if (val[0]<256)
		{ movex = xval0 +
		    (int)(((float)(i-1)+(float)(256-val[0]) /
		    (float)(val[1]-val[0])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr) * exp_vert);
		  drawx = xval0 + (int)((float)(i-1) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr) + (float)(256-val[0]) /
		    (float)(val[2]-val[0])) * exp_vert);
                  if (sort)
		    { s_adraw(drawx,drawy);
		      movex = diagx; movey = diagy;
                    }
		  else
                    { amove(movex,movey);
		      adraw(drawx,drawy);
                    }
                }
              if (val[3]<256)
		{ movex = xval0 +
		    (int)(((float)(i)-(float)(256-val[3]) /
		    (float)(val[2]-val[3])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr+1) * exp_vert);
		  drawx = xval0 + (int)((float)(i) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr+1) - (float)(256-val[3]) /
		    (float)(val[1]-val[3])) * exp_vert);
                  if (sort)
		    { s_adraw(drawx,drawy);
                    }
                  else
		    { amove(movex,movey);
		      adraw(drawx,drawy);
                    }
                }
	    }
          else /* (val[1]<256)&&(val[2]<256) */
	    { 
	      if (val[0]>256)
		{ movex = xval0 +
		    (int)(((float)(i)-(float)(256-val[1]) /
		    (float)(val[0]-val[1])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr) * exp_vert);
		  drawx = xval0 + (int)((float)(i-1) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr+1) - (float)(256-val[2]) /
		    (float)(val[0]-val[2])) * exp_vert);
                  if (sort)
		    { s_adraw(drawx,drawy);
		      movex = diagx; movey = diagy;
                    }
                  else
                    { amove(movex,movey);
		      adraw(drawx,drawy);
                    }
                }
              if (val[3]>256)
		{ movex = xval0 +
		    (int)(((float)(i-1)+(float)(256-val[2]) /
		    (float)(val[3]-val[2])) * exp_horiz);
                  movey = yval0 + (int)((float)(yincr+1) * exp_vert);
		  drawx = xval0 + (int)((float)(i) * exp_horiz);
		  drawy = yval0 +
		    (int)(((float)(yincr) + (float)(256-val[1]) /
		    (float)(val[3]-val[1])) * exp_vert);
                  if (sort)
		    { s_adraw(drawx,drawy);
                    }
                  else
                    { amove(movex,movey);
		      adraw(drawx,drawy);
                    }
                }
            }
	    i++;
	} /* if (i<npnt) */
    } /* while (i<npnt) */
  oldcontour = newcontour;

#ifdef VNMRJ
#undef adraw
#endif 
}

/***********/
static int contourplot(phase_display)
/***********/
int phase_display;
/* calculate and display a contour plot */
{ int plusminus,colorend,temp,f1,f2,l,nf1,nf2;
  int incr,pbuf1,pbuf2,trace;
  char *fulltrace;
  float *phasfl;
  short *pbuf[2];
  float exp_h, exp_v;
  extern double expf_dir();
  char name1[64], name2[64];
  double d, d2, factor = 0.0;
  int npts;

  /* allocate trace buffers, np/2 points, 2 bytes per point */
  if ((pbuf[0]=(short *)allocateWithId(sizeof(short)*npnt,"dpcon"))==0)
    { Werrprintf("cannot allocate buffer space 1"); ABORT; }
  if ((pbuf[1]=(short *)allocateWithId(sizeof(short)*npnt,"dpcon"))==0)
    { Werrprintf("cannot allocate buffer space 2"); release(pbuf[0]);ABORT; }
  /* allocate phasefile buffer, np/2 points, 4 bytes per point */
  if ((fulltrace=(char *)allocateWithId(npnt1+1,"dpcon"))==0)
    { Werrprintf("cannot allocate full trace buffer");
      release(pbuf[0]); release(pbuf[1]); ABORT; }
  if (!phase_display) { colorend = 0; colors = 0; }
  else if (colors) { colorend = colors-1; colors = colorend; }
  else colorend = 1;
  /* go through loop for positive and negative contours */
  if (!plot) Wgmode();
#ifdef VNMRJ
  if (!plot) new_x_adraw(1);
#endif

    if(P_getreal(GLOBAL, "thickFactor", &factor, 2)) factor = 0.0;
  if(sw>0) d = wp/sw;
  else d = 1.0;
  if(sw1>0) d2 = wp1/sw1;
  else d2 = 1.0;
  if(d2 > d) d = d2;
  
  if(factor < 0) { // thickness depends on wp/sw and factor = 0.0 to -1.0
     d = (1.0 - d)*fabs(factor);
  } else { // thickness depends on data points and graphics pixels, factor = 0.0 to 10 
     npts = d*fn/2;
     if(npts>0) d = factor*(double)dnpnt/(double)npts;
     else d = 0.0;
  }
  if(d>1.0) d=1.0;
  if(d<0) d=0.0;

  getOptName(SPEC_LINE_MIN,name1);
  getOptName(SPEC_LINE_MAX,name2);
  //Winfoprintf("## thick %s %s %f",name1,name2,d);

  set_spectrum_thickness(name1, name2, d);

  exp_h = (float) expf_dir(HORIZ);
  exp_v = (float) expf_dir(VERT);
  for (plusminus=colors; plusminus<=colorend; plusminus++)
    { if (!colorflag) color(CYAN);
      pbuf1 = 0; pbuf2 = 1;
/*      cont = 4e-5 / vs;
      temp = (int) (th + 0.5);
      while (temp>0) { cont *= 2.0; temp--; }
      cont = 8.0 / (256.0 * cont); */
      cont = vs2d * 512.0;
      temp = (int) (th + 0.5);
      while (temp>0) { cont /= 2.0; temp--; }
      if (plusminus==1) cont = -cont;
      f1 = fpnt1; f2 = fpnt1 + npnt1 - 1;
      for (l=0; l<=npnt1; l++) fulltrace[l] = 1;
      /* go through loop for each contour level */
      for (l=1; l<=numcont; l++)
	{ disp_index(l);
	  nf1 = f2; nf2 = f1;
	  yval0 = dfpnt2+1;
	  xval0 = dfpnt+1;
	  oldcontour = 0;

/*  The first argument to "changecolor" was originally "l+1".
    I cannot explain this and find that a value of "l" works.	*/

	  if (colorflag) changecolor(l,plusminus,phase_display);
	  if ((phasfl=gettrace(f1,fpnt))==0)
           {  release(pbuf[0]); release(pbuf[1]); release(fulltrace);
	     ABORT;
	   }
          if (!plot)  grf_batch(1);
	  scfix1(phasfl,1,cont,pbuf[pbuf2],1,npnt);
          incr = 0;
	  /* go through each trace */
	  for (trace = f1+1; trace <= f2; trace++)
	    { temp = pbuf1; pbuf1 = pbuf2; pbuf2 = temp;
	      if (fulltrace[trace-fpnt1]||oldcontour||fulltrace[trace+1-fpnt1])
                { if ((phasfl=gettrace(trace,fpnt))==0) 
                    { release(pbuf[0]); release(pbuf[1]);
		      release(fulltrace);
                      if (!plot)  grf_batch(0);
		      ABORT;
		    }
		  scfix1(phasfl,1,cont,pbuf[pbuf2],1,npnt);
		  newcontour = zcon3(pbuf[pbuf2],npnt);
		  if (!newcontour) fulltrace[trace-fpnt1] = 0;
		} 
              else newcontour = 0;
	      if (newcontour)
		{ if (trace<nf1) nf1 = trace;
		  if (trace>nf2) nf2 = trace;
		  contour(f1-fpnt1+incr,pbuf[pbuf1],pbuf[pbuf2],exp_h,exp_v);
		}
              else if (oldcontour)
		contour(f1-fpnt1+incr,pbuf[pbuf1],pbuf[pbuf2],exp_h,exp_v);
	      incr++;
              if (sort)
	        if ((trace==f2) ||
		  ((!fulltrace[trace-fpnt1])&&(fulltrace[trace-1-fpnt1])))
		  dumpbuffer();
	    } /* end of loop through each trace */
          cont = cont/contmult;
	  if (f1<nf1-1) f1 = nf1 - 1;
	  if (f2>nf2+1) f2 = nf2 + 1;
          if (!plot) {
             flush_draw();
             grf_batch(0);
          }
	} /* end of loop for each level */
    } /* end of loop for plusminus */
  release(pbuf[0]); release(pbuf[1]); release(fulltrace);
  disp_index(0);
  RETURN;
}

void init_dpcon()
{
  double d;
  int color_pos, color_neg;

   if(P_getreal( GLOBAL, "mf_color_pos", &d, 1 )) color_pos = 0;
   else color_pos = (int)d;
   if(P_getreal( GLOBAL, "mf_color_neg", &d, 1 )) color_neg = 0;
   else color_neg = (int)d;
   if(P_getreal( GLOBAL, "mf_numcont", &d, 1 )) numcont = 4;
   else numcont = (int)d;
   if(P_getreal( GLOBAL, "mf_contmult", &d, 1 )) contmult = 2.0;
   else contmult = d;
   if(P_getreal( GLOBAL, "colorMode", &d, 1 )) b_colors = 0;
   else b_colors = (int)d;

   if(color_pos > 0 && color_neg > 0) colors = 0;
   else if(color_pos > 0) colors = 1;
   else if(color_neg > 0) colors = 2;
}

/****************************/
int dpcon(argc,argv,retc,retv)
/****************************/
int argc; char *argv[]; int retc; char *retv[];
/* display contour map 						*/
/* dpcon(<colormode>,<numconts<,scalemult>>)			*/
/* numconts		number of contours, default=4		*/
/* scalemult		vertical multiplier between contours,	*/
/*			default = 2.0				*/
{ int argnumber;
  int phase_disp;
  int doaxis;
  int redisp;
  /* decode arguments */

  Wturnoff_buttons();
  /* initialize colors,numcont,contmult,colorflag */
  colors = 0; numcont = 4; contmult = 2.0;	/* defaults */
  colorflag = 1;
  doaxis = 1;
  redisp = 0;
  argnumber = 1;

  init_dpcon();

  if ((argc>argnumber)&&(isalpha(argv[1][0])))
    { if (strcmp(argv[1],"pos")==0)
        colors = 1;
      else if (strcmp(argv[1],"neg")==0)
        colors = 2;
      else if (strcmp(argv[1],"noaxis")==0)
        doaxis = 0;
      else if (strcmp(argv[1],"redisplay")==0)
        redisp = 1;
      else
        { Werrprintf ("expecting first argument pos or neg or noaxis");
          ABORT;
        }
      argnumber++;
      if ((argc>argnumber)&&(isalpha(argv[2][0])))
      { if (strcmp(argv[2],"pos")==0)
          colors = 1;
        else if (strcmp(argv[2],"neg")==0)
          colors = 2;
        else if (strcmp(argv[2],"noaxis")==0)
          doaxis = 0;
        else if (strcmp(argv[2],"redisplay")==0)
          redisp = 1;
        else
          { Werrprintf ("expecting second argument pos or neg or noaxis");
            ABORT;
          }
        argnumber++;
        if ((argc>argnumber)&&(isalpha(argv[3][0])))
        { if (strcmp(argv[3],"redisplay")==0)
          redisp = 1;
        }
      }
    }
   if (argc>argnumber)
     { sscanf(argv[argnumber],"%d",&numcont);
       if ((numcont<1)||(numcont>99))
         { Werrprintf("illegal number of contours");
           ABORT;
         }
       argnumber++;
     }
   if (argc>argnumber)
     { sscanf(argv[argnumber],"%f",&contmult);
       if ((contmult<1.01)||(contmult>1000.0))
         { Werrprintf("illegal contour scale multiplier"); ABORT;
         }
     }
   if ((strcmp(argv[0],"dpcon")==0) || (strcmp(argv[0],"ddcon")==0))
     { if (argv[0][1]=='d') colorflag = 0;
     }
  if (argv[0][0]=='p') 
    { if (i_dpcon(2)) ABORT;	/* plot on plotter */
    }
  else
    {
      if (!plot)  grf_batch(1);
      if (redisp)
        erase_dcon_box();
      else
      { if (i_dpcon(1)) {
             if (!plot)  grf_batch(0);
             ABORT;	/* display on screen */
        }
        if (argv[0][5]!='n') Wclear_graphics();
        Wshow_graphics();
        dcon_displayparms();
      }
    }
  /* now display the scale */
  if (doaxis && !redisp)
     scale2d(1,0,1,SCALE_COLOR);
                       /* first argument causes the entire box to be drawn  */
                       /* second argument is the vertical offset of the box */
                       /* third argument is non-zero because this is a draw */
                       /*  operation, not an erase                          */
                       /* fourth argument is the color                      */
  phase_disp = (get_phase_mode(HORIZ) && get_phase_mode(VERT));
  if (WisSunColor() && !plot && !redisp)
    { if (phase_disp)
        setcolormap(FIRST_PH_CLR,NUM_PH_CLRS,0,1);
      else
        setcolormap(FIRST_AV_CLR,NUM_AV_CLRS,0,0);
    }
  contourplot(phase_disp);
  if (!plot)  grf_batch(0);
  if (redisp) RETURN;
  releaseAllWithId("dpcon");
  D_allrelease();
  endgraphics();
  if (!plot)
  {
    // Wsetgraphicsdisplay("dpcon");
    // Wsetgraphicsdisplay("dconi");
    Wsetgraphicsdisplay(argv[0]);
    disp_specIndex(currentindex());
  }
  RETURN;
}
