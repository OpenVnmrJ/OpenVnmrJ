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
#include "sky.h"
#include "tools.h"
#include "variables.h"
#include "pvars.h"
#include "allocate.h"
#include "buttons.h"
#include "displayops.h"
#include "dscale.h"
#include "init_display.h"
#include "wjunk.h"
#include "aipCInterface.h"

extern int is_aip_window_opened();
extern int get_drawVscale();
extern void x_crosshair(int pos, int off1, int off2, double val1, int yval, double val2);
extern void m_noCrosshair();
extern void getVLimit(int *vmin, int *vmax);
extern void drawPlotBox();
extern int getDscaleDecimal(int);

extern int debug1;
int df_repeatscan = 0;

#define CALIB 		40000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define CURSOR_MODE	1
#define BOX_MODE	5

#define DISPLAY_REAL 0x1
#define DISPLAY_IMAGINARY 0x2
#define DISPLAY_ABSVAL 0x4
#define DISPLAY_ENVELOPE 0x8
#define DISPLAY_PHASEANGLE 0x40
#define DISPLAY_ERASE_SCALE 0x10
#define DISPLAY_DRAW_SCALE 0x20
#define DISPLAY_SCALE (DISPLAY_ERASE_SCALE | DISPLAY_DRAW_SCALE)
#define DISPLAY_ALL ~0

#define MAXMIN(val,max,min) \
  if (val > max) val = max; \
  else if (val < min) val = min

#define max(x,y) ((x)>(y)?(x):(y))

#ifdef VNMRJ
extern int vj_x_cursor(int i, int *old_pos, int new_pos, int c);
extern int vj_y_cursor(int i, int *old_pos, int new_pos, int c);

extern int getButtonMode();

void df_nextZoomin(double *c2, double *c1, double *d2, double *d1);
void df_prevZoomin(double *c2, double *c1, double *d2, double *d1);
static int showCursor();
extern void vj_alpha_mode(int n);
extern void getCurrentZoomFromList(double *c2, double *d2, double *c1, double *d1);
extern void addNewZoom(char *cmd, double c2, double d2, double c1, double d1);
extern void getNextZoom(double *c2, double *d2, double *c1, double *d1);

#endif

extern int ddf(int argc, char *argv[], int retc, char *retv[]);
extern float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag);
extern void maxfloat(register float  *datapntr, register int npnt, register float  *max);
extern void set_turnoff_routine(int (*funct)());
extern void refresh_graf();
void envelope(short *buf, int n, int sgn);

static int     oldx_cursor[2],oldy_cursor[2],idelta;
static int     ds_mode;
static int     spec1, spec2, spec3, spec4, spec5, spec6, next;
static int     erase1, erase2, erase3, erase4, erase5, erase6;
static double  phfid;
static float  *phase_data;
static float  *spectrum;
static float   dispcalib;
static float   scale;
static int     realflag;
static int     imagflag;
static int     absflag;
static int     paflag;
static int     envelopeflag;
static int     threshflag;
static int     phaseflag;
static int     spwpflag;
static int     zeroflag = FALSE;
static int     dotflag;
static int     fillflag;
static int     dcflag;
static short  *fidenvelope;
static float  *phaseangle;

/************/
struct fcomplex
/************/
{ float re,im;
};

static void b_cursor();
static void cr_line();
static int m_newcursor(int butnum, int x, int y);
static int ds_multiply(int x, int y);
static int turnoff_dfid();
static int freebuffers();
static int calc_fid(int trace);
static int fiddisplay(int what);
static void clear_envelope();
static void clear_phaseangle();

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
int getdfstat(int argc, char *argv[], int retc, char *retv[])
/*************/
{
    (void) argc;
    (void) argv;
    if (retc > 0) {
	retv[0] = newString(dfstatus(0));
    }
    RETURN;
}

/******************/
static void ds_reset()
/******************/
{
#ifdef VNMRJ
  vj_x_cursor(0,&oldx_cursor[0],0, CURSOR_COLOR);
  vj_x_cursor(1,&oldx_cursor[1],0, CURSOR_COLOR);
  vj_y_cursor(0,&oldy_cursor[0],0, CURSOR_COLOR);
#else
  Wgmode();
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[0],0);
  x_cursor(&oldx_cursor[1],0);
  y_cursor(&oldy_cursor[0],0);
  normalmode();
#endif
  spwpflag = FALSE;
  disp_status("      ");
  dfstatus("");
}

/*************************************/
static void update_xcursor(int cursor_number, int x)
/*************************************/
{
#ifdef VNMRJ
  if(!showCursor()) return;
  vj_x_cursor(cursor_number,&oldx_cursor[cursor_number],x, CURSOR_COLOR);
#else
  Wgmode();
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[cursor_number],x);
  normalmode();
#endif
}

/*************************************/
static void update_ycursor(int cursor_number, int y)
/*************************************/
{ 
#ifdef VNMRJ
  if(!showCursor()) return;
  vj_y_cursor(cursor_number,&oldy_cursor[cursor_number],y, CURSOR_COLOR);
#else
  Wgmode();
  xormode();
  color(CURSOR_COLOR);
  y_cursor(&oldy_cursor[cursor_number],y);
  normalmode();
#endif
}

/*************************************/
static void dfid_m_newcursor(int butnum, int x, int y, int moveflag)
/*************************************/
{
  (void) moveflag;
  if ((butnum == 3) && (ds_mode != BOX_MODE))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dfid);
    ds_mode = BOX_MODE;
    b_cursor();
    m_newcursor(butnum,x,y);
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    execString("menu\n");
    }
  else  {
    m_newcursor(butnum,x,y);
    }
}

/*************************************/
static int m_newcursor(int butnum, int x, int y)
/*************************************/
{

  int dez = getDscaleDecimal(0)+2;
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
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME,-PARAM_COLOR,SCALED,dez);
    disp_status("BOX   ");
    dfstatus("BOX");
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
static void b_cursor()
/*******************/
{ int x,x1,save;

  ds_mode = (ds_mode == CURSOR_MODE) ? BOX_MODE : CURSOR_MODE;
  Wturnoff_mouse();
  x  = dfpnt  + dnpnt  * (cr - sp) / wp;
  x1 = x + dnpnt  * delta  / wp;
  idelta = save  = x1 - x;
  m_newcursor(1,x,0);
  if (ds_mode==BOX_MODE)
  {
    idelta  = save;
    m_newcursor(3,x1,0);
    disp_status("BOX   ");
    dfstatus("BOX");
  }
  else
  { update_xcursor(1,0);  /* erase the second cursor */
    disp_status("CURSOR");
    dfstatus("CURSOR");
  }
  cr_line();
  activate_mouse(dfid_m_newcursor,ds_reset);
}

