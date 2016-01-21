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
/*  lookup	 -  search and return words from a text file	*/
/****************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "tools.h"
#include "vnmrsys.h"
#include "group.h"
#include "pvars.h"
#include "wjunk.h"


#define SEEK     0
#define SKIP     1
#define READ     2
#define READLINE 3
#define COUNT    4
#define WHITESPACE    5
#define FKEY     6
#define COUNTLINE     7

#define MAXWORD 1024

#ifdef  DEBUG
extern int debug1;
#define DPRINT0(str) \
	if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
	if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#else 
#define DPRINT0(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#endif 

static int custom_whitespace = 0;
static char white_space[MAXSTR];
extern char UserName[];

/********/
static void tolowerString(char *s)
/********/
/* convert all characters in string to lower case */
{ int i;
  i=0;
  while (s[i])
    {  if ((s[i]>='A')&&(s[i]<='Z')) s[i]=s[i]+'a'-'A';
       i++;
    }
}

static void setnewWhitespace(char vals[])
{
   int index,val;
   int num;

   num = strlen(vals);
   index = 0;
   custom_whitespace = 0;
   if (num && (num < 120))
   {
      while (index < num)
      {
         val = vals[index];
         index++;
         white_space[custom_whitespace++] = val;
      }
   }
   else
   {
      Werrprintf("illegal number of text delimiters");
      custom_whitespace = 0;
   }
}

static int check_whitespace(int ch)
{
   int index;
   index = 0;
   while (index < custom_whitespace)
   {
      if (white_space[index] == ch)
         return(1);
      index++;
   }
   return(0);
}

/***********************************************************/
/*  whitespace defines the characters which separate words */
/***********************************************************/
static int whitespace(int ch, int use_custom)
/***********************/
{
  if ( use_custom && custom_whitespace )
     return(check_whitespace(ch));
  else
     return((ch == ' ') || (ch == '\t') || (ch == '\n') ||
            (ch == ',') || (ch == '\r'));
}

/************************************************/
/*  priv_getline returns the next line from a file	*/
/************************************************/
static int priv_getline(FILE *path, char line[], int limit)
/**********************************/
{
  int ch,i;

  ch = line[0] = '\0';
  i = 0;
  while ((i < limit -1) && ((ch = getc(path)) != EOF) && (ch != '\n'))
    line[i++] = ch;
  // check for windows end of line '\r\n'
  if (i && (line[i-1] == '\r') && (ch == '\n'))
    i--;
  line[i] = '\0';
  if ((i == 0) && (ch == EOF))
     return(0);
  else
     return(1);
}

/************************************************/
/*  getword returns the next word from a file	*/
/*  Not a static ...  wsram uses it		*/
/************************************************/
int
getword(FILE *path, char word[], int limit, char lower[], int use_custom)
/**********************************/
{
  int ch,i;

  word[0] = '\0';
  while (((ch = getc(path)) != EOF) && whitespace(ch,use_custom))
    /* skip white space */
    ;
  if (ch == EOF)
    return(0);
  i = 0;
  while ((i < limit -1) && !whitespace(ch,use_custom) && (ch != EOF))
  {
    word[i++] = ch;
    ch = getc(path);
  }
  word[i] = '\0';
  strcpy(lower,word);
  tolowerString(lower);
  return(1);
}

