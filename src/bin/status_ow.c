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

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/seln.h>
#include <xview/textsw.h>
#include <xview/font.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <xview/alert.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>

#define	DONEQ_OR_ENTERQ		1
#define DONEQ_AND_ENTERQ	0
#define NUM_LINES	8

Frame 	base, frame_loc;
Panel	button_panel, panel_loc, mess_panel, sum_panel;
Textsw	showsw;
Panel_item  sample_item,user_item,macro_item,solvent_item,text_item,data_item;
Panel_item  err_mess, user, p1, locate_button, loc_don;          
Panel_item  S_TOTAL, S_NEW, S_ACT, S_COMP, USER_NO;
Panel_item  E_TOTAL, E_FREE, E_ACT, E_COMP;
char	*getcwd();
char    *getlogin();
char	owner[20];
char    loginname[20];
char    dataPath[120];
char    infopath[120];
char    logfile[120];
char    doneq[120]; 
char    enterq[120];
char    sumq[120];
/*
Pixwin  *pixwins[4], *pw;
u_char  red[6] =   {200,180};
u_char  green[6] = {200, 20};
u_char  blue[6] =  {  0,140};
*/
struct pixrect   *log_image, *ret_image, *loc, *don;
static int	log, pipefd, infoAddr;
Notify_value	take();
Textsw_index	start, dest, last;
static int	vnmr;
static int	done_enter;
static int	first_locate;	/* search for first or further entries? */
static int	point;		/* used for current insertion point     */
static int	save_point;	/* save location for locate		*/
static int	save_lineno;	/* save line num for locate		*/
static XrmDatabase dbase = NULL;
static Widget    mShell;


int      charHeight, charWidth;
Display *dpy;
Xv_Font  font, bfont;


static char
*get_x_def(class, name)
char  *class, *name;
{
        char       *str_type;
        XrmValue   rval;
        char       opt[128];

        if (dbase == NULL)
           return((char *) NULL);
        sprintf(opt, "%s*%s", class, name);
        if (XrmGetResource(dbase, opt, class, &str_type, &rval))
              return((char *) rval.addr);
        str_type = XGetDefault(dpy, class, name);
        return(str_type);
}


main(argc, argv)
int	argc;
char	**argv;

{
	int  x, n;
	Rect *screen;
	char label[120], current_dir[120];
        char *temp, *getenv();
	char fontname[6];
	struct passwd *pwentry,*getpwuid(); 

        temp = getenv("graphics");
	if (temp == NULL)
	{
            fprintf(stderr, "Error: environment 'graphics' is not set.\n");
            exit(0);
	}		
	if((strcmp("suncolor", temp) != 0) && (strcmp("sun", temp) != 0))
	{
            fprintf(stderr, "stopped(improper window system on terminal)\n");
            exit(0);
	}		
    	if (argc < 2)
	{
 		fprintf(stderr, "usage: status path [-fn font]\n");
		exit(0);
	}
	log = FALSE;
        temp = getlogin();
	if (temp == NULL)
	{
	    pwentry = getpwuid(getuid());
	    if (pwentry == NULL)
	    {
		fprintf(stderr, "Error: getpwuid failed\n");
		exit(0);
	    }
	    temp = pwentry->pw_name;
	}
	sprintf(loginname, "%s", temp);
        getcwd(current_dir, sizeof( current_dir ));
	vnmr = 0;
	infopath[0] = '\0';
        for(n = 1; n < argc; n++)
	{
             	if (strcmp("vnmr", argv[n]) == 0)
        	{
		      if (argc <= n+2)
		      {
			  fprintf(stderr, "Illegal parameter 'vnmr' \n");
			  exit (0);
		      }
                      strcpy(infopath, argv[n+1]);
		      vnmr = 1;
	              pipefd = atoi(argv[n+2]);
		      n = n + 2;
 		}
		else
        	{
		      if (!strcmp(argv[n], "-fn") || !strcmp(argv[n], "-display"))
			    n++;
		      else
		      {
                	   strcpy(infopath, argv[n]);
                	   if (infopath[0] == '-')
                	   {
                        	temp = &infopath[n];
                        	sprintf(infopath, "%s", temp);
                	   }
		      }
                }
        }   
        x = strlen(infopath);
	if (x <= 0)
	{
		fprintf(stderr, "ERROR: needs file directory\n");
		exit(0);
	}
        if (infopath[x-1] != '/') { infopath[x] = '/'; infopath[x+1] = '\0'; }
	sprintf(label, "               STATUS         %s", infopath);
        strcpy(owner, loginname);
	init_display(argc, argv);
	base = xv_create(NULL, FRAME,
		XV_X, 100,
		XV_Y, 0,
		WIN_ROWS, 50,
		FRAME_ARGS,  argc, argv,
		FRAME_LABEL, label,
		0);
	dpy = (Display *) xv_get(base, XV_DISPLAY);
	font = (Xv_Font) xv_get(base, XV_FONT);
	charHeight = (int) xv_get(font,FONT_DEFAULT_CHAR_HEIGHT);
	charWidth = (int) xv_get(font,FONT_DEFAULT_CHAR_WIDTH);
	screen = (Rect *) xv_get(base, WIN_SCREEN_RECT);
	n = (int) xv_get(base, XV_HEIGHT);
	if (n > screen->r_height - 20)
           xv_set(base, XV_HEIGHT, screen->r_height - 20, 0);
/**
	switch (charWidth)	
	{
	case 5:
                sprintf(fontname, "5x8");
                break;
        case 6:
                sprintf(fontname, "6x10");
                break;
        case 7:
                sprintf(fontname, "7x13");
                break;
        case 8:
                sprintf(fontname, "8x13");
                break;
        default:
                sprintf(fontname, "9x15");
                break;
   	}
	bfont = (Xv_Font) xv_find(base, FONT, FONT_NAME, fontname, NULL);
**/
	create_button();  
	create_mess();
	create_summary();
	create_locate();
	create_show();
	window_fit(base);
	xv_main_loop(base);   
	exit(0);
}

