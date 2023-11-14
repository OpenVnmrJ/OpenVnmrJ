/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------
|						|
|    iplan.h   - defines image planning params	|
|						|
+----------------------------------------------*/

#ifndef iplan_header_included
#define iplan_header_included

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "vnmrsys.h"
#include "group.h"
#include "variables.h"

#include "data.h"
#include "graphics.h"
#include "init2d.h"
//#include "wjunk.h"

#define R2D 180.0/3.14159265358979323846
#define D2R 3.14159265358979323846/180.0

#define D0    0
#define D30    30
#define D45    45
#define D90    90
#define D180    180
#define D270    270
#define D360    360
#define D361    361

#define BOX    0
#define CENTER    1

#define SQUARE    0
#define CIRCLE    1
#define RADIALCIRCLE    2

#define REGULAR   0
#define RADIAL    1
#define VOLUME    2
#define VOXEL     3
#define SATBAND   4
#define CSI2D     5
#define CSIVOXEL  6
#define VOXCSI2D  6
#define CSI3D     7
#define VOXCSI3D  8
#define CSIRAD    9
#define NONCSI    0
#define ALLSTACKS    99

#define OVERLAY 0
#define NEWOVERLAY 1
#define CLEAR 2

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef float float2[2];
typedef float float3[3];

typedef struct {
  float3 p1,p2;
} planLine;

typedef struct {
/* slice contains the coordinates of the slice box and the */
/* coordinates of the center cross section (slice with thk = 0) */

    float3 box[8];
    float3 center[4];

} iplan_box;

typedef struct {

    int np;
    float3* points;

} iplan_3Dpolygon;

typedef struct {
  char name[64];
  char strValue[64];
  double value;
  int use;
} planParam;

typedef struct {
  int overlayType;
  int planType;
  int planColor;
  char planName[64];
  planParam orient, theta, psi, phi, dim1, dim2, dim3,
        pos1, pos2, pos3, pss, thk, gap, ns;
} planParams;

typedef struct {

    char name[MAXSTR];

    int   ns;
    int type;
    int planType;
    int color;
    int mplane;  /* default 1 plane, could be 3-plane */

    int   *order;
    float2* pss;
    float2* radialAngles;

    float theta,
	  psi,
	  phi,
	  thk,
	  lro,
	  lpe,
	  lpe2,
	  pro,
	  ppe,
	  radialShift,
	  pss0,
	  gap;

    int coils;
    int coil;
    float origin[3];

    /* orientation is m2u */
    float orientation[9];
    float u2m[3][4];
    float m2u[3][4];

    iplan_box *slices;

    iplan_3Dpolygon envelope;

} iplan_stack; 

typedef struct {

    int numOfStacks;

    int* order;
    iplan_stack* stacks;

} prescription; 

typedef struct {

    int np;
    float2* points;

} iplan_polygon;

typedef struct {

    int numOfStripes;
    iplan_stack * stack;
    iplan_polygon* stripes;
    iplan_polygon* lines;
 
    iplan_polygon envelope;
    iplan_polygon handles;
    float2 handleCenter;
    float2 envelopeCenter;

    int* handles2slices;

} iplan_2Dstack;

typedef struct {

    int numOfStacks;

    iplan_2Dstack* stacks;

} iplan_overlay;

typedef struct {

    int numOfOverlays;

    iplan_overlay* overlays;

} iplan_overlays;

typedef struct {

    iplan_stack slice;
    /* other display variables, such as position, size, scale etc... */

    float m2p[3][4];
    float p2m[3][4];

    int hasFrame;
    int hasImage;
    int coils;
    int coil;
    float origin[3];
    char axis[MAXSTR];
    int pixstx, pixsty;
    int pixwd, pixht;
    int framestx, framesty;
    int framewd, frameht;

    float pixelsPerCm;

} iplan_view;

typedef struct {

   int numOfViews;
   int* ids;

   iplan_view* views;
} iplan_views;

typedef struct {
    int view;
    int stack;
    int slice;
    int isFixed;
    int isUsing;
    float2 point;

    float2 homeLocation;
    float2 currentLocation;
} iplan_mark;

/*extern const double pi = acos(double(-1.0));
*/

