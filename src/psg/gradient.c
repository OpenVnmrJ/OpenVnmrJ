/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "gradient.h"
#include "macros.h"
#include "aptable.h"
#include "apdelay.h"
#include "abort.h"

#define PFG_APBUS_DELAY_TICKS  72			/* 900 nanosecs */
#define PFGL200_APBUS_DELAY_TICKS  232			/* 2.9 usecs */

#define Z1_DAC 2			/* Using shims for gradients */
#define Z1C_DAC 3			/* Using shims for gradients */
#define X1_DAC 16			/* Using shims for gradients */
#define Y1_DAC 17			/* Using shims for gradients */

extern double getval();
extern int  bgflag;     /* debug flag */
extern char gradtype[];
extern void setHSLdelay();
extern int homospoil_bit;

extern int newacq;
 
int grad_flag;

double gradxval, gradyval, gradzval;

static int warningDone = 0;
static int warnclipDone = 0;
static int warnclipix = 1;

static void shimgradient(char gid, int gamp);
static void hgradient(char gid, int gamp);
static void Sgradient(char gid, int gamp);
static void Svgradient(char gid, int gamp0, int gampi, codeint mult);
static void Wgradient(char gid, int gamp);
static void Wvgradient(char gid, int gamp0, int gampi, codeint mult);
static void Pvgradient(char where, int gamp0, int gampi, codeint mult);
static void Pincgradient(char gid, int gamp0, int gamp1, int gamp2, int gamp3,
                         codeint mult1, codeint mult2, codeint mult3);
static void Tvgradient(char where, int gamp0, int gampi, codeint mult);
static void Tincgradient(char gid, int gamp0, int gamp1, int gamp2, int gamp3,
                         codeint mult1, codeint mult2, codeint mult3);
static int get_l_pfg_base(char where);
static int get_t_pfg_base(char where);
void calc_amp_tc(char chan, int eccno, double amp, double tc);

/*
   if dynamic call then value = gamp0+gampi*mult;
   else value = gamp0;
   all results are bounded by 12 bits +/- 2048
   mult is a dynamic variable v1..v14  


   this file has a NEW CONFIG SWITCH 
   gradtype = 'rrrr'  coordinate rotator board
	    = 'wwww'  wavegen control
	    = 'ssss'  sisco old style
            = 'nnnn'  none (abort)
	    = 'cccc'  NMRI L650 pfg without waveformer
	    = 'dddd'  NMRI L650 pfg with waveformer
	    = 'pppp'  NMRI L700 pfg without waveformer
	    = 'qqqq'  NMRI L700 pfg with waveformer
	    = 'tttt'  NMRI L200 pfg 3-axis amp without waveformer
	    = 'uuuu'  NMRI L200 pfg 3-axis amp with waveformer
	    = 'llll'  NMRI L600 low power PFG twelve bits 
	    = 'aaaa'  NMRI shims
            = 'hhhh'  homospoil pulse (z only)
*/

static void warnclip(val,cval,what)
int val,cval;
char what;
{  
   char msg[MAXSTR];
   if ((ix <= warnclipix) || (! warnclipDone))
   {
      sprintf(msg,"Gradient %c set out of range %d clipped to %d\n",what,val,cval);
      text_error(msg);
      warnclipDone = 1;
      warnclipix = ix;
   }
}

static int bound12(x,what)
int x;
char what;
{  
   if (x > 2047) 
   {
     warnclip(x,2047,what);
     x = 2047;
   }
   if (x < -2048) 
   {
     warnclip(x,-2048,what);
     x = -2048;
   }
   return(x);
}

static int bound16(x,what)
int x;
char what;
{
   if (x > 32767) 
   {
     warnclip(x,32767,what);
     x = 32767;
   }
   if (x < -32768) 
   {
     warnclip(x,-32768,what);
     x = -32768;
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

void settmpgradtype(char *tmpgradname)
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

void rgradientCodes(char gid, double rgamp)
{ 
  int ix, gamp;
  switch (gid)
  {
    case 'x': case 'X':  ix = 0;
                         gradxval = rgamp;
                         break;
    case 'y': case 'Y':  ix = 1;
                         gradyval = rgamp;
                         break;
    case 'z': case 'Z':  ix = 2;
                         gradzval = rgamp;
                         break;
    case 'n': case 'N':  break;
    default: text_error("illegal gradient case"); psg_abort(1);
  }
  setHSLdelay();
  if ((gid == 'n') || (gid == 'N'))
    return;
  gamp = (int) rgamp;
  switch (gradtype[ix])
  {
    case 'W': case 'w':	Wgradient(gid,gamp); break;
    case 'R': case 'r':	Wgradient(gid,gamp); break;
    case 'S': case 's':	Sgradient(gid,gamp); break;
    case 'C': case 'c':	tgradient(gid,rgamp); break;
    case 'D': case 'd':	tgradient(gid,rgamp); break;
    case 'P': case 'p':	pgradient(gid,rgamp); break;
    case 'Q': case 'q':	pgradient(gid,rgamp); break;
    case 'L': case 'l':	lgradient(gid,rgamp); break;
    case 'T': case 't':	tgradient(gid,rgamp); break;
    case 'U': case 'u':	tgradient(gid,rgamp); break;
    case 'A': case 'a':	shimgradient(gid,gamp); break;
    case 'H': case 'h':	hgradient(gid,gamp); break;
    default:  text_error("no gradients configured");	psg_abort(1);
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

void S_vgradient(gid,gamp0,gampi,mult)
char gid;
int gamp0,gampi;
codeint mult;
{ 
  int ix;
  setHSLdelay();
  switch (gid)
  {
    case 'x': case 'X':  ix = 0; break;
    case 'y': case 'Y':  ix = 1; break;
    case 'z': case 'Z':  ix = 2; break;
    case 'n': case 'N':  break;
    default: text_error("illegal gradient case"); psg_abort(1);
  }
  if ((gid == 'n') || (gid == 'N'))
    return;
  switch (gradtype[ix])
  {
    case 'W': case 'w':	
    case 'R': case 'r':	Wvgradient(gid,gamp0,gampi,mult); break;
    case 'S': case 's':	Svgradient(gid,gamp0,gampi,mult); break;
    case 'P': case 'p':	
    case 'Q': case 'q':	Pvgradient(gid,gamp0,gampi,mult); break;
    case 'C': case 'c':	
    case 'D': case 'd':	
    case 'T': case 't':	
    case 'U': case 'u':	Tvgradient(gid,gamp0,gampi,mult); break;
    case 'L': case 'l': text_error("no vgradient for case l");
    default:  text_error("no gradients configured");	psg_abort(1);
  }
}

/*	S_incgradient implements incgradient for the DAC board.
 */
void S_incgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3)
 char gid;
 int gamp0;
 int gamp1, gamp2, gamp3;
 codeint mult1, mult2, mult3;
{ 
  int ix;

  setHSLdelay();
  switch (gid)
  {
    case 'x': case 'X':  ix = 0; break;
    case 'y': case 'Y':  ix = 1; break;
    case 'z': case 'Z':  ix = 2; break;
    case 'n': case 'N':  break;
    default: text_error("illegal gradient case\n"); psg_abort(1);
  }
  if ((gid == 'n') || (gid == 'N'))
    return;
  switch (gradtype[ix])
  {
    case 'W': case 'w':	
    case 'R': case 'r':	
	Wincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3); 
	break;
    case 'S': case 's':	
	Sincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3); 
	break;
    case 'P': case 'p':	
    case 'Q': case 'q':	
	Pincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3); 
	break;
    case 'C': case 'c':	
    case 'D': case 'd':	
    case 'T': case 't':	
    case 'U': case 'u':	
	Tincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3); 
	break;
    case 'L': case 'l': text_error("no incgradient for case l");
    default:  text_error("no gradients configured\n");	psg_abort(1);
  }
}

