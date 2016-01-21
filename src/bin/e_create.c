/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/
/************************************************************************/
/* e_create.c contains the create routine for 'enter'			*/
/************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <suntool/sunview.h>
#include <suntool/tty.h>
#include <suntool/text.h>
#include <suntool/panel.h>
#include <sunwindow/notify.h>
#include <suntool/selection_attributes.h>

#define	LOADING		1

extern Frame		frame;
extern Icon		asm_icon;
extern Panel		active_panel;
extern Panel		choice_panel;
extern Panel		header_panel;
extern Panel		enter_panel;
extern Panel		menu_panel;
extern Panel		Tray_panel;
extern Panel_item	sample_item;
extern Panel_item	user_item;
extern Panel_item	macro_item;
extern Panel_item	solvent_item;
extern Panel_item	text_item;
extern Panel_item	Auto_Number;
extern Panel_item	Auto_User;
extern Panel_item	Submitbutton;
extern Panel_item	Deletebutton;
extern Panel_item	Editbutton;
extern Panel_item	Activemsg;
extern Panel_item	Errormsg;
extern Panel_item	Loadbutton;
extern Panel_item	Locatebutton;
extern Panel_item	Printbutton;
extern Panel_item	Savebutton;
extern Panel_item	Tray[4];
extern Panel_item	but[8];
extern Panel_item	Fln_req;
extern Panel_item	Quitbutton;
extern Textsw		enter_screen;

extern char	buffer[1024];
extern char	*default_user;
extern char	filename[80];
extern char	*vnmrsys;
extern int	active_sample_no;
extern int	arg_file;
extern int	arg_opt;
extern int	clear_tray;
extern int	load_or_save;
extern int	offonN;
extern int	offonU;
extern int	Qflag;

static short	icon_image[] = {
#include	"asm.icon"
};
mpr_static(icon_pixrect, 64, 64, 1, icon_image);

/*------------------------------------------------------------------
|	create_frame()
|	create the base frame and buttons
+------------------------------------------------------------------*/
create_frame (argc, argv)
int argc;
char **argv;
{
extern void submitproc ();
extern void AutoN ();
extern void AutoU ();
extern void deleteproc ();
extern void editproc ();
extern void set_active_sno ();
extern void saveproc ();
extern void fln_done_proc ();
extern void locateproc ();
extern void printproc ();
extern void quitproc ();
extern void trayproc ();
extern Notify_value but_handler ();
extern Notify_value enter_panel_event ();
extern Notify_value open_close_frame ();
extern Notify_value pick ();
extern Notify_value setsampleno ();
extern Notify_value setnext ();
extern Notify_value setprompt ();

Textsw_status	t_status;
char	*name,line[80];
int	i;

/* --- create the icon and base frame          --- */
   asm_icon = icon_create(ICON_IMAGE,	&icon_pixrect,
   			ICON_LABEL,	"        ",
			0);
   
   frame = window_create (0, FRAME,
	FRAME_LABEL,			"Auto Sampler Entry Frame",
	FRAME_ICON,			asm_icon,
	WIN_Y,				0,
	WIN_COLUMNS,			80,
	FRAME_SUBWINDOWS_ADJUSTABLE,	FALSE,
	0);

/* --- create the choice panel with items          --- */
   choice_panel = window_create(frame,PANEL,
        WIN_ROWS,		2,
	WIN_COLUMNS,		80,
        WIN_X,			0,
        WIN_Y,			0,
	WIN_CONSUME_KBD_EVENT,	WIN_NO_EVENTS,
	0);

   if (arg_opt)
   {  if  ( argv[arg_opt][1]=='n' || argv[arg_opt][2]=='n' )
          offonN = 1;
      else
          offonN = 0;
   }
   Auto_Number = panel_create_item (choice_panel, PANEL_CHOICE,
        PANEL_LABEL_X,		ATTR_COL(0),
        PANEL_LABEL_Y,		ATTR_ROW(0),
	PANEL_LABEL_STRING,	"Auto Sample No.:",
	PANEL_FEEDBACK,		PANEL_INVERTED,
	PANEL_NOTIFY_PROC,     	AutoN,
	PANEL_DISPLAY_LEVEL,	PANEL_ALL,
	PANEL_LAYOUT,		PANEL_HORIZONTAL,
        PANEL_VALUE_X,		ATTR_COL(3),
        PANEL_VALUE_Y,		ATTR_ROW(1),
        PANEL_VALUE,		offonN,
	PANEL_CHOICE_STRINGS,	" off "," on ",0,
	0);

   if (default_user==0)
   {  offonU = 0;
   }
   else
   {  if (arg_opt)
      {  if ( argv[arg_opt][1]=='u' || argv[arg_opt][2]=='u' )
            offonU = 1;
         else
            offonU = 0;
      }
   }
   Auto_User = panel_create_item (choice_panel, PANEL_CHOICE,
        PANEL_LABEL_X,		ATTR_COL(20),
        PANEL_LABEL_Y,		ATTR_ROW(0),
	PANEL_LABEL_STRING,	"Auto User Fill:",
	PANEL_FEEDBACK,		PANEL_INVERTED,
	PANEL_NOMARK_IMAGES,	0,
	PANEL_NOTIFY_PROC,      AutoU,
	PANEL_DISPLAY_LEVEL,	PANEL_ALL,
	PANEL_LAYOUT,		PANEL_HORIZONTAL,
        PANEL_VALUE_X,		ATTR_COL(23),
        PANEL_VALUE_Y,		ATTR_ROW(1),
        PANEL_VALUE,		offonU,
	PANEL_CHOICE_STRINGS,	" off "," on ",0,
	0);

   if (default_user == 0) panel_set(Auto_User, PANEL_SHOW_ITEM, FALSE, 0);

   Printbutton = panel_create_item (choice_panel, PANEL_BUTTON,
        PANEL_ITEM_X,		ATTR_COL(41),
        PANEL_ITEM_Y,		ATTR_ROW(0),
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"PRINT", 7, 0),
	PANEL_NOTIFY_PROC,      printproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);
							       
   Deletebutton = panel_create_item (choice_panel, PANEL_BUTTON,
        PANEL_ITEM_X,		ATTR_COL(50),
        PANEL_ITEM_Y,		ATTR_ROW(0),
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"DELETE", 7, 0),
	PANEL_NOTIFY_PROC,      deleteproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);
							       
   if (!Qflag)
      Loadbutton = panel_create_item (choice_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"LOAD",7,0),
	PANEL_NOTIFY_PROC,      saveproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);
							       
   Quitbutton = panel_create_item (choice_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	panel_button_image (choice_panel, "QUIT", 7, 0),
	PANEL_NOTIFY_PROC,      quitproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);

   if (Qflag)
      Submitbutton = panel_create_item (choice_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"SUBMIT",7,0),
	PANEL_NOTIFY_PROC,      submitproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);

   Editbutton = panel_create_item (choice_panel, PANEL_BUTTON,
        PANEL_ITEM_X,		ATTR_COL(50),
        PANEL_ITEM_Y,		ATTR_ROW(1),
	PANEL_LABEL_IMAGE,	panel_button_image (choice_panel, "EDIT", 7, 0),
	PANEL_NOTIFY_PROC,      editproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);

   if (!Qflag)
      Savebutton = panel_create_item (choice_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"SAVE AS",7,0),
	PANEL_NOTIFY_PROC,      saveproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);
							       
   Locatebutton = panel_create_item (choice_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	panel_button_image(choice_panel,"LOCATE",7,0),
	PANEL_NOTIFY_PROC,      locateproc,
	PANEL_SHOW_ITEM,	TRUE,
	0);
							       