extern int meshColor;
extern int onPlaneColor;
extern int offPlaneColor;
extern int highlight;
extern int axisColor;
extern int markColor;
extern int frameColor;
extern int handleColor;

extern iplan_views* currentViews;
extern iplan_views* prevViews;

extern prescription* activeStacks;
extern prescription* currentStacks;
extern prescription* prevStacks;

extern iplan_overlays* currentOverlays;
extern iplan_overlays* prevOverlays;

extern iplan_mark* marks;
extern int numOfMarks;
extern int isMarking;
extern int activeStack;
extern int activeView;
extern int activeHandle;
extern int handleMode;

extern int compressMode;
extern int circlePoints;
extern int defaultType;
extern int numDefaultSlices;
extern float defaultSize;
extern float defaultVoxSize;
extern float defaultThk;
extern const float pi;
extern int defaultStyle;
extern int drawMode;
extern int pen;
extern int frameColor;
extern int markColor;
extern int satBandColor;
extern int highlight;
extern int axisColor;
extern unsigned int handleSize;
extern float defaultGap;
extern int gapFixed;
extern FILE *tfile;
extern int markSize;

//extern const float eps;
extern const float pixeps;
extern const float pi;

extern int fillIntersection;
extern int drawIntersection;
extern int draw3DStack;
extern int drawStackAxes;
extern int overlayStyle;
extern int alternateOrders;
extern int showStackOrders;

void initIplan();
void initViews(iplan_views* v);
void freeViews(iplan_views* v);
void initPrescript(prescription* p);
void freePrescript(prescription* p);
void initOverlays(iplan_overlays* o);
void freeOverlays(iplan_overlays* o);

void displayStack(iplan_stack s);
int makeActiveStacks(int tree);
int saveMilestoneStacks();
int getCurrentStacks();
int getActiveStacks();
int getMilestoneStacks();
int copyStack(iplan_stack* s1, iplan_stack* s2);
int copyPrescript(prescription* p1, prescription* p2);
void multiplyAB(float* a, float* b, float* c, int row, int col);
void multiplyAtB(float* a, float* b, float* c, int row, int col);
void multiplyABt(float* a, float* b, float* c, int row, int col);
void multiplyAtBA(float* a, float* b, float* c, int size);
void rotateZ(float* a, float* b, float angle);
void rotateX(float* a, float* b, float angle);
void rotateY(float* a, float* b, float angle);
void translateXY(float* a, float* b, float x, float y);
void rotateXYZ(float* a, float* b, float angle, float x, float y);
int euler2tensor(float theta, float psi, float phi, float* orientation);
int tensor2euler(float* theta, float* psi, float* phi, float* orientation);
int euler2tensorView(float theta, float psi, float phi, float* orientation);
int tensor2eulerView(float* theta, float* psi, float* phi, float* orientation);
int euler2tensorView_3D(float theta, float psi, float phi, float* orientation);

void calSliceXYZ(iplan_stack* s);
void rotatem2u(float3 cor, float* d);
void rotateu2m(float3 cor, float* d);
void rotateAngleAboutX(float3 cor, float angle);
void rotateAngleAboutY(float3 cor, float angle);
float getRadialAngle(float th1, float psi1, float phi1,
                        float th2, float psi2, float phi2);

void copyOverlays(iplan_overlays* o1, iplan_overlays* o2);
void calOverlays();
void translatem2u(float3 cor, float3 o);
void translateu2m(float3 cor, float3 o);
float distance2(float2 c1, float2 c2);
float distance3(float3 c1, float3 c2);
int calIntersection(float3 n, float3 a, float3 b, float3 c);
void orderConvexPolygon(float2* points, int* n);
void orderConvexPolygon2(float2* points, int* n);
void displayOverlay(iplan_overlay o);

int makeAstack(iplan_stack* stack, float* orientation, float theta, float psi, float phi, 
	int type, int numSlices, float lpe, float lro, float lpe2, float thk, float3 c);
void make1pointStack_default(iplan_stack* stack,
        int type, int ns, float lpe, float lro, float lpes, float thk, float3 c);
void make1pointStack(iplan_stack* stack,
        int type, int ns, float lpe, float lro, float lpes, float thk, float3 c);
void make2pointStack(iplan_stack* stack, int type, int ns,
        float lpe, float lro, float lpe2, 
	float thk, float angle, float3 c, float size);
