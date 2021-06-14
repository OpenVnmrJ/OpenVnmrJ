/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
#include <stdio.h>
#include <string.h>
#include "group.h"
#include "acodes.h"
#include "acqparms2.h"
#include "abort.h"

#define	TRUE	1

extern double	getval();
extern char	gradtype[];
 
int grad_flag;
static int warnclipDone = 0;
static int warnclipix = 1;
static int get_pfg_base(char where);
void rgradient(char axis, double value);
void calc_amp_tc();
void pfg_reset(char where);
void pfg_blank(char where);
void pfg_enable(char where);

/************************************************************************/
/*									*/
/*     gradtype = 'wwww'  wavegen control				*/
/*		= 'ssss'  sisco old style				*/
/*		= 'nnnn'  none (abort)					*/
/*		= 'pppp'  NMRI pfg without waveformer			*/
/*		= 'qqqq'  NMRI pfg with waveformer			*/
/*		= 'llll'  NMRI low power PFG twelve bits		*/
/*		= 'hhhh'  homospoil gradient				*/
/*									*/
/* gradtype='l','a','n' supported for G2000, 'h' same as 'a'		*/
/* gradtype='l','h','a','n' supported for Mercury                       */
/*									*/
/************************************************************************/

static void warnclip(val,cval,what)
int	val,cval;
char	what;
{  
   char msg[MAXSTR];
   if ((ix <= warnclipix) || (! warnclipDone))
   {
      sprintf(msg,"Gradient %c out of range, %d clipped to %d\n",what,val,cval);
      text_error(msg);
      warnclipDone = 1;
      warnclipix = ix;
   }
}

static int bound12(x,what)
int	x;
char	what;
{  
   if (x > 2047) 
   {   warnclip(x,2047,what);
       x = 2047;
   }
   if (x < -2048) 
   {   warnclip(x,-2048,what);
       x = -2048;
   }
   return(x);
}

static float rbound16(x,what)
float x;
char what;
{
   if (x > 32767.0)
   {
     warnclip((int)x,32767,what);
     x = 32767.0;
   }
   if (x < -32768.0)
   {
     warnclip((int)x,-32768,what);
     x = -32768.0;
   }
   return(x);
}

void VAGRAD(double glevel)
{
   if ((gradstepsz == 0.0) || (gmax == 0.0) )
   {  abort_message("vagradient(): 'gradstepz' or 'gmax' is zero");
   }
   rgradient('z',glevel*gradstepsz/gmax);
}

static void rgradientCodes(char gid, double rgamp)
{
int ix;
   switch ( tolower(gid) )
   {  case 'n': return;
                break;
      case 'z': ix = 2;
                break;
      case 'x': ix = 0;
		if (tolower(gradtype[ix]) == 'l')
                {
                  abort_message("rgradient(): only 'z' gradient is supported, aborted");
                }
                else
                  break;
      case 'y': ix = 1;
		if (tolower(gradtype[ix]) == 'l')
                {
                  abort_message("rgradient(): only 'z' gradient is supported, aborted");
                }
                else
                  break;
      default:  abort_message("rgradient(): only 'z' gradient is supported, aborted");
                break;
   }
   delay(2e-7); /* set HSL */
   switch ( tolower(gradtype[ix]) )
   {  case 'p': pgradient(gid,rgamp); break;
      case 'l': lgradient(gid,rgamp); break;
      case 'h': hgradient(gid,rgamp); break;
      case 'a': shimgradient(gid,rgamp); break;
      case 'n': break;
      default:  abort_message("rgradient(): only 'p','l','h','a' type gradients supported");
                break;
   }
}

void rgradient(char axis, double value)
{
   if (value == 0.0)
      rgradientCodes(axis, value);
   else if (gradalt == 1.0)
      rgradientCodes(axis, value);
   else
   {
      ifmod2zero(ct);
         rgradientCodes(axis, value);
      elsenz(ct);
         rgradientCodes(axis, gradalt * value);
      endif(ct);
   }
}

hgradient(gid,gamp)
char gid; double gamp;
{
  char mess[MAXSTR];

  if (hgradient_present() != 0)
  {
/*    text_error("rgradient(): only 'l','h','a' type gradients supported");
    text_error("             use 'config' to correct, aborted");
    abort(1);
*/
    shimgradient(gid,gamp); /* send to shimgradient for g2000 */
  }
  switch (gid)
  {
    case 'x': case 'X': case 'y': case 'Y':
      abort_message("homospoil gradient '%c' not available\n",gid);
      break;
    case 'z': case 'Z':
      hgradient_set((int)gamp);
      break;
    default: ;
  }
  grad_flag = TRUE;
}

