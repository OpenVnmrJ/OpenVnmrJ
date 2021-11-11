/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************/
/* full.c - full,left,right,center */
/*          f                      */
/*          mark                   */
/***********************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "disp.h"
#include "group.h"
#include "tools.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"
#include "init_display.h"

#define NOTOVERLAID_ALIGNED 2
extern int getOverlayMode();
extern int currentindex();
extern void get_mark2d_info(int *first_ch, int *last_ch, int *first_direction);
extern int init2d(int get_rev, int dis_setup);
extern int ds_mark(float *specptr, double cr_val, double del_val,
            int mark_mode, int display_flag, int button_flag,
            float *int_v_ptr, float *max_i_ptr);
extern int dconi_mark(int mark_mode, int display_flag, int button_flag,
            float *int_v_ptr, float *max_i_ptr, float *f1_max_ptr, float *f2_max_ptr);

extern int VnmrJViewId;
static int do_1d_mark(int cmd_is_ds, int retc, char *retv[], int num_num_args,
                      double *num_val_ptr );
static int do_2d_mark(int retc, char *retv[], int num_num_args,
                      double *num_val_ptr );
static int check_reset(int argc, char *argv[] );
static int do_mark_reset();
static int check_trace(int argc, char *argv[] );
static int num_vals_args(int firstarg, int lastarg, char *argv[],
                         double valptr[], int max_n_vals );
static int range_check(double c, double d, int mode, int direction);
extern int mnumypnts;
extern int mnumxpnts;
extern int ymin;
extern double getPlotMargin();
extern int xcharpixels;
extern int ycharpixels;
extern int right_edge;
extern int graf_width; 
extern int graf_height; 
extern void set_vscaleMode(int mode);
extern int get_drawVscale();

int fwc = 0;
int fsc = 0;
int fwc2 = 0;
int fsc2 = 0;

/************************/
int f_cmd(int argc, char *argv[], int retc, char *retv[])
/************************/
{
    int r;
    double sw_val,rfl_val,rfp_val;
    double fn_val;

    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;

    if ( (r=P_getreal(PROCESSED,"np" ,&fn_val, 1)) )
    {
       if (P_getreal(CURRENT,"np" ,&fn_val, 1))
       {
          Werrprintf("Call to 'f' failed. Parameter 'np' does not exist");
	  RETURN; 
       }
    }
    if ( (r=P_getreal(PROCESSED,"sw" ,&sw_val, 1)) )
    {
       if (P_getreal(CURRENT,"sw" ,&sw_val, 1))
       {
          Werrprintf("Call to 'f' failed. Parameter 'sw' does not exist");
          RETURN;
       }
    }
    if (sw_val < 0.1)
       RETURN;
    P_setreal(CURRENT,"sf",0.0,0);
    P_setreal(CURRENT,"wf",fn_val/sw_val/2.0,0);
    appendvarlist("sf");

    if ( (r=P_getreal(CURRENT,"rfl" ,&rfl_val, 1)) )
        rfl_val = 0.0;
    if ( (r=P_getreal(CURRENT,"rfp" ,&rfp_val, 1)) )
        rfp_val = 0.0;
    P_setreal(CURRENT,"wp",sw_val,0);
    P_setreal(CURRENT,"sp",rfp_val - rfl_val,0);
    appendvarlist("sp,wp");

	/* do sp1 and wp1 only, if 2D experiment */
    if ( (r=P_getreal(PROCESSED,"sw1" ,&sw_val, 1)) )
	    RETURN;
    if ( (r=P_getreal(PROCESSED,"fn1" ,&fn_val, 1)) )
	    RETURN;
    if (sw_val < 0.1)
       RETURN;
    if ( (r=P_getreal(CURRENT,"rfl1" ,&rfl_val, 1)) )
        rfl_val = 0.0;
    if ( (r=P_getreal(CURRENT,"rfp1" ,&rfp_val, 1)) )
        rfp_val = 0.0;
    P_setreal(CURRENT,"wp1",sw_val,0);
    P_setreal(CURRENT,"sp1",rfp_val - rfl_val,0);
    P_setreal(CURRENT,"sf1",0.0,0);
    P_setreal(CURRENT,"wf1",fn_val/sw_val/2.0,0);
    appendvarlist("sp1,wp1");

	/* do sp2 and wp2 only, if 3D experiment */
    if ( (r=P_getreal(PROCESSED,"sw2" ,&sw_val, 1)) )
	    RETURN;
    if ( (r=P_getreal(PROCESSED,"fn2" ,&fn_val, 1)) )
	    RETURN;
    if (sw_val < 0.1)
       RETURN;
    if ( (r=P_getreal(CURRENT,"rfl2" ,&rfl_val, 1)) )
        rfl_val = 0.0;
    if ( (r=P_getreal(CURRENT,"rfp2" ,&rfp_val, 1)) )
        rfp_val = 0.0;
    P_setreal(CURRENT,"wp2",sw_val,0);
    P_setreal(CURRENT,"sp2",rfp_val - rfl_val,0);
    P_setreal(CURRENT,"sf2",0.0,0);
    P_setreal(CURRENT,"wf2",fn_val/sw_val/2.0,0);
    appendvarlist("sp2,wp2");
    RETURN;
}

