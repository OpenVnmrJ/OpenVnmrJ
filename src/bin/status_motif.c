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
/***************************************************************************
*    This procedure is used to create a window and display the ASM status. * 
*    It can run in the vnmr mode or in the unix shell. There are four      *
*    buttons in unix and five in vnmr. Each function is as follows:        * 
*        user -  change the user's name.                                   *
*        (ret/log) - display the done queue and enter queue or log file    *
*        delete - delete the directory which is being highlighted.         *
*        quit - destroy the status window and exit.                        *
*        rt (vnmr)- retrieve the FID.                                      * 
***************************************************************************/ 

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/MessageB.h>
#include <Xm/SeparatoG.h>

#define	DONEQ_OR_ENTERQ		1
#define DONEQ_AND_ENTERQ	0
#define NUM_LINES		8
#define BUFSIZE   		2048
#define WINWIDTH  		80
#define borderHeight   		5
#define RETURN          	13
#define DOWN            	1
#define UP			2

#define ALERT_STRINGS           1
#define ALERT_YES_BUTTON        2
#define ALERT_NO_BUTTON         3

Widget 	base, frame_loc;
Widget	button_panel, panel_loc, mess_panel, sum_panel;
Widget	showsw;
Widget  sample_item,user_item,macro_item,solvent_item,text_item,data_item;
Widget  err_mess, user, p1, locate_button, loc_don;          
Widget  S_TOTAL, S_NEW, S_ACT, S_COMP, USER_NO;
Widget  E_TOTAL, E_FREE, E_ACT, E_COMP;
char	*getwd();
char    *getlogin();
char	owner[20];
char    loginname[20];
char    dataPath[120];
char    infopath[120];
char    logfile[120];
char    doneq[120]; 
char    enterq[120];
char    cmd[240];
Pixel   red_pix, yel_pix;
static int	log, pipefd, infoAddr;
static int	vnmr;
static int	done_enter = 0;
static int	first_locate;	/* search for first or further entries? */
static int	point;		/* used for current insertion point     */
static int	save_point;	/* save location for locate		*/
static int	save_lineno;	/* save line num for locate		*/

Widget statusShell, statusFrame;
Widget userBut, p1But, loc_donBut, rtBut, deleteBut, quitBut;
Widget formWidget;

FILE   *fin, *enterf, *donef;
char   *search_token();
void    sample_select();

int    		screen, screenHeight;
int   		charWidth, charHeight;
int    		n, scrollup;
int    		margin;
int     	file_length;
int    		textHeight;
Arg             args[12];
char    	fontName[120];
char   		*file_string;
char   		*log_string;
Display  	*dpy;
XmString        xmstr;
XmFontList   	fontList;
XFontStruct     *xstruct;

XmTextPosition  h_pos1, h_pos2;
XmTextPosition  pos1, pos2;

static labelHeight = 15;

void set_user(), quit_proc(),  rt_proc();
void del_proc(), loc_disp(), ret_disp();
void log_disp();

main(argc, argv)
int	argc;
char	**argv;

{
	int  x;
	char label[120], current_dir[120];
        char *temp, *getenv();

        temp = getenv("graphics");
	if (temp == NULL)
	{
	    fprintf(stderr, "Error: the environment 'graphics' is NULL.\n");
            exit(0);
	}		
	if((strcmp("suncolor", temp) != 0) && (strcmp("sun", temp) != 0))
	{
            fprintf(stderr, "stopped(improper window system on terminal)\n");
            exit(0);
	}		
    	if (argc < 2 || argc > 6)
	{
 		fprintf(stderr, "usage: status path [-fn fontname] [-display Xterminal]\n");
		exit(0);
	}
	log = FALSE;
        temp = getlogin();
	sprintf(loginname, "%s", temp);
        getwd(current_dir);
	vnmr = 0;
	fontName[0] = '\0';
        for(n = 1; n < argc; n++)
        {
             if (strcmp("vnmr", argv[n]) == 0)
             {
		n++;
                strcpy(infopath, argv[n]);
		vnmr = 1;
		n++;
	        pipefd = atoi(argv[n]);
 	     }
             else if (!strcmp("-display", argv[n]))
             {
		n++;
	     }
	     else if (!strcmp("-fn", argv[n]))
	     {
		n++;
                if (argv[n] != NULL)
		   strcpy(fontName, argv[n]);
	     }
	     else
             {
                strcpy(infopath, argv[n]);
                if (infopath[0] == '-')
                {
                        temp = &infopath[1];
                        sprintf(infopath, "%s", temp);
                }
             }
        }   
        x = strlen(infopath);
        if (x <= 0)
        {
 		fprintf(stderr, "usage: status path [-fn fontname] [-display Xterminal]\n");
		exit(0);
	}
        if (infopath[x-1] != '/') { infopath[x] = '/'; infopath[x+1] = '\0'; }
	sprintf(label, "               STATUS         %s", infopath);
        strcpy(owner, loginname);
        create_mainframe(argc, argv);
	create_buttons();  
	create_mess();
	create_summary();
	create_locate();
	create_show();
        XtRealizeWidget (statusShell);
        XtMainLoop();
	exit(0);
}

create_buttons()
{
        Widget create_button_item();

        n =0;
        XtSetArg (args[n], XmNorientation, XmHORIZONTAL);  n++;
   	XtSetArg (args[n], XmNresizeHeight, True);  n++;
  	XtSetArg (args[n], XmNpacking, XmPACK_TIGHT);  n++;
  	XtSetArg (args[n], XmNmarginWidth, charWidth * 20);  n++;
  	XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER);  n++;
	XtSetArg (args[n], XmNspacing, 8);  n++;
        button_panel = XmCreateRowColumn(statusFrame, "button", args, n);
/*
        userBut = create_button_item("user", set_user);
*/
 	p1But = create_button_item("ret", ret_disp);
	loc_donBut = create_button_item("locate", loc_disp);

        if (vnmr)
        {
		rtBut = create_button_item(" rt ", rt_proc);
	}

	deleteBut = create_button_item("delete", del_proc);
	quitBut = create_button_item("quit", quit_proc);
        XtManageChild (button_panel);
}