/*****************************/
int lookup(int argc, char *argv[], int retc, char *retv[])
{
  char         codeword[MAXSTR];
  int          count;
  static char  savefilepath[MAXPATH] = "";
  FILE        *path;
  char         lower[MAXWORD];
  int          mode;
  char         nextword[MAXWORD];
  static int   saveseekloc = 0;
  int          args_requested;
  int          retc_save;
  char         filepath[MAXPATH] = "";
  int          seekloc;
  int          useKey;
  int          caseSensitive;

  mode = SEEK;
  
  strcpy(filepath,savefilepath);
  seekloc = saveseekloc;
  useKey = 0;
  if (argc>1 && (strcmp(argv[1],"mfile") == 0))
  {
    if (argc == 2)
    {
      Werrprintf("Usage: lookup('mfile','key',...)");
      ABORT;
    }
    seekloc = 0;
    sscanf(argv[2],"%[^;]; %d",filepath,&seekloc);
    argc -= 2;
    argv += 2;
    custom_whitespace = 0;
    useKey = 1;
  }
  else if (argc>1 && (strcmp(argv[1],"file") == 0))
  {
    if (argc == 2)
    {
      Werrprintf("Usage: lookup('file','path',...)");
      ABORT;
    }
    strcpy(filepath,argv[2]);
    strcpy(savefilepath,argv[2]);
    argc -= 2;
    argv += 2;
    seekloc = 0;
    custom_whitespace = 0;
  }
  else if (strcmp(filepath,"") == 0)
  {
    Werrprintf("Usage: lookup('file','path',...)");
    ABORT;
  }
  if ((path=fopen(filepath,"r")) == NULL)
  {
    Werrprintf("cannot open file %s",filepath);
    ABORT;
  }

  caseSensitive = 0;
  count = 0;
  args_requested = 0;
  retc_save = retc;
  if (seekloc)
    fseek(path,seekloc,0);

  while (--argc > 0)
  {
    strcpy(codeword,*++argv);
    tolowerString(codeword);
    if (strcmp(codeword,"seek") == 0)
    {
      mode = SEEK;
      if (argc > 1)
      {
         strcpy(codeword,*++argv);
         tolowerString(codeword);
         caseSensitive = 0;
         argc--;
      }
      else
         continue;
    }
    else if (strcmp(codeword,"seekcs") == 0)
    {
      mode = SEEK;
      if (argc > 1)
      {
         strcpy(codeword,*++argv);
         caseSensitive = 1;
         argc--;
      }
      else
         continue;
    }
    else if (strcmp(codeword,"skip") == 0)
      mode = SKIP;
    else if (strcmp(codeword,"read") == 0)
      mode = READ;
    else if (strcmp(codeword,"readline") == 0)
      mode = READLINE;
    else if (strcmp(codeword,"delimiter") == 0)
      mode = WHITESPACE;
    else if (strcmp(codeword,"filekey") == 0)
      mode = FKEY;
    else if (strcmp(codeword,"countline") == 0)
      mode = COUNTLINE;
    else if (strcmp(codeword,"count") == 0)
    {
      mode = COUNT;
      if (argc > 1)
      {
         strcpy(codeword,*++argv);
         tolowerString(codeword);
         caseSensitive = 0;
         argc--;
      }
      else
         continue;
    }
    else if (strcmp(codeword,"countcs") == 0)
    {
      mode = COUNT;
      if (argc > 1)
      {
         strcpy(codeword,*++argv);
         caseSensitive = 1;
         argc--;
      }
      else
         continue;
    }
    switch (mode)
    {
      case COUNT:
        count = 0;
        while (getword(path,nextword,MAXWORD,lower,custom_whitespace))
          if (caseSensitive)
          {
            if (strcmp(nextword,codeword) == 0)
              count++;
          }
          else
          {
            if (strcmp(lower,codeword) == 0)
              count++;
          }
        DPRINT2("found the word %s %d times\n",codeword,count);
        args_requested++;
        if (retc-- > 0)
          *retv++ = realString((double) count);
        break;
      case COUNTLINE:
        {
           int ch;
           count = 0;
           while ((ch = getc(path)) != EOF)
           {
              if (ch == '\n')
                 count++;
           }
           DPRINT1("found %d lines\n",count);
           args_requested++;
           if (retc-- > 0)
             *retv++ = realString((double) count);
        }
        break;
      case SEEK:
        if (caseSensitive)
        {
           while (getword(path,nextword,MAXWORD,lower,custom_whitespace) &&
               strcmp(nextword,codeword))
          /* search until codeword is found */
             ;
        }
        else
        {
           while (getword(path,nextword,MAXWORD,lower,custom_whitespace) &&
               strcmp(lower,codeword))
          /* search until codeword is found */
             ;
        }
        break;
      case SKIP:
        if ((argc > 1) && isReal(*(argv+1)))
        {
          count = (int) stringReal(*++argv);
          argc--;
        }
        else
          count = 1;
        DPRINT1("skipping %d words\n",count);
        while ((count-- > 0) && getword(path,nextword,MAXWORD,lower,custom_whitespace))
          /* skip count number of words */
          ;
        break;
      case READ:
        if ((argc > 1) && isReal(*(argv+1)))
        {
          count = (int) stringReal(*++argv);
          argc--;
        }
        else
          count = 1;
        DPRINT1("returning %d words\n",count);
        args_requested += count;
        while ((count-- > 0) && getword(path,nextword,MAXWORD,lower,custom_whitespace))
          if (retc-- > 0)
            *retv++ = newString(nextword);
        break;
      case READLINE:
        if ((argc > 1) && isReal(*(argv+1)))
        {
          count = (int) stringReal(*++argv);
          argc--;
        }
        else
          count = 1;
        DPRINT1("returning %d lines\n",count);
        args_requested += count;
        while ((count-- > 0) && priv_getline(path,nextword,MAXWORD))
          if (retc-- > 0)
            *retv++ = newString(nextword);
        break;
      case WHITESPACE:
        if (argc > 1)
        {
          setnewWhitespace(*++argv);
          argc--;
          mode = SEEK;
        }
        else
        {
          Werrprintf("delimiter keyword must be followed by characters");
          fclose(path);
          ABORT;
        }
        break;
      case FKEY:
        args_requested += 1;
        if (retc-- > 0)
        {
           sprintf(lower,"%s; %ld",filepath,ftell(path));
           *retv++ = newString(lower);
        }
        break;
    }
  }
  if (!useKey)
     saveseekloc = ftell(path);
  if (args_requested + 1 == retc_save)
  {
     DPRINT3("requested %d args; %d colon args; returned %d args\n",
              args_requested, retc_save, retc_save - retc);
     retc_save -= retc;
     while (retc > 1)
     {
       *retv++ = newString("");
       retc--;
     }
     if (retc-- > 0)
        *retv++ = realString((double) retc_save);
  }
  while (retc-- > 0)
    *retv++ = newString("");
  fclose(path);
  RETURN;
}

