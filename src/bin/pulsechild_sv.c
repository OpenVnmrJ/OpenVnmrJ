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
 This is the child program of pulsetool.c, and controls everything
 having to do with directory listings, files, and editing of files.
 The on-line help manual is also contained in and controlled by this
 program.
************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <strings.h>
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
#include "pulsechild.h"

#define MAX_ENTRIES        800
#define MAX_ENTRY_LENGTH   100
#define CHAR_GAP 4

#ifdef sparc
#define BLINK_TIME         10
#else
#define BLINK_TIME         1
#endif sparc

#define FILE_WIN_ROWS	15   /* number of text rows in the file browser win */

static caddr_t	object[NUM_OBJECTS];
static Pixwin		*window;
static Pixfont		*font;

int			depth;		/* pixel depth for display */
int			color_display;
char			*display_name=NULL;/* name of display */

static Notify_value     pipe_reader();

#define NUM_COLORS 2
static u_char	red[]   = {255,  0},
		green[] = {255,  0},
		blue[]  = {255,230};

static struct singlecolor   foreground;
static struct singlecolor   background;
extern int		char_width, char_height, char_ascent, char_descent;

/*************************************************************************
*  init_window():  
*************************************************************************/
void init_window(argc,argv)
int argc;
char *argv[];
{
    Notify_value pipe_reader();
    static int my;
    static int *me = &my;

    notify_set_input_func(me, pipe_reader, 0);
}

/*****************************************************************************
*       Set up font and return character string with font name
******************************************************************************/
void setup_font(font_name)
char **font_name;
{
    font = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.16");
    *font_name = (char *)malloc(sizeof(char)*12);
    strcpy(*font_name,"screen.r.16");
    char_height = font->pf_defaultsize.y;
    char_width = font->pf_defaultsize.x;
    char_ascent = (int)(0.8*char_height+0.5);
    char_descent = char_height - char_ascent;
}

int get_canvas_width(which)
int which;
{
    return((int)window_get(object[which],CANVAS_WIDTH));
}

int get_canvas_height(which)
int which;
{
    return((int)window_get(object[which],CANVAS_HEIGHT));
}

/*****************************************************************************
*       Set up colormap
******************************************************************************/
void setup_colormap()
{
    foreground.red = 0; foreground.green = 0; foreground.blue = 230;
    background.red = 255; background.green = 255; background.blue = 255;
}

/*****************************************************************************
*       Create gc 
******************************************************************************/
void create_gc()
{
}

/*****************************************************************************
*       Create button at (x,y) with label and notify proc attached.
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
*       Create choice buttons at (x,y) with label strings and notify_proc
*	attached.
******************************************************************************/
void create_choice_button(which,parent,x,y,label,num_strings,strings,
                        notify_proc)
int which,parent;
int x, y;
char *label;
int num_strings;
char *strings[];
int (*notify_proc)();
{
    int i;

    object[which] = panel_create_item(object[parent], PANEL_CHOICE,
                        WIN_X, x, WIN_Y, y,
                        PANEL_LABEL_STRING, label,
                        PANEL_CHOICE_STRING, 0, strings[0],
                        PANEL_NOTIFY_PROC, notify_proc,
                        NULL);
    for (i=1;i<num_strings;i++)
      panel_set(object[which],PANEL_CHOICE_STRING,i,strings[i],NULL);
}

/*****************************************************************************
*       Given column number, return pixel value.
******************************************************************************/
int column(which,col)
int which, col;
{
    return(ATTR_COL(col));
}

/*****************************************************************************
*       Given row number, return pixel value.
******************************************************************************/
int row(which,r)
int which, r;
{
    return(ATTR_ROW(r));
}

/*****************************************************************************
*       Create panel area at (x,y) of size width by height.
******************************************************************************/
void create_panel(which,parent,x,y,width,height)
int which,parent;
int x, y, width, height;
{
    Pixwin *pw;

    object[which] = window_create(object[parent], PANEL,
            WIN_X,          x,
            WIN_Y,          y,
            WIN_WIDTH,      width,
            WIN_HEIGHT,     height,
            WIN_FONT,      font,
            WIN_ROW_GAP,   -10,
            NULL);
}

/*****************************************************************************
*       Create textsw
******************************************************************************/
void create_sw(which,parent,rows,cols,filename)
int which, parent;
int rows, cols;
char *filename;
{
    object[which] =
        window_create(object[parent], TEXTSW,
	    TEXTSW_LOWER_CONTEXT, -1,
	    TEXTSW_DISABLE_CD, TRUE,
            TEXTSW_FILE, filename,
	    TEXTSW_DISABLE_LOAD, TRUE,
	    TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
	    WIN_ROWS,  rows,
	    WIN_COLUMNS,  cols,
            WIN_FONT, font,
            FRAME_INHERIT_COLORS, TRUE,
            NULL);
}

