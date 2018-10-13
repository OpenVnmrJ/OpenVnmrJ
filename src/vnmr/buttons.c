/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|
|	buttons.c
|
|	This module contains code to setup buttons functions
|	on both the Sun and graphOn.  It also sets up the mouse
|	cursor on the Graphon.
|
+-----------------------------------------------------------------------*/
#define HELP_NAME_LEN   128

#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "tools.h"
#include "wjunk.h"
#include "allocate.h"

#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	if (Tflag) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3)
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#endif 

#define BUTTONS         0
#define ESC             0x1B
#define MAX_NUM_BUTS    40
#define MOUSE           1
#define OFF             0
#define ON              1

#define	GRAPHON_XCHAR_PIX	13
#define TEK4105_XCHAR_PIX	6
#define TEK4107_XCHAR_PIX	8

typedef int (*PFI)();		/* Pointer to Function returning an Integer */

extern int           active;			/*  Needed for TEK */
extern int           interuption;
extern int           working;
extern jmp_buf       jmpEnvironment;
extern int           Flip();
extern int           Stop();
extern void newMouseHandler(int x, int y, int button, int mask, int dummy,
	int (*mouse_move)(), int (*mouse_but)(), int (*mouse_return)());
extern int  sendTripleEscToMaster(char code, char *string_to_send );
extern void setActiveWin(int x,  int y);
extern void p11_updateDisChanged();

static int getMask(int bid, int but, int click, int move, int modifier);

static int           linelength = 80;  /* num of char for buttons */
static int           Wturnoff_buttonsCalled = OFF;

char                 help_name[HELP_NAME_LEN];
int                  tekCurSor = 1;
int                  curSor  = 0; /* cursor flag initially off */
int                  curSorM = 0; /* mouse cursor flag initially off */
int                  curSorB = 0; /* button cursor flaginitially off */
PFI                  funcs[MAX_NUM_BUTS];
char                 but_label[MAX_NUM_BUTS][32];
static int           num_but;
int                (*mouseMove)()   = NULL;
int                (*mouseButton)() = NULL;
int                (*mouseReturn)() = NULL;
int                (*mouseClick)() = NULL;
int                (*mouseGeneral)() = NULL;

int                (*aip_mouseMove)()   = NULL;
int                (*aip_mouseButton)() = NULL;
int                (*aip_mouseReturn)() = NULL;
int                (*aip_mouseClick)() = NULL;
int                (*aip_mouseGeneral)() = NULL;

static int           mouseIsActive = 0;
int                  nextPanel = 1;
int                  buttons_active = 0;
static char          header[82];	/* Room for 80 characters + null */
static PFI           returnFunc;
static int           isJMouse = 0;
static int           aspMouse = 0;
static unsigned int  JSHIFT = 0;
static unsigned int  JCTRL = 0;
static unsigned int  JMETA = 0;
static unsigned int  JALT = 0;
static unsigned int  JBUTTON1 = 0;
static unsigned int  JBUTTON2 = 0;
static unsigned int  JBUTTON3 = 0;
static unsigned int  JPRESS = 0;
static unsigned int  JRELEASE = 0;
static unsigned int  JMOVE = 0;
static unsigned int  JDRAG = 0;
static unsigned int  JCLICK = 0;
static unsigned int  JEXIT = 0;
static unsigned int  JENTER = 0;
static int  butData = 0;
static int  curButton = 0;
static int buttonsDown = 0;
static int  multiWin = 0;
static int  xoffset = 0;
static int  yoffset = 0;
static int  aip_opened = 0;

struct Lable { char *ptr;		/* pointer to first character */
	       int   start;		/* starting column */
	       int   length;		/* length of label */
	       int   pixelstart;	/* starting pixel */
	       int   pixelend;		/* ending pixel */
	     };

struct Lable         lable[MAX_NUM_BUTS];
static void sendButtonToMaster(int n, char *l);
void enableTekGin();
void disableTekGin();
void setUpFun(char *name, int i);

