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

/*   A Hung Lin Creation  */

#include <stdio.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xproto.h>

#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/Form.h>
#include <Xol/OblongButt.h>
#include <Xol/Scrollbar.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/PopupWindo.h>

#define  XMARGIN   6
#define  YMARGIN  12
#define  APLINE   15
#define  HSLINE   24
#define  COMMENT  57
#define  ROTOR    0x80
#define  EXTCLK   0x40
#define  INTRP    0x20
#define  STAG     0x10
#define  CTC      0x08
#define  CLOOP    0x06
#define  SLOOP    0x04
#define  ELOOP    0x02
#define  APBUS    0x01
#define  CTCST    0x0C
#define  CTCEN    0x0A
#define  DELAY    0x00
#define  DBASE    100
#define  DINCR    10
#define  APDATA   29
#define  APDATALEN   8
#define  APCHIP    9
#define  APCHIPLEN 4 
#define  APREG    13
#define  APREGLEN 8 
#define  CHNUM 	  6 
#define  TWO_24  16777216.0      /* 2 to_the_power 24 */


/* #define  MEMSIZE  2048 */
#define  MEMSIZE  4096
#define  BINARY	  0
#define  ASCII	  1

static Widget  topShell;
static Widget  mainFrame;
static Widget  scrolledWindow;
static Widget  textWidget;
static Widget  titleWidget;
static Widget  title2Widget;
static Widget  scrollbar = NULL;
static Widget  upBut, downBut;
static Widget  nextBut, prevBut;
static Widget  loadBut, exitBut;
static Widget  saveBut;
static Widget  popupShell = NULL;
static Widget  popupShell2 = NULL;
static Widget  errorMessage;
static Widget  errorMessage2;
static Widget  lenWidget;
static Widget  formatWidget;
static Window  titleWin;
static Window  title2Win;
static Window  textWin;
static Display *dpy;
static GC       gc;
static Pixel  winBg;
static Pixel  borderPix;
static Pixel  textPix;
static Pixel  commentPix;
static Pixel  reversePix;
static Pixel  redPix, greenPix;
static Pixel  xblack;
static Pixel  errorPix;
static Pixel  loopPix;
static Pixel  itemPix1, itemPix2;
static int   winDepth, screenWidth;
static int   n;
static int   textObscured;
static int   max_len; /* the max. length of a line */
static int   bufPtr = 9999;
static int   fbufPtr = 9999;
static int   fifoNum = 0;
static int   fifoWord = 64;
static int   fifoFormat = BINARY;
static int   botmIndex = 1;
static int   scrollIndex = 1;
static int   hilitIndex = 0;
static int   textCols, textRows, rowHeight;
static int   line_gap;
static int   charWidth, charHeight;
static int   labelWidth, labelHeight;
static int   textWinWidth, textWinHeight;
static int   char_ascent;
static int   label_ascent;
static int   diff_y, diff_x;
static int   backing_store;
static int   resizeFlag = 0;

/*  lock tranceiver values */
int   lkduty, lkrate, lkpulse;
int   lkpower, rcvrpower;
double lkphase, lkfreq;
int   lk_power_table[49] = {
	 0,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  4,
	 4,  4,  5,  6,  6,  7,  8,  9, 10, 11, 13, 14,
	16, 18, 20, 23, 25, 28, 32, 36, 40, 45, 51, 57, 
	64, 72, 80, 90,101,113,128,143,161,180,202,227, 255};

/* power of each channel */

int    obs_preamp;
int    rcvr_amp;
int    rcvr_filter;

static XFontStruct  *textFontInfo;
static XFontStruct  *labelFontInfo;
static char   infile[128];
static char   outfile[128];
static char   bfifo[98];
static Dimension   scroll_width, scroll_height;
static Dimension   titleWidth, titleHeight;

Widget popupFrame, fileWidget;
Widget saveWidget;

Pixel  xcolor;

static Arg     args[20];

Dimension   width,height;

char   *ctl_names[10] = {"ROTO", " EXT", "INTR", "  ID", " CTC",
			 "STRT", " END", "  AP", "    ", " R/W" };
char   *hs_names[5] = { " 180", "  90", " WFG", "XMTR", "RCVR" };
char   *ap_bits[5] = {"  Bit 55", "  Bit 26", "  Bit  0"};

typedef struct _fifo_list {
		int	id;
		int	len;
		int	disp;
		char    *data;
		struct  _fifo_list  *prev;
		struct  _fifo_list  *next;
	} fifo_list;

static fifo_list   *fifoList = NULL;
static fifo_list   *infoList = NULL;
static fifo_list   *dispStart = NULL;

typedef struct _mem_list {
		char    *data;
		struct  _mem_list  *next;
	} mem_list;
static mem_list   *fmemList = NULL;
static mem_list   *imemList = NULL;
static mem_list   *imemPtr, *fmemPtr;

struct _xmtr_info {
		int	hs_sel;
		int	amp_sel;
		int	dec_hl;
		int	mixer_sel;
		int	fine_attn;
		int	coarse_attn;
		int	dmm;
		int	active;
		double   phase;
		double   dmf;
	};
struct _xmtr_info  xmtr_info[CHNUM];




static  long  MaxSum = 0;
static  long  NP = 0;
static  long  NT = 0;
static  long  SrcAddr = 0;
static  long  DstAddr = 0;


int    charPtr;
int    errorNum;
char   tmpstr[128];
char   convert_bit_ch();
int    create_info_list();
void   disp_info();
void   draw_title();
void   draw_title2();

main(argc, argv)
int	argc;
char	**argv;
{
	int	i;
	char	*p;

	infile[0] = '\0';
	for (i = 1; i < argc; ++i)
   	{
      	   p = argv[i];
      	   if (*p == '-')
		i++;
	   else
	   {
		sprintf(infile, argv[i]);
		break;
	   }
	}
        max_len = COMMENT + 24;
	fifoNum = 1;
	create_info_list();
	create_windows(argc, argv);
/*
	get_colors();
*/
	draw_title2();
	disp_info(1); 
	XtMainLoop();
}

static void
text_win_resize(widget, data, e)
Widget          widget;
caddr_t         data;
XEvent          *e;
{
	static  int  old_rows = 0;
	static  int  first_time = 1;

	if (first_time)
		first_time = 0;
	else
		resizeFlag = 1;
	XtSetArg (args[0], XtNwidth, &width);
	XtSetArg (args[1], XtNheight, &height);
	XtGetValues(textWidget, args, 2);
	textWinWidth = (int) width;
	textWinHeight = (int) height;
	textCols = width / charWidth;
	textRows = height / rowHeight;
	if (textRows > fifoNum)
	{
		XtUnmapWidget(scrollbar);
		dispStart = infoList;
		disp_info(1);
	}
	else
	{
		XtMapWidget(scrollbar);
/*
	    	if (old_rows != textRows)
		{
		    disp_info(1);
		    if (scrollIndex >= (dispStart->id + textRows))
		    {
			scrollIndex = dispStart->id + textRows - 1;
	     		XtSetArg (args[0], XtNsliderValue, scrollIndex);
	     		XtSetValues(scrollbar, args, 1);
		    }
		}
*/
		disp_info(1);
	}
	old_rows = textRows;
}


static void
change_size(c, data, e)
Widget          c;
caddr_t         data;
XEvent          *e;
{
	static  Dimension  oldw = 0;
	static  Dimension  oldh = 0;

   	if (e->type != ConfigureNotify)
            return;
	XtSetArg (args[0], XtNwidth, &width);
	XtSetArg (args[1], XtNheight, &height);
	XtGetValues(topShell, args, 2);
	if (oldw != width || oldh != height)
	{
	    oldw = width;
	    oldh = height;
	    height = height - diff_y;
	    width = width - diff_x;
	    XtResizeWidget(scrollbar, scroll_width, height, 1);
	}
}


static int
get_hilit_line()
{
	int	    hiline;
	fifo_list   *node;

	if (hilitIndex < dispStart->id || hilitIndex > botmIndex)
	    return(-1);
	node = dispStart;
	hiline = 0;
	for(;;)
	{
	    if (node->id == hilitIndex)
		break;
	    if (node->disp > 0)
	        hiline += 2;
	    else
	        hiline++;
	    if (hiline >= textRows)
		return(-1);
	    node = node->next;
	}
	return(hiline);
}


draw_info(start_node, start_line, lines)
fifo_list  *start_node;
int	   start_line, lines;
{
	int	 x, y, k, lp, cspace, len;
	int	 last_line, x2;
	char     *mess, *mess2;
	fifo_list   *node, *pnode;

	y = rowHeight * start_line + char_ascent + 2;
	node = start_node;
	last_line = start_line + lines;
	if (last_line > textRows)
	     last_line = textRows;
	lp = 0;
	cspace = textCols - COMMENT - 2;
	k = start_line - 1;
	while (lp < start_line)
	{
	    if (node->disp > 0)
	    {
		if (lp >= k) /* needs to draw the rest part of the last line */
		{
	   	    mess = node->data + COMMENT;
	    	    if (*mess == '?')
			set_color(errorPix);
		    else
			set_color(commentPix);
		    mess = node->data + node->disp;
		    len = node->len - node->disp;
		    if (cspace >= len)
		        x = charWidth * (COMMENT + 2);
		    else
		        x = charWidth * XMARGIN;
	            XDrawString (dpy, textWin, gc, x, y, mess, len);
		}
		lp += 2;
	    }
	    else
		lp++;
	    node = node->next;
	    if (node == NULL)
	    {
	       if (scrollIndex < dispStart->id)
	       {
		  scrollIndex = dispStart->id;
	          XtSetArg (args[0], XtNsliderValue, scrollIndex - 1);
	          XtSetValues(scrollbar, args, 1);
	       }
		return;
	    }
	}
	set_color(textPix);
	k = lp;
	pnode = node;
	y = rowHeight * lp + char_ascent + 2;
	while (k < last_line)
	{
	   XDrawString (dpy, textWin, gc, 0, y, node->data, COMMENT);
	   mess = node->data + COMMENT;
	   if (node->len > textCols)
	   {
		mess2 = node->data + textCols;
		while (*mess2 != ',' && *mess2 != ' ')
		   mess2--;
		if (*(mess2+1) == '\0')
		   mess2 = mess;
		len = mess2 - mess;
	   }
	   else
		mess2 = mess;
	   if (mess2 > mess)
	   {
		len = mess2 - mess;
		node->disp = mess2 - node->data;
		k += 2;
	        y += rowHeight * 2;
	   }
	   else
	   {
	        len = node->len - COMMENT;
		node->disp = 0;
		k++;
	   	y += rowHeight;
	   }
	   node = node->next;
	   if (node == NULL)
	       break;
	}
	node = pnode;
	y = rowHeight * lp + char_ascent + 2;
	x = charWidth * COMMENT;
	set_color(commentPix);
	k = lp;
	while (k < last_line)
	{
	   mess = node->data + COMMENT;
	   if (node->disp > 0)
		len = node->disp - COMMENT;
	   else
	        len = node->len - COMMENT;
	   if (*mess == '?' || *mess == '$')
	   {
		if (*mess == '?')
		    set_color(errorPix);
		else
		    set_color(loopPix);
	        mess++;
		len--;
	        XDrawString (dpy, textWin, gc, x, y, mess, len);
		if (node->disp > 0)
		{
		    mess = node->data + node->disp;
		    len = node->len - node->disp;
		    if (k < last_line - 1)
		    {
		       if (len > cspace)
			   x2 = charWidth * XMARGIN;
		       else
			   x2 = x + charWidth;
	   	       y += rowHeight;
	               XDrawString (dpy, textWin, gc, x2, y, mess, len);
		    }
		    k += 2;
		}
		else
		    k++;
		set_color(commentPix);
	   }
	   else
	   {
	        XDrawString (dpy, textWin, gc, x, y, mess, len);
		if (node->disp > 0)
		{
		    mess = node->data + node->disp;
		    len = node->len - node->disp;
		    if (k < last_line - 1)
		    {
		       if (len > cspace)
			   x2 = charWidth * XMARGIN;
		       else
			   x2 = x + charWidth * 2;
	   	       y += rowHeight;
	               XDrawString (dpy, textWin, gc, x2, y, mess, len);
		    }
		    k += 2;
		}
		else
		    k++;
	   }
	   if (k >= last_line)
		break;
	   if (node->next == NULL)
		break;
	   node = node->next;
	   y += rowHeight;
	}
	node = dispStart;
	y = 0;
	for (;;)
	{
	   if (node->next == NULL)
		break;
	   if (node->disp > 0)
		y += 2;
	   else
	        y++;
	   if (y >= textRows)
		break;
	   node = node->next;
	}
	botmIndex = node->id;
	if (scrollIndex < dispStart->id || scrollIndex > botmIndex)
	{
	     if (scrollIndex < dispStart->id)
		scrollIndex = dispStart->id;
	     else
		scrollIndex = botmIndex;
	     XtSetArg (args[0], XtNsliderValue, scrollIndex - 1);
	     XtSetValues(scrollbar, args, 1);
	}
}


static void
disp_info(clean)
int	clean;
{
	int	x, y, hiline;

	if (dispStart == NULL)
	     return;
	hiline = get_hilit_line();
	if (clean)
	    XClearWindow(dpy, textWin);
	else if (hiline >= 0)
	{
	    y = rowHeight * hiline;
	    XClearArea(dpy, textWin, 0, y, textWinWidth, rowHeight, FALSE);
	}
	draw_info(dispStart, 0, textRows);
	hiline = get_hilit_line();
	if (hiline >= 0)
	    reverse_line(-1, hiline);
}


