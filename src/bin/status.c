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

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/seln.h>
#include <suntool/textsw.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <suntool/alert.h>

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
char	*getwd();
char    *getlogin();
char	owner[20];
char    loginname[20];
char    dataPath[120];
char    infopath[120];
char    logfile[120];
char    doneq[120]; 
char    enterq[120];
char    sumq[120];
Pixwin  *pixwins[4], *pw;
u_char  red[6] =   {200,180};
u_char  green[6] = {200, 20};
u_char  blue[6] =  {  0,140};
struct pixrect   *x, *y, *loc, *don;
static int	log, pipefd, infoAddr;
Notify_value	take();
Textsw_index	start, dest, last;
static int	vnmr;
static int	done_enter;
static int	first_locate;	/* search for first or further entries? */
static int	point;		/* used for current insertion point     */
static int	save_point;	/* save location for locate		*/
static int	save_lineno;	/* save line num for locate		*/

main(argc, argv)
int	argc;
char	**argv;

{
	int  x;
	char label[120], current_dir[120];
        char *temp, *getenv();
	Rect  *screen;

        temp = getenv("graphics");
	if((strcmp("suncolor", temp) != 0) && (strcmp("sun", temp) != 0))
	{
            fprintf(stderr, "stopped(improper window system on terminal)\n");
            exit(0);
	}		
    	if (argc < 2)
	{
 		fprintf(stderr, "usage: status path\n");
		exit(0);
	}
	log = FALSE;
        temp = getlogin();
	sprintf(loginname, "%s", temp);
        getwd(current_dir);
        if (strcmp("vnmr", argv[1]) == 0)
        {
                strcpy(infopath, argv[2]);
		vnmr = 1;
	        pipefd = atoi(argv[3]);
 	}
	else
        {
                strcpy(infopath, argv[1]);
		vnmr = 0;
                if (infopath[0] == '-')
                {
                        temp = &infopath[1];
                        sprintf(infopath, "%s", temp);
                }
        }   
        x = strlen(infopath);
        if (infopath[x-1] != '/') { infopath[x] = '/'; infopath[x+1] = '\0'; }
	sprintf(label, "               STATUS         %s", infopath);
        strcpy(owner, loginname);
	base = window_create(NULL, FRAME,
		WIN_X, 100,
		WIN_Y, 0,
		WIN_HEIGHT, ATTR_ROW(50),
		FRAME_ARGS,  argc, argv,
		FRAME_LABEL, label,
		0);
	screen = (Rect *) window_get(base, WIN_SCREEN_RECT);
	window_set(base, WIN_HEIGHT, screen->r_height - 10, 0);
	pixwins[0] = (Pixwin *) window_get(base, WIN_PIXWIN);
	pw = pixwins[0];
	pw_setcmsname(pw, "base");
	pw_putcolormap(pw, 0, 2, red, green, blue);
	create_button();  
	create_mess();
	create_summary();
	create_show();
	create_locate();
	window_main_loop(base);   
	exit(0);
}

create_button()
{
	void rm_proc(), set_user(), quit_proc();
	void log_disp(), ret_disp(), remove(), rt_proc();
        void del_proc(), loc_disp();

	button_panel = window_create(base, PANEL, 0);
/*
	(void) panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE, 
			panel_button_image(button_panel, "user", 0, 0),
			PANEL_LABEL_X, ATTR_COL(18),
			PANEL_NOTIFY_PROC, set_user,
			0);
*/
        x    = panel_button_image(button_panel, "log", 4, 0);
        y    = panel_button_image(button_panel, "ret", 4, 0);
	loc = panel_button_image(button_panel, "locate", 6, 0);
	don = panel_button_image(button_panel, "done",   6, 0);
	p1= panel_create_item(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE, y,
			PANEL_LABEL_X, ATTR_COL(18),
			PANEL_NOTIFY_PROC, ret_disp,
			0);          
	loc_don= panel_create_item(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE, loc,
			PANEL_NOTIFY_PROC, loc_disp,
			0);          
        if (vnmr)
        {
              (void) panel_create_item(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE,
                        panel_button_image(button_panel, "rt", 4, 0),
			PANEL_NOTIFY_PROC, rt_proc,
                        0);
	}

	(void) panel_create_item(button_panel, PANEL_BUTTON,
                        PANEL_LABEL_IMAGE,
			panel_button_image(button_panel, "delete", 0, 0),
                        PANEL_NOTIFY_PROC, del_proc,
			0);
	(void) panel_create_item(button_panel, PANEL_BUTTON,
			PANEL_LABEL_IMAGE,
			panel_button_image(button_panel, "quit", 4, 0),
			PANEL_NOTIFY_PROC, quit_proc,
			0); 
	window_fit_height(button_panel);
}


