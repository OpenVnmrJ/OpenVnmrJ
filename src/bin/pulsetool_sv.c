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

/************************************************************************
************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <strings.h>
#include <ctype.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>
#include <suntool/textsw.h>
#include <suntool/tty.h>
#include <suntool/alert.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dir.h>
#include "pulsetool.h"

#define CMS_SIZE 64
static u_char           red[CMS_SIZE]=  {  0, 255, 230, 220, 190, 150,  80,   0,
                                           0, 255, 255, 255,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                         155, 175, 195, 205, 215, 235, 255,   0,
                                         165, 180, 195, 210, 225, 240, 255,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0, 255}
,

                        green[CMS_SIZE]={  0,   0,   0,   0,  50,  80, 110, 140,
                                         255, 180, 255,   0, 200, 255,   0,   0,
                                          95, 105, 115, 125, 135, 145, 155, 165,
                                         175, 185, 195, 210, 225, 240, 255,   0,
                                         120, 130, 140, 150, 160, 170, 180,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                         175, 185, 200, 215, 235, 255,   0, 255}
,

                        blue[CMS_SIZE]= {  0,   0, 170, 210, 230, 250, 250, 250,
                                         255,   0,   0, 255, 255,   0, 255, 255,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                         165, 180, 195, 210, 225, 240, 255,   0,
                                           0,   0,   0,   0,   0,   0,   0,   0,
                                           0,   0,   0,   0,   0,   0, 230, 255}
;

static struct singlecolor  foreground;
static struct singlecolor  background;

static int              *display_name=NULL;
static int		depth;
static Icon             icon;
extern int		char_width, char_height, char_ascent, char_descent;

static short icon_data[] = {
/* Format_version=1, Width=64, Height=64, Depth=1, Valid_bits_per_item=16 */
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFE,0x7FFF,0xFFFF,
    0xFFFF,0xFFFC,0xBFFF,0xFFFF,0xFFFF,0xFFFD,0xBFFF,0xFFFF,
    0xFFFF,0xFFFA,0xDFFF,0xFFFF,0xFFFF,0xFFFB,0x9FFF,0xFFFF,
    0xFFFF,0xFFF6,0xEFFF,0xFFFF,0xF000,0x0000,0x2FFF,0xFFFF,
    0xEFFF,0xFFFF,0xCFFF,0xFFFF,0xDFFF,0xFFFF,0xE7FF,0xFFFF,
    0xD8ED,0xBF38,0x67FF,0xFFFF,0xDB6D,0xBEDB,0xE7FF,0xFFFF,
    0xDB6D,0xBF78,0xE7FF,0xFFFF,0xD8ED,0xBFBB,0xEBFF,0xFFFF,
    0xDBED,0xBEDB,0xEBFF,0xFFFF,0xDBF3,0x8738,0x6BFF,0xFFFF,
    0xDFFF,0xFFFF,0xE9FF,0xFFFF,0xEFFF,0xFFFF,0xCDFF,0xFFFF,
    0xF000,0x0000,0x39FF,0xFFFF,0xFFFF,0xFFAE,0xEDFF,0xFFFF,
    0xFFFF,0xFF3B,0xBAFF,0xFFFF,0xFFFF,0x3F6E,0xEEFC,0xFFFF,
    0xFFFE,0x9F3B,0xBAFB,0x7FFF,0xFFFC,0xDF6E,0xEEFA,0xBFFF,
    0xF8FD,0xAEBB,0xBB73,0xBF1F,0xF37A,0xEEEE,0xEE76,0xDEEF,
    0xEEBB,0xAEBB,0xBB73,0x9DB7,0xDBB6,0xE4EE,0xEEAE,0xECEB,
    0xCE93,0xB5BB,0xBBAB,0xABBB,0xBBAE,0xEAEE,0xEE8E,0xE6ED,
    0xAEEB,0xBBBB,0xBB8B,0xB3B9,0xDB96,0xE4EE,0xEEAE,0xEAEB,
    0xCEB3,0xB5BB,0xBBAB,0xADBB,0xEBBA,0xEEEE,0xEE76,0xDCE7,
    0xF67B,0xAEBB,0xBB73,0x9EAF,0xF8FC,0xEEEE,0xEE76,0xBF1F,
    0xFFFD,0x9F3B,0xBAFB,0xBFFF,0xFFFE,0xDF6E,0xEEFA,0x7FFF,
    0xFFFF,0x3F3B,0xBAFC,0xFFFF,0xFFFF,0xFF6E,0xEEFF,0xFFFF,
    0xFFFF,0xFFBB,0xB9FF,0xFFFF,0xFFFF,0xFFAE,0xE000,0x000F,
    0xFFFF,0xFFBB,0xAFFF,0xFFF7,0xFFFF,0xFFAE,0xDFFF,0xFFFB,
    0xFFFF,0xFFDB,0x9833,0xCEFB,0xFFFF,0xFFCE,0xDEED,0xB6FB,
    0xFFFF,0xFFDB,0x9EED,0xB6FB,0xFFFF,0xFFEE,0xDEED,0xB6FB,
    0xFFFF,0xFFEB,0x9EED,0xB6FB,0xFFFF,0xFFEE,0xDEF3,0xCE1B,
    0xFFFF,0xFFEB,0x9FFF,0xFFFB,0xFFFF,0xFFF6,0xEFFF,0xFFF7,
    0xFFFF,0xFFF3,0xB000,0x000F,0xFFFF,0xFFF6,0xEFFF,0xFFFF,
    0xFFFF,0xFFFB,0x9FFF,0xFFFF,0xFFFF,0xFFFA,0xDFFF,0xFFFF,
    0xFFFF,0xFFFD,0xBFFF,0xFFFF,0xFFFF,0xFFFC,0xBFFF,0xFFFF,
    0xFFFF,0xFFFE,0x7FFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF
};

