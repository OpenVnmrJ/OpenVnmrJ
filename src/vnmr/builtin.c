/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------
|
|       builtin.c
|
|       These routines are some of the builtin commands of the
|       magical interpreter.  The commands are
|
|               echo         - simple echo command such as found on Unix 
|               substr       - pick a substring out of a string variable 
|               length       - return length of a string argument 
|               input        - get input from the keyboard 
|               real         - force a variable to a real 
|               string       - force a variable to be a string  
|               format       - similar to the printf format command 
|               groupcopy    - copy a group of variables between trees.     
|               create       - create a variable
|               prune        - prune the current tree
|               paramcopy    - copy a parameter to another parameter
|               destroy      - destroy a variable
|               destroygroup - destroy all variables of a group
|               display      - display variables      
|               mstat        - display memory statistic of allocated space
|               getfile      - return number of files or file name in a
|                              directory
|
+----------------------------------------------------------------------*/

#include "symtab.h"
#include "variables.h"
#include "vnmrsys.h"
#include "group.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pvars.h"
#include "allocate.h"
#include "buttons.h"
#include "tools.h"
#include "wjunk.h"

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#else 
#include  stat
#include  "dirent.h"
#endif 

#ifdef  DEBUG
extern int Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	if (Tflag) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3)
#define TPRINT4(str, arg1, arg2, arg3, arg4) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define TPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#define TPRINT4(str, arg1, arg2, arg3, arg4) 
#define TPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif 

extern void  outjShow();
extern int   outjInit();
extern void  outjCtInit();
extern void  RshowJVals(symbol *s);
extern void  RshowVar(FILE *f, varInfo *v);
extern void  RshowVars(FILE *f, symbol *s);
extern void  RshowVals(FILE *f, symbol *s);
extern void  RcountJVals(symbol *s);
extern int   P_deleteGroupVar(int tree, int group);
extern void  checkarray(char *name, short size);
extern int   find_param_file(char *init, char *final);
extern int   goodType(char *type);
extern int   goodGroup(char *group);
extern void  copyGVars(symbol **fp, symbol **tp, int group);
extern void  P_balance(symbol **tp);
extern void  jsetcopyGVars(int flag);
extern void  resetMagicVar(varInfo *v, char *name);
extern void  nmr_exit(char *modeptr);
extern void  ignoreSigpipe();
extern int   beepOn;
extern int   interuption;
extern pid_t HostPid;

#ifdef VNMRJ
extern void popBackingStore();
#endif

struct idpack {
        const char *id;
        struct idpack *next;
        };

typedef struct idpack PACK;

/* Function prototypes */
static void enter(const char *, PACK **);
static int isDirectory(const char *);
static int isNotEntered(const char *nameid, PACK *rootpt);

#ifdef VNMRJ
extern int VnmrJViewId;
#endif 

void clearRets(int retc, char *retv[])
{   int i;

    for (i=0; i<retc; ++i)
        retv[i] = NULL;
}

/*-----------------------------------------------------------------------
|
|       beeper
|
|       Turn on or off the beeper on Werrprintf
|
+----------------------------------------------------------------------*/

int beeper(int argc, char *argv[], int retc, char *retv[])
{

    (void) argc;
    if (retc > 0)
    {
       retv[0] = intString( beepOn );
    }
    else if (strcmp(argv[0],"beepon") == 0)
    {
       beepOn = 1;
    }
    else if (strcmp(argv[0],"beepoff") == 0)
    {
       beepOn = 0;
    }
    RETURN;
}

/*-----------------------------------------------------------------------
|
|       exit
|
|       A simple command to gracefully exit nmr program
|
+----------------------------------------------------------------------*/

int nmrExit(int argc, char *argv[], int retc, char *retv[])
{   extern char vnMode[];

    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    ignoreSigpipe();
    nmr_exit(vnMode);   
    RETURN;
}

/*-----------------------------------------------------------------------
|
|       echo
|
|       A simple builtin command, sort of like echo(1) from UNIX.
|
+----------------------------------------------------------------------*/

int echo(int argc, char *argv[], int retc, char *retv[])
{   int i;
    int newLine;
    int noArgs;

    noArgs  = 1;
    newLine = 1;
    Wturnoff_buttons();
    Wshow_text();
    for (i=1; i<argc; ++i)
        if ((strcmp(argv[i],"-n") == 0) && noArgs)
            newLine = 0;
        else
        {   noArgs = 0;
            Wscrprintf("%s ",argv[i]);
        }
    if (newLine)
        Wscrprintf("\n");
    if (retc)
      {  Werrprintf("echo does not return values\n");
         clearRets(retc,retv);
         ABORT;
      }
    RETURN;
}

/*-----------------------------------------------------------------------
|  is_whitespace : compare character to a whitespace string
+----------------------------------------------------------------------*/
int is_whitespace(char c, int len, char delimiter[])
{
  register int j;
  for (j=0; j<=len; j++)
  {
    if (c == delimiter[j])
      return(1);
  }
  return(0);
}

void trLine(char inLine[], char subLine[], char newChar[], char outLine[] )
{
   int i=0;
   int j=0;
   int k;
   int foundOne = 0;
   int ch;
   int trCh;

   while ( (ch = inLine[i]) != '\0')
   {
      if (j < MAXSTR*4 - 1)
      {
         k = 0;
         foundOne = 0; 
         while ( (trCh = subLine[k]) != '\0')
         {
            if ( (trCh == '\\') && (subLine[k+1] != '\0'))
            {
               if (subLine[k+1] == 'r')
               {
                  trCh ='\r';
                  k++;
               }
               else if (subLine[k+1] == 'n')
               {
                  trCh ='\n';
                  k++;
               }
               else if (subLine[k+1] == 't')
               {
                  trCh ='\t';
                  k++;
               }
            }
            if (ch == trCh)
            {
               foundOne = 1;
               break;
            }
            k++;
         }
         if ( foundOne )
         {
            if (newChar[0] != '\0')
               outLine[j++] = newChar[0];
         }
         else
         {
            outLine[j++] = ch;
         }
      }
      i++;
   }
   outLine[j] = '\0';
}

/*-----------------------------------------------------------------------
|
|       substr
|
+----------------------------------------------------------------------*/