static void
expose_info(c, data, event)
Widget          c;
caddr_t         data;
XEvent          *event;
{
	if (event->xexpose.count > 0)
            return;
	if (resizeFlag)
	{
	   resizeFlag = 0;
	   return;
	}
	disp_info(0);
}




static void
scrollbar_cb(widget, client_data, call_date)
Widget          widget;
XtPointer       client_data, call_date;
{
	int	oldIndex, dline;
        OlScrollbarVerify
                        *sbv = (OlScrollbarVerify *)call_date;

        sbv->ok = TRUE;
	if (dispStart == NULL)
	     return;
	oldIndex = scrollIndex;
	scrollIndex = sbv->new_location + 1;
	dline = scrollIndex - oldIndex;
	if (dline < 0)
	     scroll_up(-dline);
	else if (dline > 0)
	     scroll_down(dline);
}


scroll_up(lines)
int   lines;
{
	int	newIndex, dx, dy, dh, dline, hiline;
	int	up_lines, old_top;
	char    *mess;

	if (dispStart == NULL || dispStart->id <= 1)
	     return;
	newIndex = dispStart->id - lines;
	if (newIndex < 1)
	     newIndex = 1;
	dline = 0;
	up_lines = 0;
	old_top = dispStart->id;
        while (dispStart != NULL)
        {
             if (dispStart->id == newIndex)
                    break;
	     if (dispStart->prev == NULL)
		    break;
             dispStart = dispStart->prev;
	     if (dispStart->len > textCols)
	     {
		mess = dispStart->data + textCols;
		while (*mess != ',' && *mess != ' ')
		   mess--;
		if (*(mess+1) != '\0')
		   dispStart->disp = 1;
		else
		   dispStart->disp = 0;
	     }
	     else
		dispStart->disp = 0;
	     up_lines = up_lines + dispStart->disp + 1;
	     dline++;
        }
	if (dline == 0)
	     return;
/*
	if (scrollIndex >= dispStart->id + textRows)
	{
	     scrollIndex = dispStart->id + textRows - 1;
	     XtSetArg (args[0], XtNsliderValue, scrollIndex - 1);
	     XtSetValues(scrollbar, args, 1);
	}
*/
	if (textObscured || up_lines >= textRows)
	{
		disp_info(1);
		return;
	}
	dy = rowHeight * up_lines;
	dh = rowHeight * (textRows - up_lines);
	XCopyArea (dpy, textWin, textWin, gc, 0, 0, textWinWidth, dh,
				0, dy);
	XClearArea(dpy, textWin, 0, 0, textWinWidth, dy, FALSE);
	draw_info(dispStart, 0, up_lines);
	if (hilitIndex < old_top)
	{
	    hiline = get_hilit_line();
	    if (hiline >= 0)
	     	reverse_line(-1, hiline);
	}
}


scroll_down(lines)
int     lines;
{
	int	newIndex, dx, dy, dh, dline, hiline;
	int	down_lines, old_botm;
	fifo_list   *node;

	if (dispStart == NULL || dispStart->next == NULL)
	     return;
	newIndex = dispStart->id + lines;
	if (newIndex > fifoNum)
	     newIndex = fifoNum;
	dline = 0;
	down_lines = 0;
	old_botm = botmIndex;
        while (dispStart != NULL)
        {
             if (dispStart->id == newIndex)
                    break;
	     if (dispStart->next == NULL)
		    break;
	     if (dispStart->disp > 0)
		down_lines += 2;
	     else
	        down_lines++;
             dispStart = dispStart->next;
	     dline++;
        }
	if (dline == 0)
	     return;
/*
	if (scrollIndex < dispStart->id)
	{
	     scrollIndex = dispStart->id;
	     XtSetArg (args[0], XtNsliderValue, scrollIndex - 1);
	     XtSetValues(scrollbar, args, 1);
	}
*/
	if (textObscured || down_lines >= textRows)
	{
	     disp_info(1);
	     return;
	}
	dy = rowHeight * down_lines;
	dh = rowHeight * (textRows - down_lines) + 2;
	XCopyArea (dpy, textWin, textWin, gc, 0, dy, textWinWidth, dh,
				0, 0);
	dy =  textWinHeight - dh;
	XClearArea(dpy, textWin, 0, dh, textWinWidth, dy, FALSE);
/*
	node = dispStart;
	dline = 0;
        while (node != NULL)
        {
               if (node->id == newIndex)
                    break;
               node = node->next;
	       dline++;
	}
	if(node != NULL)
	     draw_info(node, dline, textRows - dline);
*/
	draw_info(dispStart, textRows - down_lines, textRows);
	if (hilitIndex > old_botm)
	{
	     hiline = get_hilit_line();
	     if (hiline >= 0)
	     	reverse_line(-1, hiline);
	}
}



static void
draw_title(c, data, e)
Widget          c;
caddr_t         data;
XEvent          *e;
{
	int	x, y, j, h, k;

	XtSetArg (args[0], XtNwidth, &titleWidth);
	XtSetArg (args[1], XtNheight, &titleHeight);
	XtGetValues(titleWidget, args, 2);
	set_color(itemPix1);
	x = XMARGIN * charWidth - 1;
	j = charWidth * 8 + 2;
	XFillRectangle(dpy, titleWin,gc,x,0,j,titleHeight);
	x = APLINE * charWidth - 1;
	j = charWidth * 8 + 2;
	XFillRectangle(dpy, titleWin,gc,x,0,j,titleHeight);
	x = HSLINE * charWidth - 1;
	j = charWidth * 32 + 2;
	XFillRectangle(dpy, titleWin,gc,x,0,j,titleHeight);
	set_color(itemPix2);
	x = (HSLINE + 3) * charWidth - 2;
	y = charHeight;
	j = charWidth * 5 + 2;
	h = titleHeight - y;
	for (k = 0; k < 5; k++)
	{
	    XFillRectangle(dpy, titleWin,gc,x,y,j,h);
	    x = x + charWidth * 6;
	}
	set_color(textPix);
	y = char_ascent + 3;
	x = XMARGIN * charWidth + charWidth / 2;
	XDrawString(dpy, titleWin, gc, x, y, "Control", 7);
	y += charHeight;
	x = (XMARGIN + 2) * charWidth;
	XDrawString(dpy, titleWin, gc, x, y, "Code", 4);
	x = (APLINE + 3) * charWidth;
	y = char_ascent + 3;
	XDrawString(dpy, titleWin, gc, x, y, "Ap", 2);
	y += charHeight;
	x = (APLINE + 2) * charWidth;
	XDrawString(dpy, titleWin, gc, x, y, "Data", 4);
	x = (HSLINE + 12) * charWidth;
	y = char_ascent + 3;
	XDrawString(dpy, titleWin, gc, x, y, "HS Lines", 8);
	y += charHeight;
	x = (HSLINE + 3) * charWidth + charWidth / 2;
	j = 5;
	while ( j > 0 )
	{
	    sprintf(tmpstr, "Ch %d", j);
	    XDrawString(dpy, titleWin, gc, x, y, tmpstr, 4);
	    j--;
	    x += charWidth * 6;
	}
	y = char_ascent + 3;
	x = charWidth * (COMMENT + 13);
	sprintf(tmpstr, "Total: ", fifoNum);
	XDrawString(dpy, titleWin, gc, x, y, "Total: ", 7);
	y += charHeight;
	x = charWidth * COMMENT;
	sprintf(tmpstr, "Comment     Errors: ");
	XDrawString(dpy, titleWin, gc, x, y, tmpstr, strlen(tmpstr));
	update_message();
	set_color(borderPix);
	XDrawLine(dpy, titleWin, gc, 0, titleHeight-1, width, titleHeight-1);
	update_message();
}


update_message()
{
	int    x, y, w, h;

	set_color(textPix);
	y = char_ascent + 3;
	x = charWidth * (COMMENT + 20);
	w = titleWidth - x;
	h = titleHeight - 2;
	XClearArea(dpy, titleWin, x, 0, w, h, FALSE);
	sprintf(tmpstr, "%d", fifoNum);
	XDrawString(dpy, titleWin, gc, x, y, tmpstr, strlen(tmpstr));
	y += charHeight;
	if (errorNum > 0)
	   set_color(redPix);
	sprintf(tmpstr, "%d", errorNum);
	XDrawString(dpy, titleWin, gc, x, y, tmpstr, strlen(tmpstr));
}


static void
draw_title2(c, data, e)
Widget          c;
caddr_t         data;
XEvent          *e;
{
	int	x, y;
	int	j, k;

	XtSetArg (args[0], XtNwidth, &width);
	XtSetArg (args[1], XtNheight, &height);
	XtGetValues(title2Widget, args, 2);
	if (labelFontInfo)
	    XSetFont(dpy, gc, labelFontInfo->fid);
	set_color(textPix);
	y = label_ascent + 3;
	for (j = 0; j < 4; j++)
	{
	    x = XMARGIN * charWidth + charWidth - labelWidth;;
	    for(k = 0; k < 10; k++)
	    {
	        XDrawString(dpy, title2Win, gc, x, y, ctl_names[k] + j, 1);
		x += charWidth;
	    }
	    y += labelHeight;
	}
	y = label_ascent + 3;
	for (j = 0; j < 4; j++)
	{
	    x = (HSLINE + 4) * charWidth - labelWidth;
	    for(k = 0; k < 5; k++)
	    {
	        XDrawString(dpy, title2Win, gc, x, y, hs_names[k] + j, 1);
		x += charWidth;
	    }
	    y += labelHeight;
	}
	y = label_ascent + 3;
	for (j = 0; j < 4; j++)
	{
	    x = (HSLINE + 28) * charWidth - labelWidth;
	    for(k = 0; k < 5; k++)
	    {
	        XDrawString(dpy, title2Win, gc, x, y, hs_names[k] + j, 1);
		x += charWidth;
	    }
	    y += labelHeight;
	}
	XSetFont(dpy, gc, textFontInfo->fid);
/*
	x = charWidth * COMMENT;
	y = height -  charHeight + char_ascent;
	XDrawString(dpy, title2Win, gc, x, y, "Comment", 7);
*/
}

static void
hilight(c, data, event)
Widget          c;
caddr_t         data;
XEvent          *event;
{
	int    line, newIndex, old_line, x, y;
	fifo_list   *node;
	static int   times = 0;

	if (event->xbutton.time - times > 500)
	{
	     times = event->xbutton.time;
	     return;
	}
	times = event->xbutton.time;
	line = (event->xbutton.y) / rowHeight;
	if (line >= textRows)
	     return;
	if (dispStart == NULL)
	     return;
	node = dispStart;
	y = 0;
	for(;;)
	{
	    if (node->disp > 0)
		y += 2;
	    else
		y++;
	    if (y > line)
		break;
	    node = node->next;
	    if (node == NULL)
		break;
	}
	if (node != NULL)
	    newIndex = node->id;
	else
	    return;
	if (newIndex != hilitIndex)
	{
	     old_line = get_hilit_line();
	     hilitIndex = newIndex;
	     line = get_hilit_line();
	     reverse_line(old_line, line);
	}
}


static void
scroll_text(widget, client_data, call_date)
Widget          widget;
XtPointer       client_data, call_date;
{
	int   order;

	order = (int) client_data;
	if (order)
	    scroll_down(1);
	else
	    scroll_up(1);
}


static void
change_page(widget, client_data, call_date)
Widget          widget;
XtPointer       client_data, call_date;
{
	int   order, k;
	fifo_list   *node;

	if (dispStart == NULL)
	     return;
	order = (int) client_data;
	if ( order ) /* previous page */
	{
	     order = 0;
	     k = 0;
	     node = dispStart;
	     while ( k < textRows)
	     {
		if (node->prev == NULL)
		    break;
		node = node->prev;
		if (node->len > textCols)
		    k += 2;
		else
		    k++;
		order++;
	     }
	     if (order > 0)
	        scroll_up(order);
	}
	else  /* next page */
	{
	     if (botmIndex >= fifoNum)
		return;
	     order = 0;
	     k = 0;
	     node = dispStart;
	     while ( k < textRows)
	     {
		if (node->disp > 0)
		    k += 2;
		else
		    k++;
		if (node->next == NULL)
		    break;
		node = node->next;
		order++;
	     }
	     if (order > 0)
	        scroll_down(order);
	}
}


static void
load_fifo()
{
	if (popupShell == NULL)
	    disp_load_window();
	else
	{
            n = 0;
            XtSetArg (args[n], XtNstring, "    ");  n++;
	    XtSetValues(errorMessage, args, n);
	    XtPopup(popupShell, XtGrabNone);
	}
}

static void
save_popup()
{
	if (popupShell2 == NULL)
	    disp_save_window();
	else
	{
            n = 0;
            XtSetArg (args[n], XtNstring, "    ");  n++;
	    XtSetValues(errorMessage2, args, n);
	    XtPopup(popupShell2, XtGrabNone);
	}
}

static void
exit_cb(widget, client_data, call_date)
Widget          widget;
XtPointer       client_data, call_date;
{
	XCloseDisplay(dpy);
	exit(0);
}


static void
vis_event_proc(win,data,event)
Widget   win;
caddr_t  data;
XEvent  *event;
{
    if(event->type != VisibilityNotify)
        return;
    if(event->xvisibility.state == VisibilityPartiallyObscured)
        textObscured = 1;
    else
        textObscured = 0;
}


void
set_len_menu(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	int   len;

	len = (int) client_data;
       	n = 0;
	if (len == 64)
        	XtSetArg(args[n], XtNstring, "64 ");
	else
        	XtSetArg(args[n], XtNstring, "96 ");
        n++;
	XtSetValues(lenWidget, args, n);
	fifoWord = len;
}

