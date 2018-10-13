/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "pvars.h"
#include "wjunk.h"
#include "variables.h"

#define FALSE           0
#define TRUE            1
#define SHORTSTR		20	
#define NOTOVERLAID_ALIGNED	2

extern int VnmrJViewId;
extern void  ds_centerPeak(int x, int y, int but);
extern void  df_centerPeak(int x, int y, int but);
extern void  dconi_centerPeak(int x, int y, int but);
extern void  ds_zoomCenterPeak(int x, int y, int but);
extern void  df_zoomCenterPeak(int x, int y, int but);
extern void  dconi_zoomCenterPeak(int x, int y, int but);
extern void df_newCrosshair(int x, int y);
extern int getOverlayMode();
extern void dconi_noCrosshair();
extern void ds_noCrosshair();
extern void dconi_newCrosshair4freq(int num, double c1, double c2,
        double f1, double f2, char *tr, char *ax1, char *ax2);
extern void ds_newCrosshair4freq(int num, double c1, double c2,
	double f1, double f2, char *tr, char *ax1, char *ax2);
extern void dconi_newCursor4freq(int num, double c1, double c2,
        double d1, double d2, int mode, char *tr, char *ax1, char *ax2);
extern void ds_newCursor4freq(int num, double c1, double c2,
            double d1, double d2, int mode, char *tr, char *ax1, char *ax2);
extern void dconi_newSpwp(int num, double c1, double c2,
        double d1, double d2, int but, char *tr, char *ax1, char *ax2);
extern void ds_newSpwp(int num, double c1, double c2,
	double d1, double d2, int but, char *tr, char *ax1, char *ax2);
extern void dconi_newVs(double newVs, char *ax1, char *ax2);
extern void ds_newVs(double newVs, char *ax);
extern int isCrosshairOk(int x, int y);
extern void dconi_newCrosshair(int x, int y);
extern void ds_newCrosshair(int x, int y);

static float m_spx, m_wpx, m_spy, m_wpy;
int trackCursor( argc, argv, retc, retv )
int argc;
char *argv[];
int retc;
char *retv[];
{
/* trackCursor('on'), set trackCursor = 1
   trackCursor('off'), set trackCursor = 0
   trackCursor('cursor'), draw cursor at frequency c1, c2 
   trackCursor('crosshair'), draw crosshair at c1, c2 
*/
    char tmp[ SHORTSTR ];
    double d;
    int m;
    float c1, c2, d1, d2, f1, f2;
    
    if(argc < 1) RETURN;

    Wgetgraphicsdisplay(tmp,SHORTSTR);
    if(strstr(tmp,"d") != tmp) RETURN;
    if(strstr(tmp,"dss") == tmp) RETURN;

    if(strcasecmp(argv[1], "noCrosshair") == 0) { 
      if(strstr(tmp, "con") != NULL) {
	dconi_noCrosshair();
      } else if(strstr(tmp, "ds") != NULL) {
	ds_noCrosshair();
      }
    } else if(strcasecmp(argv[1], "crosshair") == 0) { 

      if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
      if(d < 0.5) RETURN;

      if(strstr(tmp, "con") != NULL && argc > 8) {
	  c1 = atof(argv[2]);
	  c2 = atof(argv[3]);
	  f1 = atof(argv[4]);
	  f2 = atof(argv[5]);
	  dconi_newCrosshair4freq(4, c1, c2, f1, f2, argv[6], argv[7], argv[8]);

      } else if(strstr(tmp, "con") != NULL && argc > 4) {
	  c2 = atof(argv[2]);
	  f2 = atof(argv[3]);
	  dconi_newCrosshair4freq(2, 0.0, c2, 0.0, f2, "f1", "", argv[4]);

      } else if(strstr(tmp, "ds") != NULL && argc > 8) {
	  c1 = atof(argv[2]);
	  c2 = atof(argv[3]);
	  f1 = atof(argv[4]);
	  f2 = atof(argv[5]);
	  ds_newCrosshair4freq(4, c1, c2, f1, f2, argv[6], argv[7], argv[8]);

      } else if(strstr(tmp, "ds") != NULL && argc > 4) {
	  c2 = atof(argv[2]);
	  f2 = atof(argv[3]);
	  ds_newCrosshair4freq(2, 0.0, c2, 0.0, f2, "f1", "", argv[4]);
      }

    } else if(strcasecmp(argv[1], "cursor") == 0) { 

      if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
      if(d < 0.5) RETURN;

      if(strstr(tmp, "con") != NULL && argc > 9) {
	  c1 = atof(argv[2]);
	  c2 = atof(argv[3]);
	  d1 = atof(argv[4]);
	  d2 = atof(argv[5]);
	  m = atoi(argv[6]);
	  dconi_newCursor4freq(4, c1, c2, d1, d2, m, argv[7], argv[8], argv[9]);

      } else if(strstr(tmp, "con") != NULL && argc > 5) {
	  c2 = atof(argv[2]);
	  d2 = atof(argv[3]);
	  m = atoi(argv[4]);
	  dconi_newCursor4freq(2, 0.0, c2, 0.0, d2, m, "f1", "", argv[5]);

      } else if(strstr(tmp, "ds") != NULL && argc > 9) {
	  c1 = atof(argv[2]);
	  c2 = atof(argv[3]);
	  d1 = atof(argv[4]);
	  d2 = atof(argv[5]);
	  m = atoi(argv[6]);
	  ds_newCursor4freq(4, c1, c2, d1, d2, m, argv[7], argv[8], argv[9]);

      } else if(strstr(tmp, "ds") != NULL && argc > 5) {
	  c2 = atof(argv[2]);
	  d2 = atof(argv[3]);
	  m = atoi(argv[4]);
	  ds_newCursor4freq(2, 0.0, c2, 0.0, d2, m, "f1", "", argv[5]);
      }
    } else if(strcasecmp(argv[1], "spwp") == 0) { 

      if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
      if(d < 0.5)
        if (P_getreal(GLOBAL,"trackAxis", &d, 2) != 0) d = 0.0;
      if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) RETURN;

      if(strstr(tmp, "con") != NULL && argc > 9) {
	  m_spy = atof(argv[2]);
	  m_spx = atof(argv[3]);
	  m_wpy = atof(argv[4]);
	  m_wpx = atof(argv[5]);
	  m = atoi(argv[6]);
	  dconi_newSpwp(4, m_spy, m_spx, m_wpy, m_wpx, m, argv[7], argv[8], argv[9]);

      } else if(strstr(tmp, "con") != NULL && argc > 5) {
	  m_spx = atof(argv[2]);
	  m_wpx = atof(argv[3]);
	  m = atoi(argv[4]);
	  dconi_newSpwp(2, 0.0, m_spx, 0.0, m_wpx, m, "f1", "", argv[5]);

      } else if(strstr(tmp, "ds") != NULL && argc > 9) {
	  m_spy = atof(argv[2]);
	  m_spx = atof(argv[3]);
	  m_wpy = atof(argv[4]);
	  m_wpx = atof(argv[5]);
	  m = atoi(argv[6]);
	  ds_newSpwp(4, m_spy, m_spx, m_wpy, m_wpx, m, argv[7], argv[8], argv[9]);

      } else if(strstr(tmp, "ds") != NULL && argc > 5) {
	  m_spx = atof(argv[2]);
	  m_wpx = atof(argv[3]);
	  m = atoi(argv[4]);
	  ds_newSpwp(2, 0.0, m_spx, 0.0, m_wpx, m, "f1", "", argv[5]);
      }
    } else if(strcasecmp(argv[1], "vs2d") == 0 ) { 
        if(strstr(tmp, "con") != NULL && argc > 4) {
	   dconi_newVs(atof(argv[2]),argv[3],argv[4]);
	}
    } else if(strcasecmp(argv[1], "vs") == 0 ) { 
        if(strstr(tmp, "ds") != NULL && argc > 3) {
	   ds_newVs(atof(argv[2]),argv[3]);
	}
    }
        RETURN;
}

