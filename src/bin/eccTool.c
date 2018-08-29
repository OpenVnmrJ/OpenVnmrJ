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
/*
**  Phil Hornung 
*/

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <sys/file.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>
#include <sunwindow/notify.h>
#include <suntool/scrollbar.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define C1	 2
#define C2	 2
#define C3	25
#define C4	25
#define ERROR -1

/**************************************************************************/
/* Frames  */
static Frame		base_frame;
/* Main Panel */
static Panel		main_panel;
static Panel_item	gradient_item,
			filesys_item,
			clear_button,
			shim_cycle,
			file_name_item,
			msg_item,
			msg1_item,
			duty_item,
			gain_item,
			slew_item,
			tc_item[5],
			amp_item[5];

static void  load_page();
static void  load_loc();
static void  clear_values(); 
static void  read_comp_file();
static void  setup_comp_file(); 
static void  write_comp_file(); 
static void  show_browse();
static void  hide_browse();
static void  quit();

static Notify_value text_respond();
char *strpos();
/********************************************************************/

struct adj
{	
    double max_,min_;
    char un[20],key[8];
};

struct range_t
{
    char xkey[8];
    int  page_no,
	 tc_no;
}
range_table[15] =
{   "tc_x_1",  0, 0,
    "tc_x_2",  0, 1,
    "tc_x_3",  0, 2,
    "tc_x_4",  0, 3,
    "tc_x_5",  0, 4,
    "tc_y_1",  1, 0,
    "tc_y_2",  1, 1,
    "tc_y_3",  1, 2,
    "tc_y_4",  1, 3,
    "tc_y_5",  1, 4,
    "tc_z_1",  2, 0,
    "tc_z_2",  2, 1,
    "tc_z_3",  2, 2,
    "tc_z_4",  2, 3,
    "tc_z_5",  2, 4
};

struct page
{
    struct adj  gain,slew,duty;
    struct adj  tcpage[5],amppage[5];
}
fullset[3];


Scrollbar scrl;
Pixfont	*pf;
int pfx,pfy;
static char vn[200],
	    vnx[200],
	    vng[200];
	    vnu[200];
	    lastread[200],
	    lastwrite[200],
	    lastsent[200];
int my_uid,my_gid;

struct _list_element 
{	
	char lename[64];   /* file name */
	int  interstat;
	int  yb,yt;
        struct _list_element *enext;
} *ebase,*ecurrent,*eselected;

#define LPOS	36
#define	VSPACE	 6
#define CHEIGHT	350
/* 
*/
/**********************************************************************************/
/* Frames  */
Frame		browse_frame;
Canvas		browse_canvas;
Menu		browse_menu;
Pixwin		*bpw;
void		browse_event();

short key__data[] = {
/* made by iconedit */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x1C00,0x3E00,0x77FF,
	0x63FF,0x7736,0x3E36,0x1C24,0x0000,0x0000,0x0000,0x0000
};
mpr_static(key_pr,16,16,1,key__data);

char porw;
int  numtcs,firstpage,lastpage;
char vnHost[256];
int  vnPort, vnPid;
/********************************************************************/
main(argc,argv)
int argc;
char **argv;
{
    int k;
    char *getenv();
    porw = 'w';
    numtcs = 5;
    firstpage = 0;
    lastpage  = 2;
    vnPort = -1;
    vnPid = -1;
    vnHost[0] = '\0';
    
    if (argc == 4)
    {
      strcpy(vnHost,argv[1]);
      vnPort = atoi(argv[2]);
      vnPid  = atoi(argv[3]);
    }
/*
    while (--argc > 0)
    {
      argv++;
      if (!strcmp(*argv,"-I"))
	 porw = 'w';
      if (!strcmp(*argv,"-P"))
      {
	 porw = 'p';
	 numtcs = 4;
      }
      if (!strcmp(*argv,"-1"))
      {
	 firstpage = 2;
	 lastpage = 2;
      }
    }
 */
    greg_it();
    my_uid = getuid();
    my_gid = getgid();
    strcpy(vn,getenv("vnmrsystem"));
    strcpy(vng,vn);
    strcat(vng,"/imaging/eddylib");
    strcpy(vnu,getenv("vnmruser"));
    strcat(vnu,"/imaging/eddylib");
    strcpy(vnx,vng);  /* global is default */
    pf = pf_default();
    pfx = pf->pf_defaultsize.x;
    pfy = pf->pf_defaultsize.y;
    base_frame =
	window_create(0, FRAME,FRAME_NO_CONFIRM,TRUE,
			FRAME_LABEL,"ECC Tool V7.x ",
        WIN_X,738,WIN_Y,538,WIN_WIDTH,414,0);

    /* init even if unused */
    init_page(0);
    init_page(1);
    init_page(2);
    init_main_panel();
    init_browser();
    window_fit_height(main_panel);
    window_fit_height(base_frame);
    set_page(0);
    window_main_loop(base_frame);
    exit(0);
}

