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

/* bug in IRIX curses library forces the use of all the #ifdef IRIX lines */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curses.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>

#define   HOSTLEN	128
#define   PATHLEN	128
#define   NAMELEN	128
#define   TYPELEN	22
#define   USELEN	22
#define   PORTLEN	22
#define   BAUDLEN	22
#define   RETURN	13
#define   ESCAP		27
#define   FIRSTCHAR	32
#define   LASTCHAR	125

#define   START_X	5
#define   first_col	20
#define   info_col	42
#define   START_Y	3
#define   CMD_Y		5
#define   NAME_Y	6
#define   USE_Y		8
#define   TYPE_Y	10
#define	  HOST_Y	12
#define   PORT_Y	14
#define   BAUD_Y	16
#define   SHARED_Y	18
#define   MESS_Y	21

#define   HEIGHT	18
#define   WIDTH		70

int    x, y;
int    cc;
int    change;
int    NEW, mess;
int    VIEW;
int    cmdindex;
int    oldindex;
int    cmdline;
int    local_host;
int    fd_err;
int    last_col;
int    max_len;
int    name_ptr;
int    use_ptr;
int    type_ptr;
int    host_ptr;
int    port_ptr;
int    baud_ptr;
int    shared_ptr;
int    *dis_ptr;
int    adm_user, root_user;

WINDOW   *helpscr;
#define  cmdno 9 
char   *reverse;
char   *normal;
char   *bel;
char   *tgetstr();
char   *str;
char   *getenv();
char   *xtermname;
char   *edit_str;
char   hostname[HOSTLEN];
char   host_input[HOSTLEN];
char   name_input[NAMELEN];
char   *sysdir;
char   devicefile[PATHLEN];
char   typefile[PATHLEN];
void   ddne();
char  *func[] = {
		 "next", "prev", "modify", "save",
		 "create", "delete", 
                 "exec", "help", "quit"
                };
char  *attr[] = {
		 "device name:",  
		 "        Use:",
		 "device type:",
		 "       Host:",
		 "       Port:",
		 "  Baud Rate:",
		 "     Shared:"
	        };

struct  node {
		int   saved;
                char  name[NAMELEN];
                char  use[USELEN];
                char  type[TYPELEN];
                char  host[HOSTLEN];
                char  port[PORTLEN];
                char  baud[BAUDLEN];
                char  shared[6];
                struct node *prev;
                struct node *next;
             };

struct  baud {
                char  rate[BAUDLEN];
                struct baud *prev;
                struct baud *next;
             };

struct  port {
                char  num[PORTLEN];
                struct port *next;
             };

struct  host_node {
                    char  name[HOSTLEN];
                    struct host_node *next;
                  };
struct  devicetype {
                     char  type[TYPELEN];
                     struct devicetype *next;
                   };

struct node  *first, *cur_node;
struct node  *last, *edit_node;
struct devicetype  *typeptr, *cur_type;
struct baud  *baud_1;
struct port  *port_1;
struct host_node  *host_1;
int   confirm();
char  termbuf[1024];

main(argc, argv)
int     argc;
char    *argv[];
{
	int  i, ptr, uid;
        char  *temp;
        char  term_cap[512]; 
	struct passwd  *pwentry;

        if (argc == 2)
	     sysdir = argv[1];
        else
        {
             sysdir = (char *)getenv("vnmrsystem");
             if (!sysdir)
                  sysdir = "/vnmr";
	}
        sprintf(devicefile, "%s/devicenames", sysdir);
        sprintf(typefile, "%s/devicetable", sysdir);
        fd_err = open("/dev/null", O_RDWR | O_CREAT);
        if (fd_err > 0)
                dup2(fd_err, 2);
        baudTable();
        sort_baud();
/*
        portTable();
*/
        hostTable();
        typeTable();
        signal(SIGINT, ddne);
        xtermname = (char *)getenv("TERM");
        if (strcmp(xtermname, "graphon") == 0)
        {
               bel = "\7";
               reverse = "\033[7m";
               normal = "\033[0m";
        }
        else
	{
              if (tgetent(termbuf, getenv("TERM")) == 1)
              {
                  temp = term_cap;
                  if((bel = tgetstr("bl", &temp)) == NULL)
                        bel = "\7"; 
                  if ((reverse = tgetstr("mr", &temp)) == NULL)
                        reverse = "\033[7m";
	          if ((normal = tgetstr("se", &temp)) == NULL)
                        normal = "\033[0m";
                  if (tgetnum("col") < 79 || tgetnum("li") < 24)
                  {
                        printf(" Screen size is too small, abort!\n");
                        Exit();
                  }
		  i = 0;
		  while(*(reverse + i) >= '0' && *(reverse + i) <= '9')
			i++;
		  sprintf(reverse, "%s", reverse + i);
		  i = 0;
		  while(*(normal + i) >= '0' && *(normal + i) <= '9')
			i++;
		  sprintf(normal, "%s", normal + i);
              }
              else 
              {
                  printf("Unknown terminal type !\n");
                  Exit();
              }
	} 
 	if ((gethostname(&hostname[0], HOSTLEN-1)) != 0)
	{
	      printf("cannot get hostname, abort !\n");
	      Exit();
	}
        mess = 0;	
        createList();
	initscr();
        cbreak();
        noecho();
        x = 5;
        y = START_Y;
        drawbox();
        nonl();
        initial();
        if (first == NULL)
        {
	     disp_mess("file is empty");
             VIEW = 0;
        }
        else
	{
             cur_node = first;
             view (first);
    	}
        cmdline = 0;
	oldindex = 0;
        moveCmd();
	NEW = 0;
	change = 0;
	uid = geteuid();
	pwentry = getpwuid(uid);
	adm_user = 0;
	root_user = 0;
	if (pwentry != NULL)
	{
	  if (strcmp(pwentry->pw_name, "root") == 0)
	  {
		adm_user = 1;
		root_user = 1;
	  }
	  if (strcmp(pwentry->pw_name, "vnmr1") == 0)
		adm_user = 1;
	}

	for(;;)
	{
		cc = getch();
                getyx(stdscr, y, x);
                if (mess)  /* there is message, remove it  */
                {
                      move(MESS_Y, 0);
                      deleteln();
                      insertln();
                      move (y, x);
                      refresh();
                      mess = 0;
                }
                if (cc == ESCAP)
                {    
		      escap();
                      continue;
                }
                if (cmdline)
                {
                     if (cc == RETURN)
                     {
                           exec_cmd();
                     }
                     else
                     {
                           if ((cc < LASTCHAR) && (cc > FIRSTCHAR))
                           {
                                 oldindex = cmdindex;
                                 select_cmd(cc);
                           }
                     }      
		     continue;
                }
                if (cc == RETURN)
                {
                     if (!line_check())
                          continue;
                     if (!local_host && y == HOST_Y)
                           y = SHARED_Y;
                     else
                     {
                           if (y == SHARED_Y)
                                y = NAME_Y;
                           else
                                y = y + 2;
                     }
                     move(y, first_col);
                     refresh();
		     set_new_par();
		     continue;
                }      
                if ((cc == 127) || (cc == 8))
                {
                     getyx(stdscr, y, x);
		     delete_char();
		     if (cc == 8 && x > first_col)
			 x--;
                     move(y, x);
                     refresh();
	             continue;
               }
               if ((cc >= FIRSTCHAR) && (cc <= LASTCHAR))
                     add_char(cc);
        }
}



