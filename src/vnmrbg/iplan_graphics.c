/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "iplan.h"
#include "iplan_graphics.h"
#include "aipCInterface.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

// extern int graf_height;
extern void sendPlanParams(prescription* plans);
extern  planParams *getPlanTag(int planType);
extern void savePrescriptForType(char* path, int planType);

iplan_mark* marks = NULL;
int numOfMarks = 0;
int isMarking = -1;
int iMarks = 0;
int nMarks = 1;
// addVoxelMode=0, not in addVoxel mode.
// addVoxelMode=1, in addVoxel mode, but not voxel is added yet.
// addVoxelMode=2, in addVoxel mode, one or more voxels are added.
int addVoxelMode=0;

static short xorflag;
static short debug = 0;

int usepos1 = 1; // pro
int usepos2 = 1; // ppe
int usepos3 = 1; // pss0
int selectedSlice = -1;
int selectedStack = -1;
int selectedOverlay = -1;
int activeSlice = -1;
int activeHandle = -1;
int activeVertex1 = -1;
int activeVertex2 = -1;
int activeMark = -1;

int pen = MAGENTA;
int meshColor = PINK_COLOR; 
int onPlaneColor = MAGENTA; 
int offPlaneColor = RED;
int drawMode = NEWOVERLAY;
int highlight = YELLOW;
int axisColor = RED;
unsigned int handleSize = 10;
int orderMode = 0;
int showStackOrders = 0;

int drawIntersection = 1;
int fillIntersection = 0;
int draw3DStack = 0;
int drawStackAxes = 0;
int overlayStyle = BOX;
int alternateOrders = 0;

int markSize = 10;
int markColor = RED;
int frameColor = WHITE;
int handleColor = YELLOW;
int level = 6;
int IBlen = 0;

float2 prevP;

static int orgx;
static int orgy;
static int win_width = 400;
static int win_height = 400;
static int disMode = 1;
static int fixedSlice = -1;
static int imageChanged = 0;
static int isCliped = -1;
int mcoils = 1;
int cubeFOV = 0;

static void mouse_but(int x, int y0, int button, int combo, int release);
static void mouse_drag(int x, int y0, int button, int combo, int status);
static void mouse_move(int x, int y0, int button, int combo, int status);
static void mouse_click(int x, int y0, int button, int combo, int click);
static void update_iplan(int len, int* ids, int* changeFlags);

static void selectOverlay(int view, int stack);
void drawOverlaysForView(int nv);
void getTranslation(int view, float2 p, float2 prevP, 
			int view2, float2 newp);
int getActStackInd();
void drawCSI3DMesh(int view, int lineColor, int drawBackPlanes);
void draw3DBox(iplan_stack *stack, iplan_view *view, int lineColor,
	int drawBackPlanes);
void drawFOVlabel(iplan_stack *stack, iplan_view *view, int lineColor);
void getStack(iplan_stack* stack, int planType);
void calcMirrorPlane(int imageStackType, int tagStackType, int controlStackType);
void calcASLControlPlane(int imageStackType, int tagStackType, int controlStackType, int show);

// call this when type is changed.
void updateUsePos(int planType) {
   planParams *tag = getCurrPlanParams(planType);
   if(tag == NULL) return;
   usepos1=tag->pos1.use;
   usepos2=tag->pos2.use;
   usepos3=tag->pos3.use;
}

int getIplanType() {

    double d;
    int type = 0;
    if(!P_getreal(CURRENT, "iplanType", &d, 1)) {
	type = (int)d;
    }
    if(type < 0) return 0;
    else return type;
}

int getMgradient()
{
    double d;
    if(!P_getreal(GLOBAL, "mgradient", &d, 1)) {
	return (int)d;
    } else return 1;
}

int getMcoils()
{
    double d;
    if(!P_getreal(GLOBAL, "mcoils", &d, 1)) {
	return (int)d;
    } else return 1;
}

void getOriginForCoil(int coil, float *origin)
{
   int i;
    vInfo           paraminfo;
   double d;

   if(currentViews != NULL)
   for(i=0; i<currentViews->numOfViews; i++) {
      if(currentViews->views[i].coil == coil) {
	origin[0] = currentViews->views[i].origin[0];
	origin[1] = currentViews->views[i].origin[0];
	origin[2] = currentViews->views[i].origin[0];
        return;
      }
   }

    origin[0] = 0;
    if(!(P_getVarInfo(GLOBAL, "moriginx", &paraminfo)) && 
	coil <= paraminfo.size) {
    	if(!(P_getreal(GLOBAL, "moriginx", &d, coil))) {
		origin[0] = d;
	} 
    }

    origin[1] = 0;
    if(!(P_getVarInfo(GLOBAL, "moriginy", &paraminfo)) && 
	coil <= paraminfo.size) {
    	if(!(P_getreal(GLOBAL, "moriginy", &d, coil))) {
		origin[1] = d;
	}
    }

    origin[2] = 0;
    if(!(P_getVarInfo(GLOBAL, "moriginz", &paraminfo)) && 
	coil <= paraminfo.size) {
    	if(!(P_getreal(GLOBAL, "moriginz", &d, coil))) {
		origin[2] = d;
	}
    }
}

int getCoil()
{
   int ch = 1;
   int view = 0;

   if(currentViews == NULL) return 1;
   if(activeView > 0 && activeView < currentViews->numOfViews) 
	view = activeView;
   ch = currentViews->views[view].coil;
   if(ch <= 0 || ch > mcoils) ch = 1;

   return ch;  
}

/******************/
void l2mShift(iplan_stack *s, float3 o)
/******************/
{
    float3 p;
    p[0]=s->origin[0];
    p[1]=s->origin[1];
    p[2]=s->origin[2];
    rotatem2u(p, s->orientation);
    o[0] += p[0];
    o[1] += p[1];
    o[2] += p[2];
}

/******************/
void m2lShift(iplan_stack *s, float3 o)
/******************/
{
    float3 p;
    p[0]=s->origin[0];
    p[1]=s->origin[1];
    p[2]=s->origin[2];
    rotatem2u(p, s->orientation);
    o[0] -= p[0];
    o[1] -= p[1];
    o[2] -= p[2];

/* or
    o[0]=s->ppe;
    o[1]=s->pro;
    o[2]=s->pss0;
    rotateu2m(o, s->orientation);
    o[0] -= s->origin[0];
    o[1] -= s->origin[1];
    o[2] -= s->origin[2];
    rotatem2u(o, s->orientation);
*/

}

/******************/
void centerTarget_1()
/******************/
{
    
}

/******************/
void centerTarget_m()
/******************/
{
    char orient[MAXSTR], str[MAXSTR]; 
    float orientation[9];
    float theta, psi, phi;
    float *originx, *originy, *originz;
    float3 *ps;
    float3 o;
    double d;
    vInfo           paraminfo;
    int i, j, err, ns, coil, m;
    float gap, lro, lpe, lpe2, thk, shift;
    float *pss;
    iplan_stack *stack;
    int nv, nd;
    int type = REGULAR;
    nd = 100;

    type = getIplanType();

    mcoils = getMcoils();
    originx = (float *)malloc(sizeof(float)*mcoils);
    originy = (float *)malloc(sizeof(float)*mcoils);
    originz = (float *)malloc(sizeof(float)*mcoils);
    ps = (float3 *)malloc(sizeof(float3)*mcoils);
    err = 0;

    if(!(P_getVarInfo(GLOBAL, "moriginx", &paraminfo)) && 
	paraminfo.size == mcoils) {
	for(i=0; i<mcoils; i++) {
    	    if(!(P_getreal(GLOBAL, "moriginx", &d, i+1))) {
		originx[i] = d;
	    } else err++;
	}
    } else err++;

    if(!(P_getVarInfo(GLOBAL, "moriginy", &paraminfo)) && 
	paraminfo.size == mcoils) {
	for(i=0; i<mcoils; i++) {
    	    if(!(P_getreal(GLOBAL, "moriginy", &d, i+1))) {
		originy[i] = d;
	    } else err++;
	}
    } else err++;

    if(!(P_getVarInfo(GLOBAL, "moriginz", &paraminfo)) && 
	paraminfo.size == mcoils) {
	for(i=0; i<mcoils; i++) {
    	    if(!(P_getreal(GLOBAL, "moriginz", &d, i+1))) {
		originz[i] = d;
	    } else err++;
	}
    } else err++;

    if(err) {
	free(originx);
	free(originy);
	free(originz);
	free(ps);
	return;
    }

    P_getstring(CURRENT, "orient", orient, 1, MAXSTR);
    if(strcmp(orient,"trans") == 0) {
	theta = 0;
	psi = 0;
	phi = 0;
    } else if(strcmp(orient,"cor") == 0) {
	theta = 90;
	psi = 0;
	phi = 0;
    } else if(strcmp(orient,"sag") == 0) {
	theta = 90;
	psi = 90;
	phi = 0;
    } else if(strcmp(orient,"trans90") == 0) {
	theta = 0;
	psi = 0;
	phi = 90;
    } else if(strcmp(orient,"cor90") == 0) {
	theta = 90;
	psi = 0;
	phi = 90;
    } else if(strcmp(orient,"sag90") == 0) {
	theta = 90;
	psi = 90;
	phi = 90;
    } else {
	theta = 0;
	psi = 0;
	phi = 0;
    }

    euler2tensor(theta, psi, phi, orientation);

    getDefaults(CURRENT, type, &ns, &lpe, &lro, &lpe2, &thk);
    if(!P_getreal(CURRENT, "gap", &d, 1)) gap = d;
    else if(ns > 1 && lpe2 > ns*thk) gap = (lpe2 - ns*thk)/(ns - 1);
    else gap = 0;
    pss = (float*)malloc(sizeof(float)*ns);
    if(ns%2 == 0) shift = (ns)/2.0 - 0.5;
    else shift = (ns-1.0)/2.0;

    for(i=0; i<mcoils; i++) {
	o[0] = originx[i];
	o[1] = originy[i];
	o[2] = originz[i];
        rotatem2u(o, orientation);
      for(j=0; j<ns; j++) {
        if(o[j] < 0) nv = (int)(o[j]*nd - 0.5);
        else nv = (int)(o[j]*nd + 0.5);
        o[j] = ((float)nv)/nd;
      }
	ps[i][0] = o[0];
	ps[i][1] = o[1];
	ps[i][2] = o[2];

	m = i+1;
        
	P_setreal(CURRENT, "mppe", (double)o[0], m);
	P_setreal(CURRENT, "mpro", (double)o[1], m);
	P_setreal(CURRENT, "mpss", (double)o[2], m);
	P_setreal(CURRENT, "mlro", (double)lro, m);
	P_setreal(CURRENT, "mlpe", (double)lpe, m);
	P_setreal(CURRENT, "mlpe2", (double)lpe2, m);
	P_setreal(CURRENT, "mthk", (double)thk*10, m);

    if(strcmp(orient,"3orthogonal") == 0) {
        sprintf(str,"%.2f %.2f %.2f",90.0, 90.0, 0.0);
	P_setstring(CURRENT, "mtheta", str, m);
        sprintf(str,"%.2f %.2f %.2f",90.0, 0.0, 0.0);
	P_setstring(CURRENT, "mpsi", str, m);
        sprintf(str,"%.2f %.2f %.2f",0.0, 0.0, 0.0);
	P_setstring(CURRENT, "mphi", str, m);
    } else {
        sprintf(str,"%.2f",theta);
	P_setstring(CURRENT, "mtheta", str, m);
        sprintf(str,"%.2f", psi);
	P_setstring(CURRENT, "mpsi", str, m);
        sprintf(str,"%.2f", phi);
	P_setstring(CURRENT, "mphi", str, m);
    }

    } 
     sendPnew0(10, " mpro mppe mpss mtheta mpsi mphi mlro mlpe mlpe2 mthk");
     execString("setplan\n");
    
    if(currentViews != NULL && currentStacks != NULL) {
        for(i=0; i<currentStacks->numOfStacks; i++) {
	   stack = &(currentStacks->stacks[i]);
	   if(stack->type == type) {
		coil = currentStacks->stacks[i].coil;
		if(coil < 1 || coil > mcoils) continue;
		stack->ppe=ps[coil-1][0];
		stack->pro=ps[coil-1][1];
		stack->pss0=ps[coil-1][2];
/*
fprintf(stderr, "center %d %d %f %f %f\n", i, coil, stack->ppe, stack->pro, stack->pss0);
*/
		updatePss(i, 0, stack->ns-1);
        	calTransMtx(stack);
        	calSliceXYZ(stack);
        	calOverlayForAllViews(i);
	   }
	}
        drawOverlays();
    } 
    free(originx);
    free(originy);
    free(originz);
    free(ps);
    free(pss);
}

/******************/
void centerTarget()
/******************/
{
    mcoils = getMcoils();
    if(mcoils > 1) centerTarget_m();
    else centerTarget_1();
}

/******************/
int handle2slice(int i)
/******************/
{
    if(activeView == -1 || activeStack == -1) return(-1);
    if(currentStacks->stacks[activeStack].type != RADIAL) return(-1);

    return(currentOverlays->overlays[activeView].stacks[activeStack].handles2slices[i]);
}

/******************/
int startIplan0(int type)
/******************/
{
    createCommonPlanParams();

    mcoils = getMcoils();
    if(mcoils > 1) { 
      if(type < 0) type = getIplanType();
      planParams *tag = getCurrPlanParams(type);
      if(tag != NULL) { 
        setUseParam(type,tag->pos1.name,0); 
        updateUsePos(type);
      }
    }

    IBlen = aipGetNumberOfImages();

    if(aipOwnsScreen() && IBlen > 0) {

        initIplan();

        getCurrentIBViews(IBlen);

        aipRegisterDisplayListener(update_iplan);
        aipRegisterMouseListener(mouse_move, NULL);

    } else {
        endIplan();
	return(-9);
    }
/* no longer need this. IB takes care of it. */
/*
    Wsetgraphicsdisplay("gplan_update");
*/
    initMarks();
    makeActiveStacks(CURRENT); 

    /* need to initialize memory allocation for currentStacks */
    /* copying defualt to currentStacks, then clear it up */

        copyPrescript(activeStacks, currentStacks);
        calOverlays();
        while(currentStacks->numOfStacks > 0) deleteStack(0);

    RETURN;

}

/******************/
int startIplan(int type)
/******************/
{
    int i = startIplan0(type);
    if(i != -9) getCurrentStacks();

    RETURN;
}

/******************/
void endIplan()
/******************/
{
   if(currentViews == NULL || currentStacks == NULL) return;

   aipUnregisterMouseListener(mouse_move, NULL);
/*
    Jactivate_mouse(NULL, NULL, NULL, NULL, NULL);
    Wsetgraphicsdisplay("");
*/
   updateType();
   sendParamsByOrder(ALLSTACKS);
   clearStacks();

   freeViews(currentViews);
   freeViews(prevViews);
   freePrescript(activeStacks);
   freePrescript(currentStacks);
   freePrescript(prevStacks);
   freeOverlays(currentOverlays);
   freeOverlays(prevOverlays);

   currentViews = NULL;
   prevViews = NULL;
   activeStacks = NULL;
   currentStacks = NULL;
   prevStacks = NULL;
   currentOverlays = NULL;
   prevOverlays = NULL;

}

/******************/
void initMarks()
/******************/
{
    int i;
    int x0, y0;

/*
    if(currentViews->numOfViews > 1) numOfMarks = 3;
    else numOfMarks = 2; 
*/
    numOfMarks = 3;

    if(marks == NULL)
    marks = (iplan_mark*)malloc(sizeof(iplan_mark)*numOfMarks);
/*
    x0 = (int)((double)(aip_mnumxpnts-right_edge)/2);
    y0 =  markSize;
*/
    x0 = -1;
    y0 = -1;

    for(i=0; i<numOfMarks; i++) {
	marks[i].view = -1;
	marks[i].stack = -1;
	marks[i].slice = -1;
	marks[i].isFixed = 0;
	marks[i].isUsing = 0;
    	marks[i].homeLocation[0] = x0 - i*2*markSize; 
    	marks[i].homeLocation[1] = y0; 
    	marks[i].currentLocation[0] = marks[i].homeLocation[0]; 
    	marks[i].currentLocation[1] = marks[i].homeLocation[1]; 
    }
}

/******************/
void removeMark(int i)
/******************/
{
    if(marks == NULL || i < 0 || i >= numOfMarks) return;

    if(iMarks > 0 && marks[i].isUsing > 0) iMarks--;

    marks[i].view = -1;
    marks[i].stack = -1;
    marks[i].slice = -1;
    marks[i].isUsing = 0;
    marks[i].currentLocation[0] = marks[i].homeLocation[0];
    marks[i].currentLocation[1] = marks[i].homeLocation[1];
    marks[i].isFixed = 0;

}

/******************/
void removeAllMarks()
/******************/
{
    int j;

    if(iMarks < 1) return;

    for(j=0; j<numOfMarks; j++) removeMark(j);

    iMarks = 0;
    sendValueToVnmr("iplanMarking", iMarks); 
    appendJvarlist("iplanMarking");
    // writelineToVnmrJ("pnew", "1 iplanMarking");
}

void stopMarking()
{
    activeMark = -1;
    isMarking = -1;
    writelineToVnmrJ("vnmrjcmd", "cursor pointer");
//    sendValueToVnmr("iplanMarking", 0); 
//    writelineToVnmrJ("pnew", "1 iplanMarking");
    return;
}

int startMarking()
{
    int i;
                                                                                                 
    if(currentStacks == NULL || currentViews == NULL) {
        if(startIplan0(-1) == -9) return -1;
    }

    if(iMarks >= nMarks) {
        stopMarking();
    } else {

/* check the three marks to see whether there is one that is not used */

    isMarking = -1;
    for(i=0; i<numOfMarks; i++) {
        if(marks[i].isUsing == 0 ) {
          isMarking = i;
        }
    }

    if(isMarking == -1) {
        activeMark = -1;
        writelineToVnmrJ("vnmrjcmd", "cursor pointer");
    } else {
        writelineToVnmrJ("vnmrjcmd", "cursor pencil");
    }

    }
    sendValueToVnmr("iplanMarking", iMarks); 
    appendJvarlist("iplanMarking");
    // writelineToVnmrJ("pnew", "1 iplanMarking");
    return isMarking;
}

void clearMarking()
{
    int i, view;
    activeMark = -1;
    isMarking = -1;

    for(i=0; i<numOfMarks; i++) {
	if(marks[i].isUsing) {
	  view = marks[i].view;
          removeMark(i);
	  drawOverlaysForView(view);
	}
    }

    iMarks = 0;
    writelineToVnmrJ("vnmrjcmd", "cursor pointer");
    sendValueToVnmr("iplanMarking", iMarks); 
    appendJvarlist("iplanMarking");
    // writelineToVnmrJ("pnew", "1 iplanMarking");
    return;
}

/******************/
void setMarkMode(int mode)
/******************/
{
/* this function will remove or activate one mark each time it is */
/* executed. if all the marks (maximum number of marks, numOfMarks, is 3) */
/* are removed, or activated, it does nothing. */

/* when a mark is activated (mode > 0), the cursor changes into a pencil */
/* when you click the graphic area, a mark will be put there */
/* marks on the graphic area can be moved around. */

/* when a mark is removed (mode = 0, or single click the mark with middle */
/* mouse button), the mark will be removed from the screen. cursor is pointer. */

    int i, view;

/* do nothing if iplan hasn't started */
/*
    if(currentViews == NULL || currentStacks == NULL) return;
*/
    if(currentStacks == NULL || currentViews == NULL) {
        if(startIplan0(-1) == -9) return;
    }

/* initialize cursor and isMarking */
    isMarking = -1;
    writelineToVnmrJ("vnmrjcmd", "cursor pointer");

/* check the three marks to see whether there is one that is not used */

    for(i=0; i<numOfMarks; i++) {
        if(mode > 0 && marks[i].isUsing == 0 ) {
          //marks[i].isUsing = mode;
          isMarking = i;
          writelineToVnmrJ("vnmrjcmd", "cursor pencil");
          return;
        } else if(mode <= 0 && marks[i].isUsing > 0) {
	  view = marks[i].view;
          removeMark(i);
	  activeMark = -1;
	  drawOverlaysForView(view);
          return;
        }
    }

/*  all three marks are used, set to reuse the mark later. */
    if(mode > 0 && isMarking == -1) {
	isMarking = numOfMarks;
        writelineToVnmrJ("vnmrjcmd", "cursor pencil");
    }

    sendValueToVnmr("iplanMarking", iMarks); 
    appendJvarlist("iplanMarking");
    // writelineToVnmrJ("pnew", "1 iplanMarking");

    return;
}

/******************/
void drawMarks(int flag, int i)
/******************/
{
    if(marks == NULL || currentViews == NULL || currentStacks == NULL) return;

    if(i >= 0 && i < 3 && marks[i].isUsing)
    drawMark(flag, marks[i].currentLocation, marks[i].isFixed);
}

/******************/
void drawMark(int flag, float2 mark, int isFixed)
/******************/
{
    /* should check to see mark is within the frame */

    if(mark[0] < 0 || mark[1] < 0) return;

    color(markColor);
    set_color_levels(level);
    set_color_intensity(1);
    drawCross(flag, (int)mark[0], (int)mark[1], markSize, 2, "LineSolid"); 
    if(isFixed) {
	color(highlight); 
    	set_color_levels(level);
    	set_color_intensity(1);
        drawCircle(flag, (int)mark[0], (int)mark[1], markSize/2, 0, 360*64, 4, "LineSolid"); 
    }
}

/******************/
void iplan_set_win_size(int x, int y, int w, int h) 
/******************/
{
    orgx = x;
    orgy = y;
    win_width = w;
    win_height = h;
    aip_mnumxpnts = w - 4;
    aip_mnumypnts = h - 4;
}

/******************/
void initDraw(int mode)
/******************/
{
    aipUnregisterDisplayListener(update_iplan); 
    refreshImages(mode);
    aipRegisterDisplayListener(update_iplan);
}

/******************/
void refreshImages(int mode)
/******************/
{
    int i;
    int* ids;

    if(aipOwnsScreen() && IBlen > 0) {

        ids = (int*)malloc(sizeof(int)*IBlen);

        aipGetImageIds(IBlen, ids);

	for(i=0; i<IBlen; i++) aipRefreshImage(ids[i]);

        free(ids);

    } else {

    	if(mode == NEWOVERLAY) {
      	   popBackingStore();
      	   toggleBackingStore(0);
      	   xorflag = 0;
    	} else if(mode == OVERLAY) {
      	   toggleBackingStore(0);
      	   xorflag = 0;
    	} else if(mode == CLEAR) {
      	   popBackingStore();
      	   return;
    	}
    }
}

int intersected(iplan_2Dstack *overlay)
{
   int k;
   for(k=0; k<overlay->numOfStripes; k++) {
	if(overlay->stripes[k].np > 0) return 1;
   }
   return 0;
}

/******************/
int getOrder(int* orders, int n, int j)
/******************/
{
/* stack parameter int* order are mapping orders used internally. */
/* It needs to be converted to acquire orders to display. */

   int i;
 
   if(orderMode) return orders[j];
   else 
   for(i=0; i<n; i++)
      if(fabs(orders[i])-1 == j) return i+1;

   return -1; 
}

/******************/
void drawStackOrders(int flag, int j, int nv)
/******************/
{
    int i, l;
    int x1, y1;
    int xmin, xmax, ymin, ymax;
    float2 center;
    int size = 16;

    if(currentStacks == NULL || currentViews == NULL) return;

    xmin = currentViews->views[nv].framestx;
    xmax = currentViews->views[nv].framestx +(currentViews->views[nv].framewd);

    ymin = currentViews->views[nv].framesty - (currentViews->views[nv].frameht);
    ymax = currentViews->views[nv].framesty;

    /* draw stack orders. */

    color(axisColor);
    set_color_levels(level);


    if(getCrosssectionCenter(center, nv, j)) {
           x1 = (int)center[0]; 
           y1 = (int)center[1]; 

/* draw coil id instead of stack order
           i = getOrder(currentStacks->order, currentStacks->numOfStacks, j);
*/
	   i = currentStacks->stacks[j].coil;
           l = 1+(int)log10(i); 

           if(i > 0 && x1 > xmin && x1 < xmax && y1 > ymin && y1 < ymax) 
           drawString(flag, x1, y1, size, (char *)intString(i), l);
    }
}

/******************/
void drawSliceOrders(int flag, int j, int nv)
/******************/
{
    int i, k, l;
    int x1, y1;
    int xmin, xmax, ymin, ymax;
    int nstrps, npts;
    int size = 8;

    if(currentStacks == NULL || currentViews == NULL) return;

    xmin = currentViews->views[nv].framestx;
    xmax = currentViews->views[nv].framestx +(currentViews->views[nv].framewd);

    ymin = currentViews->views[nv].framesty - (currentViews->views[nv].frameht);
    ymax = currentViews->views[nv].framesty;

    /* draw slice orders. */
    color(axisColor);
    set_color_levels(level);

      if(currentStacks->stacks[j].ns > 1) {
        nstrps = currentOverlays->overlays[nv].stacks[j].numOfStripes;
        for(k=0; k<nstrps; k++) {
	  
          npts = currentOverlays->overlays[nv].stacks[j].stripes[k].np;

          if(npts > 0 && currentStacks->stacks[j].order[k] > 0) {

            i = getOrder(currentStacks->stacks[j].order, currentStacks->stacks[j].ns, k);
            l = 1+(int)log10(i); 

	    x1 = currentOverlays->overlays[nv].stacks[j].stripes[k].points[0][0];
	    y1 = currentOverlays->overlays[nv].stacks[j].stripes[k].points[0][1];

            if(x1 > xmin && x1 < xmax && y1 > ymin && y1 < ymax) 
	    drawString(flag, x1, y1, size, (char *)intString(i), l);
          }
        }
      }
}

/******************/
void drawInterSection(int flag, iplan_2Dstack *overlay, int j, int nv)
/******************/
{
    int k, l;
    int x1, y1, x2, y2;
    int nstrps, npts;
    float2 points[12];
    int xmin, xmax, ymin, ymax;
    int intensity;
    int nm, nmarks, markind[3];
    iplan_stack *stack;

    if(currentStacks == NULL || currentViews == NULL) return;
    if(overlay == NULL) return;
    stack = overlay->stack;
    if(stack == NULL) return;

    if(nv < 0 || nv >= currentViews->numOfViews) return;
    xmin = currentViews->views[nv].framestx;
    xmax = currentViews->views[nv].framestx +(currentViews->views[nv].framewd);

    ymin = currentViews->views[nv].framesty - (currentViews->views[nv].frameht);
    ymax = currentViews->views[nv].framesty;

    color(stack->color);

/*
    if(draw3DStack) {
*/
    	intensity = level*0.5 - 2;
    	set_color_levels(level);
    	set_color_intensity(intensity);
/*
    }
*/

    nstrps = overlay->numOfStripes;
    if(overlayStyle == BOX) {
          for(k=0; k<nstrps; k++) {
	  
	      npts = overlay->stripes[k].np;

	      if(npts > 0 && stack->order[k] >= 0) {

	        x2 = overlay->stripes[k].points[npts-1][0];
	        y2 = overlay->stripes[k].points[npts-1][1];

                for(l=0; l<npts; l++) {

	          x1 = x2;
	          y1 = y2;

		  x2 = overlay->stripes[k].points[l][0];
		  y2 = overlay->stripes[k].points[l][1];
	
		  if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
		       (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) { 
		  } else {
			drawLine(flag, x1, y1, x2, y2, 1, "LineSolid");
		  }

		  points[l][0] = x2;
		  points[l][1] = y2;
	        }

	        if(npts > 2 && fillIntersection) {
		      fillPolygon(flag, points, npts, "Convex", "CoordModeOrigin");
		      /*draw marks again if they are covered */
		      nmarks = countMarksForSlice(k, markind);
                      if(nmarks > 0) for(nm = 0; nm<nmarks; nm++) {
                       	drawMarks(flag, markind[nm]);
			/* set the color back */
    			color(stack->color);
    			intensity = level*0.5 - 2;
    			set_color_levels(level);
    			set_color_intensity(intensity);
		      }
	        }
     	     }
  	  }
    } else if(overlayStyle == CENTER) {
          for(k=0; k<nstrps; k++) {

	     npts = overlay->lines[k].np;

	     if(npts > 0 && stack->order[k] >= 0) {

	        x2 = overlay->lines[k].points[npts-1][0];
	        y2 = overlay->lines[k].points[npts-1][1];

                for(l=0; l<npts; l++) {

		   x1 = x2;
		   y1 = y2;

		   x2 = overlay->lines[k].points[l][0];
		   y2 = overlay->lines[k].points[l][1];

		   if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
		       (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) { 
		   } else drawLine(flag, x1, y1, x2, y2, 1, "LineSolid");
	    	}
	     }
	  }
    } 
}

/******************/
void drawOverlay(int flag, int j)
/******************/
{
    int nv;
    for(nv=0; nv<currentViews->numOfViews; nv++) 
	drawOverlayForView(flag, j, nv);
}

/******************/
void drawOverlayForView(int flag, int j, int nv)
/******************/
{
    int i,lineColor;
    double d;

    if(currentStacks == NULL || currentViews == NULL) return;

    if(j < 0 || j >= currentStacks->numOfStacks) return; 

    i = currentStacks->stacks[j].coil;
    if((P_getreal(GLOBAL, "mshowoverlap", &d, 1))) d = 1.0;
    if(mcoils > 1 && d < 0.5 && i != getCoil()) {
	if(j == activeStack) activeStack = -1;
	return; 
    }

    iplan_2Dstack *overlay = &(currentOverlays->overlays[nv].stacks[j]);
    if(!intersected(overlay)) lineColor=offPlaneColor;
    else lineColor=onPlaneColor; 

	/* draw lines behind the screen, then draw intersection, then draw lines */
	/* in the front. sign = 1 behind, sign = -1 front, sign = 0 both */

    	if(drawIntersection) drawInterSection(flag, overlay, j, nv);
    	if(draw3DStack) 
	  draw3DBox(&(currentStacks->stacks[j]),&(currentViews->views[nv]),lineColor,1);
    	if(drawStackAxes) 
	  drawFOVlabel(&(currentStacks->stacks[j]),&(currentViews->views[nv]), axisColor);
	if(mcoils > 1) drawStackOrders(flag, j, nv);
	if(showStackOrders) drawSliceOrders(flag, j, nv);
}

/******************/
void drawNonFixedOverlays(int nv, int flag)
/******************/
{
    int i;

    if(activeStack >= 0 && activeStack < currentStacks->numOfStacks) {
        drawOverlayForView(flag, activeStack, nv);
	if(activeSlice >= 0 && activeSlice <currentStacks->stacks[activeStack].ns)
        drawActiveSlice(flag, nv);
        else if(nv == activeView) drawHandles(flag);
    }

    for(i=0; i<numOfMarks; i++) 
	if(nv == marks[i].view) drawMarks(flag, i);
}

