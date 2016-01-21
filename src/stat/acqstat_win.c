/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#ifdef MOTIF
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/DrawingAP.h>
#include <Xm/List.h>
#include <Xm/Protocols.h>
#include "acqstat.icon"
#include "acqsetup.icon"
#define Frame        Widget
#define Canvas       Widget
#define Panel        Widget
#define Panel_item   Widget
#define BORDERWIDTH     XmNborderWidth
#define TEXTWIDGET      xmLabelWidgetClass
#endif

#include "acqstat_item.h"

#define FRAME_WIDTH  45
#define DISCONNECTSIZ 14
#define RIGHT_ADJ  1

#define SAVE  	1
#define LOAD  	2
#define REMOVE 	3

extern  int  debug, acq_ok, useInfostat;
extern  char RemoteHost[]; 
extern  long IntervalTime;
extern  void exitproc();

#ifdef MOTIF
Frame   frame;
Panel   commandpanel;
Panel   statusPanel;

Panel_item quitbutton;
Panel_item pan_items[LASTITEM+1];
#endif

static  int     chWidth = 9;
static  int	chHeight = 15;



#define  HGAP	4
#define  VGAP   4
#define  COLORLEN   24

#define  IBMSVR  0
#define  SUNSVR  1
#define  SGISVR  2

static  int     n, screen;
static  int     screenHeight = 1;
static  int     screenWidth = 1;
static  int     winWidth;
static  int     winHeight;
static  int     spanelHeight = 1;
static  int	use_def_file = 0;
static  int	ch2Width = 9;
static  int     ch2Height = 15;
static  int	ch_ascent, ch2_ascent;
static  int	rowHeight, charWidth = 0;
static  int     setupActive = 0;
static  int     setup_item = -1;
static  int     shell_x, shell_y, shell_w, shell_h;
static  int     decoW, decoH;  /*  window decoration */
static  int     hlit_x, hlit_y, hlit_w, hlit_h, hlit_item;
static  int	hdif_x, hdif_y;
static  int	buttomPress = 0;
static  int	xserver = 0;
static  int	xrelease;
static  char    labelFontName[120];
static  char    valueFontName[120];
static  char    oldl_FontName[120];
static  char    oldv_FontName[120];
static  char    winBgName[COLORLEN];
static  char    old_BgName[COLORLEN];
static  char    winFgName[COLORLEN];
static  char    framelabel[80];
static  char    setup_file[128];
static  char    hostName[40];
#ifdef MOTIF
static  GC      labelGc = NULL, valueGc = NULL; 
static  GC	hlitGc = NULL;
static  Font	label_font= NULL, value_font= NULL;
static  Font    def_font = NULL;
static  Atom	deleteAtom;
static  Pixel	winBg, winFg, focusPix;
static  Pixel	hlit_pix, redPix, greenPix, bluePix;
static  Widget  statusShell = NULL;
static  Window  statusWin;
static  Widget  setupShell = NULL;
static  Window  setupID = NULL;
static  Widget  setupWin = NULL;
static  Widget  geomWidget, bgWidget, bfontWidget, vfontWidget;
static  Widget  fgWidget, titleWidget, hostWidget;
static  Widget  setBut, itemWidget, labelWidget;
static  Widget  colorW, colorW2;
static  Widget  showWidget, noWidget;
static  Widget  item_menu;
static  Widget  messWidget;
static  Widget  fileShell = NULL;
static  Window  fileID = NULL;
static  Widget  fileWin, loadScrWin, fileWidget;
static  Widget  warningShell = NULL;
static  Window  warnId = NULL;
static  Widget  errWidget, errWidget2;
static  Widget  warnBut1, warnBut2, warnBut3;
static  Widget  sep1, sep2;
static  Widget  menuShell;
static  Cursor  activeCursor = NULL;
static  Display  *dpy;
static  Colormap cmap;
static  XFontStruct  *labelFontInfo = NULL;
static  XFontStruct  *valueFontInfo = NULL;
static  XFontStruct  *defFontInfo = NULL;
static  Dimension    butHeight;
static  Dimension    width, height;
static  Dimension    warnWidth;
Arg     args[24];
char    tmpstr[250];
char    errstr[120];
Window  shellId;
XWindowAttributes win_attributes;

static  char *color_list[5] = {"LightGray", "red", "blue", "black", "white"};
static  char *fcolor_list[5] = {"black", "white", "red", "blue", "brown"};
static  char *font_list[5] = {"8x13", "9x15", "courb24", "charb24", "courb36.iso4" };
static  char *vfont_list[5] = {"8x13", "9x15", "courb24", "charb24", "courb36.iso4" };

#define XtNset          XmNset
#define DeleteListItem  XmListDeleteAllItems
#define AddListItem     XmListAddItem
static  XmString        xmstr;
static  XmFontList      warnFontInfo;

#endif
static int (*save_load_proc)() = NULL;
extern DoTheChores();
char   *get_text_item();

#define  NONE	0
#define  ON	1
#define  OFF	2


#ifdef MOTIF
typedef  struct _item_rec {
	 int	def;
	 int	loc_x, loc_y;
	 int	x, y, length;
	 int	dloc_x, dloc_y;
	 int	show, dshow, sshow;  /* ON, OFF, NONE */
	 int	name_len, dname_len, sname_len;
	 int    width, full_width;
	 char	*name;
	 char	*dname;
	 char	*sname;
	 char	color[COLORLEN];
	 char	dcolor[COLORLEN];
	 char	scolor[COLORLEN];
	 Pixel  pix;
	 Widget widget;
	} item_rec;

item_rec   acq_items[LASTITEM+1];

typedef  struct _item_order {
	int	 id;
	int	 x, y, x2;
	struct _item_order   *prev;
	struct _item_order   *next;
	} item_order;

static item_order  *item_list = NULL;

typedef  struct _def_rec {
	int	id;
	char    *name;
	struct _def_rec  *next;
	} def_rec;

static  def_rec   *def_list = NULL;

static  FILE  *fd = NULL;


static Pixel
query_pixel(name)
char  *name;
{
	XColor xcol1, xcol2;

	if (useInfostat == 0)
	   return(9999);
	if(XAllocNamedColor(dpy, cmap, name, &xcol1, &xcol2))
	   return(xcol1.pixel);
	else
	   return(9999);
}



Widget 
create_button(parent, label, num, func)
Widget  parent;
char    *label;
int	num;
void    (*func)();
{
	Widget  button;

        n =0;
        xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
	XtSetArg (args[n], XmNtraversalOn, FALSE);  n++;
        button = (Widget)XmCreatePushButton(parent, "button", args, n);
        XtManageChild (button);
        XtAddCallback(button, XmNactivateCallback, func, (XtPointer)num);
/*
        XtFree(xmstr);
*/
        XmStringFree(xmstr);
	return(button);
}

Widget 
create_row_col(parent, vertical, pad, space)
Widget  parent;
int	vertical;
int	pad, space;
{
	Widget  row_col;

	n = 0;
	if (vertical)
	{
	    XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	    XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	    XtSetArg (args[n], XmNmarginHeight, 1);  n++;
	}
	else
	{
	    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	    XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);  n++;
	    XtSetArg(args[n], XmNspacing, space);  n++;
            XtSetArg(args[n], XmNmarginWidth, pad);  n++;
        }
	row_col = (Widget)XmCreateRowColumn(parent, "", args, n);
   	XtManageChild (row_col);
	return(row_col);
}

Widget 
create_rc_widget(parent, topwidget, vertical, pad, space)
int	vertical;
int	pad, space;
{
	Widget   rcwidget;

        n =0;
        XtSetArg(args[n], BORDERWIDTH, 0);  n++;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNresizable, TRUE);  n++;

	if (topwidget != NULL)
	{
	    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
            XtSetArg (args[n], XmNtopWidget, topwidget);  n++;
	}
	else
	{
            XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	}
	if (vertical)
	{
	    XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	    XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	    XtSetArg (args[n], XmNmarginHeight, 1);  n++;
	}
	else
	{
	    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	    XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);  n++;
	    XtSetArg(args[n], XmNspacing, space);  n++;
            XtSetArg(args[n], XmNmarginWidth, pad);  n++;
        }
	rcwidget = (Widget)XmCreateRowColumn(parent, "", args, n);
   	XtManageChild (rcwidget);
	return(rcwidget);
}

static void
show_setup(w, data, ev)
Widget          w;
caddr_t         data;
XEvent          *ev;
{
	if (useInfostat == 0)
                return;
        if (ev->xbutton.button != 3)
                return;
        XmMenuPosition(menuShell, ev);
        XtManageChild(menuShell);
}

Widget
create_separtor(parent, topwidget)
Widget parent, topwidget;
{
	Widget  tw;

	n = 0;
        n =0;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNresizable, TRUE);  n++;
	if (topwidget != NULL)
	{
	    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
            XtSetArg (args[n], XmNtopWidget, topwidget);  n++;
	}
	else
	{
            XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	}
	tw = (Widget)XmCreateSeparatorGadget(parent, "separator", args, n);
	XtManageChild(tw);
	return(tw);
}


static int
set_get_label_item(w, label, fontlist)
Widget   w;
char     *label;
XmFontList  fontlist;
{
	XmString        mstring;
	int	        len;

   	mstring = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[0], XmNlabelString, mstring);
	XtSetValues(w, args, 1);
	if (fontlist)
	    len = (int) XmStringWidth(fontlist, mstring) + 20;
	else
	    len = strlen(label) * 10;
/*
   	XtFree(mstring);
*/
        XmStringFree(mstring);
	return(len);
}



static
set_item_menu(item)
int	item;
{
	show_item_info(item);
	highlit_item(item);
	XtSetArg(args[0], XmNmenuHistory, acq_items[item].widget);
	XtSetValues(item_menu, args, 1);
}



int
setup_default(file)
char    *file;
{
	char   *tpr;

	if (useInfostat == 0)
	    return;
	fd = NULL;
	if ((tpr = (char *)getenv("vnmruser")) != NULL)
	{
		sprintf(tmpstr, "%s/templates/acqstat/%s", tpr, file);
		fd = fopen(tmpstr,"r");
	}
	if (fd == NULL)
	{
	     if (tpr != NULL)
		sprintf(errstr, "%s/templates/acqstat/%s", tpr, file);
	     else
		sprintf(errstr, "%s", file);
	     if ((tpr = (char *)getenv("vnmrsystem")) != NULL)
	     {
		sprintf(tmpstr, "%s/user_templates/acqstat/%s", tpr, file);
                fd = fopen(tmpstr,"r");
	     }
	}
	if (fd != NULL)
	{
	     read_set_default();
	     fclose(fd);
	     return(1);
	}
	else {
	     if (strcmp(file,"default") != NULL)
	     {
	     	disp_error("Could not open file:", 1);
	     	disp_error(errstr, 2);
	     }
	     return(0);
	}
}