/*------------------------------------------------------------------------
|
|	cursorOn  cursorOff
|
|	These routines turn on or off the cursor on the graphon
|	Since both activate_mouse and activate_buttons turns on
|	and off the mouse cursor we have a two state system that
|	determines when to turn off or on the cursor.
|
+------------------------------------------------------------------------*/

void
set_win_num(int n)
{
    if (n > 1)
       multiWin = 1;
    else
       multiWin = 0;
}

void
set_win_offset(int x, int y)
{
    xoffset = x;
    yoffset = y;
}


void cursorOn(int who)
{  switch (who)
   { case BUTTONS: curSorB = 1;
		   break;
     case MOUSE:   curSorM = 1;
		   break;
     default:      return;
		   break;
   }
   if (curSor == 0)
   {  curSor = 1;
#ifndef VNMRJ
      if (Wisgraphon())
      {  fprintf(stderr,"%c[1;2;2v",ESC); /* turn on arrow cursor */
	 fprintf(stderr,"%c[2;1;1v",ESC); /* cursor to report on click */
	 Woverlap(); 
      }
      else if (Wistek())
      {
         if (Wistek42xx()) {
             if (tekCurSor) enableTekGin();
         }
         else Gactivate_icon();
      }
#endif
   }
}

void enableTekGin()
{
#ifndef VNMRJ
    if (active != 2) printf( "\033%%!0" );	/*  Require TEK mode */
    defineTekKey( -155, "q" );
    defineTekKey( -157, "w" );
    defineTekKey( -159, "e" );
    printf( "\033IED0" );			/*  Enable GIN */
    if (active != 2) printf( "\033%%!1" );	/*  Back to ANSI mode */
#endif
}

void cursorOff(int who)
{  
   int  wasOn;

   wasOn = (curSorB || curSorM);
   switch (who)
   { case BUTTONS: curSorB = 0;
		   break;
     case MOUSE:   curSorM = 0;
		   break;
     default:      return;
		   break;
   }
   if (!curSorB && !curSorM)
   {  curSor = 0;
#ifndef VNMRJ
      if (Wisgraphon())
	 fprintf(stderr,"%c[1;0;2v",ESC); /* turn off arrow cursor */
      else if (Wistek())
      {
         if ( wasOn )
         {
            if (Wistek42xx()) {
                if (tekCurSor) disableTekGin();
            }
	    else Gdeactive_icon();
         }
      }
#endif
   }
}

void disableTekGin()
{
#ifndef VNMRJ
    if (active != 2) printf( "\033%%!0" );	/*  Require TEK mode */
    printf( "\033IDD0" );			/*  Disable GIN */
    defineTekKey( -155, "" );
    defineTekKey( -157, "" );
    defineTekKey( -159, "" );
    if (active != 2) printf( "\033%%!1" );	/*  Back to ANSI mode */
#endif
}

/*------------------------------------------------------------------------
|
|	Wactivate_mouse 
|
|	This routine sets the address of routines that are called when
|	a mouse button is pushed, a mouse is moved with a button down
|	and the routine that is called when Wturnoff_mouse is called.
|
+------------------------------------------------------------------------*/

int Wisactive_mouse()
{
   return(mouseIsActive);
}

int WisJactive_mouse()
{
   return(isJMouse);
}

void Wactivate_mouse(int (*move)(), int (*button)(), int (*quit)() )
{
   if (mouseReturn) {
       int (*tempFunc)();
       tempFunc = mouseReturn;
       mouseReturn = NULL;  /* Null out return function */
       (*tempFunc)();
   }
   mouseMove   = move;
   mouseButton = button;
   mouseReturn = quit;
   isJMouse = 0;
   TPRINT3("Wactivate_mouse move=0x%x  button=0x%x quit=0x%x\n"
		    ,move,button,quit);
#ifdef VNMRJ
   writelineToVnmrJ("mouse", "on");
#else
   cursorOn(MOUSE); /* turn cursor on */
/* positionCursor(300,300); */
#endif 
   mouseIsActive = 1;
}

int getAspMouse() { return aspMouse;}

