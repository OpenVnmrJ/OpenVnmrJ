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

#include "node.h"
#include "symtab.h"
#include "variables.h"
#include "vnmrsys.h"
#include "magical_cmds.h"
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
/*#include <sys/dir.h>*/
#include <dirent.h>
#include <sys/stat.h>

#define MAXSHSTRING	1024
#define CATT 	0
#define CD	1
#define CP	2 
#define LF	3
#define MKDIR	4 
#define MV	5 
#define PWD	6
#define RM	7
#define RMANY	8
#define RMDIR	9
#define SHELL	10
#define W	12
#define SHELLI	13

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
#else   DEBUG
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#define TPRINT4(str, arg1, arg2, arg3, arg4) 
#define TPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif  DEBUG

extern char    *getInputBuf();
extern char    *newString();
extern char    *realString();
extern double   stringReal();
extern int      beepOn;
extern int      isReal();
extern symbol **getTreeRoot();
extern varInfo *createVar();
extern varInfo *findVar();
extern varInfo *RcreateVar();
extern varInfo *rfindVar();

extern int create();
extern int destroy();
extern int destroygroup();
extern int echo();
extern int format();
extern int groupcopy();
extern int length();
extern int shellcmds();
extern int string();
extern int substr();
extern int unixtime();

/* commands MUST be alphabetized */

cmd cmd_table[] = {
    {"create"		, create},
    {"destroy"		, destroy},
    {"destroygroup"	, destroygroup},
    {"echo"		, echo},
    {"format"		, format},
    {"groupcopy"	, groupcopy},
    {"length"		, length},
    {"rm"		, shellcmds},
    {"shell"		, shellcmds},
    {"string"		, string},
    {"substr"		, substr},
    {"systemtime"	, unixtime},
    {"unixtime"		, unixtime},
    {NULL ,  NULL}	
};

int
(*getBuiltinFunc(name))()
char *name;
{
    int (*rtn)() = 0;
    int len = strlen(name);
    cmd *p;
    for (p=cmd_table;p->name; p++){
	if (strncmp(name, p->name, len) == 0){
	    rtn = p->func;
	    break;
	}
    }
    return rtn;
}


void
clearRets(retc,retv)
  int retc;
  char *retv[];
{   int i;

    for (i=0; i<retc; ++i)
        retv[i] = NULL;
}

/*-----------------------------------------------------------------------
|
|       echo
|
|       A simple builtin command, sort of like echo(1) from UNIX.
|
+----------------------------------------------------------------------*/
int
echo(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
    int i;
    int newLine = 1;
    int noArgs = 1;

    for (i=1; i<argc; ++i){
        if ((strcmp(argv[i],"-n") == 0) && noArgs){
            newLine = 0;
        }else{
	    noArgs = 0;
            Wscrprintf("%s ",argv[i]);
        }
    }
    if (newLine){
        Wscrprintf("\n");
    }
    if (retc){
	Werrprintf("echo does not return values\n");
	clearRets(retc,retv);
	ABORT;
    }
    return 0;
}