int setFullChart(int d2flag) {
   int r;
  double sc,wc,sc2,wc2,wcmax,wc2max;
  if ( (r=P_getreal(GLOBAL,"wcmax"  ,&wcmax,  1)) )
    { P_err(r,"global ","wcmax:");  wcmax = 500.0; }
  if ( (r=P_getreal(GLOBAL,"wc2max" ,&wc2max, 1)) )
    { P_err(r,"global ","wc2max:"); wc2max = 240.0; }
  if ( (r=P_getreal(CURRENT,"sc" ,&sc, 1)) )
    { P_err(r,"current ","sc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc" ,&wc, 1)) )
    { P_err(r,"current ","wc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"sc2" ,&sc2, 1)) )
    { P_err(r,"current ","sc2:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc2" ,&wc2, 1)) )
    { P_err(r,"current ","wc2:"); return 1; }

      if ( d2flag ) { 
  	  double xband;
	  double yband; 
          if ( ((mnumxpnts-right_edge) < 5) || ((mnumypnts-ymin) < 5) )
          {
             yband = xband = wcmax / 10.0;
          }
          else
          {
  	     xband = 9*xcharpixels/((double)(mnumxpnts-right_edge) / wcmax);
	     yband = 3*ycharpixels/((double)(mnumypnts-ymin) / wc2max); 
          }
	  wc  = wcmax - xband;
	  sc  = 0;
          wc2 = wc2max - yband;
          sc2 = yband/2;
      } else {
	  if(get_drawVscale() > 0) set_vscaleMode(0);
	  double margin = getPlotMargin();
          wc  = wcmax-2*margin;
	  sc  = margin;
          wc2 = wc2max-2*margin;
          sc2 = margin;
      }
      dss_sc = 0;
      dss_wc = wc;
      dss_sc2 = 0;
      dss_wc2 = wc2;
      P_setreal(CURRENT,"dss_sc",dss_sc,0);
      P_setreal(CURRENT,"dss_wc",dss_wc,0);
      P_setreal(CURRENT,"dss_sc2",dss_sc2,0);
      P_setreal(CURRENT,"dss_wc2",dss_wc2,0);

  P_setreal(CURRENT,"sc",sc,0);
  P_setreal(CURRENT,"wc",wc,0);
  P_setreal(CURRENT,"sc2",sc2,0);
  P_setreal(CURRENT,"wc2",wc2,0);
  fwc=(int)wc;
  fsc=(int)sc;
  fwc2=(int)wc2;
  fsc2=(int)sc2;
  RETURN;
}