void
set_format_menu(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	int   which;

	which = (int) client_data;
       	n = 0;
	if (which == BINARY)
        	XtSetArg(args[n], XtNstring, "binary ");
	else
        	XtSetArg(args[n], XtNstring, "ascii  ");
	n++;
	XtSetValues(formatWidget, args, n);
	fifoFormat = which;
}



XrmDatabase  dbase;

create_windows(argc, argv)
int	argc;
char	**argv;
{
	Dimension   height, height2;
	Dimension   width, width2;
	Position    pos_y;
	Widget	    twidget, tmpButton;
	Widget	    formatBut, lenBut;
	Widget	    menuPane1, menuPane2;
	char	    *resource;
	XSetWindowAttributes  attrs;

	topShell = OlInitialize("Fifo", "Fifo", NULL, 0,
                          (Cardinal *)&argc, argv);

	if (!topShell)
   	{
           fprintf(stderr, " Error: Can't open display, exiting...\n");
           exit(0);
   	}

	dpy = XtDisplay(topShell);
	screenWidth = DisplayWidth (dpy, DefaultScreen(dpy));
	winDepth = DefaultDepth(dpy, XDefaultScreen(dpy));
	if (XGetDefault(dpy,"Fifo", "font") == NULL)
	{
	     dbase = XtDatabase(dpy);
	     if (screenWidth > 1000)
		XrmPutStringResource(&dbase, "Fifo*font", "9x15");
	     else
		XrmPutStringResource(&dbase, "Fifo*font", "8x13");
	}
	get_colors();
	n = 0;
	if (infoList != NULL)
	     sprintf(bfifo, "Fifo: %s", infile);
	else
	     sprintf(bfifo, "Fifo: ");
	XtSetArg (args[n], XtNtitle, bfifo);  n++;
/*
	XtSetValues(topShell, args, n);
*/
	n = 0;
	XtSetArg (args[n], XtNwidth, 100);  n++;
	XtSetArg (args[n], XtNheight, 200);  n++;
	mainFrame = XtCreateManagedWidget("",
                        formWidgetClass, topShell, args, n);

        n = 0;
        XtSetArg (args[n], XtNwidth, 8);  n++;
        XtSetArg (args[n], XtNheight, 8);  n++;
	XtSetArg (args[n], XtNmappedWhenManaged, FALSE);  n++;
	twidget = XtCreateWidget("text", staticTextWidgetClass,
				mainFrame, args, n);
        n = 0;
	XtSetArg(args[n], XtNfont, &textFontInfo);  n++;
	XtSetArg(args[n], XtNfontColor, &textPix);  n++;
	XtSetArg(args[n], XtNborderColor, &borderPix);  n++;
	XtSetArg(args[n], XtNbackground, &winBg);  n++;
	XtGetValues(twidget, args, n);
	reversePix = winBg ^ textPix;
	charHeight = textFontInfo->max_bounds.ascent
                      + textFontInfo->max_bounds.descent;
	charWidth = textFontInfo->max_bounds.width;
	char_ascent = textFontInfo->max_bounds.ascent;
	line_gap = 6;
	if ((resource=XGetDefault(dpy,"Fifo", "lineGap")) != NULL)
	     line_gap = atoi(resource);
	if (line_gap < 2)
	     line_gap = 2;
	labelFontInfo = NULL;
	if ((resource=XGetDefault(dpy,"label", "font")) != NULL)
	    labelFontInfo = XLoadQueryFont(dpy, resource);
	if (labelFontInfo == NULL)
	{
	    if (charWidth >= 9)
	      labelFontInfo = XLoadQueryFont(dpy, "8x13");
	    else if (charWidth >= 8)
	      labelFontInfo = XLoadQueryFont(dpy, "7x13");
	    else if (charWidth >= 7)
	      labelFontInfo = XLoadQueryFont(dpy, "6x12");
	    else
	      labelFontInfo = XLoadQueryFont(dpy, "5x8");
	}
	if (labelFontInfo)
	{
	     labelHeight = labelFontInfo->max_bounds.ascent
                      + labelFontInfo->max_bounds.descent;
	     labelWidth = labelFontInfo->max_bounds.width;
	     label_ascent = labelFontInfo->max_bounds.ascent;
	}
	else
	{
	     labelWidth = charWidth;
	     labelHeight = charHeight;
	     label_ascent = char_ascent;
	}

	rowHeight = charHeight + line_gap;
	width = charWidth * max_len;
	if (width > (screenWidth - 40))
	      width = screenWidth - 40;
        n = 0;
        XtSetArg (args[n], XtNheight, charHeight * 2 + 3);  n++;
        XtSetArg (args[n], XtNwidth, width);  n++;
        XtSetArg(args[n], XtNborderWidth, 0); n++;
        XtSetArg (args[n], XtNx, 0);  n++;
        XtSetArg (args[n], XtNy, 0);  n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE);  n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE);  n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
        titleWidget = XtCreateManagedWidget("title", drawAreaWidgetClass,
                                  mainFrame, args, n);
        n = 0;
        XtSetArg (args[n], XtNheight, 4 * labelHeight + 3);  n++;
        XtSetArg (args[n], XtNwidth, width);  n++;
        XtSetArg(args[n], XtNborderWidth, 0); n++;
        XtSetArg (args[n], XtNx, 0);  n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE);  n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE);  n++;
	XtSetArg (args[n], XtNyRefWidget, titleWidget);  n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
/*
	XtSetArg (args[n], XtNyResizable, TRUE);  n++;
*/
        title2Widget = XtCreateManagedWidget("title", drawAreaWidgetClass,
                                  mainFrame, args, n);

	height = rowHeight * 20;
        n = 0;
	XtSetArg (args[n], XtNwidth, width);  n++;
	XtSetArg (args[n], XtNheight, height);  n++;
        XtSetArg(args[n], XtNborderWidth, 1); n++;
        XtSetArg(args[n], XtNsensitive, TRUE);   n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE);  n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE);  n++;
	XtSetArg (args[n], XtNyAttachBottom, FALSE);  n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyRefWidget, title2Widget);  n++;
        textWidget = XtCreateManagedWidget("", drawAreaWidgetClass,
                                  mainFrame, args, n);
        n = 0;
        XtSetArg(args[n], XtNborderWidth, 0); n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE);  n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE);  n++;
	XtSetArg (args[n], XtNyAttachBottom, TRUE);  n++;
	XtSetArg (args[n], XtNxResizable, FALSE);  n++;
	XtSetArg (args[n], XtNyResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyRefWidget, title2Widget);  n++;
	XtSetArg (args[n], XtNxRefWidget, textWidget);  n++;
	XtSetArg (args[n], XtNsliderMin, 0);  n++;
	XtSetArg (args[n], XtNsliderMax, fifoNum);  n++;
	XtSetArg (args[n], XtNproportionLength, 1); n++;
	scrollbar = XtCreateManagedWidget("scrollbar", scrollbarWidgetClass,
                                  mainFrame, args, n);

	
	XtAddEventHandler(textWidget,StructureNotifyMask,False,text_win_resize, NULL);
        n = 0;
        XtSetArg(args[n], XtNborderWidth, 0); n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE);  n++;
	XtSetArg (args[n], XtNyAddHeight, TRUE);  n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE);  n++;
	XtSetArg (args[n], XtNyAttachBottom, TRUE);  n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyResizable, FALSE);  n++;
	XtSetArg (args[n], XtNyRefWidget, textWidget);  n++;
	XtSetArg (args[n], XtNhSpace, 6);  n++;
	XtSetArg (args[n], XtNvPad, 6);  n++;
	XtSetArg (args[n], XtNhPad, 6);  n++;
	twidget = XtCreateManagedWidget("", controlAreaWidgetClass,
				 mainFrame, args, n);

        n = 0;
	upBut = XtCreateManagedWidget(" Up ",
                        oblongButtonWidgetClass, twidget, args, n);
	downBut = XtCreateManagedWidget("Down",
                        oblongButtonWidgetClass, twidget, args, n);
	nextBut = XtCreateManagedWidget("Next Page",
                        oblongButtonWidgetClass, twidget, args, n);
	prevBut = XtCreateManagedWidget("Previous Page",
                        oblongButtonWidgetClass, twidget, args, n);
	loadBut = XtCreateManagedWidget("Load Fifo",
                        oblongButtonWidgetClass, twidget, args, n);
	saveBut = XtCreateManagedWidget("Save Text",
                        oblongButtonWidgetClass, twidget, args, n);

        n = 0;
        XtSetArg(args[n], XtNstring, " Word Length:"); n++;
        XtSetArg(args[n], XtNstrip, FALSE); n++; 
        XtCreateManagedWidget("",
                        staticTextWidgetClass, twidget, args, n);
	lenBut = XtCreateManagedWidget ("menu",
                        abbrevMenuButtonWidgetClass, twidget, args, n);
	n = 0;
        XtSetArg (args[n], XtNmenuPane, (XtArgVal)&menuPane1); n++;
        XtGetValues(lenBut, args, n);
	n = 0;
        XtSetArg (args[n], XtNlabel, "64"); n++;
        tmpButton = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, menuPane1, args, n);
        XtAddCallback(tmpButton, XtNselect, set_len_menu, 64);
	n = 0;
        XtSetArg (args[n], XtNlabel, "96"); n++;
        tmpButton = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, menuPane1, args, n);
        XtAddCallback(tmpButton, XtNselect, set_len_menu, 96);
        n = 0;
        XtSetArg(args[n], XtNstring, "64 "); n++;
        XtSetArg(args[n], XtNrecomputeSize, FALSE); n++;
        XtSetArg(args[n], XtNfontColor, redPix); n++;
        lenWidget = XtCreateManagedWidget("",
                        staticTextWidgetClass, twidget, args, n);

        n = 0;
        XtSetArg(args[n], XtNstring, "Format:"); n++;
        XtCreateManagedWidget("",
                        staticTextWidgetClass, twidget, args, n);
	formatBut = XtCreateManagedWidget ("menu",
                        abbrevMenuButtonWidgetClass, twidget, args, n);
	n = 0;
        XtSetArg (args[n], XtNmenuPane, (XtArgVal)&menuPane2); n++;
        XtGetValues(formatBut, args, n);
	n = 0;
        XtSetArg (args[n], XtNlabel, "binary"); n++;
        tmpButton = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, menuPane2, args, n);
        XtAddCallback(tmpButton, XtNselect, set_format_menu, BINARY);
	n = 0;
        XtSetArg (args[n], XtNlabel, "ascii"); n++;
        tmpButton = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, menuPane2, args, n);
        XtAddCallback(tmpButton, XtNselect, set_format_menu, ASCII);
        n = 0;
        XtSetArg(args[n], XtNstring, "binary "); n++;
        XtSetArg(args[n], XtNrecomputeSize, FALSE); n++;
        XtSetArg(args[n], XtNfontColor, redPix); n++;
        formatWidget = XtCreateManagedWidget("",
                        staticTextWidgetClass, twidget, args, n);


	n = 0;
	exitBut = XtCreateManagedWidget("Exit",
                        oblongButtonWidgetClass, twidget, args, n);


	XtAddCallback(upBut, XtNselect, scroll_text, (XtPointer) 0);
	XtAddCallback(downBut, XtNselect, scroll_text, (XtPointer) 1);
	XtAddCallback(nextBut, XtNselect, change_page, (XtPointer) 0);
	XtAddCallback(prevBut, XtNselect, change_page, (XtPointer) 1);
	XtAddCallback(loadBut, XtNselect, load_fifo, NULL);
	XtAddCallback(saveBut, XtNselect, save_popup, NULL);
	XtAddCallback(exitBut, XtNselect, exit_cb, NULL);

	n = 0;
	XtSetArg (args[n], XtNheight, &height);  n++;
	XtSetArg (args[n], XtNwidth, &width);  n++;
	XtSetArg (args[n], XtNy, &pos_y);  n++;
	XtGetValues(title2Widget, args, n);
	XtRealizeWidget(topShell);

	XtSetArg (args[0], XtNheight, &height);
	XtSetArg (args[1], XtNwidth, &width);
	XtGetValues(topShell, args, 2);
	XtSetArg (args[0], XtNheight, &height2);
	XtGetValues(textWidget, args, 1);
	diff_y = height - height2;
	XtSetArg (args[0], XtNheight, &scroll_height);
	XtSetArg (args[1], XtNwidth, &scroll_width);
	XtGetValues(scrollbar, args, 2);
	titleWin = XtWindow(titleWidget);
	title2Win = XtWindow(title2Widget);
	textWin = XtWindow(textWidget);
	gc = DefaultGC(dpy, DefaultScreen(dpy));
	XSetFont(dpy, gc, textFontInfo->fid);

	set_color(textPix);
	if (DoesBackingStore(DefaultScreenOfDisplay(dpy)) == NotUseful)
              backing_store = 0;
	else
        {
              attrs.backing_store = Always;
              XChangeWindowAttributes(dpy, textWin, CWBackingStore, &attrs);
              XChangeWindowAttributes(dpy, title2Win, CWBackingStore, &attrs);
              backing_store = 1;
        }

	XtAddEventHandler(topShell,StructureNotifyMask,False,change_size, NULL);
	XtAddCallback(scrollbar, XtNsliderMoved, scrollbar_cb, (XtPointer)0);
	XtAddEventHandler(textWidget,ButtonReleaseMask, False, hilight, NULL);
	XtAddEventHandler(textWidget,VisibilityChangeMask, False, vis_event_proc, NULL);
/*
	if (!backing_store)
	{
*/
	   XtAddEventHandler(titleWidget,ExposureMask, False, draw_title, NULL);
	   XtAddEventHandler(title2Widget,ExposureMask, False, draw_title2, NULL);
	   XtAddEventHandler(textWidget,ExposureMask, False, expose_info, NULL);