int substr(int argc, char *argv[], int retc, char *retv[])
{
    int count,index,length,number;
    int i;
    int mode, replace, length2;

    replace = 0;
    mode = argc;
    if ( (argc == 6) && !strcmp(argv[2],"find") &&
         !strcmp(argv[4],"delimiter") )
    {
       replace = 2;
       mode = 3;
    }
    else if ((argc == 5) && (strcmp(argv[2],"tr")==0))
       mode = 3;
    else if (argc == 5)
    {
       if (strcmp(argv[3],"delimiter")==0)
       {
          replace = 2;
          mode = 3;
       }
       else if ( !strcmp(argv[2],"find") && !strcmp(argv[4],"csv"))
       {
          mode = 3;
       }
       else
       {
          replace = 1;
          mode = 4;
       }
    }
    else if ((argc == 4) && (strcmp(argv[3],"delimiter")==0))
       mode = 3;
    else if ((argc == 4) && (strcmp(argv[3],"csv")==0))
       mode = 3;
    else if ((argc == 4) && (strcmp(argv[2],"find")==0))
       mode = 3;
    else if ((argc == 4) && (strcmp(argv[2],"basename")==0))
       mode = 3;
    else if ((argc == 4) && (strcmp(argv[2],"dirname")==0))
       mode = 3;
    else if ((argc == 4) && (strcmp(argv[2],"squeeze")==0))
       mode = 3;
    else if (((argc == 3) || (argc == 4)) && (strcmp(argv[2],"trim")==0))
       mode = 3;
    if (mode == 4)
    {
        TPRINT3("substr: substr('%s',%s,%s)...\n",argv[1],argv[2],argv[3]);
        length = strlen(argv[1]);
        if (isReal(argv[2]))
        {   index = (int)stringReal(argv[2]);
            if (index < 1)
               index = 1;
            if (replace && (index > length+1))
               index = length+1;
            if ((0 < index) && (index <= length+1))
            {   if (isReal(argv[3]))
                {   number = (int)stringReal(argv[3]);
                    count  = (length-index)+1;
                    TPRINT1("substr: ...length= %d\n",length);
                    TPRINT1("substr:    number= %d\n",number);
                    TPRINT1("substr:    count = %d\n",count);
                    if (count < number)
                    {   number = count;
                        TPRINT1("substr: ...reset number to %d\n",count);
                    }
                    if (1 <= retc)
                    {
                        length2 = 0;
                        if (replace)
                        {
                           length2 = strlen(argv[4]);
                           count = index + number;
                           number = length - number + length2;
                        }
                        if (number < 0)
                           number = 0;
                        if ( (retv[0]=(char *)allocateWithId(number+1,"substr")) )
                        {
                          if (replace)
                          {
                            int j;
                            i = 0;
                            for (j=1; j<index; ++j)
                            {
                                retv[0][i] = argv[1][i];
                                i++;
                            }
                            for (j=0; j<length2; ++j)
                            {
                                retv[0][i] = argv[4][j];
                                i++;
                            }
                            j = 0;
                            while (i < number)
                            {
                                retv[0][i] = argv[1][count-1+j];
                                i++;
                                j++;
                            }
                            retv[0][i] = '\0';
                          }
                          else
                          {
                            for (i=0; i<number; ++i)
                            {
                                TPRINT3("substr:    retv[0][%d] = argv[1][%d] (='%c')\n",i,index-1+i,argv[1][index-1+i]);
                                retv[0][i] = argv[1][index-1+i];
                            }
                            TPRINT1("substr:    retv[0][%d] = '\\0'\n",count);
                            retv[0][number] = '\0';
                          }
                        }
                        else
                        {   Werrprintf("substr: out of memory!");
                            clearRets(retc,retv);
                        }
                        if (1 < retc)
                            Werrprintf("substr: only one value returned!");
                        RETURN;
                    }
                }
                else
                {   Werrprintf("substr: index out of range!");
                    clearRets(retc,retv);
                    ABORT;
                }
            }
            else
              { if (index>0)
                  { if (retc!=1)
                      { Werrprintf("substr: one value to return!");
                        ABORT;
                      }
                    else
                      { if ( (retv[0]=(char *)allocateWithId(2,"substr")) )
                          retv[0][0]='\0';
                        RETURN;
                      }
                  }
                else 
                  { Werrprintf("substr: bad starting index!");
                    clearRets(retc,retv);
                    ABORT;
                  }
              }
           }
         else
         {   Werrprintf("substr: bad character count!");
             clearRets(retc,retv);
             ABORT;
         }
    }
    else if (mode == 3)
    {
        TPRINT2("substr: substr('%s',%s)...\n",argv[1],argv[2]);
        if (isReal(argv[2]))
        {
            register int c;
            int      first;
            int      csv = 0;
            char     stringval[1024],
                     delimiter[MAXSTR] = " \t";

            if ((argc > 4) && (replace == 2))
            {   strncpy(delimiter,argv[4],MAXSTR-1);
                if (strlen(argv[4]) > (MAXSTR-1))
                  Werrprintf("substr: delimiter too long, truncating to %d characters",MAXSTR-1);
            }
            if ((argc == 4) && ! strcmp(argv[3],"csv") )
               csv = 1;
            index = (int)stringReal(argv[2]);
            if (index < 1)
            {   Werrprintf("substr: bad word count!");
                clearRets(retc,retv);
                ABORT;
            }
            length = strlen(argv[1]);
            i = 0;
            count = 1;
            if (csv)
            {
               length2 = 0;
               while ((count < index) && (i < length) && ( (c = argv[1][i]) != '\0') )
               {
                    if  (argv[1][i] == '"')
                    {
                       i++;
                       while ( ((c = argv[1][i]) != '\0') && (argv[1][i] != '"') )
                          i++;
                       if (argv[1][i] == '"')
                          i++;
                    }
                    if (argv[1][i] == ',')
                    {
                       count++;
                    }
                    i++;
               }
            }
            else
            {
               length2 = strlen(delimiter);
               while ((count < index) && (i < length))
               {
                  while (((c = argv[1][i]) != '\0') && (is_whitespace(c,length2,delimiter)))
                    i++;   /* skip white space */
                  while (((c = argv[1][i]) != '\0') && (!is_whitespace(c,length2,delimiter)))
                    i++;   /* skip word */
                  count++;
               }
               while (((c = argv[1][i]) != '\0') && (is_whitespace(c,length2,delimiter)))
                 i++;   /* skip white space */
            }
            if ( (argv[1][i] == '\0') && ! csv)
            {
               if (retc >= 1)
                   clearRets(retc,retv);
               else
                   Winfoprintf("substr: word %d is not present in string '%s'", index,argv[1]);
               RETURN;
            }
            if ( (count < index) && csv)
            {
               if (retc >= 1)
                   clearRets(retc,retv);
               else
                   Winfoprintf("substr: word %d is not present in string '%s'", index,argv[1]);
               RETURN;
            }
            count = 0;
            first = i+1;
            if (csv)
            {
               while ( ((c = argv[1][i]) != '\0') && (argv[1][i] != ',') )
               {
                    if  (argv[1][i] == '"')
                    {
                       stringval[count++] = argv[1][i++];
                       while ( ((c = argv[1][i]) != '\0') && (argv[1][i] != '"') )
                          stringval[count++] = argv[1][i++];
                       if (argv[1][i] == '"')
                          stringval[count++] = argv[1][i++];
                    }
                    if (argv[1][i] != ',')
                    {
                       stringval[count++] = argv[1][i++];
                    }
               }
            }
            else
            {
               while (((c = argv[1][i]) != '\0') && (!is_whitespace(c,length2,delimiter))
                       && (count < 1000)) /* get word */
               {
                  stringval[count++] = argv[1][i++];
               }
            }
            stringval[count] = '\0';
            if (retc >= 1)
            {
                retv[0] = newString(stringval);
                if ( (retc >= 2) && ! csv)
		   retv[1] = realString( (double) first );
                if ( (retc >= 3) && ! csv)
		   retv[2] = realString( (double) count );
                if ( (retc >= 4) && ! csv)
                {
                   /* return what is left after removing the requested word */
                   /* If first word, remove preceding and trailing white space */
                   /* If not the first word, remove preceding white space only */
                   i = first + count - 1;
                   if (index == 1)
                   {
                      /* skip trailing white space */
                      while (((c = argv[1][i]) != '\0') &&
                              (is_whitespace(c,length2,delimiter)))
                         i++;
		      retv[3] = newString( & argv[1][i] );
                   }
                   else
                   {
                      int len;
                      char *tmpstr;
                      char *ptr;

                      len = strlen(argv[1]);
                      tmpstr = (char *)allocateWithId(len,"substr");
                      strncpy(tmpstr,argv[1],first-1);
                      *(tmpstr+first-1) = '\0';
                      i = first - 2;
                      while ( (i > 0) && ((c = argv[1][i]) != '\0') &&
                               (is_whitespace(c,length2,delimiter)))
                      {
                         *(tmpstr+i) = '\0';
                         --i;
                      }
                      i = first + count - 1;
                      ptr = argv[1];
                      strcat(tmpstr,(ptr+i) );
		      retv[3] = newString( tmpstr );
                      release(tmpstr);
                   }
                }
            }
            else
                Winfoprintf("word %d is %s starting at character %d",
                   index,stringval,first);
            RETURN;
        }
        else if (! strcmp(argv[2],"wc"))
        {
            register int c;
            int      csv = 0;
            char     delimiter[MAXSTR] = " \t";

            if ((argc > 4) && (replace == 2))
            {   strncpy(delimiter,argv[4],MAXSTR-1);
                if (strlen(argv[4]) > (MAXSTR-1))
                  Werrprintf("substr: delimiter too long, truncating to %d characters",MAXSTR-1);
            }
            if ((argc == 4) && ! strcmp(argv[3],"csv") )
               csv = 1;
            length = strlen(argv[1]);
            i = 0;
            count = 0;
            if (csv)
            {
               length2 = 0;
               while ( (i < length) && ( (c = argv[1][i]) != '\0') )
               {
                    if  (argv[1][i] == '"')
                    {
                       i++;
                       while ( ((c = argv[1][i]) != '\0') && (argv[1][i] != '"') )
                          i++;
                       if (argv[1][i] == '"')
                          i++;
                    }
                    if (argv[1][i] == ',')
                    {
                       count++;
                    }
                    i++;
               }
               if (length > 0)
                  count++;
            }
            else
            {
               length2 = strlen(delimiter);
               while (i < length)
               {
                  while (((c = argv[1][i]) != '\0') && (is_whitespace(c,length2,delimiter)))
                    i++;   /* skip white space */
                  if (argv[1][i] != '\0')
                  {
                     while (((c = argv[1][i]) != '\0') &&
                             (!is_whitespace(c,length2,delimiter)))
                       i++;   /* skip word */
                     count++;
                  }
               }
            }
            if (retc >= 1)
            {
                retv[0] = intString(count);
            }
            else
                Winfoprintf("string '%s' has %d word%s.",
                             argv[1],count,(count==1) ? "" : "s");
            RETURN;
        }
        else if (! strcmp(argv[2],"basename"))
        {
           char tmp[MAXSTR*4];
           char tmp2[MAXSTR*4];
           int len;
           char *ptr;

           /* First remove trailing '/' characters */
           strcpy(tmp2,argv[1]);
           len = strlen(tmp2);
           while ( (len > 1) && tmp2[len-1] == '/')
           {
              tmp2[len-1] = '\0';
              len--;
           }
           if (len == 0)
              strcpy(tmp2,".");
           ptr = strrchr(tmp2,'/');
           if ( ! ptr)
           {
              strcpy(tmp,tmp2);
           }
           else
           {
              if (len == 1)
                 strcpy(tmp,ptr);
              else
                 strcpy(tmp,ptr+1);
           }
           if (retc >= 1)
           {
             if (retc == 1)
             {
                if (argc > 3)
                {
                   size_t alen, tlen;
                   alen = strlen(argv[3]);
                   tlen = strlen(tmp);
                   if ( (tlen > alen) && !strcmp((tmp+tlen-alen),argv[3]) )
                      *(tmp+tlen-alen) = '\0';
                }
                retv[0] = newString(tmp);
             }
             else if ( (strcmp(tmp,"..") == 0) || (strcmp(tmp,".") == 0) )
             {
                retv[0] = newString(tmp);
                retv[1] = newString("");
             }
             else
             {
                ptr = strrchr(tmp,'.');
                if ( ( ! ptr) || ( ! strlen(ptr+1)) )
                {
                   retv[0] = newString(tmp);
                   retv[1] = newString("");
                }
                else
                {
                   if (argc > 3)
                   {
                      char *ptr2;
                      ptr2 = strstr(argv[3],ptr);
                      if (ptr2)
                      {
                         retv[1] = newString(ptr+1);
                         *ptr = '\0';
                         retv[0] = newString(tmp);
                      }
                      else
                      {
                         retv[0] = newString(tmp);
                         retv[1] = newString("");
                      }
                   }
                   else
                   {
                      retv[1] = newString(ptr+1);
                      *ptr = '\0';
                      retv[0] = newString(tmp);
                   }
                }
             }
           }
           else
           {
             Winfoprintf("basename of %s is %s", argv[1],tmp);
           }

        }
        else if (! strcmp(argv[2],"dirname"))
        {
           char tmp[MAXSTR*4];
           char tmp2[MAXSTR*4];
           int len;
           char *ptr;

           /* First remove trailing '/' characters */
           strcpy(tmp2,argv[1]);
           len = strlen(tmp2);
           while ( (len > 1) && tmp2[len-1] == '/')
           {
              tmp2[len-1] = '\0';
              len--;
           }
           if (len == 0)
              strcpy(tmp2,".");
           ptr = strrchr(tmp2,'/');
           if ( ! ptr)
           {
              strcpy(tmp,".");
           }
           else
           {
              strcpy(tmp,tmp2);
              if (ptr != tmp2)
                 tmp[ptr - tmp2] = '\0';
           }
           if (retc >= 1)
           {
              retv[0] = newString(tmp);
              if (retc >= 2)  /* extract basename also */
              {
                 ptr = strrchr(tmp2,'/');
                 if ( ! ptr)
                 {
                    strcpy(tmp,tmp2);
                 }
                 else
                 {
                    if (strlen(tmp2) == 1)
                       strcpy(tmp,tmp2);
                    else
                       strcpy(tmp,ptr+1);
                 }
                 if (retc == 2)
                   retv[1] = newString(tmp);
                 else if ( (strcmp(tmp,"..") == 0) || (strcmp(tmp,".") == 0) )
                 {
                    retv[1] = newString(tmp);
                    retv[2] = newString("");
                 }
                 else
                 {
                    ptr = strrchr(tmp,'.');
                    if ( ( ! ptr) || ( ! strlen(ptr+1)) )
                    {
                       retv[1] = newString(tmp);
                       retv[2] = newString("");
                    }
                    else
                    {
                       if (argc > 3)
                       {
                          char *ptr2;
                          ptr2 = strstr(argv[3],ptr);
                          if (ptr2)
                          {
                             retv[2] = newString(ptr+1);
                             *ptr = '\0';
                             retv[1] = newString(tmp);
                          }
                          else
                          {
                             retv[1] = newString(tmp);
                             retv[2] = newString("");
                          }
                       }
                       else
                       {
                          retv[2] = newString(ptr+1);
                          *ptr = '\0';
                          retv[1] = newString(tmp);
                       }
                    }
                 }
              }
           }
           else
           {
             Winfoprintf("dirname of %s is %s", argv[1],tmp);
           }

        }
        else if (! strcmp(argv[2],"find"))
        {
            register int c;
            int      first;
            char     delimiter[MAXSTR] = " \t";
            int      found;
            int      numchar = 0;
            int      wlen;
            int      csv = 0;

            if ((argc > 5) && (replace == 2))
            {   strncpy(delimiter,argv[5],MAXSTR-1);
                if (strlen(argv[5]) > (MAXSTR-1))
                  Werrprintf("substr: delimiter too long, truncating to %d characters",MAXSTR-1);
            }
            if ((argc == 5) && ! strcmp(argv[4],"csv") )
               csv = 1;
            if ( (strlen(argv[3]) < 1) && ! csv)
            {   Werrprintf("substr: bad search word");
                clearRets(retc,retv);
                ABORT;
            }
            wlen = strlen(argv[3]);
            first = found = 0;
            length = strlen(argv[1]);
            i = 0;
            count = 0;
            if (csv)
            {
               length2 = 0;
               first=i;
               numchar = 0;
               while ( (i < length) && ( (c = argv[1][i]) != '\0') && !found )
               {
                    if  (argv[1][i] == '"')
                    {
                       i++;
                       numchar++;
                       while ( ((c = argv[1][i]) != '\0') && (argv[1][i] != '"') )
                       {
                          i++;
                          numchar++;
                       }
                       if (argv[1][i] == '"')
                       {
                          i++;
                          numchar++;
                       }
                    }
                    if (argv[1][i] == ',')
                    {
                       count++;
                       found =  ((wlen == numchar) &&
                            !strncmp(&argv[1][first],argv[3],numchar));
                       if ( found )
                       {
                          break;
                       }
                       numchar=0;
                       first = i+1;
                    }
                    else
                    {
                       numchar++;
                    }
                    i++;
               }
               if ( ! found )
               {
                  found =  ((wlen == numchar) &&
                            !strncmp(&argv[1][first],argv[3],numchar));
                  if (found)
                     count++;
               }
            }
            else
            {
            length2 = strlen(delimiter);
            while (! found  && (i < length))
            {

               count++;
               while (((c = argv[1][i]) != '\0') && (is_whitespace(c,length2,delimiter)))
                 i++;   /* skip white space */
               numchar = 0;
               first = i;
               while (((c = argv[1][i]) != '\0') && (!is_whitespace(c,length2,delimiter)))
               {
                 i++;   /* skip word */
                 numchar++;
               }
               found =  ((wlen == numchar) &&
                         !strncmp(&argv[1][first],argv[3],numchar));
            }
            while (((c = argv[1][i]) != '\0') &&
                    (is_whitespace(c,length2,delimiter)))
              i++;   /* skip white space */
            }
            if (!found)
            {
               if (retc >= 1)
               {
                   retv[0] = intString( 0 );
                   if (retc >= 2)
		      retv[1] = intString( i+1 );
                   if (retc >= 3)
		      retv[2] = intString( 0 );
                   if (retc >= 4)
		      retv[3] = newString( argv[1] );
               }
               else
                   Winfoprintf("substr: word %s is not present in string '%s'", argv[3],argv[1]);
               RETURN;
            }
            if (retc >= 1)
            {
                retv[0] = intString( count);
                if (retc >= 2)
		   retv[1] = intString( first+1 );
                if (retc >= 3)
		   retv[2] = intString( numchar );
                if (retc >= 4)
                {
                   if (csv)
                   {
                      if (count == 1)
                      {
                         if ( ! length)
		            retv[3] = newString( argv[1] );
                         else
		            retv[3] = newString( & argv[1][numchar+1] );
                      }
                      else
                      {
                         char *tmpstr;

                         tmpstr = (char *)allocateWithId(length,"substr");
                         i=0;
                         while (i < first)
                         {
                            tmpstr[i] = argv[1][i];
                            i++;
                         }

                         if ( (i+numchar) == length) // last word
                         {
                            if (i)
                               i--;  // remove last comma
                         }
                         else
                         {
                            while (argv[1][i+numchar+1] != '\0')
                            {
                               tmpstr[i] = argv[1][i+numchar+1];
                               i++;
                            }
                         }
                         tmpstr[i] = '\0';
		         retv[3] = newString( tmpstr );
                         release(tmpstr);
                      }
                   }
                   else
                   {
                   /* return what is left after removing the requested word */
                   /* If first word, remove preceding and trailing white space */
                   /* If not the first word, remove preceding white space only */
                   if (count == 1)
                   {
                      i = first + numchar;
                      /* skip trailing white space */
                      while (((c = argv[1][i]) != '\0') &&
                              (is_whitespace(c,length2,delimiter)))
                         i++;
		      retv[3] = newString( & argv[1][i] );
                   }
                   else
                   {
                      int len;
                      char *tmpstr;
                      char *ptr;

                      len = strlen(argv[1]);
                      tmpstr = (char *)allocateWithId(len,"substr");
                      strncpy(tmpstr,argv[1],first-1);
                      *(tmpstr+first-1) = '\0';
                      i = first - 2;
                      while ( (i > 0) && ((c = argv[1][i]) != '\0') &&
                               (is_whitespace(c,length2,delimiter)))
                      {
                         *(tmpstr+i) = '\0';
                         --i;
                      }
                      i = first + numchar;
                      ptr = argv[1];
                      strcat(tmpstr,(ptr+i) );
		      retv[3] = newString( tmpstr );
                      release(tmpstr);
                   }
                   }
                }
            }
            else
                Winfoprintf("word %s starts at character %d",argv[3],first);
            RETURN;
        }
        else if (! strcmp(argv[2],"squeeze"))
        {
            char squeezed[MAXSTR*4];
            int i=0;
            int j=0;
            char ch;

            while ( (ch = argv[1][i]) != '\0')
            {
               if (j < MAXSTR*4 - 1)
               {
                  squeezed[j++] = ch;
                  if (ch == argv[3][0])
                  {
                     while ( argv[1][i+1] == argv[3][0] )
                     {
                        i++;
                     }
                  }
               }
               i++;
            }
            squeezed[j] = '\0';
            if (retc)
               retv[0] = newString(  squeezed );
            else
               Winfoprintf("squeezed:  %s",squeezed);
        }
        else if (! strcmp(argv[2],"trim"))
        {
            char trimmed[MAXSTR*4];
            char delimiter[8] = " \t";
            int i=0;
            int j=0;
            int k;
            int trimit;
            int didOne = 0;
            char ch;
            int len;
            char *dels;

           
            if (argc == 3)
               dels = delimiter;
            else
               dels = argv[3];
            len = strlen(dels);
            while ( (ch = argv[1][i]) != '\0')
            {
               if (j < MAXSTR*4 - 1)
               {
                  if ( ! didOne )
                  {
                     trimit = 0;
                     for (k=0; k<len; k++)
                        if (ch == dels[k])
                           trimit = 1;
                     if ( ! trimit)
                     {
                        trimmed[j++] = ch;
                        didOne = 1;
                     }
                  }
                  else
                  {
                     trimmed[j++] = ch;
                  }
               }
               i++;
            }
            trimmed[j] = '\0';
            if (retc)
               retv[0] = newString(  trimmed );
            else
               Winfoprintf("trimed:  %s",trimmed);
        }
        else if (! strcmp(argv[2],"tr"))
        {
            char trimmed[MAXSTR*4];
            trLine(argv[1], argv[3], argv[4], trimmed);
            if (retc)
               retv[0] = newString(  trimmed );
            else
               Winfoprintf("substr tr:  %s",trimmed);
        }
        else
        {   Werrprintf("substr: bad word index!");
            clearRets(retc,retv);
            ABORT;
        }
    }
    else
    {   Werrprintf("substr: wrong number of arguments!");
        clearRets(retc,retv);
        ABORT;
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	length
|
|	length returns the number of characters in the first argument
|	If a return vector is available, the value is stored in the
|	1st return variable; otherwise, the length is displayed
|
+-----------------------------------------------------------------------------*/

int length(int argc, char *argv[], int retc, char *retv[])
{
	int	len;

	if (argc < 2) {
		Werrprintf( "%s:  must provide a string", argv[ 0 ] );
		ABORT;
	}

	len = strlen( argv[ 1 ] );
	if (retc < 1)
	  Winfoprintf( "%s has %d characters",argv[ 1 ], len );

/*  Note manner in which the value is returned from the command.
    If this convention is not followed, the VNMR program will crash.	*/

	else {
		retv[ 0 ] = realString( (double) len );
	}

	RETURN;
}

/*------------------------------------------------------------------------------
|
|	Strstr(string1, string2 <,'last'>)
|
|	strstr returns the starting position of the first occurrence
|	of string2 in string1.  The first character position is 1,
|	not 0, as is the C style.  Returns 0 if string2 does not occur
|       in string 1.  Returns 1 if string2 is an empty string.
|       Returns 0 if string1 is an empty string.
|       If a third 'last' argument is passed, it will look for the last
|       occurrence of string2 in string1.
+-----------------------------------------------------------------------------*/
int Strstr(int argc, char *argv[], int retc, char *retv[])
{
   int   len;
   int   ret;
   char *ptr;


   if ( (argc != 3) && (argc != 4) )
   {
      Werrprintf( "Usage: %s(string1,string2)", argv[ 0 ]);
      ABORT;
   }
   ptr = argv[1];
   len = strlen( argv[2] );
   ret = 1;
   if (len)
   {
      if (!strlen(argv[1]) || ((ptr = strstr(argv[1],argv[2])) == NULL) )
      {
         ret = 0;
      }
      else
      {
         ret = ptr - argv[1];
         ret += 1;
         if ( (argc == 4) && (strcmp(argv[3],"last") == 0) )
         {
            char *ptr2;
            ptr2 = ptr + 1;
            while ((ptr = strstr(ptr2,argv[2])) != NULL)
            {
               ret = ptr - argv[1];
               ret += 1;
               ptr2 = ptr + 1;
            }
         }
      }
   }
   if (retc < 1)
      Winfoprintf( "strstr match character is %d",ret );
   else
   {
      retv[0] = realString( (double) ret );
      if (retc > 1)
      {
         char tmp[MAXSTR*4];
         if ( (ret == 0) || (strcmp(argv[1],argv[2]) == 0) )
         {
            retv[1] = newString("");
            if (retc > 2)
               retv[2] = newString("");
         }
         else
         {
            strcpy(tmp,argv[1]);
            tmp[ret-1] = '\0';
            retv[1] = newString(tmp);
            if (retc > 2)
               retv[2] = newString(&argv[1][ret+len-1]);
         }
      }
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|       input
|
|       input procedure allows input from the keyboard to assign
|       one or more values to variables. The procedure allows an
|       input prompt to be printed on the command line. 
|       The user passes the number of requested variables to be
|       inputed and the system tries to satisfy all the requests.
|       The default delimiter between parameters is the comma. However,
|       any delemeter of you choosing can be chosen in second parameter. 
|       Ex.
|        Usage  input('enter name,x,y'):stringvar,realvar,realvar2
|       
|       Ex input on terminal
|       enter name x,y hello out there,23,34
|
|       Ex.
|        Usage  input('enter name,x,y',' '):stringvar,realvar,realvar2
|       
|       Ex input on terminal
|       enter name x,y hello 34 35
|
|
+-----------------------------------------------------------------------------*/

int Rinput(int argc, char *argv[], int retc, char *retv[])
{   char *delimiter;   
    char *prompt;   
    char *ptr;   
    char *start;   
    char  tempbuf[256];
    int   i;

/*  Wturnoff_buttons();	*/
    TPRINT2("input:  argc = %d  retc = %d\n",argc,retc);
    switch (argc)
    { case 3:   delimiter = argv[2];
                prompt = argv[1];
                break;
      case 2:   delimiter = ",";        
                prompt = argv[1];
                break;
      default:  delimiter = ",";
                prompt = NULL;
                break;
    }

#ifdef VNMRJ
    /* make sure Java graphics is updated */
    popBackingStore();
#endif
    i = 0;
    if (retc > 0) /* break up line into parameters */
    {   if (W_getInputCR(prompt,tempbuf,sizeof(tempbuf)))
        {   start = ptr = tempbuf;
            while (*ptr && i<retc)
            {   if (*ptr == *delimiter || *ptr == '\n')
                {   *ptr++ = '\0';
                    retv[i] = newString(start);
                    start = ptr;
                    i++;
                }
                else
                    ptr++;
            }
        }               
        else if (interuption)
        {
          ABORT;
        }
        else
            Werrprintf("input: End of file or read error?");
    }
    else
    {   Werrprintf("input: Return variable needed!");
        ABORT;
    }
  RETURN;
}

/*------------------------------------------------------------------------------
|
|       real
|
|       Creates a real variable if it doesn't exist 
|
+-----------------------------------------------------------------------------*/

int real(int argc, char *argv[], int retc, char *retv[])
{   int flag,i;

    (void) retc;
    (void) retv;
    flag = 1;
    if (2 <= argc)
    {   for(i=1;i<argc;i++)
        {   varInfo     *v;
         
            TPRINT2("real:  argv[%d] is \"%s\"\n",i,argv[i]);
            if(strlen(argv[i]))
            {   if ((v = findVar(argv[i])) == NULL)
                {   if ( (v = createVar(argv[i])) )
                    {
                        v->T.basicType = T_REAL;
                        resetMagicVar(v,argv[i]);
                    }
                    else
                        flag = 0;
                }
                else
                {   if (v->T.basicType != T_REAL)
                    {
                        if(v->T.basicType ==  T_UNDEF)
                            v->T.basicType = T_REAL;
                        else
                        {    Werrprintf("real: Type clash variable \"%s\"!",argv[i]);
                            flag = 0;   
                        }
                    }
                 }
            }    
        }
        return(!flag);
    }
    else
    {   Werrprintf("real: wrong number of arguments!");
        ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|       string
|
|       Creates a string variable if it doesn't exist 
|
+-----------------------------------------------------------------------------*/

int string(int argc, char *argv[], int retc, char *retv[])
{   int flag,i;

    (void) retc;
    (void) retv;
    flag = 1;
    if (2 <= argc)
    {   for(i=1;i<argc;i++)
        {   varInfo     *v;
         
            if ((v = findVar(argv[i])) == NULL)
            {   if ( (v = createVar(argv[i])) )
                {
                    v->T.basicType = T_STRING;
                    resetMagicVar(v,argv[i]);
                }
                else
                    flag = 0;
            }
            else
            {   if (v->T.basicType != T_STRING)
                {
                    if(v->T.basicType ==  T_UNDEF)
                        v->T.basicType = T_STRING;
                    else
                    {   Werrprintf("string: Type clash variable \"%s\"!",argv[i]);
                        flag = 0;       
                    }
                }
            }
        }
        return(!flag);
    }
    else
    {   Werrprintf("string: wrong number of arguments!");
        ABORT;
    }
}

/********/
void vnmr_tolower(char *s)
/********/
/* convert all characters in string to lower case */
{
   int             i;

   i = 0;
   while (s[i])
   {
      if ((s[i] >= 'A') && (s[i] <= 'Z'))
	 s[i] = s[i] + 'a' - 'A';
      i++;
   }
}

/********/
static void vnmrToUpper(char *s)
/********/
/* convert all characters in string to upper case */
{
   int             i;

   i = 0;
   while (s[i])
   {
      if ((s[i] >= 'a') && (s[i] <= 'z'))
	 s[i] = s[i] + 'A' - 'a';
      i++;
   }
}

/*------------------------------------------------------------------------------
|
|       format
|
|       Formats a real number into length n, precision m 
|       Usage  format(real,n,m):string
|
+-----------------------------------------------------------------------------*/

int format(int argc, char *argv[], int retc, char *retv[])
{
    if (argc == 4)
    {   char forstr[32];
        char retstr[66];
        int  length,precision;
	int width;
	double value;
	char *tptr;

        TPRINT3("format: format(%s,%s,%s)...\n",argv[1],argv[2],argv[3]);
        if (isReal(argv[1]))
        {
	    value = stringReal( argv[ 1 ] );
            TPRINT1("format: ...input = %f \n",value);

	/* Establish number of digits to the left of the decimal point
	   by formatting the value using %-60.0f.  Than add the requested
	   precision and verify the resulting number of characters.	*/

	    sprintf(retstr,"%-60.0f",value);
	    tptr = strchr( retstr, ' ' );		/* eliminate blank */
	    if (tptr != NULL)				/* characters to the */
	      *tptr = '\0';				/* right of the digits */
	    length = strlen( retstr );
	    precision = atoi( argv[ 3 ] );
	    if (length + precision > 63) {
		Werrprintf( "%s:  result would have too many characters", argv[ 0 ] );
		ABORT;
	    }
	    width = atoi(argv[2]);
	    if (*argv[2] == '0' && width > 1){
		/* Pad field with leading zeros */
		sprintf(forstr,"%%%s.%df",argv[2],precision);
	    }else{
		sprintf(forstr,"%%%d.%df", width, precision);
	    }
            TPRINT1("format: ...format= \"%s\"\n",forstr);
            sprintf(retstr,forstr,stringReal(argv[1]));
            TPRINT1("format: ...result= \"%s\"\n",retstr);
            if (1 <= retc)
                retv[0] = newString(retstr);
            else
            {   Winfoprintf("%s reformatted as %s",argv[1],retstr);
                clearRets(retc,retv);
            }
        }
        else
        {   Werrprintf("The first argument must be a real number for this mode of %s",argv[ 0 ]);
            clearRets(retc,retv);
            ABORT;
        }
    }
    else if (argc == 3)
    {
       char retstr[256];

       if (strcmp(argv[2],"lower") == 0)
       {
          strcpy(retstr,argv[1]);
          vnmr_tolower(retstr);
       }
       else if (strcmp(argv[2],"upper") == 0)
       {
          strcpy(retstr,argv[1]);
          vnmrToUpper(retstr);
       }
       else if (strcmp(argv[2],"isreal") == 0)
       {
          int res = isReal(argv[1]);
          if (1 <= retc)
             retv[0] = intString(res);
          else
          {  Winfoprintf("%s %s a real number",argv[1],(res == 1) ? "is" : "is not");
             clearRets(retc,retv);
          }
          RETURN;
       }
       else if (strcmp(argv[2],"expand") == 0)
       {
          int inIndex=0;
          int outIndex=0;
          int count=0;
          int ch;

          while ( (ch = argv[1][inIndex]) != '\0' )
          {
             if (ch == '\t')
             {
                count = 8;
                count -= (outIndex+8) % 8;
                for (; count > 0; count--)
                   retstr[outIndex++] = ' ';
                inIndex++;
             }
             else
             {
                retstr[outIndex++] = argv[1][inIndex++];
             }
          }
          retstr[outIndex]='\0';
       }
       else
       {  Werrprintf("The second argument must be 'upper', 'lower' or 'expand' for this mode of %s",argv[ 0 ]);
          clearRets(retc,retv);
          ABORT;
       }
       if (1 <= retc)
          retv[0] = newString(retstr);
       else
       {  Winfoprintf("%s reformatted as %s",argv[1],retstr);
          clearRets(retc,retv);
       }
    }
    else
    {   Werrprintf("%s: wrong number of arguments!",argv[ 0 ]);
        clearRets(retc,retv);
        ABORT;
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|       getRandom
|
|       generates a random number
|       Usage  format(max,'real'):val
|
+-----------------------------------------------------------------------------*/

#define RMAX 2147483647

int getRandom(int argc, char *argv[], int retc, char *retv[])
{
   static int seeded = 0;
   int retReal = 0;
   int index;
   double max = 0.0;
   int useMax = 0;

   if (!seeded)
   {
      srandom( HostPid );
      seeded = 1;
   }
   index = 1;
   while (index < argc)
   {
      if ( ! strcmp("real",argv[index]) )
         retReal = 1;
      else if (isReal(argv[index]) )
      {
         max = strtod(argv[index],NULL);
         useMax = 1;
      }
      index++;
   }
   if (retReal)
   {
      double val = (double) random();
      if (useMax)
      {
         val /= (double) RMAX;
         val *= max;
      }
      if (retc)
         retv[0] = realString(val);
      else
         Winfoprintf("random number: %g", val);
   }
   else
   {
      long val = random();
      if (useMax)
      {
         long range;
         int imax;
         int negMax = 0;

         if (max < 0)
         {
            max = -max;
            negMax = 1;
         }
         imax = (int) (max + 0.1);
         range = RMAX / (imax + 1);  /* determine size of max + 1 ranges of integers */
         val /= range;
         if (val > imax)
            val = imax;
         if (negMax)
            val = -val;
      }
      if (retc)
         retv[0] = intString(val);
      else
         Winfoprintf("random integer: %ld", val);
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|       groupcopy
|
|       This routine is used to copy a set of variable of a group from
|       one tree to another.
|       Default tree is current. Default type is real.
|       Usage -- groupcopy(fromtree,totree,group)
|                trees can be  current,global,processed
|                group can be all,sample,acquisition,processing,display
|
+-----------------------------------------------------------------------------*/

int groupcopy(int argc, char *argv[], int retc, char *retv[])
{   int    group;
    symbol **fromroot;
    symbol **toroot;

    (void) retc;
    (void) retv;
    if (argc == 4)
    {   if ( (fromroot = getTreeRoot(argv[1])) )
        {   if ( (toroot = getTreeRoot(argv[2])) )
            {   if (fromroot != toroot)
                {   if ( 0 <= ( group = goodGroup(argv[3])))
                    {
#ifdef VNMRJ
			char to_tree[MAXPATH+8];

			strncpy(to_tree,argv[2],MAXPATH);
			vnmr_tolower(to_tree);
			if (strcmp(to_tree,"current")==0)
			  jsetcopyGVars( 0 );
#endif 
			copyGVars(fromroot,toroot,group);
                        P_balance(toroot);
#ifdef VNMRJ
			if (strcmp(to_tree,"current")==0)
			  jsetcopyGVars( -1 );
#endif 
                        TPRINT3("Copied from \"%s\" to \"%s\" group \"%s\"!"
                          ,argv[1],argv[2],argv[3]);
                        RETURN;
                    }
                    else
                    {   Werrprintf("groupcopy: \"%s\" not valid group!",
                                argv[3]);
                        ABORT;
                    }
                }
                else
                {   Werrprintf("groupcopy: can not copy to same tree!");
                    ABORT;
                }
            }
            else
            {   Werrprintf("groupcopy: \"%s\" is not valid tree!",argv[2]);
                ABORT;
            }
        }
        else
        {   Werrprintf("groupcopy: \"%s\" is not valid tree!",argv[1]);
            ABORT;
        }
    }
    else if (argc == 3)
    {
       int fromTree = getTreeIndex(argv[1]);
       int toTree = getTreeIndex(argv[2]);
       if (fromTree < 0)
       {
          Werrprintf("groupcopy: \"%s\" is not valid tree!",argv[1]);
          ABORT;
       }
       if (toTree < 0)
       {
          Werrprintf("groupcopy: \"%s\" is not valid tree!",argv[2]);
          ABORT;
       }
       P_pruneTree(toTree,fromTree);
       P_copy(fromTree,toTree);
       RETURN;
    }
    else
    {   Werrprintf("Usage -- groupcopy(fromtree,totree<,group>)!");
        ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|       paramcopy
|
|       This routine is used to copy a variable to another variable.
|       The variables can be in the same or different trees.
|       Usage -- paramcopy(fromVar,toVar, fromTree, toTree)
|                trees can be  current,global,processed,usertree
|       Default tree is current.
|
+-----------------------------------------------------------------------------*/

int paramcopy(int argc, char *argv[], int retc, char *retv[])
{
   int fromTree = CURRENT;
   int toTree = CURRENT;

   if (argc >= 3)
   {
      if (argc >= 4)
      {
         if ( (fromTree = getTreeIndex(argv[3])) == -1 )
         {
            Werrprintf("paramcopy: \"%s\" is not valid tree!",argv[3]);
            ABORT;
         }
         if (argc == 5)
         {
            if ( (toTree = getTreeIndex(argv[4])) == -1 )
            {
               Werrprintf("paramcopy: \"%s\" is not valid tree!",argv[4]);
               ABORT;
            }
         }
      }
      if (P_copyvar(fromTree,toTree,argv[1],argv[2]) < 0)
      {
         Werrprintf("paramcopy: parameter \"%s\" does not exist", argv[1]);
         ABORT;
      }
   }
   else
   {   Werrprintf("Usage -- paramcopy(fromVar,toVar<,fromTree<,toTree>>)");
       ABORT;
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|       create
|
|       This routine is used to create a type variable on a tree. 
|       Default tree is current. Default type is real.
|       Usage -- create(name[,type[,tree]])
|                type can be  real,string,delay,frequency,flag,pulse,integer
|                tree can be  current,global,processed
|
+-----------------------------------------------------------------------------*/

int create(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    char    *type;
    int      typeindex;
    symbol **root;
    int      doInit = 0;
    char    *initStr = "";

#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 

    switch (argc)
    { case 2:   tree = "current";
                type = "real";
                break;
      case 3:   
                type = argv[2];
                tree = "current";
                break;
      case 4:   
      case 5:   
      case 7:   
                type = argv[2];
                tree = argv[3];
                break;
      default:  Werrprintf("Usage -- create(name[,type[,tree]]!");
                ABORT;
    }
    if ((typeindex = goodType(type)) == 0)
    {   Werrprintf("create:  \"%s\" bad type!",type);
        ABORT;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {   Werrprintf("create:  \"%s\"  bad tree name!",tree);
        ABORT;
    }
    if ( (argc == 5)  || ( (argc == 7) && !strcmp(argv[5],"1") ) )
    {
       int ret;

       ret = P_getsubtype(getTreeIndex(tree),argv[1]);
       /* parameter exists and is correct type. Just return */
       if ( (ret > 0) && (ret == typeindex) )
       {
          if (retc)
             retv[0] = intString(0);
          RETURN;
       }
       /* parameter exists and is wrong type. Delete it */
       if ( (ret >= 0) && (ret != typeindex) )
       {
          ret = P_deleteVar(getTreeIndex(tree),argv[1]);
       }
       doInit = 1;
       initStr = argv[4];
    }
    if(goodName(argv[1]))
    {   if(RcreateVar(argv[1],root,typeindex))
        {
            TPRINT3("created variable \"%s\" type \"%s\" in \"%s\" tree.",
               argv[1],type,tree);
            resetMagicVar(P_getVarInfoAddr(getTreeIndex(tree),argv[1]),argv[1]);
#ifdef VNMRJ
            if ((argc<7) && ((strcmp(tree,"global")==0) || (strcmp(tree,"systemglobal")==0)))
            {
               int index;
               int num;
               double dval;
               char msg[MAXSTR];

               if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
               {
                  num = (int) (dval+0.1);
                  for (index=1; index <= num; index++)
                  {
                     if (index != VnmrJViewId)
                     {
                        sprintf(msg,"VP %d %s('%s','%s','%s','%s',%d,'vp')\n",
                              index, argv[0], argv[1],type,tree,initStr,doInit);
                        writelineToVnmrJ("vnmrjcmd",msg);
                     }
                  }
               }
            }
            else
#endif 
              if (argc < 7)
                appendvarlist(argv[1]);
            if (doInit)
            {
               if ( !strcmp(type,"string") || !strcmp(type,"flag") )
               {
                  P_setstring(getTreeIndex(tree),argv[1],initStr,1);
               }
               else
               {
                  P_setreal(getTreeIndex(tree),argv[1],stringReal(initStr),0);
               }
            }
            if (retc)
               retv[0] = intString(1);
            RETURN;
        }
        else
        {
           if ( (argc < 7) && (retc == 0) )
           {
              Werrprintf("create: \"%s\" already in tree!",argv[1]);
              ABORT;
           }
           if (retc)
              retv[0] = intString(0);
           RETURN;
        }
    }
    else
    {   Werrprintf("create: \"%s\" not valid variable name!",argv[1]);
        ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|       prune
|
|       This routine is used to destroy variables in the current tree. 
|       which are not present in the supplied parameter file.
|       Usage -- prune(filename)
|
+-----------------------------------------------------------------------------*/

int prune(int argc, char *argv[], int retc, char *retv[])
{
    int      ret,tree;
    char     parfilename[ MAXPATHL ];

    (void) retc;
    (void) retv;
#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 

    if (argc != 2)
    {
      Werrprintf("Usage -- prune(filename)");
      ABORT;
    }
    tree = getTreeIndex("temporary");
    P_treereset(tree);		/* clear the tree first */

/*  If command parameter is a Directory; or it does not exist in the file system...  */

    if (isDirectory( argv[ 1 ] ) || access( argv[ 1 ], R_OK ) != 0) {
	if (find_param_file( argv[ 1 ], &parfilename[ 0 ] ) != 0) {
	    Werrprintf( "%s:  cannot access parameters from %s", argv[ 0 ], argv[ 1 ] );
	    ABORT;
	}
    }
    else
      strcpy( &parfilename[ 0 ], argv[ 1 ] );

    if (P_read(tree, &parfilename[ 0 ] ))
    {
      Werrprintf("problem loading parameters from %s", &parfilename[ 0 ]);
      ABORT;
    }
    ret = P_pruneTree(getTreeIndex("current"),tree);
    P_treereset(tree);		/* clear the tree first */
    if (ret == 0)
    {
        TPRINT0("variables in current tree are pruned");
        RETURN;
    }
    if (ret == -1)
    {   Werrprintf("tree does not exists");
        ABORT;
    }   
    RETURN;
}

static int sgetword(char *str, char word[], int limit, int *index)
/**********************************/
{
  int ch,i;

  word[0] = '\0';
  while ( ((ch = *(str + *index)) != '\0') && (ch == ' '))
  {
    /* skip white space */
    *index += 1;
  }
  if (ch == '\0')
    return(0);
  i = 0;
  while ((i < limit -1) && (ch != ' ') && (ch != '\0'))
  {
    word[i++] = ch;
    *index += 1;
    ch = *(str + *index);
  }
  word[i] = '\0';
  return(1);
}

/*------------------------------------------------------------------------------
|
|       destroy
|
|       This routine is used to destroy variable on a tree. 
|       Default tree is current.
|       Usage -- destroy(name[,tree, [index]])
|                tree can be  current,global,processed
|
+-----------------------------------------------------------------------------*/

int destroy(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    int      ret;
    int vars = 0;
    int delindex = 0;
    char parName[MAXSTR];
    int itree;
    int index = 0;
    int count = 0;
    int curTree;

#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 

    switch (argc)
    { case 2:   tree = "current";
                break;
      case 3:
                tree = argv[2];
                break;
      case 4:
      case 5:
                tree = argv[2];
                if (strcmp(argv[3],"vp"))
                {
                   delindex = atoi(argv[3]);
                   if (delindex < 1)
                      delindex = 1;
                }

                break;
      default:  Werrprintf("Usage -- destroy(name[,tree [,index]])");
                ABORT;
    }

    ret = itree = getTreeIndex(tree);
    curTree = (strcmp(tree,"current") == 0);
    if (ret != -1)
    {
          ret = -2;
          while (sgetword(argv[1],parName,MAXSTR, &index) )
          {
             if ( curTree && ! delindex )
             {
                checkarray(parName,0);
             }
             if (delindex)
             {
                int num;
                num = P_getsize(itree, parName, NULL);
                if (num == 0)
                   ret = -2;
                else if (delindex > num)
                {
                   ret = -3;
                   if ( ! retc)
                      Werrprintf("%s: %s index %d does not exists",
                           argv[0], parName, delindex);
                }
                else if (num == 1)
                   ret = P_deleteVar(itree,parName);
                else
                   ret = P_deleteVarIndex(itree,parName,delindex);
             }
             else
             {
                ret = P_deleteVar(itree,parName);
             }
             if ( curTree && !ret)
             {
                appendvarlist(parName);
             }
             vars++;
             if ( ! ret)
                count++;
          }
          if (count)
             ret = 0;
    }
    if (retc>0)
      retv[0] = realString((ret == 0) ? 1.0 : 0.0);
    if (ret == 0)
    {
        TPRINT2("destroyed variable '%s' in '%s' tree", argv[1],tree);
#ifdef VNMRJ
        if ( strcmp(argv[argc-1],"vp") && ((strcmp(tree,"global")==0) || (strcmp(tree,"systemglobal")==0)) )
        {
           int index;
           int num;
           double dval;
           char msg[MAXSTR];

           if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
           {
              num = (int) (dval+0.1);
              for (index=1; index <= num; index++)
              {
                 if (index != VnmrJViewId)
                 {
                    if (argc == 3)
                       sprintf(msg,"VP %d %s('%s','%s','vp')\n",index,argv[0],argv[1],tree);
                    else if (argc == 4)
                       sprintf(msg,"VP %d %s('%s','%s','%s','vp')\n",index,argv[0],argv[1],tree,argv[3]);
                    writelineToVnmrJ("vnmrjcmd",msg);
                 }
              }
           }
           appendvarlist(argv[1]);
        }
#endif 
        RETURN;
    }
    if (ret == -2)
    {
       if (argc != 4)
       {
          if (retc)
             RETURN;
          else
          {
             if (vars)
                Werrprintf("Variables '%s' do not exist in '%s' tree",
                         argv[1],tree);
             else
                Werrprintf("Variable '%s' does not exist in '%s' tree",
                         argv[1],tree);
             ABORT;
          }
       }
    }
    if (ret == -1)
    {
        if (retc)
           RETURN;
        else
        {
           Werrprintf("'%s' tree does not exists",tree);
           ABORT;
        }
    }   
    RETURN;
}

/*------------------------------------------------------------------------------
|
|       destroygroup
|
|       This routine is used to destroy variable of a group in a tree. 
|       Default tree is current.
|       Usage -- destroygroup(group[,tree])
|                tree can be  current,global,processed
|                group can be all,sample,acquisition,processing,display,
|                and spin.
|
+-----------------------------------------------------------------------------*/

int destroygroup(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    int      ret;

    (void) retc;
    (void) retv;
#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 

    switch (argc)
    { case 2:   tree = "current";
                break;
      case 3:   tree = argv[2];
                break;
      default:  Werrprintf("Usage -- destroygroup(group[,tree])");
                ABORT;
    }
    ret = P_deleteGroupVar(getTreeIndex(tree),goodGroup(argv[1]));
    if (ret == 0)
    {
      TPRINT2("destroyed group '%s' in '%s' tree",argv[1],tree);
      RETURN;
    }
    if (ret == -17)
    {   Werrprintf("Group '%s' does not exist in '%s' tree",argv[1],tree);
        ABORT;
    }
    if (ret == -1)
    {   Werrprintf("'%s' tree does not exists",tree);
        ABORT;
    }   
    RETURN;
}

/*------------------------------------------------------------------------------
|
|       display
|
|       This routine displays one or more variables and their parameters 
|       from a tree (either current, global, or processed).
|       The default tree is current.  
|       Usage -- display(name|*[,current|,global|,processed])
|
+-----------------------------------------------------------------------------*/

int display(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;

    (void) retc;
    (void) retv;
    Wturnoff_buttons();
    Wshow_text();
    if( 2 <= argc && argc < 4)
    {   if (argc == 3)
        {
           tree = argv[2];
           root = getTreeRoot(argv[2]);
        }
        else
        {
           tree = "current";
           root = getTreeRoot("current");
        }
        if (root)
        {   if ( ! strcmp(argv[1],"*"))/*show all variables in a tree */
            {   Wscrprintf("Showing all variables for \"%s\" tree\n",tree);
                RshowVals(stdout,*root);
            }
            else
                if ( ! strcmp(argv[1],"**") )/* show everything in a tree */
                {   Wscrprintf("Showing variables for \"%s\" tree\n",tree);
                    RshowVars(stdout,*root);
                }
            else
            {   varInfo *v;
                if ( (v = rfindVar(argv[1],root)) ) /* if variable exists */
                {   RshowVar(stdout,v);
                }
                else
                {   Werrprintf("Variable \"%s\" doesn't exist in \"%s\" tree!",
				argv[1],tree); 
                    RETURN;
                }
            }
            Wsettextdisplay("display");
            RETURN;
        }
        else
        {   Werrprintf("Tree doesn't exist!");
            ABORT;
        }
    }
    else
    {   Werrprintf("Usage -- display(name|*[,current|,global|,processed])!");
        ABORT;
    }
}

/*------------------------------------------------------------------------------
|    jsendArrayMenu - send arrayable parameters to vnmrj popup                 |
+-----------------------------------------------------------------------------*/
void jsendArrayMenu()
{
	symbol **root;
	char tree[9] = "current";

	outjCtInit();
	root = getTreeRoot( tree );
	RcountJVals(*root);

	if (outjInit() == 0)
	{
	  root = getTreeRoot( tree );
	  RshowJVals(*root);
	  outjShow();
	}
}



/*----------------------------------------------------------------------------
|
|       mstat
|
|       mstat describes the memory statistics of allocated packets
|       that uses or used allocateWithId.
|       mstat without an argument will print all ids.
+----------------------------------------------------------------------------*/

static void do_mstat(char *id)
{   const char *nameid;
    char *pnt;
    int   i;
    PACK *rootpt = NULL;
    PACK *p,*q;
 
    if (id == NULL)
    {   pnt = NULL;
        while ( scanFor(NULL,&pnt,&i,&nameid))
            if (isNotEntered(nameid,rootpt))
                enter(nameid,&rootpt);
        p = rootpt;
        while (p)
        {   Wscrprintf("%-16s %8d blocks %8d chars\n",p->id,
                blocksAllocatedTo(p->id),charsAllocatedTo(p->id));
            p = p->next;
        }
        Wscrprintf("\nA total of %d blocks %d chars\n\n",blocksAllocated(0),charsAllocated(0));
        p = rootpt;
        while (p)
        {
            q = p->next;
            release((char *) p);
            p = q;
        }
    }
    else
    {   Wscrprintf("%s  %d blocks %d chars\n",id,
                blocksAllocatedTo(id),charsAllocatedTo(id));
    }
}

int mstat(int argc, char *argv[], int retc, char *retv[])
{
   (void) retc;
   (void) retv;
   Wturnoff_buttons();
   Wshow_text();
   if (argc == 1)
        do_mstat(NULL);
    else
        do_mstat(argv[1]);
    RETURN;
}

static int isNotEntered(const char *nameid, PACK *rootpt)
{   while (rootpt)
        if (strcmp(rootpt->id,nameid) == 0)
            return(0);
        else
            rootpt = rootpt->next;
    return(1);
}

static void enter(const char *id, PACK **rootpt)
{   PACK *p;

    p       = (PACK *)allocateWithId(sizeof(PACK),"mstat");
    p->id   = id;
    p->next = *rootpt;
    *rootpt = p;
}


int mvDir( char *indir, char *outdir )
{
   DIR	*dirp;
   struct dirent *dp;
   int ret = 0;
 
   if ( (dirp = opendir(indir)) )
   {
      char inname[MAXPATH*2];
      char outname[MAXPATH*2];

      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
      {
         sprintf(inname,"%s/%s",indir,dp->d_name);
         sprintf(outname,"%s/%s",outdir,dp->d_name);
         if (rename(inname,outname))
	 {
	    if (errno != EBUSY)
		 ret = -1;
	 }
      }   
      closedir(dirp);
   }
   else
   {
      return(-1);
   }
   return(ret);
}

/*
 *  Removes all files in the named directory that end with the suffix
 *  Returns 0 for success, -1 for failure.
 */
int unlinkFilesWithSuffix( char *dirpath, char *suffix )
{
   DIR	*dirp;
   struct dirent *dp;
 
   if ( (dirp = opendir(dirpath)) )
   {
      char newname[MAXPATH*2];
      int len;

      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
      {
         sprintf(newname,"%s/%s",dirpath,dp->d_name);
         len = strlen(newname) - strlen(suffix);
         if ((len> 0) && ! strcmp(&newname[len],suffix) )
         {
            unlink(newname);
         }
      }   
      closedir(dirp);
   }
   else
   {
      return(-1);
   }
   return(0);
}

/*------------------------------------------------------------------------
|	isDirectory
|	
|	This routine determines if name is a directory
+-----------------------------------------------------------------------*/

static int isDirectory(const char *dirname)
{
#ifdef VMS

/*  The VMS version is complex because several different formats
    for expressing the name of a directory must be supported.
    These are best illustrated by example:

    Assume that the following directory exists on VMS:
	[ROBERT.VNMR.EXP1]
    and the current working directory is:
	[ROBERT.VNMR]
    Then the following arguments to isDirectory should return a
    non-zero result:
	[ROBERT.VNMR.EXP1]
	[ROBERT.VNMR]EXP1.DIR
	[ROBERT.VNMR]EXP1
	EXP1.DIR
	EXP1							*/

#define  NAM$C_MAXRSS	256			/* From RMS */
    char	workspace[ NAM$C_MAXRSS+2 ];	/* Room for NUL char */
    char	*bptr, *dptr, *sptr;
    int 	dlen, ival;
    extern char *rindex();
    struct stat buf;

    if (strlen( dirname ) > NAM$C_MAXRSS) return( 0 );
    strcpy( &workspace[ 0 ], dirname );
    sptr = rindex( &workspace[ 0 ], ';' );	/* Pointer to semi-colon */
    dptr = rindex( &workspace[ 0 ], '.' );	/* Pointer to dot */
    bptr = rindex( &workspace[ 0 ], ']' );	/* Pointer to right-bracket */

    if (sptr != NULL) *sptr = '\0';		/* No version numbers, please */
    dlen = strlen( &workspace[ 0 ] );

/*  First case:  no dot and no right bracket.  Append ".dir"
    to the file name.						*/

    if (dptr == NULL && bptr == NULL)
      strcat( &workspace[ 0 ], ".dir" );

/*  Check for three cases if a right-bracket is present:
    1.  If the right bracket is the last character, use
	GET_PARENTDIR() to obtain the actual name of the
	parent directory.

    2.  If no dot, or the dot is part of the directory tree
	and thus its address is less than that of the bracket,
	append ".dir" to the file name.

    3.  Otherwise, use the name "as is".			*/

    else if (bptr != NULL) {
	if (bptr - &workspace[ 0 ] == dlen-1)
	  get_parentdir( &workspace[ 0 ], &workspace[ 0 ], NAM$C_MAXRSS );
	else if (dptr == NULL || ( dptr != NULL && dptr < bptr ))
	  strcat( &workspace[ 0 ], ".dir" );
    }

/*  If no right bracket present, but there is a dot, use as is.  */

    ival = stat( &workspace[ 0 ], &buf );	/* Returns 0 if successful */
    if (ival) return( 0 );			/* No directory if error */
    else      return(buf.st_mode & S_IFDIR);
#else 
    struct stat buf;				/* UNIX version much simpler */

    /* The buf.st_mode field is not set if stat() fails */
    buf.st_mode = 0;
    stat(dirname, &buf);
    return(buf.st_mode & S_IFDIR);
#endif 
}

/*------------------------------------------------------------------------
|
|	This module counts the number of files
|	in the selected directory
|       Files beginning with a "." are ignored.
|
+-----------------------------------------------------------------------*/
extern int getitem(char *dirname, int index, char *filename, char *fileCmp, int recurse,
                   int regExp, int saveInPar, varInfo *v1);
extern int getitem_sorted(char *dirname, int index, char *filename, char *fileCmp, int recurse,
                   int regExp, int saveInPar, varInfo *v1);

/*------------------------------------------------------------------------
|
|       getfile
|
|       This routine returns either the number of files in the specified
|       directory or the name of the n th file in the directory.
|       Files beginning with a "." are ignored.
|       Usage -- getfile(directory<,item>)
|
+-----------------------------------------------------------------------*/

int getfile(int argc, char *argv[], int retc, char *retv[])
{
  int  index;
  char filename[MAXPATH];
  char fileCmp[MAXPATH];
  char dirname[MAXPATH];
  int  i;
  int res;
  int recurse = 0;
  int regExp = 0;
  int saveInPar = 0;
  varInfo *v1 = NULL;

  if ( (argc == 1) || ! isDirectory(argv[1]) )
  {
     if (argc > 1)
     {
       if (retc>0)
          retv[0] = intString(0);
       else
          Werrprintf("getfile: %s is not a directory", argv[1]);
       RETURN;
     }
     Werrprintf("Usage -- getfile(directory <,index[,'alphasort'][,match]>)");
     ABORT;
  }
  res = 2;
  index = 0;
  if ( (argc > 2) && isReal(argv[2]))
  {
     index = (int) stringReal(argv[2]);
     if (index <= 0)
     {
        Werrprintf("%s: file index %d does not exist",argv[0],index);
        ABORT;
     }
     res = 3;
  }
  else if ( (argc > 2) && !strcmp(argv[2],"RETURNPAR") )
  {
     symbol **root;

     if ( (argc < 3) || (argv[3][0] != '$') )
     {
        Werrprintf("%s: 'RETURNPAR' option must be following by local parameter name",argv[0]);
        ABORT;
     }
     if ((root=selectVarTree(argv[3])) == NULL)
     {
        Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[3]);
        ABORT;
     }
     if ((v1 = rfindVar(argv[3],root)) == NULL)
     {   Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[3]);
         ABORT;
     }
     if (v1->T.basicType == T_REAL)
     {
        Werrprintf("%s: variable \"%s\" must be string type",argv[0],argv[3]);
        ABORT;
     }
     saveInPar = 1;
     res = 4;
  }
  int alphaSort = 0;
  fileCmp[0] = '\0';
  filename[0] = '\0';
  for (i=res; i<argc; i++)
  {
     if (! strcmp(argv[i],"alphasort") )
        alphaSort = 1;
     else if (! strcmp(argv[i],"-R") )
        recurse = 1;
     else if (! strcmp(argv[i],"regex") )
        regExp = 1;
     else if (! strcmp(argv[i],"regexdot") )
        regExp = 2;
     else
        strcpy(fileCmp,argv[i]);
  }
  // alphasort not needed if just counting items
  if (!index && !saveInPar)
     alphaSort = 0;
  if (regExp && (fileCmp[0] == '\0') )
  {
     regExp = 0;
  }
  strcpy(dirname,argv[1]);
  if ( dirname[strlen(dirname) -1] != '/')
     strcat(dirname,"/");
  if ( ! alphaSort)
  {
     if ( (res = getitem(dirname,index,filename,fileCmp,recurse,regExp, saveInPar, v1)) < 0)
        ABORT;
  }
  else if ( (res = getitem_sorted(dirname,index,filename,fileCmp, recurse, regExp, saveInPar, v1)) < 0 )
     ABORT;
  if (index)       /* a specific file name is wanted */
  {
    char extension[MAXPATH];
    int  temp,len;
    extension[0] = '\0';
    temp = len = strlen(filename);
    while ((len) && (filename[len-1] != '.'))
      len--;
    if (len)
    {
      filename[len-1] = '\0';
      for (i = 0; i < temp - len; i++)
        extension[i] = filename[len + i];
      extension[temp - len] = '\0';
    }
    if (retc>0)
    {
      retv[0] = newString(filename);
      if (retc>1)
        retv[1] = newString(extension);
    }
    else
      Winfoprintf("file %d in %s is named %s with extension %s",
                   index,argv[1],filename,extension);
  }
  else  /* the number of files is wanted  */
  {
    if (retc>0)
      retv[0] = intString(res);
    else
      Winfoprintf("%s has %d files",argv[1],res);
  }
  RETURN;
}


/*  Unit conversion */

struct _unitType {
   char id[16];
   char label[16];
   int useSlopePar;
   int useInterPar;
   double slopeVal;
   double interVal;
   char *slopePar;
   int  slopeTree;
   int  slopeMult;
   char *interPar;
   int  interTree;
   int  interAdd;
   struct _unitType *next;
};

typedef struct _unitType unitType;
static unitType *startP = NULL;

static unitType *getUnit(char *name, unitType **start)
{
   unitType *ptr;
   ptr = *start;
   while (ptr)
   {
      if (strcmp(ptr->id,name) == 0)
         return(ptr);
      ptr = ptr->next;
   }
   return(NULL);
}

int getUserUnit(char *unit, char *label, double *slope, double *intercept)
{
   unitType *ptr;
   double tmp;

   if ( (ptr = getUnit(unit,&startP)) )
   {
      if (ptr->useSlopePar)
      {
         if (P_getreal(ptr->slopeTree,ptr->slopePar,&tmp,1) == 0)
         {
            if (tmp == 0.0)
               return(0);
            else
               *slope = (ptr->slopeMult) ? tmp : 1.0/tmp;
         }
      }
      else
         *slope = ptr->slopeVal;
      if (ptr->useInterPar)
      {
         if (P_getreal(ptr->interTree,ptr->interPar,&tmp,1) == 0)
         {
            *intercept = (ptr->interAdd) ? tmp : -tmp;
         }
      }
      else
         *intercept = ptr->interVal;
      if (label != NULL)
         strcpy(label,ptr->label);
      return(1);
   }
   else
      return(0);
}

/* Linear conversion of input based on unit selection */
double unitConvert(char *unit, double startVal)
{
   double slope = 1.0;
   double intercept = 0.0;

   if (!getUserUnit(unit, (char *)NULL, &slope, &intercept))
      Werrprintf("unknown unit %s",unit);
   return( startVal*slope + intercept );
}

static void showUnit(unitType **start)
{
   unitType *ptr;

   ptr = *start;
   while (ptr)
   {
      if (strcmp(ptr->id,""))
      {
         Wscrprintf("unit %s with label '%s'\n",
                     ptr->id, ptr->label);
         if (ptr->useSlopePar)
            Wscrprintf("   Slope     : Uses parameter %s from %s tree as a %s\n",
                        ptr->slopePar, getRoot(ptr->slopeTree),
                        (ptr->slopeMult) ? "multiplier" : "divisor" );
         else
            Wscrprintf("   Slope     : Uses value %g\n", ptr->slopeVal );
         if (ptr->useInterPar)
            Wscrprintf("   Intercept : Uses parameter %s from %s tree as a %s addition\n",
                        ptr->interPar, getRoot(ptr->interTree),
                        (ptr->interAdd) ? "positive" : "negative" );
         else
            Wscrprintf("   Intercept : Uses value %g\n", ptr->interVal );
      }
      ptr = ptr->next;
   }
}

static void setDefaults(unitType *p)
{
   p->useSlopePar =0;
   p->useInterPar =0;
   p->slopeVal = 1.0;
   p->interVal = 0.0;
   p->slopePar = 0;
   p->slopeTree = 0;
   p->slopeMult = 1;
   p->interPar = 0;
   p->interTree = 0;
   p->interAdd = 1;
}

static unitType *getUnitdef(char *name, unitType **start)
{
   unitType *p;

   p = *start;
   while (p)
   {
      if ((strcmp(p->id,name) == 0) || (strcmp(p->id,"") == 0))
      {
         if (strcmp(p->id,"") == 0)
            strcpy(p->id,name);
         if (p->slopePar)
            release(p->slopePar);
         if (p->interPar)
            release(p->interPar);
         setDefaults(p);
         return(p);
      }
      p = p->next;
   }
   p = (unitType *) allocateWithId(sizeof(unitType),"unit");
   strcpy(p->id,name);
   setDefaults(p);
   p->next = *start;
   *start = p;
   return(p);
}

int unit(int argc, char *argv[], int retc, char *retv[])
{
   unitType *p;
   int argnum;

   (void) retc;
   (void) retv;
   if (argc == 1)
   {
      /* display defined units */
      showUnit(&startP);
      RETURN;
   }
   if (isReal(argv[1]))
   {
      Werrprintf("unit: first argument must be a unit name and cannot start with a digit");
      ABORT;
   }
   if (strlen(argv[1]) > 12)
   {
      Werrprintf("unit: first argument must be a unit name of less than 12 characters");
      ABORT;
   }
   if (argc < 4)
   {
      Werrprintf("unit: label and slope information must be supplied");
      ABORT;
   }
   p = getUnitdef(argv[1], &startP);
   strcpy(p->label,argv[2]);
   argnum = 3;
   if (isReal(argv[argnum]))
   {
      p->slopeVal = stringReal(argv[argnum]);
      argnum++;
      if (p->slopeVal == 0.0)
      {
         strcpy(p->id,"");
         RETURN;
      }
   }
   else
   {
      int treename;

      p->useSlopePar =1;
      p->slopePar = (char *) allocateWithId(strlen(argv[argnum]) + 1, "unit");
      strcpy(p->slopePar, argv[argnum]);
      argnum++;
      p->slopeTree = getTreeIndex("current");
      p->slopeMult = 1;
      if (argnum < argc)
      {
         if ((treename = getTreeIndex(argv[argnum])) >= 0)
         {
            p->slopeTree = treename;
            argnum++;
         }
         else if (strcmp(argv[argnum],"div") == 0)
         {
            p->slopeMult = 0;
            argnum++;
         }
         else if (strcmp(argv[argnum],"mult") == 0)
         {
            p->slopeMult = 1;
            argnum++;
         }
      } 
      if (argnum < argc)
      {
         if ((treename = getTreeIndex(argv[argnum])) >= 0)
         {
            p->slopeTree = treename;
            argnum++;
         }
         else if (strcmp(argv[argnum],"div") == 0)
         {
            p->slopeMult = 0;
            argnum++;
         }
         else if (strcmp(argv[argnum],"mult") == 0)
         {
            p->slopeMult = 1;
            argnum++;
         }
      } 
   }
   if (argnum < argc)
   {
      if (isReal(argv[argnum]))
      {
         p->interVal = stringReal(argv[argnum]);
         argnum++;
      }
      else
      {
         int treename;

         p->useInterPar =1;
         p->interPar = (char *) allocateWithId(strlen(argv[argnum]) + 1, "unit");
         strcpy(p->interPar, argv[argnum]);
         argnum++;
         p->interTree = getTreeIndex("current");
         p->interAdd = 1;
         if (argnum < argc)
         {
            if ((treename = getTreeIndex(argv[argnum])) >= 0)
            {
               p->interTree = treename;
               argnum++;
            }
            else if (strcmp(argv[argnum],"sub") == 0)
            {
               p->interAdd = 0;
               argnum++;
            }
            else if (strcmp(argv[argnum],"add") == 0)
            {
               p->interAdd = 1;
               argnum++;
            }
         } 
         if (argnum < argc)
         {
            if ((treename = getTreeIndex(argv[argnum])) >= 0)
            {
               p->interTree = treename;
               argnum++;
            }
            else if (strcmp(argv[argnum],"sub") == 0)
            {
               p->interAdd = 0;
               argnum++;
            }
            else if (strcmp(argv[argnum],"add") == 0)
            {
               p->interAdd = 1;
               argnum++;
            }
         } 
      }
   }
   RETURN;
}
