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
/*  dfid-	display a 1D FID			*/
/*							*/
/*	note:  function names which begin with "b_" are	*/
/*	       called when buttons (function keys) are	*/
/*	       used.  function names which begin with	*/
/*	       "m_" are called when the mouse is used	*/
/********************************************************/

#include "vnmrsys.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"

extern float *get_one_fid();
extern int debug1;
extern int Bnmr;  /* background flag */
int df_repeatscan = 0;

#define CALIB 		40000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define CURSOR_MODE	1
#define BOX_MODE	5

#define MAXMIN(val,max,min) \
  if (val > max) val = max; \
  else if (val < min) val = min

static int     oldx_cursor[2],oldy_cursor[2],idelta;
static int     ds_mode;
static int     spec1,spec2,next,erase1,erase2;
static double  phfid;
static float  *phase_data;
static float  *spectrum;
static float   dispcalib;
static float   scale;
static int     imagflag;
static int     threshflag;
static int     phaseflag;
static int     spwpflag;
static int     zero;
static int     dotflag;

/************/
struct complex
/************/
{ float re,im;
};

/******************/
static char *dfstatus(char *stat)
/******************/
{
    static char status[16] = "";

    if (stat) {
	strncpy(status, stat, sizeof(status));
	status[sizeof(status)-1] = '\0';
    }
    return status;
}

/*************/
getdfstat(argc,argv,retc,retv)
/*************/
int argc,retc;
char *argv[],*retv[];
{
    if (retc > 0) {
	retv[0] = newString(dfstatus(0));
    }
    RETURN;
}

/******************/
static ds_reset()
/******************/
{ Wgmode();
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[0],0);
  x_cursor(&oldx_cursor[1],0);
  y_cursor(&oldy_cursor[0],0);
  normalmode();
  spwpflag = FALSE;
  disp_status("      ");
}

/*************************************/
static update_xcursor(cursor_number,x)
/*************************************/
int cursor_number,x;
{ Wgmode();
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[cursor_number],x);
  normalmode();
}

/*************************************/
static update_ycursor(cursor_number,y)
/*************************************/
int cursor_number,y;
{ Wgmode();
  xormode();
  color(CURSOR_COLOR);
  y_cursor(&oldy_cursor[cursor_number],y);
  normalmode();
}

/*************************************/
static dfid_m_newcursor(butnum,x,y,moveflag)
/*************************************/
int butnum,x,y,moveflag;
 
{ int ox,oy,ox1,oy1;
  int turnoff_dfid();
 
  if ((butnum == 3) && (ds_mode != BOX_MODE))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dfid);
    ds_mode = BOX_MODE;
    b_cursor();
    m_newcursor(butnum,x,y,moveflag);
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    execString("menu\n");
    }
  else  {
    m_newcursor(butnum,x,y,moveflag);
    }
}

/*************************************/
static m_newcursor(butnum,x,y,moveflag)
/*************************************/
int butnum,x,y,moveflag;
{ int r;

  Wgmode();
  if ((butnum==3) && (ds_mode!=BOX_MODE))	/*  box button */
  {
    if (ds_mode!=CURSOR_MODE)
    {
      ds_reset();
      ds_mode = -1;
      b_cursor();
    }
    ds_mode = BOX_MODE;
    InitVal(FIELD6,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME,-PARAM_COLOR,SCALED,3);
    disp_status("BOX   ");
  }

  if (butnum==2)
    ds_multiply(x,y);
  else
  {
    set_cursors((ds_mode == BOX_MODE),(butnum == 1),&x,
                 oldx_cursor[0],oldx_cursor[1],&idelta,dfpnt,dnpnt);
    if (ds_mode==BOX_MODE)
    {
      delta  = idelta  * wp  / dnpnt ;
      UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
      if (butnum==1)	/* cursor button */
        update_xcursor(1,x+idelta);
      else
      {
        update_xcursor(1,oldx_cursor[0]+idelta);
        UpdateVal(HORIZ,DELTA_NAME,delta,SHOW);
      }
    }
    if (butnum==1)	/* cursor button */
    {
      update_xcursor(0,x);
      cr = sp + (x - dfpnt ) * wp  / dnpnt;
      UpdateVal(HORIZ,CR_NAME,cr,SHOW);
    }
  }
  if (ds_mode == BOX_MODE)  {
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    }
  else  {
    if (P_setstring(GLOBAL,"crmode","c",0))
      Werrprintf("Unable to set variable \"crmode\".");
    }

  return 0;
}