void Wturnoff_mouse()
{  int (*tempFunc)();

   aspMouse = 0;
   mouseIsActive = 0;
   mouseMove   = NULL;
   mouseButton = NULL;
   mouseGeneral = NULL;
   mouseClick = NULL;
   isJMouse = 0;
   if (mouseReturn) /* if it exists */
   {  tempFunc = mouseReturn;
      mouseReturn = NULL;  /* Null out return function */
      (*tempFunc)();
   }
#ifdef VNMRJ
   writelineToVnmrJ("mouse", "off");
#else
   cursorOff(MOUSE); /* turn cursor Off */
#endif 
}

/*
void positionCursor(int x, int y)
{  if (Wisgraphon())
      fprintf(stderr,"%c[10;%d;%dv",ESC,x,y);
}
 */

void processMouse(int button, int move, int release, int x, int y)
{
   TPRINT1("processMouse event = %d\n",button);

   if (move) {
       /*  Mouse has moved */
       if (mouseMove)
           (*mouseMove)(x, y, button);
   } else {
       if (release) {
           buttonsDown &= ~(1 << button);
       } else {
           buttonsDown |= 1 << button;
       }
       if (mouseButton)                     /* if button routine defined */
           (*mouseButton)(button, release, x, y);
   }
}

void Jset_aip_mouse_mode(int on)
{
   aip_opened = on;
}

void Jactivate_mouse(int (*drag)(), int (*pressRelease)(), int (*click)(),
                     int (*gen)(), int (*quit)())
{
   if (aip_opened > 0) {
      if (aip_mouseReturn) {
          int (*tempFunc)();
          tempFunc = aip_mouseReturn;
          aip_mouseReturn = NULL;  /* Null out return function */
          (*tempFunc)();
      }
      aip_mouseMove   = drag;
      aip_mouseButton = pressRelease;
      aip_mouseClick = click;
      aip_mouseGeneral = gen;
      aip_mouseReturn = quit;
   }
   else {
      if (mouseReturn) {
          int (*tempFunc)();
          tempFunc = mouseReturn;
          mouseReturn = NULL;  /* Null out return function */
          (*tempFunc)();
      }
      mouseMove   = drag;
      mouseButton = pressRelease;
      mouseClick = click;
      mouseGeneral = gen;
      mouseReturn = quit;
   }
   cursorOn(MOUSE); /* turn cursor on */
   curButton = 0;
   mouseIsActive = 1;
   isJMouse = 1;
   aspMouse = 1;
#ifdef VNMRJ
   writelineToVnmrJ("mouse", "on");
#endif 
}

void Jturnoff_mouse()
{
   int (*tempFunc)();

   if (aip_opened < 1) {
       Wturnoff_mouse();
       return;
   }
   // mouseIsActive = 0;
   aip_mouseMove   = NULL;
   aip_mouseButton = NULL;
   aip_mouseGeneral = NULL;
   aip_mouseClick = NULL;
   isJMouse = 0;
   if (aip_mouseReturn) /* if it exists */
   {  tempFunc = aip_mouseReturn;
      aip_mouseReturn = NULL;  /* Null out return function */
      (*tempFunc)();
   }
#ifdef VNMRJ
   writelineToVnmrJ("mouse", "off");
#endif 
}

void Jturnoff_aspMouse()
{
   aspMouse = 0;
   Jturnoff_mouse();
}

#define BUTTON1   0x100
#define BUTTON2   0x200
#define BUTTON3   0x400

