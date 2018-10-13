/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Note that the following codes are stolen from Xview example		*
*  "menu_dir2.c" with some modification.				*
*  It is used to reculsively generate a pullright menu for all		*
*  directories/files below it as a needed basis.			*
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <sys/stat.h>
#include <dirent.h>
#ifndef MAXPATHLEN
#include <sys/param.h>
#endif /* MAXPATHLEN */
#include "stderr.h"

#define	KEY_ADDR_DATA	1

/* Because there is no way to destroy the title item of a menu  */
/* we use static variable.					*/
static char title[50];

Menu filelist_menu_pullright(Menu_item, Menu_generate, u_long, char *);
char **get_dir_contents(char *path);
static void my_action_proc(Menu, Menu_item);
static Menu gen_pullright(Menu_item, Menu_generate);
static char *getfilename(char *);
static Menu_item add_path_to_menu(char *, u_long);
static int str_cmp(char **, char **);

/************************************************************************
*									*
*  This function will starts a pull-right menu from a specified		*
*  directory.  It lists all files in a menu.				*
*									*/
Menu
filelist_menu_pullright(Menu_item /*mi*/, Menu_generate op,
			u_long callback_func, char *start_dirpath)
{
   static Menu menu=NULL;

   //fprintf(stderr,"filelist_menu_pullright(): mi=%d, op=%d\n", mi, op);/*CMP*/
   if (op == MENU_DISPLAY)
   {
      Menu_item item;
      char *dirpath;

      if (start_dirpath == NULL)
	 dirpath = ".";			/* Use current directory */
      else
	 dirpath = start_dirpath;	/* Use a specified directory */

      //
      // Don't destroy old menus, may still be in use by pinned-up menus.
      // ***********************************************************
      // 		THIS IS A MEMORY LEAK
      // ***********************************************************
      // Have to figure out how to tell a menu is no longer needed.
      // Or, better, rewrite the pinnable, popup menus as popup panels
      // and then restore this commented put stuff.
      //
      //if (menu)
      //{
      //   int j;
      //   free((char *)xv_get(menu, MENU_CLIENT_DATA));
 
         /* Destroy all items in the menu				*/
         /* Note that these codes are needed because XView DOES NOT	*/
         /* destroy all items in a menu although XView manual		*/
         /* says it does.						*/
      //   for (j=(int)xv_get(menu, MENU_NITEMS); j>0; j--)
      //   {
      //      item = (Menu_item)xv_get(menu, MENU_NTH_ITEM, j);
      //      xv_set(menu, MENU_REMOVE, j, NULL);
      //      xv_destroy(item);
      //   }
      //   xv_destroy(menu);
      //} 

      if (item = add_path_to_menu(dirpath, callback_func))
      {
         menu = (Menu)xv_get(item, MENU_PULLRIGHT);
 
         /* Destroy the item since we already get its pullrigh menu	*/
         xv_destroy(item);
      }
      else
      {
         menu = xv_create(NULL, MENU,
                MENU_TITLE_ITEM,        strcpy(title, dirpath),
                MENU_STRINGS,   "<No such directory>", NULL,
                NULL);
      }
   }
   return(menu);
}

/************************************************************************
*									*
*  Routine to be called when the user has selected a menu item.		*
*									*/
static void
my_action_proc(Menu menu, Menu_item menu_item)
{
   void (*notify_func)(char *, char *);

   /* Get the callback function address */
   notify_func = (void(*)(char *, char *))xv_get(menu_item,
			  XV_KEY_DATA, KEY_ADDR_DATA);

   /* Call the user function */
   if (notify_func)
      (*notify_func)((char *)xv_get(menu, MENU_CLIENT_DATA),
	          (char *)xv_get(menu_item, MENU_STRING));
}

/************************************************************************
*									*
* Return an allocated char * that points to the last item in a path.	*
*									*/
static char *
getfilename(char *path)
{
    char *p;

    if (p = strrchr(path, (int) '/'))
        p++;
    else
        p = path;
    return (strdup(p));
}