/*
	}
*/
}


init_vals()
{
	int	k;

	lkpulse = lkduty = lkrate  = 0;
	lkphase = lkpower = lkfreq = 0;
	rcvr_amp = 0;
	obs_preamp = 0;
	for(k = 0; k < CHNUM; k++)
	{
	     xmtr_info[k].hs_sel = 0;
	     xmtr_info[k].amp_sel = 0;
	     xmtr_info[k].dec_hl = 0;
	     xmtr_info[k].mixer_sel = 0;
	     xmtr_info[k].fine_attn = 0;
	     xmtr_info[k].coarse_attn = 0;
	     xmtr_info[k].dmm = 0;
	     xmtr_info[k].phase = 0;
	     xmtr_info[k].dmf = 0;
	     xmtr_info[k].active = 0;
	}
}


int
create_info_list()
{
	int	len, k, x;
        FILE   *fd;
        unsigned char   buf[26];
        unsigned char   buf2[256];
	static fifo_list   *node, *node1, *node2, *snode;
	mem_list *newBlock;
	int  convert_fifo();
	

        if (strlen(infile) <= 0)
              return(0);
	if (fifoFormat == BINARY)
	{
	      if(convert_fifo() <= 0)
                 return(-1);

	}
        if ((fd = fopen(infile, "r")) == NULL)
	{
	      if (!topShell)
	         perror(infile);
              return(-1);
	}
	init_vals();
        fifoNum = 0;
        errorNum = 0;
	hilitIndex = -1;
	if (fifoList)
	{
	      clear_node(fifoList);
	      fifoList = NULL;
	}
	if (infoList)
	{
	      clear_node(infoList);
	      infoList = NULL;
	}
	imemPtr = imemList;
	fmemPtr = fmemList;
	if (fmemList)
	      fbufPtr = 0;
	if (imemList)
	      bufPtr = 0;
        while (fgets(buf2, 255, fd) != NULL)
        {
	      len = 0;
	      k = 0;
	      while (len < 16 && buf2[k] != '\0')
	      {
		  if (buf2[k] != ' ' && buf2[k] != '\n')
		  	buf[len++] = buf2[k];
		  k++;
	      }
	      if (len <= 0)
		  continue;
              fifoNum++;
	      node = (fifo_list *) malloc(sizeof(fifo_list));
	      node->next = NULL;
	      node->prev = NULL;
	      node->len = len;
	      node->id = fifoNum;
	      snode = (fifo_list *) malloc(sizeof(fifo_list));
	      snode->next = NULL;
	      snode->prev = NULL;
	      snode->id = fifoNum;
	      snode->disp = 0;
	      if ((fbufPtr+len+1) > MEMSIZE)
	      {
		  if (fmemPtr == NULL || fmemPtr->next == NULL)
		  {
		     newBlock = (mem_list *) malloc(sizeof(mem_list));
		     newBlock->data = (char *) malloc(MEMSIZE);
		     newBlock->next = NULL;
		  }
		  else
		     newBlock = fmemPtr->next;
		  if (fmemList == NULL)
		     fmemList = newBlock;
		  else
		     fmemPtr->next = newBlock;
		  fmemPtr = newBlock;
		  fbufPtr = 0;
	      }
	      node->data = fmemPtr->data + fbufPtr;
	      strcpy(node->data, buf);
	      fbufPtr = fbufPtr + len + 1;

	      if (bufPtr > (MEMSIZE - 110))
	      {
		  if (imemPtr == NULL || imemPtr->next == NULL)
		  {
		     newBlock = (mem_list *) malloc(sizeof(mem_list));
		     newBlock->data = (char *) malloc(MEMSIZE);
		     newBlock->next = NULL;
		  }
		  else
		     newBlock = imemPtr->next;
		  if (imemList == NULL)
		     imemList = newBlock;
		  else
		     imemPtr->next = newBlock;
		  imemPtr = newBlock;
		  bufPtr = 0;
	      }
	      snode->data = imemPtr->data + bufPtr;
              sprintf (snode->data, "%4d:   ", fifoNum);
              charPtr = XMARGIN;
	      if (len > 16)
		len = 16;
              convert_ch_bit (&buf[0], bfifo, len);
	      for (k = 0; k < 8; k++)
	         snode->data[charPtr++] = bfifo[k];
	      snode->data[charPtr++] = ' ';
	      /* 1-bit of AP R/W bit  */
	      snode->data[charPtr++] = bfifo[8];
	      x = 9;
              for (k = 0; k < 7; k++)
	      {
	         snode->data[charPtr++] = convert_bit_ch(4, &bfifo[x]);
		 x += 4;
	      }
	      snode->data[charPtr++] = ' ';
	      /*  1-bit of Rotor  */
	      snode->data[charPtr++] = bfifo[37];
	      snode->data[charPtr++] = bfifo[38];
	      snode->data[charPtr++] = ' ';
	      x = 0;
	      len = len * 4;
              for (k = 39; k < len; k++)
	      {
	      	  snode->data[charPtr++] = bfifo[k];
		  x++;
		  if (x >= 5)
		  {
			x = 0;
		 	snode->data[charPtr++] = ' ';
		  }
	      }
	      add_comment(snode->data);
/**
              snode->data[charPtr++] = '\0';
	      bufPtr += charPtr;
	      charPtr += k;
**/
	      snode->len = charPtr;
              snode->data[charPtr++] = '\0';
	      bufPtr += charPtr;
	      if (max_len < charPtr)
		 max_len = charPtr;
	      if (fifoList == NULL)
	         fifoList = node;
	      else
	      {
		 node->prev = node1;
		 node1->next = node;
	      }
	      node1 = node;
	      if (infoList == NULL)
	         infoList = snode;
	      else
	      {
		 snode->prev = node2;
		 node2->next = snode;
	      }
	      node2 = snode;
        }
        fclose (fd);
	if (fifoFormat == BINARY)
	   unlink(infile);
	dispStart = infoList;
	if (scrollbar)
	{
	   if (textRows > fifoNum)
		XtUnmapWidget(scrollbar);
	   else
		XtMapWidget(scrollbar);
	}
	if (infoList)
	     return(1);
	else
	     return(0);
} 


clear_node(node)
fifo_list *node;
{
	fifo_list  *pnode, *cnode;

	pnode = node;
	while (pnode != NULL)
	{
	     cnode = pnode->next;
	     free(pnode);
	     pnode = cnode;
	}
}


char
convert_bit_ch(bit_num, buf)
int	bit_num;
char    *buf;
{
	static  char  retdata;
	int	      val;

	val = 0;
	while (bit_num > 0)
	{
	    if (*buf == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    bit_num--;
	    buf++;
	}
	if (val <= 9)
	    retdata = val + '0';
	else
	    retdata = 'A' + val - 10;
	return(retdata);
}
	
	

/*  convert char to binary */
convert_ch_bit(sbuf, buf, num)
char  *sbuf;
char  *buf;
int   num;
{
	int	k, val;
	char	*ch;

    ch = sbuf;
    while (num > 0)
    {
	if (*ch >= 'a')
	   val = 10 + *ch - 'a';
	else if (*ch >= 'A')
	   val = 10 + *ch - 'A';
	else
	   val = *ch - '0';	
	switch (val) {
	   case  0:
		sprintf(buf, "0000");
		break;
	   case  1:
		sprintf(buf, "0001");
		break;
	   case  2:
		sprintf(buf, "0010");
		break;
	   case  3:
		sprintf(buf, "0011");
		break;
	   case  4:
		sprintf(buf, "0100");
		break;
	   case  5:
		sprintf(buf, "0101");
		break;
	   case  6:
		sprintf(buf, "0110");
		break;
	   case  7:
		sprintf(buf, "0111");
		break;
	   case  8:
		sprintf(buf, "1000");
		break;
	   case  9:
		sprintf(buf, "1001");
		break;
	   case  10:
		sprintf(buf, "1010");
		break;
	   case  11:
		sprintf(buf, "1011");
		break;
	   case  12:
		sprintf(buf, "1100");
		break;
	   case  13:
		sprintf(buf, "1101");
		break;
	   case  14:
		sprintf(buf, "1110");
		break;
	   case  15:
		sprintf(buf, "1111");
		break;
	   default:
		fprintf(stderr, "Error: unknown number '%c'\n", ch);
	}
	ch ++;
	buf = buf + 4;
	num --;
    }
}


reverse_line(old_line, new_line)
int	old_line, new_line;
{
	int	y;

	XSetFunction(dpy, gc, GXxor);
	set_color(reversePix);
	if (old_line >= 0)
	{
	     y = rowHeight * old_line;
	     XFillRectangle(dpy, textWin,gc,3,y,textWinWidth-6,rowHeight);
	}
	if (new_line >= 0)
	{
	     y = rowHeight * new_line;
	     XFillRectangle(dpy, textWin,gc,3,y,textWinWidth-6,rowHeight);
	}
	XSetFunction(dpy, gc, GXcopy);
	set_color(textPix);
}

get_colors()
{
     int         i;
     char	 *res;
     static char *colors[] = {"red", "peru", "brown", "cyan1", "gold2"};
     XrmValue    fromType, toType;

     xblack = XBlackPixel(dpy, 0);
     if (winDepth < 7)
     {
	redPix = textPix;
	greenPix = textPix;
	commentPix = textPix;
	errorPix = textPix;
	return;
     }
     for (i = 0; i < 5; i++)
     {
        fromType.size = sizeof (colors[i]);
        fromType.addr = colors[i];
        XtConvert(topShell, XtRString, &fromType, XtRPixel, &toType);

        switch (i) {
         case 0:
                redPix = *((Pixel *)toType.addr);
		errorPix = redPix;
                break;
         case 1:
                loopPix = *((Pixel *)toType.addr);
                break;
         case 2:
                commentPix = *((Pixel *)toType.addr);
                break;
         case 3:
                itemPix1 = *((Pixel *)toType.addr);
                break;
         case 4:
                itemPix2 = *((Pixel *)toType.addr);
                break;
        }
     }
     if ((res = XGetDefault(dpy,"Fifo", "commentColor")) != NULL)
     {
        fromType.size = sizeof (res);
        fromType.addr = res;
        XtConvert(topShell, XtRString, &fromType, XtRPixel, &toType);
	commentPix = *((Pixel *)toType.addr);
	XFree(res);
     }
     if ((res = XGetDefault(dpy,"Fifo", "errorColor")) != NULL)
     {
        fromType.size = sizeof (res);
        fromType.addr = res;
        XtConvert(topShell, XtRString, &fromType, XtRPixel, &toType);
	errorPix = *((Pixel *)toType.addr);
	XFree(res);
     }
     if ((res = XGetDefault(dpy,"Fifo", "loopColor")) != NULL)
     {
        fromType.size = sizeof (res);
        fromType.addr = res;
        XtConvert(topShell, XtRString, &fromType, XtRPixel, &toType);
	loopPix = *((Pixel *)toType.addr);
	XFree(res);
     }
}


int
bin2dec(start, len)
int	start, len;
{
	int	val;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
	return(val);
}


convert_b_d(buf, start, len, total)
char   *buf;
int	start, len;
int	*total;
{
	char    tbuf[16];
	int	val;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
	sprintf(tbuf, "%d ", val);
	len = strlen(tbuf);
	strcpy(buf, tbuf);
	*total = *total + len;
}


int
cal_delay(buf, start, len, total)
char   *buf;
int	start, len;
int	*total;
{
	char    tbuf[16];
	double	val;
	int	base;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
/*
	val = val - 7;
*/
	val = val - 5;
	if (val < 0)
	    val = 0;
/*
	val = 100 + val * 10;
	val = 37.5 + val * 12.5;
*/
	val = 100 + val * 12.5;
	base = 0;
	while (val > 1000 && base < 4)
	{
	     base++;
	     val = val / 1000;
	}
	switch (base) {
	   case 0:
		sprintf(tbuf, "%g ns", val);
		break;
	   case 1:
		sprintf(tbuf, "%g us", val);
		break;
	   case 2:
		sprintf(tbuf, "%g ms", val);
		break;
	   case 3:
		sprintf(tbuf, "%g sec", val);
		break;
	}
	strcpy(buf, tbuf);
	len = strlen(tbuf);
	*total = *total + len;
	return(len);
}


/*  ************************************************************
   Here's the tranlation code, if apbus goes to  ap_bus(comment)
   where it is further broken down for translation
   **************************************************************/
add_comment (buf)
char  *buf;
{
	char   *comment;
	int    len, ctl_val, rlen;

	ctl_val = 0;
	for (len = 0; len < 8; len++)
	{
	    if (bfifo[len] == '1')
	        ctl_val = ctl_val * 2 + 1;
	    else
	        ctl_val = ctl_val * 2;
	}
	comment = buf + charPtr;
	len = 0;
	if (ctl_val == CLOOP)
	{
		strcpy(comment, "$set loop counter to ");
		len = 21;
		convert_b_d(comment + len, 13, 24, &len);
		charPtr += len;
		strcat(comment, "+ 1 ");
		charPtr += 5;
		return;
	}
	if (bfifo[7] == '0' && bfifo[8] == '1')
	{
		strcpy(comment, " halt ... ");
		len = 10;
		charPtr += len;
		return;
	}

	switch (ctl_val) {
	   case  ROTOR:
		strcpy(comment, " rotor sync");
		len = 11;
		break;
	   case  EXTCLK:
		sprintf(comment, " set extern clock counter to ");
		len = 29;
		convert_b_d(comment + len, 9, 28, &len);
		break;
	   case  INTRP:
		strcpy(comment, " software interrupt ");
		len = 20;
		convert_b_d(comment + len, 33, 4, &len);
		break;
	   case  STAG:
		strcpy(comment, " id tag ");
		len = 8;
		convert_b_d(comment + len, 19, 18, &len);
		break;
	   case  CTC:
		strcpy(comment, " acquire data, and delay ");
		len = 25;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  CTCST:
		strcpy(comment, "$acquire data, delay ");
		len = 21;
		comment += len;
		rlen = cal_delay(comment, 9, 28, &len);
		strcpy(comment+rlen, ", start loop ");
		len += 13;
		break;
	   case  CTCEN:
		strcpy(comment, "$acquire data, delay ");
		len = 21;
		comment += len;
		rlen = cal_delay(comment, 9, 28, &len);
		strcpy(comment+rlen, ", end loop ");
		len += 11;
		break;
	   case  SLOOP:
		strcpy(comment, "$start loop, delay ");
		len = 19;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  ELOOP:
		strcpy(comment, "$end loop, delay ");
		len = 17;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  APBUS:
/*
		strcpy(comment, " Ap bus: ");
		len = 9;
		charPtr += len;
		ap_bus(comment + len);
*/
		ap_bus(comment);
		return;
		break;
	   case  DELAY:
		sprintf(comment, " delay ");
		len = 7;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   default:
		strcpy(comment, "? ??? ");
		len = 6;
		errorNum++;
		break;
	}
	charPtr += len;
}

set_color(color)
Pixel  color;
{

	if (color != xcolor)
	{
	   xcolor = color;
	   XSetForeground(dpy, gc, color);
	}
}

void 
rt_fifo()
{
	int	ret;
	char   *data, *data2;

        XtSetArg(args[0], XtNstring, &data);
        XtGetValues(fileWidget, args, 1);
        data2 = data;
        while (*data2 == ' ' || *data2 == '\t')
            data2++;
	if (strlen(data2) > 0)
            strcpy(infile, data2);
        XtFree(data);
	ret = create_info_list();
	if (ret < 0)
	{
            n = 0;
            XtSetArg (args[n], XtNstring, "file does not exist.");  n++;
	    XtSetValues(errorMessage, args, n);
	    XBell(dpy, 20);
	    return;
	}
	if (ret > 0)
	{
	    XtPopdown(popupShell);
	    n = 0;
	    XtSetArg (args[n], XtNsliderMin, 0);  n++;
	    XtSetArg (args[n], XtNsliderMax, fifoNum);  n++;
	    XtSetArg (args[n], XtNproportionLength, 1); n++;
	    XtSetArg (args[n], XtNsliderValue, 0);  n++;
	    XtSetValues(scrollbar, args, n);
	    scrollIndex = 1;
	    sprintf(bfifo, "Fifo: %s", infile); 
	    n = 0;
	    XtSetArg (args[n], XtNtitle, bfifo);  n++;
	    XtSetValues(topShell, args, n);
	    disp_info(1);
	    update_message();
	}
}



static void
verify_file(widget, client_data, call_date)
        Widget          widget;
        XtPointer       client_data,
                        call_date;
{
        OlTextFieldVerify  *fv;

        fv = (OlTextFieldVerify *)call_date;
        if (fv->reason == OlTextFieldReturn)
                rt_fifo();
}

void  save_text()
{
	char   *data, *data2;
	FILE    *fout;
	fifo_list   *node;
	char    tmp_str[256];

        XtSetArg(args[0], XtNstring, &data);
        XtGetValues(saveWidget, args, 1);
        data2 = data;
        while (*data2 == ' ' || *data2 == '\t')
            data2++;
	if (strlen(data2) > 0)
            strcpy(outfile, data2);
        XtFree(data);

	if (infoList == NULL)
	     return;
        if ((fout = fopen(outfile, "w")) == NULL)
	{
	      if (!topShell)
	         perror(outfile);
              return;
	}

	node = infoList;
	while (node != NULL)
	{
	   strncpy(tmp_str, node->data, node->len);
	   tmp_str[node->len] = '\0';
	   fprintf(fout, "%s \n",tmp_str);
	   node = node->next;
	}

        fclose(fout);
	XtPopdown(popupShell2);
}


static void
verify_save_file(widget, client_data, call_date)
        Widget          widget;
        XtPointer       client_data,
                        call_date;
{
        OlTextFieldVerify  *fv;

        fv = (OlTextFieldVerify *)call_date;
        if (fv->reason == OlTextFieldReturn)
                save_text();
}


void
close_popup()
{
	XtPopdown(popupShell);
}

void
close_popup2()
{
	XtPopdown(popupShell2);
}



disp_load_window()
{
        Widget     w1, w2, w3, w4, but1, but2;
	Widget 	   tmpWidget;
        Dimension  width, height;
	Position    pos_x, pos_y;

	n = 0;
	XtSetArg (args[n], XtNx, &pos_x);  n++;
	XtSetArg (args[n], XtNy, &pos_y);  n++;
	XtGetValues(topShell, args, n);
	n = 0;
	XtSetArg (args[n], XtNresizeCorners, TRUE);  n++;
	XtSetArg (args[n], XtNpushpin, OL_NONE);  n++;
	XtSetArg (args[n], XtNx, pos_x + 100);  n++;
	XtSetArg (args[n], XtNy, pos_y + 100);  n++;
	popupShell = XtCreatePopupShell("Load fifo", popupWindowShellWidgetClass,
                              topShell, args, n);
	n = 0;
	XtSetArg(args[n], XtNupperControlArea, &tmpWidget);  n++;
	XtGetValues (popupShell, args, n);

	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	popupFrame = XtCreateManagedWidget("",
                        controlAreaWidgetClass, tmpWidget, args, n);

        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        w1 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popupFrame, args, n);

        n = 0;
        XtSetArg (args[n], XtNstring, "File:");  n++;
        w2 = XtCreateManagedWidget("",
                        staticTextWidgetClass, w1, args, n);
        n = 0;
        XtSetArg (args[n], XtNcharsVisible, 30);  n++;
        fileWidget = XtCreateManagedWidget("",
                        textFieldWidgetClass, w1, args, n);
        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        XtSetArg (args[n], XtNhSpace, 80);  n++;
        XtSetArg (args[n], XtNhPad, 60);  n++;
        w3 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popupFrame, args, n);
        n = 0;
	but1 = XtCreateManagedWidget("Load",
                        oblongButtonWidgetClass, w3, args, n);
	but2 = XtCreateManagedWidget("Cancel",
                        oblongButtonWidgetClass, w3, args, n);
        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        w4 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popupFrame, args, n);
        n = 0;
        XtSetArg (args[n], XtNwidth, charWidth * 20);  n++;
        XtSetArg (args[n], XtNheight, charHeight);  n++;
        errorMessage = XtCreateManagedWidget("",
                        staticTextWidgetClass, w4, args, n);
	XtAddCallback(but1, XtNselect, rt_fifo, (XtPointer) 0);
	XtAddCallback(but2, XtNselect, close_popup, (XtPointer) 0);

        XtAddCallback(fileWidget, XtNverification, verify_file, NULL);

	XtRealizeWidget(popupShell);
	XtPopup(popupShell, XtGrabNone);
}