escap()
{
	int  xc;
        int x2, y2;

	xc = getch();
	if (xc == 91)
	{
		xc = getch();
                getyx(stdscr, y, x);
		switch(xc) {
		case 'A':    /*   move up  */
                           if (!cmdline)
                           {
                                if (!line_check())
                                     return;
                                y2 = y -2;
                                if (!local_host && y2 == BAUD_Y)
                                    y2 = HOST_Y;
                                if (y2 <= NAME_Y - 1)
                                {
                                    link_node();
                                    moveCmd();
                                }
                                else
                                {
                                    x2 = first_col;
                                }
                           }
                           else
                           {
                                printf("%s", bel);
                                refresh();
                           }
		           break;
		          	 	
		case 'B':    /*   move down  */
                           if (!cmdline)
                           {
                                if (!line_check())
                                     return;
                                if (y == SHARED_Y)
                                      y2 = NAME_Y;
                                else
                                {
                                      if (!local_host && y == HOST_Y)
                                           y2 = SHARED_Y;
                                      else
			                   y2 = y + 2;
                                }
                                x2 = first_col;
                           }
                           else
                           {
                                printf("%s", bel);
                                refresh();
                           }
			   break;

		case 'C':    /*   move right   */
                           if (cmdline)
                           {
                                oldindex = cmdindex;
                                cmdindex++;
                                if (cmdindex == cmdno)
                                      cmdindex = 0;
                                moveCmd();
                           }
                           else
                           {
                                y2 = y;
                                if (inch() != ' ' && x < last_col)
                                      x2 = x + 1;
                                else if (x == last_col)
			        {
                                      shift_left();
                                      x2 = x;
                                }
				else
                                {
                                      move(0,0);
#if defined IRIX || SOLARIS
                                      printf("%s", bel);
#else
                                      printw("%s", bel);
#endif
                                      refresh();
                                      x2 = x;
                                }
				move(y2, x2);
				refresh();
				return;
                           }
			   break;

		case 'D':    /*   move left   */
                           if (cmdline)
                           { 
                                oldindex = cmdindex;
                                cmdindex--;
                                if (cmdindex < 0)
                                      cmdindex = cmdno - 1;
                                moveCmd();
                           }
                           else
                           {
			        x2 = x - 1;
			        if (x2 < first_col)
				{
				    shift_right();
				    x2 = first_col;
				}
                                y2 = y;
				move(y2, x2);
				refresh();
				return;
                           }
			   break;
		}
                if (!cmdline)
                {
		     move(y2, x2);
                     refresh();
		     set_new_par();
                }
	}
}



void ddne()
{
        nl();
        echo();
        clear();
	refresh();
        endwin();
	Exit();
}


Exit()
{
        exit(0);
}


initial()
{
	int  i;
	int  x1, y1;

	x1 = START_X + 2;
	y1 = START_Y + 1;
	for(i = 0; i < cmdno; i++)
	{
	    move (y1, x1);
	    addstr(func[i]);
            x1 = x1 + strlen(func[i]) + 2;
	}
        refresh();
	x1 = START_X + 1;
	y1 = NAME_Y;
        for(i = 0; i < 7; i++)
	{
	    move(y1, x1);
	    addstr(attr[i]);
	    y1 = y1 + 2;
	}
        refresh();
}
		


moveCmd()
{
	int  i, k;
	int  x1, y1;

	x1 = START_X + 2;
	y1 = START_Y + 1;
        move(y1, 0);
        refresh();
        x1= START_X + 2;
        for (i = 0; i < oldindex; i++)
        {
              x1 = x1 + strlen(func[i]) + 2;
        }
	move(y1, x1);
	refresh();
        printf("%s", func[oldindex]);
	refresh();
        x1= START_X + 2;
        for (i = 0; i < cmdindex; i++)
        {
              x1 = x1 + strlen(func[i]) + 2;
        }
	move(y1, 0);
	refresh();
	move(y1, x1);
        refresh();
        printf("%s%s",reverse, func[cmdindex]);
        printf("%s", normal);
        refresh();
        cmdline = 1;
        return;
}



createList()
{
        char   *sp;
        char   data[256];
        char   name[NAMELEN];
        char   use[USELEN];
        char   type[TYPELEN];
        char   host[HOSTLEN];
        char   port[PORTLEN];
        char   baud[BAUDLEN];
        char   shared[6];
        char   tmpport[PORTLEN];
	FILE   *fd;
        struct node  *new_node;

        first = NULL;
        cur_node = NULL;
	fd = fopen(devicefile, "r");
        if (fd == NULL)
        {
/*
                printf("cannot open devicenames\n");
                exit(0);
*/
                return;
        }
        sp = fgets(data, 256, fd);
        while (sp)
        {
		name[0] = '\0';
		use[0] = '\0';
		type[0] = '\0';
		host[0] = '\0';
		port[0] = '\0';
		baud[0] = '\0';
		shared[0] = '\0';
	     	if (strncmp(sp, "Name", 4) == 0)
		{
		   sscanf(sp, "%*s%s", name);
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	           if (strncmp(sp, "Use", 3) == 0)
		   {
		       sscanf(sp, "%*s%s", use);
                   }
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	     	   if (strncmp(sp, "Type", 4) == 0)
		   {
		       sscanf(sp, "%*s%s", type);
                   }
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	     	   if (strncmp(sp, "Host", 4) == 0)
		   {
		       sscanf(sp, "%*s%s", host);
                   }
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	     	   if (strncmp(sp, "Port", 4) == 0)
		   {
		       sscanf(sp, "%*s%s", port);
		/*****************
		       tmpport[0] = '\0';
		       sscanf(sp, "%*s%s", tmpport);
		       if (strlen(tmpport) > 0)
		       {
		          if (strstr(tmpport, "/dev") == NULL)
			      sprintf(port, "/dev/tty%s", tmpport);
		          else
			      sprintf(port, "%s", tmpport);
		       }
		*****************/
                   }
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	     	   if (strncmp(sp, "Baud", 4) == 0)
		   {
		       sscanf(sp, "%*s%s", baud);
                   }
                   if (!(sp = fgets(data, 256, fd)))
                       break;
	     	   if (strncmp(sp, "Shared", 6) == 0)
		   {
		       sscanf(sp, "%*s%s", shared);
                   }
                   new_node = (struct node *)malloc(sizeof(struct node));
                   strcpy(new_node->name, name);
                   strcpy(new_node->type, type);
                   strcpy(new_node->use, use);
                   strcpy(new_node->host, host);
                   strcpy(new_node->port, port);
                   strcpy(new_node->baud, baud);
                   strcpy(new_node->shared, shared);
		   new_node->saved = 1;
                   new_node->next = NULL;
                   new_node->prev = NULL;
                   if (first == NULL)
                       first = new_node;
                   else
                   {
                       cur_node->next = new_node;
                       new_node->prev = cur_node;
                   }
                   cur_node = new_node;
                }
                sp = fgets(data, 256, fd);
	}
        fclose(fd);             
        cur_node = first;
}