/*
 *  rights(right<,false value>)  check if a right is granted
 *  The false value can be specified, to make it easy to use
 *  as a show condition (don't show or gray it out)
 *
 */

/*  rightsTemplate holds the name of the current template in
 *  which to search for rights.
 *  allRights is a flag with three values.
 *     -1 means the rights has bot been initialized.  Operator
 *        change should set it to -1.
 *      1 means that the operator has all rights.  This happens
 *        if the operator is not in the operatorlist file or
 *        the template does not exist.
 *      0 means use the template to determine a right.
 */
static char rightsTemplate[MAXSTR];
static char lastOperator[MAXSTR]= "";
static int allRights = -1;
 
static int initUserRights(char *operator)
{
   char  opPath[MAXSTR];
   FILE *fd;
   char  nextline[MAXWORD];
   char *token;

   allRights = 1;
   sprintf(opPath,"%s/adm/users/operators/operatorlist",systemdir);
   if ((fd=fopen(opPath,"r")) == NULL)
   {
      return(1);
   }
   while (priv_getline(fd,nextline,MAXWORD))
   {
      char op[MAXWORD];
      char own[MAXWORD];
      char dum1[MAXWORD];
      char dum2[MAXWORD];
      char dum3[MAXWORD];
      char dum4[MAXWORD];
      char profile[MAXWORD];
      int ret;

      if (nextline[0] != '#')
      {
         ret = sscanf(nextline,"%s  %[^;];%[^;];%[^;];%[^;];%[^;];%s",
                op,own,dum1,dum2,dum3,dum4,profile);

         // This must be the newer version with only 6 elements, reread
         // into the correct variables.
         if(ret == 6) {
           ret = sscanf(nextline,"%s  %[^;];%[^;];%[^;];%[^;];%s",
                op,own,dum1,dum2,dum3,profile);
         }


         // 'own' is a comma separated list of users.  If there is a comma, we
         // need to check each of the users for a match.
         token = strtok(own, ",");
         while (token != NULL) {
            if ( (ret>=6) && !strcmp(op,operator) && !strcmp(token,UserName) )
            {
               allRights = 0;
               if (profile[strlen(profile)-1] == ';')
                  profile[strlen(profile)-1] = '\0';
               sprintf(rightsTemplate,"%s/adm/users/userProfiles/%s.txt",
                       systemdir,profile);
               break;
            }
            token=strtok(NULL,",");
         }
      }
   }
   fclose(fd);
   return(0);
}