create_mess()
{
        Widget tmpwidget;

        tmpwidget = XmCreateSeparatorGadget(statusFrame,"separator", 0, 0);
        XtManageChild (tmpwidget);
	n =0;
        XtSetArg (args[n], XmNorientation, XmHORIZONTAL);  n++;
        XtSetArg (args[n], XmNpacking, XmPACK_NONE);  n++;
        mess_panel = XmCreateRowColumn (statusFrame, "mess_panel", args, n);
        XtManageChild (mess_panel);
	n = 0;
        XtSetArg (args[n], XmNx, 0);  n++;
        xmstr = XmStringLtoRCreate("current user:", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
        tmpwidget = XmCreateLabel(mess_panel, "label", args, n);
        XtManageChild (tmpwidget);
        XtFree(xmstr);

        n = 0;
        XtSetArg (args[n], XmNwidth, charWidth * 30);  n++;
        XtSetArg (args[n], XmNmarginHeight, 1);  n++;
        XtSetArg (args[n], XmNx, charWidth * 14);  n++;
        XtSetArg (args[n], XmNshadowThickness, 0); n++;
        if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
	user = XmCreateText(mess_panel, "user", args, n); 
        XtManageChild (user);
        XmTextSetString(user, owner);
        XtSetSensitive(user, False);

	n = 0;
        xmstr = XmStringLtoRCreate(" ", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNx, charWidth * 46);  n++;
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
        err_mess = XmCreateLabel(mess_panel, "label", args, n);
        XtManageChild (err_mess);
        XtFree(xmstr);
 }



create_summary()
{
         Widget  tmpwidget;
         Widget  sumFrame;
         Widget  create_item();

        tmpwidget =  XmCreateSeparatorGadget(statusFrame,"separator", 0, 0);
        XtManageChild (tmpwidget);
        formWidget = XmCreateForm(statusFrame, "form", NULL, 0);
        XtManageChild(formWidget);
        n =0;
        XtSetArg (args[n], XmNpacking, XmPACK_NONE);  n++;
        XtSetArg (args[n], XmNheight, charHeight * 8);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
/*
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
*/
        sum_panel = XmCreateRowColumn (formWidget, "statusWindow", args, n);
        XtManageChild (sum_panel);
        margin = (charHeight * 2 + 46) / 4;

        tmpwidget = create_item("QUEUED", 15, 1);
        tmpwidget = create_item("ACTIVE", 24, 1);
        tmpwidget = create_item("COMPLETE", 33, 1);
        tmpwidget = create_item("TOTAL", 45, 1);
        tmpwidget = create_item("SAMPLES    :", 1, 2);
        tmpwidget = create_item("EXPERIMENTS:", 1, 3);
        tmpwidget = create_item("USERS      :", 1, 4);
        S_TOTAL = create_item(" ", 46, 2);
        S_NEW = create_item(" ", 17, 2);
        S_ACT = create_item(" ", 26, 2);
        S_COMP = create_item(" ", 37, 2);
        E_TOTAL = create_item("-", 49, 3);
        E_FREE = create_item("-", 20, 3);
        E_ACT = create_item(" ", 26, 3);
        E_COMP = create_item(" ", 37, 3);
        USER_NO = create_item(" ", 46, 4);
        tmpwidget = create_item("-", 20, 4);
        tmpwidget = create_item("-", 40, 4);
        tmpwidget = create_item("-", 29, 4);

        get_summary(); 
}


create_show()
{
	struct stat statbuf;
        void take();
	int  height;

	textHeight = screenHeight - charHeight * 10 - 100;
        textHeight = (textHeight / charHeight) * charHeight;

	n = 0;
        XtSetArg (args[n], XmNcolumns, WINWIDTH);  n++;
        XtSetArg (args[n], XmNheight, textHeight);  n++;
        XtSetArg (args[n], XmNmaxLength, BUFSIZE);  n++;
        XtSetArg (args[n], XmNscrollVertical, True);  n++;
        XtSetArg (args[n], XmNscrollHorizontal, False);  n++;
        XtSetArg (args[n], XmNeditMode, XmMULTI_LINE_EDIT);  n++;
        XtSetArg (args[n], XmNcursorPositionVisible, False);  n++;
        if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
        showsw = XmCreateScrolledText (statusFrame, "outputWindow", args, n);
        XtManageChild (showsw);

        XtSetSensitive(showsw, False);
        XtAddEventHandler(showsw,ButtonReleaseMask, False, take, NULL);
        XtSetSensitive(showsw, True);
        h_pos1 = 0;
	h_pos2 = 0;
        ret_disp();
}

create_locate()
{
        void locate_done_proc();
	void locateproc();
	void setnext();
	Widget  create_loc_item();
	Widget  create_text_item();

	n = 0;
	XtSetArg (args[n], XmNmappedWhenManaged, False); n++;
/*
        XtSetArg (args[n], XmNwidth, charWidth * WINWIDTH);  n++;
*/
    	XtSetArg (args[n], XmNresizeWidth, False);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        frame_loc = XmCreateRowColumn (formWidget, "statusWindow", args, n);
        XtManageChild(frame_loc);
        n =0;
        XtSetArg (args[n], XmNpacking, XmPACK_NONE);  n++;
   	XtSetArg (args[n], XmNresizeHeight, True);  n++;
        XtSetArg (args[n], XmNheight, charHeight * 6);  n++;
        panel_loc = XmCreateRowColumn(frame_loc, "button", args, n);
        create_loc_item(" SAMPLE#: ", 1, 1);
        create_loc_item("    USER: ", 1, 2);
        create_loc_item("   MACRO: ", 1, 3);
	create_loc_item(" SOLVENT: ", 1, 4);
	create_loc_item("    TEXT: ", 1, 5);
	create_loc_item("    DATA: ", 1, 6);
        n =0;
        xmstr = XmStringLtoRCreate("locate", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        XtSetArg (args[n], XmNx, charWidth * 50);  n++;
        XtSetArg (args[n], XmNy, 0); n++; 

	XtSetArg (args[n], XmNtraversalOn, FALSE);  n++;
        if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
        locate_button = XmCreatePushButton(panel_loc, "buttons", args, n);
        XtManageChild (locate_button);
        XtAddCallback(locate_button, XmNactivateCallback, locateproc, NULL);
        XtFree(xmstr);

	sample_item = create_text_item(30, 12, 1);
	user_item = create_text_item(36, 12, 2);
	macro_item = create_text_item(65, 12, 3);
	solvent_item = create_text_item(30, 12, 4);
	text_item = create_text_item(65, 12, 5);
	data_item = create_text_item(68, 12, 6);
        XtManageChild(panel_loc);
}


void locate_done_proc()
{
        clear_mess();
        n =0;
        xmstr = XmStringLtoRCreate("locate", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        XtSetValues(loc_donBut, args, 1);
        XtRemoveCallback(loc_donBut,XmNactivateCallback,locate_done_proc,NULL);
        XtAddCallback(loc_donBut, XmNactivateCallback, loc_disp, NULL);
        XtManageChild (loc_donBut);
	XtUnmapWidget(frame_loc);
        XtMapWidget(sum_panel);
        XtFree(xmstr);
	first_locate = TRUE;
}

void loc_disp()
{
        XWindowAttributes win_attributes;
	Window  win;
	XWindowChanges    newdim;
	int     win_x, win_y;

        clear_mess();
        n =0;
        xmstr = XmStringLtoRCreate(" done ", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        XtSetValues(loc_donBut, args, 1);
        XtRemoveCallback(loc_donBut, XmNactivateCallback, loc_disp, NULL);
        XtAddCallback(loc_donBut, XmNactivateCallback, locate_done_proc, NULL);
/*
        XtManageChild (loc_donBut);
*/
        XtFree(xmstr);

        XtUnmapWidget(sum_panel);
        XtMapWidget(frame_loc);
}



/*-------------------------------------------------------------
|  locateproc
+------------------------------------------------------------*/
void
locateproc(item,but_num,call_data)
Widget     item;
int        but_num;
caddr_t    call_data;
{
char	*samp, *user, *macr, *solv, *text, *data;
char    *token, *buffer;
int	found, previous;

   samp = XmTextGetString(sample_item);
   user = XmTextGetString(user_item);
   macr = XmTextGetString(macro_item);
   solv = XmTextGetString(solvent_item);
   text = XmTextGetString(text_item);
   data = XmTextGetString(data_item);
   while (*samp == ' ') samp++;    /* ignore leading spaces */
   while (*user == ' ') user++;
   while (*macr == ' ') macr++;
   while (*solv == ' ') solv++;
   while (*text == ' ') text++;
   while (*data == ' ') data++;
   clear_mess();
   if (*samp==0 && *user==0 && *macr==0 && *solv==0 && *text==0 && *data==0)
   { 
      set_mess("You must enter something");
      XmTextSetInsertionPosition(sample_item, 0);
      return;
   }

   found = FALSE;
   if (first_locate) point = 0;
   else              point = save_point;
   buffer = file_string + point;
   while (!found)
   {  
        previous = 0;
        if (*samp)
        {
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "SAMPLE#"))
		{
                     if ((token = search_token(buffer, " \n\t")) != NULL)
		     {
		         if ( !strcmp(samp, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		}
            }
            if (!found)
                break;
        }
        if (*user)
        {
            found = FALSE;
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "USER"))
		{
                     if ((token = search_token(buffer, " \t\n")) != NULL)
		     {
		         if ( !strcmp(user, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		     if (previous)
                         break;
		}
            }
            if (!found && token == NULL)
                break;
            else if (!found)
		continue;
        }
        if (*macr)
        {
            found = FALSE;
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "MACRO"))
		{
                     if ((token = search_token(buffer, " \t\n")) != NULL)
		     {
		         if ( !strcmp(macr, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		     if (previous)
                         break;
		}
            }
            if (!found && token == NULL)
                break;
            else if (!found)
		continue;
        }
        if (*solv)
        {
            found = FALSE;
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "SOLVENT"))
		{
                     if ((token = search_token(buffer, " \t\n")) != NULL)
		     {
		         if ( !strcmp(solv, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		     if (previous)
                         break;
		}
            }
            if (!found && token == NULL)
                break;
            else if (!found)
		continue;
        }
        if (*text)
        {
            found = FALSE;
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "TEXT"))
		{
                     if ((token = search_token(buffer, "\n")) != NULL)
		     {
			 while(*token == ' ')
				token++;
		         if ( !strcmp(text, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		     if (previous)
                         break;
		}
            }
            if (!found && token == NULL)
                break;
            else if (!found)
		continue;
        }
        if (*data)
        {
            found = FALSE;
            while ((token = search_token(buffer, ":")) != NULL) 
            {
                buffer = NULL;
                if (!strcmp(token, "DATA"))
		{
                     if ((token = search_token(buffer, " \t\n")) != NULL)
		     {
		         if ( !strcmp(data, token))
		         {
		              found = TRUE;
                              previous = 1;
		              break;
		         }
		     }
		     if (previous)
                         break;
		}
            }
            if (!found && token == NULL)
                break;
            else if (!found)
		continue;
        }
   }
   if (found)
   {  
      XmTextSetInsertionPosition(showsw, infoAddr);
      sample_select();
      save_point = (int)infoAddr; 
      first_locate = FALSE;
   }
   else
   { 
      set_mess("No (more) matching entry found");
      save_point = 0;
      first_locate = TRUE;
   }
}

void setnext()
{}

/*-------------------------------------------------------------
|  inittimer/0  set event timer for count down
|       pass function pointer to be activated with alarm 
/*-------------------------------------------------------------
|  inittimer/0  set event timer for count down
|       pass function pointer to be activated with alarm 

/****************************************************************************
*  To make sure that the left mouse button has been pressed and released.   *
*  If so, select the right sample.                                          *
****************************************************************************/
void
take(client, data, e)
Widget          client;
caddr_t         data;
XEvent          *e;
{
  	Position x, y;
  	char   *str;

  	clear_mess();
  	sample_select();
}


/*****************************************************************************
*   This function will pass the fid pathname thruogh pipe to its parent      *
*   processor.                                                               *
*****************************************************************************/
void
rt_proc()
{
        char  filepath[160],rtFile[170],*ptr;

        if (dataPath[0] == '\0')
	{
             set_mess("rt: unknown file");
             return;
	} 
        if (h_pos2 > 0)
           XmTextSetHighlight(showsw, h_pos1, h_pos2+1, XmHIGHLIGHT_NORMAL);
        pos1 = infoAddr;
        pos2 = infoAddr + strlen(dataPath);
        XmTextSetHighlight(showsw, pos1, pos2, XmHIGHLIGHT_SELECTED); 
	strcpy(filepath,infopath);
	ptr = strrchr(dataPath,'/'); 
        if (ptr==0L) ptr = &dataPath[0]; else ptr++;
	strcat(filepath,ptr);
	strcat(filepath,".fid");
        if (!search_file(filepath))
             return;
        sprintf(rtFile, "rt('%s')", filepath);
 	if (write(pipefd, rtFile, strlen(rtFile)) == -1)
		set_mess("ERROR: pipe can not write\n");
	else
		quit_proc();
}



/******************************************************************************
*    This function is used to select the sample and hightlight it.            *
******************************************************************************/

void
sample_select()
{
        XmTextPosition top, current;
        Position  top_x, top_y;
        Position  cur_x, cur_y;
        char   *token, *tokptr;
        int     find, index;

        top = XmTextGetTopCharacter(showsw);
        XmTextPosToXY(showsw, top, &top_x, &top_y);
        current = XmTextGetInsertionPosition(showsw);
        XmTextPosToXY(showsw, current, &cur_x, &cur_y);
        current = XmTextXYToPos(showsw, top_x, cur_y);
        find = 0;
        while(current >= top && !find)
        {
                index = (int) current;
                tokptr = file_string+index;
                if ((token = search_token(tokptr, ": \t\n")) != NULL) 
		{
                     if(strcmp("SAMPLE#",token) == 0)
                     {
                           find = 1;
                           break;
                      }
		}
                current--;
                if (current >= 0 && current >= top)
                {
                     XmTextPosToXY(showsw, current, &cur_x, &cur_y);
                     current = XmTextXYToPos(showsw, top_x, cur_y);
                }
                else
                     break;
        }
        if (!find)
        {
                scrollup = -8;
                if (!strcmp("STATUS", token))
			scrollup = -7;
		else if (!strcmp(token, "DATA"))
			scrollup = -6;
		else if (!strcmp(token, "USERDIR"))
			scrollup = -5;
		else if (!strcmp(token, "TEXT"))
			scrollup = -4;
		else if (!strcmp(token, "SOLVENT"))
			scrollup = -3;
		else if (!strcmp(token, "MACRO"))
			scrollup = -2;
		else if (!strcmp(token, "USER"))
			scrollup = -1;
                XmTextScroll(showsw, scrollup);
                current = XmTextGetTopCharacter(showsw);
                while(current >= 0)
                {
                     index = (int) current;
                     tokptr = file_string+index;
                     if ((token = search_token(tokptr, ": \t\n")) != NULL) 
		     {
                          if(strcmp("SAMPLE#", token) == 0)
                          {
                               find = 1;
                               break;
                          }
                     }
                     XmTextScroll(showsw, -1);
                     XmTextPosToXY(showsw, current, &cur_x, &cur_y);
                     current = XmTextXYToPos(showsw, top_x, cur_y);
                }
                if (!find)
                     return;
/*
                if(current > 1) 
                   XmTextScroll(showsw, -1);
*/
        }
        find = 0;
        tokptr = file_string+index;
        while (((token = search_token(tokptr, ": \t\n")) != NULL) && !find)
        {
                tokptr = NULL;
                if (strcmp(token, "DATA") == 0)
                {
                       find = 1;
                       if((token = search_token(tokptr, " \n\t")) != NULL)
                                sprintf(dataPath, "%s", token);
                        break;
                }
        }
        if (!find)
        {
                dataPath[0] = '\0';
                return;
        }
       if (h_pos2 > 0)
           XmTextSetHighlight(showsw, h_pos1, h_pos2+1, XmHIGHLIGHT_NORMAL);
        h_pos1 = current;
        while(*(file_string+index) != NULL)
        {
                if (*(file_string+index) == '-')
                {
                        if (strncmp(file_string+index, "-----", 5) == 0)
                        {
                                index--;
                                h_pos2 = (XmTextPosition)(index);
                                break;
                        }
                }
                index++;
        }
        if (h_pos2 > h_pos1)
        {
            XmTextPosToXY(showsw, h_pos1, &cur_x, &cur_y);
            n = (charHeight * 8 + cur_y - textHeight) / charHeight;
            if (n > 1) 
                   XmTextScroll(showsw, n);
            XmTextSetHighlight(showsw, h_pos1, h_pos2+1, XmHIGHLIGHT_SELECTED);
	    save_point = h_pos2 + 1;
        }
	else
	    h_pos2 = 0;
}



void
quit_proc()
{
        if (file_string)
             XtFree(file_string);
	if (log_string)
             XtFree(log_string);
        XtDestroyWidget(statusShell);
        exit(0);
}



void
ret_disp(item,but_num,call_data)
Widget     item;
int        but_num;
caddr_t    call_data;
{
	int  len_done, len_enter;
        void  log_disp();
	struct stat statbuf;

        sprintf(doneq, "%s/doneQ", infopath);
        sprintf(enterq, "%s/enterQ", infopath);
        len_done = len_enter = 0;
        if (file_string)
             XtFree(file_string);
        if (log_string)
	{
             XtFree(log_string);
	     log_string = NULL;
	}
	if (access(doneq, 4) == 0)
	{
             if ((donef = fopen(doneq, "r")) != NULL)
	     {
		if (stat(doneq, &statbuf) == 0)
                     len_done = statbuf.st_size;
	     }
             if (access(enterq, 4) == 0)
		     done_enter = DONEQ_AND_ENTERQ;
             else
		     done_enter = DONEQ_OR_ENTERQ;
	}
	else
        {
             if (access(enterq, 4) == 0)
		     done_enter = DONEQ_OR_ENTERQ;
             else
	     {
                     set_mess("ERROR:  not  file or directory");
                     return;
             }
	}
        if ((enterf = fopen(enterq, "r")) != NULL)
	{
		if (stat(enterq, &statbuf) == 0)
                     len_enter = statbuf.st_size;
	}
        file_length = len_done + len_enter;
        file_string = (char *) XtMalloc(file_length);
	if (donef)
	{
	     fread(file_string, sizeof(char), len_done, donef);
	     fclose(donef);
	}
	if (enterf)
	{
	     fread(file_string + len_done, sizeof(char), len_enter, enterf);
	     fclose(enterf);
	}

        XmTextSetString(showsw, file_string);

/*
        XtMapWidget(locate_button);
*/
	xmstr = XmStringLtoRCreate("log", XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(p1But, args, 1);
        XtFree(xmstr);
        XtRemoveCallback(p1But, XmNactivateCallback, ret_disp, NULL);
        XtAddCallback(p1But, XmNactivateCallback, log_disp, NULL);
        clear_mess();
        log = FALSE;
        dataPath[0] = '\0';
}


/***************************************************************************
*   display the log file                                                   *
***************************************************************************/
void
log_disp()
{
        char  *ptr;
	FILE  *logfd;
	struct stat statbuf;

        if ((dataPath[0] == '\0') || (infoAddr == NULL))
        {
               set_mess("Error: No data path name");
               return;
        }
        XmTextSetHighlight(showsw,h_pos1, h_pos2+1, XmHIGHLIGHT_NORMAL); 
        pos1 = infoAddr;
        pos2 = infoAddr + strlen(dataPath);
        XmTextSetHighlight(showsw,pos1, pos2, XmHIGHLIGHT_SELECTED); 
	strcpy(logfile,infopath);
	ptr = strrchr(dataPath,'/'); 
	if (ptr==0L) ptr = &dataPath[0]; else ptr++;
	strcat(logfile,ptr);
	strcat(logfile,".fid/log");
   	if(search_file(logfile) == 0)
		return;
/*
        XtUnmapWidget(locate_button);
*/
	xmstr = XmStringLtoRCreate("ret", XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(p1But, args, 1);
        XtFree(xmstr);
        XtRemoveCallback(p1But, XmNactivateCallback, log_disp, NULL);
        XtAddCallback(p1But, XmNactivateCallback, ret_disp, NULL);
	log = TRUE;
        if (log_string)
	     XtFree(log_string);
        if (file_string)
	{
	     XtFree(file_string);
             file_string = NULL;
	}
        if ((logfd = fopen(logfile, "r")) == NULL)
             return;
        if (stat(logfile, &statbuf) == 0)
             file_length = statbuf.st_size;
        else
             file_length = 100000;
        log_string = (char *) XtMalloc(file_length);
        fread(log_string, sizeof(char), file_length, logfd);
        XmTextSetString(showsw, log_string);
        fclose(logfd);
}



int 
search_file(file)
char *file;
{
	int	k;
        extern char     *sys_errlist[];

        clear_mess();
	k = access(file, 4);
	if (k < 0)
	{
	      sprintf(cmd, "can not access %s", file);
              set_mess(cmd);
	      return(0);
	}
	return(1);
}


	
void	disp_confirmer();
void
del_proc()
{
	char	buffer[50];
	int	answer, del_more, i, lcount;

        clear_mess();
        if (log)
		return;
        answer = 0;
	if ((dataPath[0] == '\0') || (infoAddr == NULL))
	{
		set_mess("Error: No data path name");
		return;
	}
        pos1 = infoAddr;
        pos2 = infoAddr + strlen(dataPath);
        XmTextSetHighlight(showsw,h_pos1, h_pos2 + 1, XmHIGHLIGHT_NORMAL); 
        XmTextSetHighlight(showsw,pos1, pos2, XmHIGHLIGHT_SELECTED); 
        answer = alert_prompt(  ALERT_STRINGS,
				"Data will be deleted.",
				"\n  Are you sure?", 
				NULL,
				ALERT_YES_BUTTON, "Yes",
                                ALERT_NO_BUTTON,  "No",
                                0 );
        if (answer == 0)
                return;
	delete();
}


/****************************************************************************
*    To delete the directory, which path name is in the done queue, selected*
*    by hitting the mouse on that sample.                                   * 
****************************************************************************/
delete()
{
	char  file[120], *ptr;
        struct stat status;
	struct passwd *pwentry;

	strcpy(file,infopath);
	ptr = strrchr(dataPath,'/');
	if (ptr==0) ptr=&dataPath[0]; else ptr++;
	strcat(file,ptr);
	strcat(file,".fid");
        if (access(file, 0) != 0)
        {
                sprintf(cmd, "couldn't find %s", file);
                set_mess(cmd);
 		return;
	}  
        if (stat(file, &status) == -1)
	{
                sprintf(cmd, "couldn't stat %s", file);
                set_mess(cmd);
                return;
	}
        if ((pwentry = getpwuid(status.st_uid)) == NULL)
        {
                set_mess("error: couldn't find owner");
		return;
	}
	if((strcmp("root", pwentry->pw_name) != 0) && (strcmp(loginname,pwentry->pw_name) != 0))
	{
                set_mess("Not owner, permission denied");
		return;
	}
        sprintf(cmd, "rm -rf %s", file);
	system(cmd);
}

void
set_user()
{
        XmTextPosition  lastpos;

        clear_mess();
        lastpos = XmTextGetLastPosition(user);
        XmTextReplace(user, 0, lastpos, "");  
        XmTextSetInsertionPosition(user, 0);
	XSetInputFocus(dpy, XtWindow(user), RevertToParent, CurrentTime);
        n = 0;
        XtSetArg (args[n], XmNcursorPositionVisible, True);  n++;
        XtSetValues(user, args, 1);
}


int
sindex (str, ch)
char	*str, *ch;
{
	int 	current, next, find;

	current = 0;
	next = 0;
	find = 0;
	while(str[next])
	{
	  	if (str[next] == *ch)
		{	
			find = 1;	
			current = next;
		}
		next++;
	}
	if (find)
		return(current);
	else
		return(-1);
}
	

string_cpy(str1, str2, from, to)
char	*str1, *str2;
int	*from, *to;
{
        int   index;

	index = 0;
	while (*from <= *to && str2[*from]) 
	{
		str1[index] = str2[*from];
                index++;
		*from = *from + 1;  
	}
}



struct  list {
                char  name[20];
                struct  list  *next;
        };


/***************************************************************************
*    To get the summary information and display them in the summary window.*
***************************************************************************/
get_summary()
{
 	int s_queue, s_new, s_total, s_act, s_comp;
	int e_queue, e_act, e_comp, e_total;
        int users, loc;
        FILE *fd;
        char buf[80], *token, *strptr;
        char label[5], sample[100];
        struct   list    *user_list;
        char  dir[50];

        getwd(dir);
        sprintf(enterq, "%s/enterQ", infopath);
        sprintf(doneq, "%s/doneQ", infopath);
        s_queue = s_act = s_comp = s_total = 0;
	e_queue = e_act = e_comp = e_total = 0;
        users = 0;
        user_list = (struct list *) malloc(sizeof(struct list));
        user_list->next = NULL;
        for(loc = 0; loc < 100; loc++)
                sample[loc] = ' '; 
   	if ((fd = fopen(enterq, "r")) != (FILE *) NULL)
	{
		while(fgets(buf, 81, fd) != (char *) NULL)
 		{
			strptr = buf;
			if((token=strtok(strptr, " \t:")) != NULL)
                        {
                              strptr = NULL;
                              if (strcmp("SAMPLE#", token) == 0)
      		                 	s_queue++;
                              else if (strcmp("USER", token) == 0)
                              {
                                     if ((token=strtok(strptr, " \t")) != NULL)
                                          listSearch(user_list, token, &users);
        }       }       }     }
        fclose(fd);
	if ((fd = fopen(doneq, "r")) != (FILE *) NULL)
	{
		while(fgets(buf, 81, fd) != (char *) NULL)
		{
			strptr = buf;
			if ((token = strtok(strptr, " \t:")) != NULL)
			{
	                      strptr = NULL;
                              if (strcmp("SAMPLE#", token) == 0)
			      {
			            e_comp++;
                                    if ((token=strtok(strptr, " \t")) != NULL)
                                    {
                                           loc = atoi(token);
                                           if (sample[loc] != 'x') 
                                           {
                                                  sample[loc] = 'x';
 				                  s_comp++;
                                           }
                                     }
			      }      
                              else if (strcmp("USER", token) == 0)
                              {
                                     if ((token=strtok(strptr, " \t")) != NULL)
                                          listSearch(user_list, token, &users);
                              }
                              else if (strcmp("STATUS", token) == 0)
                              {
                                      if((token=strtok(strptr, " \t\n")) !=NULL)
                                      {
                                             if (strcmp("Active", token) == 0)
					     {
                                                    if (sample[loc] == 'x')
                                                    {
							  sample[loc] = ' ';
                                                          s_comp--;
                                                    }
                                                    s_act++;
						    e_act++;
                                                    e_comp--;
        }       }       }     }       }      }
        fclose(fd);
	s_total = s_queue + s_comp + s_act;
	e_total = e_queue + e_comp;
        sprintf(label, "%4d", s_queue);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(S_NEW, args, 1);
        XtFree(xmstr);
        sprintf(label, "%4d", s_total);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(S_TOTAL, args, 1);
        XtFree(xmstr);
        sprintf(label, "%4d", s_comp);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(S_COMP, args, 1);
        XtFree(xmstr);
	sprintf(label, "%4d", s_act);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(S_ACT, args, 1);
        XtFree(xmstr);
	sprintf(label, "%4d", e_act);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(E_ACT, args, 1);
        XtFree(xmstr);
        sprintf(label, "%4d", e_comp); 
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(E_COMP, args, 1);
        XtFree(xmstr);
        sprintf(label, "%4d", users);
	xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(USER_NO, args, 1);
        XtFree(xmstr);
	free(user_list);
}



/**************************************************************************
*     This is a queue list search function used to search the user list.  *
*     If the the user name (token) is new, then it will be added in the   *
*     list and the user number will be increased, otherwise, do nothing.  *
**************************************************************************/
listSearch(userlist, token, users)
struct list *userlist;
char *token;
int *users;
{
        int find;
        struct list *node;

   	if (userlist->name == NULL)
        {
                sprintf(userlist->name,"%s", token);
                *users = 1;
        }
        else
        {   
                node = userlist;
                find = FALSE;
                for(;;)
                {
        	        if (strcmp(node->name, token)== 0)
                        {
                               find = TRUE;
                               break;
                        }
                        else if (node->next == NULL)
                               break;
                        else
                               node = node->next;
                } 
                if (!find)
                {
                        node->next = (struct list *) malloc(sizeof(struct list));
                        node = node->next;
                        sprintf(node->name,"%s",token);
                        node->next = NULL;
                        *users = *users + 1; 
                }
   	}	
}

setFocus(widget)
Widget   widget;
{
     XSetInputFocus(XtDisplay(widget), XtWindow(widget),
			 RevertToParent, CurrentTime);
     XtSetKeyboardFocus(statusShell, widget);
}


inputHandler(client, data, e)
Widget          client;
caddr_t         data;
XEvent          *e;
{
    char            tmp[2];
    int             jump;
    KeySym          keysym;
    XComposeStatus  compose;

   jump = 0;
   if (XLookupString(&(e->xkey), tmp, 2, &keysym, &compose))
   {
         if (tmp[0] == RETURN)
                jump = DOWN;
   }
   else
   {
         switch(keysym) {
         case XK_Down:
                        jump = DOWN;
                        break;
         case XK_Up:
                        jump = UP;
                        break;
         }
    }
    if (jump == UP)
    {
        if (client == sample_item)       setFocus(data_item);
        else if (client == user_item)    setFocus(sample_item);
        else if (client == macro_item)   setFocus(user_item);
        else if (client == solvent_item) setFocus(macro_item);
        else if (client == text_item)    setFocus(solvent_item);
        else if (client == data_item)    setFocus(text_item);
    }
    else if (jump == DOWN)
    {
        if (client == sample_item)       setFocus(user_item);
        else if (client == user_item)    setFocus(macro_item);
        else if (client == macro_item)   setFocus(solvent_item);
        else if (client == solvent_item) setFocus(text_item);
        else if (client == text_item)    setFocus(data_item);
        else if (client == data_item)    setFocus(sample_item);
    }
}




typedef  struct{
                Pixel   fg, bg;
                XFontStruct *fontinfo;
                } ApplicationData, *dataptr;
static   XtResource  statusFont[] = {
        { XmNfontList, XmCFontList, XtRFontStruct, sizeof(XFontStruct *),
          XtOffset(dataptr, fontinfo), XtRString, "fixed"},
        };
ApplicationData   Data;


create_mainframe(argc, argv)
int argc;
char **argv;
{
        char   *argv2[8];
	char   *param = "-geometry";
	char   *location = "+80+0";
        char    title[120], color_name[8];
        XColor  xcolor;
        
        for(n = 0; n < argc; n++)
		argv2[n] = argv[n];
/*
	argv2[n++] = param; 
	argv2[n++] = location;
*/
	argv2[n] = NULL;
/*
	XtToolkitInitialize();
	dpy = XtOpenDisplay (NULL, NULL, argv2[0], "status", NULL, 0,
                                &n, argv2);
*/
        statusShell = XtInitialize("status", "Status", NULL, 0,
                          &n, argv2);

	dpy = XtDisplay(statusShell);
        fontList = NULL;
        if (fontName[0] != '\0')
        {
             n = strlen(fontName);
             while(n)
             {
                if (fontName[n] == ' ')
                    fontName[n] = '\0';
                n--;
             }
             if ((xstruct = XLoadQueryFont(dpy, fontName))==NULL)
                fprintf (stderr,"couldn't open %s font\n", fontName);
             else
                fontList = XmFontListCreate (xstruct, " ");
        }

        sprintf(title, "status  %s", infopath);
	n = 0;
        XtSetArg (args[n], XmNtitle, title);  n++;
        XtSetArg (args[n], XmNresizeHeight, True);  n++;
/*
        statusShell = XtAppCreateShell (NULL, "status",
                             applicationShellWidgetClass, dpy, args, n);
*/
	XtSetValues(statusShell, args, n);
        screen = DefaultScreen(dpy);
	screenHeight = DisplayHeight (dpy, screen);
	n = 0;
        XtSetArg (args[n], XmNorientation, XmVERTICAL);  n++;
	statusFrame = XmCreateRowColumn (statusShell, "status", args, n);
    	XtManageChild (statusFrame);
	if (!fontList)
        {
            XtGetApplicationResources(statusFrame, &Data, statusFont,
                              XtNumber(statusFont), NULL, 0);
            xstruct = Data.fontinfo;
            if(xstruct)
                 fontList = XmFontListCreate (xstruct, " ");
        }
        if (xstruct != NULL)
	{
            charWidth =xstruct->max_bounds.width;
            charHeight = xstruct->max_bounds.ascent
                      + xstruct->max_bounds.descent;
        }
        else
        {
            charWidth = 10;
            charHeight = 15; 
        }
	n = 0;
        XtSetArg (args[n], XmNwidth, charWidth * WINWIDTH + 22);  n++;
    	XtSetArg (args[n], XmNresizeWidth, False);  n++;
    	XtSetValues(statusFrame, args, n);
        red_pix = yel_pix = -1;
        if (DefaultDepth(dpy, screen) > 2)
        {
                sprintf(color_name, "red");
                if (XParseColor(dpy,DefaultColormap(dpy,screen),color_name,
                                &xcolor))
                {
                    if (XAllocColor(dpy, DefaultColormap(dpy, screen),&xcolor))
                         red_pix = xcolor.pixel;
                }
                sprintf(color_name, "yellow");
                if (XParseColor(dpy,DefaultColormap(dpy,screen),color_name,
                                &xcolor))
                {
                    if (XAllocColor(dpy, DefaultColormap(dpy, screen),&xcolor))
                         yel_pix = xcolor.pixel;
                }
        }
}
	
Widget create_item(label, x, y)
char   *label;
int     x, y;
{
   Widget   retWidget;

   n =0;
   xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   XtSetArg (args[n], XmNlabelString, xmstr);  n++;
   XtSetArg (args[n], XmNx, charWidth * x);  n++;
   XtSetArg (args[n], XmNy, (charHeight + margin) * (y - 1));  n++;
   if (fontList)
             XtSetArg(args[n], XmNfontList,  fontList); n++;
   retWidget = XmCreateLabel(sum_panel, "sum", args, n);
   XtManageChild (retWidget);
   XtFree(xmstr);
   return(retWidget);
}

char *
search_token(str1, str2)
char  *str1, *str2;
{
	int     index, length, n;
	int     count;
        char   *retptr = NULL;
	static char *strptr = NULL;
	
	if (retptr != NULL)
	{
		XtFree(retptr);
		retptr = NULL;
	}
        if (str1 != NULL)
	     strptr = str1;
        else
             str1 = strptr;
	if (str1 != NULL)
          while(*str1 == ' ')
             str1++;
        strptr = str1;
        index = strlen(str2);
	if (str1 != NULL)
	  while(*str1 != NULL)	
	  {
                for(n = 0; n < index; n++)
                {
		    if (*str1 == *(str2+n))
		    {   
			infoAddr = strptr - file_string;
                        length = str1 - strptr;
		        retptr = (char *)  XtMalloc(length+1);
                        strncpy(retptr, strptr, length);
		        *(retptr + length) = '\0';
                        strptr = str1 + 1;
		        return(retptr);
		     }   
                } 
                switch (*str1) {
 		case '\n':
		case '\t':
			   strptr = str1 + 1;
			   while(*strptr == ' ')
				strptr++;
			   str1 = strptr - 1;
			   break;
		}
		str1++;
	  }
	return((char *) 0);
}
	 	

clear_mess()
{
	xmstr = XmStringLtoRCreate("", XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(err_mess, args, 1);
        XtFree(xmstr);
}


set_mess(mess)
char *mess;
{
	xmstr = XmStringLtoRCreate(mess, XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[0], XmNlabelString, xmstr);
        XtSetValues(err_mess, args, 1);
        XtFree(xmstr);
        XBell(dpy, 100);
}
	
Widget create_loc_item(label, x, y)
char   *label;
int     x, y;
{
   Widget   retWidget;
   Dimension height;

   n =0;
   xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   XtSetArg (args[n], XmNlabelString, xmstr);  n++;
   XtSetArg (args[n], XmNx, charWidth * x);  n++;
   XtSetArg (args[n], XmNy, (labelHeight +2) * (y - 1));  n++;
   if (fontList)
        XtSetArg(args[n], XmNfontList,  fontList); n++;
   retWidget = XmCreateLabel(panel_loc, "sum", args, n);
   XtManageChild (retWidget);
   XtFree(xmstr);
   if (y == 1)
   {
       n = 0;
       XtSetArg (args[n], XmNheight, &height); n++;
       XtGetValues(retWidget, args, n);
       labelHeight = (int) height;
   }
 
   return(retWidget);
}



Widget create_text_item(width, x, y)
int     width, x, y;
{
   Widget   retWidget;

   n =0;
   XtSetArg (args[n], XmNwidth, charWidth * width);  n++;
   XtSetArg (args[n], XmNmarginHeight, 1);  n++;
   XtSetArg (args[n], XmNx, charWidth * x);  n++;
   XtSetArg (args[n], XmNy, (labelHeight+2) * (y - 1));  n++;
   XtSetArg (args[n], XmNshadowThickness, 0); n++;
   if (fontList)
        XtSetArg(args[n], XmNfontList,  fontList); n++;
   retWidget = XmCreateText(panel_loc, "text", args, n);
   XtManageChild (retWidget);
   XtAddEventHandler(retWidget,KeyPressMask, False, inputHandler, NULL);
   XtAddEventHandler(retWidget,ButtonReleaseMask, False, setFocus, NULL);
   return(retWidget);
}

Widget
create_button_item(label, func)
char  *label;
int   *func();
{
   Widget   button;

   n =0;
   xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
   XtSetArg (args[n], XmNlabelString, xmstr);  n++;
   XtSetArg (args[n], XmNtraversalOn, FALSE);  n++;
   if (fontList)
        XtSetArg(args[n], XmNfontList,  fontList); n++;
   button = XmCreatePushButton(button_panel, "buttons", args, n);
   XtManageChild (button);
   XtAddCallback(button, XmNactivateCallback, func, NULL);
   XtFree(xmstr);
   return(button);
}


ok_proc(widget, client_data, call_data)
Widget  widget;
int    *client_data;
caddr_t  call_data;
{
        *client_data = 1;
        return;
}

no_proc(widget, client_data, call_data)
Widget  widget;
int    *client_data;
caddr_t  call_data;
{
        *client_data = 0;
        return;
}

int
alert_prompt(va_alist)
va_dcl
{
        Widget     warning, helpbut;
        Widget     okbut, nobut;
        XEvent     event;
        int        result, message, keyword;
        char       *str;
        va_list    args_ptr;
        XmString   xmstr2, xmstr_yes, xmstr_no;;

        message = 0;
        xmstr = XmStringLtoRCreate("", XmSTRING_DEFAULT_CHARSET);
        va_start(args_ptr);
        while (keyword = va_arg(args_ptr, int))
        {
                switch (keyword) {
                case  ALERT_STRINGS:
                        while ((str = va_arg(args_ptr, char *)) != NULL)
                        {
                            xmstr2 = XmStringLtoRCreate(str, XmSTRING_DEFAULT_CHARSET)
;
                            xmstr = XmStringConcat(xmstr, xmstr2);
                            XtFree(xmstr2);
                        }
                        break;
                case  ALERT_YES_BUTTON:
                        str = va_arg(args_ptr, char *);
                        xmstr_yes = XmStringLtoRCreate(str, XmSTRING_DEFAULT_CHARSET);
                        break;
                case  ALERT_NO_BUTTON:
                        str = va_arg(args_ptr, char *);
                        xmstr_no = XmStringLtoRCreate(str, XmSTRING_DEFAULT_CHARSET);
                        break;
                }
        }
        va_end(args_ptr);
        result = 3;
        n = 0;
        XtSetArg (args[n], XmNokLabelString, xmstr_yes);  n++;
        XtSetArg (args[n], XmNcancelLabelString, xmstr_no);  n++;
        XtSetArg (args[n], XmNmessageString, xmstr);  n++;
        XtSetArg (args[n], XmNmessageAlignment, XmALIGNMENT_BEGINNING);  n++;
        xmstr2 = XmStringLtoRCreate("STATUS WARNING", XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNdialogTitle, xmstr2);  n++;
        if(red_pix >= 0 && yel_pix >= 0)
        {
            XtSetArg (args[n], XmNbackground, red_pix); n++;
            XtSetArg (args[n], XmNforeground, yel_pix); n++;
        }
        if(fontList)
        {
              XtSetArg (args[n], XmNbuttonFontList, fontList); n++;
              XtSetArg (args[n], XmNlabelFontList, fontList); n++;
        }
        warning = XmCreateWarningDialog(statusShell, "status", args, n);
        helpbut = XmMessageBoxGetChild (warning, XmDIALOG_HELP_BUTTON);
        XtUnmanageChild (helpbut);
        okbut = XmMessageBoxGetChild (warning, XmDIALOG_OK_BUTTON);
        nobut = XmMessageBoxGetChild (warning, XmDIALOG_CANCEL_BUTTON);
        XtAddCallback(warning, XmNokCallback, ok_proc, &result);
        XtAddCallback(warning, XmNcancelCallback, no_proc, &result);
        XtManageChild (warning);
        XtFree(xmstr);
        XtFree(xmstr2);
        XtFree(xmstr_yes);
        XtFree(xmstr_no);
        XBell(dpy, 100);
        for(;;)
        {
                XtNextEvent(&event);
                XtDispatchEvent(&event);
                if (result != 3)
                      return(result);
        }
}

