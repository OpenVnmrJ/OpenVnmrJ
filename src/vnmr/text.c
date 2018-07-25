/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/*								*/
/* text			- display and return the text file	*/
/* text(string)		- set the text file to string		*/
/* ctext		- clear the text file			*/
/* menu			- activate GuidePath system		*/
/* newmenu('menuname')	- set menu name for later menu calls	*/
/* page			- change plotter page			*/
/* write		- write to devices			*/
/* pen			- select a pen or color for drawing	*/
/* move			- move to location for start of drawing	*/
/* draw			- draw to location (graphics or plotter)*/
/* mm			- convert position in Hz or PPM to mm	*/
/*								*/
/****************************************************************/

#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "allocate.h"
#include "buttons.h"
#include "data.h"
#include "disp.h"
#include "group.h"
#include "graphics.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"
#include "vfilesys.h"
#include "init_display.h"

#include <ctype.h>
#include <math.h>
#include <sys/types.h>         /* for chmod call */
#include <sys/stat.h>          /* for chmod call */

#define BUFSIZE     1024	/* maximum size of text to be passed back */
#define MAXTEXTLINE 80
#define MAX_NUM_BUTS    40
#define MAXLABELSIZE	60

#define CANT_OPEN		-2
#define MENU_FILE_TOO_LONG	-3

#ifdef  DEBUG
extern int Rflag;
#define RPRINT(level, str) \
	if (Rflag > level) fprintf(stderr,str)
#define RPRINT1(level, str, arg1) \
	if (Rflag > level) fprintf(stderr,str,arg1)
#else
#define RPRINT(level, str) 
#define RPRINT1(level, str, arg1) 
#endif

#define min(x,y) ((x)<(y)?(x):(y))

extern int           interuption;
extern int           working;
extern jmp_buf       jmpEnvironment;
extern double yppmm;
extern void plotpage(int n, char *filename, int retc, char *retv[]);
extern void getMouse(int event_type, int button_ask, int *retX, int *retY,
         int *b1, int *b2, int *b3);
extern int  net_write(char *netAddr, char *netPort, char *message);
extern void toggleBackingStore(int onoff);
extern void popBackingStore();
extern void autoRedisplay();
extern void Wclearerr();
extern void Wsetcommand();
extern int colorindex(char *colorname, int *index);
extern void currentDate(char* cstr, int len );
extern void Wactivate_vj_buttons(int number, PFV returnRoutine);
extern void executeFunction(int num);
extern int  setplotter();
extern int macroLoad(char *name, char *filepath);
extern void purgeOneMacro(char *n);
extern void dontSaveArgs();
#ifdef VNMRJ
extern int is_aip_window_opened();
extern void save_aip_menu_name(char *n);
#endif

static FILE* fptr = NULL;
int menuflag=0;
int menuon=0;
static int xorflag=0;
static char menuname[STR128] = "main";	/* currently active menu */
static char aipMenuname[STR128] = "main";
static char vjMenuname[STR128] = "main";
static char tmpStr[1024];
static int (*button_turnoff_routine)();
int button_turnoff_flag=0;
/* these have been made global so that nmr_draw and tnmr_draw can share */
static int curp = BLACK;     /*  current pen or color */
static int disp = 1;             /*  flag for graphics or plotter */
static int isAipMenu = 0;
static int vj_button_num = 0;
static int aip_button_num = 0;
static int aipMenuActive = 0;
static int vjMenuActive = 0;

typedef struct  {
     int  size;
     char *cmd;
   } CMD_NODE;

static CMD_NODE *saved_vj_cmds = NULL;
static CMD_NODE *saved_aip_cmds = NULL;
static char  aip_labels[MAX_NUM_BUTS][MAXLABELSIZE+1];

static int draw1(double xval, double yval, int disp, int curp, char argv0[]);
static int ovlytest(char *instring);
int drawbox1(float xval, float xval2, float yval, float yval2,
             int pen, int disp, int limit, int retc,
             float *x1_end, float *y2_end);

/**************************/
static void writetext(char *s, int max1, int retc)
/**************************/
{
  FILE *textfile;
  char tfilepath[MAXPATHL];
  char tline[MAXTEXTLINE+1];
  int i,c,j;
  if (retc==0)
    { Wshow_text();
      Walpha(W_SCROLL);
      Wscrprintf("\n");
    }
  strcpy(tfilepath,curexpdir);
#ifdef UNIX
  strcat(tfilepath,"/text");
#else
  strcat(tfilepath,"text");
#endif
  i = 0;
  j = 0;
  if ( (textfile=fopen(tfilepath,"r")) )
    { c = getc(textfile);
      while (c != EOF)
	{ if (j<max1-1)
	    s[j++] = c;
	  if (c == '\n')
	    { tline[i] = '\0';
              if (retc==0)
	        Wscrprintf("%s\n",tline);
	      i = 0;
	    }
          else
	    { if (i>=MAXTEXTLINE)
		{ tline[i] = '\0';
                  if (retc==0)
		    Wscrprintf("%s\n",tline);
		  i = 0;
		}
	      tline[i++] = c;
	    }
          c = getc(textfile);
        }
      if (retc==0)
        Wscrprintf("\n");
      fclose(textfile);
      s[j] = 0;
    }
  else Werrprintf("Unable to open text file");
}

/***************/
static void settext(char *s)
/***************/
{
  FILE *textfile;
  char tfilepath[MAXPATHL];
  int i;
  i=strlen(s);
  while (i>0)
    { i--;
      if (s[i]=='\\') s[i]='\n';
    }
  strcpy(tfilepath,curexpdir);
#ifdef UNIX
  strcat(tfilepath,"/text");
#else
  strcat(tfilepath,"text");
#endif
  if ( (textfile=fopen(tfilepath,"w+")) )
    { fprintf(textfile,"%s\n",s);
      fclose(textfile);
      chmod(tfilepath,0666);
    }
  else Werrprintf("Unable to open text file");
}

/***************************/
int text(int argc, char *argv[], int retc, char *retv[])
/***************************/
{ char s[BUFSIZE];
  Wturnoff_buttons();
  if (strcmp(argv[0],"ctext")==0)
  {
    settext("");
#ifdef VNMRJ
    appendvarlist("curexp");
#endif
  }
  else
    { if (argc>1)
        { if (argc>2)
	    { Werrprintf("usage -  text(string)");
	      ABORT;
            }
          settext(argv[1]);
        }
      writetext(s,BUFSIZE,retc);
#ifdef VNMRJ
      appendvarlist("curexp");
#endif
      if (retc>0)
	{ retv[0] = newString(s);
	}
    }
  RETURN;
}

void set_vnmrj_plot_params()
{
    char ptmp[MAXPATH];

    currentDate(ptmp, MAXPATH);
    if (P_setstring(CURRENT,"time_plotted",ptmp,1))
    {
      P_creatvar(CURRENT,"time_plotted",T_STRING);
      P_setstring(CURRENT,"time_plotted",ptmp,1);
    }
    P_setgroup(CURRENT,"time_plotted",G_PROCESSING);
#ifdef VNMRJ
    appendvarlist("time_plotted");
#endif
}

/****************************************************************/
/*								*/
/* page		- submit plot to plotter and change paper	*/
/*								*/
/****************************************************************/

/***************************/
int page(int argc, char *argv[], int retc, char *retv[])
/***************************/
{ char   *filename,nofln[2];
  int    npages;
  double v;
  // Wturnoff_buttons();
  setplotter();
  nofln[0]='\000';
  filename=nofln;
  npages = 1;
  if (argc>1)
  {  if ((argc==2)&&(isReal(argv[1])))
     {  v = (stringReal(argv[1]));
        if ((v<0.0)||(v>10.0))
        {  Werrprintf("usage - page(n), where n=0..10");
	   ABORT;
        }
        else npages = (int)v;
      }
      else if (argc==2)
           {
              filename=argv[1];
           }
           else
           {  Werrprintf("usage - page(n,<fln|'clear'>), where n=0..10");
              ABORT;
           }
  }
  set_vnmrj_plot_params();
  disp_status("PAGE    ");
  plotpage(npages,filename,retc,retv);
  disp_status("        ");
  RETURN;
}

/******************/
void set_turnoff_routine(int (*funct)())
/******************/
{
    button_turnoff_routine = funct;
    button_turnoff_flag = 1;
}