shimgradient(gid, gamp)
char gid; double gamp;
{
  int val;
  double dshimval=0.0;

  switch (gid)
  {
    case 'z': case 'Z':
      if (P_getreal(CURRENT,"z1c",&dshimval,1) != 0)
      {
        abort_message("parameter z1c not found\n");
      }
      break;
    case 'x': case 'X':
      if (P_getreal(CURRENT,"x1",&dshimval,1) != 0)
      {
        abort_message("parameter x1 not found\n");
      }
      break;
    case 'y': case 'Y':
      if (P_getreal(CURRENT,"y1",&dshimval,1) != 0)
      {
        abort_message("parameter y1 not found\n");
      }
      break;
    default:
      abort_message("invalid gradient type '%c'\n", gid);
      break;
  }
  val = ((int)(gamp + dshimval + 0.5));
  val = bound12(val, gid);
  shimgradient_set(gid, val);
  grad_flag = TRUE;
}

zero_all_gradients()
{	
  if (tolower(gradtype[2]) == 'p') rgradient('z',0.0);
  if (tolower(gradtype[2]) == 'l') rgradient('z',0.0);
  if (tolower(gradtype[2]) == 'h') rgradient('z',0.0);
  if (tolower(gradtype[2]) == 'a') rgradient('z',0.0);
  if (tolower(gradtype[0]) == 'a') rgradient('x',0.0);
  if (tolower(gradtype[1]) == 'a') rgradient('y',0.0);
}

int getorientation(c1,c2,c3,orientname)
char  *c1,*c2,*c3,*orientname;
{
   char orientstring[MAXSTR];
   int i;
   getstr(orientname,orientstring);
   if (orientstring[0] == '\0') 
   {
   abort_message("can't find variable in tree\n");
   }
/*     fprintf(stderr,"orientname = %s\n",orientname); */
   for (i=0;i<3;i++) 
   {
     switch(orientstring[i])
     {
       case 'X': case 'x':	
       case 'Y': case 'y': 
       case 'Z': case 'z':
       case 'R': case 'r':
       case 'N': case 'n':
       break;
       default: return(-1);
     }
   }
   *c1 = orientstring[0];
   *c2 = orientstring[1];
   *c3 = orientstring[2];
   return(0);
}
/********************************************************/
/*							*/
/********************************************************/
/*							*/
/*	all_grad_reset sets all gradients to proper 	*/
/*	state turns on PFG if set to y			*/
/*							*/
/********************************************************/
all_grad_reset()
{
char	pfg_on[MAXSTR];

   getstr2nwarn("pfgon",pfg_on);

   if (tolower(gradtype[2]) == 'l')
   {  l_reset('z');
      lgradient('z',0.0);
      if (tolower(pfg_on[2]) == 'y') 
         l_cntrl('z',1);
      else
         l_cntrl('z',0);
   }
   if (tolower(gradtype[2]) == 'p')
   {  pgradient('z',0.0);
      if (tolower(pfg_on[2]) == 'y')
         pfg_enable('z');
      else
         pfg_blank('z');
   }
}
/********************************************************/
/*	getstr2nwarn()/2				*/
/*	returns string value of variable from two trees	*/
/*	current then global				*/
/********************************************************/
getstr2nwarn(variable,buf)
char *variable;
char buf[];
{
   if (P_getstring(CURRENT, variable, buf, 1, MAXSTR))
   {  if (P_getstring(GLOBAL, variable, buf, 1, MAXSTR))
      strcpy(buf,"nnnn"); 
   }
}
 
ecc_handle()
{
  if(tolower(gradtype[0]) == 'p')
  {
    calc_amp_tc('x',1,0.0, 0.23);
    calc_amp_tc('x',2,0.0, 2.30);
    calc_amp_tc('x',3,0.0,165.0);
    calc_amp_tc('x',4,0.0, 23.0);
  }
  if(tolower(gradtype[1]) == 'p')
  {
    calc_amp_tc('y',1,0.0, 0.23);
    calc_amp_tc('y',2,0.0, 2.30);
    calc_amp_tc('y',3,0.0,165.0);
    calc_amp_tc('y',4,0.0, 23.0);
  }
  if(tolower(gradtype[2]) == 'p')
  {
    calc_amp_tc('z',1,0.0, 0.23);
    calc_amp_tc('z',2,0.0, 2.30);
    calc_amp_tc('z',3,0.0,165.0);
    calc_amp_tc('z',4,0.0, 23.0);
  }
}