static mpr_static(icon_pr, 64, 64, 1, icon_data);

static caddr_t		object[NUM_OBJECTS];
static Pixfont		*font,*canvas_font;
static Pixwin		*window[NUM_WINDOWS];

int			color_display=0;

Notify_value	signal1_handler(),
		dead_child(),
		pipe_reader();

char		*pulse_frame_label[2] = {"     Amplitude            Phase             Frequency             Real             Imaginary      Fourier Transform",
                                         "        Mx                  My                 Mz                 Mxy                Phase        Fourier Transform"};

static int   ME;
static int   *me = &ME;
static int   xor_flag = FALSE;

/*****************************************************************************
*		EXTERNAL VARIABLES AND FUNCTIONS
******************************************************************************/
extern int tochild,
	   fromchild,
	   childpid,
	   interrupt;
/*****************************************************************************
*		WINDOWING FUNCTIONS
******************************************************************************/
int is_frame(which)
int which;
{
   if ((which > FIRST_FRAME) && (which < LAST_FRAME))
     return(TRUE);
   else
     return(FALSE);
}

int is_button(which)
int which;
{
   if ((which > FIRST_BUTTON) && (which < LAST_BUTTON))
     return(TRUE);
   else
     return(FALSE);
}

/*****************************************************************************
*	init window system and set display and depth variables 
******************************************************************************/
void init_window(argc,argv)
int argc;
char *argv[];
{
}

/*****************************************************************************
*	Set up font and return character string with font name 
******************************************************************************/
void setup_font(font_name)
char **font_name;
{
    font = pf_open("/usr/lib/fonts/fixedwidthfonts/serif.r.16");
    canvas_font = pf_open("/usr/lib/fonts/fixedwidthfonts/serif.r.14");
    *font_name = (char *)malloc(sizeof(char)*12);
    strcpy(*font_name,"serif.r.14");
    char_height = canvas_font->pf_defaultsize.y;
    char_width = canvas_font->pf_defaultsize.x;
    char_ascent = (int)(0.8*canvas_font->pf_defaultsize.y + 0.5);
    char_descent = char_height-char_ascent;
}

/*****************************************************************************
*	Set up and attach icon 
******************************************************************************/
void setup_icon()
{
     icon = icon_create(ICON_IMAGE, &icon_pr, 0);
     window_set(object[BASE_FRAME],FRAME_ICON,icon,NULL);
}

/*****************************************************************************
*	Set up colormaps - cms for all objects except canvases, and canvas_cms
*	for canvases.
******************************************************************************/
void setup_colormap()
{
    foreground.red = 0; foreground.green = 0; foreground.blue = 230;
    background.red = 255; background.green = 255; background.blue = 255;
}

/*****************************************************************************
*	Create gc for object which.
******************************************************************************/
void create_gc(which)
int which;
{
}

/*****************************************************************************
*	Create button at (x,y) with label and notify proc attached.
******************************************************************************/
void create_button(which,parent,x,y,label,notify_proc)
int which, parent, x, y;
char *label;
int (*notify_proc)();
{
    object[which] = panel_create_item(object[parent], PANEL_BUTTON,
			PANEL_ITEM_X, x, PANEL_ITEM_Y, y,
			PANEL_LABEL_IMAGE, panel_button_image(object[parent],
				label,strlen(label),0),
			PANEL_NOTIFY_PROC, notify_proc,
			NULL);
}

/*****************************************************************************
*	Given column number, return pixel value.
******************************************************************************/
int column(which,col)
int which, col;
{
    return(ATTR_COL(col));
}

/*****************************************************************************
*	Given row number, return pixel value.
******************************************************************************/
int row(which,r)
int which, r;
{
    return(ATTR_ROW(r));
}