/************/
void turnoff_menu()
/************/
{
  if (button_turnoff_flag)  {
    button_turnoff_routine();
    button_turnoff_flag = 0;
    }
  menuon = 0;
}

/***************/
static void exec_menu(int button)
/***************/
{ char buffer[1024];
  register int i;
  int e;
  if ( (e=P_getstring(GLOBAL,"mstring",buffer,button,1022)) )
    { P_err(e,"mstring",":");
      Wturnoff_buttons();
      return;
    }
  i = 0;
  while (buffer[i])
    { if (buffer[i]=='`')
        buffer[i]='\'';
      else if (buffer[i]=='\n')
        buffer[i]=' ';
      i++;
    }
  strcat(buffer,"\n");
#ifdef VNMRJ
  writelineToVnmrJ("vbgcmd", buffer);
#endif
  execString(buffer);
}

/*-------------------------------------------------------------------------
     exec_vj_menu and exec_aip_menu replace executeFunction in terminal.c
-------------------------------------------------------------------------*/
void exec_vj_menu(int button)
{
    // CMD_NODE  *node;

    if (button > MAX_NUM_BUTS || button < 1)
        return;
    aipMenuActive = 0;
    vjMenuActive = 1;
    menuon = 1;
    Wactivate_vj_buttons(vj_button_num, turnoff_menu);
    executeFunction(button - 1);
    /*************
    Wclearerr();
    if (setjmp(jmpEnvironment) != 0) {
        Buttons_off();
        working = 0;
        interuption = 0;
        Wsetcommand();
        vjMenuActive = 0;
        return;
    }
    node = NULL;
    if (saved_vj_cmds != NULL)
        node = &saved_vj_cmds[button];
    working = 1;
    if (node == NULL || node->cmd == NULL)
        exec_menu(button);
    else
        execString(node->cmd);
    autoRedisplay(); // check if redisplay of anything needed
    Buttons_off();
    working = 0;
    interuption = 0;
    Wsetcommand();
    *************/
    vjMenuActive = 0;
}


void exec_aip_menu(int button)
{
    char gCmd[22];
    char gCmd2[22];

#ifdef VNMRJ
    CMD_NODE  *node;

    if (button > MAX_NUM_BUTS || button < 1)
        return;
    aipMenuActive = 1;
    vjMenuActive = 0;
    menuon = 1;
    Wactivate_vj_buttons(aip_button_num, turnoff_menu);
    Wclearerr();
    if (setjmp(jmpEnvironment) != 0) {
        Buttons_off(); /* Turn buttons off if necessary */
        working = 0;
        interuption = 0;
        Wsetcommand();
        aipMenuActive = 0;
        return;
    }
    gCmd[0] = '\0';
    Wgetgraphicsdisplay(gCmd,20);
    Wsetgraphicsdisplay("");
    node = NULL;
    if (saved_aip_cmds != NULL)
        node = &saved_aip_cmds[button];
    working = 1;
    if (node == NULL || node->cmd == NULL)
        exec_menu(button);
    else {
#ifdef VNMRJ
        writelineToVnmrJ("vbgcmd", node->cmd);
#endif
        execString(node->cmd);
    }
    autoRedisplay();
    Buttons_off();
    working = 0;
    interuption = 0;
    Wsetcommand();
    aipMenuActive = 0;
    gCmd2[0] = '\0';
    Wgetgraphicsdisplay(gCmd2,20);
    if (strlen(gCmd2) < 1)
        Wsetgraphicsdisplay(gCmd);
#endif
}

void clear_aip_menu()
{
     aip_button_num = 0;
}

static void restore_commands(int num, CMD_NODE *snode)
{
    int  n;
    CMD_NODE  *cnode;

    if (snode == NULL || num < 1)
        return;
    for (n = 1; n <= num; n++) {
         cnode = &snode[n];
         if (cnode->cmd != NULL)
             P_setstring(GLOBAL,"mstring",cnode->cmd,n);
    }
}

static void save_commands(int num, CMD_NODE *snode)
{
    int i, k, n;
    CMD_NODE  *cnode;

    if (snode == NULL)
         return;
    for (n = 1; n <= num; n++) {
        if (P_getstring(GLOBAL,"mstring",tmpStr,n,1020) == 0)
        {
            k = (int) strlen(tmpStr);
            cnode = &snode[n];
            if (cnode->size < k || (cnode->cmd == NULL)) {
                if (cnode->cmd != NULL) {
                     free(cnode->cmd);
                     cnode->cmd = NULL;
                }
                cnode->size = 0;
                cnode->cmd = (char *)malloc((size_t)(k + 4));
                if (cnode->cmd == NULL)
                    return;
                cnode->size = k;
            }
            i = 0;
            while (tmpStr[i]) {
                if (tmpStr[i]=='`')
                    tmpStr[i]='\'';
                else if (tmpStr[i]=='\n')
                    tmpStr[i]=' ';
                i++;
            }
            tmpStr[i]='\n';
            strcpy(cnode->cmd, tmpStr);
        }
    }
}

static void save_vj_commands(int num)
{
    int n;
    CMD_NODE  *node;

    if (num < 1)
       return;
    if (saved_vj_cmds == NULL) {
       saved_vj_cmds = (CMD_NODE *) malloc(sizeof(CMD_NODE) * MAX_NUM_BUTS);
       if (saved_vj_cmds == NULL)
           return;
       for (n = 0; n < MAX_NUM_BUTS; n++) {
            node = &saved_vj_cmds[n];
            node->size = 0;
            node->cmd = NULL;
       }
    }
    save_commands(num, saved_vj_cmds);
}

static void save_aip_commands(int num)
{
    int n;
    CMD_NODE  *node;

    if (num < 1)
       return;
    if (saved_aip_cmds == NULL) {
       saved_aip_cmds = (CMD_NODE *) malloc(sizeof(CMD_NODE) * MAX_NUM_BUTS);
       if (saved_aip_cmds == NULL)
           return;
       for (n = 0; n < MAX_NUM_BUTS; n++) {
            node = &saved_aip_cmds[n];
            node->size = 0;
            node->cmd = NULL;
       }
    }
    save_commands(num, saved_aip_cmds);
}



/********************************************************/
/*
    Sequence is:

	Input:	Menu name
	Call:	locate_menufile
	Output: Menu file

	Input:  Menu file
	Call:	read_menu_file
	Result: text stored in internal buffer

	Call:	exec_menu_buf
	Result: values for mlabel, mstring parameters

	Call:	display_menu
	Result: New menu is displayed
							*/

#define MAXMENULENGTH	10000
static char	menutext[MAXMENULENGTH];

#ifdef VNMRJ
int locate_iconfile(char *iconname )
{
	int found;
	char tmppath[ MAXPATH ];
	char tmpname[ MAXPATH ]={0};

	char *state_cmd=strchr(iconname,':');
	if(state_cmd!=NULL){
		int n=(int)(state_cmd-iconname);
		strncpy(tmpname,iconname,n-1);
	}
	else
		strcpy(tmpname,iconname);
	if ( (found = appdirFind(iconname, "iconlib", tmppath, NULL, R_OK)) )
	return( found );
	if (strcmp(tmpname,"noicon.gif")==0)
	{
		Werrprintf("Error: noicon.gif not found");
		return( -2 );
	}
	else
	{
		strcpy( tmpname, "noicon.gif" );
		return( locate_iconfile(tmpname) );
	}
}
#endif

int locate_menufile(char *menuname, char *menupath )
{
	int found;

#ifdef VNMRJ
        if ( (found = appdirFind(menuname, "menujlib", menupath, NULL, R_OK)) )
	    return( 0 );
#else

        if ( (found = appdirFind(menuname, "menulib", menupath, NULL, R_OK)) )
	    return( 0 );
#endif

	return( -1 );
}

int verify_menu_params()
{
	int	e;
	vInfo	label_info, string_info;

	if ( (e=P_getVarInfo(GLOBAL,"mlabel",&label_info)==-2) )
	  P_creatvar(GLOBAL,"mlabel",ST_STRING);
	if ( (e=P_getVarInfo(GLOBAL,"mstring",&string_info)==-2) )
	  P_creatvar(GLOBAL,"mstring",ST_STRING);
#ifdef VNMRJ
	{
	  vInfo   icon_info;
	  if ( (e=P_getVarInfo(GLOBAL,"micon",&icon_info)==-2) )
	  {
	    P_creatvar(GLOBAL,"micon",ST_STRING);
	    P_setprot(GLOBAL,"micon",16);
	    P_setgroup(GLOBAL,"micon",0);
	    P_setlimits(GLOBAL,"micon",0.0,0.0,0.0); /* remove from array */
	  }
	}
#endif
	return( 0 );
}

