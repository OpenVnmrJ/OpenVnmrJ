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
#include "allocate.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "pvars.h"
#include "wjunk.h"
#include "variables.h"
#include "aipCInterface.h"
#include "buttons.h"

#define CURSOR_MODE     1
#define BOX_MODE        5

#define FALSE           0
#define TRUE            1
#define SHORTSTR	20	
#define SWAP(a,b) {swap=(a);(a)=(b);(b)=swap;}

#define CLOSE          0
#define OPEN           1
#define STANDARD       0
#define WALKUP         1
#define IMAGING        2

#define NOTOVERLAID_NOTALIGNED 0
#define OVERLAID_NOTALIGNED 1
#define NOTOVERLAID_ALIGNED 2
#define OVERLAID_ALIGNED 3
#define STACKED 4
#define UNSTACKED 5

#define DEFAULT_CHART  0
#define DEFAULT_1D  1
#define DEFAULT_2D  2
#define ALIGN_1D_X  3
#define ALIGN_1D_Y  4
#define ALIGN_2D  5
#define ALIGN_1D_XY  6

/* minmumn delta and delta1 (in Hz) for zoom*/
#define MIN_DELTA      100

#define TEXTTYPE  "textFrame" 
#define VJCMD     "vnmrjcmd" 

extern int graf_width;
extern int graf_height;
extern int VnmrJViewId;
extern int start_from_ft;
extern int start_from_ft2d;

extern int getAspMouse();
extern int getOverlayMode(); 
extern int getChartMode(); 
extern int getActiveWin(); 
extern int aipOwnsScreen();
extern void ds_overlaySpec(int);
extern void dconi_overlaySpec(int);
extern void ds_sendSpecInfo(int);
extern void dconi_sendSpecInfo(int);
extern void dconi_getNextCursor(double *c2, double *c1, double *d2, double *d1);
extern void dconi_setNextCursor(double c2, double c1, double d2, double d1);
extern void dconi_sendSpwp(double c1, double c2, double d1, double d2, int mode);
extern void ds_sendSpwp(double c2, double d2, int but);
extern int  dconi_currentZoom(double *c2, double *d2, double *c1, double *d1);
extern int  get_ds_threshold_flag();
extern int  get_ds_threshold_loc();
extern int  get_ds_cursor_flag();
extern void set_ds_threshold_flag(int n);
extern void set_ds_threshold_loc(int n);
extern void set_ds_cursor_flag(int n);
extern void dconi_nextZoomin(double *, double *, double *, double *);
extern void ds_nextZoomin(double *, double *, double *, double *);
extern void df_nextZoomin(double *, double *, double *, double *);
extern void df_spwp(int but, int x, int y, int mflag);
extern void df_zoom(int mode);
extern void ds_zoom(int mode);
extern void dconi_zoom(int mode);
extern void df_setCursor(int x0, int x1);
extern void ds_setCursor(int x0, int x1);
extern void dconi_setCursor(int x0, int y0, int x1, int y1);
extern void dconi_setInsetCursor(int x0, int y0, int x1, int y1);
extern void ds_inset(float c, float d);
extern void clearGraphFunc();
extern void copy_inset_frame_cmd(int newid, int oldid);
extern int imagefile(int args, char *argv[], int retc, char *retv[]);
extern void dconi_spwp(int but, int x, int y, int mflag);

extern int getButtonMode();
extern int get_window_width();
extern int get_window_height();
extern void jframeFunc(const char *, int, int, int, int, int); 
extern int getUseXFunc();
extern int get_axis_freq(int);
extern double ds_spwp(int, int, int, int);
extern void jmouse_spwp(int, int, int, int);
extern void dfww_spwp(int, int, int, int);
extern void getAxes(char *ax2, char *ax1, int len);
extern void getAxis(char * axis, int n);
extern int expdir_to_expnum(char *expdir);
extern void set_jframe_id(int id);
extern int  isReexecCmd(char *sName);

int testFrameID(int id);
void mfZoom(int step);
void overlaySpec(int mode);
void saveZoom();
void setCurrZoom(int);
static void doZoom(int mode);
static void getNextZoomin(double *c2, double *c1, double *d2, double *d1);
static int isImaging();
static int showTextLayout(char *path);
static int saveTextLayout(char *path);
static int getNumberFrames();

typedef struct {
   double cr;
   double cr1;
   double delta;
   double delta1;
   double sp;
   double wp;
   double sp1;
   double wp1;
   double sp2;
   double wp2;
   double sf;
   double wf;
   double sf1;
   double wf1;
   double sf2;
   double wf2;
   double sc;
   double wc;
   double sc2;
   double wc2;
   double vp;
   double ho;
   double vo;
   double vs;
   double vs2d;
   double vsproj;
   double threshold;
   double showAxis;
   double showFields;
   double phasing;
   double phasef;
   double lifrq;
   double lvltlt;
   int    dfpnt, dnpnt;
   int    dfpnt2, dnpnt2;
   int    ds_threshold_flag;
   int    ds_threshold_y;
   int    ds_cursor_flag;
   char aig[SHORTSTR];
   char crmode[SHORTSTR];
   char trace[SHORTSTR];
   char axis[SHORTSTR];
   char f1[SHORTSTR];
   char f2[SHORTSTR];
   char intmod[SHORTSTR];

   } spectrumInfo;

typedef struct {
   double cr;
   double cr1;
   double delta;
   double delta1;

   } zoomInfo;

typedef struct {
   double scale_sp;
   double scale_wp;
   double scale_scl;
   double scale_vp_off;
   int    scale_rev;
   int    scale_color;
   int    scale_vp;
   int    scale_df;
   int    scale_dn;
   int    scale_df2;
   int    scale_axis;
   int    scale_flag;
   } dscaleInfo;

typedef struct {

   int expID;
   int vpID;
   int objID; // imagefile id
   int x, y, w, h;
   char displayType[SHORTSTR]; 
   char displayCmd[SHORTSTR]; 
   char fidfile[MAXSTR]; 
   char textColor[MAXSTR]; 
   int fontSize; 
   char textStyle[MAXSTR]; 
   int status; 
   int saved; 
   spectrumInfo params; 
   dscaleInfo dparams; 
   int currzoom;
   int maxzooms;
   zoomInfo *zoomList;
   char cmd[8];
   int dpf_flag;
   int dpir_flag;
   char dpf_cmd[64];
   char dpir_cmd[64];
} frameInfo;

static frameInfo *frames = NULL;
static int maxframes = 0;
static int currframe = 0;
static int debug = 0;
static char tmpStr[MAXSTR];

void
printFrame(int id)
{
    int i, first, last, nframes;

    if(id < 0) {
	first = 0; 
	last = maxframes-1; 
    } 
    else {
	first = id; 
	last = id; 
    }
       
    if(!testFrameID(last)) return;

    nframes = getNumberFrames();

    for(i=first; i<=last; i++) {
	if(frames[i].status == CLOSE ) continue;
fprintf(stderr, "nframes, currframe %d %d\n", nframes, currframe);
fprintf(stderr, "id, expID, vpID %d %d %d\n", i, frames[i].expID, frames[i].vpID);
fprintf(stderr, "x, y, w, h %d %d %d %d\n", 
frames[i].x, frames[i].y, frames[i].w, frames[i].h);
fprintf(stderr, "displayType, displayCmd %s %s\n", frames[i].displayType, frames[i].displayCmd);
    }
}

void initZoomInfo(int id)
{
    if(!testFrameID(id)) return;

    strcpy(frames[id].cmd,"");
    if(frames[id].zoomList != NULL && frames[id].maxzooms > 0)
        free(frames[id].zoomList);
    frames[id].maxzooms=0;
    frames[id].currzoom=-1;
    frames[id].zoomList = NULL;
}


void initFrameInfo(int id)
{
    frameInfo *frame;

    if(!testFrameID(id)) return;

    frame = &frames[id];
    frame->saved = 0;
    frame->status = CLOSE;

    frame->expID = expdir_to_expnum(curexpdir);
    frame->vpID = VnmrJViewId;
    frame->objID = 0;
    strcpy(frame->fidfile, "");
    strcpy(frame->displayType, "");
    strcpy(frame->displayCmd, "");
    strcpy(frame->textColor, "yellow");
    strcpy(frame->textStyle, "PlainText");
    frame->fontSize = 14;
    initZoomInfo(id);
    frame->params.ds_threshold_flag = 0;
    frame->params.ds_threshold_y = 0;
    frame->params.ds_cursor_flag = 0;
    frame->dparams.scale_flag = 0;
    frame->dpf_flag = 0;
    frame->dpir_flag = 0;
    strcpy(frame->dpf_cmd, "");
    strcpy(frame->dpir_cmd, "");
}

static int getNumberFrames()
{
    int i, n;

    if(frames == NULL || maxframes < 1) return 0;

    n = 0;
    for(i=1; i<maxframes; i++) {
	if(frames[i].status != CLOSE) n++;	
    }
    return n;
}

void deleteFrame(int id)
{
    int i, i1, n;
    if(!testFrameID(id)) return;

    i1 = n = 0;
    for(i=1; i<maxframes; i++) {
	if(i == id) {
	  if(i == 1) frames[i].status = CLOSE;
	  else initFrameInfo(i);
	  i1 = id;
        } else if(frames[i].status == OPEN && 
	    strcmp(frames[i].displayType,"text") != 0) n++;
    }
    if(n == 0) 
          jframeFunc("closeall", 0, 0, 0, 0, 0);
    else if(i1 == id)
          jframeFunc("close", id, 0, 0, 0, 0);
}

void deleteText()
{
    int i;
    if(frames == NULL || maxframes < 1) return;

    for(i=1; i<maxframes; i++) {
	if(strcasecmp(frames[i].displayType, "text") == 0) {
	  initFrameInfo(i);
	}
    }
    //jframeFunc("closealltext", 0, 0, 0, 0, 0);
    writelineToVnmrJ(VJCMD, "canvas textBox deleteall");
}