void
mainShell_exit(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
	exitproc();
}


void 
error_proc()
{
	XtPopdown(warningShell);
}

void
warning_proc(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
	if ((int) client_data != 0)
	{
		if ( save_load_proc != NULL)
		   save_load_proc();
	}
	save_load_proc = NULL;
	XtPopdown(warningShell);
}
	     



disp_warning(mess, which, warning)
char   *mess;
int    which, warning;
{
	static  Dimension  ww1, ww2;

	if (warningShell == NULL)
	    create_errorWindow(mess);
	if (warningShell)
	{
	    if (warning)
	    {
		XtUnmapWidget(warnBut2);
		XtMapWidget(warnBut1);
		XtMapWidget(warnBut3);
	    }
	    else
	    {
	        XtUnmapWidget(warnBut1);
	        XtUnmapWidget(warnBut3);
	        XtMapWidget(warnBut2);
	    }
	    if (which == 1)
	    {
	       ww1 = set_get_label_item(errWidget, mess, warnFontInfo);
	       XBell(dpy, 30);
	    }
	    else
	    {
	       ww2 = set_get_label_item(errWidget2, mess, warnFontInfo);
	       if (ww1 > ww2)
		   ww2 = ww1;
	       if (ww2 > warnWidth)
                  XtSetArg (args[0], XtNwidth, ww2 + 20);
	       else
                  XtSetArg (args[0], XtNwidth, warnWidth);
	       XtSetValues(warningShell, args, 1);
		
	       XtPopup (warningShell, XtGrabExclusive);
	       XRaiseWindow(dpy, warnId);
	    }
	}
}


disp_error(mess, which)
char   *mess;
int    which;
{
	disp_warning(mess, which, 0);
}
#endif


disp_item(item)
int    item;
{
#ifdef MOTIF
	int	x, y, w, diff;
	static  Pixel  xcolor= 0;
	static  Pixel  xcolor2= 0;

	if (useInfostat == 0)
		return;
	if (acq_items[item].show == NONE)
		return;
	if (labelGc == NULL)
	{
	     labelGc = XCreateGC(dpy, statusWin, 0, 0);
	     if (label_font)
		XSetFont(dpy, labelGc, label_font);
	}
	if (valueGc == NULL)
	{
	     valueGc = XCreateGC(dpy, statusWin, 0, 0);
	     if (value_font)
		XSetFont(dpy, valueGc, value_font);
	}
	x = acq_items[item].x;
	y = acq_items[item].y;
	w = acq_items[item].width;
	if (hlit_item >= 0)
	{
	      diff = abs(hlit_y - y);
	      if (diff <= hlit_h)
	          XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
	}
	XClearArea(dpy, statusWin, x, y, w, rowHeight, FALSE);

	if (acq_items[item].show == ON)
	{
	    if (item > LASTTITLE)
	    {
		if (xcolor2 != acq_items[item].pix)
		{
		    xcolor2 = acq_items[item].pix;
		    XSetForeground(dpy, valueGc, xcolor2);
		}
	        y = y + ch_ascent;
		XDrawString(dpy, statusWin, valueGc, x, y, acq_items[item].name,
                        (int)strlen(acq_items[item].name));
	    }
	    else
	    {
		if (xcolor != acq_items[item].pix)
		{
		    xcolor = acq_items[item].pix;
		    XSetForeground(dpy, labelGc, xcolor);
		}
	    	y = y + ch_ascent;
	   	XDrawString(dpy, statusWin, labelGc, x, y, acq_items[item].name,
                        (int)strlen(acq_items[item].name));
	    }
	}
	if (hlit_item >= 0)
	{
	      if (diff <= hlit_h)
	          XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
	}
#endif
}


set_item_string(item, str)
int    item;
char  *str;
{
#ifdef MOTIF
	int	len;

	if (useInfostat == 0)
		return;
	len = strlen(str);
	if (len < 0)
	   len = 0;
	if (len > acq_items[item].name_len)
	   len = acq_items[item].name_len;
/*
	strncpy(acq_items[item].name, str, acq_items[item].name_len);
*/
	strncpy(acq_items[item].name, str, len);
	acq_items[item].name[len] = '\0';
	disp_item(item);
#endif
}


show_item(item, on)
int   item, on;
{
#ifdef MOTIF
	if (useInfostat == 0)
		return;
	if( acq_items[item].show == on)
		return;
	if (acq_items[item].show != NONE)
	{
	    if (on)
	        acq_items[item].show = ON;
	    else
	        acq_items[item].show = OFF;
	    disp_item(item);
	}
#endif
}


#ifdef MOTIF
static void
expose_items(widget, client_data, ev)
Widget          widget;
caddr_t         client_data;
XEvent          *ev;
{
        int         item, vitem, old_item;
	item_order  *p_list, *c_list;

	if (useInfostat == 0)
		return;
	old_item = hlit_item;
	if (hlit_item >= 0)
	{
	     hlit_item = -1;
	     XClearWindow(dpy, statusWin);
	}
        if (item_list == NULL)
        {
             p_list = NULL;
             for(item = FIRSTTITLE; item <= LASTTITLE; item++)
             {
		 vitem = item + LASTTITLE;
                 c_list = (item_order *) malloc(sizeof(item_order));
                 c_list->id = item;
                 c_list->x = acq_items[item].x;
                 c_list->y = acq_items[item].y;
		 acq_items[vitem].x = acq_items[item].x + acq_items[item].width;
	         acq_items[item].full_width = acq_items[item].width + 
				acq_items[vitem].width;
                 c_list->x2 = acq_items[item].x + acq_items[item].full_width;
                 c_list->prev = p_list;
                 c_list->next = NULL;
                 if (item_list == NULL)
                    item_list = c_list;
                 else
                    p_list->next = c_list;
                 p_list = c_list;
             }
        }

	redraw_items();
	if (old_item >= 0)
	      highlit_item(old_item);
}




redraw_items()
{
	int	     item;
	item_order  *c_list;

	if (useInfostat == 0)
		return;
	c_list = item_list;
	while (c_list != NULL)
	{
	     item = c_list->id;
	     if (acq_items[item].show == ON)
	     {
	    	   disp_item(item);
	    	   disp_item(item + LASTTITLE);
	     }
	     c_list = c_list->next;
	}
}



create_item(item, col, row, label, len, dir)
int 	item, col, row, len, dir;
char   *label;
{

	if (useInfostat == 0)
		return;
    	if (!acq_items[item].def)
	{
	    if (len <= 0)  /* it is label */
	    {
	    	len = (int)strlen(label) + 1;
	    	acq_items[item].loc_x = col - len;
	    }
	    else
	        acq_items[item].loc_x = col;
	    acq_items[item].loc_y = row;
	    acq_items[item].name = (char *)malloc(len + 2);
	    acq_items[item].name_len = len;
	    acq_items[item].length = len;
	    strcpy(acq_items[item].name, label);

	    acq_items[item].dloc_x = acq_items[item].loc_x;
	    acq_items[item].dloc_y = row;
	    acq_items[item].dname = (char *)malloc(len + 2);
	    acq_items[item].dname_len = len;
	    strcpy(acq_items[item].dname, label);
	}

	if (acq_items[item].dshow == ON)
	{
	    acq_items[item].show = ON;
	    acq_items[item].sshow = ON;
	}

	acq_items[item].x = acq_items[item].loc_x * charWidth + HGAP;
	acq_items[item].y = acq_items[item].loc_y * (rowHeight + VGAP) + VGAP;
	if (item > LASTTITLE)
	    acq_items[item].width = acq_items[item].name_len * ch2Width;
	else
	{
	    if (labelFontInfo)
		acq_items[item].width = XTextWidth (labelFontInfo,
		 	acq_items[item].name, strlen(acq_items[item].name)) + 4;
	    else
	        acq_items[item].width = acq_items[item].name_len * chWidth + 4;
	}
}

cal_item_width()
{
        int    item;
	item_order  *clist;

	if (useInfostat == 0)
		return;
        for(item = FIRSTTITLE; item <= LASTTITLE; item++)
        {
	   acq_items[item].full_width = acq_items[item].width + 
				acq_items[item+LASTTITLE].width;
	}
	clist = item_list;
	while (clist != NULL)
	{
	   clist->x2 = acq_items[clist->id].x + acq_items[clist->id].full_width;
	   clist = clist->next;
	}
}


cal_item_loc()
{
        int    m, m2;
	item_order  *clist;

	if (useInfostat == 0)
		return;
	clist = item_list;
	while (clist != NULL)
	{
	     m = clist->id;
	     m2 = m + LASTTITLE;
	     acq_items[m].x = acq_items[m].loc_x * charWidth + HGAP;
             acq_items[m].y = acq_items[m].loc_y * (rowHeight + VGAP) + VGAP;
	     if (labelFontInfo)
		acq_items[m].width = XTextWidth (labelFontInfo,
			acq_items[m].name, strlen(acq_items[m].name)) + 4;
	     else
	        acq_items[m].width = acq_items[m].name_len * chWidth + 4;
	     acq_items[m2].x = acq_items[m].x + acq_items[m].width;
	     acq_items[m2].y = acq_items[m].y;
	     acq_items[m2].width = acq_items[m2].name_len * ch2Width;
	     acq_items[m].full_width = acq_items[m].width + acq_items[m2].width;
	     clist->x = acq_items[m].x;
	     clist->y = acq_items[m].y;
	     clist->x2 = acq_items[m].x + acq_items[m].full_width;
	     clist = clist->next;
	}
}



pick_item_proc(x, y)
int	x, y;
{
	int	    item, len;
	item_order  *c_list;

	if (useInfostat == 0)
		return;
	item = -1;
	highlit_item(-1);
	c_list = item_list;
	while (c_list != NULL)
	{
	      if (c_list->x <= x && x < c_list->x2)
	      {
		   if (c_list->y <= y && y < c_list->y + rowHeight)
		   {
			if (acq_items[c_list->id].show)
			   item = c_list->id;
		   }
	      }
	      c_list = c_list->next;
	}

	if (item >= 0)
	{
	     buttomPress = 1;
	     hdif_x = x - acq_items[item].x;
	     hdif_y = y - acq_items[item].y;
	     set_item_menu(item);
	}
}


stack_top_item(item)
int	item;
{
	item_order  *c_list, *p_list;

	if (useInfostat == 0)
		return;
	p_list = item_list;
	c_list = p_list;
	while (p_list != NULL)
	{
		if (p_list->id == item)
		    c_list = p_list;
		if (p_list->next == NULL)
		    break;
		p_list = p_list->next;
	}
	if (c_list != p_list)
	{
		if (item_list == c_list && c_list->next != NULL)
		     item_list = c_list->next;
		p_list->next = c_list;
		if (c_list->prev != NULL)
		    c_list->prev->next = c_list->next;
		c_list->next->prev = c_list->prev;
		c_list->prev = p_list;
		c_list->next = NULL;
	}
}


