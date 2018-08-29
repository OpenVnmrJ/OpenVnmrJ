/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPMOUSE_H
#define ASPMOUSE_H

#include <string>
#include "AspFrame.h"
#include "AspRegion.h"

using std::string;

/*
enum {			// Mouse event modifiers
	b1 = 0x100,
	b2 = 0x200,
	b3 = 0x400,
	down = 0x10000,
	up = 0x20000,
	click = 0x40000,
	drag = 0x80000,
	mmove = 0x100000,
	shift = 0x1000000,
	ctrl = 0x2000000,
	alt = 0x4000000,
	meta = 0x8000000
};
*/

class AspMouse
{
public:
    // NB: Modes 1 - "lastExternalState" are made visible to the VnmrJ
    // interface, and so have fixed values.
    typedef enum {
        noState = 0,
	select = 1,
	vs = 2,
	createBand = 3,
	modifyBand = 4,
	createBox = 5,
	modifyBox = 6,
	cursor1 = 7,
	cursor2 = 8,
	zoom = 9,
        pan = 10,
        array = 11,
        traceF1 = 12,
        traceF2 = 13,
        traceF1F2 = 14,
        phasing = 15,
	createPeak = 16,
	modifyPeak,
	createPoint = 18,
	modifyPoint,
	createLine = 20,
	modifyLine,
	createArrow = 22,
	modifyArrow,
	createOval = 24,
	modifyOval,
	createPolygon = 26,
	modifyPolygon,
	createText = 28,
	modifyText,
	createXBar = 30,
	modifyXBar,
	createYBar = 32,
	modifyYBar,
	createInteg = 34,
	modifyInteg,
	createPolyline = 36,
	modifyPolyline,
	createRegion = 38,
	modifyRegion,
	userMode = 98,
        lastExternalState,
    } mouseState_t;

    static int aspSetState(int argc, char *argv[], int retc, char *retv[]);

    static mouseState_t setState(mouseState_t newState);
    static void reset();
    static void event(int x, int y, int button, int mask, int dummy);
    static void wheelEvent(int clicks, double factor);

    static mouseState_t getState() { return state;}
    static void grabMouse();
    static void releaseMouse();
    static int prevX, prevY;
    static int numCursors;
    static bool creating;

    static void startDrag(spAspFrame_t frame, int x, int y);
    static void endDrag(spAspFrame_t frame, int x,int y,int w,int h);

private:
    static mouseState_t state;
    static mouseState_t origState;
    static mouseState_t restoreState;
    static char userCursor[128];
    static char userMacro[128];
    static spAspRoi_t roi;
    static AspAnno *anno;
    static spAspPeak_t peak;
    static spAspInteg_t integ;
    static spAspRegion_t region;
    static spAspTrace_t trace;
};

#endif /* ASPMOUSE_H */