void make3pointStack(iplan_stack* stack, int type, int numSlices, 
	float lpe, float lro, float lpe2, 
	float thk, float2 p1, float2 p2, float2 p3, float3 c, float size);

void calEuler(float3 p1, float3 p2, float3 p3, float* theta, float* psi, float* phi);

float containedInConvexPolygon(float2 point, float2* boundary, int np);
int containedInRectangle(float2 point, int stx, int sty, int wd, int ht);
int containedInCircle(float2 point, float2 center, float r);
float calPloygonArea(float2* polygon, int np);
void calEnvelopeOverlay(int view, int stack);
void calRadialEnvelopeOverlay(int nv, int i, int nb);
void transposeM(float* f, int row, int col);
void calSatBandOverlays(int view, int stack);
void calStackOverlays(int view, int stack);
void fitInView(float2* points, int* np, int view);
int trimLine(int* x1, int* y1, int* x2, int* y2, 
        int xmin, int ymin, int xmax, int ymax);

void makeSatBand(int type, int planType, iplan_stack* stack, float stheta, float spsi, float sphi,
        float satpos, float satthk);
void makeVolume(int type, int planType, iplan_stack* stack, float vtheta, float vpsi, float vphi,
        float vox1, float vox2, float vox3, float pos1, float pos2, float pos3);
void makeStack(int type, int planType, iplan_stack* stack, int ns, float theta, float psi, float phi,
        float lpe, float lro, float lpe2, float ppe, float pro, float pss0,
	float gap, float thk, float* pss, float radialShift);
void orderPss(iplan_stack* s);
int makeStackByEuler(float theta, float psi, float phi, int type);
int makeAstackByEuler(iplan_stack* stack, int type, int ns, 
	float lpe, float lro, float lpe2,
        float thk, float3 c, float theta, float psi, float phi);


void calSliceXYZ2(iplan_stack* s);
void calTransMtx(iplan_stack* stack);
void transform(float m[3][4], float c[3]);
void calOverlay(int view, int stack);
void calOverlayForAllViews(int stack);
void calOverlayForAview(int stack, int view);
void freeAStack(iplan_stack* s);
void sortSingleSlices(int type, int planType, prescription* p, int* k, float* theta1, float* psi1, 
	float* phi1, float* thk1, float* lro1, float* lpe1, float* pro1, 
	float* ppe1, float* pss01, float* pss1, float* radialAngles1, int ss, 
	float* radialShift1);

int getCompressMode();
void resetActive();
void getDefaults(int tree, int type,
        int* nslices, float* lpe, float* lro, float* lpe2, float* thk);
void alternateSliceOrders(int* orders, int ns);
void makeOverlay(iplan_stack *stack, iplan_view *view, iplan_2Dstack *stack2);
void calEnvelope(iplan_stack *stack, iplan_view *view, iplan_2Dstack *stack2);
int make3orthogonal(int type);
void createCommonPlanParams();
void readPlanPars(char *path);
void snapAngle(float* a);
void savePrescript(char* path);
int getTypeByTagName(char *planName);
void printCurrTag();
void setPlanValue(char* paramName, char *value);
void getDimPos_m(int tree, int type, float3 dim, float3 pos);
int getCompressMode();
int getTypeByPlanType(int planType);
char *getTagNameByType(int type);
void getPlanParamNames(int planType, char *parlist);
void getNewPos(float3 oldEuler, float3 oldPos, float3 newEuler, float3 newPos);
void getNewDim(float3 oldEuler, float3 oldDim, float3 newEuler, float3 newDim);
int is3Plane(prescription* plans, int planType);
void doDependency(int stack, float3 dp);
void orthDependency(int stack);
void CSIDependency(int stack);
void cubeDependency(int stack, float3 dp);
void setUseParam(int planType, char *paramName, int mode);
int getUseParam(int planType, char *paramName);
void getStack(iplan_stack* stack, int planType);
int calcBoxIntersection(iplan_view *view, float3 *box, float2 *points);
planParams *getCurrPlanParams(int planType);

extern void  clearVar(const char *name);
extern int appdirFind(const char *filename, const char *lib, char *fullpath, const char *suffix,
               int perm);

#endif