void processJMouse(int argc, char *argv[])
{
    int x, y, but, drag, release, bid, click, modifier;
    int move, combo;
#ifdef OLD
    int k, xmask, bmask;
#endif

    if (argc < 7)
	return;
    but = atoi(argv[2]);
    drag = atoi(argv[3]);
    release = atoi(argv[4]);
    x = atoi(argv[5]);
    y = atoi(argv[6]);
    if (argc > 10) {
        bid = atoi(argv[7]);
        click = atoi(argv[8]);
	modifier = atoi(argv[9]);
	move = atoi(argv[10]);
    }
    else {
        bid = 0;
        click = 0;
	modifier = 0;
	move = 0;
    }
    if (bid == JPRESS) {
        buttonsDown |= 1 << but;
    } else if (bid == JRELEASE) {
        buttonsDown &= ~(1 << but);
    }
#ifdef VNMRJ
    if (click > 1 && but == 0 && multiWin) {
        setActiveWin(x, y); 
        return;
    }
    if(drag)
        writelineToVnmrJ("drag","1");
#endif 
    if (multiWin) {
        x = x - xoffset;
        y = y - yoffset;
    }

   if(bid == JRELEASE) p11_updateDisChanged();

   if (!isJMouse && !aspMouse) {
#ifdef VNMRJ
	  combo = getMask(bid, but, click, move, modifier);
          newMouseHandler(x, y, but, combo, 0, 
		mouseMove, mouseButton, mouseReturn);
/*
	  if (drag) {
              if (mouseMove) {
                (*mouseMove)(x, y, but);
                update_overlay_image(0, 0);
              }
          } 
	  else if (bid == JRELEASE || bid == JPRESS || release) {
              m_noCrosshair();
              if (mouseButton) {
                (*mouseButton)(but, release, x, y);
                update_overlay_image(0, release);
              }
          } else if(move) {
              m_crosshair(but, release, x, y);
          }
*/
#else
	if (drag) {
	    if (mouseMove) {
	    	(*mouseMove)(x, y, but);
	    }
	}
	else if (mouseButton) {
	    if (bid == JRELEASE || bid == JPRESS || release) {
	 	(*mouseButton)(but, release, x, y);
            }
	}
#endif
        return;
   }

#ifdef VNMRJ

#ifdef OLD
   xmask = queryMouse();

   /* the combo data use four bytes to represent mouse event.
    * bit0 to bit7: click number.
    * bit8: button_1, bit9: button_2, bit10: button_3,
    * bit16: button_press, but17: button_release, but18: button_click,
    * bit19: drag. bit20: move.
    * bit24: SHIFT, bit25: CTRL, bit26: ALT, bit27: META
   */
   combo = 0;
   but = curButton;
   if (xmask & ShiftMask)
	combo |= 0x1000000; /* bit 25 */
   if (xmask & ControlMask)
	combo |= 0x2000000; /* bit 26 */
   if (xmask & Mod1Mask) /* ALT */
	combo |= 0x4000000; /* bit 27 */
   if (xmask & Mod4Mask) /* META */
	combo |= 0x8000000; /* bit 28 */
   if (move) {
   	if (bid == JMOVE)
	    butData = 0;
	combo |= 0x100000;  /* bit 20 */
   }
   else {
   	if (bid == JCLICK) {

	    if (click > 255)
	    	click = 255;
	    combo |= click;
	    combo |= 0x40000; /* bit 18 */
	}
	else if (bid == JDRAG) {
	    combo |= 0x80000; /* bit 19 */
	}
	else { /* press or release */
	    bmask = 0;
   	    if (xmask & Button1Mask)
	        bmask = BUTTON1;
   	    if (xmask & Button2Mask)
	        bmask |= BUTTON2;
   	    if (xmask & Button3Mask)
	        bmask |= BUTTON3;
	    k = bmask ^ butData;
	    if (k != 0) {
	      if (k == BUTTON3)
		curButton = 2;
	      else if (k == BUTTON2)
		curButton = 1;
	      else
		curButton = 0;
	    }
	    
   	    if (bid == JPRESS)
		combo |= 0x10000; /* bit 16 */
   	    else if (bid == JRELEASE)
	 	combo |= 0x20000; /* bit 17 */
	    butData = bmask;
	    but = curButton;
	}
	combo |= butData;
   }
#else 
    combo = getMask(bid, but, click, move, modifier);

#endif 
   if (aspMouse && mouseGeneral) {
        (*mouseGeneral)(x, y, but, combo, 0);
	return;
   }
   if (drag)
   {
	if (mouseMove) {
	    (*mouseMove)(x, y, but, combo, 0);
	    return;
	}
   }
   if (bid == JCLICK)
   {
	if (mouseClick) {
            (*mouseClick)(x, y, but, combo, click);
	    return;
	}
   }
   if (bid == JRELEASE || bid == JPRESS)
   {
	if (mouseButton) {
            (*mouseButton)(x, y, but, combo, release);
	    return;
	}
   }
   if (mouseGeneral) {
        (*mouseGeneral)(x, y, but, combo, 0);
	return;
   }
#endif 
}