init_page(i)
int i;
{   
      int j;
      struct page *pt_;
      pt_ = &fullset[i];
      if (porw == 'w')
      {
      strcpy(pt_->gain.un,"20.0");
      strcpy(pt_->slew.un,"20.0");
      strcpy(pt_->duty.un,"10.0");
      strcpy(pt_->gain.key,"gain");
      strcpy(pt_->slew.key,"slew");
      strcpy(pt_->duty.key,"dutycl");
      pt_->slew.max_ = 100.0;
      pt_->duty.max_ = 100.0;
      pt_->gain.max_ = 100.0;
      pt_->slew.min_ = 0.0;
      pt_->duty.min_ = 0.0;
      pt_->gain.min_ = 0.0;
      /* changed to reflect 3-20-90 rework */
      pt_->tcpage[0].max_ = 254.0;
      pt_->tcpage[0].min_ = 254.0/9.0;
      pt_->tcpage[1].max_ =   40.0;
      pt_->tcpage[1].min_ =   40.0/9.0;
      pt_->tcpage[2].max_ =   10.0;
      pt_->tcpage[2].min_ =   10.0/9.0;
      pt_->tcpage[3].max_ =   2.0;
      pt_->tcpage[3].min_ =   2.0/9.0;
      pt_->tcpage[4].max_ =   1.0;
      pt_->tcpage[4].min_ =   1.0/9.0;
      strcpy(pt_->tcpage[0].un,"254.0");
      strcpy(pt_->tcpage[1].un,"40.0");
      strcpy(pt_->tcpage[2].un,"10.0");
      strcpy(pt_->tcpage[3].un,"2.0");
      strcpy(pt_->tcpage[4].un,"1.0");
      }
      else
      {  /* pfg version */
      pt_->tcpage[0].max_ = 0.23;
      pt_->tcpage[0].min_ = 0.23/11.0;
      pt_->tcpage[1].max_ =   2.34;
      pt_->tcpage[1].min_ =   2.34/11.0;
      pt_->tcpage[2].max_ =   165.0;
      pt_->tcpage[2].min_ =   165.0/11.0;
      pt_->tcpage[3].max_ =   23.50;
      pt_->tcpage[3].min_ =   23.50/11.0;
      strcpy(pt_->tcpage[0].un,"0.230");
      strcpy(pt_->tcpage[1].un,"2.34");
      strcpy(pt_->tcpage[2].un,"165.0");
      strcpy(pt_->tcpage[3].un,"23.5");
      strcpy(pt_->tcpage[4].un,"1.0");
      }
      strcpy(pt_->amppage[0].un,"0.0");
      strcpy(pt_->amppage[1].un,"0.0");
      strcpy(pt_->amppage[2].un,"0.0");
      strcpy(pt_->amppage[3].un,"0.0");
      strcpy(pt_->amppage[4].un,"0.0");
      strcpy(pt_->tcpage[0].key,"tc1");
      strcpy(pt_->tcpage[1].key,"tc2");
      strcpy(pt_->tcpage[2].key,"tc3");
      strcpy(pt_->tcpage[3].key,"tc4");
      strcpy(pt_->tcpage[4].key,"tc5");
      strcpy(pt_->amppage[0].key,"amp1");
      strcpy(pt_->amppage[1].key,"amp2");
      strcpy(pt_->amppage[2].key,"amp3");
      strcpy(pt_->amppage[3].key,"amp4");
      strcpy(pt_->amppage[4].key,"amp5");
      for (j = 0; j < numtcs; j++)
      {
       pt_->amppage[j].max_ =100.0;
       pt_->amppage[j].min_ =  0.0;
      }
}