/******************/
void drawActiveOverlayForView(int flag, int nv)
/******************/
{
    int x0, y0, wd, ht;

    if(currentStacks == NULL || currentViews == NULL ||
	nv < 0 || nv >= currentViews->numOfViews) return;

/*
    x0 = currentViews->views[nv].framestx - 2;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty - 2;
    wd = currentViews->views[nv].framewd + 4;
    ht = currentViews->views[nv].frameht + 4;
*/
    x0 = currentViews->views[nv].framestx;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty;
    wd = currentViews->views[nv].framewd;
    ht = currentViews->views[nv].frameht;

    if (isCliped != nv) {
        aip_setRegion(x0, y0, wd, ht);
        isCliped = nv;
    }
    copy_from_pixmap2(x0, y0, x0, y0, wd, ht);
    
/* draw on canvasPixmap */
    flag = 1;
    drawNonFixedOverlays(nv, flag);

/* copy to xid */
    if(getWin_draw()) {
	copy_aipmap_to_window(x0, y0, x0, y0, wd, ht);
    }
}

/******************/
void drawActiveOverlay(int flag, int j)
/******************/
{
    int nv;
    if(currentStacks == NULL || currentViews == NULL) return;

    if(j < 0 || j >= currentStacks->numOfStacks) return; 

    flag = 1;
    activeStack = j;

    for(nv=0; nv<currentViews->numOfViews; nv++) {
      drawActiveOverlayForView(flag, nv);
    }
    isCliped = -1;
    aip_setRegion(0, 0, 0, 0);
}

/******************/
void drawFixedOverlays(int nv, int flag)
/******************/
{
    int i;
    if(currentStacks == NULL || currentViews == NULL ||
	nv < 0 || nv >= currentViews->numOfViews) return;


    for(i=0; i<currentStacks->numOfStacks; i++) {
	if(i != activeStack) {
	  drawOverlayForView(flag, i, nv);
	}
    }
}

/******************/
void drawOverlaysForView(int nv)
/******************/
{
    int flag = 1;
    int setClip = 0;
    int x0, y0, wd, ht;

    if(!aipOwnsScreen() || currentStacks == NULL || currentViews == NULL || 
	nv < 0 || nv >= currentViews->numOfViews) return;
/*
    x0 = currentViews->views[nv].framestx - 2;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty - 2;
    wd = currentViews->views[nv].framewd + 4;
    ht = currentViews->views[nv].frameht + 4;
*/
    x0 = currentViews->views[nv].framestx;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty;
    wd = currentViews->views[nv].framewd;
    ht = currentViews->views[nv].frameht;

    aip_clear_pixmap(x0, y0, wd, ht);

    aipUnregisterDisplayListener(update_iplan); 
    aipRefreshImage(currentViews->ids[nv]);
    aipRegisterDisplayListener(update_iplan);

    if (isCliped != nv) {
        aip_setRegion(x0, y0, wd, ht);
        isCliped = nv;
        setClip = 1;
    }

// draw csi wire mesh first before drawing any plane overlay. 
// drawCSI3DMesh will do nothing if no csi planType.
    drawCSI3DMesh(nv, meshColor, 0);
    drawFixedOverlays(nv, flag);

    copy_to_pixmap2(x0, y0, x0, y0, wd, ht);
    drawNonFixedOverlays(nv, flag);
    if (setClip) {
        aip_setRegion(0, 0, 0, 0);
        isCliped = -1;
    }

/* copy to xid */
    if(getWin_draw()) {
        copy_aipmap_to_window(x0, y0, x0, y0, wd, ht);
    }
}

/******************/
void drawOverlays()
/******************/
{
    int i;

    if(!aipOwnsScreen() || currentStacks == NULL || currentViews == NULL) return;

    for(i=0; i<currentViews->numOfViews; i++) {
	drawOverlaysForView(i);
    }
}

/******************/
int getColor_m(int coil)
/******************/
{
    int view = 0;
    if(activeView > 0) view = activeView;

    pen = CYAN;

    if(coil < 1 && currentViews != NULL && view < currentViews->numOfViews) {
        coil = currentViews->views[view].coil; 
    }

    if(coil == 2) pen = MAGENTA;
    else if(coil == 3) pen = GREEN;
    else if(coil == 4) pen = BLUE;

    return(pen);
}

void addVoxel(float x, float y) {

    float3 c;
    float lpe, lro, lpe2, thk;
    int nslices, i;

    c[0] = x;
    c[1] = y;
    c[2] = 0.0;

    /* transform mark position to magnet system */
    transform(currentViews->views[activeView].p2m, c);

    getDefaults(CURRENT, VOXEL, &nslices, &lpe, &lro, &lpe2, &thk);

    currentStacks->numOfStacks += 1;
    currentStacks->order = (int*)realloc(currentStacks->order,
		sizeof(int)*(currentStacks->numOfStacks));

    for(i=0; i<currentStacks->numOfStacks; i++) {
       currentStacks->order[currentStacks->numOfStacks-1] = i+1;
    }
    currentStacks->stacks = (iplan_stack *)realloc(currentStacks->stacks, 
		sizeof(iplan_stack)*(currentStacks->numOfStacks));

    make1pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]), 
	    VOXEL, nslices, lpe, lro, lpe2, thk, c);

    activeStack = currentStacks->numOfStacks-1;
    resetActive();
    updateType();
    sendParamsByOrder(ALLSTACKS);

    calOverlays();

    drawOverlays();

}

/******************/
int addAstack(int type)
/******************/
{
    int i, view1, view2, view3;
    float3 p1, p2, p3, c;
    float size, x, y, angle;
    int nslices;
    float lpe, lro, lpe2, thk;
    int coil = 1;

    if(currentStacks == NULL || currentViews == NULL) {
        if(startIplan0(-1) == -9) RETURN;
    }

    defaultType = getIplanType();
    if(type < 0) type = defaultType;

    getDefaults(CURRENT, type, &nslices, &lpe, &lro, &lpe2, &thk);

   coil = getCoil(); 
/* remove stacks of other types */
    if(type == REGULAR) {
	for(i=0; i<currentStacks->numOfStacks; i++) {
	   if(currentStacks->stacks[i].coil == coil &&
	      (currentStacks->stacks[i].type == RADIAL ||
//	      currentStacks->stacks[i].type == VOLUME ||
	      currentStacks->stacks[i].type == REGULAR)) {
    		deleteStack(i);
		i--;
	   }
	}
    } else if(type == RADIAL) {
	for(i=0; i<currentStacks->numOfStacks; i++) {
	   if(currentStacks->stacks[i].coil == coil &&
	     (currentStacks->stacks[i].type == REGULAR ||
	      currentStacks->stacks[i].type == RADIAL ||
	      currentStacks->stacks[i].type == VOLUME)) {
    		deleteStack(i);
		i--;
	   }
	}
    } else if(type == VOLUME) {
	for(i=0; i<currentStacks->numOfStacks; i++) {
	   if(currentStacks->stacks[i].coil == coil &&
	     (currentStacks->stacks[i].type == RADIAL ||
//	      currentStacks->stacks[i].type == REGULAR ||
	      currentStacks->stacks[i].type == VOLUME)) {
    		deleteStack(i);
		i--;
	   }
	}
    }

    if(currentStacks->numOfStacks == 0) pen = MAGENTA;

    currentStacks->numOfStacks += 1;
   
    /* realloc and assign order */

    currentStacks->order = (int*)realloc(currentStacks->order,
		sizeof(int)*(currentStacks->numOfStacks));

    for(i=0; i<currentStacks->numOfStacks; i++) {
       currentStacks->order[currentStacks->numOfStacks-1] = i+1;
    }
    //currentStacks->order[currentStacks->numOfStacks-1] = coil;
 
    currentStacks->stacks = (iplan_stack *)
	realloc(currentStacks->stacks, 
		sizeof(iplan_stack)*(currentStacks->numOfStacks));
/*
    if(type == VOXEL)
    P_setstring(CURRENT, "vorient", "trans", 1);
    else if(type == SATBAND)
    P_setstring(CURRENT, "sorient", "trans", 1);
    else
    P_setstring(CURRENT, "orient", "trans", 1);
*/
    if(getNumOfNewMarks() == 0) {
        //if(currentStacks->numOfStacks == 1 || activeView < 0 || 
/* center of scout.
        if(activeView < 0 || 
		activeView >= currentViews->numOfViews) activeView = 0;
        c[0] = currentViews->views[activeView].pixstx 
		+ (currentViews->views[activeView].pixwd)/2;
        c[1] = currentViews->views[activeView].pixsty 
		- (currentViews->views[activeView].pixht)/2;
        c[2] = 0;
	// transform mark position to magnet system 
	transform(currentViews->views[activeView].p2m, c);
*/
	c[0]=0;
	c[1]=0;
	c[2]=0;

	if(getMcoils() > 1) 
        make1pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]), type, nslices, lpe, lro, lpe2, thk, c);
	else
        make1pointStack_default(&(currentStacks->stacks[currentStacks->numOfStacks-1]), type, nslices, lpe, lro, lpe2, thk, c);
    } else if(getNumOfNewMarks() == 1) {

  	i = getNewMark(1);

        view1 = marks[i].view;
	activeView = view1;
      	c[0] = marks[i].currentLocation[0];
      	c[1] = marks[i].currentLocation[1];
	c[2] = 0.0;

	/* transform mark position to magnet system */
	transform(currentViews->views[view1].p2m, c);

        make1pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]), 
	    type, nslices, lpe, lro, lpe2, thk, c);

    } else if(getNumOfNewMarks() == 2) {
  	i = getNewMark(1);
        view1 = marks[i].view;
	activeView = view1;
      	x = marks[i].currentLocation[0];
      	y = marks[i].currentLocation[1];
      	p1[0] = x;
      	p1[1] = y;
	p1[2] = 0.0;
	transform(currentViews->views[view1].p2m, p1);

  	i = getNewMark(2);
        view2 = marks[i].view;
      	p2[0] = marks[i].currentLocation[0];
      	p2[1] = marks[i].currentLocation[1];
	x = p2[0] - x;
	y = p2[1] - y;
	p2[2] = 0.0;
	transform(currentViews->views[view2].p2m, p2);

        if(view1 == view2) {
/* works only if the marks are on the same view */

	c[0] = (p1[0] + p2[0])/2.0;
    	c[1] = (p1[1] + p2[1])/2.0;
    	c[2] = (p1[2] + p2[2])/2.0;

	if(x == 0.0) angle = D90;
	angle = R2D*atan2(y,x); 

        size = distance3(p1, p2)*5.0/4.0;
       
	make2pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]),
            type, nslices, lpe, lro, lpe2, thk, angle,c,size);

	} else {

        p3[0] = marks[i].currentLocation[0];
        p3[1] = marks[i].currentLocation[1];
        p3[2] = currentViews->views[view2].pixwd;
        transform(currentViews->views[view2].p2m, p3);

        c[0] = (p1[0] + p2[0])/2.0;
        c[1] = (p1[1] + p2[1])/2.0;
        c[2] = (p1[2] + p2[2])/2.0;

        size = distance3(p1, p2)*5.0/4.0;

        make3pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]),
            type, nslices, lpe, lro, lpe2, thk, p1,p2,p3,c,size);

	}
    } else if(getNumOfNewMarks() == 3) {
  	i = getNewMark(1);
        view1 = marks[i].view;
	activeView = view1;
      	p1[0] = marks[i].currentLocation[0];
      	p1[1] = marks[i].currentLocation[1];
	p1[2] = 0.0;
	transform(currentViews->views[view1].p2m, p1);

  	i = getNewMark(2);
        view2 = marks[i].view;
      	p2[0] = marks[i].currentLocation[0];
      	p2[1] = marks[i].currentLocation[1];
	p2[2] = 0.0;
	transform(currentViews->views[view2].p2m, p2);

  	i = getNewMark(3);
        view3 = marks[i].view;
      	p3[0] = marks[i].currentLocation[0];
      	p3[1] = marks[i].currentLocation[1];
	p3[2] = 0.0;
	transform(currentViews->views[view3].p2m, p3);

	c[0] = (p1[0] + p2[0] + p3[0])/3.0;
    	c[1] = (p1[1] + p2[1] + p3[1])/3.0;
    	c[2] = (p1[2] + p2[2] + p3[2])/3.0;

        size = max(distance3(p1, p2), distance3(p2, p3));
        size = max(size, distance3(p1, p3))*5.0/4.0;

	if(view1 == view2 && view1 == view3) {
	    lpe = max(lpe, size); 
	    lro = max(lro, size); 
	    lpe2 = max(lpe2, size); 
	    makeAstackByEuler(&(currentStacks->stacks[currentStacks->numOfStacks-1]), 
	    type, nslices, lpe, lro, lpe2, thk, c, 
		currentViews->views[view3].slice.theta, 
		currentViews->views[view3].slice.psi, 
		currentViews->views[view3].slice.phi);
	} else
	    make3pointStack(&(currentStacks->stacks[currentStacks->numOfStacks-1]),
            type, nslices, lpe, lro, lpe2, thk, p1,p2,p3,c,size);
    }
    activeStack = currentStacks->numOfStacks-1;
    resetActive();
    updateDefaultType();
    updateType();
    sendParamsByOrder(ALLSTACKS);

    calOverlays();

    drawOverlays();

    for(i=0; i<numOfMarks; i++) {
	if(marks[i].isUsing)
	    getLocation2(marks[i].currentLocation, &(marks[i].view),
                &(marks[i].stack), &(marks[i].slice));
    }

    RETURN;
}

/******************/
void resetActive()
/******************/
{
    if(currentStacks == NULL) return;

    //if(activeStack < 0 || activeStack >= currentStacks->numOfStacks)
          activeStack = currentStacks->numOfStacks-1;

    handleMode = SQUARE;
    activeHandle = -1;
    activeSlice = -1;
    activeVertex1 = -1;
    activeVertex2 = -1;
}

/******************/
int removeAstack(int ind)
/******************/
{
    if(orderMode) RETURN; 

    if(currentStacks == NULL || currentViews == NULL) RETURN;

    if(ind < 0 || ind >= currentStacks->numOfStacks) RETURN; 

    if(ind <= activeStack) activeStack--;

    deleteStack(ind);

    resetActive();
    updateType();
    updateDefaultType();
    sendParamsByOrder(ALLSTACKS);

    drawOverlays();

    RETURN;
}

/******************/
void deleteSelected(int type)
/******************/
{
    int i,ind;
    if(currentStacks == NULL || currentStacks->numOfStacks < 1) return;

    if(activeSlice != -1) {
	deleteSlice();
	return;
    }
 
    ind = selectedOverlay;
    if(type < 0) {
      if(ind < 0 || ind >= currentStacks->numOfStacks)
	ind = currentStacks->numOfStacks-1; 
    } else {
      if(!(ind >=0 && ind < currentStacks->numOfStacks &&
	currentStacks->stacks[ind].planType == type)) {
          ind=-1;
          for(i=currentStacks->numOfStacks-1;i>=0;i--) {
            if(currentStacks->stacks[i].planType == type) {
		ind=i;
		break;
	    }
	  }
	}
    }
    if(ind<0) return;

    removeAstack(ind);
    selectedOverlay=currentStacks->numOfStacks-1;
    P_setreal(CURRENT, "planSs", (double)(selectedOverlay+1), 1);
    appendJvarlist("planSs");
    // writelineToVnmrJ("pnew", "1 planSs");
}

void addType(int type, char *orient)
{
   int i,n,found;
   vInfo           paraminfo;
   double d;
   planParams *tag = getCurrPlanParams(type);
   
   if(!(P_getVarInfo(CURRENT, "iplanDefaultType", &paraminfo))) {
        n = paraminfo.size;
   } else {
        createCommonPlanParams();
        n=1;
   } 
   
   if(currentStacks == NULL || currentStacks->numOfStacks < 1) { // no plan is displayed.
     clearVar("iplanDefaultType");
     n=0;
   } 

   found=0;
   for(i=0;i<n;i++){ 
    if(!P_getreal(CURRENT, "iplanDefaultType", &d, i+1) && type == (int)d) { 
        found=1;
	break;
    }
   }
   
   if(!found)
     P_setreal(CURRENT, "iplanDefaultType",(double)type,n+1);

   if(strlen(orient) == 0)
     getCurrentStacks(); // use current plan params
   else { // use given orientation 
     if(tag != NULL) { 
	setPlanValue(tag->orient.name, orient);
     } else {
	setPlanValue("orient", orient);
     }
   }
}

void deleteType(int type)
{
   int i,j,n, types[99], found;
   vInfo           paraminfo;
   double d;
   
   if(!(P_getVarInfo(CURRENT, "iplanDefaultType", &paraminfo))) {
        n = paraminfo.size;
   } else return;	
   
   j=0;
   found=0; 
   for(i=0;i<n;i++){ 
    if(!P_getreal(CURRENT, "iplanDefaultType", &d, i+1)) {
      if(type != (int)d) {
	types[j]=d;
        j++;
      } else found++;
    }
   }
   
   if(found==0) return;
   if(j==0) {
	clearStacks();
	return;	
   }

   clearVar("iplanDefaultType");
   for(i=0;i<j;i++) 
	 P_setreal(CURRENT, "iplanDefaultType",(double)types[i],i+1);

   getCurrentStacks();
}

/******************/
void deleteStack(int ind)
/******************/
{
    int i, j;
    int order, type;

    if(currentStacks == NULL) return;

    if(ind < 0 || ind >= currentStacks->numOfStacks) return;

    order = currentStacks->order[ind];
    type = currentStacks->stacks[ind].type;

    currentStacks->numOfStacks -= 1;
    for(i=ind; i<currentStacks->numOfStacks; i++) {
	copyStack(&(currentStacks->stacks[i+1]), &(currentStacks->stacks[i]));
	currentStacks->order[i] = currentStacks->order[i+1];
    }

    freeAStack(&(currentStacks->stacks[currentStacks->numOfStacks]));

    currentStacks->stacks = (iplan_stack *)
	realloc(currentStacks->stacks, 
		sizeof(iplan_stack)*(currentStacks->numOfStacks));

    /* reorder the stacks */

    for(i=0; i<currentStacks->numOfStacks; i++) 
	if(currentStacks->order[i] > order) currentStacks->order[i] -= 1;

    for(i=0; i<currentViews->numOfViews; i++) {
        currentOverlays->overlays[i].numOfStacks -= 1;
 	for(j=ind; j<currentOverlays->overlays[i].numOfStacks; j++) {
	    copy2Dstack(&(currentOverlays->overlays[i].stacks[j+1]),
			&(currentOverlays->overlays[i].stacks[j]));
	}

 	free2Dstack(&(currentOverlays->overlays[i]
		.stacks[currentOverlays->overlays[i].numOfStacks]));

	currentOverlays->overlays[i].stacks = (iplan_2Dstack*)
	    realloc(currentOverlays->overlays[i].stacks,
		sizeof(iplan_2Dstack)*(currentOverlays->overlays[i].numOfStacks));

    }

}

/******************/
void free2Dstack(iplan_2Dstack* s)
/******************/
{
    int i;
    if(s == NULL) return;

    for(i=0; i<s->numOfStripes; i++) {
	free(s->stripes[i].points);
	free(s->lines[i].points);
	s->stripes[i].np = 0;
	s->lines[i].np = 0;
	s->stripes[i].points = NULL;
	s->lines[i].points = NULL;
    }
    free(s->stripes);
    free(s->lines);
    s->numOfStripes = 0;
    s->stripes = NULL;
    s->lines = NULL;

    free(s->envelope.points);
    free(s->handles.points);
    free(s->handles2slices);
    s->envelope.np = 0;
    s->handles.np = 0;
    s->envelope.points = NULL;
    s->handles.points = NULL;
    s->handles2slices = NULL;
}

/******************/
void copy2Dstack(iplan_2Dstack* s1, iplan_2Dstack* s2)
/******************/
{
    int i, j;

    if(s2 != NULL) free2Dstack(s2);

    if(s1 == NULL || s1->numOfStripes <= 0) return;

    s2->numOfStripes = s1->numOfStripes;
    s2->stripes = (iplan_polygon*)malloc(sizeof(iplan_polygon)*s1->numOfStripes);
    s2->lines = (iplan_polygon*)malloc(sizeof(iplan_polygon)*s1->numOfStripes);

    for(i=0; i<s1->numOfStripes; i++) {
        s2->stripes[i].np = s1->stripes[i].np;
	s2->stripes[i].points = (float2*)
		malloc(sizeof(float2)*s1->stripes[i].np);
	for(j=0; j<s1->stripes[i].np; j++) {
	    s2->stripes[i].points[j][0] = s1->stripes[i].points[j][0];
	    s2->stripes[i].points[j][1] = s1->stripes[i].points[j][1];
	}	

        s2->lines[i].np = s1->lines[i].np;
	s2->lines[i].points = (float2*)
		malloc(sizeof(float2)*s1->lines[i].np);
	for(j=0; j<s1->lines[i].np; j++) {
	    s2->lines[i].points[j][0] = s1->lines[i].points[j][0];
	    s2->lines[i].points[j][1] = s1->lines[i].points[j][1];
	}	
    }

    if(s1->envelope.points != NULL) {

      s2->envelope.np = s1->envelope.np;
      s2->envelope.points = (float2*)
                malloc(sizeof(float2)*s1->envelope.np);
      for(j=0; j<s1->envelope.np; j++) {
        s2->envelope.points[j][0] = s1->envelope.points[j][0];
        s2->envelope.points[j][1] = s1->envelope.points[j][1];
      }
    }

    if(s1->handles.points != NULL) {

      s2->handles.np = s1->handles.np;
      s2->handles.points = (float2*)
                malloc(sizeof(float2)*s1->handles.np);
      for(j=0; j<s1->handles.np; j++) {
        s2->handles.points[j][0] = s1->handles.points[j][0];
        s2->handles.points[j][1] = s1->handles.points[j][1];
      }
    }

    if(s1->handles2slices != NULL) {
	s2->handles2slices = (int*)malloc(sizeof(int)*s1->handles.np);
	for(j=0; j<s1->handles.np; j++)
	    s2->handles2slices[j] = s1->handles2slices[j];
    }
}

/******************/
int clearStacks()
/******************/
{
    if(currentStacks == NULL || currentStacks->numOfStacks < 1 || currentViews == NULL) RETURN;

    resetActive();
/*
    updateType();
    sendParamsByOrder(ALLSTACKS);
*/
    while(currentStacks->numOfStacks > 0) deleteStack(0);

    removeAllMarks();

    drawOverlays();

    RETURN;
}

/******************/
int disStripes()
/******************/
{
    overlayStyle = BOX;

    if(currentStacks == NULL || currentViews == NULL) RETURN;
    drawOverlays();
    RETURN;
}

/******************/
int disCenterLines()
/******************/
{
    overlayStyle = CENTER;

    if(currentStacks == NULL || currentViews == NULL) RETURN;
    drawOverlays();
    RETURN;
}

void sStack2Point(int view, int stack, float2 p2, float z, float3 p3)
{
    if(view < 0 || view >= currentViews->numOfViews) return;
    if(stack < 0 || stack >= currentStacks->numOfStacks) return;

    p3[0] = p2[0]; 
    p3[1] = p2[1]; 
    p3[2] = z; 

    transform(currentStacks->stacks[stack].u2m, p3);
    transform(currentViews->views[view].m2p, p3);
}

/******************/
void sPoint2Stack(int view, int stack, float2 p2, float z, float3 p3)
/******************/
{
/*
    float3 origin;
 */

    if(view < 0 || view >= currentViews->numOfViews) return;
    if(stack < 0 || stack >= currentStacks->numOfStacks) return;

    p3[0] = p2[0]; 
    p3[1] = p2[1]; 
    p3[2] = z; 

    transform(currentViews->views[view].p2m, p3);
    transform(currentStacks->stacks[stack].m2u, p3);
/*
    origin[0] = currentStacks->stacks[stack].ppe;
    origin[1] = currentStacks->stacks[stack].pro;
    origin[2] = currentStacks->stacks[stack].pss0;

    rotatem2u(p3, currentStacks->stacks[stack].orientation);

    translatem2u(p3, origin);
*/
}

/******************/
void updateSlice(float2 p, float2 prevP)
/******************/
{
    int fixedMarks;
    float3 p3, prevPoint, sCenter;
    float angle;
    float2 center, p1, p2;

    if(activeView < 0 || activeView >= currentViews->numOfViews ||
	activeStack < 0 || activeStack >= currentStacks->numOfStacks) return;

    /* move slice */

    fixedMarks = countFixedMarksForSlice(activeSlice);
    if(fixedMarks == 0) {

        sPoint2Stack(activeView, activeStack, p, 0.0, p3);
    	sPoint2Stack(activeView, activeStack, prevP, 0.0, prevPoint);

	if(currentStacks->stacks[activeStack].type == REGULAR) {
	    currentStacks->stacks[activeStack].pss[activeSlice][1] 
		+= p3[2] - prevPoint[2];
	} else if(currentStacks->stacks[activeStack].type == RADIAL) {

            sCenter[0] = 0.0;
            sCenter[1] = 0.0;
            sCenter[2] = 0.0;
            angle = currentStacks->stacks[activeStack].radialAngles[activeSlice][0]
                        +currentStacks->stacks[activeStack].radialAngles[activeSlice][1];
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);

	    center[0] = sCenter[1];
	    center[1] = sCenter[2];
	    p1[0] = prevPoint[1];
	    p1[1] = prevPoint[2];
	    p2[0] = p3[1];
	    p2[1] = p3[2];
	    currentStacks->stacks[activeStack].radialAngles[activeSlice][1] 
		-= calAngle(center, p2, p1);
	}

        calTransMtx(&(currentStacks->stacks[activeStack]));
        calSliceXYZ(&(currentStacks->stacks[activeStack]));
        calOverlayForAllViews(activeStack);
        drawActiveOverlay(0,activeStack);
    }
}

/******************/
void rotateVZ(float3 cor, float angle)
/******************/
{
    float zMatrix[9];

    angle *= D2R;

    	zMatrix[0] = cos(angle);
    	zMatrix[1] = sin(angle);
    	zMatrix[2] = 0.0;
    	zMatrix[3] = -sin(angle);
    	zMatrix[4] = cos(angle);
    	zMatrix[5] = 0.0;
    	zMatrix[6] = 0.0;
    	zMatrix[7] = 0.0;
    	zMatrix[8] = 1.0;
      
	/* changed from rotatem2u to rotateu2m when y is inverted */
    	rotateu2m(cor, zMatrix);
}

/******************/
void trimAngle(float* angle)
/******************/
{
    int i;
    i = *angle;
    *angle = i;
}

// rCenterOk=0 if rotation center (rCenterX and rCenterY)  will be recalculated.
// rCenterOk=1 if given rotation center (rCenterX and rCenterY) will be used. 
/******************/
void updateRotation0(int istack, float2 p, float2 prevP, int rCenterOk, float rCenterX, float rCenterY)
/******************/
{
    float2 rCenter;
    float3 cCenter, sCenter;
    float z, angle;
    int i;
    iplan_stack *stack;

    if(activeView <0 || activeView >= currentViews->numOfViews ||
	istack < 0 || istack >= currentStacks->numOfStacks) return;

    stack = &(currentStacks->stacks[istack]);

    // return if change of euler angles or orient is disabled
    planParams *tag = getCurrPlanParams(stack->planType);
    if(tag != NULL) {
	if(tag->orient.use < 0 || tag->theta.use < 0 || 
		tag->psi.use < 0 || tag->phi.use < 0) return;
    }

    // We don't allow CSIVOXEL be rotated by mouse button.
    // rCenterOk=0 if called by mouse button.
    if(!rCenterOk && stack->planType == CSIVOXEL) { 
      for(i=0;i<currentStacks->numOfStacks;i++)
	if(currentStacks->stacks[i].planType == CSI2D ||
		currentStacks->stacks[i].planType == CSI3D) return;
    }

    if(!rCenterOk) rCenterOk = getRotationCenter(rCenter, activeView, istack) != -1; 
    else {
	rCenter[0]=rCenterX;
	rCenter[1]=rCenterY;
    }
    if(rCenterOk) {

    	/* calculate new center */
	/* getStackCenter uses current orientation of the stack */
	/* so has to be called before update to the new orientation*/ 

    	getStackCenter(cCenter, activeView, istack);

    	z = cCenter[2];
/*
	if(aipOwnsScreen() && IBlen > 0) {

	  angle = calAngle(rCenter, p, prevP);

    	  cCenter[0] = -(cCenter[0] - rCenter[0]);
    	  cCenter[1] = cCenter[1] - rCenter[1];
    	  cCenter[2] = 0.0;

    	  rotateVZ(cCenter, angle);

    	  rCenter[0] = rCenter[0] - cCenter[0];
    	  rCenter[1] = cCenter[1] + rCenter[1];

	  sCenter[0] = rCenter[0];
	  sCenter[1] = rCenter[1];
	  sCenter[2] = z;
	} else {
*/
	  angle = calAngle(rCenter, prevP, p);

    	  cCenter[0] = cCenter[0] - rCenter[0];
    	  cCenter[1] = cCenter[1] - rCenter[1];
    	  cCenter[2] = 0.0;

    	  rotateVZ(cCenter, angle);

    	  sCenter[0] = cCenter[0] + rCenter[0];
    	  sCenter[1] = cCenter[1] + rCenter[1];
	  sCenter[2] = z;
/*
	}
*/
        transform(currentViews->views[activeView].p2m, sCenter);

    	/*calculate new orientation */

    	rotateZ(stack->orientation,
	    currentViews->views[activeView].slice.orientation, angle);

/*  to keep tensor in valid form after a rotation, */
/*  calculate euler and recalculate the tensor. */

    	tensor2euler(&(stack->theta), 
		     &(stack->psi), 
		     &(stack->phi), 
		      stack->orientation);
    	euler2tensor((stack->theta), 
		     (stack->psi), 
		     (stack->phi), 
		      stack->orientation);

    	/* important: the center is rotated to the new orientation!!! */

    	rotatem2u(sCenter, stack->orientation);

    	tensor2euler(&(stack->theta),
		&(stack->psi),	
		&(stack->phi),	
		stack->orientation);

	if(stack->type == SATBAND) {

    	    stack->pss0 = sCenter[2];

	} else if(stack->type == VOXEL) {

    	    stack->ppe = sCenter[0];
    	    stack->pro = sCenter[1];
    	    stack->pss0 = sCenter[2];

	} else if(stack->type == RADIAL) {

    	    stack->ppe = usepos2*sCenter[0];
    	    stack->pro = usepos1*sCenter[1];
    	    stack->pss0 = usepos3*sCenter[2];

	} else {

    	    stack->ppe = usepos2*sCenter[0];
    	    stack->pro = usepos1*sCenter[1];
    	    stack->pss0 = usepos3*sCenter[2];

	}

        calTransMtx(stack);
    	calSliceXYZ(stack);
        calOverlayForAllViews(istack);

	if(stack->planType == CSI2D ||
		stack->planType == CSI3D) {
	  int i;
	  for(i=0;i<currentStacks->numOfStacks;i++)
	     if(currentStacks->stacks[i].planType == CSIVOXEL)
		updateRotation0(i, p, prevP,1,rCenter[0],rCenter[1]);
	} 
    }
}