void process_csi_JMouse(int argc, char *argv[])
{
    int x, y, but, drag, release, bid, click, modifier;
    int move, combo;

    if (aip_opened < 1) {
       processJMouse(argc, argv);
       return;
    }

#ifdef VNMRJ
    if (argc < 7)
        return;
    but = atoi(argv[2]);
    drag = atoi(argv[3]);
    release = atoi(argv[4]); 
    x = atoi(argv[5]);
    y = atoi(argv[6]);
    if (argc > 10) {
        bid = atoi(argv[7]);
        click = atoi(argv[8]);
        modifier = atoi(argv[9]);
        move = atoi(argv[10]); 
    }
    else {
        bid = 0;
        click = 0;
        modifier = 0;
        move = 0;
    }
    if (bid == JPRESS) {
        buttonsDown |= 1 << but;
    } else if (bid == JRELEASE) {
        buttonsDown &= ~(1 << but);
    }
    combo = getMask(bid, but, click, move, modifier);

    if (drag) {
        if (aip_mouseMove) {
            (*aip_mouseMove)(x, y, but, combo, 0);
            return;
        }
    }
    if (bid == JCLICK) {
        if (aip_mouseClick) {
            (*aip_mouseClick)(x, y, but, combo, click);
            return;
        }
    }
    if (bid == JRELEASE || bid == JPRESS) {
        if (aip_mouseButton) {
            (*aip_mouseButton)(x, y, but, combo, release);
            return;
        }
    }
    if (aip_mouseGeneral) { 
        (*aip_mouseGeneral)(x, y, but, combo, 0);
        return;
    }

#endif
}

void set_jmouse_mask(int argc, char *argv[])
{
	int k;

	for (k = 2; k < argc; k++)
	{
	   switch (k) {
	    case 2:
		    JSHIFT = atoi(argv[k]);
		    break;
	    case 3:
		    JCTRL = atoi(argv[k]);
		    break;
	    case 4:
		    JMETA = atoi(argv[k]);
		    break;
	    case 5:
		    JALT = atoi(argv[k]);
		    break;
	    case 6:
		    JBUTTON1 = atoi(argv[k]);
		    break;
	    case 7:
		    JBUTTON2 = atoi(argv[k]);
		    break;
	    case 8:
		    JBUTTON3 = atoi(argv[k]);
		    break;
	   }
	}
}

void set_csi_jmouse_mask(int argc, char *argv[])
{
    set_jmouse_mask(argc, argv);
}

void set_jmouse_mask2(int argc, char *argv[])
{
	int k;

	for (k = 2; k < argc; k++)
	{
	   switch (k) {
	    case 2:
		    JPRESS = atoi(argv[k]);
		    break;
	    case 3:
		    JRELEASE = atoi(argv[k]);
		    break;
	    case 4:
		    JMOVE = atoi(argv[k]);
		    break;
	    case 5:
		    JDRAG = atoi(argv[k]);
		    break;
	    case 6:
		    JCLICK = atoi(argv[k]);
		    break;
	    case 7:
		    JEXIT = atoi(argv[k]);
		    break;
	    case 8:
		    JENTER = atoi(argv[k]);
		    break;
	   }
	}
}

void set_csi_jmouse_mask2(int argc, char *argv[])
{
      set_jmouse_mask2(argc, argv);
}

static void clearFuncs()
{  register int i;

   for (i=0; i<MAX_NUM_BUTS; i++)
      funcs[i] = NULL;
}