init_main_panel()
{
    int i, row_cnt;
    char tmp[32];

    main_panel = window_create(base_frame,PANEL,0,
        WIN_X,738,WIN_Y,538,WIN_WIDTH,414,0);

    if (vnPort != -1)
    (void)panel_create_item(main_panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(2),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(main_panel, "Setup", 0, (Pixfont *)0),
	PANEL_NOTIFY_PROC, setup_comp_file, 
	0);

    (void)panel_create_item(main_panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(9),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(main_panel, "Write", 0, (Pixfont *)0),
	PANEL_NOTIFY_PROC, write_comp_file,
	0);

    (void)panel_create_item(main_panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(16),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(main_panel, "Read", 0, (Pixfont *)0),
        PANEL_NOTIFY_PROC, read_comp_file, 
	0);


    clear_button = panel_create_item(main_panel,PANEL_BUTTON,
		    PANEL_ITEM_X, ATTR_COL(C3+4),
		    PANEL_ITEM_Y, ATTR_ROW(0),
		    PANEL_LABEL_IMAGE, 
			   panel_button_image(main_panel,"Clear",5,0),
		    PANEL_NOTIFY_PROC, clear_values, 
		    0);

    (void)panel_create_item(main_panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(36),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(main_panel, "Quit", 0, (Pixfont *)0),
        PANEL_NOTIFY_PROC, quit,
	0);

    (void)panel_create_item(main_panel, PANEL_BUTTON, 
	PANEL_ITEM_X, ATTR_COL(22),
	PANEL_ITEM_Y, ATTR_ROW(0),
	PANEL_LABEL_IMAGE, panel_button_image(main_panel, "Files", 0, (Pixfont *)0),
        PANEL_NOTIFY_PROC, show_browse,
	0);

    if (firstpage == 0)
    gradient_item = panel_create_item(main_panel,PANEL_CYCLE,
		    PANEL_LABEL_STRING, "Gradient:",
		    PANEL_CHOICE_STRINGS,"X","Y","Z", 0,
		    PANEL_VALUE,0,
		    PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		    PANEL_ITEM_X, ATTR_COL(C1),
		    PANEL_ITEM_Y, ATTR_ROW(4),
		    PANEL_NOTIFY_PROC, load_page,
		    0);
    else  
    gradient_item = panel_create_item(main_panel,PANEL_CYCLE,
		    PANEL_LABEL_STRING, "Gradient:",
		    PANEL_CHOICE_STRINGS,"Z", 0,
		    PANEL_VALUE,0,
		    PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		    PANEL_ITEM_X, ATTR_COL(C1),
		    PANEL_ITEM_Y, ATTR_ROW(4),
		    PANEL_NOTIFY_PROC, load_page,
		    0);
    filesys_item = panel_create_item(main_panel,PANEL_CYCLE,
                   PANEL_LABEL_STRING, "Source:",
                   PANEL_CHOICE_STRINGS,"Global", "Local",0,
                   PANEL_VALUE,0,
                   PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
                   PANEL_ITEM_X, ATTR_COL(C3),
                   PANEL_ITEM_Y, ATTR_ROW(4),
                   PANEL_NOTIFY_PROC, load_loc,
                   0);
 

    file_name_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C1),
	            PANEL_ITEM_Y, ATTR_ROW(3),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
	            PANEL_LABEL_STRING, "File name:",
	            0);

    row_cnt = 5;
    if (porw == 'w')
    {
    gain_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C1),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
	            PANEL_LABEL_STRING, "Gain:",
		    PANEL_NOTIFY_LEVEL, PANEL_ALL,
		    /*
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_NOTIFY_STRING, "\n\r\t?dDZHP",
		    */
		    PANEL_NOTIFY_PROC, text_respond, 
	            0);

    duty_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C3),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
	            PANEL_LABEL_STRING, "Duty cycle:",
		    PANEL_NOTIFY_LEVEL, PANEL_ALL,
		    PANEL_NOTIFY_PROC, text_respond, 
	            0);

    row_cnt++;
    slew_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C1),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
	            PANEL_LABEL_STRING, "Slew:",
		    PANEL_NOTIFY_LEVEL, PANEL_ALL,
		    PANEL_NOTIFY_PROC, text_respond, 
	            0);
    row_cnt++;
    }
    for (i = 0; i < numtcs; i++)
    {
    sprintf(tmp,"Tc %1d:",i+1);

    tc_item[i] =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C2),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt + i),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
		    PANEL_NOTIFY_LEVEL, PANEL_ALL,
		    PANEL_NOTIFY_PROC, text_respond, 
	            PANEL_LABEL_STRING,tmp,
	            0);
    
    sprintf(tmp,"Amp %1d:",i+1);
   
    amp_item[i] =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C4),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt + i),
	            PANEL_VALUE_DISPLAY_LENGTH,12,
		    PANEL_NOTIFY_LEVEL, PANEL_ALL,
		    PANEL_NOTIFY_PROC, text_respond, 
	            PANEL_LABEL_STRING, tmp,
	            0);

    }
    row_cnt += numtcs;
    msg_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C1),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt),
	            PANEL_VALUE_DISPLAY_LENGTH,36,
	            PANEL_LABEL_STRING, "",
	            0);

    row_cnt++;
    msg1_item =
    panel_create_item(main_panel, PANEL_TEXT, 
	            PANEL_ITEM_X, ATTR_COL(C1),
	            PANEL_ITEM_Y, ATTR_ROW(row_cnt),
	            PANEL_VALUE_DISPLAY_LENGTH,36,
	            PANEL_LABEL_STRING, "",
	            0);

    panel_set_value(msg_item,"");
    panel_set_value(msg1_item,"");
    window_fit(main_panel);
}

static void  load_page()
{
    int k,l;
    if (firstpage == 2) 
      return; /* not appropriate for 1 gradient system */
    k = (int) panel_get_value(gradient_item);
    /*  x = 0 , y = 1, z = 2 in a cycle */
    l = k + 2;
    l %= 3; /* make a three item cycle */
    capture_page(l);
    set_page(k);
}

capture_page(k)
int k;
{
    int i;
    struct page *pt_;
    pt_ = &fullset[k];
    if (porw == 'w')
    {
    strcpy(pt_->duty.un,(char *)panel_get_value(duty_item));
    strcpy(pt_->gain.un,(char *)panel_get_value(gain_item));
    strcpy(pt_->slew.un,(char *)panel_get_value(slew_item));
    }
    for (i = 0; i < numtcs; i++)
    {
      strcpy(pt_->tcpage[i].un,(char *)panel_get_value(tc_item[i]));
      strcpy(pt_->amppage[i].un,(char *)panel_get_value(amp_item[i]));
    }
}

set_page(k)
int k;
{
    int i;
    struct page *pt_;
    /* load the values from the selected page */
    pt_ = &fullset[k];
    if (porw == 'w')
    {
    panel_set_value(duty_item,pt_->duty.un);
    panel_set_value(gain_item,pt_->gain.un);
    panel_set_value(slew_item,pt_->slew.un);
    }
    for (i = 0; i < numtcs; i++)
    {
      panel_set_value(tc_item[i],pt_->tcpage[i].un);
      panel_set_value(amp_item[i],pt_->amppage[i].un);
    }
}

/*********************************************************/
static void  load_loc()
{
  int k,l;
  k = (int) panel_get_value(filesys_item);
  if (k == 0)
    strcpy(vnx,vng);
  else
    strcpy(vnx,vnu);
  /* if we are not displaying return */
  discard_files();
  if (!window_get(browse_frame,WIN_SHOW,0))
    return;
  /* if window is up, then discard and load */
  load_files(vnx);
}

/*********************************************************/