/*******************/
static b_cursor()
/*******************/
{ int x,x1,save;

  ds_mode = (ds_mode == CURSOR_MODE) ? BOX_MODE : CURSOR_MODE;
  Wturnoff_mouse();
  x  = dfpnt  + dnpnt  * (cr - sp) / wp;
  x1 = x + dnpnt  * delta  / wp;
  idelta = save  = x1 - x;
  m_newcursor(1,x,0,0);
  if (ds_mode==BOX_MODE)
  {
    idelta  = save;
    m_newcursor(3,x1,0,0);
    disp_status("BOX   ");
  }
  else
  { update_xcursor(1,0);  /* erase the second cursor */
    disp_status("CURSOR");
  }
  cr_line();
  activate_mouse(dfid_m_newcursor,ds_reset);
}

/**************/
static cr_line()
/**************/
{
  DispField1(FIELD1, PARAM_COLOR,(imagflag) ? "vpfi" : "vpf ");
  DispField2(FIELD1,-PARAM_COLOR,(imagflag) ? vpi : vp, 1);
  InitVal(FIELD4,HORIZ,CR_NAME, PARAM_COLOR,NOUNIT,
                       CR_NAME,-PARAM_COLOR,SCALED,3);
  DispField1(FIELD5, PARAM_COLOR,"vf");
  DispField2(FIELD5,-PARAM_COLOR, vs, 1);
  if (ds_mode==BOX_MODE)
    InitVal(FIELD6,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         DELTA_NAME,-PARAM_COLOR,SCALED,3);
  else
    InitVal(FIELD6,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,3);
}

/**************/
static ph_line()
/**************/
{
  DispField1(FIELD1, PARAM_COLOR,"vpf ");
  DispField2(FIELD1,-PARAM_COLOR,vp, 1);
  DispField1(FIELD4, PARAM_COLOR,"phfid");
  DispField2(FIELD4,-PARAM_COLOR,phfid+c_block.head->rpval, 1);
  DispField1(FIELD5, PARAM_COLOR,"vf");
  DispField2(FIELD5,-PARAM_COLOR,vs, 1);
  InitVal(FIELD6,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                       BLANK_NAME, PARAM_COLOR,SCALED,3);
}

/*****************/
static newspec(inset,fp,np,dfp,dnp)
/*****************/
int inset;
int fp,np,dfp,dnp;
{ int res;
  struct datapointers datablock;

  if (calc_fid(specIndex-1)) return(ERROR);
  phase_data = spectrum;
  erase1 = 0;
  erase2 = 0;
  res = oldx_cursor[0];
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  Wclear_graphics();
  show_plotterbox();
  ResetLabels();
  displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,mnumypnts-3,1,FID_COLOR);
  update_xcursor(0,res);
  if (imagflag)
    imagdisp();
  if (inset)
  {
    fid_ybars(spectrum+(fp * 2),(double) (vs * scale),dfp,dnp,np,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
    erase1 = 0;
    displayspec(dfp,dnp,0,&spec1,&spec1,&erase1,0,0,FID_COLOR);
    erase1 = 0;
    displayspec(dfp,dnp,0,&next,&spec1,&erase1,mnumypnts-3,1,FID_COLOR);
    if (imagflag && !zero)
    {
      fid_ybars(spectrum+(fp*2)+1,(double) (vs * scale),dfp,dnp,np,
                 dfpnt2 + (int)(dispcalib * (vpi + wc2/2.0)),next,dotflag);
      erase2 = 0;
      displayspec(dfp,dnp,0,&spec2,&spec2,&erase2,0,0,IMAG_COLOR);
      erase2 = 0;
      displayspec(dfp,dnp,0,&next,&spec2,&erase2,mnumypnts-3,1,IMAG_COLOR);
    }
  }
  if (dscale_on())
    new_dscale(FALSE,TRUE);
  ph_line();
}

/*****************/
static exit_phase()
/*****************/
{ int res;

  if (phaseflag)
  {
    if (calc_fid(specIndex-1)) return(ERROR);
    Wclear_graphics();
    show_plotterbox();
    ResetLabels();
    oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
    erase1 = 0;
    erase2 = 0;
    fiddisp();
    if (imagflag)
      imagdisp();
    if (dscale_on())
      new_dscale(FALSE,TRUE);
    if (ds_mode < 0)
      b_cursor();
    phaseflag = FALSE;
  }
}

