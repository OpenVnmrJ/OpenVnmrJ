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
#include "pvars.h"
#include "aipCInterface.h"

iplan_views* currentViews = NULL;
iplan_views* prevViews = NULL;

prescription* activeStacks = NULL;
prescription* currentStacks = NULL;
prescription* prevStacks = NULL;

iplan_overlays* currentOverlays = NULL;
iplan_overlays* prevOverlays = NULL;

int activeStack = -1;
int activeView = -1;

int handleMode = SQUARE;
float defaultGap = 1.0;
int gapFixed = 0;

int defaultType = REGULAR;
int numDefaultSlices = 5;
float defaultSize = 5.0;
float defaultVoxSize = 10.0;
float defaultThk = 0.4;
/* at the moment float is used for all the data. */
/* converting direction tensor to euler angle is limited */
/* by the roundup err of float. this error is defined as */
/* eps. eps = 1.0e-4 corresponds to an error of 0.2 degree. */ 
const float eps = 1.0e-6; 
const float pixeps = 1.0; 
const float pi = 3.14159;

/* 0 parallel, 1 compressed, 2 standard */
int compressMode = 1;
int circlePoints = 9;

extern int getMcoils();
extern int getIplanType();
extern int getColor_m(int coil);
extern int startIplan0(int type);
extern void getOriginForCoil(int coil, float *origin);
extern int stackChanged(prescription* p);
extern int getOverlayType(int planType);
extern int getPlanColor(int planType);
extern int getPlanColorByOrient(int ,float, float, float);

/******************/
void initIplan()
/******************/
{
/* initIplan is called in startIplan to free memory and initialize */
/* all views and stacks. startIplan can be executed repeatedly. */

  if(currentViews == NULL) {
    currentViews = (iplan_views *)malloc(sizeof(iplan_views));
    initViews(currentViews);
  } else freeViews(currentViews);

  if(prevViews == NULL) {
    prevViews = (iplan_views *)malloc(sizeof(iplan_views));
    initViews(prevViews);
  } else freeViews(prevViews);

  if(activeStacks == NULL) {
    activeStacks = (prescription *)malloc(sizeof(prescription));
    initPrescript(activeStacks);
  } else freePrescript(activeStacks);

  if(currentStacks == NULL) {
    currentStacks = (prescription *)malloc(sizeof(prescription));
    initPrescript(currentStacks);
  } else freePrescript(currentStacks);

  if(prevStacks == NULL) {
    prevStacks = (prescription *)malloc(sizeof(prescription));
    initPrescript(prevStacks);
  } else freePrescript(prevStacks);

  if(currentOverlays == NULL) {
    currentOverlays = (iplan_overlays *)malloc(sizeof(iplan_overlays));
    initOverlays(currentOverlays);
  } else freeOverlays(currentOverlays);

  if(prevOverlays == NULL) {
    prevOverlays = (iplan_overlays *)malloc(sizeof(iplan_overlays));
    initOverlays(prevOverlays);
  } else freeOverlays(prevOverlays);
}

/******************/
void freeAStack(iplan_stack* s)
/******************/
{
    if(s == NULL) return;

    if(s->pss != NULL)
    free(s->pss);
    if(s->radialAngles != NULL)
    free(s->radialAngles);
    if(s->order != NULL)
    free(s->order);
    if(s->slices != NULL)
    free(s->slices);
    if(s->envelope.points != NULL)
    free(s->envelope.points);
    s->envelope.np = 0;
    s->ns = 0;

    /* set points to NULL, so free them again won't hurt. */
    s->pss = NULL;
    s->radialAngles = NULL;
    s->order = NULL;
    s->slices = NULL;
    s->envelope.points = NULL;

}

/******************/
void initViews(iplan_views* v)
/******************/
{
    if(v == NULL) 
    v = (iplan_views *)malloc(sizeof(iplan_views));

    v->numOfViews = 0;
    v->views = NULL;
    v->ids = NULL;
}

/******************/
void freeViews(iplan_views* v)
/******************/
{
    int i;

    if(v == NULL) return;

    for(i=0; i<v->numOfViews; i++) {
	freeAStack(&(v->views[i].slice));
    }
    v->numOfViews = 0;
    free(v->views);
    free(v->ids);
    v->views = NULL;
    v->ids = NULL;
}

/******************/
void initPrescript(prescription* p)
/******************/
{
    if(p == NULL) 
    p = (prescription *)malloc(sizeof(prescription));

    p->numOfStacks = 0;
    p->stacks = NULL;
    p->order = NULL;
}

/******************/
void freePrescript(prescription* p)
/******************/
{
    	int j;

    if(p == NULL) return;

        for(j=0; j<p->numOfStacks; j++) {
	    freeAStack(&(p->stacks[j]));
        }
	p->numOfStacks = 0;
        free(p->stacks);
        free(p->order);
	p->stacks = NULL;
	p->order = NULL;
}

/******************/
void initOverlays(iplan_overlays* o)
/******************/
{
    if(o == NULL) 
    o = (iplan_overlays *)malloc(sizeof(iplan_overlays));

	o->numOfOverlays = 0;
	o->overlays = NULL;
}

/******************/
void freeOverlays(iplan_overlays* o)
/******************/
{
    int i, l;

    if(o == NULL) return;

    for(l=0; l<o->numOfOverlays; l++) {
	for(i=0; i<o->overlays[l].numOfStacks; i++) {

	    free2Dstack(&(o->overlays[l].stacks[i]));
	}
        o->overlays[l].numOfStacks = 0;
	free(o->overlays[l].stacks);
        o->overlays[l].stacks = NULL;
    }
    o->numOfOverlays = 0;
    free(o->overlays);
    o->overlays = NULL;
}

/******************/
void displayStack(iplan_stack s) 
/******************/
{
    int i, j;

    fprintf(stderr, "**********************\n");
    fprintf(stderr, "ns = %d\n", s.ns);
    fprintf(stderr, "type = %d\n", s.type);
    fprintf(stderr, "color = %d\n", s.color);
    fprintf(stderr, "theta = %f\n", s.theta);
    fprintf(stderr, "psi = %f\n", s.psi);
    fprintf(stderr, "phi = %f\n", s.phi);
    fprintf(stderr, "thk = %f\n", s.thk);
    fprintf(stderr, "lro = %f\n", s.lro);
    fprintf(stderr, "lpe = %f\n", s.lpe);
    fprintf(stderr, "lpe2 = %f\n", s.lpe2);
    fprintf(stderr, "pro = %f\n", s.pro);
    fprintf(stderr, "ppe = %f\n", s.ppe);
    fprintf(stderr, "pss0 = %f\n", s.pss0);
    fprintf(stderr, "gap = %f\n", s.gap);
    fprintf(stderr, "radialShift = %f\n", s.radialShift);

    for(i=0; i<s.ns; i++) {
	fprintf(stderr, "order, pss, shift, radialAngle = %d %f %f %f %f\n", 
		s.order[i], s.pss[i][0], s.pss[i][1], 
		s.radialAngles[i][0], s.radialAngles[i][1]);
	fprintf(stderr, "slice 8 corner xyz: \n");
        for(j=0; j<8; j++) {
		fprintf(stderr, "%f %f %f\n", s.slices[i].box[j][0],
					s.slices[i].box[j][1],
					s.slices[i].box[j][2]);
	}
	fprintf(stderr, "slice 4 center cut xyz: \n");
        for(j=0; j<4; j++) {
		fprintf(stderr, "%f %f %f\n", s.slices[i].center[j][0],
					s.slices[i].center[j][1],
					s.slices[i].center[j][2]);
	}
    }
   
    fprintf(stderr, "stack envelope: \n");
    for(i=0; i<s.envelope.np; i++)
	fprintf(stderr, "%f %f %f\n", (double) *s.envelope.points[0],
					(double) *s.envelope.points[1],
                                        (double) *s.envelope.points[2]);
    fprintf(stderr, "direction tensors: \n");
    for(i=0; i<3; i++)
        fprintf(stderr, "%f %f %f\n",s.orientation[i*3], s.orientation[i*3+1], 
		s.orientation[i*3+2]); 

    fprintf(stderr, "**********************\n");
}

/******************/
int make3orthogonal(int type)
/******************/
/* 3 orthogonal stacks with numSlices, defaultSize, */
/* the stack is at the center of the image */
{
    if(getMcoils() > 1) {
	makeStackByEuler(0.0, 0.0, 0.0, type);

        if(type == VOXEL) {
    	   execString("vorient='3orthogonal' sorient='trans' orient='3orthogonal'\n");
        } if(type == SATBAND) {
    	   execString("sorient='3orthogonal' orient='trans' vorient='trans'\n");
        } else {
    	   execString("orient='3orthogonal' sorient='trans' vorient='3orthogonal'\n");
        }
	return 1; 
    }

    planParams *tag = getCurrPlanParams(type);
    if(tag == NULL) ABORT;
    setPlanValue(tag->orient.name, "3orthogonal");

    RETURN;
}

/******************/
void snapAngle(float* a)
/******************/
{
    float angle, delta = 1.0;
    angle = *a;
    if((angle > -delta && angle < delta) || (angle > D360-delta && angle < D360+delta))
        angle = 0.0;
    else if(angle > D90-delta && angle < D90+delta) angle = 90;
    else if(angle > -D90-delta && angle < -D90+delta) angle = -90;
    else if(angle > D180-delta && angle < D180+delta) angle = 180;
    else if(angle > -D180-delta && angle < -D180+delta) angle = -180;
    else if(angle > D270-delta && angle < D270+delta) angle = 270;
    else if(angle > -D270-delta && angle < -D270+delta) angle = -270;

    *a = angle;
}

/******************/
void getDefaults(int tree, int planType, 
	int* nslices, float* lpe, float* lro, float* lpe2, float* thk)
/******************/
{
    int r;
    double d;
    planParams *tag = getCurrPlanParams(planType);

    if(tag != NULL) {
   	*nslices = tag->ns.value;
   	*lpe = tag->dim1.value;
   	*lro = tag->dim2.value;
   	*lpe2 = tag->dim3.value;
   	*thk = (tag->thk.value);
    } else {
	*nslices = numDefaultSlices; 
	*lpe = defaultSize;
	*lro = defaultSize;
	*lpe2 = defaultSize;
	*thk = defaultThk;
    }

    if(getOverlayType(planType) == REGULAR &&
	!(r = P_getreal(tree, "gap", &d, 1))) *lpe2 = (*nslices)*(d + (*thk)) - d;
}

/******************/
int makeStackByEuler_m(float theta, float psi, float phi, int type)
/******************/
{
    char str[MAXSTR], cmd[MAXSTR];
    /* to make sure type is up to date */
    type = getIplanType();

    snapAngle(&theta);
    snapAngle(&psi);
    snapAngle(&phi);
    if(theta == 0.0 && psi == 0.0 && phi == 0.0) strcpy(str,"trans");
    else if(theta == 90.0 && psi == 0.0 && phi == 0.0) strcpy(str, "cor");
    else if(theta == 90.0 && psi == 90.0 && phi == 0.0) strcpy(str, "sag"); 
    else if(theta == 0.0 && psi == 0.0 && phi == 90.0) strcpy(str,"trans90");
    else if(theta == 90.0 && psi == 0.0 && phi == 90.0) strcpy(str, "cor90");
    else if(theta == 90.0 && psi == 90.0 && phi == 90.0) strcpy(str, "sag90"); 
    else strcpy(str, "oblique"); 
/*
    if(type == SATBAND) {
       if(!P_getreal(CURRENT, "stheta", &d, 1) && theta != (float)d) 
	setValue("stheta", theta, 1);
       if(!P_getreal(CURRENT, "spsi", &d, 1) && psi != (float)d) 
        setValue("spsi", psi, 1);
       if(!P_getreal(CURRENT, "sphi", &d, 1) && phi != (float)d) 
        setValue("sphi", phi, 1);
    } else if(type == VOXEL) {
       if(!P_getreal(CURRENT, "vtheta", &d, 1) && theta != (float)d) 
	setValue("vtheta", theta, 1);
       if(!P_getreal(CURRENT, "vpsi", &d, 1) && psi != (float)d) 
        setValue("vpsi", psi, 1);
       if(!P_getreal(CURRENT, "vphi", &d, 1) && phi != (float)d) 
        setValue("vphi", phi, 1);
    } else {
       if(!P_getreal(CURRENT, "theta", &d, 1) && theta != (float)d) 
	setValue("theta", theta, 1);
       if(!P_getreal(CURRENT, "psi", &d, 1) && psi != (float)d) 
        setValue("psi", psi, 1);
       if(!P_getreal(CURRENT, "phi", &d, 1) && phi != (float)d) 
        setValue("phi", phi, 1);
    }
*/
           if(type == VOXEL) {
		sprintf(cmd,"vorient='%s' sorient='trans' orient='trans'\n", str);
           } else if(type == SATBAND) {
		sprintf(cmd,"sorient='%s' vorient='trans' orient='trans'\n", str);
           } else {
		sprintf(cmd,"orient='%s' vorient='trans' sorient='trans'\n", str);
           }
    	   execString(cmd);

	   if(strcmp(str, "oblique") == 0 && type == VOXEL) {
		sprintf(cmd,"vtheta=%f vpsi=%f vphi=%f\n", theta, psi, phi);
	   } else if(strcmp(str, "oblique") == 0 && type == SATBAND) {
		sprintf(cmd,"stheta=%f spsi=%f sphi=%f\n", theta, psi, phi);
	   } else if(strcmp(str, "oblique") == 0) {
		sprintf(cmd,"theta=%f psi=%f phi=%f\n", theta, psi, phi);
	   }
    	   execString(cmd);
 
    appendJvarlist("theta psi phi");
    // writelineToVnmrJ("pnew", "3 theta psi phi");
    getCurrentStacks();
    RETURN;
}

/******************/
int makeStackByEuler_1(float theta, float psi, float phi, int planType)
/******************/
/* a single stack with numDefaultSlices, defaultSize, */
/* and orientation theta, psi, phi. */
/* the stack is at the center of the image */
{
    planParams *tag = getCurrPlanParams(planType);
    if(tag == NULL) ABORT;

    clearVar(tag->theta.name);
    clearVar(tag->psi.name);
    clearVar(tag->phi.name);
    P_setreal(CURRENT, tag->theta.name, theta, 1);
    P_setreal(CURRENT, tag->psi.name, psi, 1);
    P_setreal(CURRENT, tag->phi.name, phi, 1); 
    // "oblique" simply means use current theta, psi, phi.
    setPlanValue(tag->orient.name,"oblique"); 
    RETURN;
}

int makeStackByEuler(float theta, float psi, float phi, int type)
{
   if(getMcoils() > 1) makeStackByEuler_m(theta, psi, phi, type);
   else makeStackByEuler_1(theta, psi, phi, type);
   RETURN;
}


/******************/
int makeAstackByEuler(iplan_stack* stack, int type, int ns, float lpe, float lro, 
	float lpe2, float thk, float3 c, float theta, float psi, float phi)
/******************/
{
    float orientation[9];

    euler2tensor(theta, psi, phi, orientation);

    makeAstack(stack, orientation, theta, psi, phi, type, ns, lpe, lro, lpe2, thk, c);

    RETURN;
}

// this uses current theta, psi, phi 
/******************/
void make1pointStack(iplan_stack* stack,  
	int planType, int ns, float lpe, float lro, float lpe2, float thk, float3 c)
/******************/
{
/* this new make1pointStack no long use the orientation of activeView
   to determine the orientation of the overlay. It use parameter orient. */

    char orient[MAXSTR];
    float theta, psi, phi, orientation[9];
    planParams *tag = getCurrPlanParams(planType);

    if(tag != NULL) { 
      theta = tag->theta.value;
      psi = tag->psi.value;
      phi = tag->phi.value;
      strcpy(orient,tag->orient.strValue);      
    } else {
      theta = 0.0;
      psi = 0.0;
      phi = 0.0;
      strcpy(orient,"trans");      
    }

    // does not work for 3 plan.
    if(strcmp(orient,"3orthogonal") == 0) return;

    euler2tensor(theta, psi, phi, orientation);

    makeAstack(stack, orientation, theta, psi, phi, planType, ns, lpe, lro, lpe2, thk, c);
}

// this make plan perpendicular to scout
/******************/
void make1pointStack_default(iplan_stack* stack,  
	int planType, int ns, float lpe, float lro, float lpe2, float thk, float3 c)
/******************/
{
    float theta, psi, phi, orientation[9];
    int view = activeView;
    int type = getOverlayType(planType);

    if(view < 0 || view > currentViews->numOfViews) view = 0;

  //if(type == VOXEL || type == VOLUME) { 
  if(type != SATBAND) { 
	make1pointStack(stack, planType, ns, lpe, lro, lpe2, thk, c);
	return;	
  }

    /* orientation of stack is that of selected view (activeView), */
    /* plus a 90 degree rotation about x-axis so it is perpendicular to */
    /* the selected view (currentViews->views[view].slice) */

    /* get orientation of scout image */

    theta = currentViews->views[view].slice.theta;
    psi = currentViews->views[view].slice.psi;
    phi = currentViews->views[view].slice.phi;

    euler2tensor(theta, psi, phi, orientation);

// rotate about x-axis by 90 or -90 

    if(theta >= 90) rotateY(orientation, orientation, 1.0*D90);
    else rotateY(orientation, orientation, -1.0*D90);

    if(psi >= 90)
    rotateZ(orientation, currentViews->views[view].slice.orientation, -1.0*D90);
    else rotateZ(orientation, currentViews->views[view].slice.orientation, 1.0*D90);

//  to keep tensor in valid form after a rotation, 
//  calculate euler and recalculate the tensor. 
    tensor2euler(&theta, &psi, &phi, orientation);
    euler2tensor(theta, psi, phi, orientation);

    makeAstack(stack, orientation, theta, psi, phi, planType, ns, lpe, lro, lpe2, thk, c);
}

/******************/
int makeAstack(iplan_stack* stack, float* orientation, float theta, float psi, float phi, 
	int planType, int ns, float lpe, float lro, float lpe2, float thk, float3 c)