int read_menu_file(char *menufname )
{
	int	 c, i;
	FILE	*menufptr;

	menufptr = fopen( menufname, "r" );
	if (menufptr == NULL)
	  return( CANT_OPEN );
	i = 0;
#ifdef VNMRJ
	strcpy(menutext,"micon='noicon.gif' ");
	i += strlen(menutext);
#endif
	c = getc(menufptr);
	while (c != EOF) {
		if (i>=MAXMENULENGTH-2) {
			fclose(menufptr);
			return( MENU_FILE_TOO_LONG );
		}
		if (c=='\n') c=' ';
		menutext[i++] = c;
		c = getc(menufptr);
	}
	fclose(menufptr);
	menutext[i++]='\n';
	menutext[i++]=0;
	return( 0 );
}

int exec_menu_buf()
{
	return( execString( &menutext[ 0 ] ) );
}

/*  Using the `mstring' and `mlabel' arrayed string parameters,
    display the menu.  Use `micon' also if vnmrj.

    Important note:  This routine does NOT call Wturnoff_buttons.
    An active application should be able to display a new menu
    without turning itself off in the act of displaying the new menu.	*/

#ifndef VNMRJ

int display_menu(char *menuname, void (*turnoff_buttons_routine)() )
{
	int	arraysize, e, iter;
	char	label[MAX_NUM_BUTS][MAXLABELSIZE];
	char	*labelptr[MAX_NUM_BUTS];
        PFV     cmd[MAX_NUM_BUTS];
	vInfo	label_info, string_info;

	if (e=P_getVarInfo(GLOBAL,"mlabel",&label_info)) {
		P_err(e,"mlabel",":");
		return( -1 );
	}
	if (e=P_getVarInfo(GLOBAL,"mstring",&string_info)) {
		P_err(e,"mstring",":");
		return( -1 );
	}
	if (label_info.size != string_info.size) {
		Werrprintf("size of array mlabel and mstring are not equal");
		return( -1 );
	}
	arraysize = label_info.size;
	if (arraysize>MAX_NUM_BUTS) {
		Werrprintf("more than %d button labels are not allowed",
                            MAX_NUM_BUTS);
		return( -1 );
	}
	for (iter=0; iter<arraysize; iter++)
        {
	  *label[iter]=0;
           cmd[iter] = exec_menu;
        }
	for (iter=1; iter<=arraysize; iter++)
        {
	   if (e=P_getstring(GLOBAL,"mlabel",label[iter-1],iter,MAXLABELSIZE))
           {
		P_err(e,"mlabel",":");
		return( -1 );
	   }
           labelptr[iter-1] = label[iter-1];
	}

	Wactivate_buttons(arraysize, labelptr, cmd, turnoff_buttons_routine,
                          menuname);
	return( 0 );
}

#else


static int is_new_aip_menu(int num, char *labels[]) 
{
    int isNew, n;

    isNew = 0;

    if (num != aip_button_num)
       isNew = 1;
    else { 
       for (n = 0; n < num; n++) {
          if (strncmp(labels[n],aip_labels[n], MAXLABELSIZE) != 0 ) {
             isNew = 1;
             break;
          }
       }
    }
    if (isNew) {
        for (n = 0; n < num; n++) {
           strncpy(aip_labels[n], labels[n], MAXLABELSIZE);
        }
    }
    aip_button_num = num;
    return (isNew);
}

/**
static void clear_menu_command(int isAip) {
#ifdef VNMRJ
    if (isAip)
        writelineToVnmrJ("aipbuttoncmd","-1");  
    else
        writelineToVnmrJ("buttoncmd","-1");  
#endif
}

static void send_menu_command(int num, int isAip) {
#ifdef VNMRJ
    char  tmpCmd[1024];
    int   n;

    for (n = 1; n <= num; n++) {
        tmpCmd[0] = '\0';
	if ( P_getstring(GLOBAL,"mstring",tmpCmd,n,1020) == 0 ) {
            sprintf(tmpStr,"%d %s",n-1, tmpCmd);
            if (isAip)
                writelineToVnmrJ("aipbuttoncmd",tmpStr);  
            else
                writelineToVnmrJ("buttoncmd",tmpStr);  
         }
    }
#endif
}
**/

static int activate_aip_menu(int num, char *labels[]) 
{
    int n;

    if (is_new_aip_menu(num, labels) < 1)
        return(0);
#ifdef VNMRJ
    writelineToVnmrJ("aipbutton","-1 doneButtons");  
    for (n = 0; n < num; n++) {
        sprintf(tmpStr,"%d %s",n, labels[n]);
        writelineToVnmrJ("aipbutton",tmpStr);  
    }
      
    writelineToVnmrJ("aipbutton","-2 doneButtons");  
#endif
    save_aip_commands(num);
    return (1);
}

int
display_menu(char *menuname, void (*turnoff_buttons_routine)() )
{
	int	arraysize, e, iter, loc_icon;
	char	label[MAX_NUM_BUTS][MAXLABELSIZE * 2];
	char	*labelptr[MAX_NUM_BUTS];
	char    tmplabel[MAXLABELSIZE];
	char    tmpicon[MAXLABELSIZE];
        PFV     cmd[MAX_NUM_BUTS];
	vInfo	label_info, string_info, icon_info;

	if ( (e=P_getVarInfo(GLOBAL,"mlabel",&label_info)) )
        {
		P_err(e,"mlabel",":");
		return( -1 );
	}
	if ( (e=P_getVarInfo(GLOBAL,"mstring",&string_info)) )
        {
		P_err(e,"mstring",":");
		return( -1 );
	}
	if ( (e=P_getVarInfo(GLOBAL,"micon",&icon_info)) )
        {
		P_err(e,"micon",":");
		return( -1 );
	}
	if (label_info.size != string_info.size) {
		Werrprintf("size of array mlabel and mstring are not equal");
		return( -1 );
	}
	arraysize = label_info.size;
	if (arraysize>MAX_NUM_BUTS) {
		Werrprintf("more than %d button labels are not allowed",
                            MAX_NUM_BUTS);
		return( -1 );
	}
	for (iter=0; iter<arraysize; iter++)
        {
	  *label[iter]=0;
           cmd[iter] = exec_menu;
        }
	for (iter=1; iter<=arraysize; iter++)
        {
	   if ( (e=P_getstring(GLOBAL,"mlabel",tmplabel,iter,MAXLABELSIZE - 4)) )
           {
		P_err(e,"mlabel",":");
		return( -1 );
	   }
	   if (iter <= icon_info.size)
	   {
	      if ( (e=P_getstring(GLOBAL,"micon",tmpicon,iter,MAXLABELSIZE)) )
	      {
	           P_err(e,"micon",":");
	           strcpy(tmpicon,"noicon.gif");
	      }
	   }
	   else
	      strcpy(tmpicon,"noicon.gif");
	   loc_icon = locate_iconfile( tmpicon );
           sprintf( label[iter-1], "%d %s %s", loc_icon, tmpicon, tmplabel );
           labelptr[iter-1] = label[iter-1];
	}

        // clear_menu_command(isAipMenu);
        if (isAipMenu) {
	   activate_aip_menu(arraysize, labelptr);
           Wactivate_vj_buttons(arraysize, turnoff_menu);
           restore_commands(vj_button_num, saved_vj_cmds);
        }
        else {
	   Wactivate_buttons(arraysize, labelptr, cmd, turnoff_buttons_routine,
                          menuname);
           vj_button_num = arraysize;
           save_vj_commands(arraysize);
        }
        // send_menu_command(arraysize, isAipMenu);
	return( 0 );
}

#endif /* endif VNMRJ */

void setMenuName(const char *nm)
{
   if (strlen(nm)<(size_t)STR128)
      strcpy(menuname,nm);
}