void zero_all_gradients()
{	
  char tag;
  tag = tolower(gradtype[0]);
  if ((tag == 'w') || (tag == 'r') || (tag == 's') || (tag == 'c') || (tag == 'd') || (tag == 'p') || (tag == 'q') || (tag == 'l') || (tag == 't') || (tag == 'u') || (tag == 'a'))
    gradient('x',0);
  tag = tolower(gradtype[1]);
  if ((tag == 'w') || (tag == 'r') || (tag == 's') || (tag == 'c') || (tag == 'd') || (tag == 'p') || (tag == 'q') || (tag == 'l') || (tag == 't') || (tag == 'u') || (tag == 'a'))
    gradient('y',0);
  tag = tolower(gradtype[2]);
  if ((tag == 'w') || (tag == 'r') || (tag == 's') || (tag == 'c') || (tag == 'd') || (tag == 'p') || (tag == 'q') || (tag == 'l') || (tag == 't') || (tag == 'u') || (tag == 'a') || (tag == 'h'))
    gradient('z',0);
}

static void shimgradient(char gid, int gamp)
{ 
  double shimval,dshimset;
  int tamp,dac_num,shimamp;
  char console_name[MAXSTR], mess[MAXSTR];

  if (P_getstring(CURRENT, "console", console_name, 1, MAXSTR) != 0)
  {
    sprintf(mess,"PSG: parameter console not found\n");
    text_error(mess);
    psg_abort(1);
  }
  if ( P_getreal(GLOBAL,"shimset",&dshimset,1) != 0 )
  {
    text_error("PSG: shimset not found.\n");
    psg_abort(1); 
  } 

  switch (gid) {
    case 'x': case 'X':  shimval = getval("x1"); dac_num = X1_DAC; break;
    case 'y': case 'Y':  shimval = getval("y1"); dac_num = Y1_DAC; break;
    case 'z': case 'Z':  
	switch ((int)(dshimset+0.5)) {
	  case 1: case 2: case 5: case 10:
	    shimval = getval("z1c"); dac_num = Z1C_DAC; break;
	  default:
    	    shimval = getval("z1"); dac_num = Z1_DAC; break;
	}
	break; 
    default:
      sprintf(mess,"shimgradient %c not available\n",gid);
      text_error(mess);
      psg_abort(1); 
  }
  shimamp = IRND(shimval);
  /* use shimamp=0 to switch off shimgradient */

  gamp = gamp + shimamp;
  switch ((int)(dshimset+0.5)) { 	/* bounds */
    case 1: case 2: case 10: case 11:  tamp = bound12(gamp,gid); break;
    default:  tamp = bound16(gamp,gid); break;
	}
  gamp = tamp;

  putcode(SETSHIM); /* Invalid Acode for inova, acode length mismatch */
  if (!newacq)
    putcode(1);	/* number of shims to set */
  putcode(dac_num);
  putcode(gamp);
  grad_flag = TRUE;
}

static void hgradient(char gid, int gamp)
{
  char mess[MAXSTR];
  switch (gid) 
  {
    case 'x': case 'X': case 'y': case 'Y':
      sprintf(mess,"homospoil gradient %c not available\n",gid);
      text_error(mess);
      psg_abort(1);
      break;
    case 'z': case 'Z': break;
    default: ;
  }
  if (gamp == 0) /* homospoil_off */
  {
    if (newacq)
        {
        putcode(IHOMOSPOIL);
        putcode(FALSE);
        }
    else
        HSgate(homospoil_bit,FALSE);

  }
  else /* homospoil_on */
  {
    if (newacq)
        {
        putcode(IHOMOSPOIL);
        putcode(TRUE);
        }
    else
        HSgate(homospoil_bit,TRUE);
  }
  grad_flag = TRUE;
}

