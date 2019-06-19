/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "vnmrsys.h"
#include "graphics.h"
#include "group.h"
#include "pvars.h"
#include "buttons.h"
#include "wjunk.h"
extern void insetFrame(int x0, int y0, int x1, int y1);
extern void zoomFrame(int butpressx, int butpressy, int x, int y);
extern void finishZoomFrame(int butpressx, int butpressy, int x, int y);
extern void finishInsetFrame(int butpressx, int butpressy, int x, int y);
extern void spwpFrame(int butpressx, int butpressy, int x, int y);
extern void centerPeak(int x, int y, int but);
extern void zoomCenterPeak(int x, int y, int but);
extern void m_noCrosshair();
extern void m_crosshair(int but, int release, int x, int y);
extern void update_overlay_image(int cleanWindow, int mouseRelease);
extern int appdirFind(char *filename, char *lib, char *fullpath,
                      char *suffix, int perm);

static char buttonMacro[MAXSTR]; 

int  butpressx = 0;
int  butpressy = 0;


void newMouseHandler(int x, int y, int but, int mask, int dummy,
    int (*mouse_move)(), int (*mouse_but)(), int (*mouse_return)());
void newMouseReset();
void userEvent(int x, int y, int mask, int mouseFrameID, char *userMacro);

int
getButtonMode() {
     double d;
     if(P_getreal(GLOBAL, "buttonMode", &d, 1) != 0) return MODELESS;
     else return (int)d;
}

void setButtonMode(int mode) {
    char cmd[64];
    if(mode != USER_MODE) strcpy(buttonMacro,"");
    sprintf(cmd,"setButtonMode(%d)\n",mode); 
    execString(cmd);
}

int setButtonState(int argc, char *argv[], int retc, char *retv[]) {
   
   int mode = 0;
   if(argc>1) mode = atoi(argv[1]);
   setButtonMode(mode);
   
   if(mode == USER_MODE && argc>2) {
      if(appdirFind(argv[2], "maclib", NULL, "", R_OK) ) strcpy(buttonMacro,argv[2]);
      else strcpy(buttonMacro,"");
   }
   RETURN;
}

static int mouseMaskVal;

int mouseMask()
{
   return(mouseMaskVal);
}