/******************/
/* thk is in mm */
/* c are in cm, relative to the center of the image */
{
    int i;
    float angle, pro, ppe, pss0, gap, shift;
    float* pss;
    int type = getOverlayType(planType);

    /* c (the center of stack) was in the magnet system, need to be rotated */
    /* to the stack (user) system. */

    if(type != RADIAL) {
	// euler is passed as args, no need to calculate them.
       // tensor2euler(&theta, &psi, &phi, orientation);
	rotatem2u(c, orientation);

    } else {
        /* rotate radial by pi/4 so the orientation of the stack is */
        /* that of the center slice (initially radial fan is pi/2). */

        /* rotate the orientation */
            angle = -1.0*D45;
            rotateX(orientation, orientation, angle);

/*  to keep tensor in valid form after a rotation, */
/*  calculate euler and recalculate the tensor. */
            tensor2euler(&theta, &psi, &phi, orientation);
            euler2tensor(theta, psi, phi, orientation);

        /* rotate the center to the new orientation */
            rotatem2u(c, orientation);

	/* if radialShift == lro, move stack to the center. */
/*
            c[1] -= lro*cos(D2R*angle);
            c[2] += lro*sin(D2R*angle);
*/
    }

    ppe = c[0];
    pro = c[1];
    pss0 = c[2];

    snapAngle(&theta);
    snapAngle(&psi);
    snapAngle(&phi);

/* already scaled by getDefaults(...)
    if(type == VOXEL) {
	lpe *= 0.1;
	lro *= 0.1;
	lpe2 *= 0.1;
     }
*/

    if(type == SATBAND) {
	makeSatBand(type, planType, stack, theta, psi, phi, pss0, thk);
    } else if(type == VOLUME || type == VOXEL) {
	makeVolume(type, planType, stack, theta, psi, phi,
		lpe, lro, lpe2, ppe, pro, pss0);
    } else if(type == REGULAR) {
	
        pss = (float*) malloc(sizeof(float)*ns);


        /*if(ns > 1 && lpe2 > thk) gap = (lpe2 - thk)/(ns - 1); */
	if(ns > 1 && lpe2 > ns*thk) gap = (lpe2 - ns*thk)/(ns - 1);
	else gap = 0;

        if(ns%2 == 0) shift = (ns)/2. - 0.5;
	else shift = (ns-1.0)/2.0;

        /* for(i=0; i<ns; i++) pss[i] = (i-shift)*(gap); */
        for(i=0; i<ns; i++) pss[i] = (i-shift)*(thk + gap);

	makeStack(type, planType, stack, ns, theta, psi, phi,
                  lpe, lro, lpe2, ppe, pro, pss0, gap, thk, pss, 0.0);

	free(pss);
    } else if(type == RADIAL) {

        pss = (float*) malloc(sizeof(float)*ns);

        if(ns > 1) gap = 90/(ns-1); /*gap is angle*/
	else gap = 0;
 
        for(i=0; i<ns; i++) pss[i] = i*gap;

	makeStack(type, planType, stack, ns, theta, psi, phi,
                  lpe, lro, gap*(ns-1), ppe, pro, pss0, gap, thk, pss, 0.0);
	free(pss);
    }
/*
    displayStack(*stack);
*/
    RETURN;
}