static int menu_changed(int num, char *name[])
{
   int i;
   if ( num != num_but )
      return(1);
   if ( !Wissun() )
      return(1);
   for (i=0; i<num; i++)
      if (strcmp(name[i],but_label[i]) )
         return(1);
   return(0);
}
/*-------------------------------------------------------------------------
|	Wactivate_buttons.
|
|	This routine sets up buttons and associated subroutine
|	calls.  It also sets up lables for the graphic terminals.
|	Activate_buttons turns on the mouse cursor (if it is not already on)
|	and will call associated routines if the mouse button is pushed
|	in close proximity to the button labels on the graphon screen
|
/+-----------------------------------------------------------------------*/

void Wactivate_buttons(int number, char *name[], PFI cmd[],
                       PFI returnRoutine, char *menuname)
{
   int    i;

   TPRINT0("Wactivate_buttons: starting\n");
   if (MAX_NUM_BUTS < number)
   {  Werrprintf("Limit of %d buttons please",MAX_NUM_BUTS);
      return;
   }	
   Wturnoff_buttonsCalled = OFF; /* do not clear buttons anymore */
   clearFuncs();  /* null out previous functions */
   /* erase existing buttons */

   linelength = 80;
   header[0] = '\0';

   for (i=0;i<number;i++)
   {
      TPRINT2("Wactivate_buttons:i=%d adding %s\n",i,name[i]);
      funcs[i] = cmd[i];                 /* store function address */
      if (linelength - (int) strlen(name[i]) - 2 < 0)
      {  Werrprintf("adding %s but only %d left, buttons would run off page",
                     name[i],linelength);
	 return;
      }
   }
   if (menu_changed(number,name) )
   {
      if (Wissun())
         sendButtonToMaster( 0, NULL );
#ifndef VNMRJ
      else
         Wclearpos(1,1,1,80); /* clear top status line */
#endif 
      for (i=0;i<number;i++)
         setUpFun(name[i],i);
#ifdef VNMRJ
      sendButtonToMaster( -2, "doneButtons" ); /* button display complete */
#endif 
   }
   returnFunc = returnRoutine;  /* pointer to return function */
#ifndef VNMRJ
   if (Wisgraphon() || Wishds() || Wistek()) /* if we have terminal, print out header */
      Wprintfpos(1,1,1,"%s",header);
#endif 
   /* last argument is the name of a help file. Lets make a copy */
   strncpy(help_name,menuname,HELP_NAME_LEN);
   help_name[HELP_NAME_LEN-1] = '\0';
#ifdef DEBUG
   if (Tflag)
   {  int i;

      for (i=0; i<number; i++)
	 TPRINT2("name=%s  func = 0x%x\n",name[i],funcs[i]);
      TPRINT1("return address is 0x%x\n",returnFunc);
      TPRINT1("help file name is %s\n",help_name);
   }
#endif 
   buttons_active = num_but = number;
#ifndef VNMRJ
   cursorOn(BUTTONS);
#endif 
}

void Wactivate_vj_buttons(int number, PFI returnRoutine)
{

   Wturnoff_buttonsCalled = OFF;
   returnFunc = returnRoutine;
   buttons_active = num_but = number;
}

#ifdef OLDCODE
/*-----------------------------------------------------------------------
|	inVert/1
|
|	This program prints an inverted buttons
|
+-----------------------------------------------------------------------*/
void inVert(int but)
{   char *name;
    int cwindow;

    TPRINT0("inVert:starting\n");
    but--;
    name = (char *)allocateWithId(lable[but].length-1,"inVert");
    strncpy(name,lable[but].ptr,lable[but].length-1);
    name[lable[but].length-2] = '\0';
    cwindow = active;
    if (cwindow != 1) Wsetactive( 1 );
    WinverseVideo();
    Wprintfpos(1,1,lable[but].start,"%s",name);
    WnormalVideo();
    if (cwindow != 1) Wsetactive( cwindow );
    release(name);
}