/************************************************************************
*									*
* gen_pullright() is called in the following order:			*
*   Pullright menu needs to be displayed. (MENU_PULLRIGHT)		*
*   Menu is about to be dismissed (MENU_DISPLAY_DONE)			*
*      User made a selection (before menu notify function)		*
*      After the notify routine has been called.			*
* The above order is done whether or not the user makes a		*
* menu selection.							*
* Always return newly created menu.					*
*									*/
static Menu
gen_pullright(Menu_item mi, Menu_generate op)
{
   Menu menu;
   Menu_item new_item, old_item = mi;
   char buf[MAXPATHLEN];

   if (op == MENU_DISPLAY)
   {
      menu = (Menu)xv_get(old_item, MENU_PARENT);
      (void)sprintf(buf, "%s/%s",
            xv_get(menu, MENU_CLIENT_DATA), xv_get(old_item, MENU_STRING));

      /* get old menu and free it -- we're going to build another */
      if (menu = (Menu)xv_get(old_item, MENU_PULLRIGHT))
      {
	 int j;
	 Menu_item item;

         free((char *)xv_get(menu, MENU_CLIENT_DATA));
         
	 /* Destroy all items in the menu 			    */
	 /* Note that these codes are needed because XView DOES NOT */
	 /* destroy all items in a menu although XView manual	    */
	 /* says it does.					    */
	 for (j=(int)xv_get(menu, MENU_NITEMS); j>0; j--)
	 {
	    item = (Menu_item)xv_get(menu, MENU_NTH_ITEM, j);
	    xv_set(menu, MENU_REMOVE, j, NULL);
	    xv_destroy(item);
	 }

         xv_destroy(menu);
      }
      if (new_item = add_path_to_menu(buf, 
	   (u_long)xv_get(old_item, XV_KEY_DATA, KEY_ADDR_DATA)))
      {
          menu = (Menu)xv_get(new_item, MENU_PULLRIGHT);
          xv_destroy(new_item);
          return menu;
      }
   }
   if (!(menu = (Menu)xv_get(old_item, MENU_PULLRIGHT)))
   {
      menu = (Menu)xv_create(NULL, MENU,
              MENU_STRINGS, "Couldn't build a menu.", NULL,
              NULL);
   }
   return menu;
}