exec_cmd()
{
	switch(cmdindex) {
	case 0:       /*   show next device     */
		next();
                NEW = 0;
                change = 0;
		break;
	case 1:       /*   show previous device     */
		previous();
                NEW = 0;
                change = 0;
		break;
        case 2:       /*   modify current device  */
                if (!adm_user)
		{
	            disp_mess("Sorry, you are not root or vnmr1, permision denied");
                    break;
		}
                if ((VIEW) || (NEW))
                {
                    change = 1;
                    cmdline = 0;
                    oldindex = cmdindex;
                    move (NAME_Y, info_col-1);
                    refresh();
                    clrtoeol();
                    mvaddstr(NAME_Y, info_col, "");
                    mvaddch(NAME_Y, START_X + WIDTH, '|');
                    move (USE_Y, info_col-1);
                    refresh();
                    clrtoeol();
                    mvaddstr(USE_Y, info_col, "[Plotter, Printer or Both]");
                    mvaddch(USE_Y, START_X + WIDTH, '|');
                    move (SHARED_Y, info_col-1);
                    refresh();
                    clrtoeol();
                    mvaddstr(SHARED_Y, info_col, "[Yes or No]");
                    mvaddch(SHARED_Y, START_X + WIDTH, '|');
                    move(NAME_Y, first_col);
                    refresh();
		    if (VIEW)
		        edit_node = cur_node;
		    edit_str = edit_node->name;
		    name_ptr = 0;
		    use_ptr = 0;
		    type_ptr = 0;
		    host_ptr = 0;
		    port_ptr = 0;
		    baud_ptr = 0;
		    shared_ptr = 0;
	            max_len = NAMELEN - 1;
		    last_col = info_col - 2;
		    dis_ptr = &name_ptr;
                }
                break;
	case 3:        /*  save devices to file devicenames  */
                if (!adm_user)
		{
	            disp_mess("Sorry, you are not root or vnmr1, permision denied");
                    break;
		}
		copy_dump();
		break;
	case 4:        /*  create new device   */
                if (!adm_user)
		{
	            disp_mess("Sorry, you are not root or vnmr1, permision denied");
                    break;
		}
                oldindex = cmdindex;
		new();
                change = 0;
                VIEW = 0;
                break;
	case 5:        /*  delete device   */
                if (!adm_user)
		{
	            disp_mess("Sorry, you are not root or vnmr1, permision denied");
                    break;
		}
		if (delete())
                {
                     NEW = 0;
                     VIEW = 0;
                     change = 0;
                }
		break;
        case 6:         /*  make printcap  */
                if (!root_user)
		{
	            disp_mess("Sorry, you are not root, permision denied");
                    break;
		}
                execute();
                break;
	case 7:       /*   create help window   */
                help();
                break;
        case 8:       /*  exit  */
		if(!check_save())
		    break;
                ddne();
                break;
	}
        if ((cmdindex != 2) && (cmdindex != 4))
             moveCmd();
}



view(Node)
struct  node  *Node;
{
        sweepscr();
        if (Node == NULL)
             return;
        disp_str(NAME_Y, first_col, Node->name);
        disp_str(USE_Y, first_col, Node->use);
	disp_str(TYPE_Y, first_col, Node->type);
	disp_str(HOST_Y, first_col, Node->host);
	disp_str(PORT_Y, first_col, Node->port);
	disp_str(BAUD_Y, first_col, Node->baud);
	disp_str(SHARED_Y, first_col, Node->shared);
        refresh();
        VIEW = 1;
	name_ptr = 0;
	use_ptr = 0;
	type_ptr = 0;
	host_ptr = 0;
	port_ptr = 0;
	baud_ptr = 0;
	shared_ptr = 0;
        edit_str = edit_node->name;
	max_len = NAMELEN - 1;
        last_col = info_col - 2;
        dis_ptr = &name_ptr;
}


next()
{
        if (cur_node != NULL)
	{
            if (cur_node->next != NULL)
            {
                  cur_node = cur_node->next;
                  view(cur_node);
                  return;
            }
    	}
        disp_mess("no more");
        refresh();
}


previous()
{
        if (cur_node != NULL)
	{
            if (cur_node->prev != NULL)
            {
                    cur_node = cur_node->prev;
                    view(cur_node);
                    return;
            }
    	}
        disp_mess("no more");
        refresh();
}



sweepscr()
{
	int i;

        x = START_X;
	y = NAME_Y;
	move(y, 0);
	refresh();
        for(i = 0; i < 7; i++)
        {
              move(y, first_col);
              refresh();
              clrtoeol();
              mvaddch(y, START_X + WIDTH, '|');
              y = y + 2;
              refresh();
        }
        change = 0;
}
              


new()
{
        edit_node = (struct node *) calloc(1, sizeof(struct node));
        edit_node->saved = 0;
        edit_node->prev = NULL;
        edit_node->next = NULL;
        sweepscr();
        NEW = 1;
        mvaddstr(NAME_Y, info_col, "");
        mvaddstr(USE_Y, info_col, "[Plotter, Printer or Both]");
        mvaddstr(SHARED_Y, info_col, "[Yes or No]");
        refresh();
        move(NAME_Y, first_col);
        last_col = info_col - 2;
	max_len = NAMELEN - 1;
        cmdline= 0;
        refresh();
	name_ptr = 0;
	use_ptr = 0;
	type_ptr = 0;
	host_ptr = 0;
	port_ptr = 0;
	baud_ptr = 0;
	shared_ptr = 0;
        edit_str = edit_node->name;
        dis_ptr = &name_ptr;
}



/*  link the current displayed node to the tree  */
link_node()
{
        if (change)
        {
             change = 0;
             if (is_node_empty())   /* current display is blank  */
             {
                   if (cur_node->prev != NULL)
                   {
                       if (cur_node->next != NULL)
                       {
                             cur_node->prev->next = cur_node->next;
                             cur_node->next->prev = cur_node->prev;
                       }
                       else
                       {
                             cur_node->prev->next = NULL;
                       } 
                   }
                   else
                   {
                       if (cur_node->next != NULL)
                             cur_node->next->prev = NULL;
                   }
                   if (first == cur_node)
                       first = cur_node->next;
	           VIEW = 0;
            }
            return;
        }
	if (NEW)
	{
            NEW = 0;
	    if (is_node_empty())
	    {
                  free((struct node *)edit_node);
		  return;
	    }
	    VIEW = 1;
            if (first == NULL)
	    {
                  first = edit_node;
		  cur_node = edit_node;
		  return;
	    }
	    last = first;
	    for(;;)
            {
	         if (last->next != NULL)
                      last = last->next;
                 else
                      break;
            }
            last->next = edit_node;
            edit_node->prev = last;
            cur_node = edit_node;
	}
}