/*-----------------------------------------------------------------------
|
|       substr
|
+----------------------------------------------------------------------*/
int
substr(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
    int count,index,length,number;
    int i;

    if (argc == 4)
    {
        TPRINT3("substr: substr('%s',%s,%s)...\n",argv[1],argv[2],argv[3]);
        length = strlen(argv[1]);
        if (isReal(argv[2]))
        {   index = (int)stringReal(argv[2]);
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
                    {   if (retv[0]=(char *)allocateWithId(number+1,"substr"))
                        {   for (i=0; i<number; ++i)
                            {
                                TPRINT3("substr:    retv[0][%d] = argv[1][%d] (='%c')\n",i,index-1+i,argv[1][index-1+i]);
                                retv[0][i] = argv[1][index-1+i];
                            }
                            TPRINT1("substr:    retv[0][%d] = '\\0'\n",count);
                            retv[0][number] = '\0';
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
                      { if (retv[0]=(char *)allocateWithId(2,"substr"))
                          retv[0][0]=0;
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
    else if (argc == 3)
    {
        TPRINT2("substr: substr('%s',%s)...\n",argv[1],argv[2]);
        if (isReal(argv[2]))
        {
            register int c;
            char     stringval[1024];

            index = (int)stringReal(argv[2]);
            if (index < 1)
            {   Werrprintf("substr: bad word count!");
                clearRets(retc,retv);
                ABORT;
            }
            length = strlen(argv[1]);
            i = 0;
            count = 1;
            while ((count < index) && (i < length))
            {
               while (((c = argv[1][i]) != '\0') && ((c == ' ') || (c == '\t')))
                 i++;   /* skip white space */
               while (((c = argv[1][i]) != '\0') && (c != ' ') && (c != '\t'))
                 i++;   /* skip word */
               count++;
            }
            while (((c = argv[1][i]) != '\0') && ((c == ' ') || (c == '\t')))
              i++;   /* skip white space */
            if (argv[1][i] == '\0')
            {
                clearRets(retc,retv);
                RETURN;
            }
            count = 0;
            while (((c = argv[1][i]) != '\0') && (c != ' ') && (c != '\t') &&
                    (count < 1000))
            {
               stringval[count++] = argv[1][i++];
            }
            stringval[count] = '\0';
            if (retc == 1)
                retv[0] = newString(stringval);
            else
                Winfoprintf("word %d is %s",index,stringval);
            RETURN;
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
int
length(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
	char	tmpstr[ 10 ];
	int	len;

	if (argc < 2) {
		Werrprintf( "%s:  must provide a string", argv[ 0 ] );
		ABORT;
	}

	len = strlen( argv[ 1 ] );
	if (retc < 1)
	  Winfoprintf( "%d characters", len );

/*  Note manner in which the value is returned from the command.
    If this convention is not followed, the VNMR program will crash.	*/

	else {
		sprintf( &tmpstr[ 0 ], "%d", len );
		retv[ 0 ] = realString( (double) len );
	}

	RETURN;
}

/*------------------------------------------------------------------------------
|
|       string
|
|       Creates a string variable if it doesn't exist 
|
+-----------------------------------------------------------------------------*/
int
string(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{   int flag,i;

    flag = 1;
    if (2 <= argc)
    {   for(i=1;i<argc;i++)
        {   varInfo     *v;
         
            if ((v = findVar(argv[i])) == NULL)
            {   if (v = createVar(argv[i]))
                    v->T.basicType = T_STRING;
                else
                    flag = 0;
            }
            else
            {   if (v->T.basicType != T_STRING)
                    if(v->T.basicType ==  T_UNDEF)
                        v->T.basicType = T_STRING;
                    else
                    {   Werrprintf("string: Type clash variable \"%s\"!",argv[i]);
                        flag = 0;       
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
static 
tolower(s)
/********/
/* convert all characters in string to lower case */
char           *s;
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
static 
toupper(s)
/********/
/* convert all characters in string to upper case */
char           *s;
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
int
format(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
    if (argc == 4)
    {   char forstr[32];
        char retstr[66];
        int  length,precision;
	int width;
	double value;
	char *tptr;
#ifndef LINUX
	extern char *strchr();
#endif
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
        if (!isReal(argv[1]))
        {
            char retstr[256];

            if (strcmp(argv[2],"lower") == 0)
            {
               strcpy(retstr,argv[1]);
               tolower(retstr);
            }
            else if (strcmp(argv[2],"upper") == 0)
            {
               strcpy(retstr,argv[1]);
               toupper(retstr);
            }
            else
            {   Werrprintf("The second argument must be 'upper' or 'lower' for this mode of %s",argv[ 0 ]);
                clearRets(retc,retv);
                ABORT;
            }
            if (1 <= retc)
                retv[0] = newString(retstr);
            else
            {   Winfoprintf("%s reformatted as %s",argv[1],retstr);
                clearRets(retc,retv);
            }
        }
        else
        {   Werrprintf("The first argument must be a string for this mode of %s",argv[ 0 ]);
            clearRets(retc,retv);
            ABORT;
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
int
groupcopy(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{   int    group;
    symbol **fromroot;
    symbol **toroot;

    if (argc == 4)
    {   if (fromroot = getTreeRoot(argv[1]))
        {   if (toroot = getTreeRoot(argv[2]))
            {   if (fromroot != toroot)
                {   if ( 0 <= ( group = goodGroup(argv[3])))
                    {   copyGVars(fromroot,toroot,group);
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
    else
    {   Werrprintf("Usage -- groupcopy(fromtree,totree,group)!");
        ABORT;
    }
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
int
create(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
    char    *tree;
    char    *type;
    int      typeindex;
    symbol **root;

#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif DEBUG

    switch (argc)
    { case 2:   tree = "current";
                type = "real";
                break;
      case 3:   
                type = argv[2];
                tree = "current";
                break;
      case 4:   
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
    if(goodName(argv[1]))
    {   if(RcreateVar(argv[1],root,typeindex))
        {
            TPRINT3("created variable \"%s\" type \"%s\" in \"%s\" tree.",
               argv[1],type,tree);
            RETURN;
        }
        else
        {   Werrprintf("create: \"%s\" already in tree!",argv[1]);
            ABORT;
        }
    }
    else
    {   Werrprintf("create: \"%s\" not valid variable name.!",argv[1]);
        ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|       destroy
|
|       This routine is used to destroy variable on a tree. 
|       Default tree is current.
|       Usage -- destroy(name[,tree])
|                tree can be  current,global,processed
|
+-----------------------------------------------------------------------------*/
int
destroy(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{   char    *tree;
    int      ret;

#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif DEBUG

    switch (argc)
    { case 2:   tree = "current";
                break;
      case 3:   tree = argv[2];
                break;
      default:  Werrprintf("Usage -- destroy(name[,tree])");
                ABORT;
    }
    ret = P_deleteVar(getTreeIndex(tree),argv[1]);
    if (ret == 0)
    {
        TPRINT2("destroyed variable '%s' in '%s' tree", argv[1],tree);
        RETURN;
    }
    if (ret == -2)
    {   Werrprintf("Variable '%s' does not exist in '%s' tree",argv[1],tree);
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
int
destroygroup(argc,argv,retc,retv)
  int argc;
  char *argv[];
  int retc;
  char *retv[];
{
    char    *tree;
    int      ret;

#ifdef DEBUG
    if (Tflag)
    {   int i;
        
        for (i=0; i<argc ;i++)
            TPRINT2("create: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif DEBUG

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

/*----------------------------------------------------------------------
|
|    getcmd
|
|	This routine returns a integer corresonding to a shell command.
|	This integer is used by a case statement.
|
/+---------------------------------------------------------------------*/

static int
getcmd(command)
  char *command;
{   if (strcmp(command,"cat") == NULL)
	return CATT;
    if (strcmp(command,"cd") == NULL)
	return CD;
    if (strcmp(command,"cp") == NULL || strcmp(command,"copy") == NULL)
	return CP;
    if (strcmp(command,"lf") == NULL || strcmp(command,"ls") == NULL 
                      || strcmp(command,"dir") == NULL)
	return LF;
    if (strcmp(command,"mkdir") == NULL)
	return MKDIR;
    if (strcmp(command,"mv") == NULL || strcmp(command,"rename") == NULL)
	return MV;
    if (strcmp(command,"pwd") == NULL)
	return PWD;
    if (strcmp(command,"rm") == NULL)
	return RM;
    if (strcmp(command,"delete") == NULL)
	return RMANY;
    if (strcmp(command,"rmdir") == NULL)
	return RMDIR;
    if (strcmp(command,"shell") == NULL)
	return SHELL;
    if (strcmp(command,"shelli") == NULL)
	return SHELLI;
    if (strcmp(command,"w") == NULL)
	return W;
    return -1;
}

/*---------------------------------------------------------------------------
|
|    shellcmds
|
|    This module contains some basic shell commands 
|
+----------------------------------------------------------------------------*/


shellcmds(argc,argv,retc,retv)
  int argc,retc;
  char *argv[],*retv[];
{
    /*Wturnoff_buttons();*/
    switch(getcmd(argv[0]))
    { /*case CATT:	return (Cat(argc,argv,retc,retv));
      case CD:	 	return (ch_dir(argc,argv,retc,retv));
      case CP:		return (Cp(argc,argv,retc,retv));
      case LF:	 	return (Lf(argc,argv,retc,retv));
      case MKDIR:	return (mk_dir(argc,argv,retc,retv));
      case MV:		return (Mv(argc,argv,retc,retv));
      case PWD:	 	return (Pwd(argc,argv,retc,retv));*/
      case RM:		return (Rm(argc,argv,retc,retv));
      /*case RMANY:	return (del_file(argc,argv,retc,retv));
      case RMDIR:	return (rm_dir(argc,argv,retc,retv));*/
      case SHELL: 	return (Shell(argc,argv,retc,retv));
      /*case SHELLI: 	return (Shell(argc,argv,retc,retv));
      case W:		return (Whoinfo(argc,argv,retc,retv));*/
      default:		fprintf(stderr,"shellcmd: (%d) unknown command\n",
				getcmd(argv[0]));
			ABORT;
    }
}

int
Shell(argc,argv,retc,retv)
     int argc; char *argv[];
     int retc; char *retv[];
{
    char  cmdstr[1024];   
    int   i;

    /* INTERACTIVE MODE NOT CURRENTLY ALLOWED */

    if (argc == 1){
	Werrprintf("%s usage: %s('shell command')", argv[0]);
	ABORT;
    }else{
	/* non-interactive shell */
	cmdstr[0] = '\0';
	for (i=1; i<argc; i++){
	    if (i > 1){
		strncat(cmdstr, " ", MAXSHSTRING - strlen(cmdstr) -1);
	    }
	    strncat(cmdstr, argv[i], MAXSHSTRING - strlen(cmdstr) -1);
	}

	/*  Redirect standard input to come from /dev/null, so the command will
	 *  receive EOF if it attempts to read anything.  Keeps VNMR commands
	 *  such as shell('cat') from locking up the VNMR program.
	 */
	strncat(cmdstr," </dev/null",MAXSHSTRING - strlen(cmdstr) -1);
	system(cmdstr);
    }
    RETURN;
}

#ifdef NOTDEFINED
static char
*get_cwd()
{
    FILE   	   *stream;
    int     	   len;
    char   	   *p;
    static char    data[1024];

     if ((stream = popen_call( "pwd", "r")) == NULL)
        return((char *) NULL);
     p = fgets_nointr(data,1024,stream);
     len = strlen(data);
     if (data[len - 1] == '\n')
          data[len - 1] = '\0';
     pclose_call(stream);
     return(data);
}

/*------------------------------------------------------------------------------
|
|	Cat
|
|	This procedure  does a standard cat command.
|
+-----------------------------------------------------------------------------*/

int Cat(argc,argv,retc,retv)
  int argc; char *argv[]; int retc; char *retv[];
{  char  cmdstr[1024];   
   int   i;
   int   screenLength;
   FILE *stream;
   char *tptr1, *tptr2;
   char  tfilepath[MAXPATHL];
   extern int printOn;

    if (argc == 1) /* This is a no no, must have arguments for cat */
    {	Werrprintf("Error: no arguments. Usage--cat('file')");
	ABORT;
    }
   /* determine if files exists and is readable by user */
   for (i=1; i<argc; i++)
      if (access(argv[i],F_OK|R_OK) == -1 || strlen(argv[i]) < 1)
      {  Werrprintf("File '%s' not accessible",argv[i]);
         ABORT;
      }
    strcpy(cmdstr,"/bin/cat ");
    for (i=1; i<argc; i++)
    {
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
    }
    Wsettextdisplay("Cat");
    Wshow_text();
    screenLength = WscreenSize();

   /* Startup the cat command and pipe in the output */
   if (Wissun() && !printOn)
   {
        for (i=1; i<argc; i++)
        {
	    if (argv[i][0] != '/')
            {
		/*
                if (getcwd( &tfilepath[ 0 ], sizeof( tfilepath ) - 1 ) == NULL)
		*/
		if (( tptr1 = get_cwd()) == NULL)
                {
                     Werrprintf( "Cannot obtain current working directory" );
                     return;
                }
		sprintf(tfilepath, "%s/", tptr1);
                strcat(tfilepath, argv[i]);
            }
            else
                strcpy(tfilepath, argv[i]);
            sendTripleEscToMaster( 'P', tfilepath);
        }
        RETURN;
   }

   if ((stream = popen_call(cmdstr,"r"))  == NULL)
   {  Werrprintf("Problem with creating cat command with popen");
      ABORT;
   }
   Wscrprintf("\n"); /* do a CRLF for seperation */
   More(stream,screenLength);  /* more it out to screen */
   RETURN;
}
#endif /* NOTDEFINED */

/*----------------------------------------------------------------------------
|
|	rm
|
|	This procedure  does a standard rm command.
|
+---------------------------------------------------------------------------*/

int
Rm(argc,argv,retc,retv)
  int argc; char *argv[];
  int retc; char *retv[];
{   char cmdstr[1024];   
    char *ival;
    int  i;
#ifndef LINUX
    extern char *strchr();
#endif

    strcpy(cmdstr,"/bin/rm");
    for (i=1; i<argc; i++)
    {	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
    }
    TPRINT1("rm: command string \"%s\"\n",cmdstr);
    system(cmdstr);
    for (i=1; i<argc; i++)
      if (argv[i][0] != '-')
          if ( access(argv[i],F_OK) == 0)
             Werrprintf("cannot delete %s",argv[i]);
    RETURN;
}

static double
double_of_timeval( tv )
struct timeval	tv;
{
	return( (double) (tv.tv_sec) + ((double) (tv.tv_usec)) / 1.0e6 );
}

/*---------------------------------------------------------------------------
|
|	unixtime -  make underlying basic UNIX / POSIX time
|                   available to Magical
|
---------------------------------------------------------------------------*/

int
unixtime( argc, argv, retc, retv )
int argc;
char *argv[];
int retc;
char *retv[];
{
	double		dtime;
	struct timeval	curtime;

	gettimeofday( &curtime, NULL );
	dtime = double_of_timeval( curtime );
	if (retc < 1) {
		if (argc < 2)
		  Wscrprintf( "UNIX time: %d\n", (int) dtime );
		else
		  Wscrprintf( "%s: %d\n", argv[ 1 ], (int) dtime );
	}
	else {
		char	tstr[ 30 ];

		sprintf( &tstr[ 0 ], "%.16g", dtime );
		retv[ 0 ] = newString( &tstr[ 0 ] );
	}

	RETURN;
}