void showText()
{
    int i;
    char cmd[MAXSTR];
    if(frames == NULL || maxframes < 1) return;

    for(i=1; i<maxframes; i++) {
        if(strcasecmp(frames[i].displayType, "text") == 0 &&
          	frames[i].status == CLOSE) {
	   frames[i].status = OPEN;
          sprintf(cmd, "mfaction('displayText', '%s', %d, %d, %d, %d, %d,'%s',%d,'%s')\n",
           frames[i].displayCmd,
           i,
	   frames[i].x,
	   frames[i].y,
	   frames[i].w,
	   frames[i].h,
           frames[i].textColor,
           frames[i].fontSize,
           frames[i].textStyle);
           execString(cmd);
/*
           jframeFunc("open", i, frames[i].x, frames[i].y, frames[i].w, frames[i].h);
	   return;
*/
       }
    }
}

void closeText()
{
    int i;
    if(frames == NULL || maxframes < 1) return;

    for(i=2; i<maxframes; i++) {
	if(strcasecmp(frames[i].displayType, "text") == 0) {
	   frames[i].status = CLOSE;
	}
    }
    jframeFunc("closealltext", 0, 0, 0, 0, 0);
}

void deleteFrames()
{
    int i;
    if(frames == NULL || maxframes < 1) return;

    for(i=1; i<maxframes; i++) {
	  if(i != 1)
	      initFrameInfo(i);
         // else
         //     frames[i].status = CLOSE;
    }

    jframeFunc("closeall", 0, 0, 0, 0, 0);
}

void updateFrameG(int id, int x, int y, int w, int h)
{
    if(w == 0 || h == 0) return;
    if(!testFrameID(id)) return;

    frames[id].w = w;
    frames[id].h = h;
    frames[id].x = x;
    frames[id].y = y;
}

void initFrame(char *type, int id, int x, int y, int w, int h)
{
   if(!testFrameID(id)) return;

   initFrameInfo(id);

   strcpy(frames[id].displayType, type);
   if(P_getstring(CURRENT, "file", frames[id].fidfile, 1, MAXSTR)) 
	strcpy(frames[id].fidfile, "");

   updateFrameG(id, x, y, w, h);
}

void makeDefaultFrame()
{
   initFrame("graphics", 0, 0, 0, 0, 0);
}

void updateFrameGraphicsCmd(const char *value)
{
    if(value == NULL || strlen(value) <= 0) return;
if(debug)
fprintf(stderr, "updateFrameGraphicsCmd %s\n", value);
    if(strcmp(value,"clear") == 0) {
        clearGraphFunc();
        deleteText();
    }
    if(strcmp(value,"jexp") == 0 || 
	strcasecmp(value,"rt") == 0 ||
	strcmp(value,"clear") == 0) return;

    if(!testFrameID(currframe)) return; 
    if(strcasecmp(frames[currframe].displayType, "graphics") != 0) return;

    strcpy(frames[currframe].displayCmd, value);
}

void updateFrameTextCmd(const char *value)
{
    if(value == NULL || strlen(value) < 1) return;
    if(!testFrameID(currframe)) return; 
    if(strcasecmp(frames[currframe].displayType, "text") != 0) return;

    strcpy(frames[currframe].displayCmd, value);
}

int addFrame(const char *type, const char *cmd, int id, int x, int y, int w, int h)
{
   char str[MAXSTR];
   if(!testFrameID(id)) return 0;

if(debug)
fprintf(stderr, "addFrame %s %s %d %d %d %d %d\n", type, cmd, id, x, y, w, h);
   //initFrame(type, id, x, y, w, h);
    frames[id].saved = 0;
    frames[id].expID = expdir_to_expnum(curexpdir);
    frames[id].vpID = VnmrJViewId;
   strcpy(frames[id].displayType, type);
   if(P_getstring(CURRENT, "file", frames[id].fidfile, 1, MAXSTR)) 
	strcpy(frames[id].fidfile, "");
   updateFrameG(id, x, y, w, h);

   frames[id].status = OPEN;
   currframe = id;
   if(strcasecmp(type,"text") == 0) updateFrameTextCmd(cmd);
   else updateFrameGraphicsCmd(cmd);

   if(debug) printFrame(-1);

   if(strcmp(frames[id].displayType,"text") == 0) {
        sprintf(str, "mfaction('displayText', '%s', %d, %d, %d, %d, %d,'%s',%d,'%s')\n",
           frames[id].displayCmd,
           id,
	   frames[id].x,
	   frames[id].y,
	   frames[id].w,
	   frames[id].h,
           frames[id].textColor,
           frames[id].fontSize,
           frames[id].textStyle);
        execString(str);
   } else
     jframeFunc("open", id, x, y, w, h);

   return 1;
}

int
testFrameID(int id)
{
    if(frames == NULL || id < 1 || id >= maxframes)
         return 0;
    else return 1;
}

int
getFrameID()
{
   int id;
   if(frames == NULL || maxframes < 1) return 0;
   else if(currframe < maxframes && frames[currframe].status != CLOSE) 
	id = currframe;
   else id = 0;
   return id;
}

void
getFrameScalePars(double *sp, double *wp, double *scl, double *vp_off,
                  int *rev, int *color, int *vp, int *df, int *dn, int *df2,
                  int *axis, int *flag)
{
   int id;

   id = getFrameID();
   if (id == 0)
   {
      *sp = *wp = *scl = *vp_off = 1.0;
      *rev = *color = *vp = *df = *dn = *df2 = *axis = *flag = 0;
   }
   else
   {
      *sp = frames[id].dparams.scale_sp;
      *wp = frames[id].dparams.scale_wp;
      *scl = frames[id].dparams.scale_scl;
      *vp_off = frames[id].dparams.scale_vp_off;
      *rev = frames[id].dparams.scale_rev;
      *color = frames[id].dparams.scale_color;
      *vp = frames[id].dparams.scale_vp;
      *df = frames[id].dparams.scale_df;
      *dn = frames[id].dparams.scale_dn;
      *df2 = frames[id].dparams.scale_df2;
      *axis = frames[id].dparams.scale_axis;
      *flag = frames[id].dparams.scale_flag;
   }
}

void
setFrameScalePars(double sp, double wp, double scl, double vp_off,
                  int rev, int color, int vp, int df, int dn, int df2,
                  int axis, int flag)
{
   int id;

   id = getFrameID();
   if (id == 0)
   {
      return;
   }
   frames[id].dparams.scale_sp = sp;
   frames[id].dparams.scale_wp = wp;
   frames[id].dparams.scale_scl = scl;
   frames[id].dparams.scale_vp_off = vp_off;
   frames[id].dparams.scale_rev = rev;
   frames[id].dparams.scale_color = color;
   frames[id].dparams.scale_vp = vp;
   frames[id].dparams.scale_df = df;
   frames[id].dparams.scale_dn = dn;
   frames[id].dparams.scale_df2 = df2;
   frames[id].dparams.scale_axis = axis;
   frames[id].dparams.scale_flag = flag;
}

void savePars(int id) {
  
   char str[SHORTSTR];

   if(!testFrameID(id)) return;
 
   if(P_getreal(CURRENT, "cr", &(frames[id].params.cr), 1)) 
	frames[id].params.cr = cr;
   if(P_getreal(CURRENT, "cr1", &(frames[id].params.cr1), 1)) 
	frames[id].params.cr1 = 0;
   if(P_getreal(CURRENT, "delta", &(frames[id].params.delta), 1)) 
	frames[id].params.delta = delta;
   if(P_getreal(CURRENT, "delta1", &(frames[id].params.delta1), 1)) 
	frames[id].params.delta1 = 0;
   if(P_getreal(CURRENT, "sp", &(frames[id].params.sp), 1)) 
	frames[id].params.sp = sp;
   if(P_getreal(CURRENT, "wp", &(frames[id].params.wp), 1)) 
	frames[id].params.wp = wp;
   if(P_getreal(CURRENT, "sp1", &(frames[id].params.sp1), 1)) 
	frames[id].params.sp1 = sp1;
   if(P_getreal(CURRENT, "wp1", &(frames[id].params.wp1), 1)) 
	frames[id].params.wp1 = wp1;
   if(P_getreal(CURRENT, "sp2", &(frames[id].params.sp2), 1)) 
	frames[id].params.sp2 = sp1;
   if(P_getreal(CURRENT, "wp2", &(frames[id].params.wp2), 1)) 
	frames[id].params.wp2 = wp1;
   if(P_getreal(CURRENT, "sf", &(frames[id].params.sf), 1)) 
	frames[id].params.sf = 0;
   if(P_getreal(CURRENT, "wf", &(frames[id].params.wf), 1)) 
	frames[id].params.wf = 0;
   if(P_getreal(CURRENT, "sf1", &(frames[id].params.sf1), 1)) 
	frames[id].params.sf1 = 0;
   if(P_getreal(CURRENT, "wf1", &(frames[id].params.wf1), 1)) 
	frames[id].params.wf1 = 0;
   if(P_getreal(CURRENT, "sf2", &(frames[id].params.sf2), 1)) 
	frames[id].params.sf2 = 0;
   if(P_getreal(CURRENT, "wf2", &(frames[id].params.wf2), 1)) 
	frames[id].params.wf2 = 0;
   if(P_getreal(CURRENT, "sc", &(frames[id].params.sc), 1)) 
	frames[id].params.sc = sc;
   if(P_getreal(CURRENT, "wc", &(frames[id].params.wc), 1)) 
	frames[id].params.wc = wc;
   if(P_getreal(CURRENT, "sc2", &(frames[id].params.sc2), 1)) 
	frames[id].params.sc2 = sc2;
   if(P_getreal(CURRENT, "wc2", &(frames[id].params.wc2), 1)) 
	frames[id].params.wc2 = wc2;
   if(P_getreal(CURRENT, "vp", &(frames[id].params.vp), 1)) 
	frames[id].params.vp = vp;
   if(P_getreal(CURRENT, "ho", &(frames[id].params.ho), 1)) 
	frames[id].params.ho = ho;
   if(P_getreal(CURRENT, "vo", &(frames[id].params.vo), 1)) 
	frames[id].params.vo = vo;
   if(P_getreal(CURRENT, "vs", &(frames[id].params.vs), 1)) 
	frames[id].params.vs = vs;
   if(P_getreal(CURRENT, "vs2d", &(frames[id].params.vs2d), 1)) 
	frames[id].params.vs2d = vs2d;
   if(P_getreal(CURRENT, "vsproj", &(frames[id].params.vsproj), 1)) 
	frames[id].params.vsproj = vsproj;
   if(P_getreal(CURRENT, "th", &(frames[id].params.threshold), 1)) 
	frames[id].params.threshold = 3.0;
   if(P_getreal(CURRENT, "lifrq", &(frames[id].params.lifrq), 1)) 
	frames[id].params.lifrq = 0.0;
   if(P_getreal(CURRENT, "lvltlt", &(frames[id].params.lvltlt), 1)) 
	frames[id].params.lvltlt = 1.0;
   if(P_getreal(GLOBAL, "mfShowAxis", &(frames[id].params.showAxis), 1)) 
	frames[id].params.showAxis = -1;
   if(P_getreal(GLOBAL, "phasing", &(frames[id].params.phasing), 1)) 
	frames[id].params.phasing = -1;
   if(P_getreal(GLOBAL, "phasef", &(frames[id].params.phasef), 1)) 
	frames[id].params.phasef = -1;
   if(P_getreal(GLOBAL, "mfShowFields", &(frames[id].params.showFields), 1)) 
	frames[id].params.showFields = -1;
   if(P_getstring(CURRENT, "aig", frames[id].params.aig, 1, SHORTSTR)) 
	strcpy(frames[id].params.aig, "ai");
   if(P_getstring(GLOBAL, "crmode", frames[id].params.crmode, 1, SHORTSTR)) 
	strcpy(frames[id].params.crmode, "c");
   if(P_getstring(CURRENT, "trace", frames[id].params.trace, 1, SHORTSTR)) 
	strcpy(frames[id].params.trace, "f1");
   if(P_getstring(CURRENT, "intmod", frames[id].params.intmod, 1, SHORTSTR)) 
	strcpy(frames[id].params.intmod, "full");
   if(P_getstring(CURRENT, "axis", frames[id].params.axis, 1, SHORTSTR)) 
	strcpy(frames[id].params.axis, "p");
   frames[id].params.dfpnt = dfpnt;
   frames[id].params.dnpnt = dnpnt;
   frames[id].params.dfpnt2 = dfpnt2;
   frames[id].params.dnpnt2 = dnpnt2;
   frames[id].params.ds_threshold_flag = get_ds_threshold_flag();
   frames[id].params.ds_threshold_y = get_ds_threshold_loc();
   frames[id].params.ds_cursor_flag = get_ds_cursor_flag();

   frames[id].saved = 1;

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strstr(str, "con") != NULL) { 
	getAxes(frames[id].params.f2, frames[id].params.f1, 7);
    } else {
	getAxis(frames[id].params.f2, 7);
	strcpy(frames[id].params.f1, frames[id].params.f2);
    }

}