/*-----------------------------------------------------------------------
|	reInVert/1
|
|	This program restores an inverted buttons
|
+-----------------------------------------------------------------------*/
void reInVert(int butt)
{   char *name;

    butt--;
    name = (char *)allocateWithId(lable[butt].length-1,"reInVert");
    strncpy(name,lable[butt].ptr,lable[butt].length-1);
    name[lable[butt].length-2] = '\0';
    Wprintfpos(1,1,lable[butt].start,"%s",name);
    release(name);
}
#endif


void setUpFun(char *name, int i)
{   char label[128];

    strcpy(label,"x");
    strcat(label,name);
    label[0]= i + '1';
    TPRINT2("setUpFun: setting function key '%s linelenght=%d'\n",
             label,linelength);

   strcpy(but_label[i],name);
   if (Wissun())
     sendButtonToMaster( i+1, label );
#ifdef OLDCODE
   /* store string for display on top of terminal */
   else if (Wisgraphon() || Wishds() || Wistek())
   {  lable[i].ptr = header + strlen(header);/*pointer to label in header */
      lable[i].start = strlen(header)+1;
      lable[i].length = strlen(label) + 2;

      if (Wisgraphon())
      {
          lable[i].pixelstart = lable[i].start * GRAPHON_XCHAR_PIX;
          lable[i].pixelend = lable[i].pixelstart
             + strlen(label+2) * GRAPHON_XCHAR_PIX;
      }
      else if (Wistek4x05())
      {
          lable[i].pixelstart = lable[i].start * TEK4105_XCHAR_PIX;
          lable[i].pixelend = lable[i].pixelstart
             + strlen(label+2) * TEK4105_XCHAR_PIX;
      }
      else if (Wistek4x07())
      {
          lable[i].pixelstart = lable[i].start * TEK4107_XCHAR_PIX;
          lable[i].pixelend = lable[i].pixelstart
             + strlen(label+2) * TEK4107_XCHAR_PIX;
      }

      TPRINT3("lable[%d] start=%d length=%d\n",i,lable[i].start,
		lable[i].length);
      strcat(header,label);
      strcat(header,"  ");	
      linelength = linelength - strlen(label) -2;
   }
#endif 
}

/*---------------------------------------------------------------------------
|	getButtonPushed/2
|
|	This routine determines what panel button was pushed 
|
+---------------------------------------------------------------------------*/

int getButtonPushed(int x, int y)
{  int i;

   for (i=0; i<buttons_active; i++)
   {  if (lable[i].pixelstart <= x && x <= lable[i].pixelend)
      {
	 TPRINT1("getButtonPushed: button %d pushed\n",i+1);
	 return i+1;
      }
   }
   TPRINT0("getButtonPushed: no buttons pushed\n");
   return 0;
}

/*---------------------------------------------------------------------------
|	Buttons_off/0
|
|	This routine clears the buttons if Wturnoff_buttonsCalled flag 
|       has been set
|
+---------------------------------------------------------------------------*/
void Buttons_off()
{
   if (Wturnoff_buttonsCalled)
   {   Wturnoff_buttonsCalled = OFF;
       num_but = 0;
#ifdef VNMRJ
      if (Wissun())
	 sendButtonToMaster( 0, NULL );
#else
      else 
      {  Wclearpos(1,1,1,80); /* clear top status line */
         header[0] = '\0'; 
      } 
#endif 
   }
}

/*--------------------------------------------------------------------------
|
|  Wturnoff_buttons/0
|
|  This routine clears the functions corresponding to buttons, sets
|  various things and executes the button return function if it exists.
|  This routine does not erase the existing buttons since they should
|  be erase by the next buttons that appear.
|
+-------------------------------------------------------------------------*/