// fwc,fsc,fwc2,fsc2 are always set to wc,sc,wc2,sc2 by Vnmrbg
// they differ only if user explicitly set wc,sc,wc2,sc2 in macro or command line.
// In this case, resize window will not call setFullChart.
int adjustFull() {
  int r;
  double t_sc,t_wc,t_sc2,t_wc2;
  if ( (r=P_getreal(CURRENT,"sc" ,&t_sc, 1)) )
    { P_err(r,"current ","sc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc" ,&t_wc, 1)) )
    { P_err(r,"current ","wc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"sc2" ,&t_sc2, 1)) )
    { P_err(r,"current ","sc2:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc2" ,&t_wc2, 1)) )
    { P_err(r,"current ","wc2:"); return 1; }

  if ( ((int)t_sc==fsc) &&
       ((int)t_wc==fwc) &&
       ((int)t_sc2==fsc2) &&
       ((int)t_wc2==fwc2) )
    return 1;
  else
    return 0;
}

int get2Dflag() { 
  int d2flag;
  double proc, ni_val;

   d2flag = -1;
   if ( !P_getreal(CURRENT, "procdim", &proc, 1) ) 
   {
      if (proc > 1.5)
         d2flag = 1;
      else if (proc > 0.5)
         d2flag=0;
   }
   if (d2flag == -1)
   {
      if ( !P_getreal(PROCESSED, "nD", &ni_val, 1) ) 
         d2flag = (ni_val > 1.0);
   }

   if (d2flag == -1)
   {
      if ( !P_getreal(PROCESSED, "ni", &ni_val, 1) ) 
	 d2flag = (ni_val > 0.5);
   }

   if (d2flag == -1)
   {
      if ( !P_getreal(PROCESSED, "ni2", &ni_val, 1) ) 
         d2flag = (ni_val > 1.0);
   }
   if (d2flag == -1)
      d2flag = 0;

   return d2flag;
}

/***************************/
int full(int argc, char *argv[], int retc, char *retv[])
/***************************/
{ int r,d2flag,dsflag;
  double sc,wc,sc2,wc2,wcmax,wc2max,xband;

   (void) argc;
   (void) retc;
   (void) retv;
   d2flag = get2Dflag();

  dsflag = 0;	/* preliminary */

  if (strcmp(argv[0],"full")==0) 
    {
        if (argc > 1)
        {
           if ( !strcasecmp(argv[1],"1d") )
              d2flag = 0;
           else if ( !strcasecmp(argv[1],"2d") )
              d2flag = 1;
        }
	setFullChart(d2flag && (!dsflag));
  	appendvarlist("sc,wc,sc2,wc2");
	RETURN;
    }

  if ( (r=P_getreal(GLOBAL,"wcmax"  ,&wcmax,  1)) )
    { P_err(r,"global ","wcmax:");  wcmax = 500.0; }
  if ( (r=P_getreal(GLOBAL,"wc2max" ,&wc2max, 1)) )
    { P_err(r,"global ","wc2max:"); wc2max = 240.0; }
  if ( (r=P_getreal(CURRENT,"sc" ,&sc, 1)) )
    { P_err(r,"current ","sc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc" ,&wc, 1)) )
    { P_err(r,"current ","wc:"); return 1; }
  if ( (r=P_getreal(CURRENT,"sc2" ,&sc2, 1)) )
    { P_err(r,"current ","sc2:"); return 1; }
  if ( (r=P_getreal(CURRENT,"wc2" ,&wc2, 1)) )
    { P_err(r,"current ","wc2:"); return 1; }

  xband = wcmax/10;

  if (strcmp(argv[0],"fullt")==0)
    { wc2 = (wc2max - xband) * 0.75;
      sc2 = 0;
      wc  = (wcmax - xband) * 0.80;
      sc  = xband/4;
    }
  else if (strcmp(argv[0],"left")==0)
    { if (d2flag & !dsflag)
	{ wc  = wcmax/2.0 - xband;
	  sc  = wc + xband;
        }
      else
	{ wc  = wcmax / 2.0;
	  sc  = wc;
	}
      wc2 = wc2max - xband;
      sc2 = 0;
    }
  else if (strcmp(argv[0],"center")==0)
    { wc  = wcmax/2.0 - xband;
      sc2 = 0.0;
      if (wc>(wc2max-sc2)) wc = wc2max-sc2;
      sc  = (wcmax-wc-xband)/2 + xband/8;
      wc2 = wc;
    }
  else if (strcmp(argv[0],"right")==0)
    { if (d2flag & !dsflag)
	{ wc  = wcmax/2.0 - xband;
	  sc  = xband/4;
        }
      else
	{ wc  = wcmax / 2.0;
	  sc  = 0.0;
	}
      wc2 = wc2max - xband;
      sc2 = 0;
    }
  else
    { Werrprintf("illegal callname %s in full",argv[0]);
      ABORT;
    }

      dss_sc = 0;
      dss_wc = wc;
      dss_sc2 = 0;
      dss_wc2 = wc2;
      P_setreal(CURRENT,"dss_sc",dss_sc,0);
      P_setreal(CURRENT,"dss_wc",dss_wc,0);
      P_setreal(CURRENT,"dss_sc2",dss_sc2,0);
      P_setreal(CURRENT,"dss_wc2",dss_wc2,0);

  P_setreal(CURRENT,"sc",sc,0);
  P_setreal(CURRENT,"wc",wc,0);
  P_setreal(CURRENT,"sc2",sc2,0);
  P_setreal(CURRENT,"wc2",wc2,0);
  appendvarlist("sc,wc,sc2,wc2");
  RETURN;
}