static void resetPars(int id, int sendToVj) {
   char str[STR64];
   int  k;
   if(!testFrameID(id)) return;
   if(!frames[id].saved) return;

   P_setreal(CURRENT, "cr", frames[id].params.cr, 1); 
   P_setreal(CURRENT, "cr1", frames[id].params.cr1, 1); 
   P_setreal(CURRENT, "delta", frames[id].params.delta, 1); 
   P_setreal(CURRENT, "delta1", frames[id].params.delta1, 1); 
   P_setreal(CURRENT, "sp", frames[id].params.sp, 1); 
   P_setreal(CURRENT, "wp", frames[id].params.wp, 1); 
   P_setreal(CURRENT, "sp1", frames[id].params.sp1, 1); 
   P_setreal(CURRENT, "wp1", frames[id].params.wp1, 1); 
   P_setreal(CURRENT, "sp2", frames[id].params.sp2, 1); 
   P_setreal(CURRENT, "wp2", frames[id].params.wp2, 1); 
   P_setreal(CURRENT, "sf", frames[id].params.sf, 1); 
   P_setreal(CURRENT, "wf", frames[id].params.wf, 1); 
   P_setreal(CURRENT, "sf1", frames[id].params.sf1, 1); 
   P_setreal(CURRENT, "wf1", frames[id].params.wf1, 1); 
   P_setreal(CURRENT, "sf2", frames[id].params.sf2, 1); 
   P_setreal(CURRENT, "wf2", frames[id].params.wf2, 1); 
   P_setreal(CURRENT, "sc", frames[id].params.sc, 1); 
   P_setreal(CURRENT, "wc", frames[id].params.wc, 1); 
   P_setreal(CURRENT, "sc2", frames[id].params.sc2, 1); 
   P_setreal(CURRENT, "wc2", frames[id].params.wc2, 1); 
   P_setreal(CURRENT, "vp", frames[id].params.vp, 1); 
   P_setreal(CURRENT, "ho", frames[id].params.ho, 1); 
   P_setreal(CURRENT, "vo", frames[id].params.vo, 1); 
   P_setreal(CURRENT, "vs", frames[id].params.vs, 1); 
   P_setreal(CURRENT, "vs2d", frames[id].params.vs2d, 1); 
   P_setreal(CURRENT, "vsproj", frames[id].params.vsproj, 1); 
   P_setreal(CURRENT, "th", frames[id].params.threshold, 1); 
   P_setreal(CURRENT, "lifrq", frames[id].params.lifrq, 1); 
   P_setreal(CURRENT, "lvltlt", frames[id].params.lvltlt, 1); 
   P_setstring(CURRENT, "aig", frames[id].params.aig, 1); 
   P_setstring(GLOBAL, "crmode", frames[id].params.crmode, 1); 
   P_setstring(CURRENT, "trace", frames[id].params.trace, 1); 
   P_setstring(CURRENT, "intmod", frames[id].params.intmod, 1); 
   P_setstring(CURRENT, "axis", frames[id].params.axis, 1); 
   if (frames[id].params.phasing >= 0)
        P_setreal(GLOBAL, "phasing", frames[id].params.phasing, 1); 
   if (frames[id].params.phasef >= 0)
        P_setreal(GLOBAL, "phasef", frames[id].params.phasef, 1); 
   dfpnt = frames[id].params.dfpnt;
   dnpnt = frames[id].params.dnpnt;
   dfpnt2 = frames[id].params.dfpnt2;
   dnpnt2 = frames[id].params.dnpnt2;
   set_ds_threshold_flag(frames[id].params.ds_threshold_flag);
   set_ds_threshold_loc(frames[id].params.ds_threshold_y);
   set_ds_cursor_flag(frames[id].params.ds_cursor_flag);
   k = 0;

   if(strcmp(frames[id].displayType, "text") == 0) {
     P_setstring(GLOBAL, "mfText", frames[id].displayCmd, 1); 
     P_setstring(GLOBAL, "mfTextColor", frames[id].textColor, 1); 
     P_setstring(GLOBAL, "mfTextStyle", frames[id].textStyle, 1); 
     P_setreal(GLOBAL, "mfTextFontSize", (double)frames[id].fontSize, 1); 
   } else { 
     // Wsetgraphicsdisplay(frames[id].displayCmd);
     P_setstring(GLOBAL, "mfText", "", 1); 
     strcpy(str, "");
     if (frames[id].params.showAxis >= 0) {
        P_setreal(GLOBAL, "mfShowAxis", frames[id].params.showAxis, 1); 
        strcat(str, "mfShowAxis ");
        k++;
     }
     if (id == 1 && frames[id].params.showFields >= 0) {
        P_setreal(GLOBAL, "mfShowFields", frames[id].params.showFields, 1); 
        k++;
        strcat(str, "mfShowFields");
     }
   }
    if (sendToVj == 0)
       return;
    if (k > 0)
       appendJvarlist(str);
    /***
    if (k == 1)
	writelineToVnmrJ("pnew 1",str);	
    else if (k == 2)
	writelineToVnmrJ("pnew 2",str);	
    ***/

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strstr(str, "con") != NULL) { 
	getAxes(frames[id].params.f2, frames[id].params.f1, 7);
    } else {
	getAxis(frames[id].params.f2, 7);
	strcpy(frames[id].params.f1, frames[id].params.f2);
    }
}

void retrievePars(int id) {
     resetPars(id, 1);
}

int
getNextFrameID()
{
    int i, id;
    if(frames == NULL) {
	maxframes = 10;
        frames=(frameInfo *)calloc(sizeof(frameInfo),maxframes);
    	for(i=0; i<maxframes; i++) {
	     initFrameInfo(i);
	}
	makeDefaultFrame();
    }

    id = -1;
    for(i=1; i<maxframes; i++) 
	if(frames[i].status == CLOSE) {
 	    id = i;	
	    break;
        }

    if(id < 0) {
	  id = maxframes;
	  maxframes += 10;
          frames = (frameInfo *) realloc(frames, sizeof(frameInfo)*maxframes); 
    	  for(i = id; i<maxframes; i++) {
	     initFrameInfo(i);
	  }
    }
    savePars(id);
    return id;
}

void redisplayFrame(int id)
{
    char cmd[MAXSTR];
    if(!testFrameID(id)) return;

    strcpy(cmd, frames[id].displayCmd);
if(debug)
fprintf(stderr,"redisplayFrame0 %d %s\n", id, cmd);

    if(strcasecmp(frames[id].displayType, "graphics") == 0 &&
	strlen(cmd) > 0 && isReexecCmd(cmd)) {

	  if(strstr(cmd,"con") != NULL) {
             sprintf(cmd, "full dconi('again')\n");
	  } else if(strcmp(frames[id].displayCmd,"ds") == 0 || 
		strcmp(frames[id].displayCmd,"dfid") == 0 ||
		strcmp(frames[id].displayCmd,"df") == 0) {
             sprintf(cmd, "full %s('again')\n", frames[id].displayCmd);
	  } else {
             sprintf(cmd, "%s\n", frames[id].displayCmd);
	  }
          execString(cmd);
if(debug)
fprintf(stderr,"redisplayFrame1 %d %s\n", id, cmd);
    } else if(strcasecmp(frames[id].displayType, "text") == 0) { 
/*
        sprintf(cmd, "mfaction('displayText', '%s','%s',%d,'%s')\n", 
	   frames[id].displayCmd,
	   frames[id].textColor,
	   frames[id].fontSize,
	   frames[id].textStyle);
*/
/*
        iprintf(cmd, "mfaction('updateText', '%s')\n", 
	   frames[id].displayCmd);
        execString(cmd);
*/
        P_setstring(GLOBAL, "mfText", frames[id].displayCmd, 1); 
        appendJvarlist("mfText");
	// writelineToVnmrJ("pnew 1","mfText");	
if(debug)
fprintf(stderr,"redisplayFrame2 %d %s\n", id, cmd);
    }

}