void m_noCrosshair()
{
    int i, vps;
    char tmp[ SHORTSTR ], cmd[MAXSTR];
    double d;

    Wgetgraphicsdisplay(tmp,SHORTSTR);
    if(strstr(tmp,"d") != tmp) return;

    if(strstr(tmp, "con") != NULL) {
	dconi_noCrosshair();
    } else if(strstr(tmp, "ds") != NULL) {
        ds_noCrosshair();
    }

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'noCrosshair\\')')\n", i+1);
          execString(cmd);

          //writelineToVnmrJ("CR ", cmd);
        }
   }
}

void m_crosshair(int but, int release, int x, int y)
{
    char tmp[ SHORTSTR ];
    double d;
    int y0;

/* crosshair == 1 to track mouse move*/
/* this function is called by processJMouse of buttons.c for mouse move */ 

    if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
    
    if(d < 0.5 || !isCrosshairOk(x, y)) {
	return;
    }

    y0 = mnumypnts-y-1;
    Wgetgraphicsdisplay(tmp,SHORTSTR);
    if(strstr(tmp,"d") != tmp) return;

    if(strstr(tmp, "con") != NULL) {
	dconi_newCrosshair(x, y0);
    } else if(strstr(tmp, "ds") != NULL) {
        ds_newCrosshair(x, y0);
    } else if(strstr(tmp, "df") != NULL) {
        df_newCrosshair(x, y0);
    }
}

void centerPeak(int x, int y, int but)
{
    char tmp[ SHORTSTR ];

    int y0 = mnumypnts-y-1;
    Wgetgraphicsdisplay(tmp,SHORTSTR);
    if(strstr(tmp,"d") != tmp) return;

    if(strstr(tmp, "con") != NULL) {
	dconi_centerPeak(x, y0, but);
    } else if(strstr(tmp, "ds") != NULL) {
        ds_centerPeak(x, y0, but);
    } else if(strstr(tmp, "df") != NULL) {
        df_centerPeak(x, y0, but);
    }
}

void zoomCenterPeak(int x, int y, int but)
{
    char tmp[ SHORTSTR ];

    int y0 = mnumypnts-y-1;
    Wgetgraphicsdisplay(tmp,SHORTSTR);
    if(strstr(tmp,"d") != tmp) return;

    if(strstr(tmp, "con") != NULL) {
	dconi_zoomCenterPeak(x, y0, but);
    } else if(strstr(tmp, "ds") != NULL) {
        ds_zoomCenterPeak(x, y0, but);
    } else if(strstr(tmp, "df") != NULL) {
        df_zoomCenterPeak(x, y0, but);
    }
}

void getSpwpInfo(double *spx, double *wpx, double *spy, double *wpy) {
     *spx = m_spx;
     *wpx = m_wpx;
     *spy = m_spy;
     *wpy = m_wpy;
}
