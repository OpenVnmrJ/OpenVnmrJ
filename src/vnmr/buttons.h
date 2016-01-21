/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

typedef void (*PFV)();		/* Pointer to Function returning an Integer */

extern void set_win_num(int n);
extern void set_win_offset(int x, int y);
extern void cursorOn(int who);
extern void enableTekGin();
extern void cursorOff(int who);
extern void disableTekGin();
extern int Wisactive_mouse();
extern void Wactivate_mouse(int (*move)(), int (*button)(), int (*quit)() );
extern void Wturnoff_mouse();
extern void positionCursor(int x, int y);
extern void processMouse(int button, int move, int release, int x, int y);
extern void Jactivate_mouse(int (*drag)(), int (*pressRelease)(),
                 int (*click)(), int (*gen)(), int (*quit)());
extern void Jturnoff_mouse();
extern void processJMouse(int argc, char *argv[]);
extern void set_jmouse_mask(int argc, char *argv[]);
extern void set_jmouse_mask2(int argc, char *argv[]);
extern void Wactivate_buttons(int number, char *name[], PFV cmd[],
                       PFV returnRoutine, char *menuname);
extern void inVert(int but);
extern void reInVert(int butt);
extern void setUpFun(char *name, int i);
extern int getButtonPushed(int x, int y);
extern void Buttons_off();
extern void Wturnoff_buttons();
extern int getMouseButtons(int argc, char *argv[], int retc, char *retv[]);
extern int getMask(int bid, int but, int click, int move, int modifier);

enum {                      // Mouse event modifiers
	b1 = 0x100,
	b2 = 0x200,
	b3 = 0x400,
	down = 0x10000,
	up = 0x20000,
	click = 0x40000,
	drag = 0x80000,
	move = 0x100000,
	shift = 0x1000000,
	ctrl = 0x2000000,
	alt = 0x4000000,
	meta = 0x8000000
};

#endif
