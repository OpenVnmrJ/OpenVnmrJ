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

#include <stdio.h>

#ifdef X11
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#ifdef MOTIF
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Protocols.h>

#else
#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/OblongButt.h>
#include <Xol/StaticText.h>

#endif

#define YOFF 20
#define XOFF 20

typedef struct _butinfo {
	char   *label;
	char   *ret_val;
	struct _butinfo *next;
      } BUTINFO;

BUTINFO  *info_start = NULL;

char  *prgName = "Vnmr2";
char  *className = "confirmer";
char  *geomp = "-geometry";
char  *nargv[32];
char  location[32];
char  *title;

int     nargc;
Arg     args[20];
Widget  topShell;
Widget  mainShell;
Widget  mainWin;
Widget  butWin;
Widget  messWin;
Pixel   fpix;
static  Widget  infoWidget = NULL;
Window  shellId;
Display *dpy;
Atom    deleteAtom;
EventMask emask;
XWindowAttributes but_attr;

static Widget  popupBut = NULL;
static Widget  menuPane = NULL;
static Window  popupId = 0;
static int  menu_up = 0;
	Colormap  cmap;

char  *fontp = "-xrm";
char  fontName[128];

void
confirm_proc(widget, c_data, call_data)
Widget    widget;
XtPointer c_data;
XtPointer call_data;
{
	BUTINFO *info;

	info = (BUTINFO *) c_data;
	if (info == NULL)
	    printf("0\n");
	else
	    printf("%s\n", info->ret_val);
	XCloseDisplay(dpy);
	exit(0);
}


void
resize_shell(w, client_data, xev)
  Widget          w;
  XtPointer       client_data;
  XEvent          *xev;
{
	int	menu_unmapped;

        if (xev->type == VisibilityNotify)
        {
	    menu_unmapped = 1;
	    if (popupBut)
	    {
		if (popupId <= 0)
		   popupId = XtWindow(popupBut);
		if (popupId > 0)
		{
		   if (XGetWindowAttributes(dpy, popupId, &but_attr))
		   {
			if (but_attr.map_state == IsViewable)
		    	    menu_unmapped = 0;
		   }
		}
	    }
	    if (menu_unmapped)
		XRaiseWindow(dpy, shellId);
        }
        else if (xev->type == UnmapNotify)
		XtMapWidget(topShell);
}

#ifdef OLIT
static void
shell_exit( w, client_data, event )
Widget w;
char *client_data;
void *event;
{
        OlWMProtocolVerify      *olwmpv;

        olwmpv = (OlWMProtocolVerify *) event;
        if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
	  printf("0\n");
	  XCloseDisplay(dpy);
	  exit(0);
        }
}


#else

XmString mstring;

void
shell_exit()
{
	printf("0\n");
	XCloseDisplay(dpy);
	exit(0);
}

#endif