create_button()
{
	void rm_proc(), set_user(), quit_proc();
	void log_disp(), ret_disp(), rt_proc();
        void del_proc(), loc_disp();

	button_panel = xv_create(base, PANEL, 0);
/*
	(void) xv_create(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
			panel_button_image(button_panel, "user", 0, 0),
			PANEL_LABEL_X,  xv_col(button_panel, 16),
			PANEL_NOTIFY_PROC, set_user,
			0);
*/
        log_image    = panel_button_image(button_panel, "log ", 0, 0);
        ret_image    = panel_button_image(button_panel, "ret ", 0, 0);
	loc = panel_button_image(button_panel, "locate", 0, 0);
	don = panel_button_image(button_panel, " done ", 0, 0);
	p1= xv_create(button_panel, PANEL_BUTTON,
			PANEL_LABEL_X,  xv_col(button_panel, 16),
                        PANEL_LABEL_IMAGE, ret_image,
			PANEL_NOTIFY_PROC, ret_disp,
			0);          
	loc_don= xv_create(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE, loc,
			PANEL_NOTIFY_PROC, loc_disp,
			0);          
        if (vnmr)
        {
              (void) xv_create(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE,
                        panel_button_image(button_panel, " rt ", 0, 0),
			PANEL_NOTIFY_PROC, rt_proc,
                        0);
	}

	(void) xv_create(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE,
			panel_button_image(button_panel, "delete", 0, 0),
                        PANEL_NOTIFY_PROC, del_proc,
			0);
	(void) xv_create(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE,
			panel_button_image(button_panel, "quit", 0, 0),
			PANEL_NOTIFY_PROC, quit_proc,
			0); 
	window_fit_height(button_panel);
}


create_mess()
{
	mess_panel = xv_create(base, PANEL,
			WIN_ROWS, 1,
			0);
  	user = xv_create(mess_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "current user: ",
			PANEL_VALUE, owner,  
			PANEL_VALUE_DISPLAY_LENGTH, 30,
			0);
	err_mess = xv_create(mess_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, " ",
			0);
 }