/**************/
static char *vpStr()
/**************/
{
    double dummy;
    int isactive;

    if (imagflag
	&& P_getparinfo(CURRENT, "vpfi", &dummy, &isactive) == 0
	&& isactive)
    {
	return "vpfi";
    } else {
	return "vpf";
    }
}

/**************/
static double vpVal()
/**************/
{
    double dummy;
    int isactive;

    if (imagflag
	&& P_getparinfo(CURRENT, "vpfi", &dummy, &isactive) == 0
	&& isactive)
    {
	return vpi;
    } else {
	return vp;
    }
}

/**************/
static void setVpVal(double value)
/**************/
{
    double dummy;
    int isactive;

    if (imagflag
	&& P_getparinfo(CURRENT, "vpfi", &dummy, &isactive) == 0
	&& isactive)
    {
	P_setreal(CURRENT, "vpfi", value, 0);
	vpi = value;
    } else {
	P_setreal(CURRENT, "vpf", value, 0);
	vp = value;
    }
}

/**************/
static void cr_line()
/**************/
{
  int dez = getDscaleDecimal(0)+2;
  DispField1(FIELD1, PARAM_COLOR, vpStr());
  DispField2(FIELD1,-PARAM_COLOR, vpVal(), 1);
  InitVal(FIELD3,HORIZ,CR_NAME, PARAM_COLOR,NOUNIT,
                       CR_NAME,-PARAM_COLOR,SCALED,dez);
  DispField1(FIELD2, PARAM_COLOR,"vf");
  DispField2(FIELD2,-PARAM_COLOR, vs, 1);
  if (ds_mode==BOX_MODE)
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         DELTA_NAME,-PARAM_COLOR,SCALED,dez);
  else
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,dez);
}

/**************/
static void ph_line()
/**************/
{
  DispField1(FIELD1, PARAM_COLOR,"vpf ");
  DispField2(FIELD1,-PARAM_COLOR,vp, 1);
  DispField1(FIELD3, PARAM_COLOR,"phfid");
  DispField2(FIELD3,-PARAM_COLOR,phfid+c_block.head->rpval, 1);
  DispField1(FIELD2, PARAM_COLOR,"vf");
  DispField2(FIELD2,-PARAM_COLOR,vs, 1);
  InitVal(FIELD4,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                       BLANK_NAME, PARAM_COLOR,SCALED,3);
}

/*****************/
static int newspec(int inset, int fp, int np, int dfp, int dnp)
/*****************/
{ int res;
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);

  if (calc_fid(specIndex-1)) return(ERROR);
  phase_data = spectrum;
  erase1 = 0;
  erase2 = 0;
  erase3 = 0;
  erase4 = 0;
  erase5 = 0;
  erase6 = 0;
  res = oldx_cursor[0];
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  Wclear_graphics();
  show_plotterbox();
  drawPlotBox();
  ResetLabels();
  displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,vmax,vmin,FID_COLOR);
  update_xcursor(0,res);
  fiddisplay(DISPLAY_IMAGINARY | DISPLAY_ABSVAL | DISPLAY_ENVELOPE | DISPLAY_PHASEANGLE);
  if (inset)
  {
    fid_ybars(spectrum+(fp * 2),(double) (vs * scale),dfp,dnp,np,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
    erase1 = 0;
    displayspec(dfp,dnp,0,&spec1,&spec1,&erase1,0,0,FID_COLOR);
    erase1 = 0;
    displayspec(dfp,dnp,0,&next,&spec1,&erase1,vmax,vmin,FID_COLOR);
    if (imagflag && !zeroflag)
    {
      fid_ybars(spectrum+(fp*2)+1,(double) (vs * scale),dfp,dnp,np,
                 dfpnt2 + (int)(dispcalib * (vpVal() + wc2/2.0)),next,dotflag);
      erase2 = 0;
      displayspec(dfp,dnp,0,&spec2,&spec2,&erase2,0,0,IMAG_COLOR);
      erase2 = 0;
      displayspec(dfp,dnp,0,&next,&spec2,&erase2,vmax,vmin,IMAG_COLOR);
    }
  }
  if (dscale_on())
    new_dscale(FALSE,TRUE);
  ph_line();
  return(COMPLETE);
}

/*****************/
static int exit_phase()
/*****************/
{

  if (phaseflag)
  {
    if (calc_fid(specIndex-1)) return(ERROR);
    Wclear_graphics();
    show_plotterbox();
    ResetLabels();
    oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
    erase1 = 0;
    erase2 = 0;
    erase3 = 0;
    erase4 = 0;
    erase5 = 0;
    erase6 = 0;
    fiddisplay(DISPLAY_ALL & ~DISPLAY_ERASE_SCALE);
    if (ds_mode < 0)
      b_cursor();
    phaseflag = FALSE;
  }
  return(COMPLETE);
}

/*****************/
static void phase_disp(int xpos, int *fp, int *np, int *dfp, int *dnp)
/*****************/
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
static int phasit(int fp, int np, int dfp, int dnp, double delt)
/*****************/
{
  float       *phase_spec;
  struct fcomplex  buff;
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);

  if ((phase_spec = (float *) allocateWithId(sizeof(float) * (np*2 +2),"dfid"))==0)
  {
    Werrprintf("cannot allocate phasing buffer");
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dfid);
    return(ERROR);
  }
  drawPlotBox();
  if (debug1)
    Wscrprintf("\n phfid=%g delt=%g \n",phfid,delt);
  phasefunc((float *) &buff,1,0.0,delt);
    /* complex phase rotation */
  vvvcmult(phase_data + (fp*2),1,&buff,0,phase_spec,1,np);
  fid_ybars(phase_spec,vs * scale,dfp,dnp,np,
         dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  displayspec(dfp,dnp,0,&next,&spec1,&erase1,vmax,vmin,FID_COLOR);
  if (imagflag && !zeroflag)
  {
    fid_ybars(phase_spec+1,vs * scale,dfp,dnp,np,
           dfpnt2 + (int)(dispcalib * (vpVal() + wc2/2.0)),next,dotflag);
    displayspec(dfp,dnp,0,&next,&spec2,&erase2,vmax,vmin,IMAG_COLOR);
  }
  release(phase_spec);
  return(COMPLETE);
}