static void Sgradient(char gid, int gamp)
{ int tamp,dac_num;
  char mess[MAXSTR];

  validate_imaging_config("Sgradient");

  tamp = bound12(gamp,gid); /* bounds */
  switch (gid) {
/* these should be externally keyed to the system type */
    case 'x': case 'X':  dac_num = XGRAD; break;
    case 'y': case 'Y':  dac_num = YGRAD; break;
    case 'z': case 'Z':  dac_num = ZGRAD; break;
    default:
      sprintf(mess,"dac case bad = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
  }
  putcode(GRADIENT);
  putcode(dac_num);
  putcode(tamp);
  curfifocount += 4;
  grad_flag = TRUE;
}

static void Svgradient(char gid, int gamp0, int gampi, codeint mult)
{ 
  char mess[MAXSTR];
  int dac_num,dacamp0,dacampi;

  validate_imaging_config("Svgradient");

  /* test valid range */
  if (((mult < v1) || (mult > v14)) && ((mult < t1) || (mult > t60))) {
      sprintf(mess,"mult illegal dynamic %d \n",mult);
      text_error(mess);
      psg_abort(1); 
  }

  if ((mult >= t1) && (mult <= t60))
  {
      mult = tablertv(mult);
  }

  dacamp0 = bound12(gamp0,gid);
  dacampi = bound12(gampi,gid);
  switch (gid) {
/* these should be externally keyed to the system type */
   case 'x': case 'X':  dac_num = XGRAD; break;
   case 'y': case 'Y':  dac_num = YGRAD; break;
   case 'z': case 'Z':  dac_num = ZGRAD; break;
   default:
      sprintf(mess,"dac case bad = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
  }
  putcode(VGRADIENT);
  putcode(dacampi);
  putcode(mult); 
  putcode(dac_num);
  putcode(dacamp0);
  curfifocount += 4;
  grad_flag = TRUE;
}


/*	S_incgradient implements incgradient for the DAC board.
 */
Sincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3)
 char gid;
 int gamp0;
 int gamp1, gamp2, gamp3;
 codeint mult1, mult2, mult3;
{ 
    char mess[MAXSTR];
    int i;
    int dac_num;
    int mult[4];
    int gamp[4];
    int dacamp[4];
    int wg_ap_addr;
    
    validate_imaging_config("Sincgradient");

    /* Copy arguments into arrays. Note that mult[0] isn't used! */
    mult[1] = mult1;
    mult[2] = mult2;
    mult[3] = mult3;
    gamp[0] = gamp0;
    gamp[1] = gamp1;
    gamp[2] = gamp2;
    gamp[3] = gamp3;
    
    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((mult[i] < v1) || (mult[i] > v14))
	    && mult[i] != zero
	    && mult[i] != ct)
	{
	    sprintf(mess,"Sincgradient: mult[%d] illegal RT variable: %d \n",
		    i, mult[i]);
	    text_error(mess);
	    psg_abort(1); 
	}
    }
    
    for (i=0; i<=3; i++){
	dacamp[i] = bound12(gamp[i]);
    }
    switch (gid) {
	/* these should be externally keyed to the system type */
      case 'x': case 'X':  dac_num = XGRAD; break;
      case 'y': case 'Y':  dac_num = YGRAD; break;
      case 'z': case 'Z':  dac_num = ZGRAD; break;
      default:
	sprintf(mess,"Sincgradient(): Bad gradient specified: %c\n",gid);
	text_error(mess);
	psg_abort(1); 
    }
    putcode(INCGRAD);
    putcode(dac_num);
    putcode(mult[1]);
    putcode(dacamp[1]);
    putcode(mult[2]);
    putcode(dacamp[2]);
    putcode(mult[3]);
    putcode(dacamp[3]);
    putcode(dacamp[0]);
    curfifocount += 4;
    grad_flag = TRUE;
}


static void Wgradient(char gid, int gamp)
{ int tamp,wg_ap_addr;
  char mess[MAXSTR];
  tamp = bound16(gamp,gid); /* bounds */
  /* get_wg_apbase is in wg.c */
  wg_ap_addr = get_wg_apbase(gid)+3;
  if (wg_ap_addr < 0) 
  {
      sprintf(mess,"Bad Gradient Specified = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
  }
  if (bgflag)
       fprintf(stderr,"%d   %d from gradient\n",wg_ap_addr,tamp);
  putcode(WG3);
  putcode((codeint) wg_ap_addr);
  putcode((codeint) tamp);
  curfifocount += 3; /* check */
  grad_flag = TRUE;
}


static void Wvgradient(char gid, int gamp0, int gampi, codeint mult)
{ 
  char mess[MAXSTR];
  int wg_ap_addr,dacamp0,dacampi;

  /* test valid range */
  if (((mult < v1) || (mult > v14)) && ((mult < t1) || (mult > t60))) {
      sprintf(mess,"mult illegal dynamic %d \n",mult);
      text_error(mess);
      psg_abort(1); 
  }

  if ((mult >= t1) && (mult <= t60))
  {
      mult = tablertv(mult);
  }

  dacamp0 = bound16(gamp0,gid);
  dacampi = bound16(gampi,gid);
  wg_ap_addr = get_wg_apbase(gid)+3;
  if (wg_ap_addr < 0) 
  {
      sprintf(mess,"Bad Gradient Specified = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
  }
  putcode(WGD3);
  putcode(wg_ap_addr);
  putcode(mult); 
  putcode(dacampi);
  putcode(dacamp0);
  curfifocount += 4;
  grad_flag = TRUE;
}


/*	Wincgradient() implements incgradient() for the gradient WFG boards.
 */
Wincgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3)
 char gid;
 int gamp0;
 int gamp1, gamp2, gamp3;
 codeint mult1, mult2, mult3;
{ 
    char mess[MAXSTR];
    int i;
    int gamp[4];
    int dacamp[4];
    int wg_ap_addr;
    codeint mult[4];

    validate_imaging_config("Wincgradient");

    /* Copy arguments into arrays. Note that mult[0] isn't used! */
    mult[1] = mult1;
    mult[2] = mult2;
    mult[3] = mult3;
    gamp[0] = gamp0;
    gamp[1] = gamp1;
    gamp[2] = gamp2;
    gamp[3] = gamp3;
    
    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((mult[i] < v1) || (mult[i] > v14))
	    && mult[i] != zero
	    && mult[i] != ct)
	{
	    sprintf(mess,"WG_incgradient: mult[%d] illegal RT variable: %d \n",
		    i, mult[i]);
	    text_error(mess);
	    psg_abort(1); 
	}
    }
    
    for (i=0; i<=3; i++){
	dacamp[i] = bound16(gamp[i]);
    }
    wg_ap_addr = get_wg_apbase(gid)+3;
    if (wg_ap_addr < 0){
	sprintf(mess,"WG_incgradient: Bad Gradient Specified = %c\n",gid);
	text_error(mess);
	psg_abort(1);
    }
    putcode(INCWGRAD);
    putcode(wg_ap_addr);
    putcode(mult[1]);
    putcode(dacamp[1]);
    putcode(mult[2]);
    putcode(dacamp[2]);
    putcode(mult[3]);
    putcode(dacamp[3]);
    putcode(dacamp[0]);
    curfifocount += 4;
    grad_flag = TRUE;
}
/*
 
   shaped calls and support are in wg.c 

*/