int selectTextFrameByID(int id)
{
    char cmd[MAXSTR];

    if(!testFrameID(id)) return 0;
    savePars(currframe);

    if (currframe != id) {
        currframe = id;
        retrievePars(currframe);	
    }

    sprintf(cmd, "mfaction('updateText', '%s')\n", 
	   frames[id].displayCmd);
    execString(cmd);
    //redisplayFrame(id);
    return 1;
}

int selectDisplayFrameByID(int id)
{
    // char cmd[MAXSTR];

    if(!testFrameID(id)) return 0;
    if(frames[id].status == CLOSE) frames[id].status = OPEN;
    //if(id == currframe) return 0;

    savePars(currframe);
    if(frames[id].expID != frames[currframe].expID) {
        /* set currframe before jexp! */
	// sprintf(cmd, "jexp%d\n", frames[id].expID);
	// execString(cmd);
    }

    if (currframe != id) {
        currframe = id;
        retrievePars(currframe);	
    }

    //redisplayFrame(id);
    return 1;
}

int selectFrameByID(int id)
{
if(debug)
Winfoprintf("selectFrameByID %d %d\n", id, currframe);
    if(!testFrameID(id)) return 0;

    if(strcmp(frames[id].displayType, "text") == 0) 
	return selectTextFrameByID(id);
    else
	return selectDisplayFrameByID(id);
}    

void
insetFrame(int x0, int y0, int x1, int y1)
{
    writelineToVnmrJ(VJCMD, "canvas rubberband");
}

void
zoomFrame(int x0, int y0, int x1, int y1)
{
    char str[SHORTSTR];
    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strstr(str, "con") != NULL) { 
	writelineToVnmrJ(VJCMD, "canvas rubberbox");
    } else {
	writelineToVnmrJ(VJCMD, "canvas rubberarea");
    }
}

void
finishZoomFrame(int x0, int y0, int x1, int y1)
{
    char str[SHORTSTR];
    int swap, y;

//if from right to left, then zoom to full spectrum.
//    if(x0 > x1) { 
//	mfZoom(0);	
//        return;
//    }

 	if(x0 > x1) SWAP(x0, x1);
 	if(y0 > y1) SWAP(y0, y1);
    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strcasecmp(str, "df") == 0) { 
	if(x0 < dfpnt || x0 > dfpnt+dnpnt ||
	   x1 < dfpnt || x1 > dfpnt+dnpnt ||
	   (x0+2) >=  x1) return;
	df_setCursor(x0, x1);
	df_zoom(BOX_MODE);
    } else if(strcasecmp(str, "ds") == 0) { 
    	if(getOverlayMode() == OVERLAID_ALIGNED && getChartMode() == ALIGN_1D_Y) {
	   // swab x, y, and x0, x1 for side spectrum
	   x1 = mnumxpnts - y0;
	   x0 = mnumxpnts - y1;
	} 
	if(x0 < dfpnt || x0 > dfpnt+dnpnt ||
	   x1 < dfpnt || x1 > dfpnt+dnpnt ||
	   (x0+2) >=  x1) return;
	ds_setCursor(x0, x1);
	ds_zoom(BOX_MODE);
    } else if(strstr(str,"con") != NULL) {
	if(x0 < dfpnt || x0 > dfpnt+dnpnt ||
	   x1 < dfpnt || x1 > dfpnt+dnpnt) return;
	y = mnumypnts - y0 -1;
	if(y < dfpnt2 || y > dfpnt2+dnpnt2) return;
	y = mnumypnts - y1 -1;
	if(y < dfpnt2 || y > dfpnt2+dnpnt2) return;
	if((x0+2) >=  x1 || (y0+2) >=  y1) return;
	dconi_setCursor(x0, y0, x1, y1);
	dconi_zoom(BOX_MODE);
    } else if (strlen(str)>2 && (strncmp(str, "ds", 2) == 0)) { // arrayed spec 
        // set sp, wp
	double dss_wc,dss_sc;
        if(P_getreal(CURRENT,"dss_wc",&dss_wc,1)) dss_wc = wc;
        if(P_getreal(CURRENT,"dss_sc",&dss_sc,1)) dss_sc = sc;
        if(dss_wc != wc || dss_sc != sc) {
          dnpnt = (int)((double)(mnumxpnts-right_edge)*dss_wc/wcmax);
          dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-dss_wc-dss_sc)/wcmax);
	}
	if(x0 < dfpnt || x0 > dfpnt+dnpnt ||
	   x1 < dfpnt || x1 > dfpnt+dnpnt ||
	   (x0+2) >=  x1) return;

         if (get_axis_freq(HORIZ) == 0) {
	    sp = (x1 - dfpnt ) * wp  / dnpnt  + sp; 
	    wp = (x0 - x1 ) * wp  / dnpnt; 
         } else {
            sp = - (x1 - dfpnt ) * wp  / dnpnt  + wp + sp;
	    wp = (x1 - x0 ) * wp  / dnpnt; 
         }
	 P_setreal(CURRENT, "sp", sp, 1);
         P_setreal(CURRENT, "wp", wp, 1);
	 execString("showarrays('redisplay')\n"); 
    } else if (strlen(str)>2 && (strncmp(str, "df", 2) == 0)) { // arrayed fids 
        // set sf, wf
	double dss_wc,dss_sc;
        if(P_getreal(CURRENT,"dss_wc",&dss_wc,1)) dss_wc = wc;
        if(P_getreal(CURRENT,"dss_sc",&dss_sc,1)) dss_sc = sc;
        if(dss_wc != wc || dss_sc != sc) {
          dnpnt = (int)((double)(mnumxpnts-right_edge)*dss_wc/wcmax);
          dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-dss_wc-dss_sc)/wcmax);
	}
	if(x0 < dfpnt || x0 > dfpnt+dnpnt ||
	   x1 < dfpnt || x1 > dfpnt+dnpnt ||
	   (x0+2) >=  x1) return;

	 sp = (x0 - dfpnt ) * wp  / dnpnt  + sp; 
	 wp = (x1 - x0 ) * wp  / dnpnt; 
	 P_setreal(CURRENT, "sf", sp, 1);
         P_setreal(CURRENT, "wf", wp, 1);
	 execString("showarrays('redisplay')\n"); 
    }

    execString("mfaction('finishZoomFrame')\n");
}

void
finishInsetFrame(int x0, int y0, int x1, int y1)
{
    char str[SHORTSTR];
    int swap;
    int oldId;
    int newid = getNextFrameID();

    if(!testFrameID(newid)) return;

if(debug)
fprintf(stderr, "finishInsetFrame0 %d %d %d %d\n", x0, y0, x1, y1);
 	if(x0 > x1) SWAP(x0, x1);
 	if(y0 > y1) SWAP(y0, y1);
	if((x0+2) >=  x1) return;
	if((y0+2) >=  y1) return;

    oldId = currframe;
    savePars(currframe);        

if(debug)
fprintf(stderr, "finishInsetFrame1 %d %d %d %d\n", x0, y0, x1, y1);
    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strcasecmp(str, "df") == 0) 
	df_setCursor(x0, x1);
    else if(strcasecmp(str, "ds") == 0) 
	ds_setCursor(x0, x1);
    else if(strstr(str,"con") != NULL) 
	dconi_setInsetCursor(x0, y0, x1, y1);
    
    if(!addFrame("graphics", str, newid, x0, y0, x1-x0, y1-y0)) return;

/*
    copy_inset_frame_cmd(newid, oldId);
    set_jframe_id(newid);
    sunGraphClear();
*/

    if(strcasecmp(str, "df") == 0) 
        df_zoom(BOX_MODE);
    else if(strcasecmp(str, "ds") == 0) 
        ds_zoom(BOX_MODE);
    else if(strstr(str,"con") != NULL)  
	dconi_zoom(BOX_MODE);

    execString("mfaction('finishInsetFrame')\n");
}

int getTextFrame(char *type, char *cmd)
{
   int i;
   if(frames == NULL || maxframes < 2) return -1;
   if(strcasecmp(type, "text") != 0) return -1;

   for(i=2; i<maxframes; i++) {
	if(strcmp(frames[i].displayType, type) == 0 &&
	  strcmp(frames[i].displayCmd, cmd) == 0) return i;
   } 
   return -1;
}