disp_save_window()
{
        Widget     w1, w2, w3, w4, but1, but2;
	Widget 	   tmpWidget;
        Dimension  width, height;
	Position    pos_x, pos_y;
        Widget popFrame;

	n = 0;
	XtSetArg (args[n], XtNx, &pos_x);  n++;
	XtSetArg (args[n], XtNy, &pos_y);  n++;
	XtGetValues(topShell, args, n);
	n = 0;
	XtSetArg (args[n], XtNresizeCorners, TRUE);  n++;
	XtSetArg (args[n], XtNpushpin, OL_NONE);  n++;
	XtSetArg (args[n], XtNx, pos_x + 100);  n++;
	XtSetArg (args[n], XtNy, pos_y + 100);  n++;
	popupShell2 = XtCreatePopupShell("Save Text", popupWindowShellWidgetClass,
                              topShell, args, n);
	n = 0;
	XtSetArg(args[n], XtNupperControlArea, &tmpWidget);  n++;
	XtGetValues (popupShell2, args, n);

	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	popFrame = XtCreateManagedWidget("",
                        controlAreaWidgetClass, tmpWidget, args, n);

        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        w1 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popFrame, args, n);

        n = 0;
        XtSetArg (args[n], XtNstring, "File:");  n++;
        w2 = XtCreateManagedWidget("",
                        staticTextWidgetClass, w1, args, n);
        n = 0;
        XtSetArg (args[n], XtNcharsVisible, 30);  n++;
        saveWidget = XtCreateManagedWidget("",
                        textFieldWidgetClass, w1, args, n);
        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        XtSetArg (args[n], XtNhSpace, 80);  n++;
        XtSetArg (args[n], XtNhPad, 60);  n++;
        w3 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popFrame, args, n);
        n = 0;
	but1 = XtCreateManagedWidget("Save",
                        oblongButtonWidgetClass, w3, args, n);
	but2 = XtCreateManagedWidget("Cancel",
                        oblongButtonWidgetClass, w3, args, n);
        n = 0;
        XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
        w4 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, popFrame, args, n);
        n = 0;
        XtSetArg (args[n], XtNwidth, charWidth * 20);  n++;
        XtSetArg (args[n], XtNheight, charHeight);  n++;
        errorMessage2 = XtCreateManagedWidget("",
                        staticTextWidgetClass, w4, args, n);
	XtAddCallback(but1, XtNselect, save_text, (XtPointer) 0);
	XtAddCallback(but2, XtNselect, close_popup2, (XtPointer) 0);

        XtAddCallback(saveWidget, XtNverification, verify_save_file, NULL);

	XtRealizeWidget(popupShell2);
	XtPopup(popupShell2, XtGrabNone);
}



/* **********************************************************************

   Apbus break down for tranlation
   
   **********************************************************************/
ap_bus(outbuf)
char  *outbuf;
{
	int   reg_num, k, chip_num;
        long dataVal;

	if (bfifo[8] == '1')
	{
	    sprintf(outbuf, "Ap: read ");
	    charPtr += 9;
	    return;
	}
	chip_num = 0;
	for (k = APCHIP; k < APCHIP + APCHIPLEN; k++)
	{
	    if (bfifo[k] == '1')
	         chip_num = chip_num * 2 + 1;
	    else
	         chip_num = chip_num * 2;
	}
	if (chip_num == 7)  /*  AP & F */
	{
	    ap_f(outbuf);
	    return;
	}
	reg_num = 0;
	for (k = APREG; k < APREG + APREGLEN; k++)
	{
	       if (bfifo[k] == '1')
	         reg_num = reg_num * 2 + 1;
	       else
	         reg_num = reg_num * 2;
	}

	dataVal = 0L;
        /* APDATA - 8 , since we want 16 bit values not just 8 bit values 29-36*/
	/* k = 21 to 36 */
	for (k = APDATA - 8; k < APDATA + APREGLEN; k++)
	{
	    if (bfifo[k] == '1')
	         dataVal = dataVal * 2 + 1;
	    else
	         dataVal = dataVal * 2;
        }


	if (chip_num == 0xB)
	{
	    if (reg_num >= 0x90 && reg_num <= 0xCB)
	    {
		xmtr_control(outbuf, reg_num);
		return;
	    }
	    if (reg_num >= 0x50 && reg_num <= 0x57)
	    {
		lock_control(outbuf, reg_num);
		return;
	    }
	    if (reg_num >= 0x48 && reg_num <= 0x4f)
	    {
		magnet_leg_interface(outbuf, reg_num);
		return;
	    }
	    if (reg_num >= 0x40 && reg_num <= 0x43)
	    {
		rcvr_interface(outbuf, reg_num);
		return;
	    }
	    if (reg_num >= 0x30 && reg_num <= 0x3F)
	    {
		amp_route(outbuf, reg_num);
		return;
	    }
	}

	/* if it a waveform generator, RF or Gradient */
	if (chip_num == 0xC)
	{
	    wfggrd(outbuf, reg_num, dataVal & 0xffff);
        /*    strcpy(comstr,outbuf); */
            return;
        }

        /* MSR board ap */
        if (chip_num == 0xD)
        {
	    if ((reg_num >= 0) && ( reg_num <= 0xf))
  	       strcpy(tmpstr," MSR board");
	    else if ((reg_num >= 0x10) && ( reg_num <= 0x1f))
  	       strcpy(tmpstr," BOB board");
	    else
	       strcpy(tmpstr,"???? ");
            strcpy(outbuf,tmpstr);
	    charPtr += strlen(tmpstr);
            return;
        }

	/* STM Ap */
	if (chip_num == 0xE)
	{
	    /* sprintf(outbuf, "  STM... "); */
	    /* charPtr += 9; */

             int stm_num = 0;

            /* STM_AP_ADR      0x0E00,  STM_AP_ADR2     0x0E20 
	       STM_AP_ADR3     0x0E40,  STM_AP_ADR4     0x0E60
            */

            /* printf("STM: reg_num: %d, dataVal: 0x%lx\n",reg_num,dataVal); */
 
	    if (reg_num > 0x5f)  /* E60 */
            {
              stm_num = 3;
	      reg_num -= 0x60;
            }
            else if (reg_num > 0x3f) /* E40 */
            {
              stm_num = 2;
	      reg_num -= 0x40;
            }
            else if (reg_num > 0x1f)  /* E20 */
            {
              stm_num = 1;
	      reg_num -= 0x20;
            }
            else
	      stm_num = 0;

            stm_control(outbuf,stm_num,reg_num,dataVal & 0xffff);
	    return;
	}
	sprintf(outbuf, "???? ");
	charPtr += 5;
	errorNum++;
}