/*  Definitions used by MARK  */

#define CURSOR_MODE	1
#define BOX_MODE	5
#define FALSE		0
#define TRUE		1

/*  External variables used by MARK  */

extern double	 cr, delta, rflrfp, sw, rflrfp1, sw1;
extern int	 d2flag;
extern int	 dconi_mode;
extern int	 ds_mode;
extern float	*calc_spec();

/************************************************************************/
/*  The `mark' command has several "helper" subroutines.  The name of	*/
/*  each starts with `do_'.  Each returns 0 if successful; -1 if not.	*/
/*  Each displays a message (or one of its subroutines displays the	*/
/*  message) if an error occurs.					*/
/*									*/
/*  January 1989:  `reset' keyword erases both output files		*/
/*		   MARK without arguments requires a display command	*/
/*		   (DS or DCONI) to be current in the graphics window	*/
/************************************************************************/

int mark(int argc, char *argv[], int retc, char *retv[] )
{
	int	 firstarg, cmd_is_ds, mark_ret_val, num_num_args, reset_flag,
		 trace_flag;
	double	 mark_args[ 4 ];

/************************************************************************/
/*  Check for `reset' as a keyword.  Need to verify syntax.  Three	*/
/*  situations:  "reset" used improperly; "reset" used correctly;	*/
/*  no "reset" keyword.							*/
/************************************************************************/

	reset_flag = check_reset( argc, argv );
	if (reset_flag < 0)
	  ABORT;
	else if (reset_flag > 0) {
		if (do_mark_reset() == 0)
		  RETURN;
		else
		  ABORT;
	}

/************************************************************************/
/*  Check for `trace' as a keyword.  Situation is similar to `reset',	*/
/*  except there is no file name and the returned value distinguishes   */
/*  `trace' as first from `trace' as last argument (the only 2 choices) */
/************************************************************************/

	firstarg = 1;			/* Skip command name */
	trace_flag = check_trace( argc, argv );
	if (trace_flag < 0)
	  ABORT;
	if (trace_flag != 0) {
		if (retc > 2) {
			Werrprintf(
    "%s:  cannot return more than 2 values when using `trace' keyword", argv[ 0 ]
			);
			ABORT;
		}
		if (trace_flag == 1)	/* Skip over 1st argument */
		  firstarg = 2;		/* if it was `trace'	  */
		else			/* Otherwise it was the last */
		  argc--;		/* argument, so reduce total */
	}				/* number of arguments by 1  */

/************************************************************************/
/*  Extract numeric values, number of numeric values.  The `mark'	*/
/*  command accepts 0, 1, 2 or 4 such arguments.  If `num_vals_args'	*/
/*  returns -1, the subroutine has already posted an error message.	*/
/************************************************************************/

	num_num_args = num_vals_args( firstarg, argc, argv, &mark_args[ 0 ], 4 );
	if (num_num_args < 0)
	  ABORT;
	else if (num_num_args == 3 || num_num_args > 4) {
		Werrprintf( "%s:  incorrect number of numeric arguments", argv[ 0 ] );
		ABORT;
	}

/************************************************************************/
/*  Get the current graphics command to establish if DS was the last 	*/
/*  such command executed.  In that case, we perform a 1D operation	*/
/*  unless MARK was called with 4 numeric arguments.			*/
/*									*/
/*  Abort if current graphics command is not DS or DCONI.  Add checks	*/
/*  for other display commands (DCON, DPCON, etc.) here if required.	*/
/************************************************************************/

	cmd_is_ds = WgraphicsdisplayValid( "ds" );
	if ( !cmd_is_ds &&
	     !WgraphicsdisplayValid( "dconi" ) &&
	     num_num_args == 0 )
	{
		Werrprintf(
	    "%s:  requires arguments when no data displayed", argv[ 0 ]
		);
		ABORT;
	}

/************************************************************************/
/*  `init2d' subroutine defines `d2flag' and many other important	*/
/*  variables.  First argument instructs `init2d' to set "reverse"	*/
/*  flag if trace = "f1".  Second argument instructs `init2d' to	*/
/*  prepare chart variables for displaying data.			*/
/************************************************************************/

	if (init2d( 1, 1 ))
	  ABORT;

/************************************************************************/
/*  Now that `d2flag' is defined, check on 4 more error conditions:	*/
/*  A.  2D data is present, last command not `ds', 1 numeric argument	*/
/*      and no `trace' keyword.						*/
/*  B.  No 2D data present and 4 numeric arguments entered.		*/
/*  C.  No 2D data present and the `trace' keyword was used.		*/
/*  D.  No 2D data present and command returns more than 2 values.	*/
/************************************************************************/

	if (d2flag) {
		if (trace_flag == 0 && cmd_is_ds == 0 && num_num_args == 1) {
			Werrprintf(
    "%s:  'trace' keyword required with 2D data and 1 numeric argument", argv[ 0 ]
			);
			ABORT;
		}
	}
	else {					/* No 2D data */
		if (num_num_args == 4) {
			Werrprintf(
	    "%s:  Cannot have 4 numeric arguments with 1D data", argv[ 0 ]
			);
			ABORT;
		}
		if (trace_flag) {
			Werrprintf(
	    "%s:  Cannot use 'trace' keyword with 1D data", argv[ 0 ]
			);
			ABORT;
		}
		if (retc > 2) {
			Werrprintf(
	    "%s:  Cannot return more than 2 values with 1D data", argv[ 0 ]
			);
			ABORT;
		}
	}

/*  2D operations.  If 2D data is present and the `trace' keyword was
    NOT selected, then there were either 2 or 4 numeric arguments.	*/

	if (num_num_args == 4 || (d2flag && trace_flag == 0 && cmd_is_ds == 0)) {
		mark_ret_val = do_2d_mark(
			 retc, retv,
			 num_num_args,
			&mark_args[ 0 ]
		);
	}

/*  1D operations.  Come here if
    A.  num_num_args != 4 AND
    B.  d2flag is clear OR trace_flag is set OR cmd_is_ds is set.	*/

	else {
		mark_ret_val = do_1d_mark(
			 cmd_is_ds,
			 retc, retv,
			 num_num_args,
			&mark_args[ 0 ]
		);
	}

	if (mark_ret_val == 0)
	  RETURN;
	else
	  ABORT;
}