create_mess()
{
	mess_panel = window_create(base, PANEL,
			WIN_HEIGHT, ATTR_ROW(1),
			0);
  	user = panel_create_item(mess_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, "current user: ",
			PANEL_VALUE, owner,  
			PANEL_VALUE_DISPLAY_LENGTH, 30,
			0);
	err_mess = panel_create_item(mess_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "",
			0);
 }



create_summary()
{
	sum_panel = window_create(base, PANEL,
			WIN_HEIGHT, ATTR_ROW(6),
                        0);
        panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "QUEUED",
			PANEL_LABEL_Y, ATTR_ROW(1),
			PANEL_LABEL_X, ATTR_COL(15),
			0);
	panel_create_item(sum_panel, PANEL_MESSAGE, 
                        PANEL_LABEL_STRING, "ACTIVE", 
                        PANEL_LABEL_X, ATTR_COL(24),
			0);
	panel_create_item(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "COMPLETE",  
                        PANEL_LABEL_X, ATTR_COL(33), 
                        0); 
        panel_create_item(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "TOTAL",   
                        PANEL_LABEL_X, ATTR_COL(45),
			0);
        panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "SAMPLES    :",
			PANEL_LABEL_X, ATTR_COL(1),
			PANEL_LABEL_Y, ATTR_ROW(2),
			0);
	panel_create_item(sum_panel, PANEL_MESSAGE, 
                        PANEL_LABEL_STRING, "EXPERIMENTS:",
                        PANEL_LABEL_X, ATTR_COL(1),
                        PANEL_LABEL_Y, ATTR_ROW(3), 
                        0); 
        panel_create_item(sum_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "USERS      :",
			PANEL_LABEL_X, ATTR_COL(1),
			PANEL_LABEL_Y, ATTR_ROW(4),
			0);
        S_TOTAL = panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, ATTR_COL(46),
                        PANEL_LABEL_Y, ATTR_ROW(2),
			0);
	S_NEW = panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, ATTR_COL(17), 
                        PANEL_LABEL_Y, ATTR_ROW(2),
			0);
    	S_ACT = panel_create_item(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, ATTR_COL(26),  
                        PANEL_LABEL_Y, ATTR_ROW(2),
			0);
	S_COMP= panel_create_item(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, ATTR_COL(37),  
                        PANEL_LABEL_Y, ATTR_ROW(2),
			0);
	E_TOTAL = panel_create_item(sum_panel, PANEL_MESSAGE,    
                        PANEL_LABEL_STRING, "-", 
                        PANEL_LABEL_X, ATTR_COL(49), 
                        PANEL_LABEL_Y, ATTR_ROW(3), 
                        0);
	E_FREE = panel_create_item(sum_panel, PANEL_MESSAGE,  
                        PANEL_LABEL_STRING, "-", 
                        PANEL_LABEL_X, ATTR_COL(20),  
                        PANEL_LABEL_Y, ATTR_ROW(3), 
                        0);
	E_ACT = panel_create_item(sum_panel, PANEL_MESSAGE,   
                        PANEL_LABEL_STRING, "", 
                        PANEL_LABEL_X, ATTR_COL(26),  
                        PANEL_LABEL_Y, ATTR_ROW(3),
                        0);
	E_COMP= panel_create_item(sum_panel, PANEL_MESSAGE,    
                        PANEL_LABEL_STRING, "", 
                        PANEL_LABEL_X, ATTR_COL(37),
                        PANEL_LABEL_Y, ATTR_ROW(3),
                        0);
        USER_NO = panel_create_item(sum_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "",
			PANEL_LABEL_X, ATTR_COL(46),
                        PANEL_LABEL_Y, ATTR_ROW(4),
			0); 
        panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, ATTR_COL(20),
                        PANEL_LABEL_Y, ATTR_ROW(4),
                        0);
        panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, ATTR_COL(40),
                        PANEL_LABEL_Y, ATTR_ROW(4),
                        0);
        panel_create_item(sum_panel, PANEL_MESSAGE,
                        PANEL_LABEL_STRING, "-",
                        PANEL_LABEL_X, ATTR_COL(29),
                        PANEL_LABEL_Y, ATTR_ROW(4),
                        0);

        get_summary(); 
}