/*****************************************************************************
*	Create menu with "num_strings" objects with labels in "strings"
*	and notify proc for menu selection "notify_proc".  Attach menu to
*	button "which".
******************************************************************************/
void attach_menu_to_button(which,menu,num_strings,strings,notify_proc)
int which, menu, num_strings;
char *strings[];
int (*notify_proc)();
{
    int i;
    Menu_item mi;
    void menu_button_event_proc();
    int create_menu_proc();

    object[menu] = menu_create(NULL);
    for (i=0;i<num_strings;i++)  {
	mi = (Menu_item)menu_create_item(
		MENU_STRING, strings[i],
		MENU_NOTIFY_PROC, create_menu_proc,
		NULL);
	menu_set(object[menu],MENU_APPEND_ITEM, mi, NULL);
	}
    panel_set(object[which],PANEL_EVENT_PROC,menu_button_event_proc,NULL);
}

void menu_button_event_proc(item,event)
Panel_item	item;
Event		*event;
{
    int obj;
    extern char *create_menu_strings[];
    int menu_val;
    void frame_message();

    obj = find_object(item);
    switch (event_action(event)) {
        case  MS_LEFT:
        case  MS_MIDDLE:
        case  MS_RIGHT:
            if (event_is_down(event)) {
                panel_begin_preview(item, event) ;
		switch (obj)  {
		  case MAIN_CREATE_BUTTON :
                	menu_val = (int)menu_show(object[CREATE_MENU],
				object[MAIN_BUTTON_PANEL], event, 0);
			break;
		  default :
			frame_message("Error : No menu attached to button",BASE_FRAME);
			break;
		  }
                panel_accept_preview(item,event) ;
            }
        }
}

/*****************************************************************************
*	Create cycle at (x,y) with label and strings as cycle optons.
*	Attach notify proc.
******************************************************************************/
void create_cycle(which,parent,x,y,label,num_strings,strings,def,
			notify_proc)
int which,parent;
int x, y;
char *label;
int num_strings;
char *strings[];
int def;
int (*notify_proc)();
{
    int i;

    y += 4;
    object[which] = panel_create_item(object[parent], PANEL_CYCLE,
			PANEL_ITEM_X, x, PANEL_ITEM_Y, y,
			PANEL_LABEL_STRING, label,
			PANEL_CHOICE_STRING, 0, strings[0],
			PANEL_NOTIFY_PROC, notify_proc,
			PANEL_VALUE, def,
			NULL);
    for (i=1;i<num_strings;i++)
      panel_set(object[which],PANEL_CHOICE_STRING,i,strings[i],NULL);
}

/*****************************************************************************
*	Create type-in text object at (x,y) with label and a type-in area of
*	len characters and default string of value.  Attach notify proc.
******************************************************************************/
void create_panel_text(which,parent,x,y,label,value,len,notify_proc)
int which,parent;
int x, y;
char *label, *value;
int len;
int (*notify_proc)();
{
    object[which] = panel_create_item(object[parent], PANEL_TEXT,
			PANEL_ITEM_X, x, PANEL_ITEM_Y, y,
			PANEL_LABEL_STRING, label,
			PANEL_VALUE, value,
			PANEL_VALUE_DISPLAY_LENGTH, len,
			PANEL_NOTIFY_PROC, notify_proc,
			NULL);
}


/*****************************************************************************
*	Create panel message object at (x,y) with message label.
******************************************************************************/
void create_panel_message(which,parent,x,y,label)
int which,parent;
int x, y;
char *label;
{
    object[which] = panel_create_item(object[parent], PANEL_MESSAGE,
			PANEL_ITEM_X, x, PANEL_ITEM_Y, y,
			PANEL_LABEL_STRING, label,
			NULL);
}

/*****************************************************************************
*	Create panel area at (x,y) of size width by height.
******************************************************************************/
void create_panel(which,parent,x,y,width,height)
int which,parent;
int x, y, width, height;
{
    Pixwin *pw;

    if ((which == MAIN_BUTTON_PANEL) || (which == MAIN_PARAMETER_PANEL))  {
      width += 24;
      }
    object[which] = window_create(object[parent], PANEL,
            WIN_X,          x,
            WIN_Y,          y,
            WIN_WIDTH,      width,
            WIN_HEIGHT,     height,
	    WIN_FONT,	   font,
	    WIN_ROW_GAP,   -10,
            NULL);
    pw = (Pixwin *)window_get(object[which],WIN_PIXWIN);
    depth = pw->pw_pixrect->pr_depth;
    if ((which != CREATE_PANEL) && (which != SIMULATE_PANEL))  {
      if (depth > 1)  {
	color_display = TRUE;
	pw_setcmsname(pw,"pt_panelcolor");
	pw_putcolormap(pw,0,2,&red[CMS_SIZE-2],&green[CMS_SIZE-2],&blue[CMS_SIZE-2]);
	}
      }

}