/***********************/
int menu(int argc, char *argv[], int retc, char *retv[])
/***********************/
{
  char menufilepath[MAXPATH];
  char savemenuname[MAXSTR];
  char tmpmenu[MAXSTR];
  int ival;
  static char ifMenu[MAXSTR];
  static char altMenu[MAXSTR];
  extern int noGraph;

  if(noGraph) RETURN;

  RPRINT(1,"menu: starting\n");
  if (argc>3)
    { Werrprintf("usage - menu('menuname'), menu('off'), menu, newmenu('menuname')");
      ABORT;
    }
  else if (argc==3)
  {
     strcpy(ifMenu,argv[1]);
     strcpy(altMenu,argv[2]);
     RETURN;
  }
  else if (argc==2)
  {  if (strlen(argv[1])>(size_t)STR128)
        { Werrprintf("menu name too long");
          return 1;
        }
      if (strcmp(argv[1],"off")==0)
        { /* menu('off') command */
          menuflag = 0;
          menuon=0;
          Wturnoff_buttons();
          RETURN;
        }
      if (! strcmp(ifMenu,argv[1]) )
         strcpy(menuname,altMenu);
      else
         strcpy(menuname,argv[1]);
      ifMenu[0]= '\0';
  }
  else {  // argc==1
#ifdef VNMRJ
      if (is_aip_window_opened()) {
          if (aipMenuActive)
              strcpy(menuname, aipMenuname);
          else if (vjMenuActive)
              strcpy(menuname, vjMenuname);
      }
#endif
  }
  if (retc>0)
    retv[0] = newString(menuname);
  if (strcmp(argv[0],"newmenu")==0) /* only set the name for subsequent */
     RETURN;                      /* calls to menu */
  if (Bnmr)
     RETURN;
  // RPRINT(1,"menu: calling Wturnoff_buttons()\n");
  // Wturnoff_buttons();  // Wactivate_buttons func will do.
  // RPRINT(1,"menu: Wturnoff_buttons() done\n");
  verify_menu_params();

  ival = locate_menufile( &menuname[ 0 ], &menufilepath[ 0 ] );
  if (ival < 0) {
    Werrprintf("menu %s does not exist",menuname);
    menuflag=0;
    menuon=0;
    Wturnoff_buttons();
#ifdef VNMRJ
    RETURN;
#else
    ABORT;
#endif
  }
  isAipMenu = aipMenuActive;
#ifdef VNMRJ
  if (strncmp(menuname, "aip", 3) == 0) {
     if (is_aip_window_opened()) {
         isAipMenu = 1;
         save_aip_menu_name(menuname);
     }
  }
#endif

  sprintf(tmpmenu,"%s_menu",menuname);
  if (macroLoad(tmpmenu,menufilepath))
  {
    Werrprintf("cannot load menu file %s",menufilepath);
    menuflag=0;
    menuon=0;
    if (isAipMenu == 0)
        Wturnoff_buttons();
    isAipMenu = 0;
#ifdef VNMRJ
    RETURN;
#else
    ABORT;
#endif
  }

  menuon = 1;
  strcpy(savemenuname,menuname);
  strcat(tmpmenu,"\n");
  execString(tmpmenu);

  sprintf(tmpmenu,"%s_menu",menuname);
  purgeOneMacro(tmpmenu);
  RPRINT(1,"done\n");
  if (strcmp(savemenuname,menuname)!=0) RETURN;

  ival = display_menu( menuname, turnoff_menu );
  if (isAipMenu)
      strcpy(aipMenuname,menuname);
  else
      strcpy(vjMenuname,menuname);
  isAipMenu = 0;
  if (ival < 0) {
    menuon = 0;
#ifdef VNMRJ
    RETURN;
#else
    ABORT;
#endif
  }

  menuflag = 1;
  menuon = 1;
  RETURN;
}

#define T_PLOTTER 1
#define PRINTER 2
#define LINE3   3
#define ERROR   4
#define ALPHA   5
#define T_GRAPHICS 6
#define TFILE    7 
#define RESET    8 
#define NET      9 
#define MAXARGS 16 

/***************/
static void m_settext(int device, char filename[], char *s, int newline)
/***************/
{
  FILE *textfile;
  char tfilepath[MAXPATH];
  int i;
  i=strlen(s);
  if (newline)
    while (i>0)
    { i--;
      if (s[i]=='\\')
      {
         s[i]='\n';
         if ((i>0) && (s[i-1] == '\\'))
         {
            i--;
            s[i]=' ';
         }
      }
    }
  strcpy(tfilepath,filename);
  if (device==TFILE)
     textfile=fopen(tfilepath,"a");
  else
     textfile=fopen(tfilepath,"w");
  if (textfile)
    {
      if (device==TFILE)
      {  if (newline)
           fprintf(textfile,"%s\n",s);
         else
           fprintf(textfile,"%s",s);
      }
      fclose(textfile);
      chmod(tfilepath,0666);
    }
  else Werrprintf("Unable to open file %s",tfilepath);
}

