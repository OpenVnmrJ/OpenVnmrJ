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
**	X version of eccTool 
*/
/*
Still to go:
save button  - works
read button  - works
setup button - not fully checked
put a tag on read only 
lock/unlock function
mask directories and dot files
 < add a local/global toggle >
*/

#include <stdio.h>
#include <math.h>
#include <termio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/PopupWindo.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/Stub.h>
#include <Xol/Form.h>
#include <Xol/RubberTile.h>
#include "xph.h"

#define ERROR -1
/* Global Window Definitions */

char *larry[]={"X","Y","Z"};
char *moe[]={"global","local"};
Widget toplevel,topform,bcon,fcon,econ1,econ2;
Widget button[10],gcu,msg,msg1,msg2;
Xph_ex_ptr gex,lib_ex;
Xph_entry_ptr  f1,f2,aw[5],tw[5],gn1,sl1,dty;
/* 
   do not override id's of those above 
   - they are keyed to 
   internal parameters 
*/
Xph_dir_list_ptr list;

/*
	Non Window Section
*/
char porw;
int  numtcs,firstpage,curpage,lastpage;
int  my_uid,my_gid;
char vn[200],vnx[200],vnu[200],vng[200];
char lastread[200],lastwrite[200],lastsent[200];
char consoletype[256];
int  okToSend = -1;

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
    struct adj  tcd[5],ampd[5];
}
fullset[3];


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
    xph_message_set(msg1,"Value is out of range!");
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
   while ((pntr = strchr(base,*search)) != 0) 
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
      pt_->tcd[0].max_ = 254.0;
      pt_->tcd[0].min_ = 254.0/9.0;
      pt_->tcd[1].max_ =   40.0;
      pt_->tcd[1].min_ =   40.0/9.0;
      pt_->tcd[2].max_ =   10.0;
      pt_->tcd[2].min_ =   10.0/9.0;
      pt_->tcd[3].max_ =   2.0;
      pt_->tcd[3].min_ =   2.0/9.0;
      pt_->tcd[4].max_ =   1.0;
      pt_->tcd[4].min_ =   1.0/9.0;
      strcpy(pt_->tcd[0].un,"254.0");
      strcpy(pt_->tcd[1].un,"40.0");
      strcpy(pt_->tcd[2].un,"10.0");
      strcpy(pt_->tcd[3].un,"2.0");
      strcpy(pt_->tcd[4].un,"1.0");
      }
      else
      {  /* pfg version */
      pt_->tcd[0].max_ = 0.23;
      pt_->tcd[0].min_ = 0.23/11.0;
      pt_->tcd[1].max_ =   2.34;
      pt_->tcd[1].min_ =   2.34/11.0;
      pt_->tcd[2].max_ =   165.0;
      pt_->tcd[2].min_ =   165.0/11.0;
      pt_->tcd[3].max_ =   23.50;
      pt_->tcd[3].min_ =   23.50/11.0;
      strcpy(pt_->tcd[0].un,"0.230");
      strcpy(pt_->tcd[1].un,"2.34");
      strcpy(pt_->tcd[2].un,"165.0");
      strcpy(pt_->tcd[3].un,"23.5");
      strcpy(pt_->tcd[4].un,"1.0");
      }
      strcpy(pt_->ampd[0].un,"0.0");
      strcpy(pt_->ampd[1].un,"0.0");
      strcpy(pt_->ampd[2].un,"0.0");
      strcpy(pt_->ampd[3].un,"0.0");
      strcpy(pt_->ampd[4].un,"0.0");
      strcpy(pt_->tcd[0].key,"tc1");
      strcpy(pt_->tcd[1].key,"tc2");
      strcpy(pt_->tcd[2].key,"tc3");
      strcpy(pt_->tcd[3].key,"tc4");
      strcpy(pt_->tcd[4].key,"tc5");
      strcpy(pt_->ampd[0].key,"amp1");
      strcpy(pt_->ampd[1].key,"amp2");
      strcpy(pt_->ampd[2].key,"amp3");
      strcpy(pt_->ampd[3].key,"amp4");
      strcpy(pt_->ampd[4].key,"amp5");
      for (j = 0; j < numtcs; j++)
      {
       pt_->ampd[j].max_ =100.0;
       pt_->ampd[j].min_ =  0.0;
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
      xph_entry_set(dty,pt_->duty.un);
      xph_entry_set(gn1,pt_->gain.un);
      xph_entry_set(sl1,pt_->slew.un);
    }
    for (i = 0; i < numtcs; i++)
    {
      xph_entry_set(tw[i],pt_->tcd[i].un);
      xph_entry_set(aw[i],pt_->ampd[i].un);
    }
}