create_summary()
{
	int    height;

	height = (charHeight + 4) * 6;
	sum_panel = xv_create(base, PANEL,
/*
			XV_FONT,    bfont,
*/
			XV_FONT,    font,
			XV_HEIGHT,  height,
                        0);
	height = height / 4; 
        xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "QUEUED",
			PANEL_LABEL_X, xv_col(sum_panel, 15),
			XV_Y,		0,
			0);
	xv_create(sum_panel, PANEL_MESSAGE, 
                        PANEL_LABEL_STRING, "ACTIVE", 
                        PANEL_LABEL_X, xv_col(sum_panel, 24),
			XV_Y,		0,
			0);
	xv_create(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "COMPLETE",  
                        PANEL_LABEL_X, xv_col(sum_panel, 33), 
			XV_Y,		0,
                        0); 
        xv_create(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "TOTAL",   
                        PANEL_LABEL_X, xv_col(sum_panel, 45),
			XV_Y,		0,
			0);
        xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "SAMPLES    :",
			PANEL_LABEL_X, xv_col(sum_panel, 1),
			XV_Y,		height,
			0);
	xv_create(sum_panel, PANEL_MESSAGE, 
                        PANEL_LABEL_STRING, "EXPERIMENTS:",
                        PANEL_LABEL_X, xv_col(sum_panel, 1),
			XV_Y,		2 * height,
                        0); 
        xv_create(sum_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "USERS      :",
			PANEL_LABEL_X, xv_col(sum_panel, 1),
			XV_Y,		3 * height,
			0);
        S_TOTAL = xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, xv_col(sum_panel, 46),
			XV_Y,		height,
			0);
	S_NEW = xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, xv_col(sum_panel, 17), 
			XV_Y,		height,
			0);
    	S_ACT = xv_create(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, xv_col(sum_panel, 26),  
			XV_Y,		height,
			0);
	S_COMP= xv_create(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, xv_col(sum_panel, 37),  
			XV_Y,		height,
			0);
	E_TOTAL = xv_create(sum_panel, PANEL_MESSAGE,    
                        PANEL_LABEL_STRING, "-", 
                        PANEL_LABEL_X, xv_col(sum_panel, 48), 
			XV_Y,		2 * height,
                        0);
	E_FREE = xv_create(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "-", 
                        PANEL_LABEL_X, xv_col(sum_panel, 19),  
			XV_Y,		2 * height,
                        0);
	E_ACT = xv_create(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "", 
                        PANEL_LABEL_X, xv_col(sum_panel, 26),  
			XV_Y,		2 * height,
                        0);
	E_COMP= xv_create(sum_panel, PANEL_MESSAGE,    
                        PANEL_LABEL_STRING, "", 
                        PANEL_LABEL_X, xv_col(sum_panel, 37),
			XV_Y,		2 * height,
                        0);
        USER_NO = xv_create(sum_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, xv_col(sum_panel, 46),
			XV_Y,		3 * height,
			0); 
        xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, xv_col(sum_panel, 19),
			XV_Y,		3 * height,
                        0);
        xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, xv_col(sum_panel, 39),
			XV_Y,		3 * height,
                        0);
        xv_create(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, xv_col(sum_panel, 28),
			XV_Y,		3 * height,
                        0);

        get_summary(); 
	window_fit_height(sum_panel);
}


create_show()
{
	Textsw   textsw;
        void     ret_disp();

	showsw = xv_create(base, TEXTSW,
                        TEXTSW_FILE, 		0,
			WIN_COLUMNS,		80,
			XV_FONT, 		font,
			WIN_CONSUME_EVENTS, 	WIN_NO_EVENTS,
                        WIN_MOUSE_BUTTONS, 	0,
			0);
	textsw = textsw_first(showsw);
        while(textsw)
	{
		notify_interpose_event_func(textsw, take, NOTIFY_SAFE);   
		textsw = textsw_next(textsw);
	}
        ret_disp();
}