/******************/
int makeMultiScans(int tree, int planType, int numStacks)
/******************/
{
    int i, j;
    vInfo           paraminfo;
    
    float* pss;
    float shift, theta, psi, phi, thk, lro, lpe, lpe2, pro, ppe, pss0, gap;
    int ns;

    double d;
    int r;
    int type = getOverlayType(planType);

    ns = 1;
    if(!(P_getVarInfo(tree, "pss", &paraminfo))) ns = paraminfo.size;
    if(ns <= 0) ns = 1;

    pss = (float *)malloc(sizeof(float)*ns);

    freePrescript(activeStacks);
    pen = MAGENTA;
    activeStacks->stacks = (iplan_stack *)malloc(sizeof(iplan_stack)*numStacks);

    activeStacks->numOfStacks = numStacks;

    activeStacks->order = (int *)malloc(sizeof(int)*numStacks);

    for(i=0; i<numStacks; i++) { 
            activeStacks->order[i] = i+1;
    }

    for(i=0; i<numStacks; i++) {

      pro = 0;
      if(!(r = P_getVarInfo(tree, "mpro", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mpro", &d, i+1)) 
	   pro = d;
      }

      ppe = 0;
      if(!(r = P_getVarInfo(tree, "mppe", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mppe", &d, i+1)) 
	   ppe = d;
      }

      pss0 = 0;
      if(!(r = P_getVarInfo(tree, "mpss", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mpss", &d, i+1)) 
	   pss0 = d;
      }

/* use lro etc. to force FOV and thk to be the same */
      lro = defaultSize;
      if(!P_getreal(tree, "lro", &d, 1)) lro = d;

      lpe = defaultSize;
      if(!P_getreal(tree, "lpe", &d, 1)) lpe = d;

      thk = defaultThk;
      if(!P_getreal(tree, "thk", &d, 1)) thk = d*0.1;

      theta=0;
      if(!P_getreal(tree, "theta", &d, 1)) theta = d;

      psi=0;
      if(!P_getreal(tree, "psi", &d, 1)) psi = d;

      phi=0;
      if(!P_getreal(tree, "phi", &d, 1)) phi = d;

      gap=0;
      if(!P_getreal(tree, "gap", &d, 1)) gap = d;

      lpe2=0;
      if(!P_getreal(tree, "lpe2", &d, 1)) lpe2 = d;

     if(type == REGULAR) {
      if(ns%2 == 0) shift = (ns)/2. - 0.5;
      else shift = (ns-1.0)/2.0;
      for(j=0; j<ns; j++) pss[j] = (j-shift)*(thk + gap);

      lpe2 = gap*(ns-1) + ns*thk;
     }

/* use mlro etc. to allow different values
      lro = defaultSize;
      if(!(r = P_getVarInfo(tree, "mlro", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mlro", &d, i+1)) 
	   lro = d;
      }

      lpe = defaultSize;
      if(!(r = P_getVarInfo(tree, "mlpe", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mlpe", &d, i+1)) 
	   lpe = d;
      }

      lpe2 = defaultSize;
      if(!(r = P_getVarInfo(tree, "mlpe2", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mlpe2", &d, i+1)) 
	   lpe2 = d;
      }

      thk = defaultThk;
      if(!(r = P_getVarInfo(tree, "mthk", &paraminfo))) {
	if(i < paraminfo.size && !P_getreal(tree, "mthk", &d, i+1)) 
	   thk = d*0.1;
      }

      if(ns > 1 && lpe2 > ns*thk) gap = (lpe2 - ns*thk)/(ns - 1);
      else gap = 0;

      theta = 0;
      if(!(r = P_getVarInfo(tree, "mtheta", &paraminfo))) {
	if(i < paraminfo.size && !P_getstring(tree, "mtheta", str, i+1, MAXSTR)) {

    	   if((ptr = (char*) strtok(str," "))) {
    	     theta = atof(ptr);
	   }
        }
      }

      psi = 0;
      if(!(r = P_getVarInfo(tree, "mpsi", &paraminfo))) {
	if(i < paraminfo.size && !P_getstring(tree, "mpsi", str, i+1, MAXSTR)) {

    	   if((ptr = (char*) strtok(str," "))) {
    	     psi = atof(ptr);
	   }
        }
      }

      phi = 0;
      if(!(r = P_getVarInfo(tree, "mphi", &paraminfo))) {
	if(i < paraminfo.size && !P_getstring(tree, "mphi", str, i+1, MAXSTR)) {

    	   if((ptr = (char*) strtok(str," "))) {
    	     psi = atof(ptr);
	   }
        }
      }
*/
      if(type == VOLUME)
        makeVolume(type, planType, &(activeStacks->stacks[i]), theta, psi, phi,
                lpe, lro, lpe2, ppe, pro, pss0);
      else {
        makeStack(REGULAR, planType, &(activeStacks->stacks[i]), ns, theta, psi, phi,
                  lpe, lro, lpe2, ppe, pro, pss0, gap, thk, pss, 0.0);
      }

/* overwirte some values set by makeStack */
      activeStacks->stacks[i].color = getColor_m(i+1);
      activeStacks->stacks[i].coil = i+1;
      activeStacks->stacks[i].coils = getMcoils();
      getOriginForCoil(i+1, activeStacks->stacks[i].origin);
    }

    free(pss);
    RETURN;

}

/******************/
int makeActiveScans0(int tree, int planType, int* initk)
/******************/
{
    int numStacks;
    int i, j, ind = 0;
    vInfo           paraminfo;
    
    float* pss;
    float theta, psi, phi, thk, lro, lpe, lpe2, pro, ppe, pss0, gap, radialAngles;
    float *theta1, *psi1, *phi1, *thk1, *lro1, *lpe1, *pro1, *ppe1, *pss01, *pss1, *radialAngles1;
    int ns, ss, size, k;
    float radialShift;
    float *radialShift1;

    double d;
    int r;
    int err;
    int indpss = 0; 
    int ind2 = 0; 
    int comp;
    int type = getOverlayType(planType);
    planParams *tag = getCurrPlanParams(planType);
    if(tag == NULL) return 0;

/* determine compressMode */
    numStacks = 0;
    if(!P_getVarInfo(tree, tag->ns.name, &paraminfo)) numStacks = paraminfo.size;
    if(numStacks > 1 && !P_getVarInfo(tree, tag->pss.name, &paraminfo) &&
        numStacks == paraminfo.size ) comp = 0;
    else comp = compressMode;

/* multiple stacks are arrayed by ns */
/* assuming ns is an integer array of size numStacks, ns[0] is the */
/* number of slices for the first stack and so on.                  */ 
/* pss is a float array of size ns[0] + ns[1]...                    */

    err = 0;

  if(comp != 0) {
    if(!(r = P_getVarInfo(tree, tag->theta.name, &paraminfo))) {
        numStacks = paraminfo.size;
    } else err++;

    if(!(r = P_getVarInfo(tree, tag->psi.name, &paraminfo))) {
        if(paraminfo.size > numStacks) numStacks = paraminfo.size;
    } else err++;

    if(!(r = P_getVarInfo(tree, tag->phi.name, &paraminfo))) {
        if(paraminfo.size > numStacks) numStacks = paraminfo.size;
    } else err++;
  }
    if(err != 0) numStacks = 0;

    size = *initk + numStacks;
    if(*initk == 0) { 
    	freePrescript(activeStacks);
    	pen = MAGENTA;
    	activeStacks->stacks = (iplan_stack *)malloc(sizeof(iplan_stack)*size);
    } else if(numStacks > 0) 
    activeStacks->stacks = (iplan_stack *)realloc(activeStacks->stacks,sizeof(iplan_stack)*size);

    /* get regular stacks, radial stacks and volumes */

    /* single slice stacks will be saved these arrays and dealt with later. */

    theta1 = (float*)malloc(sizeof(float)*numStacks);
    psi1 = (float*)malloc(sizeof(float)*numStacks);
    phi1 = (float*)malloc(sizeof(float)*numStacks);
    thk1 = (float*)malloc(sizeof(float)*numStacks);
    lro1 = (float*)malloc(sizeof(float)*numStacks);
    lpe1 = (float*)malloc(sizeof(float)*numStacks);
    pro1 = (float*)malloc(sizeof(float)*numStacks);
    ppe1 = (float*)malloc(sizeof(float)*numStacks);
    pss01 = (float*)malloc(sizeof(float)*numStacks);
    pss1 = (float*)malloc(sizeof(float)*numStacks);
    radialAngles1 = (float*)malloc(sizeof(float)*numStacks);
    radialShift1 = (float*)malloc(sizeof(float)*numStacks);

    ss = 0;  /* ss is number of single slice stacks */

    k = *initk;

    for(i=0; i<numStacks; i++) {

      err = 0;

      if(!(r = P_getVarInfo(tree, tag->pss.name, &paraminfo))) ns = paraminfo.size;
      else {ns=1; err++;} 

      if(compressMode) {
	ind = i;
	ind2 = 0;
	indpss = 0;
      } else {
	ind = indpss;
	ind2 = indpss;
      }

    /* volnv2 is arrayed for multiple stacks */
    /* determine whether volnv2 > 0, if so type =  VOLUME */
    /* if multislice stack exists, type will be overridden */ 
/*
      if(!(r = P_getreal(tree, "volnv2", &d, ind+1)) 
*/
/*
      if(!P_getreal(tree, "iplanType", &d, ind2+1)) type = (int)d;
      else if(!(r = P_getreal(tree, "volnv2", &d, ind2+1))
        && (int)d > 0) type = VOLUME;
      else type = REGULAR;
*/
      if(!(r = P_getreal(tree, tag->theta.name, &d, ind+1))) theta = d;
      else theta = 0;

      if(!(r = P_getreal(tree, tag->psi.name, &d, ind+1))) psi = d;
      else psi = 0;

      if(!(r = P_getreal(tree, tag->phi.name, &d, ind+1))) phi = d;
      else phi = 0;

      if(!(r = P_getreal(tree, tag->thk.name, &d, ind2+1))) thk = 0.1*d;
      else {thk=0.2; err++;}

      if(!(r = P_getreal(tree, tag->dim1.name, &d, ind2+1))) lpe = d;
      else {lpe=10; err++;}

      if(!(r = P_getreal(tree, tag->dim2.name, &d, ind2+1))) lro = d;
      else {lro=10; err++;}

      if(!(r = P_getreal(tree, tag->dim3.name, &d, ind2+1))) lpe2 = d;
      else {lpe2=10; err++;}

      if(!(r = P_getreal(tree, tag->pos1.name, &d, ind2+1))) ppe = d;
      else {ppe=0; err++;}

      if(!(r = P_getreal(tree, tag->pos2.name, &d, ind2+1))) pro = d;
      else {pro=0; err++;}

      pss = (float*)malloc(sizeof(float)*ns);
      if(err == 0 && type == VOLUME) { 

        if(!(r = P_getreal(tree, tag->pos3.name, &d, indpss+1))) pss[0] = d;
	else {pss[0] = 0; err++;}
        indpss++; 

        if(err == 0) {
            if(!(r = P_getreal(tree, tag->pos3.name, &d, ind2+1))) pss0 = d;
	    else pss0 = pss[0];
 
	    makeVolume(type, planType, &(activeStacks->stacks[k]), theta, psi, phi,
                lpe, lro, lpe2, ppe, pro, pss0);
	    k++;
	}

      } else if(err == 0 && ns > 1) {

        for(j=0; j<ns; j++) {
          if(!(r = P_getreal(tree, tag->pss.name, &d, indpss+1))) {
	      pss[j] = d;
          } else err++;
	  indpss++;
	}

        if(err == 0) {
/*
            if(!(r = P_getreal(tree, posZ, &d, ind2+1))) pss0 = d;
            else {
*/
	    	pss0 = 0.0;
            	for(j=0; j<ns; j++) pss0 += pss[j];
		pss0 /= ns;
/*
	    }
*/
            for(j=0; j<ns; j++) pss[j] -= pss0;

            if(!(r = P_getreal(tree, tag->gap.name, &d, ind2+1))) gap = d;
            else gap = fabs(pss[1] - pss[0]) - thk;
            /* else gap = fabs(pss[1] - pss[0]);*/

            lpe2 = ns*(thk + gap) - gap;
	    makeStack(type, planType, &(activeStacks->stacks[k]), ns, theta, psi, phi,
                  lpe, lro, lpe2, ppe, pro, pss0, gap, thk, pss, 0.0);
	    k++;
	}

      } else if(err == 0 && ns == 1) {

        if(!(r = P_getreal(tree, tag->pss.name, &d, indpss+1))) pss[0] = d;
        else err++;
	indpss++;

        if(!(r = P_getreal(tree, "radialAngles", &d, ind2+1))) radialAngles = d;
        else radialAngles = 0.0;

        if(!(r = P_getreal(tree, "radialShift", &d, ind2+1))) radialShift = d;
        else radialShift = 0.0;
/*
        if(!(r = P_getreal(tree, posZ, &d, ind2+1))) pss0 = d;
	else 
*/
	pss0 = pss[0];

	if(err == 0) {
      	    theta1[ss] = theta;
      	    psi1[ss] = psi;
      	    phi1[ss] = phi;
      	    lro1[ss] = lro;
      	    lpe1[ss] = lpe;
      	    thk1[ss] = thk;
      	    pro1[ss] = pro;
      	    ppe1[ss] = ppe;
      	    pss01[ss] = pss0;
      	    pss1[ss] = pss[0];
      	    radialAngles1[ss] = radialAngles;
	    radialShift1[ss] = radialShift;

	    ss++;
	}
      }
      free(pss);
    }

    if(err == 0 && ss > 0) {
	sortSingleSlices(type, planType, activeStacks, &k, theta1, psi1, phi1,
	thk1, lro1, lpe1, pro1, ppe1, pss01, pss1, radialAngles1, ss, radialShift1);
    }

    free(theta1);
    free(psi1);
    free(phi1);
    free(thk1);
    free(lro1);
    free(lpe1);
    free(pro1);
    free(ppe1);
    free(pss01);
    free(pss1);
    free(radialAngles1);
    free(radialShift1);

/*
    fprintf(stderr, "# of slices, err %d %d\n", k, err);
*/

    if(k < size) 
	activeStacks->stacks = (iplan_stack *)
		realloc(activeStacks->stacks, sizeof(iplan_stack)*k);

    activeStacks->numOfStacks = k;

    if(*initk == 0)
    activeStacks->order = (int *)malloc(sizeof(int)*k);
    else if(numStacks > 0) 
    activeStacks->order = (int *)realloc(activeStacks->order,sizeof(int)*k);

   /* satBands, voxels and stacks are order seperately */ 

    for(i=0; i<k; i++) { 
            activeStacks->order[i] = i+1;
    }

    *initk = k;

    return(k);
}

/******************/
int makeActiveScans(int tree, int planType, int* initk)
/******************/
{
      int coils = getMcoils();
      if(coils > 1) makeMultiScans(tree, planType, coils);
     else makeActiveScans0(tree, planType, initk);
     RETURN;
}

/******************/
int makeActiveSatBands(int tree, int planType, int* initk)
/******************/
{
    int numSatBands = 0;
    int i;
    vInfo           paraminfo;
    
    float stheta, spsi, sphi, satpos, satthk;
    int size, k;

    double d;
    int r;
    int err = 0;
    int type = getOverlayType(planType);
    planParams *tag = getCurrPlanParams(planType);
    if(tag == NULL) return 0;

    if(!(r = P_getVarInfo(tree, tag->theta.name, &paraminfo))) 
	numSatBands = paraminfo.size;
    else err++;

    if(!(r = P_getVarInfo(tree, tag->psi.name, &paraminfo))) {
	if(paraminfo.size < numSatBands) numSatBands = paraminfo.size;
    } else err++;

    if(!(r = P_getVarInfo(tree, tag->phi.name, &paraminfo))) {
	if(paraminfo.size < numSatBands) numSatBands = paraminfo.size;
    } else err++;

    if(err != 0) numSatBands = 0;
	
    size = *initk + numSatBands;
    if(*initk == 0) {
	freePrescript(activeStacks);
    	pen = MAGENTA;
    	activeStacks->stacks = (iplan_stack *)malloc(sizeof(iplan_stack)*size);
    } else if(numSatBands > 0) 
    activeStacks->stacks = (iplan_stack *)realloc(activeStacks->stacks,sizeof(iplan_stack)*size);

    /* get satBand if exists */

    k = *initk;

    for(i=0; i<numSatBands; i++) {

      err = 0;

      if(!(r = P_getreal(tree, tag->theta.name, &d, i+1))) stheta = d;
      else {stheta = 0; err++;}

      if(!(r = P_getreal(tree, tag->psi.name, &d, i+1))) spsi = d;
      else {spsi = 0; err++;}

      if(!(r = P_getreal(tree, tag->phi.name, &d, i+1))) sphi = d;
      else {sphi = 0; err++;}

      if(!(r = P_getreal(tree, tag->pos3.name, &d, i+1))) satpos = d;
      else {satpos = 0; err++;}

      if(!(r = P_getreal(tree, tag->thk.name, &d, i+1))) satthk = 0.1*d;
      else {satthk = 0.5; err++;}

      if(err == 0) {

        makeSatBand(type, planType, &(activeStacks->stacks[k]), stheta, spsi, sphi, satpos, satthk);

	k++;
      }
    }

/*
    fprintf(stderr, "# of slices, err %d %d\n", k, err);
*/

    if(k < size) 
	activeStacks->stacks = (iplan_stack *)
		realloc(activeStacks->stacks, sizeof(iplan_stack)*k);

    activeStacks->numOfStacks = k;

    if(*initk == 0)
    activeStacks->order = (int *)malloc(sizeof(int)*k);
    else if(numSatBands > 0) 
    activeStacks->order = (int *)realloc(activeStacks->order,sizeof(int)*k);

    for(i=0; i<k; i++) { 
	    activeStacks->order[i] = i+1;
    }

    *initk = k;

    return(k);
}

/******************/
int makeActiveVoxels(int tree, int planType, int *initk)
/******************/
{
    int numVoxels=0;
    int i;
    vInfo           paraminfo;
    
    float pos1, pos2, pos3, vox1, vox2, vox3, vphi, vpsi, vtheta;
    int size, k;

    double d;
    int r;
    int err = 0;
    int type = getOverlayType(planType) ;
    planParams *tag = getCurrPlanParams(planType);
    if(tag == NULL) return 0;

    if(!(r = P_getVarInfo(tree, tag->theta.name, &paraminfo)))  
	numVoxels = paraminfo.size;
    else err++;

    if(!(r = P_getVarInfo(tree, tag->psi.name, &paraminfo))) {
	if(paraminfo.size < numVoxels) numVoxels = paraminfo.size;
    } else err++;

    if(!(r = P_getVarInfo(tree, tag->phi.name, &paraminfo))) {
	if(paraminfo.size < numVoxels) numVoxels = paraminfo.size;
    } else err++;

    if(err != 0) numVoxels = 0;

    size = *initk + numVoxels;

    if(*initk == 0) { 
        freePrescript(activeStacks);
    	pen = MAGENTA;
        activeStacks->stacks = (iplan_stack *)malloc(sizeof(iplan_stack)*size);
    } else if(numVoxels > 0) 
    activeStacks->stacks = (iplan_stack *)realloc(activeStacks->stacks,sizeof(iplan_stack)*size);

    /* get voxel if exists */

    k = *initk;

    for(i=0; i<numVoxels; i++) {

      err = 0;

      if(!(r = P_getreal(tree, tag->theta.name, &d, i+1))) vtheta = d;
      else {vtheta = 0; err++;}

      if(!(r = P_getreal(tree, tag->psi.name, &d, i+1))) vpsi = d;
      else {vpsi = 0; err++;}

      if(!(r = P_getreal(tree, tag->phi.name, &d, i+1))) vphi = d;
      else {vphi = 0; err++;}

      if(!(r = P_getreal(tree, tag->pos1.name, &d, i+1))) pos1 = d;
      else {pos1 = 0; err++;}

      if(!(r = P_getreal(tree, tag->pos2.name, &d, i+1))) pos2 = d;
      else {pos2 = 0; err++;}

      if(!(r = P_getreal(tree, tag->pos3.name, &d, i+1))) pos3 = d;
      else {pos3 = 0; err++;}

      if(!(r = P_getreal(tree, tag->dim1.name, &d, i+1))) vox1 = d*0.1;
      else {vox1 = 0; err++;}

      if(!(r = P_getreal(tree, tag->dim2.name, &d, i+1))) vox2 = d*0.1;
      else {vox2 = 0; err++;}

      if(!(r = P_getreal(tree, tag->dim3.name, &d, i+1))) vox3 = d*0.1;
      else {vox3 = 0; err++;}

      if(err == 0) {
	makeVolume(type, planType, &(activeStacks->stacks[k]), vtheta, vpsi, vphi,
                vox1, vox2, vox3, pos1, pos2, pos3);

	k++;
      }
    }

/*
    fprintf(stderr, "# of slices, err %d %d\n", k, err);
*/

    if(k < size) 
	activeStacks->stacks = (iplan_stack *)
		realloc(activeStacks->stacks, sizeof(iplan_stack)*k);

    activeStacks->numOfStacks = k;

    if(*initk == 0)
    activeStacks->order = (int *)malloc(sizeof(int)*k);
    else if(numVoxels > 0) 
    activeStacks->order = (int *)realloc(activeStacks->order,sizeof(int)*k);

    for(i=0; i<k; i++) { 
            activeStacks->order[i] = i+1;
    }

    *initk = k;

    return(k);
}

/******************/
int makeActiveStacks(int tree)
/******************/
{
    double d;
    int initk = 0;
    int i, n, type, planType;
    vInfo           paraminfo;

    if(!(P_getVarInfo(tree, "iplanDefaultType", &paraminfo))) {
        n = paraminfo.size;
    } else n = 0;

    if(n > 0) {
       for(i=0; i<n; i++) {
         if(!(P_getreal(tree, "iplanDefaultType", &d, i+1))) {
            planType = (int)d;
	    type=getOverlayType(planType);
            if(type == VOXEL) makeActiveVoxels(tree, planType, &initk);
            else if(type == SATBAND) makeActiveSatBands(tree, planType, &initk);
            else makeActiveScans(tree, planType, &initk);
       
         } else break;
       }
    } else {

       planType = getIplanType();
       type=getOverlayType(planType);

       if(type == VOXEL) makeActiveVoxels(tree, planType, &initk);
       else if(type == SATBAND) makeActiveSatBands(tree, planType, &initk);
       else makeActiveScans(tree, planType, &initk);
    }

    RETURN;
}

/******************/
void sortSingleSlices(int type, int planType, prescription* p, int* k, float* theta1, float* psi1, 
	float* phi1, float* thk1, float* lro1, float* lpe1, float* pro1, 
	float* ppe1, float* pss01, float* pss1, float* radialAngles1, int ss, float* radialShift1)
/******************/
{
    float theta, psi, phi, thk, lro, lpe, lpe2, pro, ppe, apss, pss0, pssmin, pssmax; 
    float gap, angle;
    float* pss;
    int i, j, l, ns;
    float3 c;
    float radialShift;
/* delta to allow 0.1 cm or 0.1 degree deviation */
    float delta = 0.1;

    pss = (float*)malloc(sizeof(float)*ss);

    /* sort single slices for regular stacks */

    if(ss > 1) {
      for(i=0; i<ss-1; i++) if(radialAngles1[i] > D360) {
	
	ns = 1;
	theta = theta1[i];
	psi = psi1[i];
	phi = phi1[i];
	thk = thk1[i];
	lro = lro1[i];
	lpe = lpe1[i];
	pro = pro1[i];
	ppe = ppe1[i];
	pss[ns-1] = pss1[i];
	apss = pss1[i];
	pss0 = pss01[i];
        pssmin = pss1[i];
        pssmax = pss1[i];

	for(j=i+1; j<ss; j++) {
            if(fabs(theta - theta1[j]) < delta &&
                fabs(psi - psi1[j]) < delta &&
                fabs(phi - phi1[j]) < delta &&
                fabs(thk - thk1[j]) < 0.1*delta &&
                fabs(lro - lro1[j]) < delta &&
                fabs(lpe - lpe1[j]) < delta &&
                fabs(pss0 - pss01[j]) < delta &&
                fabs(pro - pro1[j]) < delta &&
                fabs(ppe - ppe1[j]) < delta ) {

		ss--;
		ns++;
		pss[ns-1] = pss1[j];
		apss += pss1[j];
		if(pss1[j] < pssmin) pssmin = pss1[j];
		if(pss1[j] > pssmax) pssmax = pss1[j];
		for(l=j; l<ss; l++) {
		    theta1[l] = theta1[l+1];
		    psi1[l] = psi1[l+1];
        	    phi1[l] = phi1[l+1];
        	    thk1[l] = thk1[l+1];
        	    lro1[l] = lro1[l+1];
        	    lpe1[l] = lpe1[l+1];
        	    pro1[l] = pro1[l+1];
        	    ppe1[l] = ppe1[l+1];
        	    pss01[l] = pss01[l+1];
		    pss1[l] = pss1[l+1];
                    radialAngles1[l] = radialAngles1[l+1];
                    radialShift1[l] = radialShift1[l+1];
	  	}
		j--;
	    }
	}
	
	if(ns > 1 && type == REGULAR) {
	    lpe2 = pssmax - pssmin + thk;
	    apss /= ns;
/* apps and pss0 should be the same. */
/*
        fprintf(stderr,"apps, pss0 %f %f\n", apss, pss0);
*/
	    /* if(ns > 1 && lpe2 > thk) gap = (lpe2-thk)/(ns-1);*/
	    if(ns > 1 && lpe2 > ns*thk) gap = (lpe2-ns*thk)/(ns-1);
	    else gap = 0;
	    for(j=0; j<ns; j++) pss[j] -= apss;

	    makeStack(type, planType, &(p->stacks[*k]), ns, theta, psi, phi,
                  lpe, lro, lpe2, ppe, pro, apss, gap, thk, pss, 0.0);
	    (*k)++;

            ss--;
	    for(l=i; l<ss; l++) {
		    theta1[l] = theta1[l+1];
                    psi1[l] = psi1[l+1];
                    phi1[l] = phi1[l+1];
                    thk1[l] = thk1[l+1];
                    lro1[l] = lro1[l+1];
                    lpe1[l] = lpe1[l+1];
                    pro1[l] = pro1[l+1];
                    ppe1[l] = ppe1[l+1];
                    pss01[l] = pss01[l+1];
                    pss1[l] = pss1[l+1];
                    radialAngles1[l] = radialAngles1[l+1];
                    radialShift1[l] = radialShift1[l+1];
            }
	    i--;
	}
      }
    }

    /* sort out single slices for radial stacks */
    /* for radial stack, ppe and pss0 are different in user system of */
    /* individual slices, but in magnet system all ppe, pro, pss0 are the same. */  
    /* when the parameters were sent to vnmr, the center (ppe, pro, pss0) was */
    /* rotated by radialAngles1[i] to the user system of individual slices. */
    /* now rotate the center by -radialAngles1[i] to the magnet system. */

    if(ss > 1) {
      for(i=0; i<ss-1; i++) if(radialAngles1[i] <= D360) {
	
	ns = 1;
	theta = theta1[i];
	psi = psi1[i];
	phi = phi1[i];
	thk = thk1[i];
	lro = lro1[i];
	lpe = lpe1[i];
	pss[ns-1] = radialAngles1[i];
	radialShift = radialShift1[i];
        c[0] = ppe1[i];
        c[1] = pro1[i] - radialShift;
        c[2] = pss01[i];
        rotateAngleAboutX(c, -radialAngles1[i]);
        ppe = c[0];
        pro = c[1];
        apss = c[2];
        gap = 0.0;

	for(j=i+1; j<ss; j++) {
            angle = radialAngles1[j];
            c[0] = ppe1[j];
            c[1] = pro1[j] - radialShift;
            c[2] = pss01[j];
            rotateAngleAboutX(c, -angle);
/*
        fprintf(stderr,"center %d %d %f %f %f %f %f %f\n", i, j, ppe, pro, apss, c[0],c[1],c[2]);
*/
            if( fabs(thk - thk1[j]) < 0.1*delta &&
                fabs(lro - lro1[j]) < delta &&
                fabs(lpe - lpe1[j]) < delta &&
                fabs(ppe - c[0]) < 2*delta &&
                fabs(pro - c[1]) < 2*delta &&
                fabs(apss - c[2]) < 2*delta &&
                fabs(radialShift - radialShift1[j]) < delta) {

		ss--;
		ns++;
		pss[ns-1] = angle;
		if(angle > gap) gap = angle;
		for(l=j; l<ss; l++) {
		    theta1[l] = theta1[l+1];
		    psi1[l] = psi1[l+1];
        	    phi1[l] = phi1[l+1];
        	    thk1[l] = thk1[l+1];
        	    lro1[l] = lro1[l+1];
        	    lpe1[l] = lpe1[l+1];
        	    pro1[l] = pro1[l+1];
        	    ppe1[l] = ppe1[l+1];
        	    pss01[l] = pss01[l+1];
        	    radialAngles1[l] = radialAngles1[l+1];
		    radialShift1[l] = radialShift1[l+1];
		}
		j--;
	    }
	}
	
	if(ns > 1 && type == RADIAL) {
	    gap /= (ns-1);

	    makeStack(type, planType, &(p->stacks[*k]), ns, theta, psi, phi,
                  lpe, lro, gap*(ns-1), ppe, pro, apss, gap, thk, pss, radialShift);
	    (*k)++;

            ss--;
	    for(l=i; l<ss; l++) {
		    theta1[l] = theta1[l+1];
                    psi1[l] = psi1[l+1];
                    phi1[l] = phi1[l+1];
                    thk1[l] = thk1[l+1];
                    lro1[l] = lro1[l+1];
                    lpe1[l] = lpe1[l+1];
                    pro1[l] = pro1[l+1];
                    ppe1[l] = ppe1[l+1];
                    pss01[l] = pss01[l+1];
                    radialAngles1[l] = radialAngles1[l+1];
                    radialShift1[l] = radialShift1[l+1];
            }
            i--;
	}
      }
    }

    /* the rest is single slice stacks */

    if(ss > 0 && type == REGULAR) { 
      for(i=0; i<ss; i++) {
	ns = 1;
	pss[0] = pss1[i] - pss01[i];
        thk = thk1[i];
	gap = 0.0;
	makeStack(type, planType, &(p->stacks[*k]), ns, theta1[i], psi1[i], phi1[i],
                  lpe1[i], lro1[i], lpe1[i], ppe1[i], pro1[i], pss01[i], gap, thk, pss, 0.0);
	(*k)++;
      }
    }

    free(pss);
}

/******************/
void orderPss(iplan_stack* s)
/******************/
{
    int i, j, k;
    float value;
    int order;
    int* orders;

    if(s->type == SATBAND || s->type == VOXEL || 
	s->type == VOLUME || s->ns < 2) return;

    if(s->type == REGULAR) {
      for(i=0; i<(s->ns)-1; i++) {
	value = s->pss[i][0];
	k = -1;
	for(j=i+1; j<s->ns; j++) {
            if(s->pss[j][0] < value) {
		value = s->pss[j][0];
		k = j;
	    }
	}
	if(k != -1) {
 	    value = s->pss[i][0];
	    s->pss[i][0] = s->pss[k][0];
	    s->pss[k][0] = value;
	    order = s->order[i];
	    s->order[i] = s->order[k];
	    s->order[k] = order;
	}
      }

    } else if(s->type == RADIAL) {
      for(i=0; i<(s->ns)-1; i++) {
	value = s->radialAngles[i][0];
	k = -1;
	for(j=i+1; j<s->ns; j++) {
            if(s->radialAngles[j][0] < value) {
		value = s->radialAngles[j][0];
		k = j;
	    }
	}
	if(k != -1) {
            value = s->radialAngles[i][0];
	    s->radialAngles[i][0] = s->radialAngles[k][0];
	    s->radialAngles[k][0] = value;
	    order = s->order[i];
	    s->order[i] = s->order[k];
	    s->order[k] = order;
	}
      }
    } 

/* convert acquire orders to mapping orders */

      orders = (int*)malloc(sizeof(int)*s->ns);

      for(k=0; k<s->ns; k++) {
        for(i=0;i<s->ns; i++) {
            if(s->order[i] == k+1) {
               orders[k] = i+1;
               break;
            }
        }
      }
      for(k=0; k<s->ns; k++)
        s->order[k] = orders[k];

      free(orders);
/*
    for(i=0; i<s->ns; i++) 
	fprintf(stderr,"pss, order %f %d\n", s->pss[i][0], s->order[i]);
*/
}

/******************/
void makeSatBand(int type, int planType, iplan_stack* stack, float stheta, float spsi, float sphi, 
	float satpos, float satthk)
/******************/
{

/* set mmouse parameter from currentViews->views. */

        int view = activeView;
        if(view < 0 || view > currentViews->numOfViews) view = 0;

	if(currentViews != NULL && currentViews->numOfViews > 0) {
	  stack->coil = currentViews->views[view].coil;
	  stack->coils = currentViews->views[view].coils;
	  stack->origin[0] = currentViews->views[view].origin[0];
	  stack->origin[1] = currentViews->views[view].origin[1];
	  stack->origin[2] = currentViews->views[view].origin[2];
	} else {
	  stack->coil = 1;
	  stack->coils = 1;
	  stack->origin[0] = 0;
	  stack->origin[1] = 0;
	  stack->origin[2] = 0;
	}

        euler2tensor(stheta, spsi, sphi, stack->orientation);

	stack->mplane = 0;
	stack->type = type;
	stack->planType = planType;
	strcpy(stack->name,getTagNameByType(planType));
        stack->color = getPlanColor(planType);
	stack->ns = 1;
	stack->theta = stheta;
	stack->psi = spsi;
	stack->phi = sphi;
	stack->thk = satthk;
	stack->lro = 0.0;
	stack->lpe = 0.0;
	stack->lpe2 = 0.0;
	stack->pro = 0.0;
	stack->ppe = 0.0;
	stack->pss0 = satpos;
	stack->gap = 0.0;
      	stack->pss = (float2 *) malloc(sizeof(float2));
      	stack->radialAngles = (float2 *) malloc(sizeof(float2));
      	stack->order = (int *) malloc(sizeof(int));
      	stack->slices = (iplan_box *)malloc(sizeof(iplan_box));
	stack->envelope.np = 0;
	stack->envelope.points = NULL;
	stack->order[0] = 1;
	stack->pss[0][0] = 0.0;
	stack->pss[0][1] = 0.0;
	stack->radialAngles[0][0] = 0.0;
	stack->radialAngles[0][1] = 0.0;
        stack->radialShift = 0;

	calTransMtx(stack);
}

/******************/
void calTransMtx(iplan_stack* stack)
/******************/
{
/* m2u is rotatem2u followed by translatem2u */
/* u2m is translateu2m followed by rotateu2m */
/* the following is the net results. */
/* note, shift of the center is negative in translatem2u, */
/* and possitive in translateu2m */

    int j, k;
    float cor[3];

	/* cor is the position (center of the stack) in user system */
        cor[0] = stack->ppe;
        cor[1] = stack->pro;
        cor[2] = stack->pss0;

        for(j=0; j<3; j++) {
            stack->m2u[j][3] = -cor[j];
            for(k=0; k<3; k++) {
                stack->m2u[j][k]
                   = stack->orientation[j*3+k];
                stack->u2m[k][j]
                   = stack->orientation[j*3+k];
            }
        }
        rotateu2m(cor, stack->orientation);
        for(j=0; j<3; j++)
            stack->u2m[j][3] = cor[j];
}

// note, vox1 and vox2, pos1 and pos2 are swapped.
/******************/
void makeVolume(int type, int planType, iplan_stack* stack, float vtheta, float vpsi, float vphi, 
	float vox1, float vox2, float vox3, float pos1, float pos2, float pos3)
/******************/
{
 	int view; 
        planParams *tag = getCurrPlanParams(planType);
	if(type != VOLUME && type != VOXEL) return;

	if(tag != NULL) {
	   pos1 *= tag->pos1.use;
	   pos2 *= tag->pos2.use;
	   pos3 *= tag->pos3.use;
	}

/* set mmouse parameter from currentViews->views. */
        view = activeView;
        if(view < 0 || view > currentViews->numOfViews) view = 0;

	if(currentViews != NULL && currentViews->numOfViews > 0) {
	  stack->coil = currentViews->views[view].coil;
	  stack->coils = currentViews->views[view].coils;
	  stack->origin[0] = currentViews->views[view].origin[0];
	  stack->origin[1] = currentViews->views[view].origin[1];
	  stack->origin[2] = currentViews->views[view].origin[2];
	} else {
	  stack->coil = 1;
	  stack->coils = 1;
	  stack->origin[0] = 0;
	  stack->origin[1] = 0;
	  stack->origin[2] = 0;
	}

        euler2tensor(vtheta, vpsi, vphi, stack->orientation);

	stack->mplane = 0;
	stack->type = type;
	stack->planType = planType;
	strcpy(stack->name,getTagNameByType(planType));
        stack->color = getPlanColor(planType);
	stack->ns = 1;
	stack->theta = vtheta;
	stack->psi = vpsi;
	stack->phi = vphi;
	stack->lpe = vox1;
	stack->lro = vox2;
	stack->lpe2 = vox3;
	stack->thk = vox3;
	stack->ppe = pos1;
	stack->pro = pos2;
	stack->pss0 = pos3;
	stack->gap = 0.0;
      	stack->pss = (float2 *) malloc(sizeof(float2));
      	stack->radialAngles = (float2 *) malloc(sizeof(float2));
      	stack->order = (int *) malloc(sizeof(int));
      	stack->slices = (iplan_box *)malloc(sizeof(iplan_box));
	stack->envelope.np = 8;
	stack->envelope.points = (float3 *) malloc(sizeof(float3)*stack->envelope.np);
	stack->order[0] = 1;
	stack->pss[0][0] = 0.0;
	stack->pss[0][1] = 0.0;
	stack->radialAngles[0][0] = 0.0;
	stack->radialAngles[0][1] = 0.0;
        stack->radialShift = 0;

	calTransMtx(stack);
        calSliceXYZ(stack);
}

/******************/
void makeStack(int type, int planType, iplan_stack* stack, int ns, float theta, float psi, float phi, 
	float lpe, float lro, float lpe2, float ppe, float pro, float pss0, 
	float gap, float thk, float* pss, float radialShift)
/******************/
{
	int i, view;
        planParams *tag = getCurrPlanParams(planType);
	if(type != REGULAR && type != RADIAL) return;

	if(tag != NULL) {
	   ppe *= tag->pos1.use;
	   pro *= tag->pos2.use;
	   pss0 *= tag->pos3.use;
	}

/* set mmouse parameters from currentViews->views. */
 
        view = activeView;
        if(view < 0 || view > currentViews->numOfViews) view = 0;

	if(currentViews != NULL && currentViews->numOfViews > 0) {
	  stack->coil = currentViews->views[view].coil;
	  stack->coils = currentViews->views[view].coils;
	  stack->origin[0] = currentViews->views[view].origin[0];
	  stack->origin[1] = currentViews->views[view].origin[1];
	  stack->origin[2] = currentViews->views[view].origin[2];
	} else {
	  stack->coil = 1;
	  stack->coils = 1;
	  stack->origin[0] = 0;
	  stack->origin[1] = 0;
	  stack->origin[2] = 0;
	}
        euler2tensor(theta, psi, phi, stack->orientation);

	stack->mplane = 0;
	stack->type = type;
	stack->planType = planType;
	strcpy(stack->name,getTagNameByType(planType));
        stack->color = getPlanColorByOrient(planType, theta, psi, phi);
	stack->ns = ns;
	stack->theta = theta;
	stack->psi = psi;
	stack->phi = phi;
	stack->lpe = lpe;
	stack->lro = lro;
	stack->lpe2 = lpe2;
	stack->thk = thk;
	stack->ppe = ppe;
	stack->pro = pro;
	stack->pss0 = pss0;
	stack->gap = gap;
      	stack->pss = (float2 *) malloc(sizeof(float2)*ns);
      	stack->radialAngles = (float2 *) malloc(sizeof(float2)*ns);
      	stack->order = (int *) malloc(sizeof(int)*ns);
      	stack->slices = (iplan_box *)malloc(sizeof(iplan_box)*ns);
	stack->radialShift = radialShift;

	if(type == REGULAR) {

	    stack->envelope.np = 8;
	    stack->envelope.points = (float3 *) malloc(sizeof(float3)*stack->envelope.np);

	    for(i=0; i<ns; i++) {
	    	stack->order[i] = i+1;
	    	stack->pss[i][0] = pss[i];
	    	stack->pss[i][1] = 0.0;
	    	stack->radialAngles[i][0] = 0.0;
	    	stack->radialAngles[i][1] = 0.0;
	    }

	} else if(type == RADIAL) {

	    stack->envelope.np = 4*circlePoints;
	    stack->envelope.points = (float3 *) malloc(sizeof(float3)*stack->envelope.np);

	    for(i=0; i<ns; i++) {
	    	stack->order[i] = i+1;
	    	stack->pss[i][0] = 0.0;
	    	stack->pss[i][1] = 0.0;
	    	stack->radialAngles[i][0] = pss[i];
	    	stack->radialAngles[i][1] = 0.0;
	    }
 	}

	if(ns > 1) orderPss(stack);

	calTransMtx(stack);
        calSliceXYZ(stack);
}

/******************/
void createCommonPlanParams()
/******************/
{
    vInfo           paraminfo;

    if(P_getVarInfo(CURRENT, "iplanDefaultType", &paraminfo) == -2) {
        P_creatvar(CURRENT, "iplanDefaultType", ST_REAL);
	P_setgroup(CURRENT,"iplanDefaultType",G_DISPLAY);
	P_setreal(CURRENT, "iplanDefaultType", (double)0, 1);
    }
    if(P_getVarInfo(CURRENT, "iplanType", &paraminfo) == -2) {
        P_creatvar(CURRENT, "iplanType", ST_REAL);
	P_setgroup(CURRENT,"iplanType",G_DISPLAY);
	P_setreal(CURRENT, "iplanType", (double)0, 1);
    }
    if(P_getVarInfo(CURRENT, "iplanMarking", &paraminfo) == -2) {
        P_creatvar(CURRENT, "iplanMarking", ST_REAL);
	P_setgroup(CURRENT,"iplanMarking",G_DISPLAY);
	P_setreal(CURRENT, "iplanMarking", (double)0, 1);
    }
    if(P_getVarInfo(CURRENT, "planSs", &paraminfo) == -2) {
        P_creatvar(CURRENT, "planSs", ST_REAL);
	P_setgroup(CURRENT,"planSs",G_DISPLAY);
	P_setreal(CURRENT, "planSs", (double)0, 1);
    }
}

void savePrescriptForType(char* path, int planType)
{
   char parlist[MAXSTR], cmd[MAXSTR];
   getPlanParamNames(planType, parlist);
   sprintf(cmd, "writeparam('%s','%s','current','add')\n",path,parlist);
   execString(cmd);
}

void savePrescript(char* path)
{
   vInfo           paraminfo;
   char parlist[MAXSTR], cmd[MAXSTR];
   double d;
   int i, n, planType;

    if(!(P_getVarInfo(CURRENT, "iplanDefaultType", &paraminfo))) {
        n = paraminfo.size;
    } else n = 0;

    if(n > 0) {
       for(i=0; i<n; i++) {
         if(!(P_getreal(CURRENT, "iplanDefaultType", &d, i+1))) {
            planType = (int)d;
            getPlanParamNames(planType, parlist);
	    strcat(parlist,"iplanDefaultType");
	    strcat(parlist,",");
	    strcat(parlist,"iplanType");
    	    sprintf(cmd, "writeparam('%s','%s','current','add')\n",path,parlist);
    	    execString(cmd);
         } else break;
       }
    }
}

/******************/
int saveMilestoneStacks()
/******************/
{
    
    char path[MAXPATH];
    if(currentStacks == NULL || currentViews == NULL) RETURN;

    strcpy(path, curexpdir);
    strcat(path, "/iplan_milestone" );
    savePrescript(path);
    RETURN;
}

/******************/
int getCurrentStacks()
/******************/
{
    if(currentStacks == NULL || currentViews == NULL) {
	if(startIplan0(-1) == -9) ABORT;
    } else makeActiveStacks(CURRENT);

    copyPrescript(currentStacks, prevStacks);
    copyPrescript(activeStacks, currentStacks);

    resetActive();
    updateType();
    sendParamsByOrder(ALLSTACKS);

    calOverlays();
    drawOverlays();
    RETURN;
}

/******************/
int getActiveStacks()
/******************/
{
    if(currentStacks == NULL || currentViews == NULL) {
	if(startIplan0(-1) == -9) RETURN;
    } 
    makeActiveStacks(CURRENT);

    copyPrescript(currentStacks, prevStacks);
    copyPrescript(activeStacks, currentStacks);

    resetActive();
    updateType();
    sendParamsByOrder(ALLSTACKS);

    calOverlays();
    drawOverlays();
    RETURN;
}

/******************/
void readPlanPars(char *path)
/******************/
{
    char cmd[MAXSTR];

    sprintf(cmd, "fread('%s')\n", path);
    execString(cmd);
}

/******************/
int getMilestoneStacks()
/******************/
{
    if(currentStacks == NULL || currentViews == NULL) {
	if(startIplan0(-1) == -9) {
           RETURN;
        }
    }
    
    char path[MAXSTR];
    strcpy(path, curexpdir);
    strcat(path, "/iplan_milestone" );

    readPlanPars(path);
    getCurrentStacks();
    RETURN;
}

/******************/
int copyStack(iplan_stack* s1, iplan_stack* s2)
/******************/
{
	int i, j, k;

	s2->coil = s1->coil;
	s2->coils = s1->coils;
	s2->origin[0] = s1->origin[0];
	s2->origin[1] = s1->origin[1];
	s2->origin[2] = s1->origin[2];
	s2->mplane = s1->mplane;
	s2->ns = s1->ns;
	s2->type = s1->type;
	s2->planType = s1->planType;
	strcpy(s2->name,s1->name);
	s2->color = s1->color;
	s2->theta = s1->theta;
	s2->psi = s1->psi;
	s2->phi = s1->phi;
	s2->thk = s1->thk;
	s2->lro = s1->lro;
	s2->lpe = s1->lpe;
	s2->lpe2 = s1->lpe2;
	s2->pro = s1->pro;
	s2->ppe = s1->ppe;
	s2->pss0 = s1->pss0;
	s2->gap = s1->gap;
	s2->radialShift = s1->radialShift;

	s2->pss = (float2 *)malloc(sizeof(float2)*(s1->ns));
	s2->radialAngles = (float2 *)malloc(sizeof(float2)*(s1->ns));
	s2->order = (int *)malloc(sizeof(int)*(s1->ns));
	s2->slices = (iplan_box *)malloc(sizeof(iplan_box)*(s1->ns));
	
	for(j=0; j<(s1->ns); j++) {
	    s2->pss[j][0] = s1->pss[j][0];
	    s2->pss[j][1] = s1->pss[j][1];
	    s2->radialAngles[j][0] = s1->radialAngles[j][0];
	    s2->radialAngles[j][1] = s1->radialAngles[j][1];
	    s2->order[j] = s1->order[j];
	    for(i=0; i<8; i++) {
	    	for(k=0; k<3; k++) {
		    s2->slices[j].box[i][k] = s1->slices[j].box[i][k];
		}
	    }
	    for(i=0; i<4; i++) {
	    	for(k=0; k<3; k++) {
		    s2->slices[j].center[i][k] = s1->slices[j].center[i][k];
		}
	    }
	}

	s2->envelope.np = s1->envelope.np;
	s2->envelope.points = (float3 *)malloc(sizeof(float3)*s1->envelope.np);

        if(s1->envelope.points != NULL)
	  for(j=0; j<s1->envelope.np; j++) 
	    for(k=0; k<3; k++) s2->envelope.points[j][k] = s1->envelope.points[j][k];
	    
	for(j=0; j<9; j++) s2->orientation[j] = s1->orientation[j];

        for(i=0; i<3; i++)
            for(j=0; j<4; j++) {
                s2->u2m[i][j] = s1->u2m[i][j];
                s2->m2u[i][j] = s1->m2u[i][j];
            }

    RETURN;
}

/******************/
int copyPrescript(prescription* p1, prescription* p2)
/******************/
{

    int i;

    if(p2 != NULL) freePrescript(p2);

    if(p1 == NULL) RETURN;

    if(p1->numOfStacks == 0 || p1->stacks == NULL) {
	p2->numOfStacks = 0;
	p2->stacks = NULL;
    } else {
    	p2->numOfStacks = p1->numOfStacks;
    	p2->stacks = (iplan_stack *)malloc(sizeof(iplan_stack)*(p1->numOfStacks));

    	for(i=0; i<p1->numOfStacks; i++) 
            copyStack(&(p1->stacks[i]), &(p2->stacks[i]));
    }

    if(p1->order == NULL) p2->order = NULL;
    else {
    	p2->order = (int *)malloc(sizeof(int)*(p1->numOfStacks));
    	for(i=0; i<p1->numOfStacks; i++) p2->order[i] = p1->order[i];
    } 
    RETURN;
}

/******************/
void multiplyAB(float* a, float* b, float* c, int row, int col) 
/******************/
/*   C = A.B  */
{
    int i, j, k;
    int l = 0;
    float* d;

    d = (float*)malloc(sizeof(float)*row*col);

    for(i=0; i<row; i++) { 
        for(j=0; j<col; j++) { 
	    d[l] = 0.0;
            for(k=0; k<row; k++) { 
	        d[l] += a[i*row+k]*b[k*col+j];
	    }
	    l++;
	}
    }
    for(i=0; i<row*col; i++) c[i] = d[i];
    for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) c[i] = c[i]/fabs(c[i]); 
    //for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) Winfoprintf("larger than 1 %f",c[i]); 
    free(d);
}

/******************/
void multiplyABt(float* a, float* b, float* c, int row, int col) 
/******************/
/*   C = A.B  */
{
    int i, j, k;
    int l = 0;
    for(i=0; i<row; i++) { 
        for(j=0; j<col; j++) { 
	    c[l] = 0.0;
            for(k=0; k<row; k++) { 
	        c[l] += a[i*row+k]*b[j*col+k];
	    }
	    l++;
	}
    }
    for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) c[i] = c[i]/fabs(c[i]); 
    //for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) Winfoprintf("larger than 1 %f",c[i]); 
}

/******************/
void multiplyAtB(float* a, float* b, float* c, int row, int col) 
/******************/
/*   C = A.B  */
{
    int i, j, k;
    int l = 0;
    for(i=0; i<row; i++) { 
        for(j=0; j<col; j++) { 
	    c[l] = 0.0;
            for(k=0; k<row; k++) { 
	        c[l] += a[k*row+i]*b[k*col+j];
	    }
	    l++;
	}
    }
    for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) c[i] = c[i]/fabs(c[i]); 
    //for(i=0; i<row*col; i++) if(fabs(c[i]) > 1.0) Winfoprintf("larger than 1 %f",c[i]); 
}

/******************/
void multiplyAtBA(float* a, float* b, float* c, int size) 
/******************/
/*   C = At.B.A  where At is transpose of A, A and B are square matrices*/
{
    float *d;
    int i, j, k;
    int l = 0;

    d = (float *)malloc(sizeof(float)*size*size);

    multiplyAB(b,a,d,size,size);

    for(i=0; i<size; i++) { 
        for(j=0; j<size; j++) { 
	    c[l] = 0.0;
            for(k=0; k<size; k++) { 
	        c[l] += a[k*size+i]*d[k*size+j];
	    }
	    l++;
	}
    }
    for(i=0; i<size*size; i++) if(fabs(c[i]) > 1.0) c[i] = c[i]/fabs(c[i]); 
    //for(i=0; i<size*size; i++) if(fabs(c[i]) > 1.0) Winfoprintf("larger than 1 %f",c[i]); 
    free(d);
}

/******************/
void rotateZ(float* a, float* b, float angle)
/******************/
/* rotate plane a by angle about an axis perpendicular to plane b and */
/* passes the center of plane b: Bt.R.B.A */
{
    int i;
    float c[9],d[9];
    float r[9];
    angle *= D2R;

    r[0] = cos(angle); 
    r[1] = sin(angle); 
    r[2] = 0.0; 
    r[3] = -sin(angle); 
    r[4] = cos(angle); 
    r[5] = 0.0; 
    r[6] = 0.0; 
    r[7] = 0.0; 
    r[8] = 1.0; 

    multiplyAtBA(b,r,d,3);
    multiplyAB(a,d,c,3,3); 
    for(i=0; i<9; i++) a[i] = c[i];
}

/******************/
void rotateX(float* a, float* b, float angle)
/******************/
/* rotate plane a by angle about the x axis of plane b and */
/* passes the center of plane b. */
{
    int i;
    float c[9], d[9];
    float r[9];
    angle *= D2R;

    r[0] = 1.0; 
    r[1] = 0.0; 
    r[2] = 0.0; 
    r[3] = 0.0; 
    r[4] = cos(angle); 
    r[5] = sin(angle); 
    r[6] = 0.0; 
    r[7] = -sin(angle); 
    r[8] = cos(angle); 

    multiplyAtBA(b,r,d,3);
    multiplyAB(a,d,c,3,3); 

    for(i=0; i<9; i++) a[i] = c[i];
}

/******************/
void rotateY(float* a, float* b, float angle)
/******************/
/* rotate plane a by angle about the y axis of plane b and */
/* passes the center of plane b. */
{
    int i;
    float c[9], d[9];
    float r[9];
    angle *= D2R;

    r[0] = cos(angle); 
    r[1] = 0.0; 
    r[2] = -sin(angle); 
    r[3] = 0.0; 
    r[4] = 1.0; 
    r[5] = 0.0; 
    r[6] = sin(angle); 
    r[7] = 0.0; 
    r[8] = cos(angle); 

    multiplyAtBA(b,r,d,3);
    multiplyAB(a,d,c,3,3); 

    for(i=0; i<9; i++) a[i] = c[i];
}

/******************/
void translateXY(float* a, float* b, float x, float y)
/******************/
/* translate plane a on plane b by (x,y). */
{
    int i;
    float c[9], d[9];
    float t[9];
    t[0] = 1.0; 
    t[1] = 0.0; 
    t[2] = x; 
    t[3] = 0.0; 
    t[4] = 1.0; 
    t[5] = y; 
    t[6] = 0.0; 
    t[7] = 0.0; 
    t[8] = 1.0; 

    multiplyAtBA(b,t,d,3); 
    multiplyAB(d,a,c,3,3);
    for(i=0; i<9; i++) a[i] = c[i];
}

/******************/
void rotateXYZ(float* a, float* b, float angle, float x, float y)
/******************/
/* rotate plane a by angle about an axis perpendicular to plane b and */
/* passes (x,y) of plane b. */
{
    int i;
    float c[9], d[9], e[9];
    float r[9], t[9];
    angle *= D2R;

    r[0] = cos(angle); 
    r[1] = sin(angle); 
    r[2] = 0.0; 
    r[3] = -sin(angle); 
    r[4] = cos(angle); 
    r[5] = 0.0; 
    r[6] = 0.0; 
    r[7] = 0.0; 
    r[8] = 1.0; 

    t[0] = 1.0; 
    t[1] = 0.0; 
    t[2] = x; 
    t[3] = 0.0; 
    t[4] = 1.0; 
    t[5] = y; 
    t[6] = 0.0; 
    t[7] = 0.0; 
    t[8] = 1.0; 

    multiplyAtBA(t,r,d,3);
    multiplyAtBA(b,d,e,3);
    multiplyAB(e,a,c,3,3); 
    for(i=0; i<9; i++) a[i] = c[i];
}

float trimreal(float f, float pr) {
    int i;
    if(pr == 0.0) pr = 1.0e-4;
    i = (int)(f/pr);
    return i*pr;
}

/******************/
int euler2tensor(float theta, float psi, float phi, float* orientation)
/******************/
/* orientation is (-1, 1, 1)*m2p of image browser, i.e., invert the sign of x-axis. */ 
/* m2p rotates from magnet to pixel frame (origin is left upper corner). */
/* orientation (of stacks) rotates from magnet to "user" frame, i.e., (x, y, z) = */ 
/* (ppe, pro, pss0), a left handed system with origin being the right upper corner. */ 
{
    int i;

    theta *= D2R;
    psi *= D2R;
    phi *= D2R;

    orientation[0] = cos(psi)*cos(phi) + cos(theta)*sin(phi)*sin(psi);
    orientation[1] = sin(psi)*cos(phi) - cos(theta)*sin(phi)*cos(psi);
    orientation[2] = sin(phi)*sin(theta);

    orientation[3] = -cos(psi)*sin(phi) + cos(theta)*cos(phi)*sin(psi);
    orientation[4] = -sin(phi)*sin(psi) - cos(theta)*cos(phi)*cos(psi);
    orientation[5] = cos(phi)*sin(theta);

    orientation[6] = -sin(psi)*sin(theta);
    orientation[7] = cos(psi)*sin(theta);
    orientation[8] = cos(theta);

    for (i=0; i<9; i++)
	orientation[i] = trimreal(orientation[i], eps);

    RETURN;
}

// Note, the solution is not unique.
/******************/
int tensor2euler(float* theta, float* psi, float* phi, float* orientation)
/******************/
{
    int i;

/* take care of round up errors */
    for(i=0; i<9; i++) {
        if(fabs(orientation[i]) < eps) orientation[i] = 0.0;
        if(orientation[i] > 1.0) orientation[i] = 1.0;
        if(orientation[i] < -1.0) orientation[i] = -1.0;
    }

    if(fabs(orientation[8] - 1.0) < eps) {

// theta = 0, cos(theta) = 1, sin(theta) = 0, z-axis unchanged 
// psi and phi are not distinguishable, so set phi=0 
// orientation[0] = cos(psi), orientation[1] = sin(psi) 
// we set phi = 0. 
        *theta = 0.0;
        *phi = 0.0;
        *psi = atan2(orientation[1],orientation[0]);
        if(fabs(*psi) < eps) *psi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;

    } else if(fabs(orientation[8] + 1.0) < eps ) {
        *theta = pi;
        *phi = 0.0;
        *psi = atan2(orientation[1],orientation[0]);
        if(fabs(*psi) < eps) *psi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;

    } else {

// use orientation[8]=cos(theta) if theta is closer to zero (near 90).
// use sin(theta) if theta is closer to +/-1 (near 0 or 180).
// cos(2*theta) = 1-2*(orientation[2]*orientation[2] + orientation[5]*orientation[5])

        *theta = acos(orientation[8]);

        if(fabs(*theta) < eps) *theta = 0.0;

        *psi = atan2(-orientation[6], orientation[7]);
        *phi = atan2(orientation[2], orientation[5]);

        if(fabs(*psi) < eps) *psi = 0.0;
        if(fabs(*phi) < eps) *phi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;
        if(*phi < 0.0) *phi = 2*pi+*phi;

    }

    *theta *= R2D;
    *psi *= R2D;
    *phi *= R2D;

    RETURN;
}

/******************/
int euler2tensorView(float theta, float psi, float phi, float* orientation)
/******************/
// this is the same as in fdf header
{
    theta *= D2R;
    psi *= D2R;
    phi *= D2R;

    orientation[0] = -cos(psi)*cos(phi) - cos(theta)*sin(phi)*sin(psi);
    orientation[1] = -sin(psi)*cos(phi) + cos(theta)*sin(phi)*cos(psi);
    orientation[2] = -sin(phi)*sin(theta);

    orientation[3] = -cos(psi)*sin(phi) + cos(theta)*cos(phi)*sin(psi);
    orientation[4] = -sin(phi)*sin(psi) - cos(theta)*cos(phi)*cos(psi);
    orientation[5] = cos(phi)*sin(theta);

    orientation[6] = -sin(psi)*sin(theta);
    orientation[7] = cos(psi)*sin(theta);
    orientation[8] = cos(theta);

    RETURN;
}

/******************/
int euler2tensorView_3D(float theta, float psi, float phi, float* orientation)
/******************/
// this is the same as in 3D fdf header
{
    theta *= D2R;
    psi *= D2R;
    phi *= D2R;

    orientation[0] = -sin(phi)*sin(psi) - cos(theta)*cos(phi)*cos(psi);
    orientation[1] = cos(psi)*sin(phi) - cos(theta)*cos(phi)*sin(psi);
    orientation[2] = cos(phi)*sin(theta);

    orientation[3] = sin(psi)*cos(phi) - cos(theta)*sin(phi)*cos(psi);
    orientation[4] = -cos(psi)*cos(phi) - cos(theta)*sin(phi)*sin(psi);
    orientation[5] = sin(phi)*sin(theta);

    orientation[6] = cos(psi)*sin(theta);
    orientation[7] = sin(psi)*sin(theta);
    orientation[8] = cos(theta);

    RETURN;
}

/******************/
int tensor2eulerView(float* theta, float* psi, float* phi, float* orientation)
/******************/
{
    int i;

/* take care of round up errors */
    for(i=0; i<9; i++) {
        if(fabs(orientation[i]) < eps) orientation[i] = 0.0;
        if(orientation[i] > 1.0) orientation[i] = 1.0;
        if(orientation[i] < -1.0) orientation[i] = -1.0;
    }

    if(fabs(orientation[8] - 1.0) < eps) {

// theta = 0, cos(theta) = 1, sin(theta) = 0, z-axis unchanged 
// psi and phi are not distinguishable, so set phi zero 
// if phi = 0, orientation[0] = -cos(psi), orientation[1] = -sin(psi) 
// we set phi = 0. 
        *theta = 0.0;
        *phi = 0.0;
        *psi = atan2(-orientation[1],-orientation[0]);
        if(fabs(*psi) < eps) *psi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;

    } else if(fabs(orientation[8] + 1.0) < eps) {
        *theta = pi;
        *phi = 0.0;
        *psi = atan2(-orientation[1],-orientation[0]);
        if(fabs(*psi) < eps) *psi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;

    } else {

// use orientation[8]=cos(theta) if theta is closer to zero (near 90).
// use sin(theta) if theta is closer to +/-1 (near 0 or 180).
// cos(2*theta) = 1-2*(orientation[2]*orientation[2] + orientation[5]*orientation[5])

        *theta = acos(orientation[8]);
 
        if(fabs(*theta) < eps) *theta = 0.0;
        *psi = atan2(-orientation[6], orientation[7]);
        *phi = atan2(-orientation[2], orientation[5]);

        if(fabs(*psi) < eps) *psi = 0.0;
        if(fabs(*phi) < eps) *phi = 0.0;
        if(*psi < 0.0) *psi = 2*pi+*psi;
        if(*phi < 0.0) *phi = 2*pi+*phi;
    }

    *theta *= R2D;
    *psi *= R2D;
    *phi *= R2D;

    RETURN;
}

/******************/
void transform(float m[3][4], float c[3])
/******************/
{
    int i, j;
    float xyz[3];
    for(i=0; i<3; i++) {
        xyz[i] = m[i][3];
        for(j=0; j<3; j++)
            xyz[i] += m[i][j]*c[j];
    }

    for(i=0; i<3; i++)
        c[i] = xyz[i];

}

/******************/
void calSliceXYZ(iplan_stack* s)
/******************/
{
    int i, j, ns;
    float x, y, z, mz, z0, angle;
    float3 rect[4];

    if(s->type == SATBAND) return;

/* make sure I got the definition of lro, lpe and pss right. */
/* check out with image expert about pro and ppe shifts. */

    ns = s->ns;

    x = (s->lpe)*0.5;
    y = (s->lro)*0.5;

    if(s->type == VOLUME) z0 = (s->lpe2)*0.5;
    else z0 = (s->thk)*0.5;

    for(i=0; i<ns; i++) {
        z = s->pss[i][0] + s->pss[i][1] + z0;
        mz = s->pss[i][0] + s->pss[i][1] - z0;
	s->slices[i].box[0][0] = x; 
	s->slices[i].box[0][1] = y; 
	s->slices[i].box[0][2] = z; 
	s->slices[i].box[1][0] = -x; 
	s->slices[i].box[1][1] = y; 
	s->slices[i].box[1][2] = z; 
	s->slices[i].box[2][0] = -x; 
	s->slices[i].box[2][1] = -y; 
	s->slices[i].box[2][2] = z; 
	s->slices[i].box[3][0] = x; 
	s->slices[i].box[3][1] = -y; 
	s->slices[i].box[3][2] = z; 
	s->slices[i].box[4][0] = x; 
	s->slices[i].box[4][1] = y; 
	s->slices[i].box[4][2] = mz; 
	s->slices[i].box[5][0] = -x; 
	s->slices[i].box[5][1] = y; 
	s->slices[i].box[5][2] = mz; 
	s->slices[i].box[6][0] = -x; 
	s->slices[i].box[6][1] = -y; 
	s->slices[i].box[6][2] = mz; 
	s->slices[i].box[7][0] = x; 
	s->slices[i].box[7][1] = -y; 
	s->slices[i].box[7][2] = mz; 

	z = s->pss[i][0] + s->pss[i][1];
	s->slices[i].center[0][0] = x; 
	s->slices[i].center[0][1] = y; 
	s->slices[i].center[0][2] = z; 
	s->slices[i].center[1][0] = -x; 
	s->slices[i].center[1][1] = y; 
	s->slices[i].center[1][2] = z; 
	s->slices[i].center[2][0] = -x; 
	s->slices[i].center[2][1] = -y; 
	s->slices[i].center[2][2] = z; 
	s->slices[i].center[3][0] = x; 
	s->slices[i].center[3][1] = -y; 
	s->slices[i].center[3][2] = z; 

     	if(s->type == RADIAL) {

            angle = s->radialAngles[i][0] + s->radialAngles[i][1];

            for(j=0; j<8; j++) {
                s->slices[i].box[j][1] += s->radialShift;
                rotateAngleAboutX(s->slices[i].box[j], angle);
            }

            for(j=0; j<4; j++) {
                s->slices[i].center[j][1] += s->radialShift;
                rotateAngleAboutX(s->slices[i].center[j], angle);
            }
        }

    	for(j=0; j<8; j++) { 

            transform(s->u2m, s->slices[i].box[j]);
/*
	    rotateu2m(s->slices[i].box[j], s->orientation);
            translateu2m(s->slices[i].box[j], center);
*/
	}

    	for(j=0; j<4; j++) { 

            transform(s->u2m, s->slices[i].center[j]);
/*
	    rotateu2m(s->slices[i].center[j], s->orientation);
            translateu2m(s->slices[i].center[j], center);
*/
	}
    }

/* determine envelope */

   if(s->envelope.np > 0 && s->envelope.points != NULL) {
    if(s->type != RADIAL) {
      for(i=0; i<4; i++)
	for(j=0; j<3; j++) {
	  s->envelope.points[i][j] = s->slices[0].box[4+i][j];
	  s->envelope.points[4+i][j] = s->slices[ns-1].box[i][j];
	}
    } else if(s->type == RADIAL) {
/*
      for(j=0; j<3; j++) {
        s->envelope.points[0][j] = s->slices[0].box[4][j];
        s->envelope.points[ns-1][j] = s->slices[ns-1].box[0][j];
        s->envelope.points[ns][j] = s->slices[ns-1].box[5][j];
        s->envelope.points[2*ns-1][j] = s->slices[0].box[1][j];
        s->envelope.points[2*ns][j] = s->slices[0].box[7][j];
        s->envelope.points[3*ns-1][j] = s->slices[ns-1].box[3][j];
        s->envelope.points[3*ns][j] = s->slices[ns-1].box[6][j];
        s->envelope.points[4*ns-1][j] = s->slices[0].box[2][j];
      } 
      if(ns > 2) for(i=1; i<ns-1; i++) 
 	for(j=0; j<3; j++) {
	  s->envelope.points[i][j] = s->slices[i].center[0][j];
	  s->envelope.points[ns+i][j] = s->slices[ns-1-i].center[1][j];
	  s->envelope.points[2*ns+i][j] = s->slices[i].center[3][j];
	  s->envelope.points[3*ns+i][j] = s->slices[ns-1-i].center[2][j];
	} 
*/
      ns = circlePoints;
      y += fabs(s->radialShift);
      for(i=0; i<ns; i++) {
        rect[0][0] = x;
        rect[0][1] = y;
        rect[0][2] = 0;
        rect[1][0] = -x;
        rect[1][1] = y;
        rect[1][2] = 0;
        rect[2][0] = -x;
        rect[2][1] = -y;
        rect[2][2] = 0;
        rect[3][0] = x;
        rect[3][1] = -y;
        rect[3][2] = 0;

        angle = i*D180/ns;
        for(j=0; j<4; j++) {
                rotateAngleAboutX(rect[j], angle);
                transform(s->u2m, rect[j]);
        }
        for(j=0; j<3; j++) {
          s->envelope.points[i][j] = rect[0][j];
          s->envelope.points[2*ns-i-1][j] = rect[3][j];
          s->envelope.points[2*ns+i][j] = rect[1][j];
          s->envelope.points[4*ns-i-1][j] = rect[2][j];
        }
      }
    }
   }
}

/******************/
void rotatem2u(float3 cor, float* d)
/******************/
{
    int i, j;
    float3 xyz;

    for(i=0; i<3; i++) {
	xyz[i] = 0.0; 
        for(j=0; j<3; j++) {
	    xyz[i] += cor[j]*d[i*3+j]; 
	}
    }
    for(i=0; i<3; i++) {
	cor[i] = xyz[i];
    }
}

void rotateu2m(float3 cor, float* d)
/******************/
{
    int i, j;
    float3 xyz;

    for(i=0; i<3; i++) {
	xyz[i] = 0.0; 
        for(j=0; j<3; j++) {
	    xyz[i] += cor[j]*d[j*3+i]; 
	}
    }
    for(i=0; i<3; i++) {
	cor[i] = xyz[i];
    }
}

/******************/
void rotateAngleAboutX(float3 cor, float angle)
/******************/
{
    float r[9];
    angle *= D2R;

    r[0] = 1.0;
    r[1] = 0.0;
    r[2] = 0.0;
    r[3] = 0.0;
    r[4] = cos(angle);
    r[5] = sin(angle);
    r[6] = 0.0;
    r[7] = -sin(angle);
    r[8] = cos(angle);

    rotateu2m(cor, r);

}

/******************/
void rotateAngleAboutY(float3 cor, float angle)
/******************/
{
    float r[9];
    angle *= D2R;

    r[0] = cos(angle);
    r[1] = 0.0;
    r[2] = -sin(angle);
    r[3] = 0.0;
    r[4] = 1.0;
    r[5] = 0.0;
    r[6] = sin(angle);
    r[7] = 0.0;
    r[8] = cos(angle);

    rotateu2m(cor, r);

}

/******************/
void copyOverlays(iplan_overlays* o1, iplan_overlays* o2)
/******************/
{
}

/******************/
void calOverlay(int view, int stack)
/******************/
{
    if(currentStacks == NULL || currentViews == NULL) return;
    if(view < 0 || view >= currentViews->numOfViews || 
	stack < 0 || stack >= currentStacks->numOfStacks) return;

    if(currentStacks->stacks[stack].type == SATBAND) calSatBandOverlays(view, stack);
    else calStackOverlays(view, stack);
}

/******************/
void calOverlayForAllViews(int stack)
/******************/
{
    int i;
    for(i=0; i<currentViews->numOfViews; i++) 
	calOverlayForAview(stack, i);

}

/******************/
void calOverlayForAview(int stack, int view)
/******************/
{
/* used for graphical updating, where the stacks and views are allocated */

/* free 2Dstack then allocate the memoty since currentStacks->stacks[stack].ns may have */
/* changed */

    free2Dstack(&(currentOverlays->overlays[view].stacks[stack]));

    currentOverlays->overlays[view].stacks[stack].stack = &(currentStacks->stacks[stack]);
    currentOverlays->overlays[view].stacks[stack].numOfStripes =
                currentStacks->stacks[stack].ns;
    currentOverlays->overlays[view].stacks[stack].stripes = (iplan_polygon*)
                malloc(sizeof(iplan_polygon)*(currentStacks->stacks[stack].ns));
    currentOverlays->overlays[view].stacks[stack].lines = (iplan_polygon*)
                malloc(sizeof(iplan_polygon)*(currentStacks->stacks[stack].ns));

	calOverlay(view, stack);
}

/******************/
void calOverlays()
/******************/
{

    int i, nv;

    copyOverlays(currentOverlays, prevOverlays);
    freeOverlays(currentOverlays);

    currentOverlays->numOfOverlays = currentViews->numOfViews;
    currentOverlays->overlays = (iplan_overlay*)
		malloc(sizeof(iplan_overlay)*currentViews->numOfViews);

    for(nv=0; nv<currentViews->numOfViews; nv++) {

    	currentOverlays->overlays[nv].numOfStacks = 
		currentStacks->numOfStacks;

    	currentOverlays->overlays[nv].stacks = (iplan_2Dstack *)
		malloc(sizeof(iplan_2Dstack)*
		(currentOverlays->overlays[nv].numOfStacks));

    	for(i=0; i<currentStacks->numOfStacks; i++) {

    	    currentOverlays->overlays[nv].stacks[i].stack = 
		&(currentStacks->stacks[i]);
	    currentOverlays->overlays[nv].stacks[i].numOfStripes =
		currentStacks->stacks[i].ns;
	    currentOverlays->overlays[nv].stacks[i].stripes = (iplan_polygon*)
		malloc(sizeof(iplan_polygon)*(currentStacks->stacks[i].ns));
	    currentOverlays->overlays[nv].stacks[i].lines = (iplan_polygon*)
		malloc(sizeof(iplan_polygon)*(currentStacks->stacks[i].ns));

	    calOverlay(nv, i);
    	}
    }

}

/******************/
void fitInView(float2* points, int* np, int view)
/******************/
{
    int i, j, k, n;
    int x1, y1, x2, y2, x3, y3, xmin, xmax, ymin, ymax;
    float2* p;

    n = *np;
    p = (float2 *)malloc(sizeof(float2)*n*2);

    xmin = currentViews->views[view].framestx;
    xmax = currentViews->views[view].framestx +(currentViews->views[view].framewd);

    ymin = currentViews->views[view].framesty - (currentViews->views[view].frameht);
    ymax = currentViews->views[view].framesty;

    x2 = points[n-1][0];
    y2 = points[n-1][1];
    x3 = x2;
    y3 = y2;

    *np = 0;

    for(i=0; i<n; i++) {

	x1 = x3;
	y1 = y3;

    	x2 = points[i][0];
    	y2 = points[i][1];
	x3 = x2;
	y3 = y2;

	k = trimLine(&x1, &y1, &x2, &y2, xmin, ymin, xmax, ymax);
	if(k == 1) {
	   p[*np][0] = x1;
	   p[*np][1] = y1;
	   (*np)++ ;
	} else if(k == 2) {
	   p[*np][0] = x1;
	   p[*np][1] = y1;
	   (*np)++ ;
	   p[*np][0] = x2;
	   p[*np][1] = y2;
	   (*np)++ ;
 	}
    }

    n=0;
    for(i=0; i<*np; i++) {
	k=0;
	for(j=0; j<n; j++) {
	    if(p[i][0] == points[j][0] && p[i][1] == points[j][1]) {
		k = 1;
		break;
	    }
	}
	if(k == 0) {
	    points[n][0] = p[i][0];
	    points[n][1] = p[i][1];
	    n++;
	}
    }

    *np = n;

    free(p);
}

/******************/
int trimLine(int* x1, int* y1, int* x2, int* y2, 
	int xmin, int ymin, int xmax, int ymax)
/******************/
{
    int dx, dy;
    int k1 = 0;
    int k2 = 0;
    int x, y;
 
    /* both points are within the window */

    if( *x1 >= xmin && *x1 <= xmax && *x2 >= xmin && *x2 <= xmax && 
	*y1 >= ymin && *y1 <= ymax && *y2 >= ymin && *y2 <= ymax) return(2);

    dx = *x2-*x1;
    dy = *y2-*y1;

    /* when the line is outside the window, but intersects with */
    /* xmin, xmax, ymin, or ymax, the corner near the intersection */
    /* is returned for drawing or filling convex polygon properly. */ 
    /* very tedious !! */

    if(*x1 < xmin && *x2 > xmin) {
	y = *y2 - (*x2-xmin)*dy/dx;
	if(y < ymin && *y2 < ymin) {	
    	    *x1 = xmin;
	    *y1 = ymin;
            return(1);
	}
	if(y > ymax && *y2 > ymax) {
    	    *x1 = xmin;
            *y1 = ymax;
            return(1);
        }
    }
    if(*x2 < xmin && *x1 > xmin) {
	y = *y1 + (xmin-*x1)*dy/dx;
	if(*y1 < ymin && y < ymin) {	
    	    *x1 = xmin;
	    *y1 = ymin;
            return(1);
	}
	if(*y1 > ymax && y > ymax) {
    	    *x1 = xmin;
            *y1 = ymax;
            return(1);
        }
    }

    if(*x1 > xmax && *x2 < xmax) {
	y = *y2 - (*x2-xmax)*dy/dx;
	if(y < ymin && *y2 < ymin) {	
    	    *x1 = xmax;
	    *y1 = ymin;
            return(1);
	}
	if(y > ymax && *y2 > ymax) {
    	    *x1 = xmax;
            *y1 = ymax;
            return(1);
        }
    }
    if(*x2 > xmax && *x1 < xmax) {
	y = *y1 + (xmax-*x1)*dy/dx;
	if(*y1 < ymin && y < ymin) {	
	    *x1 = xmax;
	    *y1 = ymin;
            return(1);
	}
	if(*y1 > ymax && y > ymax) {
            *x1 = xmax;
            *y1 = ymax;
            return(1);
        }
    }

    if(*y1 < ymin && *y2 > ymin) {
	x = *x2 - (*y2-ymin)*dx/dy;
	if(x < xmin && *x2 < xmin) {	
	    *y1 = ymin;
	    *x1 = xmin;
            return(1);
	}
	if(x > xmax && *x2 > xmax) {
            *y1 = ymin;
            *x1 = xmax;
            return(1);
        }
    }
    if(*y2 < ymin && *y1 > ymin) {
	x = *x1 + (ymin-*y1)*dx/dy;
	if(*x1 < xmin && x < xmin) {	
	    *y1 = ymin;
	    *x1 = xmin;
            return(1);
	}
	if(*x1 > xmax && x > xmax) {
            *y1 = ymin;
            *x1 = xmax;
            return(1);
        }
    }

    if(*y1 > ymax && *y2 < ymax) {
	x = *x2 - (*y2-ymax)*dx/dy;
	if(x < xmin && *x2 < xmin) {	
	    *y1 = ymax;
	    *x1 = xmin;
            return(1);
	}
	if(x > xmax && *x2 > xmax) {
            *y1 = ymax;
            *x1 = xmax;
            return(1);
        }
    }
    if(*y2 > ymax && *y1 < ymax) {
	x = *x1 + (ymax-*y1)*dx/dy;
	if(*x1 < xmin && x < xmin) {	
	    *y1 = ymax;
	    *x1 = xmin;
            return(1);
	}
	if(*x1 > xmax && x > xmax) {
            *y1 = ymax;
            *x1 = xmax;
            return(1);
        }
    }

    /* return zero point if the line is outside the window */

    if((*x1 < xmin && *x2 < xmin) || (*x1 > xmax && *x2 > xmax) || 
	(*y1 < ymin && *y2 < ymin) || (*y1 > ymax && *y2 > ymax)) return(0);

    if(*x1 >= xmin && *x1 <= xmax && *y1 >= ymin && *y1 <= ymax) k1++;
    else {

    if(*x1 < xmin) {
    	*x1 = xmin;
	*y1 = *y2 - (*x2-*x1)*dy/dx;
	if(*y1 >= ymin && *y1 <= ymax) k1++;
    } else if(*x1 > xmax) {
	*x1 = xmax;
	*y1 = *y2 - (*x2-*x1)*dy/dx;
	if(*y1 >= ymin && *y1 <= ymax) k1++;
    } 

    if(*y1 < ymin) {
	*y1 = ymin;
	*x1 = *x2 - (*y2-*y1)*dx/dy;
	if(*x1 >= xmin && *x1 <= xmax) k1++;
    } else if(*y1 > ymax) {
	*y1 = ymax;
	*x1 = *x2 - (*y2-*y1)*dx/dy;
	if(*x1 >= xmin && *x1 <= xmax) k1++;
    }
 
    }

    if(*x2 >= xmin && *x2 <= xmax && *y2 >= ymin && *y2 <= ymax) k2++;
    else {

    if(*x2 < xmin) {
	*x2 = xmin;
	*y2 = *y1 + (*x2-*x1)*dy/dx;
	if(*y2 >= ymin && *y2 <= ymax) k2++;
    }
    if(*x2 > xmax) {
	*x2 = xmax;
	*y2 = *y1 + (*x2-*x1)*dy/dx;
	if(*y2 >= ymin && *y2 <= ymax) k2++;
    }
	    
    if(*y2 < ymin) {
	*y2 = ymin;
	*x2 = *x1 + (*y2-*y1)*dx/dy;
	if(*x2 >= xmin && *x2 <= xmax) k2++;
    }
    if(*y2 > ymax) {
	*y2 = ymax;
	*x2 = *x1 + (*y2-*y1)*dx/dy;
	if(*x2 >= xmin && *x2 <= xmax) k2++;
    }
 
    }

    if(k1 == 0) {
	*x1 = *x2;
	*y1 = *y2;
    }

    return(k1+k2);
}

/******************/
void calStackOverlays(int nv, int i)
/******************/
{
     makeOverlay(&(currentStacks->stacks[i]),
		&(currentViews->views[nv]), 
		&(currentOverlays->overlays[nv].stacks[i]));

    calEnvelopeOverlay(nv, i);
/*
    if(stack->type != RADIAL) calRegularEnvelopeOverlay(nv, i);
    else calRadialEnvelopeOverlay(nv, i, nb);
*/
/*
    displayOverlay(currentOverlays->overlays[nv]);
*/
}

// float3 plane[4]
// float3 points[12]
int calcPlaneIntersection(iplan_view *view, float3 *plane, float2 *points) { 
    int k, k1, l, err, np;
    float3 norm, c, cor[4];

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = view->pixwd;

            np = 0;

	    for(k=0; k<4; k++) { 
		  for(l=0; l<3; l++) cor[k][l] = plane[k][l];
		  transform(view->m2p, cor[k]);
	    }

	    for(k=0; k<4; k++) {
		    if((k+1) < 4) k1 = k+1;
		    else k1 = 0;
		    err = calIntersection(norm, cor[k], cor[k1], c);
		    if(err == 0) {
			points[np][0] = c[0];
                        points[np][1] = c[1];
			np++;
		    } else if(err == -1) {
			points[np][0] = cor[k][0];
                        points[np][1] = cor[k][1];
			np++;
			points[np][0] = cor[k1][0];
                        points[np][1] = cor[k1][1];
			np++;
		    }
	    }
    return np;
}

// float3 box[8]
// float3 points[12]
int calcBoxIntersection(iplan_view *view, float3 *box, float2 *points) { 
    int k, k1, l, err, np;
    float3 norm, c, cor[8];

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = view->pixwd;

            np = 0;

	    for(k=0; k<8; k++) { 
		  for(l=0; l<3; l++) cor[k][l] = box[k][l];
		  transform(view->m2p, cor[k]);
	    }

	    for(k=0; k<4; k++) {
		    if(k+1 < 4) k1 = k+1;
		    else k1 = 0;
		    err = calIntersection(norm, cor[k], cor[k1], c); 
		    if(err == 0) {
			points[np][0] = c[0];
                        points[np][1] = c[1];
			np++;
		    } else if(err == -1) {
			points[np][0] = cor[k][0];
                        points[np][1] = cor[k][1];
			np++;
			points[np][0] = cor[k1][0];
                        points[np][1] = cor[k1][1];
			np++;
		    }

		    err = calIntersection(norm, cor[k], cor[k+4], c); 
		    if(err == 0) {
			points[np][0] = c[0];
                        points[np][1] = c[1];
			np++;
		    } else if(err == -1) {
			points[np][0] = cor[k][0];
                        points[np][1] = cor[k][1];
			np++;
			points[np][0] = cor[k+4][0];
                        points[np][1] = cor[k+4][1];
			np++;
		    }

		    err = calIntersection(norm, cor[k+4], cor[k1+4], c); 
		    if(err == 0) {
			points[np][0] = c[0];
                        points[np][1] = c[1];
			np++;
		    } else if(err == -1) {
			points[np][0] = cor[k+4][0];
                        points[np][1] = cor[k+4][1];
			np++;
			points[np][0] = cor[k1+4][0];
                        points[np][1] = cor[k1+4][1];
			np++;
		    }
	    }

	    if(np > 3) orderConvexPolygon(points, &np); 
	return np;
}

/******************/
void makeOverlay(iplan_stack *stack, iplan_view *view, iplan_2Dstack *stack2)
/******************/
{
    int j, k, l, np, nb;
    float2 points[12];

    nb = 0;
    for(j=0; j<stack->ns; j++) {

            np = calcPlaneIntersection(view, stack->slices[j].center, points);
/*
	    fitInView(points, &np, nv);
*/
	    stack2->lines[j].np = np;
    	    stack2->lines[j].points = 
		(float2 *)malloc(sizeof(float2)*np);
	    for(k=0; k<np; k++) {
                for(l=0; l<2; l++)
                    stack2->lines[j]
                        .points[k][l] = points[k][l];
            }

            np = calcBoxIntersection(view, stack->slices[j].box, points);
/*
	    fitInView(points, &np, nv);
*/
	    stack2->stripes[j].np = np;
    	    stack2->stripes[j].points = 
		(float2 *)malloc(sizeof(float2)*np);
	    for(k=0; k<np; k++) {
		nb++;
                for(l=0; l<2; l++)
                    stack2->stripes[j]
                        .points[k][l] = points[k][l];
	    }
    }

}

/******************/
void calRadialEnvelopeOverlay(int nv, int i, int nb)
/******************/
/* didn't quite work for radial. */
/* determine the envelope. using the points of all the stripes in the stack*/
{
    int j, k, l, np;
    float2* boundary;
    float x1, y1, x2, y2, d, dmax;
    int h1=0, h2=0;

	boundary = (float2*)malloc(sizeof(float2)*nb);

	np = 0;
	for(j=0; j<currentStacks->stacks[i].ns; j++) 
	    for(k=0; k<currentOverlays->overlays[nv].stacks[i].stripes[j].np; k++) {
		for(l=0; l<2; l++)
		    boundary[np][l] =
                  currentOverlays->overlays[nv].stacks[i].stripes[j].points[k][l];
		np++;
	    }

	if(np > 3) orderConvexPolygon(boundary, &np);

	currentOverlays->overlays[nv].stacks[i].envelope.np = np;
        currentOverlays->overlays[nv].stacks[i].envelope.points = 
                (float2 *)malloc(sizeof(float2)*np);
        for(k=0; k<np; k++) { 
            for(l=0; l<2; l++)
                currentOverlays->overlays[nv].stacks[i].envelope.points[k][l]
		    = boundary[k][l];
 	}

/* determine the vertices where the handles will be planced. */
/* for REGULAR and VOLUME stacks, handles are placed on all vertices */
/* of the envelope, where as for RADIAL stacks, handles are placed */
/* only on 4 vertices that are separated farthest. */

	if(np < 5 || currentStacks->stacks[i].type != RADIAL) {
	    currentOverlays->overlays[nv].stacks[i].handles.np = np; 
	    currentOverlays->overlays[nv].stacks[i].handles.points =
		(float2 *)malloc(sizeof(float2)*np);
	    for(k=0; k<np; k++) {
              for(l=0; l<2; l++)
                currentOverlays->overlays[nv].stacks[i].handles.points[k][l]
                    = boundary[k][l];
            }
	} else { 

	/* type == RADIAL and np > 4 */

	    currentOverlays->overlays[nv].stacks[i].handles.np = 4;
	    currentOverlays->overlays[nv].stacks[i].handles.points =
		(float2 *)malloc(sizeof(float2)*4);

	    /* find the first pair */

	    x2 = boundary[np-1][0];	
	    y2 = boundary[np-1][1];	

	    dmax = 0.0;
	    for(k=0; k<np; k++) {
		x1 = x2;
          	y1 = y2;

          	x2 = boundary[k][0];
          	y2 = boundary[k][1];

          	d = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
          	if(d > dmax) {
                    dmax = d;
                    h1 = k;
          	}
            }

	    /* find the second pair */

	    x2 = boundary[np-1][0];	
	    y2 = boundary[np-1][1];	

	    dmax = 0.0;
	    for(k=0; k<np; k++) {
		x1 = x2;
          	y1 = y2;

          	x2 = boundary[k][0];
          	y2 = boundary[k][1];

          	d = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
          	if(k != h1 && d > dmax) {
                    dmax = d;
                    h2 = k;
          	}
            }

	    for(k=0; k<2; k++) {
	      	h1 = h1 - k;
	      	if(h1 < 0) h1 = np-1;
	      	h2 = h2 - k;
	      	if(h2 < 0) h2 = np-1;
	    	for(l=0; l<2; l++) { 
	      	   currentOverlays->overlays[nv].stacks[i].handles.points[k][l] =
			boundary[h1][l];
	      	   currentOverlays->overlays[nv].stacks[i].handles.points[k+2][l] =
			boundary[h2][l];
		}
	    }
	}
 
 	free(boundary);
}

/******************/
void getEnvelopeCenter(float* center, float2* pts, int n)
/******************/
{
    int i;
    center[0] = 0.0;
    center[1] = 0.0;

    if(n > 0) {
        for(i=0; i<n; i++) {
            center[0] += pts[i][0];
            center[1] += pts[i][1];
        }
        center[0] /= n;
        center[1] /= n;
    }

}

/******************/
void calRadialHandles(float2* p, int* n, int* sliceInd, 
	iplan_stack *stack, iplan_view *view)
/******************/
{
    int i, j, k, l;
    int ns, np, np2, np4;
    int err;
    float3* points;
    float3 norm, cor1, cor2, c;

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = view->pixwd;

    ns = stack->ns;
    np = 4*ns;

    i = 0;
    if(np > 0) {
      points = (float3*)malloc(sizeof(float3)*np);
      for(j=0; j<ns; j++)
        for(k=0; k<3; k++) {
          points[j][k] = stack->slices[j].center[0][k];
          points[ns+j][k] = stack->slices[ns-1-j].center[3][k];
          points[2*ns+j][k] = stack->slices[j].center[1][k];
          points[3*ns+j][k] = stack->slices[ns-1-j].center[2][k];
        }

        for(j=0; j<np; j++) {
                transform(view->m2p, points[j]);
        }

        np2 = np/2;
        np4 = np/4;

        cor2[0] = points[np2-1][0];
        cor2[1] = points[np2-1][1];
        cor2[2] = points[np2-1][2];

        l = 0;
        for(j=0; j<np2; j++) {

            k = l;
            l = j;
            if(l > ns-1) l = np2-l-1;

            cor1[0] = cor2[0];
            cor1[1] = cor2[1];
            cor1[2] = cor2[2];

            cor2[0] = points[j][0];
            cor2[1] = points[j][1];
            cor2[2] = points[j][2];

            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
                p[i][0] = c[0];
                p[i][1] = c[1];
                   sliceInd[i] = l;
                i++;
            } else if(err == -1) {
                p[i][0] = cor1[0];
                p[i][1] = cor1[1];
                   sliceInd[i] = k;
                i++;
                p[i][0] = cor2[0];
                p[i][1] = cor2[1];
                   sliceInd[i] = l;
                i++;
            }
        }

        cor2[0] = points[2*np2-1][0];
        cor2[1] = points[2*np2-1][1];
        cor2[2] = points[2*np2-1][2];

        l = 0;
        for(j=0; j<np2; j++) {

            k = l;
            l = j;
            if(l > ns-1) l = np2-l-1;

            cor1[0] = cor2[0];
            cor1[1] = cor2[1];
            cor1[2] = cor2[2];

            cor2[0] = points[np2+j][0];
            cor2[1] = points[np2+j][1];
            cor2[2] = points[np2+j][2];

            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
                p[i][0] = c[0];
                p[i][1] = c[1];
                   sliceInd[i] = l;
                i++;
            } else if(err == -1) {
                p[i][0] = cor1[0];
                p[i][1] = cor1[1];
                   sliceInd[i] = k;
                i++;
                p[i][0] = cor2[0];
                p[i][1] = cor2[1];
                   sliceInd[i] = l;
                i++;
            }
        }

        for(j=0; j<np2; j++) {
            l = j;
            if(l > ns-1) l = np2-l-1;
            cor1[0] = points[j][0];
            cor1[1] = points[j][1];
            cor1[2] = points[j][2];
            cor2[0] = points[np2+j][0];
            cor2[1] = points[np2+j][1];
            cor2[2] = points[np2+j][2];
            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
                p[i][0] = c[0];
                p[i][1] = c[1];
                   sliceInd[i] = l;
                i++;
            } else if(err == -1) {
                p[i][0] = cor1[0];
                p[i][1] = cor1[1];
                   sliceInd[i] = l;
                i++;
                p[i][0] = cor2[0];
                p[i][1] = cor2[1];
                   sliceInd[i] = l;
                i++;
            }
        }

        for(j=0; j<np4; j++) {
            l = j;
            cor1[0] = points[j][0];
            cor1[1] = points[j][1];
            cor1[2] = points[j][2];
            cor2[0] = points[np2-j-1][0];
            cor2[1] = points[np2-j-1][1];
            cor2[2] = points[np2-j-1][2];
            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
                p[i][0] = c[0];
                p[i][1] = c[1];
                   sliceInd[i] = l;
                i++;
            } else if(err == -1) {
                p[i][0] = cor1[0];
                p[i][1] = cor1[1];
                   sliceInd[i] = l;
                i++;
                p[i][0] = cor2[0];
                p[i][1] = cor2[1];
                   sliceInd[i] = l;
                i++;
            }

            cor1[0] = points[np2+j][0];
            cor1[1] = points[np2+j][1];
            cor1[2] = points[np2+j][2];
            cor2[0] = points[2*np2-j-1][0];
            cor2[1] = points[2*np2-j-1][1];
            cor2[2] = points[2*np2-j-1][2];
            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
                p[i][0] = c[0];
                p[i][1] = c[1];
                   sliceInd[i] = l;
                i++;
            } else if(err == -1) {
                p[i][0] = cor1[0];
                p[i][1] = cor1[1];
                   sliceInd[i] = l;
                i++;
                p[i][0] = cor2[0];
                p[i][1] = cor2[1];
                   sliceInd[i] = l;
                i++;
            }
        }
        free(points);
    }
    *n = i;
}


