/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPMOUSE_H
#define AIPMOUSE_H

#include <string>
using std::string;

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

class Roi;
class Mouse
{
public:
    // NB: Modes 1 - "lastExternalState" are made visible to the VnmrJ
    // interface, and so have fixed values.
    typedef enum {
        noState = 0,
	select = 1,
	vs = 2,
	createPoint = 3,
	createLine = 4,
	createBox = 5,
	createPolyline = 6,
	createPolygon = 7,
	zoom = 8,
        createOval = 10,
	selectCSIvox = 11,
	userMode = 98,
	imageMath = 100,
        lastExternalState,
        pan,
	modifyPoint,
	modifyLine,
	modifyBox,
        modifyOval,
	modifyPolyline,
	modifyPolygon,
        notOwner,
        previous
    } mouseState_t;

    static int aipSetState(int argc, char *argv[], int retc, char *retv[]);

    static bool creatingPoint();
    static bool creatingLine();
    static mouseState_t setState(mouseState_t newState);
    //static void move(int x, int y, int button);
    // void click(int button, int updown, int x, int y);
    static void reset();
    static void event(int x, int y, int button, int mask, int dummy);
    static void wheelEvent(int clicks, double factor);

private:
    static mouseState_t state;
    static Roi *roi;
    static char userCursor[128];
    static char userMacro[128];
    static int mouseFrameID;
};

#endif /* AIPMOUSE_H */