/****************************/
int nmr_write(int argc, char *argv[], int retc, char *retv[])
/****************************/
{ int device,argnum,x,y,i,reverse,col;
  int newline = 1;
  int writeXor = 0;
  char text[4096];
  char filename[MAXPATH];
  char *textPtr;
  char *fmtPtr;
  char *tptr;
  char  tf[32], tstr[4096];
  char *tstrPtr;
  char *emess;
  double nppmm;
  int ovlymode=0;
  int textlen;

  emess="usage - write('%s'<,color><,'reverse'>,x,y,template,..)";
  reverse = 0;
  if (argc<3)
    { Werrprintf("usage - write(device,template,...)");
      ABORT;
    }
  argnum=1;
  if ( ! strcmp(argv[1],"") && ! strcmp(argv[2],"line3") )
  {
     device = LINE3;
     argnum++;
  }
  else if (strcmp(argv[1],"plotter")==0) device = T_PLOTTER;
  else if (strcmp(argv[1],"printer")==0) device = PRINTER;
  else if (strcmp(argv[1],"line3")==0)   device = LINE3;
  else if (strcmp(argv[1],"file")==0)    device = TFILE;
  else if (strcmp(argv[1],"fileline")==0){device= TFILE; newline = 0; }
  else if (strcmp(argv[1],"reset")==0)   device = RESET;
  else if (strcmp(argv[1],"error")==0)   device = ERROR;
  else if (strcmp(argv[1],"alpha")==0)   device = ALPHA;
  else if (strcmp(argv[1],"graphics")==0)device = T_GRAPHICS;
  else if (strcmp(argv[1],"net")==0)     device = NET;
  else
   { Werrprintf(
      "write: devices are plotter,printer,line3,file,reset(file),error,alpha,graphics");
     ABORT;
   }
  argnum++;
   if (device == NET) {
      if (argc < 5) {
        Werrprintf("usage - write('net',net_IP,net_port,template,...)");
	RETURN;
      }
   }

  if ((device==T_PLOTTER)||(device==T_GRAPHICS))
    { col = PARAM_COLOR;
      if (device==T_PLOTTER)
        { if (setplotter())
          {
            Winfoprintf("write('plotter',...) command ignored");
            if (retc > 0)
               retv[0] = realString((double) 1.0);
            RETURN;
          }
	  nppmm = ppmm;
	}
      else
        { if (setdisplay()) return 1;
	  nppmm = yppmm;
	}
      while ((argc>argnum)&&(!isReal(argv[argnum])))
        { if (strcmp(argv[argnum],"reverse")==0)
            reverse = 1;
          else if (strcmp(argv[argnum],"xor")==0)
            writeXor = 1;
          else if (colorindex(argv[argnum],&col))
            ;
          else if (!(ovlymode = ovlytest(argv[argnum])))
            { Werrprintf(emess,argv[1]);
              ABORT;
            }
          argnum++;
        }
      if ((argc<argnum+3)||(!isReal(argv[argnum]))||(!isReal(argv[argnum+1])))
        { Werrprintf(emess,argv[1]);
          ABORT;
        }
      x = (int)(stringReal(argv[argnum++])*ppmm);
      if (x<0) x=0;
      y = (int)(stringReal(argv[argnum++])*nppmm/ymultiplier)+ymin;
      // y = (int)(stringReal(argv[argnum++])*nppmm/ymultiplier);
      if (x>=mnumxpnts) x = mnumxpnts-1;
      if (y<0) y=0;
      // else if (y>=mnumypnts-ycharpixels) y = mnumypnts-ycharpixels-1;
      if (device==T_GRAPHICS)
        { Wshow_graphics();
          Wgmode();
	  if (writeXor)
             xormode();
	  else
             normalmode();
          if (ovlymode == 3)
	    {
              popBackingStore();
              toggleBackingStore(1);
              RETURN;
            }
          if (ovlymode == 2)
	    {
              popBackingStore();
              toggleBackingStore(0);
	    }
           if (ovlymode == 1)
	     {
               toggleBackingStore(0);
             }
        }

      amove(x,y);
      color(col);
      charsize(0.7);
      if (retc > 0)
         retv[0] = realString((double) (ycharpixels * ymultiplier) / nppmm);
    }
  if (device==TFILE || device==RESET)
        {
            if (verify_fname( argv[ argnum ] )) {
               Werrprintf( "file path '%s' not valid", argv[ argnum ] );
               ABORT;
            }

            strcpy(filename,argv[argnum]);
            if (device==TFILE)
            {
               argnum++;
               if (argc<4)
               {
                   Werrprintf("usage - write('file',filename,template,...)");
                   ABORT;
               }
            }
        }
 if (device == NET)
      argnum = 4;
  if (argc-argnum-1>MAXARGS)
    { Werrprintf("write: too many arguments");
      if (retc > 0)
	 retv[0] = newString("");
      ABORT;
    }

  /* This formatted output conversion copies the format string to the
   * output string.  When a % is encountered in the format string, that
   * conversion is performed via sprintf and the result is appended to the
   * output string.  This continues until the end of the format
   * string is reached. An alternative scheme of using vsprintf
   * was previously used. The final argument to vsprintf is a variable
   * of type va_list.  With older compilers, an array of string pointers
   * ( char *args[] ) could be substituted for a va_list.  This is not
   * correct and no longer works with newer compilers in 64-bit mode.
   */

  /* If format string is following by the string "noReformat" then
   * don't substitute for any % symbols in the format string
   */

  if ( (argnum < argc - 1) && ! strcmp(argv[argc-1],"noReformat") )
  {
     tptr = argv[argnum];
     textlen = 0;
  }
  else
  {
    /* Make sure the final output string is large enough */
    /* It takes the sum of the format string and all the arguments */
    /* This could still fail if large field width specifiers are used */
    textlen = strlen( argv[ argnum ] );
    for (i=argnum+1; i<argc; i++)
      textlen += strlen(argv[i]);
    if (textlen >= sizeof(text))
    {
       tptr = allocateWithId(textlen+1024,"nmrw");;
       tstrPtr = allocateWithId(textlen+1024,"nmrw");;
    }
    else
    {
       tptr = text;
       tstrPtr = tstr;
       textlen = 0;
    }
    /* s is the pointer to the format string */
    /* textPtr is the working pointer to the final output string */
    fmtPtr = argv[ argnum ];
    textPtr = tptr;
    ++argnum;
    while (*fmtPtr)
    {
     if (*fmtPtr == '%')
     {
        ++fmtPtr;
        if (*fmtPtr == '%')
        {
           /* The %% case */
           *textPtr++ = *fmtPtr++;
        }
        else
        {
           int charsLeft = sizeof(tf);
           char  *ptr2;

           if (argnum >= argc)
           { Werrprintf("write: not enough arguments as required in template");
              if (textlen)
                 releaseWithId("nmrw");
              if (retc > 0)
                 retv[0] = newString("");
              ABORT;
           }
           /* tf holds the format for a single conversion */
           /* tstr holds the result of that conversion, via sprintf */
           ptr2 = tf;
           *ptr2++ = '%';
           while ( ( ((*fmtPtr >='0') && (*fmtPtr <='9')) ||
                     (*fmtPtr =='.') || (*fmtPtr =='+')  ||
                     (*fmtPtr =='-') || (*fmtPtr =='#') ||
                     (*fmtPtr ==' ') ) &&
                   (charsLeft > 4) )
           {
              /* Collect flag, field width, and precision characters */
              *ptr2++ = *fmtPtr++;
              --charsLeft;
           }
           /* At this point *fmtPtr is the conversion character */
           *ptr2++ = *fmtPtr;
           *ptr2 = '\0';
           if (charsLeft <= 4)
           {
              Werrprintf("write: faulty format conversion <%s>", tf+1);
              if (textlen)
                 releaseWithId("nmrw");
              if (retc > 0)
                 retv[0] = newString("");
              ABORT;
           }
           switch ( *fmtPtr )
           {
              case 's' :
                          sprintf(tstrPtr, tf, argv[argnum]);
                          break;

              case 'c' :
	                  if (strlen(argv[argnum])!=1)
                          {
                             char *arg = argv[argnum];
                             if ((*arg == '\\') && isReal(arg+1) )
                             {
                               /* allow for non-printable characters to be written */
                               int val = (int)(stringReal(arg+1));
                               if ((val >= 0) && (val <= 127))
                               {
                                  sprintf(tstrPtr, tf, val);
                               }
                               else
                               {
                                  Werrprintf("write: individual character out of range (0 <= ch <= 127)");
                                  if (textlen)
                                     releaseWithId("nmrw");
                                  if (retc > 0)
	                             retv[0] = newString("");
                                  ABORT;
                               }
                             }
                             else
                             { Werrprintf("write: individual character argument expected");
                               if (textlen)
                                  releaseWithId("nmrw");
                               if (retc > 0)
	                          retv[0] = newString("");
                               ABORT;
                             }
                          }
                          else
	                  {
	                     sprintf(tstrPtr, tf, argv[argnum][0]);
                          }
                          break;

              case 'd' :
              case 'o' :
              case 'x' :
                          if (isReal(argv[argnum]))
                          {
                             sprintf(tstrPtr, tf, (int)stringReal(argv[argnum]));
                          }
                          else
                          { Werrprintf("write: argument %d must be numeric",argnum);
                            if (textlen)
                              releaseWithId("nmrw");
                            if (retc > 0)
	                       retv[0] = newString("");
                            ABORT;
                          }
                          break;

              case 'f' :
              case 'e' :
              case 'g' :
                          if (isReal(argv[argnum]))
                          { sprintf(tstrPtr, tf, stringReal(argv[argnum]));
                          }
                          else
                          { Werrprintf("write: argument %d must be numeric",argnum);
                            if (textlen)
                              releaseWithId("nmrw");
                            if (retc > 0)
	                       retv[0] = newString("");
                            ABORT;
                          }
                          break;

              default  :
                          { Werrprintf("write: illegal conversion %c in template",*fmtPtr);
                            if (textlen)
                              releaseWithId("nmrw");
                            if (retc > 0)
	                       retv[0] = newString("");
                            ABORT;
                          }
                          break;
           }
           /* append the conversion to the final string */
           /* two reasons for not using strcat.
            * 1. The result string textPtr is not yet null terminated.
            * 2. Need to update textPtr anyway.
            */
           ptr2 = tstrPtr;
           while ( *ptr2 )
              *textPtr++ = *ptr2++;
           ++fmtPtr;
           ++argnum;
        }
     }
     else
     {
        /* normal case of just copying characters from format string to output string */
        /* Escape characters are handled already by magical interpreter */
        *textPtr++ = *fmtPtr++;
     }
    }
    *textPtr = '\0';
    if (argnum != argc)
    { Werrprintf("write: more arguments than required in template");
       if (textlen)
       releaseWithId("nmrw");
       if (retc > 0)
          retv[0] = newString("");
       ABORT;
    }
  }

  switch (device)
    { case T_GRAPHICS:
      case T_PLOTTER: if (reverse)
                       dvstring(tptr);
                     else
                       dstring(tptr);
                     endgraphics();
                     break;
      case ALPHA:    Wshow_text();
		     Wscrprintf("%s\n",tptr);
                     break;
      case LINE3:    if (retc > 0)
	                retv[0] = newString(tptr);
                     else
                        Winfoprintf("%s",tptr);
                     break;
      case TFILE:    m_settext(device,filename,tptr,newline);
                     break;
      case RESET:    m_settext(device,filename,"",newline);
                     break;
      case ERROR:    Werrprintf("%s",tptr);
                     break;
      case NET:      {
                        int res = -1;
#ifdef VNMRJ
                        res = net_write(argv[2],argv[3], tptr);
#endif
                        if (retc > 0)
                        {
	                   retv[0] = intString( (res) ? 0 : 1);
                           if (retc > 1)
	                      retv[1] = intString(res);
                        }
                     }
                     break;
      case PRINTER:  Werrprintf("write(printer,..) not yet implemented");
                     ABORT;
    }
  toggleBackingStore(1); /* always */
  if (textlen)
     releaseWithId("nmrw");
  RETURN;
}