ap_f(outbuf)
char  *outbuf;
{
	int    		reg_num, k, m, dataVal, ptsno;
	static int      freqIndex = 0, mode;
	static double	freq, f1, f2;

	reg_num = 0;
	for (k = APREG; k < APREG + APREGLEN; k++)
	{
	    if (bfifo[k] == '1')
	         reg_num = reg_num *2 + 1;
	    else
	         reg_num = reg_num *2;
	}
	dataVal = 0;
	for (k = APDATA; k < APDATA + APREGLEN; k++)
	{
	    if (bfifo[k] == '1')
	         dataVal = dataVal * 2 + 1;
	    else
	         dataVal = dataVal * 2;
	}
	switch (reg_num) {
	  case  0x20:
		freqIndex = 0;
		freq = 0.0;
		if (dataVal == 0)
		{
		   mode = 0;  /* direct mode */
		   sprintf(outbuf, " set frequency in direct mode ");
		   charPtr += 30;
		}
		else if (dataVal == 4)
		{
		   mode = 1;  /* overrange mode */
		   sprintf(outbuf, " set frequency in overrange mode ");
		   charPtr += 33;
		}
		else if (dataVal == 0x0C)
		{
		   mode = 2;  /* underrange mode */
		   sprintf(outbuf, " set frequency in underrange mode ");
		   charPtr += 34;
		}
		else
		{
		   	sprintf(outbuf, " unknown mode %2X ", dataVal);
			charPtr += 17;
		}
		break;
	  case  0x21:
		 f1 = 0;
		 f2 = 0;
		/*  data in fifo is reversed */
		 for (k = APDATA; k < APDATA + 4; k++)
		 {
		   if (bfifo[k] == '0')
		     f2 = f2 * 2 + 1;
		   else
		     f2 = f2 * 2;
		 }
		 for (k = APDATA+4; k < APDATA + 8; k++)
		 {
		   if (bfifo[k] == '0')
		     f1 = f1 * 2 + 1;
		   else
		     f1 = f1 * 2;
		}
		m = 0;
		switch (freqIndex) {
		   case 0:
		      freq = f1 * 0.1 + f2;
		      break;
		   case 1:
		      freq = freq + f1 * 10 + f2 * 100;
		      break;
		   case 2:
		      freq = freq + f1 * 1000 + f2 * 10000;
		      break;
		   case 3:
		      freq = freq + f1 * 100000 + f2 * 1.0e6;
		      break;
		   case 4:
		      freq = freq + f1 * 10.0e6 + f2 * 100.0e6;
		      break;
		   default:
		      {
			m = 1;
		        sprintf(outbuf, "? Error: over frequency range ");
		        charPtr += 30;
			errorNum++;
		      }
		}
		freqIndex++;
		if (m == 0)
		{
		   sprintf(tmpstr, "   \(%d\) frequency is %g Hz ", freqIndex,  freq);
		   m = strlen(tmpstr);
		   strcpy(outbuf, tmpstr);
		   charPtr += m;
		}
		break;
	  case  0x22:
		if (dataVal > 0)
		{
		   if ((dataVal != 1) & (dataVal != 2))
                   {
		     ptsno = 2;
                     while(1)
                     {
		        dataVal = dataVal >> 1;
			if (dataVal == 1)
			   break;
			ptsno++; 
                     }
                   }
	           else
		     ptsno = dataVal;

		   if (mode == 1)
		        freq = freq + 100000;
		   else if (mode == 2)
		        freq = 100000 - freq;
		   k = 0;
		   if (freq > 1000.0)
		   {
			freq = freq / 1000.0;
			k++;
		   }
		   if (freq > 1000.0)
		   {
			freq = freq / 1000.0;
			k++;
		   }
		   sprintf(outbuf, " set channel %d frequency to ", ptsno);
                   charPtr += 28;
		   outbuf = outbuf + 28;
		   if (k == 0)
		   {
			sprintf(tmpstr, "%g Hz ", freq);
		   }
		   else if (k == 1)
		   {
			sprintf(tmpstr, "%g KHz ", freq);
		   }
		   else
		   {
			sprintf(tmpstr, "%g MHz ", freq);
		   }
		   strcpy(outbuf, tmpstr);
		   charPtr += strlen(tmpstr);
		}
		else
		{
		   sprintf(outbuf, " reset Ap latch bits ");
		   charPtr += 21;
		}
		break;
	   default:
		sprintf(outbuf, "? Error: unknown register 0x%2X ", reg_num);
		charPtr += 31;
		errorNum++;
		break;
	}
}



xmtr_control(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	static int   chan = 1;
	int	     degree;
	static double dmf1, dmf2, dmf3;

	if (reg_num >= 0xC0)
	{
	    reg_num = reg_num - 0xC0;
	    chan = 4;
	}
	else if (reg_num >= 0xB0)
	{
	    reg_num = reg_num - 0xB0;
	    chan = 3;
	}
	else if (reg_num >= 0xA0)
	{
	    reg_num = reg_num - 0xA0;
	    chan = 2;
	}
	else
	{
	    reg_num = reg_num - 0x90;
	    chan = 1;
	}
	switch (reg_num) {
	  case  0:
		xmtr_info[chan].amp_sel = bin2dec(29, 4);
		xmtr_info[chan].hs_sel = bin2dec(33, 4);
		if (xmtr_info[chan].hs_sel == 0)
		{
		   sprintf(outbuf, "? Error: bad HS Line selection '0' ");
		   charPtr += 35;
		   errorNum++;
		   return;
		}
		sprintf(outbuf, " xmtr %d use group %d of HSline, ", chan, xmtr_info[chan].hs_sel);
		charPtr += 31;
		outbuf += 31;
		if (xmtr_info[chan].amp_sel == 1)
		    sprintf(outbuf, "and high band amp ");
		else
		    sprintf(outbuf, "and low band amp  ");
		charPtr += 18;
		break;
	  case  1:
		xmtr_info[chan].mixer_sel = bin2dec(29, 1);
		xmtr_info[chan].dec_hl = bin2dec(28, 1);
		if (xmtr_info[chan].dec_hl == 1)
		{
		    sprintf(outbuf, " set xmtr %d decoupler to CW mode ", chan);
		    charPtr += 33;
		}
		else if (xmtr_info[chan].mixer_sel)
		{
		    sprintf(outbuf, " set xmtr %d mixer to high band ", chan);
		    charPtr += 31;
		}
		else
		{
		    sprintf(outbuf, " set xmtr %d mixer to low band ", chan);
		    charPtr += 30;
		}
		break;
	  case  2:
		sprintf(outbuf, " overwrite transmitter / receiver pair ");
		charPtr += 39;
		break;
	  case  3:
		break;
	  case  4:
		degree = bin2dec(29, 8);
		xmtr_info[chan].phase = (double) degree * 0.25;
		sprintf(tmpstr, "   (1) small-angle phase %g degree ", xmtr_info[chan].phase);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  5:
		degree = bin2dec(35, 2);
		xmtr_info[chan].phase += (double) degree * 64;
		if (xmtr_info[chan].phase > 90.0)
		    xmtr_info[chan].phase += 38.0;
		sprintf(tmpstr, "   (2) set xmtr %d phase %g degree ", chan, xmtr_info[chan].phase);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  6:
		xmtr_info[chan].fine_attn = bin2dec(25, 12);
		sprintf(tmpstr, "   (1) linear amplitude %d ", xmtr_info[chan].fine_attn);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  7:
		xmtr_info[chan].fine_attn += bin2dec(33, 4) * 256;
		sprintf(tmpstr, "   (2) xmtr %d linear amplitude is %d ", chan, xmtr_info[chan].fine_attn);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  8:
		degree = bin2dec(29, 8);
		dmf1 = (double) degree * 4.76837;
		sprintf(tmpstr, "   (1) xmtr %d DMF %g Hz ", chan, dmf1);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  9:
		degree = bin2dec(29, 8);
		dmf2 = (double) degree * 1220.7031;  /* 4.76 * 2^8  */
		sprintf(tmpstr, "   (2) xmtr %d DMF %g Hz ", chan, dmf2 + dmf1);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  10:
		degree = bin2dec(34, 3);
		dmf3 = (double) degree * 312500;  /*  4.76 * 2^16 */
		xmtr_info[chan].dmf = dmf3 + dmf2 + dmf1;
		sprintf(tmpstr, "   (3) xmtr %d DMF %g Hz ", chan, xmtr_info[chan].dmf);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  11:
		xmtr_info[chan].dmm = bin2dec(33, 4);
		xmtr_info[chan].active = bin2dec(29, 1);
		if (!xmtr_info[chan].active)
		{
		   sprintf(outbuf, " disable xmtr %d DMF ", chan);
		   charPtr += 20;
		   return;
		}
		else
		{
		   sprintf(outbuf, " enable xmtr %d DMF, set mode to ",chan);
		   charPtr += 32;
		   outbuf += 32;
		   switch (xmtr_info[chan].dmm) {
		      case 0:
		      case 8:
			   strcpy(tmpstr, "CW ");
		           break;
		      case 1:
		      case 9:
			   strcpy(tmpstr, "Square wave ");
		           break;
		      case 2:
		      case 10:
			   strcpy(tmpstr, "FM-FM ");
		           break;
		      case 3:
		      case 11:
			   strcpy(tmpstr, "External ");
		           break;
		      case 4:
		      case 12:
			   strcpy(tmpstr, "Waltz ");
		           break;
		      case 5:
		      case 13:
			   strcpy(tmpstr, "GARP ");
		           break;
		      case 6:
		      case 14:
			   strcpy(tmpstr, "MLEV ");
		           break;
		      case 7:
		      case 15:
			   strcpy(tmpstr, "XY32 ");
		           break;
		   }
		   strcpy(outbuf, tmpstr);
		   charPtr += strlen(tmpstr);
		}
		break;
	  default:
		sprintf(outbuf, "? Error: unknown register %2X ", reg_num);
		charPtr += 29;
		errorNum++;
		break;
	}
}