int delete()
{
struct node  *tmpnode;

        if (VIEW)
        {
               if (!confirm())
                     return(0);
        }
        else
        {
	       disp_mess("nothing to delete");
               refresh();
               return(1);
        }
	sweepscr();
        if (cur_node != NULL)
        {
             tmpnode = cur_node;
             if (cur_node->prev != NULL)
	     {
                  cur_node->prev->next = cur_node->next;
                  if (cur_node->next != NULL)
                  {
                      cur_node->next->prev = cur_node->prev;
                  }
             }
             else
             {
                  if (cur_node->next != NULL)
                  {
                       first = cur_node->next;
                       cur_node->next->prev = NULL;
                  }
                  else
                       cur_node = NULL;
             }
	}
        free((struct node *)tmpnode);
        NEW = 0;
        VIEW = 0;
        return(1);
}



int
is_node_empty()
{
       char  *label;

       label = edit_node->name;
       if(is_string_empty(label))
       {
           label = edit_node->use;
           if(is_string_empty(label))
           {
              label = edit_node->type;
              if(is_string_empty(label))
              {
                  label = edit_node->host;
                  if (is_string_empty(label))
			return(1);
	      }
	   }
       }
       return(0);
}



int
is_string_empty(str)
char   *str;
{
       int    res;

       res = 1;
       while( *str != '\0')
       {
	   if( *str != ' ')
	   {
		res = 0;
		break;
	   }
	   str++;
       }
       return(res);
}

	

int confirm()
{
	int  cur_x, cur_y;

        disp_mess("Hit RET key to confirm delete, other key to cancel");
	cc = getch();
        deleteln();
        insertln();
	refresh();
        if (cc == RETURN)
            return(1);
        else
            return(0);
}

              


int    h_cols;
int    h_lines;

help()
{
	int    modify;
        struct  node  *temp;

        temp = (struct node *) calloc(1, sizeof(struct node));
        if (temp == NULL)
        {
            return;
        }
	h_cols = 72;
	h_lines = 23;
	helpscr = newwin(h_lines, h_cols, 1, 7);
	overlay(helpscr, stdscr);
        clear();
        refresh();
	wmove(helpscr, 0, 0);
        box(helpscr, '.', '.');
	wrefresh(helpscr);
        disp_menu1();
	wrefresh(helpscr);
        cc = wgetch(helpscr);
        if (cc != RETURN)
        {
              wclear(helpscr);
              wrefresh(helpscr);
              wmove(helpscr, 0, 0);
              box(helpscr, '.', '.');
              wrefresh(helpscr);
              disp_menu2();
        }
        wclear(helpscr);
	wrefresh(helpscr);
        delwin(helpscr); 
        modify = change;
        drawbox();
        initial();
        refresh();
        view(cur_node);
        cmdindex = 7;
        moveCmd();
}



drawbox()
{
	int  i;

        x = START_X;
        y = START_Y;
        for(i = 0; i < HEIGHT; i++)
        {
             move(y, x);
             addch('|');
             y++;
        }
        x = START_X + WIDTH;
	y = START_Y;
        for(i = 0; i < HEIGHT; i++)
        {
             move(y, x);
             addch('|');
             y++;
        }
        x = START_X + 1;
        y = START_Y;
        move(y, x);
        for(i = 0; i < WIDTH - 1; i++)
        {
             addch('-');
        }
        x = START_X + 1;
        y = START_Y+2;
        move(y, x);
        for(i = 0; i < WIDTH - 1; i++)
        {
             addch('-');
        }
        x = START_X + 1;
        y = START_Y + HEIGHT - 1;
        move(y, x);
        for(i = 0; i < WIDTH - 1; i++)
        {
             addch('-');
        }
        refresh();
}






disp_menu1()
{
	int nx, ny, count, loop;
        char  list[30];

	ny = 1;
	nx = WIDTH / 2 - 6;
        mvwaddstr(helpscr, ny, nx, "MENU");
        ny++;
        nx = 3;
        mvwaddstr(helpscr, ny, nx, "next -- display the next device attributes");
        ny++;
        mvwaddstr(helpscr, ny, nx, "prev -- display the previous device attributes");
        ny++;
        mvwaddstr(helpscr, ny, nx, "modify -- enter edit mode to modify current device attributes");
        ny++;
        mvwaddstr(helpscr, ny, nx, "save -- save the modified or new device attributes");
        ny++;
        mvwaddstr(helpscr, ny, nx, "create -- create a blank table for entering new device attributes");
        ny++;
        mvwaddstr(helpscr, ny, nx, "delete -- remove the displayed device from file");
        ny++;
        mvwaddstr(helpscr, ny, nx, "exec -- make printcap");
        ny++;
        mvwaddstr(helpscr, ny, nx, "quit -- exit");
        ny = ny + 2;
        mvwaddstr(helpscr, ny, nx, "  The device type must be one of the following:");
        ny++;
        cur_type = typeptr;
	count = 1;
        while(cur_type)
        {
            nx = 3;
            for(loop = 1; loop <= 3; loop++)
            {
                 if (cur_type != NULL)
                 {
                      sprintf(list, "%2d. %s", count, cur_type->type);
                      mvwaddstr(helpscr, ny, nx, list);
                      cur_type = cur_type->next;
                      nx = nx + 22;
                      count++;
                 }
                 else
                      break;
            }
            ny++;
            if (ny > h_lines - 4)
            {
                 if ( !wait_scroll())
                      return;
                 ny = ny - 3;
            }
        }
        mvwaddstr(helpscr, h_lines -3, 6, "* Hit any key for more information *");
        mvwaddstr(helpscr, h_lines -2, 6, "* Hit RET key to return *");
        wrefresh(helpscr);
}



disp_menu2()
{
	int  nx, ny;
        int  i, k;
	char shorthost[64];
        struct  port  *cur_port;
        struct  baud  *cur_baud;
        struct  host_node  *cur_host;

	nx = 4;
      	ny = 1;
        mvwaddstr(helpscr, ny, nx, "system available baud rates are:");
        wrefresh(helpscr);
        cur_baud = baud_1;
        k = 1;
        while( cur_baud != NULL)
	{
              nx = 6;
              ny++;
	      for(i = 0; i < 5; i++)
              {
                    wmove (helpscr, ny, nx);
                    wprintw(helpscr, " %2d. %s", k, cur_baud->rate);
                    nx = nx + 12;
                    wrefresh(helpscr);
                    k++;
                    cur_baud = cur_baud->next;
                    if (cur_baud == NULL)
                          break;
              }
    	}
/***
	nx = 4;
	ny = ny + 2;
        mvwaddstr(helpscr, ny, nx, "system available ports are:");
        wrefresh(helpscr);
        cur_port = port_1;
        k = 1;
        while( cur_port != NULL)
	{
               nx = 6;
               ny = ny + 1;
               if (ny > h_lines - 5)
               {
                   if ( !wait_scroll())
                        return;
                   ny = ny - 3;
               }
               for(i = 0; i < 5; i++)
               {
                    wmove(helpscr, ny, nx);       
                    wprintw(helpscr, " %2d. %s", k, cur_port->num);
                    wrefresh(helpscr);
                    nx = nx + 12;
                    k++;
                    cur_port = cur_port->next;
                    if (cur_port == NULL)
                          break;
               }
	}
***/
	nx = 4;
	ny = ny + 2;
        mvwaddstr(helpscr, ny, nx, "system available hosts are:");
        wrefresh(helpscr);
        cur_host = host_1;
        k = 1;
        while( cur_host != NULL)
	{
               nx = 6;
               ny = ny + 1;
               if (ny > h_lines - 5)
               {
                    if ( !wait_scroll())
                        return;
                    ny = ny -3;
               }
               for(i = 0; i < 4; i++)
               {
		    if (nx + (int)strlen(cur_host->name) > 67)
		    {
		        if (i != 0)
			  break;
		        strncpy(shorthost, cur_host->name, 62);
                        wmove(helpscr, ny, nx);       
                        wprintw(helpscr, " %2d. %s", k, shorthost);
		    }
		    else
		    {  
                        wmove(helpscr, ny, nx);       
                        wprintw(helpscr, " %2d. %s", k, cur_host->name);
		    }
                    wrefresh(helpscr);
		    nx = nx + (((int)strlen(cur_host->name) / 15) + 1) * 15;
                    k++;
                    cur_host = cur_host->next;
                    if (cur_host == NULL)
                          break;
               }
	}
        mvwaddstr(helpscr, h_lines -2, 6, "* Hit RET key to return *");
        wrefresh(helpscr);
        for(;;)
        {
               cc = wgetch(helpscr);
               if (cc == RETURN)
                    return;
 	}
}