static int do_1d_mark(int cmd_is_ds, int retc, char *retv[], int num_num_args,
                      double *num_val_ptr )
{
	char	 return_buf[ 20 ];
	int	 ctrace, mark_mode, update_flag;
	float	*spectrum, integral, max_int;
	double	 cr_val, del_val;

/*  `calc_spec' is a subroutine in `proc2d.c'  */

        mark_mode = CURSOR_MODE;
	ctrace = currentindex();
	spectrum = calc_spec( ctrace-1, 0, FALSE, TRUE, &update_flag );
	if (num_num_args == 2) {
		cr_val = num_val_ptr[ 0 ];
		del_val = num_val_ptr[ 1 ];
		mark_mode = BOX_MODE;
	}
	else if (num_num_args == 1) {
		cr_val = num_val_ptr[ 0 ];
		del_val = 0.0;
		mark_mode = CURSOR_MODE;
	}
	else {				/* No numeric arguments */
		if (cmd_is_ds) {
			if (ds_mode == CURSOR_MODE || ds_mode == BOX_MODE)
			  mark_mode = ds_mode;
			else
			  mark_mode = CURSOR_MODE;
		}
		cr_val = cr;
		del_val = (mark_mode == CURSOR_MODE) ? 0.0 : delta;
	}
	if (range_check( cr_val, del_val, mark_mode, HORIZ))
	  ABORT;

/*  The ds mark program gets its cursor and delta from its argument list.  */

	ds_mark( spectrum,
		 cr_val,
		 del_val,
		 mark_mode,
		 (retc == 0),		/* display results on the screen */
		 FALSE,			/* ds_mark called from mark */
		&integral,
		&max_int
	);

	if (retc > 1) {
		rtoa( integral, &return_buf[ 0 ], 20 );
		retv[ 1 ] = newString( &return_buf[ 0 ] );
	}
	if (retc > 0) {
		rtoa( max_int, &return_buf[ 0 ], 20 );
		retv[ 0 ] = newString( &return_buf[ 0 ] );
	}

	RETURN;
}