/* gradtype='p' calls */
pgradient(where,value)
int where;
float value;
{
int	base,tmp;
   base = get_pfg_base(where);
   if (base == 0)
      return;
   value = rbound16(value,where);
   value *= 16.0;  /* scale to 20 bits */
   tmp = (int) value;
   pfg_20(tmp,base);
   grad_flag = TRUE;  /* enable kill function */
}

pfg_20(num20,base)
int num20, base;
{
   int word[7],mask,nmask;
   mask = base & 0x0fc00;
   word[0] = base | 0xA000; /* select the base register */
   word[1] = mask | 0xB000; /* set to set point dac     */
   word[2] = base | 0xA001; /* select the base register */
   nmask = (num20 & 0x000f0000) >> 16;
   word[3] = mask | 0xB000 | nmask;
   nmask = (num20 & 0x0000ff00) >> 8;
   word[4] = mask | 0xB000 | nmask;
   nmask = (num20 & 0x000000ff);
   word[5] = mask | 0xB000 | nmask;
   word[6] = mask | 0xB000;
   /* strobe word if necessary and output */
   putcode(APBOUT);
   putcode(5);                 /* 6 words - 1 */
   for (mask = 0; mask < 6; mask++)
     putcode(word[mask]);
   curfifocount += 6;
}

double tcmax[4]={ 0.235,2.35,165.0,23.5};
/************************************************************************/
/*	this routine is the interface to the Highland amp tc pairs      */
/*	these can only be written as pairs 				*/
/*      chan = 'x','y','z',  tcno = {1,2,3,4}				*/
/*      0<= amp <=100,  tcmax/11 <= tc <= tcmax			 	*/
/************************************************************************/
void calc_amp_tc(chan,eccno,amp,tc)
char chan;
int eccno;
double amp,tc;
{
     int amp12,tc12;
     double xx,tc_min,tc_max;
     char tmp[64],tchn;
     tchn = tolower(chan); 
     if (!((tchn == 'x') || (tchn == 'y') || (tchn == 'z')))
     {
       abort_message("illegal compensation channel");
     }
     if (!((eccno > 0) || (eccno <= 5)))
     {
       abort_message("illegal compensation number");
     }
     xx = amp;
     xx *= 40.950;
     amp12 = (int) (xx + 0.5);
     if (amp12 > 4095)
	 amp12 = 4095;
     if (amp12 < 0)
	 amp12 = 0;
     /* k is the amplitude word */
     tc_max = tcmax[eccno-1];
     tc_min = 0.089*tc_max;  /*  let 1% over/under slide thru and clip */
     if ((tc > 1.01*tc_max) || (tc < tc_min))
     {
        sprintf(tmp,"%c, #%d out of range [%f,%f]\n",chan,eccno,tc_min,tc_max);
        text_error(tmp);
        return;
     }
     xx =  (tc_max/tc - 1.0)*409.5;
     tc12 = (int) (xx + 0.5);
     if (tc12 > 4095)
	 tc12 = 4095;
     if (tc12 < 0)
	 tc12 = 0;
     pfg_12s(chan,eccno,tc12,amp12); /* order */
     delay(0.000020);  /* HARD CODED 20 microseconds to let dacs update */
}

void pfg_reset(char where) /* THIS ACTION TAKES 50 MILLISECONDS */
{
   /* reset but not enabled */
   pfg_reg(where,2,2);
   pfg_reg(where,2,0);
}

void pfg_blank(char where)
{
   /* disable - no reset */
   pfg_reg(where,2,0);
}

void pfg_enable(char where)
{
   /* enable - no reset */
   pfg_reg(where,2,1);
}

pfg_select_addr(which,where)
int which;
char where;
{
   pfg_reg(where,0,(which & 0x3));
}