static void  setup_comp_file() 
{
  char file_name[60],bigstring[120],tstr[60];
  int iw,base_length;
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  strcpy(tstr,(char *)panel_get_value(file_name_item));
  strcat(file_name,tstr);
  if (strlen(file_name) <= base_length) 
  {
     panel_set_value(msg1_item,"Nothing to send!");
     return;
  }
  sprintf(bigstring,"%s/bin/eccsend %s /%s/acqqueue/eddy.out -S",vn,file_name,vn);
  tell_browser(tstr);
  iw = system(bigstring);
  if (iw == 0)
  {
    if ((strcmp(lastwrite,tstr) == 0) || (strcmp(tstr,lastread) == 0))
      panel_set_value(msg_item,"");
    else
      panel_set_value(msg_item,"not last read/written?");
    panel_set_value(msg1_item,"");
    strcpy(lastsent,tstr);
    iw = tellVnmr(tstr);
    if (iw != 0) 
       panel_set_value(msg1_item,"REMOTE SEND FAILED");
    return;
  }
  iw /= 256;
  if (iw == 1)
  {
    panel_set_value(msg_item,"Nothing sent - no eccsend!!");
    panel_set_value(msg1_item,"");
    return;
  }
  if (iw == 0x0fe)
  {
    panel_set_value(msg_item,"No such file to send!!");
    panel_set_value(msg1_item,"");
    return;
  }
  if (iw == 0x0fd)
  {
    panel_set_value(msg_item,"Could not write eddy.out!!");
    panel_set_value(msg1_item,"");
  }
}

static void  write_comp_file() 
{
  char file_name[60],tstr[40];
  int j,k,i,base_length,alarm;
  FILE *fds;
  k = (int) panel_get_value(gradient_item);
  alarm = 0;
  if (firstpage ==2)
    k = 2;
  capture_page(k);
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  strcpy(tstr,(char *)panel_get_value(file_name_item));
  strcat(file_name,tstr);
  if (strlen(file_name) <= base_length) 
  {
     panel_set_value(msg_item,"No file name??");
     return;
  }
  if ((fds = fopen(file_name,"w")) == NULL)
  {
    panel_set_value(msg_item,"could not write - file LOCKED?");
    return;
  }
  tell_browser(tstr);
  fprintf(fds,"\" output from eddy current window\n");
  for (i = firstpage ; i < lastpage+1; i++)
  {
    fprintf(fds,"\"time constant data for %c coil follows\n",'x'+i);
    fprintf(fds,"\"this data must reflect hardware changes\n");
    for (k = 0; k < numtcs; k++)
    {
    fprintf(fds,"tc_%c_%1d       %9.3f\n",'x'+i,k+1,fullset[i].tcpage[k].max_);
    }
    fprintf(fds,"\"data for %c coil follows\n%c:\n",'x'+i,'x'+i);
    if (porw == 'w')
    {
      alarm += check_value(fullset[i].gain);
      fprintf(fds,"%-7s   %8s\n",fullset[i].gain.key,fullset[i].gain.un);
      alarm += check_value(fullset[i].duty);
      fprintf(fds,"%-7s   %8s\n",fullset[i].duty.key,fullset[i].duty.un);
      alarm += check_value(fullset[i].slew);
      fprintf(fds,"%-7s   %8s\n",fullset[i].slew.key,fullset[i].slew.un);
    }
    for (j = 0; j < numtcs; j++)
    {
       alarm += check_value(fullset[i].tcpage[j]);
       fprintf(fds,"%-7s  %8s\n",fullset[i].tcpage[j].key,fullset[i].tcpage[j].un);
       alarm += check_value(fullset[i].amppage[j]);
       fprintf(fds,"%-7s  %8s\n",fullset[i].amppage[j].key,fullset[i].amppage[j].un);
    }
  }
  fclose(fds);
  strcpy(lastwrite,tstr);
  if (alarm == 0)
    panel_set_value(msg_item,"");
  panel_set_value(msg1_item,"");
}

tellVnmr(tstr)
{
    char msg[256];

    sprintf(msg,"\neddysend('%s',1)\n",tstr);
    return ( sendasync(vnHost,vnPort,vnPid,msg) == ERROR );
}

tell_browser(name_m)
char *name_m;
{
    struct _list_element *ec;
    int len;
    if (!window_get(browse_frame,WIN_SHOW,0)) 
       return;
    ec = ebase;
    while ((ec != NULL) && (strcmp(ec->lename,name_m)))
      ec = ec->enext;
    if (ec != NULL)
    {
      /* highlight & save into the other window */
      if (ec != eselected)
      {
        len = strlen(ec->lename);
        pw_writebackground(bpw,LPOS-2,ec->yt-2,len*pfx+20,pfy+5,PIX_NOT(PIX_DST));
      }
      if ((eselected != NULL) && (eselected != ec))
      {
	/* de-highlight */
        len = strlen(eselected->lename);
        pw_writebackground(bpw,LPOS-2,eselected->yt-2,len*pfx+20,pfy+5,PIX_NOT(PIX_DST));
      }
      eselected=ec;
    }
    else
    {
      load_files(vnx);
    }
}

static struct page *pt_;