/* --- create the tray panel with items          --- */
   Tray_panel = window_create(frame,PANEL,
        WIN_ROWS,		4,
	WIN_COLUMNS,		80,
        WIN_X,			0,
        WIN_BELOW,		choice_panel,
	WIN_CONSUME_KBD_EVENT,	WIN_NO_EVENTS,
	0);

   Tray[0] = panel_create_item(Tray_panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	"  1"," 2"," 3"," 4"," 5"," 6"," 7",
					" 8"," 9","10","11","12","13","14",
					"15","16","17","18","19","20","21",
					"22","23","24","25",0,
		PANEL_DISPLAY_LEVEL,	PANEL_ALL,
		PANEL_FEEDBACK,		PANEL_INVERTED,
		PANEL_SHOW_MENU_MARK,	FALSE,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_NOTIFY_PROC,	trayproc,
		PANEL_SHOW_ITEM,	TRUE,
		0);

   Tray[1] = panel_create_item(Tray_panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	" 26","27","28","29","30","31","32",
					"33","34","35","36","37","38","39",
					"40","41","42","43","44","45","46",
					"47","48","49","50",0,
		PANEL_DISPLAY_LEVEL,	PANEL_ALL,
		PANEL_FEEDBACK,		PANEL_INVERTED,
		PANEL_SHOW_MENU_MARK,	FALSE,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_NOTIFY_PROC,	trayproc,
		PANEL_SHOW_ITEM,	TRUE,
		0);

   Tray[2] = panel_create_item(Tray_panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	" 51","52","53","54","55","56","57",
					"58","59","60","61","62","63","64",
					"65","66","67","68","69","70","71",
					"72","73","74","75",0,
		PANEL_DISPLAY_LEVEL,	PANEL_ALL,
		PANEL_FEEDBACK,		PANEL_INVERTED,
		PANEL_SHOW_MENU_MARK,	FALSE,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_NOTIFY_PROC,	trayproc,
		PANEL_SHOW_ITEM,	TRUE,
		0);

   Tray[3] = panel_create_item(Tray_panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	" 76","77","78","79","80","81","82",
					"83","84","85","86","87","88","89",
					"90","91","92","93","94","95","96",
					"97","98","99","100",0,
		PANEL_DISPLAY_LEVEL,	PANEL_ALL,
		PANEL_FEEDBACK,		PANEL_INVERTED,
		PANEL_SHOW_MENU_MARK,	FALSE,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_NOTIFY_PROC,	trayproc,
		PANEL_SHOW_ITEM,	TRUE,
		0);