int frameAction( argc, argv, retc, retv )
int argc;
char *argv[];
int retc;
char *retv[];
{
   int mode, id, x0, y0, x1, y1;
   char cmd[MAXSTR];

   if(isImaging() > 1) RETURN;

   if(argc < 2) {
	Werrprintf("Usage: frameAction(cmd, id, x0, y0, wd, ht)\n");
	RETURN;
   }

   if(strcasecmp(argv[1], "fullsize") == 0) {
        if(argc > 2) id = atoi(argv[2]);
	else id = getFrameID();
        if(!testFrameID(id)) RETURN;
        jframeFunc("open", id, 0, 0, 0, 0);         
	RETURN;
   }

   if(strcasecmp(argv[1], "overlaySpec") == 0) {
	if(argc > 2) overlaySpec(atoi(argv[2]));
	RETURN;
   }

   if(strcasecmp(argv[1], "print") == 0) {
        printFrame(-1);
	RETURN;
   }

   if(strcasecmp(argv[1], "getFrameCount") == 0 && retc > 0) {
	retv[0] = realString((double)getNumberFrames()); 
	RETURN;
   }

   if(strcasecmp(argv[1], "select") == 0 && argc > 2) {
        if(frames == NULL || maxframes < 1 ) RETURN;
	id  = atoi(argv[2]);
        if(!testFrameID(id)) RETURN;
        jframeFunc("open", id, frames[id].x, frames[id].y, frames[id].w, frames[id].y);
	RETURN;
   }

   if(strcasecmp(argv[1], "getid") == 0 && retc > 0) {
	retv[0] = realString((double)getFrameID()); 
	RETURN;
   }

   if(strcasecmp(argv[1], "endrubberband") == 0 && argc > 6) {
	id = atoi(argv[2]);
	if(id > 0 && id != currframe) RETURN;
 
	x0 = atoi(argv[3]);
	y0 = atoi(argv[4]);
	x1 = atoi(argv[5]);
	y1 = atoi(argv[6]);

        mode = getButtonMode();
        if(getAspMouse()) {
	    aspFrame("enddrag", id, x1, y1, x0, y0); // x0,y0 are start, x1,y1 are end
	} else if(mode == ZOOM_MODE)
	    finishZoomFrame(x0, y0, x1, y1);
	else if(mode == INSET_MODE)
	    finishInsetFrame(x0, y0, x1, y1);

	RETURN;
   } 

   if(strcasecmp(argv[1], "getWinSize") == 0) {
	if(retc < 2) RETURN;

	if(frames == NULL || maxframes < 1) {
	   retv[0] = realString((double)(mnumxpnts - right_edge));
	   retv[1] = realString((double)mnumypnts);
	} else {
	   retv[0] = realString((double)frames[1].w);
	   retv[1] = realString((double)frames[1].h);
	}
	RETURN;
   }

   if(strcasecmp(argv[1], "getFrameG") == 0) {
	if(retc < 2) RETURN;
        if(argc > 2) id = atoi(argv[2]);
	else id = getFrameID();
        if(id < 0 || !testFrameID(id)) RETURN;

	if(retc > 3) {
	   retv[0] = realString((double)frames[id].x);
	   retv[1] = realString((double)frames[id].y);
	   retv[2] = realString((double)frames[id].w);
	   retv[3] = realString((double)frames[id].h);
	} else if(retc > 1) {
	   retv[0] = realString((double)frames[id].w);
	   retv[1] = realString((double)frames[id].h);
	}
	RETURN;
   }

   if(strcasecmp(argv[1], "setcmd") == 0 && argc > 4) {

	id = atoi(argv[2]);
	if(!testFrameID(id)) RETURN;
	
	if(strcasecmp(frames[id].displayType, argv[3]) == 0) {
           strcpy(frames[id].displayCmd, argv[4]);
	} 
	RETURN;
   }

   if(strcasecmp(argv[1], "newId") == 0) {
        id = getNextFrameID();
	if(retc > 0) retv[0] = realString((double)id);
	RETURN;
   }

   if(strcasecmp(argv[1], "show") == 0) {
	   
	if(argc > 3) {
	   id = getTextFrame(argv[2], argv[3]);
	   if(id < 0) {
        	id = getNextFrameID();
        	if(!testFrameID(id)) RETURN;

		x0 = 0;
		y0 = 0;
		x1 = 0.5*frames[1].w;
		y1 = 0.5*frames[1].h;
		if(argc > 7) {
	   	    x0 = atoi(argv[4]);
	   	    y0 = atoi(argv[5]);
	   	    x1 = atoi(argv[6]);
	   	    y1 = atoi(argv[7]);
		} 
		if(x1 <= 0) x1=0.5*frames[1].w;
		if(y1 <= 0) y1=0.5*frames[1].h;

		if(argc > 10) {
		   strcpy(frames[id].textColor, argv[8]);
		   frames[id].fontSize = atoi(argv[9]);
		   strcpy(frames[id].textStyle, argv[10]);
		}
           	addFrame(argv[2], argv[3], id, x0, y0, x1, y1);
		RETURN;
	   }
	} else RETURN;

        if(!testFrameID(id)) RETURN;

	x0 = frames[id].x; 
	y0 = frames[id].y;
	x1 = frames[id].w;
	y1 = frames[id].h;
	if(argc > 7) {
	   x1 = atoi(argv[6]);
	   y1 = atoi(argv[7]);
 	}
	if(x1 <= 0) x1 = frames[id].w;
	if(y1 <= 0) y1 = frames[id].h;

	if(argc > 10) {
	   strcpy(frames[id].textColor, argv[8]);
	   frames[id].fontSize = atoi(argv[9]);
	   strcpy(frames[id].textStyle, argv[10]);
	}
	frames[id].status = OPEN;
   	//jframeFunc("open", id, x0, y0, x1, y1);
        sprintf(cmd, "mfaction('displayText', '%s', %d, %d, %d, %d, %d,'%s',%d,'%s')\n",
           frames[id].displayCmd,
           id,
	   frames[id].x,
	   frames[id].y,
	   frames[id].w,
	   frames[id].h,
           frames[id].textColor,
           frames[id].fontSize,
           frames[id].textStyle);
           execString(cmd);
	RETURN;
   }

   if(strcasecmp(argv[1], "new") == 0) {
        id = getNextFrameID();
        if(!testFrameID(id)) RETURN;

	if(argc > 7) {
	   x0 = atoi(argv[4]);
	   y0 = atoi(argv[5]);
	   x1 = atoi(argv[6]);
	   y1 = atoi(argv[7]);
           addFrame(argv[2], argv[3], id, x0, y0, x1, y1);
	} else if(argc > 6) {
	   x0 = atoi(argv[3]);
	   y0 = atoi(argv[4]);
	   x1 = atoi(argv[5]);
	   y1 = atoi(argv[6]);
           addFrame(argv[2], "", id, x0, y0, x1, y1);
	} else if(argc > 2) {
           addFrame(argv[2], "", id, 0, 0, 0.5*frames[0].w, 0.5*frames[0].h);
	} else {
           addFrame("graphics", "", id, 0, 0, 0.5*frames[0].w, 0.5*frames[0].h);
	}

	RETURN;
   }

   if(strcasecmp(argv[1], "delete") == 0 && argc > 2) {
        id = atoi(argv[2]);
	if(id < 1) deleteFrames();
	else deleteFrame(id);
	RETURN;
   }  

   if(strcasecmp(argv[1], "deleteSel") == 0) {
	id = getFrameID();
	if(id < 1) deleteFrames();
	else deleteFrame(id);
	RETURN;
   }

   if(strcasecmp(argv[1], "deleteAll") == 0) {
	deleteFrames();
	RETURN;
   }

   if(strcasecmp(argv[1], "deleteText") == 0) {
	deleteText();
	RETURN;
   }

   if(strcasecmp(argv[1], "closeText") == 0) {
	closeText();
	RETURN;
   }

   if(strcasecmp(argv[1], "endDrawText") == 0) {
// show other closed text
	showText();
	RETURN;
   }

   if(strcasecmp(argv[1], "showText") == 0) {
	showText();
	RETURN;
   }

   if(strcasecmp(argv[1], "showTextLayout") == 0 && argc > 2) {
	showTextLayout(argv[2]);
	RETURN;
   }

   if(strcasecmp(argv[1], "saveTextLayout") == 0 && argc > 2) {
	saveTextLayout(argv[2]);
	RETURN;
   }

   if(strcasecmp(argv[1], "mfZoom") == 0 && argc > 2) {
	mfZoom(atoi(argv[2]));
	RETURN;
   }

   RETURN;
}

void updateFrameExp(int n)
{
     char cmd[MAXSTR];
     if(!testFrameID(currframe)) return; 

     initZoomInfo(currframe);

     sprintf(cmd, "mfaction('jexp',%d, %d)\n", n, currframe);
     execString(cmd);

     frames[currframe].expID = n;
}

void updateFrameRT(char *path)
{
     char cmd[MAXSTR];
     if(!testFrameID(currframe)) return; 

     initZoomInfo(currframe);

     //if(currframe == 1) return; 

     sprintf(cmd, "mfaction('rt','%s',%d, %d, %d)\n", path, currframe, 
	frames[currframe].expID, frames[1].expID);
     execString(cmd);

     strcpy(frames[currframe].fidfile, path);
}

static int isImaging()
{
return 0;
/*
    int b;
    char str[MAXSTR];

    if(getUseXFunc()) return 0;

    if(P_getstring( GLOBAL, "appmode", str, 1, MAXSTR )) strcpy(str,"");
    if(strcasecmp(str, "imaging") == 0) b = IMAGING;
    else if(strcasecmp(str, "walkup") == 0) b = WALKUP;
    else b = STANDARD;

    if(b == IMAGING && aipOwnsScreen()) return 2;
    else if(b == IMAGING) return 1;
    else return 0;
*/
}

int okExec(char *str)
{
   if(!testFrameID(currframe)) return 1;

   if(strcasecmp(frames[currframe].displayType, "text") == 0) {
        if(strstr(str, "jMove") == str ||
	   strstr(str, "jFunc") == str ||
	   strstr(str, VJCMD) == str ||
	   strstr(str, "mf") == str ||
	   strstr(str, "frameAction") == str ||
	   strstr(str, "isimagebrowser") == str ) { 
	  
	   return 1;
	} else {
           jframeFunc("open", 1, frames[1].x, frames[1].y, frames[1].w, frames[1].h);
	   return 0;
	}
   }
   else return 1;
}

void frame_update(char *cmd, char *value) {
	int id=1;
	if (isImaging() > 1)
		return;

	if (debug)
		fprintf(stderr, "frame_update %s %s %d\n", cmd, value, currframe);

	if (strcasecmp(cmd, "init") == 0) {
		id = getNextFrameID();
		if (id == 1 && isImaging() == 0) {
			addFrame("graphics", "", 1, 0, 0, 0, 0);
		} else {
			currframe = id;
		}
		execString("mfaction('init')\n");
	} else if (strcasecmp(cmd, "jexp") == 0) {
		updateFrameExp(atoi(value));
	} else if (strcasecmp(cmd, "rt") == 0) {
		updateFrameRT(value);
	} else if (strcasecmp(cmd, "graphics") == 0) {
		updateFrameGraphicsCmd(value);
	} else if (strcasecmp(cmd, "text") == 0) {
		updateFrameTextCmd(value);
	} else if (strcasecmp(cmd, "exit") == 0) {
		if (currframe != 1) {
			resetPars(1, 0);
			/***
			 currframe = -1;
			 sprintf(cmd, "jexp%d\n", frames[1].expID);
			 execString(cmd);
			 *****/
		}
	}
}

void frame_updateG(char *cmd, int id, int x, int y, int w, int h) 
{
    if(isImaging() > 0) return;
    if(!testFrameID(id)) return;

    if(frames == NULL || maxframes < 1) return;
   
    if(strcasecmp(cmd, "wsize") == 0) {
	frames[0].w = w; 
	frames[0].h = h; 
if(debug)
Winfoprintf("frame_updateG %d %d %d %d\n", w, h, frames[0].w, frames[0].h);
    }
if(debug)
fprintf(stderr,"frame_updateG %s %d %d %d %d %d\n", cmd, id, x, y, w, h);
    if(!testFrameID(id)) return; 

    if(strcasecmp(cmd, "select") == 0) {
	updateFrameG(id, x, y, w, h);
        selectFrameByID(id);
	//redisplayFrame(id);
    } else if (strcasecmp(cmd, "loc") == 0) {
	updateFrameG(id, x, y, w, h);
    } else if (strcasecmp(cmd, "resize") == 0) {
	updateFrameG(id, x, y, w, h);
	//redisplayFrame(id);
    }
}

