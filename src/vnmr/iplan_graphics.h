/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef iplan_graph_header_included
#define iplan_graph_header_included

int startIplan(int type);

void drawOverlays();

int getColor(int type, float theta, float psi, float phi);

int addAstack(int type);
int removeAstack(int ind);
void deleteStack(int ind);
void copy2Dstack(iplan_2Dstack* s1, iplan_2Dstack* s2);
int clearStacks();
int disStripes();
int disCenterLines();
void getAstack(iplan_stack* stack, int type,
        int numSlices, float factor);
void get2pointStack(iplan_stack* stack, int type,
        int numSlices, float factor);
void get3pointStack(iplan_stack* stack, int type,
        int numSlices, float factor);
void mark2xyz(iplan_mark m, float3 p);
void getThirdPoint(iplan_mark m, float3 p);
void sPoint2View(int view, float2 p2, float z, float3 p3);
void sPoint2Stack(int view, int stack, float2 p2, float z, float3 p3);
int getMark(iplan_mark* m);
void updateSlice(float2 p, float2 prevP);
void updateRotation(float2 p, float2 prevP, int rCenterOk, float rCenterX, float rCenterY);
void updateTranslation(int stack, float2 p, float2 prevP);
void updateTranslationAll(float2 p, float2 prevP);
void updateSExpansion(float2 p, float2 prevP);
void updateExpansion(float2 p, float2 prevP);
void projection(iplan_mark m1, iplan_mark m2, float3 p);
void points2euler(iplan_mark m1, iplan_mark m2, float3 p);

void free2Dstack(iplan_2Dstack* s);
void drawHandles(int flag);

void getLocation(float2 p, int* view, int* stack, int* slice,
        int* handle, int* handle1, int* handle2, int* mark);
void getLocation2(float2 p, int* view, int* stack, int* slice);

int getCrosssectionCenter(float2 cCenter, int activeView, int activeStack);
void getStackCenter(float3 cCenter, int activeView, int activeStack);
float calAngle(float2 cCenter, float2 prevP, float2 p);

void updatePss(int act, int first, int last);
int updateZforZ(float lpe2, int act);
void updateZforGap(float gap, int act);
int updateZforNs(int ns, int act, int gapFixed);
void updateZforThk(float thk, int act);

int setValue(char* paramName, float value, int ind);
int sendValueToVnmr(char* paramName, float value);
void sendValues(int act);
void updatePrescription(int act);
int gplan(int argc, char *argv[], int retc,char *retv[]);
void setGapMode(int mode);
void drawFrames(int flag);
void drawInterSection(int flag, iplan_2Dstack *overlay, int j, int view);
void setDrawInterSection(int mode);
void setDraw3D(int mode);
void setDrawAxes(int mode);
void drawActiveSlice(int flag, int nv);
void deleteSlice();
void restoreStack();
void setFillPolygon(int mode);

void initMarks();
void drawMarks(int flag, int i);
void drawMark(int flag, float2 mark, int isFixed);
int getNumOfMarks(int view, int stack);
int getNewMark(int n);
int countfixedMarksForStack(int stack);
int countFixedMarksForSlice(int slice);
int getFixedMarksForStack(int stack, int i);
int getRotationCenter(float2 center, int view, int stack);
void deleteSelected();
void drawSliceOrders(int flag, int j, int nv);
void drawStackOrders(int flag, int j, int nv);
void setDrawOrders(int mode);
void setOrderZero();
void updateOrders();
int sendPssByOrder(int act);
int sendByOrder(char* paramName);
void orderValues(int* orders, float* values, int n);
void sendParamsByOrder(int type);
int sendIntArrayValues(char* paramName, int* values, int n);
int sendRealArrayValues(char* paramName, float* values, int n);
void updateSliceOrders(int stack, int slice);
void updateStackOrders(int stack);
int overlaySize(iplan_2Dstack *overlay);
void rotateVZ(float3 cor, float angle);
void makeDragPerpendicularToEdge(float2 p, float2 prevP, float2 v1, float2 v2);
void makeHandle2slice();
void sendPnew(char* str);
void getCurrentIBViews(int len);
void getIBviews(int len, int* ids);
void getIBview(iplan_view* view, int id);
void initDraw(int mode);
int countMarksForSlice(int slice, int ind[3]);
void setDisplayStyle(int mode);
int* getSliceIndexByOrder(int stack);
int* getStackIndexByOrder();
int writeParamInProcparByOrder(FILE* fp, char* paramName);
int sendStacks(FILE* fp, int type);
int sendSatBands(FILE* fp);
int sendVoxels(FILE* fp);
int writeParamInProcpar(FILE* fp, char* paramName, float* values, int n);
int sendParallelValues(int act, char* paramName);
int updateOverlays();
int imagesChanged();
void alternateSlices(int mode, int act);
void endIplan();
void refreshImages(int mode);
int eraseOverlays();
int majorOrient(float theta, float psi, float phi);
void showOverlays(int mode);
void reDrawWindow(int id);
void drawOverlayForView(int flag, int j, int nv);
void updateType();
void setOrientValue(char* paramName, char* str, int iplan);
void updateArray(char* arraystr, int type);
void setAxisAndType(int type);
int isIplanObj(int x, int y0, int view);
void sendPnew0(int i, const char *str);
int overlaySize_max(iplan_2Dstack *overlay);
int getNumOfNewMarks();
void updateDefaultType();
int countFixedMarksForStack(int stack);
int orderPssValuesForStack(int act, float* values);
int getFixedSlice();
int getActStackInfo(char* key);
int getIplanType();
int getCubeFOV();
int majorOrient(float theta, float psi, float phi);

extern void createPlanParams(int type);
extern int brkPath(char* path, char* root, char* name); 

extern void set_color_levels(int);
extern void set_color_intensity(int level);
extern void drawCross(int flag, int x, int y, unsigned int size,
        unsigned int line_width, char* style);
extern void drawCircle(int flag, int x, int y, unsigned int size,
        int angle1, int angle2, unsigned int line_width,  char* style);
extern void popBackingStore();
extern void toggleBackingStore(int onoff);
extern void drawString(int flag, int x, int y, int size, char* text, int length);
extern void drawLine(int flag, int x1, int y1, int x2, int y2,
        unsigned int line_width, char* style);
extern void fillPolygon(int flag, float2* pts, int num, char* s, char* m);
extern void copy_from_pixmap2(int x, int y, int x2, int y2, int w, int h);
extern int getWin_draw();
// extern void copy_from_pixmap(int x, int y, int x2, int y2, int w, int h);
extern void copy_aipmap_to_window(int x, int y, int x2, int y2, int w, int h);
extern void aip_clear_pixmap(int x, int y, int w, int h);
extern void copy_to_pixmap2(int x, int y, int x2, int y2, int w, int h);
extern void drawRectangle(int flag, int x, int y, unsigned int width, unsigned int hight,
        unsigned int line_width, char* style);
extern void aip_setRegion(int x, int y, int wd, int ht);
#endif