void newMouseHandler(int x, int y, int but, int mask, int dummy,
		int(*mouse_move)(), int(*mouse_but)(), int(*mouse_return)()) {
	double d;
	int mode;
        static int button2 = -1;
        static int button3 = -1;
	int release = 0;
	int mouseProcessed = 0;
	static char gmode[256]="";
	Wgetgraphicsdisplay(gmode, 20);
	int stackplot=0;
	if (strlen(gmode)>2 && (strncmp(gmode, "df", 2) == 0 || strncmp(gmode, "ds", 2) == 0))
		stackplot=1;

        if (button3 == -1)
        {
	   if (P_getreal(GLOBAL, "useButton3", &d, 1) != 0)
		button3 = 1;
	   else
		button3 = (int) d;
        }

        if (button2 == -1)
        {
	   if (P_getreal(GLOBAL, "useButton2", &d, 1) != 0)
		button2 = 1;
	   else
		button2 = (int) d;
        }

        mouseMaskVal = mask;
	mode = getButtonMode();
	/*
	 note: it is important that the following three "if statements" are in order
	 since they may modify mode and but.
	 */
	//ignore middle mouse button if button2 == 0
	if (but == 1 && !button2)
		return;

	//right mouse button is allowed only if mode == MODELESS and button3 == 1
	if (but == 2 && !(button3))
		return;

	//convert shift+but1 or ctrl+but1 to button3 if mode == MODELESS
	if (but == 0 && ((mask & shift) || (mask & ctrl)) && (mode == MODELESS
			&& !button3))
		but = 2;

	//all but2 event is MODELESS
	if (but == 1)
		mode = MODELESS;
	if(stackplot && (mode != ZOOM_MODE && mode != PAN_MODE))
		mode = MODELESS;

	// mode independent
	switch (mask) {
	case b1 + down:
	case b2 + down:
	case b3 + down:
		m_noCrosshair();
		butpressx = x;
		butpressy = y;
		break;
	case b1 + up:
	case b2 + up:
	case b3 + up:
		release = 1;
		break;
	case move:
		m_crosshair(but, release, x, y);
		break;
	}

	/* handle new mode */
	mouseProcessed = 0;
	switch (mode) {
	case USER_MODE:
		mouseProcessed = 1;
		userEvent(x,y,mask, 1, buttonMacro);
		break;

	case SELECT_MODE:
		mouseProcessed = 0;
		break;
	case ZOOM_MODE:
		mouseProcessed = 1;
		switch (mask) {
		case b1 + click + 2:
		case b3 + click + 2:
			zoomCenterPeak(x, y, but + 1);
			break;
		case b1 + down:
			//m_noCrosshair();
			zoomFrame(butpressx, butpressy, x, y);
			break;
		case b1 + up:
			break;
		}
		break;
	case INSET_MODE:
		mouseProcessed = 1;
		switch (mask) {
		case b1 + down:
			//m_noCrosshair();
			writelineToVnmrJ("vnmrjcmd", "canvas rubberband");
			insetFrame(butpressx, butpressy, x, y);
		
		case b1 + up:
			break;
		}
		break;
	case PAN_MODE:
		mouseProcessed = 1;
		switch (mask) {
		case b1 + click + 2:
		case b3 + click + 2:
			centerPeak(x, y, but + 1);
			break;
		case b1 + down:
		case b3 + down:
		case b1 + shift + down:
		case b1 + ctrl + down: 
			//m_noCrosshair();
			spwpFrame(4, x, y, mask);
			break;
		case b1 + drag:
		case b3 + drag:
		case b1 + shift + drag:
		case b1 + ctrl + drag:
			spwpFrame(but + 1, x, y, mask);
			break;
		case b1 + up:
		case b3 + up:
			break;
		}
		break;
	case VS_MODE:
		but = 1;
		break;
	case STRETCH_MODE:
		mouseProcessed = 1;
		switch (mask) {
		case b1 + down:
			//m_noCrosshair();
			spwpFrame(4, x, y, 0);
			break;
		case b1 + drag:
			spwpFrame(3, x, y, 0);
			break;
		case b1 + up:
			break;
		}
		break;
	case MODELESS:
	default:
		break;
	}

	// call old mouse functions
	if (!mouseProcessed) {
		switch (mask) {
		case b1 + drag:
		case b1 + shift + drag:
		case b1 + ctrl + drag:
		case b2 + drag:
		case b3 + drag:
		case b3 + shift + drag:
		case b3 + ctrl + drag:
			if (mouse_move) {
				(*mouse_move)(x, y, but);
				update_overlay_image(0, 0);
			}
			break;
		case b1 + down:
		case b1 + shift + down:
		case b1 + ctrl + down:
		case b1 + up:
		case b1 + shift + up:
		case b1 + ctrl + up:
		case b2 + down:
		case b2 + up:
		case b3 + down:
		case b3 + shift + down:
		case b3 + ctrl + down:
		case b3 + up:
		case b3 + shift + up:
		case b3 + ctrl + up:
			//m_noCrosshair();
			if (mouse_but) {
				(*mouse_but)(but, release, x, y);
				update_overlay_image(0, release);
			}
			break;
		}
	}
}

void
newMouseReset()
{
}

void userEvent(int x, int y, int mask, int mouseFrameID, char *userMacro) {
	char cmd[MAXSTR];
        if(strlen(userMacro) < 1) return;
 
        strcpy(cmd,"");
        switch (mask) { // only set some of the event
        case b1+drag:
          sprintf(cmd,"%s('b1+drag',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+ctrl+shift+click+1:
          sprintf(cmd,"%s('b1+ctrl+shift+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+shift+click+1:
          sprintf(cmd,"%s('b1+shift+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+ctrl+click+1:
          sprintf(cmd,"%s('b1+ctrl+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+click+1:
          sprintf(cmd,"%s('b1+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b2+click+1:
          sprintf(cmd,"%s('b2+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b3+click+1:
          sprintf(cmd,"%s('b3+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+down:
          sprintf(cmd,"%s('b1+down',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b2+down:
          sprintf(cmd,"%s('b2+down',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b3+down:
          sprintf(cmd,"%s('b3+down',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b1+up:
          sprintf(cmd,"%s('b1+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b2+up:
          sprintf(cmd,"%s('b2+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        case b3+up:
          sprintf(cmd,"%s('b3+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
             break;
        }
        if(strlen(cmd)>0) execString(cmd);
}

int userWheelEvent(int clicks) {
     char cmd[MAXSTR];
     if(getButtonMode() != USER_MODE) return 0; 
     if(strlen(buttonMacro) < 1) return 1;

     sprintf(cmd,"%s('wheel',%d,%d)\n",buttonMacro,clicks,1);
     execString(cmd);
     return 1;
}