pfg_12s(id,which,top12,bot12)
char id;
int which,top12,bot12;
{
   int word[6], mask, nmask, base;
   base = get_pfg_base(id);
   mask = base & 0x0fc00;
   word[0] = base | 0xA000; 		/* select the base register */
   nmask = which & 7; 			/* mask to valid bits       */
   word[1] = mask | 0xB000 | nmask; 	/* to desired dac     	    */
   word[2] = base | 0xA001; 		/* select the base+1 register */
   nmask = top12 >> 4;	/* pick off top 8 	    */
   word[3] = mask | 0xB000 | nmask; 	
   nmask = ((top12 & 0x0f) << 4) | ((bot12 & 0x0f00) >> 8);	
/*   fprintf(stderr,"bot12 = %x  nmask = %x\n",bot12,nmask); */
   word[4] = mask | 0xB000 | nmask; 	
   nmask = bot12 & 0x00ff;	
   word[5] = mask | 0xB000 | nmask; 	
   /* strobe word if necessary and output */
   putcode(APBOUT);
   putcode(5);
   for (mask = 0; mask < 6; mask++)
     putcode(word[mask]);
   curfifocount += 6;
} 

pfg_reg(where,reg,what)
char where;
int reg,what;
{
int word[2],base;
   base = get_pfg_base(where);
   if (base == 0)
      return;
   word[0] = 0xA000 | base | (reg & 0x03);
   word[1] = 0xB000 | (base & 0x0f00) | (what & 0x0ff);
   putcode(APBOUT);
   putcode(1);
   putcode(word[0]);
   putcode(word[1]);
   curfifocount += 2;
}

/* This routine has x,y,z,r axis present */
/* For gradtype='p' call */
static int get_pfg_base(char where)
{
int base;
   switch (where)
   {
     case 'x': case 'X':   base = 0x0C50; break;
     case 'y': case 'Y':   base = 0x0C54; break;
     case 'z': case 'Z':   base = 0x0C58; break;
     case 'r': case 'R':   base = 0x0C5c; break;
     case 'n': case 'N':   base = 0; break;
     default:
     {
         abort_message("Unrecognized PFG channel = %c\n",where);
     }
   }
   return(base);
}

/* This routine has only z axis */
/* gradtype='l' calls */
static int get_l_pfg_base(where)
char where;
{
int base;
   switch ( tolower(where) )
   { case 'n': base = 0; break;
     case 'z': base = 0x0C68; break;
     default:  abort_message("Unrecognized PFG channel = %c\n",where);
               break;
   }
   return(base);
}

lgradient(who,howmuch)
char who;
double howmuch;
{
int temp,i,base;
short obuff[7];
   grad_flag = TRUE;
   temp = (int) howmuch;
   temp = bound12(temp,who);
   base = get_l_pfg_base(who); 
   obuff[0] = 3;
   obuff[1] = 0xA000 | base;
   obuff[2] = 0xB000 | (base & 0x0f00) | (temp & 0x0ff);
   obuff[3] = 0xA000 | ((base + 1) & 0x0fff);
   obuff[4] = 0xB000 | (base & 0x0f00) | ((temp >> 8) & 0x0ff);
   putcode(APBOUT);
   for (i=0; i< 5; i++)
       putcode(obuff[i]);
}

l_reset(who)
char who;
{
short obuff[10];
int i,base;
   base = get_l_pfg_base(who);
   obuff[0] = 3;
   obuff[1] = 0xA002 + (base & 0x0fff);
   obuff[2] = 0xB002 | (base & 0x0f00);
   obuff[3] = obuff[1];
   obuff[4] = 0xB000 | (base & 0x0f00);
   putcode(APBOUT);
   for (i=0; i< 5; i++)
       putcode(obuff[i]);
}

l_cntrl(who,tog)
char who;
int tog;
{
short obuff[10];
int i,base;
   base = get_l_pfg_base(who);
   base &= 0x0fff;
   obuff[0] = 1;
   obuff[1] = 0xA002 + base;
   obuff[2] = 0xB000 | (base & 0x0f00) | (tog & 3);
   putcode(APBOUT);
   for (i=0; i< 3; i++)
       putcode(obuff[i]);
}

void zgradpulse(double gval,double gdelay)
{
   if (gdelay > 2e-7)
   {  rgradient('z',gval);
      delay(gdelay);
      rgradient('z',0.0);
   }  
}

lk_sampling_off()
/* dummy function to switch lock sampling off */
/* needed for deuterium gradient shimming */
{
}

settmpgradtype(char *tmpgradname)
{
  char tmpGradtype[MAXSTR];
  if (P_getstring(CURRENT, tmpgradname, tmpGradtype, 1, MAXSTR) == 0)
  {
    if (strlen(tmpGradtype) >= 3)
    {
      strcpy(gradtype,tmpGradtype);
      P_setstring(GLOBAL,"gradtype",tmpGradtype,1);
      checkGradtype();
    }
  }
}
