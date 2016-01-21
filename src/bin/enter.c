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
#include <sys/types.h>
#include <sys/stat.h>
#include "enter.h"

#define CTRL_U		21
#define CTRL_W		23
#define DELETE		127
#define	ESC		27
#define SAMPLECOL	1
#define	USERCOL		2
#define MACROCOL	3
#define	RETURN		13
#define	SOLVENTCOL	4
#define	TEXTCOL		5
#define NUM_LINES	8
#define	SAVING		0
#define	LOADING		1

       char	buffer[1024];
static char	*buf;
       char	*default_user;
       char	*default_userdir;
       char	*vnmrsys;
       char	filename[80];
       char	autodir[256];
static char	null = '\000';
static char	print_opt[40];
static char	print_type[40];

       int	active_sample_no;
       int	arg_file;	/* which argument is file name?		*/
       int	arg_opt;	/* which argument is option 		*/
static int	arg_tray;	/* which argument is tray size?		*/
static int      arg_traymin;    /* which argument is traymin?           */
       int	clear_tray;	/* clear tray toggles or not		*/
int	first_locate;	/* search for first or further entries? */
int	next_sno;	/* last fully entered sample number	*/
       int	load_or_save;	/* flag, are we loading or saving?	*/
       int	offonN;		/* auto numbering flag			*/
       int	offonU;		/* auto user flag			*/
static int	point;		/* used for current insertion point     */
       int	Qflag;		/* flag for Q option			*/
static int	sampleno;	/* last sample # entered or supplied    */
       int	save_point;	/* save location for locate		*/
       int	save_lineno;	/* save line num for locate		*/
       int	traycnt[100];	/* keeps track of entries/location	*/
       int	traymax;	/* last location to be acessed in tray	*/
       int      traymin;	/* starting location (auto numbering)	*/

extern char    *get_text_item_value();

/*------------------------------------------------------------------*/
main (argc, argv)
int argc;
char **argv;
{
char	*term_type;
   term_type = (char *)getenv("graphics");
   if ( term_type == NULL || strncmp(term_type,"sun",3) )
   {  printf("enter only supported on sun terminals\n");
      exit();
   }
   if ( initialize(argc,argv) ) return;
/*   fprintf(stderr,"Initialized\n"); fflush(stderr);  */
   if ( create_frame (argc, argv) ) return;
   enter_window_loop();
}
/*------------------------------------------------------------------
|	initialize
|	initialize paramters
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   92-2-7     rolf k.    1. allow for additional numeric argument 
|				traymin, to set starting point of
|				sample entry (will also turn on
|				automatic numbering)
+-----------------------------------------------------------------*/
initialize(argc,argv)
int argc;
char **argv;
{
char	path[256];
int	i;
struct stat dirbuf;
   clear_tray = first_locate = TRUE;
   next_sno =  sampleno = 0;
   buf = &buffer[0];
   default_user    = (char *)getenv("USER");
   default_userdir = (char *)getenv("vnmruser");
   if (default_user == 0)
      fprintf(stderr,"enter: warning, USER not defined\n");
   vnmrsys = (char *)getenv("vnmrsystem");
   if (vnmrsys == 0)
   {  fprintf(stderr,"enter: vnmrsystem not defined\n");
      return 1;
   }
   for (i=0; i<100; i++)
      traycnt[i] = 0;
 
   arg_file = arg_opt = arg_tray = Qflag = 0;
   print_type[0] = '\0';
   print_opt[0] = '\0';

   arg_traymin = 0;
   for (i=1; i<argc; i++)
   {  if (argv[i][0]=='-')
      {  if (argv[i][1]=='Q') Qflag = 1;
         else if (argv[i][1]=='P') strcpy(print_opt,&argv[i][2]);
         else if (argv[i][1]=='T') strcpy(print_type,&argv[i][2]);
	 else if (argv[i][1]=='n' || argv[i][1] == 'u')
               arg_opt=i;
         else i++;
      }
      else if (argv[i][0]>='0' && argv[i][0]<='9')
      {
         if (arg_tray == 0)
         {
            arg_tray = i;
         }
         else
         {   
            arg_traymin = i;
         }
      }   
      else arg_file = i;
   }
   strcpy(path,argv[arg_file]);
   stat(path,&dirbuf);
   if ( S_ISDIR(dirbuf.st_mode) )
   {  arg_file=0;
      strcpy(autodir,path);
   }
   else
      autodir[0]='\000';

   if (arg_traymin!=0) traymin = atoi(argv[arg_traymin]);
   else          traymin = 1;
   if (arg_tray!=0) traymax = atoi(argv[arg_tray]);
   else          traymax = 100;

   if (traymin > 0) next_sno = sampleno = traymin;
   return 0;
}