/*****************************************************************************
*	Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame(which,parent,label,x,y,width,height)
int which, parent;
char *label;
int x, y, width, height;
{
    object[which] =
        (Frame)window_create( parent, FRAME,
            FRAME_LABEL,    label,
            WIN_X,          x,
            WIN_Y,          y,
            WIN_WIDTH,      width,
            WIN_HEIGHT,     height,
	    WIN_FONT, font,
	    FRAME_FOREGROUND_COLOR, &foreground,
	    FRAME_BACKGROUND_COLOR, &background,
	    FRAME_INHERIT_COLORS, TRUE,
            NULL);
}

/*****************************************************************************
*	Create canvas at (x,y) of size width by height.  Attach an event
*	handler and a repaint process.  Specify if border is to be shown (TRUE)
*	or hidden (FALSE).
******************************************************************************/
void create_canvas(which,parent,x,y,width,height,repaint_proc,event_proc,border)
int which, parent, x, y, width, height;
int (*repaint_proc)(), (*event_proc)();
int border;
{
    if (which == PLOT_WIN)  {
      x += (int)window_get(object[LG_WIN],WIN_X) - 1;
      y += (int)window_get(object[LG_WIN],WIN_Y) - 1;
      parent = BASE_FRAME;
      }
    if (which == LG_WIN)   {
      width += 24;
      }
    object[which] = window_create(object[parent], CANVAS,
                WIN_X,  x,
                WIN_Y,  y,
                WIN_FONT, canvas_font,
                WIN_WIDTH,      width,
                WIN_HEIGHT,     height,
                CANVAS_REPAINT_PROC, repaint_proc,
                WIN_CONSUME_PICK_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, NULL,
		WIN_EVENT_PROC, event_proc,
                NULL);

    window[which-FIRST_WINDOW] = canvas_pixwin(object[which]);
    depth = ((Pixwin *)(window[which-FIRST_WINDOW]))->pw_pixrect->pr_depth;
    if (depth > 1)  {
      color_display = TRUE;
      pw_setcmsname(window[which-FIRST_WINDOW],"pt_colormap");
      pw_putcolormap(window[which-FIRST_WINDOW],0,CMS_SIZE,red,green,blue);
      }
}

/*****************************************************************************
*	Set object which to the right of other.
******************************************************************************/
void set_win_right_of(which, other)
int which, other;
{
    window_set(object[which],WIN_RIGHT_OF, object[other], NULL);
}

/*****************************************************************************
*	Set object which below other.
******************************************************************************/
void set_win_below(which, other)
int which, other;
{
    window_set(object[which],WIN_BELOW, object[other], NULL);
}

/*****************************************************************************
*	Set panel text value.
******************************************************************************/
void set_panel_value(which,value)
int which;
win_val value;
{
    panel_set(object[which],PANEL_VALUE,value,NULL);
}

/*****************************************************************************
*	Get panel text value.
******************************************************************************/
win_val get_panel_value(which)
int which;
{
    return((win_val)panel_get(object[which],PANEL_VALUE));
}

/*****************************************************************************
*	Set panel label value.
******************************************************************************/
void set_panel_label(which,str)
int which;
char *str;
{
    if (is_button(which))
      panel_set(object[which],PANEL_LABEL_IMAGE,
       panel_button_image(object[MAIN_BUTTON_PANEL],str,strlen(str),NULL),NULL);
    else
      panel_set(object[which],PANEL_LABEL_STRING,str,NULL);
}

/*****************************************************************************
*	Set frame label value.
******************************************************************************/
void set_frame_label(which,value)
int which;
win_val value;
{
    if (which == BASE_FRAME)
      window_set(object[which],FRAME_LABEL,pulse_frame_label[value],NULL);
    else
      window_set(object[which],FRAME_LABEL,value,NULL);
}

/*****************************************************************************
*       Return width of Panel object.
******************************************************************************/
int object_width(which)
int which;
{
    Rect *item;

    item = (Rect *)panel_get(object[which],PANEL_ITEM_RECT);
    if ((which >= FIRST_BUTTON) && (which <= LAST_BUTTON + 2))
      return((int)item->r_width+8);
    else
      return((int)item->r_width);
}

/*****************************************************************************
*       set width of Panel.
******************************************************************************/
int set_width(which,width)
int which,width;
{
    if ((which == MAIN_PARAMETER_PANEL)||(which == MAIN_BUTTON_PANEL))
      return;
    window_set(object[which],WIN_WIDTH,width,NULL);
}

/*****************************************************************************
*       set height of Panel.
******************************************************************************/
int set_height(which,height)
int which,height;
{
    window_set(object[which],WIN_HEIGHT,height,NULL);
}

/*****************************************************************************
*	Show object.
******************************************************************************/
void show_object(which)
{
    if ((which==BASE_FRAME)||(which==CREATE_FRAME)||(which==SIMULATE_FRAME)) 
      window_set(object[which],WIN_SHOW,TRUE,NULL);
    else
      panel_set(object[which],PANEL_SHOW_ITEM,TRUE,NULL);
}

/*****************************************************************************
*	Hide object.
******************************************************************************/
void hide_object(which)
{
    if ((which==BASE_FRAME)||(which==CREATE_FRAME)||(which==SIMULATE_FRAME)) 
      window_set(object[which],WIN_SHOW,FALSE,NULL);
    else
      panel_set(object[which],PANEL_SHOW_ITEM,FALSE,NULL);
}