/* --- create the menu panel but don't show the 8 buttons yet --- */
   menu_panel = window_create(frame,PANEL,
        WIN_ROWS,		1,
	WIN_COLUMNS,		80,
        WIN_X,			0,
	WIN_CONSUME_KBD_EVENT,	WIN_NO_EVENTS,
	WIN_CONSUME_PICK_EVENTS,	WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, 0,
	PANEL_ITEM_X_GAP,	4,
        WIN_BELOW,		Tray_panel,
	0);

/* setup the eight menu buttons */

   for (i = 0; i < 8; i++)
   {
      but[i] = panel_create_item(menu_panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC,	but_handler,
		PANEL_CLIENT_DATA,	i,  /* button number */
		0);
   }

   menu("e_sample");

/* --- create then message panel. --- */
/* this panel is split to show the active sample number */
/* if Qflag is set and no argument was a filename       */

   if (Qflag && !arg_file) i = 66;
   else			   i = 80;
   header_panel = window_create(frame,PANEL,
        WIN_ROWS,		1,
	WIN_COLUMNS,		i,
        WIN_X,			0,
        WIN_BELOW,		menu_panel,
	WIN_CONSUME_PICK_EVENT,	WIN_NO_EVENTS,
	0);

   Errormsg = panel_create_item (header_panel, PANEL_MESSAGE,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(0),
        PANEL_LABEL_STRING,	" ",
	PANEL_SHOW_ITEM,	FALSE,
	0);

   Fln_req = panel_create_item (header_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(0),
	PANEL_NOTIFY_PROC,	fln_done_proc,
	PANEL_LABEL_STRING,	"Enter Filename: ",
	PANEL_SHOW_ITEM,	FALSE,
	0);

/* small pannel to show the active sample no */
    if (Qflag && !arg_file)
    {   active_panel = window_create(frame, PANEL,
	WIN_ROWS,		1,
	WIN_COLUMNS,		13,
	WIN_RIGHT_OF,		header_panel,
	0);

       Activemsg = panel_create_item (active_panel, PANEL_MESSAGE,
	PANEL_LABEL_X,		5,
	PANEL_LABEL_Y,		ATTR_ROW(0),
        PANEL_LABEL_STRING,	" ",
	PANEL_SHOW_ITEM,	TRUE,
	0);
    }