/******************/
void calEnvelopeOverlay(int view, int stack)
/******************/
{
     calEnvelope(&(currentStacks->stacks[stack]),
		&(currentViews->views[view]), 
		&(currentOverlays->overlays[view].stacks[stack]));

}

/******************/
void calEnvelope(iplan_stack *stack, iplan_view *view, iplan_2Dstack *stack2)
/******************/
{
    int i, j, k, l;
    int ns, np, np2, np4;
    int err;
    float3* points;
    float2* p;
    float3 norm, cor1, cor2, c;
    int *sliceInd;

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = view->pixwd;

    ns = stack->ns;
    np = stack->envelope.np;
    if(np > 0) {
	points = (float3*)malloc(sizeof(float3)*np);
	p = (float2*)malloc(sizeof(float2)*(4*np));

        for(j=0; j<np; j++) {
              for(k=0; k<3; k++)
                points[j][k] = stack->envelope.points[j][k];

		transform(view->m2p, points[j]);
        }
	
        i = 0;
	np2 = np/2;
        np4 = np/4;

        cor2[0] = points[np2-1][0];
        cor2[1] = points[np2-1][1];
        cor2[2] = points[np2-1][2];

        for(j=0; j<np2; j++) {

            cor1[0] = cor2[0];
            cor1[1] = cor2[1];
            cor1[2] = cor2[2];

            cor2[0] = points[j][0];
            cor2[1] = points[j][1];
            cor2[2] = points[j][2];

	    err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
		p[i][0] = c[0];
                p[i][1] = c[1];
                i++;
	    } else if(err == -1) {
		p[i][0] = cor1[0];
                p[i][1] = cor1[1];
		i++;
		p[i][0] = cor2[0];
                p[i][1] = cor2[1];
		i++;
            }
        }

        cor2[0] = points[2*np2-1][0];
        cor2[1] = points[2*np2-1][1];
        cor2[2] = points[2*np2-1][2];

        for(j=0; j<np2; j++) {

            cor1[0] = cor2[0];
            cor1[1] = cor2[1];
            cor1[2] = cor2[2];

            cor2[0] = points[np2+j][0];
            cor2[1] = points[np2+j][1];
            cor2[2] = points[np2+j][2];

	    err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
		p[i][0] = c[0];
                p[i][1] = c[1];
                i++;
	    } else if(err == -1) {
		p[i][0] = cor1[0];
                p[i][1] = cor1[1];
		i++;
		p[i][0] = cor2[0];
                p[i][1] = cor2[1];
		i++;
            }
        }

	for(j=0; j<np2; j++) {
            cor1[0] = points[j][0];
            cor1[1] = points[j][1];
            cor1[2] = points[j][2];
            cor2[0] = points[np2+j][0];
            cor2[1] = points[np2+j][1];
            cor2[2] = points[np2+j][2];
	    err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
		p[i][0] = c[0];
                p[i][1] = c[1];
            	i++;
	    } else if(err == -1) {
		p[i][0] = cor1[0];
                p[i][1] = cor1[1];
		i++;
		p[i][0] = cor2[0];
                p[i][1] = cor2[1];
		i++;
	    }
        }
	
	if(stack->type == RADIAL) {

          for(j=0; j<np4; j++) {
            cor1[0] = points[j][0];
            cor1[1] = points[j][1];
            cor1[2] = points[j][2];
            cor2[0] = points[np2-j-1][0];
            cor2[1] = points[np2-j-1][1];
            cor2[2] = points[np2-j-1][2];
            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
		p[i][0] = c[0];
                p[i][1] = c[1];
                i++;
	    } else if(err == -1) {
		p[i][0] = cor1[0];
                p[i][1] = cor1[1];
		i++;
		p[i][0] = cor2[0];
                p[i][1] = cor2[1];
		i++;
            }

            cor1[0] = points[np2+j][0];
            cor1[1] = points[np2+j][1];
            cor1[2] = points[np2+j][2];
            cor2[0] = points[2*np2-j-1][0];
            cor2[1] = points[2*np2-j-1][1];
            cor2[2] = points[2*np2-j-1][2];
            err = calIntersection(norm, cor1, cor2, c);
            if(err == 0) {
		p[i][0] = c[0];
                p[i][1] = c[1];
                i++;
	    } else if(err == -1) {
		p[i][0] = cor1[0];
                p[i][1] = cor1[1];
		i++;
		p[i][0] = cor2[0];
                p[i][1] = cor2[1];
		i++;
            }
	  }
        }

        if(i > 3) orderConvexPolygon(p, &i);

        /* call this before the envelope is truncated. */
        getEnvelopeCenter(stack2->envelopeCenter,
                p, i);