getorientation(c1,c2,c3,orientname)
char  *c1,*c2,*c3,*orientname;
{
   char orientstring[MAXSTR];
   int i;
   getstr(orientname,orientstring);
   if (orientstring[0] == '\0') 
   {
   text_error("can't find variable in tree\n");
   psg_abort(1);
   }
   else if (bgflag)
     fprintf(stderr,"orientname = %s\n",orientname);
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
/*	all_grad_reset sets all gradients to proper 	*/
/*	state turns on PFG if set to y			*/
/*							*/
/********************************************************/
all_grad_reset()
{
    char pfg_on[MAXSTR];
    /* any imaging wfgs */
    if (anygradcrwg || anygradwg || anypfgw)
    {
      command_wg(0x0c20,0x080);
      command_wg(0x0c28,0x080);
      command_wg(0x0c30,0x080);
    }
    if(is_porq(gradtype[0]))
      pfg_reset('x');
    if(is_porq(gradtype[1]))
      pfg_reset('y');
    if(is_porq(gradtype[2]))
      pfg_reset('z');
    if (tolower(gradtype[0]) == 'l')
      l_reset('x');
    if (tolower(gradtype[1]) == 'l')
      l_reset('y');
    if (tolower(gradtype[2]) == 'l')
      l_reset('z');
     getstr2nwarn("pfgon",pfg_on);
     if ((gradtype[0] == 'p') || (gradtype[0] == 'q')) 
     {
       pgradient('x',0.0);
       if ((pfg_on[0] == 'y') || (pfg_on[0] == 'Y'))
         pfg_enable('x');
       else
         pfg_blank('x');
     }
     if ((gradtype[1] == 'p') || (gradtype[1] == 'q')) 
     {
       pgradient('y',0.0);
       if ((pfg_on[1] == 'y') || (pfg_on[1] == 'Y'))
         pfg_enable('y');
       else
         pfg_blank('y');
     }
     if ((gradtype[2] == 'p') || (gradtype[2] == 'q')) 
     {
       pgradient('z',0.0);
       if ((pfg_on[2] == 'y') || (pfg_on[2] == 'Y'))
         pfg_enable('z');
       else
         pfg_blank('z');
     }
    /* 50 MILLISECOND RESET PULSE - BAD THINGS CAN HAPPEN IF REMOVED */
     if ((is_porq(gradtype[0])) || (is_porq(gradtype[1])) || 
         (is_porq(gradtype[2])))
        delay(0.050); 
     if (gradtype[0] == 'l')
     {
       lgradient('x',0.0);
       if ((pfg_on[0] == 'y') || (pfg_on[0] == 'Y'))
         l_cntrl('x',1);
       else
         l_cntrl('x',0);
     }
     if (gradtype[1] == 'l') 
     {
       lgradient('y',0.0);
       if ((pfg_on[1] == 'y') || (pfg_on[1] == 'Y'))
         l_cntrl('y',1);
       else
         l_cntrl('y',0);
     }
     if (gradtype[2] == 'l')
     {
       lgradient('z',0.0);
       if ((pfg_on[2] == 'y') || (pfg_on[2] == 'Y'))
         l_cntrl('z',1);
       else
         l_cntrl('z',0);
     }
     if ((gradtype[0] == 'c') || (gradtype[0] == 'd') || 
         (gradtype[0] == 't') || (gradtype[0] == 'u') || 
         (gradtype[1] == 'c') || (gradtype[1] == 'd') || 
         (gradtype[1] == 't') || (gradtype[1] == 'u') || 
         (gradtype[2] == 'c') || (gradtype[2] == 'd') || 
         (gradtype[2] == 't') || (gradtype[2] == 'u'))
     {
          l200_reset(1);
     } 
     if ((gradtype[0] == 'c') || (gradtype[0] == 'd') ||
         (gradtype[0] == 't') || (gradtype[0] == 'u')) 
     {
       if ((pfg_on[0] == 'y') || (pfg_on[0] == 'Y'))
         t_enbl('x');
       else
         t_dsbl('x');
       tgradient('x',0.0);
     }
     if ((gradtype[1] == 'c') || (gradtype[1] == 'd') ||
         (gradtype[1] == 't') || (gradtype[1] == 'u')) 
     {
       if ((pfg_on[1] == 'y') || (pfg_on[1] == 'Y'))
         t_enbl('y');
       else
         t_dsbl('y');
       tgradient('y',0.0);
     }
     if ((gradtype[2] == 'c') || (gradtype[2] == 'd') ||
         (gradtype[2] == 't') || (gradtype[2] == 'u')) 
     {
       if ((pfg_on[2] == 'y') || (pfg_on[2] == 'Y'))
         t_enbl('z');
       else
         t_dsbl('z');
       tgradient('z',0.0);
     }

}
 /*-----------------------------------------------------------------
 |	getstr2nwarn()/2
 |	returns string value of variable from two trees
 |	current then global
 +------------------------------------------------------------------*/
 getstr2nwarn(variable,buf)
 char *variable;
 char buf[];
 {
     if (P_getstring(CURRENT, variable, buf, 1, MAXSTR))
     {
        if (P_getstring(GLOBAL, variable, buf, 1, MAXSTR))
           strcpy(buf,"nnnn"); 
     }
 }
 
ecc_handle()
{
    char buf[MAXSTR];
    if (P_getstring(GLOBAL,"chiliConf",buf,1,MAXSTR))
    {
       ecc_zero();
       return;
    }
    if (buf[0] == 'E')
      ecc_enable();
    /* if chiliConf defined, but not 'E', skip */

}

ecc_zero()
{
  if(is_porq(gradtype[0]))
  {
    calc_amp_tc('x',1,0.0, 0.23);
    calc_amp_tc('x',2,0.0, 2.30);
    calc_amp_tc('x',3,0.0,165.0);
    calc_amp_tc('x',4,0.0, 23.0);
  }
  if(is_porq(gradtype[1]))
  {
    calc_amp_tc('y',1,0.0, 0.23);
    calc_amp_tc('y',2,0.0, 2.30);
    calc_amp_tc('y',3,0.0,165.0);
    calc_amp_tc('y',4,0.0, 23.0);
  }
  if(is_porq(gradtype[2]))
  {
    calc_amp_tc('z',1,0.0, 0.23);
    calc_amp_tc('z',2,0.0, 2.30);
    calc_amp_tc('z',3,0.0, 2.36);
    calc_amp_tc('z',4,0.0, 23.0);
  }
}

ecc_enable()
 {
    char buf[MAXSTR];
    double amp1z,tc1z,amp2z,tc2z,amp3z,tc3z,amp4z,tc4z;
    if(is_porq(gradtype[2]))
    {
    if (P_getreal(GLOBAL,"amp1z",&amp1z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"amp2z",&amp2z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"amp3z",&amp3z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"amp4z",&amp4z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"tc1z",&tc1z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"tc1z",&tc1z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"tc2z",&tc2z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"tc3z",&tc3z,1) < 0)
      return;
    if (P_getreal(GLOBAL,"tc4z",&tc4z,1) < 0)
      return;
    fprintf(stderr,"mark\n");
    /* do it */
    calc_amp_tc('z',1,amp1z,tc1z);
    delay(0.005);
    calc_amp_tc('z',2,amp2z,tc2z);
    delay(0.005);
    calc_amp_tc('z',3,amp3z,tc3z);
    delay(0.005);
    calc_amp_tc('z',4,amp4z,tc4z);
    delay(0.005);
   }
 }


/************************************************************************
*
*	PFG Highland Technology interface module - 
*				Phil Hornung		12-2-91
*
************************************************************************/
/*  calls in this module 
 pgradient(where,value)   set where amplifier to value
 pfg20 			  should be static
 pfg_12s(top12,bot12,where,base) should have better interface 
 pfg_reset(where) 
 pfg_blank(where)
 pfg_enable(where)
 pfg_select_addr(which,where) 
 pfg_quick_zero(where) 
*/

/************ABORT CALLS**************/

#include "ACQ_SUN.h"

static int get_pfg_base();

pgradient(where,value)
int where;
float value;
{
  int base,tmp;
  char graddisableflag[2];
  if (bgflag)
   fprintf(stderr,"pgradient call on %c with value %f\n",where,value);
   base = get_pfg_base(where);
   if (base == 0) 
     return;
   if ( P_getstring(GLOBAL,"gradientdisable",&graddisableflag,1,2) == 0 )
   {
     if ((graddisableflag[0] == 'y'))
     {
      value = 0.0;
      if ((ix == 1) && (! warningDone)) warn_message("no gradients active for gradientdisable = 'y' \n");
      warningDone = 1;
     }
   }
   value = rbound16(value,where);
   value *= 16.0;  /* scale to 20 bits */
   tmp = (int) value;
   pfg_20(tmp,base);
   grad_flag = TRUE;  /* enable kill function */
} 
   
/************************************************************************/
/*									*/
/*	commands available:						*/
/*	enable 				blank  				*/
/*	set address			zero current dac		*/
/*	reset 								*/
/*									*/
/************************************************************************/
/*									*/
/*	start up reset, enable, set all dacs to zero un-blank 		*/
/*	global parameter control blank on/off ultimately		*/
/*									*/
/************************************************************************/
/* THIS ACTION TAKES 50 MILLISECONDS */


pfg_reset(where)
char where;
{
  if (bgflag)
   fprintf(stderr,"pfg_reset call on %c\n",where);
   /* reset but not enabled */
   pfg_reg(where,2,2);
   pfg_reg(where,2,0);
}

pfg_blank(where)
char where;
{
  if (bgflag)
   fprintf(stderr,"pfg_blank call on %c\n",where);
   /* disable - no reset */
   pfg_reg(where,2,0);
}

pfg_enable(where)
char where;
{
  if (bgflag)
   fprintf(stderr,"pfg_enable call on %c\n",where);
   /* enable - no reset */
   pfg_reg(where,2,1);
}

pfg_select_addr(which,where)
int which;
char where;
{
   int tmp;
  if (bgflag)
   fprintf(stderr,"pfg_select_addr call\n");
   tmp = which & 0x3;
   pfg_reg(where,0,tmp);
}

pfg_quick_zero(where)
char where;
{
  if (bgflag)
   fprintf(stderr,"pfg_quick_zero call on %c\n",where);
   pfg_reg(where,0,8);
   pfg_reg(where,0,0);
}
/*************************************************************
*
*	specific function drivers 
*	-- probably should be static
*************************************************************/
/*************************************************************
*	num20 should be tested by calling function
*	pfg_20 currently points dac - it could be sped up
*	by insuring pointer to correct place
*************************************************************/
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
   putcode((c68int) APBOUT);
   putcode((c68int) 5);			/* 6 words - 1 */
   for (mask = 0; mask < 6; mask++)
     putcode((c68int) word[mask]);
   curfifocount += 6;
#ifdef SHORT_TRANSACTION_MODE
   nmask = (num20 & 0x000f0000) >> 16;
   word[2] = mask | 0x9000 | nmask;
   nmask = (num20 & 0x0000ff00) >> 8;
   word[3] = mask | 0xB000 | nmask;
   nmask = (num20 & 0x000000ff);
   word[4] = mask | 0xB000 | nmask;
   /* strobe word if necessary and output */
   putcode((c68int) APBOUT);
   putcode((c68int) 4);			/* 5 words - 1 */
   for (mask = 0; mask < 5; mask++)
     putcode((c68int) word[mask]);
   curfifocount += 5;
#endif
}