static void  setup_comp_file() 
{
  char file_name[60],bigstring[120],tstr[60];
  int iw,base_length;
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  xph_entry_get(f1,tstr);
  strcat(file_name,tstr);
  if (strlen(file_name) <= base_length) 
  {
     xph_message_set(msg,"Nothing to send!");
     return;
  }
  if (strcmp(consoletype,"inova") == 0)		/* inova */
     sprintf(bigstring,"%s/bin/eccsend %s /%s/acqqueue/eddy.out -S -I",
							vn,file_name,vn);
  else				/* unity or unityplus */
     sprintf(bigstring,"%s/bin/eccsend %s /%s/acqqueue/eddy.out -S -U",
							vn,file_name,vn);
  iw = system(bigstring);
  if (iw == 0)
  {
    /*
    if ((strcmp(lastwrite,tstr) == 0) || (strcmp(tstr,lastread) == 0))
      xph_message_set(msg1,"");
    else
     xph_message_set(msg,"not last file read/written?");
    */
    strcpy(lastsent,tstr);
    iw = tellVnmr(tstr);
    if (iw != 0) 
      xph_message_set(msg,"REMOTE SEND FAILED");
    return;
  }
  iw /= 256;
  if (iw == 1)
  {
     xph_message_set(msg,"Nothing sent - no eccsend!!");
     return;
  }
  if (iw == 0x0fe)
  {
     xph_message_set(msg1,"No such file to send!!");
     return;
  }
  if (iw == 0x0fd)
  {
     xph_message_set(msg1,"Could not write eddy.out!!");
  }
}

static void  write_comp_file() 
{
  char file_name[60],tstr[40];
  int j,k,i,base_length,alarm;
  FILE *fds;
  alarm = 0;
  capture_page(curpage);
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  xph_entry_get(f1,tstr);
  strcat(file_name,tstr);
  if (strlen(file_name) <= base_length) 
  {
     xph_message_set(msg,"No Name?");
     return;
  }
  if ((fds = fopen(file_name,"w")) == NULL)
  {
    xph_message_set(msg,"Could not write file!");
    return;
  }
  fprintf(fds,"\" output from eddy current window\n");
  for (i = firstpage ; i < lastpage+1; i++)
  {
    fprintf(fds,"\"time constant data for %c coil follows\n",'x'+i);
    fprintf(fds,"\"this data must reflect hardware changes\n");
    for (k = 0; k < numtcs; k++)
    {
    fprintf(fds,"tc_%c_%1d       %9.3f\n",'x'+i,k+1,fullset[i].tcd[k].max_);
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
       alarm += check_value(fullset[i].tcd[j]);
       fprintf(fds,"%-7s  %8s\n",fullset[i].tcd[j].key,fullset[i].tcd[j].un);
       alarm += check_value(fullset[i].ampd[j]);
       fprintf(fds,"%-7s  %8s\n",fullset[i].ampd[j].key,fullset[i].ampd[j].un);
    }
  }
  fclose(fds);
  if ((strcmp(lastwrite,tstr)) || (strcmp(tstr,lastread)))
  {
     deleteList(list); 
     dirList(list,vnx);
     initList(list);
  }
  strcpy(lastwrite,tstr);
  if (alarm == 0)
    xph_message_set(msg,"");
}


static struct page *pt_;

static void  read_comp_file() 
{
  char file_name[60],tstr[60],ustr[80];
  int k,base_length;
  FILE *frd;
  pt_ = &fullset[0];
  strcpy(file_name,vnx);
  strcat(file_name,"/");
  base_length = strlen(file_name);
  xph_entry_get(f1,tstr);
  strcat(file_name,tstr);
  if (strlen(file_name) <= base_length) 
  {
     xph_message_set(msg,"No file name!");
     return;
  }
  if ((frd = fopen(file_name,"r")) == NULL)
  {
    xph_message_set(msg,"Could not open file");
    return;
  }
  while (getline(frd,ustr,80) != EOF)
  { 
     match_line(ustr);
  }
  fclose(frd);
  strcpy(lastread,tstr);
  set_page(curpage);
}