stm_control(char *outbuf, int stm_num, int reg_num, long data)
{
    int i = stm_num;
    char stmtmpstr[256];

/*
    		sprintf(stmtmpstr,"STM apdata: 0x%lx    ",data);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
*/

    switch(reg_num)
    {
       case 0x0 : 	/*  Control Register */ 
    		sprintf(stmtmpstr,"STM %d Cntrl Reg: 0x%lx",i,data);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		stmcntrlinfo(stmtmpstr,data);
                strcat(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x2 : 	/*  Tag Word (16 bits) */ 
    		sprintf(stmtmpstr,"STM %d Tag Reg: 0x%lx",i,data);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;

       case 0x4 :	/* Spares */
       case 0x6 :
       case 0x8 :
       case 0xa :
    		sprintf(stmtmpstr,"STM %d Spare Regs",i);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;

       case 0x0c:	/* STM_AP_MAXSUM0  0x0c    Maxsum word 0 */
    		sprintf(stmtmpstr,"STM %d LSW MaxSum",i);
		MaxSum = data;
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;

       case 0x0e: 	/* STM_AP_MAXSUM1  0x0e    Maxsum word 1 */
		MaxSum |=  data << 16;
		/* printf("MaxSum: %ld\n",MaxSum); */
    		sprintf(stmtmpstr,"STM %d LSW & MSW MaxSum: %ld (0x%lx)",i,MaxSum,MaxSum);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;

       case 0x10: 	/* STM_AP_SRC_ADR0 0x10    Source Address low word (16 bits) */
		SrcAddr = data;
    		sprintf(stmtmpstr,"STM %d LSW SrcAddr",i);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x12: 	/* STM_AP_SRC_ADR1 0x12    Source Address High word (16 bits) */
		SrcAddr |= data << 16;
		/* printf("SrcAddr: 0x%lx\n",SrcAddr); */
    		sprintf(stmtmpstr,"STM %d LSW & MSW SrcAddr: 0x%lx",i,SrcAddr);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x14: 	/* STM_AP_DST_ADR0 0x14    Destination Address low word (16 bits) */
		DstAddr = data;
    		sprintf(stmtmpstr,"STM %d LSW DstAddr",i);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x16: 	/* STM_AP_DST_ADR1 0x16    Destination Address High word (16 bits) */
		DstAddr |= data << 16;
		/* printf("DstAddr: 0x%lx\n",SrcAddr); */
    		sprintf(stmtmpstr,"STM %d LSW & MSW DstAddr: 0x%lx",i,DstAddr);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x18: 	/* STM_AP_NP_CNT0  0x18    Remaining # Points low word (16 bits) */
		NP = data;
    		sprintf(stmtmpstr,"STM %d LSW NP",i);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x1a: 	/* STM_AP_NP_CNT1  0x1a    Remaining # Points High word (16 bits) */
		NP |= data << 16;
		/* printf("NP: 0x%lx\n",SrcAddr); */
    		sprintf(stmtmpstr,"STM %d LSW & MSW NP: %ld (0x%lx)",i,NP,NP);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x1c: 	/* STM_AP_NTR_CNT0 0x1c    Remaining # Transients low word (16 bits) */
		NT = data;
    		sprintf(stmtmpstr,"STM %d LSW NT",i);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;
       case 0x1e: 	/* STM_AP_NTR_CNT1 0x1e    Remaining # Transients High word (16 bits) */
		NP |= data << 16;
		/* printf("NT: 0x%lx\n",SrcAddr); */
    		sprintf(stmtmpstr,"STM %d LSW & MSW NT: %ld (0x%lx)",i,NT,NT);
                strcpy(outbuf,stmtmpstr);
    		charPtr += strlen(stmtmpstr);
		break;

    }
}


stmcntrlinfo(char *outbuf,int data)
{
    char tmpbuf[256];
    int phase = decodephs(data&0x3);
    sprintf(tmpbuf," - phase: %d, ",phase);
    strcpy(outbuf,tmpbuf);

    if ( 0x8 & data )
    {
       strcpy(tmpbuf," MemZero, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x10 & data )
    {
       strcpy(tmpbuf," Single Prec, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x20 & data )
    {
       strcpy(tmpbuf," Enable ADC1, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x40 & data )
    {
       strcpy(tmpbuf," Enable ADC2, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x80 & data )
    {
       strcpy(tmpbuf," Enable STM, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x100 & data )
    {
       strcpy(tmpbuf," Reload Np & Addrs, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x800 & data )
    {
       strcpy(tmpbuf," HS Ovflow Itrp Enable, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x1000 & data )
    {
       strcpy(tmpbuf," Remaining NT Zero Itrp Enable, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x2000 & data )
    {
       strcpy(tmpbuf," Remaining NP Zero Itrp Enable, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x4000 & data )
    {
       strcpy(tmpbuf," MaxSum Itrp Enable, ");
       strcat(outbuf,tmpbuf);
    }
    if ( 0x8000 & data )
    {
       strcpy(tmpbuf," Immediate Itrp Enable, ");
       strcat(outbuf,tmpbuf);
    }
}


decodephs(unsigned long phase)
{
    int phaseval;
    switch(phase)
    {
       case 0x0:
		phaseval = 0;
		break;
       case 0x1:
		phaseval = 90;
		break;
       case 0x2:
		phaseval = 180;
		break;
       case 0x3:
		phaseval = 270;
		break;
    }
    return(phaseval);
}



wfggrd(char *outbuf, int reg_num,unsigned long data)
{

  if (reg_num >= 0x10 && reg_num < 0x50)
  {
    rfwfggrd(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x50 && reg_num < 0x60)
  {
     pfg2(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x60 && reg_num < 0x70)
  {
     pfg1(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x88 && reg_num < 0x90)
  {
     triax(outbuf,reg_num,data);
  }
  else
  {
    sprintf(tmpstr," C%x - ???? ",reg_num);
    strcpy(outbuf,tmpstr);
    charPtr += strlen(tmpstr);
  }
}

static int activewfg;

rfwfggrd(char *outbuf, int reg_num,unsigned long data)
{
   char strbuf[512];
/** The Waveform Generator has  ap address of
 * 0xc10 - rf xmtr1
 * 0xc18 - rf dec1
 * 0xc20 - X gradient
 * 0xc28 - Y gradient
 * 0xc30 - Z gradient
 * 0xc38 - R gradient (unused memorial to David Foxall)
 * 0xc40 - rf dec 3
 * 0xc48 - rf dec 4

***********************************
 * This class only deals with a wavegen used for gradients.
 *
 * Register Definitions
 ***********************************
 * Write Control functions
 * Reg 0 (base+0)       - Select start address in Wfg memory
 * Reg 1 (base+1)       - Write data to be stored in Wfg memory
 *                        address automatically increments after each write.
 * Reg 2 (base+2)       - Write control register
 * Reg 3 (base+3)       - Direct output register (for gradient values)
 *
 * Control Register definition
 * bit 0                - Start command
 * bit 1                - infinite loop
 * bit 2                - mode
 * bit 3                - point and go ?
 * bit 7                - board reset
 *
 *
 *  RF Wfg
 * IB_START =   0x00000000;
 * IB_STOP =    0x20000000;
 * IB_SCALE =   0x40000000;
 * IB_DELAYTB = 0x60000000;
 * IB_WAITHS =  0x80000000;
 * IB_PATTB =   0xa0000000;
 * IB_LOOPEND = 0xc0000000;
 * IB_SEQEND =  0xe0000000;
 * AMPGATE = 0x10000000;
 *
 *
 * ADDR_REG  = 0;
 * LOAD_REG  = 1;
 * CMD_REG   = 2;
 * DIRECT_REG= 3;
 * CMDGO_REG = 4; 
 * CMDLOOP_REG = 6; 
 */
  int len;
  if (reg_num >= 0x10 && reg_num <= 0x17)
  {
    sprintf(strbuf," Xmtr 1 WFG");
    addrfcmd(strbuf,reg_num - 0x10,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x18 && reg_num <= 0x1a)
  {
    sprintf(strbuf," Xmtr 2 WFG");
    addrfcmd(strbuf,reg_num - 0x18,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x40 && reg_num <= 0x47)
  {
    sprintf(strbuf," Xmtr 3 WFG");
    addrfcmd(strbuf,reg_num - 0x40,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x48 && reg_num <= 0x4a)
  {
    sprintf(strbuf," Xmtr 3 WFG");
    addrfcmd(strbuf,reg_num - 0x48,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x20 && reg_num <= 0x27)
  {
    sprintf(strbuf," X Axis Gradient WFG");
    addrfcmd(strbuf,reg_num - 0x20,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x28 && reg_num <= 0x29)
  {
    sprintf(strbuf," Y Axis Gradient WFG");
    addrfcmd(strbuf,reg_num - 0x28,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x30 && reg_num <= 0x37)
  {
    sprintf(strbuf," Z Axis Gradient WFG");
    addrfcmd(strbuf,reg_num - 0x30,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else if (reg_num >= 0x38 && reg_num <= 0x39)
  {
    sprintf(strbuf," R Axis Gradient WFG");
    addrfcmd(strbuf,reg_num - 0x38,data);
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
  else
  {
    sprintf(strbuf,"???? ");
    strcpy(outbuf,strbuf);
    charPtr += strlen(strbuf);
  }
}
 
#define TYPE_IB 1
#define TYPE_PAT 2
static int wfgdatatype;

addrfcmd(char *outbuf,int reg_num, unsigned long data)
{
  char tmpstr[80];
  unsigned long addr;
 /*
 * Reg 0 (base+0)       - Select start address in Wfg memory
 * Reg 1 (base+1)       - Write data to be stored in Wfg memory
 *                        address automatically increments after each write.
 * Reg 2 (base+2)       - Write control register
 */
    /* printf("regn: 0x%x, data: 0x%lx\n",reg_num,data); */
    if (reg_num == 0)
    {
       addr = data & 0xffff;
       sprintf(tmpstr,"- Select Start Addr: 0x%lx",addr);
       strcat(outbuf,tmpstr);
       if ( addr < 0xf000)  /* add below 0xf000 are instruction blocks */
	 wfgdatatype = TYPE_IB;
       else
         wfgdatatype = TYPE_PAT;

    }
    else if (reg_num == 1)
    {
       if (wfgdatatype == TYPE_IB)
       { 
           strcat(outbuf," - Write IB: ");
           decodeIB(outbuf,data);
       } 
       else if (wfgdatatype == TYPE_PAT)
       { 
           strcat(outbuf," - Write Pattern: ");
           decodePat(outbuf,data);
       } 
       /* strcat(outbuf," - Write data to wfg memory"); */
    }
    else if (reg_num == 2)
    {
       strcat(outbuf," - Write cntrl Reg:");
       if (data & 0x80)
         strcat(outbuf," Reset,");
       if (data & 0x2)
         strcat(outbuf," Infinite Loop,");
       if (data & 0x1)
         strcat(outbuf," Start,");
    }
    else if (reg_num == 3)
    {
       strcat(outbuf," - Direct Output Reg.");
    }
    else
    {
	strcat(outbuf," - unknown Reg.");
    }
}

static int ib_msb = 0;
static unsigned long ib_tmpval;

decodeIB(char *outbuf,unsigned long data)
{
    char tmpstr[80];
    unsigned int type;
    unsigned long start_addr,end_addr;
    int delaycnt,loopcnt,ampscaler,pattimecnt;

    /* printf(">>>> raw data: 0x%lx\n", data & 0xffff); */
    if (ib_msb == 0)
    {
      ib_tmpval = data & 0xffff;
      ib_msb = 1;
      /* printf(">>>> tmpval: 0x%lx\n", ib_tmpval); */
      return(0);
    }
    else
    {
      ib_tmpval |= (data & 0xffff) << 16;;
      /* printf(">>>> tmpval: 0x%lx\n", ib_tmpval); */
      ib_msb = 0;
    }
 
    /* printf("\n>>>> tmpval = 0x%lx\n",ib_tmpval); */
 
    type = ib_tmpval >> 29;
 
    /* printf(" ==== type = %d \n",type); */
 
    if (type == 0)
    {
        int ibindex;
        start_addr = (ib_tmpval >> 8) & 0xffff;
        sprintf(tmpstr,"pattern addr: 0x%lx",start_addr);
        strcat(outbuf,tmpstr);
    }
    else if (type == 1)
    {
      end_addr = (ib_tmpval >> 8) & 0xffff;
      delaycnt = (ib_tmpval & 0xff);
      sprintf(tmpstr,"end addr: 0x%lx, delaycnt: %d",end_addr,delaycnt);
      strcat(outbuf,tmpstr);
    }
    else if (type == 2)
    {
      loopcnt = (ib_tmpval >> 16) & 0xff;
      ampscaler = ib_tmpval & 0xffff;
      sprintf(tmpstr,"loopcnt: %d, scaler: %d",loopcnt,ampscaler);
      strcat(outbuf,tmpstr);
    }
    else if (type == 4)
    {
      /* wait for HSline trigger */
      strcat(outbuf,"Wait 4 HS Trigger");
    }
    else if (type == 3)
    {
      delaycnt = ib_tmpval & 0xfffffff;
      sprintf(tmpstr,"delaycnt: %d (%d ns)",delaycnt,delaycnt*50);
      strcat(outbuf,tmpstr);
    }
    else if (type == 5)
    {
      pattimecnt = ib_tmpval & 0xfffffff;
      sprintf(tmpstr,"pattern time: %d (%d ns)",pattimecnt,pattimecnt*50);
      strcat(outbuf,tmpstr);
    }
    else if (type == 7)
    {
      /* end of sequence */
      strcat(outbuf,"End of Sequence");
    }
}    

static int rfwfg_msb = 0;
static unsigned long wfgtmpval;

decodePat(char *outbuf, unsigned long data)
{
    char tmpstr[80];
    int duration,phase,phaseq,amp;
    int index;
    long addr;
    float rphase;
 
    if (rfwfg_msb == 0)
    {  
      wfgtmpval = data & 0xffff;
      rfwfg_msb = 1;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
      return(0);
    }
    else
    {
      wfgtmpval |= (data & 0xffff) << 16;
      rfwfg_msb = 0;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
    }
 
    /* printf("\n >>>>>> wfgtmpval: 0x%lx\n",wfgtmpval); */
    duration = wfgtmpval & 0xff;
    amp = (wfgtmpval >> 8) & 0x3ff;
    phase = (wfgtmpval >> 18) & 0x7ff;
    phaseq = 0;
    if (phase & 0x400) 
       phaseq = 180;
    if (phase & 0x200)
	phaseq += 90;
    phase &= 0x1ff;
    rphase = (float) phase * 0.25;
    rphase += phaseq;
    sprintf(tmpstr,"amp: %d, phase: %6.2f, time: %d",amp,rphase,duration);
    strcat(outbuf,tmpstr);
}


/*****************************************************************************
* pfg1
* BASE_APADDR_X       =  0x0c60;
* BASE_APADDR_Y       =  0x0c64;
* BASE_APADDR_Z       =  0x0c68;
* BASE_APADDR_R       =  0x0c6c;
* 
* AMP_ADDR_REG        =  0x0;
* AMP_VALUE_REG       =  0x0;
* AMP_RESET_REG       =  0x2;
*
************************************************************************/
pfg1(char *outbuf,int reg_num,unsigned long data)
{
    if (reg_num == 0x60)
	strcpy(tmpstr," Performa 1, X Axis Value");
    else if (reg_num == 0x62)
	strcpy(tmpstr," Performa 1, X Axis Reset");
    else if (reg_num == 0x64)
	strcpy(tmpstr," Performa 1, Y Axis Value");
    else if (reg_num == 0x66)
	strcpy(tmpstr," Performa 1, Y Axis Reset");
    else if (reg_num == 0x68)
	strcpy(tmpstr," Performa 1, Z Axis Value");
    else if (reg_num == 0x6a)
	strcpy(tmpstr," Performa 1, Z Axis Reset");
    else if (reg_num == 0x6c)
	strcpy(tmpstr," Performa 1, R Axis Value");
    else if (reg_num == 0x6e)
	strcpy(tmpstr," Performa 1, R Axis Reset");
    strcpy(outbuf,tmpstr);
    charPtr += strlen(tmpstr);
}

/***********************************************************************
* Performa 2 Controller 
*
*               REFERENCE DATA
*
*       ap buss address C50-C53,C54-C57,C58-C5b,C5c-C5f
*                         x       y       z    reserved
*
*       register 0
*               bit 0 - bit 2 address of Highland function
*               bit 3 - set to clear current dac to zero
*       register 1
*               all bits data to dac's
*               successive writes. order
*               9cZA, bcBC,bcDE,bcxx => ZABCDE (ABCDE <- setpoint dac)
*               bcxx causes strobes
*
*       register 2
*               bit 0   enable power stage
*               bit 1   reset?
*
*       register 3      status register
*               bit 0   highland ok ^
*
*       APSEL   0xA000   APWRT 0xB000   APWRTI 0x9000
************************************************************************/
pfg2(char *outbuf,int reg_num, unsigned long data)
{
    if ((reg_num >= 0x50) && (reg_num <= 0x53))
       strcpy(tmpstr," Performa 2, X Axis");
    if ((reg_num >= 0x54) && (reg_num <= 0x57))
       strcpy(tmpstr," Performa 2, Y Axis");
    if ((reg_num >= 0x58) && (reg_num <= 0x5b))
       strcpy(tmpstr," Performa 2, Z Axis");
    strcpy(outbuf,tmpstr);
    charPtr += strlen(tmpstr);
}




/***********************************************************
*
*Triax
*
*     0x0c88         DAC data low byte
*     0x0c89         DAC data high byte
*     0x0c8a         DAC control and update 
*     0x0c8b         Amp enable and reset 
*
*   BASE_APADDR_X       =  0x0c88;
*   BASE_APADDR_Y       =  0x0c88;
*   BASE_APADDR_Z       =  0x0c88;
*   BASE_APADDR_R       =  0x0c88;
*
*   VALUE_SELECT_X        =  0x20;
*   VALUE_SELECT_Y        =  0x48;
*   VALUE_SELECT_Z        =  0x90;
*
*   CNTRL_ENABLE_X        =  0x100;
*   CNTRL_ENABLE_Y        =  0x200;
*   CNTRL_ENABLE_Z        =  0x400;
* 
*   AMP_ADDR_REG        =  0x0;
*   AMP_VALUE_REG       =  0x0;
*   AMP_RESET_REG       =  0x2;
* 
*   AMP_RESET_VALUE       =  0x4000;
*
*        case 'x': case 'X':
*            apbaseaddr = BASE_APADDR_X;
*            gradcntrlmask = CNTRL_ENABLE_X;
*            gradvalueaddr = VALUE_SELECT_X;
*            naxis = 0;
*      apreg then select then value
*
*/
triax(char *outbuf,int reg_num, unsigned long data)
{
    int dacadr,chanselect,chanupdate;
    if (reg_num == 0x88)
         sprintf(tmpstr," Triax - DAC data low byte, 0x%lx",data&0xff);
    else if (reg_num == 0x89)
         sprintf(tmpstr," Triax - DAC data high byte, 0x%lx",data&0xff);
    else if (reg_num == 0x8a)
    {
	 dacadr = (data & 0x7);

         sprintf(tmpstr," Triax - DAC Control DACaddr: %d,  ",dacadr);
         
	 
         chanselect =  (data >> 3) & 0x3;
         chanupdate =  (data >> 5) & 0x7;
         
	 if (chanselect == 0)
	   strcat(tmpstr,"X selected,");
	 if (chanselect & 0x1)
	   strcat(tmpstr,"Y selected,");
	 if (chanselect & 0x2)
	   strcat(tmpstr,"Z selected,");

	 if (data & 0x1)
	   strcat(tmpstr,"X updated,");
	 if (data & 0x2)
	   strcat(tmpstr,"Y updated,");
	 if (data & 0x4)
	   strcat(tmpstr,"Z updated,");
    }
    else if (reg_num == 0x8b)
    {
         sprintf(tmpstr," Triax - Amp ");
	 if (data & 0x1)
	   strcat(tmpstr,"X enable,");
	 if (data & 0x2)
	   strcat(tmpstr,"Y enable,");
	 if (data & 0x4)
	   strcat(tmpstr,"Z enable,");
	 if (data & 0x80)
	   strcat(tmpstr,"Reset,");
    }

    strcpy(outbuf,tmpstr);
    charPtr += strlen(tmpstr);
}

lock_control(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k, p;
	double  ff;

	switch (reg_num) {
	 case  0x50:   /* lk phase  */
		lkphase = (double) bin2dec(29, 8);
		lkphase = lkphase * 360 / 255;
		sprintf(tmpstr, " set lock phase to %g degree ", lkphase);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x51:  /* lk duty, rate  */
		lkduty = bin2dec(33, 4);
		lkrate = bin2dec(31, 2);
		lkpulse = bin2dec(29, 2);
		sprintf(tmpstr, " set lock duty cycle to %d%%, ", lkduty);
		if (lkpulse == 0) /* lk rate depends on bits 4, 5 */
		{
		    if (lkrate == 0)
			strcat(tmpstr, "lock rate 2 KHz ");
		    else if (lkrate == 1)
			strcat(tmpstr, "lock rate 20 Hz ");
		    else if (lkrate == 2)
			strcat(tmpstr, "lock rate 1 Hz ");
		    else
			strcat(tmpstr, "lock rate triggered by HSline ");
		}
		else
		{
		    if (lkpulse == 1)
			strcat(tmpstr, "lock x-on / r-off ");
		    else if (lkpulse == 2)
			strcat(tmpstr, "lock x-off / r-on ");
		    else
			strcat(tmpstr, "lock x-on / r-on  ");
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x52:  /* lk power */
	 case  0x53:  /* rcvr gain */
		p = bin2dec(29, 8);
		k = 0;
		while (k < 50)
		{
		    if (lk_power_table[k] >= p)
			break;
		    k++;
		}
		if (reg_num == 0x52)
		{
		    lkpower = k;
		    sprintf(tmpstr, " set lock power to %d dB ", lkpower);
		}
		else
		{
		    rcvrpower = k;
		    sprintf(tmpstr, " set receiver gain to %d dB ", rcvrpower);
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x54:  /* lk freq */
		strcpy(outbuf, "   lock frequency \(byte 1\) ");
		charPtr += 27;
		break;
	 case  0x55:
		strcpy(outbuf, "   lock frequency \(byte 2\) ");
		charPtr += 27;
		break;
	 case  0x56:
		lkfreq = lkfreq * 40000000 / TWO_24;
		ff = lkfreq;
		k = 0;
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (k == 0)
		   sprintf(tmpstr, " set lock frequency to %g Hz ", ff);
		else if (k == 1)
		   sprintf(tmpstr, " set lock frequency to %g KHz ", ff);
		else
		   sprintf(tmpstr, " set lock frequency to %g MHz ", ff);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x57:
		strcpy(tmpstr,"lock controller status reg.");
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
                break;
		
	}

}


magnet_leg_interface(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k, k2, x;

	switch (reg_num) {
	 case  0x48:   /* preamp */
		k = bin2dec(29, 8);
		if (k == 1)
		    sprintf(tmpstr, " xmtr/rcvr preamp will follow ATG1 ");
		else if (k == 2)
		    sprintf(tmpstr, " xmtr/rcvr preamp will follow ATG2 ");
		else
		{
		    sprintf(tmpstr, "? Error: bad number 0x%X for preamp gate ", k);
		    errorNum++;
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x49:   /* relay */
		if (bfifo[36] == '0')
		    sprintf(tmpstr, " relay select high band, ");
		else
		    sprintf(tmpstr, " relay select low band, ");
		if (bfifo[35] == '0')
		    strcat(tmpstr, "mixer is high band, ");
		else
		    strcat(tmpstr, "mixer is low band, ");
		if (bfifo[34] == '0')
		    strcat(tmpstr, "high band from chan 3, ");
		else
		    strcat(tmpstr, "low band from chan 3, ");
		if (bfifo[33] == '0')
		    strcat(tmpstr, "high band from chan 4, ");
		else
		    strcat(tmpstr, "low band from chan 4, ");
		if (bfifo[32] == '0')
		    strcat(tmpstr, "LO from chan 1 ");
		else
		    strcat(tmpstr, "LO from chan 2 ");
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	
	 case  0x4A:   /* preamp */
		obs_preamp = 0;
		if (bfifo[35] == '1')
		    obs_preamp = 12;
		if (bfifo[36] == '1')
		    obs_preamp += 12;
		sprintf(outbuf, " set obs preamp attn to %2d dB ", obs_preamp);
		charPtr += 30;
		break;
	 case  0x4D:   /* solids amp */
		k = bin2dec(29, 4);
		k2 = bin2dec(33, 4);
		if ((k != 14 && k != 2) || (k2 != 14 && k2 != 2))
		{
		    sprintf(outbuf, "? Error:");
		    charPtr += 8;
		    outbuf += 8;
		}
		if (k == 14)  /* 0xE */
		    sprintf(tmpstr, " set solids amp low band to CW mode, ");
		else if (k == 2)
		    sprintf(tmpstr, " set solids amp low band to PULSE mode, ");
		else
		{
		    sprintf(tmpstr, " bad number 0x%X for low band, ",k);
		    errorNum++;
		}

		if (k2 == 14)
		    strcat(tmpstr, "high band to CW mode ");
		else if (k2 == 2)
		    strcat(tmpstr, "high band to PULSE mode ");
		else
		{
		    strcpy(outbuf, tmpstr);
		    x = strlen(tmpstr);
		    charPtr += x;
		    outbuf += x;
		    sprintf(tmpstr, "bad number 0x%X for high band ", k);
		    errorNum++;
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x4E:   /* user bits  */
		k = bin2dec(29, 8);
		sprintf(outbuf, " set user bits to 0x%2X  ", k);
		charPtr += 23;
		break;
	}
}


rcvr_interface(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k;
	double  ff;

	switch (reg_num) {
	 case  0x40:   /* audio filter */
		rcvr_filter = 200 * bin2dec(29, 8) + 200;
		ff = (double) rcvr_filter;
		k = 0;
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		sprintf(outbuf, " set rcvr filter to ");
		charPtr += 20;
		outbuf += 20;
		if (k == 0)
		   sprintf(tmpstr, "%g Hz ",ff);
		else if (k == 1)
		   sprintf(tmpstr, "%g KHz ",ff);
		else
		   sprintf(tmpstr, "%g MHz ",ff);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x42:   /* IF attenuator */
		rcvr_amp = 0;
		if (bfifo[31] == '1')
		   rcvr_amp = 2;
		if (bfifo[32] == '1')
		   rcvr_amp += 4;
		if (bfifo[33] == '1')
		   rcvr_amp += 6;
		if (bfifo[34] == '1')
		   rcvr_amp += 12;
		if (bfifo[35] == '1')
		   rcvr_amp += 12;
		rcvr_amp += obs_preamp;
		sprintf(outbuf, " set IF attn to %2d dB ", rcvr_amp);
		charPtr += 22;
		break;
	}
}

amp_route(outbuf, reg_num)
char  	*outbuf;
int	reg_num;
{
	int	k, chan;

	if (reg_num <= 0x33 || (reg_num >= 0x38 && reg_num <= 0x3B))
	{
	    switch (reg_num) {
	 	case  0x30:   /* channel 4 power */
			chan = 4;
			break;
	 	case  0x31:   /* channel 3 power */
			chan = 3;
			break;
	 	case  0x32:   /* channel 2 power */
	 	case  0x3A:
			chan = 2;
			break;
	 	case  0x33:   /* channel 1 power */
	 	case  0x3B:
			chan = 1;
			break;
	 	case  0x38:   /* channel 6 power */
			chan = 6;
			break;
	 	case  0x39:   /* channel 5 power */
			chan = 5;
			break;
		}
		k = bin2dec(29, 8);
                if (bfifo[36] == '1')
                {
                    sprintf(outbuf, "? Error: bad number 0x%2X for coarse dB ", k);
                    charPtr += 39;
		    errorNum++;
                    return;
                }
		k = k / 2;
		sprintf(outbuf, " set channel %d coarse attn to %2d dB ", chan, k);
		charPtr +=  36;
		xmtr_info[chan].coarse_attn = k;
		return;
	}

	switch (reg_num) {
	 case  0x34:
	 case  0x3C:
		set_amp_brick(outbuf, 1);
		break;
	 case  0x35:
		set_amp_brick(outbuf, 3);
		break;
	 case  0x3D:
		set_amp_brick(outbuf, 5);
		break;
	 case  0x36:
		k = bin2dec(29, 8);
		sprintf(outbuf, " set register 0x36 to 0x%2X ", k);
		charPtr +=  27;
		break;
	}
	

}

set_amp_brick(outbuf, chan)
char  	*outbuf;
int     chan;
{
	int   k;

	k = bin2dec(33, 4);
	if (k == 0)
	    sprintf(tmpstr, " turn off amp brick %d,", chan);
	else if (k >= 8)
	    sprintf(tmpstr, " set amp brick %d to CW mode,", chan);
	else
	    sprintf(tmpstr, " set amp brick %d to PULSE mode,", chan);
	k = strlen(tmpstr);
	strcpy(outbuf, tmpstr);
	charPtr += k;
	outbuf += k;
	k = bin2dec(29, 4);
	if (k == 0)
	    sprintf(tmpstr, " turn off amp brick %d ", chan+1);
	else if (k >= 8)
	    sprintf(tmpstr, " set amp brick %d to CW mode ", chan+1);
	else
	    sprintf(tmpstr, " set amp brick %d to PULSE mode ", chan+1);
	strcpy(outbuf, tmpstr);
	charPtr += strlen(tmpstr);
}



int  convert_fifo()
{
	char   tmp_file[128];
	int     n, line_len, count, tt, val;
        unsigned int   rdata;
        unsigned char  cdata, vbyte;
	FILE    *fout, *fin;


	sprintf(tmp_file, "%s", tempnam("/tmp", "fifo"));
        if ((fin = fopen(infile, "r")) == NULL)
	{
	      if (!topShell)
	         perror(infile);
              return(-1);
	}
        if ((fout = fopen(tmp_file, "w")) == NULL)
              return(-1);
	line_len = fifoWord / 8;
	n = 0;
	while ((rdata = fgetc(fin)) != EOF)
        {
           cdata = rdata;
	   vbyte = 0x80;
	   count = 0;
	   while (count < 2)
	   {
		val = 0;
		for(tt = 0; tt < 4; tt++)
		{
		    if (vbyte & cdata)
			val = val * 2 + 1;
		    else
			val = val * 2;
		    vbyte = vbyte >> 1;
		}
		fprintf(fout, "%1x", val);
		count++;
	   }
           n++;
           if (n == line_len)
           {
		fprintf(fout, "\n");
                n = 0;
           }
        }
        fclose(fout);
        fclose(fin);
	strcpy(infile, tmp_file);
	return(1);
}