/*****************************************************************************
*	Given a window object, find it's index in "objects" array.
******************************************************************************/
int find_object(item)
caddr_t item;
{
    int i;

    i = 0;
    while (i<NUM_OBJECTS)  {
      if (object[i] == item)
        return(i);
      i++;
      }
    return(-1);  /* error */
}

/*****************************************************************************
*	Do frame fit operation to resize frame to fit contents.
******************************************************************************/
void fit_frame(which)
int which;
{
   window_fit(object[which]);
}

/*****************************************************************************
*	Do frame fit operation to resize frame to fit contents in vertical
*	direction.
******************************************************************************/
void fit_height(which)
int which;
{
   window_fit_height(object[which]);
}

/*****************************************************************************
*	Set up signal handler for interrupt signal from pulsechild and
*	loop on BASE_FRAME.
******************************************************************************/
void window_start_loop(which)
int which;
{
    window_fit(object[which]);
    (void)notify_set_signal_func(object[which],signal1_handler,SIGUSR1,
		NOTIFY_ASYNC);
    window_main_loop(object[which]);
}

/*****************************************************************************
*	Tell window system to batch all draw requests.
******************************************************************************/
void start_graphics(which)
int which;
{
   pw_batch_on(window[which-FIRST_WINDOW]);
}

/*****************************************************************************
*	Tell window system to do all pending draw requests NOW.
******************************************************************************/
void flush_graphics(which)
int which;
{
   pw_batch_off(window[which-FIRST_WINDOW]);
}

/*****************************************************************************
*	Pop up confirm box with "Yes" or "No" options.
******************************************************************************/
int notice_yn(which,str)
int which;
char *str;
{
    int val;
    val = alert_prompt(object[BASE_FRAME],NULL,
                ALERT_MESSAGE_STRINGS, str, NULL,
                ALERT_BUTTON_YES, "Yes",
                ALERT_BUTTON_NO, "No",
		ALERT_NO_BEEPING, 1,
                NULL);
    if (val == ALERT_NO)
      return(FALSE);
    else
      return(TRUE);
}

/*****************************************************************************
*	Pop up informational box.
******************************************************************************/
void notice_ok(which,str)
int which;
char *str;
{
    alert_prompt(object[BASE_FRAME],NULL,
                ALERT_MESSAGE_STRINGS, str, NULL,
                ALERT_BUTTON_YES, "Ok",
		ALERT_NO_BEEPING, 1,
                NULL);
}

/******************************************************************************
*		EVENT HANDLERS
******************************************************************************/

/******************************************************************************
*	Repaint six small canvases on top of window.
******************************************************************************/
int repaint_small_canvases(canvas,pixwin,repaint_area)
Canvas canvas;
Pixwin *pixwin;
Rectlist *repaint_area;
{
    int obj;

    /* Figure out which canvas prompted this request and redraw it */
    if (pixwin == window[SM_WIN_1-FIRST_WINDOW])
      obj = SM_WIN_1;
    else if (pixwin == window[SM_WIN_2-FIRST_WINDOW])
      obj = SM_WIN_2;
    else if (pixwin == window[SM_WIN_3-FIRST_WINDOW])
      obj = SM_WIN_3;
    else if (pixwin == window[SM_WIN_4-FIRST_WINDOW])
      obj = SM_WIN_4;
    else if (pixwin == window[SM_WIN_5-FIRST_WINDOW])
      obj = SM_WIN_5;
    else if (pixwin == window[SM_WIN_6-FIRST_WINDOW])
      obj = SM_WIN_6;
    do_repaint_small_canvases(obj);
}

/******************************************************************************
*	Process events in six small canvases on top of window.
******************************************************************************/
int select_large_state(canvas, event)
Canvas	   canvas;
Event      *event;
{
    int ev, obj, button;
    Pixwin *pixwin = canvas_pixwin(canvas);

    /* figure out which button was pressed */
    button = -1;  obj = -1;
    if (((ev=event_action(event)) == MS_LEFT) && event_is_up(event)) {
      button = 1;
      }
    else if (((ev=event_action(event)) == MS_MIDDLE) &&
			event_is_up(event))  {
      button = 2;
      }
    /* if button was pressed, figure out which canvas it was pressed in */
    if (button > 0)  {
      if (pixwin == window[SM_WIN_1-FIRST_WINDOW])
        obj = SM_WIN_1;
      else if (pixwin == window[SM_WIN_2-FIRST_WINDOW])
        obj = SM_WIN_2;
      else if (pixwin == window[SM_WIN_3-FIRST_WINDOW])
        obj = SM_WIN_3;
      else if (pixwin == window[SM_WIN_4-FIRST_WINDOW])
        obj = SM_WIN_4;
      else if (pixwin == window[SM_WIN_5-FIRST_WINDOW])
        obj = SM_WIN_5;
      else if (pixwin == window[SM_WIN_6-FIRST_WINDOW])
        obj = SM_WIN_6;
      if (obj > 0)
	do_select_large_state(obj, button);
      }
}