/*****************************************************************************
*       Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame(which,parent,label,x,y,width,height)
int which, parent;
char *label;
int x, y, width, height;
{
    object[which] =
        (Frame)window_create(object[parent], FRAME,
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
*       Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame_with_event_proc(which,parent,label,x,y,width,height,ev_proc)
int which, parent;
char *label;
int x, y, width, height;
int (*ev_proc)();
{
    object[which] =
        (Frame)window_create( parent, FRAME,
            FRAME_LABEL,    label,
            WIN_X,          x,
            WIN_Y,          y,
            WIN_WIDTH,      width,
            WIN_HEIGHT,     height,
            WIN_FONT, font,
	    WIN_SHOW, FALSE,
            FRAME_INHERIT_COLORS, TRUE,
            NULL);
}

/*****************************************************************************
*       Create canvas at (x,y) of size width by height.  Attach an event
*       handler and a repaint process.
******************************************************************************/
void create_canvas(which,parent,x,y,width,height,repaint_proc,event_proc)
int which, parent, x, y, width, height;
int (*repaint_proc)(), (*event_proc)();
{
    void create_gc();

    object[which] = window_create(object[parent], CANVAS,
                WIN_X,  x,
                WIN_Y,  y,
                WIN_FONT, font,
                WIN_WIDTH,      width,
                WIN_HEIGHT,     height,
                CANVAS_REPAINT_PROC, repaint_proc,
                WIN_EVENT_PROC, event_proc,
                NULL);
    window = canvas_pixwin(object[which]);
    depth = window->pw_pixrect->pr_depth;
    if (depth > 1)  {
      color_display = TRUE;
      pw_setcmsname(window,"pc_colormap");
      pw_putcolormap(window,0,NUM_COLORS,red,green,blue);
      }
}

/*****************************************************************************
*       Set object which below other.
******************************************************************************/
void set_win_below(which, other)
int which, other;
{
    window_set(object[which],WIN_BELOW, object[other], NULL);
}

/*****************************************************************************
*       Return width of Panel object.
******************************************************************************/
int object_width(which)
int which;
{
    Rect *item;
 
    item = (Rect *)panel_get(object[which],PANEL_ITEM_RECT);
    return((int)item->r_width+5);
}
 