put_item_proc(x, y)
int	x, y;
{
	int	locx, locy, oldy, oldx;
	int	pitem, vitem;
	item_order  *c_list, *p_list;

	if (useInfostat == 0)
		return;
	buttomPress = 0;
	pitem = hlit_item;
	highlit_item(-1);
	locx = (hlit_x - HGAP + 1) / charWidth;
	locy = (hlit_y - VGAP + rowHeight * 0.6) / (rowHeight + VGAP);
	if (locx < 0)
		locx = 0;
	if (locy < 0)
		locy = 0;
	oldy = acq_items[pitem].y;
	oldx = acq_items[pitem].x;
	XClearArea(dpy, statusWin, oldx, oldy,
			acq_items[pitem].full_width, rowHeight, FALSE);
	acq_items[pitem].loc_x = locx;
	acq_items[pitem].loc_y = locy;

	vitem = pitem + LASTTITLE;
	acq_items[vitem].loc_x = locx + acq_items[pitem].name_len + 1;
	acq_items[vitem].loc_y = locy;
	
	acq_items[pitem].x = locx * charWidth + HGAP;
	acq_items[pitem].y = locy * (rowHeight + VGAP) + VGAP;
	acq_items[vitem].x = acq_items[pitem].x + acq_items[pitem].width;
	acq_items[vitem].y = acq_items[pitem].y;

	p_list = item_list;
	c_list = p_list;
	while (p_list != NULL)
	{
		if (p_list->id == pitem)
		{
		    c_list = p_list;
		    c_list->x = acq_items[pitem].x;
		    c_list->y = acq_items[pitem].y;
		    c_list->x2 = acq_items[pitem].x + acq_items[pitem].full_width;
		}
		if (p_list->next == NULL)
		    break;
		p_list = p_list->next;
	}
	if (c_list != p_list)
	{
		if (item_list == c_list && c_list->next != NULL)
		     item_list = c_list->next;
		p_list->next = c_list;
		if (c_list->prev != NULL)
		    c_list->prev->next = c_list->next;
		c_list->next->prev = c_list->prev;
		c_list->prev = p_list;
		c_list->next = NULL;
	}

	p_list = item_list;
	/*  draw items under the picked item */
	while (p_list != NULL)
	{
	    if (acq_items[p_list->id].show == ON)
	    {
	      if (p_list->y == oldy && p_list != c_list)
	      {
			disp_item(p_list->id);
			disp_item(p_list->id + LASTTITLE);
	      }
	    }
	    p_list = p_list->next;
	}
	disp_item(pitem);
	disp_item(vitem);
	highlit_item(pitem);
}



move_hlit_proc(x, y)
int	x, y;
{
	int	hx, hy;

	hx = x - hdif_x;
	hy = y - hdif_y;
	if (hx + hlit_w >= winWidth)
	     hx = winWidth - hlit_w - 1;
	if (hx < HGAP)
	     hx = HGAP;
	if (hy + hlit_h >= winHeight)
	     hy = winHeight - hlit_h - 1;
	if (hy < VGAP)
	     hy = VGAP;
	
	if (hx == hlit_x && hy == hlit_y)
	     return;
	XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
	hlit_x = hx;
	hlit_y = hy;
	XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
}



void
button_proc(w, data, ev)
Widget          w;
caddr_t         data;
XEvent          *ev;
{
	if (useInfostat == 0)
		return;
	switch (ev->type) {
        case   ButtonPress:
		if (ev->xbutton.button != 1)
			return;
		pick_item_proc(ev->xbutton.x, ev->xbutton.y);
		break;

	case   ButtonRelease:
		if (ev->xbutton.button != 1)
			return;
		if ( !buttomPress || hlit_item < 0)
			return; 
		put_item_proc(ev->xbutton.x, ev->xbutton.y);
		break;

	case   MotionNotify:
		if ( !buttomPress || hlit_item < 0)
			return; 
		move_hlit_proc(ev->xmotion.x, ev->xmotion.y);
		break;

	case   LeaveNotify:
		if ( !buttomPress || hlit_item < 0)
			return; 
		highlit_item(-1);
		break;
	}
}


void
setproc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	EventMask    mask;

	if (useInfostat == 0)
		return;
	XRaiseWindow(dpy, shellId);
	if (setupShell == NULL)
	{
	     create_setupWindow();
	     if (hlitGc == NULL)
	     {
	         hlitGc = XCreateGC(dpy, statusWin, 0, 0);
	         XSetFunction(dpy, hlitGc, GXxor);
	         XSetForeground(dpy, hlitGc, hlit_pix);
	     }
	}
	if (setupShell != NULL && hlitGc != NULL)
	{
	     XtPopup (setupShell, XtGrabNone);
	     XRaiseWindow(dpy, setupID);
	     setupActive = 1;
	     if (activeCursor == NULL)
		activeCursor = XCreateFontCursor(dpy, XC_hand2);
	     if (activeCursor)
		XDefineCursor(dpy, statusWin, activeCursor);
	     mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask | LeaveWindowMask;
	     cal_item_width();
	     show_item_info(setup_item);
	     highlit_item(setup_item);
	     XtAddEventHandler(statusPanel,mask, False, button_proc, NULL);
	}
}


/*------------------------------------------------------------------
|
|       create_Bframe()
|       create the base frame and command panel of buttons for
|       the acquisition status display
+------------------------------------------------------------------*/
char    *nargv[20];
char    *geomp = "-geometry";
char    *backp = "-bg";
char    geom[32];
int	nargc;

create_Bframe(argc, argv)
int     argc;
char   **argv;
{
	Dimension   height, width;
        int n, isRemote;
	char   *vendor;
	XrmDatabase dbase;
	unsigned long   val;
	void setup_set_proc();

	strcpy(geom, "-0+0");
	strcpy(winBgName, "snow2");
	strcpy(old_BgName, "XX");
	strcpy(winFgName, "black");
	strcpy(labelFontName, "9x15");
	strcpy(oldl_FontName, "xx");
	strcpy(valueFontName, "9x15");
	strcpy(oldv_FontName, "xx");
	strcpy(setup_file, "default");
	framelabel[0] = '\0';
	hostName[0] = '\0';
	n = 1;
	isRemote = 0;
	while (n < argc)
	{
	     if (strcmp(argv[n], "-f") == 0)
	     {
		if (n < argc - 1 && argv[n+1] != NULL)
			strcpy(setup_file, argv[n+1]);
		n++;
	     }
             else if (strcmp("-fn", argv[n]) == 0)
	     {
		if (n < argc - 1 && argv[n+1] != NULL)
		{
		     strcpy(valueFontName, argv[n+1]);
		     strcpy(labelFontName, argv[n+1]);
		}
		n++;
	     }
	     else
	     {
		if (strcmp(argv[n], RemoteHost) == 0)
		   isRemote = 1;
	     }
	     n++;
	}

	init_defaults();
	if (isRemote)
	     strcpy(hostName, RemoteHost);
	else if (hostName[0] != '\0')
	     strcpy(RemoteHost, hostName);

	nargv[0] = argv[0];
	nargv[1] = geomp;
	nargv[2] = geom;
	nargv[3] = backp;
	nargv[4] = winBgName;
	nargc = 5;
	n = 1;
	while (n < argc && nargc < 19)
	    nargv[nargc++] = argv[n++];
	nargv[nargc] = NULL;

	statusShell = XtInitialize("Acqstat", "Acqstat", NULL, 0,
                          (Cardinal *)&nargc, nargv);

	if ((int)strlen(framelabel) <= 0)
            sprintf(framelabel,"%s ACQUISITION STATUS",RemoteHost);
        n = 0;

	if (useInfostat == 0)
	{
            XtSetArg (args[n], XtNmappedWhenManaged, FALSE); n++;
	}
        XtSetArg (args[n], XtNtitle, framelabel); n++;
	XtSetValues(statusShell,args,n);
        dpy = XtDisplay(statusShell);
        screen = DefaultScreen(dpy);
        screenHeight = DisplayHeight (dpy, screen);
        screenWidth = DisplayWidth (dpy, screen);
        XtSetArg(args[0], XtNiconPixmap, XCreateBitmapFromData (dpy,
                                    XtScreen(statusShell)->root, acq_icon_bits,
                                    acq_icon_width, acq_icon_height));
        XtSetValues (statusShell, args, 1);
	cmap = XDefaultColormap(dpy, screen);

	vendor = ServerVendor(dpy);
	if (vendor != NULL)
	{
	    while (*vendor == ' ' || *vendor == '\t')
		vendor++;
	    if (strncmp(vendor, "International", 13) == 0)
                xserver = IBMSVR;
	    else if (strncmp(vendor, "Silicon", 7) == 0)
		xserver = SGISVR;
	    else
	    {   
		if (strncmp(vendor, "Sun", 3) == 0)
                    xserver = SUNSVR;
	        else if (strncmp(vendor, "X11/NeWS", 8) == 0)
                    xserver = SUNSVR;
	    }
	    xrelease = VendorRelease(dpy);
	}

        n = 0;

	XtSetArg (args[n], XmNx, 0);  n++;
	XtSetArg (args[n], XmNy, 0);  n++;
	XtSetArg (args[n], XmNwidth, 300);  n++;
	XtSetArg (args[n], XmNheight, 200);  n++;
	frame = XtCreateManagedWidget("Acqstat", xmFormWidgetClass,
                                statusShell, args, n);
	butHeight = 20;
	set_font();
	setup_set_proc(0, 0, 0);
	redraw_items();
}

void
resize_window(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
        Dimension       width, height;

        n = 0;
        XtSetArg(args[n], XtNwidth, &width);  n++;
        XtSetArg(args[n], XtNheight, &height);  n++;
        XtGetValues(statusPanel, args, n);
	winHeight = height;
	winWidth = width;
}


static Pixel
get_focus_pixel(back)
  Pixel   back;
{
        int      m;
        int      red, green, blue;
        XColor   xcolor;

	if (useInfostat == 0)
	   return(0);
        xcolor.pixel = back;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XQueryColor(dpy, cmap, &xcolor);
        red = xcolor.red >> 8;
        green = xcolor.green >> 8;
        blue = xcolor.blue >> 8;
        if (red > 120 || green > 120 || blue > 120)
            m = -1;
        else
            m = 1;
        while(xcolor.pixel == back)
        {
              if(red > 3)
                  red += m;
              if(green > 3)
                  green += m;
              if(blue > 3)
                  blue += m;
              xcolor.red = red << 8;
              xcolor.green = green << 8;
              xcolor.blue = blue << 8;
              XAllocColor(dpy, cmap, &xcolor);
        }
        return( xcolor.pixel);
}



