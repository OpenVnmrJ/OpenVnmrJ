/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>

#include "ACQPROC_strucs.h"
#include "hostAcqStructs.h"
#include "acqmeter.icon"
#include "acqmp.icon"

#ifdef OLIT
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/Exclusives.h>
#include <Xol/Form.h>
#include <Xol/Menu.h>
#include <Xol/MenuButton.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/ScrollingL.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>

#define BORDERWIDTH     XtNborderWidth
#define XtInitialize    OlInitialize
#define TEXTWIDGET      staticTextWidgetClass
#endif /* OLIT */

#ifdef MOTIF
#include <X11/Intrinsic.h>
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
#define BORDERWIDTH     XmNborderWidth
#define TEXTWIDGET      xmLabelWidgetClass
#endif /* MOTIF */

extern  void exitproc();

#define  IBMSVR  0
#define  SUNSVR  1
#define  SGISVR  2

#define  HOSTLEN    40
#define  COLORLEN   24

int  debug = 0;
int  acq_ok;
char User[HOSTLEN];
char LocalHost[HOSTLEN];
char RemoteHost[HOSTLEN];
int  Procpid;

struct hostent	 local_entry;
static char	*local_addr_list[ 2 ] = { NULL, NULL };
static int	 local_addr;

AcqStatBlock  CurrentBlock;

#define REREGISTERTIME 400L	/* interval to reregister with Infoproc
                                 * Infoproc will disconnect if not
                                 * reregistered every 600 seconds
                                 */

#define icon_width 	16
#define icon_height 	16

static unsigned char icon_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
   0x04, 0x01, 0x38, 0x01, 0xe0, 0x03, 0x80, 0x73, 0x00, 0x9f, 0x00, 0x9f,
   0xc0, 0xe3, 0x40, 0x02, 0x40, 0x02, 0x80, 0x03};

static unsigned char center_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xf8, 0x1f, 0x78, 0x1e, 0x78, 0x1e, 0x38, 0x1c, 0x08, 0x10, 0x08, 0x10,
   0x38, 0x1c, 0x78, 0x1e, 0x78, 0x1e, 0xf8, 0x1f};

static  int     n, screen;
static  int     screenHeight = 1;
static  int     screenWidth = 1;
static  int     winWidth;
static  int     winHeight;
static  int     winDepth;
static  int     setup_item = -1;
static  int     layout;
static  int     nlayout;
static  int     chart_type, nchart_type;
static  int     acqActive;
static  int	charWidth= -1;
static  int	fixWidth;
static  int	xserver = 0;
static  int	xrelease;
static  int	propActive = 0;
static  int     charHeight, ch_ascent, ch_descent;
static  int     shell_x, shell_y, shell_w, shell_h;
static  int     decoW, decoH;  /*  window decoration */
static  int     boardW, boardH;  /*  window decoration */
static  int     mapW = 0, mapH = 0;
static  double  delay_time = 5.0;
static  char    winBgName[COLORLEN];
static  char    winFgName[COLORLEN];
static  char    gridColor[COLORLEN];
static  char    old_BgName[COLORLEN];
static  char    fontName[120];
static  GC      chart_gc;
static  Font	xfont;
static  Pixel	winBg, winFg, focusPix;
static  Pixel	gridPix, redPix, greenPix, bluePix;
static  Widget  mainShell = NULL;
static  Widget  mainframe = NULL;
static  Widget  chartPanel;
static  Widget  warningShell = NULL;
static  Widget  errWidget, errWidget2;
static  Widget  showWidget[2];
static  Widget  centerWidget[2];
static  Pixmap  chartMap = NULL;
static  Widget  menuShell = NULL;
static  Widget  propShell = NULL;
static  Widget  propPanel;
static  Widget  typeWidget[2];
static  Widget  hostWidget, timeWidget;
static  Widget  layoutWidget[2];
static  Widget  item_menu;
static  Widget  bgWidget, fgWidget, gcolorWidget;
static  Widget  fontWidget;
static  Widget  setvWidget, upperWidget, lowerWidget;
static  Widget  grafWidget[2], dirWidget[2];
static  Widget  logWidget[2], saveWidget;
static  Widget  gmodeWidget[4];
static  Widget  fileShell = NULL;
static  Widget  fileWin;
static  Widget  fileScrWin, fileWidget;
static  Window  propShellId, fileID, warningID;
static  Widget  warnBut1, warnBut2, warnBut3;
static  Display  *dpy;
static  Colormap cmap;
static  Atom    deleteAtom;
static  XFontStruct  *fontInfo = NULL;
static  Dimension    width, height;
static  char    *user_dir = NULL;
Arg     args[12];
char    tmpstr[250];
char    errstr[120];
static  char     setup_file[120];
Window  shellId, chartWin;
XWindowAttributes win_attributes;

#ifdef  MOTIF

#define XtNset          XmNset
XmString        xmstr;
#define DeleteListItem  XmListDeleteAllItems
#define AddListItem     XmListAddItem

#else
OlListItem    olitem;
OlListToken   (*DeleteListItem ) ();
OlListToken   (*AddListItem) ();

#endif

#define  LABELMODE  1
#define  TEXTMODE   2

#define  HORIZONTAL 0
#define  VERTICAL   1

#define  OFF 	    0
#define  ON   	    1

#define  HISTORIC   1
#define  THERMO     2

#define  LOCKCHART	0
#define  VTCHART	1
#define  SPINCHART	2
#define  TOTALCHART	3

#define  LINEMODE 	1
#define  SOLIDMODE	2

#define  HCHART 	1
#define  VCHART		2

#define SAVE    1
#define LOAD    2
#define REMOVE  3

static  Widget menu_buttons[TOTALCHART+2];
static  char   *chart_name[TOTALCHART] = {"LOCK", "VT", "SPIN" };
static  char   *menu_show_label[TOTALCHART] = {
			"Show Lock", "Show VT", "Show Spinner" };
static  char   *menu_close_label[TOTALCHART] = {
			"Close Lock", "Close VT", "Close Spinner" };

typedef struct _item_attr {
	int	x, y;
	int	width, height;
	int	tx, ty;
	int	tw, th;
	int	twidth, theight;
	int	show, nshow;
	int	active;
	int	mode;
	int	nmode;
	int	cenbut_x, cenbut_y; 
	int	direction;
	int	ndirection;
	int	center; 
	int	grid, ngrid; 
	float	setval; 
	float	upper, nupper; 
	float	lower, nlower; 
	float   *data;
	int     data_len;
	int     disp_len;
	int     dindex;
	int     points;
	float   ratio;
	float   tratio;
	float   scale;
	int     mnum;
	int	log; 
	int	nlog; 
	int	cenbut; 
	int	ncenbut; 
	int	fname_len;
	int	nfname_len;
	FILE    *fd;
	char	*fname;
	char	*nfname;
	char	bcolor[COLORLEN];
	char	fcolor[COLORLEN];
	char	gcolor[COLORLEN];
	Pixel   bg, fg, grPix;
  } item_attr;

item_attr     disp_items[TOTALCHART];
static  int   disp_order[TOTALCHART];
static  float cur_val[TOTALCHART];

static int	icon_num, icon_x, icon_y;
static int	cent_num, cent_x, cent_y;
static XPoint   iconpoints[128];
static XPoint   cutpnts[128];
static XPoint   centerpnts[128];
static XPoint   cxpnts[128];
static XPoint   xpoints[128];
static XSegment xsegments[128];
static Widget   chartItems[TOTALCHART];

typedef  struct _def_rec {
        int     id;
        char    *name;
#ifdef  OLIT
        OlListToken  item;
#endif
        struct _def_rec  *next;
        } def_rec;

static  def_rec   *def_list = NULL;

static int (*save_load_proc)() = NULL;


#ifdef OLIT
static void
mainShell_exit( w, client_data, event )
Widget w;
char *client_data;
void *event;
{
	OlWMProtocolVerify	*olwmpv;

	olwmpv = (OlWMProtocolVerify *) event;
	if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
		exitproc();
	}
}

Widget 
create_button(parent, label, num, func)
Widget  parent;
char    *label;
int	num;
void    (*func)();
{
	Widget  button;

        n = 0;
	XtSetArg (args[n], XtNlabel, label);  n++;
	button = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, parent, args, n);
	XtAddCallback(button, XtNselect, func, (XtPointer) num);
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
	if (vertical == VERTICAL)
	{
	    XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	    XtSetArg (args[n], XtNmeasure, 1);  n++;
	}
	else
	{
	    XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
            XtSetArg (args[n], XtNmeasure, 1);  n++;
	    if (pad > 0)
	    {
            	XtSetArg (args[n], XtNhPad, pad);  n++;
            	XtSetArg (args[n], XtNhSpace, space);  n++;
	    }
	}
	row_col = XtCreateManagedWidget("",
                        controlAreaWidgetClass, parent, args, n);
	return(row_col);
}

Widget 
create_form_row_col(parent,brother,  direction, pad, space)
Widget  parent, brother;
int	direction;
int	pad, space;
{
	Widget  row_col;

	n = 0;
	if (direction == VERTICAL)
	{
	    XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	    XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	    XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	    XtSetArg(args[n], XtNxResizable, True);  n++;
	    XtSetArg(args[n], XtNborderWidth, 1);  n++;
	    XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
            XtSetArg (args[n], XtNmeasure, 1);  n++;
	}
	else
	{
	    XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	    XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	    XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	    XtSetArg(args[n], XtNxResizable, True);  n++;
	    XtSetArg(args[n], XtNborderWidth, 1);  n++;
	    XtSetArg (args[n], XtNyAttachBottom, TRUE); n++;
	    XtSetArg (args[n], XtNyResizable, TRUE);  n++;
	    XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
            XtSetArg (args[n], XtNmeasure, 1);  n++;
	    if (pad > 0)
	    {
            	XtSetArg (args[n], XtNhPad, pad);  n++;
            	XtSetArg (args[n], XtNhSpace, space);  n++;
	    }
	}
	if (brother != NULL)
	{
	    XtSetArg(args[n], XtNyRefWidget, brother); n++;
	}
	row_col = XtCreateManagedWidget("",
                        controlAreaWidgetClass, parent, args, n);
	return(row_col);
}


static
set_button(button)
 Widget  button;
{
	if (propActive)
	{
	   XtSetArg(args[0], XtNset, TRUE);
           XtSetValues(button, args, 1);
	}
}

#else

void
mainShell_exit(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
	exitproc();
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
        button = (Widget) XmCreatePushButton(parent, "button", args, n);
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
	if (vertical == VERTICAL)
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
	row_col = (Widget) XmCreateRowColumn(parent, "", args, n);
   	XtManageChild (row_col);
	return(row_col);
}

Widget
create_form_row_col(parent,brother,  direction, pad, space)
Widget  parent, brother;
int     direction;
int     pad, space;
{
        Widget  row_col;

        n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNborderWidth, 1);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	if (brother != NULL)
        {
	    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
   	    XtSetArg (args[n], XmNtopWidget,brother); n++;
        }
        if (direction == VERTICAL)
        {
	   XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	   XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	}
	else
	{
	    XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
	    XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);  n++;
	    XtSetArg(args[n], XmNspacing, space);  n++;
	    XtSetArg(args[n], XmNmarginWidth, space);  n++;
	}
	row_col = (Widget) XmCreateRowColumn(parent, "", args, n);
   	XtManageChild (row_col);
	return(row_col);
}


static
set_button(button)
 Widget  button;
{
	if (propActive)
	{
	    XmToggleButtonSetState(button, TRUE, TRUE);
	}
}


#endif