/************************************/
static int m_newphase(int butnum, int x, int y, int moveflag)
/************************************/
{
  int    dum;
  static int    fp,np,dfp,dnp;
  static int    last_ph;
  double fine;

  if (butnum == 4)
  {
    dnp = dfp = 0;
    update_xcursor(0,oldx_cursor[0]);
    update_xcursor(1,oldx_cursor[1]);
    ph_line();
    return(COMPLETE);
  }

  Wgmode();
  if (butnum == 2)
  {
    if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return 0;
    disp_status("PHASE");
    dfstatus("PHASE");
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
      DispField2(FIELD3,-PARAM_COLOR,phfid+c_block.head->rpval, 1);
    }
  }
  return(COMPLETE);
}

/*****************/
static int b_phase()
/*****************/
{
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
  dfstatus("PHASE");
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
  return(COMPLETE);
}

#ifdef XXX
/************************************/
static void m_thcursor(int butnum, int x, int y, int moveflag)
/************************************/
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
    DispField2(FIELD3,-PARAM_COLOR,th, 1);
  }
}
#endif

/************************************/
static void m_spwp(int butnum, int x, int y, int moveflag)
/************************************/
{
	double freq, hzpp, spsav, wpsav,newvp;
	static int spon, wpon;
	static double frqset;
	int dum;
	static double vpsave=0;

	P_getreal(CURRENT,"vpf",&newvp,1);

	if (butnum == 4)
		spon = wpon = FALSE;
	else if (butnum == 2)
		ds_multiply(x, y);
	else {
		spsav = sp;
		wpsav = wp;
		if (butnum == 3) {
			if (!wpon) {
				set_cursors(0, 1, &x, oldx_cursor[0], oldx_cursor[1], &dum,
						dfpnt, dnpnt);
				frqset = (double) (x - dfpnt) / (double) dnpnt;
				update_xcursor(0, x);
				set_cursors(0, 1, &y, oldy_cursor[0], oldy_cursor[1], &dum,
						dfpnt2, mnumypnts - dfpnt2);
				update_ycursor(0, y);
				wpon = y;
			} else {
				double wpmax;

				wpmax = sw - sw / (double) (fn / 2);
				wp += (wp / mnumypnts) * (y - wpon);
				if (wp > wpmax) {
					wp = wpmax;
					sp = 0.0;
				} else {
					if (wp < 20.0 * sw / fn)
						wp = 20.0 * sw / fn;
					sp -= (wp - wpsav) * frqset;
				}
			}
			UpdateVal(HORIZ, WP_NAME, wp, SHOW);
			spon = FALSE;
		} else {
			if (spon) {
				freq = sp + (double) (x - dfpnt) * wp / (double) dnpnt;
				sp -= freq - frqset;
			} else
				update_ycursor(0, 0);
			set_cursors(0, 1, &x, oldx_cursor[0], oldx_cursor[1], &dum, dfpnt,
					dnpnt);
			frqset = sp + (double) (x - dfpnt) * wp / (double) dnpnt;
			if (!spon)
				update_xcursor(0, x);
			spon = TRUE;
			wpon = FALSE;
		}
		if (debug1)
			Wscrprintf("before sp_wp sp= %g, wp= %g\n", sp, wp);
		hzpp = sw / (double) (fn / 2);
		if (wp != wpsav) {
			if (sp < 0)
				sp = 0.0;
			if (sp + wp > sw - hzpp) {
				wp = sw - hzpp;
				sp = 0.0;
			}
		}
		if ((wpon) && (wp == wpsav))
			sp = spsav;
		checkreal(&wp, 4.0 * hzpp, sw - hzpp);
		wp = (double) ((int) (wp / hzpp + 0.01)) * hzpp;
		checkreal(&sp, rflrfp, sw - wp - hzpp);
		sp = (double) ((int) (sp / hzpp + 0.01)) * hzpp;
		if (debug1)
			Wscrprintf("after sp_wp sp= %g, wp= %g\n", sp, wp);
		if ((sp != spsav) || (wp != wpsav) || (newvp!=vpsave)) {
			UpdateVal(HORIZ, SP_NAME, sp, NOSHOW);
			UpdateVal(HORIZ, WP_NAME, wp, NOSHOW);
			if (spon)
				update_xcursor(0, dfpnt + (int) ((double) dnpnt * (frqset - sp)
						/ wp));
			exp_factors(FALSE);
			fiddisplay(DISPLAY_ALL);
			UpdateVal(HORIZ, SP_NAME, sp, SHOW);
		}
	}
	vpsave=newvp;
}

/*****************/
static void b_spwp()
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
    m_spwp(4,0,0,0);
    disp_status("sf wf ");
    dfstatus("sf wf");
    InitVal(FIELD3,HORIZ,SP_NAME, PARAM_COLOR,NOUNIT,
                         SP_NAME,-PARAM_COLOR,SCALED,3);
    InitVal(FIELD4,HORIZ,WP_NAME, PARAM_COLOR,NOUNIT,
                         WP_NAME,-PARAM_COLOR,SCALED,3);
  }
  else
    b_cursor();
}

/*****************/
static void b_dscale()
/*****************/
{
  if (dscale_on())
  {
    new_dscale(TRUE,FALSE);
#ifdef VNMRJ
    P_setreal(GLOBAL, "mfShowAxis", 0.0, 0);
#endif
  }
  else {
#ifdef VNMRJ
    P_setreal(GLOBAL, "mfShowAxis", 1.0, 0);
#endif
    new_dscale(FALSE,TRUE);
  }
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

#ifdef XXX
/*****************/
static void b_thresh()
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
    dfstatus("thresh");
    DispField1(FIELD3, PARAM_COLOR,"th");
    DispField2(FIELD3,-PARAM_COLOR,th, 1);
    InitVal(FIELD4,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,3);
  }
  else
  {
    ds_mode = (oldmode == CURSOR_MODE) ? -1 : CURSOR_MODE;
    b_cursor();
  }
  threshflag = (!threshflag);
}
#endif