/*****************************************************************************
*       set width of Panel.
******************************************************************************/
int set_width(which,width)
int which,width;
{
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
*       Show object.
******************************************************************************/
void show_object(which)
{
    window_set(object[which],WIN_SHOW,TRUE,NULL);
}

/*****************************************************************************
*       Hide object.
******************************************************************************/
void hide_object(which)
{
    window_set(object[which],WIN_SHOW,FALSE,NULL);
}

/*****************************************************************************
*       Hide frame label.
******************************************************************************/
void hide_frame_label(which)
{
    window_set(object[which],FRAME_SHOW_LABEL,FALSE,NULL);
}

/*****************************************************************************
*       Get panel value.
******************************************************************************/
win_val get_panel_value(which)
int which;
{
    return((win_val)panel_get(object[which],PANEL_VALUE));
}

/*****************************************************************************
*       Given a window object, find it's index in "objects" array.
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
*       Do frame fit operation to resize frame to fit contents.
******************************************************************************/
void fit_frame(which)
int which;
{
   window_fit(object[which]);
}

/*****************************************************************************
*       Do frame fit operation to resize frame to fit contents in vertical
*       direction.
******************************************************************************/
void fit_height(which)
int which;
{
   window_fit_height(object[which]);
}

/*****************************************************************************
*       loop on BASE_FRAME.
******************************************************************************/
void window_start_loop(which)
int which;
{
    window_fit(object[which]);
    window_main_loop(object[which]);
}

/*****************************************************************************
*       Tell window system to do all pending draw requests NOW.
******************************************************************************/
void flush_graphics()
{
}

/*****************************************************************************
*       Pop up confirm box with "Yes" or "No" options.
******************************************************************************/
int notice_yn(which,str)
int which;
char *str;
{
    int val;
    val = alert_prompt(NULL,NULL,
                ALERT_MESSAGE_STRINGS, str, NULL,
                ALERT_BUTTON_YES, "Yes",
                ALERT_BUTTON_NO, "No",
                NULL);
    if (val == ALERT_NO)
      return(FALSE);
    else
      return(TRUE);
}

/*****************************************************************************
*       Pop up informational box.
******************************************************************************/
void notice_prompt_ok(which,str)
int which;
char *str;
{
    alert_prompt(NULL,NULL,
                ALERT_MESSAGE_STRINGS, str, NULL,
                ALERT_BUTTON_YES, "Ok",
                NULL);
}

/*****************************************************************
*  This routine reads data coming from the parent Pulsetool,
*  in 512 byte blocks, taking the appropriate action based on
*  the string received from a successful read.
*****************************************************************/
static Notify_value
pipe_reader(unused, fd)
int   *unused;
int   fd;
{
    char  buf[512];
    int   nread;

    nread = read(fd, buf, 512);
    if (nread > 0)
	buf[nread] = '\0';
    switch(nread) {
	case -1:
	    perror("(child)pipe_read: read");
	    exit(1);
        case  0:
	    perror("(child)pipe_read: read(EOF)");
	    exit(1);
        default:
	    if (do_pipe_reader(buf))
	      return(NOTIFY_DONE);
	}
}

/*****************************************************************************
*	process events on base frame - just hide it when the program starts 
*****************************************************************************/
void base_frame_event_proc()
{
}

/*****************************************************************
*  This routine displays a window containing help information
*  about all of the Pulsetool capabilities.  It is initially
*  called from pipe_reader, and then by either the "Done" button
*  in the help window, or by one of the help choices in the 
*  window.  Each time a new help selection is made, the old 
*  message items used to display the previous text are destroyed,
*  and new ones created for the new text.  Each block of text
*  must end with the line "END".
*****************************************************************/
void help_proc(item, event)
Panel_item   item;
Event        *event;
{
    int   obj, choice;

    obj = find_object(item);
    choice = (int)get_panel_value(HELP_CHOICE_BUTTONS);
    do_help_proc(obj,choice);
}

/********************************************************************
*	show text for new help catagory
********************************************************************/
void show_text(which,num,txt)
int which, num;
char *txt[];
{
    int i;

    window_set(object[which],TEXTSW_BROWSING,FALSE,
                       TEXTSW_INSERT_MAKES_VISIBLE, FALSE,NULL);
    textsw_erase(object[which],0,TEXTSW_INFINITY);
    window_set(object[which],TEXTSW_INSERTION_POINT,0,NULL);
    for (i=0;i<num;i++)
      textsw_insert(object[which],txt[i],strlen(txt[i]));
    window_set(object[which],TEXTSW_BROWSING,TRUE,NULL);
}

/*****************************************************************
*	kills all windows
*****************************************************************/
void quit_proc()
{
    char   cmdstring[64];

    window_set(object[BASE_FRAME], FRAME_NO_CONFIRM, TRUE, 0);
    window_destroy(object[BASE_FRAME]);
}

/*****************************************************************
*  This routine is called by the "Done", "Chdir", "Parent",
*  and "Load" buttons, and also when the "Directory" and
*  "Pulse Name" text items are modified in the parent pulsetool.
*  In the case of the first three, all of the PANEL_TEXT items
*  are first destroyed, so that the next directory listing does
*  not continue to create more and more items.
*****************************************************************/
void file_control(item)
Panel_item   item;
{
    int          counter=0;
    char         new_name[64], string[512];
    Panel_item   file_selection;

    do_file_control(find_object(item));
}
/************************************************************************
*  Display the directory listing in the "files_panel" popup.
*  Each time this routine is called, "entry_counter" filenames
*  are written row by row to a canvas.  If all rows don't fit
*  on one screen, allows paging forward or back to see all filenames.
*************************************************************************/
void display_directory_list(canvas, pixwin, repaint_area)
Canvas canvas;
Pixwin *pixwin;
Rectlist *repaint_area;
{
    do_display_directory_list();
}

/****************************************************************************
*  Process events in the file browser window.  Only events we need to consider
*  are configuration events and mouse down events.  
*  Call display_directory_list() to redraw the window.
*****************************************************************************/

void directory_event_proc(canvas, event)
Canvas canvas;
Event *event;
{
    int button_down = FALSE,
	resize = FALSE,
 	x = 0, y = 0, ev;

    if (((ev = event_action(event)) == MS_LEFT) && (event_is_down(event)))  {
      button_down = TRUE;
      x = event_x(event);
      y = event_y(event);
      }
    do_directory_event_proc(button_down, x, y, resize);
}

/****************************************************************************
*   Write a string to a canvas
*****************************************************************************/

void draw_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    int width, height;

    width = font->pf_defaultsize.x*strlen(str);
    height = font->pf_defaultsize.y;
    pw_writebackground(window, x, y - height, width,height + 2,PIX_SRC);
    pw_text(window, x, y, PIX_SRC, font, str);
}

/****************************************************************************
*   Write a string to a canvas in reverse video
*****************************************************************************/

void draw_inverse_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    pw_writebackground(window, x, y - char_ascent-1,char_width*strlen(str),
				char_height+2,PIX_SRC|PIX_COLOR(1));
    pw_ttext(window, x, y, PIX_SRC, font, str);
}

/****************************************************************************
*   Get width of a string
*****************************************************************************/

int get_string_width(str)
char *str;
{
    int width;
    width=char_width*strlen(str);
    return(width);
}

/****************************************************************************
*   Clear area of a canvas
*****************************************************************************/

void clear_area(which, x1, y1, x2, y2)
int which, x1, y1, x2, y2;
{
    pw_writebackground(window, x1, y1, x2, y2,PIX_SRC);
}

/****************************************************************************
*   Clear entire canvas
*****************************************************************************/

void clear_window(which)
int which;
{
    pw_writebackground(window, 0, 0,
		(int)window_get(object[FILES_CANVAS],WIN_WIDTH),
		(int)window_get(object[FILES_CANVAS],WIN_HEIGHT),
		PIX_SRC);
}
