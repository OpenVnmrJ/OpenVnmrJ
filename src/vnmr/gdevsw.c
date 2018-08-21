/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*******************************************************************************
* file gdevsw.c:  Contains code to switch among various versions of graphics   *
*      --------   routines, based on the current output device type.  switc-   *
*                 ing is at a much lower frequency than calls to these         *
*                 routines, saving execution time.                             *
*									       *
* The VMS version does not use the `raster' routines, those with "raster_" in  *
* the routine name, as these use the pixrect data structure, available only    *
* with SunView.  But we do want to access the PostScript routines on VMS.  So  *
* we define a dummy VMS routine for the raster entry.  Then in graphics.c we   *
* prevent the raster entry from being used while allowing access to the        *
* PostScript stuff.  The dummy routine alerts to accidental access.	       *
*******************************************************************************/

#include <stdio.h>
#include "vnmrsys.h"
#define FILE_IS_GDEVSW
#include "graphics.h"
#undef FILE_IS_GDEVSW
#include "wjunk.h"

int gen_vstring(char *s)		 
{ int i;
  i = 0;
  while (s[i])
    (*(*active_gdevsw)._vchar)(s[i++]);
  return(0);
}

int gen_no_op()		 
{
  return(0);
}

void set_bootup_gfcn_ptrs()
{
#ifdef SUN
   extern int sun_setdisplay(), sun_sunGraphClear(), sun_grf_batch(), 
              sun_usercoordinate(), sunColor_sun_window(), 
	      sunColor_change_contrast(), sunColor_change_color();
   extern int sun_rdraw(), sun_adraw(), sun_dchar(), sun_dstring(), 
	      sun_dvchar(), sun_ybars(), sun_grayscale_box(), sun_box(), 
	      sun_normalmode(), sun_xormode();
   extern int sun_vchar(), sun_vstring(), sun_dimage();
#endif 
   extern int sun_raster_color();
#ifdef X11
   extern int raster_color();
#endif 

#ifdef VNMRJ
   extern int sun_dvstring();
#endif 


   extern int plot_endgraphics(), plot_graf_clear(), plot_color(), 
	      plot_charsize(), plot_amove(), plot_rdraw(), plot_adraw(), 
	      plot_dchar(), plot_dstring(), plot_dvchar(), plot_dvstring(), 
	      plot_ybars(), plot_grayscale_box(), plot_box(), plot_normalmode(),
	      plot_xormode(),gen_dvstring();
   extern int plot_vchar(), plot_vstring(), plot_fontsize();

/*  The next block of routines on the Sun assume a pixrect data
    structure is available, so they are only defined for the Sun.	*/

#ifdef SUN
   extern int fraster_charsize(), raster_amove(), raster_rdraw(), 
	      raster_adraw(), raster_dchar(), raster_dstring(), raster_dvchar(),
	      raster_ybars(), raster_grayscale_box(), raster_box();
   extern int raster_vchar(), raster_fontsize(), raster_dimage();
#else 
   extern int vms_no_raster();
#endif 

   extern int default_endgraphics(), default_graf_clear(), default_charsize(),
	      default_coord0(), default_amove(), default_dvstring(), 
	      default_sun_window(), default_change_contrast(), 
	      default_change_color(),default_ybars();

   extern int ps_endgraphics(), ps_graf_clear(), ps_color(), ps_charsize(),
              ps_amove(), ps_rdraw(), ps_adraw(), ps_dchar(),ps_dstring(),
              ps_dvchar(), ps_dvstring(), ps_ybars(), ps_normalmode(),
              ps_xormode(), ps_grayscale_box(), ps_box(); 

   extern int ps_vchar(), ps_vstring(), ps_fontsize(), ps_dimage();

#ifndef SIS
   extern int graphon_setdisplay(), tek4x05_setdisplay(), tek4x07_setdisplay(), 
              hds_setdisplay(), default_setdisplay();
   extern int graphon_hds_coord0(), tek_coord0();
   extern int default_sunGraphClear();
   extern int default_grf_batch();
   extern int tek_change_color();
   extern int default_usercoordinate();
   extern int graphon_hds_color(), tek_color(), default_color();
   extern int graphon_hds_amove(), tek_amove();
   extern int default_rdraw();
   extern int graphon_hds_adraw(), tek_adraw(), default_adraw();
   extern int graphon_hds_dchar(), tek4x05_dchar(), tek4x07_dchar(), 
	      default_dchar();
   extern int default_dstring();
   extern int graphon_hds_dvchar(), tek4x05_dvchar(), tek4x07_dvchar(), 
	      default_dvchar();
   extern int graphon_hds_dvstring();
   extern int graphon_hds_ybars(), tek_ybars();
   extern int graphon_hds_grayscale_box(), default_grayscale_box();
   extern int graphon_hds_box(), tek_box(), default_box();
   extern int graphon_hds_normalmode(), tek_normalmode(), default_normalmode();
   extern int graphon_hds_xormode(), tek_xormode(), default_xormode();
#endif 

   gdevsw_array[C_TERMINAL]._endgraphics = default_endgraphics;
   gdevsw_array[C_TERMINAL]._graf_clear  = default_graf_clear;
   gdevsw_array[C_TERMINAL]._charsize    = default_charsize;
   gdevsw_array[C_TERMINAL]._fontsize    = default_charsize;

   gdevsw_array[C_PLOT]._endgraphics     = plot_endgraphics;
   gdevsw_array[C_PLOT]._graf_clear      = plot_graf_clear;
   gdevsw_array[C_PLOT]._color		 = plot_color;
   gdevsw_array[C_PLOT]._charsize        = plot_charsize;
   gdevsw_array[C_PLOT]._fontsize        = plot_fontsize;
   gdevsw_array[C_PLOT]._amove	         = plot_amove;
   gdevsw_array[C_PLOT]._rdraw	         = plot_rdraw;
   gdevsw_array[C_PLOT]._adraw	         = plot_adraw;
   gdevsw_array[C_PLOT]._dchar	         = plot_dchar;
   gdevsw_array[C_PLOT]._dstring	 = plot_dstring;
   gdevsw_array[C_PLOT]._dvchar	         = plot_dvchar;
   gdevsw_array[C_PLOT]._vchar	         = plot_vchar;
   gdevsw_array[C_PLOT]._dvstring	 = plot_dvstring;
   gdevsw_array[C_PLOT]._vstring	 = plot_vstring;
   gdevsw_array[C_PLOT]._ybars	         = plot_ybars;
   gdevsw_array[C_PLOT]._grayscale_box   = plot_grayscale_box;
   gdevsw_array[C_PLOT]._box             = plot_box;
   gdevsw_array[C_PLOT]._normalmode      = plot_normalmode;
   gdevsw_array[C_PLOT]._xormode         = plot_xormode;
   gdevsw_array[C_PLOT]._dimage	         = gen_no_op;

   gdevsw_array[C_PSPLOT]._endgraphics   = ps_endgraphics;
   gdevsw_array[C_PSPLOT]._graf_clear    = ps_graf_clear;
   gdevsw_array[C_PSPLOT]._color	 = ps_color;
   gdevsw_array[C_PSPLOT]._charsize      = ps_charsize;
   gdevsw_array[C_PSPLOT]._fontsize      = ps_fontsize;
   gdevsw_array[C_PSPLOT]._amove	 = ps_amove;
   gdevsw_array[C_PSPLOT]._rdraw	 = ps_rdraw;
   gdevsw_array[C_PSPLOT]._adraw	 = ps_adraw;
   gdevsw_array[C_PSPLOT]._dchar	 = ps_dchar;
   gdevsw_array[C_PSPLOT]._dstring  	 = ps_dstring;
   gdevsw_array[C_PSPLOT]._dvchar	 = ps_dvchar;
   gdevsw_array[C_PSPLOT]._vchar	 = ps_vchar;
   gdevsw_array[C_PSPLOT]._dvstring	 = ps_dvstring;
   gdevsw_array[C_PSPLOT]._vstring	 = ps_vstring;
   gdevsw_array[C_PSPLOT]._ybars	 = ps_ybars;
   gdevsw_array[C_PSPLOT]._grayscale_box = ps_grayscale_box;
   gdevsw_array[C_PSPLOT]._box           = ps_box;
   gdevsw_array[C_PSPLOT]._normalmode    = ps_normalmode;
   gdevsw_array[C_PSPLOT]._xormode       = ps_xormode;
   gdevsw_array[C_PSPLOT]._dimage        = ps_dimage;

#ifdef SUN
   gdevsw_array[C_RASTER]._endgraphics   = plot_endgraphics;
   gdevsw_array[C_RASTER]._graf_clear    = plot_graf_clear;
#ifdef X11
   gdevsw_array[C_RASTER]._color         = raster_color;
#else 
   gdevsw_array[C_RASTER]._color         = sun_raster_color;
#endif 
   gdevsw_array[C_RASTER]._charsize      = fraster_charsize;
   gdevsw_array[C_RASTER]._fontsize      = raster_fontsize;
   gdevsw_array[C_RASTER]._amove         = raster_amove;
   gdevsw_array[C_RASTER]._rdraw         = raster_rdraw;
   gdevsw_array[C_RASTER]._adraw	 = raster_adraw;
   gdevsw_array[C_RASTER]._dchar	 = raster_dchar;
   gdevsw_array[C_RASTER]._dstring	 = raster_dstring;
   gdevsw_array[C_RASTER]._dvchar	 = raster_dvchar;
   gdevsw_array[C_RASTER]._vchar	 = raster_vchar;
   gdevsw_array[C_RASTER]._dvstring      = gen_dvstring;
   gdevsw_array[C_RASTER]._vstring       = gen_vstring;
   gdevsw_array[C_RASTER]._ybars         = raster_ybars;
   gdevsw_array[C_RASTER]._grayscale_box = raster_grayscale_box;
   gdevsw_array[C_RASTER]._box           = raster_box;
   gdevsw_array[C_RASTER]._normalmode    = plot_normalmode;
   gdevsw_array[C_RASTER]._xormode       = plot_xormode;
   gdevsw_array[C_RASTER]._dimage        = raster_dimage;
#else 
   gdevsw_array[C_RASTER]._endgraphics   = vms_no_raster;
   gdevsw_array[C_RASTER]._graf_clear    = vms_no_raster;
   gdevsw_array[C_RASTER]._color         = vms_no_raster;
   gdevsw_array[C_RASTER]._charsize      = vms_no_raster;
   gdevsw_array[C_RASTER]._fontsize      = vms_no_raster;
   gdevsw_array[C_RASTER]._amove         = vms_no_raster;
   gdevsw_array[C_RASTER]._rdraw         = vms_no_raster;
   gdevsw_array[C_RASTER]._adraw	 = vms_no_raster;
   gdevsw_array[C_RASTER]._dchar	 = vms_no_raster;
   gdevsw_array[C_RASTER]._dstring	 = vms_no_raster;
   gdevsw_array[C_RASTER]._dvchar	 = vms_no_raster;
   gdevsw_array[C_RASTER]._vchar	 = vms_no_raster;
   gdevsw_array[C_RASTER]._dvstring      = vms_no_raster;
   gdevsw_array[C_RASTER]._vstring       = vms_no_raster;
   gdevsw_array[C_RASTER]._ybars         = vms_no_raster;
   gdevsw_array[C_RASTER]._grayscale_box = vms_no_raster;
   gdevsw_array[C_RASTER]._box           = vms_no_raster;
   gdevsw_array[C_RASTER]._normalmode    = vms_no_raster;
   gdevsw_array[C_RASTER]._xormode       = vms_no_raster;
   gdevsw_array[C_RASTER]._dimage        = vms_no_raster;
#endif 

   if ( Wissun() )
   {
      _setdisplay     = sun_setdisplay;
      _coord0         = default_coord0;
      _sunGraphClear  = sun_sunGraphClear;
      _grf_batch      = sun_grf_batch;
      _usercoordinate = sun_usercoordinate;
      gdevsw_array[C_TERMINAL]._color         = sun_raster_color;
      gdevsw_array[C_TERMINAL]._amove         = default_amove;
      gdevsw_array[C_TERMINAL]._rdraw         = sun_rdraw;
      gdevsw_array[C_TERMINAL]._adraw         = sun_adraw;
      gdevsw_array[C_TERMINAL]._dchar         = sun_dchar;
      gdevsw_array[C_TERMINAL]._dstring       = sun_dstring;
      gdevsw_array[C_TERMINAL]._dvchar        = sun_dvchar;
      gdevsw_array[C_TERMINAL]._vchar         = sun_vchar;
#ifdef VNMRJ
      gdevsw_array[C_TERMINAL]._dvstring      = sun_dvstring;
#else
      gdevsw_array[C_TERMINAL]._dvstring      = gen_dvstring;
#endif
      gdevsw_array[C_TERMINAL]._vstring       = sun_vstring;
      gdevsw_array[C_TERMINAL]._ybars         = sun_ybars;
      gdevsw_array[C_TERMINAL]._grayscale_box = sun_grayscale_box;
      gdevsw_array[C_TERMINAL]._box           = sun_box;
      gdevsw_array[C_TERMINAL]._normalmode    = sun_normalmode;
      gdevsw_array[C_TERMINAL]._xormode       = sun_xormode;
      gdevsw_array[C_TERMINAL]._dimage        = sun_dimage;
      if ( WisSunColor() )
      {
	 _sun_window      = sunColor_sun_window;
	 _change_contrast = sunColor_change_contrast;
	 _change_color    = sunColor_change_color;
      }
      else
      {
         gdevsw_array[C_TERMINAL]._color = default_color;
	 _sun_window      = default_sun_window;
	 _change_contrast = default_change_contrast;
	 _change_color    = default_change_color;
      }
   }
   else
#ifndef VNMRJ
   if ( Wisgraphon() )
   {
      _setdisplay      = graphon_setdisplay;
      _coord0          = graphon_hds_coord0;
      _sunGraphClear   = default_sunGraphClear;
      _grf_batch       = default_grf_batch;
      _sun_window      = default_sun_window;
      _change_contrast = default_change_contrast;
      _change_color    = default_change_color;
      _usercoordinate  = default_usercoordinate;
      gdevsw_array[C_TERMINAL]._color         = graphon_hds_color;
      gdevsw_array[C_TERMINAL]._amove         = graphon_hds_amove;
      gdevsw_array[C_TERMINAL]._rdraw         = default_rdraw;
      gdevsw_array[C_TERMINAL]._adraw         = graphon_hds_adraw;
      gdevsw_array[C_TERMINAL]._dchar         = graphon_hds_dchar;
      gdevsw_array[C_TERMINAL]._dstring       = default_dstring;
      gdevsw_array[C_TERMINAL]._dvchar        = graphon_hds_dvchar;
      gdevsw_array[C_TERMINAL]._vchar         = graphon_hds_dchar;
      gdevsw_array[C_TERMINAL]._dvstring      = graphon_hds_dvstring;
      gdevsw_array[C_TERMINAL]._vstring       = gen_vstring;
      gdevsw_array[C_TERMINAL]._ybars         = graphon_hds_ybars;
      gdevsw_array[C_TERMINAL]._grayscale_box = graphon_hds_grayscale_box;
      gdevsw_array[C_TERMINAL]._box           = graphon_hds_box;
      gdevsw_array[C_TERMINAL]._normalmode    = graphon_hds_normalmode;
      gdevsw_array[C_TERMINAL]._xormode       = graphon_hds_xormode;
      gdevsw_array[C_TERMINAL]._dimage        = gen_no_op;
   }
   else if ( Wistek() )
   {
      _coord0          = tek_coord0;
      _sunGraphClear   = default_sunGraphClear;
      _grf_batch       = default_grf_batch;
      _sun_window      = default_sun_window;
      _change_contrast = default_change_contrast;
      _change_color    = tek_change_color;
      _usercoordinate  = default_usercoordinate;
      gdevsw_array[C_TERMINAL]._color         = tek_color;
      gdevsw_array[C_TERMINAL]._amove         = tek_amove;
      gdevsw_array[C_TERMINAL]._rdraw         = default_rdraw;
      gdevsw_array[C_TERMINAL]._adraw         = tek_adraw;
      gdevsw_array[C_TERMINAL]._dstring       = default_dstring;
      gdevsw_array[C_TERMINAL]._dvstring      = gen_dvstring;
      gdevsw_array[C_TERMINAL]._vstring       = gen_vstring;
      gdevsw_array[C_TERMINAL]._ybars         = tek_ybars;
      gdevsw_array[C_TERMINAL]._grayscale_box = default_grayscale_box;
      gdevsw_array[C_TERMINAL]._box           = tek_box;
      gdevsw_array[C_TERMINAL]._normalmode    = tek_normalmode;
      gdevsw_array[C_TERMINAL]._xormode       = tek_xormode;
      gdevsw_array[C_TERMINAL]._dimage        = gen_no_op;
      if ( Wistek4x05() )
      {
	 _setdisplay = tek4x05_setdisplay;
         gdevsw_array[C_TERMINAL]._dchar  = tek4x05_dchar;
         gdevsw_array[C_TERMINAL]._dvchar = tek4x05_dvchar;
         gdevsw_array[C_TERMINAL]._vchar = tek4x05_dchar;
      }
      else if ( Wistek4x07() )
      {
	 _setdisplay = tek4x07_setdisplay;
         gdevsw_array[C_TERMINAL]._dchar  = tek4x07_dchar;
         gdevsw_array[C_TERMINAL]._dvchar = tek4x07_dvchar;
         gdevsw_array[C_TERMINAL]._vchar = tek4x07_dchar;
      }
      else
	 Werrprintf ( "Programming error:  Unknown Tektronix terminal type" );
   }
   else if ( Wishds() )
   {
      _setdisplay      = hds_setdisplay;
      _coord0          = graphon_hds_coord0;
      _sunGraphClear   = default_sunGraphClear;
      _grf_batch       = default_grf_batch;
      _sun_window      = default_sun_window;
      _change_contrast = default_change_contrast;
      _change_color    = default_change_color;
      _usercoordinate  = default_usercoordinate;
      gdevsw_array[C_TERMINAL]._color         = graphon_hds_color;
      gdevsw_array[C_TERMINAL]._amove         = graphon_hds_amove;
      gdevsw_array[C_TERMINAL]._rdraw         = default_rdraw;
      gdevsw_array[C_TERMINAL]._adraw         = graphon_hds_adraw;
      gdevsw_array[C_TERMINAL]._dchar         = graphon_hds_dchar;
      gdevsw_array[C_TERMINAL]._dstring       = default_dstring;
      gdevsw_array[C_TERMINAL]._dvchar        = graphon_hds_dvchar;
      gdevsw_array[C_TERMINAL]._vchar         = graphon_hds_dchar;
      gdevsw_array[C_TERMINAL]._dvstring      = graphon_hds_dvstring;
      gdevsw_array[C_TERMINAL]._vstring       = gen_vstring;
      gdevsw_array[C_TERMINAL]._ybars         = graphon_hds_ybars;
      gdevsw_array[C_TERMINAL]._grayscale_box = graphon_hds_grayscale_box;
      gdevsw_array[C_TERMINAL]._box           = graphon_hds_box;
      gdevsw_array[C_TERMINAL]._normalmode    = graphon_hds_normalmode;
      gdevsw_array[C_TERMINAL]._xormode       = graphon_hds_xormode;
      gdevsw_array[C_TERMINAL]._dimage        = gen_no_op;
   }
   else
#endif
   {
      _setdisplay      = default_setdisplay;
      _coord0          = default_coord0;
      _sunGraphClear   = default_sunGraphClear;
      _grf_batch       = default_grf_batch;
      _sun_window      = default_sun_window;
      _change_contrast = default_change_contrast;
      _change_color    = default_change_color;
      _usercoordinate  = default_usercoordinate;
      gdevsw_array[C_TERMINAL]._color         = default_color;
      gdevsw_array[C_TERMINAL]._amove         = default_amove;
      gdevsw_array[C_TERMINAL]._rdraw         = default_rdraw;
      gdevsw_array[C_TERMINAL]._adraw         = default_adraw;
      gdevsw_array[C_TERMINAL]._dchar         = default_dchar;
      gdevsw_array[C_TERMINAL]._dstring       = default_dstring;
      gdevsw_array[C_TERMINAL]._dvchar        = default_dvchar;
      gdevsw_array[C_TERMINAL]._vchar         = default_dchar;
      gdevsw_array[C_TERMINAL]._dvstring      = default_dvstring;
      gdevsw_array[C_TERMINAL]._vstring       = gen_vstring;
      gdevsw_array[C_TERMINAL]._ybars         = default_ybars;
      gdevsw_array[C_TERMINAL]._grayscale_box = default_grayscale_box;
      gdevsw_array[C_TERMINAL]._box           = default_box;
      gdevsw_array[C_TERMINAL]._normalmode    = default_normalmode;
      gdevsw_array[C_TERMINAL]._xormode       = default_xormode;
      gdevsw_array[C_TERMINAL]._dimage        = gen_no_op;
   }
   active_gdevsw = &gdevsw_array[C_TERMINAL];
}

/**************/
int gen_dvstring(char *s)		 
/**************/
{ int i;
  i = 0;
  while (s[i])
    (*(*active_gdevsw)._dvchar)(s[i++]);
  return(0);
}