int
getSelectedFrameID(int x, int y)
{
/* get frame id of the mouse. If overlapped, get the smallest frame */ 
    int i, minsize, size, id, x0, y0, x1, y1;

    if(frames == NULL || maxframes < 1) return 0;

    minsize = frames[0].w * frames[0].h;
    
    if(!testFrameID(currframe)) return 0; 

    x += frames[currframe].x;
    y += frames[currframe].y;

    id = 0;
    for(i=1; i<maxframes; i++) {
      if(frames[i].status == CLOSE) continue;
      x0 = frames[i].x;
      y0 = frames[i].y;
      x1 = x0 + frames[i].w;
      y1 = y0 + frames[i].h;
      size = frames[i].w * frames[i].h;
      if(x >= x0 && x <= x1 && y >= y0 && y <= y1) {
        if(size <= minsize) {
           minsize = size;
           id = i;
        }
      }
    }
    return id;
}

int isCrosshairOk(int x, int y) 
{
    int id;
    if(frames == NULL || maxframes < 1) return 1;
    if(getNumberFrames() <= 1) return 1;

    id = getSelectedFrameID(x, y);
    if(id < 1) return 0;

    if(id == currframe && 
	strcmp(frames[id].displayType, "graphics") == 0) return 1;
    else return 0; 
}

static int saveTextLayout(char *path)
{
   FILE *fp = NULL;
   int i, k = 0;
   float x, y;

   if(frames == NULL || maxframes < 1) return k;
   
   if((fp = fopen(path, "w")) != NULL) {
	fprintf(fp, "<template>\n");
	for(i=1; i<maxframes; i++) {
	   if(frames[i].status == CLOSE || strcmp(frames[i].displayType, "text") != 0) continue;

	   x = frames[i].x;
	   if(frames[0].w > 0) x /= frames[0].w;
	   y = frames[i].y;
	   if(frames[0].h > 0) y /= frames[0].h;

	   fprintf(fp, "   <textfile name=\"%s\" x=\"%f\" y=\"%f\" w=\"%d\" h=\"%d\" color=\"%s\" size=\"%d\" style=\"%s\"/>\n",
	   frames[i].displayCmd, x, y, frames[i].w, frames[i].h,
	   frames[i].textColor, frames[i].fontSize, frames[i].textStyle); 
	   k++;
	}
	fprintf(fp, "</template>\n");
        fclose(fp);
   }

   return k;
}

static int showTextLayout(char *path)
{
   FILE *fp = NULL;
   char buf[MAXSTR], str[MAXSTR], cmd[MAXSTR];
   char *sptr;
   int i, id, k = 0;
   float x, y;
   
   if(frames == NULL || maxframes < 1) return k;

   if((fp = fopen(path, "r")) != NULL) {
	deleteText();
	while (fgets(buf,sizeof(buf),fp)) {
	   if((sptr = strstr(buf,"<textfile")) == NULL) continue;

    	   id = getNextFrameID();
	   if(!testFrameID(id)) return k;

	   frames[id].status = OPEN;
	   strcpy(frames[id].displayType, "text");
	   if((sptr = strstr(buf,"name=\"")) != NULL) {
		sptr += 6;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		strcpy(frames[id].displayCmd, str);
	   }
	   if((sptr = strstr(buf,"x=\"")) != NULL) {
		sptr += 3;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		x = atof(str);
	        if(frames[0].w > 0) frames[id].x = (int)(x*frames[0].w);
		else frames[id].x = (int)x;
	   }
	   if((sptr = strstr(buf,"y=\"")) != NULL) {
		sptr += 3;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		y = atof(str);
	        if(frames[0].h > 0) frames[id].y = (int)(y*frames[0].h);
		else frames[id].y = (int)y;
	   }
	   if((sptr = strstr(buf,"w=\"")) != NULL) {
		sptr += 3;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		frames[id].w = atoi(str);
	   }
	   if((sptr = strstr(buf,"h=\"")) != NULL) {
		sptr += 3;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		frames[id].h = atoi(str);
	   }
	   if((sptr = strstr(buf,"color=\"")) != NULL) {
		sptr += 7;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		strcpy(frames[id].textColor, str);
	   }
	   if((sptr = strstr(buf,"size=\"")) != NULL) {
		sptr += 6;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		frames[id].fontSize = atoi(str);
	   }
	   if((sptr = strstr(buf,"style=\"")) != NULL) {
		sptr += 7;  
		strcpy(str, sptr);
		i = 0;
		while(str[i] != '\"') i++;
		str[i]='\0';
		strcpy(frames[id].textStyle, str);
	   }
//           jframeFunc("open", id, frames[id].x, frames[id].y, frames[id].w, frames[id].h);
           sprintf(cmd, "mfaction('displayText', '%s', %d, %d, %d, %d, %d,'%s',%d,'%s')\n",
           frames[id].displayCmd,
           id,
	   frames[id].x,
	   frames[id].y,
	   frames[id].w,
	   frames[id].h,
           frames[id].textColor,
           frames[id].fontSize,
           frames[id].textStyle);
           execString(cmd);
	   k++;
	}
        
	// close the frames showText will open them again.
/*
	for(k=0; k<maxframes; k++) {
	   if(strcmp(frames[k].displayType, "text") == 0) frames[k].status = CLOSE;
	}

	showText();
*/
        fclose(fp);
   }
   return k;
}

// check whether new zoom encompassing zoomList[i]
int zoomEncompassing(char *cmd, double c2, double d2, double c1, double d1, zoomInfo zoom)
{
   double ep=0, epsi=0;
   int b;
   if(strstr(cmd,"con") != NULL) 
      b = (c2-zoom.cr)>=ep && (d2-zoom.delta)>=ep
        && (c1-zoom.cr1)>=ep && (d1-zoom.delta1)>=ep;
   else if(strstr(cmd,"df") != NULL)
      b = (zoom.cr-c2)>=0 && (d2-zoom.delta)>=epsi;
   else b = (c2-zoom.cr)>=ep && (d2-zoom.delta)>=ep;

   return b; 
}

// check whether new zoom is encompassed by zoomList[i]
int zoomEncompassed(char *cmd, double c2, double d2, double c1, double d1, zoomInfo zoom)
{
   double ep=0, epsi=0;
   int b;
   if(strstr(cmd,"con") != NULL) 
      b = (zoom.cr-c2)>=ep && (zoom.delta-d2)>=ep 
        && (zoom.cr1-c1)>=ep && (zoom.delta1-d1)>=ep;
   else if(strstr(cmd,"df") != NULL)
      b = (c2-zoom.cr)>=0 && (zoom.delta-d2)>=epsi;
   else b = (zoom.cr-c2)>=ep && (zoom.delta-d2)>=ep;
   return b; 
}

// check whether new zoom is the same as zoomList[i]
int zoomEqual(char *cmd, double c2, double d2, double c1, double d1, zoomInfo zoom)
{
   double ep2=1, ep=1, epsi=1e-3;
   int b;
   if(strstr(cmd,"con") != NULL) 
     b = fabs(zoom.cr-c2)<ep2 && fabs(zoom.delta-d2)<ep2 
        && fabs(zoom.cr1-c1)<ep2 && fabs(zoom.delta1-d1)<ep2;
   else if(strstr(cmd,"df") != NULL) 
     b = fabs(zoom.cr-c2)<epsi && fabs(zoom.delta-d2)<epsi;
   else b = fabs(zoom.cr-c2)<ep && fabs(zoom.delta-d2)<ep; 
   return b;
}

// this is called by ds, dconi or dfid "expand" to add a new zoom to zoomList. 
// we make sure that zoomList[i] encompasses zoomList[i+1].
// Note, c2, d2, are for direct dimension and c1, d1 are for indirect dimension.
void addNewZoom(char *cmd, double c2, double d2, double c1, double d1)
{
// get current frame
    frameInfo *f;
    int i, j, b;

    if(!testFrameID(currframe)) return;

    f = &(frames[currframe]);
    if(f == NULL) return;
 
//Winfoprintf("new zoom %s %d %d %f %f %f %f\n",cmd,f->maxzooms, f->currzoom, c2,d2,c1,d1); 
    // init zoomList, or clear existing zoomList if cmd changed.
    if(f->zoomList ==NULL || strcmp(f->cmd, cmd) != 0) {
        f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)); 
	f->maxzooms = 1;
    	f->currzoom=0;
   	f->zoomList[0].cr = c2;
    	f->zoomList[0].delta = d2;
    	f->zoomList[0].cr1 = c1;
    	f->zoomList[0].delta1 = d1;
        strcpy(f->cmd,cmd);
	return;
    }
    strcpy(f->cmd,cmd);

    int nzooms = f->maxzooms;
    // see if the new zoom already exists in zoomList.
    for(i=0; i<nzooms; i++) {
      b = zoomEqual(cmd, c2,d2,c1,d1, f->zoomList[i]);
      if(b) {
    	 f->currzoom=i;
//for (j=0;j<nzooms; j++)
//Winfoprintf("zoom exist %d %d %d %f %f %f %f\n",f->maxzooms, f->currzoom, j,f->zoomList[j].cr, f->zoomList[j].delta, f->zoomList[j].cr1, f->zoomList[j].delta1); 
	 return;
      }
    }
 
    // try to insert the new zoom before the first zoomList[i] it encompases.
    for(i=0; i<nzooms; i++) {
      b = zoomEncompassed(cmd, c2,d2,c1,d1, f->zoomList[i]);
      if(b) continue; // new zoom is encompassed by zoomList[i].

      // first zoomList[i] that doe snot encompasses the new zoom
      b = zoomEncompassing(cmd, c2,d2,c1,d1, f->zoomList[i]);
      if(b) {
        // but the new zoom encompasses zoomList[i], so we insert it
	// before zoomList[i].
        nzooms++;
        f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)*nzooms); 
 	for(j=nzooms-1; j>i; j--) {
    	  f->zoomList[j].cr = f->zoomList[j-1].cr;
    	  f->zoomList[j].delta = f->zoomList[j-1].delta;
    	  f->zoomList[j].cr1 = f->zoomList[j-1].cr1;
    	  f->zoomList[j].delta1 = f->zoomList[j-1].delta1;
	}	
    	f->zoomList[i].cr = c2;
    	f->zoomList[i].delta = d2;
    	f->zoomList[i].cr1 = c1;
    	f->zoomList[i].delta1 = d1;
	f->maxzooms=nzooms;
        f->currzoom=i;