double tcmax[4]={ 0.235,2.35,2.36,23.5};
/************************************************************************/
/*	this routine is the interface to the Highland amp tc pairs      */
/*	these can only be written as pairs 				*/
/*      chan = 'x','y','z',  tcno = {1,2,3,4}				*/
/*      0<= amp <=100,  tcmax/11 <= tc <= tcmax			 	*/
/************************************************************************/
void calc_amp_tc(char chan, int eccno, double amp, double tc)
{
     int amp12,tc12;
     double xx,tc_min,tc_max;
     char tmp[64],tchn;
     tchn = tolower(chan); 
     if (!((tchn == 'x') || (tchn == 'y') || (tchn == 'z')))
     {
       text_error("illegal compensation channel");
       psg_abort(1);
     }
     if (!((eccno > 0) || (eccno <= 5)))
     {
       text_error("illegal compensation number");
       psg_abort(1);
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

static void Pvgradient(char where, int gamp0, int gampi, codeint mult)
{ 
  char mess[MAXSTR];
  int base,dacamp0,dacampi;

  /* test valid range */
  if (((mult < v1) || (mult > v14)) && ((mult < t1) || (mult > t60))) {
      sprintf(mess,"mult illegal dynamic %d \n",mult);
      text_error(mess);
      psg_abort(1); 
  }

  if ((mult >= t1) && (mult <= t60))
  {
      mult = tablertv(mult);
  }

  base = get_pfg_base(where);
  dacamp0 = bound16(gamp0,where);
  dacampi = bound16(gampi,where);
  
  if (base < 0) 
  {
      sprintf(mess,"Bad Gradient Specified = %c \n",where);
      text_error(mess);
      psg_abort(1); 
  }
  putcode(PVGRADIENT);
  putcode(base);
  putcode(mult); 
  putcode(dacampi);
  putcode(dacamp0);
  curfifocount += 4;
  grad_flag = TRUE;
}

static void Pincgradient(char gid, int gamp0, int gamp1, int gamp2, int gamp3,
                         codeint mult1, codeint mult2, codeint mult3)
{ 
    char mess[MAXSTR];
    int i;
    int gamp[4];
    int dacamp[4];
    int base;
    codeint mult[4];


    /* Copy arguments into arrays. Note that mult[0] isn't used! */
    mult[1] = mult1;
    mult[2] = mult2;
    mult[3] = mult3;
    gamp[0] = gamp0;
    gamp[1] = gamp1;
    gamp[2] = gamp2;
    gamp[3] = gamp3;
    
    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((mult[i] < v1) || (mult[i] > v14))
	    && mult[i] != zero
	    && mult[i] != ct)
	{
	    sprintf(mess,"Pincgradient: mult[%d] illegal RT variable: %d \n",
		    i, mult[i]);
	    text_error(mess);
	    psg_abort(1); 
	}
    }
    
    for (i=0; i<=3; i++){
	dacamp[i] = bound16(gamp[i]);
    }
  base = get_pfg_base(gid);
  
  if (base < 0) 
  {
      sprintf(mess,"Bad Gradient Specified = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
  }
    putcode(INCPGRAD);
    putcode(base);
    putcode(mult[1]);
    putcode(dacamp[1]);
    putcode(mult[2]);
    putcode(dacamp[2]);
    putcode(mult[3]);
    putcode(dacamp[3]);
    putcode(dacamp[0]);
    curfifocount += 4;
    grad_flag = TRUE;
}

static void Tvgradient(char where, int gamp0, int gampi, codeint mult)
{ 
  char mess[MAXSTR];
  int base,dacamp0,dacampi;
  char tag;
  tag = tolower(gradtype[2]);

  validate_imaging_config("Tvgradient");

  /* test valid range */
  if (((mult < v1) || (mult > v14)) ) {
      sprintf(mess,"mult illegal dynamic %d \n",mult);
      text_error(mess);
      psg_abort(1); 
  }
  base = get_t_pfg_base(where);
  dacamp0 = bound16(gamp0,where);
  dacampi = bound16(gampi,where);

  /* divide dac units in half for L200 amplifier only */
  if ((tag == 't') || (tag == 'u'))
  {
  dacamp0 = dacamp0/2;
  dacampi = dacampi/2;
  } 
  
  if (base < 0) 
  {
      sprintf(mess,"Bad Gradient Specified = %c \n",where);
      text_error(mess);
      psg_abort(1); 
  }
  t_grad_selectone(where);	/* sets gradient board to accept one 	*/
				/* value for specified axis.		*/
  putcode(TVGRADIENT);
  putcode(base);
  putcode(mult); 
  putcode(dacampi);
  putcode(dacamp0);
  curfifocount += 3;
  grad_flag = TRUE;
}

static void Tincgradient(char gid, int gamp0, int gamp1, int gamp2, int gamp3,
                         codeint mult1, codeint mult2, codeint mult3)
{ 
    char mess[MAXSTR];
    int i;
    int gamp[4];
    int dacamp[4];
    int base;
    codeint mult[4];
    char tag;
    tag = tolower(gradtype[2]);

    validate_imaging_config("Tincgradient");

    /* Copy arguments into arrays. Note that mult[0] isn't used! */
    mult[1] = mult1;
    mult[2] = mult2;
    mult[3] = mult3;
    gamp[0] = gamp0;
    gamp[1] = gamp1;
    gamp[2] = gamp2;
    gamp[3] = gamp3;
    
    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((mult[i] < v1) || (mult[i] > v14))
	    && mult[i] != zero
	    && mult[i] != ct)
	{
	    sprintf(mess,"Pincgradient: mult[%d] illegal RT variable: %d \n",
		    i, mult[i]);
	    text_error(mess);
	    psg_abort(1); 
	}
    }
    
    for (i=0; i<=3; i++){
	dacamp[i] = bound16(gamp[i]);
  	/* divide dac units in half for L200 amplifier only */
  	if ((tag == 't') || (tag == 'u'))
  	{
	dacamp[i] = dacamp[i]/2;
    }
    }
    base = get_t_pfg_base(gid);
  
    if (base < 0) 
    {
      sprintf(mess,"Bad Gradient Specified = %c \n",gid);
      text_error(mess);
      psg_abort(1); 
    }

    t_grad_selectone(gid);	/* sets gradient board to accept one 	*/
				/* value for specified axis.		*/
    putcode(INCTGRAD);
    putcode(base);
    putcode(mult[1]);
    putcode(dacamp[1]);
    putcode(mult[2]);
    putcode(dacamp[2]);
    putcode(mult[3]);
    putcode(dacamp[3]);
    putcode(dacamp[0]);
    curfifocount += 3;
    grad_flag = TRUE;
}