// this is for mult mouse only.
/******************/
void updateOtherOverlays(int act)
/******************/
{
    int i, j;
    iplan_stack *stack;
    iplan_stack *astack;

    if(mcoils <= 1) return;

    astack = &(currentStacks->stacks[act]);

// for the same coil, only euler can be different

    for(i=0; i<currentStacks->numOfStacks; i++) {

     //if(currentStacks->order[i] != currentStacks->order[act]) continue;

     stack = &(currentStacks->stacks[i]);
     if(i != act && stack->type == astack->type) {
	stack->gap = astack->gap; 
	stack->lro = astack->lro; 
	stack->lpe = astack->lpe; 
	stack->lpe2 = astack->lpe2; 
	stack->thk = astack->thk; 
	if(stack->ns == astack->ns) {
           for(j=0; j<astack->ns; j++) {
		stack->pss[j][0] = astack->pss[j][0];
		stack->pss[j][1] = astack->pss[j][1];
	   }
        }
        calTransMtx(stack);
        calSliceXYZ(stack);
        calOverlayForAllViews(i);
     }
    }
     
    drawOverlays();
}

/******************/
void updateRotation(float2 p, float2 prevP, int rCenterOk, float rCenterX, float rCenterY)
/******************/
{
     updateRotation0(activeStack, p, prevP, rCenterOk, rCenterX, rCenterY);
     //drawActiveOverlay(0, activeStack);
     drawOverlays();
}

/******************/
void updateRotationAll(float2 p, float2 prevP, int rCenterOk, float rCenterX, float rCenterY)
/******************/
{
    int i;
    float2 p1, p2;

    for(i=0; i<currentStacks->numOfStacks; i++) {
   	p1[0] = p[0];
	p1[1] = p[1];
	p2[0] = prevP[0];
	p2[1] = prevP[1];
        updateRotation0(i, p1, p2, rCenterOk, rCenterX, rCenterY);
    }

    drawOverlays();
}

/******************/
void sendPnew0(int i, const char *str)
/******************/
{
    char strValue[MAXSTR];
    if(str[0] == ' ') sprintf(strValue, "%d%s", i, str);
    else sprintf(strValue, "%d %s", i, str);
    appendJvarlist(str);
    // writelineToVnmrJ("pnew", strValue);
}

/******************/
void updateRad(float2 p, float2 prevP)
/******************/
{
    float3 p3, prevPoint;

    if(activeView <0 || activeView >= currentViews->numOfViews ||
        activeStack < 0 || activeStack >= currentStacks->numOfStacks) return;

    if(currentStacks->stacks[activeStack].type != RADIAL) return;

    sPoint2Stack(activeView, activeStack, p, 0.0, p3);
    sPoint2Stack(activeView, activeStack, prevP, 0.0, prevPoint);

    currentStacks->stacks[activeStack].radialShift += p3[1] - prevPoint[1];

    calTransMtx(&(currentStacks->stacks[activeStack]));
    calSliceXYZ(&(currentStacks->stacks[activeStack]));
    calOverlayForAllViews(activeStack);
    drawActiveOverlay(0, activeStack);
}

/******************/
void updateTranslation0(int istack, float2 p, float2 prevP, int n) 
/******************/
{
    float3 p3, prevPoint;
    float z;
    int i;
    iplan_stack *stack;

    if(activeView <0 || activeView >= currentViews->numOfViews ||
	istack < 0 || istack >= currentStacks->numOfStacks) return;

    stack = &(currentStacks->stacks[istack]);

    planParams *tag = getCurrPlanParams(stack->planType);
    if(tag != NULL) {
	if(tag->pos1.use < 0 || tag->pos2.use < 0 || tag->pos3.use < 0) return;
    }

    if(n == 1 && activeMark == -1 
	&& countFixedMarksForStack(istack) != 0) return;
    if(is3Plane(currentStacks, stack->planType)) return;

    if(stack->planType == CSIVOXEL) {
        for(i=0; i<currentStacks->numOfStacks; i++) {
	  if(currentStacks->stacks[i].planType == CSI2D) {
    	     sPoint2Stack(activeView, i, p, 0.0, p3);
    	     sPoint2Stack(activeView, i, prevP, 0.0, prevPoint);
	     p[0] = p3[0];
	     p[1] = p3[1];
	     z = prevPoint[2]; 
    	     sStack2Point(activeView, i, p, z, p3);
	     p[0] = p3[0];
	     p[1] = p3[1];
	     break;
	  }
        }     	
    }

    sPoint2Stack(activeView, istack, p, 0.0, p3);
    sPoint2Stack(activeView, istack, prevP, 0.0, prevPoint);

    if(stack->type == SATBAND) {
    	stack->pss0 += p3[2] - prevPoint[2];

    } else if(stack->planType == VOXEL) {
    	stack->ppe += p3[0] - prevPoint[0];
    	stack->pro += p3[1] - prevPoint[1];
    	stack->pss0 += p3[2] - prevPoint[2];

    } else if(stack->planType == CSIVOXEL) {
    	stack->ppe += p3[0] - prevPoint[0];
    	stack->pro += p3[1] - prevPoint[1];
    	stack->pss0 += p3[2] - prevPoint[2];

    } else if(stack->type == RADIAL) {
    	stack->ppe += usepos2*(p3[0] - prevPoint[0]);
    	stack->pro += usepos1*(p3[1] - prevPoint[1]);
    	stack->pss0 += usepos3*(p3[2] - prevPoint[2]);

    } else {
    	stack->ppe += usepos2*(p3[0] - prevPoint[0]);
    	stack->pro += usepos1*(p3[1] - prevPoint[1]);
    	stack->pss0 += usepos3*(p3[2] - prevPoint[2]);
    }
	    
    calTransMtx(stack);
    calSliceXYZ(stack);
    calOverlayForAllViews(istack);

    if(stack->planType == CSI2D ||
	stack->planType == CSI3D) { 
	float3 oldpos, oldeuler, newpos, neweuler;
	oldpos[0] = usepos2*(p3[0] - prevPoint[0]);
	oldpos[1] = usepos1*(p3[1] - prevPoint[1]);
	oldpos[2] = usepos3*(p3[2] - prevPoint[2]);
	oldeuler[0] = stack->theta;
	oldeuler[1] = stack->psi;
	oldeuler[2] = stack->phi;
        for(i=0; i<currentStacks->numOfStacks; i++) {
	   if(currentStacks->stacks[i].planType == CSIVOXEL) {
		neweuler[0] = currentStacks->stacks[i].theta;
		neweuler[1] = currentStacks->stacks[i].psi;
		neweuler[2] = currentStacks->stacks[i].phi;
	     	getNewPos(oldeuler, oldpos, neweuler, newpos);
    		currentStacks->stacks[i].ppe += newpos[0];
    		currentStacks->stacks[i].pro += newpos[1];
    		currentStacks->stacks[i].pss0 += newpos[2];
    		calTransMtx(&(currentStacks->stacks[i]));
    		calSliceXYZ(&(currentStacks->stacks[i]));
    		calOverlayForAllViews(i);
	   }
	}
    }
}

/******************/
void updateTranslationAll(float2 p, float2 prevP) 
/******************/
{
    int i;
    float2 p1, p2, newp;

    for(i=0; i<currentStacks->numOfStacks; i++) {
   	p1[0] = p[0];
	p1[1] = p[1];
	p2[0] = prevP[0];
	p2[1] = prevP[1];
	updateTranslation0(i, p1, p2, currentStacks->numOfStacks);
    }

    for(i=0; i<numOfMarks; i++) {
	if(marks[i].isUsing) {
	    getTranslation(activeView, p, prevP, 
			marks[i].view, newp); 
	    marks[i].currentLocation[0] += newp[0];
	    marks[i].currentLocation[1] += newp[1];
	}
    }

    drawOverlays();
}

/******************/
void updateTranslation(int stack, float2 p, float2 prevP) 
/******************/
{
    updateTranslation0(stack, p, prevP, 1);

    //drawActiveOverlay(0, stack);
    drawOverlays();
}

/******************/
void makeDragPerpendicularToEdge(float2 p, float2 prevP, float2 v1, float2 v2)
/******************/
{
    float delta = 2.0;

    if(fabs(v1[0] - v2[0]) < delta) p[1] = prevP[1];
    else if(fabs(v1[1] - v2[1]) < delta) p[0] = prevP[0];
    else p[1] = (p[0]-prevP[0])*(v1[0]-v2[0])/(v2[1]-v1[1]) + prevP[1];
}

/******************/
void updateActiveHandle(int view, int stack)
/******************/
{
    int i;
    for(i=0; i<currentOverlays->overlays[view].stacks[stack].handles.np; i++)
        if(currentOverlays->overlays[view].stacks[stack].handles2slices[i] ==
        currentStacks->stacks[stack].ns-1) {
            activeHandle = i;
            return;
        }
}

/******************/
void updateSExpansion(float2 p, float2 prevP) 
/******************/
{
    float z, angle;
    float3 p3, prevPoint, sCenter;
    float3 dp, hp[2];
    float2 v1, v2, center, p1, p2;
    int k, slice, ns, nh = 0;
    iplan_stack *stack;
    iplan_2Dstack *overlay;

    if(activeView <0 || activeView >= currentViews->numOfViews ||
	activeStack < 0 || activeStack >= currentStacks->numOfStacks) return;

    stack = &(currentStacks->stacks[activeStack]);
    overlay = &(currentOverlays->overlays[activeView].stacks[activeStack]);

    planParams *tag = getCurrPlanParams(stack->planType);
    if(tag != NULL) {
	if(tag->dim1.use < 0 || tag->dim2.use < 0 
		|| tag->dim3.use < 0 || tag->thk.use < 0) return;
    }

    ns = stack->ns;

    if(stack->type != SATBAND &&
        activeVertex1 != -1 && activeVertex2 != -1) {
      /* makeDragPerpendicularToEdge */
      nh = 2;
      k = activeVertex1;
      v1[0] = overlay->handles.points[k][0];
      v1[1] = overlay->handles.points[k][1];
      k = activeVertex2;
      v2[0] = overlay->handles.points[k][0];
      v2[1] = overlay->handles.points[k][1];
      makeDragPerpendicularToEdge(p, prevP, v1, v2);
    }
    
    sPoint2Stack(activeView, activeStack, p, 0.0, p3);
    sPoint2Stack(activeView, activeStack, prevP, 0.0, prevPoint);

    if(nh == 2) {
      sPoint2Stack(activeView, activeStack, v1, 0.0, hp[0]);
      sPoint2Stack(activeView, activeStack, v2, 0.0, hp[1]);
    } else if(activeHandle != -1) {
      nh = 1;
      k = activeHandle;
      v1[0] = overlay->handles.points[k][0];
      v1[1] = overlay->handles.points[k][1];
      sPoint2Stack(activeView, activeStack, v1, 0.0, hp[0]);
    } else nh = 0;

    sCenter[0] = 0.0;
    sCenter[1] = 0.0;
    sCenter[2] = 0.0;

    if(stack->type == SATBAND) {
        dp[2] = fabs(p3[2]-sCenter[2]) - fabs(prevPoint[2]-sCenter[2]);
	stack->thk += 2.0*dp[2];
    } else {

	if(stack->type == RADIAL &&
		activeVertex1 == -1 && activeVertex2 == -1 && activeHandle != -1) {

        /* handle is selected */

	    slice = handle2slice(activeHandle);

            if(slice == 0 || slice == ns-1) {

	    /* rotate the points to orientation of slice 0 */
	    angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);

                /* expand the angles */

                center[0] = sCenter[1];
                center[1] = sCenter[2];
                p1[0] = prevPoint[1];
                p1[1] = prevPoint[2];
                p2[0] = p3[1];
                p2[1] = p3[2];
                if(slice == 0) angle = calAngle(center, p2, p1);
                else angle = calAngle(center, p1, p2);
                if((fixedSlice == -2 || gapFixed) && angle > 0) {
                z = (stack->ns+1)*
                    stack->gap;
                angle = stack->gap;
                } else if((fixedSlice == -2 || gapFixed) && angle < 0) {
                z = (stack->ns-3)*
                    stack->gap;
                angle = -stack->gap;
                } else
                z = (stack->ns-1)*
                    stack->gap + 2*angle;
                if(z > 0 && z < D360-stack->gap) {
                  updateZforZ(z, activeStack);

                  sCenter[0] = stack->ppe;
                  sCenter[1] = stack->pro;
                  sCenter[2] = stack->pss0;

                  rotateu2m(sCenter, stack->orientation);

                  rotateX(stack->orientation,
                        stack->orientation, -angle);

	/* when tensor is rotated multiple times, error propagation */
	/* distorts the tensor. to remain the tensor in valid form */
	/* euler angles are calculated and then the tensor is recalculated */
	/* from euler angles everything the tensor is rotated. */ 
		  tensor2euler(&(stack->theta), 
			  &(stack->psi), 
			  &(stack->phi), 
			   stack->orientation);
		  euler2tensor((stack->theta), 
			  (stack->psi), 
			  (stack->phi), 
			   stack->orientation);

		  rotatem2u(sCenter, stack->orientation);

    		  stack->ppe = usepos2*sCenter[0];
    		  stack->pro = usepos1*sCenter[1];
		  if(fixedSlice == -1)
    		  stack->pss0 = usepos3*sCenter[2];

		}

	    } else {

		/* expand lro and lpe */

            sCenter[1] += stack->radialShift;

            /* rotate the points to orientation of slice 0 */
            angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);

                dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
                dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
                stack->lpe += 2.0*dp[0];
                stack->lro += 2.0*dp[1];
            }

    	} else if(stack->type == RADIAL
		&& activeVertex1 != -1 && activeVertex2 != -1) {

	/* edge is selected */

            sCenter[1] += stack->radialShift;

            slice = min((float)handle2slice(activeVertex1),
                (float)handle2slice(activeVertex2));
            /* rotate the points to orientation of slice 0 */
            angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);
            dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
            dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
            stack->lpe += 2.0*dp[0];
            stack->lro += 2.0*dp[1];

    	} else {

    	    dp[0] = p3[0] - prevPoint[0];
    	    dp[1] = p3[1] - prevPoint[1];
    	    dp[2] = p3[2] - prevPoint[2];

            if(nh == 1) {
		if(hp[0][0] < 0) dp[0] = -dp[0];
		if(hp[0][1] < 0) dp[1] = -dp[1];
		if(hp[0][2] < 0) dp[2] = -dp[2];
	    } else if(nh == 2) {
		if(hp[0][0] < 0 || hp[1][0] < 0) dp[0] = -dp[0];
		if(hp[0][1] < 0 || hp[1][1] < 0) dp[1] = -dp[1];
		if(hp[0][2] < 0 || hp[1][2] < 0) dp[2] = -dp[2];
	    } else {
    	        dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
    	        dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
    	        dp[2] = fabs(p3[2]-sCenter[2]) - fabs(prevPoint[2]-sCenter[2]);
	    }

	    if(stack->lpe <= 0) stack->lpe = fabs(2.0*dp[0]);
	    else stack->lpe += 2.0*dp[0];
	    if(stack->lpe < 0) stack->lpe = 0;
	    if(stack->lro <= 0) stack->lro = fabs(2.0*dp[1]);
    	    else stack->lro += 2.0*dp[1];
	    if(stack->lro < 0) stack->lro = 0;

	    if(dp[2] != 0.0 && stack->ns == 1 && !gapFixed) {
	        if(stack->thk <= 0) stack->thk = fabs(2.0*dp[2]);
		else stack->thk += 2.0*dp[2];
		if(stack->thk < 0) stack->thk = 0;
	        if(stack->lpe2 <= 0) stack->lpe2 = fabs(2.0*dp[2]);
		else stack->lpe2 += 2.0*dp[2];
		if(stack->lpe2 < 0) stack->lpe2 = 0;
	    } else if(dp[2] != 0.0) {
    	      z = (stack->ns-1)*stack->gap+stack->ns*stack->thk;
	      if(z<= 0) z = fabs(2.0*dp[2]);
	      else z += 2.0*dp[2];
	      if(z < 0) z = 0;
    	      updateZforZ(z, activeStack);
              if(!gapFixed && stack->gap<0) {
		z = stack->ns*stack->thk;
		updateZforZ(z, activeStack);
	      }
	    }
	    doDependency(activeStack, dp);
    	}
    }

    calTransMtx(stack);
    calSliceXYZ(stack);
    calOverlayForAllViews(activeStack);
        if(stack->type == RADIAL &&
        stack->ns != ns &&
        (fixedSlice == -2 || gapFixed) && handle2slice(activeHandle) != 0)
        updateActiveHandle(activeView, activeStack);
    //drawActiveOverlay(0, activeStack);
    drawOverlays();
}

/******************/
void updateExpansion(float2 p, float2 prevP) 
/******************/
/* if gap fixed, increase ns, otherwise increase gap. */
{
    float z, angle;
    float3 dp, cp, hp[2];
    float3 p3, prevPoint, sCenter;
    float2 v1, v2, center, p1, p2;
    int k, slice, ns, nh=0;
    iplan_stack *stack;
    iplan_2Dstack *overlay;

    if(activeView <0 || activeView >= currentViews->numOfViews ||
	activeStack < 0 || activeStack >= currentStacks->numOfStacks) return;

    stack = &(currentStacks->stacks[activeStack]);
    overlay = &(currentOverlays->overlays[activeView].stacks[activeStack]);

    planParams *tag = getCurrPlanParams(stack->planType);
    if(tag != NULL) {
	if(tag->dim1.use < 0 || tag->dim2.use < 0 
		|| tag->dim3.use < 0 || tag->thk.use < 0) return;
    }

            ns = stack->ns;

    if(stack->type != SATBAND && 
        activeVertex1 != -1 && activeVertex2 != -1) {
      /* makeDragPerpendicularToEdge */
      nh = 2;
      k = activeVertex1;
      v1[0] = overlay->handles.points[k][0];
      v1[1] = overlay->handles.points[k][1];
      k = activeVertex2;
      v2[0] = overlay->handles.points[k][0];
      v2[1] = overlay->handles.points[k][1];
      makeDragPerpendicularToEdge(p, prevP, v1, v2);
    }
    
    sPoint2Stack(activeView, activeStack, p, 0.0, p3);
    sPoint2Stack(activeView, activeStack, prevP, 0.0, prevPoint);

    if(nh == 2) {
      sPoint2Stack(activeView, activeStack, v1, 0.0, hp[0]);
      sPoint2Stack(activeView, activeStack, v2, 0.0, hp[1]);
    } else if(activeHandle != -1) {
      nh = 1;
      k = activeHandle;
      v1[0] = overlay->handles.points[k][0];
      v1[1] = overlay->handles.points[k][1];
      sPoint2Stack(activeView, activeStack, v1, 0.0, hp[0]);
    } else nh = 0;

    sCenter[0] = 0.0;
    sCenter[1] = 0.0;
    sCenter[2] = 0.0;

    if(stack->type == SATBAND) {
        dp[2] = fabs(p3[2]-sCenter[2]) - fabs(prevPoint[2]-sCenter[2]);
        cp[2] = p3[2] - prevPoint[2];
		  if(fixedSlice == -1)
	stack->pss0 += 0.5*cp[2];
	stack->thk += dp[2];
    } else {
	if(stack->type == RADIAL &&
                activeVertex1 == -1 && activeVertex2 == -1 && activeHandle != -1) {

        /* handle is selected */

            slice = handle2slice(activeHandle);

            if(slice == 0 || slice == ns-1) {


            /* rotate the points to orientation of slice 0 */
            angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);

                /* expand the angles */

                center[0] = sCenter[1];
                center[1] = sCenter[2];
                p1[0] = prevPoint[1];
                p1[1] = prevPoint[2];
                p2[0] = p3[1];
                p2[1] = p3[2];
                if(slice == 0) angle = calAngle(center, p2, p1);
                else angle = calAngle(center, p1, p2);

                if((fixedSlice == -2 || gapFixed) && angle > 0) {
                z = (stack->ns)*stack->gap;
                angle = stack->gap;

                } else if((fixedSlice == -2 || gapFixed) && angle < 0) {
                z = (stack->ns-2)*stack->gap;
                angle = -stack->gap;
                } else
                z = (stack->ns-1)*stack->gap + angle;
                if(z > 0 && z < D360-stack->gap) {
                  updateZforZ(z, activeStack);

                  if(slice == 0) {

    		    sCenter[0] = stack->ppe;
    		    sCenter[1] = stack->pro;
    		    sCenter[2] = stack->pss0;

		    rotateu2m(sCenter, stack->orientation);

		    rotateX(stack->orientation, 
			stack->orientation, -angle);

	/* when tensor is rotated multiple times, error propagation */
	/* distorts the tensor. to remain the tensor in valid form */
	/* euler angles are calculated and then the tensor is recalculated */
	/* from euler angles everything the tensor is rotated. */ 
		    tensor2euler(&(stack->theta), &(stack->psi), 
			&(stack->phi), stack->orientation);
		    euler2tensor((stack->theta), (stack->psi), 
			  (stack->phi), stack->orientation);

		    rotatem2u(sCenter, stack->orientation);

    		    stack->ppe = usepos2*sCenter[0];
    		    stack->pro = usepos1*sCenter[1];
		  if(fixedSlice == -1)
    		    stack->pss0 = usepos3*sCenter[2];

		  }
                }

            } else {

                /* expand lro and lpe */

            sCenter[1] += stack->radialShift;

            /* rotate the points to orientation of slice 0 */
            angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);

                dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
                dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
    		cp[0] = p3[0] - prevPoint[0];
    		cp[1] = p3[1] - prevPoint[1];

                stack->ppe += usepos2*0.5*cp[0];
/*
                stack->pro += 0.5*cp[1];
*/
                stack->radialShift += 0.5*cp[1];

                stack->lpe += 1.0*dp[0];
                stack->lro += 1.0*dp[1];
            }
        } else if(stack->type == RADIAL
                && activeVertex1 != -1 && activeVertex2 != -1) {

        /* edge is selected */

            sCenter[1] += stack->radialShift;

            slice = min((float)handle2slice(activeVertex1),
                (float)handle2slice(activeVertex2));
            /* rotate the points to orientation of slice 0 */
            angle = -slice*stack->gap;
            rotateAngleAboutX(prevPoint, angle);
            rotateAngleAboutX(p3, angle);
            dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
            dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
            cp[0] = p3[0] - prevPoint[0];
            cp[1] = p3[1] - prevPoint[1];

            stack->ppe += usepos2*0.5*cp[0];
/*
            stack->pro += 0.5*cp[1];
*/
            stack->radialShift += 0.5*cp[1];

            stack->lpe += 1.0*dp[0];
            stack->lro += 1.0*dp[1];

        } else {
// not RADIAL

    	    cp[0] = p3[0] - prevPoint[0];
    	    cp[1] = p3[1] - prevPoint[1];
    	    cp[2] = p3[2] - prevPoint[2];

            if(nh == 1) {
		if(hp[0][0] > 0) dp[0] = cp[0];
		else dp[0] = -cp[0];
		if(hp[0][1] > 0) dp[1] = cp[1];
		else dp[1] = -cp[1];
		if(hp[0][2] > 0) dp[2] = cp[2];
		else dp[2] = -cp[2];
	    } else if(nh == 2) {
		if(hp[0][0] > 0 && hp[1][0] > 0) dp[0] = cp[0];
		else dp[0] = -cp[0];
		if(hp[0][1] > 0 && hp[1][1] > 0) dp[1] = cp[1];
		else dp[1] = -cp[1];
		if(hp[0][2] > 0 && hp[1][2] > 0) dp[2] = cp[2];
		else dp[2] = -cp[2];
	    } else {
    	        dp[0] = fabs(p3[0]-sCenter[0]) - fabs(prevPoint[0]-sCenter[0]);
    	        dp[1] = fabs(p3[1]-sCenter[1]) - fabs(prevPoint[1]-sCenter[1]);
    	        dp[2] = fabs(p3[2]-sCenter[2]) - fabs(prevPoint[2]-sCenter[2]);
	    }

	    if(stack->lpe <=0 ) stack->lpe = fabs(dp[0]);
            else stack->lpe += dp[0];
	    if(stack->lpe < 0) stack->lpe = 0;
	    else {
	        if(stack->type == VOXEL)
    	        stack->ppe += 0.5*cp[0];
	        else stack->ppe += usepos2*0.5*cp[0];
	    }
	    
            if(stack->lro <= 0) stack->lro = fabs(dp[1]);
            else stack->lro += dp[1];
	    if(stack->lro <= 0)  stack->lro = 0;
	    else 
    	        stack->pro += usepos1*0.5*cp[1];
	    if(dp[2] != 0.0 && stack->ns == 1 && !gapFixed) {
		if(stack->lpe2 <= 0) stack->lpe2 = fabs(dp[2]);
		else stack->lpe2 += dp[2];
		if(stack->lpe2 < 0) stack->lpe2 = 0;
		if(stack->thk <= 0) stack->thk = fabs(dp[2]);
		else stack->thk += dp[2];
		if(stack->thk < 0) stack->thk = 0;
	    } else if(dp[2] != 0.0) {
    	     z = (stack->ns-1)* stack->gap + stack->ns* stack->thk;
	     if(z <=0) z=fabs(dp[2]);
	     else z += dp[2];

	     if(stack->type == REGULAR && z <= stack->thk) z = stack->thk;
             else if(z <= 0) z = 0;
	     //else {
	/* determine the shift of the center along z */
	      if(stack->type == REGULAR 
		&& (fixedSlice == -2 || gapFixed)) {
	      	  ns = stack->ns;
              	  updateZforZ(z, activeStack);
		  if(!gapFixed && stack->gap<0) {
		     z = stack->ns* stack->thk;
              	     updateZforZ(z, activeStack);
		  }
	      	  z = fabs(stack->ns - ns)* (stack->gap + stack->thk); 
		  if(fixedSlice == -1) {
	      	    if(stack->gap > 0 && cp[2] > 0) 
    	              stack->pss0 += usepos3*0.5*z;
	      	    else if(stack->gap > 0 && cp[2] < 0)
    	              stack->pss0 += -usepos3*0.5*z;
		  }
	      } else {
              	  updateZforZ(z, activeStack);
		  if(!gapFixed && stack->gap<0) {
		     z = stack->ns* stack->thk;
              	     updateZforZ(z, activeStack);
		  }
		  if(stack->type == VOXEL ||
		 	stack->type == VOLUME) {
		     stack->pss0 += usepos3*0.5*cp[2];
		  } else if(fixedSlice == -1 && stack->gap > 0)
		  stack->pss0 += usepos3*0.5*cp[2];
	      }
	     //}
	    }
	    doDependency(activeStack, dp);
        }
    }

    calTransMtx(stack);
    calSliceXYZ(stack);
    calOverlayForAllViews(activeStack);
        if(stack->type == RADIAL &&
        stack->ns != ns &&
        (fixedSlice == -2 || gapFixed) && handle2slice(activeHandle) != 0)
        updateActiveHandle(activeView, activeStack);
    //drawActiveOverlay(0, activeStack);
    drawOverlays();
}

void doDependency(int stack, float3 dp) {
   orthDependency(stack);
   CSIDependency(stack);
   cubeDependency(stack, dp);
}

void orthDependency(int stack) {
    int i, planType;
    planType = currentStacks->stacks[stack].planType;
    if(is3Plane(currentStacks, planType)) {
       	for(i=0; i<currentStacks->numOfStacks; i++) {
   	   if(currentStacks->stacks[i].planType == planType) {
             currentStacks->stacks[i].thk = currentStacks->stacks[stack].thk;	
             currentStacks->stacks[i].gap = currentStacks->stacks[stack].gap;	
             currentStacks->stacks[i].ns = currentStacks->stacks[stack].ns;	
             currentStacks->stacks[i].lpe = currentStacks->stacks[stack].lpe;
             currentStacks->stacks[i].lro = currentStacks->stacks[stack].lro;
    	     currentStacks->stacks[i].lpe2 = 
		(currentStacks->stacks[i].ns-1)*currentStacks->stacks[i].gap
		+ currentStacks->stacks[i].ns*currentStacks->stacks[i].thk;
             currentStacks->stacks[i].ppe = 0.0;
             currentStacks->stacks[i].pro = 0.0;
             currentStacks->stacks[i].pss0 = 0.0;
	     if(i != stack) {
               updateZforZ(currentStacks->stacks[i].lpe2, i);
    	       calTransMtx(&(currentStacks->stacks[i]));
               calSliceXYZ(&(currentStacks->stacks[i]));
               calOverlayForAllViews(i);
	     }
	   }
	}
    } 
}

void CSIDependency(int stack) {
    int i;
    // note, CSI2D and CSIVOXEL may have different orientation.
    if(currentStacks->stacks[stack].planType == CSI2D) { 
    	// rotate CSIVOXEL to CSI2D orientation,
    	// make CSIVOXEL's pos[2] = CSI2D's pss0, and
    	// make CSIVOXEL's dim[2] = CSI2D's thk, then 
    	// rotate back to CSIVOXEL orientation. 
	float3 oldpos, olddim, oldeuler, newpos, newdim, neweuler;
        neweuler[0]=currentStacks->stacks[stack].theta;	
        neweuler[1]=currentStacks->stacks[stack].psi;	
        neweuler[2]=currentStacks->stacks[stack].phi;	
       	for(i=0; i<currentStacks->numOfStacks; i++) {
   	   if(currentStacks->stacks[i].planType == CSIVOXEL) {
             oldeuler[0]=currentStacks->stacks[i].theta;	
             oldeuler[1]=currentStacks->stacks[i].psi;	
             oldeuler[2]=currentStacks->stacks[i].phi;	
	     oldpos[0]=currentStacks->stacks[i].ppe;
	     oldpos[1]=currentStacks->stacks[i].pro;
	     oldpos[2]=currentStacks->stacks[i].pss0;
	     olddim[0]=currentStacks->stacks[i].lpe;
	     olddim[1]=currentStacks->stacks[i].lro;
	     olddim[2]=currentStacks->stacks[i].lpe2;
	     getNewPos(oldeuler, oldpos, neweuler, newpos);
	     getNewDim(oldeuler, olddim, neweuler, newdim);
	     newpos[2] = currentStacks->stacks[stack].pss0;
	     newdim[2] = currentStacks->stacks[stack].thk;
	     getNewPos(neweuler, newpos, oldeuler, oldpos);
	     getNewDim(neweuler, newdim, oldeuler, olddim);
    	     currentStacks->stacks[i].ppe = oldpos[0];
    	     currentStacks->stacks[i].pro = oldpos[1];
    	     currentStacks->stacks[i].pss0 = oldpos[2];
    	     currentStacks->stacks[i].lpe = olddim[0];
    	     currentStacks->stacks[i].lro = olddim[1];
    	     currentStacks->stacks[i].lpe2 = olddim[2];
	     currentStacks->stacks[i].thk = olddim[2];
    	     calTransMtx(&(currentStacks->stacks[i]));
             calSliceXYZ(&(currentStacks->stacks[i]));
             calOverlayForAllViews(i);
	   }
	}
    } else if(currentStacks->stacks[stack].planType == CSIVOXEL) {
    	// rotate CSIVOXEL to CSI2D orientation,
    	// make CSI2D's pss0 = CSIVOXEL's pos[2], and
    	// make CSI2D's thk = CSIVOXEL's dim[2]. 
	float3 oldpos, olddim, oldeuler, newpos, newdim, neweuler;
        oldpos[0]=currentStacks->stacks[stack].ppe;	
        oldpos[1]=currentStacks->stacks[stack].pro;	
        oldpos[2]=currentStacks->stacks[stack].pss0;	
        olddim[0]=currentStacks->stacks[stack].lpe;	
        olddim[1]=currentStacks->stacks[stack].lro;	
        olddim[2]=currentStacks->stacks[stack].lpe2;	
        oldeuler[0]=currentStacks->stacks[stack].theta;	
        oldeuler[1]=currentStacks->stacks[stack].psi;	
        oldeuler[2]=currentStacks->stacks[stack].phi;	
       	for(i=0; i<currentStacks->numOfStacks; i++) {
   	   if(currentStacks->stacks[i].planType == CSI2D) {
             neweuler[0]=currentStacks->stacks[i].theta;	
             neweuler[1]=currentStacks->stacks[i].psi;	
             neweuler[2]=currentStacks->stacks[i].phi;	
	     getNewPos(oldeuler, oldpos, neweuler, newpos);
	     getNewDim(oldeuler, olddim, neweuler, newdim);
    	     currentStacks->stacks[i].pss0 = newpos[2];
    	     currentStacks->stacks[i].thk = newdim[2];
	     if(currentStacks->stacks[i].ns == 1)
    	       currentStacks->stacks[i].lpe2 = newdim[2];
    	     calTransMtx(&(currentStacks->stacks[i]));
             calSliceXYZ(&(currentStacks->stacks[i]));
             calOverlayForAllViews(i);
	   }
	}
    }
}