static void  read_comp_file() 
{
  char file_name[60],tstr[60],ustr[80];
  int k,base_length;
  FILE *frd;
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  strcpy(tstr,(char *)panel_get_value(file_name_item));
  strcat(file_name,tstr);
  if (strlen(file_name) < base_length) 
  {
     panel_set_value(msg1_item,"No name to read!");
     return;
  }
  if ((frd = fopen(file_name,"r")) == NULL)
  {
    panel_set_value(msg1_item,"Could not open file!");
    return;
  }
  pt_ = &fullset[0];
  while (getline(frd,ustr,80) != EOF)
  { 
     match_line(ustr);
  }
  fclose(frd);
  tell_browser(tstr);
  strcpy(lastread,tstr);
  k = (int) panel_get_value(gradient_item) + firstpage;
  set_page(k);
  panel_set_value(msg_item,"");
  panel_set_value(msg1_item,"");
}

int qerror = 0;

match_line(xstr)
char *xstr;
{   
    char *yy,*uu,*parse_adj(),*parse_range();
    int i;
    yy = xstr;
    /* skip comments */
    if ((*yy == '"') || (*yy == '*') || (*yy == '#'))
        return;
    if ((yy = parse_range(yy)) == NULL) return;
    if ((uu = strpos(yy,"x:")) != 0)
    {
      pt_ = &fullset[0];
      yy = uu +2;
    }
    if (*yy == '\0') return;
    if ((uu = strpos(yy,"y:")) != 0)
    {
      pt_ = &fullset[1];
      yy = uu +2;
    }
    if (*yy == '\0') return;
    if ((uu = strpos(yy,"z:")) != 0)
    {
      pt_ = &fullset[2];
      yy = uu +2;
    }
    /* */
    if (*yy == '\0') return;
    if ((porw == 'w') && ((yy = parse_adj(&(pt_->gain),yy)) == NULL)) return;
    if ((porw == 'w') && ((yy = parse_adj(&(pt_->duty),yy)) == NULL)) return;
    if ((porw == 'w') && ((yy = parse_adj(&(pt_->slew),yy)) == NULL)) return;
    for (i = 0; i < numtcs; i++)
    {
      if ((yy = parse_adj(&(pt_->tcpage[i]),yy)) == NULL) return;
      if ((yy = parse_adj(&(pt_->amppage[i]),yy)) == NULL) return;
    }
}

char *parse_adj(curadj,yy)
struct adj *curadj;
char *yy;
{   char *uu,tmp[20];
    double dbl;
    int k;
    uu = yy;
    if ((uu = strpos(yy,curadj->key)) != 0)
    {
	uu += strlen(curadj->key);
	k = sscanf(uu,"%F",&dbl);
	if ((k == 1) && (dbl >= curadj->min_) && (dbl <= curadj->max_))
        {
	  sprintf(tmp,"%-6.2f",dbl);
          strcpy(curadj->un,tmp);
        }
	else 
	  qerror++;
	uu = index(uu,',');
	if (uu != NULL)  uu++;
    }
    else
	uu = yy;
    return(uu);
}

char *parse_range(yy)
char *yy;
{
   int i,j,k,l;
   char *uu;
   double dbl;
   /* allow no range change for pfg */
   if (porw == 'p')
     return(yy);
   i = 0;
   while ((i < 15) && ((uu = strpos(yy,range_table[i].xkey)) == 0))
     i++;
   if (i > 14)
     return(yy);
   j = range_table[i].page_no; 
   k = range_table[i].tc_no; 
   uu += strlen(range_table[i].xkey);
   l = sscanf(uu,"%F",&dbl);
   if (l == 1)
   {
      fullset[j].tcpage[k].max_ = dbl;
      fullset[j].tcpage[k].min_ = dbl/9.0;
   }
   return(uu);
}

/********************************************************/

static void clear_values()   
{  
   int k;
   k = (int) panel_get_value(gradient_item);
   init_page(k);
   set_page(k);
  panel_set_value(msg_item,"");
  panel_set_value(msg1_item,"");
}

static void
quit()
{
	window_destroy(base_frame);
} 

#define TRUE 1
#define FALSE 0


static Notify_value
text_respond(item, event)
Panel_item	item;
Event		*event;
{
    char msg[40],this_str[100];
    struct adj xx;
    double z;
    int k,inrange,cmd,tcflag;
    cmd = event->ie_code; 
    tcflag = 0;
    for (k = 0; k < numtcs; k++)
      tcflag = tcflag || (item == tc_item[k]); 
    find_adj(&xx,item);
    switch (cmd)
    {
       case 43:  return(PANEL_NONE); break; /* a plus sign */
       case 46:  /* a decimal point */
       case 48:  /* digits 0 to 9 */
       case 49: case 50: case 51: case 52:
       case 53: case 54: case 55: case 56:
       case 57: 
                strcpy(this_str,(char *)panel_get_value(item));
	        /* don't core dump and ignore bogus input */
		if (strlen(this_str) > 10)
		   return(PANEL_NONE); 
       case 127: return(PANEL_INSERT); break; /* and del key */
       case 68: /* D */
         sprintf(this_str,"%-7.3f",xx.min_+0.001);
         panel_set_value(item,this_str);
         return(PANEL_NONE);
	 break;
    }
    if (!tcflag) 
    {
     switch (cmd)
     {
       /* Z */
       case 90: strcpy(this_str,"0.0");
                panel_set_value(item,this_str);
                return(PANEL_NONE);
		break;
       /* P F */
       case 80: case 70: 
                strcpy(this_str,"100.0");
                panel_set_value(item,this_str);
                return(PANEL_NONE);
		break;
       /* H */
       case 72: 
                strcpy(this_str," 50.0");
                panel_set_value(item,this_str);
                return(PANEL_NONE);
		break;
     }
    }
    else
    {
     /* TC's go here */
     switch (cmd)
     {
       /* F L */
       case 72: case 76: 
                sprintf(this_str,"%-6.2f",xx.max_-0.001);
                panel_set_value(item,this_str);
                return(PANEL_NONE);
		break;
     }
    }
    /******************************************/
    strcpy(this_str,(char *)panel_get_value(item));
    k = sscanf(this_str,"%F",&z);
    /* k != 1 nothing to convert bogus value */
    if (k != 1) 
       inrange = FALSE;
    else
       inrange = ((xx.min_ <= z) && (xx.max_ >= z));
    sprintf(msg,"range is [%5.1f to %5.1f] :%5.1f",xx.min_,xx.max_,z);
    /* ? typed */
    if (cmd == 63)
    {
       panel_set_value(msg1_item,msg);
       return(PANEL_NONE);
    }
    if (inrange == FALSE)
    {
       panel_set_value(msg_item,"value is in ERROR!");
       panel_set_value(msg1_item,msg);
       return(PANEL_NONE);
    }
    panel_set_value(msg_item,"");
    panel_set_value(msg1_item,"");
    if ((cmd > 32) && (cmd < 127))
       return(PANEL_NONE);
    if (event->ie_shiftmask != 0)
       return(PANEL_PREVIOUS);
    else
       return(PANEL_NEXT);
}