/*
	fitInView(p, &i, view);
*/
        stack2->envelope.np = i;
        stack2->envelope.points =
                (float2 *)malloc(sizeof(float2)*i);

        for(k=0; k<i; k++) {
            for(l=0; l<2; l++) {
	   	stack2->envelope.points[k][l]
                    = p[k][l];
	    }
	} 

       if(stack->type != RADIAL) {
        stack2->handles.np = i;
        stack2->handles.points =
                (float2 *)malloc(sizeof(float2)*i);

        for(k=0; k<i; k++) {
            for(l=0; l<2; l++) {
                stack2->handles.points[k][l]
                    = p[k][l];
	    }
	} 

          stack2->handleCenter[0]=
          stack2->envelopeCenter[0];
          stack2->handleCenter[1]=
          stack2->envelopeCenter[1];

          stack2->handles2slices = NULL;
        } else {

          sliceInd = (int*)malloc(sizeof(int)*16*ns);
          p = (float2*)realloc(p, sizeof(float2)*(16*ns));

          calRadialHandles(p, &i, sliceInd, stack, view);

          getEnvelopeCenter(stack2->handleCenter,
                p, i);

          stack2->handles.np = i;
          stack2->handles.points =
                (float2 *)malloc(sizeof(float2)*i);

          for(k=0; k<i; k++) {
            for(l=0; l<2; l++) {
                stack2->handles.points[k][l]
                    = p[k][l];
            }
          }

        /* determine fat handles */
          stack2->handles2slices =
                (int*)malloc(sizeof(int)*i);

          for(l=0; l<i; l++)
                stack2->handles2slices[l] = sliceInd[l];
/*
          for(l=0; l<i; l++)
        fprintf(stderr,"sliceInd %d %d %d\n", view, stack, sliceInd[l]);
*/
          free(sliceInd);
        }

	free(points);
	free(p);
    } else {
	stack2->envelope.np = 0;
	stack2->envelope.points = NULL; 
	stack2->handles.np = 0;
	stack2->handles.points = NULL; 
	stack2->handles2slices = NULL;
    } 
}