void Wturnoff_buttons()
{
   PFI tempFunc;

   TPRINT0("Wturnoff_buttons: beginning\n");
#ifndef VNMRJ
   clearFuncs();
#endif
   linelength = 80;

   /* Mark that Wturnoff_buttons have been called.  After the
       main routine that called Wturnoff_buttons finished, the
       buttons should be cleared if Wactivate_buttons has not been
       called.  The main routine can either be initiated with a 
       button click or by entering a command in the command window.
                                                                 */
   Wturnoff_buttonsCalled = ON;

/******            Do not erase existing buttons here anymore          ********/
/*
 *#ifdef SUN
 *  if (Wissun())
 *  {  for (i=0; i<8 ;i++)
 *     {   panel_set(but[i], PANEL_SHOW_ITEM, FALSE,0);
 *     }
 *  }
 *  else
 *#endif
 *  {  Wclearpos(1,1,1,80);
 *     header[0] = NULL;
 *  }
 *  end of commented out section
 */
   buttons_active = 0;
   help_name[0] = '\0';
   if (returnFunc) /* if it exists */
   {  tempFunc = returnFunc;
      TPRINT1("Wturnoff_buttons:calling return function 0x%x\n",tempFunc);
      returnFunc = NULL;  /* Null out return function */
      (*tempFunc)();
   }
#ifndef VNMRJ
   cursorOff(BUTTONS);
   resetButtonPushed(); /* clear button pushed, so not reinversed video */
#endif
   TPRINT0("\nWturnoff_buttons: ending\n");
}

/*  Only call this routine if on the SUN console!!!  */

#define STDOUT_FD	1

/*  n:  Button number (0 to clear) */
/*  l:	Label (ignored if 0) */
static void sendButtonToMaster(int n, char *l)
{
#ifdef SUN

   if (!Wissun()) return;		/* Get rid of unwanted calls */
   if (n == 0)
     sendTripleEscToMaster( 'b', "0" ); 	/* Erase row of buttons. */
   else
     sendTripleEscToMaster( 'b', l );		/* Send new button */
#else 
   return;
#endif 
}

/**
 * This routine gets which mouse buttons are down.  Similar functionality
 * is in gin(), but that routine does a lot of initialization, and beeps
 * if there is no transformed data.
 *
 * Returns: Bit mapped integer--a set bit means the corresponding mouse
 * button is down.  E.g., return=5 means first and third buttons are
 * down, return=1 means first button is down.  Return 0 if no buttons
 * are down.
 */
int
getMouseButtons(int argc, char *argv[], int retc, char *retv[])
{
    if (retc > 0) {
        retv[0] = intString(buttonsDown);
    } else {
        Winfoprintf("mouseButtons = %d", buttonsDown);
    }
    RETURN;
}

int
getMask(int bid, int but, int click, int move, int modifier)
{ 
   int bmask, k;
   int combo = 0;
   if (modifier & JSHIFT)
	combo |= 0x1000000; /* bit 25 */
   if (modifier & JCTRL)
	combo |= 0x2000000; /* bit 26 */
   if (modifier & JALT) /* ALT */
	combo |= 0x4000000; /* bit 27 */
   if (modifier & JMETA) /* META */
	combo |= 0x8000000; /* bit 28 */
   if (move) {
/*  commented out so modifier key can be used for move
        combo = 0;
*/
   	if (bid == JMOVE)
	    butData = 0;
	combo |= 0x100000;  /* bit 20 */
   }
   else {
   	if (bid == JCLICK) {
	    if (click > 255)
	    	click = 255;
	    combo |= click;
	    combo |= 0x40000; /* bit 18 */
	}
	else if (bid == JDRAG) {
	    combo |= 0x80000; /* bit 19 */
	}
	else { /* press or release */
	    bmask = 0;
   	    if (but == 0)
	        bmask = BUTTON1;
   	    if (but == 1)
	        bmask |= BUTTON2;
   	    if (but == 2)
	        bmask |= BUTTON3;
	    k = bmask ^ butData;
	    if (k != 0) {
	      if (k == BUTTON3)
		curButton = 2;
	      else if (k == BUTTON2)
		curButton = 1;
	      else
		curButton = 0;
	    }
	    
   	    if (bid == JPRESS)
		combo |= 0x10000; /* bit 16 */
   	    else if (bid == JRELEASE)
	 	combo |= 0x20000; /* bit 17 */
	    butData = bmask;
	    but = curButton;
	}
	combo |= butData;
   }
   return combo;
}