static int do_2d_mark(int retc, char *retv[], int num_num_args,
                      double *num_val_ptr )
{
	char	 return_buf[ 20 ];
	int	mark_mode;
	float	f1_max, f2_max, integral, max_int;
	double	cr_vert, cr_horiz, del_vert, del_horiz;
	double	cr_vert_orig, cr_horiz_orig, del_vert_orig, del_horiz_orig;
        int     first_ch,last_ch,first_direction;

/*  The cr_vert_orig ... del_horiz_orig variables save the current cr`s
    and delta`s for each dimension, allowing the program to substitute
    the User arguments for the duration of the mark program.		*/

        get_mark2d_info(&first_ch,&last_ch,&first_direction);
	if (num_num_args == 4) {
            if (first_direction == HORIZ)
            {
		cr_vert = num_val_ptr[ 0 ];
		del_vert = num_val_ptr[ 1 ];
		cr_horiz = num_val_ptr[ 2 ];
		del_horiz = num_val_ptr[ 3 ];
            }
            else
            {
		cr_horiz = num_val_ptr[ 0 ];
		del_horiz = num_val_ptr[ 1 ];
		cr_vert = num_val_ptr[ 2 ];
		del_vert = num_val_ptr[ 3 ];
            }
	    mark_mode = BOX_MODE;
	}
	else if (num_num_args == 2) {
            if (first_direction == HORIZ)
            {
		cr_vert = num_val_ptr[ 0 ];
		cr_horiz = num_val_ptr[ 1 ];
            }
            else
            {
		cr_horiz = num_val_ptr[ 0 ];
		cr_vert = num_val_ptr[ 1 ];
            }
	    del_horiz = 0.0;
	    del_vert = 0.0;
	    mark_mode = CURSOR_MODE;
	}

/************************************************************************/
/*  Establish the mode (box vs. cursor).  First use the number of 	*/
/*  return values.  If no return values, use the mode from DCONI.	*/
/*  If that does not decide it, default to cursor mode.			*/
/************************************************************************/

	else {
		if (retc > 0)
		  mark_mode = (retc > 1) ? BOX_MODE : CURSOR_MODE;
		else if (dconi_mode == CURSOR_MODE || dconi_mode == BOX_MODE)
		  mark_mode = dconi_mode;
		else
		  mark_mode = CURSOR_MODE;
                get_cursor_pars(HORIZ,&cr_horiz,&del_horiz);
                get_cursor_pars(VERT,&cr_vert,&del_vert);
	}
	if (range_check( cr_horiz, del_horiz, mark_mode, HORIZ))
	  ABORT;
	if (range_check( cr_vert, del_vert, mark_mode, VERT))
	  ABORT;

/*  Save some current values as original vaules.	*/

	if (num_num_args > 0) {
		get_cursor_pars( HORIZ, &cr_horiz_orig, &del_horiz_orig );
		set_cursor_pars( HORIZ, cr_horiz, del_horiz );
		get_cursor_pars( VERT, &cr_vert_orig, &del_vert_orig );
		set_cursor_pars( VERT, cr_vert, del_vert );
	}

/*  The dconi mark program gets its cursors and deltas from the Display Objects.  */

	dconi_mark( mark_mode,
		    TRUE,		/* display results on the screen */
		    FALSE,		/* dconi_mark called from mark   */
		   &integral,
		   &max_int,
		   &f1_max,
		   &f2_max);

/*  Restore original values as current vaules.	*/

	if (num_num_args > 0) {
		set_cursor_pars( HORIZ, cr_horiz_orig, del_horiz_orig );
		set_cursor_pars( VERT, cr_vert_orig, del_vert_orig );
	}

	if (retc > 3) {
		rtoa( f2_max, &return_buf[ 0 ], 20 );
		retv[ 3 ] = newString( &return_buf[ 0 ] );
	}
	if (retc > 2) {
		rtoa( f1_max, &return_buf[ 0 ], 20 );
		retv[ 2 ] = newString( &return_buf[ 0 ] );
	}
	if (retc > 1) {
		rtoa( integral, &return_buf[ 0 ], 20 );
		retv[ 1 ] = newString( &return_buf[ 0 ] );
	}
	if (retc > 0) {
		rtoa( max_int, &return_buf[ 0 ], 20 );
		retv[ 0 ] = newString( &return_buf[ 0 ] );
	}

	RETURN;
}