/******************************************************************************
*	Repaint large canvas.
******************************************************************************/
int repaint_large_canvas(canvas,pixwin,repaint_area)
Canvas canvas;
Pixwin *pixwin;
Rectlist *repaint_area;
{
    do_repaint_large_canvas();
}

int large_canvas_event_handler(canvas, event)
Canvas  canvas;
Event      *event;
{
}

/******************************************************************************
*	Repaint plot canvas.
******************************************************************************/
int repaint_plot_canvas(canvas,pixwin,repaint_area)
Canvas canvas;
Pixwin *pixwin;
Rectlist *repaint_area;
{
    do_repaint_plot_canvas();
}

/******************************************************************************
*	Process events in plot canvas.
******************************************************************************/
int plot_canvas_event_handler(canvas, event)
Canvas  canvas;
Event      *event;
{
    int ev, button, down, up, drag, ctrl, x, y;
    button = -1;
    drag = FALSE;
    /* figure out if button or drag event */
    if ((ev=event_action(event)) == MS_LEFT)  {
      button = 1;
      }
    else if (ev == MS_MIDDLE)  {
      button = 2;
      }
    else if (ev == MS_RIGHT)  {
      button = 3;
      }
    else if (ev == LOC_DRAG)  {  /* if drag, figure out which button is down */
      drag = TRUE;
      if (window_get(object[PLOT_WIN],WIN_EVENT_STATE, MS_LEFT))
	button = 1;
      else if (window_get(object[PLOT_WIN],WIN_EVENT_STATE, MS_MIDDLE))
	button = 2;
      else if (window_get(object[PLOT_WIN],WIN_EVENT_STATE, MS_RIGHT))
	button = 3;
      }
    /* if button or drag event, figure out if button up or down and whether
	control key also pressed */
    if (button > 0)  {
      up = FALSE;
      if (event_is_up(event))
        up = TRUE;
      down = FALSE;
      if (event_is_down(event))
        down = TRUE;
      ctrl = FALSE;
      if (event_ctrl_is_down(event))
	ctrl = TRUE;
      x = event_x(event);
      y = event_y(event);
      do_plot_canvas_event_handler(button,down,up,drag,ctrl, x, y);
      }
}

/******************************************************************************
*	Process file button events.
******************************************************************************/
int files_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_files_proc(find_object(item));
}

/******************************************************************************
*	Process Thresh/Scale button events.
******************************************************************************/
int horiz_notify_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_horiz_notify_proc();
}

/******************************************************************************
*	Process Expand/Full button events.
******************************************************************************/
int expand_notify_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_expand_notify_proc();
}

/******************************************************************************
*	Process Simulate button events.
******************************************************************************/
int simulate_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_simulate_proc(find_object(item));
}

/******************************************************************************
*	Process Pulse/Simulate cycle events.
******************************************************************************/
int display_cycle_proc(item, event)
Panel_item  item;
Event      *event;
{
    do_display_cycle_proc();
}

/******************************************************************************
*     Process Grid cycle events.
******************************************************************************/
int grid_cycle_proc(item, event)
Panel_item  item;
Event      *event;
{
    do_grid_cycle_proc();
}

/******************************************************************************
*     Process Create cycle events.
******************************************************************************/
int create_cycle_proc(item, event)
Panel_item  item;
Event      *event;
{
}

/******************************************************************************
*	Process Simulate 3D button events.
******************************************************************************/
int simulate_3d_proc(item,event)
Panel_item  item;
Event      *event;
{
    do_simulate_3d_proc();
}

/*****************************************************************
*  Activate the Help window when Help button pressed (controlled by pulsechild)
*****************************************************************/
void help_proc(item, event)
Panel_item   item;
Event        *event;
{
    void send_to_pulsechild();

    if (item == object[MAIN_HELP_BUTTON]) {
	send_to_pulsechild("help");
    }
}


/*****************************************************************
* Destroy the parent window system, and kill the child process when Quit button
* pressed.  A "quit" signal is sent to the child only if the xv_destroy
* is successful, i.e., if the user does not cancel with the
* right mouse button.
*****************************************************************/
void quit_proc(item,event)
Panel_item item;
Event *event;
{
  int resp;
  void send_to_pulsechild();

/*    resp = alert_prompt(object[MAIN_BUTTON_PANEL],event,
	ALERT_MESSAGE_STRINGS, "Do you really want to quit?", NULL,
	ALERT_BUTTON_YES, "Quit",
	ALERT_BUTTON_NO,  "Cancel",
	NULL);
    if (resp == ALERT_YES)  {*/
      (void) notify_set_input_func(me,NULL,fromchild);
      if (window_destroy(object[BASE_FRAME]))  {
        send_to_pulsechild("quit");
	}
/*      }*/
}


/******************************************************************************
*	Process Save button events.
******************************************************************************/
void save_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_save_proc(find_object(item));
}