find_adj(yy,itm)
struct adj *yy;
Panel_item itm;
{
     int i,k;
     struct page *pt_;
     k = (int) panel_get_value(gradient_item);
     pt_ = &fullset[k];
     if ((porw == 'w') && (itm == duty_item))
        *yy = pt_->duty;
     if ((porw == 'w') && (itm == gain_item))
        *yy = pt_->gain;
     if ((porw == 'w') && (itm == slew_item))
        *yy = pt_->slew;
     for (i = 0; i < numtcs; i++)
     {
       if (itm == tc_item[i])
         *yy = pt_->tcpage[i];
       if (itm == amp_item[i])
         *yy = pt_->amppage[i];
     }
}


/* 
phil's utilities 
*/
int check_value(uu)
struct adj uu;
{
   double zz;
   int k,l;
   k = sscanf(uu.un,"%F",&zz);
   l = 0;
   if (!((k == 1) && (zz >= uu.min_) && (zz <= uu.max_)))
   {
    panel_set_value(msg_item,"Value is out of range!");
    l = 1;
   }
   return(l); 
}

int getline(fd,targ,mxline)
FILE *fd;
char *targ;
int mxline;
{   
   char uu,*tt;
   int cnt,tmp;
   tt = targ;
   cnt = 0;
   do
   {  
    tmp = getc(fd);
    uu = (char) tmp;
   if (isupper(uu))
      uu = tolower(uu);
    if (cnt < mxline)
      *tt++ = uu;    /* read but discard chars after mxline */
    cnt++;
   }
   while ((tmp != EOF) && (uu != '\n'));
   if (cnt >= mxline)
      targ[mxline -1] = '\0';
   else 
      *--tt = '\0';	/* ensure a null terminated list */
   if (tmp == EOF)
     return(EOF);
   else
     return(cnt);
}

char *strpos(target,search)
char *target,*search;
{
   char *base,*pntr,*pntr1,*pntr2;
   base = target;
   if (target == NULL) return(NULL);
   if (search == NULL) return(NULL);
   while ((pntr = index(base,*search)) != 0) 
   { /* first char matches */
     pntr1 = pntr;
     pntr2 = search;
     while ((*pntr2 != '\0') && (*pntr1 != '\0') && (*pntr1++ == *pntr2))
      pntr2++;
     if (*pntr2 == '\0') return pntr;
     base = pntr + 1;
   } 
   return NULL;
}



init_browser()
{
    browse_frame = window_create(
			base_frame, FRAME,FRAME_NO_CONFIRM,TRUE,
			FRAME_LABEL,"EccTool Files",
			FRAME_SHOW_LABEL,TRUE,
			0);
    scrl = scrollbar_create(0);
    browse_canvas = 
        window_create(browse_frame, CANVAS, 
	    WIN_CONSUME_PICK_EVENT,	LOC_DRAG, 
	    WIN_VERTICAL_SCROLLBAR,     scrl,
	    CANVAS_AUTO_SHRINK,		FALSE,
	    CANVAS_AUTO_EXPAND,		TRUE,
	    CANVAS_RETAINED,		FALSE,
	    CANVAS_HEIGHT,		CHEIGHT,
	    WIN_WIDTH,			410,
	    WIN_HEIGHT,			CHEIGHT, 
	    WIN_Y,			0,
	    WIN_EVENT_PROC, 		browse_event, 
	    0); 
    bpw = canvas_pixwin(browse_canvas);  
    window_set(browse_canvas,CANVAS_RETAINED,TRUE,0);

    browse_menu = 
	     menu_create(MENU_STRING_ITEM,"-- Help Menu --",0,
			 MENU_STRING_ITEM,"Left   reads",0,
			 MENU_STRING_ITEM,"Middle lock/unlock",0,
			 /*MENU_STRING_ITEM,"Shift left copies name only",0, */
			 MENU_STRING_ITEM,"Key indicates locked file",0,
			 MENU_ACTION_ITEM,"Done",hide_browse,
			 0);
			    
    window_fit(browse_frame); 
}