/************************************************************************/
/*  Check for `reset' keyword in MARK command.				*/
/*  Return 0 if keyword not present.					*/
/*  Return -1 if keyword used incorrectly.				*/
/*  Return 1 if keyword present and used correctly.			*/
/************************************************************************/

static int check_reset(int argc, char *argv[] )
{
	int	found_reset, iter;

/*  No arguments except command name?  */

	if (argc < 2)
	  return( 0 );

/*  Scan argument vector for `reset'  */

	found_reset = 0;
	iter = 1;
	while (iter < argc && found_reset == 0)
	  if (strcmp( argv[ iter ], "reset" ) == 0)
	    found_reset = 131071;
	  else
	    iter++;

	if (found_reset == 0)
	  return( 0 );

/************************************************************************/
/*  If `reset' present, it must be the only argument.			*/
/************************************************************************/

	if ( argc > 2) {
		Werrprintf( "%s:  `reset' must be the only argument", argv[ 0 ] );
		return( -1 );
	}

	return( 1 );
}

/************************************************************************/
/*  implements the `reset' keyword of the `mark' command.		*/
/*  A global function so the DS command can access it.			*/
/************************************************************************/

static int do_mark_reset()
{
	char	final_path[ MAXPATHL ]; 
	int	ival1, ival2;

	ival1 = ival2 = 0;
	strcpy( &final_path[ 0 ], &curexpdir[ 0 ] );
#ifdef UNIX
	strcat( &final_path[ 0 ], "/mark1d.out" );
#else 
	strcat( &final_path[ 0 ], "mark1d.out" );
#endif 
	if (access( &final_path[ 0 ], 0 ) == 0) {
		ival1 = unlink( &final_path[ 0 ] );
		if (ival1 != 0) {
			Werrprintf( "Cannot remove %s", &final_path[ 0 ] );
		}
	}

	strcpy( &final_path[ 0 ], &curexpdir[ 0 ] );
#ifdef UNIX
	strcat( &final_path[ 0 ], "/mark2d.out" );
#else 
	strcat( &final_path[ 0 ], "mark2d.out" );
#endif 
	if (access( &final_path[ 0 ], 0 ) == 0) {
		ival2 = unlink( &final_path[ 0 ] );
		if (ival2 != 0) {
			Werrprintf( "Cannot remove %s", &final_path[ 0 ] );
		}
	}

	if (ival1 != 0 || ival2 != 0)
	  return( -1 );
	else
	  return( 0 );
}