main(argc, argv)
int	argc;
char    **argv;
{
	int	  n, x, y, node_num, set_font;
	BUTINFO   *info_node, *new_node;
	XWindowChanges newdim;


	if (argc < 4)
	{
	    printf("0\n");
	    exit(0);
	}
	x = 250;
	y = 400;
	title = NULL;
	node_num = 0;
	set_font = 0;
	for(n = 1; n < argc; n++)
	{
	    if (argv[n][0] == '-')
	    {
		if (n+1 < argc && argv[n+1] != NULL)
		{
		    if (argv[n][1] == 'x')
		       	    x = atoi(argv[n+1]);
		    else if (argv[n][1] == 'y')
		    {
		            y = atoi(argv[n+1]);
		    }
		    else if (strcmp(argv[n], "-fn") == 0)
		    {
		       set_font = 1;
		       sprintf(fontName, "*fontList:%s", argv[n+1]);
		    }
		}
		n++;
	    }
	    else if (title == NULL)
		title = argv[n];
	    else
	    {
		new_node = (BUTINFO *) malloc(sizeof(BUTINFO));
		if (new_node == NULL)
		{
		    printf("0\n");
                    exit(0);
		}
		new_node->next = NULL;
		new_node->label = (char *) malloc(strlen(argv[n]) + 2);
		strcpy(new_node->label, argv[n]);
		n++;
		if (argv[n] != NULL)
		{
		   new_node->ret_val = (char *) malloc(strlen(argv[n]) + 2);
		   strcpy(new_node->ret_val, argv[n]);
		}
		else
		{
		   new_node->ret_val = (char *) malloc(4);
		   strcpy(new_node->ret_val, "0");
		}
		if (info_start == NULL)
		   info_start = new_node;
		else
		   info_node->next = new_node;
		info_node = new_node;
		node_num++;
	    }
	}
	sprintf(location, "+%d+%d", x, y);
	nargv[0] = prgName;
	nargv[1] = geomp;
	nargv[2] = location;
	nargc = 3;
	for(n = 1; n < argc; n++)
	{
	     if (nargc > 28)
		break;
	     nargv[nargc++] = argv[n];
	}
	if (set_font)
	{
	    nargv[nargc++] = fontp;
	    nargv[nargc++] = fontName;
	}
	nargv[nargc] = NULL;

	mainShell = XtInitialize("Vnmr2","Vnmr2",
                        NULL, 0, (Cardinal *)&nargc, nargv);
	dpy = XtDisplay(mainShell);

	cmap = XDefaultColormap(dpy, XDefaultScreen(dpy));
       
	n = 0;
	XtSetArg (args[n], XtNtitle, "Confirmer");  n++;
	XtSetValues (mainShell, args, n);

	n = 0;
        XtSetArg (args[n], XtNx, x);  n++;
        XtSetArg (args[n], XtNy, y);  n++;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
#ifdef MOTIF
        XtSetArg (args[n], XmNborderWidth, 3);  n++;
#else
        XtSetArg (args[n], XtNborderWidth, 3);  n++;
#endif
	topShell = XtCreatePopupShell("", overrideShellWidgetClass, mainShell,
                args, n);

	if (node_num > 2)
	    create_menu_window();
	else
	    create_button_window();


/**
	emask = SubstructureNotifyMask | VisibilityChangeMask;
	XtAddEventHandler(topShell, emask, False, resize_shell, NULL);
#ifdef  OLIT
        OlAddCallback( topShell, XtNwmProtocol, shell_exit, NULL );
#else
        deleteAtom = XmInternAtom(dpy, "WM_DELETE_WINDOW", FALSE);
        XmAddProtocolCallback(topShell, XM_WM_PROTOCOL_ATOM(topShell),
                        deleteAtom, shell_exit, 0);
#endif
**/

#ifdef MOTIF
	XtVaSetValues(topShell, XmNborderColor, fpix, NULL);
#endif
	XtRealizeWidget(topShell);
	shellId = XtWindow(topShell);
	newdim.x = x;
	newdim.y = y;
	XConfigureWindow(dpy, shellId, CWX | CWY, &newdim);
	XtPopup(topShell, XtGrabNone);
	XtMainLoop();
}