Pixel get_focus_pixel(back)
  Pixel   back;
{
        int      m;
        int      red, green, blue;
	XColor   xcolor;

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
set_shell_dim(w, h)
int	w, h;
{
	int	   ww, hh, x, y;

	ww = w + boardW;
	hh = h + boardH;
	if (ww + decoW > screenWidth)
	    ww = screenWidth - decoW;
	if (hh + decoH > screenHeight)
	    hh = screenHeight - decoH;
        if (xserver != SUNSVR)
        {
	    x = shell_x + decoW;
	    if (x + ww + boardW > screenWidth)
		x = screenWidth - ww - boardW;
	    y = shell_y + decoH;
	    if (y + hh + boardH > screenHeight)
		y = screenHeight - hh - boardH;
	}
        else
        {
	    if (xrelease == 3000)  /* It is SunOS, not Solaris */
	    {
		x = shell_x + decoW -1;
		y = shell_y + decoH - 1;
		if (x + ww + boardW > screenWidth)
		    x = screenWidth - ww - boardW - 1;
		if (y + hh + boardH > screenHeight)
		    y = screenHeight - hh - boardH - 1;
	    }
	    else
	    {
		x = shell_x;
		y = shell_y;
		if (x + ww + decoW + boardW > screenWidth)
		    x = screenWidth - ww - decoW - boardW;
		if (y + hh + decoH + boardH > screenHeight)
		    y = screenHeight - hh - decoH - boardH;
	    }
	}
        XtConfigureWidget(mainShell, x, y, ww, hh, 1);
}


cal_chart_geom()
{
	int	    item, count, m, len, len2;
	int	    ws, hs, x, y, k;
	float       *pdata, *sdata;
	Dimension   ww, hh, ds;

	count = 0;
	for(m = 0; m < TOTALCHART; m++)
	{
	     if (disp_order[m] >= 0)
		count++;
	}
	if (count <= 0)
	     count = 1;
        n = 0;
        XtSetArg (args[n], XtNwidth, &ww); n++;
        XtSetArg (args[n], XtNheight, &hh); n++;
	XtGetValues(chartPanel, args, n);
	if (layout == HCHART)
	{
		hs = hh;
		if (chart_type == THERMO)
		{
		     ws = ww / count;
		     if (ws < 7)
			ws = 7;
		     if (hs < 27 + charHeight)
			hs= 27 + charHeight;
		     ds = ws * count;
		}
		else
		{
		     ws = (ww - count + 1) / count;
		     if (ws < 20)
			ws = 20;
		     ds = ws * count + count -1;
		}
		if (ds != ww || hs != hh)
		{
		    if (ds < ww)
			ds += count;
		    ds += boardW;
		    hs += boardH;
        	    n = 0;
        	    XtSetArg (args[n], XtNwidth, ds); n++;
        	    XtSetArg (args[n], XtNheight, hs); n++;
		    XtSetValues (mainShell, args, n);
		    return;
		}
	}
	else  /*  vertical */
	{
		ws = ww;
		if (chart_type == THERMO)
		{
		     hs = hh / count;
		     if (ws < 7)
			ws = 7;
		     if (hs < 27 + charHeight)
			hs = 27 + charHeight;
		     ds = hs * count;
		}
		else
		{
		     if (ws < 20)
			ws = 20;
		     hs = (hh - count + 1) / count;
		     ds = hs * count + count -1;
		}
		if (ds != hh || ws != ww)
		{
		    if (ds < hh)
			ds += count;
		    ds += boardH;
		    ws += boardW;
        	    n = 0;
        	    XtSetArg (args[n], XtNwidth, ws); n++;
        	    XtSetArg (args[n], XtNheight, ds); n++;
		    XtSetValues (mainShell, args, n);
		    return;
		}
	}
	for(m = 0; m < TOTALCHART; m++)
	{
	     if (chart_type == THERMO)
	     {
	        disp_items[m].twidth = ws;
	        disp_items[m].theight = hs;
	     }
	     else
	     {
	        disp_items[m].width = ws;
	        disp_items[m].height = hs;
	     }
	     if (disp_items[m].direction == HCHART)
		len = ws;
	     else
		len = hs;
	     if (disp_items[m].data_len < len)
	     {
		sdata = disp_items[m].data;
		len2 = disp_items[m].data_len;
		disp_items[m].data_len = ((len % 256) + 1) * 256;
		disp_items[m].data = (float *)malloc(
			sizeof(float) *  disp_items[m].data_len);
		pdata = disp_items[m].data;
		if (sdata != NULL)
		{
		   k = 0;
		   while (k < len2)
		   {
		     *pdata = *sdata++;
		     pdata++;
		     k++;
		   }
		   free(sdata);
		}
	     }
	}
	if (mapW != ws || mapH != hs)
	{
	     if (chartMap)
	     {
		XFreePixmap(dpy, chartMap);
		chartMap = NULL;
	     }
	     chartMap = XCreatePixmap(dpy, chartWin, ws, hs, winDepth);
	     mapW = ws;
	     mapH = hs;
	}
	x = 0;
	y = 0;
	for(m = 0; m < TOTALCHART; m++)
	{
	    k = disp_order[m];
	    if (k >= 0)
	    {
	        disp_items[k].x = x;
	        disp_items[k].y = y;
		if (layout == HCHART && chart_type == HISTORIC)
		{
		    if (x > 0)
		    {
		           XSetForeground (dpy, chart_gc, winFg);
			   XDrawLine(dpy, chartWin, chart_gc, x, 0, x, hs);
		    }
		    x++;
	        }
	        cal_chart_dim(k);
	        if (layout == HCHART)
		    x = x + ws;
	        else
		    y = y + hs;

		draw_chart(k);
	    }
	}
}

cal_thermo_dim(id)
int	id;
{
	int	w, h, sc, gap;
	float   range, vs, fv, level;

	h = disp_items[id].theight - charHeight - 6;
	if (h < 21)
	     h = 21;
	w = disp_items[id].twidth / 6;

	if (w > h / 6)
	     w = h / 6;
	if (w < 7)
	     w = 7;
	if (w % 2)
	     w++;
	h = h - w;
	disp_items[id].tx = disp_items[id].twidth / 2;
	disp_items[id].ty = disp_items[id].theight - charHeight - 4 -
	     w / 2;
	disp_items[id].tw = w;
	disp_items[id].th = h;
        range = disp_items[id].upper - disp_items[id].lower;
	if (range == 0.0)
	     range = 1.0;
        disp_items[id].tratio = (float) h / range;
	if (disp_items[id].tratio < 0.0)
	     disp_items[id].tratio = 0.0;
	if (disp_items[id].grid >= 2)
	{
	     if (screenHeight > 900)
		 sc = 20;
	     else
		 sc = 10;
	     level = h / sc;
	     while (level < 3.0 && sc > 5)
	     {
		 sc -= 2;
		 level = h / sc;
	     }
	     if (sc < 5 || level < 3.0)
	     {
		 disp_items[id].scale = 0.0;
		 return;
	     }
	     fv = range / level;
	     if (fv < 0.1)
	     {
		fv = 0.1;
		gap = 1;
   	        vs = fv;
	     }
	     else
	     {
	     	level = 1.0;
	     	vs = fv;
	     	while (vs < 1.0)
	     	{
		   level = level * 10.0;
		   vs = fv * level;
	     	}
	     	while (vs >= 10.0)
	     	{
		   level = level / 10.0;
		   vs = fv * level;
	     	}
	     	if (vs <= 1.25)
	     	{
		   vs = 1.0;
		   gap = 1;
	     	}
	     	else if (vs <= 5.0)
		{
		   vs = 5.0;
		   gap = 1;
	     	}
	     	else
		{
		   vs = 10.0;
		   gap = 1;
	     	}
	     	vs = vs / level;
		if ((vs > fv * 3.0)  && (gap == 1))
		{
		     vs = vs / 5.0;
		     gap = 5;
		}
		fv = vs;
	     }
		
	     disp_items[id].scale = fv;
	     h = charHeight / (fv * disp_items[id].tratio);
	     disp_items[id].mnum = (h / 2 + 1) * gap;
	}
}



cal_chart_dim(id)
int	id;
{
        int	width, height, sc, gap;
	float   range, level, vs, fv;

     if (chart_type == THERMO)
     {
	cal_thermo_dim(id);
	return;
     }
     range = disp_items[id].upper - disp_items[id].lower;
     if (range <= 0.0)
	range = 1.0;
     if (disp_items[id].direction == HCHART)
     {
	if (disp_items[id].grid >= 2)
	     height = disp_items[id].height - charHeight - ch_ascent - 4;
	else
	     height = disp_items[id].height - charHeight - 4;
	if (height < 0)
	     height = 0;
	disp_items[id].ratio = (float) height / range;
	if (disp_items[id].ratio < 0.0)
	    disp_items[id].ratio = 0.0;
	if (disp_items[id].setval >= disp_items[id].lower && 
			disp_items[id].setval <= disp_items[id].upper)
	     disp_items[id].center = height - (disp_items[id].setval - 
			disp_items[id].lower) * disp_items[id].ratio + 2;
	else
	     disp_items[id].center = height + 2; 
	if (disp_items[id].grid >= 2)
		disp_items[id].center = disp_items[id].center + ch_ascent;
	disp_items[id].disp_len = disp_items[id].width - 9;
	if (disp_items[id].disp_len < 1)
		disp_items[id].disp_len = 1;
	if (disp_items[id].grid >= 2)
        {
	     if (screenHeight > 900)
	     {
		 sc = 20;
		 gap = 7;
	     }
	     else
	     {
		 sc = 10;
		 gap = 5;
	     }
	     level = height / sc;
	     while (level < 3.0 && sc > gap)
	     {
		 sc -= 2;
		 level = height / sc;
	     }
	     if (sc < gap || level < 3.0)
	     {
		 disp_items[id].scale = 0.0;
		 return;
	     }
	     fv = range / level;
	     if (fv < 0.1)
	     {
		fv = 0.1;
		gap = 1;
   	        vs = fv;
	     }
	     else
	     {
	     	level = 1.0;
	     	vs = fv;
	     	while (vs < 1.0)
	     	{
		   level = level * 10.0;
		   vs = fv * level;
	     	}
	     	while (vs >= 10.0)
	     	{
		   level = level / 10.0;
		   vs = fv * level;
	     	}
	     	if (vs <= 1.25)
	     	{
		   vs = 1.0;
		   gap = 1;
	     	}
	     	else if (vs <= 5.0)
		{
		   vs = 5.0;
		   gap = 1;
	     	}
	     	else
		{
		   vs = 10.0;
		   gap = 1;
	     	}
	     	vs = vs / level;
		if ((vs > fv * 3.0)  && (gap == 1))
		{
		     vs = vs / 5.0;
		     gap = 5;
		}
		fv = vs;
	     }
		
	     disp_items[id].scale = fv;
	     height = charHeight / (fv * disp_items[id].ratio);
	     disp_items[id].mnum = (height / 2 + 1) * gap;

	     sprintf(tmpstr, "%.1f", disp_items[id].upper);
	     width = (int) strlen(tmpstr);
	     sprintf(tmpstr, "%.1f", disp_items[id].lower);
	     if ((int) strlen(tmpstr) > width)
		width = strlen(tmpstr);
	     width += 2;
	     disp_items[id].disp_len = disp_items[id].width - 
					charWidth * width - 15;
	     if (disp_items[id].disp_len < 0)
		disp_items[id].disp_len = 0;
        }
	return;
     }
    /*  if it is vertical, not support */
     width = disp_items[id].width - 4;
     disp_items[id].ratio = (float) width / range;
     if (disp_items[id].ratio < 0.0)
	disp_items[id].ratio = 0.0;
     if (disp_items[id].setval >= disp_items[id].lower &&
			disp_items[id].setval <= disp_items[id].upper)
        disp_items[id].center = (disp_items[id].setval - disp_items[id].lower) *
				 disp_items[id].ratio + 2;
     else
        disp_items[id].center = 2;
     disp_items[id].disp_len = disp_items[id].height - charHeight - 4;
}


draw_thermo_chart(id)
int	id;
{
	int	cx, cy, cw, ch, yh;
	int	x1, x2, x3, x4, y1, y2, w2, mm;
	float   val, max, min, vs, ratio, setval;
	char    data[12];

	XSetForeground (dpy, chart_gc, disp_items[id].bg);
	XFillRectangle (dpy, chartMap, chart_gc, 0, 0, mapW, mapH);

	cw = disp_items[id].tw;
	cx = disp_items[id].tx;
	cy = disp_items[id].ty;
	ch = disp_items[id].th;
	max = disp_items[id].upper;
	min = disp_items[id].lower;
	ratio = disp_items[id].tratio;
	XSetForeground (dpy, chart_gc, winFg);

	x1 = cx - cw / 2;
	y1 = cy - cw / 2;
	XDrawArc(dpy, chartMap, chart_gc, x1, y1, cw, cw, 0, -180 * 64);

	y1 = y1 - ch;
	XDrawArc(dpy, chartMap, chart_gc, x1, y1, cw, cw, 0, 180 * 64);

	y1 = cy - ch;
	XDrawLine(dpy, chartMap, chart_gc, x1, y1, x1, cy);
	x1 = cx + cw / 2;
	XDrawLine(dpy, chartMap, chart_gc, x1, y1, x1, cy);

	w2 = cw / 2;
	if (w2 < 1)
	   w2 = 1;
	if ( w2 % 2 == 0 )
	   w2++;
	x1 = cx - (w2 -1) / 2;
	val = cur_val[id];
	if (id == VTCHART)
		val = val / 10.0;
	if (val > max)
	    val = max;
	if (val < min)
	    val = min;
	y1 = cy - (float) (val - min) * ratio;
	yh = cy - y1 + 1;
	XSetForeground (dpy, chart_gc, disp_items[id].fg);
	XFillRectangle(dpy, chartMap, chart_gc, x1, y1, w2, yh);

	w2 = charWidth;
	if (w2 < 8)
	    w2 = 8;
	if (disp_items[id].grid)
	{
	    XSetForeground (dpy, chart_gc, disp_items[id].grPix);
	    x2 = cx - cw / 2;
	    x1 = x2 - w2;
	    if (x1 < 0)
		x1 = 0;
	    XDrawLine(dpy, chartMap, chart_gc, x1, cy, x2, cy);
	    setval = disp_items[id].setval;
	    if (setval >= min && setval <= max)
	        y1 = cy - (setval - min) * ratio; 
	    else
	    {
		setval = min - 1.0;
	        y1 = cy;
	    }
	    y2 = cy - ch;
	    XDrawLine(dpy, chartMap, chart_gc, x1, y2, x2, y2);
	    x1 = cx + cw / 2;
	    x2 = x1 + w2;
	    XDrawLine(dpy, chartMap, chart_gc, x1, cy, x2, cy);
	    /* draw set mark */
	    if (setval >= min && id != LOCKCHART)
	    {
	        XDrawLine(dpy, chartMap, chart_gc, x1, y1, x2, y1);
		XDrawLine(dpy, chartMap, chart_gc, x1+2, y1-1, x1+5, y1-1);
	    	XDrawLine(dpy, chartMap, chart_gc, x1+4, y1-2, x1+5, y1-2);
	    	XDrawLine(dpy, chartMap, chart_gc, x1+2, y1+1, x1+5, y1+1);
	    	XDrawLine(dpy, chartMap, chart_gc, x1+4, y1+2, x1+5, y1+2);
	    }

	    XDrawLine(dpy, chartMap, chart_gc, x1, y2, x2, y2);
	    if (disp_items[id].grid > 1)
	    {
		x3 = x2 + 3;
		sprintf(data, "%.1f", disp_items[id].lower);
		XDrawString (dpy, chartMap, chart_gc, x3, cy+ch_descent, data, strlen(data));
		if (setval >= min && id != LOCKCHART)
		{
		   sprintf(data, "%.1f", disp_items[id].setval);
		   XDrawString (dpy, chartMap, chart_gc, x3, y1+ch_descent,
					 data, strlen(data));
		}
		sprintf(data, "%.1f", disp_items[id].upper);
		XDrawString (dpy, chartMap, chart_gc, x3, y2+ch_descent, data, strlen(data));
	    }
	    if (disp_items[id].grid > 2 && disp_items[id].scale != 0.0)
	    {
	        x1 = cx - cw / 2;
	        x2 = x1 - w2 / 2;
	        x3 = x1 - w2;
	 	vs = disp_items[id].scale;
		mm = disp_items[id].mnum;
		max = max - min;
		y1 = 1;
		val = vs;
		while (val < max)
		{
		      yh = cy - (float) val * ratio;
		      if (y1 % mm == 0)
		      {
			  sprintf(data, "%.1f", min + val);
			  y2 = strlen(data);
			  if (fixWidth)
				x4 = x3 - y2 * charWidth - 2;
			  else
				x4 = x3 - XTextWidth(fontInfo, data, y2) - 2;
			  XDrawString (dpy, chartMap, chart_gc, x4, yh+ch_descent, data, y2);
		          XDrawLine(dpy, chartMap, chart_gc, x3, yh, x1, yh);
		      }
		      else
		          XDrawLine(dpy, chartMap, chart_gc, x2, yh, x1, yh);
		      y1++;
		      val = vs * y1;
		}
	    }
	    XSetForeground (dpy, chart_gc, disp_items[id].fg);
	}
	sprintf(data, "%s", chart_name[id]);
	ch = strlen(data);
	if (disp_items[id].grid == 0 && disp_items[id].cenbut == 0)
	{
	    if (fixWidth)
	        x2 = charWidth * ch;
	    else
	       x2 = XTextWidth(fontInfo, data, ch);
	    x1 = cx - x2 / 2;
	    if (x1 < 0)
	       x1 = 0;
	}
	else
	    x1 = 4;
	y1 = disp_items[id].theight - ch_descent - 1;
	XDrawString (dpy, chartMap, chart_gc, x1, y1, data, ch);

	if (!acq_ok || !acqActive)
	{
	    x1 = disp_items[id].twidth - icon_width - 6;
	    y1 = disp_items[id].theight - icon_height;
	    if (icon_x != x1 || icon_y != y1)
	    {
		icon_x = x1;
		icon_y = y1;
		copy_icon_points(cutpnts, iconpoints, x1, y1, icon_num);
	    }
	    XDrawPoints(dpy, chartMap, chart_gc, iconpoints, icon_num,
					CoordModeOrigin);
	}
	else if (disp_items[id].grid > 0)
	{
	    if (id == LOCKCHART)
	        sprintf(data, "%5.2f%%",cur_val[id]);
	    else if (id == VTCHART)
	        sprintf(data, "%4.1fC", (float)cur_val[id] / 10.0);
	    else
	        sprintf(data, "%dHz",(int)cur_val[id]);
	    ch = (int)strlen(data);
	    if (fixWidth)
	        x1 = disp_items[id].twidth - ch * charWidth - 4;
	    else
	        x1 = disp_items[id].twidth - XTextWidth(fontInfo, data, ch) - 4;
	    if (x1 < 0)
	        x1 = 0;
	    XDrawString (dpy, chartMap, chart_gc, x1, y1, data, ch);
	}
	if (disp_items[id].cenbut)
	{
	    x1 = (disp_items[id].twidth - icon_width) / 2;
	    y1 = disp_items[id].theight - icon_height;
	    disp_items[id].cenbut_x = x1;
	    disp_items[id].cenbut_y = y1;
	    if (cent_x != x1 || cent_y != y1)
	    {
		cent_x = x1;
		cent_y = y1;
		copy_icon_points(centerpnts, cxpnts, x1, y1, cent_num);
	    }
	    XDrawPoints(dpy, chartMap, chart_gc, cxpnts, cent_num,
					CoordModeOrigin);
	}

	XCopyArea(dpy, chartMap, chartWin, chart_gc, 0, 0, mapW, mapH,
			disp_items[id].x, disp_items[id].y);
}



draw_chart(id)
int    id;
{
	int	x1, x2, x3, x4, y, y1, y2, center;
	int	solid, vertical, len, dp, mm, tlen;
	int	pindex, sindex;
	float   *pdata, max, min, val, setval, ratio, mx, vs;
	char    data[12];

	if (chartMap == NULL)
	     return;
    	if (chart_type == THERMO)
     	{
	      draw_thermo_chart(id);
	      return;
        }
	if (disp_items[id].mode == SOLIDMODE)
	    solid = 1;
	else
	    solid = 0;
	if (disp_items[id].direction == VCHART)
	    vertical = 1;
	else
	    vertical = 0;
	XSetForeground (dpy, chart_gc, disp_items[id].bg);
	XFillRectangle (dpy, chartMap, chart_gc, 0, 0, mapW, mapH);
	XSetForeground (dpy, chart_gc, disp_items[id].fg);
        ratio = disp_items[id].ratio;
        setval = disp_items[id].setval;
	max = disp_items[id].upper;
	min = disp_items[id].lower;
	if (setval < min || setval > max)
	    setval = min;
	if (disp_items[id].ratio <= 0.0)
	{
	   XCopyArea(dpy, chartMap, chartWin, chart_gc, 0, 0, mapW, mapH,
			disp_items[id].x, disp_items[id].y);
	   return;
	}

        sindex = 0;
        pindex = 0;

	if (disp_items[id].points < disp_items[id].disp_len)
	{
	    len = disp_items[id].points;
	    dp = 0;

	}
	else
	{
	    len = disp_items[id].disp_len;
	    dp = disp_items[id].dindex - len;
	    if (dp < 0)
		dp = disp_items[id].data_len + dp;
	}
	pdata = disp_items[id].data;

	if (vertical)
	{
	   center = disp_items[id].center;
	   y1 = disp_items[id].y + disp_items[id].disp_len - len + 2; 
	   if (solid)
		x1 = center;
	   else
	   {
		val = *(pdata+dp);
		if (val < min)
			val = min;
		if (val > max)
			val = max;
		x1 = (val - setval) * ratio + center;
	   }
	   while (len > 0)
	   {
		val = *(pdata+dp);
		if (val < min)
			val = min;
		if (val > max)
			val = max;
		
		x2 = (val - setval) * ratio + center;
		if (x1 != x2)
		{
		    xsegments[sindex].y1 = y1;
		    xsegments[sindex].y2 = y1;
		    if (solid)
		       xsegments[sindex].x1 = center;
		    else
		       xsegments[sindex].x1 = x1;
		    xsegments[sindex].x2 = x2;
		    sindex++;
		}
	        else
		{
		    xpoints[pindex].x = x1;
		    xpoints[pindex].y = y1;
		    pindex++;
		}
		y1++;
		x1 = x2;
		if (sindex > 127)
		{
		    XDrawSegments(dpy, chartMap, chart_gc, xsegments, sindex);
		    sindex = 0;
		}
		if (pindex > 127)
		{
		    XDrawPoints(dpy, chartMap, chart_gc, xpoints, pindex, 
					CoordModeOrigin);
		    pindex = 0;
	    	}
	 	len--;
		dp++;
		/* it is ring buffer */
		if (dp >= disp_items[id].data_len)
		    dp = 0;
	    }
	}  /* vertical */

	center = disp_items[id].center;
	if (disp_items[id].grid > 0)
	{
	   /* draw max, min, and set marks */
	    XSetForeground (dpy, chart_gc, disp_items[id].grPix);
	    y1 = center - (max - setval) * ratio;
	    y = center;
	    y2 = center + (setval - min) * ratio;	
	    x2 = disp_items[id].width -1;
	    x1 = x2 - 5;
	    for(mm = 0; mm < 3; mm++)
	    {
	       XDrawLine(dpy, chartMap, chart_gc, x1, y1+mm, x2, y1+mm);
	       XDrawLine(dpy, chartMap, chart_gc, x1, y2-mm, x2, y2-mm);
	       if (setval > min && id != LOCKCHART)
	       {
	          XDrawLine(dpy,chartMap,chart_gc,x1,center-mm,x2,center-mm);
	          XDrawLine(dpy,chartMap,chart_gc,x1,center+mm,x2,center+mm);
	       }
	       x1 = x1 + 2;
	    }
	    x3 = 4;
	    if (disp_items[id].grid > 1 && disp_items[id].scale > 0.0)
	    {
	        x1 = disp_items[id].width - disp_items[id].disp_len - 9;
	        x2 = disp_items[id].width - 6;
	        x3 = x1 - 3;
	        x4 = x1 - 6;
	 	vs = disp_items[id].scale;
		mm = disp_items[id].mnum;
		if (disp_items[id].grid > 2)
		    sindex = 1;
		else
		    sindex = 0;
		mx = max - min;
		y1 = 0;
		y = center;
		val = min;
		while (val < max)
		{
		    y2 = y - (val - setval) * ratio;
		    if (y1 % mm == 0)
		    {
			  sprintf(data, "%.1f", val);
			  tlen = strlen(data);
			  if (fixWidth)
				x4 = x3 - tlen * charWidth - 4;
			  else
				x4 = x3 - XTextWidth(fontInfo, data, tlen) - 4;
			  XDrawString (dpy, chartMap, chart_gc, x4, y2+ch_descent, data, tlen);
		 	  if (sindex)
		            XDrawLine(dpy, chartMap, chart_gc, x3-3, y2, x2, y2);
			  else
		            XDrawLine(dpy, chartMap, chart_gc, x3-3, y2, x1, y2);
		    }
		    else
		    {
			  if (sindex)
		            XDrawLine(dpy, chartMap, chart_gc, x3, y2, x2, y2);
			  else
		            XDrawLine(dpy, chartMap, chart_gc, x3, y2, x1, y2);
		    }
		    y1++;
		    val = vs * y1 + min;
		}
	        y2 = center + (setval - min) * ratio;	
		XDrawLine(dpy, chartMap, chart_gc, x1, y2, x2, y2);
	        y1 = center - (max - setval) * ratio;
	        XDrawLine(dpy, chartMap, chart_gc, x3-3, y1, x2, y1);
		XDrawLine(dpy, chartMap, chart_gc, x1, y1, x1, y2);
		sprintf(data, "%.1f", max);
	        tlen = strlen(data);
		if (fixWidth)
			x4 = x3 - tlen * charWidth - 4;
		else
			x4 = x3 - XTextWidth(fontInfo, data, tlen) - 4;
		XDrawString (dpy, chartMap, chart_gc, x4, y1+ch_descent, data, tlen);
	    }
	    XSetForeground (dpy, chart_gc, disp_items[id].fg);
	}
	    

	sindex = 0;
	if ( !vertical )
	{
	   if (disp_items[id].grid > 0)
	        x1 = disp_items[id].width - len - 7;
	   else
	        x1 = disp_items[id].width - len - 4;
	   if (solid)
		y1 = center;
	   else
	   {
		val = *(pdata+dp);
		if (id == VTCHART)
		        val = val / 10.0;
		if (val < min)
			val = min;
		if (val > max)
			val = max;
		y1 = (setval - val) * ratio + center;
	   }
	   while (len > 0)
	   {
		val = *(pdata+dp);
		if (id == VTCHART)
		        val = val / 10.0;
		vs = val;
		if (val < min)
			val = min;
		if (val > max)
			val = max;
		y2 = (setval - val) * ratio + center;
		if (y1 != y2)
		{
		    xsegments[sindex].x1 = x1;
		    xsegments[sindex].x2 = x1;
		    if (solid)
		       xsegments[sindex].y1 = center;
		    else
		       xsegments[sindex].y1 = y1;
		    xsegments[sindex].y2 = y2;
		    sindex++;
		}
	        else
		{
		    xpoints[pindex].x = x1;
		    xpoints[pindex].y = y1;
		    pindex++;
		}
		x1++;
		if (!solid)
		    y1 = y2;
		if (sindex > 127)
		{
		    XDrawSegments(dpy, chartMap, chart_gc, xsegments, sindex);
		    sindex = 0;
		}
		if (pindex > 127)
		{
		    XDrawPoints(dpy, chartMap, chart_gc, xpoints, pindex, 
					CoordModeOrigin);
		    pindex = 0;
	    	}
	 	len--;
		dp++;
		if (dp >= disp_items[id].data_len)
		    dp = 0;
	    }
	}
	if (sindex > 0)
		XDrawSegments(dpy, chartMap, chart_gc, xsegments, sindex);

	if (pindex > 0)
		XDrawPoints(dpy, chartMap, chart_gc, xpoints, pindex,
					CoordModeOrigin);
	sprintf(data, "%s", chart_name[id]);
	len = (int)strlen(data);
	if (disp_items[id].grid == 0 && disp_items[id].cenbut == 0)
	{
	    if (fixWidth)
	        x3 = (disp_items[id].width - len * charWidth) / 2;
	    else
	        x3 = (disp_items[id].width - XTextWidth(fontInfo, data, len)) / 2;
	    if (x3 < 0)
	        x3 = 0;
	}
	y1 = disp_items[id].height - ch_descent - 1;
	XDrawString (dpy, chartMap, chart_gc, x3, y1, data, len);

	if (!acq_ok || !acqActive)
	{
	    x1 = disp_items[id].width - icon_width - 6;
	    y1 = disp_items[id].height - icon_height;
	    if (icon_x != x1 || icon_y != y1)
	    {
		icon_x = x1;
		icon_y = y1;
		copy_icon_points(cutpnts, iconpoints, x1, y1, icon_num);
	    }
	    XDrawPoints(dpy, chartMap, chart_gc, iconpoints, icon_num,
					CoordModeOrigin);
	}
	else if (disp_items[id].grid > 0)
	{
	    if (id == LOCKCHART)
	        sprintf(data, "%5.2f%%",vs);
	    else if (id == VTCHART)
	        sprintf(data, "%4.1fC",vs);
	    else
	        sprintf(data, "%dHz",(int)vs);
	    len = (int)strlen(data);
	    if (fixWidth)
	        x3 = disp_items[id].width - len * charWidth - 4;
	    else
	        x3 = disp_items[id].width - XTextWidth(fontInfo, data, len) - 4;
	    if (x3 < 0)
	        x3 = 0;
	    XDrawString (dpy, chartMap, chart_gc, x3, y1, data, len);
	}
	if (disp_items[id].cenbut)
	{
	    x1 = (disp_items[id].width - icon_width) / 2;
	    y1 = disp_items[id].height - icon_height;
	    disp_items[id].cenbut_x = x1;
	    disp_items[id].cenbut_y = y1;
	    if (cent_x != x1 || cent_y != y1)
	    {
		disp_items[id].cenbut_x = x1;
		disp_items[id].cenbut_y = y1;
		cent_x = x1;
		cent_y = y1;
		copy_icon_points(centerpnts, cxpnts, x1, y1, cent_num);
	    }
	    XDrawPoints(dpy, chartMap, chart_gc, cxpnts, cent_num,
					CoordModeOrigin);
	}

	XCopyArea(dpy, chartMap, chartWin, chart_gc, 0, 0, mapW, mapH,
			disp_items[id].x, disp_items[id].y);
}



static void
expose_panel(widget, client_data, call_data)
Widget          widget;
caddr_t         client_data;
caddr_t         call_data;
{
	int	m, k, x, y;

	for(m = 0; m < TOTALCHART; m++)
	{
	    k = disp_order[m];
	    if (k < 0)
		continue;
	    if (disp_items[k].x > 0)
	    {
		if (layout == HCHART && chart_type == HISTORIC)
		{
		       XSetForeground (dpy, chart_gc, winFg);
		       x = disp_items[k].x - 1;
		       XDrawLine(dpy, chartWin, chart_gc, x, 0, 
					x, disp_items[k].height);
		}
	    }
	    draw_chart(k);
	}
}



put_data(id, val)
int   id;
float val;
{
	float   *sdata;

	cur_val[id] = val;
	sdata = disp_items[id].data;
	*(sdata + disp_items[id].dindex) = val;
	disp_items[id].dindex = disp_items[id].dindex + 1;
	if (disp_items[id].dindex >= disp_items[id].data_len)
		disp_items[id].dindex = 0;
	if (disp_items[id].points < disp_items[id].data_len)
		disp_items[id].points = disp_items[id].points + 1;
}


update_chart(statblock)
AcqStatBlock *statblock;
{
	int   num;
  	float  val;

	acqActive = 1;
	if (statblock->Acqstate <= ACQ_INACTIVE)
	   acqActive = 0;
	if (!acqActive || !acq_ok)
	{
	   put_data (LOCKCHART, 0.0);
	   put_data (SPINCHART, 0.0);
	   put_data (VTCHART, 0.0);
	   for(num = 0; num < TOTALCHART; num++)
	   {
		if (disp_order[num] >= 0)
			draw_chart(disp_order[num]);
	   }
	   return;
	}

	val = (float) statblock->AcqLockLevel;
	if (val > 1300.0)
	{
       	     val = 1300.0 / 16.0 + (val - 1300) / 15.0;
             if (val > 100.0 ) val = 100.0;
        }
        else
             val /= 16.0;
	num = val * 100.0;
	val = (float) num / 100.0;
	if (val < 0.0)
	     val = 0.0;
	put_data (LOCKCHART, val);

	if (statblock->AcqSpinAct < 0)
	     put_data (SPINCHART, 0.0);
	else
	     put_data (SPINCHART, (float)statblock->AcqSpinAct);
	if (statblock->AcqSpinSet < 0)
	     statblock->AcqSpinSet = 0;
	if (disp_items[SPINCHART].setval != (float)statblock->AcqSpinSet)
	{
	     disp_items[SPINCHART].setval = (float)statblock->AcqSpinSet;
	     if (disp_items[SPINCHART].show)
	         cal_chart_dim(SPINCHART);
	}

	if (statblock->AcqVTAct == 30000)
	     val = 0.0;
	else
	{
	     val = (float)statblock->AcqVTAct;
	     /* round up the val into one digit after point */
	     num = val * 10.0;
	     val = (float) num / 10.0;
	}
	if (val < -273.0)
	     val = -273.0;
	put_data (VTCHART, val);
	if (statblock->AcqVTSet < -273)
	     statblock->AcqVTSet = -273;
	if (statblock->AcqVTSet == 30000)
	{
	     if (disp_items[VTCHART].setval != 0.0)
	     {
	          disp_items[VTCHART].setval = 0.0;
	          if (disp_items[VTCHART].show)
	               cal_chart_dim(VTCHART);
	     }
	}
	else
	{
	     if (disp_items[VTCHART].setval != (float)statblock->AcqVTSet / 10) 
	     {
		   disp_items[VTCHART].setval = (float)statblock->AcqVTSet / 10;
	           if (disp_items[VTCHART].show)
		       cal_chart_dim(VTCHART);
	     }
	}
	for(num = 0; num < TOTALCHART; num++)
	{
	    if (disp_order[num] >= 0)
		draw_chart(disp_order[num]);
	}
}

readInfo()
{
    struct timeval clock;
    static long PresentTime;
    static long lastregister = 0;
    AcqStatBlock statbuf;	/* acq update status buffer */
    int ret;

    inittimer(0.0,0.0,NULL);

    if (debug)
       fprintf(stderr,"readInfo acq_ok = %d\n",acq_ok);
    if (acq_ok)
    {
        if (debug)
             fprintf(stderr,"readInfo updating screen");
	if ((ret = readacqstatblock(&statbuf)) > 0)
	{
	     update_chart(&statbuf);
	     CurrentBlock.Acqstate = statbuf.Acqstate;
	     CurrentBlock.AcqLockLevel = statbuf.AcqLockLevel;
	     CurrentBlock.AcqSpinAct = statbuf.AcqSpinAct;
	     CurrentBlock.AcqSpinSet = statbuf.AcqSpinSet;
	     CurrentBlock.AcqVTAct = statbuf.AcqVTAct;
	     CurrentBlock.AcqVTSet = statbuf.AcqVTSet;
             gettimeofday(&clock,NULL);  /* get time of connect */
             PresentTime = clock.tv_sec;
             if (PresentTime - lastregister > REREGISTERTIME)
             {
               reregister();
               lastregister = PresentTime;
             }
	}
	else
	{
             acq_ok = Acqproc_ok(RemoteHost);
	     if (!acq_ok)
	        CurrentBlock.Acqstate = ACQ_INACTIVE;
	     update_chart(&CurrentBlock);
	}
        if (debug)
             fprintf(stderr," result = %d\n",ret);
	if (!acqActive)  /* This is for infoProc  */
             inittimer(10.0, 10.0, readInfo);
	else
             inittimer(delay_time, delay_time, readInfo);
    }
    else
    {
       acq_ok = initIPCinfo(RemoteHost);
       if (acq_ok)
       {
          if (debug)
             fprintf(stderr,"readInfo initializing socket\n");
          gettimeofday(&clock,NULL);  /* get time of connect */
          lastregister = PresentTime = clock.tv_sec;
          initsocket();
          acqregister();	/* register status port with acquisition */
          inittimer(2.0,2.0,readInfo);
       }
       else
       {
	  CurrentBlock.Acqstate = ACQ_INACTIVE;
	  update_chart(&CurrentBlock);
          inittimer(10.0,10.0,readInfo);
	}
    }
}



void
resize_panel(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	cal_chart_geom();
}



void
resize_shell(w, client_data, xev)
  Widget          w;
  XtPointer       client_data;
  XEvent          *xev;
{
	if (xev->type == ConfigureNotify)
	{
	    shell_h = xev->xconfigure.height;
            shell_w = xev->xconfigure.width;
	    cal_mainShell_loc();
	}
}


void
main_menu_proc(w, data, ev)
Widget          w;
caddr_t         data;
XEvent          *ev;
{
	int    k, m, p, evx, evy;
	float  fval, fval2;

	if (ev->xbutton.button != 1)
		return;
        switch (ev->type) {
        case   ButtonRelease:
		evx = ev->xbutton.x;
		evy = ev->xbutton.y;
		p = -1;
		for(m = 0; m < TOTALCHART; m++)
		{
		    if (disp_order[m] >= 0)
		    {
		        k = disp_order[m];
			if (disp_items[k].x <= evx && disp_items[k].y <= evy)
			    p = k;
		    }
		}
		if (p < 0)
		    return;
		evy = evy - disp_items[p].y;
		if (disp_items[p].cenbut && evy > disp_items[p].cenbut_y)
		{
		    evx = evx - disp_items[p].x;
		    if (evx >= disp_items[p].cenbut_x && 
			     evx <= disp_items[p].cenbut_x + icon_width)
		    {
			fval = (disp_items[p].upper - disp_items[p].lower) / 2.0;
			if (p == VTCHART)
			     fval2 = cur_val[p] / 10.0;
			else
			     fval2 = cur_val[p];
			disp_items[p].upper = fval2 + fval;
			disp_items[p].lower = fval2 - fval;
			disp_items[p].nupper = disp_items[p].upper;
			disp_items[p].nlower = disp_items[p].lower;
			cal_chart_dim(p);
	        	draw_chart(p);
			if (p == setup_item)
			{
		            sprintf(tmpstr, "%g ", disp_items[p].nupper);
			    set_text_item(upperWidget, tmpstr);
			    sprintf(tmpstr, "%g ", disp_items[p].nlower);
			    set_text_item(lowerWidget, tmpstr);
			}
		    }
		}
		    
		if (propActive)
		{
		    show_item_info(p, 1);
#ifdef OLIT
		    XtSetArg(args[0], XtNlabel, chart_name[p]);
		    XtSetValues(item_menu, args, 1);
#else
             	    XtSetArg(args[0], XmNmenuHistory, chartItems[p]);
             	    XtSetValues(item_menu, args, 1);
#endif
		}
		break;
	}
}

set_item_show(id, on)
int    id, on;
{
	int	m, p;

	if (on)
	{
	    for(m = 0; m < TOTALCHART; m++)
	    {
		if (disp_order[m] < 0)
		{
		     disp_order[m] = id;
		     break;
		}
	    }
	    disp_items[id].show = 1;
	    disp_items[id].nshow = 1;

#ifdef OLIT
            XtSetArg (args[0], XtNlabel, menu_close_label[id]);
#else
	    xmstr = XmStringCreate(menu_close_label[id], XmSTRING_DEFAULT_CHARSET);
   	    XtSetArg(args[0], XmNlabelString, xmstr);
#endif
	    XtSetValues(menu_buttons[id], args, 1);
	}
	else 
	{
	    p = TOTALCHART;
	    for(m = 0; m < TOTALCHART; m++)
	    {
		if (disp_order[m] == id)
		{
		    p = m;
		    disp_order[m] = -1;
		    break;
		}
	    }
	    for (m = p; m < TOTALCHART - 1; m++)
		    disp_order[m] = disp_order[m+1];
	    disp_order[TOTALCHART-1] = -1;
	    disp_items[id].show = 0;
	    disp_items[id].nshow = 0;
#ifdef OLIT
            XtSetArg (args[0], XtNlabel, menu_show_label[id]);
#else
	    xmstr = XmStringCreate(menu_show_label[id], XmSTRING_DEFAULT_CHARSET);
   	    XtSetArg(args[0], XmNlabelString, xmstr);
#endif
	    XtSetValues(menu_buttons[id], args, 1);
	}
}


void
menu_but_proc(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
	int    k, m, count;
	int    p, w, h; 

	k = (int) client_data;
	if (k == TOTALCHART)
	{
	    if (propShell == NULL)
            	create_prop_window();
	    if (propShell)
	    {
	        XtMoveWidget(propShell, 300, 400);
	       if (propActive)
	           XRaiseWindow (dpy, propShellId);
	       else
	           XtPopup (propShell, XtGrabNone);
	       propActive = 1;
	    }
	    return;
	}
	if (k == TOTALCHART + 1)
	{
	    exitproc();
	    exit(0);
	}

	count = 0;
	if (disp_items[k].show == 0) /*  show this item */
	{
	    if (k == setup_item)
                set_button(showWidget[1]);
	    set_item_show(k, ON);
	}
	else 
	{
	    if (k == setup_item)
                set_button(showWidget[0]);
	    set_item_show(k, OFF);
	}
	count = 0;
	for(m = 0; m < TOTALCHART; m++)
	{
	    if (disp_items[m].show)
	        count++;
	}
	if (count <= 0)
	{
	    XClearWindow(dpy, chartWin);
	    count = 1;
	}
	if (layout == HCHART)
	{	
	    if (chart_type == HISTORIC)
	    {
	        w = disp_items[0].width * count + count - 1;
	        h = disp_items[0].height;
	    }
	    else
	    {
	        w = disp_items[0].twidth * count + count - 1;
	        h = disp_items[0].theight;
	    }
	}
	else
	{
	    if (chart_type == HISTORIC)
	    {
	        w = disp_items[0].width;
	        h = disp_items[0].height * count + count - 1;
	    }
	    else
	    {
	        w = disp_items[0].twidth;
	        h = disp_items[0].theight * count + count - 1;
	    }
	}
	set_shell_dim(w, h);
}



int
set_font(font_name)
char    *font_name;
{
	if (fontInfo)
		XFreeFontInfo(NULL, fontInfo, 1);
        if ((fontInfo = XLoadQueryFont(dpy, font_name))==NULL)
	{
	     if (charWidth <= 0)
                fontInfo = XLoadQueryFont(dpy, "6x10");
	}
	fixWidth = 1;
	if (fontInfo != NULL)
        {
		xfont = fontInfo->fid;
	 	charHeight = fontInfo->max_bounds.ascent +
                                 fontInfo->max_bounds.descent;
            	charWidth = fontInfo->max_bounds.width;
		ch_ascent = fontInfo->max_bounds.ascent;
		ch_descent = charHeight - ch_ascent;
		if (charWidth != fontInfo->min_bounds.width)
			fixWidth = 0;
	        XSetFont(dpy, chart_gc, xfont);
		return(1);
        }
	else
	{
		/*  set random number */
		if (charWidth <= 0)
		{
	 	   charWidth = 10;
		   charHeight = 15;
		   ch_ascent = 11;
		   ch_descent = 4;
		}
		return(0);
	}
}


static int
create_icon_points(sbits, xpoints)
 char     *sbits;
 XPoint   *xpoints;
{
	int	x, y, num;
	char   *ptr;
	unsigned short  k;

	num = 0;
	for (y = 0; y < icon_height; y++)
	{
	   k = 1;
	   ptr = sbits + y * (icon_width / 8);
	   for (x = 0; x < icon_width; x++)
	   {
		if (*ptr & k)
		{
		    xpoints[num].x = x;
		    xpoints[num].y = y;
		    num++;
		}
		k = k << 1;
		if (k > 0x80)
	   	{
		    ptr++;
		    k = 1;
		}
	   }
	}
	return(num);
}

static
copy_icon_points(spnts, dpnts, sx, sy, num)
XPoint   *spnts, *dpnts;
int   sx, sy, num;
{
	int   k;

	for (k = 0; k < num; k++)
	{
		dpnts[k].x = spnts[k].x + sx;
		dpnts[k].y = spnts[k].y + sy;
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
char    *bname = "LightGray";
char    *fnp = "-fn";
char    *fname = "courr14";
char    geom[24];
int	nargc;

static XrmOptionDescRec options[] = {
{ "-fn", "*Acqmeter*fontList", XrmoptionSepArg, (caddr_t) NULL }
};

void
focus_p(c, data, e)
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
        	if (count > 5)
        	   XtRemoveEventHandler(mainShell,FocusChangeMask, False, focus_p, NULL);
	    }
        }
}