//for (j=0;j<nzooms; j++)
//Winfoprintf("insert zoom %d %d %d %f %f %f %f\n",f->maxzooms, f->currzoom,j, f->zoomList[j].cr, f->zoomList[j].delta, f->zoomList[j].cr1, f->zoomList[j].delta1); 
	return; 
     } else { 
	// new zoom does not encompass zoomList[i], so we replace 
	// zoomList[i] with the new zoom, and delete zooms beyond zoomList[i].
        // this ensure that zoomList[i] always encompasses zoomList[i+1]. 
    	f->zoomList[i].cr = c2;
    	f->zoomList[i].delta = d2;
    	f->zoomList[i].cr1 = c1;
    	f->zoomList[i].delta1 = d1;
	nzooms=i+1;
        f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)*nzooms); 
	f->maxzooms=nzooms;
        f->currzoom=i;
//for (j=0;j<nzooms; j++)
//Winfoprintf("purge zoom %d %d %d %f %f %f %f\n",f->maxzooms, f->currzoom, j,f->zoomList[j].cr, f->zoomList[j].delta, f->zoomList[j].cr1, f->zoomList[j].delta1); 
	return;
      }
    }
   
    // if it gets here, then the new zoom is encompassed by all elements
    // in zoomList. we append it to the end of the list.
    nzooms++;
    f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)*nzooms); 
    f->zoomList[nzooms-1].cr = c2;
    f->zoomList[nzooms-1].delta = d2;
    f->zoomList[nzooms-1].cr1 = c1;
    f->zoomList[nzooms-1].delta1 = d1;
    f->maxzooms=nzooms;
    f->currzoom=nzooms-1;
//for (j=0;j<nzooms; j++)
//Winfoprintf("add zoom %d %d %d %f %f %f %f\n",f->maxzooms, f->currzoom,j, f->zoomList[j].cr, f->zoomList[j].delta, f->zoomList[j].cr1, f->zoomList[j].delta1); 
}

// this is called when initializing first element of zoomLis. 
// it returns current spectrum window.
void getCurrentZoom(double *c2, double *d2, double *c1, double *d1)
{
    char str[SHORTSTR];

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strstr(str,"con") != NULL) {
      dconi_currentZoom(c2,d2,c1,d1);
    } else {
      if ( get_axis_freq(HORIZ) )
     	*c2  = sp + delta;
      else
        *c2 = sp;
      *d2  = fabs(wp);
      *c1=0;
      *d1=0;
    }
}

// this is used to determine whether zoomList should be initialized by current window. 
int isFull()
{
   if(revflag && (sw1-wp1)<10) return 1;
   else if((sw-wp)<10) return 1;
   else return 0;
}

// this is called by "full" zoom out.
// if spectrum is already fully zoommed out, cursors will not be changed
// (set d2=-1 so the caller will not change current cursors), 
// otherwise cursors will be set to current zoomList[i] (i.e., current window),
// before zoommed to full, so current window will be restored in zoom in.
// Note, we use getCurrentZoom only if zoomList needs to be initialized,
// because getCurrentZoom is based on swp, wp, which are slightly different from 
// window defined by zoomList[i].
void getCurrentZoomFromList(double *c2, double *d2, double *c1, double *d1)
{
    frameInfo *f;
    int i;

    if(!testFrameID(currframe)) return;

    f = &(frames[currframe]);
    if(f == NULL) return;

    if(f->zoomList==NULL && !isFull()) {
        f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)); 
	f->maxzooms = 1;
    	f->currzoom=0;
        getCurrentZoom(&(f->zoomList[0].cr),&(f->zoomList[0].delta),&(f->zoomList[0].cr1),&(f->zoomList[0].delta1));
    }

    i = f->currzoom;
    if(i < 0 || i >= f->maxzooms) { // already zoommed to full.
        *d2=-1;
	return;
    }

    if(c1==NULL || d1==NULL) { // called by ds
      *c2=f->zoomList[i].cr;
      *d2=f->zoomList[i].delta;
    } else if(revflag) {
      *c2=f->zoomList[i].cr1;
      *d2=f->zoomList[i].delta1;
      *c1=f->zoomList[i].cr;
      *d1=f->zoomList[i].delta;
    } else {
      *c1=f->zoomList[i].cr1;
      *d1=f->zoomList[i].delta1;
      *c2=f->zoomList[i].cr;
      *d2=f->zoomList[i].delta;
    }
}

// this is called by "expand" to set cursors for next zoom.
void getNextZoom(double *c2, double *d2, double *c1, double *d1)
{
    frameInfo *f;
    int i;

    if(!testFrameID(currframe)) return;

    f = &(frames[currframe]);
    if(f == NULL) return;

    i = f->currzoom + 1;
    if(i<(f->maxzooms)) {
        if(c1==NULL || d1==NULL) {
	   *c2=f->zoomList[i].cr;
	   *d2=f->zoomList[i].delta;
	} else {
	   *c1=f->zoomList[i].cr1;
	   *d1=f->zoomList[i].delta1;
	   *c2=f->zoomList[i].cr;
	   *d2=f->zoomList[i].delta;
	}
    } else {
	getNextZoomin(c2, c1, d2, d1);
    }
}

// this is called to zoom in and out by step.
// if step=0, then full zoom will be called.
// if step > 0, we expand window to current cursors (which was set to "next zoom"
// by "expand".
// if step < 0, we zoom out to previous zoom.
void mfZoom(int step)
{
    int i;
    frameInfo *f;
    double c2,d2,c1,d1;

    char str[SHORTSTR];

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strcasecmp(str, "ds") != 0 && strcasecmp(str, "df") != 0 &&
	strstr(str,"con") == NULL) return;

    if(!testFrameID(currframe)) return;

    f = &(frames[currframe]);

    if(f->zoomList == NULL && !isFull()) { // initialize to current window if needed.
        f->zoomList = (zoomInfo *) realloc(f->zoomList, sizeof(zoomInfo)); 
	f->maxzooms = 1;
    	f->currzoom=0;
        getCurrentZoom(&(f->zoomList[0].cr),&(f->zoomList[0].delta),&(f->zoomList[0].cr1),&(f->zoomList[0].delta1));
    }

    if(step > 0.5 || step < -0.5) i = f->currzoom + step; 
    else i = -1;

    if(i < 0) { // zoom out to full. 
      doZoom(CURSOR_MODE);
      f->currzoom = -1;
      return;
    } 

    f->currzoom = i;
    if(step < 0 && i < f->maxzooms) { // do this to set cursors only if zoom out.
        c2=f->zoomList[i].cr;
        d2=f->zoomList[i].delta;
        c1=f->zoomList[i].cr1;
        d1=f->zoomList[i].delta1;
        if(strstr(str,"con") != NULL) {
	  dconi_setNextCursor(c2,c1,d2,d1);
        } else {
	  cr=c2;
 	  delta=d2;
        }
    } // else zoom is already set by previous "expand".
    doZoom(BOX_MODE);
}

static void doZoom(int mode)
{
    char str[SHORTSTR];

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;

    if(strcasecmp(str, "df") == 0) { 
	df_zoom(mode);
    } else if(strcasecmp(str, "ds") == 0) { 
	ds_zoom(mode);
    } else if(strstr(str,"con") != NULL) {
	dconi_zoom(mode);
    }
}

static void getNextZoomin(double *c2, double *c1, double *d2, double *d1)
{
    char str[SHORTSTR];

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;

    if(strcasecmp(str, "ds") == 0) { 
	ds_nextZoomin(c2, c1, d2, d1);
    } else if(strcasecmp(str,"df") == 0) {
	df_nextZoomin(c2, c1, d2, d1);
    } else if(strstr(str,"con") != NULL) {
	dconi_nextZoomin(c2, c1, d2, d1);
    }
}

void spwpFrame(int but, int x, int y, int mflag) {
	char str[SHORTSTR];
        static double prev_sp = 0;

	int y1 = mnumypnts - y -1;
        int dfpntx  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
        int dnpntx  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
        int dfpnty = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
        int dnpnty = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
	if(x < dfpntx || x > dfpntx+dnpntx ||
	   y1 < dfpnty || y1 > dfpnty+dnpnty) return;

	Wgetgraphicsdisplay(str, SHORTSTR);

	if (strstr(str, "d") != str)
		return;
	if (strcmp(str, "ds") == 0 && start_from_ft == 1)
		return;
	if (strcmp(str, "dconi") == 0 && start_from_ft2d == 1)
		return;

	if (strstr(str, "con") != NULL) {
		dconi_spwp(but, x, y, mflag);
	} else if (strncasecmp(str, "ds",2) == 0){
		if(!(mflag & shift) && !(mflag & ctrl))
			jmouse_spwp(but, x, y, mflag);
		if(strlen(str)==2)
			ds_spwp(but, x, y, mflag);
		else { // arrayed spec
		   double newSp = - (x - dfpnt ) * wp  / dnpnt  + wp + sp;
		   if(but == 4) {
			prev_sp = newSp;
			return;
		   }  
		   sp += (prev_sp - newSp); 
	 	   P_setreal(CURRENT, "sp", sp, 1);
	 	   execString("showarrays('redisplay')\n"); 
		}
	} else if (strncasecmp(str, "df",2) == 0){
		if(!(mflag & shift) && !(mflag & ctrl))
			jmouse_spwp(but, x, y, mflag);
		if(strlen(str)==2)
			df_spwp(but, x, y, mflag);
		else { // arrayed fids 
	 	   double newSp = (x - dfpnt ) * wp  / dnpnt  + sp; 
		   if(but == 4) {
			prev_sp = newSp;
			return;
		   }  
		   sp += (prev_sp - newSp); 
	 	   P_setreal(CURRENT, "sf", sp, 1);
	 	   execString("showarrays('redisplay')\n"); 
		}
	}
}