/*****************/
static void b_imaginary()
/*****************/
/* Cycle the (real / real+imaginary / real+zero) display state. */
{

  if (phaseflag)
    Wturnoff_mouse();
  Wgmode();
  if (imagflag && zeroflag) {
    realflag = TRUE;
    imagflag = zeroflag = FALSE;
  } else if (imagflag) {
    realflag = zeroflag = TRUE;
  } else {
    realflag = imagflag = TRUE;
    zeroflag = FALSE;
  }
  fiddisplay(DISPLAY_REAL | DISPLAY_IMAGINARY);
  if (!imagflag) {
    /* Erase the imaginary or zero trace */
    erase2 = 0;
    displayspec(dfpnt,dnpnt,0,&spec2,&spec2,&erase2,0,0,IMAG_COLOR);
    erase2 = 0;
  }
  if (absflag) {
      erase3 = 0;
      displayspec(dfpnt,dnpnt,0,&spec3,&spec3,&erase3,0,0,MAGENTA);
      erase3 = 0;
  }
  if (paflag) {
      erase6 = 0;
      displayspec(dfpnt,dnpnt,0,&spec6,&spec6,&erase6,0,0,ORANGE);
      erase6 = 0;
  }
  if (envelopeflag) {
      erase4 = erase5 = 0;
      displayspec(dfpnt,dnpnt,0,&spec4,&spec4,&erase4,0,0,WHITE);
      displayspec(dfpnt,dnpnt,0,&spec5,&spec5,&erase5,0,0,WHITE);
      erase4 = erase5 = 0;
  }
  absflag = envelopeflag = 0;
  paflag = 0;

  DispField1(FIELD2, PARAM_COLOR,"vf");
  DispField2(FIELD2,-PARAM_COLOR,vs, 1);
  DispField1(FIELD1, PARAM_COLOR, vpStr());
  DispField2(FIELD1,-PARAM_COLOR, vpVal(), 1);

  if (!imagflag)  {
    if (P_setstring(CURRENT,"displaymode","r",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  else if (!zeroflag)  {
    if (P_setstring(CURRENT,"displaymode","ir",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  else  {
    if (P_setstring(CURRENT,"displaymode","zir",0))
      Werrprintf("Unable to set variable \"displaymode\".");
    }
  appendvarlist("displaymode");

  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

/****************/
static int b_expand()
/****************/
{ double spsav,wpsav;
  double c2,d2;

  ds_reset();
  spsav = sp;
  wpsav = wp;
  if (ds_mode!=BOX_MODE)
  {
    disp_status("FULL  ");
    dfstatus("FULL");
    /* set sp,wp for full display */
    sp  = 0.0;
    wp  = sw - sw/(double) (fn/2);
    ds_mode=CURSOR_MODE; /* force b_cursor() into the BOX_MODE */
#ifdef VNMRJ
    getCurrentZoomFromList(&c2,&d2,NULL,NULL);
    if(d2<0) {
        c2=cr;d2=delta;
    }
    cr=c2; delta=d2;
    UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
    UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
#endif
  }
  else
  {
    disp_status("EXPAND");
    dfstatus("EXPAND");
    /* set sp,wp according to expansion box */
    sp  = cr;
    wp  = delta;
    set_sp_wp(&sp,&wp,sw,fn/2,rflrfp);
    delta = wp;
    cr = sp;
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
#ifdef VNMRJ
       addNewZoom("df",cr,delta,0,0);
       getNextZoom(&cr,&delta,NULL,NULL);
       UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
       UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
#endif

  }
  Wgmode();
  Wturnoff_mouse();
  /* store the parameters */
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  exp_factors(FALSE);
  fiddisplay(DISPLAY_ALL);
  b_cursor();
#ifdef VNMRJ
  if(is_aip_window_opened()) aipSpecViewUpdate();
#endif
  return 0;
}

#ifdef XXX
extern int menuflag;
/****************/
static void b_return()
/****************/
{
  Wturnoff_buttons();
  Wsetgraphicsdisplay("df");
  EraseLabels();
  if (menuflag)
    execString("menu\n");
}
#endif

/************************/
static int ds_multiply(int x, int y)
/************************/
{
  int spec;

  Wgmode();
  if(get_drawVscale()) new_dscale(TRUE,FALSE);
  if (x < dfpnt + (int)(0.05 * (double)dnpnt) || x >= dfpnt+dnpnt )
  {
    setVpVal( (double) (y - dfpnt2) / dispcalib - wc2/2.0);
    P_setreal(CURRENT, vpStr(), vpVal(), 0);
    DispField2(FIELD1,-PARAM_COLOR, vpVal(), 1);
    if(get_drawVscale()) fiddisplay(DISPLAY_ALL & ~DISPLAY_ERASE_SCALE);
    else fiddisplay(DISPLAY_ALL);
  }
  else
  {
      /* Set vscale with cursor. Select trace to scale on. */
    if (absflag) {
	spec = spec3;
    } else if (paflag) {
	spec = spec6;
    } else if (envelopeflag) {
	spec = spec4;
    } else if (realflag) {
	spec = spec1;
    } else if (imagflag) {
	spec = spec2;
    } else {
	return COMPLETE;
    }
    vs *= vs_mult(x, y, mnumypnts,
		  dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),
		  dfpnt, dnpnt,
		  spec);
    MAXMIN(vs,1.0e9,0.01);
    P_setreal(CURRENT,"vf",vs,0);
    DispField2(FIELD2,-PARAM_COLOR, vs, 1);
    if(get_drawVscale()) fiddisplay(DISPLAY_ALL & ~DISPLAY_ERASE_SCALE);
    else fiddisplay(DISPLAY_ALL & ~DISPLAY_SCALE);
  }
  return(COMPLETE);
}

/*****************/
static int turnoff_dfid()
/*****************/
{
  Wgmode();
#ifdef VNMRJ
  vj_alpha_mode(TRUE);
#endif
  if (phaseflag)
     Wturnoff_mouse();
  Wturnoff_mouse();
  phaseflag = FALSE;
  ds_reset();
  endgraphics();
/*  exit_display();*/
  if (freebuffers()) return(ERROR);
  return(COMPLETE);
}

static void
calcFidarea(float *data, int npts)
{
    register int i, cnt;
    register double area = 0;
    register double a;
    register double b;
    char *pname = "fidarea";
    char pnewString[MAXSTR];
    double norm;

    cnt = npts / 2;
    i = 0;
    if (realflag)
    {
       while (i < cnt)
       {
          a = data[i];
          i += 2;
          area += fabs(a);
       }
    }
    else
    {
       while (i < cnt)
       {
          a = data[i];
          i++;
          b = data[i];
          i++;
          area += sqrt(a*a + b*b);
       }
    }
    area /= (double) cnt;

    if (!P_getreal(GLOBAL,"fidnorm",&norm,1))
    {
       area *= norm;
    }
    /* NB: Be oblivious to failure to set the parameter. */
    i = P_setreal(GLOBAL, pname, area, 1);
    /* NB: appendvarlist() pops us out of df mode, send pnew directly. */
#ifdef VNMRJ
    sprintf(pnewString, "1 %s %.4g", pname, area);
    writelineToVnmrJ("pnew", pnewString);
#endif 
}

/****************/
static int imagdisp()
/****************/
{  float scl;
   int vmin = 1;
   int vmax = mnumypnts - 3;
   getVLimit(&vmin, &vmax);
   drawPlotBox();

   scl = (zeroflag) ? 0.0 : scale;
   if (fillflag)
   {
      int off = dfpnt2 + (int)(dispcalib * (vpVal() + wc2/2.0));

      fid_ybars(spectrum+(fpnt * 2)+1,(double) (vs * scl),dfpnt,dnpnt,npnt,
             off,next,dotflag);
      fill_ybars(dfpnt,dnpnt,off,next);
   }
   else
   {
      fid_ybars(spectrum+(fpnt * 2)+1,(double) (vs * scl),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vpVal() + wc2/2.0)),next,dotflag);
   }
   displayspec(dfpnt,dnpnt,0,&next,&spec2,&erase2,vmax,vmin,IMAG_COLOR);
   return(COMPLETE);
}

/****************/
static int phaseangledisp()
/****************/
{
   int npts = fn/2;
   int i;
   float r = 180/3.14159;
   int vmin = 1;
   int vmax = mnumypnts - 3;
   getVLimit(&vmin, &vmax);
   drawPlotBox();
   
   phaseangle = (float *) allocateWithId( npts * sizeof(float), "phaseangle"); 
   phaseangle2(spectrum, phaseangle, npts, COMPLEX, 1, 1, FALSE, 0, 0);
   for(i=0; i<npts; i++) phaseangle[i] *= r;

   if (fillflag)
   {
      int off = dfpnt2 + (int)(dispcalib * (vp + wc2/2.0));

      calc_ybars(phaseangle+(fpnt),1,(double) (vs * scale),dfpnt,dnpnt,npnt,
             off,next);
      fill_ybars(dfpnt,dnpnt,off,next);
   }
   else
   {
      calc_ybars(phaseangle+(fpnt),1,(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next);
   }
   displayspec(dfpnt,dnpnt,0,&next,&spec6,&erase6,vmax,vmin,ORANGE);
   
   return(COMPLETE);
}

/****************/
static int absdisp()
/****************/
{
   int vmin = 1;
   int vmax = mnumypnts - 3;
   getVLimit(&vmin, &vmax);
   drawPlotBox();

   if (fillflag)
   {
      int off = dfpnt2 + (int)(dispcalib * (vp + wc2/2.0));

      abs_ybars(spectrum + (fpnt * 2), (double)(vs * scale), dfpnt, dnpnt, npnt,
	      off, next, dotflag, 1, 0);
      fill_ybars(dfpnt,dnpnt,off,next);
   }
   else
   {
      abs_ybars(spectrum + (fpnt * 2), (double)(vs * scale), dfpnt, dnpnt, npnt,
	      dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)), next, dotflag, 1, 0);
   }
   displayspec(dfpnt, dnpnt, 0, &next, &spec3, &erase3, vmax, vmin,
               ABSVAL_FID_COLOR);
   return(COMPLETE);
}

/****************/
static int envelopedisp()
/****************/
{
    int yoffset;
    int vmin = 1;
    int vmax = mnumypnts - 3;
    getVLimit(&vmin, &vmax);
    drawPlotBox();


    yoffset = dfpnt2 + (int)(dispcalib * (vp + wc2 / 2.0));
    envelope_ybars(spectrum + (fpnt * 2),
		   (double)(vs * scale),
		   dfpnt,
		   dnpnt,
		   npnt,
		   yoffset,
		   next,
		   dotflag,
		   1);
    displayspec(dfpnt,
		dnpnt,
		0,
		&next,
		&spec4,
		&erase4,
		vmax,
		vmin,
		FID_ENVEL_COLOR);
    envelope_ybars(spectrum + (fpnt * 2),
		   (double)(vs * scale),
		   dfpnt,
		   dnpnt,
		   npnt,
		   yoffset,
		   next,
		   dotflag,
		   -1);
    displayspec(dfpnt,
		dnpnt,
		0,
		&next,
		&spec5,
		&erase5,
		vmax,
		vmin,
		FID_ENVEL_COLOR);

    return(COMPLETE);
}

/****************/
static int freebuffers()
/****************/
{ int res;

  if(c_buffer>=0) /* release last used block */
    if ( (res=D_release(D_PHASFILE,c_buffer)) )
    {
      D_error(res); 
      D_close(D_PHASFILE);  
      return(ERROR);
    }
  return(COMPLETE);
}

/*************/
static void setwindows()
/*************/
{
  Wclear_graphics();
  change_color(1,1);        /* reset the color table */
  refresh_graf();           /* some window systems needs refresh */
  Wgmode(); /* goto tek graphics and set screen 2 active */
  show_plotterbox();
  Wshow_graphics();
}

/****************************/
static int ds_checkinput(int argc, char *argv[], int *trace)
/****************************/
{ int arg_no;

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
static void init_flags()
/*************/
{
    char pstring[16];
    char* parname;

    dotflag = TRUE;
    if (P_getstring(CURRENT,"dotflag" ,pstring, 1, sizeof(pstring)) == 0) {
	dotflag = (pstring[0] != 'n');
    }
    realflag = TRUE;
    dcflag = fillflag = imagflag = absflag = envelopeflag = zeroflag = FALSE;
    paflag = FALSE;
    parname = "displaymode";
    if (P_getstring(GLOBAL, "acqmode", pstring, 1, sizeof(pstring)) == 0) {
        if (strcmp(pstring, "fidscan") == 0) {
            parname = "fidscanmode";
        }
    }
    if (P_getstring(CURRENT, parname, pstring, 1, sizeof(pstring)) == 0) {
	realflag = strchr(pstring, 'r') != NULL;
	dcflag = strchr(pstring, 'd') != NULL;
	fillflag = strchr(pstring, 'f') != NULL;
	imagflag = strchr(pstring, 'i') != NULL;
	absflag = strchr(pstring, 'a') != NULL;
	envelopeflag = strchr(pstring, 'e') != NULL;
	zeroflag = strchr(pstring, 'z') != NULL;
	paflag = strchr(pstring, 'p') != NULL;
    }
    clear_envelope();
    clear_phaseangle();
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
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  clear_dscale();
}

/*************/
static int init_vars2()
/*************/
{
  char flag[16];

  spec1  = 0;
  spec2  = 1;
  spec3  = 2;
  spec4  = 3;
  spec5  = 4;
  spec6  = 5;
  next   = 6;
  erase1 = 0;
  erase2 = 0;
  erase3 = 0;
  erase4 = 0;
  erase5 = 0;
  erase6 = 0;
  scale = 0.0;
  phaseflag = FALSE;
  init_display(); /* release old spectra? */
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  setwindows(); /* clears graphics and shows plotterbox */
  ResetLabels();
  dotflag = TRUE;
  if (P_getstring(CURRENT,"dotflag" ,flag, 1,16) == 0) 
      dotflag = (flag[0] != 'n');
/* displayspec or erasespec can erase old before display new */
  if (calc_fid(specIndex-1)) return(ERROR); /* gets new fid */
  return(COMPLETE);
}

/*************/
static int init_vars3()
/*************/
{
  spec1  = 0;
  spec2  = 1;
  spec3  = 2;
  spec4  = 3;
  spec5  = 4;
  spec6  = 5;
  next   = 6;
  erase1 = 0;
  erase2 = 0;
  erase3 = 0;
  erase4 = 0;
  erase5 = 0;
  erase6 = 0;
  scale = 0.0;
  phaseflag = FALSE;
  init_display(); /* release old spectra? */
/*  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0; */
    /* remove cursor */
  ResetLabels();
  init_flags();

  if (calc_fid(specIndex-1)) return(ERROR); /* gets new fid */
  return(COMPLETE);
}

/*************/
static int init_ds(int argc, char *argv[])
/*************/
{

  if (ds_checkinput(argc,argv,&specIndex)) return(ERROR);
  revflag = 0;
  if(initfid(1)) return(ERROR);
  if ((specIndex < 1) || (specIndex > nblocks * specperblock))
  { if (!WgraphicsdisplayValid("df") && (argc>1) && (isReal(argv[1])))
      Werrprintf("spectrum %d does not exist",specIndex);
    specIndex = 1;
  }
  init_vars1();
  return(COMPLETE);
}

/*************/
int dfid(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  int redisplay_param_flag, do_menu=FALSE;
  int fidshim = FALSE;

#ifdef VNMRJ
  if( !Bnmr && !(argc>1 &&
       (strcmp(argv[argc-1], "redisplay parameters") == 0 || 
	strcmp(argv[1], "again") == 0)))
  {
	execString("setButtonMode(0)\n");
  }
#endif

  if (argc > 1 && strcmp(argv[1],"fidshim") == 0) {
      fidshim = TRUE;
      argc--;
      argv--;
  }

  if (Bnmr)
  {
     if (fidshim)
     {
       revflag = 0;
       if(initfid(1)) return(ERROR);
       if (df_repeatscan == 1)  /* This is only true during wbs processing */
         fn = -fn;             /* force display of new fid. See get_one_fid() */
       if (calc_fid(specIndex-1)) /* gets new fid */
         ABORT;
       calcFidarea(spectrum, fn);
     }
     RETURN;
  }


  if (!fidshim) {
      Wturnoff_buttons();
      set_turnoff_routine(turnoff_dfid);
  }
#ifdef VNMRJ
  vj_alpha_mode(FALSE);
#endif

/*  restore_original_cursor(); */

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
        b_imaginary();
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
    else if (strcmp(argv[1],"fidshim") == 0) {
        /* Already checked, but avoid error of default "else" */
    }
    else if (strcmp(argv[1],"again") == 0) {
        argc--;
	argv--;
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
            imagflag = FALSE; zeroflag = FALSE;
          }
          else if (strcmp(argv[2],"zero") == 0) {
            imagflag = TRUE;  zeroflag = FALSE;
          }
          else {
            imagflag = TRUE;  zeroflag = TRUE;
          }
        }
        else {
          imagflag = FALSE; zeroflag = FALSE;
        }
        b_imaginary();
        set_turnoff_routine(turnoff_dfid);
        RETURN;
      }
      else  {
          Werrprintf("Must be in df to use %s option",argv[1]);
          ABORT;
      }
    }
  }

  if (fidshim) {
      if (df_repeatscan == 1)  /* This is only true during wbs processing */
         fn = -fn;             /* force display of new fid. See get_one_fid() */
      if (calc_fid(specIndex-1)) return(ERROR); /* gets new fid */
      fiddisplay(DISPLAY_ALL & ~DISPLAY_SCALE);
      calcFidarea(spectrum, fn);
      if (Wisactive_mouse())
        return(COMPLETE);
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

/*  if ((strcmp(argv[0],"df")==0) && (df_repeatscan == 1)) */
  if (df_repeatscan == 1)
  {
    if (!WgraphicsdisplayValid("df") || (c_buffer < 0))
    {
      df_repeatscan = 2;
/*      execString("dfid\n"); */
      if (imagflag && zeroflag)
      {
        dfid(argc,argv,retc,retv);
        imagflag = TRUE;
        zeroflag = TRUE;
        imagdisp();
      }
      else if (imagflag)
      {
        dfid(argc,argv,retc,retv);
        imagflag = TRUE;
        zeroflag = FALSE;
        imagdisp();
      }
      else
        dfid(argc,argv,retc,retv);
      df_repeatscan = 1;
      return(COMPLETE);
    }

    if (realflag)
	erasespec( spec1, 0, FID_COLOR ); 
    if (imagflag)
	erasespec( spec2, 0, IMAG_COLOR );
    if (absflag)
	erasespec( spec3, 0, MAGENTA );
    if (paflag)
	erasespec( spec6, 0, ORANGE );
    if (envelopeflag) {
	erasespec( spec4, 0, WHITE );
	erasespec( spec5, 0, WHITE );
    }
    if (init_vars3()) return(ERROR);
    fiddisplay(DISPLAY_ALL & ~DISPLAY_SCALE);
    activate_mouse(dfid_m_newcursor,ds_reset);
/* for cursor, either imitate b_cursor() or find out who erases it */
    Wsetgraphicsdisplay("df");
    return(COMPLETE);
  }

  init_flags();

  if (!WgraphicsdisplayValid("df") ||
      ((argc>1) && (isReal(argv[1]))) ||
      (c_buffer < 0))
  {
    if (init_ds(argc,argv))
      return(ERROR);
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
        if ((res = P_getVarInfo(CURRENT,argv[i],&info)) == 0
	    || (res = P_getVarInfo(GLOBAL,argv[i],&info)) == 0)
	{
	    update = (info.group == G_DISPLAY
		      || strcmp(argv[i],"phfid") == 0
		      || strcmp(argv[i],"lsfid") == 0);
	}
        i++;
      }
      if (!update)
        return(COMPLETE);
    }
    revflag = 0;
    if(initfid(1)) return(ERROR);
    ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
    if ((!WgraphicsdisplayValid("df")) || ((argc>1) && (!isReal(argv[1]))) ||
		 (!redisplay_param_flag))
    {
      if (P_setstring(GLOBAL,"crmode","c",0))
        Werrprintf("Unable to set variable \"crmode\".");
      do_menu = TRUE;
    }
  }
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  if (init_vars2()) return(ERROR);
  fiddisplay(DISPLAY_ALL & ~DISPLAY_ERASE_SCALE);
  releasevarlist();
  b_cursor();
  Wsetgraphicsdisplay("df");
  if (do_menu)
    execString("menu('dfid')\n");
  set_turnoff_routine(turnoff_dfid);
  return(COMPLETE);
}