create_locate()
{
void 	locate_done_proc();
void 	locateproc();
void  	setnext();
int   	height;
int   	x1, x2, difx;

	height = (int) xv_get(sum_panel, XV_HEIGHT);
	height = charHeight + 4;
	panel_loc = xv_create(base, PANEL,
		XV_SHOW, 		FALSE,
		WIN_BELOW, 		mess_panel,
		XV_X,			0,
		WIN_ROW_GAP, 		1,
		PANEL_ITEM_Y_GAP,       1,
		0);
	sample_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_col(panel_loc, 0),
		XV_Y,			0,	
		PANEL_LABEL_STRING,	"  SAMPLE#:",
		PANEL_VALUE_DISPLAY_LENGTH,	5,
		0);
        x1 = (int)xv_get(sample_item, PANEL_VALUE_X);

	user_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_col(panel_loc, 0),
		XV_Y,			height,	
		PANEL_LABEL_STRING,	"USER:",
		PANEL_VALUE_DISPLAY_LENGTH,	32,
		0);
	x2 = (int)xv_get(user_item, PANEL_VALUE_X);
	difx = x1 - x2;
	xv_set(user_item, 
		PANEL_LABEL_X,          xv_col(panel_loc, 0) + difx,
		PANEL_VALUE_X,		x1,
		0);

	macro_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_col(panel_loc, 0),
		XV_Y,			2 * height,	
		PANEL_LABEL_STRING,	"MACRO:",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
		0);
	x2 = (int)xv_get(macro_item, PANEL_VALUE_X);
	difx = x1 - x2;
	xv_set(macro_item, 
		PANEL_LABEL_X,          xv_col(panel_loc, 0) + difx,
		PANEL_VALUE_X,		x1,
		0);

	solvent_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_row(panel_loc, 0),
		XV_Y,			3 * height,	
		PANEL_LABEL_STRING,	" SOLVENT: ",
		PANEL_VALUE_STORED_LENGTH,	20,
		PANEL_VALUE_DISPLAY_LENGTH,	20,
		0);
	x2 = (int)xv_get(solvent_item, PANEL_VALUE_X);
	difx = x1 - x2;
	xv_set(solvent_item, 
		PANEL_LABEL_X,          xv_col(panel_loc, 0) + difx,
		PANEL_VALUE_X,		x1,
		0);

	text_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_row(panel_loc, 0),
		XV_Y,			4 * height,	
		PANEL_LABEL_STRING,	"TEXT:",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
		PANEL_VALUE_STORED_LENGTH,	128,
		0);
	x2 = (int)xv_get(text_item, PANEL_VALUE_X);
	difx = x1 - x2;
	xv_set(text_item, 
		PANEL_LABEL_X,          xv_col(panel_loc, 0) + difx,
		PANEL_VALUE_X,		x1,
		0);

	data_item = xv_create(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		xv_row(panel_loc, 0),
		XV_Y,			5 * height,	
		PANEL_LABEL_STRING,	"DATA:",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
		PANEL_VALUE_STORED_LENGTH,	128,
		0);
	x2 = (int)xv_get(data_item, PANEL_VALUE_X);
	difx = x1 - x2;
	xv_set(data_item, 
		PANEL_LABEL_X,          xv_col(panel_loc, 0) + difx,
		PANEL_VALUE_X,		x1,
		0);

	locate_button = xv_create(panel_loc, PANEL_BUTTON,
		PANEL_LABEL_IMAGE,
				 panel_button_image(panel_loc,"locate",0,0),
		PANEL_LABEL_X,		xv_col(panel_loc, 50),
		XV_Y,			0,	
		PANEL_NOTIFY_PROC,	locateproc,
		0);
/* --- create full enter screen and fill if argument supplied --- */

	height = (int) xv_get(sum_panel, XV_HEIGHT);
	xv_set(panel_loc, XV_HEIGHT, height, NULL);
	
}

void locate_done_proc()
{
	void loc_disp();

	first_locate = TRUE;
	clear_errmsg();
	xv_set(panel_loc,	XV_SHOW, FALSE, 0);
	xv_set(loc_don, 
		PANEL_LABEL_IMAGE,	loc,
		PANEL_NOTIFY_PROC,	loc_disp,
		0);
}

void loc_disp()
{
	xv_set(panel_loc,	XV_SHOW, TRUE, 0);
	clear_errmsg();
	xv_set(loc_don, 
		PANEL_LABEL_IMAGE,	don,
		PANEL_NOTIFY_PROC,	locate_done_proc,
		0);
}