create_button_window()
{
	int	 n, bnum;
	BUTINFO  *info_node;
	char     *mptr;
	Widget   button;
	Dimension	w, h, tw, bw;

#ifdef OLIT
	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	XtSetArg (args[n], XtNvPad, YOFF);  n++;
	XtSetArg (args[n], XtNhPad, XOFF);  n++;
	XtSetArg (args[n], XtNvSpace, 20);  n++;
	mainWin =(Widget) (Widget)XtCreateManagedWidget(className,
                        controlAreaWidgetClass, topShell, args, n);
	n = 0;
	XtSetArg (args[n], XtNstring, title);  n++;
        XtSetArg (args[n], XtNgravity, WestGravity);  n++;
        XtSetArg (args[n], XtNstrip, FALSE);  n++;
        XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
        messWin = (Widget)XtCreateManagedWidget("label", staticTextWidgetClass,
                        mainWin, args, n);
	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	XtSetArg (args[n], XtNhPad, 30);  n++;
	XtSetArg (args[n], XtNhSpace, 30);  n++;
	butWin = (Widget)XtCreateManagedWidget("",
                        controlAreaWidgetClass, mainWin, args, n);
	info_node = info_start;
	while (info_node != NULL)
	{
	    n = 0;
	    XtSetArg (args[n], XtNlabel, info_node->label); n++;
	    button = (Widget)XtCreateManagedWidget("",
                        oblongButtonWidgetClass, butWin, args, n);
            XtAddCallback(button, XtNselect, confirm_proc, (XtPointer) info_node);
	    info_node = info_node->next;
	}

#else
	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	XtSetArg(args[n], XmNmarginHeight, YOFF);  n++;
	XtSetArg(args[n], XmNmarginWidth, XOFF);  n++;
        mainWin = (Widget) XmCreateRowColumn(topShell, className, args, n);
        XtManageChild (mainWin);

	tw = 0;
	mptr = title;
	for (;;)
	{
	   while (*mptr != '\0')
	   {
		if (*mptr == '\\' && *(mptr+1) == 'n')
		{
		    *mptr = '\0';
		    mptr += 2;
		    break;
		}
		mptr++;
	   }
	   if (*title == '\0')
		break;
	   

	   n =0;
           mstring = XmStringLtoRCreate(title, XmSTRING_DEFAULT_CHARSET);
           XtSetArg (args[n], XmNlabelString, mstring);  n++;
           XtSetArg (args[n], XmNalignment, XmALIGNMENT_CENTER);  n++;
           messWin = (Widget)XmCreateLabel(mainWin, "label", args, n);
           XtManageChild (messWin);
/*
           XtFree(mstring);
*/
           XmStringFree(mstring);
	   title = mptr;
	   XtVaGetValues(messWin, XmNwidth, &w, NULL);
	   if (w > tw)
		tw = w;
	}
	XtVaGetValues(messWin, XmNforeground, &fpix, NULL);

	n =0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	XtSetArg (args[n], XmNmarginHeight, 10);  n++;
	XtSetArg (args[n], XmNmarginWidth, 30);  n++;
	XtSetArg (args[n], XmNspacing, 30);  n++;
        butWin = (Widget) XmCreateRowColumn(mainWin, "", args, n);
        XtManageChild (butWin);

	info_node = info_start;
	bw = 0;
	bnum = 0;
	while (info_node != NULL)
	{
	   n = 0;
	   mstring = XmStringLtoRCreate(info_node->label, XmSTRING_DEFAULT_CHARSET);
	   XtSetArg (args[n], XmNlabelString, mstring);  n++;
/*
	   XtSetArg (args[n], XmNdefaultButtonShadowThickness, 1);  n++;
*/
	   button = (Widget) XmCreatePushButton(butWin, "", args, n);
           XtManageChild (button);
           XtAddCallback(button, XmNactivateCallback, confirm_proc, (XtPointer)info_node);
/*
           XtFree(mstring);
*/
           XmStringFree(mstring);
	   info_node = info_node->next;
	   XtVaGetValues(button, XmNwidth, &w, NULL);
	   bw += w;
	   bnum++;
	}
	n = ((int)tw - (int)bw - 30 * (bnum -1)) / 2;
	if (n > 0)
	   XtVaSetValues(butWin, XmNmarginWidth, (Dimension) n, NULL);

#endif
}


#ifdef OLIT
create_menu_window()
{
	int	 n, yoffset;
	Widget   formWidget, menuBut;
	Widget   twidget;
	BUTINFO  *info_node;

	n = 0;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
	formWidget = (Widget)XtCreateManagedWidget(className, formWidgetClass,
                        topShell, args, n);
	yoffset = YOFF * 2;
	n = 0;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
        XtSetArg(args[n], XtNyOffset, yoffset); n++;
        XtSetArg(args[n], XtNyAttachOffset, yoffset); n++;
        XtSetArg(args[n], XtNxOffset, XOFF); n++;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg (args[n], XtNstring, title); n++;
        twidget = (Widget)XtCreateManagedWidget("", staticTextWidgetClass,
                        formWidget, args, n);
        n = 0;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
        XtSetArg(args[n], XtNyOffset, yoffset); n++;
        XtSetArg(args[n], XtNyAttachOffset, yoffset); n++;
        XtSetArg (args[n], XtNpushpin, (XtArgVal)OL_NONE); n++;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg(args[n], XtNxRefWidget, twidget); n++;
        XtSetArg(args[n], XtNxOffset, XOFF); n++;
        menuBut = (Widget)XtCreateManagedWidget("",
                        abbrevMenuButtonWidgetClass, formWidget, args, n);
	n = 0;
        XtSetArg (args[n], XtNmenuPane, (XtArgVal)&menuPane); n++;
        XtGetValues(menuBut, args, n);

	info_node = info_start;
	while (info_node != NULL)
	{
	    n = 0;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
	    XtSetArg (args[n], XtNlabel, info_node->label); n++;
	    popupBut = (Widget)XtCreateManagedWidget("",
                        oblongButtonWidgetClass, menuPane, args, n);
            XtAddCallback(popupBut, XtNselect, confirm_proc, (XtPointer) info_node);
	    info_node = info_node->next;

	}
	n = 0;
	XtSetArg (args[n], XtNcolormap, cmap); n++;
        XtSetArg(args[n], XtNyOffset, yoffset); n++;
        XtSetArg(args[n], XtNyAttachOffset, yoffset); n++;
        XtSetArg(args[n], XtNxAttachOffset, XOFF); n++;
	XtSetArg(args[n], XtNstring, info_start->label); n++;
        XtSetArg(args[n], XtNgravity, WestGravity); n++;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg(args[n], XtNxRefWidget, menuBut); n++;
        XtSetArg(args[n], XtNxOffset, 4); n++;
        XtSetArg(args[n], XtNxResizable, TRUE); n++;
        XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
        infoWidget = (Widget)XtCreateManagedWidget("label", staticTextWidgetClass,
                        formWidget, args, n);
}