/******************************************************************************
*	Process Print button events.
******************************************************************************/
void print_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_print_proc(find_object(item));
}

/******************************************************************************
*	Process events when create menu item is selected.
******************************************************************************/
int create_menu_proc(menu, m_item)
Menu    menu;
Menu_item         m_item;
{
    char *menu_str;
    menu_str = (char *)menu_get(m_item,MENU_STRING);
    do_create_menu_proc(menu_str);
}

/******************************************************************************
*	Process events when one of parameters at bottom of main window is
*	modified.
******************************************************************************/
int param_notify_proc(item, event)
Panel_item   item;
Event      *event;
{
    do_param_notify_proc(find_object(item));
}

/******************************************************************************
*	Process Create button events.
******************************************************************************/
int create_proc(item, event)
Panel_item   item;
Event        *event;
{
    do_create_proc(find_object(item));
}


/******************************************************************************
*		INTERPROCESS COMMUNICATION
*
*	This stuff depends on the window system, too, or at least for 
*	sending and receiving signals....
******************************************************************************/

/*****************************************************************
*  This routine catches the SIGUSR1 signal sent by pulsechild
*  when a simulation is cancelled with the Cancel button.  The
*  Cancel button lives in pulsechild, and will therefore
*  eventually be acknowledged even though pulsetool is working
*  100% in the bloch_simulate routine.  The variable "interrupt"
*  is periodically checked during a simulation.
*****************************************************************/
Notify_value
signal1_handler()
{
    interrupt = 1;
    return(NOTIFY_DONE);
}

/*****************************************************************
*  This routine writes "string" to pulsechild via the pipe 
*  "tochild".
*****************************************************************/
void send_to_pulsechild(string)
char     *string;
{
    int    returnval;
    strcat(string, "\0");
    returnval = write(tochild, string, strlen(string)+1);
}


/*****************************************************************
*  Fork a child process and exec pulsechild.  
*****************************************************************/
void
start_pulsechild(argc, argv)
int argc;
char *argv[];
{
    int      i, numfds, pipeto[2], pipefrom[2];
    extern char *font_name;

    if (pipe(pipeto) < 0  ||  pipe(pipefrom) < 0) {
        perror("pipe");
        exit(1);
    }
    switch (childpid = fork()) {
        case -1:
            perror("fork");
            exit(1);
        case  0:
	    /****
	    * This is the child.  The pipe to parent pulsetool is via
	    * stdout, and stdin serves as the pipe from pulsetool.
	    ****/
            dup2(pipeto[0], 0);
            dup2(pipefrom[1], 1);
            
            numfds = getdtablesize();
            for (i=3; i<numfds; i++)
                close(i);

            execvp("pulsechild", argv);
            perror("execvp");
            exit(1);
        default:
	    /****
	    * This is the parent.
	    ****/
            close(pipeto[0]);
            close(pipefrom[1]);
            tochild = pipeto[1];
            fromchild = pipefrom[0];
            break;
    }
    (void) notify_set_input_func(me, pipe_reader, fromchild);
    (void) notify_set_wait3_func(me, dead_child, childpid);
}


/*****************************************************************
*  This is the communications link between pulsetool and
*  pulsechild.  It reads 512 byte blocks from the "fromchild"
*  pipe, and acts on the received string appropriately.
*****************************************************************/
Notify_value
pipe_reader(frame, fd)
Frame    frame;
int      fd;
{
    char     buf[512];
    int      i, nread;
    extern char filename[], directory_name[];
    extern char pattern_name[], *font_name;
    void frame_message();

    nread = read(fd, buf, 512);
    if (nread > 0)
	buf[nread] = '\0';
    switch (nread) {
        case -1:
            perror("(parent)pipe_read: read");
            exit(1);
        case  0:
            perror("(parent)pipe_read: read(EOF)");
            exit(1);
        default:
            if (!strncmp(buf, "childok", 7)) {
            }
	    /****
	    * Load a new file.
	    ****/
            else if (!strncmp(buf, "load", 4)) {
                strcpy(pattern_name, &buf[4]);
			set_panel_value(PARAM_FILE_NAME,pattern_name);
			read_pattern_file(0);
		    }
		    /****
		    * Change the working directory.
		    ****/
		    else if (!strncmp(buf, "dir", 3)) {
			strcpy(directory_name, &buf[3]);
			if (chdir(directory_name) == -1) {
			    frame_message("Error:  Unable to change directory.", BASE_FRAME);
			}
			getwd(directory_name);
			set_panel_value(PARAM_DIRECTORY_NAME,directory_name);
            }
	    /****
	    * Load the file written by the editor in pulsechild.  A delay
	    * is inserted by means of the for-loop and the frame_message to
	    * allow the :w in the editor to be completed before reading.
	    ****/
            else if (!strncmp(buf, "view", 4)) {
                strcpy(pattern_name, &buf[4]);
                for (i=0; i<5e4; i++) {}
		set_panel_value(PARAM_FILE_NAME,pattern_name);
                read_pattern_file(0);
            }
    }
    return(NOTIFY_DONE);
}