/************************************************************************/
/*	all numbers should be range checked before this routine		*/
/************************************************************************/
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
  if (bgflag)
   fprintf(stderr,"bot12 = %x  nmask = %x\n",bot12,nmask);
   word[4] = mask | 0xB000 | nmask; 	
   nmask = bot12 & 0x00ff;	
   word[5] = mask | 0xB000 | nmask; 	
   /* strobe word if necessary and output */
   putcode((c68int) APBOUT);
   putcode((c68int) 5);
   for (mask = 0; mask < 6; mask++)
     putcode((c68int) word[mask]);
   curfifocount += 6;
} 
/********************************************************
*	write access to PFG registers			*
********************************************************/
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
  putcode((c68int) word[0]);
  putcode((c68int) word[1]);
  curfifocount += 2;
}

static int get_pfg_base(where)
char where;
{
   int base;
   char tstr[32];
   switch (where)
   {
     case 'x': case 'X':   base = 0x0C50; break;
     case 'y': case 'Y':   base = 0x0C54; break;
     case 'z': case 'Z':   base = 0x0C58; break;
     case 'r': case 'R':   base = 0x0C5c; break;
     case 'n': case 'N':   base = 0; break;
     default: 
     {
	 sprintf(tstr,"Unrecognized PFG channel = %c\n",where);
	 text_error(tstr);
	 psg_abort(1);
     }
   }
   return(base);
}
/***********************************************************************
*
*		REFERENCE DATA
*
*	ap buss address C50-C53,C54-C57,C58-C5b,C5c-C5f
*			  x       y       z    reserved
*
*	register 0	
*		bit 0 - bit 2 address of Highland function
*		bit 3 - set to clear current dac to zero
*	register 1
*		all bits data to dac's
*		successive writes. order
*		9cZA, bcBC,bcDE,bcxx => ZABCDE (ABCDE <- setpoint dac)
*		bcxx causes strobes
*
*	register 2
*		bit 0	enable power stage
*		bit 1   reset?
*
*	register 3	status register
*		bit 0	highland ok ^
*
*	APSEL	0xA000   APWRT 0xB000   APWRTI 0x9000
***********************************************************************
*	highland address 0 - setpoint dac
*	highland address 1 - ecc 1
*	highland address 2 - ecc 2
*	highland address 3 - ecc 3
*	highland address 4 - ecc 4
***********************************************************************
*
*	amplitude 0 <= amp <= 4095   33% drive
*	time constants
*	tau = constant[i]/(0.1+(code/4096))
*
*	time constant table (milliseconds)
*	tc#	max		min
*	1	  0.23		 0.0214		
*	2	  2.34		 0.214
*	3	165.0		15
*	4	 23.5		 2.1
*
*	11:1 ratio!
*	tc = tcmax/(1+code/409.6)
*	code = (tmax/tc - 1)*409.5	
*
*	jumper settings ap address SW2
*	address		pos1	pos2	logical
*	c50-c53		on	on	X
*	c54-c57		on	off	Y
*	c58-c5b		off	on	Z
*
***********************************************************************/
/********* 3 AMP L600 implementation ***************/
/* lgradient is a 12 bit DAC */
/* lreset */
/* lenable */
/* lblank */