static
create_pull_down_menu()
{
	Widget  pwidget, mbut;

	if (useInfostat == 0)
	   return;
	if (debug)
	    fprintf(stderr, "  create_pull_down_menu \n");
	menuShell = (Widget) XmCreatePopupMenu(statusPanel, "Menu", NULL, 0);

	xmstr = XmStringCreate("Properties...", XmSTRING_DEFAULT_CHARSET);
        XtSetArg(args[0], XmNlabelString, xmstr);
        setBut = (Widget)XmCreatePushButtonGadget(menuShell, "",args, 1);
        XtManageChild (setBut);
        XtAddCallback(setBut, XmNactivateCallback, setproc, NULL);
/*
        XtFree(xmstr);
*/
        XmStringFree(xmstr);

	xmstr = XmStringCreate("Exit", XmSTRING_DEFAULT_CHARSET);
        XtSetArg(args[0], XmNlabelString, xmstr);
        quitbutton = (Widget)XmCreatePushButtonGadget(menuShell, "",args, 1);
        XtManageChild (quitbutton);
        XtAddCallback(quitbutton, XmNactivateCallback, exitproc, NULL);
/*
        XtFree(xmstr);
*/
        XmStringFree(xmstr);

	XtSetArg (args[0], XtNheight, &butHeight);
        XtGetValues(quitbutton, args, 1);

	if (debug)
	    fprintf(stderr, "  create_pull_down_menu done\n");
}




create_disp_panel()
{

	if (useInfostat == 0)
	   return;
	if (debug)
	    fprintf(stderr, " create_disp_panel\n");
	n =0;
	XtSetArg (args[n], BORDERWIDTH, 0);  n++;
	XtSetArg (args[n], XmNmarginHeight, 0);  n++;
	XtSetArg (args[n], XmNmarginWidth, 0);  n++;
	XtSetArg (args[n], XmNwidth, 300);  n++;
	XtSetArg (args[n], XmNheight, 200);  n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNresizable, TRUE);  n++;
	statusPanel = XtCreateManagedWidget("text", xmDrawingAreaWidgetClass,
                         frame, args, n);
	n =0;
	XtSetArg (args[n], XmNforeground, &winFg);  n++;
	XtSetArg (args[n], XmNbackground, &winBg);  n++;
	XtGetValues(statusPanel, args, n);

	XtAddEventHandler(statusPanel,ExposureMask, False, expose_items, NULL);
        XtAddEventHandler(statusPanel,StructureNotifyMask, False, resize_window,
                         NULL);
	get_colors();
	set_item_def_color();

	create_pull_down_menu();
	XtAddEventHandler(statusPanel, ButtonPressMask, False, show_setup,NULL);
	if (debug)
	    fprintf(stderr, " create_disp_panel  done\n");
}

void
resize_shell(w, client_data, xev)
  Widget          w;
  XtPointer       client_data;
  XEvent          *xev;
{
	if (useInfostat == 0)
	   return;
	if (xev->type == ConfigureNotify)
	{
	    shell_x = xev->xconfigure.x;
	    shell_y = xev->xconfigure.y;
	    shell_h = xev->xconfigure.height;
	    shell_w = xev->xconfigure.width;
	    cal_mainShell_loc();
	}
}

cal_mainShell_loc()
{
	int	  rx, ry;
	Window    win;
	XWindowAttributes win_attributes;

	if (useInfostat == 0)
	   return;
	XTranslateCoordinates(dpy, shellId, RootWindow(dpy, 0), 0, 0, &rx, &ry, &win);
	if (win != NULL)
	{
	   if (XGetWindowAttributes(dpy, win, &win_attributes))
	   {
		shell_x = win_attributes.x;
		shell_y = win_attributes.y;
		decoW = rx - shell_x;
		decoH = ry - shell_y;
	        if (shell_x < 0)
		    shell_x = 0;
	    	if (shell_y < 0)
		    shell_y = 0;
	        sprintf(geom, "%dx%d+%d+%d", shell_w, shell_h, shell_x, shell_y);
	        if (setupActive)
	            set_label_item(geomWidget, geom);
	   }
	}
}

void
focus_proc(c, data, e)
Widget          c;
caddr_t         data;
XEvent          *e;
{
	static int count = 0;
	XFocusChangeEvent fe;

	if (e->type == FocusIn)
	{
           fe = e->xfocus;
           if (fe.detail != NotifyAncestor)
           {
		XSetInputFocus(dpy, DefaultRootWindow(dpy),
			RevertToPointerRoot, CurrentTime);
		count++;
		if (count > 6)
		   XtRemoveEventHandler(statusShell,FocusChangeMask, False, focus_proc, NULL);
           }
        }
}


fit_win_height()
{
        Dimension       width, height;
	Position        posx, posy;
        int     item, vitem, old_item, mx, my, df;
	item_order  *p_list, *c_list;

	if (useInfostat == 0)
	   return;
	if (debug)
	    fprintf(stderr, " fit_win_height\n");
        if (item_list == NULL)
        {
	     mx = 0;
	     my = 0;
	     df = 0;
             p_list = NULL;
             for(item = FIRSTTITLE; item <= LASTTITLE; item++)
             {
    	         if (acq_items[item].def)
			df = 1;
		 vitem = item + LASTTITLE;
                 c_list = (item_order *) malloc(sizeof(item_order));
                 c_list->id = item;
                 c_list->x = acq_items[item].x;
                 c_list->y = acq_items[item].y;
		 acq_items[vitem].x = acq_items[item].x + acq_items[item].width;
	         acq_items[item].full_width = acq_items[item].width + 
				acq_items[vitem].width;
                 c_list->x2 = acq_items[item].x + acq_items[item].full_width;
                 c_list->prev = p_list;
                 c_list->next = NULL;
		 if (c_list->x2 > mx)
		    mx = c_list->x2;
		 if(c_list->y > my)
		    my = c_list->y;
                 if (item_list == NULL)
                    item_list = c_list;
                 else
                    p_list->next = c_list;
                 p_list = c_list;
             }
	     if (!df)
	     {
		mx = mx + HGAP;
		my = my + rowHeight + VGAP;
	        XtResizeWidget(statusShell, mx, my, 0);
	     }
        }

	XtAddEventHandler(statusShell,FocusChangeMask, False, focus_proc, NULL);
	XtRealizeWidget (statusShell);
	statusWin = XtWindow(statusPanel);
	deleteAtom = XmInternAtom(dpy, "WM_DELETE_WINDOW", FALSE);
	XmAddProtocolCallback(statusShell, XM_WM_PROTOCOL_ATOM(statusShell),
			deleteAtom, mainShell_exit, 0);
	if (debug)
	    fprintf(stderr, " fit_win_height done\n");
}
#endif

/*-------------------------------------------------------------
|  reinittimer/0  reset event timer for count down
|       pass function pointer to be activated with alarm
|
+------------------------------------------------------------*/
reinittimer(timsec,intvl,funccall)
double timsec;
double intvl;
void (*funccall) ();
{
    inittimer(timsec,intvl,funccall);
}


/*-------------------------------------------------------------
|  inittimer/0  set event timer for count down
|       pass function pointer to be activated with alarm
|
+------------------------------------------------------------*/
void (*timerfunc)();
XtIntervalId  timerId = 0;

timerproc()
{
        if(timerId == 0)
            return;
        timerId = 0;
	if (timerfunc != NULL)
            timerfunc();
}

inittimer(timsec,intvl,funccall)
double timsec;
double intvl;
void (*funccall) ();
{
    unsigned long  msec;

    timerfunc = funccall;
    if(timerId != 0)
    {
         XtRemoveTimeOut(timerId);
         timerId = 0;
    }
    msec = timsec * 1000;  /* milliseconds */
    if(funccall != NULL)
         timerId = XtAddTimeOut(msec, timerproc, NULL);
}


acqstat_window_loop()
{
#ifdef MOTIF
    shellId = XtWindow(statusShell);
    if (useInfostat != 0)
        XtAddEventHandler(statusShell,StructureNotifyMask, False, resize_shell,
                         NULL);
#endif
    XtMainLoop();
}

	

#ifdef MOTIF
set_item_attr(num)
int   num;
{
	int	id, oid, k;
	int	x, y, len, show;
	char    *ptr, *ptr2, name[63], color[24];


    	if (useInfostat == 0)
	     return;
	if (num > 200)
	     id = num - 200 + LASTTITLE;
	else
	     id = num - 100;
	ptr = tmpstr;
	while (*ptr == ' ')
	     ptr++;
	while (*ptr != ' ')
	     ptr++;
	
	if (sscanf(tmpstr, "%d%d%d%d%d",&oid,&x,&y,&len, &show) != 5)
		return;
	if ((ptr = fgets(tmpstr, 200, fd)) == NULL)
		return;
	while (*ptr == ' ')
		ptr++;
	k = 0;
	if (*ptr == '"')
	{
	      ptr++;
	      while (*ptr != '"' && *ptr != '\0' && *ptr != '\n')
		color[k++] = *ptr++;
	      if (*ptr == '"')
	        ptr++;
	}
	else
	{
	     while (*ptr != ' ' && *ptr != '\0' && *ptr != '\n')
		color[k++] = *ptr++;
	}
	color[k] = '\0';
		
	if (id <= LASTTITLE)
	{
	     while (*ptr == ' ')
		ptr++;
	     k = 0;
	     if (*ptr == '"')
	     {
		ptr++;
		while (*ptr != '"' && *ptr != '\0' && *ptr != '\n')
			name[k++] = *ptr++;
	     }
	     else
	     {
		while (*ptr != ' ' && *ptr != '\0' && *ptr != '\n')
			name[k++] = *ptr++;
	     }
	     name[k] = '\0';
	}
	else
	{
	      if (acq_items[id].name != NULL)
	          strcpy(name, acq_items[id].name);
		else
		   strcpy(name, " ");
	}
	acq_items[id].def = 1;
	acq_items[id].loc_x = x;
	acq_items[id].loc_y = y;
	acq_items[id].dloc_x = x;
	acq_items[id].dloc_y = y;
	acq_items[id].dshow = show;
	acq_items[id].show = show;
	acq_items[id].sshow = show;
	acq_items[id].dname_len = len;
	acq_items[id].name_len = len;
	strcpy(acq_items[id].color, color);
	strcpy(acq_items[id].dcolor, color);
	if(len >  acq_items[id].length)
	{
	     if (acq_items[id].name != NULL)
	     {
		free(acq_items[id].name);
		free(acq_items[id].dname);
	     }
	     acq_items[id].name = (char *) malloc(len + 2);
	     acq_items[id].dname = (char *) malloc(len + 2);
	     acq_items[id].length = len;
	}

	strcpy(acq_items[id].name, name);
	strcpy(acq_items[id].dname, name);
}