#else

create_menu_window()
{
	int	 n;
	Widget   formWidget, menuBut;
	Widget   button, button_0;
	BUTINFO  *info_node;
	char	 *mptr;

	n = 0;
	XtSetArg(args[n], XmNverticalSpacing, YOFF);  n++;
	XtSetArg(args[n], XmNhorizontalSpacing, XOFF);  n++;
	XtSetArg(args[n], XmNborderWidth, 0);  n++;
	formWidget = (Widget)XtCreateManagedWidget(className, xmFormWidgetClass,
                         topShell, args, n);
	n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
	XtSetArg(args[n], XmNmarginHeight, YOFF);  n++;
	XtSetArg(args[n], XmNmarginWidth, XOFF);  n++;
        mainWin = (Widget) XmCreateRowColumn(formWidget, "", args, n);
        XtManageChild (mainWin);
	mptr = title;
	for (;;)
	{
	   while (*mptr != '\0')
	   {
		if (*mptr == '\\' && *(mptr+1) == 'n')
		{
		    *mptr = '\0';
		    mptr += 2;
		    break;
		}
		mptr++;
	   }
	   if (*title == '\0')
		break;
	   n =0;
           mstring = XmStringLtoRCreate(title, XmSTRING_DEFAULT_CHARSET);
           XtSetArg (args[n], XmNlabelString, mstring);  n++;
           XtSetArg (args[n], XmNalignment, XmALIGNMENT_BEGINNING);  n++;
           messWin = (Widget)XmCreateLabel(mainWin, "label", args, n);
           XtManageChild (messWin);
/*
           XtFree(mstring);
*/
           XmStringFree(mstring);
	   title = mptr;
	}
	XtVaGetValues(messWin, XmNforeground, &fpix, NULL);

        menuPane = (Widget)XmCreatePulldownMenu(formWidget, "", NULL, 0);
	info_node = info_start;
	while (info_node != NULL)
        {
	   n =0;
           mstring = XmStringCreate(info_node->label, XmSTRING_DEFAULT_CHARSET);
           XtSetArg(args[n], XmNlabelString, mstring); n++;
	   popupBut = (Widget) XmCreatePushButton(menuPane, "label", args, n);
           XtAddCallback(popupBut, XmNactivateCallback, confirm_proc, (XtPointer) info_node);
           XtManageChild(popupBut);
	   if (info_node == info_start)
		button_0 = popupBut;
	   info_node = info_node->next;
           XmStringFree(mstring);
        }
        n = 0;
        XtSetArg(args[n], XmNsubMenuId, menuPane); n++;
        XtSetArg(args[n], XmNmenuHistory, button_0); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftWidget, mainWin); n++;
        menuPane = (Widget) XmCreateOptionMenu(formWidget, "label", args, n);
        XtManageChild(menuPane);
}


#endif

#else

main(argc, argv)
int	argc;
char    **argv;
{
	printf("0\n");
}

#endif