void set_active_sno()
{
FILE	*active_fd;
int	i;
char	icon_line[10];
   strcpy(filename,autodir);
   strcat(filename,"/sampleinfo");
   if (lockfile(filename)) return;
   active_fd = fopen(filename,"r");
   if (!active_fd)
   {  active_sample_no = 0;
      sprintf(icon_line,"  NONE  ");
      sprintf(buffer,"Active: NONE");
   }
   else
   {  fgets(buffer,80,active_fd);
      i=0;
      while(buffer[i]!=':') i++;
      while(buffer[i]==' '); i++;
      active_sample_no = atoi(&buffer[i]);
      sprintf(icon_line,"%5d",active_sample_no);
      sprintf(buffer,"Active: %3d",active_sample_no);
   }
   fclose(active_fd);
   unlockfile(filename);
   set_window_icon_label(icon_line);
   set_label_item_value(Activemsg, buffer);
}


/*-------------------------------------------------------------
|  open_close_frame ()
+------------------------------------------------------------*/
void     open_close_frame ()
{
void fln_done_proc ();
 
 
     if (!arg_file)
      {  load_or_save = LOADING;
         strcpy(filename,autodir);
         strcat(filename,"/enterQ");
	 set_text_item_value(Fln_req, filename);
         fln_done_proc();
	 reset_text_window(0);
         strcpy(filename,autodir);
         strcat(filename,"/doneQ");
	 set_text_item_value(Fln_req, filename);
         clear_tray = FALSE;
         fln_done_proc();
         clear_tray = TRUE;
	 reset_text_window(0);
         set_active_sno();
         if (!active_sample_no)
	    set_label_item_value(Activemsg, "Active: NONE");
         else
         {  sprintf(buffer,"Active: %3d",active_sample_no);
	    set_label_item_value(Activemsg, buffer);
         }
         inittimer(10.0,10.0,set_active_sno);
      }
     else
      {  load_or_save = LOADING;
         /* filename is already set */
         fln_done_proc();
	 reset_text_window(0);
      }
}

/*-------------------------------------------------------------
|  setprompt
+------------------------------------------------------------*/
void setprompt()
{
char	*samp,*user,*macr,*solv,*text;
char	*one_samp,one_text[80];
int	from_sno,i,many_flag,to_sno;
   showButton("0");
   samp = get_text_item_value(sample_item);
   user = get_text_item_value(user_item);
   macr = get_text_item_value(macro_item);
   solv = get_text_item_value(solvent_item);
   text = get_text_item_value(text_item);
   if (*samp==0 || *user==0 || *macr==0 || *solv==0)
   {
      set_active_item(sample_item);
      menu("e_sample");
      return;
   }
   while (*samp && (*samp<'0' || *samp>'9') )	/* skip leading non digits */
      *samp++;
   while (*samp!=0)				/* read until end of text  */
   {  one_samp = &one_text[0];			/* point where to save     */
      many_flag = FALSE;			/* assume no '-' yet       */
      while (*samp>='0' && *samp<='9')		/* copy one number         */
         *one_samp++ = *samp++;
      while (*samp && (*samp<'0' || *samp>'9') )/* skip non digits        */
      {  if (*samp=='-') many_flag=TRUE;	/* check for '-' character */
         *samp++;
      }
      *one_samp = 0;				/* append NULL		   */

      if (many_flag)				/* if '-'		   */
      {  from_sno = atoi(one_text);		/* save as starting number */
         one_samp = &one_text[0];
         while (*samp>='0' && *samp<='9')	/* get end of range	   */
            *one_samp++ = *samp++;
         to_sno = atoi(one_text);
         if (!to_sno) break;			/* check if there are digits*/
         while (*samp && (*samp<'0' || *samp>'9') )/* skip junk	   */
            *samp++;
         for (i=from_sno; i<=to_sno; i++)	/* for whole range	   */
         {  sprintf(one_text,"%d",i);
	    add_one_entry(one_text,user,macr,solv,text);
         }
      }
      else					/* no '-', one entry only  */
	 add_one_entry(one_text,user,macr,solv,text);
   }
   set_sample_user();
}


set_sample_user()
{
char    line[80];
   sampleno = 0;
   if (offonN)
   {  if (!next_sno) next_sno++;
      sprintf(line, "%d", next_sno);
      set_text_item_value(sample_item, line);
   }
   else
      set_text_item_value(sample_item, &null);
   if (offonU)
      set_text_item_value(user_item, default_user);
   else
      set_text_item_value(user_item, &null);
   set_text_item_value(macro_item, &null);
   set_text_item_value(solvent_item, &null);
   set_text_item_value(text_item, &null);
   if (offonU && offonN)
   {
      set_active_item(macro_item);
      menu("e_macro");
   }
   else
      if (!offonU && !offonN)
      {
         set_active_item(sample_item);
         menu("e_sample");
      }
      else
      {  if (!offonU)
         {
            set_active_item(user_item);
            menu("e_user");
         }
         if (!offonN) 
         {
            set_active_item(sample_item);
            menu("e_sample");
         }
      }
}