read_set_default()
{
	char   *in;
	char   token[120];
	int    num;

	if (fgets(tmpstr, 120, fd) == NULL)
		return;
	if (strncmp(tmpstr, "Varian", 6) != 0)
		return;
	while ((in = fgets(tmpstr, 200, fd)) != NULL)
	{
		if (sscanf(in, "%d%s", &num, token) != 2)
		     continue;
		if (num > 100)
		{
		     set_item_attr(num);
		     continue;
		}
		switch (num) {
		 case 1:   /* geometry */
		     strcpy(geom, token);
		     break;
		 case 2:   /* background */
		     strcpy(winBgName, token);
		     break;
		 case 3:   /* label font */
		     strcpy(labelFontName, token);
		     break;
		 case 4:   /* value font */
		     strcpy(valueFontName, token);
		     break;
		 case 5:   /* foreground */
		     strcpy(winFgName, token);
		     break;
		 case 6:   /* title */
		     /* This only gets first word of title */
		     /*strcpy(framelabel, token);*/
		     break;
		 case 7:   /* host */
		     /*strcpy(hostName, token);*/
                     break;
		}
	}
}

init_defaults()
{
	int	item;

	for (item = 0; item <= LASTITEM; item++)
	{
		acq_items[item].dshow = ON;
                if ((RF10s1AvgVal <= item && item <= RFMonVal) ||
                    (RF10s1AvgTitle <= item && item <= RFMonTitle))
                {
                    acq_items[item].dshow = OFF;
                }
		acq_items[item].show = NONE;
		acq_items[item].sshow = NONE;
		acq_items[item].name = NULL;
		acq_items[item].name_len = 0;
		acq_items[item].length = 0;
		acq_items[item].dname = NULL;
		acq_items[item].dname_len = 0;
		acq_items[item].sname = NULL;
		acq_items[item].sname_len = 0;
		acq_items[item].def = 0;
		strcpy(acq_items[item].color, "black");
		strcpy(acq_items[item].dcolor, "black");
	}
}

int
exec_remove()
{
	sprintf(tmpstr, "%s/templates/acqstat/%s", getenv("vnmruser"), setup_file);
	unlink(tmpstr);
	create_file_list();
}


remove_proc()
{
	char    *tpr;
	struct   stat   f_stat;

	save_load_proc = NULL;
	if ((tpr = (char *)getenv("vnmruser")) == NULL)
	{
	    disp_error("Error: env parameter 'vnmruser' is not set.", 1);
	    disp_error("  ", 2);
	    return;
	}

	sprintf(tmpstr, "%s/templates/acqstat/%s", tpr, setup_file);
	if (stat(tmpstr, &f_stat) == 0)
	{
		save_load_proc = exec_remove;
		sprintf(tmpstr, "File '%s' will be deleted.", setup_file);
		disp_warning(tmpstr, 1, 1);
		sprintf(tmpstr, "Are you sure?");
		disp_warning(tmpstr, 2, 1);
		return;
	}
}


exec_save()
{
	int	id, quot;
	char    *tpr;

	fd = NULL;
	if ((tpr = (char *)getenv("vnmruser")) != NULL)
	{
		sprintf(tmpstr, "%s/templates/acqstat/%s", tpr, setup_file);
		fd = fopen(tmpstr,"w+");
		if (fd == NULL)
		{
		   sprintf(tmpstr, "mkdir -p  %s/templates/acqstat", tpr);
		   system(tmpstr);
		   sprintf(tmpstr, "%s/templates/acqstat/%s", tpr, setup_file);
		   fd = fopen(tmpstr,"w+");
		}
	}
	if (fd == NULL)
	{
	    disp_error("Could not open file: ", 1);
	    if (tpr != NULL)
	       disp_error(tmpstr, 2);
	    else
	       disp_error(setup_file, 2);
	    return;
	}
	fprintf(fd, "Varian Acqstat 1.0\n");
	fprintf(fd, "1 %s\n", geom);

	tpr = get_text_item(bgWidget);
	if (tpr != NULL)
	    fprintf(fd, "2 %s\n", tpr);
	else
	    fprintf(fd, "2 %s\n", winBgName);

	tpr = get_text_item(bfontWidget);
	if (tpr != NULL)
	    fprintf(fd, "3 %s\n", tpr);
	else
	    fprintf(fd, "3 %s\n", labelFontName);

	tpr = get_text_item(vfontWidget);
	if (tpr != NULL)
	    fprintf(fd, "4 %s\n", tpr);
	else
	    fprintf(fd, "4 %s\n", valueFontName);

	tpr = get_text_item(fgWidget);
	if (tpr != NULL)
	    fprintf(fd, "5 %s\n", tpr);
	else
	    fprintf(fd, "5 %s\n", winFgName);

	/* tpr = get_text_item(titleWidget);
	if (tpr != NULL)
	    fprintf(fd, "6 %s\n", tpr);
	else
	    fprintf(fd, "6 %s\n", framelabel);
	*/

	/* tpr = get_text_item(hostWidget);
	if (tpr != NULL)
	    fprintf(fd, "7 %s\n", tpr);
	else
	    fprintf(fd, "7 %s\n", RemoteHost);
	*/

	for(id = FIRSTTITLE; id <= LASTTITLE; id++)
	{
	    fprintf(fd, "%d %d %d %d ",id+100,acq_items[id].loc_x, acq_items[id].loc_y,acq_items[id].name_len);
	    if (acq_items[id].show == NONE)
		fprintf(fd, "0\n");
	    else
		fprintf(fd, "1\n");
	    tpr = acq_items[id].color;
	    quot = 0;
	    while (*tpr != '\0')
	    {
		if (*tpr++ == ' ')
		   quot = 1;
	    }
	    if (quot)
		fprintf(fd, "  \"%s\" ", acq_items[id].color);
	    else
		fprintf(fd, "  %s ", acq_items[id].color);
	    quot = 0;
	    tpr = acq_items[id].name;
	    while (*tpr != '\0')
	    {
		if (*tpr++ == ' ')
		   quot = 1;
	    }
	    if (quot)
	        fprintf(fd, " \"%s\"\n", acq_items[id].name);
	    else
	        fprintf(fd, " %s\n", acq_items[id].name);
	}
	for(id = LASTTITLE+1; id <= LASTITEM ; id++)
	{
	    fprintf(fd, "%d %d %d %d ",id+200-LASTTITLE,acq_items[id].loc_x, acq_items[id].loc_y,acq_items[id].name_len);
	    if (acq_items[id].show == NONE)
		fprintf(fd, "0\n");
	    else
		fprintf(fd, "1\n");
	    tpr = acq_items[id].color;
	    quot = 0;
	    while (*tpr != '\0')
	    {
		if (*tpr++ == ' ')
		   quot = 1;
	    }
	    if (quot)
		fprintf(fd, "  \"%s\"\n", acq_items[id].color);
	    else
		fprintf(fd, "  %s\n", acq_items[id].color);
	}
	fclose(fd);
	XtPopdown(fileShell);
}
	


save_proc()
{
	char    *tpr;
	struct   stat   f_stat;

	save_load_proc = NULL;
	if ((tpr = (char *)getenv("vnmruser")) == NULL)
	{
	    disp_error("Error: env parameter 'vnmruser' is not set.", 1);
	    disp_error("  ", 2);
	    return;
	}

	sprintf(tmpstr, "%s/templates/acqstat/%s", tpr, setup_file);
	if (stat(tmpstr, &f_stat) == 0)
	{
		save_load_proc = exec_save;
		sprintf(tmpstr, "File '%s' already exists.", setup_file);
		disp_warning(tmpstr, 1, 1);
		sprintf(tmpstr, "It will be overwritten.");
		disp_warning(tmpstr, 2, 1);
		return;
	}
	exec_save();
}


#define  LABELMODE  1
#define  TEXTMODE   2


void
setupWin_close(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
	EventMask   mask;

	if (useInfostat == 0)
	   return;
	if (activeCursor)
	     XUndefineCursor(dpy, statusWin);
        mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask | LeaveWindowMask;
	XtRemoveEventHandler(statusPanel,mask, False, button_proc, NULL);
	highlit_item(-1);
	setupActive = 0;
	XtPopdown(setupShell);
	if (warningShell)
	   XtPopdown(warningShell);
	if (fileShell)
	   XtPopdown(fileShell);
}




void
setupWin_exit()
{
	EventMask   mask;

	if (useInfostat == 0)
	   return;
	if (activeCursor)
	     XUndefineCursor(dpy, statusWin);
        mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask | LeaveWindowMask;
	XtRemoveEventHandler(statusPanel,mask, False, button_proc, NULL);
	setupShell = NULL;
	highlit_item(-1);
	setupActive = 0;
	if (warningShell)
	    XtPopdown(warningShell);
	if (fileShell)
	    XtPopdown(fileShell);
}

void
fileWin_exit()
{
	fileShell = NULL;
}


void
warnWin_exit()
{
	warningShell = NULL;
}


set_label_item(w, label)
Widget   w;
char     *label;
{
	XmString        mstring;

    	if (useInfostat == 0)
	     return;
   	mstring = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[0], XmNlabelString, mstring);
	XtSetValues(w, args, 1);
/*
   	XtFree(mstring);
*/
        XmStringFree(mstring);
}


set_text_item(w, label)
Widget   w;
char     *label;
{
	XmTextSetString(w, label);
}


char * get_text_item(w)
Widget   w;
{
	int	len;
	static  char    tstr[120];
	static  char   *data, *attr;

	attr = NULL;
	data = (char *) XmTextGetString(w);
	if (data != NULL && (int)strlen(data) > 0)
	{
	     while (*data == ' ' || *data == '\t')
		data++;
	     strcpy(tstr, data);
	     len = (int)strlen(tstr) - 1;
	     while (len >= 0)
	     {
		if (tstr[len] != ' ' && tstr[len] != '\t')
		    break;
		tstr[len--] = '\0';
	     }
	     attr = tstr;
	}
	return(attr);
}


#endif

static
change_host(host_name)
char   *host_name;
{
        inittimer(0.0,0.0,NULL);
        if ((int)strlen(RemoteHost) > 0)
        {
            if (Acqproc_ok(RemoteHost))
            {
                unregister();
            }
            killrpctcp();
        }
        acq_ok = 0;
        strcpy(RemoteHost, host_name);
}