tellVnmr(tstr)
{
    char msg[256];

    sprintf(msg,"\neddysend('%s',1)\n",tstr);
    return( sendToVnmr(msg) );
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
      if ((yy = parse_adj(&(pt_->tcd[i]),yy)) == NULL) return;
      if ((yy = parse_adj(&(pt_->ampd[i]),yy)) == NULL) return;
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
	uu = strchr(uu,',');
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
      fullset[j].tcd[k].max_ = dbl;
      fullset[j].tcd[k].min_ = dbl/9.0;
   }
   return(uu);
}

void
loc_cb(w,cd,cld)
Widget w;
caddr_t cd,cld;
{
   if (xph_ex_get(lib_ex) == 1)
     strcpy(vnx,vnu);
   else
     strcpy(vnx,vng); /* default to global */
   /* now signal the file window for update */
   deleteList(list);
   dirList(list,vnx, XPH_DIR_NO_DIR | XPH_DIR_NO_DOT);
   initList(list);
}

void
entry_cb(w,cd,cld)
Widget w;
caddr_t cd,cld;
{
   char tmp[80],tmp1[40];
   int i;
   float x,min,max;
   /* use amplitude range as the default */
   max = 100.0;
   min = 0.0;
   /* cd is an xph_entry_ptr */
   xph_entry_get(cd,tmp);
   for (i=0; i < numtcs; i++)
   {
     if (cd == (caddr_t) tw[i])
     {
       printf("t%d\n",i);
       max = fullset[curpage].tcd[i].max_;
       min = fullset[curpage].tcd[i].min_;
     }
   }
   i = sscanf(tmp,"%f%s",&x,tmp1);
   if ((i != 1) || (x > max) || (x < min))
   {
     sprintf(tmp,"%4.2f",min);
     xph_entry_set(cd,tmp);
     sprintf(tmp,"range [%3.2f,%3.2f]",min,max);
     xph_message_set(msg1,tmp);
     /* post a range message */
   }
   else
   {
     /* clear a range message */
     xph_message_set(msg1,"");
   }
}


void 
burp(w,cd,cld)
Widget w;
caddr_t cd,cld;
{
   char tmp[80];
   int i;
   double x;
   if (!strcmp(cd,"quit"))
     exit(0);
   /*
   printf("called  %s %x\n",cd,cld);
   */
   if (!strcmp(cd,"defaults"))
   {
     /* reset current page */
     init_page(curpage);
     set_page(curpage);
     xph_message_set(msg1,"");
     xph_message_set(msg2,"");
   }
   if (!strcmp(cd,"read"))
      read_comp_file();
   if (!strcmp(cd,"save"))
      write_comp_file();
   if (!strcmp(cd,"setup"))
      setup_comp_file();
}

capture_page(k)
int k;
{
    int i;
    struct page *pt_;
    pt_ = &fullset[k];
    if (porw == 'w')
    {
      xph_entry_get(dty,pt_->duty.un);
      xph_entry_get(gn1,pt_->gain.un);
      xph_entry_get(sl1,pt_->slew.un);
    }
    for (i = 0; i < numtcs; i++)
    {
      xph_entry_get(tw[i],pt_->tcd[i].un);
      xph_entry_get(aw[i],pt_->ampd[i].un);
    }
}

/* verified */
void 
swap_page(w,cd,cld)
Widget w;
caddr_t cd,cld;
{
   char tmp[80];
   int i;
   double x;
   capture_page(curpage);
   curpage = xph_ex_get(gex);
   set_page(curpage);
}