void cubeDependency(int stack, float3 dp) {
    if(cubeFOV) {
     if(fabs(dp[0]) > fabs(dp[1])) 
       currentStacks->stacks[stack].lro = currentStacks->stacks[stack].lpe;
     else 
       currentStacks->stacks[stack].lpe = currentStacks->stacks[stack].lro;

     if(currentStacks->stacks[stack].type!=REGULAR) {
      if(fabs(dp[0]) > fabs(dp[2]) || fabs(dp[1]) > fabs(dp[2])) {
       currentStacks->stacks[stack].lpe2 = currentStacks->stacks[stack].lpe;
      } else {
       currentStacks->stacks[stack].lro = currentStacks->stacks[stack].lpe2;
       currentStacks->stacks[stack].lpe = currentStacks->stacks[stack].lpe2;
      }
     }
    }
}

/******************/
void updatePss(int act, int first, int last)
/******************/
{
/* lpe2, ns and gap are updated before calling this function */

    int i;
    float shift;

    if(currentStacks->stacks[act].type == VOLUME ||
	currentStacks->stacks[act].type == VOXEL ||
	currentStacks->stacks[act].type == SATBAND) return;

    if(currentStacks->stacks[act].type == REGULAR) {

	if((currentStacks->stacks[act].ns)%2 == 0)
		shift = (currentStacks->stacks[act].ns)/2.0 - 0.5;
	else shift = (currentStacks->stacks[act].ns-1.0)/2.0;

        for(i=first; i<=last; i++) {
/* reorder is done in updateZforNs only when ns changed 
        	currentStacks->stacks[act].order[i] = i+1; 
*/
        	currentStacks->stacks[act].pss[i][0] = (i-shift)
		*(currentStacks->stacks[act].thk + currentStacks->stacks[act].gap);
        	currentStacks->stacks[act].pss[i][1] = 0.0; 
        	currentStacks->stacks[act].radialAngles[i][0] = 0.0;
        	currentStacks->stacks[act].radialAngles[i][1] = 0.0;
	}
    } else if(currentStacks->stacks[act].type == RADIAL) {

          for(i=first; i<=last; i++) {
/* reorder is done in updateZforNs only when ns changed 
        	currentStacks->stacks[act].order[i] = i+1; 
*/
        	currentStacks->stacks[act].pss[i][0] = 0.0;
        	currentStacks->stacks[act].pss[i][1] = 0.0; 
        	currentStacks->stacks[act].radialAngles[i][0] = 
			i*currentStacks->stacks[act].gap;
        	currentStacks->stacks[act].radialAngles[i][1] = 0.0;
	  }
    }
}

/******************/
int updateZforZ(float lpe2, int act)
/******************/
{
/* for RADIAL, lpe2 is angel */

    int ns; 
    float d;
    int b = 0;

    if(lpe2 != currentStacks->stacks[act].lpe2) b = 1; 

    if(currentStacks->stacks[act].type == REGULAR ||
	currentStacks->stacks[act].type == RADIAL) {
      
      if(currentStacks->stacks[act].type == REGULAR && (fixedSlice == -2 || gapFixed)) {
	/* ns changes and lpe2 also changes (calculated in updateZforNs) */
/*
	if(currentStacks->stacks[act].ns <= 1 && currentStacks->stacks[act].gap == 0.0)
                currentStacks->stacks[act].gap = defaultGap;	
*/
        if(currentStacks->stacks[act].ns < 1) currentStacks->stacks[act].ns = 1;
	//if(currentStacks->stacks[act].gap < 0) currentStacks->stacks[act].gap = 0;

	d = lpe2 - currentStacks->stacks[act].lpe2;

	if(d > 0.1*(currentStacks->stacks[act].thk+currentStacks->stacks[act].gap)) { 
		ns = currentStacks->stacks[act].ns + 1;
		updateZforNs(ns, act, gapFixed);
	} else if(d < -0.1*(currentStacks->stacks[act].thk+currentStacks->stacks[act].gap)) { 
		ns = currentStacks->stacks[act].ns - 1;
/* gap
		if(ns <= 1) {
		   ns = 1;
		   currentStacks->stacks[act].gap = 0;
		}
*/
		updateZforNs(ns, act, gapFixed);
	}

      } else if(currentStacks->stacks[act].type == RADIAL && (fixedSlice == -2 || gapFixed)) {
        if(currentStacks->stacks[act].ns <= 1 && currentStacks->stacks[act].gap == 0.0)
                currentStacks->stacks[act].gap = defaultGap;

	d = lpe2 - currentStacks->stacks[act].lpe2;

        if(d > 1.5*(currentStacks->stacks[act].gap)) {
                ns = currentStacks->stacks[act].ns + 2;
                updateZforNs(ns, act, gapFixed);
        } else if(d > 0.5*(currentStacks->stacks[act].gap)) {
                ns = currentStacks->stacks[act].ns + 1;
                updateZforNs(ns, act, gapFixed);
        } else if(d < -1.5*(currentStacks->stacks[act].gap)) {
                ns = currentStacks->stacks[act].ns - 2;
                updateZforNs(ns, act, gapFixed);
        } else if(d < -0.5*(currentStacks->stacks[act].gap)) {
                ns = currentStacks->stacks[act].ns - 1;
                updateZforNs(ns, act, gapFixed);
        }
      } else {
	/* ns fixed, gap and lpe2 changed */

	currentStacks->stacks[act].lpe2 = lpe2;
	if(currentStacks->stacks[act].type == REGULAR && 
		currentStacks->stacks[act].ns > 1) 
		currentStacks->stacks[act].gap = 
		(lpe2-currentStacks->stacks[act].ns*currentStacks->stacks[act].thk)
		/(currentStacks->stacks[act].ns-1); 
		//if(currentStacks->stacks[act].gap < 0) currentStacks->stacks[act].gap = 0;
	else if(currentStacks->stacks[act].type == RADIAL &&
		currentStacks->stacks[act].ns > 1) 
		currentStacks->stacks[act].gap = lpe2
		/(currentStacks->stacks[act].ns-1); 

    	updatePss(act, 0, currentStacks->stacks[act].ns-1);

      }
    } else if(currentStacks->stacks[act].type == VOLUME) {
	currentStacks->stacks[act].lpe2 = lpe2;
	currentStacks->stacks[act].thk = lpe2;
    } else if(currentStacks->stacks[act].type == VOXEL) {
	currentStacks->stacks[act].lpe2 = lpe2;
	currentStacks->stacks[act].thk = lpe2;
    }

    return b;
}

/******************/
int updateZforNs(int ns, int act, int gapFixed)
/******************/
{

    int i;
    int prev, type;
    int b = 0;

    prev = currentStacks->stacks[act].ns;

    if(ns <= 0 || prev == ns) return b;
    else if(prev != ns) b = 1;

    type = currentStacks->stacks[act].type;

    if(type == VOLUME || type == VOXEL || type == SATBAND) return 0;
    if(type == RADIAL && (fixedSlice == -2 || gapFixed) && currentStacks->stacks[act].gap > 0
        && ns*currentStacks->stacks[act].gap > D360)
        ns = D360/currentStacks->stacks[act].gap;

    currentStacks->stacks[act].ns = ns;

    currentStacks->stacks[act].order = (int*)
	realloc(currentStacks->stacks[act].order, sizeof(int)*ns);
    currentStacks->stacks[act].pss = (float2*)
	realloc(currentStacks->stacks[act].pss, sizeof(float2)*ns);
    currentStacks->stacks[act].radialAngles = (float2*)
	realloc(currentStacks->stacks[act].radialAngles, sizeof(float2)*ns);
    currentStacks->stacks[act].slices = (iplan_box *)
        realloc(currentStacks->stacks[act].slices, sizeof(iplan_box)*ns);

    if(type == RADIAL) {
	currentStacks->stacks[act].envelope.np = 4*circlePoints;
        currentStacks->stacks[act].envelope.points = (float3 *) 
	realloc(currentStacks->stacks[act].envelope.points, sizeof(float3)*(4*circlePoints));
    } 

    if((fixedSlice == -2 || gapFixed)) {
	/* lpe2 changes */
/*
	if(prev <= 1 && currentStacks->stacks[act].gap == 0.0)
		currentStacks->stacks[act].gap = defaultGap;
*/
	if(currentStacks->stacks[act].gap  < 0) currentStacks->stacks[act].gap = 0;
	//if(type == REGULAR && ns > 1) 
	if(type == REGULAR && ns > 0) 
		currentStacks->stacks[act].lpe2 = 
		currentStacks->stacks[act].ns*currentStacks->stacks[act].thk + 
		currentStacks->stacks[act].gap*(ns-1);
	//else if(type == RADIAL && ns > 1) 
	else if(type == RADIAL && ns > 0) 
		currentStacks->stacks[act].lpe2 =  
		currentStacks->stacks[act].gap*(ns-1);

	updatePss(act, 0, ns-1);

	ns = 0;
        
        for(i=0; i<currentStacks->stacks[act].ns; i++)
            if(currentStacks->stacks[act].order[i] > 0) ns++;
    } else {
	/* lpe2 fixed, gap changes */

	if(type == REGULAR && ns > 1) { 
		currentStacks->stacks[act].gap = 
		(currentStacks->stacks[act].lpe2 
		- currentStacks->stacks[act].ns*currentStacks->stacks[act].thk)/(ns-1);
		// if(currentStacks->stacks[act].gap < 0.0) currentStacks->stacks[act].gap = 0.0;
/*
	} else if(type == REGULAR) { 
                if(currentStacks->stacks[act].gap < 0) {
		    currentStacks->stacks[act].gap = 0;
		    currentStacks->stacks[act].lpe2 = 
		    currentStacks->stacks[act].thk;
		}
*/
	} else if(type == RADIAL && ns > 1) { 
		currentStacks->stacks[act].gap = 
		(currentStacks->stacks[act].lpe2)/(ns-1);
        }

        updatePss(act, 0, ns-1);

	ns = 0;
        for(i=0; i<currentStacks->stacks[act].ns; i++)
            if(currentStacks->stacks[act].order[i] > 0) ns++;
    }

    for(i=0; i<currentStacks->stacks[act].ns; i++)
	    currentStacks->stacks[act].order[i] = i+1;
    if(alternateOrders > 0) alternateSlices(alternateOrders, -1);

    return b;
}

/******************/
void drawActiveSlice(int flag, int nv)
/******************/
{
    int i, j;
    int x1, y1, x2, y2;
    int xmin, xmax, ymin, ymax;

    if(activeSlice < 0 || activeStack < 0 || 
	activeView < 0 || nv < 0 || nv >= currentViews->numOfViews ||
	currentViews->numOfViews == 0 || 
	currentStacks->numOfStacks == 0 ||
	currentOverlays->numOfOverlays == 0 ||
	currentOverlays->overlays[activeView].numOfStacks == 0 ||
	activeSlice >= currentStacks->stacks[activeStack].ns ||
	activeSlice >= 
	  currentOverlays->overlays[activeView].stacks[activeStack].numOfStripes ||
	activeView >= currentViews->numOfViews ||
	activeView >= currentOverlays->numOfOverlays ||
	activeStack >= currentStacks->numOfStacks ||
	activeStack >= currentOverlays->overlays[activeView].numOfStacks) return;

    if(currentStacks->stacks[activeStack].order[activeSlice] <= 0) return;

    color(highlight);

      xmin = currentViews->views[nv].framestx;
      xmax = currentViews->views[nv].framestx +(currentViews->views[nv].framewd);

      ymin = currentViews->views[nv].framesty - (currentViews->views[nv].frameht);
      ymax = currentViews->views[nv].framesty;

      j = currentOverlays->overlays[nv].stacks[activeStack]
                .stripes[activeSlice].np;
      x2 = currentOverlays->overlays[nv].stacks[activeStack]
		.stripes[activeSlice].points[j-1][0];
      y2 = currentOverlays->overlays[nv].stacks[activeStack]
		.stripes[activeSlice].points[j-1][1];

      for(i=0; i<j; i++) {
	x1 = x2;
	y1 = y2;

	x2 = currentOverlays->overlays[nv].stacks[activeStack]
                .stripes[activeSlice].points[i][0];
	y2 = currentOverlays->overlays[nv].stacks[activeStack]
                .stripes[activeSlice].points[i][1];

	if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
               (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) {
        } else {
	    if(nv == activeView) drawLine(flag, x1,y1,x2,y2,2,"LineOnOffDash");
	    else drawLine(flag, x1,y1,x2,y2,2,"LineSolid");
	}
      }
}

// find shortest side of a polygon
/******************/
int overlaySize_min(iplan_2Dstack *overlay)
/******************/
{
    int i, np, d, dmin;
    int x1, y1, x2, y2;

    np = overlay->handles.np;

    if(np <= 0) return(handleSize);

    x2 = overlay->handles.points[np-1][0];
    y2 = overlay->handles.points[np-1][1];

    dmin = 1.0e6;
    for(i=0; i<np; i++) {

	x1 = x2;
	y1 = y2;

    	x2 = overlay->handles.points[i][0];
    	y2 = overlay->handles.points[i][1];
	d = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
	if(d < dmin) dmin = d;
    }
    return(dmin);
}

// find longest side of a polygon
/******************/
int overlaySize_max(iplan_2Dstack *overlay)
/******************/
{
    int i, np, d, dmax;
    int x1, y1, x2, y2;

    np = overlay->handles.np;

    if(np <= 0) return(handleSize);

    x2 = overlay->handles.points[np-1][0];
    y2 = overlay->handles.points[np-1][1];

    dmax = 0;
    for(i=0; i<np; i++) {

	x1 = x2;
	y1 = y2;

    	x2 = overlay->handles.points[i][0];
    	y2 = overlay->handles.points[i][1];
	d = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
	if(d > dmax) dmax = d;
    }
    return(dmax);
}

/******************/
void drawHandles(int flag)
/******************/
{
                                                                                                 
    int view = 0;
    int stack = 0;
    int x1, y1, x2, y2;
    int i, curp, np, ns;
    unsigned int size;
    static float2* points = NULL;
    static int pSize = 0;
    float2 cCenter;
    int xmin, xmax, ymin, ymax;
    int type, k;
    iplan_2Dstack *overlay;
                                                                                                 
    if(activeSlice >= 0 || activeStack < 0 || activeView < 0 ||
        currentViews->numOfViews == 0 ||
        currentStacks->numOfStacks == 0 ||
        currentOverlays->numOfOverlays == 0 ||
        currentOverlays->overlays[activeView].numOfStacks == 0 ||
        activeView >= currentViews->numOfViews ||
        activeView >= currentOverlays->numOfOverlays ||
        activeStack >= currentStacks->numOfStacks ||
        activeStack >= currentOverlays->overlays[activeView].numOfStacks) return;
                                                                                                 
    if(activeView > 0) view = activeView;
    if(activeStack > 0) stack = activeStack;
                                                                                                 
    overlay = &(currentOverlays->overlays[view].stacks[stack]);
    if(!intersected(overlay)) return;

    type = overlay->stack->type;
    curp = overlay->stack->color;
                                                                                                 
    size = handleSize;
/*
    if(type == SATBAND) size = handleSize;
    else size = (int)min((float)handleSize, 1.0*(float)overlaySize_max(overlay));
 */                                                                                                
    np = overlay->envelope.np;
    xmin = currentViews->views[view].framestx;
    xmax = currentViews->views[view].framestx +(currentViews->views[view].framewd);
                                                                                                 
    ymin = currentViews->views[view].framesty - (currentViews->views[view].frameht);
    ymax = currentViews->views[view].framesty;
                                                                                                 
    if(np > 0) {
      if (pSize < np) {
        if (points != NULL) free(points);
        pSize = np;
        points = (float2*)malloc(sizeof(float2)*np);
      }
      for(i=0; i<np; i++) {
            points[i][0] = overlay->envelope.points[i][0];
            points[i][1] = overlay->envelope.points[i][1];
      }
                                                                                                 
      color(curp);
                                                                                                 
      /* draw envelope */
                                                                                               
      x2 = points[np-1][0];
      y2 = points[np-1][1];
                                                                                                 
      k = np-1;
      for(i=0; i<np; i++) {
                                                                                                 
        x1 = x2;
        y1 = y2;
                                                                                                 
        x2 = points[i][0];
        y2 = points[i][1];
                                                                                                 
        if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
               (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) {
        } else if(handleMode == SQUARE && ((k == activeVertex1 && i == activeVertex2)
                || (i == activeVertex1 && k == activeVertex2))) {
            color(handleColor);
	    drawLine(flag, x1,y1,x2,y2,2,"LineSolid");
            color(curp);
        } else if(selectedStack != -1 && selectedStack == stack) { 
            color(highlight);
            drawLine(flag, x1,y1,x2,y2,1,"LineOnOffDash");
            color(curp);
	}
        k = i;
      }
                                                                                                
      //free(points);
    }
    np = overlay->handles.np;
    ns = overlay->numOfStripes;
                                                                                                 
    if(np > 0) {
      if (pSize < np) {
        if (points != NULL) free(points);
        pSize = np;
        points = (float2*)malloc(sizeof(float2)*np);
      }
      for(i=0; i<np; i++) {
            points[i][0] = overlay->handles.points[i][0];
            points[i][1] = overlay->handles.points[i][1];
      }
                                                                                                 
      color(curp);
                                                                                                 
      /* draw handles for all corners */
                                                                                                 
      for(i=0; i<np; i++) {
                                                                                                 
        x2 = points[i][0];
        y2 = points[i][1];
                                                                                                 
        if(type != SATBAND && (x2 == xmin || x2 == xmax || y2 == ymin || y2 == ymax)) {
        } else {
           if(activeHandle >= 0 && handleMode == SQUARE) {
            color(handleColor);
	    drawRectangle(flag, x2, y2, size, size, 6, "LineSolid");
            color(curp);
           } else if(activeHandle >= 0 && handleMode == CIRCLE) {
            color(handleColor);
            drawCircle(flag, x2, y2, size, 0, 360*64, 2, "LineSolid");
            color(curp);
	   }
/*
           } else
	    drawCircle(flag, x2, y2, size, 0, 360*64, 2, "LineSolid"); 
*/
      }
      if(handleMode == CIRCLE || handleMode == RADIALCIRCLE) {
        getRotationCenter(cCenter, activeView, activeStack);
        x2 = (int)cCenter[0];
        y2 = (int)cCenter[1];
                                                                                                 
        if(x2 <= xmin || x2 >= xmax || y2 <= ymin || y2 >= ymax) {
        } else if(activeHandle >= 0) {
            color(handleColor);
            drawCircle(flag, x2, y2, 2, 0, 360*64, 2, "LineSolid");
            color(curp);
        } else drawCircle(flag, x2, y2, 2, 0, 360*64, 2, "LineSolid");
        }
      }
      //free(points);
    }
}

/******************/
static void mouse_click(int x, int y0, int button, int combo, int click)
/******************/
{
    int type, view;
    float2 p;
    int y;
    unsigned int size;

    size = 10;

    y = aip_mnumypnts-y0-1;
    p[0] = x;
    p[1] = y;

    type = -1;
    if(currentStacks != NULL && activeStack >= 0 && activeStack < currentStacks->numOfStacks)
        type = currentStacks->stacks[activeStack].type;

    if(selectedSlice != -1 && click == 2) {
	activeSlice = selectedSlice;
	drawOverlays();
    } else if(activeSlice != -1) { 
	activeSlice = -1;
        selectedSlice = -1;
	drawOverlays();
    }
	
    if((combo & 0x1000000) && orderMode &&
	activeSlice != -1 && activeView != -1 && activeStack != -1) {
	updateSliceOrders(activeStack, activeSlice);
	drawOverlays();
/*
    } else if(button == 2 && activeMark == -1 && showStackOrders == 0) {
	showStackOrders = 1;
	drawOverlays();
    } else if(button == 2 && activeMark == -1 && showStackOrders == 1) {
	showStackOrders = 0;
	drawOverlays();
*/
    } 

    if(activeMark != -1 && button == 2) {
	view = marks[activeMark].view;
	removeMark(activeMark);
        activeMark = -1;
	drawOverlaysForView(view);

	if(isMarking != -1) {
	   startMarking();
	} else {
	    sendValueToVnmr("iplanMarking", iMarks); 
            appendJvarlist("iplanMarking");
	    // writelineToVnmrJ("pnew", "1 iplanMarking");
	}

    /* check fixed marks */
      	fixedSlice = getFixedSlice();

    }
/*
    if(activeMark != -1 && activeView != -1 && activeStack != -1 && click == 2) {
*/
    if(activeMark != -1 && click == 2) {
	  view = marks[activeMark].view;
          if(marks[activeMark].isFixed == 0) marks[activeMark].isFixed = 1;
	  else if(marks[activeMark].isFixed == 1) marks[activeMark].isFixed = 0; 
	  drawOverlaysForView(view);

    /* check fixed marks */
      	fixedSlice = getFixedSlice();

    } 
	
    prevP[0] = p[0];
    prevP[1] = p[1];
}

void getTranslation(int view, float2 p, float2 prevP, 
			int view2, float2 newp) {

    float3 p3, prevP3;

    if(view == view2) {
    	newp[0] = p[0] - prevP[0];
    	newp[1] = p[1] - prevP[1];

    } else {
	p3[0] = p[0];
	p3[1] = p[1];
	p3[2] = 0;
	transform(currentViews->views[view].p2m, p3);
	transform(currentViews->views[view2].m2p, p3);
	prevP3[0] = prevP[0];
	prevP3[1] = prevP[1];
	prevP3[2] = 0;
	transform(currentViews->views[view].p2m, prevP3);
	transform(currentViews->views[view2].m2p, prevP3);
	newp[0] = p3[0] - prevP3[0];
	newp[1] = p3[1] - prevP3[1];
    }
}

/******************/
int minEdge(int view, int stack)
/******************/
{
    int i, np, d, dmin;
    int x1, y1, x2, y2;

    np = currentOverlays->overlays[view].stacks[stack].handles.np;

    if(np <= 0) return(handleSize);

    x2 = currentOverlays->overlays[view].stacks[stack].handles.points[np-1][0];
    y2 = currentOverlays->overlays[view].stacks[stack].handles.points[np-1][1];

    dmin = currentViews->views[view].pixwd + currentViews->views[view].pixht;
    for(i=0; i<np; i++) {

	x1 = x2;
	y1 = y2;

    	x2 = currentOverlays->overlays[view].stacks[stack].handles.points[i][0];
    	y2 = currentOverlays->overlays[view].stacks[stack].handles.points[i][1];
	d = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
	if(d > handleSize && d < dmin) dmin = d;
    }
    return(dmin);
}

/******************/
static void mouse_drag(int x, int y0, int button, int combo, int status)
/******************/
{
    float2 p, newp;
    int y, view, i, s2;

    y = aip_mnumypnts-y0-1;
    p[0] = x;
    p[1] = y;

    // when holding shift key, activeStack stack can be dragged regardless
    // whether handle or vertex is selected.
    if(combo & 0x1000000) {
        if(activeStack == -1) activeStack = 0;
	activeHandle = -1;
        activeVertex1 = -1;
        activeVertex2 = -1;
	updateTranslation(activeStack, p, prevP); 
 
    } else if(activeMark >= 0 && activeMark < numOfMarks) {
	view = marks[activeMark].view;
   	marks[activeMark].currentLocation[0] = p[0];
   	marks[activeMark].currentLocation[1] = p[1];
	getLocation2(marks[activeMark].currentLocation, &(marks[activeMark].view),
                &(marks[activeMark].stack), &(marks[activeMark].slice));

	if(marks[activeMark].isFixed && marks[activeMark].stack != -1) {

	   s2 = marks[activeMark].stack;

	   for(i=0; i<numOfMarks; i++) { 
		if(marks[i].isFixed && i != activeMark) { 
		    getTranslation(view, p, prevP, 
			marks[i].view, newp); 
		    marks[i].currentLocation[0] += newp[0];
		    marks[i].currentLocation[1] += newp[1];

		    if(marks[i].stack != marks[activeMark].stack
			&& marks[i].stack != s2) {
			s2 = marks[i].stack;
			updateTranslation(s2, p, prevP);
		    }

		}
	   }

	   if(activeStack != marks[activeMark].stack) {
	       activeStack = marks[activeMark].stack;
	       updateType();
    	       drawOverlays();
	   }

	   activeView = marks[activeMark].view;

	   updateTranslation(activeStack, p, prevP); 
	   
	} else {
	drawOverlaysForView(marks[activeMark].view);
	if(view != marks[activeMark].view) drawOverlaysForView(view);
	}

    } else if(activeSlice != -1 && activeView != -1 && activeStack != -1) {

	updateSlice(p, prevP);

    } else if(activeView != -1 && activeStack != -1 && activeHandle != -1) {
        if(button == 2) handleMode = CIRCLE;
    	else handleMode = SQUARE;

	if(handleMode ==  SQUARE) {
 
           /* left = 0, middle = 1, right = 2 */
	   updateExpansion(p, prevP);

	} else if(handleMode ==  CIRCLE || handleMode ==  RADIALCIRCLE) {
	   updateRotation(p, prevP, 0, 0.0, 0.0);
	}
    } else if(handleMode ==  SQUARE && activeView != -1 && activeStack != -1 &&
        activeVertex1 != -1 && activeVertex2 != -1) {

	if(button == 2 || fixedSlice >= 0) updateSExpansion(p, prevP);
	else updateExpansion(p, prevP);

    } else if(selectedStack != -1 && activeVertex1 == -1 && activeVertex2 == -1 &&
	activeHandle == -1 && activeView != -1 && activeStack != -1) {
	updateTranslation(activeStack, p, prevP); 
    } 
    prevP[0] = p[0];
    prevP[1] = p[1];
}

#ifdef XXX
/******************/
static void mouse_reset()
/******************/
{
    toggleBackingStore(1);
}
#endif

/******************/
void getLocation2(float2 p, int* view, int* stack, int* slice)
/******************/
{
    int i, j, k;
    float2 p1;
    float a, amax, pi2;
    iplan_2Dstack *overlay;

    *view = -1;
    *stack = -1;
    *slice = -1;

    if(currentStacks == NULL || currentViews == NULL || currentOverlays == NULL) return;

    for(i=0; i<currentViews->numOfViews; i++) {
        if(containedInRectangle(p, currentViews->views[i].framestx,
                currentViews->views[i].framesty,
                currentViews->views[i].framewd,
                currentViews->views[i].frameht)) {
            *view = i;
        }
    }

    if(*view < 0 || *view >= currentViews->numOfViews) return;
    i = *view;

	  /* test slices */
	
        amax = 0;
        for(j=0; j<currentOverlays->overlays[i].numOfStacks; j++) {
  	  overlay = &(currentOverlays->overlays[i].stacks[j]);
	  for(k=0; k<overlay->numOfStripes; k++) {
	   
           a = containedInConvexPolygon(p,
                overlay->stripes[k].points,
                overlay->stripes[k].np);
            if(a > amax) {
		amax = a;
                 *slice = k;
		 *stack = j;
            }
          }
	}
	pi2 = 1.99*pi;
	if(amax > pi2) {
	    return;
	} else {
		*slice = -1;
		*stack = -1;
	}

	/* test stack */

        for(j=0; j<currentOverlays->overlays[i].numOfStacks; j++) {

  	  overlay = &(currentOverlays->overlays[i].stacks[j]);
	  if(currentStacks->stacks[j].type != RADIAL) {

	     if(containedInConvexPolygon(p, 
		overlay->handles.points,
		overlay->handles.np) > 1.99*pi) {
	
		*stack = j;
		return;
	     }

	  } else {
             p1[0] = overlay->handleCenter[0];
             p1[1] = overlay->handleCenter[1];
             if(overlay->handles.np > 0 &&
                containedInCircle(p, p1, 0.3*overlaySize_max(overlay))) {
	
		*stack = j;
		return;
	     }
	  }

	}
}    

/******************/
int isOverStack(float2 p, int i, int j)
/******************/
{
	float f;
	float2 p1;
   	iplan_2Dstack *overlay;

	overlay = &(currentOverlays->overlays[i].stacks[j]);

	  if(currentStacks->stacks[j].type != RADIAL) {
	     f = containedInConvexPolygon(p,
                overlay->handles.points,
                overlay->handles.np);
	     if(f > 1.99*pi) {
		return 1;
	     }

	  } else {
             p1[0] = overlay->handleCenter[0];
             p1[1] = overlay->handleCenter[1];
             if(overlay->handles.np > 0 &&
                containedInCircle(p, p1, 0.3*overlaySize_max(overlay))) {

		return 1;
	     }
	  }
	  return 0;
}

// find polygon area. 
int overlayArea(iplan_2Dstack *overlay)
{
    int i, np, d;
    int x1, y1, x2, y2;

    np = overlay->handles.np;

    if(np <= 0) return(0);

    x2 = overlay->handles.points[np-1][0];
    y2 = overlay->handles.points[np-1][1];

    d=0;
    for(i=0; i<np; i++) {

	x1 = x2;
	y1 = y2;

    	x2 = overlay->handles.points[i][0];
    	y2 = overlay->handles.points[i][1];
        d += (x1*y2 - x2*y1);
    }
    if(d<0) d=-d;
    return d/2;
}

int getSelectedStack(float2 p, int i)
{
     int j, k, kmin = 1e6;
     int stack = -1;
     for(j=0; j<currentOverlays->overlays[i].numOfStacks; j++) {
	    k = overlayArea(&(currentOverlays->overlays[i].stacks[j]));
	    if(isOverStack(p, i, j) && k < kmin) {
		kmin = k;
		stack = j;	
	    } 
     }
     return stack;
}