#ifdef MOTIF
store_item_info(item)
int	item;
{
	int	vid, len;
	Boolean set;
	static  char   *data;

	vid = item + LASTTITLE;
	data = get_text_item(labelWidget);
	if (data != NULL)
	{
	     len = (int)strlen(data);
	     if (len > 0)
	     {
		if (len > acq_items[item].sname_len) 
		{
		    if (acq_items[item].sname != NULL)
		        free(acq_items[item].sname);
		    acq_items[item].sname = (char *) malloc(len + 2);
		    acq_items[item].sname_len = len;
		}
		strcpy(acq_items[item].sname, data);
	    }
	    else if (acq_items[item].sname != NULL)
		strcpy(acq_items[item].sname, "");
	}
	data = get_text_item(colorW);
	if (data != NULL)
	{
	     len = (int)strlen(data);
	     if (len > 0)
		strcpy(acq_items[item].scolor, data);
	     else
		strcpy(acq_items[item].scolor, "");
	}
	else
	     strcpy(acq_items[item].scolor, acq_items[item].dcolor);
	data = get_text_item(colorW2);
	if (data != NULL)
	{
	     len = (int)strlen(data);
	     if (len > 0)
		strcpy(acq_items[vid].scolor, data);
	     else 
		strcpy(acq_items[vid].scolor, "");
	}
	else
	     strcpy(acq_items[vid].scolor, acq_items[vid].dcolor);

	XtSetArg(args[0], XtNset, &set);
	XtGetValues(showWidget, args, 1);
	acq_items[item].sshow = set;
	acq_items[vid].sshow = set;
}


void
setup_reset_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	m, m2;

	if (setup_item < 0)
	    return;
	m = setup_item;
	m2 = m + LASTTITLE;
	if (acq_items[m].dname_len > acq_items[m].sname_len)
	{
	      if (acq_items[m].sname != NULL)
                  free(acq_items[m].sname);
	      acq_items[m].sname = (char *) malloc(acq_items[m].dname_len);
	      acq_items[m].sname_len = acq_items[m].dname_len;
	}
	strcpy(acq_items[m].sname, acq_items[m].dname);
	strcpy(acq_items[m].scolor, acq_items[m].dcolor);
	strcpy(acq_items[m2].scolor, acq_items[m2].dcolor);
	acq_items[m].show = acq_items[m].dshow;
	set_text_item(labelWidget, acq_items[m].dname);
	set_text_item(colorW, acq_items[m].dcolor);
	set_text_item(colorW2, acq_items[m2].dcolor);
	XtSetArg(args[0], XtNset, TRUE);
	if (acq_items[m].dshow == ON)
		XtSetValues(showWidget, args, 1);
	else
		XtSetValues(noWidget, args, 1);
	XtSetArg(args[0], XtNset, FALSE);
	if (acq_items[m].dshow == ON)
		XtSetValues(noWidget, args, 1);
	else
		XtSetValues(showWidget, args, 1);
}


void
setup_set_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int   m, m2;
	int   nw, nh, changed;
	char  *data;
	XColor xcol1, xcol2;
	item_order  *clist;

	if (setup_item < 0)
	    return;
	if (setup_item > 0) {
	    store_item_info(setup_item);
	}
	clist = item_list;
	while (clist != NULL)
	{
	    m = clist->id;
	    m2 = m + LASTTITLE;
	    if (acq_items[m].sname != NULL && (int)strlen(acq_items[m].sname) > 0)
	    {
		if (acq_items[m].sname_len > acq_items[m].length)
		{
		     free(acq_items[m].name);
		     acq_items[m].length = acq_items[m].sname_len;
		     acq_items[m].name = (char *) malloc(acq_items[m].length + 2);
	        }
		strcpy(acq_items[m].name, acq_items[m].sname);
		acq_items[m].name_len = (int)strlen(acq_items[m].name) + 1;
		acq_items[m2].loc_x = acq_items[m].loc_x + acq_items[m].name_len;
	    }
	    if ((int)strlen(acq_items[m].scolor) > 0)
	    {
		if (strcmp(acq_items[m].scolor, acq_items[m].color) != 0)
		{
		     if(XAllocNamedColor(dpy, cmap, acq_items[m].scolor,
				 &xcol1, &xcol2))
		     {
		        strcpy(acq_items[m].color, acq_items[m].scolor);
			acq_items[m].pix = xcol1.pixel;
		     }
	             else
	   	     {
	     		set_item_menu(m);
		        sprintf(errstr, "Color '%s' is not available.", acq_items[m].scolor);
			disp_error(errstr, 1);
			disp_error(" ", 2);
		        return;
	   	     }
		}
	    }
	    if ((int)strlen(acq_items[m2].scolor) > 0)
	    {
		if (strcmp(acq_items[m2].scolor, acq_items[m2].color) != 0)
		{
		     if(XAllocNamedColor(dpy, cmap, acq_items[m2].scolor,
				 &xcol1, &xcol2))
		     {
		        strcpy(acq_items[m2].color, acq_items[m2].scolor);
			acq_items[m2].pix = xcol1.pixel;
		     }
	             else
	   	     {
	     		set_item_menu(m);
		        sprintf(errstr, "Color '%s' is not available.", acq_items[m2].scolor);
			disp_error(errstr, 1);
			disp_error(" ", 2);
		        return;
	   	     }
		}
	    }
	    if (acq_items[m].sshow == 0)
	    {
		if (acq_items[m].show == ON)
		{
		     acq_items[m].show = OFF;
		     disp_item(m);
		}
		if (acq_items[m2].show == ON)
		{
		     acq_items[m2].show = OFF;
		     disp_item(m2);
		}
		acq_items[m].show = NONE;
		acq_items[m2].show = NONE;
	    }
	    else
	    {
		if (acq_items[m].show != ON)
		{
		     acq_items[m].show = ON;
		     disp_item(m);
		}
		if (acq_items[m2].show != ON)
		{
		     acq_items[m2].show = ON;
		     disp_item(m2);
		}
	    }

	    clist = clist->next;
	}
	if (titleWidget) {
	    data = get_text_item(titleWidget);
	    if (data != NULL && strcmp(data, framelabel))
	    {
		sprintf(framelabel, data);
		XtSetArg (args[0], XtNtitle, framelabel);
		XtSetValues(statusShell,args,1);
	    }

	    data = get_text_item(bgWidget);
	    if (data != NULL)
	    {
		strcpy(winBgName, data);
		set_win_bg();
	    }

	    data = get_text_item(fgWidget);
	    if (data != NULL)
	    {
		strcpy(winFgName, data);
	    }

	    changed = 0;
	    m = hlit_item;
	    hlit_item = -1;
	    data = get_text_item(bfontWidget);
	    if (data != NULL)
		strcpy(labelFontName, data);

	    data = get_text_item(vfontWidget);
	    if (data != NULL)
		strcpy(valueFontName, data);

	    set_font();

	    cal_item_loc();
	
	    data = get_text_item(hostWidget);
	    if (data != NULL)
	    {
		if (strcmp(RemoteHost, data) != 0)
		{
		    change_host(data);
		    inittimer(2.0,0.0,DoTheChores);
		}
	    }
	    else
		set_text_item(hostWidget, RemoteHost);
	    XClearWindow(dpy, statusWin);
	    redraw_items();
	    highlit_item(m);
	}
}



void
setup_file_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int       k;
	def_rec   *plist;

	store_item_info(setup_item);
	if (fileShell == NULL)
	     create_fileWindow();
	if (fileShell)
	{
	     create_file_list();
	     XtPopup (fileShell, XtGrabNone);
	     XRaiseWindow(dpy, fileID);
	}
}


create_file_list()
{
	int       k;
	def_rec   *plist;

	create_def_list();
	plist = def_list;
	k = 0;
	while (plist != NULL)
	{
	     k++;
	     plist->id = k;
   	     xmstr = XmStringLtoRCreate(plist->name, XmSTRING_DEFAULT_CHARSET);
	     AddListItem(loadScrWin, xmstr, 0);
	     plist = plist->next;
	}
}




Widget 
create_text_widget(parent, mode, label_1, label_2)
Widget  parent;
int	mode;
char    *label_1, *label_2;
{
	int	n;
	Widget  widget, formWidget, lwidget;
	XmString        mstring;

	n = 0;
   	XtSetArg(args[n], XmNheight, butHeight + 2);  n++;
   	formWidget = (Widget)XmCreateForm(parent, "frame", args, n);
   	XtManageChild (formWidget);
	n =0;
   	mstring = XmStringLtoRCreate(label_1, XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[n], XmNlabelString, mstring);  n++;
   	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	lwidget = (Widget)XmCreateLabel(formWidget, "label", args, n);
	XtManageChild (lwidget);
/*
   	XtFree(mstring);
*/
        XmStringFree(mstring);

	n = 0;
	XtSetArg (args[n], XmNshadowThickness, 1); n++;
	XtSetArg (args[n], XmNresizeWidth, TRUE); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftWidget, lwidget); n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNmarginHeight, 1); n++;
	XtSetArg (args[n], XmNverifyBell, FALSE); n++;
	if (mode == LABELMODE)
	{
	    if (label_2 != NULL)
	       mstring = XmStringLtoRCreate(label_2, XmSTRING_DEFAULT_CHARSET);
	    else
	       mstring = XmStringLtoRCreate("   ", XmSTRING_DEFAULT_CHARSET);
            XtSetArg (args[n], XmNlabelString, mstring);  n++;
	    widget = (Widget)XmCreateLabel(formWidget, "text", args, n);
	    XtManageChild (widget);
/*
   	    XtFree(mstring);
*/
            XmStringFree(mstring);
	}
	else
	{
	    widget = (Widget)XmCreateText(formWidget, "text", args, n);
	    XtManageChild (widget);
	    if (label_2 != NULL)
		XmTextSetString(widget, label_2);
	}
	return(widget);
}


create_show_buttons(parent)
Widget   parent;
{
	Widget   twidget, pwidget, p2widget;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);  n++;
	pwidget = (Widget)XmCreateRowColumn(parent, "", args, n);
   	XtManageChild (pwidget);

	n =0;
        xmstr = XmStringLtoRCreate("       Item Show:", XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[n], XmNlabelString, xmstr);  n++;
	twidget = (Widget)XmCreateLabel(pwidget, "label", args, n);
	XtManageChild (twidget);
/*
        XtFree(xmstr);
*/
        XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);  n++;
	p2widget = (Widget)XmCreateRadioBox(pwidget, "", args, n);
   	XtManageChild (p2widget);


        n =0;
        showWidget = (Widget)XmCreateToggleButtonGadget(p2widget, "Yes", args, n);
        XtManageChild (showWidget);

        noWidget = (Widget)XmCreateToggleButtonGadget(p2widget, "No", args, n);
        XtManageChild (noWidget);
}




create_exec_buttons(parent)
Widget   parent;
{
	Widget   pwidget;

	create_button(parent, "Apply", 1, setup_set_proc);
	create_button(parent, "Reset", 1, setup_reset_proc);
	create_button(parent, "File", 1, setup_file_proc);
	create_button(parent, "Close", 1, setupWin_close);
}