int wait_scroll()
{
	int  kk;

        mvwaddstr(helpscr, h_lines -3, 10, 
                   "* Hit RET key to return, any key to scroll *");
        wrefresh(helpscr);
        wmove(helpscr, h_lines -2, h_cols / 2 - 3);
#if defined IRIX || SOLARIS
	printf("%s",reverse);
        wprintw(helpscr, "-more-");
	wrefresh(helpscr);
	printf("%s",normal);
#else
        wprintw(helpscr, "%s-more-", reverse);
        wprintw(helpscr, "%s", normal);
#endif
        wrefresh(helpscr);
        cc = wgetch(helpscr);
        if (cc == RETURN)
        {	
               return(0);
        }
        wmove(helpscr, h_lines - 3, 0);
        wdeleteln(helpscr);
        wdeleteln(helpscr);
        wdeleteln(helpscr);
        winsertln(helpscr);
        winsertln(helpscr);
        winsertln(helpscr);
        wrefresh(helpscr);
        scrollok(helpscr, TRUE);
        scroll(helpscr);
        scroll(helpscr);
        scroll(helpscr);
        scrollok(helpscr, FALSE);
        wmove(helpscr, 0, 0);
        wdeleteln(helpscr);
        winsertln(helpscr);
        mvwaddch(helpscr, h_lines -6, h_cols -1, '.');
        mvwaddch(helpscr, h_lines -6, 0, '.');
        mvwaddch(helpscr, h_lines -5, h_cols -1, '.');
        mvwaddch(helpscr, h_lines -5, 0, '.');
        mvwaddch(helpscr, h_lines -4, h_cols -1, '.');
        mvwaddch(helpscr, h_lines -4, 0, '.');
        mvwaddch(helpscr, h_lines -3, h_cols -1, '.');
        mvwaddch(helpscr, h_lines -3, 0, '.');
        mvwaddch(helpscr, h_lines -2, h_cols -1, '.');
        mvwaddch(helpscr, h_lines -2, 0, '.');
        wmove(helpscr, h_lines - 1, 0);
        for(kk = 0; kk < h_cols; kk++)
             waddch(helpscr, '.'); 
        wmove(helpscr, 0, 0);
        for(kk = 0; kk < h_cols; kk++)
             waddch(helpscr, '.'); 
        wrefresh(helpscr);
        return(1);
}







execute()
{
	int   lp;
	int   error;
	char  cmd[128];
 
        if (!root_user)
            return;
	lp = 0;
        error = 0;
        last = first;
        while (last != NULL) 
	{
              if ((int)strlen(last->name) < 1)
              {
                   error = 1;
                   break; 
              }
              if ((int)strlen(last->use) < 1)
              {
                   error = 1;
                   break; 
              }
              if ((int)strlen(last->type) < 1)
              {
                   error = 1;
                   break; 
              }
              if ((int)strlen(last->host) < 1)
              {
                   error = 1;
                   break; 
              }
              if (strcmp(hostname, last->host) == 0)
              {
                   if ((int)strlen(last->port) < 1)
                   {
                        error = 1;
                        break; 
                   }
                   if ((int)strlen(last->baud) < 1)
                   {
                        error = 1;
                        break; 
                   }
              }
              if ((int)strlen(last->shared) < 1)
              {
                   error = 1;
                   break; 
              }
	      if (strcmp(last->name, "lp") == 0)
              {
		  lp++;
              }
              last = last->next;
	}
        if (error)
        {
              cur_node = last;
              view(cur_node);
	      disp_mess("This device attributes is incomplete");
              moveCmd();
              return;
	}
        if (lp > 1)
        {
	      disp_mess("There are too many \"lp\" name, only one can be allowed");
              moveCmd();
              return;
	}
        if(lp == 0)
        {
	      disp_mess("There is no \"lp\" name for default device");
	      move (MESS_Y+1, START_X);
#if defined IRIX || SOLARIS
	      refresh();
	      printf("%s", reverse);
              printw("Do you want to add one ? [y or n]");
	      printf("%s", bel);
	      refresh();
	      printf("%s", normal);
#else
              printw("%sDo you want to add one ? [y or n]", reverse);	
	      printw("%s", normal);
              printw("%s", bel);
#endif
              refresh();
              for(;;)
              {
                    cc = getch();
                    if (cc == 'y')
                    {
	                  move (MESS_Y, START_X);
                          deleteln();
                          deleteln();
                          refresh();
                          moveCmd();
                          return;
                    }
                    if (cc == 'n')
		    {
	                  move (MESS_Y, START_X);
                          deleteln();
                          deleteln();
                          refresh();
                          break;
                    }
	      }
	      mess = 0;
	}
        copy_dump();
        clear();
        nl();
        echo();
        refresh();
        endwin();
	sprintf(cmd, "%s/bin/makeprintcap %s", sysdir, sysdir);
	system(cmd);
        Exit();
}

        