static int checkRights( char *right )
{
   FILE *fd;
   char nextline[MAXWORD];
   int val = 1;

   if ((fd=fopen(rightsTemplate,"r")) == NULL)
   {
      return(val);
   }
   while (priv_getline(fd,nextline,MAXWORD))
   {
      char rightName[MAXSTR];
      char rightValue[MAXSTR];
      int ret;

      if (nextline[0] != '#')
      {
         ret = sscanf(nextline,"%s %s", rightName,rightValue);
         if ( (ret==2) && !strcasecmp(rightName,right) )
         {
            if (strcmp(rightValue,"true"))
               val = 0;
            break;
         }
      }
   }
   fclose(fd);
   return(val);
}

/*****************************/
int rights(int argc, char *argv[], int retc, char *retv[])
/*****************************/
{
   char operator[MAXSTR];

   if ((argc != 2) && (argc != 3) )
   {
      Winfoprintf("Usage: rights('nameOfRight'<,error return value>)");
      RETURN;
   }
   if (P_getstring(GLOBAL,"operator",operator,1,MAXSTR-1))
   {
      lastOperator[0] = '\0';
      if (retc)
         retv[0] = realString((double) 1.0);
      else
         Winfoprintf("operator rights initialization failed. All rights granted");
      allRights = -1;
      RETURN;
   }
   if ( (lastOperator[0] == '\0') || strcmp(operator,lastOperator) )
   {
      if (initUserRights(operator))
      {
         if (retc)
            retv[0] = realString((double) 1.0);
         else
            Winfoprintf("rights initialization failed. All rights granted");
         RETURN;
      }
   }
   if (allRights == 1)
   {
      if (retc)
         retv[0] = realString((double) 1.0);
      else
         Winfoprintf("all rights granted");
      RETURN;
   }
   strcpy(lastOperator,operator);
   if (access(rightsTemplate, R_OK) == 0)
   {
      if (checkRights(argv[1]))
      {
         if (retc)
            retv[0] = realString((double) 1.0);
         else
            Winfoprintf("right %s granted", argv[1]);
      }
      else
      {
         if (retc)
         {
            if (argc == 3)
               retv[0] = newString(argv[2]);
            else
               retv[0] = realString((double) 0.0);
         }
         else
            Winfoprintf("right %s denied", argv[1]);
      }
      RETURN;
   }
   if (retc)
      retv[0] = realString((double) 1.0);
   else
      Winfoprintf("all rights granted");
   RETURN;
}

/* rightsEval is duplicate of rights command but is callable from C functions
 * single argument is the right name
 */
/*****************************/
int rightsEval(char *rightsName)
/*****************************/
{
   char operator[MAXSTR];

   if (P_getstring(GLOBAL,"operator",operator,1,MAXSTR-1))
   {
      /* operator rights initialization failed. All rights granted */
      allRights = -1;
      return(1);
   }
   if ( (lastOperator[0] == '\0') || strcmp(operator,lastOperator) )
   {
      if (initUserRights(operator))
      {
         /* rights initialization failed. All rights granted */
         return(1);
      }
   }
   if (allRights == 1)
   {
      /* all rights granted */
      return(1);
   }
   strcpy(lastOperator,operator);
   if (access(rightsTemplate, R_OK) == 0)
   {
      if (checkRights(rightsName))
      {
         /* right granted */
         return(1);
      }
      else
      {
         /* right denied */
         return(0);
      }
   }
   /* all rights granted */
   return(1);
}