/*-------------------------------------------------------------
|  setsampleno
+------------------------------------------------------------*/
int
setsampleno(chr)
char    chr;
{
char	*line,*line1;
int	i;

      switch(chr)
      {  case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
		 {
		    line = get_text_item_value(sample_item);
		    sampleno = 0;
		    msg(" ");
 		    while(*line == ' ') line++;
		    while(*line != NULL)
		    {
			if (*line == '-' || *line == ',' || *line == ' ')
			   sampleno = 0;
			else
			   sampleno = 10 * sampleno + (int)(*line-'0');
			line++;
		    }
		    sampleno = 10 * sampleno + (int)(chr-'0');
                    if (sampleno<0 || sampleno>traymax)
                    {  error ("Sample number outside tray range");
                       return(0);  /*  PANEL_NONE */
                    }
                    return(1);     /*  PANEL_INSERT */
                    break;
                 }

         case ',':
         case '-':
         case ' ':
         case CTRL_W:
         case CTRL_U:
                 {  sampleno=0;
                    return(1);
                    break;
                 }
         case DELETE:
                 {
		    line = get_text_item_value(sample_item);
                    line1 = line;
                    while (*line1 != 0) line1++;
                    line1--;
                    if (line<= line1)
                    {  if (*line1>='0' && *line1<='9')
                          sampleno = (sampleno - (int)(*line1-'0') ) / 10;
                       else
                       {  line1--;
                          sampleno = 0; i=1;
                          while (line<=line1 && *line1>='0' && *line1<='9')
			  {  sampleno = sampleno  + (int)(*line1-'0') * i;
                             line1--; i *= 10;
                          }
                       }
                    }
                    else
                       sampleno = 0;
                    return(1);
                    break;
                 }
         case RETURN:
		 {  msg(" ");
                    if (offonU)
                    {
		       set_active_item( macro_item);
                       menu("e_macro");
                       return(0);
                    }
                    else
                       menu("e_user");
                       return(2);  /*  PANEL_NEXT  */
                    break;
                 }
          default :
                 {  error ("Legal characters are '0' through '9' ',' '-' <space> <Return> <Delete>");
                    return(0);
                 }
      }
}

set_next_entry(c,meta)
int c, meta;
{  
   if (meta)
   {  
      if (c == sample_item)       menu("e_text");
      else if (c == user_item)    menu("e_sample");
      else if (c == macro_item)   menu("e_user");
      else if (c == solvent_item) menu("e_macro");
      else if (c == text_item)    menu("e_solvent");
   }
   else
   {
      if (c == sample_item)       menu("e_user");
      else if (c == user_item)    menu("e_macro");
      else if (c == macro_item)   menu("e_solvent");
      else if (c == solvent_item) menu("e_text");
      else if (c == text_item)    setprompt();
   }
}

/*-------------------------------------------------------------
|  trayproc()
+------------------------------------------------------------*/
void trayproc(item)
int item;
{
char	num_text[10];

   if (item<1 || item>traymax)
   {  error ("Sample number outside tray range");
      return;
   }
   else
      msg(" ");
   sprintf(num_text," %d",item);
   set_enter_panel_focus();
   set_text_item_value(sample_item, num_text);
   if (offonU)
   {
       set_active_item(macro_item);
       menu("e_macro");
   }
   else
   {
       set_active_item(user_item);
       menu("e_user");
   }
   first_locate = TRUE;
}



/*-------------------------------------------------------------
|  AutoN
+------------------------------------------------------------*/
void AutoN (offon)
int		offon;
{
char	line[10];
   if (offon)
   {
      if (!next_sno) next_sno++;
      if (next_sno > traymax) next_sno = 1;
      sprintf(line, "%d", next_sno);
      set_text_item_value(sample_item, line);
      if (offonU)
      {
         set_active_item(macro_item);
         menu("e_macro");
      }
      else
      {
         set_active_item(user_item);
         menu("e_user");
      }
   }
   else
      sampleno=0;
   offonN = offon;
}

/*-------------------------------------------------------------
|  AutoU
+------------------------------------------------------------*/
void AutoU (offon)
int		offon;
{
   if (offon)
   {
      set_text_item_value(user_item, default_user);
      if (offonN)
      {
         menu("e_macro");
         set_active_item(macro_item);
      }
      else
      {
         menu("e_sample");
         set_active_item(sample_item);
      }
   }
   offonU = offon;
}