void
setupWin_map(w, client_data, event)
  Widget          w;
  caddr_t          client_data;
  XEvent          *event;
{
	EventMask   mask;

        mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask | LeaveWindowMask;
        if (event->type == UnmapNotify)
        {
	    if (activeCursor)
		XUndefineCursor(dpy, statusWin);
	    XtRemoveEventHandler(statusPanel,mask, False, button_proc, NULL);
	    highlit_item(-1);
	    setupActive = 0;
	    if (warningShell)
	   	XtPopdown(warningShell);
	    if (fileShell)
	   	XtPopdown(fileShell);
	}
        if (event->type == MapNotify)
        {
	     if (activeCursor)
		XDefineCursor(dpy, statusWin, activeCursor);
	     XtAddEventHandler(statusPanel,mask, False, button_proc, NULL);
	     setupActive = 1;
	     cal_item_width();
	     show_item_info(setup_item);
	     highlit_item(setup_item);
	}
}




Widget
create_popup_shell(parent, title)
char  *title;
{
	Widget  shell;

	n = 0;
	XtSetArg (args[n], XtNtitle, title); n++;
	shell = XtCreatePopupShell("Acqstat",
			 transientShellWidgetClass, parent, args, n);
	return(shell);
}



void
load_select_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	def_rec   *clist;
	int   item;
	XmListCallbackStruct  *cb;

	cb = (XmListCallbackStruct *) call_data;
	if (cb->reason != XmCR_SINGLE_SELECT)
		return;
	item = cb->item_position;
	clist = def_list;
	while (clist != NULL)
	{
	       if (clist->id == item)
	       {
		   set_text_item(fileWidget, clist->name);
		   return;
	       }
	       clist = clist->next;
	}
}


void
file_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	type;
	char   *data;

	type = (int) client_data;
	if (type == 0)
	{
	     XtPopdown(fileShell);
	     return;
	}
	data = get_text_item(fileWidget);
	if (data != NULL && (int)strlen(data) > 0)
	{
	    strcpy(setup_file, data);
	    if (type == LOAD)
		load_proc();
	    else if (type == SAVE)
		save_proc();
	    else if (type == REMOVE)
		remove_proc();
	}
	else
	{
	    disp_error("Error:  File name is empty. ", 1);
	    disp_error("  ", 2);
	}
}


load_proc()
{
	int	m, x, y;
	unsigned int  ww, hh;

	if (useInfostat == 0)
		return;
	m = hlit_item;
	if( !setup_default(setup_file))
		return;
	if (fileShell) {
	    XtPopdown(fileShell);
	}
	set_font();
	set_item_def_color();
	cal_item_loc();
	XParseGeometry(geom, &x, &y, &ww, &hh);
	if (xserver != SUNSVR)
	{
	        x = x + decoW;
	        y = y + decoH;
	}
	else if (xrelease == 3000)  /*  SunOS, not Solaris */
	{
	        x = x + decoW - 1;
	        y = y + decoH - 1;
	}

	XtConfigureWidget(statusShell, x, y, ww, hh, 1);
	set_win_bg();
	XClearWindow(dpy, statusWin);
	hlit_item = -1;
	redraw_items();
	highlit_item(m);
	if (setupShell)
	{
	        set_label_item(geomWidget, geom);
	        set_text_item(hostWidget, hostName);
	        set_text_item(titleWidget, framelabel);
	        set_text_item(bgWidget, winBgName);
	        set_text_item(fgWidget, winFgName);
	        set_text_item(bfontWidget, labelFontName);
	        set_text_item(vfontWidget, valueFontName);
		show_item_info(setup_item);
	        highlit_item(setup_item);
	}
        XtSetArg (args[0], XtNtitle, framelabel);
	XtSetValues(statusShell,args,1);
	if (strcmp(RemoteHost, hostName) != 0)
        {
                change_host(hostName);
                inittimer(2.0,0.0,DoTheChores);
        }
	setup_set_proc(0, 0, 0);
}

create_fileWindow()
{
	Widget  pwidget, twidget;
	Atom	removeAtom;
	Position	y;

	fileShell = create_popup_shell(statusShell, "Acqstatus File");

	fileWin = create_row_col(fileShell, 1, 0, 0);

	n = 0;
    	XtSetArg(args[n], XmNselectionPolicy, (XtArgVal) XmSINGLE_SELECT); n++;
    	XtSetArg(args[n], XmNvisibleItemCount, (XtArgVal) 6); n++;
    	loadScrWin = XmCreateScrolledList(fileWin, "", args, n);
	XtManageChild (loadScrWin);
	XtAddCallback(loadScrWin, XmNsingleSelectionCallback, load_select_proc, NULL);


	fileWidget = create_text_widget(fileWin, TEXTMODE, "File:", setup_file, NULL, 0);

	pwidget = create_row_col(fileWin, 0, 10, 16);
	create_button(pwidget, " Save ", SAVE, file_proc);
	create_button(pwidget, " Load ", LOAD, file_proc);
	create_button(pwidget, "Remove", REMOVE, file_proc);
	create_button(pwidget, "Close ", 0, file_proc);

	XtRealizeWidget (fileShell);
	XmAddProtocolCallback(fileShell, XM_WM_PROTOCOL_ATOM(fileShell),
			deleteAtom, fileWin_exit, 0);
	y = shell_y + shell_h + decoH + 22;
	if (y > screenHeight - 300)
		y = screenHeight - 300;
	XtMoveWidget(fileShell, shell_x, y);
	fileID = XtWindow(fileShell);
}



create_setupWindow()
{
	Position        x, y;
	Widget  	t1, sep;
	Widget  	rc1, rc2, rc3;

	setupShell = create_popup_shell(statusShell, "Acqstatus Setup");
		
	cal_mainShell_loc();
	n = 0;
	setupWin = (Widget)XmCreateForm(setupShell, "", args, n);
        XtManageChild (setupWin);
	rc1 = create_rc_widget(setupWin, NULL, 1, 0, 0);

	geomWidget = create_text_widget(rc1,LABELMODE,
			"        Geometry:", geom, NULL, 0);
	hostWidget = create_text_widget(rc1, TEXTMODE, 
			"   Host Computer:", RemoteHost, NULL, 0);
	titleWidget = create_text_widget(rc1, TEXTMODE, 
			"           Title:", framelabel, NULL, 0);
	bgWidget = create_text_widget(rc1, TEXTMODE, 
			"Background Color:", winBgName, color_list, 5);
	fgWidget = create_text_widget(rc1, TEXTMODE, 
			"Foreground Color:", winFgName, fcolor_list, 5);
	bfontWidget = create_text_widget(rc1,TEXTMODE,
			"   Font Of Label:", labelFontName, font_list, 4);
	vfontWidget = create_text_widget(rc1,TEXTMODE,
			"   Font Of Value:", valueFontName, vfont_list, 4);

	t1 = create_separtor(setupWin, rc1);

	rc2 = create_rc_widget(setupWin, t1, 1, 0, 0);

	create_item_menu(rc2);

	labelWidget = create_text_widget(rc2, TEXTMODE, 
			"           Label:", "    ", NULL, 0);
	colorW = create_text_widget(rc2, TEXTMODE, 
			"  Color Of Label:", "    ", NULL, 0);
	colorW2 = create_text_widget(rc2, TEXTMODE,
			"  Color Of Value:", "    ", NULL, 0);

	create_show_buttons(rc2);

	t1 = create_separtor(setupWin, rc2);

	rc3 = create_rc_widget(setupWin, t1, 0, 10, 25);
	create_exec_buttons(rc3);

	XtRealizeWidget (setupShell);
	y = shell_y + shell_h + decoH + 10;
	if (y > screenHeight - 300)
		y = screenHeight - 300;
	XtMoveWidget(setupShell, shell_x, y);
	XtAddEventHandler( setupShell, StructureNotifyMask, False,
                                setupWin_map, (XtPointer)0);
	setupID = XtWindow(setupShell);
	XmAddProtocolCallback(setupShell, XM_WM_PROTOCOL_ATOM(setupShell),
			deleteAtom, setupWin_exit, 0);
}


highlit_item(item)
int	item;
{
    	if (useInfostat == 0)
	     return;
	if (hlit_item >= 0)
	       XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
	if (item >= 0 && acq_items[item].show == ON)
	{
		hlit_x = acq_items[item].x - 1;
		hlit_y = acq_items[item].y - 1;
		hlit_w = acq_items[item].full_width - 1;
	        XDrawRectangle(dpy, statusWin, hlitGc, hlit_x, hlit_y,
			hlit_w, hlit_h);
		hlit_item = item;
	}
	else
		hlit_item = -1;
}


show_item_info(item)
int	item;
{
	int	vitem;

    	if (useInfostat == 0)
	     return;
	if (item >= 0 && item == setup_item)
		return;
	if (item < 0)
	       item = FIRSTTITLE; 
	if (setup_item >= 0)
	       store_item_info(setup_item);
	if (acq_items[item].sname != NULL)
	    set_text_item(labelWidget, acq_items[item].sname);
	else
	    set_text_item(labelWidget, acq_items[item].name);
	if ((int)strlen(acq_items[item].scolor) > 0)
	    set_text_item(colorW, acq_items[item].scolor);
	else if ((int)strlen(acq_items[item].color) > 0)
	    set_text_item(colorW, acq_items[item].color);
	vitem = item + LASTTITLE;
	if ((int)strlen(acq_items[vitem].scolor) > 0)
	    set_text_item(colorW2, acq_items[vitem].scolor);
	else if ((int)strlen(acq_items[vitem].color) > 0)
	    set_text_item(colorW2, acq_items[vitem].color);
	XtSetArg(args[0], XtNset, TRUE);
	if (acq_items[item].sshow == ON)
		XtSetValues(showWidget, args, 1);
	else
		XtSetValues(noWidget, args, 1);
	XtSetArg(args[0], XtNset, FALSE);
	if (acq_items[item].sshow == ON)
		XtSetValues(noWidget, args, 1);
	else
		XtSetValues(showWidget, args, 1);
	setup_item = item;
}

void
select_item(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
	int	item, len;

	item = (int)client_data;
	highlit_item(-1);
	stack_top_item(item);
	disp_item(item);
	disp_item(item+LASTTITLE);
	show_item_info(item);
	highlit_item(item);
}