copy_dump()
{
	FILE  *fdin, *fdout;
	char  data[256];
        char  cmd[128];
        char  *pump;
        int   oldfile;
        int   pumpsign;

	if ((fdin = fopen(devicefile, "r")) == NULL)
        {
             oldfile = 0;
             umask(022);
        }
        else
             oldfile = 1;
        if ((fdout = fopen("/tmp/tempdevice", "w")) == NULL)
	     return;
        pump = "##########################";
	pumpsign = 0;
	while(oldfile && (fgets(data, 256, fdin) != NULL))
        {
             if (data[0] == '#')
	     {
                 if (!pumpsign)
                 {
                      fputs(data, fdout);
                      if (strncmp(data, pump, 25) == 0)
                            pumpsign = 1;
                 }
                 else
                 {
                      if (strncmp(data, pump, 25) != 0)
                      {
                            fputs(data, fdout);
                            pumpsign = 0;
                      }
                 }
             }        
	}
        fclose(fdin);
        last = first;
        while(last != NULL)
        {
              if (!pumpsign)
              {
	            fprintf(fdout, "%s\n", pump); 
	      }
              else
	      {
                    pumpsign = 0;
              }
	      fprintf(fdout, "Name    %s\n", last->name); 
              switch (last->use[0])  {
              case 'p':  last->use[0] = 'P';
                         break;
              case 'b':  last->use[0] = 'B';
                         break;
	      }
	      fprintf(fdout, "Use     %s\n", last->use); 
	      fprintf(fdout, "Type    %s\n", last->type); 
	      fprintf(fdout, "Host    %s\n", last->host); 
	      fprintf(fdout, "Port    %s\n", last->port); 
	      fprintf(fdout, "Baud    %s\n", last->baud); 
              switch (last->shared[0])  {
              case 'y':  last->shared[0] = 'Y';
                         break;
              case 'n':  last->shared[0] = 'N';
                         break;
	      }
	      fprintf(fdout, "Shared  %s\n", last->shared); 
	      last->saved = 1;
              last = last->next;
 	}
	fclose(fdout);
        sprintf(cmd, "rm -f %s", devicefile);
        system(cmd);
        sprintf(cmd, "cp /tmp/tempdevice %s", devicefile);
        system(cmd);
        system("rm -f /tmp/tempdevice");
        if (cmdindex == 3)
	{
	      disp_mess("File is saved");
              moveCmd();
	}
}



select_cmd(ss)
int  ss;
{
	switch(ss) {
	case 'n':   /*next  */
		  cmdindex = 0;
                  break;
     	case 'p':   /*  previous  */
		  cmdindex = 1;
                  break;
     	case 'm':   /*  modify  */
		  cmdindex = 2;
                  break;
     	case 's':   /* save  */
		  cmdindex = 3;
                  break;
     	case 'c':   /*  create  */
		  cmdindex = 4;
                  break;
     	case 'd':   /*  delete  */
		  cmdindex = 5;
                  break;
     	case 'e':   /*  execution  */
		  cmdindex = 6;
                  break;
     	case 'h':   /*  help  */
		  cmdindex = 7;
                  break;
     	case 'q':   /*  quit  */
		  cmdindex = 8;
                  break;
      	default:
                  return;
	}
        moveCmd();
}



static baudTable()
{
	struct  baud  *cur_baud, *new_baud;
        char  out_from_awk[256];
        char  *strptr, *tokptr;
	FILE  *pf;
	char  *value;
	int   repeat;
   	int   speed;


	pf = popen( "awk '/sp#/ { print $0}'\
	            < /etc/gettytab", "r");

	while(fgets(out_from_awk, 256, pf) != NULL)
	{
	     strptr = &out_from_awk[0];
	     while((tokptr = strtok(strptr, ":")) != (char *) 0)
	     {
		  strptr = (char *) 0;
		  if (strncmp(tokptr, "sp#", 3) == 0)
		  {
                       value = tokptr + 3;
                       speed = atoi(value);
                       if (speed <= 0)
                             break;
                       new_baud = baud_1;
                       repeat = 0;
                       while(new_baud != NULL)
                       {
                             if (strcmp(new_baud->rate, value) == 0)
                             {
                                   repeat = 1;
                                   break;
                             }
                             new_baud = new_baud->next;
                       }
                       if (repeat)
                             break;
                       if (baud_1 == NULL)
		       {
		          baud_1 = (struct baud *) malloc(sizeof(struct baud)); 
                          strcpy(baud_1->rate, value);
                          baud_1->next = NULL;
                          baud_1->prev = NULL;
                          cur_baud = baud_1;
		       }
		       else
			{
			     new_baud = (struct baud *) malloc( 
                                          sizeof(struct baud));
                             strcpy(new_baud->rate, value);
                             new_baud->next = NULL;
                             new_baud->prev = cur_baud;
                             cur_baud->next = new_baud;
                             cur_baud = new_baud;
			}
                        break;
                    }
              }
	}
	pclose(pf);
}
	


static portTable()
{
	struct port  *cur_port, *new_port; 
        char  out_from_awk[256];
	FILE  *pf;
	char  *value;

	system("ls /dev > /tmp/tmpdev");
	pf = popen( "awk '/tty[a-z0-9]/ {print $0}'\
		    < /tmp/tmpdev", "r"
		  );
	while( fgets(out_from_awk, 256, pf) != NULL)
	{
		value = &out_from_awk[0] + 3;
                if (port_1 == NULL)
                {
              	     port_1 = (struct port *) malloc (sizeof(struct port));
                     strcpy(port_1->num, value);
                     port_1->num[strlen(port_1->num) -1] = '\0';
                     cur_port = port_1;
                     cur_port->next = NULL;
                }
                else
                {
                     new_port = (struct port *) malloc(sizeof(struct port));
                     strcpy( new_port->num, value);
                     new_port->num[strlen(new_port->num) -1] = '\0';
                     new_port->next = NULL;
                     cur_port->next = new_port;
                     cur_port = new_port;
                }
     	}
        pclose(pf);
        system("rm -f /tmp/tmpdev");
}




static hostTable()
{
	struct host_node  *cur_host, *new_host; 
        char  host_name[256];
 	char  data[HOSTLEN];
	FILE  *fd_hosts;

	host_1 = NULL;
        fd_hosts = popen ("ypcat hosts", "r");
        if (fd_hosts == (FILE *) NULL)
        {
            if ((fd_hosts = fopen("/etc/hosts", "r")) == NULL)
	    {
                printf("cannot open /etc/hosts file\n");
                Exit();
            }
	} 
        else
	{
            if (fgets(host_name, 256, fd_hosts) == NULL)
            {
                pclose(fd_hosts);
                if ((fd_hosts = fopen("/etc/hosts", "r")) == NULL)
	        {
                     printf("cannot open /etc/hosts file\n");
                     Exit();
                }
            }
            else
            {
                pclose(fd_hosts);
                fd_hosts = popen ("ypcat hosts", "r");
            }
        }
      	while( fgets(host_name, 256, fd_hosts) != NULL)
        {
/*
	        if ((host_name[0] == '#') || strlen(host_name) < 5)
*/
	        if (host_name[0] < '0' || host_name[0] > '9')
                     continue;
                sscanf(host_name, "%*s%s", data);
                if (data == NULL)
                     continue;
                if (strcmp(data, "localhost") == 0)
                     continue;
                if (host_1 == NULL)
                {
              	     host_1 = (struct host_node *) malloc 
                               (sizeof(struct host_node));
		     if ((int)strlen(data) > HOSTLEN-2)
                         strncpy(host_1->name, data, HOSTLEN-2);
		     else
                         strcpy(host_1->name, data);
                     cur_host = host_1;
                     cur_host->next = NULL;
                }
                else
                {
                     new_host = (struct host_node *) malloc
                                 (sizeof(struct host_node));
		     if ((int)strlen(data) > HOSTLEN-2)
                         strncpy(new_host->name, data, HOSTLEN-2);
		     else
                         strcpy( new_host->name, data);
                     new_host->next = NULL;
                     cur_host->next = new_host;
                     cur_host = new_host;
                }
     	}
        fclose(fd_hosts);
}