/******************/
int containedInCircle(float2 point, float2 center, float r)
/******************/
{
    if(sqrt((point[0]-center[0])*(point[0]-center[0]) + 
	(point[1]-center[1])*(point[1]-center[1])) > r) return(0);
    else return(1);
}

/******************/
int containedInRectangle(float2 point, int stx, int sty, int wd, int ht)
/******************/
{
    if(point[0] > (stx-1) && point[0] < (stx+wd+1) &&
	point[1] < (sty+1) && point[1] > (sty-ht-1)) return(1);
    else return(0); 
}

/******************/
int containedInConvexPolygonOld(float2 point, float2* boundary, int np)
/******************/
{
    float2 center;
    float x1, y1, x2, y2, x, y, r, s, c, py, cy;
    int i, tol = eps;
  
    if(np == 0) return(0);

    if(np == 1) {
	x1 = boundary[0][0] - 2;
	y1 = boundary[0][1] + 2;
	x2 = 4;
	y2 = 4;
	return containedInRectangle(point, x1, y1, x2, y2);
    } else if(np == 2) {
	x1 = min(boundary[0][0], boundary[1][0]) - 2;
	y1 = max(boundary[0][1], boundary[1][1]) + 2;
	x2 = fabs(boundary[1][0] - boundary[0][0]) + 4;
	y2 = fabs(boundary[1][1] - boundary[0][1]) + 4;
	return containedInRectangle(point, x1, y1, x2, y2);
    }
	
    center[0] = 0.0;
    center[1] = 0.0;
    for(i=0; i<np; i++) {
	center[0] += boundary[i][0];
	center[1] += boundary[i][1];
    }
    center[0] /= np;
    center[1] /= np;

    x2 = boundary[np-1][0];
    y2 = boundary[np-1][1];

    for(i=0; i<np; i++) {

  	x1 = x2;
	y1 = y2;
	x2 = boundary[i][0];
    	y2 = boundary[i][1];

	x = x2 - x1;
	y = y2 - y1;
	r = sqrt(x*x + y*y);

	if(r > 0.0) {
	  s = y/r;
	  c = x/r;
	  py = c*(point[1]-y1) - s*(point[0]-x1); 
	  cy = c*(center[1]-y1) - s*(center[0]-x1); 
	/* py and cy should have the same sign */
	  if((cy >= tol && py <= -tol) || (cy <= -tol && py >= tol)) return(0);
	}
    }

    return(1);
}