create_main_window(argc, argv)
int     argc;
char   **argv;
{
	Dimension   w1, w2, h1, h2;
        int    n, k;
        char   framelabel[80];
	char   *vendor;
	EventMask   mask;

	strcpy(geom, "-400+0");
	strcpy(winBgName, "white");
	strcpy(winFgName, "black");
	strcpy(fontName, "6x10");
	strcpy(gridColor, "gray");
	n = 1;
	while (n < argc - 1)
	{
	     if (strcmp(argv[n], "-fg") == 0)
	     {
		if (argv[n+1] != NULL)
			strcpy(winFgName, argv[n+1]);
		n++;
	     }
             else if (strcmp("-bg", argv[n]) == 0)
	     {
		if (argv[n+1] != NULL)
			strcpy(winBgName, argv[n+1]);
		n++;
	     }
	     n++;
	}

	nargv[0] = argv[0];
	nargv[1] = geomp;
	nargv[2] = geom;
	nargv[3] = fnp;
	nargv[4] = fname;
	nargv[5] = backp;
	nargv[6] = bname;
	nargc = 7;
	n = 1;
	while (n < argc && nargc < 19)
	    nargv[nargc++] = argv[n++];
	nargv[nargc] = NULL;

	init_items();

	mainShell = XtInitialize("Acqmeter", "Acqmeter", options,
                         XtNumber(options), (Cardinal *)&nargc, nargv);

        sprintf(framelabel,"%s Acqmeter",RemoteHost);

        n = 0;
        XtSetArg (args[n], XtNtitle, framelabel); n++;
	XtSetValues(mainShell,args,n);
        dpy = XtDisplay(mainShell);
        screen = DefaultScreen(dpy);
        screenHeight = DisplayHeight (dpy, screen);
        screenWidth = DisplayWidth (dpy, screen);
	winDepth = XDisplayPlanes (dpy, screen);
        XtSetArg(args[0], XtNiconPixmap, XCreateBitmapFromData (dpy,
                                    XtScreen(mainShell)->root, acqm_icon_bits,
                                    acqm_icon_width, acqm_icon_height));
        XtSetValues (mainShell, args, 1);
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


#ifdef MOTIF
	
        n = 0;
	XtSetArg (args[n], XmNwidth, 200);  n++;
	XtSetArg (args[n], XmNheight, 100);  n++;
	mainframe = XtCreateManagedWidget("Acqmeter", xmFormWidgetClass,
                                mainShell, args, n);
	n =0;
	XtSetArg (args[n], BORDERWIDTH, 1);  n++;
	XtSetArg (args[n], XmNmarginHeight, 0);  n++;
	XtSetArg (args[n], XmNmarginWidth, 0);  n++;
	XtSetArg (args[n], XmNwidth, 200);  n++;
	XtSetArg (args[n], XmNheight, 100);  n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNresizable, TRUE);  n++;
	chartPanel = XtCreateManagedWidget("", xmDrawingAreaWidgetClass,
                         mainframe, args, n);
	n =0;
	XtSetArg (args[n], XmNforeground, &winFg);  n++;
	XtSetArg (args[n], XmNbackground, &winBg);  n++;
	XtGetValues(chartPanel, args, n);