sort_baud()
{
	int   i;
        int   loop1, loop2;
        int   a, b;
	struct baud  *p, *q;

	p = baud_1;
        i = 0;
	while(p != NULL)
	{
             i++;
             p = p->next;
	}
  	for(loop1 = 1; loop1 < i; loop1++)
	{
	     p = baud_1;
	     for(loop2 = 1; loop2 < i; loop2++)
	     {
                   q = p->next;
                   a = atoi(p->rate);
                   b = atoi(q->rate);
                   if (a > b)
	           {
			p->next = q->next;
			q->next = p;
                        q->prev = p->prev;
			p->prev = q;
			if (p == baud_1)
			       baud_1 = q;
                        else
                               q->prev->next = q;
                    }
                    else
                        p = p->next;
	      }
	}
}
           



int line_check()
{
	int  error, i, xx, p;
        int  repeat;
        struct  baud  *baud_node;
        struct  port  *port_node;
        struct  host_node  *host_n;

        getyx(stdscr, y, x);
        if ((int)strlen(edit_node->host) <= 0)
            local_host = 0;
        else
        {
            if (strcmp(hostname, edit_node->host) == 0)
                 local_host = 1;
            else
                 local_host = 0;
        }
        if (!local_host)
	{
              if ((y == PORT_Y) || (y == BAUD_Y))
                     return(1);
	}
        error = 0;
        repeat = 0;
	if ((int)strlen(edit_str) <= 0)
	     return(1);
	switch(y)  {
        case NAME_Y:      /*  name  */
                      last = first;
                      while (last != NULL)
                      {
                            if (strcmp(last->name, edit_str) == 0)
                            {
                                  if ((change) && (last == cur_node))
                                  {
                                        last = last->next;
                                        continue;
                                  }
                                  error = 1;
                                  repeat = 1;
                                  break;
                            }
                            last = last->next;
                      }
                      break;
 
	case USE_Y:      /*  use  */
	              if ((strcmp(edit_str, "Plotter")!= 0) &&
			  (strcmp(edit_str, "plotter")!= 0))
                      {
                         if ((strcmp(edit_str, "Printer") != 0) &&
			     (strcmp(edit_str, "printer")!= 0))
	                 {
                             if ((strcmp(edit_str, "Both")!= 0) && 
				 (strcmp(edit_str, "both")!= 0))
                                     error = 1;
                         }
                      }
                      break;
			
	case TYPE_Y:     /*  type  */
                      error = 1;
                      cur_type = typeptr;
                      while(cur_type)
                      {
	                  if (strcmp(edit_str, cur_type->type) == 0)
                          {
                                error = 0;
                                break;
                          }
                          cur_type = cur_type->next;
	              }
                      break;
	case HOST_Y:     /*  hostname  */
                      error = 1;
                      host_n = host_1;
	              while(host_n != NULL)
	              {
	                  if (strcmp(edit_str, host_n->name) == 0)
	                  {
                               error = 0;
		               break;
	                  }
                          host_n = host_n->next;
	              }
		      if (error)
		      {
		 	 if (strcmp(edit_str, hostname) == 0)
			   error = 0;
		      }
                      break;
	case PORT_Y:     /*  port  */
                      error = 0;
/***
                      port_node = port_1;
                      while(port_node != NULL)
                      {
                          if(strcmp(edit_str, port_node->num) == 0)
                          {
                               error = 0;
                               break;
	                  }
                          port_node = port_node->next;
	              }
**/
                      break;
	case BAUD_Y:     /*  baud  */
#if defined IRIX || SOLARIS
                      error = 0;
#else
                      error = 1;
                      baud_node = baud_1;
	              while(baud_node != NULL)
	              {
	                   if (strcmp(edit_str, baud_node->rate) == 0)
	                   {
                                error = 0;
		                break;
	                   }
	                   baud_node = baud_node->next;
	              }
#endif
                      break;
	case SHARED_Y:     /*  shared  */
                      if ((strcmp(edit_str, "Yes") != 0) && 
			  (strcmp(edit_str, "yes") != 0))
                      {
                            if ((strcmp(edit_str, "No") != 0) && 
				(strcmp(edit_str, "no") != 0))
                                  error = 1;
                      }
       }
       if (!error)
           return(1);
  	move(y, first_col);
	refresh();
#if defined IRIX || SOLARIS
	printf("%s", reverse);
#else
	printw("%s", reverse);
#endif
	refresh();
	p = *dis_ptr;
	xx = first_col;
	while ( xx <= last_col && *(edit_str+p) != NULL)
	{
	     addch(*(edit_str+p));
	     p++;
	     xx++;
        }
#if defined IRIX || SOLARIS
 	refresh();
	printf("%s", normal);
#else
	printw("%s", normal);
#endif
 	refresh();
	move(MESS_Y, 5);
        if (repeat)
        {
#if defined IRIX || SOLARIS
	      printf("%s",reverse);
	      printw("This name already exists");
	      refresh();
#else
	      printw("%sThis name already exists", reverse);	
#endif
        }
        else
        {
#if defined IRIX || SOLARIS
	      printf("%s",reverse);
	      printw("This attribute is not available");
	      refresh();
#else
	      printw("%sThis attribute is not available", reverse);	
#endif
        }
#if defined IRIX || SOLARIS
	printf("%s", normal);
#else
	printw("%s", normal);
#endif
 	refresh();
	move(MESS_Y+1, 5);
#if defined IRIX || SOLARIS
 	refresh();
	printf("%s", reverse);
	printw("Do you want to correct it ? [y or n]");
	printf("%s", bel);
	refresh();
	printf("%s", normal);
#else
	printw("%sDo you want to correct it ? [y or n]", reverse);	
	printw("%s", normal);
	printw("%s", bel);
#endif
 	refresh();
	for(;;)
	{
	     cc = getch();
	     if ((cc == 'y') || (cc == 'n'))
		   break;
	}
	move(MESS_Y, 0);
	deleteln();
	deleteln();
	refresh();
	move (y, first_col);
	refresh();
	p = *dis_ptr;
	xx = first_col;
	while ( xx <= last_col && *(edit_str+p) != NULL)
	{
	     addch(*(edit_str+p));
	     p++;
	     xx++;
        }
        if (cc == 'y')
        {
	    move (y, first_col);
            refresh();
            return(0);
	}
	else
	    return(1);
}



typeTable()
{
	char  data[256];
	char  name[20];
	char  *sp;
	FILE  *fd;
        struct  devicetype  *new_type;

	typeptr = NULL;
        fd = fopen(typefile, "r");
        if (fd == NULL)
        {
            printf("can not open %s file\n", typefile);
	    sleep(3);
            return;
        }
	sp = fgets(data, 256, fd);
	while(sp)
	{
	    if (strncmp(sp, "PrinterType", 11) == 0)
	    {
       		sscanf(sp, "%*s%s", name);
                if (name != NULL) 
                {
                     new_type = (struct devicetype *) malloc(sizeof(struct devicetype));
                     strcpy(new_type->type, name);
                     new_type->next = NULL;
                     if (typeptr == NULL)
                     {
                         typeptr = new_type;
                     }
                     else
                     {
                         cur_type->next = new_type;
                     }
                     cur_type = new_type;
                }
           }
           sp = fgets(data, 256, fd);
	}
        fclose(fd);
}
	