lgradient(who,howmuch)
char who;
double howmuch;
{
  int temp,i,base;
  short obuff[7];
  char graddisableflag[2];
  grad_flag = TRUE;
  if ( P_getstring(GLOBAL,"gradientdisable",&graddisableflag,1,2) == 0 )
  {
    if ((graddisableflag[0] == 'y'))
    {
     howmuch = 0.0;
     if ((ix == 1) && (! warningDone)) warn_message("no gradients active for gradientdisable = 'y' \n");
     warningDone = 1;
    }
  }
  temp = (int) howmuch;
  temp = bound12(temp,who);
  base = get_l_pfg_base(who); 
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFG_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(base));
  	putcode((codeint)(temp));
  	curfifocount += 2;
  }
  else
  {
  	obuff[0] = 3;
  	obuff[1] = 0xA000 | base;
  	obuff[2] = 0xB000 | (base & 0x0f00) | (temp & 0x0ff);
  	obuff[3] = 0xA000 | ((base + 1) & 0x0fff);
  	obuff[4] = 0xB000 | (base & 0x0f00) | ((temp >> 8) & 0x0ff);
  	putcode(APBOUT);
  	for (i=0; i< 5; i++)
  	  putcode(obuff[i]);
  	curfifocount += 4;
  }
}

l_reset(who)
char who;
{
  short obuff[10];
  int i,base;
  base = get_l_pfg_base(who);
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(5);
  	putcode(PFG_APBUS_DELAY_TICKS);
  	putcode(2);
  	putcode((codeint)(base | 0x2));
  	putcode((codeint)(0x2));
  	putcode((codeint)(0x0));
  }
  else
  {
  	obuff[0] = 3;
  	obuff[1] = 0xA002 + (base & 0x0fff);
  	obuff[2] = 0xB002 | (base & 0x0f00);
  	obuff[3] = obuff[1];
  	obuff[4] = 0xB000 | (base & 0x0f00);
  	putcode(APBOUT);
  	for (i=0; i< 5; i++)
  	  putcode(obuff[i]);
  }
  curfifocount += 4;
}