/*-------------------------------------------------------------
|  locateproc
+------------------------------------------------------------*/
void locateproc ()
{
char	*samp, *user, *macr, *solv, *text, *data, *cpy_ptr, line[160];
char	*ptr, buffer[1010];
int	samp_found, user_found, macr_found, solv_found, text_found, data_found;
int	found, line_number;
void	sample_select();

   samp = (char *)xv_get(sample_item,  PANEL_VALUE);
   user = (char *)xv_get(user_item,    PANEL_VALUE);
   macr = (char *)xv_get(macro_item,   PANEL_VALUE);
   solv = (char *)xv_get(solvent_item, PANEL_VALUE);
   text = (char *)xv_get(text_item,    PANEL_VALUE);
   data = (char *)xv_get(data_item,    PANEL_VALUE);
   while (*samp == ' ') samp++;   /* ignore leading spaces */
   if (*samp==0 && *user==0 && *macr==0 && *solv==0 && *text==0 && *data==0)
   {  xv_set(panel_loc,   PANEL_CARET_ITEM, sample_item, 0);
      err_disp("You must enter something");
      return;
   }
   clear_errmsg();
   found = FALSE;
   if (first_locate) line_number = point = 0;
   else            { line_number = save_lineno; point = save_point; }
   xv_get(showsw,TEXTSW_CONTENTS,point,buffer,1000);
   while ( buffer[0] != '\0' && buffer[1] != '\0' && !found)
   {  ptr = &buffer[0];
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*samp)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
          if ( !strcmp(&line[0], samp) )  samp_found = TRUE;
          else                            samp_found = FALSE;
      }
      else samp_found = TRUE;
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*user)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
          if ( !strcmp(&line[0], user) )  user_found = TRUE;
          else                            user_found = FALSE;
      }
      else user_found = TRUE;
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*macr)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
          if ( !strcmp(&line[0], macr) )  macr_found = TRUE;
          else                            macr_found = FALSE;
      }
      else macr_found = TRUE;
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*solv)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
          if ( !strcmp(&line[0], solv) )  solv_found = TRUE;
          else                            solv_found = FALSE;
      }
      else solv_found = TRUE;