/*****************************************************************
*  Reap the child process' death, in its unlikely event.
*****************************************************************/
Notify_value
dead_child(frame, pid, status, rusage)
Frame           frame;
int             pid;
union wait      *status;
struct rusage   *rusage;
{
    (void) notify_set_input_func(frame, NOTIFY_FUNC_NULL, fromchild);
    close(tochild);
    close(fromchild);
    /*printf("The program \"pulsechild\" cannot be found, or has died.\n");*/
    exit(-1);
}


/*****************************************************************
*  Display "string" as a flashing message in specified frame's
*  label area.
*****************************************************************/
void frame_message(string, win)
char   *string;
int  win;
{

    alert_prompt(object[BASE_FRAME],NULL,
                        ALERT_MESSAGE_STRINGS, string, NULL,
                        ALERT_BUTTON_YES, "Ok ",
			ALERT_NO_BEEPING, 1,
                        NULL);
}

/******************************************************************************
*		DRAWING ROUTINES
******************************************************************************/

/****************************************************************************
*   Draw a line to a canvas in color
*****************************************************************************/

void win_draw_line(which, x1, y1, x2, y2, color)
int which, x1, y1, x2, y2, color;
{
    unsigned long op;
    if (xor_flag)
      op = PIX_SRC^PIX_DST;
    else
      op = PIX_SRC;
    pw_vector(window[which-FIRST_WINDOW],x1,y1,x2,y2,op,color);
}


/****************************************************************************
*   Draw segments to a canvas in color
*****************************************************************************/

void win_draw_segments(which, data, num, color)
int which;
int *data;
int num, color;
{
    int i;
    unsigned long op;

    if (xor_flag)
      op = PIX_SRC^PIX_DST;
    else
      op = PIX_SRC;

    for (i=0;i<num-1;i++)  {
      pw_vector(window[which-FIRST_WINDOW],data[2*i],data[2*i+1],
		data[2*(i+1)],data[2*(i+1)+1],op,color);
      }
}

/****************************************************************************
*   Draw a point to a canvas in color
*****************************************************************************/

void win_draw_point(which, x, y, color)
int which, x, y, color;
{
    pw_put(window[which-FIRST_WINDOW],x,y,color);
}

/****************************************************************************
*   Draw points to a canvas in color
*****************************************************************************/

void win_draw_points(which, data, num, color)
int which;
int *data;
int num, color;
{
    int i;
    unsigned long op;

    if (xor_flag)
      op = PIX_SRC^PIX_DST;
    else
      op = PIX_SRC;

    for (i=0;i<num;i++)  {
      pw_put(window[which-FIRST_WINDOW],data[2*i],data[2*i+1],color);
      }
}

/****************************************************************************
*   Write a string to a canvas in color
*****************************************************************************/

void win_draw_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    pw_text(window[which-FIRST_WINDOW],x,y,PIX_SRC|PIX_COLOR(color),
			canvas_font,str);
}

/****************************************************************************
*   Write a string to a canvas in reverse video
*****************************************************************************/

void draw_inverse_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    pw_writebackground(window[which-FIRST_WINDOW], x, y - char_ascent-1,
			char_width*strlen(str),char_height+2,
			PIX_SRC|PIX_COLOR(color));
    pw_ttext(window[which-FIRST_WINDOW], x, y, PIX_SRC, canvas_font, str);
}

/****************************************************************************
*   Clear entire canvas
*****************************************************************************/

void clear_window(which)
int which;
{
    int width, height;
    void clear_area();

    width = (int)window_get(object[which],WIN_WIDTH);
    height = (int)window_get(object[which],WIN_HEIGHT);
    if (which == PLOT_WIN)
      clear_area(which,0,0,width+1,height+1);
    else
      clear_area(which,0,0,width,height);
}

/****************************************************************************
*   Clear area of a canvas
*****************************************************************************/

void clear_area(which, x1, y1, x2, y2)
int which, x1, y1, x2, y2;
{
    pw_writebackground(window[which-FIRST_WINDOW],x1,y1,x2,y2,PIX_SRC);
}

/******************************************************************************
*	Change GC to draw in XOR mode.
******************************************************************************/
void xor_mode(which)
int which;
{
    xor_flag = TRUE;
}

/******************************************************************************
*	Change GC to draw in Normal mode.
******************************************************************************/
void normal_mode(which)
int which;
{
    xor_flag = FALSE;
}

void screen_dims(width, height)
int *width, *height;
{
/*    caddr_t win;
    Rect *screen;
    win = (Frame)window_create( NULL, FRAME,
            WIN_X,          0,
            WIN_Y,          0,
            WIN_WIDTH,      1,
            WIN_HEIGHT,     1,
            NULL);

    screen = (Rect *)window_get(win,WIN_SCREEN_RECT);
    *height = screen->r_height;
    *width = screen->r_width;
    window_destroy(win);*/
    *width=1152;
    *height=900;
}