static void
show_browse(item, event)
Panel_item	item;
Event		*event;
{
    char emsg[64];
    if (load_files(vnx) > 0)
      (void)window_set(browse_frame, WIN_SHOW, TRUE, 0);
    else
    {
      sprintf(emsg,"No access %s\n",vnx);
      panel_set_value(msg1_item,emsg);
    }
}

static void
hide_browse(item, event)
Panel_item	item;
Event		*event;
{
    discard_files();
    (void)window_set(browse_frame, WIN_SHOW, FALSE, 0);
}

static void
browse_event(canvas, event, arg)
Canvas	canvas;
Event	*event;
caddr_t	arg;
{
    int mx,my,id,xid;
    
    id = event_id(event);
    if (event_is_up(event))
        return;

    mx = event_x(event);
    my = event_y(event);
    xid = event_shift_is_down(event);
    switch (id) 
    {
        case MS_LEFT:
	    /* printf("MS LEFT CLICK\n");  */
	    select_files(mx,my,xid);
            break;
            
        case MS_MIDDLE:
	    lock_handler(mx,my);
	    /*
	    printf("MS MIDDLE CLICK could be lock/unlock\n"); 
	    */
            break;


	case MS_RIGHT:
	    /* translate the event to window space,
	     * then show the menu.
	     */
	    (void) menu_show(browse_menu, browse_canvas, 
	               canvas_window_event(browse_canvas, event), 0);	
	    break;
	
	default:
	    break;
    }
}


load_files(base)
char *base;
{
   DIR *dirp;
   struct dirent *dp;
   struct stat buf;
   struct _list_element **et;
   char stat_name[64];
   char tstr[64];
   int  ty,ll,uidf,gidf,len;
   eselected = NULL;
   ecurrent = ebase;
   dirp = opendir(base);
   if (dirp == NULL) 
   {
     sprintf(tstr,"No access:%s\n",base);
     panel_set_value(msg1_item,tstr);
     return(0);
   }
   et = &ebase;
   ty=pfy/2;
   while ((dp = readdir(dirp)) != NULL)
   {
     strcpy(stat_name,base);
     strcat(stat_name,"/");
     strcat(stat_name,dp->d_name);
     ll = stat(stat_name,&buf);
     if ((buf.st_mode & S_IFMT) != S_IFDIR)
     {
       uidf = buf.st_uid;
       gidf = buf.st_gid;
       ecurrent = (struct _list_element *) malloc(sizeof(struct _list_element));
       if (ecurrent == NULL)
       {
	 perror("malloc failed:");
	 exit(-1);
       }
       if (((uidf == my_uid) && (buf.st_mode & S_IWUSR)) || 
	   ((gidf == my_gid) && (buf.st_mode& S_IWGRP)))
       {
         ecurrent->interstat=0;
       }
       else
       {
         ecurrent->interstat=1;
       }
       *et = ecurrent;
       strcpy(ecurrent->lename,dp->d_name); 
       ecurrent->yb = ty+pfy;
       ecurrent->yt = ty;
       ty += pfy+VSPACE;  
       ecurrent->enext = NULL;
       et = &(ecurrent->enext);
     }
   }
   ty += 10;
   if (ty < CHEIGHT) 
     ty = CHEIGHT;
   pw_writebackground(bpw,0,0,414,ty,PIX_SRC);
   window_set(browse_canvas,CANVAS_HEIGHT,ty,0);
   ecurrent=ebase;
   while (ecurrent != NULL)
   {
     pw_text(bpw,LPOS,ecurrent->yb-4,PIX_SRC,NULL,ecurrent->lename);
     if (ecurrent->interstat == 1)
     {
       pw_write(bpw,8,ecurrent->yb-pfy/2-8,16,16,PIX_SRC,&key_pr,0,0);
     }
     strcpy(tstr,(char *)panel_get_value(file_name_item));
     if (!strcmp(ecurrent->lename,tstr))
     {
      eselected=ecurrent;
      len = strlen(ecurrent->lename);
      pw_writebackground(bpw,LPOS-2,ecurrent->yt-2,len*pfx+20,pfy+5,PIX_NOT(PIX_DST));
     }
     ecurrent = ecurrent->enext;
   }
   scrollbar_scroll_to(scrl,0);
   closedir(dirp);  /* important */
   return(1);
}

/*  selection */

select_files(mx,my,mz)
int mx,my,mz;
{
    struct _list_element *ecurrent;
    int len;
    ecurrent = ebase;
    while ((ecurrent != NULL) && !((my < ecurrent->yb) && (my > ecurrent->yt))) 
      ecurrent = ecurrent->enext;
    if (ecurrent != NULL)
    {
      /* highlight & save into the other window */
      if (ecurrent != eselected)
      {
        len = strlen(ecurrent->lename);
        pw_writebackground(bpw,LPOS-2,ecurrent->yt-2,len*pfx+20,pfy+5,PIX_NOT(PIX_DST));
        panel_set_value(file_name_item,ecurrent->lename);
      }
      if ((eselected != NULL) && (eselected != ecurrent))
      {
	/* de-highlight */
        len = strlen(eselected->lename);
        pw_writebackground(bpw,LPOS-2,eselected->yt-2,len*pfx+20,pfy+5,PIX_NOT(PIX_DST));
      }
      eselected=ecurrent;
      /* selection was successfull load? */
      if (mz == 0 )
	read_comp_file();
    }
}

discard_files()
{
   struct _list_element *ec,*eb;
   ec = ebase;
   while (ec != NULL)
   {
     eb = ec->enext;
     free(ec);
     ec = eb;
   }
   ebase = NULL;
}