// do not modify p,p1,p2.
// p is the point, line is defined by p1 and p2. 
// return perpendicular distance from p to the line.
// res is the resolution (5 pixel). 
int onLine(float2 p, float2 p1, float2 p2, int res)
{
   float x, y, r1, r2, r3, e;
   x = p[0] - p1[0];
   y = p[1] - p1[1];
   r1=sqrt(x*x + y*y);
   x = p[0] - p2[0];
   y = p[1] - p2[1];
   r2=sqrt(x*x + y*y);
   x = p1[0] - p2[0];
   y = p1[1] - p2[1];
   r3=sqrt(x*x + y*y);
   e = min(0.3*r3, res+1); // exclude ends of the line so handle can be selected. 
   if((r1+e) > sqrt(r2*r2+r3*r3)) return 2*res; // p,p2 angle > 90
   if((r2+e) > sqrt(r1*r1+r3*r3)) return 2*res; // p,p1 angle > 90
   x = 0.5*(r3*r3+r2*r2-r1*r1)/r3;
   y = r2*r2-x*x;
   if(y>0) return (int)sqrt(y); // height o 
   else return 0;
}

/******************/
void getLocation(float2 p, int* view, int* stack, int* slice, 
	int* handle, int* vertex1, int* vertex2, int* mark)
/******************/
{

    int i, j, k, np, cornerSize, overlaySize, onStack, edgeRes, res, minRes;
    int area, minArea, v1,v2,h;
    float2 p1, p2;
    float a, amax, pi2;
    iplan_2Dstack *overlay;

    *view = -1;
    *stack = -1;
    *handle = -1;
    *vertex1 = -1;
    *vertex2 = -1;

    if(currentStacks == NULL || currentViews == NULL) return;

    // test frame
    for(i=0; i<currentViews->numOfViews; i++) {
        if(containedInRectangle(p, currentViews->views[i].framestx,
                currentViews->views[i].framesty,
                currentViews->views[i].framewd,
                currentViews->views[i].frameht)) {
            *view = i;
        }
    }

    if(*view < 0 || *view >= currentViews->numOfViews) return;

    if(mark != NULL) {
    // test marks
      *mark = -1;
      for(i=0; i<numOfMarks; i++) {
        if(marks != NULL && marks[i].isUsing && marks[i].currentLocation[0] > 0
		&& marks[i].currentLocation[1] > 0 &&
		containedInCircle(p, marks[i].currentLocation, markSize/2)) {
	    *mark = i;
	    return;
        }
      }
    }
    i = *view;

        minArea = 4098*4098; // max view area.
        for(j=0; j<currentOverlays->overlays[i].numOfStacks; j++) {

          overlay = &(currentOverlays->overlays[i].stacks[j]);
	  overlaySize = overlaySize_min(overlay);
	  //cornerSize = (int)min((float)(0.6*handleSize), 0.6*(float)overlaySize);
	  cornerSize = 0.5*handleSize; // handleSize is 10
	  edgeRes = 0.5*handleSize;
	  onStack = isOverStack(p, i, j);
	  area = overlayArea(overlay);

	  /* test edges */
	  v1 = -1;
	  v2 = -1;
	  np = overlay->handles.np;
          minRes=edgeRes;
          p2[0] = overlay->handles.points[np-1][0];
          p2[1] = overlay->handles.points[np-1][1];
          for(k=0; k<np; k++) {
            p1[0] = p2[0];
            p1[1] = p2[1];
            p2[0] = overlay->handles.points[k][0];
            p2[1] = overlay->handles.points[k][1];
	    if((overlaySize>handleSize || !onStack)) {
	      res=onLine(p, p1, p2, edgeRes);
	      if(res < minRes) { // the closer edge is selected.
	 	minRes=res;	
                v2 = k;
                k--;
                if(k < 0) k = np-1;
                v1 = k;
	      }
            }
          }
       
	  /* test handles */

	  h = -1;
	  if(v1 == -1 || v2 == -1) {
	   for(k=0; k<overlay->handles.np; k++) {
            if((overlaySize>handleSize || !onStack) && containedInCircle(p, 
		overlay->handles.points[k], 
	        cornerSize)) {
		    h = k;
		    break;
            }
           }
	  }

	  // smaller stack (with smaller area) has higher priority
	  // to be selected. 
	  if(area < minArea) {
             if(v1 != -1 && v2 != -1) {
	        minArea = area;
	     	*stack = j;
		*handle = -1;
	     	*vertex1 = v1;
	     	*vertex2 = v2;
	     } else if(h != -1) {
	        minArea = area;
	       	*stack = j;
		*handle = h;	
	     	*vertex1 = -1;
	     	*vertex2 = -1;
	     }
	  }
        }

    if(*stack == -1) *stack = getSelectedStack(p, i);
    j = *stack;

    if(slice == NULL || *handle != -1 || j < 0 
	|| j >= currentStacks->numOfStacks) return;

    *slice = -1;
    overlay = &(currentOverlays->overlays[i].stacks[j]);

	  /* test slices */
	
        amax = 0;
      	if(overlay->numOfStripes > 1) {
	  for(k=0; k<overlay->numOfStripes; k++) {

           a = containedInConvexPolygon(p,
                overlay->stripes[k].points,
                overlay->stripes[k].np);
	    if(a > amax) {
		amax = a;
                 *slice = k;
            }
	  }
	}

	pi2 = 1.99*pi;
	if(amax > pi2) {
                 return;
	} else {
		*slice = -1;
	}
}    

/******************/
int getCrosssectionCenter(float2 cCenter, int actV, int actS)
/******************/
{
    int np;

    cCenter[0] = 0.0;
    cCenter[1] = 0.0;

    if(currentViews == 0 || actV < 0 || actV >= currentViews->numOfViews || 
       currentStacks == NULL || actS < 0 || actS >= currentStacks->numOfStacks)
	return(-1);

    if(handleMode == RADIALCIRCLE) {
        cCenter[0] = currentOverlays->overlays[actV].stacks[actS].envelopeCenter[0];
        cCenter[1] = currentOverlays->overlays[actV].stacks[actS].envelopeCenter[1];
        np = currentOverlays->overlays[actV].stacks[actS].envelope.np;

    } else {
        cCenter[0] = currentOverlays->overlays[actV].stacks[actS].handleCenter[0];
        cCenter[1] = currentOverlays->overlays[actV].stacks[actS].handleCenter[1];
        np = currentOverlays->overlays[actV].stacks[actS].handles.np;
    }

/*
    if(np != 0) {
      for(i=0; i<np; i++) {
	cCenter[0] += 
	    currentOverlays->overlays[actV].stacks[actS].handles.points[i][0];
	cCenter[1] += 
	    currentOverlays->overlays[actV].stacks[actS].handles.points[i][1];
    
      }
      cCenter[0] /= np;
      cCenter[1] /= np;
    }
*/
    return(np);
}

/******************/
void getStackCenter(float3 cCenter, int activeView, int activeStack)
/******************/
{
    float3 c;

    c[0] = currentStacks->stacks[activeStack].ppe;
    c[1] = currentStacks->stacks[activeStack].pro;
    c[2] = currentStacks->stacks[activeStack].pss0;

    /* rotate to magnet frame */
    rotateu2m(c, currentStacks->stacks[activeStack].orientation);

    /* rotate to the view */
    transform(currentViews->views[activeView].m2p, c);
    cCenter[0] = c[0];
    cCenter[1] = c[1];
    cCenter[2] = c[2];
}

/******************/
void getStackCenter2(float3 cCenter, int activeView, int activeStack)
/******************/
{
    float3 o;

    cCenter[0] = 0.0;
    cCenter[1] = 0.0;
    cCenter[2] = 0.0;

    o[0] = currentStacks->stacks[activeStack].ppe;
    o[1] = currentStacks->stacks[activeStack].pro;
    o[2] = currentStacks->stacks[activeStack].pss0;

    rotateu2m(cCenter, currentStacks->stacks[activeStack].orientation);
    translateu2m(cCenter, o);

    transform(currentViews->views[activeView].m2p, cCenter);
}

/******************/
float calAngle(float2 cCenter, float2 prevP, float2 p)
/******************/
{
    float a1, a2;
    float2 p1, p2;

    p1[0] = prevP[0] - cCenter[0]; 
    p1[1] = prevP[1] - cCenter[1]; 
    p2[0] = p[0] - cCenter[0]; 
    p2[1] = p[1] - cCenter[1]; 
    
    a1 = acos(p1[0]/sqrt(p1[0]*p1[0] + p1[1]*p1[1]));
    if(p1[1] < 0.0) a1 = 2.0*pi - a1;

    a2 = acos(p2[0]/sqrt(p2[0]*p2[0] + p2[1]*p2[1]));
    if(p2[1] < 0.0) a2 = 2.0*pi - a2;

    return(R2D*(a2-a1));
}

/******************/
int sendValueToVnmr(char* paramName, float value)
/******************/
{
    if(currentStacks == NULL || currentViews == NULL) return 0;

    return writeParamInProcpar(NULL, paramName, &value, 1);
}

/******************/
int* getStackIndexByOrder() 
/******************/
{
/* return an array contains the stack index sorted by orders. */

    int i, n; 
    int *orders;
    static int* ind;

    n = currentStacks->numOfStacks;

    ind = (int*)malloc(sizeof(int)*n);
    orders = (int*)malloc(sizeof(int)*n);

    for(i=0; i<n; i++) 
	orders[i] = currentStacks->order[i];

    for(i=0; i<n; i++) { 
/*
    	omin = n+1;
	for(j=0; j<n; j++) {
	    if(orders[j] < omin) {
		omin = orders[j];
		k = j;
	    }
	}
	ind[i] = k;
	orders[k] = n+1;
*/
	ind[i] = orders[i]-1;
    }

    free(orders);
    return(ind);
}

/******************/
int* getSliceIndexByOrder(int stack) 
/******************/
{
/* return an array contains the slice index sorted by orders. */

    int i, n;
    int *orders;
    static int* ind;

    n = currentStacks->stacks[stack].ns;

    ind = (int*)malloc(sizeof(int)*n);
    orders = (int*)malloc(sizeof(int)*n);

    for(i=0; i<n; i++) 
	orders[i] = currentStacks->stacks[stack].order[i];

    for(i=0; i<n; i++) { 
/*
    	omin = n+1;
	for(j=0; j<n; j++) {
	    if(orders[j] < omin) {
		omin = orders[j];
		k = j;
	    }
	}
	ind[i] = k;
	orders[k] = n+1;
*/
	ind[i] = orders[i]-1;
    }

    free(orders);
    return(ind);
}

/******************/
int sendPssByOrder(int act)
/******************/
{
    int k, ns; 
    float* pss;
    int b = 0;

    if(currentStacks->stacks[act].type == SATBAND ||
	currentStacks->stacks[act].type == VOXEL) return b;

    ns = currentStacks->stacks[act].ns;
    pss = (float*)malloc(sizeof(float)*ns);
    k = orderPssValuesForStack(act, pss);

/* send pss to vnmr */
    if(k > 0) b = writeParamInProcpar(NULL, "pss", pss, k);
 
    free(pss);

    return b;
}

/******************/
int orderPssValuesForStack(int act, float* values)
/******************/
{
    int i, k;
    int* ind;

    ind = getSliceIndexByOrder(act);

    k = 0;
    for(i=0; i<currentStacks->stacks[act].ns; i++) {
	if(currentStacks->stacks[act].order[ind[i]] > 0) {
	    values[k] = currentStacks->stacks[act].pss0
                + currentStacks->stacks[act].pss[ind[i]][0]
                + currentStacks->stacks[act].pss[ind[i]][1];
            k++;
        }
    }

    free(ind);
    return(k);
}

/******************/
int writeParamInProcpar(FILE* fp, char* paramName, float* values, int n)
/******************/
{
    int i, j, k, nd;
    char str[2048+1]; 
    float f;
    int b = 0;
    double d;
    vInfo           paraminfo;

    nd = 100;
    nd=1000;

    if(fp == NULL) {

      if(!(P_getVarInfo(CURRENT, paramName, &paraminfo))) {
	 f = 1.0/nd;
	 if(n != paraminfo.size) b++;
	 else for(i=0; i<n; i++) { 
	    if(!P_getreal(CURRENT, paramName, &d, i+1)) {
		if(fabs(values[i] - (float)d) > f) {
		   b++;
		} 
	    }
	 }
      }
      clearVar(paramName);

      for(i=0; i<n; i++) { 
	if(values[i] < 0) j = (int)(values[i]*nd - 0.5);
	else j = (int)(values[i]*nd + 0.5);
	f = ((float)j)/nd;
//	f=(float)values[i];  // no more truncation
	P_setreal(CURRENT, paramName, (double)f, i+1); 
      }

/* this does not take care of  paralell array */
/*
      checkarray(paramName, n);
*/
    } else {

      strcpy(str, (char*)intString(n));
      strcat(str, " ");

      for(i=0; i<n; i++) {
	if(values[i] < 0) j = (int)(values[i]*nd - 0.5);
	else j = (int)(values[i]*nd + 0.5);
 	k = strlen((char*)intString(j)); 
	f = ((float)j)/nd;
/*
	strncat(str, (char*)realString(f),k+1);
*/
	strcat(str, realString(f));
	if(i < n-1) strcat(str, " ");
	else if(i == n-1) strcat(str, "\n");
        if(strlen(str) >= MAXSTR) { 
	    fprintf(stderr,"Error: array size too big.\n");
	    return 0;
	}
      }

      fprintf(fp, "%s \n", paramName);
      fprintf(fp, "%s", str);
    }

    return b;
}