/*************/
int dfid2(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  if (!WgraphicsdisplayValid("df") || (c_buffer < 0))
  {
/*    execString("dfid\n"); */
    if (imagflag && zeroflag)
    {
      dfid(argc,argv,retc,retv);
      imagflag = TRUE;
      zeroflag = TRUE;
      imagdisp();
    }
    else if (imagflag)
    {
      dfid(argc,argv,retc,retv);
      imagflag = TRUE;
      zeroflag = FALSE;
      imagdisp();
    }
    else
      dfid(argc,argv,retc,retv);
    return(COMPLETE);
  }

  erasespec( spec1, 0, FID_COLOR ); 
  if (imagflag)
    erasespec( spec2, 0, IMAG_COLOR );
  if (init_vars3()) return(ERROR);
  fiddisplay(DISPLAY_ALL & ~DISPLAY_SCALE);
/*  Wsetgraphicsdisplay("df");
  if (do_menu)
    execString("menu('dfid')\n");
  set_turnoff_routine(turnoff_dfid);
*/
  RETURN;
}

/****************/
static int calc_fid(int trace)
/****************/
{ float datamax;

  if (debug1) Wscrprintf("function calc_fid\n");
  if ((spectrum = get_one_fid(trace,&fn,&c_block, dcflag)) == 0) return(ERROR);
  c_buffer = trace;
  c_first  = trace;
  c_last   = trace;
  if (normflag)
  {
    if ((scale == 0.0) || phaseflag)
    {
       maxfloat(spectrum,pointsperspec / 2, &datamax);
       scale = (float) dispcalib / datamax;
       if (debug1)
         Wscrprintf("datamax=%g ", datamax);
    }
  }
  else
    scale = (float) dispcalib / CALIB;
  if (debug1)
    Wscrprintf("normflag= %d scale=%g\n", normflag, scale);
  return(COMPLETE);
}