disp_mess(text)
char   *text;
{
     move(MESS_Y, START_X);
#if defined IRIX || SOLARIS
     refresh();
     printf("%s", reverse);
     printw("%s", text);
     printf("%s", bel);
     refresh();
     printf("%s", normal);
#else
     printw("%s%s", reverse, text);
     printw("%s", normal);
     printw("%s", bel);
#endif
     refresh();
     mess = 1;
}


shift_right()
{
     int   yy, xx;
     char  *str;

     getyx(stdscr, yy, xx);
     if (*dis_ptr <= 0)
	  return;
     *dis_ptr = *dis_ptr - 1;
     addch(*(*dis_ptr + edit_str));
     xx = first_col;
     move(yy, xx);
     refresh();
     str = *dis_ptr + edit_str;
     while (xx <= last_col && *str != NULL)
     {
	  addch(*(str++));
	  xx++;
     }
}


shift_left()
{
     int   yy, xx, len, p;
     char  *str;

     getyx(stdscr, yy, xx);
     len = strlen(edit_str);
     p = *dis_ptr + xx - first_col;
     if ( p >= len )
	return;
     *dis_ptr = *dis_ptr + 1;
     xx = first_col;
     move(yy, xx);
     refresh();
     str = *dis_ptr + edit_str;
     while (xx <= last_col)
     {
	  if (*str == NULL)
		addch(' ');
	  else
	        addch(*(str++));
	  xx++;
     }
}


add_char(cc)
char   cc;
{
     int   xx, yy, cindex, len, k;
     
     if (cc < FIRSTCHAR || cc > LASTCHAR)
	return;
     getyx(stdscr, yy, xx);

     len = strlen(edit_str);
     if (len >= max_len)
     {
        disp_mess("too long");
	move(yy, xx);
	refresh();
	return;
     }
     cindex = *dis_ptr + xx - first_col;
     *(edit_str+len+1) = '\0';
     while(len > cindex)
     {
	*(edit_str+len) = *(edit_str+len-1);
	len--;
     }
     *(edit_str+cindex) = cc;
     if ( xx == last_col)
     {
	*dis_ptr = *dis_ptr + 1;
	move(yy, first_col);
	refresh();
 	k = *dis_ptr;
        while(k <= cindex)
        {
	      addch(*(edit_str+k));
	      k++;
	}
        cindex++;
     }
		
     move(yy, xx);
     refresh();
     k = xx;
     while(k <= last_col && *(edit_str + cindex) != '\0')
     {
	addch(*(edit_str+cindex));
	cindex++;
	k++;
     }
     if (xx < last_col)
	xx++;
     move(yy, xx);
     refresh();
}


delete_char()
{
     int   xx, yy, cindex, len, k;
     char  *str;
     
     len = strlen(edit_str);
     if (len <= 0)
	return;
     getyx(stdscr, yy, xx);
     cindex = *dis_ptr + xx - first_col;
     str = edit_str + cindex;
     while(cindex < len)
     {
	*(edit_str+cindex) = *(edit_str+cindex+1);
	cindex++;
     }
     while (xx <= last_col)
     {
	if (*str == '\0')
	{
	    addch(' ');
	    break;
	}
	addch(*(str++));
	xx++;
     }
} 



disp_str(posy, posx, str)
int    posy, posx;
char   *str;
{
     int   xx;

     xx = START_X + WIDTH -2;
     move(posy, posx);
     while( *str != NULL && posx < xx)
     {
	addch(*(str++));
	posx++;
     }
}


set_new_par()
{
     int   xx, yy;

     getyx(stdscr, yy, xx);
     last_col = info_col - 2;
     switch (yy) {
      case NAME_Y:  max_len = NAMELEN -1;
		    edit_str = edit_node->name;
		    dis_ptr = &name_ptr;
		    break;
      case USE_Y:   max_len = USELEN -1;
		    edit_str = edit_node->use;
		    dis_ptr = &use_ptr;
		    break;
      case TYPE_Y:  max_len = TYPELEN -1;
		    edit_str = edit_node->type;
		    dis_ptr = &type_ptr;
		    break;
      case HOST_Y:  max_len = HOSTLEN -1;
		    edit_str = edit_node->host;
		    dis_ptr = &host_ptr;
		    last_col = START_X + WIDTH - 2;
		    break;
      case PORT_Y:  max_len = PORTLEN -1;
		    edit_str = edit_node->port;
		    dis_ptr = &port_ptr;
		    if (local_host)
		    {
                         if ((int)strlen(edit_str) <= 0)
			 {
                            mvaddstr(PORT_Y, first_col, "/dev/");
			    sprintf(edit_str, "/dev/");
			    refresh();
			 }
		    }
		    break;
      case BAUD_Y:  max_len = BAUDLEN -1;
		    edit_str = edit_node->baud;
		    dis_ptr = &baud_ptr;
		    break;
      case SHARED_Y:  max_len =  4;
		    edit_str = edit_node->shared;
		    dis_ptr = &shared_ptr;
		    break;
     }
}


int
check_save()
{
      last = first;
      while (last != NULL)
      {
	   if (!last->saved)
	   {
	      disp_mess("There are new entries not saved");
	      move (MESS_Y+1, START_X);
#if defined IRIX || SOLARIS
              refresh();
	      printf("%s", reverse);
              printw("Do you want to save them? [y or n]");
              printf("%s", bel);
              refresh();
	      printf("%s", normal);
#else
              printw("%sDo you want to save them? [y or n]", reverse);	
	      printw("%s", normal);
              printw("%s", bel);
#endif
              refresh();
              for(;;)
              {
                    cc = getch();
                    if (cc == 'y')
                    {
			copy_dump();
			return(1);
                    }
                    if (cc == 'n')
		    {
	                  move (MESS_Y, START_X);
                          deleteln();
                          deleteln();
                          refresh();
	                  disp_mess("Do you want to quit now? [y or n]");
			  for(;;)
			  {
				cc = getch();
				if (cc == 'y')
				     return(1);
				if (cc == 'n')
				{
	                             move (MESS_Y, START_X);
                                     deleteln();
				     mess = 0;
				     return(0);
				}
			  }
                    }
	      }
	   }
	   last = last->next;
      }
      return(1);
}

#ifdef SOLARIS

/*  Rather than rework the make file to find the library
    with gethostname, we present this Solaris equivalent.       */
 
#include <sys/systeminfo.h>

static int
gethostname( bufaddr, buflen )
char *bufaddr;
int buflen;
{
        int     ival;

        ival = sysinfo( SI_HOSTNAME, bufaddr, buflen );
        if (ival > 0) {
                bufaddr[ ival ] = '\0';
                return( 0 );
        }
        else
          return( ival );
}
#endif