/* handle the overlay for write */
static int ovlytest(char *instring)
{
    if (!strcmp(instring,"newovly"))
         return(2);
     if (!strcmp(instring,"ovly"))
         return(1);
     if (!strcmp(instring,"ovlyC"))
         return(3);
     return(0);
}
/*
 *	argtest(argc,argv,argname)
 *	test whether argname is one of the arguments passed
 */
/***********************************/
static int argtest(int argc, char *argv[], char *argname)
/***********************************/
{
  int found = 0;

  while ((--argc) && !found)
    found = (strcmp(*++argv,argname) == 0);
  return(found);
}

extern void set_graph_flag(int flg); 
/*
 *  The pen, move, draw, box and hztomm commands.
 *  Note that pen and move simply store information.  Only the
 *  draw command actually sends data to the graphics screen or
 *  plotter.
 */
/*******************************/
int nmr_draw(int argc, char *argv[], int retc, char *retv[])
/*******************************/
{

   float x_end, y_end;
   float x1mm,x2mm,y1mm,y2mm;
   int argnum  = 1;
   int limit   = 1;
   int mm      = 0;
   int drawbox = 0;
   int mmscale = 0;
   int drawboxflag = 0;
   int realcount   = 0;


   if (strcmp(argv[0],"box") == 0)
   {
      drawbox = 1;
      if (argtest(argc,argv,"nolimit"))
      {
         limit = 0;
      }
   }
   else
      if (strcmp(argv[0],"hztomm") == 0)
      {
         if (argc <=1)
	   {  Werrprintf("Arguments needed. See man('hztomm')");
              ABORT;
           }
         mm = 1;
         mmscale = (argtest(argc,argv,"scale"));
	 drawboxflag = (argtest(argc,argv,"box"));
	 if (init2d(1,1))
         {
            Wscrprintf("Problem with init2d.\n");
            ABORT;
         }
         if (!d2flag && argc > 2)
  	 {
	    Werrprintf(
	    "'hztomm' with more than 1 argument must be preceded by 2d processing");
	    ABORT;
	 }
      }
   if (argtest(argc,argv,"graphics"))
   {
      disp = 1;
   }
   else if (argtest(argc,argv,"plotter") || argtest(argc,argv,"plot"))
   {
      disp = 0;
   }

   /* Check for xor or normal draw */
   if (argtest(argc,argv,"xor"))
   {
      xorflag = 1;
   }
   else if (argtest(argc,argv,"normal"))
   {
      xorflag = 0;
   }

   /* Check for screen only or normal draw */
   if (argtest(argc,argv,"newovly"))
   {
     /* Wscrprintf("newovly"); */
      popBackingStore();
      toggleBackingStore(0);
      xorflag = 0;
   }

   /* Check for screen only or normal draw */
   if (argtest(argc,argv,"ovly"))
   {
     /* make NO update call here */
      toggleBackingStore(0);
      xorflag = 0;
   }

   if (argtest(argc,argv,"ovlyC"))
       {
         popBackingStore();
         RETURN;
       }
   if (disp)
   {
      if (setdisplay())
         ABORT;
   }
   else
   {
      if (setplotter())
         ABORT;
      dontSaveArgs();
   }

   if (strcmp(argv[0],"pen") == 0)
   {
      int col = 0;
      int found = 0;

      while ((argc>argnum) && (!found))
         found = colorindex(argv[argnum++],&col);
      if (found)
      {
         curp = col;
      }
      else
      {
         Werrprintf("incorrect color supplied to %s",argv[0]);
         ABORT;
      }
   }
   else
   {
      float xval,yval,xval2,yval2;

      set_graph_flag(1);

      x1mm = xval2 = yval = yval2 = 0.0;
      while ((argc>argnum) && (!isReal(argv[argnum])))
         argnum++;
      if ((argc>argnum)&&(isReal(argv[argnum])))
      {
         xval = stringReal(argv[argnum++]);
         realcount++;
      }
      else
      {
         Werrprintf("Argument(s) required with '%s'",argv[0]);
         ABORT;
      }
      if ((argc>argnum)&&(isReal(argv[argnum])))
      {
         yval = stringReal(argv[argnum++]);
         realcount++;
      }
      else if (!mm || argc>argnum)
      {
         Werrprintf("Usage: %s(<device>,x,y)",argv[0]);
         ABORT;
      }
      if (!mm && !drawbox)
      {
         draw1(xval,yval,disp,curp,argv[0]);
         endgraphics();
         RETURN;
      }
      if ((argc>argnum)&&(isReal(argv[argnum])))
      {
         xval2 = yval;
         yval = stringReal(argv[argnum++]);
         realcount++;
      }
      else if (!mm || argc>argnum)
      {
         Werrprintf("incorrect argument supplied to %s",argv[0]);
         ABORT;
      }
      if ((argc>argnum)&&(isReal(argv[argnum])))
      {
         yval2 = stringReal(argv[argnum++]);
         realcount++;
      }
      else if (!mm || argc>argnum)
      {
         Werrprintf("incorrect argument supplied to %s",argv[0]);
         ABORT;
      }
      x_end=-2.0;
      y_end=-2.0;
      if (init2d_getchartparms(mm))
      {
         Wscrprintf("Initialization error");
         ABORT;
      }
      if (drawbox && realcount != 4)
      {
         Werrprintf(
            "Usage: box(<options,> left-edge,right-edge,lower-edge,upper-edge");
      }
      else
      {
         if (drawbox)
         {
            if (drawbox1(xval2,xval,yval2,yval,curp,disp,limit,retc,&x_end,&y_end))
            {
               ABORT;
            }
            else
            {
               if (retc >= 2)
               {
                  retv[0] = realString((double) x_end);
                  retv[1] = realString((double) y_end);
                  RETURN;
               }
            }
         }
      }
      if (mm)
       {
         if (realcount != 1 && realcount != 2 && realcount != 4)
         {
            Werrprintf("Incorrect number of arguments. See man('hztomm')");
         }
         else
         {
            double start,len,axis_scl;
  	    int    reversed;
            get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
	    if (mmscale)
 	    {  start /= axis_scl;
	       len   /= axis_scl;
	    }
            /* Wscrprintf("start= %g    len = %g axis_scl=%g\n",
             start,len,axis_scl); */
            if (realcount>=1)
 	       x1mm = wcmax - sc - (wc/len) * (xval - start);
            if (retc > 0)
	       retv[0] = realString((double) x1mm);
	    else if (realcount == 1)
	       Winfoprintf("%7.2f Hz = %7.2f mm",xval,x1mm);
            if (realcount == 2)
            {
               get_scale_pars(VERT,&start,&len,&axis_scl,&reversed);
	       if (mmscale)
 	       {  start /= axis_scl;
	          len   /= axis_scl;
	       }
 	       y1mm = wc2 + sc2 - (wc2/len) * (yval - start);
               if (retc > 1)
		  retv[1] = realString((double) y1mm);
	       else
	          Winfoprintf(
               "Horizontal: %5.2f Hz = %5.2f mm; Vertical: %5.2f Hz = %5.2f mm",
		     xval,x1mm,yval,y1mm);
	    }
            if (realcount == 4)
            {
 	       x2mm = wcmax - sc - (wc/len) * (xval2 - start);
               get_scale_pars(VERT,&start,&len,&axis_scl,&reversed);
	       if (mmscale)
 	       {  start /= axis_scl;
	          len   /= axis_scl;
	       }
 	       y1mm = wc2 + sc2 - (wc2/len) * (yval - start);
 	       y2mm = wc2 + sc2 - (wc2/len) * (yval2 - start);
	       if (drawboxflag)
	       {
		  float x1_end,y2_end;

                  if (drawbox1(x1mm,x2mm,y1mm,y2mm,RED,disp,1,retc,
			&x1_end,&y2_end))
                     ABORT;
                  if (retc > 0)
		     retv[0] = realString((double) x1_end);
                  if (retc > 1)
		     retv[1] = realString((double) y2_end);
	       }
 	       else
	       {
                  if (retc > 1)
		     retv[1] = realString((double) x2mm);
                  if (retc > 2)
		     retv[2] = realString((double) y1mm);
                  if (retc > 3)
		     retv[3] = realString((double) y2mm);
	          else
	          Winfoprintf(
	          "Horizontal limits (mm): %.2f, %.2f; Vertical limits (mm): %.2f, %.2f",
		     x1mm,x2mm,y1mm,y2mm);
	       }
	    }
	 }
      }
   }
   toggleBackingStore(1); /* always */
   RETURN;
}