#else
	n =0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	mainframe = XtCreateManagedWidget("Acqmeter", formWidgetClass,
                                mainShell, args, n);

	n =0;
	XtSetArg (args[n], BORDERWIDTH, 1);  n++;
	XtSetArg (args[n], XtNwidth, 200);  n++;
	XtSetArg (args[n], XtNheight, 100);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg (args[n], XtNyAttachBottom, TRUE); n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg (args[n], XtNyOffset, 0); n++;
	XtSetArg (args[n], XtNyResizable, TRUE);  n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
        chartPanel = XtCreateManagedWidget("", drawAreaWidgetClass,
			 mainframe, args, n);
	n =0;
	XtSetArg (args[n], XtNforeground, &winFg);  n++;
	XtSetArg (args[n], XtNbackground, &winBg);  n++;
	XtGetValues(chartPanel, args, n);
#endif
        XtAddEventHandler(mainShell,SubstructureNotifyMask, False, resize_shell,
                         NULL);

	XtAddEventHandler(mainShell,FocusChangeMask, False, focus_p, NULL);
	XtRealizeWidget (mainShell);
#ifdef  OLIT
	OlAddCallback( mainShell, XtNwmProtocol, mainShell_exit, NULL );
#else
	deleteAtom = XmInternAtom(dpy, "WM_DELETE_WINDOW", FALSE);
	XmAddProtocolCallback(mainShell, XM_WM_PROTOCOL_ATOM(mainShell),
			deleteAtom, mainShell_exit, 0);
#endif
        shellId = XtWindow(mainShell);
        chartWin = XtWindow(chartPanel);
	XtSetArg (args[0], XtNwidth, &w1);
	XtSetArg (args[1], XtNheight, &h1);
	XtGetValues(mainShell, args, 2);
	XtSetArg (args[0], XtNwidth, &w2);
	XtSetArg (args[1], XtNheight, &h2);
	XtGetValues(chartPanel, args, 2);
	boardH = h1 - h2;
	boardW = w1 - w2;
	chart_gc = XCreateGC(dpy, chartWin, 0, 0);
        XtAddEventHandler(chartPanel,StructureNotifyMask, False, resize_panel,
                         NULL);

	set_font(fontName);
	cal_chart_geom();

	for(k = 0; k < TOTALCHART; k++)
	    set_item_def_color(k);

#ifdef OLIT
	XtAddCallback(chartPanel, XtNexposeCallback, expose_panel, NULL);
#else
	XtAddCallback(chartPanel, XmNexposeCallback, expose_panel, NULL);