/****************/
static int fiddisp()
/****************/
{
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);
  drawPlotBox();

  if (debug1)
  {
    Wscrprintf("starting fid display\n");
    Wscrprintf("fpnt=%d, npnt=%d, mnumxpnts=%d, dispcalib=%g\n",
            fpnt,npnt,mnumxpnts,dispcalib);
  }

  if (fillflag)
  {
     int off = dfpnt2 + (int)(dispcalib * (vp + wc2/2.0));

     fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,off,next,0);
     fill_ybars(dfpnt,dnpnt,off,next);
  }
  else
  {
     fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),next,dotflag);
  }
  displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,vmax,vmin,FID_COLOR);
  return(COMPLETE);
}

extern void set_vscale(int off, double vscale);
/****************/
static int fiddisplay(int what)
/****************/
{
    int off = dfpnt2 + (int)(dispcalib * (vp + wc2/2.0));
    double vscale = vs * scale;
    set_vscale(off,vscale);
    if (realflag && (what & DISPLAY_REAL)) {
        fiddisp();
    } if (imagflag && (what & DISPLAY_IMAGINARY)) {
        imagdisp();
    } if (absflag && (what & DISPLAY_ABSVAL)) {
        absdisp();
    } if (paflag && (what & DISPLAY_PHASEANGLE)) {
        phaseangledisp();
    } if (envelopeflag && (what & DISPLAY_ENVELOPE)) {
        envelopedisp();
    } if (dscale_on() && (what & DISPLAY_SCALE)) {
        new_dscale(what & DISPLAY_ERASE_SCALE, what & DISPLAY_DRAW_SCALE);
    }
    return COMPLETE;
}