/* --- create the enter panel with items           --- */

   enter_panel = window_create(frame,PANEL,
        WIN_ROWS,		6,
	WIN_COLUMNS,		80,
        WIN_X,			0,
        WIN_BELOW,		header_panel,
        WIN_CONSUME_KBD_EVENT,	WIN_TOP_KEYS,
	WIN_EVENT_PROC,		enter_panel_event,
	0);

   sample_item = panel_create_item(enter_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(0),
	PANEL_LABEL_STRING,	" SAMPLE#: ",
	PANEL_VALUE_DISPLAY_LENGTH,	65,
	PANEL_NOTIFY_LEVEL,	PANEL_ALL,
        PANEL_NOTIFY_PROC,	setsampleno,
	0);

   user_item = panel_create_item(enter_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(1),
	PANEL_LABEL_STRING,	"    USER: ",
	PANEL_VALUE_DISPLAY_LENGTH,	65,
        PANEL_NOTIFY_PROC,	setnext,
	0);

   macro_item = panel_create_item(enter_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(2),
	PANEL_LABEL_STRING,	"   MACRO: ",
	PANEL_VALUE_DISPLAY_LENGTH,	65,
        PANEL_NOTIFY_PROC,	setnext,
	0);

   solvent_item = panel_create_item(enter_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(3),
	PANEL_LABEL_STRING,	" SOLVENT: ",
	PANEL_VALUE_STORED_LENGTH,	20,
	PANEL_VALUE_DISPLAY_LENGTH,	20,
        PANEL_NOTIFY_PROC,	setnext,
	0);

   text_item = panel_create_item(enter_panel, PANEL_TEXT,
	PANEL_LABEL_X,		ATTR_COL(1),
	PANEL_LABEL_Y,		ATTR_ROW(4),
	PANEL_LABEL_STRING,	"    TEXT: ",
	PANEL_VALUE_DISPLAY_LENGTH,	65,
	PANEL_VALUE_STORED_LENGTH,	128,
        PANEL_NOTIFY_PROC,	setnext,
	0);

/* --- create full enter screen and fill if argument supplied --- */

   enter_screen = window_create(frame,TEXTSW,
	WIN_COLUMNS,		80,
	WIN_ROWS,		24,
        WIN_X,			0,
        WIN_BELOW,		enter_panel,
	WIN_CONSUME_KBD_EVENT,	WIN_NO_EVENTS,
	WIN_CONSUME_PICK_EVENTS,	WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, 0,
	TEXTSW_LOWER_CONTEXT,	2,
	0);

/* if Qflag is set, regardless whether a filename was secified,	*/
/* then nothing is shown in the textsw window, only entries 	*/
/* added will be shown.						*/
/* Upon closing the window the entries are added to the		*/
/* appropriate file, either the enterQ or the filename specified*/

/* if Qflag is set but no filename is specified, then set filename	*/
/* to be the enterQ + doneQ + sampleinfo				*/
   if (Qflag && !arg_file)
   {   load_or_save = LOADING;
       strcpy(filename,vnmrsys);
       strcat(filename,"/acqqueue/enterQ");
       panel_set(Fln_req, PANEL_VALUE, filename, 0);
       fln_done_proc();
       textsw_reset(enter_screen, 0, 0);
       strcpy(filename,vnmrsys);
       strcat(filename,"/acqqueue/doneQ");
       panel_set(Fln_req, PANEL_VALUE, filename, 0);
       clear_tray = FALSE;
       fln_done_proc();
       clear_tray = TRUE;
       textsw_reset(enter_screen, 0, 0);
       set_active_sno();
       if (!active_sample_no)
          panel_set(Activemsg, PANEL_LABEL_STRING, "Active: NONE", 0);
       else
       {  sprintf(buffer,"Active: %3d",active_sample_no);
          panel_set(Activemsg, PANEL_LABEL_STRING, buffer, 0);
       }
       inittimer(10.0,10.0,set_active_sno);
   }

/* if a filename is specified, regardless of Qflag, then that file is loaded */
   if (arg_file)
   {   
       load_or_save = LOADING;
       panel_set(Fln_req, PANEL_VALUE, argv[arg_file], 0);
       fln_done_proc();
       if (Qflag) textsw_reset(enter_screen, 0, 0);
   }

/* if the mouse is clicked in the enter screen then refer */
/* to the pick routine. The only mouse activety allowed   */
/* is the left button, which high lights the entry        */
   notify_interpose_event_func (enter_screen, pick, NOTIFY_SAFE);

/* if Qflag is specified closing and opening the frame takes extra action */
   if (Qflag) notify_interpose_event_func(frame,open_close_frame,NOTIFY_SAFE);

   set_sample_user();

   window_fit(frame);

   return (0);
}