int isTextFrame(int id) {

    if(!testFrameID(id)) return 0;

    if(strcmp(frames[id].displayType,"text") == 0) return 1;
    else return 0;
}

int isDrawable(int id) {
   // if(id > 1 && getOverlayMode() >= OVERLAID_ALIGNED) return 0;
   if(isTextFrame(id)) return 0;
   else return 1;
}

void getFrameG(int id, int *x, int *y, int *w, int *h)
{
   *x=frames[id].x;
   *y=frames[id].y;
   *w=frames[id].w;
   *h=frames[id].h;
}

extern int fwc,fsc,fwc2,fsc2;
extern int ygraphoff;
extern int ycharpixels;
extern int plot;
extern int getDscaleDecimal(int);
int frame_set_pnt() {

    int east, west, north, south;
    int overlayMode = getOverlayMode();
    int chartMode = getChartMode();

//Winfoprintf("frame_set_pnt %d %d %d %d %d %d %d %d",VnmrJViewId,overlayMode,chartMode,mnumxpnts,mnumypnts,ymin,ygraphoff, ycharpixels);
    if(overlayMode == OVERLAID_ALIGNED && plot) {
        if (d2flag != 0 && !plot) {
          Wgetgraphicsdisplay(tmpStr, SHORTSTR);
          if (strncmp(tmpStr,"ds", 2) != 0) {
	    double xband = 9*xcharpixels/((double)(mnumxpnts-right_edge) / wcmax);
            if((wcmax-wc-sc) < xband) {
                wc= wcmax - xband;
                fwc=(int)wc;
                fsc=(int)sc;
                fwc2=(int)wc2;
                fsc2=(int)sc2;
                P_setreal(CURRENT,"wc",wc,0);
	    }
	  }
        }
      	dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
        dnpnt  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
        dfpnt2 = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
        dnpnt2 = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
    } else if(overlayMode == OVERLAID_ALIGNED) {
	int band1;
        int x,y;

	// ygraphoff is for fields display, set by graphics.c setdisplay(). 
	// ygraphoff=2*ycharpixels if fields are shown, otherwise ygraphoff=0
        if(chartMode == ALIGN_1D_Y) { //x,y are swapped
	   x=mnumypnts+ygraphoff;
	   y=mnumxpnts-ygraphoff;
	} else {
           x = mnumxpnts;
           y = mnumypnts;
	} 

//put it to the east
	if(y > x)
	   band1 = y/15;
	else
	   band1 = x/15;

	west = band1;
	south = band1; 
	east = x/5;
	north = y/5;

	dfpnt  = east;
        dnpnt  = x - east - west;
	dfpnt2  = south;
        dnpnt2  = y - south - north;
	   
        if(chartMode == ALIGN_1D_Y) { //x,y are swapped
	   int dfpntx=dfpnt;
           int dnpntx=dnpnt;
           int dfpnty=dfpnt2;
           int dnpnty=dnpnt2;
           dfpnt=dfpnty+ygraphoff;
           dnpnt=dnpnty;
           dfpnt2=dfpntx;
           dnpnt2=dnpntx;
  	}
        if(chartMode == ALIGN_2D) {
           wc = (double)dnpnt*wcmax/(double)(mnumxpnts-right_edge);
           sc = wcmax - wc - (double)dfpnt*wcmax/(double)(mnumxpnts-right_edge);
           wc2 = (double)dnpnt2*wc2max/(double)(mnumypnts-ymin);
           sc2 = (double)(dfpnt2-ymin)*wc2max/(double)(mnumypnts-ymin);
           P_setreal(CURRENT,"sc",sc,0);
           P_setreal(CURRENT,"wc",wc,0);
           P_setreal(CURRENT,"sc2",sc2,0);
           P_setreal(CURRENT,"wc2",wc2,0);
        }
        fwc=(int)wc;
        fsc=(int)sc;
        fwc2=(int)wc2;
        fsc2=(int)sc2;
    } else if(overlayMode >= NOTOVERLAID_ALIGNED && chartMode == DEFAULT_2D) {
	int band1;
	int band2;

	if(mnumypnts > mnumxpnts)
	   band1 = mnumypnts/15;
	else
	   band1 = mnumxpnts/15;

 	band2 = band1/2;	
	east = band1;
	south = band1;
	west = band1;
	north = band2;
      	dfpnt  = east;
        dnpnt  = mnumxpnts - east - west;
        dfpnt2  = south;
        dnpnt2  = mnumypnts - south - north;
/*
    } else if(overlayMode >= NOTOVERLAID_ALIGNED) {
	int band1;
	int band2;

	if(mnumypnts > mnumxpnts)
	   band1 = mnumypnts/15;
	else
	   band1 = mnumxpnts/15;

 	band2 = band1/2;	
	east = 0; 
	south = band1;
	west = 0;
	north = band2;
      	dfpnt  = east;
        dnpnt  = mnumxpnts - east - west;
        dfpnt2  = south;
        dnpnt2  = mnumypnts - south - north;
*/
    } else if(overlayMode == OVERLAID_NOTALIGNED) {
	int band1;
	int band2;
        right_edge = 0; 

	if(mnumypnts > mnumxpnts)
	   band1 = mnumypnts/15;
	else
	   band1 = mnumxpnts/15;

 	band2 = band1/2;	
	east = band1; 
	south = band1;
	west = band1;
	north = band2;
      	dfpnt  = east;
        dnpnt  = mnumxpnts - east - west;
        dfpnt2 = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
        dnpnt2 = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
    } else {
      if (d2flag != 0 && !plot) {
        Wgetgraphicsdisplay(tmpStr, SHORTSTR);
        if (strncmp(tmpStr,"ds", 2) != 0) {
	  double xband = 9*xcharpixels/((double)(mnumxpnts-right_edge) / wcmax);
          if((wcmax-wc-sc) < xband) {
                wc= wcmax - xband;
                fwc=(int)wc;
                fsc=(int)sc;
                fwc2=(int)wc2;
                fsc2=(int)sc2;
                P_setreal(CURRENT,"wc",wc,0);
	  }
	}
      }
      dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
      dnpnt  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
      dfpnt2 = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
      dnpnt2 = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
    }

    return(0);
}

void overlaySpec(int mode)
{
    char str[SHORTSTR];
    if(!testFrameID(currframe)) return;
    if(currframe > 1) return;

    Wgetgraphicsdisplay(str, SHORTSTR);
    if(strstr(str,"d") != str) return;
    if(strcmp(str,"ds") == 0 && start_from_ft == 1) return;
    if(strcmp(str,"dconi") == 0 && start_from_ft2d == 1) return;

    if(strstr(str,"d") != str) return;
    if(strstr(str, "con") != NULL) { 
	dconi_overlaySpec(mode);	
    } else if(strcmp(str,"ds") == 0) {
	ds_overlaySpec(mode);	
    } else {
	execString(str);
        //window_redisplay();
    }
}

 // cmd: delete, hide, show
void imageFrameOp(int id, int fromFrameCmd, char *cmd)
{
     int i, frameId;

     if (cmd == NULL)
        return;
     frameId = -1;
     if (fromFrameCmd)
        frameId = id;
     else {
        for (i = 1; i < maxframes; i++) {
           if (strcmp(frames[i].displayType, "image") == 0) {
              if (frames[i].objID == id) {   
                 frameId = i;
                 break;
              }
           }
        }
     }
     if (frameId < 1 || frames[frameId].objID < 1)
        return;
        
     if (strcasecmp(cmd, "delete") == 0) {
        initFrameInfo(frameId);
     }
     if (fromFrameCmd < 1) // from imagefile command
        return;
     sprintf(tmpStr, "icon %s %d", cmd, frames[frameId].objID);
     writelineToVnmrJ(VJCMD, tmpStr);
}

 // cmd: delete, hide, show
void textFrameOp(int id, int fromFrameCmd, char *cmd)
{
     int i, frameId;

     if (cmd == NULL)
        return;
     frameId = -1;
     if (fromFrameCmd)
        frameId = id;
     else {
        for (i = 1; i < maxframes; i++) {
           if (strcmp(frames[i].displayType, TEXTTYPE) == 0) {
              if (frames[i].objID == id) {   
                 frameId = i;
                 break;
              }
           }
        }
     }
     if (frameId < 1 || frames[frameId].objID < 1)
        return;
        
     sprintf(tmpStr, "canvas %s %s %d", TEXTTYPE, cmd, frames[frameId].objID);
     if (strcasecmp(cmd, "delete") == 0) {
        initFrameInfo(frameId);
     }
     if (fromFrameCmd < 1) // from other command
        return;
     writelineToVnmrJ(VJCMD, tmpStr);
}

void set_dpf_flag(int b, char *cmd) {
    frameInfo *frame;

    if(!testFrameID(currframe)) return;

    frame = &frames[currframe];
    frame->dpf_flag=b;
    strcpy(frame->dpf_cmd,cmd);
}

void set_dpir_flag(int b, char *cmd) {
    frameInfo *frame;

    if(!testFrameID(currframe)) return;

    frame = &frames[currframe];
    frame->dpir_flag=b;
    strcpy(frame->dpir_cmd,cmd);
}

void do_dpf_dpir() {
    frameInfo *frame;

    if(!testFrameID(currframe)) return;

    frame = &frames[currframe];
    if(frame->dpf_flag && strlen(frame->dpf_cmd)>0) execString(frame->dpf_cmd);
    if(frame->dpir_flag && strlen(frame->dpir_cmd)>0) execString(frame->dpir_cmd);
}

void redo_dpf_dpir() {
    frameInfo *frame;

    if(!testFrameID(currframe)) return;

    frame = &frames[currframe];
    if((frame->dpf_flag && strlen(frame->dpf_cmd)>0) || 
      (frame->dpir_flag && strlen(frame->dpir_cmd)>0) ) execString("ds('again')\n");
}

void get_dpf_dpir(int *dpf_flag, int *dpir_flag) {
    frameInfo *frame;

    if(!testFrameID(currframe)) return;

    frame = &frames[currframe];
    *dpf_flag = frame->dpf_flag;
    *dpir_flag = frame->dpir_flag;
}