#endif
	mask = ButtonPressMask | ButtonReleaseMask;
	XtAddEventHandler(chartPanel, mask, False, main_menu_proc, NULL);
	create_main_menu();
	icon_num = create_icon_points(icon_bits, cutpnts);
	icon_x = -1;
	icon_y = -1;
	cent_num = create_icon_points(center_bits, centerpnts);
	cent_x = -1;
	cent_y = -1;
	cal_mainShell_loc();
}



/*-------------------------------------------------------------
|  inittimer/0  set event timer for count down
|       pass function pointer to be activated with alarm
|
+------------------------------------------------------------*/
void (*timerfunc)();
XtIntervalId  timerId = NULL;

timerproc()
{
        if(timerId == NULL)
            return;
        timerId = NULL;
	if (timerfunc != NULL)
            timerfunc();
}

inittimer(timsec,intvl,funccall)
double timsec;
double intvl;
void (*funccall) ();
{
    unsigned long  msec;

    if(timerId != NULL)
    {
         XtRemoveTimeOut(timerId);
         timerId = NULL;
    }
    msec = timsec * 1000;  /* milliseconds */
    timerfunc = funccall;
    if(funccall != NULL)
    {
         timerId = XtAddTimeOut(msec, timerproc, NULL);
    }
}


acqmeter_window_loop()
{
/**
    XtAddEventHandler(mainShell,SubstructureNotifyMask, False, resize_shell,
                         NULL);
**/
    XtMainLoop();
}

	


init_items()
{
	int	item;
	FILE    *fd;

	for (item = 0; item < TOTALCHART; item++)
	{
		disp_items[item].show = 0;
		disp_items[item].nshow = 0;
		disp_items[item].active = 0;
		disp_items[item].x = 0;
		disp_items[item].y = 0;
		disp_items[item].width = 0;
		disp_items[item].height = 0;
		disp_items[item].twidth = 60;
		disp_items[item].theight = 150;
		disp_items[item].mode = LINEMODE;
		disp_items[item].nmode = LINEMODE;
		disp_items[item].direction = HCHART;
		disp_items[item].ndirection = HCHART;
		disp_items[item].points = 0;
		disp_items[item].disp_len = 0;
		disp_items[item].data_len = 0;
		disp_items[item].dindex = 0;
		disp_items[item].data = NULL;
		disp_items[item].grid = 1;
		disp_items[item].ngrid = 1;
		disp_items[item].log = 0;
		disp_items[item].nlog = 0;
		disp_items[item].cenbut = 0;
		disp_items[item].ncenbut = 0;
		disp_items[item].fname_len = 0;
		disp_items[item].nfname_len = 0;
		disp_items[item].fd = NULL;
		disp_items[item].fname = NULL;
		disp_items[item].nfname = NULL;
		strcpy(disp_items[item].bcolor, winBgName);
		strcpy(disp_items[item].fcolor, winFgName);
		strcpy(disp_items[item].gcolor, gridColor);
		disp_order[item] = -1;
	}
	layout = HCHART;
	nlayout = HCHART;
	chart_type = HISTORIC;
	nchart_type = HISTORIC;
	disp_items[LOCKCHART].show = 1;
	disp_items[LOCKCHART].nshow = 1;
	disp_items[LOCKCHART].setval = 0.0;
	disp_items[LOCKCHART].upper = 100.0;
	disp_items[LOCKCHART].nupper = 100.0;
	disp_items[LOCKCHART].lower = 0.0;
	disp_items[LOCKCHART].nlower = 0.0;
	disp_items[VTCHART].setval = 25.0;
	disp_items[VTCHART].upper = 100.0;
	disp_items[VTCHART].nupper = 100.0;
	disp_items[VTCHART].lower = 0.0;
	disp_items[VTCHART].nlower = 0.0;
	disp_items[SPINCHART].setval = 20.0;
	disp_items[SPINCHART].upper = 30.0;
	disp_items[SPINCHART].nupper = 30.0;
	disp_items[SPINCHART].lower = 0.0;
	disp_items[SPINCHART].nlower = 0.0;
	disp_order[0] = LOCKCHART;

        fd = NULL;
	if (user_dir == NULL)
             return;
        sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
        if((fd = fopen(tmpstr,"r")) == NULL)
             return;
	read_prefer_file(fd, OFF);
	disp_items[LOCKCHART].setval = disp_items[LOCKCHART].lower; 
}


clear_item_vals()
{
	int	item;

	for (item = 0; item < TOTALCHART; item++)
	{
		disp_items[item].points = 0;
		disp_items[item].dindex = 0;
	}
}



#ifdef OLIT

set_label_item(w, label)
Widget   w;
char     *label;
{
	XtSetArg (args[0],XtNstring, label);
	XtSetValues(w, args, 1);
}


set_text_item(w, label)
Widget   w;
char     *label;
{
	XtSetArg (args[0],XtNstring, label);
	XtSetValues(w, args, 1);
}


char * get_text_item(w)
Widget   w;
{
	int	len;
	static  char    tstr[120];
	static  char   *data, *attr;

	tstr[0] = '\0';
	XtSetArg (args[0], XtNstring, &data);
        XtGetValues(w, args, 1);
	if (data == NULL)
		return(tstr);
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
	return(tstr);
}	



#else

set_label_item(w, label)
Widget   w;
char     *label;
{
	XmString        mstring;

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
	     len = (int)strlen(tstr);
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

store_item_info(item)
int	item;
{
	int	len;
	float   fval;
	static  char   *data;

	data = get_text_item(bgWidget);
	strcpy( disp_items[item].bcolor, data);
	data = get_text_item(fgWidget);
	strcpy( disp_items[item].fcolor, data);
	data = get_text_item(gcolorWidget);
	strcpy( disp_items[item].gcolor, data);
	data = get_text_item(upperWidget);
	if (strlen(data) > 0)
	{
	    if (sscanf(data, "%f", &fval) == 1)
		disp_items[item].nupper = fval;
	    else
	        disp_items[item].nupper = 0.0;
	}
	else
	    disp_items[item].nupper = 0.0;
	data = get_text_item(lowerWidget);
	if (strlen(data) > 0)
	{
	    if (sscanf(data, "%f", &fval) == 1)
	        disp_items[item].nlower = fval;
	    else
	        disp_items[item].nlower = 0.0;
	}
	else
	    disp_items[item].nlower = 0.0;
/**
	if (item != LOCKCHART)
	{
	    if (disp_items[item].nlower > disp_items[item].setval)
	        disp_items[item].nlower =  disp_items[item].setval;
	}
**/
	if (disp_items[item].nupper < disp_items[item].nlower)
	    disp_items[item].nupper = disp_items[item].nlower;
/***
	data = get_text_item(saveWidget);
	len = strlen(data);
	if ( len > disp_items[item].nfname_len)
	{
	    if (disp_items[item].nfname != NULL)
		free(disp_items[item].nfname);
	    disp_items[item].nfname = (char *) malloc(len + 12);
	    disp_items[item].nfname_len = len + 10;
	}
	if (  disp_items[item].nfname != NULL)
	    strcpy( disp_items[item].nfname, data);
***/
}



#ifdef  OLIT


Widget 
create_text_widget(parent, mode, label_1, label_2)
Widget  parent;
int	mode;
char    *label_1, *label_2;
{
	int	n, k;
	Widget  widget, formWidget, lwidget;

	n = 0;
   	formWidget = (Widget)XtCreateManagedWidget(parent, formWidgetClass,
                                     parent, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg(args[n], XtNxOffset, 4); n++;
	XtSetArg(args[n], XtNxAttachOffset, 4); n++;
	XtSetArg(args[n], XtNstring, label_1); n++;
	XtSetArg(args[n], XtNstrip, FALSE); n++;
	lwidget = XtCreateManagedWidget("", staticTextWidgetClass,
                                     formWidget, args, n);

	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxRefWidget, lwidget); n++;
	XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
        XtSetArg(args[n], XtNyAttachBottom, TRUE);
	XtSetArg(args[n], XtNxResizable, TRUE);  n++;
	XtSetArg(args[n], XtNstrip, FALSE); n++;
        XtSetArg(args[n], XtNrecomputeSize, TRUE); n++;
	XtSetArg(args[n], XtNgravity, WestGravity);  n++;
	if (label_2 != NULL)
		XtSetArg(args[n], XtNstring, label_2);
	else
		XtSetArg(args[n], XtNstring, "  ");
	n++;
	if (mode == LABELMODE)
	    widget = XtCreateManagedWidget("", staticTextWidgetClass,
                                     formWidget, args, n);
	else
	    widget = XtCreateManagedWidget("", textFieldWidgetClass,
                                     formWidget, args, n);

	return(widget);
}

Widget 
create_time_widget(parent)
Widget  parent;
{
	int	n;
	Widget  twidget, formWidget, lwidget;

	n = 0;
   	formWidget = (Widget)XtCreateManagedWidget(parent, formWidgetClass,
                                     parent, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg(args[n], XtNxOffset, 4); n++;
	XtSetArg(args[n], XtNxAttachOffset, 4); n++;
	XtSetArg(args[n], XtNstring, "Time Interval: "); n++;
	XtSetArg(args[n], XtNstrip, FALSE); n++;
	lwidget = XtCreateManagedWidget("", staticTextWidgetClass,
                                     formWidget, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxRefWidget, lwidget); n++;
        XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
	XtSetArg(args[n], XtNstring, "  5"); n++;
	XtSetArg(args[n], XtNcharsVisible, 6); n++;
	twidget = XtCreateManagedWidget("", textFieldWidgetClass,
                                     formWidget, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxRefWidget, twidget); n++;
        XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
	XtSetArg(args[n], XtNstring, "second"); n++;
	XtCreateManagedWidget("", staticTextWidgetClass,
                                     formWidget, args, n);
	return(twidget);
}

Widget 
create_ex_widget(parent, label)
Widget  parent;
char    *label;
{
	int	n;
	Widget  twidget, formWidget, lwidget;

	n = 0;
   	formWidget = (Widget)XtCreateManagedWidget(parent, formWidgetClass,
                                     parent, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg(args[n], XtNxOffset, 4); n++;
	XtSetArg(args[n], XtNxAttachOffset, 4); n++;
	XtSetArg(args[n], XtNstring, label); n++;
	XtSetArg(args[n], XtNstrip, FALSE); n++;
	lwidget = XtCreateManagedWidget("", staticTextWidgetClass,
                                     formWidget, args, n);
	n = 0;
	XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg(args[n], XtNxRefWidget, lwidget); n++;
        XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
	XtSetArg(args[n], XtNxOffset, 8); n++;
	twidget = XtCreateManagedWidget ("",
                        exclusivesWidgetClass, formWidget, args, n);
	return(twidget);
}

Widget
create_ex_button(parent, label, func, val)
Widget  parent;
char    *label;
void    (*func)();
int	val;
{
	Widget  button;

        n = 0;
        XtSetArg (args[n], XtNlabel, label);  n++;
        button = XtCreateManagedWidget("button",
                        rectButtonWidgetClass, parent, args, n);
        XtAddCallback(button, XtNselect, func, (XtPointer) val);
        return(button);
}


#else

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
/**
   	XtSetArg(args[n], XmNheight, butHeight + 2);  n++;
**/
   	formWidget = (Widget)XmCreateForm(parent, "", args, n);
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
            XtSetArg (args[n], XmNalignment, XmALIGNMENT_BEGINNING);  n++; 
	    widget = (Widget)XmCreateLabel(formWidget, "text", args, n);
	    XtManageChild (widget);
/*
   	    XtFree(mstring);
*/
            XmStringFree(mstring);
	}
	else
	{
	    if (label_2 != NULL)
	    {
                XtSetArg (args[n], XmNvalue, label_2);  n++;
	    }
	    widget = (Widget)XmCreateText(formWidget, "text", args, n);
	    XtManageChild (widget);
	}
	return(widget);
}


Widget
create_time_widget(parent)
Widget  parent;
{
        int     n;
        Widget  twidget, formWidget, lwidget;

        n = 0;
   	formWidget = (Widget)XmCreateForm(parent, "", args, n);
   	XtManageChild (formWidget);
	n =0;
   	xmstr = XmStringLtoRCreate("Time Interval: ", XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[n], XmNlabelString, xmstr);  n++;
   	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	lwidget = (Widget)XmCreateLabel(formWidget, "label", args, n);
	XtManageChild (lwidget);
/*
   	XtFree(xmstr);
*/
        XmStringFree(xmstr);

	n = 0;
	XtSetArg (args[n], XmNvalue, "  5 "); n++;
	XtSetArg (args[n], XmNshadowThickness, 1); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftWidget, lwidget); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNmarginHeight, 1); n++;
	XtSetArg (args[n], XmNverifyBell, FALSE); n++;
	twidget = (Widget)XmCreateText(formWidget, "text", args, n);
	XtManageChild (twidget);

	n = 0;
   	xmstr = XmStringLtoRCreate("second", XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        XtSetArg (args[n], XmNalignment, XmALIGNMENT_BEGINNING);  n++; 
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftWidget, twidget); n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	lwidget = (Widget)XmCreateLabel(formWidget, "label", args, n);
	XtManageChild (lwidget);
/*
   	XtFree(xmstr);
*/
        XmStringFree(xmstr);

	return(twidget);
}

Widget
create_ex_widget(parent, label)
Widget  parent;
char    *label;
{
        int     n;
        Widget  twidget, formWidget, lwidget;

        n = 0;
   	formWidget = (Widget)XmCreateForm(parent, "", args, n);

	n =0;
   	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   	XtSetArg (args[n], XmNlabelString, xmstr);  n++;
   	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
   	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	lwidget = (Widget)XmCreateLabel(formWidget, "label", args, n);
	XtManageChild (lwidget);
/*
   	XtFree(xmstr);
*/
        XmStringFree(xmstr);

	n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftWidget, lwidget); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
        XtSetArg (args[n], XmNpacking, XmPACK_COLUMN);  n++;
        twidget = (Widget) XmCreateRadioBox(formWidget, "", args, n);
        XtManageChild (twidget);
        XtManageChild (formWidget);
	return(twidget);
}


Widget
create_ex_button(parent, label, func, val)
Widget  parent;
char    *label;
void    (*func)();
int     val;
{
        Widget  button;

	n =0;
        button = (Widget) XmCreateToggleButtonGadget(parent, label, args, n);
        XtManageChild (button);
	XtAddCallback(button, XmNvalueChangedCallback, func, (XtPointer) val);
	return(button);
}



#endif



Widget
create_popup_shell(parent, title)
char  *title;
{
	Widget  shell;

	n = 0;
	XtSetArg (args[n], XtNallowShellResize, TRUE); n++;
        XtSetArg (args[n], XtNtitle, title); n++;
	shell = XtCreatePopupShell("Acqmeter",
			 transientShellWidgetClass, parent, args, n);
	return(shell);
}