/* text */
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*text)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
	  if ( strstr(&line[0],text) )    text_found = TRUE;
          else				  text_found = FALSE;
      }
      else text_found = TRUE;
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*data)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
          if ( !strcmp(&line[0], data) )  data_found = TRUE;
          else                            data_found = FALSE;
      }
      else data_found = TRUE;
      while ( *ptr != '-') ptr++;
      while (*ptr != '\n') ptr++; ptr++;
      found = samp_found && user_found && macr_found &&
              solv_found && text_found && data_found;
      if (!found)
      {   point += (int) (ptr - &buffer[0]);
          xv_get(showsw,TEXTSW_CONTENTS,point,buffer,1000);
          line_number += NUM_LINES+1;
      }
   }
   if (found)
   {  save_lineno = line_number + NUM_LINES + 1;
      save_point = point + (int) (ptr - &buffer[0]);
      ptr = &buffer[0];
      while ( *ptr != '\n')  ptr++; ptr++;
      while ( *ptr != '-') ptr++;
      xv_set(showsw, TEXTSW_INSERTION_POINT, point, 0);
      xv_set(showsw, TEXTSW_INSERTION_POINT, point, 0);
      textsw_possibly_normalize(showsw, point+ptr-buffer);
      textsw_possibly_normalize(showsw, point);
      sample_select();
      first_locate = FALSE;
   }
   else
   {  err_disp("No (more) matching entry found");
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
Notify_value
take(showsw, event, arg, type) 
Notify_client	showsw;
Event	*event;
Notify_arg	arg;
Notify_event_type	type;
{
	int  id;
	
	void sample_select();

	id = event_id(event);
        if (event_is_ascii(event) || id == MS_MIDDLE || id == MS_RIGHT)
    	     return(NOTIFY_DONE);
        dest = 0;
        notify_next_event_func(showsw, event, arg, type); 
	if (event_id(event) == MS_LEFT)
	{
	    clear_errmsg();
	    if (event_is_up(event))         
	    {
	         clear_errmsg();
	         sample_select();
	    }
	}
	return(NOTIFY_DONE);
}


/*****************************************************************************
*   This function will pass the fid pathname thruogh pipe to its parent      *
*   processor.                                                               *
*****************************************************************************/
void
rt_proc()
{
        char  filepath[160],rtFile[180];
	void  quit_proc();

	clear_errmsg();
        if (dest == 0 || dataPath[0] == '\0')
	{
             err_disp("rt: unknown file");
             return;
	} 
        textsw_set_selection(showsw, infoAddr, infoAddr+strlen(dataPath),1);
	strcpy(filepath,dataPath);
	strcat(filepath,".fid");
        if (!search_file(filepath))
             return;
        sprintf(rtFile, "rt(\'%s\')", filepath);
 	if (write(pipefd, rtFile, strlen(rtFile)) == -1)
		fprintf(stderr, "ERROR: pipe can not write\n");
	else
	{
		quit_proc();
/*                unlink(sumq);
		xv_set(base, FRAME_NO_CONFIRM, TRUE, 0);
		window_destroy(base); */
	}
}



/******************************************************************************
*    This function is used to select the sample and hightlight it.            *
******************************************************************************/
void
sample_select()
{
	int     temp, i, row, get, find, top_line;
	char	buf[130], *token, *tokptr;
	Textsw_index  mark, count;

	if (log) return;
        mark = (Textsw_index) xv_get(showsw, TEXTSW_INSERTION_POINT);
	if (mark < 0) return;
        top_line = (Textsw_index) xv_get(showsw, TEXTSW_FIRST_LINE);
        count = textsw_index_for_file_line(showsw, top_line);
	for(row= top_line; count <= mark && count >= 0; row++)
		count = textsw_index_for_file_line(showsw, row);
        row -= 2;
	temp = row;
	find = FALSE;	
        if (row == 0) 
		start = 0;
	else
	{
             while ( row > 0 && !find )
 	     {
                 count = textsw_index_for_file_line(showsw, row);
	         if (count < 0)   /* out of boundary */
			return;
	         get = 0;
		 for(i = 0; i <= 70; i++)
		 {
		     xv_get(showsw, TEXTSW_CONTENTS, count, buf, 1);
                     if (buf[0] == '-')
		     {            
		          count++;
		          get++;
	             }
	             else
		          break;
	         }			
		 if (get >= 70)
		     find = TRUE;
	         else 
		     row--;
              }
              if (row > 0)
	      {
	         row++;
	         start = textsw_index_for_file_line(showsw, row);
	      }
	      else
	         start = 0;
	}		
	find = FALSE;
  	if (temp == 0)
		temp++;
        count = textsw_index_for_file_line(showsw, temp);
	while (count != NULL && !find)
	{
              get = 0; 
              for(i = 0; i <= 70; i++) 
              { 
                  xv_get(showsw, TEXTSW_CONTENTS, count, buf, 1);
	          if (count < 0)
			return;
                  if (buf[0] == '-') 
	          {
	                count++;
                        get++; 
	          }
                  else                  
                        break;
              }                       
              if (get >= 70)
                  find = TRUE;
              else
	          temp++;
	      count = textsw_index_for_file_line(showsw, temp);
	}
	if (!find)
	      exit(0);
	last = count;
        textsw_set_selection(showsw, start, count, 1);    
	dest = count + 70;
	while (buf[0] != '\n')
	{         
	      dest++;
	      xv_get(showsw, TEXTSW_CONTENTS, dest, buf, 1);
	}
        find = FALSE;
	while(!find && temp > row)
	{
		--temp;
                infoAddr = NULL;
		count = textsw_index_for_file_line(showsw, temp);
		xv_get(showsw, TEXTSW_CONTENTS, count, buf, 130); 
                tokptr = buf;
		if ((token = strtok(tokptr, ": \t")) != NULL)
                {
			tokptr = NULL;
			if (strcmp(token, "DATA") == 0)
                        {                     
                              find = TRUE;
                              if((token = strtok(tokptr, " \t")) != NULL)
			      {
                                    char buf2[130];
				    if (*token == '\n')
					break;
                                    i = token - buf;
 				    infoAddr = count + i;
			            sprintf(buf2, "%s", token);
				    token = buf2;
                                    while(*token != '\0')
				    {
					if (*token == '\n')
					    *token = '\0';
					else
					    token++;
				    }
                                    if (buf2[0] == '/')
			               strcpy(dataPath,buf2);
                                    else
			               sprintf(dataPath, "%s%s", infopath, buf2);
        }       }       }     }

        if (!find)
          	dataPath[0] = '\0';    
}



void
quit_proc()
{
        if (!done_enter) unlink(sumq);
	else textsw_save(showsw, 0, 0);
	exit(0);
}



void
ret_disp()
{
        int  pid;
        char cmd[240];
	void  log_disp();

	clear_errmsg();
        pid = getppid();
        sprintf(doneq, "%s/doneQ", infopath);
        sprintf(enterq, "%s/enterQ", infopath);
        sprintf(sumq, "/usr/tmp/ASMTXTSW.%d", pid);
        unlink(sumq);
	xv_set(showsw, XV_SHOW, FALSE, 0);   
	if (access(doneq, 4) == 0)
	{
         	if (access(enterq, 4) == 0)
                {    sprintf(cmd, "cat %s %s > %s", doneq, enterq, sumq);
		     done_enter = DONEQ_AND_ENTERQ;
		}
                else
                {    strcpy(sumq,doneq);
		     done_enter = DONEQ_OR_ENTERQ;
		}
        }
	else
        {
		if (search_file(enterq))
           	{    strcpy(sumq,enterq);
		     done_enter = DONEQ_OR_ENTERQ;
		}
                else
		{
                     fprintf(stderr, "ERROR: neither enterQ nor doneQ found\n");
                     exit(0);
                 }
	} 
        if (!done_enter) system(cmd);
	xv_set(locate_button, XV_SHOW, TRUE, 0);
        xv_set(p1, PANEL_LABEL_IMAGE, log_image, 0);
        xv_set(p1, PANEL_NOTIFY_PROC, log_disp, 0);
        clear_errmsg();
        xv_set(mess_panel, WIN_KBD_FOCUS, FALSE, 0);
        log = FALSE;
        dest = 0;
        dataPath[0] = '\0';
        xv_set(showsw, TEXTSW_FILE, sumq, 0);
        xv_set(showsw, XV_SHOW, TRUE, 0);
}


/***************************************************************************
*   display the log file                                                   *
***************************************************************************/
void
log_disp()
{
        clear_errmsg();
        if ((dataPath[0] == '\0') || (infoAddr == NULL))
        {
                err_disp( "Unknown file path");
                return;
        }
	textsw_set_selection(showsw, infoAddr, infoAddr+strlen(dataPath),1);
	strcpy(logfile,dataPath);
	strcat(logfile,".fid/log");
   	if(search_file(logfile) == 0)
		return;
	xv_set(locate_button, XV_SHOW, FALSE, 0);
	xv_set(p1, PANEL_LABEL_IMAGE, ret_image, 0);
	xv_set(p1, PANEL_NOTIFY_PROC, ret_disp, 0);
	xv_set(mess_panel, WIN_KBD_FOCUS, FALSE, 0);
	log = TRUE;
	dest = 0;
	textsw_save(showsw, 0, 0);
        xv_set(showsw, TEXTSW_FILE, logfile, 0); 
        xv_set(showsw, XV_SHOW, TRUE, 0);
}



int 
search_file(file)
char *file;
{
	int	k;
        extern char     *sys_errlist[];

	k = access(file, 4);
	if (k < 0)
	{
		err_disp(sys_errlist[errno]);
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

        if (dest == 0 || log)
		return;
        answer = 0;
	if ((dataPath[0] == '\0') || (infoAddr == NULL))
	{
		err_disp("Unknown file path");
		return;
	}
        clear_errmsg();
        textsw_set_selection(showsw, infoAddr, infoAddr+strlen(dataPath),1);
	answer = alert_prompt ( base, (Event *)NULL,
		ALERT_MESSAGE_STRINGS,	"Press left button to confirm delete.",
					"To cancel press right button.",
					0,
		ALERT_BUTTON_YES,	"Confirm delete",
		ALERT_BUTTON_NO,	"Cancel",
		0);
	if ( answer == ALERT_NO ) return;
	delete();
        if (done_enter)
        {  del_more = TRUE;
	   lcount = 15;
           xv_set(showsw, TEXTSW_INSERTION_POINT, start, 0);
	   while (del_more && lcount)
           {  textsw_edit(showsw, SELN_LEVEL_LINE, 1, 0);
              textsw_edit(showsw, SELN_LEVEL_FIRST, 1, 0);
              xv_get(showsw, TEXTSW_CONTENTS, start, buffer, 15);
	      for (i=0; i<10; i++) if (buffer[i] == '-') del_more=FALSE;
	      lcount--;
	   }
           textsw_edit(showsw, SELN_LEVEL_LINE, 1, 0);
           textsw_edit(showsw, SELN_LEVEL_FIRST, 1, 0);
        }
}


/****************************************************************************
*    To delete the directory, which path name is in the done queue, selected*
*    by hitting the mouse on that sample.                                   * 
****************************************************************************/
delete()
{
	char  file[120], cmd[130];
        struct stat status;
	struct passwd *pwentry,*getpwuid(); 

	strcpy(file,dataPath);
	strcat(file,".fid");
        if (access(file, 0) != 0)
        {
		err_disp("No such file");
 		return;
	}  
        if (stat(file, &status) == -1)
	{
		err_disp("error: can not stat");
                return;
	}
        if ((pwentry = getpwuid(status.st_uid)) == NULL)
        {
		err_disp("error: can not find owner");
		return;
	}
	if((strcmp("root", pwentry->pw_name) != 0) && (strcmp(loginname,pwentry->pw_name) != 0))
	{
		err_disp("Not owner, permission denied");
		return;
	}
        sprintf(cmd, "rm -rf %s", file);
	system(cmd);
}

void
set_user()
{
        clear_errmsg();
	xv_set(user, PANEL_VALUE,  "", 0);
	xv_set(mess_panel, PANEL_CARET_ITEM, 
			user, 0);
	xv_set(mess_panel, WIN_KBD_FOCUS, TRUE, 0);
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
	int    dest = 0;

	while (*from <= *to && str2[*from]) 
	{
		str1[dest] = str2[*from];
		dest++;
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

        getcwd(dir, sizeof( dir ));
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
        sprintf(label, "%3d", s_queue);
   	xv_set(S_NEW, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%3d", s_total);
	xv_set(S_TOTAL, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%3d", s_comp);
        xv_set(S_COMP, PANEL_LABEL_STRING, label, 0);
	sprintf(label, "%3d", s_act);
	xv_set(S_ACT, PANEL_LABEL_STRING, label, 0);
	sprintf(label, "%3d", e_act);
 	xv_set(E_ACT, PANEL_LABEL_STRING, label, 0); 
        sprintf(label, "%3d", e_comp); 
        xv_set(E_COMP, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%3d", users);
        xv_set(USER_NO, PANEL_LABEL_STRING, label, 0);
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
/*
        char *malloc();
*/
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


err_disp(msg)
char  *msg;
{
	xv_set(err_mess, PANEL_LABEL_STRING, msg, 0);
	XBell(dpy,20);
}


clear_errmsg()
{
	xv_set(err_mess, PANEL_LABEL_STRING, " ", 0);
}


init_display(argc, argv)
int     argc;
char    *argv[];
{
        int     i, fontOption, argc2;
        char    *default_font, *argv2[8];
        char    *dispName;
        char    *dname = "unix:0.0";
        char    *option = "-fn";

	mShell = XtInitialize("Status", "Status", NULL, 0,
                          &argc, argv);

   	dpy = XtDisplay(mShell);
   	dbase = XtDatabase(dpy);
        fontOption = 0;
        argv2[0] = (char *)argv[0];
        argc2 = 1;
        dispName = NULL;
        for(i = 1; i < argc; i++)
        {
                if (!strcmp("-fn", argv[i]))
                {
                      fontOption = 1;
                      argv2[argc2++] = (char *)argv[i++];
                      if (argv[i] != NULL)
                         argv2[argc2++] = (char *)argv[i];
                      break;
                }
                if (!strcmp("-display", argv[i]))
                {
                      argv2[argc2++] = (char *)argv[i++];
                      argv2[argc2++] = (char *)argv[i];
                      dispName = (char *)argv[i];
                }
        }
        if (dispName == NULL)
        {
                dispName = getenv("DISPLAY");
                if (dispName == NULL)
                     dispName = dname;
        }
        if(!fontOption)
        {
/**
        	dpy = (Display *) XOpenDisplay(dispName);
        	if (dpy)
           	   default_font = XGetDefault(dpy, "status", "fontList");
**/
	    default_font = get_x_def("*Status", "font");
	    if (default_font == NULL)
	        default_font = get_x_def("*Status", "fontList");
            if (default_font)
            {
                     argv2[argc2++] = option;
                     argv2[argc2++] = default_font;
            }
        }

	argv2[argc2] = NULL;
        xv_init(XV_INIT_ARGS, argc2, argv2, NULL);
}