// compute sum of angles made between the point and each pair of 
// polygon points. If sum is 2pi, then the point is contained.
/******************/
float containedInConvexPolygon(float2 point, float2* boundary, int np)
/******************/
{
    float x1, y1, x2, y2;
    int i;
    float pi2, a, a1, a2, sum;
  
    if(np == 0) return(0);

    pi2 = 2.0*pi;

    if(np == 1) {
	x1 = boundary[0][0] - 2;
	y1 = boundary[0][1] + 2;
	x2 = 4;
	y2 = 4;
	if(containedInRectangle(point, x1, y1, x2, y2)) return(pi2);
        else return(0.0);
    } else if(np == 2) {
	x1 = min(boundary[0][0], boundary[1][0]) - 2;
	y1 = max(boundary[0][1], boundary[1][1]) + 2;
	x2 = fabs(boundary[1][0] - boundary[0][0]) + 4;
	y2 = fabs(boundary[1][1] - boundary[0][1]) + 4;
	if(containedInRectangle(point, x1, y1, x2, y2)) return(pi2);
        else return(0.0);
    }
	
    x2 = boundary[np-1][0] - point[0];
    y2 = boundary[np-1][1] - point[1];
    a2 = atan2(y2,x2);
    if(y2 < 0) a2 += pi2;

    sum = 0.0;
    for(i=0; i<np; i++) {

	a1 = a2;
	x2 = boundary[i][0] - point[0];
    	y2 = boundary[i][1] - point[1];
        a2 = atan2(y2,x2);
        if(y2 < 0) a2 += pi2;
	/* get smallest angle */
        a = fabs(a2 - a1);
        if(a > pi) a = pi2 - a;
	sum += a;
    }
/*
    fprintf(stderr, "sum angle %f\n", sum*360/pi2);
*/
    return(sum);
}

/******************/
float calPloygonArea(float2* polygon, int np)
/******************/
/* polygon corners (points) are already sorted in correct order for connection */
{
    int i, i1;
    float area = 0.0;

    for(i=0; i<np; i++) {
	i1 = i + 1;
	if(i1 == np) i1 = 0;
	area += polygon[i][0]*polygon[i1][1] - polygon[i1][0]*polygon[i][1];
    }

    return 0.5*area;
}

/******************/
void translatem2u(float3 cor, float3 o)
/******************/
{
    int i;
    for(i=0; i<3; i++)
        cor[i] -= o[i];
}

/******************/
void translateu2m(float3 cor, float3 o)
/******************/
{
    int i;
    for(i=0; i<3; i++)
        cor[i] += o[i];
}

/******************/
float distance2(float2 c1, float2 c2)
/******************/
{
    float d1, d2;
    d1 = c1[0]-c2[0];
    d2 = c1[1]-c2[1];
    return(sqrt(d1*d1 + d2*d2));
}

/******************/
float distance3(float3 c1, float3 c2)
/******************/
{
    float d, sum = 0.0;
    int i;
    for(i=0; i<3; i++) {
	d = c1[i] - c2[i];
	sum += d*d;
    }

    return(sqrt(sum));
}

/******************/
int calIntersection(float3 n, float3 a, float3 b, float3 c)
/******************/
{
    int err = 1;

    float t = -1.0;

/*
    f = n[0]*(a[0]-b[0]) + n[1]*(a[1]-b[1]) + n[2]*(a[2]-b[2]);
    if(f != 0.0) t = (n[0]*a[0] + n[1]*a[1] + n[2]*a[2])/f;
*/
    if(fabs(a[2]) < eps) t = 0.0;
    else if(fabs(b[2]) < eps) t = 1.0;
    else if(fabs(a[2]-b[2]) >= eps) t = a[2]/(a[2]-b[2]);
    else if(t >= 0.0 && t<= 1.0 && fabs(a[2]-b[2]) < eps) err = -1; 

    if(t >= 0.0 && t<= 1.0 && err != -1) {
	err = 0;
	c[0] = a[0] + (b[0]-a[0])*t;
	c[1] = a[1] + (b[1]-a[1])*t;
	c[2] = a[2] + (b[2]-a[2])*t;
    }

    return(err);
}

/******************/
void orderConvexPolygon2(float2* points, int* n)
/******************/
/* order the points of a convex polygon. */
{
    int i, j, k, sign;
    float x1, y1, x2, y2, x, y, r, s, c;

    float zero;

    zero = eps*100;

    for(i=0; i<*n-1; i++) {
	x1 = points[i][0];
	y1 = points[i][1];
	for(j=i+1; j<*n; j++) {
          x2 = points[j][0] - x1;
          y2 = points[j][1] - y1;
	  if(fabs(x2) < zero && fabs(y2) < zero) {
		/* i and j are the same, remove j */
	 	points[j][0] = points[*n-1][0];
                points[j][1] = points[*n-1][1];
                (*n)--;
		j--;
	  } else {
	    r = sqrt(x2*x2 + y2*y2);
	    s = y2/r;
            c = x2/r;
            x2 = c*x2 + s*y2;
            y2 = 0.0; 
	    sign = 0;
	    for(k=0; k<*n; k++) {
	      if(k != i && k != j) {
		y = c*(points[k][1]-y1) - s*(points[k][0]-x1);
                if(sign == 0 && y >= zero) sign = 1;
                else if(sign == 0 && y <= -zero) sign = -1;
                else if((sign > 0 && y <= -zero) || (sign < 0 && y >= zero) ) {
		    sign = 0;
		    break;
		}
	      }
	    }
	    if(sign != 0) {
                x = points[i+1][0];
                y = points[i+1][1];
                points[i+1][0] = points[j][0];
                points[i+1][1] = points[j][1];
                points[j][0] = x;
                points[j][1] = y;
	 	break;
	    }
	  }
	}
    }
}

/******************/
void orderConvexPolygon(float2* points, int* n)
/******************/
/* order the points of a convex polygon, and return only the vertices. */
{
    int i, j, k, sign;
    float x1, y1, x2, y2, x, y, r, s, c;

    float zero;

    zero = eps*100;

    for(i=0; i<*n-1; i++) {
	x1 = points[i][0];
	y1 = points[i][1];
	for(j=i+1; j<*n; j++) {
          x2 = points[j][0] - x1;
          y2 = points[j][1] - y1;
	  if(fabs(x2) < zero && fabs(y2) < zero) {
		/* i and j are the same, remove j */
	 	points[j][0] = points[*n-1][0];
                points[j][1] = points[*n-1][1];
                (*n)--;
		j--;
	  } else {
	    r = sqrt(x2*x2 + y2*y2);
	    s = y2/r;
            c = x2/r;
            x2 = c*x2 + s*y2;
            y2 = 0.0; 
	    sign = 0;
	    for(k=0; k<*n; k++) {
	      if(k != i && k != j) {
		y = c*(points[k][1]-y1) - s*(points[k][0]-x1);
		if(fabs(y) < zero) {
		    /* point k is on the line */
		    x = c*(points[k][0]-x1) + s*(points[k][1]-y1);
		    if((x2 < -zero && x > zero) || (x2 > zero && x < -zero)) {
			/* x is outside x1, replace and remove i */
                        points[i][0] = points[k][0];
                        points[i][1] = points[k][1];
                        points[k][0] = points[*n-1][0];
                        points[k][1] = points[*n-1][1];
                        (*n)--;
                        k--;
                        x1 = points[i][0];
                        y1 = points[i][1];
                        x2 = points[j][0] - x1;
                        y2 = points[j][1] - y1;
                        r = sqrt(x2*x2 + y2*y2);
                        s = y2/r;
                        c = x2/r;
                        x2 = c*x2 + s*y2;
                        y2 = 0.0;
		    } else if((x2 < -zero && (x - x2) < -zero) || 
			(x2 > zero && (x - x2) > zero)) {
			/* x is outside x2, replace and remove j */
                        points[j][0] = points[k][0];
                        points[j][1] = points[k][1];
                        points[k][0] = points[*n-1][0];
                        points[k][1] = points[*n-1][1];
                        (*n)--;
                        k--;
                        x2 = points[j][0] - x1;
                        y2 = points[j][1] - y1;
                        r = sqrt(x2*x2 + y2*y2);
                        s = y2/r;
                        c = x2/r;
                        x2 = c*x2 + s*y2;
                        y2 = 0.0;
		    } else {
			/* x is inside x1 and x2, remove k */
                        points[k][0] = points[*n-1][0];
                        points[k][1] = points[*n-1][1];
                        (*n)--;
			k--;
		    }
		    
		} else {
                    if(sign == 0 && y >= zero) sign = 1;
                    else if(sign == 0 && y <= -zero) sign = -1;
                    else if ( (sign > 0 && y <= -zero) || (sign < 0 && y >= zero) ) {
			sign = 0;
			break;
		    }
		}
	      }
	    }
	    if(sign != 0) {
                x = points[i+1][0];
                y = points[i+1][1];
                points[i+1][0] = points[j][0];
                points[i+1][1] = points[j][1];
                points[j][0] = x;
                points[j][1] = y;
	 	break;
	    }
	  }
	}
    }
}

/******************/
void displayOverlay(iplan_overlay o)
/******************/
{
#ifdef XXX
    int i, j, k;

    fprintf(stderr, "**********************\n");
    fprintf(stderr, "numOfStacks = %d\n", o.numOfStacks);

    for(i=0; i<o.numOfStacks; i++) {
	
	fprintf(stderr, "numOfStripes = %d\n", o.stacks[i].numOfStripes); 
        for(j=0; j<o.stacks[i].numOfStripes; j++) {
	    fprintf(stderr, "stripes, numOfPoints = %d\n", o.stacks[i].stripes[j].np);
	    for(k=0; k<o.stacks[i].stripes[j].np; k++) {
	    	fprintf(stderr, "%f %f %f\n", 
			o.stacks[i].stripes[j].points[k][0],
			o.stacks[i].stripes[j].points[k][1],
			o.stacks[i].stripes[j].points[k][2]);
	    }
	    fprintf(stderr, "lines, numOfPoints = %d\n", o.stacks[i].lines[j].np);
	    for(k=0; k<o.stacks[i].lines[j].np; k++) {
	    	fprintf(stderr, "%f %f %f\n", 
			o.stacks[i].lines[j].points[k][0],
			o.stacks[i].lines[j].points[k][1],
			o.stacks[i].lines[j].points[k][2]);
	    }
	}
    }
#endif
}