/*****************/
static phase_disp(xpos,fp,np,dfp,dnp)
/*****************/
int xpos;
int *fp,*np,*dfp,*dnp;
{
  *np = npnt * phaseflag / 100;
  if (*np<4) *np = 4;
  *fp = datapoint(sw - sp - ((xpos - dfpnt) * wp / dnpnt),sw,fn/2);
  *fp -= *np / 2;
  if (*fp<fpnt) *fp = fpnt;
  if (*fp + *np >= fpnt + npnt) *fp = fpnt + npnt - *np;
  *dnp = dnpnt * phaseflag / 100;
  if (*dnp > dnpnt) *dnp = dnpnt;
  *dfp = dfpnt + (int)((double)dnpnt * (double)(*fp - fpnt) / (double)npnt);
  if (*dfp < dfpnt) *dfp = dfpnt;
  if (*dfp + *dnp >= dfpnt + dnpnt) *dfp = dfpnt + dnpnt - *dnp;
  if (debug1)
  {
    Wscrprintf("\n x position=%d fp=%d np=%d fpnt=%d npnt=%d\n",
                   xpos,*fp,*np,fpnt,npnt);
    Wscrprintf("dfp=%d dnp=%d dfpnt=%d dnpnt=%d\n",
                   *dfp,*dnp,dfpnt,dnpnt);
  }
}

/*****************/
static phasit(fp,np,dfp,dnp,delt)
/*****************/
double delt;
int fp,np,dfp,dnp;
{
  double       factor;
  float       *phase_spec;
  struct complex  buff;
  int turnoff_dfid();

  if ((phase_spec = (float *) allocateWithId(sizeof(float) * (np*2 +2),"dfid"))==0)
  {
    Werrprintf("cannot allocate phasing buffer");
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dfid);
    return(ERROR);
  }
  if (debug1)
    Wscrprintf("\n phfid=%g delt=%g \n",phfid,delt);
  phasefunc(&buff,1,0.0,delt);
    /* complex phase rotation */
  vvvcmult(phase_data + (fp*2),1,&buff,0,phase_spec,1,np);
  fid_ybars(phase_spec,vs * scale,dfp,dnp,np,
         dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  displayspec(dfp,dnp,0,&next,&spec1,&erase1,mnumypnts-3,1,FID_COLOR);
  if (imagflag && !zero)
  {
    fid_ybars(phase_spec+1,vs * scale,dfp,dnp,np,
           dfpnt2 + (int)(dispcalib * (vpi + wc2/2.0)),next,dotflag);
    displayspec(dfp,dnp,0,&next,&spec2,&erase2,mnumypnts-3,1,IMAG_COLOR);
  }
  release(phase_spec);
}

/************************************/
static m_newphase(butnum,x,y,moveflag)
/************************************/
int butnum,x,y,moveflag;
{ double lp_change;
  int    dum;
  static int    fp,np,dfp,dnp;
  static int    last_ph;
  double delt,fine;
  extern float vs_mult();

  if (butnum == 4)
  {
    dnp = dfp = 0;
    update_xcursor(0,oldx_cursor[0]);
    update_xcursor(1,oldx_cursor[1]);
    ph_line();
    return(COMPLETE);
  }
  if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return 0;
  Wgmode();
  if (butnum == 2)
  {
    disp_status("PHASE");
    if (x<dfpnt + (int)(0.05 * (double)dnpnt))
    {
      vp = (double) (y - dfpnt2) / dispcalib;
      P_setreal(CURRENT,"vpf",vp,0);
    }
    else
    {
      vs *= vs_mult(x,y,mnumypnts,dfpnt2 +
                         (int)(dispcalib * (vp + wc2/2.0)),dfpnt,dnpnt,spec1);
      MAXMIN(vs,1.0e9,0.01);
      P_setreal(CURRENT,"vf",vs,0);
    }
    if (dnp)
    {
      y = oldy_cursor[0];
      newspec(TRUE,fp,np,dfp,dnp);
      update_ycursor(0,y);
    }
    else
      newspec(FALSE,fp,np,dfp,dnp);
  }
  else
  {
    if (!moveflag)
    {
      if ((x < dfp) || (x > dfp + dnp))
      {
        phase_disp(x,&fp,&np,&dfp,&dnp);
        set_cursors(0,1,&x,
                 oldx_cursor[0],oldx_cursor[1],&dum,dfpnt,dnpnt);
        update_xcursor(0,x);
        set_cursors(0,1,&y,
                 oldy_cursor[0],oldy_cursor[1],&dum,dfpnt2,mnumypnts - dfpnt2);
        newspec(TRUE,fp,np,dfp,dnp);
        update_ycursor(0,y);
        last_ph = y;
      }
    }
    if (y != last_ph)
    {
      fine = (butnum == 1) ? 1.0 : 8.0;
      phfid = (180.0 / mnumypnts) * (y - last_ph) / fine;
      phasit(fp,np,dfp,dnp,phfid);
      P_setreal(CURRENT,"phfid",phfid+c_block.head->rpval,0);
      DispField2(FIELD4,-PARAM_COLOR,phfid+c_block.head->rpval, 1);
    }
  }
}