/******************/
int sendStacksCompress_m(FILE* fp, int type)
/******************/
{
    int i, j, m, n, k=0, l, size=0;
    int* ind;
    int* sliceInd;
    float* pss = NULL;
    float* volnv2 = NULL;
    float* ns = NULL;
    float* theta = NULL;
    float* psi = NULL;
    float* phi = NULL;
    float* thk = NULL;
    float* lpe = NULL;
    float* lro = NULL;
    float* lpe2 = NULL;
    float* ppe = NULL;
    float* pro = NULL;
    float* pss0 = NULL;
    float* gap = NULL;
    float* radialAngles = NULL;
    float theta1, psi1, phi1, orientation[9];
    int act = -1;
    int actS = 0;
    float3 c;
    float angle;
    char str[MAXSTR], strn[64];
    int coil = 1;
    int nv, nd = 100;
    planParams *tag;

    n = currentStacks->numOfStacks;
    if(activeStack > 0 && activeStack < n) actS = activeStack; 

    // set act to activeStack if stacks are in different types
    // this may change the order of array. 

    for(i=0; i<n; i++) {
	if(currentStacks->stacks[i].type == type) {
	    act = i;
	    break;
	}
    }

    if(act == -1) return(-1);
    actS = act;

    for(i=0; i<n; i++) size += currentStacks->stacks[i].ns;
    pss = (float*)malloc(sizeof(float)*size);

    size = 0;
    for(i=0; i<n; i++) {
	if(currentStacks->stacks[i].type == RADIAL) 
	    size += currentStacks->stacks[i].ns;
	else size += 1;
    }

    volnv2 = (float*)malloc(sizeof(float)*size);
    ns = (float*)malloc(sizeof(float)*size);
    theta = (float*)malloc(sizeof(float)*size);
    psi = (float*)malloc(sizeof(float)*size);
    phi = (float*)malloc(sizeof(float)*size);
    thk = (float*)malloc(sizeof(float)*size);
    lpe = (float*)malloc(sizeof(float)*size);
    lro = (float*)malloc(sizeof(float)*size);
    lpe2 = (float*)malloc(sizeof(float)*size);
    ppe = (float*)malloc(sizeof(float)*size);
    pro = (float*)malloc(sizeof(float)*size);
    pss0 = (float*)malloc(sizeof(float)*size);
    gap = (float*)malloc(sizeof(float)*size);
    radialAngles = (float*)malloc(sizeof(float)*size);

    ind = getStackIndexByOrder(); 

  l = 0;
  coil = getCoil();
  for(m=1; m<=mcoils; m++) {
    k = 0;
    for(i=0; i<n; i++) {

    tag = getCurrPlanParams(currentStacks->stacks[i].planType);

    size = 0;
     if(currentStacks->stacks[i].coil == m) {
      sliceInd = getSliceIndexByOrder(i); 
      if(currentStacks->order[i] > 0 && 
		currentStacks->stacks[i].type == type && 
		currentStacks->stacks[i].type != RADIAL) {
		l = 0;
		for(j=0; j<currentStacks->stacks[i].ns; j++) { 
		    if(currentStacks->stacks[i].order[sliceInd[j]] > 0) {
		        pss[l] = currentStacks->stacks[i].pss0  
			+ currentStacks->stacks[i].pss[sliceInd[j]][0] 
			+ currentStacks->stacks[i].pss[sliceInd[j]][1];
			l++;
		    }
		}
		ns[k] = l;
		if(currentStacks->stacks[i].type == VOLUME) volnv2[k] = 1.0;
		else volnv2[k] = 0.0;
		theta[k] = currentStacks->stacks[i].theta;
		psi[k] = currentStacks->stacks[i].psi;
		phi[k] = currentStacks->stacks[i].phi;
		thk[k] = currentStacks->stacks[i].thk*10;
		lpe[k] = currentStacks->stacks[i].lpe;
		lro[k] = currentStacks->stacks[i].lro;
		lpe2[k] = currentStacks->stacks[i].lpe2;
		ppe[k] = currentStacks->stacks[i].ppe;
		pro[k] = currentStacks->stacks[i].pro;
		pss0[k] = currentStacks->stacks[i].pss0;
		gap[k] = currentStacks->stacks[i].gap;
		radialAngles[k] = D361;
		k++;
      } else if(currentStacks->order[i] > 0 && 
		currentStacks->stacks[i].type == type && 
		currentStacks->stacks[i].type == RADIAL) {
	for(j=0; j<currentStacks->stacks[i].ns; j++) 
	  if(currentStacks->stacks[i].order[sliceInd[j]] > 0) {
		angle = currentStacks->stacks[i].radialAngles[sliceInd[j]][0] +
                        currentStacks->stacks[i].radialAngles[sliceInd[j]][1];
		for(l=0; l<9; l++) orientation[l] = 
			currentStacks->stacks[i].orientation[l];
		rotateX(orientation, currentStacks->stacks[i].orientation, angle);
		tensor2euler(&theta1, &psi1, &phi1, orientation);
		theta[k] = theta1;
		psi[k] = psi1;
		phi[k] = phi1;
		c[0] = currentStacks->stacks[i].ppe; 
		c[1] = currentStacks->stacks[i].pro; 
		c[2] = currentStacks->stacks[i].pss0; 
		rotateAngleAboutX(c, angle);
		ns[k] = 1;
		volnv2[k] = 0;
		thk[k] = currentStacks->stacks[i].thk*10;
		lpe[k] = currentStacks->stacks[i].lpe;
		lro[k] = currentStacks->stacks[i].lro;
		lpe2[k] = currentStacks->stacks[i].lpe2;
		ppe[k] = c[0];
		pro[k] = c[1] + currentStacks->stacks[i].radialShift;
		pss0[k] = c[2];
		pss[k] = pss0[k];
		gap[k] = currentStacks->stacks[i].gap;
		radialAngles[k] = angle;
		while(radialAngles[k] > D360) radialAngles[k] -= D360;
                k++;
	}
      }
      free(sliceInd);
     }
    } 
    if(k == 0 || l == 0) continue;

    if(m == coil && k == 0) coil -= 1;

    if(k > 0) {
      sprintf(str,"%.2f",theta[0]);
      for(i=1; i<k; i++) {
	sprintf(strn," %.2f",theta[i]);
	strcat(str, strn);
      } 
      P_setstring(CURRENT, "mtheta", str, m); 

      sprintf(str,"%.2f",psi[0]);
      for(i=1; i<k; i++) {
	sprintf(strn," %.2f",psi[i]);
	strcat(str, strn);
      } 
      P_setstring(CURRENT, "mpsi", str, m); 

      sprintf(str,"%.2f",phi[0]);
      for(i=1; i<k; i++) {
	sprintf(strn," %.2f",phi[i]);
	strcat(str, strn);
      } 
      P_setstring(CURRENT, "mphi", str, m); 

       if(pro[0] < 0) nv = (int)(pro[0]*nd - 0.5);
        else nv = (int)(pro[0]*nd + 0.5);
        pro[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mpro", (double)pro[0], m);

       if(ppe[0] < 0) nv = (int)(ppe[0]*nd - 0.5);
        else nv = (int)(ppe[0]*nd + 0.5);
        ppe[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mppe", (double)ppe[0], m);

       if(pss0[0] < 0) nv = (int)(pss0[0]*nd - 0.5);
        else nv = (int)(pss0[0]*nd + 0.5);
        pss0[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mpss", (double)pss0[0], m);

       if(lro[0] < 0) nv = (int)(lro[0]*nd - 0.5);
        else nv = (int)(lro[0]*nd + 0.5);
        lro[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mlro", (double)lro[0], m);

       if(lpe[0] < 0) nv = (int)(lpe[0]*nd - 0.5);
        else nv = (int)(lpe[0]*nd + 0.5);
        lpe[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mlpe", (double)lpe[0], m);

       if(lpe2[0] < 0) nv = (int)(lpe2[0]*nd - 0.5);
        else nv = (int)(lpe2[0]*nd + 0.5);
        lpe2[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mlpe2", (double)lpe2[0], m);

       if(thk[0] < 0) nv = (int)(thk[0]*nd - 0.5);
        else nv = (int)(thk[0]*nd + 0.5);
        thk[0] = ((float)nv)/nd;
      P_setreal(CURRENT, "mthk", (double)thk[0], m);
    } else {
      P_setstring(CURRENT, "mtheta", "", m); 
      P_setstring(CURRENT, "mpsi", "", m); 
      P_setstring(CURRENT, "mphi", "", m); 
      P_setreal(CURRENT, "mpro", (double)0, m);
      P_setreal(CURRENT, "mppe", (double)0, m);
      P_setreal(CURRENT, "mpss", (double)0, m);
      P_setreal(CURRENT, "mlro", (double)0, m);
      P_setreal(CURRENT, "mlpe", (double)0, m);
      P_setreal(CURRENT, "mlpe2", (double)0, m);
      P_setreal(CURRENT, "mthk", (double)0, m);
    }
   }
   if(coil == getCoil()) {
     P_setreal(CURRENT, "mcoil", (double)coil, 1);
     P_setreal(CURRENT, "mstack", (double)actS, 1);
     sendPnew0(10, " mpro mppe mpss mtheta mpsi mphi mlro mlpe mlpe2 mthk");
     execString("setplan\n");
   } 

    free(ind);
    free(pss);
    free(volnv2);
    free(ns);
    free(theta);
    free(psi);
    free(phi);
    free(thk);
    free(lpe);
    free(lro);
    free(lpe2);
    free(ppe);
    free(pro);
    free(pss0);
    free(gap);
    free(radialAngles);

    return(k);
}

/******************/
int AequalsB(float a, float b, float delta)
/******************/
{
    if(delta == 0.0) delta = 0.01;
/*
fprintf(stderr,"AequalsB %f %f %f\n", a, b, fabs(a - b));
*/
    if(fabs(a - b) >= delta) return(0);
    else return(1);
}

void sendParamsByOrder(int type)
{
    if(mcoils > 1) sendStacksCompress_m(NULL, type);
    else {
	sendPlanParams(currentStacks);
	//sendActiveOrient(currentStacks,activeStack);
    }
}

void iplan_pnewUpdate(char* str)
{
   if(strstr(str,"aip") != NULL && aipGetNumberOfImages() < 1) {
	endIplan();
   }
}

/******************/
int majorOrient(float theta, float psi, float phi)
/******************/
{
    int i = -1;

    snapAngle(&theta);
    snapAngle(&psi);
    snapAngle(&phi);
    if((int)theta == 90 && (int)psi == 90 && (int)phi == 0) i = 2;
    if((int)theta == 90 && (int)psi == 0 && (int)phi == 0) i = 1;
    if((int)theta == 0 && (int)psi == 0 && (int)phi == 0) i = 0;
    if((int)theta == 90 && (int)psi == 90 && (int)phi == 90) i = 5;
    if((int)theta == 90 && (int)psi == 0 && (int)phi == 90) i = 4;
    if((int)theta == 0 && (int)psi == 0 && (int)phi == 90) i = 3;

    return(i);
}

/******************/
void updateEuler(char* paramName, float value,
	float* theta, float* psi, float* phi, float* orientation, 
	float* ppe, float* pro, float* pss0) 
/******************/
{
    float3 c;

	c[0] = *ppe;
	c[1] = *pro;
	c[2] = *pss0;

    	rotateu2m(c, orientation);

	if(strstr(paramName,"theta") != NULL) *theta = value;
	if(strstr(paramName,"psi") != NULL) *psi = value;
	if(strstr(paramName,"phi") != NULL) *phi = value;

    	euler2tensor(*theta, *psi, *phi, orientation);

        rotatem2u(c, orientation);

        *ppe = c[0];
        *pro = c[1];
        *pss0 = c[2];
}

/******************/
double getValue(char* paramName)
/******************/
{
   int actS = 0;   
   double d;
   vInfo           paraminfo;
   int size, i, type;

   if(currentStacks != NULL && currentStacks->numOfStacks > 0) {
        if(activeStack > 0) actS = activeStack;
	activeStack = actS;
	type = currentStacks->stacks[actS].type;
	for(i=0; i<activeStack; i++) {
	   if(currentStacks->stacks[i].type != type) actS--; 
	}
   }
   if(!(P_getVarInfo(CURRENT, paramName, &paraminfo))) {
	size = paraminfo.size;		
	if(actS >= size) actS = size; 
	else actS = actS + 1;
	if(P_getreal(CURRENT, paramName, &d, actS)) d = 0.0;
        return d;
   } else return 0.0;
}

/******************/
int setValue_mcoils()
/******************/
{
    return mcoils;
}

void isoRotationForMM(char *paramName, float value)
{
    int i, nd, nv, m;
    float ppe, pro, pss0, theta, psi, phi, orientation[9];
    double d;
    nd = 100;
    for(i=0; i<mcoils; i++) { 
        m = i + 1;

	if(!P_getreal(CURRENT, "theta", &d, 1)) theta = d;
        if(!P_getreal(CURRENT, "psi", &d, 1)) psi = d;
        if(!P_getreal(CURRENT, "phi", &d, 1)) phi = d;	

	euler2tensor(theta, psi, phi, orientation);

	if(!P_getreal(CURRENT, "mppe", &d, m)) ppe = d;
	if(!P_getreal(CURRENT, "mpro", &d, m)) pro = d;
	if(!P_getreal(CURRENT, "mpss", &d, m)) pss0 = d;

	updateEuler(paramName, value, &theta, &psi, &phi, orientation,
		&ppe, &pro, &pss0);

       if(pro < 0) nv = (int)(pro*nd - 0.5);
        else nv = (int)(pro*nd + 0.5);
        pro = ((float)nv)/nd;
      P_setreal(CURRENT, "mpro", (double)pro, m);

       if(ppe < 0) nv = (int)(ppe*nd - 0.5);
        else nv = (int)(ppe*nd + 0.5);
        ppe = ((float)nv)/nd;
      P_setreal(CURRENT, "mppe", (double)ppe, m);

       if(pss0 < 0) nv = (int)(pss0*nd - 0.5);
        else nv = (int)(pss0*nd + 0.5);
        pss0 = ((float)nv)/nd;
      P_setreal(CURRENT, "mpss", (double)pss0, m);

    }
     sendPnew0(3, " mpro mppe mpss");
     execString("setplan\n");
}

/******************/
void updatePrescription(int act)
/******************/
{
    if(disMode == 0 || currentViews == NULL || currentStacks == NULL) return;
    if(act < 0 || act >= currentStacks->numOfStacks) return;

    calTransMtx(&(currentStacks->stacks[act]));
    calSliceXYZ(&(currentStacks->stacks[act]));
    calOverlayForAllViews(act);
/*
    drawActiveOverlay(0, act);
*/
}

/******************/
static int argtest(int argc, char* argv[], char* argname)
/******************/
{
  int found = 0;
  int n;

  n = argc;

  while ((--argc) && !found)
    if(strcasecmp(*++argv,argname) == 0) found = n - argc;
  return(found);
}

/******************/
int imagesChanged()
/******************/
{
    float theta, psi, phi, scale[3];
    int i, len, epsi = 0.009;
    CImgInfo_t* imgInfor;
    int* ids;

    if(currentStacks == NULL || currentViews == NULL) return(0);

    len = aipGetNumberOfImages(); 
    if(len <= 0) {
	endIplan();
	return(0);
    }
    if(len != IBlen) return(1);

    if(len > 0) {

      ids = (int*)malloc(sizeof(int)*len);
      aipGetImageIds(len, ids);

      imgInfor = (CImgInfo_t*)malloc(sizeof(CImgInfo_t));

      for(i=0; i<len; i++) {

        aipGetImageInfo(ids[i], imgInfor);

        scale[0] = imgInfor->pixelsPerCm;
        scale[1] = -imgInfor->pixelsPerCm;
        scale[2] = -imgInfor->pixelsPerCm;

        theta = imgInfor->euler[0];
        psi = imgInfor->euler[1];
        phi = imgInfor->euler[2];

	if(fabs(theta - currentViews->views[i].slice.theta) < epsi) return(1);
	if(fabs(psi - currentViews->views[i].slice.psi) < epsi) return(1);
	if(fabs(phi - currentViews->views[i].slice.phi) < epsi) return(1);
        if(fabs(imgInfor->location[0] - currentViews->views[i].slice.ppe) < epsi) return(1);
	if(fabs(imgInfor->location[1] - currentViews->views[i].slice.pro) < epsi) return(1);
	if(fabs(imgInfor->location[2] - currentViews->views[i].slice.pss0) < epsi) return(1);
	if(fabs(imgInfor->roi[0] - currentViews->views[i].slice.lpe) < epsi) return(1);
	if(fabs(imgInfor->roi[1] - currentViews->views[i].slice.lro) < epsi) return(1);
	if(fabs(imgInfor->roi[2] - currentViews->views[i].slice.lpe2) < epsi) return(1);
      }
      free(ids);
      free(imgInfor);
    } 
    return(0);
}

/******************/
void setPosZero(int planType)
/******************/
{
    int i;

    planParams *tag = getCurrPlanParams(planType);
    if(currentStacks == NULL || tag == NULL) return;

    for(i=0; i<currentStacks->numOfStacks; i++) {
        if(currentStacks->stacks[i].planType != planType) continue;
	if(!usepos2) {
	  currentStacks->stacks[i].ppe = 0; 
	  sendValueToVnmr(tag->pos1.name, currentStacks->stacks[i].ppe); 
       	  sendPnew0(1, tag->pos1.name);
        }
	if(!usepos1) {
	  currentStacks->stacks[i].pro = 0; 
	  sendValueToVnmr(tag->pos2.name, currentStacks->stacks[i].pro); 
       	  sendPnew0(1, tag->pos2.name);
        }
	if(!usepos3) {
	  currentStacks->stacks[i].pss0 = 0; 
	  sendValueToVnmr(tag->pos3.name, currentStacks->stacks[i].pss0); 
       	  sendPnew0(1, tag->pos3.name);
        }
    	calTransMtx(&(currentStacks->stacks[i]));
    	calSliceXYZ(&(currentStacks->stacks[i]));
    }
    calOverlays();
    drawOverlays();
}

int getCubeFOV() { return cubeFOV;}

void setCubeFOV() {
   int i;
   float f;
   if(currentStacks == NULL || currentStacks->numOfStacks<1) return;
   if(activeStack<0 || activeStack >= currentStacks->numOfStacks) { 
      activeStack=0;
   }

   for(i=0; i<currentStacks->numOfStacks; i++) {
     if(currentStacks->stacks[i].type == REGULAR) {
	f=currentStacks->stacks[i].lro+currentStacks->stacks[i].lpe;
	currentStacks->stacks[i].lro = f/2;
	currentStacks->stacks[i].lpe = f/2;
     } else if(currentStacks->stacks[i].type == VOLUME ||
	currentStacks->stacks[i].type == VOXEL) {
	f=currentStacks->stacks[i].lro+currentStacks->stacks[i].lpe+currentStacks->stacks[i].lpe2;
	currentStacks->stacks[i].lro = f/3;
	currentStacks->stacks[i].lpe = f/3;
	currentStacks->stacks[i].lpe2 = f/3;
     }
   }
    calTransMtx(&(currentStacks->stacks[activeStack]));
    calSliceXYZ(&(currentStacks->stacks[activeStack]));
    calOverlayForAllViews(activeStack);
    drawActiveOverlay(0, activeStack);
    sendParamsByOrder(ALLSTACKS);
}

int getPlanType(char *str) {
   int type;
   if (isdigit(str[0])) type = atoi(str);
   else type = getTypeByTagName(str);
   return type;
}

/******************/
int gplan(int argc, char *argv[], int retc,char *retv[])
/******************/
{
    int i, n;
    int type, mode, ind;
    char* paramName;
    float value;
    double d;
    float3 dim, pos;
    char str[MAXSTR];

    if ( (n = argtest(argc,argv,"getDimPos_m")) ) {
        type = VOXEL;
        if(!P_getreal(CURRENT, "iplanType", &d, 1)) type = (int)d;
        getDimPos_m(CURRENT, type, dim, pos);
	if(retc > 2) {
	   retv[0] = realString(dim[0]);
	   retv[1] = realString(dim[1]);
	   retv[2] = realString(dim[2]);
	}
	if(retc > 5) {
	   retv[3] = realString(pos[0]);
	   retv[4] = realString(pos[1]);
	   retv[5] = realString(pos[2]);
	}
	if(retc > 6)
	   retv[6] = realString((double)type);
	return(0);
    }

    aip_mnumypnts = win_height - 4;
    defaultType = getIplanType();
/*
   for(i=0; i<argc; i++)
	fprintf(stderr,"gplan command %s\n", argv[i]);
*/
/* do this every time a command is executed */
/* always initialize mark mode */

    if(marks != NULL && isMarking != -1) {
	stopMarking();
/*
	marks[isMarking].isUsing = 0;
	isMarking = -1;
        writelineToVnmrJ("vnmrjcmd", "cursor pointer");
*/
    }

/* check to see which images has changed */
/*
    if(aipOwnsScreen() && imagesChanged()) n = updateOverlays();
*/
    if((i = imagesChanged())) n = updateOverlays();
    if(n == -9) RETURN;

/* the following commands don't involve calculations or displays */
/* they don't require that iplan has been started. */

    if ( (n = argtest(argc,argv,"centerTarget")) ) {
	centerTarget();

/*
    } else if ( (n = argtest(argc,argv,"setOrientValue")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) setPlanValue(paramName, argv[n]);
        }
	RETURN;
*/
    } else if ( (n = argtest(argc,argv,"getIndex")) ) {
	if(retc > 0) {
            retv[0] = realString((double)getActStackInd());
            return(0);
        }
    } else if ( (n = argtest(argc,argv,"getValue")) ) {
 	if(argc > ++n && retc > 0) { 
	    retv[0] = realString(getValue(argv[n]));
	    return(0);
	} else if(retc > 0) {
	    retv[0] = realString((double)0);
	    return(0);
	}
    }
    else if ( (n = argtest(argc,argv,"setValue")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) setPlanValue(paramName, argv[n]);
        }
	RETURN;
    }
/*
    if ( (n = argtest(argc,argv,"setDefaultSlices")) ) {
	value = numDefaultSlices;
	if(argc > ++n) value = atof(argv[n]);
	numDefaultSlices = value;
    }

    else if ( (n = argtest(argc,argv,"setDefaultSize")) ) {
	if(argc > ++n) {
	   if(defaultType == VOXEL) {
		defaultVoxSize = atof(argv[n])*10.0;
	   } else {
		defaultSize = atof(argv[n]);
	   }
	}
    }

    else if ( (n = argtest(argc,argv,"setDefaultThk")) ) {
	value = defaultThk;
	if(argc > ++n) value = 0.1*atof(argv[n]);
	defaultThk = value;
    }

    else if ( (n = argtest(argc,argv,"getNumOfStacks")) ) {
	if(currentStacks == NULL) value = 0;
	else value = currentStacks->numOfStacks;
	if(retc > 0) retv[0] = realString((double)value);
	return(0);
    }

    else if ( (n = argtest(argc,argv,"getDefaultSlices")) ) {
	if(retc > 0) retv[0] = realString((double)numDefaultSlices);
	return(0);
    }

    else if ( (n = argtest(argc,argv,"getDefaultSize")) ) {
        if(!(P_getreal(CURRENT, "iplanType", &d, 1))) defaultType = (int)d;
	if(defaultType == VOXEL) {
	    value = defaultVoxSize*0.1; 
        } else {
	    value = defaultSize;
	}
	if(retc > 0) retv[0] = realString(value);
	return(0);
    }

    else if ( (n = argtest(argc,argv,"getDefaultThk")) ) {
	if(retc > 0) retv[0] = realString(10*defaultThk);
	return(0);
    }
*/
    else if((n = argtest(argc,argv,"currParams"))) {
        printCurrTag();
    }

    else if ( (n = argtest(argc,argv,"setDefaultType")) ) {
    	createCommonPlanParams();	
	type = REGULAR;
	i=0;
	clearVar("iplanDefaultType");
 	while(argc > ++n) {
	 i++;
	 type = getPlanType(argv[n]);
   	 createPlanParams(type);
	 P_setreal(CURRENT, "iplanDefaultType",(double)type,i);
	 if(i==1) {
           updateUsePos(type);
           defaultType = type;
	 }
	}
        
   	P_setreal(CURRENT, "iplanType", (double)(defaultType), 1);
    }

    else if ( (n = argtest(argc,argv,"getGapMode")) ) {
	if(retc > 0) retv[0] = realString((double)gapFixed);
	return(0);
    }

    else if ( (n = argtest(argc,argv,"getUseppe")) ) {
	if(retc > 0) retv[0] = realString((double)usepos2);
	return(0);
    }

    else if ( (n = argtest(argc,argv,"setGapMode")) ) {
 	if(argc > ++n) setGapMode(atoi(argv[n]));
    }

    else if ( (n = argtest(argc,argv,"setUseppe")) ) {
 	if(argc > ++n) usepos2= atoi(argv[n]);
	setUseParam(defaultType, "ppe", usepos2);
	if(usepos2 == 0) setPosZero(defaultType);
    }

    else if ( (n = argtest(argc,argv,"notuse")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) type = getPlanType(argv[n]);
	  else type = defaultType;
 	  setUseParam(type, paramName, 0);
	  updateUsePos(type);
	  setPosZero(type);
        }
    }

    else if ( (n = argtest(argc,argv,"use")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) type = getPlanType(argv[n]);
	  else type = defaultType;
 	  setUseParam(type, paramName, 1);
	  updateUsePos(type);
        }
    }

    else if ( (n = argtest(argc,argv,"enable")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) type = getPlanType(argv[n]);
	  else type = defaultType;
 	  setUseParam(type, paramName, 1);
        }
    }

    else if ( (n = argtest(argc,argv,"disable")) ) {
        if(argc > ++n) {
	  paramName = argv[n];
          if(argc > ++n) type = getPlanType(argv[n]);
	  else type = defaultType;
 	  setUseParam(type, paramName, -1);
        }
    }

    else if ( (n = argtest(argc,argv,"getstatus")) ) {
 	if(argc > ++n && retc > 0) {
	  paramName = argv[n];
          if(argc > ++n) type = getPlanType(argv[n]);
	  else type = defaultType;
	  retv[0] = realString((double)getUseParam(type, paramName));
	}
    }


    else if ( (n = argtest(argc,argv,"cubeFOV")) ) {
 	if(argc > ++n) cubeFOV= atoi(argv[n]);
	if(cubeFOV == 1) setCubeFOV();
        if(retc > 0) retv[0] = realString((double)cubeFOV);
    }

    else if ( (n = argtest(argc,argv,"isGplan")) ) {
	if(retc > 0 && currentStacks != NULL && currentViews != NULL && currentStacks->numOfStacks>0) 
		retv[0] = realString(1.0);
	else if(retc > 0) retv[0] = realString(0.0);
    }

/* the following commands involve calculations or displays. */
/* they do nothing if iplan havn't started yet (except "startIplan") */

    else if ( (n = argtest(argc,argv,"startIplan")) ) {
	startIplan(-1);
    }

    else if ( (n = argtest(argc,argv,"addVoxel")) ) {
        if(argc > ++n) {
	    addVoxelMode = atoi(argv[n]);
	} else addVoxelMode = 1;
	if(addVoxelMode > 0) {
	  P_setreal(CURRENT, "iplanType", (double)VOXEL, 1);
	  if(currentStacks == NULL || currentViews == NULL) {
	    if(startIplan0(-1) == -9) RETURN;
	  }
          writelineToVnmrJ("vnmrjcmd", "cursor builtin-crosshair");
          //writelineToVnmrJ("vnmrjcmd", "cursor aippoint");
	} else
          writelineToVnmrJ("vnmrjcmd", "cursor pointer");
    }
    else if ( (n = argtest(argc,argv,"startMarking")) ) {
        if(argc > ++n) {
	    nMarks = atoi(argv[n]);
	    clearMarking();
	    iMarks = 0;
	} else nMarks = 1;
 	if(retc > 0) {
	    retv[0] = realString((double)startMarking());
	} else {
	    startMarking();
	} 
    }
    else if ( (n = argtest(argc,argv,"stopMarking")) ) {
	stopMarking();
    }
    else if ( (n = argtest(argc,argv,"clearMarking")) ) {
	clearMarking();
    }
    else if ( (n = argtest(argc,argv,"setMarkMode")) ) {
 	if(argc > ++n) {
	    mode = atoi(argv[n]);
 	    setMarkMode(mode);
	}
    }

    else if ( (n = argtest(argc,argv,"refresh")) ) {
	if(currentStacks != NULL && currentViews != NULL) {
          resetActive();
	  updateType();
    	  sendParamsByOrder(ALLSTACKS);
	  disMode = 1;
	  updateOverlays();
	} else if(startIplan0(-1) != -9) {
	  disMode = 1;
	  getCurrentStacks();
	}
    }

    else if ( (n = argtest(argc,argv,"showOverlays")) ) {
 	if(argc > ++n) showOverlays(atoi(argv[n]));
	else showOverlays(-1);
    }

    else if ( (n = argtest(argc,argv,"setDisplayStyle")) ) {
 	if(argc > ++n) setDisplayStyle(atoi(argv[n]));
	else setDisplayStyle(-1);
    }

    else if ( (n = argtest(argc,argv,"setDrawInterSection")) ) {
 	if(argc > ++n) setDrawInterSection(atoi(argv[n]));
	else setDrawInterSection(-1);
    }

    else if ( (n = argtest(argc,argv,"setDraw3D")) ) {
 	if(argc > ++n) setDraw3D(atoi(argv[n]));
	else setDraw3D(-1);
    }

    else if ( (n = argtest(argc,argv,"setDrawAxes")) ) {
 	if(argc > ++n) setDrawAxes(atoi(argv[n]));
	else setDrawAxes(-1);
    }

    else if ( (n = argtest(argc,argv,"setDrawOrders")) ) {
 	if(argc > ++n) setDrawOrders(atoi(argv[n]));
	else setDrawOrders(-1);
    }

    else if ( (n = argtest(argc,argv,"setFillPolygon")) ) {
 	if(argc > ++n) setFillPolygon(atoi(argv[n]));
	else setFillPolygon(-1);
    }

    else if ( (n = argtest(argc,argv,"addType")) ) {
	type = defaultType;
 	if(argc > ++n) {
	 type = getPlanType(argv[n]);
	}
 	if(argc > ++n) {
	  addType(type, argv[n]);
	} else {
	  addType(type,"");
        }
    }
    else if ( (n = argtest(argc,argv,"deleteType")) ) {
	type = defaultType;
 	if(argc > ++n) {
	 type = getPlanType(argv[n]);
	}
	deleteType(type);
    }

    else if ( (n = argtest(argc,argv,"deleteSelected")) ) {
	type = -1;
 	if(argc > ++n) {
	 type = getPlanType(argv[n]);
	}
 	deleteSelected(type);
    }

    else if ( (n = argtest(argc,argv,"deleteSlice")) ) {
 	deleteSlice();
    }

    else if ( (n = argtest(argc,argv,"restoreStack")) ) {
 	restoreStack();
    }

    else if ( (n = argtest(argc,argv,"removeAstack")) ) {
	ind = activeStack;
        if(argc > ++n) ind = atoi(argv[n]);
	removeAstack(ind);
    }

    else if ( (n = argtest(argc,argv,"clearStacks")) ) {
	clearStacks();
    }

    else if ( (n = argtest(argc,argv,"disStripes")) ) {
	disStripes();
    }

    else if ( (n = argtest(argc,argv,"disCenterLines")) ) {
	disCenterLines();
    }

    else if ( (n = argtest(argc,argv,"saveMilestoneStacks")) ) {
	saveMilestoneStacks();
    }

    else if ( (n = argtest(argc,argv,"savePrescription")) ) {
        if(argc > ++n) {
	    strcpy(str,argv[n]);  // full path
	    if(argc > ++n) { 
	      type = getPlanType(argv[n]);
	      savePrescriptForType(str,type);
	    } else savePrescript(str);
	}
    }

    else if ( (n = argtest(argc,argv,"alternateSlices")) ) {
 	if(argc > ++n) mode = atoi(argv[n]);
	else mode = -1;
        alternateOrders = mode;
        P_getstring(CURRENT, "orient", str, 1, MAXSTR);
        if(currentStacks != NULL && currentStacks->numOfStacks == 3 && 
	strcmp(str,"3orthogonal") == 0) { 
	   for(i=0; i<currentStacks->numOfStacks; i++) alternateSlices(mode, i);
	} else alternateSlices(mode, -1);
	setDrawOrders(1);
    }

    else if ( (n = argtest(argc,argv,"eraseOverlays")) ) {
	disMode = 0;
	eraseOverlays();
    }

    else if ( (n = argtest(argc,argv,"endIplan")) ) {
	endIplan();
    }

/* the following commands also involve calculations or displays. */
/* but they automatically start iplan if it hasn't started yet */

    else if ( (n = argtest(argc,argv,"loadPrescription")) ) {
        if(argc > ++n) {
	    readPlanPars(argv[n]);
	    if(currentStacks == NULL || currentViews == NULL) {
               if(startIplan0(-1) == -9) RETURN;
	    }
	    getCurrentStacks();
	}
    }

    else if ( (n = argtest(argc,argv,"addAstack")) ) {

	type = defaultType;
 	if(argc > ++n) {
	 type = getPlanType(argv[n]);
	}

	if(type == VOXCSI2D) {
	    addAstack(VOXEL);
	    value = currentStacks->stacks[0].lpe2;
	    if(value == 0) value = 0.1*defaultVoxSize;
            P_setreal(CURRENT, "thk", 10.0*value, 1);
	    addAstack(type);
	} else if(type == VOXCSI3D) {
	    addAstack(VOXEL);
	    addAstack(type);
	} else addAstack(type);
    }

    else if ( (n = argtest(argc,argv,"update")) ) {
	if(currentStacks != NULL && currentViews != NULL) {
	   getCurrentStacks();
	}
    }

    else if ( (n = argtest(argc,argv,"getCurrentStacks")) ) {
	getCurrentStacks();
    }

    else if ( (n = argtest(argc,argv,"getPrevStacks")) ) {
	copyPrescript(prevStacks, currentStacks);

        resetActive();
    	updateType();
    	sendParamsByOrder(ALLSTACKS);

    	calOverlays();
    	drawOverlays();
    }

    else if ( (n = argtest(argc,argv,"getActiveStacks")) ) {
	getActiveStacks();
    }

    else if ( (n = argtest(argc,argv,"getDefaultStacks")) ) {
	getCurrentStacks();
    }

    else if ((n = argtest(argc,argv,"getMilestoneStacks")) ) {
	getMilestoneStacks();
    }

    else if ( (n = argtest(argc,argv,"euler")) ) {
	int theta = 0.0;
	int psi = 0.0;
	int phi = 0.0;
	if(argc > ++n) theta = atoi(argv[n]); 
 	if(argc > ++n) psi = atoi(argv[n]); 
 	if(argc > ++n) phi = atoi(argv[n]); 
	makeStackByEuler(theta, psi, phi, defaultType);
    }

    else if ( (n = argtest(argc,argv,"getTransverse")) ) {
	makeStackByEuler(0.0, 0.0, 0.0, defaultType);
    }

    else if ( (n = argtest(argc,argv,"getCoronal")) ) {
	makeStackByEuler(D90, 0.0, 0.0, defaultType);
    }

    else if ( (n = argtest(argc,argv,"getSagittal")) ) {
	makeStackByEuler(D90, D90, 0.0, defaultType);
    }

    else if ( (n = argtest(argc,argv,"get3orthogonal")) ) {
	make3orthogonal(defaultType);	
    }

    else if ( (n = argtest(argc,argv,"getActStackInfo")) ) {
 	if(argc > ++n && retc > 0) {
	    retv[0] = realString((double)getActStackInfo(argv[n]));
	} else if(retc > 0) {
	    retv[0] = realString((double)-1);
	} 
    }

    if ( (n = argtest(argc,argv,"calcControlPlane")) ) {
        int imageType = 0;
        int tagType = 14;
        int controlType = 15;
  	int show = 0;
	if(argc > ++n) imageType = getPlanType(argv[n]); 
	if(argc > ++n) tagType = getPlanType(argv[n]); 
	if(argc > ++n) controlType = getPlanType(argv[n]); 
	if(argc > ++n) show = atoi(argv[n]); 
	calcASLControlPlane(imageType, tagType, controlType, show);
    }
    RETURN;
}

/******************/
int getActStackInd()
/******************/
{
    int i, ind, planType, act = 0;

    if(currentStacks == NULL || currentStacks->numOfStacks < 2) return(1);

    if(selectedOverlay >= 0 && selectedOverlay < currentStacks->numOfStacks)
        act = selectedOverlay;
    planType=currentStacks->stacks[act].planType;

    ind=0;
    for(i=0; i<=act; i++) {
        if(currentStacks->stacks[i].planType == planType) ind++;
    }
    return ind;
}

/******************/
int getActStackInd2()
/******************/
{
    int i;

    if(currentStacks == NULL || currentViews == NULL) return(-1);

/* return first slices plan */

    for(i=0; i<currentStacks->numOfStacks; i++)
	if(currentStacks->stacks[i].type == REGULAR) {
	    return i;
	}
    return -1;
}

/******************/
int isAlternated(int act)
/******************/
/* j=0, ordered, j=1 alternated, j=2 reverced */
{
    int i, n, j = 0;

    if(currentStacks == NULL) return(-1);

    n = currentStacks->stacks[act].ns;
    /* check to see if ordered */
    for(i=0; i<n; i++) {
	if(currentStacks->stacks[act].order[i] != i+1) {
	    j = 2;
	    break;
	}
    }

    /* check to see if reverce ordered */
    if(j != 0) for(i=0; i<n; i++) {
	if(currentStacks->stacks[act].order[i] != n-i) {
	    j = 1;
	    break;
	}
    }

    /* check to see if revercely alternated */
    if(j == 1 && currentStacks->stacks[act].order[0] != 1) j = 3;

    return(j);
}

/******************/
int getCurrentOrderMode()
/******************/
{
     float* pss;
     int i, j, r, ns, err=0;
     vInfo           paraminfo;
     double d;

     if(!P_getVarInfo(CURRENT, "pss", &paraminfo)) ns = paraminfo.size;
     else return(0);

     pss = (float*)malloc(sizeof(float)*ns);
     for(i=0; i<ns; i++) {
          if(!(r = P_getreal(CURRENT, "pss", &d, i+1))) {
              pss[i] = d;
          } else err++;
     }

    j = 0;
    if(err == 0) {

    /* check to see if ordered */
      for(i=1; i<ns; i++) {
	if(pss[i] < pss[i-1]) {
	    j = 2;
	    break;
	}
      }

    /* check to see if reverce ordered */
      if(j != 0) for(i=1; i<ns; i++) {
	if(pss[i] > pss[i-1]) {
	    j = 1;
	    break;
	}
      }

    /* check to see if revercely alternated */
      if(j == 1 && pss[ns-1] < pss[0]) j = 3;

    }
    free(pss);
    return(j);
}

/******************/
int getActStackInfo(char* key)
/******************/
/* key is one of the following: */
/* ns, type, color, fill, drawInterSection, draw3D, drawAxes, style */
/* and drawOrder alternateOrder */  
{
    int act = getActStackInd2();
    
    if(strcasecmp(key, "drawInterSection") == 0) {
	return drawIntersection;
    } else if(strcasecmp(key, "draw3D") == 0) {
	return draw3DStack;
    } else if(strcasecmp(key, "drawAxes") == 0) {
        return drawStackAxes;
    } else if(strcasecmp(key, "fill") == 0) {
        return fillIntersection;
    } else if(strcasecmp(key, "style") == 0) {
	return overlayStyle;
    } else if(strcasecmp(key, "drawOrder") == 0) {
	return (showStackOrders);
    } else if(act != -1 && strcasecmp(key, "alternateOrder") == 0) {
        return(isAlternated(act));
    } return 0;
}

/******************/
void alternateSliceOrders(int* orders, int ns)
/******************/
{
    int i, forward = 0;
    int ns2;

    ns2 = ns%2;
    ns2 += ns/2;

    for(i=0; i<ns; i++) if(orders[i] != ns-i) forward = 1;

/* alternate orders 1 3 5... 2 4 6...*/
    if(forward && ns%2) {
	for(i=0; i<ns2; i++) orders[i] = 1 + i*2;
	for(i=0; i<ns2-1; i++) orders[ns2+i] = 2 + i*2;
    } else if(forward) {
	for(i=0; i<ns2; i++) orders[i] = 1 + i*2;
	for(i=0; i<ns2; i++) orders[ns2+i] = 2 + i*2;
    } else if(ns%2) {
	for(i=0; i<ns2; i++) orders[ns2-1-i] = 1 + i*2;
	for(i=0; i<ns2-1; i++) orders[ns-1-i] = 2 + i*2;
    } else {
	for(i=0; i<ns2; i++) orders[ns-1-i] = 1 + i*2;
	for(i=0; i<ns2; i++) orders[ns2-1-i] = 2 + i*2;
    }

/* randomize orders */ 
/* does not work as well as alternating */
/*
 	for(i=0; i<ns; i++) {
	   
	    p = (int)(ns*rand()/rmax); 
	    o = orders[p];
	    if(p < ns/2) { 
	    	for(j=p; j<ns-1; j++) {
		   orders[j] = orders[j+1];  
		}
		orders[ns-1] = o;
 	    } else {
		for(j=p; j>0; j--) {
		    orders[j] = orders[j-1]; 
		}
		orders[0] = o;
	    }
	}	
*/

}

/******************/
void alternateSlices(int mode, int act)
/******************/
/* mode = 0, order, mode == 1 alternate, */
/* mode == 2 reverse, mode == 3 reverse alternate, mode < 0 toggle */
{
    int i, ns;
    int* orders;

    if(mode < 0) {
	if(alternateOrders < 2) alternateOrders++;
	else alternateOrders = 0;
    } else if(mode < 4) alternateOrders = mode;
    else return;

    if(currentStacks == NULL || currentStacks->numOfStacks <= 0
	|| currentViews == NULL) {
	if(alternateOrders == 0) execString("sliceorder('i')\n");
	else if(alternateOrders == 1) execString("sliceorder('a')\n");
	else if(alternateOrders == 2) execString("sliceorder('d')\n");
	else if(alternateOrders == 3) execString("sliceorder('r')\n");
	return;
    }

    if(act < 0) act = getActStackInd2();

    if(act == -1 && currentStacks->stacks[0].ns > 1) act = 0;
    else if(act == -1) return;

    ns = currentStacks->stacks[act].ns;
    if(ns < 2) return;

    orders = (int*)malloc(sizeof(int)*ns);

    if(alternateOrders == 0 || alternateOrders == 1) 
	for(i=0; i<ns; i++) orders[i] = i+1;
    else if(alternateOrders == 2 || alternateOrders == 3) 
	for(i=0; i<ns; i++) orders[i] = ns-i;

    if(alternateOrders == 1 || alternateOrders == 3) {
	alternateSliceOrders(orders, ns);  
    }

    for(i=0; i<ns; i++) currentStacks->stacks[act].order[i] = orders[i];
    free(orders);

    sendPssByOrder(act);
}

/******************/
void showOverlays(int mode)
/******************/
/* mode = 0 no display, mode = 1 show, mode = -1 toggle */
{
    if(currentStacks == NULL || currentViews == NULL) return;

    if(mode < 0) {
	if(disMode > 0) {
	   disMode = 0;
	   eraseOverlays();
           return;
        } else if(disMode == 0) {
	   disMode = 1;
	   updateOverlays();
           return;
	}
    } else if(mode == 0) {
	disMode = 0;
	eraseOverlays();
    } else if(mode > 0) { 
	disMode = 1;
	updateOverlays();
    }
}

/******************/
void setDisplayStyle(int mode)
/******************/
/* mode = 0 stripes, mode > 0 lines */
{
    overlayStyle = mode;

    if(currentStacks == NULL || currentViews == NULL) return;
    drawOverlays();
}

/******************/
void setDrawInterSection(int mode)
/******************/
/* mode = 0 not draw, mode > 0 draw */
{
    drawIntersection = mode;

    if(currentStacks == NULL || currentViews == NULL) return;
    drawOverlays();
}

/******************/
void setDraw3D(int mode)
/******************/
/* mode = 0 not draw, mode > 0 draw */
{
    draw3DStack = mode;

    if(currentStacks == NULL || currentViews == NULL) return;
    drawOverlays();
}

/******************/
void setDrawAxes(int mode)
/******************/
/* mode = 0 not draw, mode > 0 draw */
{
    drawStackAxes = mode;

    if(currentStacks == NULL || currentViews == NULL) return;
    drawOverlays();
}

/******************/
void setDrawOrders(int mode)
/******************/
/* mode = 0 not draw, mode > 0 draw */
{
    if(mode < 0) { 
        if(showStackOrders == 1) showStackOrders = 0;
        else if(showStackOrders == 0) showStackOrders = 1;
    } else showStackOrders = mode;

    if(currentStacks == NULL || currentViews == NULL) return;

    drawOverlays();
}

/******************/
void setFillPolygon(int mode)
/******************/
/* mode = 0 not fill, mode > 0 fill */
{
    fillIntersection = mode;

    if(currentStacks == NULL || currentViews == NULL) return;
    drawOverlays();
}

/******************/
void setGapMode(int mode)
/******************/
/* mode = 0 not fixed, mode > 0 fixed */
{
    gapFixed = mode;
}

/******************/
void deleteSlice()
/******************/
{
    if(orderMode) return;
    if(currentStacks == NULL || currentViews == NULL) return;

    if(activeSlice < 0 || activeStack < 0 || activeView < 0 ||
	currentViews->numOfViews == 0 || 
	currentStacks->numOfStacks == 0 ||
	currentOverlays->numOfOverlays == 0 ||
	currentOverlays->overlays[activeView].numOfStacks == 0 ||
	activeSlice >= currentStacks->stacks[activeStack].ns ||
	activeSlice >= 
	  currentOverlays->overlays[activeView].stacks[activeStack].numOfStripes ||
	activeView >= currentViews->numOfViews ||
	activeView >= currentOverlays->numOfOverlays ||
	activeStack >= currentStacks->numOfStacks ||
	activeStack >= currentOverlays->overlays[activeView].numOfStacks) return;

    currentStacks->stacks[activeStack].order[activeSlice] = 
	-1*fabs(currentStacks->stacks[activeStack].order[activeSlice]);
/*
    drawActiveOverlay(0, activeStack);
*/
    updateType();
    resetActive();
    sendParamsByOrder(ALLSTACKS);
    drawOverlays();

    activeSlice = -1;
    //activeStack = -1;
    //activeView = -1;

}

/******************/
void restoreStack()
/******************/
{
    int i, act = 0;

    if(currentStacks == NULL || currentViews == NULL) return;
    if(act >= currentStacks->numOfStacks) return; 

    if(activeStack > 0) act = activeStack;

    if(currentStacks->stacks[act].type == REGULAR) 
      for(i=0; i<currentStacks->stacks[act].ns; i++) {
	currentStacks->stacks[act].order[i] = 
		(int)fabs(currentStacks->stacks[act].order[i]);	
	currentStacks->stacks[act].pss[i][1] = 0.0;
      }
    else if(currentStacks->stacks[act].type == RADIAL)
      for(i=0; i<currentStacks->stacks[act].ns; i++) {
	currentStacks->stacks[act].order[i] = 
		(int)fabs(currentStacks->stacks[act].order[i]);	
	currentStacks->stacks[act].radialAngles[i][1] = 0.0;
      }

    calTransMtx(&(currentStacks->stacks[act]));
    calSliceXYZ(&(currentStacks->stacks[act]));
    calOverlayForAllViews(act);
/*
    drawActiveOverlay(0,act);
*/
    updateType();
    resetActive();
    sendParamsByOrder(ALLSTACKS);
    drawOverlays();

}

/******************/
int getNumOfNewMarks()
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) { 
	getLocation2(marks[i].currentLocation, &(marks[i].view), 
		&(marks[i].stack), &(marks[i].slice));
	if(marks[i].view != -1 && marks[i].stack == -1) j++;
    }
    return j;
}

/******************/
int getNumOfMarks(int view, int stack)
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) 
	if(marks[i].view == view && marks[i].stack == stack) j++;

    return j;
}

/******************/
int getNewMark(int n)
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) { 
	getLocation2(marks[i].currentLocation, &(marks[i].view),
                &(marks[i].stack), &(marks[i].slice));
	if(marks[i].view != -1 && marks[i].stack == -1) j++;
	if(j == n) return i;
    }

    return -1;
}

/******************/
void setOrderZero()
/******************/
{
    int j, k;

    if(currentStacks == NULL && currentStacks->numOfStacks < 1) return;

    for(j=0; j<currentStacks->numOfStacks; j++) {

      for(k=0; k<currentStacks->stacks[j].ns; k++)
	currentStacks->stacks[j].order[k] = 0.0;
    }
}