/************************************************************************
*									*
* The path passed in is scanned via readdir().  For each file in the	*
* path, a menu item is created and inserted into a new menu.  That	*
* new menu is made the PULLRIGHT_MENU of a newly created panel item	*
* for the path item originally passed it.  Since this routine is	*
* recursive, a new menu is created for each subdirectory under the	*
* original path.							*
*									*/
static Menu_item
add_path_to_menu(char *path, u_long addr_notify)
{
   struct stat         s_buf;		/* file status */
   Menu_item           mi;		/* menu item for pullright */
   Menu                next_menu;	/* newly created menu */
   char                buf[MAXPATHLEN];	/* temporary buffer for filename */
   static int          recursion;	/* number of depth */
   char *fname;
   char *pc;

   /* don't add a folder to the list if user can't read it */
   if (stat(path, &s_buf) == -1 || !(s_buf.st_mode & S_IREAD))
      return NULL;

   if (s_buf.st_mode & S_IFDIR)
   {
      int cnt = 0;
      int len;

      /* Only created pullright menu as necessary.  Hence return it */
      if (recursion)
         return (Menu_item)-1;
      recursion++;

      // Get an alphabetized list of the directory contents
      char **contents = get_dir_contents(path);

      /* don't bother adding to list if we can't scan it */
      if ( ! contents ){
	  return NULL;
      }

      next_menu = (Menu)xv_create(XV_NULL, MENU, NULL);

      /* For each file, append it to the menu.  If it is a directory  	*/
      /* append a pullright arrow.  It is is unreadable file, make it 	*/
      /* inactive.							*/
      char **dp;
      for (dp=contents; *dp; dp++){
         if (strcmp(*dp, ".") && strcmp(*dp, "..")){
            (void) sprintf(buf, "%s/%s", path, *dp);
            mi = add_path_to_menu(buf, addr_notify);

            if (!mi || mi == (Menu_item)-1)
	    {
               int do_gen_pullright = (mi == (Menu_item)-1);

               mi = (Menu_item)xv_create(XV_NULL, MENUITEM,
                        MENU_STRING, getfilename(*dp),
                        MENU_RELEASE,
                        MENU_RELEASE_IMAGE,
            		MENU_NOTIFY_PROC,   my_action_proc,
	    		XV_KEY_DATA,	KEY_ADDR_DATA, addr_notify,
                        NULL);

               if (do_gen_pullright)
	          /* Append a pullright arrow and deactivate NOTIFY_PROC */ 
                  xv_set(mi, MENU_GEN_PULLRIGHT, gen_pullright,
				MENU_NOTIFY_PROC,	NULL,
			NULL);
               else
                  /* unreadable file or dir - deactivate item */
                  xv_set(mi, MENU_INACTIVE, TRUE, NULL);
	    }
            xv_set(next_menu, MENU_APPEND_ITEM, mi, NULL);
            cnt++;
         }
	 free(*dp);	// Free filename memory
      }
      free((char *)contents);	// Free pointer memory

      /* Create a menu item to be return to the caller */
      mi = (Menu_item)xv_create(XV_NULL, MENUITEM,
            MENU_STRING,        getfilename(path),
            MENU_RELEASE,
            MENU_RELEASE_IMAGE,
            MENU_NOTIFY_PROC,   my_action_proc,
	    XV_KEY_DATA,	KEY_ADDR_DATA, addr_notify,
            NULL);

      if (!cnt)
      {
	 /* Create an empty menu to indicate "No files" */
	 /* Note that no NOTIFY_PROC for this item      */
	 Menu_item temp;
         temp = (Menu_item)xv_create(XV_NULL, MENUITEM,
                  MENU_STRING, strdup("<No files>"),
                  MENU_RELEASE,
                  MENU_RELEASE_IMAGE,
	    	  XV_KEY_DATA,	KEY_ADDR_DATA, addr_notify,
                  NULL);
	 xv_set(next_menu, MENU_APPEND_ITEM, temp, NULL);
      }


      /* Truncate the path name for TITLE */
      if ((len = strlen(path)) > 25)
      {
	 (void)strcpy(title,"..."); 
         (void)strcat(title, path + len - 25);
      }
      else
         (void)strcpy(title, path);

      xv_set(next_menu,
               MENU_TITLE_ITEM, title,
               MENU_CLIENT_DATA, strdup(path),
               NULL);
      xv_set(mi, MENU_PULLRIGHT, next_menu, NULL);

      recursion--;
      return mi;
   }

   /* Return a non directory file */
   /* Translate any \ in file name to / */
   fname = getfilename(path); /* Do not free--belongs to menu */
   while (pc=strchr(fname, '\\') ){
       *pc = '/';
   }
   mi = (Menu_item)xv_create(NULL, MENUITEM,
			     MENU_STRING, fname,
			     MENU_RELEASE,
			     MENU_RELEASE_IMAGE,
			     MENU_NOTIFY_PROC, my_action_proc,
			     XV_KEY_DATA, KEY_ADDR_DATA, addr_notify,
			     NULL);
   return mi;
}

char **
get_dir_contents(char *path)
{
    DIR                 *dirp;		/* directory pointer */
    struct dirent       *dp;		/* directory pointer for each file */
    
    struct Filelist{
	char *name;
	struct Filelist *next;
    } *filelist;			/* Linked list of files */
    
    /* Return failure if we can't read it */
    if (!(dirp = opendir(path))){
	return NULL;
    }

    // Fill up Linked list of file names
    struct Filelist *entry;
    struct Filelist *prev = 0;
    int nnames;
    for (nnames=0; dp = readdir(dirp); nnames++){
	entry = (struct Filelist *)malloc(sizeof(struct Filelist));
	if (nnames == 0){
	    // Initialize the Filelist header
	    filelist = entry;
	}
	entry->name = (char *)malloc(strlen(dp->d_name)+1);
	strcpy(entry->name, dp->d_name);
	entry->next = 0;
	if (prev){
	    prev->next = entry;
	}
	prev = entry;
    }
    closedir(dirp);

    // Allocate an array of pointers to the names (plus a null pointer)
    char **namev = (char **)malloc((nnames + 1) * sizeof(char *));
    int i;
    for (i=0, entry=filelist; i<nnames; i++, entry=entry->next){
	namev[i] = entry->name;
    }
    namev[i] = 0;	// Terminate list

    // Release memory for linked list (but not for names!).
    for (entry=filelist; entry; ){
	prev = entry;
	entry = entry->next;
	free((char *)prev);
    }

    // Sort the items in the list
    qsort(namev, nnames, sizeof(char *),
	  (int (*) (const void *, const void *)) str_cmp);

    return namev;
}

/************************************************************************
*									*
*  Compare two string pointers.						*
*									*/
static int
str_cmp(char **a, char **b)
{
    return (strcmp(*a, *b));
}