#ifdef VNMRJ

void df_setCursor(int x0, int x1)
{
  cr = + (x0 - dfpnt ) * wp  / dnpnt  + sp;
  delta = + (x1 - dfpnt ) * wp  / dnpnt  + sp;
  delta = delta - cr;

  P_setreal(CURRENT, "cr", cr, 1);
  P_setreal(CURRENT, "delta", delta, 1);

  return;
}

void df_zoom(int mode)
{
    ds_mode = mode;
  b_expand();

    if(!showCursor()) {
       ds_mode=CURSOR_MODE;
       b_cursor();
    }

  return;
}

static int showCursor()
{
    double d;

    if (getButtonMode() > 0) return 0;

    if (P_getreal(GLOBAL,"mfShowCursor", &d, 1) != 0) d = 1.0;

    if (d < 0.5) return 0;
    return 1;
}

void df_spwp(int but, int x, int y, int mflag)
{
   m_spwp(but, x, y, mflag);

    cr = sp;
    delta  = fabs(wp);

   //if(!showCursor()) {
      ds_mode=CURSOR_MODE;
      b_cursor();
   //}
}

void df_zoomCenterPeak(int x, int y, int but)
{
    double freq, s2, w2, d;
    double d1, d2, c1, c2;

    (void) y;
    freq = sp + (x - dfpnt) * wp / dnpnt;

    w2 = wp;
    s2 = freq - wp/2;
    if(s2 < -rflrfp) {
        w2 = 2*(freq + rflrfp);
    }
    d = freq + wp/2;
    if(d > (sw-rflrfp)) {
        d = 2*(sw - freq - rflrfp);
        if(d < w2) w2 = d;
    }
/*
fprintf(stderr,"center %f %f %f\n", rflrfp, sw, sw/(double) (fn/2)); 
*/
    sp = freq - w2/2;

    if(but == 3) {
        df_prevZoomin(&c2, &c1, &d2, &d1);
    } else if(w2 == wp) {
        df_nextZoomin(&c2, &c1, &d2, &d1);
    } else {
        wp = w2;
        c2 = freq - w2/2;
        d2 = w2;
    }

      P_setreal(CURRENT, "cr", c2, 1);
      P_setreal(CURRENT, "delta", d2, 1);
   cr=c2;
   delta=d2;

   df_zoom(BOX_MODE);
} 