/******************/
void updateOrders()
/******************/
{
    int i, j, k;
    int first2, last2;
    int o2;
    int* orders;

    for(j=0; j<currentStacks->numOfStacks; j++) { 

      o2 = 0;
      last2 = 0;
      first2 = 1;
      for(k=0; k<currentStacks->stacks[j].ns; k++) {
	if(currentStacks->stacks[j].order[k] != 0) {
	    o2++;
	    if(currentStacks->stacks[j].order[k] > last2) 
		last2 = currentStacks->stacks[j].order[k];
	    if(currentStacks->stacks[j].order[k]  == 1) 
		first2 = k;
	}
      }

      if(o2 == 0) { 
	for(k=0; k<currentStacks->stacks[j].ns; k++)
            currentStacks->stacks[j].order[k] = 
		prevStacks->stacks[j].order[k];
      } else if(first2 == currentStacks->stacks[j].ns-1) { 
	for(k=currentStacks->stacks[j].ns-1; k>=0; k--)
	    if(currentStacks->stacks[j].order[k] == 0) {
		last2++;
		currentStacks->stacks[j].order[k] = last2;
	    }
      } else {
	for(k=0; k<currentStacks->stacks[j].ns; k++)
	    if(currentStacks->stacks[j].order[k] == 0) {
		last2++;
		currentStacks->stacks[j].order[k] = last2;
	    }
      }

/* there is a twist here. in orderMode, the orders are */
/* the "user order", i.e., the order slices will be acquired. */
/* but in non-orderMode, the orders are the mapping indices */
/* i.e., use slice[order[i]] to get the correct order. */
/* acquire index are displayed, but mapping indices are used */
/* internally the order the slices. */
/* this function is called at the end of orderMode, so acquire */
/* orders need to be converted to mapping orders. */

      orders = (int*)malloc(sizeof(int)*currentStacks->stacks[j].ns);

      for(k=0; k<currentStacks->stacks[j].ns; k++) {
	for(i=0;i<currentStacks->stacks[j].ns; i++) {
	    if(currentStacks->stacks[j].order[i] == k+1) {
	       orders[k] = i+1;
	       break;
	    }
        }
      }
      for(k=0; k<currentStacks->stacks[j].ns; k++) 
	currentStacks->stacks[j].order[k] = orders[k];

      free(orders);
    }
}

/******************/
void getCurrentFrame(float2 p, int* view)
/******************/
{
    int i;
    *view = -1;

    if(currentViews == NULL) return;

    for(i=0; i<currentViews->numOfViews; i++) {
	if(containedInRectangle(p, currentViews->views[i].framestx,
            currentViews->views[i].framesty,
            currentViews->views[i].framewd,
            currentViews->views[i].frameht)) {
            *view = i;
            return;
        }
    }

}

/******************/
void getCurrentVertice(float2 p, int* view, int* stack, int* v1, int* v2, int *handle)
/******************/
{
    *stack = -1;
    *view = -1;
    *v1 = -1;
    *v2 = -1;
    *handle = -1;

        getLocation(p, view, stack, NULL, handle, v1, v2, NULL);
   handleMode = SQUARE;
}

/******************/
static void mouse_move(int x, int y0, int button, int combo, int status)
/******************/
{
    int y; 
    float2 p;
    int stack, view, v1, v2, handle;
    int release, click;
 
    // if one or more voxels were already added, then end addVoxelMode 
    // if mouse moved without holding ctrl. 
    if(addVoxelMode == 2 && !(combo & 0x2000000)) {
        addVoxelMode = 0;
        writelineToVnmrJ("vnmrjcmd", "cursor pointer");
    }

    // middle mouse (button==1) is not used.
    if(button == 1 || currentStacks == NULL || currentViews == NULL) return;


    aip_mnumypnts = win_height - 4;
    y = aip_mnumypnts-y0-1;
    p[0] = x;
    p[1] = y;

    /* if shift key is pressed, prepare the orderMode */ 
/*
    if((combo & 0x1000000) && !orderMode) {

        if(currentStacks != NULL && lastStacks != NULL)
            copyPrescript(currentStacks, lastStacks);
	orderMode = 1;
        setOrderZero();
	showStackOrders = 1;
	drawActiveOverlay(0, activeStack);
*/
    /* if shift key is released, exit the orderMode */ 
/*
    } else if(!(combo & 0x1000000) && orderMode) {
            orderMode = 0;
	    updateOrders();
	    showStackOrders = 1;
            drawActiveOverlay(0, activeStack);
    } else if((combo & 0x20000) || (combo & 0x10000)) {
*/
    if((combo & 0x20000) || (combo & 0x10000)) {
	if(combo & 0x20000) release = 1;
	else release = 0;
	mouse_but(x, y0, button, combo, release); 
    } else if(combo & 0x40000) {
	click = combo & 0xff; 
	if(combo & 2) click = 2;
	mouse_click(x, y0, button, combo, click);
    } else if(combo & 0x80000) {
	mouse_drag(x, y0, button, combo, status);
    } else if(!(combo & 0x1000000)) // mouse move without shift key.
    {
	// the following code is critical for highlighting
	// handle, edge or border.
	getCurrentVertice(p, &view, &stack, &v1, &v2, &handle);
        // edge or handle is selected only if stack is previously selected (i.e., selectedStack>=0)
        if(selectedStack < 0) {v1 = -1; v2 = -1; handle = -1;}
        if(view != activeView && stack != -1) { 
	    // a different view is selected 
	    selectedStack = stack;
            activeVertex1 = -1; 
            activeVertex2 = -1; 
            activeHandle = -1; 
            selectOverlay(view, stack);  // this will redraw old and new views 
	//} else if(stack != -1 && stack == selectedOverlay) { // a stack is selected
	} else if(stack != -1) { // a stack is selected
	    activeStack=stack;
	    // redraw only for following conditions.
            if(stack != selectedStack || v1 != -1 || v2 != -1 || handle != -1
 	    || activeVertex1 != -1 || activeVertex2 != -1 || activeHandle != -1) {
		// a stack, or handle or edge of that stack is selected.
	        selectedStack = stack;
		activeVertex1 = v1;
                activeVertex2 = v2;
                activeHandle = handle;
		if(activeVertex1 != -1 && activeVertex2 != -1) {
		   selectedStack = -1;
		   activeHandle = -1;
		}
		//if(activeHandle != -1) selectedStack = -1;
        	drawOverlaysForView(activeView);
            }
        } else if(activeVertex1 != -1 || activeVertex2 != -1 || activeHandle != -1 
		|| selectedStack != -1) {
		// no stack is selected, force handle, edge, off.
		selectedStack = -1;
                activeVertex1 = -1; 
                activeVertex2 = -1; 
                activeHandle = -1; 
        	drawOverlaysForView(activeView);
	}
    }

}

/******************/
int getFixedSlice()
/******************/
/* return -1 if no fixed mark on any slice */
/* return -2 if more than slice has fixed mark(s) */
/* return slice index if only one slice has fixed mark(s)*/
{
    int i, r = -1; 
    
    for(i=0; i<numOfMarks; i++) {
 	if(marks[i].isUsing && marks[i].isFixed && marks[i].slice >= 0) {
	    if(r < 0) r = marks[i].slice;
	    else if(r != marks[i].slice) return(-2);
	}
    }

    return(r);	    
}

static void selectOverlay(int view, int stack)
{

    int i;

    if(view != activeView) {
        i = activeView;
        activeView = view;
        view = i;
    }

    if(stack != activeStack) {
        i = activeStack;
        activeStack = stack;
        stack = i;
    }

/* draw all frames when activeStack changes so canvasPixmap2 has the correct backup.*/
    if(imageChanged == 1 || stack != activeStack) {
        drawOverlays();
        imageChanged = 0;

    } else if(view != activeView) {
        drawOverlaysForView(view);
        drawOverlaysForView(activeView);
    }

    else drawOverlaysForView(activeView);

}

/******************/
static void mouse_but(int x, int y0, int button, int combo, int release)
/******************/
{
    int i, view, stack, slice, handle, vertex1, vertex2, mark;
    float2 p;
    int y;
    unsigned int size;

    y = aip_mnumypnts-y0-1;
    p[0] = x;
    p[1] = y;

    // in addVoxelMode, add a voxel at p when mouse released.
    if(addVoxelMode > 0 && button == 0 && release == 1) {
      addVoxelMode = 2;  // this allow mouse_move to exit the mode if not ctrl
      addVoxel(p[0], p[1]);
      return;
    } else if(release == 1) {

        updateOtherOverlays(activeStack); // for multi mouse.

      	sendParamsByOrder(ALLSTACKS); // send multiple stacks whenever mouse is released

	handleMode = SQUARE;

	return;
    }

    size = 10;

    if(isMarking != -1 && button == 0) {
	if(isMarking == numOfMarks) {
	  /* all three marks are used, remove the first one. */
	  view = marks[0].view;
	  removeMark(0);
	  activeMark = -1;
	  drawOverlaysForView(view);

      	  for(i=0; i<numOfMarks-1; i++) {
	    marks[i].view = marks[i+1].view;
	    marks[i].stack = marks[i+1].stack;
	    marks[i].slice = marks[i+1].slice;
	    marks[i].isFixed = marks[i+1].isFixed;
	    marks[i].isUsing = marks[i+1].isUsing;
    	    marks[i].homeLocation[0] = marks[i+1].homeLocation[0]; 
    	    marks[i].homeLocation[1] = marks[i+1].homeLocation[1]; 
    	    marks[i].currentLocation[0] = marks[i+1].currentLocation[0]; 
    	    marks[i].currentLocation[1] = marks[i+1].currentLocation[1]; 
      	  }

          isMarking = numOfMarks-1;
	  marks[isMarking].isUsing = 1;
	  marks[isMarking].isFixed = 0;
    	}

          marks[isMarking].isUsing = 1;
	  marks[isMarking].currentLocation[0] = p[0];
	  marks[isMarking].currentLocation[1] = p[1];
	  getLocation2(marks[isMarking].currentLocation, &(marks[isMarking].view),
                &(marks[isMarking].stack), &(marks[isMarking].slice));
/*
          writelineToVnmrJ("vnmrjcmd", "cursor pointer");
	  isMarking = -1;
*/
	  mark = isMarking;
          view = marks[isMarking].view;
          stack = -1;
          slice = -1;
          handle = -1;
          vertex1 = -1;
          vertex2 = -1;
	  iMarks++;
	  startMarking();
/*
	  setMarkMode(1);
*/
    } else {
	copyPrescript(currentStacks, prevStacks);
        getLocation(p, &view, &stack, &slice, &handle, &vertex1, &vertex2, &mark);
	if(vertex1 != -1 || vertex2 != -1 || mark != -1) selectedStack = -1;
	else selectedStack = stack;
	selectedOverlay=stack;

	if(stack < 0 && activeStack >= 0) stack = activeStack;
    }

	if(debug && isIplanObj(x, y0, currentViews->ids[view])) 
	fprintf(stderr,"isIplanObj\n");

    if(mark != activeMark) {
        i = activeMark;
        activeMark = mark;
        mark = i;
    }
/*
    if(view != activeView) {
        i = activeView;
        activeView = view;
        view = i;
    }

    if(stack != activeStack) {
        i = activeStack;
        activeStack = stack;
        stack = i;
	updateType();
	P_setreal(CURRENT, "planSs", (double)(activeStack+1), 1);
        appendJvarlist("planSs");
	// writelineToVnmrJ("pnew", "1 planSs");
    }

*/
    if(slice != selectedSlice) selectedSlice = slice;
/*
    if((combo & 0x1000000) && activeHandle != -1 && handle != -1) handleMode = CIRCLE;
    else if((combo & 0x1000000)) activeHandle = -1; 
    else if(handle != activeHandle) activeHandle = handle;
*/
    if(button == 2 &&  handle != -1)  handleMode = CIRCLE;
    if(handle != activeHandle) activeHandle = handle;

/* draw all frames when activeStack changes so canvasPixmap2 has the correct backup.*/
/*
    if(imageChanged == 1 || stack != activeStack) {
	drawOverlays();
        imageChanged = 0;

    } else if(view != activeView) {
        drawOverlaysForView(view);
        drawOverlaysForView(activeView);
    }

    else drawOverlaysForView(activeView);
*/
    selectOverlay(view, stack);
    updateType();
    P_setreal(CURRENT, "planSs", (double)(activeStack+1), 1);
    appendJvarlist("planSs");
    // writelineToVnmrJ("pnew", "1 planSs");
/*
    if(vertex1 != activeVertex1) activeVertex1 = vertex1;
    if(vertex2 != activeVertex2) activeVertex2 = vertex2;
*/
    prevP[0] = p[0];
    prevP[1] = p[1];
}

void updateType()
{
    int i = 0, type;
    double d;

    if(currentStacks == NULL || currentStacks->numOfStacks <= 0) return;

    if(activeStack > 0 && activeStack < currentStacks->numOfStacks) i = activeStack;

/* set iplanType */
    type = currentStacks->stacks[i].planType;
    if(!(P_getreal(CURRENT, "iplanType", &d, 1)) &&
        (int)d != type) {
            P_setreal(CURRENT, "iplanType", (double)type, 1);
    }
    updateUsePos(type);
    sendPnew0(1, " iplanType");
}

void updateDefaultType()
{
/* set iplanDefaultType */
/* iplanDefaultType may be arrayed for all types of overlays 
   Useful only for loading multiple stacks of different type. 
   When there is multiple type, which one to show?
*/
    int i, j, n, type, found;
    int types[99];

    if(currentStacks == NULL || currentStacks->numOfStacks <= 0) return;

    clearVar("iplanDefaultType");

    n = 0;
    for(i=0; i<currentStacks->numOfStacks; i++) {
	type = currentStacks->stacks[i].planType;
	found=0;
        for (j=0; j<n; j++) {
	  if(types[j] == type) found=1;
        }
	if(!found) {
	   types[n]=type;
	   n++;
	   P_setreal(CURRENT, "iplanDefaultType", type,n);
        }
    }

    // sendPnew0(1, " iplanDefaultType");
}

/******************/
int countFixedMarksForStack(int stack)
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) {
	getLocation2(marks[i].currentLocation, &(marks[i].view), 
		&(marks[i].stack), &(marks[i].slice));
	if(stack == marks[i].stack && marks[i].isFixed) j++;
    }
    return j;
}

/******************/
int countFixedMarksForSlice(int slice)
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) {
	getLocation2(marks[i].currentLocation, &(marks[i].view), 
		&(marks[i].stack), &(marks[i].slice));
	if(slice == marks[i].slice && marks[i].isFixed) j++;
    }
    return j;
}

/******************/
int countMarksForSlice(int slice, int ind[3])
/******************/
{
    int i, j = 0;
    for(i=0; i<numOfMarks; i++) {
      getLocation2(marks[i].currentLocation, &(marks[i].view), 
              &(marks[i].stack), &(marks[i].slice));
      if(slice == marks[i].slice) {
         ind[j] = i;
         j++;
      }
    }
    return j;
}

/******************/
int getFixedMarksForStack(int stack, int i)
/******************/
{
    int j, k = 0;
    for(j=0; j<numOfMarks; j++) {
	getLocation2(marks[i].currentLocation, &(marks[i].view),
                &(marks[i].stack), &(marks[i].slice));
	if(stack == marks[j].stack && marks[j].isFixed) k++;
	if(k == i) return j;
    }

    return -1;
}

/******************/
int getRotationCenter(float2 center, int view, int stack)
/******************/
{
    int n, m1, m2;
    float3 p1, p2;
    int v;
 
    n = countFixedMarksForStack(stack);

    if(n > 2) return(-1);

    else if(n == 0) 
        return(getCrosssectionCenter(center, view, stack));

    else if(n == 1) {
        m1 = getFixedMarksForStack(stack, 1);
	v = marks[m1].view;
	p1[0] = marks[m1].currentLocation[0];
	p1[1] = marks[m1].currentLocation[1];
	p1[2] = 0.0;

        transform(currentViews->views[v].p2m, p1);
        transform(currentViews->views[view].m2p, p1);
	center[0] = p1[0];
	center[1] = p1[1];
    } 

    else if(n == 2) {
        m1 = getFixedMarksForStack(stack, 1);
	v = marks[m1].view;
	p1[0] = marks[m1].currentLocation[0];
	p1[1] = marks[m1].currentLocation[1];
	p1[2] = 0.0;

        transform(currentViews->views[v].p2m, p1);
        transform(currentViews->views[view].m2p, p1);
	center[0] = p1[0];
	center[1] = p1[1];

        m2 = getFixedMarksForStack(stack, 2);
	v = marks[m2].view;
	p2[0] = marks[m2].currentLocation[0];
	p2[1] = marks[m2].currentLocation[1];
	p2[2] = 0.0;

        transform(currentViews->views[v].p2m, p2);
        transform(currentViews->views[view].m2p, p2);
	center[0] = p2[0];
	center[1] = p2[1];
        if(fabs(p1[0] - p2[0]) > markSize || fabs(p1[1] - p2[1]) > markSize ) return -1;

	center[0] = p1[0];
	center[1] = p1[1];

    } 

    return(1);
}

/******************/
void updateSliceOrders(int stack, int slice)
/******************/
{
    int i, j, k, n;

    /* find last order, not included that of the selected slice */
    k = n = 0;
    for(i=0; i<currentStacks->stacks[stack].ns; i++)  
        if(currentStacks->stacks[stack].order[i] != 0 && i != slice) k++;

    /* find current order of the selected slice */
    j = -1;
    for(i=0; i<currentStacks->stacks[stack].ns; i++)  
	if(i == slice) { 
	    j = i; 
	    n = currentStacks->stacks[stack].order[i];
	    break;
	}

    /* if selected slice has non-zero order, reduce higher orders of slices by 1 */
    if(j != -1 && n > 0) for(i=0; i<currentStacks->stacks[stack].ns; i++)
    	if(currentStacks->stacks[stack].order[i] > n) 
		currentStacks->stacks[stack].order[i] -= 1;

    /* set the order of the selected slice to last order plus 1 */ 
    if(k < currentStacks->stacks[stack].ns)
	currentStacks->stacks[stack].order[slice] = k+1;

    if(debug) {
      for(i=0; i<currentStacks->stacks[stack].ns; i++)
	fprintf(stderr,"updateSliceOrders %d %d\n", i, currentStacks->stacks[stack].order[i]);
    }
}

/******************/
static void brkPath2(char* path, char* root, char* name)
/******************/
/* success return 1, failed return 0 */
{
    char dir[MAXSTR], base[MAXSTR];

    brkPath(path, dir, base);
    if(dir[strlen(dir)-1] == '/') {
 	strcpy(root, "");
        strncat(root, dir, strlen(dir)-1);
    } else strcpy(root, dir);
	
    if(base[strlen(base)-1] == '/') {
 	strcpy(name, "");
        strncat(name, base, strlen(base)-1);
    } else strcpy(name, base);
}

/******************/
void updateScoutInfo(int len, int* ids)
/******************/
{
    int i, j, k, n;
    char buf[MAXSTR];
    char dir[MAXSTR], name[MAXSTR], dir2[MAXSTR], name2[MAXSTR];
    char paths[10][MAXSTR];
    char path[MAXSTR], studyDataDir[MAXSTR];
    vInfo           paraminfo;
    char str[MAXSTR], scoutpaths[MAXSTR];

    if(P_getstring(CURRENT, "sqdir", studyDataDir, 1, MAXSTR)) 
    if(P_getstring(GLOBAL, "sqdir", studyDataDir, 1, MAXSTR)) 
	strcpy(studyDataDir, "");
    if(strlen(studyDataDir) > 0) {
	strcat(studyDataDir, "/data");
    } 

    n = 0;
    for(i=0; i<len && n<10; i++) {
        aipGetImagePath(ids[i], buf, MAXSTR);
	brkPath2(buf, dir, name);
	brkPath2(dir, dir2, name2);
	if(strcmp(dir2, studyDataDir) == 0) {
	    sprintf(path, "data/%s", name2);
	} else {
	    sprintf(path, "%s/%s", dir2, name2);
	}

	/* check whether is a new path */
	for(j=0; j<n; j++) 
	    if(strcmp(path, paths[j]) == 0) break;

        /* add new path */
	if(j == n) {
            strcpy(paths[n],path);
	    n++;
	} 
    }

    if(P_getVarInfo(CURRENT, "scoutpaths", &paraminfo) == -2) {
        P_creatvar(CURRENT, "scoutpaths", ST_STRING);
        P_setstring(CURRENT, "scoutpaths", "", 1);
    } else clearVar("scoutpaths");
    strcpy(scoutpaths, "");
    for(i=0; i<n; i++) {
        if(strlen(scoutpaths) < MAXSTR-strlen(paths[i])-1) {
            strcat(scoutpaths, paths[i]); 
            strcat(scoutpaths, ","); 
        }
    }

    k = strlen(scoutpaths);
    if(k > 0 && scoutpaths[k-1] == ',') {
	strcpy(str, "");
	strncat(str, scoutpaths, k-1);
    } else strcpy(str, scoutpaths);
    P_setstring(CURRENT, "scoutpaths", str, 1);
}

/******************/
void getCurrentIBViews(int len)
/******************/
{
    int* ids;

    if(len > 0) {

        ids = (int*)malloc(sizeof(int)*len);

        aipGetImageIds(len, ids);

        getIBviews(len, ids);

        updateScoutInfo(len, ids);

        free(ids);
    }
}

/******************/
int gplan_update(int argc, char *argv[], int retc,char *retv[])
/******************/
{
    if(currentViews == NULL || currentStacks == NULL) RETURN;

    aip_mnumypnts = win_height - 4;
    updateOverlays();

    RETURN;
}

/******************/
int eraseOverlays()
/******************/
{
    if(currentViews == NULL || currentStacks == NULL) RETURN;

    aipUnregisterMouseListener(mouse_move, NULL);
/*
    Jactivate_mouse(NULL, NULL, NULL, NULL, NULL);
    Wsetgraphicsdisplay("");
*/
    refreshImages(drawMode);

    RETURN;
}

/******************/
int updateOverlays()
/******************/
{
    int i;

    if(!aipOwnsScreen() || disMode == 0 || currentViews == NULL 
	|| currentStacks == NULL) RETURN;
    IBlen = aipGetNumberOfImages();

    if(aipOwnsScreen() && IBlen > 0) {
        getCurrentIBViews(IBlen);

        aipRegisterDisplayListener(update_iplan);
        aipRegisterMouseListener(mouse_move, NULL);

    } else {
        endIplan();
	RETURN;
    }

/* no longer need this. IB takes care of it. */
/*
    Wsetaraphicsdisplay("gplan_update");
*/
    for(i=0; i<currentStacks->numOfStacks; i++) {
    calTransMtx(&(currentStacks->stacks[i]));
    calSliceXYZ(&(currentStacks->stacks[i]));
    }
    calOverlays();
    drawOverlays();
    RETURN;
}

static void update_iplan(int len, int* ids, int* changeFlags)
{
   int i, j;
   if(currentViews == NULL || currentStacks == NULL) return;
   
   aip_mnumypnts = win_height - 4;
   IBlen = aipGetNumberOfImages();
/*
   if (IBlen != currentViews->numOfViews) {
              activeView = 0;
              removeAllMarks();
   }
 */ 
   getCurrentIBViews(IBlen);
   calOverlays();

   if(currentStacks->numOfStacks < 1) return;

   for(i=0; i<len; i++) {
        for(j=0; j<currentViews->numOfViews; j++) {
            if(currentViews->ids[j] == ids[i]) reDrawWindow(ids[i]);
        }
    }

}

// this function calculate m2p and p2m of the view based on current plan parameters.
// so "view" and "plan" have the same orientation.
void makeDefaultView(iplan_view* view, CFrameInfo_t* frameInfor) { 
    double d;
    double scale[3];
    int j, k;
    float pos[3];
    
    view->framewd = frameInfor->framewd;
    view->frameht = frameInfor->frameht;
    view->framestx = frameInfor->framestx;
    view->framesty = aip_mnumypnts - frameInfor->framesty;

    // get slice info from current plan parameters
    getStack(&(view->slice), getIplanType());

    // determine scale (fill the frame with FOV)
    d = max(view->slice.lpe, view->slice.lro);
    if(d <= 0) d = 10; // best guess
    view->pixelsPerCm = min(view->framewd,view->frameht)/d;

    // x and y of the view is inverted relative to the plan.
    scale[0] = -view->pixelsPerCm;
    scale[1] = -view->pixelsPerCm;
    scale[2] = view->pixelsPerCm;

    // m2p and p2m are 3x4 rotation/translation matrix.
    // determine 3x3 rotation matrix. 
    for(j=0; j<3; j++)
       for(k=0; k<3; k++) { 
           view->m2p[j][k] = scale[j] * ( view->slice.orientation[j*3+k]);
           view->p2m[k][j] = ( view->slice.orientation[j*3+k])/scale[j];
    }

    // position offset
    pos[0] = view->slice.ppe;
    pos[1] = view->slice.pro;
    pos[2] = view->slice.pss0;
    
    // determine translation matrix 
    // T = 1 0 0 X 
    //     0 1 0 Y
    //     0 0 1 Z
    //     0 0 0 1
    // T' = 1 0 0 -X 
    //      0 1 0 -Y
    //      0 0 1 -Z
    //      0 0 0  1
    // X = -pos[0]*scale[0] + frameInfor->framestx + 0.5*(frameInfor->framewd);
    // Y = aip_mnumypnts - (pos[1]*scale[1] + frameInfor->framesty + 0.5*(frameInfor->frameht));
    // Z = 0;
    // m2p = T x m2p
    // p2m = p2m x T'

    view->m2p[0][3] = -pos[0]*scale[0] + frameInfor->framestx + 0.5*(frameInfor->framewd);
    view->m2p[1][3] = aip_mnumypnts - (pos[1]*scale[1] + frameInfor->framesty + 0.5*(frameInfor->frameht));
    view->m2p[2][3] = -pos[2]*scale[2];

    for(j=0; j<3; j++) { 
	view->p2m[j][3] = 0;
        for(k=0; k<3; k++) { 
	   view->p2m[j][3] -= (view->p2m[j][k])*(view->m2p[k][3]);
	}
    }
}

/******************/
void getIBview(iplan_view* view, int id)
/******************/
{
/* m2p rotates from magnet to pixel frame (origin is left upper corner). */ 
/* orientation (of view) rotates from magnet to "logical" frame (origin is */
/* left lower corner). orientation is (1, -1, -1)*m2p of image browser, i.e., */
/* rotate 180 about x-axis. both frames are right handed */

/*        1  0  0   0  1  0                   -1  0  0   -1  0  0 */ 
/* m2p =  0 -1  0 * 1  0  0 * R(obl_matrix) *  0 -1  0 *  0  1  0 */ 
/*        0  0 -1   0  0  1                    0  0  1    0  0 -1 */ 

/* or     1  0  0                         -1  0  0 */ 
/* m2p =  0 -1  0 * R(-psi, theta, phi) *  0  1  0 */ 
/*        0  0 -1                          0  0 -1 */ 

/* where R(obl_matrix) is obl_matrix in sis__oblique.c */
/*       R(-psi, theta, phi) is Goldstein euler rotation matrix with psi = -psi */

    int j, k;
/*
    int pcenter[4];
    float s[3], ucenter[3];
 */
    float scale[3];
    CImgInfo_t* imgInfor;
    CFrameInfo_t* frameInfor;

    imgInfor = (CImgInfo_t*)malloc(sizeof(CImgInfo_t));
    frameInfor = (CFrameInfo_t*)malloc(sizeof(CFrameInfo_t));

	view->hasFrame = 1;
	view->hasImage = 1;

        if(!aipGetImageInfo(id, imgInfor)) {
	  if(!aipGetFrameInfo(id, frameInfor)) {
	    view->hasFrame = 0;
	    view->hasImage = 0;
	  } else {
	     view->hasFrame = 1;
	     view->hasImage = 0;
	     makeDefaultView(view, frameInfor);
          }
	  return;
	}

        view->coil = imgInfor->coil;
        view->coils = imgInfor->coils;
        if(view->coils > 1) {
          view->origin[0] = imgInfor->origin[0];
          view->origin[1] = imgInfor->origin[1];
          view->origin[2] = imgInfor->origin[2];
	} else {
          view->origin[0] = 0.0;
          view->origin[1] = 0.0;
          view->origin[2] = 0.0;
  	}

/* scale is the scaling matrix multiplied by a 180 rotation about x. */

        scale[0] = imgInfor->pixelsPerCm;
        scale[1] = -imgInfor->pixelsPerCm;
        scale[2] = -imgInfor->pixelsPerCm;

	view->pixelsPerCm = imgInfor->pixelsPerCm;

	for(j=0; j<3; j++)
          for(k=0; k<4; k++) { 
                view->m2p[j][k] = imgInfor->m2p[j][k];
                view->p2m[j][k] = imgInfor->p2m[j][k];
	}

/* m2p = R' x m2p, p2m = p2m x R' */

/* R' =  1  0  0  0 		*/
/*       0 -1  0  h 		*/
/*       0  0 -1  0 		*/
/*       0  0  0  1 		*/

/* h = aip_mnumypnts		*/

        for(k=0; k<4; k++) {  
	    view->m2p[1][k] = -view->m2p[1][k];
	    view->m2p[2][k] = -view->m2p[2][k];
	}
	view->m2p[1][3] += aip_mnumypnts;

	for(j=0; j<3; j++) {
//	calculate p2m[j][3] before p2m[j][1] is modified.  
	    view->p2m[j][3] = view->p2m[j][1]*aip_mnumypnts + view->p2m[j][3];
	    view->p2m[j][1] = -view->p2m[j][1];
	    view->p2m[j][2] = -view->p2m[j][2];
	}

    view->slice.pss = (float2*)malloc(sizeof(float2));
    view->slice.radialAngles = (float2*)malloc(sizeof(float2));
    view->slice.order = (int*)malloc(sizeof(int));
    view->slice.slices = (iplan_box *)malloc(sizeof(iplan_box));

/* orientation is m2p/scale */

    view->slice.theta = imgInfor->euler[0];
    view->slice.psi = imgInfor->euler[1];
    view->slice.phi = imgInfor->euler[2];
    euler2tensor(view->slice.theta,view->slice.psi,view->slice.phi,view->slice.orientation);

        view->slice.ppe = imgInfor->location[0];
        view->slice.pro = imgInfor->location[1];
        view->slice.pss0 = imgInfor->location[2];

        view->slice.lpe = imgInfor->roi[0];
        view->slice.lro = imgInfor->roi[1];
        view->slice.lpe2 = imgInfor->roi[2];

        view->pixwd = imgInfor->pixwd;
        view->pixht = imgInfor->pixht;
        view->pixstx = imgInfor->pixstx;
/* iplan followed vnmr convention that inverts y axis (relative to */
/* x window convention) to the so use aip_mnumypnts-y. */
/* while as image browser uses x window convention. */
/* we'll stick to vnmr convention, so here pixsty is invered. */
        view->pixsty = aip_mnumypnts - imgInfor->pixsty;

        view->framewd = imgInfor->framewd;
        view->frameht = imgInfor->frameht;
        view->framestx = imgInfor->framestx;
        view->framesty = aip_mnumypnts - imgInfor->framesty;

        view->slice.ns = 1;
        view->slice.type = REGULAR;
        view->slice.color = frameColor;
        view->slice.thk = defaultThk;
        view->slice.gap = 0.0;

        view->slice.pss[0][0] = imgInfor->location[2];
        view->slice.pss[0][1] = 0.0;
        view->slice.radialAngles[0][0] = 0.0;
        view->slice.radialAngles[0][1] = 0.0;
        view->slice.order[0] = 1;

        view->slice.envelope.np = 0;
        view->slice.envelope.points = NULL;

        calTransMtx(&(view->slice));
        calSliceXYZ(&(view->slice));

    free(imgInfor);
    free(frameInfor);
}