create_show()
{
	showsw = window_create(base, TEXTSW,
                        TEXTSW_FILE, 0,
			0);
	window_set(showsw, WIN_IGNORE_KBD_EVENT, WIN_ASCII_EVENTS, 0);   
	window_set(showsw, WIN_CONSUME_PICK_EVENTS, WIN_NO_EVENTS,
                        WIN_MOUSE_BUTTONS, 0,
                        0);   
	notify_interpose_event_func(showsw, take, NOTIFY_SAFE);   
        ret_disp();
}

create_locate()
{
void locate_done_proc();
void locateproc();
void setnext();
int  h;

	h = (int) window_get(sum_panel, WIN_HEIGHT);
/*
	frame_loc = window_create(NULL, FRAME,
		WIN_HEIGHT,	h,
		WIN_X,		100,
		WIN_BELOW,	mess_panel,
		WIN_SHOW,	FALSE,
		FRAME_SHOW_LABEL,	FALSE,
		FRAME_LABEL,	"locate window",
		0);
	pw = (Pixwin *) window_get(frame_loc, WIN_PIXWIN);
	pw_setcmsname(pw, "frame_loc");
	pw_putcolormap(pw, 0, 2, red, green, blue);
	panel_loc = window_create(frame_loc, PANEL, 0);
*/
	panel_loc = window_create(base, PANEL, 
		WIN_BELOW,	mess_panel,
		WIN_HEIGHT,     ATTR_ROW(6),
		WIN_SHOW,	FALSE,
		0);
	sample_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_COL(1),
		PANEL_LABEL_Y,		ATTR_ROW(0),
		PANEL_LABEL_STRING,	" SAMPLE#: ",
		PANEL_VALUE_DISPLAY_LENGTH,	27,
      /*  	PANEL_NOTIFY_PROC,	setnext, */
		0);

	user_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_COL(1),
		PANEL_LABEL_Y,		ATTR_ROW(1),
		PANEL_LABEL_STRING,	"    USER: ",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
       /* 	PANEL_NOTIFY_PROC,	setnext, */
		0);

	macro_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_COL(1),
		PANEL_LABEL_Y,		ATTR_ROW(2),
		PANEL_LABEL_STRING,	"   MACRO: ",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
       /* 	PANEL_NOTIFY_PROC,	setnext, */
		0);

	solvent_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_ROW(1),
		PANEL_LABEL_Y,		ATTR_ROW(3),
		PANEL_LABEL_STRING,	" SOLVENT: ",
		PANEL_VALUE_STORED_LENGTH,	20,
		PANEL_VALUE_DISPLAY_LENGTH,	20,
       /* 	PANEL_NOTIFY_PROC,	setnext, */
		0);

	text_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_ROW(1),
		PANEL_LABEL_Y,		ATTR_ROW(4),
		PANEL_LABEL_STRING,	"    TEXT: ",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
		PANEL_VALUE_STORED_LENGTH,	128,
	/* 	PANEL_NOTIFY_PROC,	setnext, */
		0);

	data_item = panel_create_item(panel_loc, PANEL_TEXT,
		PANEL_LABEL_X,		ATTR_ROW(1),
		PANEL_LABEL_Y,		ATTR_ROW(5),
		PANEL_LABEL_STRING,	"    DATA: ",
		PANEL_VALUE_DISPLAY_LENGTH,	65,
		PANEL_VALUE_STORED_LENGTH,	128,
	/* 	PANEL_NOTIFY_PROC,	setnext, */
		0);

	locate_button = panel_create_item(panel_loc, PANEL_BUTTON,
		PANEL_LABEL_IMAGE,
				 panel_button_image(panel_loc,"locate",0,0),
		PANEL_LABEL_X,		ATTR_COL(40),
		PANEL_LABEL_Y,		ATTR_ROW(0),
		PANEL_NOTIFY_PROC,	locateproc,
		0);