show_item_info(item, to_store)
int	item, to_store;
{
	int	vitem;

	if (item < 0)
		return;
	if (to_store && item == setup_item)
		return;
	if (to_store && setup_item >= 0)
	       store_item_info(setup_item);
	set_text_item(bgWidget, disp_items[item].bcolor);
	set_text_item(fgWidget, disp_items[item].fcolor);
	set_text_item(gcolorWidget, disp_items[item].gcolor);
	if (item == LOCKCHART)
	     strcpy(tmpstr, "    ");
	else
	     sprintf(tmpstr, "  %g ", disp_items[item].setval);
	set_label_item(setvWidget, tmpstr);
	sprintf(tmpstr, "%g ", disp_items[item].nupper);
	set_text_item(upperWidget, tmpstr);
	sprintf(tmpstr, "%g ", disp_items[item].nlower);
	set_text_item(lowerWidget, tmpstr);

	setup_item = item;
#ifdef  OLIT
	XtSetArg(args[0], XtNset, TRUE);
	if (disp_items[item].nmode == SOLIDMODE)
	    XtSetValues(grafWidget[1], args, 1);
	else
	    XtSetValues(grafWidget[0], args, 1);


/**
	if (disp_items[item].nlog)
	    XtSetValues(logWidget[1], args, 1);
	else
	    XtSetValues(logWidget[0], args, 1);
**/

	if (disp_items[item].nshow)
	    XtSetValues(showWidget[1], args, 1);
	else
	    XtSetValues(showWidget[0], args, 1);

	if (disp_items[item].ncenbut)
	    XtSetValues(centerWidget[1], args, 1);
	else
	    XtSetValues(centerWidget[0], args, 1);

	XtSetValues(gmodeWidget[disp_items[item].ngrid], args, 1);
#else
	if (disp_items[item].nmode == SOLIDMODE)
	    XmToggleButtonSetState(grafWidget[1], TRUE, TRUE);
	else
	    XmToggleButtonSetState(grafWidget[0], TRUE, TRUE);
	if (disp_items[item].nshow)
	    XmToggleButtonSetState(showWidget[1], TRUE, TRUE);
	else
	    XmToggleButtonSetState(showWidget[0], TRUE, TRUE);

	if (disp_items[item].ncenbut)
	    XmToggleButtonSetState(centerWidget[1], TRUE, TRUE);
	else
	    XmToggleButtonSetState(centerWidget[0], TRUE, TRUE);
	XmToggleButtonSetState(gmodeWidget[disp_items[item].ngrid],TRUE,TRUE);
#endif


/**
	if (disp_items[item].nfname != NULL)
	    set_text_item(saveWidget, disp_items[item].nfname);
	else
	    set_text_item(saveWidget, "  ");
***/
}



void
select_item(w, client_data, call_data)
  Widget          w;
  caddr_t          client_data;
  caddr_t          call_data;
{
        int     item, len;

        item = (int)client_data;
#ifdef OLIT
	XtSetArg(args[0], XtNlabel, chart_name[item]);
	XtSetValues(item_menu, args, 1);
#endif
        show_item_info(item, 1);
}



#ifdef OLIT

create_item_menu(parent)
Widget  parent;
{
	Widget   pulldown;
	Widget   tmpwidget, panwidget;
	int	 item, len;

	pulldown = create_row_col(parent, HORIZONTAL, 0, 0);
	n = 0;
        XtSetArg(args[n], XtNstrip, FALSE); n++;
	XtSetArg(args[n], XtNstring, "  Chart Item: "); n++;
	XtCreateManagedWidget("", staticTextWidgetClass,
                                     pulldown, args, n);
	n = 0;
	XtSetArg(args[n], XtNpushpin, (XtArgVal)OL_OUT); n++;
        XtSetArg(args[n], XtNlabelType, (XtArgVal)OL_STRING); n++;
        XtSetArg(args[n], XtNlabelJustify, (XtArgVal)OL_LEFT); n++;
        XtSetArg(args[n], XtNrecomputeSize, (XtArgVal)TRUE); n++;
	XtSetArg (args[n], XtNinputFocusColor, focusPix);  n++;
        item_menu = XtCreateManagedWidget("Menu",
                        menuButtonWidgetClass, pulldown, args, n);
	n = 0;
	XtSetArg(args[n], XtNmenuPane, (XtArgVal)&panwidget); n++;
        XtGetValues(item_menu, args, n);
        for(item = 0; item < TOTALCHART; item++)
        {
	   n = 0;
	   XtSetArg(args[n], XtNlabel, chart_name[item]); n++;
	   XtSetArg (args[n], XtNinputFocusColor, focusPix);  n++;
   	   tmpwidget = XtCreateManagedWidget("",
                        oblongButtonWidgetClass, panwidget, args, n);
           XtAddCallback(tmpwidget, XtNselect, select_item, (XtPointer)item);
        }
	XtSetArg(args[0], XtNlabel, chart_name[0]);
	XtSetValues(item_menu, args, 1);
}


create_main_menu()
{
	int	k;
	Widget  pwidget, mbut;

	n = 0;
	XtSetArg (args[n], XtNallowShellResize, TRUE); n++;
	XtSetArg (args[n], XtNresizeCorners, FALSE); n++;
	menuShell = XtCreatePopupShell("Menu",
			 menuShellWidgetClass, chartPanel, args, n);
	focusPix = get_focus_pixel(winBg);

	n = 0;
	XtSetArg (args[n], XtNmenuPane, &pwidget); n++;
	XtGetValues (menuShell, args, n);

	for(k = 0; k < TOTALCHART; k++)
	{
            n = 0;
	    if (disp_items[k].show)
	        XtSetArg (args[n], XtNlabel, menu_close_label[k]);
	    else
	        XtSetArg (args[n], XtNlabel, menu_show_label[k]);
	    n++;
	    XtSetArg (args[n], XtNinputFocusColor, focusPix);  n++;
	    menu_buttons[k] = XtCreateManagedWidget("",
                        oblongButtonWidgetClass, pwidget, args, n);
	    XtAddCallback(menu_buttons[k], XtNselect,menu_but_proc,(XtPointer)k);
	}
        n = 0;
	XtSetArg (args[n], XtNlabel, "Properties...");  n++;
	menu_buttons[k] = XtCreateManagedWidget("",
                        oblongButtonWidgetClass, pwidget, args, n);
	XtAddCallback(menu_buttons[k], XtNselect, menu_but_proc, (XtPointer) k);
	k++;
        n = 0;
	XtSetArg (args[n], XtNlabel, "Exit");  n++;
	menu_buttons[k] = XtCreateManagedWidget("",
                        oblongButtonWidgetClass, pwidget, args, n);
	XtAddCallback(menu_buttons[k], XtNselect, menu_but_proc, (XtPointer) k);
}

#else

create_item_menu(parent)
Widget  parent;
{
	Widget   pulldown;
	Widget   tmpwidget;
	XmString xmstr;
	int	 item, len;

	pulldown = (Widget)XmCreatePulldownMenu(parent, "pulldown", NULL, 0);
        for(item = 0; item < TOTALCHART; item++)
        {
	   xmstr = XmStringCreate(chart_name[item], XmSTRING_DEFAULT_CHARSET);
   	   XtSetArg(args[0], XmNlabelString, xmstr);
   	   chartItems[item] = (Widget)XmCreatePushButtonGadget(pulldown, "", args, 1);
           XtAddCallback(chartItems[item], XmNactivateCallback, select_item, item);
/*
   	   XmStringFree(xmstr);
*/
           XmStringFree(xmstr);
	   XtManageChild(chartItems[item]);
	}
	xmstr = XmStringCreate("   Chart  Item:", XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr);  n++;
	XtSetArg(args[n], XmNsubMenuId, pulldown); n++;
	XtSetArg(args[n], XmNmenuHistory, chartItems[0]); n++;
	item_menu = (Widget) XmCreateOptionMenu(parent, " ", args, n);
/*
   	XmStringFree(xmstr);
*/
        XmStringFree(xmstr);
	XtManageChild(item_menu);
}


void
show_menu_proc(w, data, ev)
Widget          w;
caddr_t         data;
XEvent          *ev;
{
	int    k, m, p;

	if (ev->xbutton.button != 3)
		return;
	XmMenuPosition(menuShell, ev);
	XtManageChild(menuShell);
}


create_main_menu()
{
	int	k;
	Widget  pwidget, mbut;

	menuShell = (Widget) XmCreatePopupMenu(chartPanel, "Menu", NULL, 0);

	for(k = 0; k < TOTALCHART; k++)
	{
            n = 0;
	    if (disp_items[k].show)
	        xmstr = XmStringCreate(menu_close_label[k], XmSTRING_DEFAULT_CHARSET);
	    else
	        xmstr = XmStringCreate(menu_show_label[k], XmSTRING_DEFAULT_CHARSET);
   	    XtSetArg(args[0], XmNlabelString, xmstr);
	    menu_buttons[k] = (Widget)XmCreatePushButtonGadget(menuShell, "",args, 1);
	    XtAddCallback(menu_buttons[k], XmNactivateCallback,menu_but_proc,(XtPointer)k);
/*
   	   XmStringFree(xmstr);
*/
           XmStringFree(xmstr);
	   XtManageChild(menu_buttons[k]);
	}
	xmstr = XmStringCreate("Properties...", XmSTRING_DEFAULT_CHARSET);
   	XtSetArg(args[0], XmNlabelString, xmstr);
	menu_buttons[k] = (Widget)XmCreatePushButtonGadget(menuShell, "",args, 1);
	XtAddCallback(menu_buttons[k], XmNactivateCallback,menu_but_proc,(XtPointer)k);
	XtManageChild(menu_buttons[k]);
	k++;
	xmstr = XmStringCreate("Exit", XmSTRING_DEFAULT_CHARSET);
   	XtSetArg(args[0], XmNlabelString, xmstr);
	menu_buttons[k] = (Widget)XmCreatePushButtonGadget(menuShell, "",args, 1);
	XtAddCallback(menu_buttons[k], XmNactivateCallback,menu_but_proc,(XtPointer)k);
	XtManageChild(menu_buttons[k]);
	XtAddEventHandler(chartPanel, ButtonPressMask, False, show_menu_proc, NULL);
}

#endif



set_item_def_color(id)
int	id;
{
	XColor xcol1, xcol2;
	
	if(XAllocNamedColor(dpy, cmap, disp_items[id].bcolor, &xcol1, &xcol2))
		disp_items[id].bg = xcol1.pixel;
	else
		disp_items[id].bg = winBg;
	if(XAllocNamedColor(dpy, cmap, disp_items[id].fcolor, &xcol1, &xcol2))
		disp_items[id].fg = xcol1.pixel;
	else
		disp_items[id].fg = winFg;
	if(XAllocNamedColor(dpy, cmap, disp_items[id].gcolor, &xcol1, &xcol2))
		disp_items[id].grPix = xcol1.pixel;
	else
		disp_items[id].grPix = winFg;
}




set_win_bg()
{
	XColor xcol1, xcol2;

	if (strcmp(winBgName, old_BgName) != 0)
	{
	   if(XAllocNamedColor(dpy, cmap, winBgName, &xcol1, &xcol2))
	   {
	        strcpy(old_BgName, winBgName);
		winBg = xcol1.pixel;
		XSetWindowBackground(dpy, chartWin, winBg);
		XSetWindowBackground(dpy, shellId, winBg);
	   }
	   else
	   {
		sprintf(errstr, "Color %s is not available", winBgName);
		disp_error(errstr, 1);
		disp_error(" ", 2);
	   }
	}
}


void
layout_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	nlayout = (int)client_data;
}

void
type_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	nchart_type = (int)client_data;
	if (nchart_type == HISTORIC)
	   XtSetArg (args[0], XtNsensitive, TRUE);
	else
	   XtSetArg (args[0], XtNsensitive, FALSE);
	XtSetValues(grafWidget[0], args, 1);
	XtSetValues(grafWidget[1], args, 1);
}

void
graphic_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	disp_items[setup_item].nmode = (int) client_data;
}


void
dir_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
	disp_items[setup_item].ndirection = (int) client_data;
}

void
log_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	disp_items[setup_item].nlog = (int) client_data;

}

void
show_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	disp_items[setup_item].nshow = (int) client_data;
}

void
cenbut_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	disp_items[setup_item].ncenbut = (int) client_data;
}

void
gmode_cb(widget, client_data, call_data)
  Widget          widget;
  XtPointer       client_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	Boolean    set;

	XtSetArg (args[0], XtNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	    return;
#endif
	disp_items[setup_item].ngrid = (int) client_data;
}

void
apply_cb()
{
	int	k, active, changed;
	int	w, h;
	float   fval;
	char    *data;

	active = 0;
	changed = 0;
	store_item_info(setup_item);
	data = get_text_item(timeWidget);
	if (strlen(data) > 0)
	{
	    if (sscanf(data, "%f", &fval) == 1)
		delay_time = fval;
	}
	data = get_text_item(fontWidget);
	if (strlen(data) > 0)
	{
	    if (strcmp(data, fontName) != 0)
	    {
		if (!set_font(data))
		{
		    disp_error("Could not open font:", 1);
		    disp_error(data, 2);
		}
		else
		    strcpy(fontName, data);
	    }
	}
	for(k = 0; k < TOTALCHART; k++)
	{
	    set_item_def_color(k);
	    if (disp_items[k].show != disp_items[k].nshow)
	    {
		set_item_show(k, disp_items[k].nshow);
		changed++;
	    }
	    disp_items[k].mode = disp_items[k].nmode;
	    disp_items[k].direction = disp_items[k].ndirection;
	    disp_items[k].upper = disp_items[k].nupper;
	    disp_items[k].lower = disp_items[k].nlower;
	    disp_items[k].grid = disp_items[k].ngrid;
	    disp_items[k].log = disp_items[k].nlog;
	    disp_items[k].cenbut = disp_items[k].ncenbut;
/***
	    if (disp_items[k].fname_len < disp_items[k].nfname_len)
	    {
		if (disp_items[k].fname != NULL)
		    free(disp_items[k].fname);
		disp_items[k].fname = (char *)malloc(disp_items[k].nfname_len);
		disp_items[k].fname_len = disp_items[k].nfname_len;
	    }
	    if (disp_items[k].nfname != NULL)
	        sprintf(disp_items[k].fname, disp_items[k].nfname);
***/
	    if (disp_items[k].show)
		active++;
	}
	disp_items[LOCKCHART].setval = disp_items[LOCKCHART].lower;
	if (active <= 0)
	{
	    XClearWindow(dpy, chartWin);
	    active = 1;
	}
	if (layout != nlayout || changed || chart_type != nchart_type)
	{
	    layout = nlayout;
	    chart_type = nchart_type;
	    if (chart_type == HISTORIC)
	    {
	    	if (layout == HCHART)
	    	{	
	    	   w = disp_items[0].width * active + active - 1;
	    	   h = disp_items[0].height;
	    	}
	    	else
	    	{
		   w = disp_items[0].width;
		   h = disp_items[0].height * active + active - 1;
	    	}
	    }
	    else
	    {
	    	if (layout == HCHART)
	    	{	
	    	   w = disp_items[0].twidth * active + active - 1;
	    	   h = disp_items[0].theight;
	    	}
	    	else
	    	{
		   w = disp_items[0].twidth;
		   h = disp_items[0].theight * active + active - 1;
	    	}
	    }

	    set_shell_dim(w, h);
	}

	data = get_text_item(hostWidget);
	if (strlen(data) > 0)
	{
	    if (strcmp(RemoteHost, data) != 0)
	    {
		change_host(data);
		inittimer(2.0,2.0,readInfo);
	    }
	}
	for(k = 0; k < TOTALCHART; k++)
	{
	    if (disp_items[k].show)
	    {
	        cal_chart_dim(k);
	        draw_chart(k);
	    }
	}
}



change_host(host_name)
char   *host_name;
{
	inittimer(0.0,0.0,NULL);
	sprintf(tmpstr, "%s Acqmeter", host_name);
        XtSetArg (args[0], XtNtitle, tmpstr);
	XtSetValues(mainShell,args, 1);
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
	clear_item_vals();
}



void
file_cb(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	  x, y;
	Window    win;

	store_item_info(setup_item);
        if (fileShell == NULL)
             create_fileWindow();
        if (fileShell)
	{
	     create_file_list();
             XTranslateCoordinates(dpy,propShellId,RootWindow(dpy, 0),
				0,0,&x,&y,&win);
	     XtMoveWidget(fileShell, x+30, y+50);
             XtPopup (fileShell, XtGrabNone);
	     XRaiseWindow(dpy, fileID);
	}
}


