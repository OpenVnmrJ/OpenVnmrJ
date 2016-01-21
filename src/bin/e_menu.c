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

/****************************************************************/
/* menu		- service menu routines for enter program	*/
/****************************************************************/

#include <stdio.h>
#include "enter.h"

#ifdef  DEBUG
#define RPRINT(str)		 fprintf(stderr,str)
#define RPRINT1(str, arg1)	 fprintf(stderr,str,arg1)
#define RPRINT2(str, arg1, arg2) fprintf(stderr,str,arg1, arg2)
#else   DEBUG
#define RPRINT(str) 
#define RPRINT1(str, arg1) 
#define RPRINT2(str, arg1, arg2) 
#endif  DEBUG

extern char		*default_userdir;
extern char		*vnmrsys;

static char		mlabel[MAX_BUTTONS][MAXLABELSIZE];
static char		mstring[MAX_BUTTONS][MAXLABELSIZE];
static int		arraysize;


/***********************/
menu(menuname)
/***********************/
char	*menuname;
{
FILE	*menufile;
char	menufilepath[MAXPATHL];
char	but_text[80];
int	c, i, j;
 
   RPRINT("menu: starting\n");
   if (strlen(menuname)>MAXPATHL)
   {  fprintf(stderr,"menu name too long");
      return 1;
   }
   showButton("0");
   RPRINT("menu: erasing done\n");
   strcpy(menufilepath,default_userdir);
   strcat(menufilepath,"/asm/");
   strcat(menufilepath,menuname);
   menufile = fopen(menufilepath,"r");
   if (menufile == 0)
   {  strcpy(menufilepath,vnmrsys);
      strcat(menufilepath,"/asm/");
      strcat(menufilepath,menuname);
      menufile = fopen(menufilepath,"r");
   }
   
   if (menufile == 0)
   {  fprintf(stderr, "menu %s does not exits",menuname);
      return(1);
   }
   i = 0;
   do
   {  do
      {  c = fgetc(menufile);
      } while (c!=EOF && c!='\'');
      j = 0;
      c = fgetc(menufile);
      while (c!=EOF && c!='\'')
      {  mlabel[i][j] = (char) c;
         c = fgetc(menufile);
         j++;
      }
      mlabel[i][j] = '\0';
      do
      {  c = fgetc(menufile);
      } while (c!=EOF && c!='\'');
      j = 0;
      c = fgetc(menufile);
      while (c!=EOF && c!='\'')
      {  mstring[i][j] = (char) c;
         c = fgetc(menufile);
         j++;
      }
      mstring[i][j] = '\0';
      i++;
   } while (c != EOF && i < MAX_BUTTONS);
   fclose(menufile);
 
   arraysize =  i  - 1;
   for (j=0; j<arraysize; j++)
       RPRINT2("label=%s\tstring=%s\n",mlabel[j],mstring[j]);
   RPRINT1("menu: arraysize = %d\n", arraysize);
   for (i=0; i<arraysize; i++)
   {  
      sprintf(but_text,"%d:%s", i+1, mlabel[i]);
      showButton(but_text);
   }
   RPRINT("menu: exiting\n");
   return(0);
}


/*----------------------------------------------------------------------
|
|	menu_but_handler
|
|	This routine is called when a button is clicked.  It figures
|	out what button has been clicked and loads the correct text.
|
+---------------------------------------------------------------------*/

menu_but_handler(fnum)
int	fnum;
{
   int	caret_item, active_item;

   if (fnum > arraysize) return;
   if ( !strcmp(mlabel[fnum],"More") )
   {
      menu(mstring[fnum]);
   }
   else
   {
      caret_item = get_active_item();
      if (caret_item < 0)
	      return;
      set_text_item_value(caret_item, mstring[fnum]);
      switch (caret_item) {
	case  sample_item:
			   active_item = user_item;
              		   menu("e_user");
			   break;
	case  user_item:
			   active_item = macro_item;
              		   menu("e_macro");
			   break;
	case  macro_item:
			   active_item = solvent_item;
              		   menu("e_solvent");
			   break;
	case  solvent_item:
			   active_item = text_item;
              		   menu("e_text");
			   break;
	case  text_item:
             		   setprompt();
	default:
			   return;
			   break;
	}
      set_active_item(active_item);
   }
   set_enter_panel_focus();
}