main(argc,argv)
int argc;
char **argv;
{
    int k;
    int nrows;
    char *getenv();
    nrows = 1;
    porw = 'w';
    numtcs = 5;
    firstpage = 0;
    lastpage  = 2;
    strcpy(lastread,"");
    strcpy(lastwrite,"");
    strcpy(lastsent,"");
    strcpy(consoletype,"inova");
    /**/
    greg_it();
    /**/
    toplevel = OlInitialize("EccTool","EccTool",NULL,0,&argc,argv);
    
    if ((argc >= 2) && (argc <= 3))
    {
        initVnmrComm(argv[1]);
        okToSend = 1;
	if (argc == 3)
	   strcpy(consoletype,argv[2]);
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
    curpage   = firstpage;
    init_page(0);
    init_page(1);
    init_page(2);
    /* file permission stuff */
    my_uid = getuid();
    my_gid = getgid();
    strcpy(vn,getenv("vnmrsystem"));
    strcpy(vng,vn);
    strcat(vng,"/imaging/eddylib");
    strcpy(vnx,vng);
    strcpy(vnu,getenv("vnmruser"));
    strcat(vnu,"/imaging/eddylib");

    topform = make_form(toplevel,"master_layout");
    bcon    = make_control(topform,"button_area",5);
    /* 
    upper top left 
    */
    button[0] = xph_obutton(bcon,"b0",(okToSend != -1) ? "setup" : "     ",burp);
    button[1] = xph_obutton(bcon,"b1","save",burp);
    button[2] = xph_obutton(bcon,"b2","read",burp);
    button[3] = xph_obutton(bcon,"b3","defaults",burp);
    button[4] = xph_obutton(bcon,"b4","quit",burp);
    /*
    fcon    = make_form(topform,"file_area");
    */
    fcon = topform;
    f1 =xph_entry(fcon,"f1","file: ",8,"",20,burp);
    msg = xph_message(fcon,"f2",20,"");
    xph_f_pos(msg,f1->cap,FALSE,0,f1->cap,TRUE,5);
    list = xph_dir_list(fcon,"f3",5,vnx,XPH_DIR_NO_DIR | XPH_DIR_NO_DOT,NULL,f1);
    xph_f_pos(list->dirw,msg,FALSE,0,msg,TRUE,10);
    /*
    f2 = xph_entry(fcon,"f1","filex: ",8,"",9,NULL);
    */
    gex = xph_exclusive(topform,"gex","Gradient: ",nrows,larry,3,swap_page);
    lib_ex = xph_exclusive(topform,"lex","files: ",nrows,moe,2,loc_cb);
    gcu    = make_control(topform,"gcu_area",2);
    gn1 = xph_entry(gcu,"g1","gain: ",8,fullset[0].gain.un,9,entry_cb);
    dty = xph_entry(gcu,"g2","duty cycle: ",8,fullset[0].duty.un,9,entry_cb);
    sl1 = xph_entry(gcu,"g3","slew rate:",8,fullset[0].slew.un,9,entry_cb);
    /* just below and left justified with bcon */
    econ1   = make_control(topform, "entry_area1",5);
    /* just below and left justified with gex */
    econ2   = make_control(topform, "entry_area2",5);
    /* just to right of and top even with econ1 */
    tw[0] = xph_entry(econ1,"e1","tc1: ",8,fullset[0].tcd[0].un,9,entry_cb);
    tw[1] = xph_entry(econ1,"e2","tc2: ",8,fullset[0].tcd[1].un,9,entry_cb);
    tw[2] = xph_entry(econ1,"e3","tc3: ",8,fullset[0].tcd[2].un,9,entry_cb);
    tw[3] = xph_entry(econ1,"e4","tc4: ",8,fullset[0].tcd[3].un,9,entry_cb);
    tw[4] = xph_entry(econ1,"e5","tc5: ",8,fullset[0].tcd[4].un,9,entry_cb);
    aw[0] = xph_entry(econ2,"e6","amp1: ",8,fullset[0].ampd[0].un,9,entry_cb);
    aw[1] = xph_entry(econ2,"e7","amp2: ",8,fullset[0].ampd[1].un,9,entry_cb);
    aw[2] = xph_entry(econ2,"e8","amp3: ",8,fullset[0].ampd[2].un,9,entry_cb);
    aw[3] = xph_entry(econ2,"e9","amp4: ",8,fullset[0].ampd[3].un,9,entry_cb);
    aw[4] = xph_entry(econ2,"e10","amp5: ",8,fullset[0].ampd[5].un,9,entry_cb);
    msg1 = xph_message(topform,"m2",20,"");
    xph_f_pos(msg1,econ1,FALSE,0,econ1,TRUE,5);
    msg2 = xph_message(topform,"m3",20,"");
    xph_f_pos(msg2,msg1,FALSE,0,msg1,TRUE,5);
    XtRealizeWidget(toplevel);
    XtMainLoop();
    exit(0);
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

    setsid();			/* change process group,  POSIX style */

    for(n=3;n < 20; n++)        /* close any inherited open file descriptors */
	close(n);
}