create_file_list()
{
        int       k;
        def_rec   *plist;
#ifdef OLIT
        OlListItem olitem;
#endif

        create_def_list();
        plist = def_list;
        k = 0;
        while (plist != NULL)
        {
             k++;
             plist->id = k;
#ifdef OLIT
             olitem.label_type = (OlDefine)OL_STRING;
             olitem.label = (XtPointer)plist->name;
             olitem.user_data = (XtPointer)k;
             olitem.mnemonic = NULL;
             olitem.attr = k;
             plist->item = (*AddListItem)(fileScrWin, 0, 0, olitem);
#else
             xmstr = XmStringLtoRCreate(plist->name, XmSTRING_DEFAULT_CHARSET);
             AddListItem(fileScrWin, xmstr, 0);
#endif
             plist = plist->next;
        }
}

void
close_prop()
{
	XtPopdown(propShell);
	propActive = 0;
	if (fileShell)
	    XtPopdown(fileShell);
	if (warningShell)
	    XtPopdown(warningShell);
}

#ifdef OLIT
static void
propShell_exit( w, client_data, event )
Widget w;
char *client_data;
void *event;
{
	OlWMProtocolVerify	*olwmpv;

	olwmpv = (OlWMProtocolVerify *) event;
	if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
	    XtPopdown(propShell);
	    propActive = 0;
	    if (fileShell)
	        XtPopdown(fileShell);
	    if (warningShell)
	        XtPopdown(warningShell);
	}
}

static void
fileWin_exit(w, client_data, event)
Widget w;
char *client_data;
void *event;
{
	OlWMProtocolVerify	*olwmpv;

	olwmpv = (OlWMProtocolVerify *) event;
	if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
	    XtPopdown(fileShell);
	    if (warningShell)
	        XtPopdown(warningShell);
	}
}


static void
warnWin_exit(w, client_data, event)
Widget w;
char *client_data;
void *event;
{
	OlWMProtocolVerify	*olwmpv;

	olwmpv = (OlWMProtocolVerify *) event;
	if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
	    XtPopdown(warningShell);
	}
}

#else

propShell_exit()
{
	propActive = 0;
	propShell = NULL;
	fileShell = NULL;
	warningShell = NULL;
}

fileWin_exit()
{
	fileShell = NULL;
	if (warningShell)
	    XtPopdown(warningShell);
}

warnWin_exit()
{
	warningShell = NULL;
}


#endif


create_prop_window()
{
	Widget   rc_1, rc_2, rc_3, pwidget;
	Widget   exWidget;
	char     nfontName[12];
	XtAppContext  win_context;

	propShell = create_popup_shell(mainShell, "Acqmeter Properties");

        pwidget = create_row_col(propShell, 1, 0, 0);

	n = 0;
#ifdef OLIT

	propPanel = XtCreateManagedWidget("Acqmeter", formWidgetClass,
			 pwidget, args, n);
#else
   	propPanel = (Widget)XmCreateForm(pwidget, "Acqmeter", args, n);
	XtManageChild (propPanel);
#endif
	XtManageChild (pwidget);

	rc_1 = create_form_row_col(propPanel, NULL, VERTICAL, 0, 4);
	hostWidget = create_text_widget (rc_1, TEXTMODE, "Host Computer: ",
			RemoteHost);
	timeWidget = create_time_widget (rc_1);

	fontWidget = create_text_widget (rc_1, TEXTMODE, "   Font  Name: ",
			 fontName);
	exWidget = create_ex_widget(rc_1, "Chart  Layout: ");
	layoutWidget[0] = create_ex_button (exWidget, "Horizontal", layout_cb, HCHART);
	layoutWidget[1] = create_ex_button (exWidget, "Vertical", layout_cb, VCHART);

	exWidget = create_ex_widget(rc_1, "  Chart  Type: ");
	typeWidget[0] = create_ex_button (exWidget, "Historic", type_cb,HISTORIC);
	typeWidget[1] = create_ex_button (exWidget, "Thermometric", type_cb, THERMO);

	XtSetArg(args[0], XtNset, TRUE);
	XtSetValues(layoutWidget[0], args, 1);
	XtSetValues(typeWidget[0], args, 1);

	rc_2 = create_form_row_col(propPanel, rc_1, VERTICAL, 0, 4);
	create_item_menu(rc_2);
	bgWidget = create_text_widget (rc_2, TEXTMODE, "  Background: ",
			 disp_items[0].bcolor);
	fgWidget = create_text_widget (rc_2, TEXTMODE,     "  Foreground: ",
			 disp_items[0].fcolor);
	gcolorWidget = create_text_widget (rc_2, TEXTMODE, "  Grid Color: ",
			 disp_items[0].gcolor);
	setvWidget = create_text_widget (rc_2, LABELMODE,  "  Set  Value: ",
			 "   ");
	upperWidget = create_text_widget (rc_2, TEXTMODE,  "  Max  Value: ",
			 "   ");
	lowerWidget = create_text_widget (rc_2, TEXTMODE,  "  Min  Value: ",
			 "   ");
	exWidget = create_ex_widget(rc_2, "Graphic Type: ");
	grafWidget[0] = create_ex_button (exWidget, "Line", graphic_cb, LINEMODE);
	grafWidget[1] = create_ex_button (exWidget, "Solid", graphic_cb, SOLIDMODE);
			
/**
	exWidget = create_ex_widget(rc_2, "  Log   Mode: ");
	logWidget[0] = create_ex_button (exWidget, "Off ", log_cb, 0);
	logWidget[1] = create_ex_button (exWidget, " On ", log_cb, 1);

	saveWidget = create_text_widget (rc_2, TEXTMODE, "Log Filename: ",
			 " ");
***/

	exWidget = create_ex_widget(rc_2, " Item   Show: ");
	showWidget[0] = create_ex_button (exWidget, "Off ", show_cb, 0);
	showWidget[1] = create_ex_button (exWidget, " On ", show_cb, 1);

	exWidget = create_ex_widget(rc_2, "Center Button: ");
	centerWidget[0] = create_ex_button (exWidget, "Off ", cenbut_cb, 0);
	centerWidget[1] = create_ex_button (exWidget, " On ", cenbut_cb, 1);

	exWidget = create_ex_widget(rc_2, " Grid   Show: ");
	gmodeWidget[0] = create_ex_button (exWidget, "Off ", gmode_cb, 0);
	gmodeWidget[1] = create_ex_button (exWidget, "Level 1", gmode_cb, 1);
	gmodeWidget[2] = create_ex_button (exWidget, "Level 2", gmode_cb, 2);
	gmodeWidget[3] = create_ex_button (exWidget, "Level 3", gmode_cb, 3);

	rc_3 = create_form_row_col(propPanel, rc_2, HORIZONTAL, 20, 20);
        create_button(rc_3, "Apply", 1, apply_cb);
        create_button(rc_3, "File", 1, file_cb);
        create_button(rc_3, "Close", 1, close_prop);

	XtRealizeWidget (propShell);
	propShellId = XtWindow(propShell);
        show_item_info(LOCKCHART, 1);
#ifdef  OLIT
	OlAddCallback( propShell, XtNwmProtocol, propShell_exit, NULL );
#else
	XmAddProtocolCallback(propShell, XM_WM_PROTOCOL_ATOM(propShell),
			deleteAtom, propShell_exit, NULL);
#endif
}



/*------------------------------------------------------------------
|
|
|
+------------------------------------------------------------------*/
main(argc,argv)
int argc;
char **argv;
{
    int    n;
    int  pid;
    int ival;
    char  *graphics;
/*
    struct passwd *getpwuid();
*/
    struct passwd *pasinfo;
    struct hostent *local_hp;

    RemoteHost[0] = '\0';    /* null name */
    strcpy(setup_file, "default");
    if (argc < 1)
    {
        fprintf(stderr,"usage: %s [ remotehostname debug_value]\n",argv[0]);
        exit(-1);
    }
    for(n = 1; n < argc; n++)
    {
	if ( !strcmp("-debug", argv[n]))
		debug = 1;
	else if (!strcmp("-f", argv[n]))
	{
	     if (n < argc-1 && argv[n+1] != NULL)
	     {
		strcpy(setup_file, argv[n+1]);
		n++;
	     }
	}
	else if (argv[n][0] == '-' )
	{
		if (argv[n+1] != NULL && argv[n+1][0] != '-')
			n++;
	}
	else
                strcpy(RemoteHost,argv[n]);
    }

    /* Dissassiate from Control Terminal */

    pid = fork();
    if (debug)
      if (pid > 0)
        fprintf(stderr, "Acqmeter PID: %d\n", pid );

    if (pid != 0)
	exit(0);	/* parent process */

    for(n=3;n < 20; n++)        /* close any inherited open file descriptors */
	close(n);

    freopen("/dev/null","r",stdin);
    freopen("/dev/console","a",stdout);

    ival = setsid();                /* the setsid program will disconnect from */
                                      /* controlling terminal and create a new */
                             /* process group, with this process as the leader */
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
    setup_signal_handlers();

    if (debug)
      fprintf(stdout,"requested RemoteHost:'%s'\n",RemoteHost);

    acq_ok = 0;
    if ((graphics = (char *)getenv("graphics")) == NULL)
    {
	fprintf(stderr,"Error: env parameter 'graphics' not set\n");
	exit(0);
    }
    if (strcmp(graphics,"sun") != 0 &&  strcmp(graphics,"suncolor") != 0)
    {
	fprintf(stderr,"Error: env parameter 'graphics' = '%s' not 'sun'\n",
		graphics);
	fprintf(stderr," Please set env 'graphics' to 'sun'\n");
	exit(0);
    }

    /* get process id */
    Procpid = getpid();
    if (debug)
        fprintf(stderr,"Process ID: %d\n",Procpid);

    user_dir = (char *) getenv("vnmruser");
 
    /* get Host machine name */
    gethostname(LocalHost,sizeof(LocalHost));
    if (debug)
        fprintf(stderr,"Local Host Name: %s\n",LocalHost);

    local_hp = gethostbyname( LocalHost );
    if (local_hp->h_length > sizeof( int ))
    {
	fprintf(stderr, "programming error, size of host address is %d, expected %d\n",
			 local_hp->h_length, sizeof( int )
	);
	exit( 1 );
    }
    memcpy( &local_entry, local_hp, sizeof( local_entry ) );
    memcpy( &local_addr, local_entry.h_addr, local_entry.h_length );
    local_entry.h_addr_list = &local_addr_list[ 0 ];
    local_entry.h_addr = (char *) &local_addr;
 
/*  WARNING:  any address fields in local_entry not explicitly set
              may be reset the next time you call gethostbyname.	*/

    /* --- get user's name --- */
    /*        get the password file information for user */
    pasinfo = getpwuid((int) getuid());
    strcpy(User,pasinfo->pw_name); /* Store user name */
    if (debug)
        fprintf(stderr,"User Name: %s\n",User);

    create_main_window(argc, argv);

    if ( RemoteHost[0] == '\0' )
       strcpy(RemoteHost,LocalHost);
    if ( (strcmp(RemoteHost,LocalHost) != 0) && (RemoteHost[0] != '\0') )
       initrpctcp(RemoteHost);

    inittimer(2.0,2.0,readInfo);

    if (!debug)
        freopen("/dev/console","a",stderr);

    acqmeter_window_loop();
}


/*  following array MUST end in -1, or a Segmented Violation may occur.
    It lets the programmer encode a list of signals to be caught, all
    using the same signal handler.					*/

static int signum_array[] = { SIGHUP, SIGINT, SIGQUIT, -1 };

setup_signal_handlers()
{
	struct sigaction	intserv;
	sigset_t		qmask;
	int			iter, ival, signum;

	for (iter = 0; ( (signum = signum_array[ iter ]) != -1 ); iter++) {
		sigemptyset( &qmask );
		sigaddset( &qmask, signum );
		intserv.sa_handler = exitproc;
		intserv.sa_mask = qmask;
		intserv.sa_flags = 0;

		ival = sigaction( signum, &intserv, NULL );
	}
	return( 0 );
}

void
input_signal()
{
     readInfo();
}


static XtInputId   input_fd;

register_input_event(fd)
int  fd;
{
	if (input_fd)
		XtRemoveInput(input_fd);
        input_fd = XtAddInput(fd, XtInputReadMask, input_signal, NULL);
}

#define HOSTNAME	1
#define TIMING  	2
#define FONTNAME   	3
#define LAYOUT		4
#define CHARTTYPE	5
#define GEOMETRY	6
#define DORDER		7
#define BCOLOR		20
#define FCOLOR		21
#define GCOLOR		22
#define SETVAL		23
#define MAXVAL		24
#define MINVAL		25
#define GTYPE		26
#define ISHOW		27
#define GSHOW		28
#define CENTER		29


save_preference()
{
	FILE   *fd;

        fd = NULL;
        sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
        fd = fopen(tmpstr,"w+");
	if (fd == NULL)
        {
            sprintf(tmpstr, "mkdir -p  %s/templates/acqmeter", user_dir);
            system(tmpstr);
            sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
            fd = fopen(tmpstr,"w+");
        }
        if (fd == NULL)
        {
	     disp_error("Could not open file:", 1);
             disp_error(tmpstr, 2);
             return(0);
        }
	store_setup(fd);
	fclose(fd);
	XtPopdown(fileShell);
}


cal_mainShell_loc()
{
	int	rx, ry;
	Window    win;
        XWindowAttributes win_attributes;

        XTranslateCoordinates(dpy,shellId,RootWindow(dpy, 0),0,0,&rx,&ry,&win);
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
             }
        }
}


store_setup(fout)
FILE  *fout;
{
	int	item, id;
	float   fval;
	char    *data;

	cal_mainShell_loc();
        fprintf(fout, "%d %dx%d+%d+%d\n", GEOMETRY, shell_w, shell_h,
				 shell_x, shell_y);
	store_item_info(setup_item);

	data = get_text_item(hostWidget);
	if (data != NULL)
	    fprintf(fout, "%d  %s\n", HOSTNAME, data);
	else
	    fprintf(fout, "%d  %s\n", HOSTNAME, RemoteHost);

	data = get_text_item(timeWidget);
	if (data != NULL)
	    fprintf(fout, "%d  %s\n", TIMING, data);
	else
	    fprintf(fout, "%d  %g\n", TIMING, delay_time);

	data = get_text_item(fontWidget);
	if (data != NULL)
	    fprintf(fout, "%d  %s\n", FONTNAME, data);
	else
	    fprintf(fout, "%d  %s\n", FONTNAME, fontName);

	fprintf(fout, "%d  %d\n", LAYOUT, layout);
	fprintf(fout, "%d  %d\n", CHARTTYPE, chart_type);
	id = 100;
	for(item = 0; item < TOTALCHART; item++)
	{
	   fprintf(fout, "%d %s\n", id + BCOLOR, disp_items[item].bcolor);
	   fprintf(fout, "%d %s\n", id + FCOLOR, disp_items[item].fcolor);
	   fprintf(fout, "%d %s\n", id + GCOLOR, disp_items[item].gcolor);
	   fprintf(fout, "%d %g\n", id + SETVAL, disp_items[item].setval);
	   fprintf(fout, "%d %g\n", id + MAXVAL, disp_items[item].upper);
	   fprintf(fout, "%d %g\n", id + MINVAL, disp_items[item].lower);
	   fprintf(fout, "%d %d\n", id + GTYPE, disp_items[item].mode);
	   fprintf(fout, "%d %d\n", id + ISHOW, disp_items[item].show);
	   fprintf(fout, "%d %d\n", id + GSHOW, disp_items[item].grid);
	   fprintf(fout, "%d %d\n", id + CENTER, disp_items[item].cenbut);
	   id += 100;
	}
	for(item = 0; item < TOTALCHART; item++)
	{
	   if (disp_order[item] >= 0)
	   	fprintf(fout, "%d %d\n", DORDER, disp_order[item]);
	}
}