/*-------------------------------------------------------------
|  printproc
+------------------------------------------------------------*/
void printproc()
{
FILE	*temp_file;
char	s[80];
int	point, count;	
   if (!(temp_file = fopen("/tmp/e_enter.tmp","w")) )
   {  error("cannot print entries");
      return;
   }
   point = 0;
   count = 1000;	
   while (count == 1000)
   {
      count = get_textsw_content(point, buffer, 1000);
      buffer[count+1] = '\000';
      fprintf(temp_file,"%s",buffer);
      count -= point;
      point += count;
   }
   fclose(temp_file);
   if (print_type[0] == '\0')
   {
      if (print_opt[0] == '\0')
          sprintf(s,"lpr /tmp/e_enter.tmp");
      else
          sprintf(s,"lpr -P%s /tmp/e_enter.tmp", print_opt);
   }
   else
   {
      if (print_opt[0] == '\0')
          sprintf(s,"/vnmr/bin/vnmrprint /tmp/e_enter.tmp");
      else
          sprintf(s,"/vnmr/bin/vnmrprint /tmp/e_enter.tmp %s %s", print_opt, print_type);
   }
   point = system(s);
#ifdef AIX
   if (point) error("Cannot print (-P option not in /etc/qconfig?)");
#else
   if (point) error("Cannot print (-P option not in /etc/printcap?)");
#endif
   unlink("/tmp/e_enter.tmp");
}


/*-------------------------------------------------------------
|  appendproc
+------------------------------------------------------------*/
void submitproc ()
{
FILE	*enter_fd;
int	count;
char	temp_name[80];
   if (arg_file)
      strcpy( temp_name, get_text_item_value(Fln_req) );
   else
   {  strcpy(temp_name,autodir);
      strcat(temp_name,"/enterQ");
   }
   if (Qflag) 
      while (lockfile(temp_name) )
      {   msg("Please stand by, file locked");
          sleep(1);
      }
   enter_fd = fopen(temp_name,"a");
   if (!enter_fd)
   {  fprintf(stderr,"filename = %s\n",temp_name);
      error("cannot submit new entries");
   }
   close_frame();
   point = 0;
   count = 1000;	
   while (count == 1000)
   {
      count = get_textsw_content(point, buffer, 1000);
      buffer[count+1] = '\000';
      fprintf(enter_fd,"%s",buffer);
      count -= point;
      point += count;
   }
   fclose(enter_fd);
   if (Qflag) unlockfile(temp_name);
   enter_reset();
   reset_text_window(0);
   msg(" ");
}
	

/*-------------------------------------------------------------
|  locateproc
+------------------------------------------------------------*/
void locateproc ()
{
char	*samp, *user, *macr, *solv, *text, *cpy_ptr, line[80];
char	*ptr;
int	samp_found, user_found, macr_found, solv_found, text_found;
int	found, last_line, line_number;

   set_text_highlight(0, 0, 1);
   samp = get_text_item_value(sample_item);
   user = get_text_item_value(user_item);
   macr = get_text_item_value(macro_item);
   solv = get_text_item_value(solvent_item);
   text = get_text_item_value(text_item);
   while (*samp == ' ') samp++;   /* ignore leading spaces */
   if (*samp==0 && *user==0 && *macr==0 && *solv==0 && *text==0)
   {
      error("You must enter one or more of SAMPLE, USER, MACRO or SOLVENT");
      set_active_item(sample_item);
      menu("e_sample");
      return;
   }
   found = FALSE;
   last_line = get_textsw_first_line();
   if (first_locate) line_number = point = 0;
   else            { line_number = save_lineno; point = save_point; }
      get_textsw_content(point,buffer,1000);
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
      while ( *ptr != ':' ) ptr++; ptr++; ptr++;
      if (*text)
      {   cpy_ptr = &line[0];
          while (*ptr != '\n') *cpy_ptr++ = *ptr++; *cpy_ptr = '\0';
	  if ( strstr(&line[0],text) )    text_found = TRUE;
          else				  text_found = FALSE;
      }
      else text_found = TRUE;
      while ( *ptr != '-') ptr++;
      while (*ptr != '\n') ptr++; ptr++;
      found = samp_found && user_found && macr_found && solv_found && text_found;
      if (!found)
      {   point += (int) (ptr - &buffer[0]);
	  get_textsw_content(point,buffer,1000);
          line_number += NUM_LINES+1;
      }
   }
   if (found)
   {  save_lineno = line_number + NUM_LINES + 1;
      save_point = point + (int) (ptr - &buffer[0]);
      ptr = &buffer[0];
      while ( *ptr != '\n')  ptr++; ptr++;
      while ( *ptr != '-') ptr++;
      set_text_highlight(point, point+ptr-buffer, 1);
      set_textsw_insert_point(point);
      if ( (line_number-last_line) > 12 || first_locate)
	 set_textsw_first_line(line_number);
      first_locate = FALSE;
   }
   else
   {  error("No matching entry found");
      first_locate = TRUE;
   }
}