// this function get info of current scout images and store in currentViews.
/******************/
void getIBviews(int len, int* ids)
/******************/
{
    int i;
    float fmin;

    freeViews(currentViews);

    currentViews->numOfViews = len;

    currentViews->views = (iplan_view *)
                malloc(sizeof(iplan_view)*len);
    currentViews->ids = (int*)malloc(sizeof(int)*currentViews->numOfViews);

    activeView = -1;                                         
    fmin = -1;
    for(i=0; i<len; i++) {
	currentViews->ids[i] = ids[i];
	getIBview(&(currentViews->views[i]), ids[i]);

/* set activeView to view of minimum offset */               
/*
        if(fmin < 0) fmin = currentViews->views[i].pixwd +
                        currentViews->views[i].pixht;

        c[0] = currentViews->views[i].pixstx+(currentViews->views[i].pixwd)/2;
        c[1] = currentViews->views[i].pixsty-(currentViews->views[i].pixht)/2;
        c[2] = 0;
        transform(currentViews->views[i].p2m, c);

        f = fabs(c[0])+fabs(c[1])+fabs(c[2]);                
        if(f < fmin) {                                      
            activeView = i;                                  
            fmin = f;
	}
*/
    }
}

/******************/
void reDrawWindow(int id)
/******************/
{
    int i, nv = 0, flag = 0;
    int x0, y0, wd, ht;
    float3 m[3];

    if(currentViews == NULL || currentViews->numOfViews <= 0) return;
    if(currentStacks == NULL) return;

    for(i=0; i<currentViews->numOfViews; i++) 
	if(currentViews->ids[i] == id) nv = i;

/* if there is a mark in the window, transform it location to the new window */
    for(i=0; i<numOfMarks; i++) 
	if(marks[i].isUsing && marks[i].view == nv) {
	    m[i][0] = marks[i].currentLocation[0];
	    m[i][1] = marks[i].currentLocation[1];
	    m[i][2] = 0.0;
	    transform(currentViews->views[nv].p2m, m[i]);
	}

    //getIBview(&(currentViews->views[nv]), id);

    for(i=0; i<numOfMarks; i++) 
	if(marks[i].isUsing && marks[i].view == nv) {
	    transform(currentViews->views[nv].m2p, m[i]);
	    marks[i].currentLocation[0] = m[i][0];
	    marks[i].currentLocation[1] = m[i][1];
	}

/*
    x0 = currentViews->views[nv].framestx - 2;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty - 2;
    wd = currentViews->views[nv].framewd + 4;
    ht = currentViews->views[nv].frameht + 4;
*/
    x0 = currentViews->views[nv].framestx;
    y0 = aip_mnumypnts - currentViews->views[nv].framesty;
    wd = currentViews->views[nv].framewd;
    ht = currentViews->views[nv].frameht;

    aipUnregisterDisplayListener(update_iplan);
    aipRefreshImage(currentViews->ids[nv]);
    aipRegisterDisplayListener(update_iplan);

// draw csi wire mesh first before drawing any plane overlay. 
// drawCSI3DMesh will do nothing if no csi planType.
    drawCSI3DMesh(nv, meshColor, 0);

/* assuming the window(of id) of the canvasPixmap and xid has only the new image */

/* draw on canvasPixmap2 */
    flag = 1;
    for(i=0; i<currentStacks->numOfStacks; i++) {
      if(i != activeStack) {
        calOverlayForAview(i, nv);
        drawOverlayForView(flag, i, nv);

      }
    }

    for(i=0; i<numOfMarks; i++) {
        if(i != activeMark && marks[i].isUsing && marks[i].view == nv)
        drawMarks(flag, i);
    }

    copy_to_pixmap2(x0, y0, x0, y0, wd, ht);

/* draw on canvasPixmap */
    flag = 1;

    if(activeStack != -1) {
        calOverlayForAview(activeStack, nv);
        drawOverlayForView(flag, activeStack, nv);
    }

    if(activeSlice >= 0) drawActiveSlice(flag, nv);
    //else if(activeView == nv) drawHandles(flag);

    for(i=0; i<numOfMarks; i++)
        if(i == activeMark && marks[i].isUsing && marks[i].view == nv)
        drawMarks(flag, i);

/* copy to xid */
    if(getWin_draw()) {
        copy_aipmap_to_window(x0, y0, x0, y0, wd, ht);
    }
}

/******************/
int getCompressMode()
/******************/
{
/* 1 is compressed, 0 is paralell */
/* 2 is not compressed (s) */
    int i, mode = 1;
    char str[MAXSTR];

    i = P_getstring(CURRENT, "seqcon", str, 1, MAXSTR);
    if(i == -2 || str[1] == 'c') mode = 1;
    else if(str[1] == 's') mode = 2;

    if(currentStacks != NULL)
      for(i=0; i<currentStacks->numOfStacks; i++)
        if(currentStacks->stacks[i].type == RADIAL) mode = 0;

    return(mode);
}

/******************/
int isIplanObj(int x, int y0, int id)
/******************/
/* 1 yes, 0 no. */
{
    int i, k, y, view = 0;
    float2 p, p1;
    iplan_2Dstack *overlay;
 
    if(currentStacks == NULL || currentStacks->numOfStacks <= 0) return(0);
    if(currentViews == NULL || currentViews->numOfViews <= 0) return(0);
    if(disMode == 0) return(0);

    if(activeVertex1 != -1 || activeVertex2 != -1 || activeHandle != -1 
		|| selectedStack != -1) return(1); 

    y = aip_mnumypnts-y0-1;
    p[0] = x;
    p[1] = y;

    for(i=0; i<currentViews->numOfViews; i++) 
	if(currentViews->ids[i] == id) view = i;

    if(currentOverlays == NULL || currentOverlays->numOfOverlays <= view) return(0);

    for(i=0; i<currentOverlays->overlays[view].numOfStacks; i++) {

	overlay = &(currentOverlays->overlays[view].stacks[i]);
	/* test stack envelope */
	if(currentStacks->stacks[i].type != RADIAL) {

          if(containedInConvexPolygon(p,
          overlay->handles.points,
          overlay->handles.np) > 1.99*pi) {

		return(1);
	  }

	} else {

          p1[0] = overlay->handleCenter[0];
          p1[1] = overlay->handleCenter[1];
          if(overlay->handles.np > 0 &&
                containedInCircle(p, p1, 0.3*overlaySize_max(overlay))) {
       
		return(1);
          }

          /* only RADIAL need to test slices */

          for(k=0; k<overlay->numOfStripes; k++) {
           if(containedInConvexPolygon(p,
           overlay->stripes[k].points,
           overlay->stripes[k].np) > 1.99*pi) {

		return(1);
           }
          }

	}
    }

    for(i=0; i<numOfMarks; i++) {
       	if(marks != NULL && marks[i].isUsing && marks[i].currentLocation[0] > 0
        && marks[i].currentLocation[1] > 0 &&
        containedInCircle(p, marks[i].currentLocation, markSize/2)) {
           return(1);
       	}
    }

    return(0);
}

int onActivePlan(int x, int y) {
   if(activeVertex1 != -1 || activeVertex2 != -1 || activeHandle != -1 
		|| selectedStack != -1) return 1; 
   else return 0;
}

int onVoxelPlan(int x, int y) {
   int i, view = 0;
   float2 p;
   if(currentStacks == NULL || currentStacks->numOfStacks < 1) return 0;
   if(currentViews == NULL || currentViews->numOfViews < 1) return 0;
   
   p[0] = x;
   p[1] = aip_mnumypnts-y-1;
   
   for(i=0; i<currentViews->numOfViews; i++) {
        if(containedInRectangle(p, currentViews->views[i].framestx,
                currentViews->views[i].framesty,
                currentViews->views[i].framewd,
                currentViews->views[i].frameht)) {
            view = i;
	    break;
        }
   }

   for(i=0; i<currentStacks->numOfStacks; i++) {
	if(currentStacks->stacks[i].type == VOXEL) {
	    if(isOverStack(p, view, i)) return 1; 
	}
   }
   return 0;
}

void unselectAndRedrawPlan() {
   selectedStack = -1;
   activeVertex1 = -1;
   activeVertex2 = -1;
   activeHandle = -1; 
   drawOverlaysForView(activeView);
}

void draw3DMeshForStack(iplan_stack *stack, iplan_view *view, int lineColor,
	int drawBackPlanes, int nx, int ny, int nz)
{
   planLine *lines;
   int ns,nl,i,j,k,ix,iy,iz;
   float x0,y0,z0;
   float sizex,sizey,sizez;
   float dimx,dimy,dimz;
   int x1,y1,x2,y2,xmin,ymin,xmax,ymax;
   float3 xd,yd,zd;
   float u2m[3][4],m2p[3][4];

   ns=stack->ns;

   if(nx<=0)nx=1;
   if(ny<=0)ny=1;
   if(nz<=0)nz=1;
   // dimension size
   dimx=stack->lpe;
   dimy=stack->lro;
   if(stack->type == VOLUME) dimz = (stack->lpe2);
   else dimz = (stack->thk);

   // grid size
   sizex=dimx/nx;
   sizey=dimy/ny;
   sizez=dimz/nz;

   // calculate direcion cosines of principle axises
   // first, make copy of u2m and m2p and remove translation elements.
   for(i=0; i<3; i++) { 
      for(j=0; j<3; j++) {
          u2m[i][j]=stack->u2m[i][j];
          m2p[i][j]=view->m2p[i][j];
      }
      // make translation elements zero.
      u2m[i][3]=0.0;
      m2p[i][3]=0.0;
   }
   // transform principle axies to the orientation of scout image (view).
   xd[0]=1.0;
   xd[1]=0.0;
   xd[2]=0.0;
   transform(u2m, xd); 
   transform(m2p, xd);
   yd[0]=0.0;
   yd[1]=1.0;
   yd[2]=0.0;
   transform(u2m, yd); 
   transform(m2p, yd);
   zd[0]=0.0;
   zd[1]=0.0;
   zd[2]=1.0;
   transform(u2m, zd); 
   transform(m2p, zd);
   
   nl=ns*((nx+1+ny+1) + (ny+1+nz+1) + (nz+1+nx+1));
   if(drawBackPlanes) nl *= 2;
   lines = (planLine*)malloc(sizeof(planLine)*nl);
   k=0;
   for(i=0; i<ns; i++) {
      if(stack->type == VOLUME) z0=0;
      else z0=stack->pss[i][0] + stack->pss[i][1] + 0.5*(stack->lpe2 - dimz);
      // xy lines (nx+1 vertical plus ny+1 horizontal lines)
      j=0;
      for(iz=0;iz<=nz;iz+=nz) {
        j=iz;
        if(!drawBackPlanes) { // draw only the front
	  if(j>0) break;
          if(zd[2]>=0) iz=0; else iz=nz; //if z<0, back (nz) is front.
        }
        for(ix=0;ix<=nx;ix++) {
 	   lines[k].p1[0] = ix*sizex;
 	   lines[k].p1[1] = 0;
 	   lines[k].p1[2] = z0+iz*sizez;
 	   lines[k].p2[0] = ix*sizex;
 	   lines[k].p2[1] = dimy; 
 	   lines[k].p2[2] = z0+iz*sizez;
           k++;
	}
        for(iy=0;iy<=ny;iy++) {
 	   lines[k].p1[0] = 0; 
 	   lines[k].p1[1] = iy*sizey;
 	   lines[k].p1[2] = z0+iz*sizez;
 	   lines[k].p2[0] = dimx; 
 	   lines[k].p2[1] = iy*sizey;
 	   lines[k].p2[2] = z0+iz*sizez;
           k++;
        }
      }
      // yz lines (nz+1 plus ny+1 lines)
      j=0;
      for(ix=0;ix<=nx;ix+=nx) {
        j=ix;
        if(!drawBackPlanes) {
	  if(j>0) break;
          if(xd[2]>=0) ix=0; else ix=nx;
        }
        for(iz=0;iz<=nz;iz++) {
 	   lines[k].p1[0] = ix*sizex;
 	   lines[k].p1[1] = 0;
 	   lines[k].p1[2] = z0+iz*sizez;
 	   lines[k].p2[0] = ix*sizex;
 	   lines[k].p2[1] = dimy; 
 	   lines[k].p2[2] = z0+iz*sizez;
           k++;
	}
        for(iy=0;iy<=ny;iy++) {
 	   lines[k].p1[0] = ix*sizex;
 	   lines[k].p1[1] = iy*sizey;
 	   lines[k].p1[2] = z0; 
 	   lines[k].p2[0] = ix*sizex;
 	   lines[k].p2[1] = iy*sizey;
 	   lines[k].p2[2] = z0+dimz; 
           k++;
        }
      }
      // xz lines (nx+1 plus nz+1 lines)
      for(iy=0;iy<=ny;iy+=ny) {
        j=iy;
        if(!drawBackPlanes) {
	  if(j>0) break;
          if(yd[2]>=0) iy=0; else iy=ny;
        }
        for(iz=0;iz<=nz;iz++) {
 	   lines[k].p1[0] = 0;
 	   lines[k].p1[1] = iy*sizey;
 	   lines[k].p1[2] = z0+iz*sizez;
 	   lines[k].p2[0] = dimx; 
 	   lines[k].p2[1] = iy*sizey;
 	   lines[k].p2[2] = z0+iz*sizez;
           k++;
	}
        for(ix=0;ix<=nx;ix++) {
 	   lines[k].p1[0] = ix*sizex;
 	   lines[k].p1[1] = iy*sizey;
 	   lines[k].p1[2] = z0; 
 	   lines[k].p2[0] = ix*sizex;
 	   lines[k].p2[1] = iy*sizey;
 	   lines[k].p2[2] = z0+dimz; 
           k++;
        }
      }
   }
   
   nl=k; // nl and k should always be the same, but just in case.
   x0=0.5*dimx;
   y0=0.5*dimy;
   z0=0.5*(stack->lpe2);
   for(i=0; i<nl;i++) {
     // move origin to the center
     lines[i].p1[0] -= x0;
     lines[i].p1[1] -= y0;
     lines[i].p1[2] -= z0;
     lines[i].p2[0] -= x0;
     lines[i].p2[1] -= y0;
     lines[i].p2[2] -= z0;
     // rotate to magnet
     transform(stack->u2m, lines[i].p1); 
     transform(stack->u2m, lines[i].p2); 
     // rotate to pixel
     transform(view->m2p, lines[i].p1);
     transform(view->m2p, lines[i].p2);
     // remove redundant (will not draw if x,y of the two points are the same)
     for(k=0;k<i;k++) {
       if(lines[i].p1[0] == lines[k].p1[0] && lines[i].p1[1] == lines[k].p1[1]
	&& lines[i].p2[0] == lines[k].p2[0] && lines[i].p2[1] == lines[k].p2[1]) {
	  lines[i].p1[0]=0;
	  lines[i].p1[1]=0;
	  lines[i].p2[0]=0;
	  lines[i].p2[1]=0;
       }
     }
   }

   xmin = view->framestx;
   xmax = view->framestx + view->framewd;
   ymin = view->framesty - view->frameht;
   ymax = view->framesty;

   color(lineColor);
   k=0;
   for(i=0; i<nl;i++) {
     x1=lines[i].p1[0];
     y1=lines[i].p1[1];
     x2=lines[i].p2[0];
     y2=lines[i].p2[1];
        
     // draw x,y of the two points that are NOT the same
     if(!(x1 == x2 && y1 == y2)) {
	k++;
	if(trimLine(&x1, &y1, &x2, &y2, xmin, ymin, xmax, ymax) == 2) {
	    if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
                (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) {
	    } else drawLine(1, x1, y1, x2, y2, 1, "LineSolid");
	}
     }
   }

   //color(stack->color);
   free(lines);
}

// called to draw 3D box of a plan overlay (3D view of overlay).
void draw3DBox(iplan_stack *stack, iplan_view *view, int lineColor, int drawBackPlanes)
{
    draw3DMeshForStack(stack,view,lineColor,drawBackPlanes,1,1,1);
}

int isOverlapped(int s, int v) {
   iplan_2Dstack *overlay = &(currentOverlays->overlays[v].stacks[s]);
   return(intersected(overlay));
}

// called to draw CSI grid for CSI planning (overlay)
void drawCSI3DMesh(int nv, int lineColor, int drawBackPlanes)
{
    int i,nx=10,ny=1,nz=1;
    double d;
    iplan_view *view = &(currentViews->views[nv]);
    iplan_stack *stack;
    for(i=0; i<currentStacks->numOfStacks; i++) {
	// don't draw if not overlap with scout
	if(!isOverlapped(i,nv)) continue;
	stack = &(currentStacks->stacks[i]);
	if(stack->planType == CSI2D) {
          if(!P_getreal(CURRENT, "nv2", &d, 1)) nx=(int)d;
          if(!P_getreal(CURRENT, "nv", &d, 1)) ny=(int)d;
	  draw3DMeshForStack(stack,view,lineColor,drawBackPlanes,nx,ny,nz);
        }
	else if(currentStacks->stacks[i].planType == CSI3D) {
          if(!P_getreal(CURRENT, "nv2", &d, 1)) nx=(int)d;
          if(!P_getreal(CURRENT, "nv", &d, 1)) ny=(int)d;
          if(!P_getreal(CURRENT, "nv3", &d, 1)) nz=(int)d;
	  draw3DMeshForStack(stack,view,lineColor,drawBackPlanes,nx,ny,nz);
        }
     } 
}

// called to draw axis labels of plan overlay.
void drawFOVlabel(iplan_stack *stack, iplan_view *view, int lineColor)
{
   int i;
   float x0,y0,z0;
   float dimx,dimy,dimz;
   int x1,y1,x2,y2,xmin,ymin,xmax,ymax;
   float3 label[3], axis[3], oregin;
   planLine lines[3];
   planParams *tag = getCurrPlanParams(stack->planType);

   // dimension size
   dimx=stack->lpe;
   dimy=stack->lro;
   dimz=stack->lpe2;

   oregin[0]=0.0;
   oregin[1]=0.0;
   oregin[2]=0.0;

   // x,y,z axises
   axis[0][0]=dimx;
   axis[0][1]=0.0;
   axis[0][2]=0.0;
   axis[1][0]=0.0;
   axis[1][1]=dimy;
   axis[1][2]=0.0;
   axis[2][0]=0.0;
   axis[2][1]=0.0;
   axis[2][2]=dimz;

   // position of x,y,z labels
   label[0][0]=0.6*dimx;
   label[0][1]=0.0;
   label[0][2]=0.0;
   label[1][0]=0.0;
   label[1][1]=0.6*dimy;
   label[1][2]=0.0;
   label[2][0]=0.0;
   label[2][1]=0.0;
   label[2][2]=0.6*dimz;

   // make oregin the center of plan
   x0=0.5*dimx;
   y0=0.5*dimy;
   z0=0.5*(stack->lpe2);
   oregin[0] -= x0;
   oregin[1] -= y0;
   oregin[2] -= z0;
   // rotate to magnet
   transform(stack->u2m, oregin); 
   // rotate to pixel
   transform(view->m2p, oregin);
   for(i=0; i<3;i++) {
     // move origin to the center
     axis[i][0] -= x0;
     axis[i][1] -= y0;
     axis[i][2] -= z0;
     label[i][0] -= x0;
     label[i][1] -= y0;
     label[i][2] -= z0;
     transform(stack->u2m, axis[i]); 
     transform(stack->u2m, label[i]); 
     transform(view->m2p, axis[i]);
     transform(view->m2p, label[i]);
     lines[i].p1[0]=oregin[0];
     lines[i].p1[1]=oregin[1];
     lines[i].p1[2]=oregin[2];
     lines[i].p2[0]=axis[i][0];
     lines[i].p2[1]=axis[i][1];
     lines[i].p2[2]=axis[i][2];
   }

   // limits
   xmin = view->framestx;
   xmax = view->framestx + view->framewd;
   ymin = view->framesty - view->frameht;
   ymax = view->framesty;

   color(lineColor);
   // draw axis and lable
   for(i=0; i<3;i++) {
     x1=lines[i].p1[0];
     y1=lines[i].p1[1];
     x2=lines[i].p2[0];
     y2=lines[i].p2[1];

     // draw x,y of the two points are NOT the same
     if(!(x1 == x2 && y1 == y2)) {
	if(trimLine(&x1, &y1, &x2, &y2, xmin, ymin, xmax, ymax) == 2) {
	    if((x1 == xmin && x2 == xmin) || (x1 == xmax && x2 == xmax) ||
                (y1 == ymin && y2 == ymin) || (y1 == ymax && y2 == ymax)) {
	    } else {
		drawLine(1, x1, y1, x2, y2, 1, "LineSolid");
		// draw label
		x1=label[i][0];
		y1=label[i][1];
                if(i==0) drawString(1, x1, y1, 8, tag->dim1.name, 
		(int)strlen(tag->dim1.name));
                else if(i==1) drawString(1, x1, y1, 8, tag->dim2.name, 
		(int)strlen(tag->dim2.name));
                else if(i==2) drawString(1, x1, y1, 8, tag->dim3.name, 
		(int)strlen(tag->dim3.name));
	    }
	}
      }
   }

   // draw oregin
   color(highlight);
   drawCircle(1,oregin[0],oregin[1],3,0, 360*64, 2, "LineSolid");

   //color(stack->color);
}

// this will be called by aipGframe
// there is nothing to prevent this being called for non-CSI data.
// It is UI's responsibility to make sure that does not happen.
void aip_drawCSI3DMesh(int id, int lineColor) // here id is image (or view) id.
{
    int nx=1,ny=1,nz=1;
    double d;
    iplan_stack *stack = (iplan_stack *)malloc(sizeof(iplan_stack));
    iplan_view *view = (iplan_view *)malloc(sizeof(iplan_view));

    if(!P_getreal(CURRENT, "nv2", &d, 1) && d>1) nx=(int)d;
    if(!P_getreal(CURRENT, "nv", &d, 1) && d>1) ny=(int)d;
    if(!P_getreal(CURRENT, "nv3", &d, 1) && d>1) nz=(int)d;
    if(nx==1 && ny==1 && nz==1) getStack(stack,VOXEL);
    else if(nz>1) getStack(stack,CSI3D);
    else getStack(stack,CSI2D);

    getIBview(view, id);
    if(view->hasFrame) draw3DMeshForStack(stack,view,lineColor,0,nx,ny,nz);
    else {
	Winfoprintf("aip_drawCSI3DMesh: frame %d is not defined.",id);
    }
    free(view);
    free(stack);
}

// calculate mirror plane, assuming memory are allocated by the caller.
void calcMirrorPlan(iplan_stack *centerStack, iplan_stack *origStack, iplan_stack *mirrorStack)
{
    // calc orientation of mirrorStack by transform a unitary vectors of origStack 
    // to centerStack orientation, reverse z, then rotate it back to megnet frame 
    float x[3] = {1,0,0};
    float y[3] = {0,1,0};
    float z[3] = {0,0,1};

    rotateu2m(x, origStack->orientation);
    rotatem2u(x, centerStack->orientation);
    x[2] = -x[2];
    rotateu2m(x, centerStack->orientation);

    rotateu2m(y, origStack->orientation);
    rotatem2u(y, centerStack->orientation);
    y[2] = -y[2];
    rotateu2m(y, centerStack->orientation);

    rotateu2m(z, origStack->orientation);
    rotatem2u(z, centerStack->orientation);
    z[2] = -z[2];
    rotateu2m(z, centerStack->orientation);

    // unitary vectors x,y,z are now direction cosins of mirrorStack.
    // invert y or z to have a right handed system.
    mirrorStack->orientation[0]=x[0];
    mirrorStack->orientation[1]=x[1];
    mirrorStack->orientation[2]=x[2];
    mirrorStack->orientation[3]=y[0];
    mirrorStack->orientation[4]=y[1];
    mirrorStack->orientation[5]=y[2];
    mirrorStack->orientation[6]=-z[0];
    mirrorStack->orientation[7]=-z[1];
    mirrorStack->orientation[8]=-z[2];

    // now calculate euler angles and orientation
    tensor2euler(&(mirrorStack->theta),
		&(mirrorStack->psi),	
		&(mirrorStack->phi),	
		mirrorStack->orientation);

    x[0] = origStack->ppe;
    x[1] = origStack->pro;
    x[2] = origStack->pss0;
    // rotate position of origStack to centerStack orientation.
    rotateu2m(x, origStack->orientation);
    rotatem2u(x, centerStack->orientation);
    // calculate mirror position,
    x[2] = 2*centerStack->pss0-x[2];
    // rotate to mirror orientation.
    rotateu2m(x, centerStack->orientation);
    rotatem2u(x, mirrorStack->orientation);
    mirrorStack->ppe = x[0];
    mirrorStack->pro = x[1];
    mirrorStack->pss0 = x[2];
}

void calcASLControlPlane(int imageStackType, int tagStackType, int controlStackType, int show)
{
    int i;
    iplan_stack *imageStack = NULL;
    iplan_stack *tagStack = NULL;
    iplan_stack *controlStack = NULL;
    
    if(show && (currentViews == NULL || currentStacks == NULL)) { 
	startIplan0(-1);
    }

    if(!show || currentViews == NULL || currentStacks == NULL) {
        calcMirrorPlane(imageStackType,tagStackType,controlStackType);
	return;
    }

    for(i=0; i<currentStacks->numOfStacks; i++) {
	if(currentStacks->stacks[i].planType == imageStackType) 
		imageStack = &(currentStacks->stacks[i]); 
	if(currentStacks->stacks[i].planType == tagStackType) 
		tagStack = &(currentStacks->stacks[i]); 
	if(currentStacks->stacks[i].planType == controlStackType) 
		controlStack = &(currentStacks->stacks[i]); 
    }
    
    if(imageStack == NULL) addType(imageStackType,"");
    if(tagStack == NULL) addType(tagStackType,"");
    if(controlStack == NULL) addType(controlStackType,"");

    if(imageStack == NULL || tagStack == NULL || controlStack == NULL) {
      // try again
      for(i=0; i<currentStacks->numOfStacks; i++) {
	if(currentStacks->stacks[i].planType == imageStackType) 
		imageStack = &(currentStacks->stacks[i]); 
	if(currentStacks->stacks[i].planType == tagStackType) 
		tagStack = &(currentStacks->stacks[i]); 
	if(currentStacks->stacks[i].planType == controlStackType) 
		controlStack = &(currentStacks->stacks[i]); 
      }
    }
    
    if(imageStack == NULL) {
       Winfoprintf("Abort: imaging plane is missing");
       return;
    }
    if(tagStack == NULL) {
       Winfoprintf("Abort: tag plane is missing");
       return;
    }
    if(controlStack == NULL) {
       Winfoprintf("Abort: control plane is missing");
       return;
    }

    calcMirrorPlan(imageStack, tagStack, controlStack);
    calTransMtx(controlStack);
    calSliceXYZ(controlStack);
    calOverlayForAllViews(currentStacks->numOfStacks-1); 
   
    sendParamsByOrder(ALLSTACKS);
    getCurrentStacks();
}

// calculate mirror plane using current parameters (when not in graphical planning mode)
void calcMirrorPlane(int imageStackType, int tagStackType, int controlStackType)
{
    float theta, psi, phi;
    float x[3] = {1,0,0};
    float y[3] = {0,1,0};
    float z[3] = {0,0,1};
    float imageOrient[9], tagOrient[9], controlOrient[9];
    planParams *imagePars = getCurrPlanParams(imageStackType);
    planParams *tagPars = getCurrPlanParams(tagStackType);
    planParams *controlPars = getCurrPlanParams(controlStackType);

    euler2tensor(imagePars->theta.value, imagePars->psi.value, imagePars->phi.value, imageOrient);
    euler2tensor(tagPars->theta.value, tagPars->psi.value, tagPars->phi.value, tagOrient);
    
    rotateu2m(x, tagOrient);
    rotatem2u(x, imageOrient);
    x[2] = -x[2];
    rotateu2m(x, imageOrient);
    
    rotateu2m(y, tagOrient);
    rotatem2u(y, imageOrient);
    y[2] = -y[2];
    rotateu2m(y, imageOrient);
    
    rotateu2m(z, tagOrient);
    rotatem2u(z, imageOrient);
    z[2] = -z[2];
    rotateu2m(z, imageOrient);
    
    controlOrient[0]=x[0];
    controlOrient[1]=x[1];
    controlOrient[2]=x[2];
    controlOrient[3]=y[0];
    controlOrient[4]=y[1];
    controlOrient[5]=y[2];
    controlOrient[6]=-z[0];
    controlOrient[7]=-z[1];
    controlOrient[8]=-z[2];

    tensor2euler(&(theta), &(psi), &(phi), controlOrient);

    controlPars->theta.value = theta;
    controlPars->psi.value = psi;
    controlPars->phi.value = phi;
    P_setreal(CURRENT, controlPars->theta.name, (double)theta, 1);
    P_setreal(CURRENT, controlPars->psi.name, (double)psi, 1);
    P_setreal(CURRENT, controlPars->phi.name, (double)phi, 1);

    // similarly, invert position of mirrorStack 
    x[0] = tagPars->pos1.value;
    x[1] = tagPars->pos2.value;
    x[2] = tagPars->pos3.value;
    rotateu2m(x, tagOrient);
    rotatem2u(x, imageOrient);
    x[2] = 2*imagePars->pos3.value-x[2];
    rotateu2m(x, imageOrient);
    rotatem2u(x, controlOrient);
    controlPars->pos1.value = x[0];
    controlPars->pos2.value = x[1];
    controlPars->pos3.value = x[2];
    P_setreal(CURRENT, controlPars->pos1.name, (double)controlPars->pos1.value, 1);
    P_setreal(CURRENT, controlPars->pos2.name, (double)controlPars->pos2.value, 1);
    P_setreal(CURRENT, controlPars->pos3.name, (double)controlPars->pos3.value, 1);
}

void utop(int frame, float *u) {
    iplan_view *view = (iplan_view *)malloc(sizeof(iplan_view));
    getIBview(view, frame);
    if(!view->hasFrame) return;

    transform(view->slice.u2m, u); 
    transform(view->m2p, u);
    u[1] = aip_mnumypnts-u[1]-1;
}

void ptou(int frame, float *p) {
    iplan_view *view = (iplan_view *)malloc(sizeof(iplan_view));
    getIBview(view, frame);
    if(!view->hasFrame) return;

    p[1] = aip_mnumypnts-p[1]-1;

    transform(view->p2m, p);
    transform(view->slice.m2u, p); 
}