void df_centerPeak(int x, int y, int but)
{
    double freq, s2, d2;

    freq = sp + (x - dfpnt) * wp / dnpnt;

    s2 = freq - wp/2;
    d2 = freq + wp/2;
    if(s2 < -rflrfp || d2 > (sw-rflrfp)) {
        df_zoomCenterPeak(x, y, but);
        return;
    }

    sp = s2;
    cr=s2;

    //P_setreal(CURRENT, "cr", s2, 1);

   df_zoom(BOX_MODE);

}

void df_nextZoomin(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;
     *c2 = sp;
     *d2 = wp;

   if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     *d2 -= d;
     *c2 -= 0.5*d;

     if(c1!=NULL) *c1 = 0;
     if(d1!=NULL) *d1 = 0;
}

void df_prevZoomin(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;
     *c2 = sp;
     *d2 = wp;

   if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     *d2 += d;
     *c2 += 0.5*d;

     *c1 = 0;
     *d1 = 0;
}

#endif

/*************/
int fidmax(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  int trace;
  float datamax;
  int doNF = 0;
  int argnum = 0;
  double tmp;
  int tmpFn;

  revflag = 0;
  if(initfid(1)) return(ERROR);
  trace = specIndex;
  if (argc>1)
  {
    if (isReal(argv[1]))
    {
      trace = (int) stringReal(argv[1]);
      argnum = 1;
    }
  }
  if ((trace < 1) || (trace > nblocks * specperblock))
  {
    trace = 1;
  }
  if (!P_getreal(PROCESSED, "nf", &tmp, 1))
  {
     if ( (tmp> 1.5) && (P_getactive(CURRENT,"cf") == 0) )
     {
        doNF = 1;
     }
  }

  tmpFn = -fn;
  if (doNF)
  {
     char *argv1[4];
     char *retv1[1];
     int cttemp;

     argv1[0]= "ddff";
     argv1[1] = (argnum) ?  argv[argnum] : "1";
     argv1[2]= "1";
     argv1[3]= "max";
     if ( ddf(4, argv1, 1, retv1) )
        return(ERROR);
     datamax = stringReal(retv1[0]);
     cttemp = 1;
     if (!P_getreal(PROCESSED, "ct", &tmp, 1))
        cttemp = (int) (tmp + 0.5);
     if (cttemp < 1)
        cttemp = 1;
     datamax /= (double) cttemp;
  }
  else if ((spectrum = get_one_fid(trace-1,&tmpFn,&c_block, dcflag)) == 0)
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

// called by init_flags so envelope will be recalculated.
static void clear_envelope() {
   if(fidenvelope != NULL) releaseWithId("fidemvelope");
   fidenvelope = NULL;
}

static void clear_phaseangle() {
   if(phaseangle != NULL) releaseWithId("phaseangle");
   phaseangle = NULL;
}

// called mouse moving and envelope is displayed.
// note, fidenvelope is vertically scaled, otherwise may overflow 32bit integer
static short *get_envelope() {
   if(fidenvelope == NULL) {
     int npts = fn/2;
     fidenvelope = (short *) allocateWithId( npts * sizeof(short), "fidemvelope"); 
     // calculate absolute values
     scabs(spectrum,2,(double)(vs * scale),fidenvelope,1,npts, 1);
     // calculate envelope 
     envelope(fidenvelope, npts, 1);
   }
   return fidenvelope;
}

static void df_update_crosshair(int x, double c2) {

    double start, len, scl, yval, spp;
    int rev, off;
    char str[16];
    float *yptr;

    // c2 is in second. get scl to convert it to unit specified by axis
    // scl=1.0 if axis='s', scl=0.001 if axis='m'. 
    get_scale_pars(HORIZ, &start, &len, &scl, &rev);
    if(scl == 0) scl=1.0;

    if(!get_drawVscale())
       x_crosshair(x, 0, mnumypnts, c2/scl, 0, 0);
    else { // determine magnitude yval. 
       spp = sw / ((double) (fn/2)); // second per point
       off = (int)(c2/spp); // index starts from 0 
       if(off < 0) off = 0;
       yptr = spectrum + 2*off; // multiply 2 because fid is complex data
       if (P_getstring(CURRENT, "displaymode", str, 1, sizeof(str))) strcpy(str,"r"); 
       if(yptr == NULL) return;
       if(strchr(str, 'r') != NULL) yval = yptr[0]; 
       else if(strchr(str, 'i') != NULL) yval = yptr[1];
       else if(strchr(str, 'a') != NULL) yval = sqrt(yptr[0]*yptr[0] + yptr[1]*yptr[1]);
       else if(strchr(str, 'e') != NULL) {
	  short *sptr = get_envelope();
	  if(sptr != NULL) {
	    sptr += off;
	    // Note, reverse vertical scaling
	    if((vs * scale) != 0) yval = (float)sptr[0]/(vs * scale);
	    else yval = (float)sptr[0];
	  } else { // max of absolute real and imaginary.
	    yval = max(fabs(yptr[0]), fabs(yptr[1]));
	  }
       } else if(strchr(str, 'p') != NULL && (yptr = phaseangle + off) != NULL) {
	 yval = yptr[0];
       } else yval = yptr[0];
       x_crosshair(x, 0, mnumypnts, c2/scl, 1, yval);
    }
}

// called by mouse moving
void df_newCrosshair(int x, int y)
{
    double c2;

    c2 = sp + (x - dfpnt ) * wp  / dnpnt;

    if(c2 < sp || c2 > sp+wp || y < 0.5 || y > mnumypnts) {
        m_noCrosshair();
        return;
    }
    df_update_crosshair(x, c2);
}