/*****************/
static b_phase()
/*****************/
{ int x;
  double value;

  phaseflag = TRUE;
  Wturnoff_mouse();
  Wgmode();
  ds_reset();
  if (debug1)
    Wscrprintf("\n init phasing of spectrum %d\n",c_buffer);
  if (debug1)
    Wscrprintf("init phasing ok\n");
  if (!(c_block.head->status & S_COMPLEX))
  { Werrprintf("cannot phase this data"); return 0; }
  phfid = 0.0;
  P_setactive(CURRENT,"phfid",ACT_ON);
  disp_status("PHASE");
  ds_mode = -1;
  if (P_getreal(GLOBAL,"phasing",&value,1))
    phaseflag = 20;
  else
  {
    phaseflag = (int) value;
    if (phaseflag < 10)
      phaseflag = 10;
    else if (phaseflag > 100)
      phaseflag = 100;
  }
  m_newphase(4,dfpnt + dnpnt / 2,dfpnt2 + mnumypnts / 2,0);
  activate_mouse(m_newphase,exit_phase);
}

/************************************/
static m_thcursor(butnum,x,y,moveflag)
/************************************/
int butnum,x,y,moveflag;
{ int dum;
  if (butnum==2)
    ds_multiply(x,y);
  else if (butnum==1)
  {
    set_cursors(0,(butnum == 1),&y,
                 oldy_cursor[0],oldy_cursor[1],&dum,dfpnt2,mnumypnts - dfpnt2);
    update_ycursor(0,y);
    th = (y - dfpnt2) / dispcalib - (vp + wc2/2.0);
    P_setreal(CURRENT,"th",th,0);
    DispField2(FIELD4,-PARAM_COLOR,th, 1);
  }
}

/************************************/
static m_spwp(butnum,x,y,moveflag)
/************************************/
int butnum,x,y,moveflag;
{
  double freq,hzpp,spsav,wpsav;
  static int spon,wpon;
  static double frqset;
  int    dum;

  if (butnum==4)
    spon = wpon = FALSE;
  else if (butnum==2)
    ds_multiply(x,y);
  else
  {
    spsav = sp;
    wpsav = wp;
    if (butnum==3)
    {
      if (!wpon)
      {
        set_cursors(0,1,&x,
                 oldx_cursor[0],oldx_cursor[1],&dum,dfpnt,dnpnt);
        frqset = (double) (x - dfpnt) / (double) dnpnt;
        update_xcursor(0,x);
        set_cursors(0,1,&y,
                 oldy_cursor[0],oldy_cursor[1],&dum,dfpnt2,mnumypnts - dfpnt2);
        update_ycursor(0,y);
        wpon = y;
      }
      else
      {
        double wpmax;

        wpmax = sw - sw/(double) (fn/2);
        wp += (wp / mnumypnts) * (y - wpon);
        if (wp > wpmax)
        {
          wp = wpmax;
          sp = 0.0;
        }
        else
        {
          if (wp < 20.0 * sw / fn)
            wp = 20.0 * sw / fn;
          sp -= (wp - wpsav) * frqset;
        }
      }
      UpdateVal(HORIZ,WP_NAME,wp,SHOW);
      spon = FALSE;
    }
    else
    {
      if (spon)
      {
        freq = sp + (double) (x - dfpnt) * wp / (double) dnpnt;
        sp -= freq - frqset;
      }
      else
        update_ycursor(0,0);
      set_cursors(0,1,&x,
                 oldx_cursor[0],oldx_cursor[1],&dum,dfpnt,dnpnt);
      frqset = sp + (double) (x - dfpnt) * wp / (double) dnpnt;
      if (!spon)
        update_xcursor(0,x);
      spon = TRUE;
      wpon = FALSE;
    }
    if (debug1)
      Wscrprintf("before sp_wp sp= %g, wp= %g\n",sp,wp);
    hzpp = sw/(double) (fn/2);
    if (wp != wpsav)
    {
       if (sp < 0)
         sp = 0.0;
       if (sp + wp > sw - hzpp)
       {
         wp = sw - hzpp;
         sp = 0.0;
       }
    }
    if ((wpon) && (wp == wpsav))
      sp = spsav;
    checkreal(&wp, 4.0*hzpp, sw-hzpp);
    wp = (double) ((int) (wp/hzpp + 0.01)) * hzpp;
    checkreal(&sp, rflrfp, sw-wp-hzpp);
    sp = (double) ((int) (sp/hzpp + 0.01)) * hzpp;
    if (debug1)
      Wscrprintf("after sp_wp sp= %g, wp= %g\n",sp,wp);
    if ((sp != spsav) || (wp != wpsav))
    {
      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
      if (spon)
        update_xcursor(0,dfpnt + (int)((double)dnpnt*(frqset-sp)/wp));
      exp_factors(FALSE);
      fiddisp();
      if (imagflag)
        imagdisp();
      if (dscale_on())
        new_dscale(TRUE,TRUE);
      UpdateVal(HORIZ,SP_NAME,sp,SHOW);
    }
  }
}