/************************************************************************/
/*  Check for `trace' keyword in MARK command.
    Return 0 if keyword not present.
    Return -1 if keyword used incorrectly.
    Return 1 if keyword present and it is the first argument.
    Return 2 if keyword present and it is the last argument.		*/
/************************************************************************/

static int check_trace(int argc, char *argv[] )
{
	int	found_trace, iter;

/*  No arguments except command name?  */

	if (argc < 2)
	  return( 0 );

/*  Scan argument vector for `trace'  */

	found_trace = 0;
	iter = 1;
	while (iter < argc && found_trace == 0)
	  if (strcmp( argv[ iter ], "trace" ) == 0)
	    found_trace = 131071;
	  else
	    iter++;

	if (found_trace == 0)
	  return( 0 );

/*  If `trace' is present, it must be the first or the last argument.
    Either 0, 1 or 2 additional arguments can be present.		*/

	if ((iter != 1 && iter != argc-1) || argc > 4) {
		if (iter != 1 && iter != argc-1)
		  Werrprintf(
	    "%s:  `trace' must be the first or the last argument", argv[ 0 ]
		  );
		else
		  Werrprintf(
	    "%s:  wrong number of arguments with `trace' keyword", argv[ 0 ]
		  );
		return( -1 );
	}

	if (iter == 1)
	  return( 1 );
	else
	  return( 2 );
}

/************************************************************************/
/*  Get NUMeric VALues from ARGument vector.

    Returns number of values present (zero or larger); negative if error.
    Starts at `firstarg'; continues to `lastarg'.  Converts up tp
    `max_n_vals'; then just counts remaining numeric arguments.

    Returns error status (-1) if non-numeric argument encountered
    in the range [firstarg, lastarg).  Note that both are passed
    as C-style indexes.

    To facilitate reporting an error, assumes the 0th element in
    `argv' is the name of the command.					*/
/************************************************************************/

static int num_vals_args(int firstarg, int lastarg, char *argv[],
                         double valptr[], int max_n_vals )
{
	int	argno, valndx;
	double	cur_val;

	for (argno = firstarg; argno < lastarg; argno++)
	  if (isReal( argv[ argno ] )) {
		cur_val = stringReal( argv[ argno ] );
		valndx = argno-firstarg;
		if (valndx < max_n_vals)
		  valptr[ valndx ] = cur_val;
	  }
	  else {
		Werrprintf( "%s:  invalid argument '%s'", argv[ 0 ], argv[ argno ] );
		return( -1 );
	  }

	return( lastarg-firstarg );
}

/************************************************************************/
/*  range check for proposed values of cursor, delta.			*/
/*									*/
/*  `mode' is either CURSOR_MODE (check only value of cursor) or	*/
/*  BOX_MODE (check cursor, cursor-delta).  Direction selects either    */
/*  the horizontal or vertical axis.                              	*/
/*									*/
/*  Do not call this subroutine without calling `init2d' !!		*/
/*									*/
/*  The maximum value is sw-rfl+rfp; the minimum value is rfp-rfl.	*/
/*									*/
/************************************************************************/

static int range_check(double c, double d, int mode, int direction)
{
	int	 errval;
	double	 maxval, minval;

	if (direction == HORIZ)
        {
		maxval = sw-rflrfp;
		minval = -rflrfp;
	}
	else
        {
		maxval = sw1-rflrfp1;
		minval = -rflrfp1;
	}
	errval = 0;
	if (c > maxval)
	  errval = -1;
	else if (c < minval)
	  errval = -2;

	if (mode == BOX_MODE) {
		if (c-d < minval)
		  errval = -3;
	}

	if (errval == 0)
	  return( 0 );

	if ((errval == -1) || (errval == -2))
        {
		Werrprintf(
	    "mark:  value of cursor too %s for %s spectral window",
              (errval == -1) ? "large" : "small",
	      (direction == HORIZ) ? "horizontal" : "vertical");
	}
	else if (errval == -3) {
		Werrprintf("mark:  range outside spectral window");
	}
	return( errval );
}