create_errorWindow(mess)
char    *mess;
{
	Widget  pwidget;
	int     x, y, k;
	Position   posx;


	if (statusShell == NULL)
	     return;
	warningShell = create_popup_shell(statusShell, "Acqstatus Warning");

	n = 0;
	XtSetArg (args[n], XmNwidth, 300);  n++;
	XtSetArg (args[n], XmNheight, 80);  n++;
	pwidget = XtCreateManagedWidget("Acqstat", xmFormWidgetClass,
                                warningShell, args, n);
	n = 0;
	XtSetArg (args[n], XmNresizeWidth, TRUE); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNalignment, XmALIGNMENT_BEGINNING);  n++;
	XtSetArg (args[n], XmNrecomputeSize, TRUE);  n++;
	xmstr = XmStringLtoRCreate(mess, XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
	errWidget = (Widget)XmCreateLabel(pwidget, "text", args, n);
	XtManageChild (errWidget);
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg (args[n], XmNtopWidget, errWidget);  n++;
	errWidget2 = (Widget)XmCreateLabel(pwidget, "text", args, n);
	XtManageChild (errWidget2);
/*
   	XtFree(xmstr);
*/
        XmStringFree(xmstr);

	XtSetArg (args[0], XmNfontList, &warnFontInfo);
	XtGetValues(errWidget, args, 1);
	n = 0;
	XtSetArg (args[n], XmNresizeWidth, TRUE); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg (args[n], XmNtopWidget, errWidget2);  n++;
	XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg (args[n], XmNpacking, XmPACK_COLUMN);  n++;
	XtSetArg (args[n], XmNspacing, 30);  n++;
        XtSetArg (args[n], XmNmarginWidth, 30);  n++;
	pwidget = (Widget)XmCreateRowColumn(pwidget, "", args, n);
   	XtManageChild (pwidget);

	warnBut1 = create_button(pwidget, "Continue", 1, warning_proc);
	warnBut2 = create_button(pwidget, " Ok ", 1, error_proc);
	warnBut3 = create_button(pwidget, " Cancel ", 0, warning_proc);
	x = (screenWidth - 400) / 2;
	y = (screenHeight-200) / 2;
	XtRealizeWidget (warningShell);
	n = 0;
	XtSetArg (args[n], XtNx, &posx);  n++;
	XtSetArg (args[n], XtNwidth, &width);  n++;
	XtGetValues(warnBut3, args, n);
	warnWidth = (Dimension)posx + width + 10;
	warnId = XtWindow(warningShell);

	XtMoveWidget(warningShell, x, y);
	XmAddProtocolCallback(warningShell, XM_WM_PROTOCOL_ATOM(warningShell),
			deleteAtom, warnWin_exit, 0);
}




create_item_menu(parent)
Widget  parent;
{
	Widget   pulldown;
	Widget   tmpwidget;
	XmString xmstr;
	int	 item, len;

	pulldown = (Widget)XmCreatePulldownMenu(parent, "pulldown", NULL, 0);
        for(item = FIRSTTITLE; item <= LASTTITLE; item++)
        {
	   strcpy(tmpstr, acq_items[item].dname);
	   len = (int)strlen(tmpstr);
	   if ( tmpstr[len-1] == ':')
		tmpstr[len-1] = '\0';
	   xmstr = XmStringCreate(tmpstr,XmSTRING_DEFAULT_CHARSET);
   	   XtSetArg(args[0], XmNlabelString, xmstr);
   	   tmpwidget = (Widget)XmCreatePushButtonGadget(pulldown, " ", args, 1);
           XtAddCallback(tmpwidget, XmNactivateCallback, select_item, item);
   	   XmStringFree(xmstr);
	   XtManageChild(tmpwidget);
	   acq_items[item].widget = tmpwidget;
	}
	xmstr = XmStringCreate("       Item Name:", XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr);  n++;
	XtSetArg(args[n], XmNsubMenuId, pulldown); n++;
	XtSetArg(args[n], XmNmenuHistory, acq_items[FIRSTTITLE].widget); n++;
	item_menu = (Widget)XmCreateOptionMenu(parent, " ", args, n);
   	XmStringFree(xmstr);
	XtManageChild(item_menu);
}




get_colors()
{
     int         i;
     static char *colors[] = {"red", "green", "blue"};
     XrmValue    fromType, toType;

     if (useInfostat == 0)
	     return;
     for (i = 0; i < 3; i++)
     {
        fromType.size = sizeof (colors[i]);
        fromType.addr = colors[i];
        XtConvert(statusShell, XtRString, &fromType, XtRPixel, &toType);

        switch (i) {
         case 0:
                redPix = *((Pixel *)toType.addr);
                break;
         case 1:
                greenPix = *((Pixel *)toType.addr);
                break;
         case 2:
                bluePix = *((Pixel *)toType.addr);
                break;
        }
     }
     hlit_pix = redPix ^ winBg;
}


set_item_def_color()
{
	int    m;
	XColor xcol1, xcol2;
	
        if (useInfostat == 0)
	     return;
	for (m = FIRSTTITLE; m <= LASTITEM; m++)
	{
	     if(XAllocNamedColor(dpy, cmap, acq_items[m].color, &xcol1, &xcol2))
		acq_items[m].pix = xcol1.pixel;
	     else
		acq_items[m].pix = winFg;
	}
}


create_def_list()
{
        def_rec        *plist, *clist;
	char	       *tpr;

        plist = def_list;
	if (def_list != NULL)
	         XmListDeleteAllItems(loadScrWin);
        while (plist != NULL)
        {
                clist = plist;
                plist = plist->next;
                if (clist->name != NULL)
                     XtFree(clist->name);
                XtFree(clist);
        }
        def_list = NULL;
	if ((tpr = (char *)getenv("vnmruser")) != NULL)
	{
		sprintf(tmpstr, "%s/templates/acqstat", tpr);
		build_def_list(tmpstr);
	}
	if ((tpr = (char *)getenv("vnmrsystem")) != NULL)
	{
		sprintf(tmpstr, "%s/user_templates/acqstat", tpr);
		build_def_list(tmpstr);
	}
}


build_def_list(dir_name)
char    *dir_name;
{
        def_rec        *plist, *clist;
        DIR             *dirp;
        struct dirent   *dp;

        dirp = opendir(dir_name);
        if(dirp == NULL)
             return;
	plist = def_list;
	while (plist != NULL)
	{
	    if (plist->next == NULL)
		break;
	    plist = plist->next;
	}

        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
        {
           if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                continue;
           clist = (def_rec *) XtMalloc( sizeof(def_rec));
           if ( plist == NULL)
                def_list = clist;
           else
                plist->next = clist;
           plist = clist;
           clist->name = (char *) XtMalloc((int)strlen(dp->d_name) + 2);
           strcpy(clist->name, dp->d_name);
           clist->next = NULL;
        }
        closedir(dirp);
}


set_font()
{
	XFontStruct  *spInfo;
	int	     w;

        if (useInfostat == 0)
	    return;
	if (strcmp(labelFontName, oldl_FontName) != 0)
	{
	    spInfo = labelFontInfo;
            if ((labelFontInfo = XLoadQueryFont(dpy, labelFontName))!=NULL)
            {
	        strcpy(oldl_FontName, labelFontName);
		if (label_font)
		    XUnloadFont(dpy, label_font);
		label_font = labelFontInfo->fid;
	 	chHeight = labelFontInfo->max_bounds.ascent +
                                 labelFontInfo->max_bounds.descent;
            	chWidth = labelFontInfo->max_bounds.width;
            	w = labelFontInfo->min_bounds.width;
		if (w != chWidth)
		    chWidth = (chWidth + w) / 2;
		ch_ascent = labelFontInfo->max_bounds.ascent;
		XFreeFontInfo(NULL, spInfo, 1);
            }
	    else
	    {
		labelFontInfo = spInfo;
	        sprintf(errstr, "Font '%s' is not available", labelFontName);
		disp_error(errstr, 1);
		disp_error(" ", 2);
		return;
	    }
	}
	if (strcmp(valueFontName, oldv_FontName) != 0)
	{
	    spInfo = valueFontInfo;
	    if ((valueFontInfo = XLoadQueryFont(dpy, valueFontName))!=NULL)
            {
	        strcpy(oldv_FontName, valueFontName);
		if (value_font)
		    XUnloadFont(dpy, value_font);
		value_font = valueFontInfo->fid;
		ch2Height = valueFontInfo->max_bounds.ascent +
                                 valueFontInfo->max_bounds.descent;
		ch2Width = valueFontInfo->max_bounds.width;
		w = valueFontInfo->min_bounds.width;
		if (w != ch2Width)
		    ch2Width = (ch2Width + w) / 2;
		ch2_ascent = valueFontInfo->max_bounds.ascent;
		XFreeFontInfo(NULL, spInfo, 1);
            }
	    else
	    {
		valueFontInfo = spInfo;
	        sprintf(errstr, "Font '%s' is not available", valueFontName);
		disp_error(errstr, 1);
		disp_error(" ", 2);
		return;
	    }
	}
	if (label_font == NULL || value_font == NULL)
	{
	   if (defFontInfo == NULL)
	       defFontInfo = XLoadQueryFont(dpy, "9x15");
	   if (defFontInfo != NULL)
	   {
		def_font = defFontInfo->fid;
		if (label_font == NULL)
		{
		    	ch_ascent = defFontInfo->max_bounds.ascent;
     			chHeight = ch_ascent + defFontInfo->max_bounds.descent;
     			chWidth = defFontInfo->max_bounds.width;
			label_font = def_font;
		}
		if (value_font == NULL)
		{
			value_font = def_font;
		    	ch2_ascent = defFontInfo->max_bounds.ascent;
     			ch2Height = ch_ascent + defFontInfo->max_bounds.descent;
     			ch2Width = defFontInfo->max_bounds.width;
		}
		if (valueFontInfo == NULL)
		     valueFontInfo = defFontInfo;
		if (labelFontInfo == NULL)
		     labelFontInfo = defFontInfo;
	   }
	}
	if (chHeight > ch2Height)
	    rowHeight = chHeight + 1;
	else
	{
	    ch_ascent = ch2_ascent;
	    rowHeight = ch2Height + 1;
	}
	charWidth = chWidth;
	hlit_h = rowHeight + 2;
	hlit_x = -1;
	hlit_item = -1;
	if (labelGc && label_font)
	    XSetFont(dpy, labelGc, label_font);
	if (valueGc && value_font)
	    XSetFont(dpy, valueGc, value_font);
}

set_win_bg()
{
	XColor xcol1, xcol2;

        if (useInfostat == 0)
	    return;
	if (strcmp(winBgName, old_BgName) != 0)
	{
	   if(XAllocNamedColor(dpy, cmap, winBgName, &xcol1, &xcol2))
	   {
	        strcpy(old_BgName, winBgName);
		winBg = xcol1.pixel;
		XSetWindowBackground(dpy, statusWin, winBg);
		shellId = XtWindow(statusShell);
		XSetWindowBackground(dpy, shellId, winBg);
     		hlit_pix = redPix ^ winBg;
		if (hlitGc) {
		    XSetForeground(dpy, hlitGc, hlit_pix);
		}
		XtSetArg (args[0], XtNbackground, winBg);
		XtSetValues(frame, args, 1);
	   }
	   else
	   {
		sprintf(errstr, "Color '%s' is not available", winBgName);
		disp_error(errstr, 1);
		disp_error(" ", 2);
	   }
	}
}
#endif

static XtInputId   input_fd = 0;

void
input_signal()
{
	DoTheChores();
}


register_input_event(fd)
int  fd;
{
        if (input_fd)
                XtRemoveInput(input_fd);
        input_fd = XtAddInput(fd, XtInputReadMask, input_signal, NULL);
}