/*****************/
static b_spwp()
/*****************/
{
  Wgmode();
  spwpflag = (!spwpflag);
  if (spwpflag)
  {
    if (phaseflag)
      Wturnoff_mouse();
    Wturnoff_mouse();
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
    activate_mouse(m_spwp,ds_reset);
    m_spwp(4);
    disp_status("sf wf ");
    InitVal(FIELD4,HORIZ,SP_NAME, PARAM_COLOR,NOUNIT,
                         SP_NAME,-PARAM_COLOR,SCALED,3);
    InitVal(FIELD6,HORIZ,WP_NAME, PARAM_COLOR,NOUNIT,
                         WP_NAME,-PARAM_COLOR,SCALED,3);
  }
  else
    b_cursor();
}

/*****************/
static b_dscale()
/*****************/
{
  if (dscale_on())
  {
    new_dscale(TRUE,FALSE);
    dscale_off();
  }
  else
    new_dscale(FALSE,TRUE);
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

/*****************/
static b_thresh()
/*****************/
{ static int oldmode;
  int y;
  Wgmode();
  if (!threshflag)
  {
    Wturnoff_mouse();
    oldmode = ds_mode;
    activate_mouse(m_thcursor,ds_reset);
    y = dfpnt2 + (int) (dispcalib * (th + (vp + wc2/2.0)));
    m_thcursor(1,0,y,0);
    disp_status("thresh");
    DispField1(FIELD4, PARAM_COLOR,"th");
    DispField2(FIELD4,-PARAM_COLOR,th, 1);
    InitVal(FIELD6,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,3);
  }
  else
  {
    ds_mode = (oldmode == CURSOR_MODE) ? -1 : CURSOR_MODE;
    b_cursor();
  }
  threshflag = (!threshflag);
}

/*****************/
static b_imaginary(int state)
/*****************/
{ int res;

  if (phaseflag)
    Wturnoff_mouse();
  Wgmode();
  if (imagflag && zero)
    imagflag = zero = FALSE;
  else if (imagflag)
    zero = TRUE;
  else
    imagflag = TRUE;
  if (imagflag)
  {
    imagdisp();
    DispField1(FIELD5, PARAM_COLOR,"vf");
    DispField2(FIELD5,-PARAM_COLOR,vs, 1);
    DispField1(FIELD1, PARAM_COLOR,"vpfi");
    DispField2(FIELD1,-PARAM_COLOR,vpi, 1);
  }
  else if ((state) || ((!state) && (erase2 != 0)))
  {
    erase2 = 0;
    displayspec(dfpnt,dnpnt,0,&spec2,&spec2,&erase2,0,0,IMAG_COLOR);
    erase2 = 0;
    DispField1(FIELD5, PARAM_COLOR,"vf");
    DispField2(FIELD5,-PARAM_COLOR,vs, 1);
    DispField1(FIELD1, PARAM_COLOR,"vpf ");
    DispField2(FIELD1,-PARAM_COLOR,vp, 1);
  }
  if (!imagflag)  {
    if (P_setstring(GLOBAL,"dfmode","r",0))
      Werrprintf("Unable to set variable \"dfmode\".");
    if (P_setstring(CURRENT,"displaymode","r",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  else if (!zero)  {
    if (P_setstring(GLOBAL,"dfmode","i",0))
      Werrprintf("Unable to set variable \"dfmode\".");
    if (P_setstring(CURRENT,"displaymode","ri",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  else  {
    if (P_setstring(GLOBAL,"dfmode","z",0))
      Werrprintf("Unable to set variable \"dfmode\".");
    if (P_setstring(CURRENT,"displaymode","z",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

/****************/
static b_expand()
/****************/
{ double spsav,wpsav;

  ds_reset();
  spsav = sp;
  wpsav = wp;
  if (ds_mode!=BOX_MODE)
  {
    disp_status("FULL  ");
    /* set sp,wp for full display */
    sp  = 0.0;
    wp  = sw - sw/(double) (fn/2);
    ds_mode=CURSOR_MODE; /* force b_cursor() into the BOX_MODE */
  }
  else
  {
    disp_status("EXPAND");
    /* set sp,wp according to expansion box */
    sp  = cr;
    wp  = delta;
    set_sp_wp(&sp,&wp,sw,fn/2,rflrfp);
    delta = wp;
    cr = sp;
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
  }
  Wgmode();
  Wturnoff_mouse();
  /* store the parameters */
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  exp_factors(FALSE);
  fiddisp();
  if (imagflag)
    imagdisp();
  if (dscale_on())
    new_dscale(TRUE,TRUE);
  b_cursor();
  return 0;
}

extern int menuflag;
/****************/
static b_return()
/****************/
{
  Wturnoff_buttons();
  Wsetgraphicsdisplay("df");
  EraseLabels();
  if (menuflag)
    execString("menu\n");
}

/************************/
static ds_multiply(x,y)
/************************/
{ extern float vs_mult();

  Wgmode();
  if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return(COMPLETE);
  if (x<dfpnt + (int)(0.05 * (double)dnpnt))
  {
    if (imagflag)
    {
      vpi = (double) (y - dfpnt2) / dispcalib - wc2/2.0;
      P_setreal(CURRENT,"vpfi",vpi,0);
      DispField2(FIELD1,-PARAM_COLOR,vpi, 1);
      imagdisp();
    }
    else
    {
      vp = (double) (y - dfpnt2) / dispcalib - wc2/2.0;
      P_setreal(CURRENT,"vpf",vp,0);
      DispField2(FIELD1,-PARAM_COLOR,vp, 1);
      fiddisp();
      if (dscale_on())
        new_dscale(TRUE,TRUE);
    }
  }
  else
  {
    vs *= vs_mult(x,y,mnumypnts,dfpnt2 +
                       (int)(dispcalib * (vp + wc2/2.0)),dfpnt,dnpnt,spec1);
    MAXMIN(vs,1.0e9,0.01);
    P_setreal(CURRENT,"vf",vs,0);
    DispField2(FIELD5,-PARAM_COLOR, vs, 1);
    fiddisp();
    if (imagflag)
      imagdisp();
  }
  return(COMPLETE);
}

/*****************/
static turnoff_dfid()
/*****************/
{ int r;

  Wgmode();
  Wturnoff_mouse();
  phaseflag = FALSE;
  ds_reset();
  endgraphics();
/*  exit_display();*/
  if (freebuffers()) return(ERROR);
  return(COMPLETE);
}

/****************/
static imagdisp()
/****************/
{ float scl;

  scl = (zero) ? 0.0 : scale;
  fid_ybars(spectrum+(fpnt * 2)+1,(double) (vs * scl),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vpi + wc2/2.0)),next,dotflag);
  displayspec(dfpnt,dnpnt,0,&next,&spec2,&erase2,mnumypnts-3,1,IMAG_COLOR);
  return(COMPLETE);
}

/****************/
static freebuffers()
/****************/
{ int res;

  if(c_buffer>=0) /* release last used block */
    if (res=D_release(D_PHASFILE,c_buffer))
    {
      D_error(res); 
      D_close(D_PHASFILE);  
      return(ERROR);
    }
  return(COMPLETE);
}

/*************/
static setwindows()
/*************/
{
  Wclear_graphics();
#ifdef SUN
  /* make_table();             /* reset the color table */
#endif
  change_color(1,1);        /* reset the color table */
  refresh_graf();           /* some window systems needs refresh */
  Wgmode(); /* goto tek graphics and set screen 2 active */
  show_plotterbox();
  Wshow_graphics();
}

/****************************/
static ds_checkinput(argc,argv,trace)
/****************************/
int argc,*trace;
char *argv[];
{ int arg_no;
  int res;

  arg_no = 1;

  if (argc>arg_no)
  {
    if (isReal(argv[arg_no]))
    {
      *trace = (int) stringReal(argv[arg_no]);
      arg_no++;
    }
  }

  if ((argc>arg_no) && (!WgraphicsdisplayValid("df")))
  {
    Werrprintf("usage - %s or %s(fid#)",argv[0],argv[0]);
    return(ERROR);
  }
  if (debug1) Wscrprintf("current trace= %d\n",*trace);
  return(COMPLETE);
}

/*************/
static void init_vars1()
/*************/
{
  disp_specIndex(specIndex);
  ds_mode = -1;
  if (P_setstring(GLOBAL,"crmode","c",0))
    Werrprintf("Unable to set variable \"crmode\".");
  threshflag  = FALSE;
  spwpflag = FALSE;
  zero = FALSE;
  imagflag = FALSE;
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  dscale_off();
}

/*************/
static int init_vars2()
/*************/
{
  char flag[16];

  spec1  = 0;
  spec2  = 1;
  next   = 2;
  erase1 = 0;
  erase2 = 0;
  scale = 0.0;
  phaseflag = FALSE;
  init_display();
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  setwindows();
  ResetLabels();
  dotflag = TRUE;
  if (P_getstring(CURRENT,"dotflag" ,flag, 1,16) == 0) 
      dotflag = (flag[0] != 'n');
  if (calc_fid(specIndex-1)) return(ERROR);
  return(COMPLETE);
}

/*************/
static init_ds(argc,argv,retc,retv)
/*************/
int argc,retc;
char *argv[],*retv[];
{

  if (ds_checkinput(argc,argv,&specIndex)) return(ERROR);
  disp_status("IN      ");
  revflag = 0;
  if(initfid(1)) return(ERROR);
  disp_status("        ");
  if ((specIndex < 1) || (specIndex > nblocks * specperblock))
  { if (!WgraphicsdisplayValid("df") && (argc>1) && (isReal(argv[1])))
      Werrprintf("spectrum %d does not exist",specIndex);
    specIndex = 1;
  }
  init_vars1();
  return(COMPLETE);
}

/*************/
dfid(argc,argv,retc,retv)
/*************/
int argc,retc;
char *argv[],*retv[];
{
  int redisplay_param_flag, do_menu=FALSE;

  if (Bnmr)
    return(COMPLETE);

  Wturnoff_buttons();
  set_turnoff_routine(turnoff_dfid);

  if (argc == 2)  {
    if (strcmp(argv[1],"toggle") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_cursor();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"imaginary") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_imaginary(TRUE);
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"imagmode") == 0)  {
        execString("dfid('imagmode','show')\n");
      }
    else if (strcmp(argv[1],"restart") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
 	if (ds_mode == CURSOR_MODE)
	  ds_mode = BOX_MODE;
 	else if (ds_mode == BOX_MODE)
	  ds_mode = CURSOR_MODE;
        b_cursor();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"expand") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_expand();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"dscale") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_dscale();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"phase") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_phase();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else if (strcmp(argv[1],"sfwf") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        b_spwp();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    else  {
      if (!isReal(argv[1]))  {
        Werrprintf("Illegal dfid option : %s",argv[1]);
        ABORT;
        }
      }
    }
  else if (argc == 3)  {
    if (strcmp(argv[1],"imagmode") == 0)  {
      if (WgraphicsdisplayValid("df") && (argc>1))  {
        if (argc>2)  {
            if (strcmp(argv[2],"show") == 0) {
                imagflag = FALSE; zero = FALSE;
              }
            else if (strcmp(argv[2],"zero") == 0) {
                imagflag = TRUE;  zero = FALSE;
              }
            else {
                imagflag = TRUE;  zero = TRUE;
              }
          }
        else {
                imagflag = FALSE; zero = FALSE;
          }
        b_imaginary(FALSE);
        set_turnoff_routine(turnoff_dfid);
        RETURN;
        }
      else  {
        Werrprintf("Must be in df to use %s option",argv[1]);
        ABORT;
        }
      }
    }

/*  Redisplay Parameters?  */

  redisplay_param_flag = 0;
  if (argc > 1)
    if (strcmp( argv[ argc-1 ], "redisplay parameters" ) == 0) {
            argc--;

/*  autoRedisplay (in lexjunk.c) is supposed
    to insure the next test returns TRUE.	*/

	    if (WgraphicsdisplayValid( argv[ 0 ] ))
              redisplay_param_flag = 1;
    }

  if (!WgraphicsdisplayValid("df") ||
      ((argc>1) && (isReal(argv[1]))) ||
      (c_buffer < 0))
  {
    if (init_ds(argc,argv,retc,retv))
      return(ERROR);
    if (!imagflag)  {
      if (P_setstring(GLOBAL,"dfmode","r",0))
        Werrprintf("Unable to set variable \"dfmode\".");
      }
    else if (!zero)  {
      if (P_setstring(GLOBAL,"dfmode","i",0))
        Werrprintf("Unable to set variable \"dfmode\".");
      }
    else  {
      if (P_setstring(GLOBAL,"dfmode","z",0))
        Werrprintf("Unable to set variable \"dfmode\".");
      }
    do_menu = TRUE;
  }
  else
  {
/*	Old test:							*/
/*  if (WgraphicsdisplayValid("df") && (argc>1) && (!isReal(argv[1])))  */
/*	See ds.c for further explaination of this change.		*/

    if (redisplay_param_flag) 
    { int i,res;
      vInfo info;
      int update = FALSE;

      i = 1;
      while (!update && (i < argc))
      {
        if ((res = P_getVarInfo(CURRENT,argv[i],&info)) == 0)
          update = ((info.group == G_DISPLAY) || (strcmp(argv[i],"phfid") == 0)
                   || (strcmp(argv[i],"lsfid") == 0));
        i++;
      }
      if (!update)
        return(COMPLETE);
    }
    disp_status("IN      ");
    revflag = 0;
    if(initfid(1)) return(ERROR);
    disp_status("        ");
    ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
    if ((!WgraphicsdisplayValid("df")) || ((argc>1) && (!isReal(argv[1]))) ||
		 (!redisplay_param_flag))
    {
      if (P_setstring(GLOBAL,"crmode","c",0))
        Werrprintf("Unable to set variable \"crmode\".");
      if (!imagflag)  {
        if (P_setstring(GLOBAL,"dfmode","r",0))
          Werrprintf("Unable to set variable \"dfmode\".");
        }
      else if (!zero)  {
        if (P_setstring(GLOBAL,"dfmode","i",0))
          Werrprintf("Unable to set variable \"dfmode\".");
        }
      else  {
        if (P_setstring(GLOBAL,"dfmode","z",0))
          Werrprintf("Unable to set variable \"dfmode\".");
        }
      do_menu = TRUE;
    }
  }
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  if (init_vars2()) return(ERROR);
  fiddisp();
  releasevarlist();
  if (imagflag)
    imagdisp();
  if (dscale_on())
    new_dscale(FALSE,TRUE);
  b_cursor();
  Wsetgraphicsdisplay("df");
  if (do_menu)
    execString("menu('dfid')\n");
  set_turnoff_routine(turnoff_dfid);
  return(COMPLETE);
}

/****************/
static calc_fid(trace)
/****************/
int trace;
{ float datamax;

  if (debug1) Wscrprintf("function calc_fid\n");
  if ((spectrum = get_one_fid(trace,&fn,&c_block, FALSE)) == 0) return(ERROR);
  c_buffer = trace;
  c_first  = trace;
  c_last   = trace;
  if ((normflag) && ((scale == 0.0) || phaseflag))
  {
    maxfloat(spectrum,pointsperspec / 2, &datamax);
    scale = (float) dispcalib / datamax;
    if (debug1)
      Wscrprintf("datamax=%g ", datamax);
  }
  else
    scale = (float) dispcalib / CALIB;
  if (debug1)
    Wscrprintf("normflag= %d scale=%g\n", normflag, scale);
  return(COMPLETE);
}

/****************/
static fiddisp()
/****************/
{
  if (debug1)
  {
    Wscrprintf("starting fid display\n");
    Wscrprintf("fpnt=%d, npnt=%d, mnumxpnts=%d, dispcalib=%g\n",
            fpnt,npnt,mnumxpnts,dispcalib);
  }
  fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,mnumypnts-3,1,FID_COLOR);
  return(COMPLETE);
}

/*************/
int fidmax(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  int trace;
  float datamax;

  revflag = 0;
  if(initfid(1)) return(ERROR);
  trace = specIndex;
  if (argc>1)
  {
    if (isReal(argv[1]))
    {
      trace = (int) stringReal(argv[1]);
    }
  }
  if ((trace < 1) || (trace > nblocks * specperblock))
  {
    trace = 1;
  }
  if ((spectrum = get_one_fid(trace-1,&fn,&c_block, 0)) == 0)
  {
     datamax = 0.0;
  }
  else
  {
     maxfloat(spectrum,pointsperspec * 2, &datamax);
  }
  if ( !retc )
     Winfoprintf("FID %d maximum value is %g", trace, datamax);
  else
     retv[0] = realString( (double) datamax );
  RETURN;
}
