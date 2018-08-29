/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPVNMRFUNCS_H
#define AIPVNMRFUNCS_H

#if defined (MOTIF) && (ORIG)
#include <X11/Intrinsic.h>
#else
#include "vjXdef.h"
#endif

#include <string>
using std::string;

#include "vnmrsys.h"            // MAXSTR

/*
extern Pixmap canvasPixmap;
*/

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

typedef void(*MouseMoveFunc)(int x, int y, int button);
typedef void(*MouseButtonFunc)(int button, int updown, int x, int y);
typedef void(*MouseResetFunc)();
typedef void(*DragFunc)(int x, int y, int button, int mask, int dummy);
typedef void(*PressFunc)(int x, int y, int button, int mask, int upflag);
typedef void(*ClickFunc)(int x, int y, int button, int mask, int clicks);
typedef void(*GenFunc)(int x, int y, int button, int mask, int dummy);
typedef void(*QuitFunc)();

extern int interuption;
extern int transparency_level;
extern "C" {
    // Defined in Vnmr
    void Wactivate_mouse(MouseMoveFunc, MouseButtonFunc, MouseResetFunc);
    void Jactivate_mouse(DragFunc, PressFunc, ClickFunc, GenFunc, QuitFunc);
    void Wturnoff_mouse();
    void Jturnoff_mouse();
    void Wturnoff_buttons();
    int Wsetgraphicsdisplay(const char *);
    char *Wgetgraphicsdisplay(char *buf, int buflen);
    //void Werrprintf(const char *fmt, const char *str);
    void Werrprintf(const char *format, ...) __attribute__((format(printf,1,2)));
    int disp_index(int i);
    int getBackingStoreFlag();
    int getBackingWidth();
    int getBackingHeight();
    void setMenuName(const char *buf);
    int P_setreal(int tree, const char *varname, double value, int index);
    int P_setstring(int tree, const char *varname, const char *value,int index);
    int P_getstring(int tree, const char *name, char *buf, int index,
		    int maxbuf);
    int P_getreal(int tree, const char *name, double *value, int index) ;
    int P_getsize(int tree, const char *name, int *val);
    int P_creatvar(int tree, const char *name, int type);
    int P_setdgroup(int tree, const char *name, int value);
    int P_setprot(int tree, const char *name, int value);
    int P_setactive(int tree, const char *name, int activeFlag);
    int P_getactive(int tree, const char *name);
    void clearVar(char *name);
    char *realString(double x);
    char *newString(const char *str);
    int getWin_draw();
    char *sendGrayColorsToVj();
    int isIplanObj(int x, int y, int id);
    void aip_drawCSI3DMesh(int id, int lineColor);
    void set_csi_display(int on);
    int getFistPoint();
    int getNumPoints();
    double getVScale(int fidflag);
    double getYoff();
    int getNtraces();
    int getSpecPoints();
    int getFidPoints();
    float *aip_GetFid(int trace);
    float *aip_GetSpec(int trace);
    float *aip_GetBaseline(int trace);
    float *aip_GetFit(int trace);
    int init2d(int get_rev, int dis_setup);
    void csi_getCSIDims(int *nx, int *ny, int *nz, int *ns);
    int csi_getInd(int iv, int iv2, int iv3, int nv, int nv2, int nv3);
    void csi_getVoxInd(int *iv, int *iv2, int *iv3, int ind, int nv, int nv2, int nv3);
    int is_aip_window_opened();
    int open_color_palette(char *name);
    void set_aip_image_info(int imgId, int order, int mapId, int transparency, char *imgName, int selected);
    void set_transparency_level(double n);
    void utop(int f, float *u);
    void ptou(int f, float *p);
    double aip_getVsFactor();

    // Defined in AIP - C linkage for use by Image Math
    void ib_msgline(const char *str);
    void ib_errmsg(const char *str);
    int interrupt(void);
    void interrupt_end(void);
    void interrupt_begin(void);
    // key is fullpath +' '+ fdf filename, (x, y) is upper left corner of the frame. 
    int drawAnnotation(const char *key, int x, int y, int w, int h, int num, int id, bool f);
    void Winfoprintf(const char *format, ...)  __attribute__((format(printf,1,2)));
    int appdirFind(const char *filename, const char *lib, char *fullpath, const char *suffix, int perm);
    int copy_file(char* origpath, char* destpath);
}

void grabMouse();
void releaseMouse();
bool aipLockScreen();
bool aipLockScreen(bool b);
bool aipHasScreen();
bool setReal(string varname, double value, bool notify);
bool setReal(string varname, int index, double value, bool notify);
bool setString(string varname, string value, bool notify);
double getReal(string varname, double defaultVal);
double getReal(string varname, int index, double defaultVal);
string getString(string varname);
string getString(string varname, string defaultVal);
bool isActive(string varname);
bool setActive(string varname, bool value);
string getCurexpdir();

inline char *retReal(double val) { return realString(val); }
inline char *retString(const char *str) { return newString(str); }
inline bool okToDrawOnCanvas() { return getWin_draw() != 0; }

#endif /* AIPVNMRFUNCS_H */