/******************/
void make2pointStack(iplan_stack* stack, int type, int ns,
        float lpe, float lro, float lpe2, 
	float thk, float angle, float3 c, float size)
/******************/
{
    float theta, psi, phi, orientation[9];
    int view = activeView;
    if(view < 0 || view > currentViews->numOfViews) view = 0;

    lpe = max(lpe, size);
    lro = max(lro, size);
    lpe2 = max(lpe2, size);

    /* get orientation of scout image */

    theta = currentViews->views[view].slice.theta;
    psi = currentViews->views[view].slice.psi;
    phi = currentViews->views[view].slice.phi;

    euler2tensor(theta, psi, phi, orientation);

    /* rotate about x-axis by 90 or -90 */

    if(theta >= 90) rotateY(orientation, orientation, 1.0*D90);
    else rotateY(orientation, orientation, -1.0*D90);

    if(psi >= 90)
    rotateZ(orientation, currentViews->views[view].slice.orientation, angle-1.0*D90);
    else rotateZ(orientation, currentViews->views[view].slice.orientation, angle+1.0*D90);

/*  to keep tensor in valid form after a rotation, */
/*  calculate euler and recalculate the tensor. */
    tensor2euler(&theta, &psi, &phi, orientation);
    euler2tensor(theta, psi, phi, orientation);

    makeAstack(stack, orientation, theta, psi, phi, type, ns, lpe, lro, lpe2, thk, c);
}

/******************/
void make3pointStack(iplan_stack* stack, int type, int ns,
        float lpe, float lro, float lpe2,
	float thk, float3 p1, float3 p2, float3 p3, float3 c, float size)
/******************/
/* x, y are in cm, relative to the center of the image */
{
    float theta, psi, phi, orientation[9];
    int view = activeView;
    if(view < 0 || view > currentViews->numOfViews) view = 0;

    lpe = max(lpe, size);
    lro = max(lro, size);
    lpe2 = max(lpe2, size);

    calEuler(p1, p2, p3, &theta, &psi, &phi);

    euler2tensor(theta, psi, phi, orientation);
/*
    rotateY(orientation, orientation, D180);
*/
/*
    rotateZ(orientation, orientation, 90);
    tensor2euler(&theta, &psi, &phi, orientation);
*/
    makeAstack(stack, orientation, theta, psi, phi, type, ns, lpe, lro, lpe2, thk, c);
}

/******************/
void transposeM(float* f, int row, int col)
/******************/
{
    int i, j;
    float* c;

    c = (float*)malloc(sizeof(float)*row*col);

    for(i=0; i<row; i++)
	for(j=0; j<col; j++)
	    c[j*row + i] = f[i*col + j];

    for(i=0; i<row; i++)
	for(j=0; j<col; j++)
	    f[i*col + j] = c[i*col + j];

    free(c);
}

/******************/
void calEuler(float3 p1, float3 p2, float3 p3, float* theta, float* psi, float* phi)
/******************/
/* calculate theta and psi that define plane that contains the three points.*/
{
    float a, b, c, x, y, z, r, xy;

    /* a, b, c are the directions */

    a = (p1[1] - p3[1])*(p2[2] - p3[2]) - (p2[1] - p3[1])*(p1[2] - p3[2]);
    b = (p2[0] - p3[0])*(p1[2] - p3[2]) - (p1[0] - p3[0])*(p2[2] - p3[2]);
    c = (p1[0] - p3[0])*(p2[1] - p3[1]) - (p2[0] - p3[0])*(p1[1] - p3[1]);

    /* x, y, z are direction cos's, i.e., cos(alphs), cos(beta), cos(gamma) */

    r = sqrt(a*a + b*b + c*c);

    if(r != 0.0) {

    x = a/r;   
    y = b/r;   
    z = c/r;   

    /* somehow theta is reversed with the above formular. */ 
    /* acos(z) is a left hand theta, but we need a right hand theta. */
/*
    *theta = -acos(z);
*/
    *theta = acos(z);

    /* psi is the angel between y axis and the xy projection of the normal. */
    /* using right hand rotation */

    xy = sqrt(x*x + y*y);

    if(xy != 0.0 && x <= 0.0) *psi = acos(y/xy);
    else if(xy != 0.0 && x > 0.0) *psi = 2.0*pi - acos(y/xy);
    else if(xy == 0.0) {
	*theta = 0.0;
	*psi = 0.0;
    }

    } else {
	*theta = 0.0;
        *psi = 0.0;
    }
/*
    *psi = pi - *psi;
*/
    /* phi is the angle between p1-p2 and x-axis */

    *phi = 0.0;
/*
    euler2tensor(*theta, *psi, *phi, orientation);
    rotatem2u(p1, orientation);
    rotatem2u(p2, orientation);
    x = p2[0]-p1[0];
    y = p2[1]-p1[1];

    if(x == 0.0) *phi = 0.5*pi;
    else *phi = atan2(y, x);
*/
    *theta *= R2D;
    *psi *= R2D;
    *phi *= R2D;
}

/******************/
void calSatBandOverlays(int nv, int i)
/******************/
{
    int k, l, err, np;
    float3 origin, norm, c, c1, c2, cor[8];
    float2 points[12];
    float thk2, handle2, xmin, xmax, ymin, ymax;
    float frame[4][3];

    /* shrink the frame by handleSize on each side */
    /* so satBand handles are not too close to the edges */
    handle2 = handleSize*0.5;

/* don't use frame size
    xmin = currentViews->views[nv].framestx + handle2;
    xmax = currentViews->views[nv].framestx +(currentViews->views[nv].framewd) - handle2;

    ymin = currentViews->views[nv].framesty - (currentViews->views[nv].frameht) + handle2;
    ymax = currentViews->views[nv].framesty - handle2;
*/
/* use image size */
    xmin = currentViews->views[nv].pixstx + handle2;
    xmax = currentViews->views[nv].pixstx +(currentViews->views[nv].pixwd) - handle2;

    ymin = currentViews->views[nv].pixsty - (currentViews->views[nv].pixht) + handle2;
    ymax = currentViews->views[nv].pixsty - handle2;
  
    frame[0][0] = xmin; 
    frame[0][1] = ymax; 
    frame[0][2] = 0.0;

    frame[1][0] = xmax; 
    frame[1][1] = ymax; 
    frame[1][2] = 0.0;

    frame[2][0] = xmax; 
    frame[2][1] = ymin; 
    frame[2][2] = 0.0;

    frame[3][0] = xmin; 
    frame[3][1] = ymin; 
    frame[3][2] = 0.0;

    for(k=0; k<4; k++) 
    transform(currentViews->views[nv].p2m, frame[k]);

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = currentViews->views[nv].pixwd;

    thk2 = 0.5*(currentStacks->stacks[i].thk);

    np = 0;

    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = currentStacks->stacks[i].pss0;

/* c1 and c2 are the two points define the line */

    for(k=0; k<4; k++) {
          for(l=0; l<3; l++) cor[k][l] = frame[k][l]; 
/*
          for(l=0; l<3; l++) cor[k][l] = 
                currentViews->views[nv].slice.slices[0].center[k][l];
*/
          rotatem2u(cor[k], currentStacks->stacks[i].orientation);
          translatem2u(cor[k], origin);

    }

    for(k=0; k<4; k++) {
            for(l=0; l<3; l++) {
                c1[l] = cor[k][l];
                if((k+1) < 4) c2[l] = cor[k+1][l];
                else c2[l] = cor[0][l];
            }
            if(c1[2] > -thk2 && c2[2] > -thk2 && c1[2] < thk2) {
                translateu2m(c1, origin);
                rotateu2m(c1, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c1);
                points[np][0] = c1[0];
                points[np][1] = c1[1];
                np++;
            } else if(c1[2] > -thk2 && c2[2] > -thk2 && c2[2] < thk2) {
                translateu2m(c2, origin);
                rotateu2m(c2, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c2);
                points[np][0] = c2[0];
                points[np][1] = c2[1];
                np++;
            } else if(c1[2] < thk2 && c2[2] < thk2 && c1[2] > -thk2) {
                translateu2m(c1, origin);
                rotateu2m(c1, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c1);
                points[np][0] = c1[0];
                points[np][1] = c1[1];
                np++;
            } else if(c1[2] < thk2 && c2[2] < thk2 && c2[2] > -thk2) {
                translateu2m(c2, origin);
                rotateu2m(c2, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c2);
                points[np][0] = c2[0];
                points[np][1] = c2[1];
                np++;
            }
    }

/* intersection of the first layer */

    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = currentStacks->stacks[i].pss0 - thk2;

    for(k=0; k<4; k++) {
          for(l=0; l<3; l++) cor[k][l] = frame[k][l]; 
/*
          for(l=0; l<3; l++) cor[k][l] =
                currentViews->views[nv].slice.slices[0].center[k][l];
*/
          rotatem2u(cor[k], currentStacks->stacks[i].orientation);
          translatem2u(cor[k], origin);
    }

    for(k=0; k<4; k++) {
            for(l=0; l<3; l++) {
                c1[l] = cor[k][l];
                if((k+1) < 4) c2[l] = cor[k+1][l];
                else c2[l] = cor[0][l];
            }
            err = calIntersection(norm, c1, c2, c);
            if(err == 0) {
                translateu2m(c, origin);
                rotateu2m(c, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c);
                points[np][0] = c[0];
                points[np][1] = c[1];
                np++;
            } else if(err == -1) {
                translateu2m(c1, origin);
                rotateu2m(c1, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c1);
                points[np][0] = c1[0];
                points[np][1] = c1[1];
                np++;
                translateu2m(c2, origin);
                rotateu2m(c2, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c2);
                points[np][0] = c2[0];
                points[np][1] = c2[1];
                np++;
            }
    }

/* intersection of the second layer */

    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = currentStacks->stacks[i].pss0 + thk2;

    for(k=0; k<4; k++) {
          for(l=0; l<3; l++) cor[k][l] = frame[k][l]; 
/*
          for(l=0; l<3; l++) cor[k][l] =
                currentViews->views[nv].slice.slices[0].center[k][l];
*/
          rotatem2u(cor[k], currentStacks->stacks[i].orientation);
          translatem2u(cor[k], origin);
    }

    for(k=0; k<4; k++) {
            for(l=0; l<3; l++) {
                c1[l] = cor[k][l];
                if((k+1) < 4) c2[l] = cor[k+1][l];
                else c2[l] = cor[0][l];
            }
            err = calIntersection(norm, c1, c2, c);
            if(err == 0) {
                translateu2m(c, origin);
                rotateu2m(c, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c);
                points[np][0] = c[0];
                points[np][1] = c[1];
                np++;
            } else if(err == -1) {
                translateu2m(c1, origin);
                rotateu2m(c1, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c1);
                points[np][0] = c1[0];
                points[np][1] = c1[1];
                np++;
                translateu2m(c2, origin);
                rotateu2m(c2, currentStacks->stacks[i].orientation);
                transform(currentViews->views[nv].m2p, c2);
                points[np][0] = c2[0];
                points[np][1] = c2[1];
                np++;
            }
    }

    if(np > 3) orderConvexPolygon(points, &np);

    getEnvelopeCenter(currentOverlays->overlays[nv].stacks[i].envelopeCenter,
                points, np);
    currentOverlays->overlays[nv].stacks[i].handleCenter[0]=
    currentOverlays->overlays[nv].stacks[i].envelopeCenter[0];
    currentOverlays->overlays[nv].stacks[i].handleCenter[1]=
    currentOverlays->overlays[nv].stacks[i].envelopeCenter[1];
/*
    fitInView(points, &np, nv);
*/
    currentOverlays->overlays[nv].stacks[i].lines[0].np = np;
    currentOverlays->overlays[nv].stacks[i].lines[0].points =
        (float2 *)malloc(sizeof(float2)*np);
    for(k=0; k<np; k++) {
        for(l=0; l<2; l++)
            currentOverlays->overlays[nv].stacks[i].lines[0]
                .points[k][l] = points[k][l];
    }


    currentOverlays->overlays[nv].stacks[i].stripes[0].np = np;
    currentOverlays->overlays[nv].stacks[i].stripes[0].points =
        (float2 *)malloc(sizeof(float2)*np);
    for(k=0; k<np; k++) {
        for(l=0; l<2; l++)
            currentOverlays->overlays[nv].stacks[i].stripes[0]
                .points[k][l] = points[k][l];
    }

    currentOverlays->overlays[nv].stacks[i].envelope.np = np;
    currentOverlays->overlays[nv].stacks[i].envelope.points =
                (float2 *)malloc(sizeof(float2)*np);
    currentOverlays->overlays[nv].stacks[i].handles.np = np;
    currentOverlays->overlays[nv].stacks[i].handles.points =
                (float2 *)malloc(sizeof(float2)*np);

    for(k=0; k<np; k++) {
        for(l=0; l<2; l++) {
            currentOverlays->overlays[nv].stacks[i].envelope.points[k][l]
                    = points[k][l];
            currentOverlays->overlays[nv].stacks[i].handles.points[k][l]
                    = points[k][l];
        }
    }
    currentOverlays->overlays[nv].stacks[i].handles2slices = NULL;
}

int euler2tensorMag(float theta, float psi, float phi, float* orientation)
// magnet to logical rotation (the same as in sis_geometry.c)
{
    int i;

    double sinpsi,cospsi,sinphi,cosphi,sintheta,costheta;

    /*Calculate the core transform matrix*********************/

    cospsi=cos(D2R*psi);
    sinpsi=sin(D2R*psi);

    cosphi=cos(D2R*phi);
    sinphi=sin(D2R*phi);

    costheta=cos(D2R*theta);
    sintheta=sin(D2R*theta);

    orientation[0]=(sinphi*cospsi-cosphi*costheta*sinpsi);
    orientation[1]=(-1.0*sinphi*sinpsi-cosphi*costheta*cospsi);
    orientation[2]=(sintheta*cosphi);

    orientation[3]=(-1.0*cosphi*cospsi-sinphi*costheta*sinpsi);
    orientation[4]=(cosphi*sinpsi-sinphi*costheta*cospsi);
    orientation[5]=(sintheta*sinphi);

    orientation[6]=(sinpsi*sintheta);
    orientation[7]=(cospsi*sintheta);
    orientation[8]=(costheta);

    for (i=0; i<9; i++)
        orientation[i] = trimreal(orientation[i], 1.0e-4);

    RETURN;
}

// called by gplan('getDimPos_m'):$dim1,$dim2,$dim3,$pos1,$pos2,$pos3
// used by fastmap. convert logical position and dimension to magnet position and dimension.
// return values are dimension and position in magnet frame.
/******************/
void getDimPos_m(int tree, int type, float3 dim, float3 pos)
/******************/
{
    int i;
    float theta=0, psi=0, phi=0;
    float orientation[9];
    float3 p[8], pmax, pmin;
    float x, y, z;
    planParams *tag = getCurrPlanParams(type);

    if(tag == NULL) return;
    theta = tag->theta.value;
    psi = tag->psi.value;
    phi = tag->phi.value;
    dim[0] = tag->dim1.value;
    dim[1] = tag->dim2.value;
    dim[2] = tag->dim3.value;
    pos[0] = tag->pos1.value;
    pos[1] = tag->pos2.value;
    pos[2] = tag->pos3.value;

    euler2tensorMag(theta, psi, phi, orientation);

    rotateu2m(pos, orientation);
    // logical frame is different by swapping and inverting x,y 
    y=-pos[0];
    pos[0]=-pos[1];
    pos[1]=y;

    x = 0.5*dim[0];
    y = 0.5*dim[1];
    z = 0.5*dim[2];

    p[0][0] = x;
    p[0][1] = y;
    p[0][2] = z;
    p[1][0] = -x;
    p[1][1] = y;
    p[1][2] = z;
    p[2][0] = -x;
    p[2][1] = -y;
    p[2][2] = z;
    p[3][0] = x;
    p[3][1] = -y;
    p[3][2] = z;
    p[4][0] = x;
    p[4][1] = y;
    p[4][2] = -z;
    p[5][0] = -x;
    p[5][1] = y;
    p[5][2] = -z;
    p[6][0] = -x;
    p[6][1] = -y;
    p[6][2] = -z;
    p[7][0] = x;
    p[7][1] = -y;
    p[7][2] = -z;

    pmin[0] = 10.0*dim[0];
    pmin[1] = 10.0*dim[1];
    pmin[2] = 10.0*dim[2];
    pmax[0] = 0;
    pmax[1] = 0;
    pmax[2] = 0;

    for(i=0; i<8; i++) {
	rotateu2m(p[i], orientation);
        // logical frame is different by swapping and inverting x,y 
	y=-p[i][0];
	p[i][0]=-p[i][1];
	p[i][1]=y;
	if(p[i][0] > pmax[0]) pmax[0] = p[i][0];
	if(p[i][1] > pmax[1]) pmax[1] = p[i][1];
	if(p[i][2] > pmax[2]) pmax[2] = p[i][2];
	if(p[i][0] < pmin[0]) pmin[0] = p[i][0];
	if(p[i][1] < pmin[1]) pmin[1] = p[i][1];
	if(p[i][2] < pmin[2]) pmin[2] = p[i][2];
    }

    dim[0] = pmax[0] - pmin[0];
    dim[1] = pmax[1] - pmin[1];
    dim[2] = pmax[2] - pmin[2];
}


// stack is guaranteed to be filled based on current parameters.
// parameters will be created and filled with default value if needed.
void getStack(iplan_stack* stack, int planType) {

  planParams *tag = getCurrPlanParams(planType);
  int type = getOverlayType(planType);
  if(type == SATBAND) { 
	makeSatBand(type, planType, stack, 
		tag->theta.value, tag->psi.value, tag->phi.value, 
		tag->pos3.value, (tag->thk.value));
  } else if(type == VOLUME) {
	makeVolume(type, planType, stack, 
		tag->theta.value, tag->psi.value, tag->phi.value, 
		tag->dim1.value, tag->dim2.value, tag->dim3.value, 
		tag->pos1.value, tag->pos2.value, tag->pos3.value);
  } else if(type == VOXEL) {
	makeVolume(type, planType, stack, 
		tag->theta.value, tag->psi.value, tag->phi.value, 
		(tag->dim1.value), (tag->dim2.value), (tag->dim3.value), 
		tag->pos1.value, tag->pos2.value, tag->pos3.value);
  } else {
	vInfo paraminfo;
	float *pss, shift, thk;
	int i, ns = 1;
        if(!P_getVarInfo(CURRENT, tag->pss.name, &paraminfo) ) ns = paraminfo.size;	
        pss = (float*) malloc(sizeof(float)*ns);
	thk = (tag->thk.value);
        if(ns%2 == 0) shift = (ns)/2. - 0.5;
	else shift = (ns-1.0)/2.0;
        for(i=0; i<ns; i++) pss[i] = (i-shift)*(thk + tag->gap.value);
	makeStack(type, planType, stack, 
		ns, tag->theta.value, tag->psi.value, tag->phi.value, 
		tag->dim1.value, tag->dim2.value, tag->dim3.value, 
                tag->pos1.value, tag->pos2.value, tag->pos3.value,
		tag->gap.value, thk, pss, 0.0);

	free(pss);
  }
}