lock_handler(mx,my)
int mx,my;
{
    struct _list_element *ecurrent;
    char file_name[64];
    int iw;
    ecurrent = ebase;
    /* find which one ? */
    while ((ecurrent != NULL) && !((my < ecurrent->yb) && (my > ecurrent->yt))) 
      ecurrent = ecurrent->enext;
    if (ecurrent != NULL)
    {
      /* if locked unlock 
	    chmod test if unlocked clear it
	 if unlocked lock  
	    chmod test if locked markit
      */
      strcpy(file_name,vnx);
      strcat(file_name,"/");
      strcat(file_name,ecurrent->lename);
      if ((ecurrent->interstat) == 1)
      {
         iw = chmod(file_name,00640);
	 if (iw == 0)
	 {
	    ecurrent->interstat = 0;
            pw_writebackground(bpw,8,ecurrent->yb-pfy/2-8,16,16,PIX_SRC);
         }
      }
      else
      {
         iw = chmod(file_name,00440);
         if (iw == 0)
	 {
	   pw_write(bpw,8,ecurrent->yb-pfy/2-8,16,16,PIX_SRC,&key_pr,0,0);
	   ecurrent->interstat = 1;
	 }
      }
    }
}

greg_it()
{
    int n;
    int tty_fd;
    /* Dissassiate form Control Terminal */

    if (fork() != 0)
	exit(0);	/* parent process */

    freopen("/dev/null","r",stdin);
    freopen("/dev/console","a",stdout);
    freopen("/dev/console","a",stderr);

    setpgrp(0, getpid());	/* change process group,  BSD style */

    for(n=3;n < 20; n++)        /* close any inherited open file descriptors */
	close(n);

      if ( (tty_fd = open("/dev/tty", O_RDWR)) >= 0)
      {
         ioctl(tty_fd, TIOCNOTTY, (char *) 0);  /* lose control tty */
         close(tty_fd);
      }
}


#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

extern int errno;

#define MAXRETRY 8
#define SLEEP 1

int Acqdebug = 0;

/*-----------------------------------------------------------------------
|
|       InitRecverAddr()/2
|       Initialize the socket addresses for display and interactive use
|
+-----------------------------------------------------------------------*/
static
InitRecverAddr(AcqHost,port,recver)
int port;
char *AcqHost;
struct sockaddr_in *recver;
{
    struct hostent *hp;
 
    /* --- name of the socket so that we may connect to them --- */
    /* read in acquisition host and port numbers to use */
    hp = gethostbyname(AcqHost);
    if (hp == NULL)
    {
       fprintf(stderr,"Unknown Hos %s",AcqHost);
       return(-1);
    }

    memset((char *) recver,0,sizeof(struct sockaddr_in)); /* clear socket info */
    memcpy( (char *)&(recver->sin_addr), hp->h_addr, hp->h_length );
    recver->sin_family = hp->h_addrtype;
    recver->sin_port = port;

    return( 0 );
}
/*------------------------------------------------------------
|
|    sendasync()/4
|       connect to  an Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
sendasync(machine,port,acqpid,message)
char *machine;
char *message;
int acqpid;
int port;
{
    char eot;
    int fgsd;   /* socket discriptor */
    int i;
    int on = 1;
    int result;
    struct sockaddr_in RecvAddr;
 
    eot = 4;
    if (Acqdebug)
        fprintf(stderr,"Sendasync():Machine: '%s',Port: %d, PID: %d,\nMsge: '%s'\n",
             machine,port,acqpid,message);

    /* --- initialize the Receiver's Internet Address --- */
    if (InitRecverAddr(machine,port,&RecvAddr))
        return(ERROR);
 
    /* --- try several times then fail --- */
    for (i=0; i < MAXRETRY; i++)
    {
        fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */
 
        if (fgsd == -1)
        {
            perror("sendacq(): socket");
            fprintf(stderr,"sendacq(): Error, could not create socket\n");
            return(ERROR);
        }
	if (Acqdebug)
          fprintf(stderr,"sendasync(): socket create for async trans %d\n",fgsd);
        setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
        /*setsockopt(fgsd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));*/
 
	if (Acqdebug)
	{
            fprintf(stderr,"sendasync(): socket process group: %d\n",acqpid);
            fprintf(stderr,"sendasync(): socket created fd=%d\n",fgsd);       
            printf("sendasync(): send signal that pipe connection is requested.\n");
	}
 
       /* --- attempt to connect to the named socket --- */
       if ((result = connect(fgsd,&RecvAddr,sizeof(RecvAddr))) != 0)
       {
          /* --- Is Socket queue full ? --- */
          if (errno != ECONNREFUSED && errno != ETIMEDOUT)
          {                             /* NO, some other error */
              perror("sendasync():aborting,  connect error");
              return(ERROR);
          }
       }                                /* Yes, try MAXRETRY times */
       else     /* connection established */
       {
            break;
       }
       fprintf(stderr,"sendasync(): Socket queue full, will retry %d\n",i);
       close(fgsd);
       sleep(SLEEP);
    }
    if (result != 0)    /* tried MAXRETRY without success  */
    {
       fprintf(stderr,"sendasync(): Max trys Exceeded, aborting send\n");
       return(ERROR);
    }
    if (Acqdebug)
        fprintf(stderr,"Sendacq(): Connection Established \n");
    write(fgsd,message,strlen(message));
    write(fgsd,&eot,sizeof(eot));
 
    close(fgsd);
    return(1);
}            