int drawbox1(float xval, float xval2, float yval, float yval2,
             int pen, int disp, int limit, int retc,
             float *x1_end, float *y2_end)
{
   float x1,y1,x2,y2;

   x1=xval;
   x2=xval2;
   y1=yval;
   y2=yval2;
   *x1_end=0.0;
   *y2_end=0.0;
   if (limit &&
    (yval2 <= sc2 || xval2 <= wcmax-sc-wc || xval > wcmax-sc || yval > sc2+wc2))
   {
      ABORT; /* mark completely outside window */
   }
   if (limit)
   {
      if (xval < wcmax-sc-wc) x1 = wcmax-sc-wc;
      if (xval2 > wcmax-sc)   x2 = wcmax-sc;
      if (yval < sc2)         y1 = sc2;
      if (yval2 > sc2+wc2)    y2 = sc2+wc2;
   }

   draw1(x1,y2,disp,pen,"move");
   if (limit && (xval < wcmax-sc-wc))
      draw1(x1,y1,disp,pen,"move");
   else
      draw1(x1,y1,disp,pen,"draw");
   if (limit && (yval < sc2))
      draw1(x2,y1,disp,pen,"move");
   else
      draw1(x2,y1,disp,pen,"draw");
   if (limit && xval2 > wcmax-sc)
      draw1(x2,y2,disp,pen,"move");
   else
      draw1(x2,y2,disp,pen,"draw");
   if (limit && yval2 > sc2+wc2)
      draw1(x1,y2,disp,pen,"move");
   else
      draw1(x1,y2,disp,pen,"draw");
   endgraphics();
   if (retc >= 2)
      if (y2<sc2+wc2-7 || (limit && y2 < wc2max-4))
         {  *x1_end=x1;
            *y2_end=y2;
         }

   RETURN;
}

static int draw1(double xval, double yval, int disp, int curp, char argv0[])
{
   int    x,y;
   static int curx = 0;             /*  current x position */
   static int cury = 0;             /*  current y position */

// there is always a round up error of 1 pixel when converting real value to pixels.
// the following caluclation is fine, except the round up error is different from 
// how cursors and peak position are calculated 
/*
   if(disp) { // don't add 0.48 if not plot
      x = (int) ((double) (mnumxpnts - right_edge) * xval / wcmax);
      y = (int) ((double) (mnumypnts-ymin) * yval / wc2max) + ymin;
   } else {
      x = (int) ((double) (mnumxpnts - right_edge) * xval / wcmax + 0.48);
      y = (int) ((double) (mnumypnts-ymin) * yval / wc2max + 0.48) + ymin;
   }
*/

// so we'll do it the same way cursors and peak position are calculated
      int dfpntx  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
      int dnpntx  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
      int dfpnty = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
      int dnpnty = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
      x = (int) (dfpntx + (1.0 - (wcmax - sc - xval)/wc) * dnpntx);
      y = (int) (dfpnty + (1.0 - (wc2 + sc2 - yval)/wc2) * dnpnty);

      if (x<0) x=0;
      if (x>=mnumxpnts) x = mnumxpnts-1;
      if (y<0) y=0;
      else if (y>=mnumypnts) y = mnumypnts-1;
      if (strcmp(argv0,"move") == 0 || strcmp(argv0,"amove") == 0)
      {
         curx = x;
         cury = y;
         if (strcmp(argv0,"amove") == 0)
	    amove(curx,cury);
      }
      else if (strcmp(argv0,"draw") == 0)
      {
         if (disp)
         {
            Wshow_graphics();
            Wgmode();
	    if (xorflag)
            	xormode();
	    else
            	normalmode();
         }

         color(curp);
         amove(curx,cury);
         adraw(curx,cury);
         adraw(x,y);
         if (disp)
            normalmode();
         curx = x;
         cury = y;
         /*endgraphics();*/
      }
   RETURN;
}


/* draw1 but no clipping */
int draw2(double xval, double yval, int disp, int curp, char argv0[])
{
   int    x,y;
   static int curx = 0;             /*  current x position */
   static int cury = 0;             /*  current y position */

      x = (int) ((double) (mnumxpnts - right_edge) * xval / wcmax + 0.48);
      y = (int) ((double) (mnumypnts-ymin) * yval / wc2max + 0.48) + ymin;
      if (strcmp(argv0,"move") == 0 || strcmp(argv0,"amove") == 0)
      {
         curx = x;
         cury = y;
         if (strcmp(argv0,"amove") == 0)
	    amove(curx,cury);
      }
      else if (strcmp(argv0,"draw") == 0)
      {
         if (disp)
         {
            Wshow_graphics();
            Wgmode();
	    if (xorflag)
            	xormode();
	    else
            	normalmode();
         }
         color(curp);
         amove(curx,cury);
         adraw(curx,cury);
         adraw(x,y);
         if (disp)
            normalmode();
         curx = x;
         cury = y;
      }
   RETURN;
}
/*
 *  The tiltbox tiltboxm commands. 
 * 
 */
/*******************************/
int tnmr_draw(int argc, char *argv[], int retc, char *retv[])
/*******************************/
{
   /*  flag for graphics or plotter */
   float x_end, y_end;
   float xpivot,ypivot,xlen,ylen,theta,ylen2,dx1,dy1,dx2,dy2,dx3,dy3;
   float xarray[10];
   int argnum  = 1;
   int realcount   = 0;
   int nboxes;
   float ct,st;
   /* work out the arguments */
   /* disp is init'd to 1 */
   if (argtest(argc,argv,"plotter") || argtest(argc,argv,"plot"))
   {
      disp = 0;
   }

   /* Check for xor or normal draw */
   if (argtest(argc,argv,"xor"))
   {
      xorflag = 1;
   }
   else if (argtest(argc,argv,"normal"))
   {
      xorflag = 0;
   }

   /* Check for screen only or normal draw */
   if (argtest(argc,argv,"newovly"))
   {
     /* Wscrprintf("newovly"); */
      popBackingStore();
      toggleBackingStore(0);
      xorflag = 0;
   }

   /* Check for screen only or normal draw */
   if (argtest(argc,argv,"ovly"))
   {
     /* make NO update call here */
      toggleBackingStore(0);
      xorflag = 0;
   }

   if (argtest(argc,argv,"ovlyC"))
       {
         popBackingStore();
         RETURN;
       }
   if (disp)
   {
      if (setdisplay())
         ABORT;
   }
   else
   {
      if (setplotter())
         ABORT;
   }

   while ((argc>argnum) && (!isReal(argv[argnum])))
         argnum++;
   /* strip away strings to the front */
      while (((argc>argnum)&&(isReal(argv[argnum]))) && (realcount < 7))
      {
         xarray[realcount] = stringReal(argv[argnum++]);
         realcount++;
      }
      if ((realcount != 5) && (realcount != 7))
      {
         Werrprintf("5 or 7 argument(s) required with '%s'",argv[0]);
         ABORT;
      }


      x_end=-2.0;
      y_end=-2.0;

      if (init2d_getchartparms(0))
      {
         Wscrprintf("Initialization error");
         ABORT;
      }
      /* put the work in here */
      theta  = xarray[0];
      xpivot = xarray[1];
      ypivot = xarray[2];
      xlen   = xarray[3];
      ylen   = xarray[4];
      if (realcount == 5)
	{
           nboxes = 1;
           ylen2 = 0.0;
        }
      else
	{
          ylen2 = xarray[5];
          nboxes = xarray[6];
        }
      /* do the rotations */
      ct = cos((double) theta);
      st = sin((double) theta);

      dx1 = xlen*ct;
      dy1 = xlen*st;

      dx2 = -1.0*ylen*st;
      dy2 = ylen*ct;

      dx3 = -1.0*ylen2*st;
      dy3 = ylen2*ct;
      /* for fun */
      draw2(xpivot-2,ypivot,disp,curp,"move");
      draw2(xpivot,ypivot-2,disp,curp,"draw");
      draw2(xpivot+2,ypivot,disp,curp,"draw");
      draw2(xpivot,ypivot+2,disp,curp,"draw");
      draw2(xpivot-2,ypivot,disp,curp,"draw");
      /* pivots about center: xpivot,ypivot now box start..*/
      xpivot -= (dx1+nboxes*dx3-dx3+dx2)/2.0;
      ypivot -= (dy1+nboxes*dy3-dy3+dy2)/2.0;
      while (nboxes-- > 0)
	{
           draw2(xpivot,ypivot,disp,curp,"move");
           draw2(xpivot+dx1,ypivot+dy1,disp,curp,"draw");
	   draw2(xpivot+dx1+dx2,ypivot+dy1+dy2,disp,curp,"draw");
           draw2(xpivot+dx2,ypivot+dy2,disp,curp,"draw");
           draw2(xpivot,ypivot,disp,curp,"draw");
           /* now add the multi-box offset */
           xpivot += dx3;
           ypivot += dy3;
        }
      endgraphics();
      if (retc >= 2)
         {
            retv[0] = realString((double) xpivot);
            retv[1] = realString((double) ypivot);
            RETURN;
         }

   toggleBackingStore(1); /* always */
   RETURN;
}