/* --- create full enter screen and fill if argument supplied --- */

	
}

void loc_disp()
{
/*
	window_set(frame_loc,	WIN_SHOW, TRUE, 0);
*/
	window_set(panel_loc,	WIN_SHOW, TRUE, 0);
	clear_err();
	panel_set(loc_don, 
		PANEL_LABEL_IMAGE,	don,
		PANEL_NOTIFY_PROC,	locate_done_proc,
		0);
}
void locate_done_proc()
{
	first_locate = TRUE;
	clear_err();
/*
	window_set(frame_loc,	WIN_SHOW, FALSE, 0);
*/
	window_set(panel_loc,	WIN_SHOW, FALSE, 0);
	panel_set(loc_don, 
		PANEL_LABEL_IMAGE,	loc,
		PANEL_NOTIFY_PROC,	loc_disp,
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
   samp = panel_get(sample_item,  PANEL_VALUE);
   user = panel_get(user_item,    PANEL_VALUE);
   macr = panel_get(macro_item,   PANEL_VALUE);
   solv = panel_get(solvent_item, PANEL_VALUE);
   text = panel_get(text_item,    PANEL_VALUE);
   data = panel_get(data_item,    PANEL_VALUE);
   while (*samp == ' ') samp++;   /* ignore leading spaces */
   if (*samp==0 && *user==0 && *macr==0 && *solv==0 && *text==0 && *data==0)
   {  window_set(panel_loc,   PANEL_CARET_ITEM, sample_item, 0);
      set_err("You must enter something");
      return;
   }
   clear_err();

   found = FALSE;
   if (first_locate) line_number = point = 0;
   else            { line_number = save_lineno; point = save_point; }
   window_get(showsw,TEXTSW_CONTENTS,point,buffer,1000);
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
          window_get(showsw,TEXTSW_CONTENTS,point,buffer,1000);
          line_number += NUM_LINES+1;
      }
   }
   if (found)
   {  save_lineno = line_number + NUM_LINES + 1;
      save_point = point + (int) (ptr - &buffer[0]);
      ptr = &buffer[0];
      while ( *ptr != '\n')  ptr++; ptr++;
      while ( *ptr != '-') ptr++;
      window_set(showsw, TEXTSW_INSERTION_POINT, point, 0);
      window_set(showsw, TEXTSW_INSERTION_POINT, point, 0);
      textsw_possibly_normalize(showsw, point+ptr-buffer);
      textsw_possibly_normalize(showsw, point);
      sample_select();
      first_locate = FALSE;
   }
   else
   {
      set_err("No (more) matching entry found");
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
	void sample_select();

	clear_err();
        notify_next_event_func(showsw, event, arg, type); 
        dest = 0;
	if (event_id(event) == MS_LEFT)
		if (event_is_up(event))         
		       sample_select();
   	window_set(showsw,
            WIN_KBD_FOCUS,          FALSE,
            0);
	return(NOTIFY_DONE);
}