l_cntrl(who,tog)
char who;
int tog;
{
  short obuff[10];
  int i,base;
  base = get_l_pfg_base(who);
  base &= 0x0fff;
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFG_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(base | 0x2));
  	putcode((codeint)(tog & 3));
  }
  else
  {
  	obuff[0] = 1;
  	obuff[1] = 0xA002 + base;
  	obuff[2] = 0xB000 | (base & 0x0f00) | (tog & 3);
  	putcode(APBOUT);
  	for (i=0; i< 3; i++)
  	  putcode(obuff[i]);
  }
  curfifocount += 2;
}

static int get_l_pfg_base(char where)
{
   int base;
   char tstr[32];
   switch (where)
   {
     case 'x': case 'X':   base = 0x0C60; break;
     case 'y': case 'Y':   base = 0x0C64; break;
     case 'z': case 'Z':   base = 0x0C68; break;
     case 'r': case 'R':   base = 0x0C6c; break;
     case 'n': case 'N':   base = 0; break;
     default: 
     {
	 sprintf(tstr,"Unrecognized PFG channel = %c\n",where);
	 text_error(tstr);
	 psg_abort(1);
     }
   }
   return(base);
}

/**************************L200 SUPPORT*************************/

static int lmask = 0;

l200_reset(j)
int j;
{
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFGL200_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(0x4000));
  	curfifocount += 2;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 2);	
	putcode((c68int) (0xAC8A));
	putcode((c68int) (0xBC00 ));
	putcode((c68int) (0x9C40 ));
	curfifocount += 3;
  }
  delay(0.005*j);
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFGL200_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(lmask));
  	curfifocount += 2;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 2);	
	putcode((c68int) (0xAC8A));
	putcode((c68int) (0xBC00 | (lmask & 0x0ff)));
	putcode((c68int) (0x9C00 | (lmask >> 8)));
	curfifocount += 3;
  }
}

int tflg1(which)
char which;
{
  int flag;
  flag = 0;
  switch (which)
  {
    case 'x': case 'X': flag = 0x100; break;
    case 'y': case 'Y': flag = 0x200; break;
    case 'z': case 'Z': flag = 0x400; break;
  }
  return(flag);
}

int tflg2(which)
char which;
{
  int flag;
  flag = 0;
  switch (which)
  {
    case 'x': case 'X': flag = 0x20; break;
    case 'y': case 'Y': flag = 0x48; break;
    case 'z': case 'Z': flag = 0x90; break;
  }
  return(flag);
}

static int get_t_pfg_base(char where)
{
   int base;
   char tstr[32];
   switch (where)
   {
     case 'x': case 'X':   base = 0x0C88; break;
     case 'y': case 'Y':   base = 0x0C88; break;
     case 'z': case 'Z':   base = 0x0C88; break;
     case 'r': case 'R':   base = 0x0C88; break;
     case 'n': case 'N':   base = 0; break;
     default: 
     {
	 sprintf(tstr,"Unrecognized PFG channel = %c\n",where);
	 text_error(tstr);
	 psg_abort(1);
     }
   }
   return(base);
}

t_enbl(which)
char which;
{
  int flag;
  flag = tflg1(which);
  lmask |= flag;
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFGL200_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(lmask));
  	curfifocount += 2;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 2);	
	putcode((c68int) 0xac8a);
	putcode((c68int) (0xBC00 | (lmask & 0x0ff)));
	putcode((c68int) (0x9C00 | (lmask >> 8)));
	curfifocount += 3;
  }
}

t_dsbl(which)
char which;
{
  int flag;
  flag = tflg1(which);
  lmask &= (~flag);
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFGL200_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(lmask));
  	curfifocount += 2;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 2);	
	putcode((c68int) 0xac8a);
	putcode((c68int) (0xBC00 | (lmask & 0x0ff)));
	putcode((c68int) (0x9C00 | (lmask >> 8)));
	curfifocount += 3;
  }
}

tgradient(which,value)
char which;
double value;
{
  int flag,tmp;
  char graddisableflag[2];
  char tag;
  tag = tolower(gradtype[2]);
  if ( P_getstring(GLOBAL,"gradientdisable",&graddisableflag,1,2) == 0 )
  {
    if ((graddisableflag[0] == 'y'))
    {
     value = 0.0;
     if ((ix == 1) && (! warningDone)) warn_message("no gradients active for gradientdisable = 'y' \n");
     warningDone = 1;
    }
  }
  tmp = (int) value;
  if ((tag == 't') || (tag == 'u'))
  {
  tmp /= 2;
  if (tmp > 16384)
     tmp = 16384;
  else if (tmp < -16384)
     tmp = -16384;
  }
  else if ((tag == 'c') || (tag == 'd'))
  {
    if (tmp > 32767)
     tmp = 32767;
    else if (tmp < -32767)
     tmp = -32767;
  }

  /* tflg2 - select and execute */
  flag = tflg2(which) | lmask;
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFG_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(flag));
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFGL200_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc88));
  	putcode((codeint)(tmp));
  	curfifocount += 4;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 5);	
	putcode((c68int) (0xAC8A));
	putcode((c68int) (0xBC00 | (flag & 0x0ff)));
	putcode((c68int) (0x9C00 | (flag >> 8)));
	putcode((c68int) (0xAC88));
	putcode((c68int) (0xBC00 | (tmp & 0x0ff)));
	putcode((c68int) (0x9C00 | ((0xff00 & tmp) >> 8)));
	curfifocount += 6;
  }
  grad_flag = TRUE;

}

t_grad_selectone(which)
char which;
{
  int flag;
  /* tflg2 - select and execute */
  flag = tflg2(which) | lmask;
  if (newacq)
  {
  	putcode(TAPBOUT);	/* placeholder for new apbcout acode */
  	putcode(4);
  	putcode(PFG_APBUS_DELAY_TICKS);
  	putcode(1);
  	putcode((codeint)(0xc8a));
  	putcode((codeint)(flag));
  	curfifocount += 2;
  }
  else
  {
	putcode((c68int) APBOUT);
	putcode((c68int) 2);	
	putcode((c68int) (0xAC8A));
	putcode((c68int) (0xBC00 | (flag & 0x0ff)));
	putcode((c68int) (0x9C00 | (flag >> 8)));
	curfifocount += 3;
  }
}