int gin(int argc, char *argv[], int retc, char *retv[])
{
   int	button_ask;
   int	event_type;
   int  x, y;
   int  b1, b2, b3;
   int  rx;
   double xmm, ymm;

   button_ask  = 0;
   event_type  = 0;
   init2d(1,1);
   if (argc > 1)
   {
      if (strlen(argv[1]) >= 8)
      {
         if (argv[1][1] == '1')
      	    button_ask = 1;
      	 else if (argv[1][1] == '2')
      	    button_ask = 2;
      	 else if (argv[1][1] == '3')
      	    button_ask = 3;
      	 else if (argv[1][1] == 'a' || argv[1][1] == 'n')
      	    button_ask = 4;
         if (strcmp(&argv[1][3], "release") == 0)
      	    event_type = 1;
         else if (strcmp(&argv[1][3], "press") == 0)
      	    event_type = 2;
      }
   }
   getMouse(event_type, button_ask, &x, &y, &b1, &b2, &b3);
   rx = mnumxpnts - right_edge;
   if (x > rx)
      x = -1;
   if (y < 0 || y > mnumypnts)
      y = -10000;
   else
      y = mnumypnts - y;
   xmm = -1;
   ymm = y;
   if (x >= 0)
   {
      //xmm = (double) (rx - x) / (double) rx; // start from right
      xmm = (double) (x) / (double) rx; // start from left (the same as move and draw commands)
      xmm *= wcmax;
   }
   if (y >= 0)
   {
      ymm = (double) (y - ymin) / (double) (mnumypnts - ymin);
      ymm *= wc2max;
   }
   if (retc > 0)
   {
      retv[ 0 ] = realString(xmm);
      if (retc > 1)
      {
         retv[ 1 ] = realString(ymm);
         if (retc > 2)
         {
            retv[ 2 ] = intString(b1);
            if (retc > 3)
            {
               retv[ 3 ] = intString(b2);
               if (retc > 4)
               {
                  retv[ 4 ] = intString(b3);
               }
            }
         }
      }
   }
   else
   {
      Winfoprintf("x= %gmm y= %gmm B1= %d B2= %d B3= %d",xmm,ymm,b1,b2,b3);
   }
   RETURN;
}

int writefile(int argc, char *argv[], int retc, char *retv[])
{
    char  key[MAXSTR];
    if(argc > 1) strcpy(key,argv[1]);
    else strcpy(key,"");

    if(strcmp(key,"close") == 0) {
	if(fptr != NULL) fclose(fptr);
	fptr = NULL;
    } else if(strcmp(key,"open") == 0 && argc > 2) {
	if(fptr != NULL) fclose(fptr);
	fptr = fopen(argv[2], "w");

        if(retc > 0 && fptr != NULL) retv[0] = intString(1);
	else if(retc >0) retv[0] = intString(0);
    } else if(strcmp(key,"append") == 0 && argc > 2) {
	if(fptr != NULL) fclose(fptr);
	fptr = fopen(argv[2], "a");

        if(retc > 0 && fptr != NULL) retv[0] = intString(1);
	else if(retc >0) retv[0] = intString(0);
    } else if(strcmp(key,"line") == 0 && argc > 2) {
	if(fptr != NULL) fprintf(fptr, "%s\n",argv[2]);
    } else {
	Winfoprintf("Usage: writefile('open',path), writefile('append',path), writefile('line',str), or writefile('close')");
    }

    RETURN;
}

int readpars(int argc, char *argv[], int retc, char *retv[])
{
    int i, k, size;
    FILE* fp;
    char  path[MAXSTR], str[MAXSTR];
    char  buf[1024];
    char *strptr;

    argc--;
    argv++;
    if(argc > 0) {
	strcpy(path,argv[0]);
	strptr = path;
	strptr += strlen(path)-8;
	if(strcmp(strptr,"/procpar")) strcat(path,"/procpar");
        argc--;
        argv++;
    } else strcpy(path,"");

    if(argc <= 0 || strlen(path) == 0 || !(fp = fopen(path,"r"))) {
	for(i=0; i<retc; i++)
	 retv[i] = newString("");
        RETURN;
    }

    k = 0;
    while (k < retc && k < argc && fgets(buf,sizeof(buf),fp)) {

	strptr = strtok(buf, " ");

        for(i=0; i<argc; i++) {
	    if(strcmp(strptr, argv[i]) == 0) {
		fgets(buf,sizeof(buf),fp);
		buf[strlen(buf)-1] = '\0';
		strptr = strstr(buf," ");
		strptr++;
		strcpy(str, strptr);
	        size = atoi((char*) strtok(buf," "));
    	        if(size == 1) {
		    strptr = (char*) strtok(NULL," ");
		    if(strstr(strptr,"\"") != NULL) strptr = (char*) strtok(strptr,"\"");
    	        } else if(size > 1) {
		    strptr = str;
		} else {
		    strptr = NULL;
		}

		if(strptr == NULL)
		    retv[i] = newString("");
		else
		    retv[i] = newString(strptr);
		k++;
		break;
	    }
	}
    }
/*
    for(i=0; i<retc; i++) {
	fprintf(stderr,"getparams %d %s\n",i,retv[i]);
    }
*/
    fclose(fp);

    RETURN;
}

int readpar(int argc, char *argv[], int retc, char *retv[])
{
    int i, n, size;
    FILE* fp;
    char  path[MAXSTR], str[4*MAXSTR], name[4*MAXSTR];
    char  buf[4*MAXSTR], tok[4];
    char *strptr;
    int ind = 1;

    if(retc < 1) RETURN;

    argc--;
    argv++;
    if(argc > 0) {
        strcpy(path,argv[0]);
        strptr = path;
        strptr += strlen(path)-8;
        if(strcmp(strptr,"/procpar")) strcat(path,"/procpar");
        argc--;
        argv++;
    } else strcpy(path,"");

    if(argc <= 0 || strlen(path) == 0 || !(fp = fopen(path,"r"))) {
        RETURN;
    }

    strcpy(name, argv[0]);
    if(argc > 1) ind = atoi(argv[1]);

    n = 0;
    strcpy(tok,"");
    while(n == 0 && fgets(buf,sizeof(buf),fp)) {

        strptr = strtok(buf, " ");

        if(strcmp(strptr, name) == 0) {
           strcpy(str,"");
	   i = 0;
           while(fgets(buf,sizeof(buf),fp)) {
             buf[strlen(buf)-1] = '\0';
	     if(i > 0 && isdigit(buf[0])) break;
             strcat(str,buf);
	     i++;
           }
	   strcpy(name,str);
           strptr = strtok(str, " ");
           size = atoi((char*) strptr);

	   strcpy(tok," ");
           if(strstr(name,"\"") != NULL) {
		strcpy(tok,"\"");
                strptr = strstr(name," ");
                strptr++;
                strptr = strtok(strptr, "\"");
           } else strptr = (char*) strtok(NULL,tok);
	   if(strptr == NULL || size < ind) {
              n++;
	      retv[0] = newString("");
              break;
           }
           for(i=0; i<size; i++) {
               if(i+1 == ind && strptr == NULL) {
                 n++;
	         retv[0] = newString("");
                 break;
               } else if(i+1 == ind) {
                 n++;
		 retv[0] = newString(strptr);
                 break;
               }
               strptr = (char*) strtok(NULL,tok);
           }
        }
    }
    fclose(fp);

    if(n > 0 && retc > 1 && strcmp(tok,"\"")==0) {
        retv[1] = newString("str");
    } else if(n > 0 && retc > 1 && strcmp(tok," ")==0) {
     	retv[1] = newString("real");
    }
    RETURN;
}