/*****************************************************************************
*   This function will pass the fid pathname thruogh pipe to its parent      *
*   processor.                                                               *
*****************************************************************************/
void
rt_proc()
{
        char  filepath[160],rtFile[180],*ptr;

        if (dest == 0 || dataPath[0] == '\0')
	{
	     set_err("rt: unknown file");
             return;
	} 
        textsw_set_selection(showsw, infoAddr, infoAddr+strlen(dataPath),1);
	strcpy(filepath,infopath);
	ptr = strrchr(dataPath,'/'); 
        if (ptr==0L) ptr = &dataPath[0]; else ptr++;
	strcat(filepath,ptr);
	strcat(filepath,".fid");
        if (!search_file(filepath))
             return;
        sprintf(rtFile, "rt\(\'%s\'\)", filepath);
 	if (write(pipefd, rtFile, strlen(rtFile)) == -1)
		fprintf(stderr, "ERROR: pipe can not write\n");
	else
	{
		quit_proc();
/*                unlink(sumq);
		window_set(base, FRAME_NO_CONFIRM, TRUE, 0);
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
        mark = (Textsw_index) window_get(showsw, TEXTSW_INSERTION_POINT);
	if (mark < 0) return;
        top_line = (Textsw_index) window_get(showsw, TEXTSW_FIRST_LINE);
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
		     window_get(showsw, TEXTSW_CONTENTS, count, buf, 1);
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
                  window_get(showsw, TEXTSW_CONTENTS, count, buf, 1);
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
	      window_get(showsw, TEXTSW_CONTENTS, dest, buf, 1);
	}
        find = FALSE;
	while(!find && temp > row)
	{
		--temp;
                infoAddr = NULL;
		count = textsw_index_for_file_line(showsw, temp);
		window_get(showsw, TEXTSW_CONTENTS, count, buf, 130); 
                tokptr = buf;
		if ((token = strtok(tokptr, ": \t")) != NULL)
                {
			tokptr = NULL;
			if (strcmp(token, "DATA") == 0)
                        {                     
                              find = TRUE;
                              if((token = strtok(tokptr, " \t")) != NULL)
			      {
                                    i = token - buf;
 				    infoAddr = count + i;
			            sprintf(dataPath, "%s", token);
				    if (dataPath[0] == '\n')
					dataPath[0] = '\0';
				    for(i = 0; i <= strlen(dataPath); i++)
				    {
					if(dataPath[i] == '\0' || dataPath[i] =='\n')
					{    dataPath[i] = '\0';
					     break;
				        }
				    }
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

	clear_err();
        pid = getppid();
        sprintf(doneq, "%s/doneQ", infopath);
        sprintf(enterq, "%s/enterQ", infopath);
        sprintf(sumq, "/usr/tmp/ASMTXTSW.%d", pid);
        unlink(sumq);
	window_set(showsw, WIN_SHOW, FALSE, 0);   
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
                     fprintf(stderr, "ERROR: no such file or directory\n");
                     exit(0);
                 }
	} 
        if (!done_enter) system(cmd);
	panel_set(locate_button, PANEL_SHOW_ITEM, TRUE, 0);
        panel_set(p1, PANEL_LABEL_IMAGE, x, 0);
        panel_set(p1, PANEL_NOTIFY_PROC, log_disp, 0);
	clear_err();
        window_set(mess_panel, WIN_KBD_FOCUS, FALSE, 0);
        log = FALSE;
        dest = 0;
        dataPath[0] = '\0';
        window_set(showsw, TEXTSW_FILE, sumq, 0);
        window_set(showsw, WIN_SHOW, TRUE, 0);
}


/***************************************************************************
*   display the log file                                                   *
***************************************************************************/
void
log_disp()
{
        char  *ptr;

	clear_err();
        if ((dataPath[0] == '\0') || (infoAddr == NULL))
        {
		set_err("Unknown file path");
                return;
        }
	textsw_set_selection(showsw, infoAddr, infoAddr+strlen(dataPath),1);
	strcpy(logfile,infopath);
	ptr = strrchr(dataPath,'/'); 
	if (ptr==0L) ptr = &dataPath[0]; else ptr++;
	strcat(logfile,ptr);
	strcat(logfile,".fid/log");
   	if(search_file(logfile) == 0)
		return;
	panel_set(locate_button, PANEL_SHOW_ITEM, FALSE, 0);
	panel_set(p1, PANEL_LABEL_IMAGE, y, 0);
	panel_set(p1, PANEL_NOTIFY_PROC, ret_disp, 0);
	window_set(mess_panel, WIN_KBD_FOCUS, FALSE, 0);
	log = TRUE;
	dest = 0;
	textsw_save(showsw, 0, 0);
        window_set(showsw, TEXTSW_FILE, logfile, 0); 
        window_set(showsw, WIN_SHOW, TRUE, 0);
}



int 
search_file(file)
char *file;
{
	int	k;
        extern char     *sys_errlist[];

	clear_err();
	k = access(file, 4);
	if (k < 0)
	{
		set_err(sys_errlist[errno]);
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

	clear_err();
        if (dest == 0 || log)
		return;
        answer = 0;
	if ((dataPath[0] == '\0') || (infoAddr == NULL))
	{
		set_err("Unknown file path");
		return;
	}
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
           window_set(showsw, TEXTSW_INSERTION_POINT, start, 0);
	   while (del_more && lcount)
           {  textsw_edit(showsw, SELN_LEVEL_LINE, 1, 0);
              textsw_edit(showsw, SELN_LEVEL_FIRST, 1, 0);
              window_get(showsw, TEXTSW_CONTENTS, start, buffer, 15);
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
	char  file[120], cmd[130], *ptr;
        struct stat status;
	struct passwd *pwentry,*getpwuid(); 
	strcpy(file,infopath);
	ptr = strrchr(dataPath,'/');
	if (ptr==0) ptr=&dataPath[0]; else ptr++;
	strcat(file,ptr);
	strcat(file,".fid");
        if (access(file, 0) != 0)
        {
		set_err("No such file or directory");
 		return;
	}  
        if (stat(file, &status) == -1)
	{
                set_err( "error: file does not exist");
                return;
	}
        if ((pwentry = getpwuid(status.st_uid)) == NULL)
        {
                set_err("error: can not find owner");
		return;
	}
	if((strcmp("root", pwentry->pw_name) != 0) && (strcmp(loginname,pwentry->pw_name) != 0))
	{
                set_err("Not owner, permission denied");
		return;
	}
        sprintf(cmd, "rm -rf %s", file);
	system(cmd);
}

void
set_user()
{
	panel_set(err_mess,PANEL_LABEL_STRING, "", 0);
	panel_set(user, PANEL_VALUE,  "", 0);
	window_set(mess_panel, PANEL_CARET_ITEM, 
			user, 0);
	window_set(mess_panel, WIN_KBD_FOCUS, TRUE, 0);
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
   	panel_set(S_NEW, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%4d", s_total);
	panel_set(S_TOTAL, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%4d", s_comp);
        panel_set(S_COMP, PANEL_LABEL_STRING, label, 0);
	sprintf(label, "%4d", s_act);
	panel_set(S_ACT, PANEL_LABEL_STRING, label, 0);
	sprintf(label, "%4d", e_act);
 	panel_set(E_ACT, PANEL_LABEL_STRING, label, 0); 
        sprintf(label, "%4d", e_comp); 
        panel_set(E_COMP, PANEL_LABEL_STRING, label, 0);
        sprintf(label, "%4d", users);
        panel_set(USER_NO, PANEL_LABEL_STRING, label, 0);
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
        char *malloc();
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

clear_err()
{
        panel_set(err_mess,
		 PANEL_LABEL_STRING,	" ",
		 0);
}

set_err(str)
char    *str;
{
        panel_set(err_mess,
		 PANEL_LABEL_STRING,	str,
		 0);
        window_bell(mess_panel);
}