load_preference()
{
	int	m, x, y;
	FILE   *fd;
 	unsigned int  ww, hh;

        fd = NULL;
        sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
        fd = fopen(tmpstr,"r");
        if (fd == NULL)
        {
	     disp_error("Could not open file:", 1);
             disp_error(tmpstr, 2);
             return(0);
        }
	inittimer(0.0,0.0, NULL);
	for (m = 0; m < TOTALCHART; m++)
	{
	     set_item_show(m, OFF);
	     disp_items[m].cenbut = 0;
	     disp_items[m].ncenbut = 0;
	}

	read_prefer_file(fd, ON);
	fclose(fd);

	XClearWindow(dpy, chartWin);
	for(m = 0; m < TOTALCHART; m++)
	    set_item_def_color(m);
	disp_items[LOCKCHART].setval = disp_items[LOCKCHART].lower;
	if (strlen(geom) > 0)
	{
	    cal_mainShell_loc();
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
            XtConfigureWidget(mainShell, x, y, ww, hh, 1);
	}
	cal_chart_geom();

        XtPopdown(fileShell);
	show_item_info(setup_item, 0);
	inittimer(1.0,1.0,readInfo);
}


static 
set_sub_attr(num, attr)
int  num;
char *attr;
{
	int	item, which;
        int     ival;
	float   fval;

	if (num > 300)
	{
	    which = SPINCHART;
	    item = num - 300;
	}
	else if (num > 200)
	{
	    which = VTCHART;
	    item = num - 200;
	}
	else
	{
	    which = LOCKCHART;
	    item = num - 100;
	}
	switch (item) {
	  case BCOLOR:
		  strcpy(disp_items[which].bcolor, attr);
		  break;
	  case FCOLOR:
		  strcpy(disp_items[which].fcolor, attr);
		  break;
	  case GCOLOR:
		  strcpy(disp_items[which].gcolor, attr);
		  break;
	  case SETVAL:
	    	  if (sscanf(attr, "%f", &fval) == 1)
			disp_items[which].setval = fval;
		  break;
	  case MAXVAL:
	    	  if (sscanf(attr, "%f", &fval) == 1)
		  {
			disp_items[which].upper = fval;
			disp_items[which].nupper = fval;
		  }
		  break;
	  case MINVAL:
	    	  if (sscanf(attr, "%f", &fval) == 1)
		  {
			disp_items[which].lower = fval;
			disp_items[which].nlower = fval;
		  }
		  break;
	  case GTYPE:
	    	  if (sscanf(attr, "%d", &ival) == 1)
		  {
			disp_items[which].mode = ival;
			disp_items[which].nmode = ival;
		  }
		  break;
	  case ISHOW:
	    	  if (sscanf(attr, "%d", &ival) == 1)
		  {
			disp_items[which].show = ival;
			disp_items[which].nshow = ival;
		  }
		  break;
	  case GSHOW:
	    	  if (sscanf(attr, "%d", &ival) == 1)
		  {
			disp_items[which].grid = ival;
			disp_items[which].ngrid = ival;
		  }
		  break;
	  case CENTER:
	    	  if (sscanf(attr, "%d", &ival) == 1)
		  {
			disp_items[which].cenbut = ival;
			disp_items[which].ncenbut = ival;
		  }
		  break;
	}
}

read_prefer_file(fin, set)
FILE  *fin;
int    set;
{
        char   *in;
        char   token[120];
        int    num, ival, id;
	float  fval;

	id = 0;
        while ((in = fgets(tmpstr, 200, fin)) != NULL)
        {
                if (sscanf(in, "%d%s", &num, token) != 2)
                     continue;
                if (num > 100)
                {
                     set_sub_attr(num, token);
                     continue;
                }
                switch (num) {
                 case HOSTNAME: 
	    	     if (RemoteHost[0] == '\0' || strcmp(RemoteHost,token) != 0)
	    	     {
			if (set)
			{
			   set_text_item(hostWidget, token);
			   change_host(token);
			}
			else
			{
			   if (RemoteHost[0] == '\0')
			   	strcpy(RemoteHost, token);
			}
	    	     }
                     break;
                 case FONTNAME: 
	    	     if (strcmp(token, fontName) != 0)
	    	     {
		         strcpy(fontName, token);
			 if (set)
			 {
			     set_text_item(fontWidget, token);
			     set_font(token);
			 }
		     }
                     break;
                 case GEOMETRY: 
                     strcpy(geom, token);
                     break;
                 case TIMING: 
	    	     if (sscanf(token, "%f", &fval) == 1)
		     {
			 delay_time = fval;
			 if (set)
			    set_text_item(timeWidget, token);
		     }
                     break;
                 case LAYOUT: 
	    	     if (sscanf(token, "%d", &ival) == 1)
		     {
			layout = ival;
			if (nlayout != ival)
			{
			   nlayout = ival;
			   if (set)
			   {
#ifdef OLIT
			     XtSetArg(args[0], XtNset, TRUE);
			     if (ival == HORIZONTAL)
			        XtSetValues(layoutWidget[0], args, 1);
			     else
			        XtSetValues(layoutWidget[1], args, 1);
#else
			     if (ival == HORIZONTAL)
	    		        XmToggleButtonSetState(layoutWidget[0], TRUE,
							 TRUE);
			     else
	    		        XmToggleButtonSetState(layoutWidget[1], TRUE,
							 TRUE);
#endif
			   }
			}
		     }
                     break;
                 case CHARTTYPE: 
	    	     if (sscanf(token, "%d", &ival) == 1)
		     {
			chart_type = ival;
			if (nchart_type != ival)
			{
			   nchart_type = ival;
			   if (set)
			   {
#ifdef OLIT
			      XtSetArg(args[0], XtNset, TRUE);
			      if (ival == HISTORIC)
			         XtSetValues(typeWidget[0], args, 1);
			      else
			         XtSetValues(typeWidget[1], args, 1);
#else
			      if (ival == HISTORIC)
	    		         XmToggleButtonSetState(typeWidget[0], TRUE, TRUE);
			      else
	    		         XmToggleButtonSetState(typeWidget[1], TRUE, TRUE);
#endif
			   }
			}
		     }
                     break;
                 case DORDER: 
	    	     if (sscanf(token, "%d", &ival) == 1)
		     {
			 if (set)
			     set_item_show(ival, ON);
			 else
			     disp_order[id++] = ival;
		     }
                     break;
                }
        }
}



create_def_list()
{
        def_rec        *plist, *clist;

        plist = def_list;
#ifdef MOTIF
	if (def_list != NULL)
	         XmListDeleteAllItems(fileScrWin);
#endif
        while (plist != NULL)
        {
                clist = plist;
                plist = plist->next;
#ifdef OLIT
                if (clist->item != NULL)
                     (*DeleteListItem) (fileScrWin, clist->item);
#endif
                if (clist->name != NULL)
                     XtFree(clist->name);
                XtFree(clist);
        }
        def_list = NULL;
	if (user_dir != NULL)
	{
		sprintf(tmpstr, "%s/templates/acqmeter", user_dir);
		build_def_list(tmpstr);
	}
	else
	{
	     disp_error("Error: environment 'vnmruser' was not set.", 1);
             disp_error(" ", 2);
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
#ifdef OLIT
	   clist->item = NULL;
#endif
           clist->next = NULL;
        }
        closedir(dirp);
}


void
file_select_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
        def_rec   *clist;
        int   item;

#ifdef  OLIT
        OlListToken     token = (OlListToken)call_data;
        OlListItem     *listitem;

        listitem = OlListItemPointer(token);
        item = (int)listitem->user_data;
#else
        XmListCallbackStruct  *cb;

        cb = (XmListCallbackStruct *) call_data;
        if (cb->reason != XmCR_SINGLE_SELECT)
                return;
        item = cb->item_position;
#endif
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


int
exec_remove()
{
     sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
     unlink(tmpstr);
     create_file_list();
}



save_proc()
{
        struct   stat   f_stat;

        sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
        if (stat(tmpstr, &f_stat) == 0)
        {
                save_load_proc = save_preference;
                sprintf(tmpstr, "File '%s' already exists.", setup_file);
                disp_warning(tmpstr, 1);
                sprintf(tmpstr, "It will be overwritten.");
                disp_warning(tmpstr, 2);
                return;
        }
	save_preference();
}


remove_proc()
{
        struct   stat   f_stat;

        sprintf(tmpstr, "%s/templates/acqmeter/%s", user_dir, setup_file);
        if (stat(tmpstr, &f_stat) == 0)
        {
            save_load_proc = exec_remove;
            sprintf(tmpstr, "File '%s' will be deleted.", setup_file);
            disp_warning(tmpstr, 1);
            sprintf(tmpstr, "Are you sure?");
            disp_warning(tmpstr, 2);
            return;
        }
}


void
file_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
        int     type;
        char   *data;

        save_load_proc = NULL;
        type = (int) client_data;
        if (type == 0)
        {
             XtPopdown(fileShell);
	     if (warningShell)
	         XtPopdown(warningShell);
             return;
        }
        if (user_dir == NULL)
        {
            disp_error("Error: env parameter 'vnmruser' is not set.", 1);
            disp_error("  ", 2);
            return;
        }

        data = get_text_item(fileWidget);
        if (data != NULL && strlen(data) > 0)
        {
            strcpy(setup_file, data);
            if (type == LOAD)
        	load_preference();
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


create_fileWindow()
{
	Widget  pwidget;

	fileShell = create_popup_shell(propShell, "Acqmeter File");

	fileWin = create_row_col(fileShell, 1, 0, 0);

	n = 0;
#ifdef  OLIT
    	XtSetArg(args[n], XtNviewHeight, (XtArgVal) 6); n++;
	fileScrWin = XtCreateManagedWidget("",
                        scrollingListWidgetClass, fileWin, args, n);
	n = 0;
	XtSetArg(args[n], XtNapplAddItem, (XtArgVal)&AddListItem); n++;
	XtSetArg(args[n], XtNapplDeleteItem, (XtArgVal)&DeleteListItem); n++;
	XtGetValues(fileScrWin, args, n);
	XtAddCallback(fileScrWin, XtNuserMakeCurrent, file_select_proc, NULL);
#else
    	XtSetArg(args[n], XmNselectionPolicy, (XtArgVal) XmSINGLE_SELECT); n++;
    	XtSetArg(args[n], XmNvisibleItemCount, (XtArgVal) 6); n++;
    	fileScrWin = (Widget)XmCreateScrolledList(fileWin, "", args, n);
	XtManageChild (fileScrWin);
	XtAddCallback(fileScrWin, XmNsingleSelectionCallback, file_select_proc, NULL);

#endif
    
	fileWidget = create_text_widget(fileWin, TEXTMODE, "File:", setup_file, NULL, 0);

        pwidget = create_row_col(fileWin, 0, 10, 16);
        create_button(pwidget, " Save ", SAVE, file_proc);
        create_button(pwidget, " Load ", LOAD, file_proc);
        create_button(pwidget, "Remove", REMOVE, file_proc);
        create_button(pwidget, "Close ", 0, file_proc);

	XtRealizeWidget (fileShell);
#ifdef OLIT
	OlAddCallback( fileShell, XtNwmProtocol, fileWin_exit, NULL );
#else
	XmAddProtocolCallback(fileShell, XM_WM_PROTOCOL_ATOM(fileShell),
			deleteAtom, fileWin_exit, 0);
#endif
	fileID = XtWindow(fileShell);
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


disp_warning(mess, which)
char   *mess;
int    which;
{
	int	x, y;
	Window  win;

        if (warningShell == NULL)
            create_warningWindow(mess);
        if (warningShell)
        {
            XtUnmapWidget(warnBut2);
            XtMapWidget(warnBut1);
            XtMapWidget(warnBut3);
            if (which == 1)
            {
               set_label_item(errWidget, mess);
               XBell(dpy, 30);
            }
            else
            {
               set_label_item(errWidget2, mess);
               XTranslateCoordinates(dpy,propShellId,RootWindow(dpy, 0),
				0,0,&x,&y,&win);
	       XtMoveWidget(warningShell, x+20, y+30);
               XtPopup (warningShell, XtGrabExclusive);
	       XRaiseWindow(dpy, warningID);
            }
        }
}

disp_error(mess, which)
char   *mess;
int    which;
{
	int	x, y;
	Window  win;

        if (warningShell == NULL)
            create_warningWindow(mess);
        if (warningShell)
        {
            XtUnmapWidget(warnBut1);
            XtUnmapWidget(warnBut3);
            XtMapWidget(warnBut2);
            if (which == 1)
            {
               set_label_item(errWidget, mess);
               XBell(dpy, 30);
            }
            else
            {
               set_label_item(errWidget2, mess);
               XTranslateCoordinates(dpy,propShellId,RootWindow(dpy, 0),
				0,0,&x,&y,&win);
	       XtMoveWidget(warningShell, x+20, y+30);
               XtPopup (warningShell, XtGrabExclusive);
	       XRaiseWindow(dpy, warningID);
            }
        }
}




create_warningWindow(mess)
char    *mess;
{
	Widget  pwidget;
	int     x, y;
	Position   posy;

	if (mainShell == NULL)
	     return;
	warningShell = create_popup_shell(propShell, "Acqmeter Warning");

#ifdef OLIT
	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
        XtSetArg (args[n], XtNrecomputeSize, TRUE); n++;
	pwidget = XtCreateManagedWidget("",
                        controlAreaWidgetClass, warningShell, args, n);
	n = 0;
	XtSetArg (args[n], XtNstring, mess);  n++;
        XtSetArg (args[n], XtNrecomputeSize, TRUE); n++;
	XtSetArg(args[n], XtNgravity, WestGravity);  n++;
        XtSetArg (args[n], XtNwidth, charWidth * 60); n++;
	XtSetArg(args[n], XtNstrip, FALSE);  n++;
	errWidget = XtCreateManagedWidget("", staticTextWidgetClass,
                                     pwidget, args, n);
	errWidget2 = XtCreateManagedWidget("", staticTextWidgetClass,
                                     pwidget, args, n);
#else
	pwidget = create_row_col(warningShell, 1, 0, 0);
	n = 0;
	xmstr = XmStringLtoRCreate(mess, XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
	errWidget = (Widget)XmCreateLabel(pwidget, "text", args, n);
	XtManageChild (errWidget);
	errWidget2 = (Widget)XmCreateLabel(pwidget, "text", args, n);
	XtManageChild (errWidget2);
/*
   	XtFree(xmstr);
*/
        XmStringFree(xmstr);
#endif
        pwidget = create_row_col(pwidget, 0, 30, 30);
        warnBut1 = create_button(pwidget, "Continue", 1, warning_proc);
        warnBut2 = create_button(pwidget, " Ok ", 1, error_proc);
        warnBut3 = create_button(pwidget, " Cancel ", 0, warning_proc);
        XtRealizeWidget (warningShell);
	XtSetArg (args[0], XtNx, 300);
	XtSetArg (args[1], XtNy, 300); 
	XtSetValues(warningShell, args, 2);
	warningID = XtWindow(warningShell);
#ifdef OLIT
	OlAddCallback( warningShell, XtNwmProtocol, warnWin_exit, NULL );
#else
	XmAddProtocolCallback(warningShell, XM_WM_PROTOCOL_ATOM(warningShell),
			deleteAtom, warnWin_exit, 0);
#endif
}


