/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* test_storage.c: test driver for the Storage module */

#ifndef lint
	static char Sid[] = "@(#)testtree.c 18.1 03/21/08 20:01:57 Dave's test driver for the Storage module";
#endif

#include <ctype.h>
#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif
#include <string.h>
#ifndef MSDOS
#include <unistd.h>
#endif

#include "boolean.h"
#include "error.h"
#include "storage.h"
#ifdef DEBUG
#include "debug_alloc.h"
#include "stack.h"
#include "tree.h"
#endif

typedef struct {
   char key[4];
   char data[12];
} TESTDATA;

#ifdef __STDC__

static void help (void);
static void mainloop (void);
static TESTDATA *get_data (void);
static int print_data (void *key, void *data);
static int kill_key (void *key, void *data);

extern int  get_reply (char *prompt, char *reply, int max, int required);
extern void setbuf (FILE *stream, char *buf);
extern int  strcmp (char *s1, char *s2);

#else

static void help( /* void */ );
static void mainloop( /* void */ );
static TESTDATA *get_data( /* void */ );
static int print_data( /* void *key, void *data */ );
static int kill_key( /* void *key, void *data */ );

extern int  get_reply( /* char *prompt, char *reply, int max, int required */ );
extern void setbuf( /* FILE *stream, char *buf */ );
extern int  strcmp( /* char *s1, char *s2 */ );

#endif

#ifdef __STDC__
int main (int argc, char **argv)
#else
int main (argc, argv)
 int    argc;
 char **argv;
#endif
{
   (void)setbuf (stdout, (char *)NULL);
   (void)setbuf (stderr, (char *)NULL);

   help();
   mainloop();

   return 0;

}  /* end of function "main" */

#ifdef __STDC__
static void help (void)
#else
static void help()
#endif
{
   error ("");
   error ("C   display count of entries");
   error ("c   clear this storage area");
   error ("d   delete entry");
   error ("f   display first entry");
   error ("m   memory used by storage area");
   error ("n   display next entry");
   error ("i   insert new node");
   error ("l   look for key");
   error ("p   print contents of storage");
   error ("r   release this storage area");
   error ("s   show tree structure");
   error ("q   quit");
   error ("?   display command summary");

}  /* end of function "help" */

#ifdef __STDC__
static void mainloop (void)
#else
static void mainloop()
#endif
{
   Storage store = (Storage)NULL;
   char  cmd [10];
   char *p;
   TESTDATA *d;

   for (;;)
   {
      if (store == (Storage)NULL)
      {
         if ( (store = Storage_create (100, strcmp, kill_key)) == (Storage)NULL)
            fatal (1, "mainloop: couldn't create storage area");

         ERROR(("mainloop:"));
         ERROR(("store=0x%08X",(STORAGE *)store));
         ERROR(("store->root=0x%08X",((STORAGE *)store)->root));
#        ifdef xDEBUG
         ERROR(("store->compare=0x%08X (%s)",((STORAGE *)store)->compare,
                ((STORAGE *)store)->compare == strcmp ? "OK" : "wrong"));
         ERROR(("store->delete=0x%08X (%s)",((STORAGE *)store)->delete,
                ((STORAGE *)store)->delete == kill_key ? "OK" : "wrong"));
         ERROR(("store->size=%d",((STORAGE *)store)->size));
#        endif
      }

      cmd[0] = '\0';
      get_reply ("Enter command:", cmd, sizeof(cmd), FALSE);

      if ( (p = strtok (cmd, " \t")) == (char *)NULL)
         exit (0);

      /* force the message "Unknown command" for responses longer
         than one character */
      if (strlen (p) > 1)
         *p = '\1';

      switch (*p)
      {
         case 'C':
            error ("count = %d", ((STORAGE *)store)->count);
            break;

         case 'c':
            if (Storage_clear (store) != S_OK)
               error ("error clearing storage area");
            break;

         case 'd':
            if (get_reply ("Enter two-letter key:", cmd, sizeof(cmd), TRUE) != 2)
               error ("Not two letters; try again!");

            else if (Storage_delete (store, (void *)cmd) != S_OK)
               error ("'%s' not found", cmd);
            break;

         case 'f':
            if ( (p = (char *)Storage_first (store)) != (char *)NULL)
               error ("DATA '%s'", p);
            break;

         case 'n':
            if ( (p = (char *)Storage_next (store)) != (char *)NULL)
               error ("DATA '%s'", p);
            break;

         case 'i':
            if ( (d = get_data()) != (TESTDATA *)NULL)
            {
               print_data ((void *)d->key, (void *)d->data);
               Storage_insert (store, (void *)d->key, (void *)d->data, REPLACE);
            }
            break;

         case 'l':
            if (get_reply ("Enter two-letter key:", cmd, sizeof(cmd), TRUE) != 2)
               error ("Not two letters; try again!");

            else if ( (p = (char *)Storage_search (store, (void *)cmd)) != (char *)NULL)
               print_data ((void *)cmd, (void *)p);

            else
               error ("'%s' not found", cmd);
            break;

         case 'm':
            check_list();
            break;

         case 'p':
            Storage_output (store, print_data);
            break;

         case 'r':
            if (Storage_release (store) != S_OK)
               error ("error releasing storage area");
            else
               store = (Storage)NULL;
            break;

         case 's':
            Storage_display (store);
            break;

         case '\0':
         case 'q':
            check_list();
            exit (0);

         case '?':
            help();
            break;

         default:
            error ("Unknown command - use '?' for help");
            break;
      }
   }
}  /* end of function "mainloop" */

#ifdef __STDC__
static TESTDATA *get_data (void)
#else
static TESTDATA *get_data()
#endif
{
   TESTDATA *d;

   if ( (d = (TESTDATA *)calloc (1, sizeof(TESTDATA))) == (TESTDATA *)NULL)
      error_exit (sys_error ("get_data: calloc data node"));

   if (get_reply ("Enter two-letter key:", d->key, sizeof(d->key), TRUE) != 2)
   {
      error ("Not two letters; try again!");
      free ((char *)d);
      d = (TESTDATA *)NULL;
   }
   else
   {
      get_reply ("Enter data:", d->data, sizeof(d->data), FALSE);
   }
   ERROR(("get_data:d=0x%08X,d->key=0x%08X,d->data=0x%08X",d,d->key,d->data));
   return d;

}  /* end of function "get_data" */

#ifdef __STDC__
static int print_data (void *key, void *data)
#else
static int print_data (key, data)
 void *key;
 void *data;
#endif
{
   printf ("KEY '%s': DATA '%s'\n", (char *)key, (char *)data);

   return (S_OK);

}  /* end of function "print_data" */

#ifdef __STDC__
static int kill_key (void *key, void *data)
#else
static int kill_key (key, data)
 void *key;
 void *data;
#endif
{
   ERROR(("kill_key: key '%s'=0x%08X,data=0x%08X",(char *)key,key,data));

   free ((char *)key);

   return (0);

}  /* end of function "kill_key" */
