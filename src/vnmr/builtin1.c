/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|	builtin1.c    
|
|		These routines are some more builtin commands or
|               procedures on the magical interpreter. They are 
|
|               clear        - clear the screen 
|               devicenames  - print a list of names of printers/plotters
|               fread        - read a variable tree in from a file 
|               fsave        - write a variable tree out to a file
|               getvalue     - get values of any variable
|               getdgroup    - get dgroup of any variable
|               getlimit     - get limits of any variable
|               gettype      - get subtype of any variable
|               off          - set a variable un active
|               on           - set a variable active
|               printoff     - Close temp file and send to printer
|               printon      - Redirect scroll output to a temp file
|               readparam    - read a paramter
|               rtx	     - read paramters based on P_LOCK bit
|               settype      - set type of any variable
|               setvalue     - set values of any variable
|               setenumeral  - set enumeral values of a string variable
|               setlimit     - set the min,max,and step size of a variable 
|               setplotdev   - set a plotter name
|               setprintdev  - set a printter name
|               setprotect   - set protection bits of a variable
|               teststr      - test string array for value
|               writeparam   - write out a paramter
+-----------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "group.h"
#include <math.h>
#include "params.h"
#include "symtab.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define _REENTRANT
#include <string.h>
#include <ctype.h>
#include "allocate.h"
#include "buttons.h"
#include "tools.h"

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

extern int find_param_file(char *init, char *final);
extern int assignReal(double d, varInfo *v, int i);
extern int assignString(const char *s, varInfo *v, int i);
extern int assignEString(char *s, varInfo *v, int i);
extern void disposeStringRvals(Rval *r);
extern int P_listnames(int tree, char *list);
extern void set_datastation(int val);
extern void textOnTop();
extern void graphicsOnTop();
extern void dgSetTtyFocus();
extern void clearGraphFunc();
extern int storeOnDisk(char *name, varInfo *v, FILE *stream);
extern int readvaluesfromdisk(symbol **root, FILE *stream);
extern int readNewVarsfromdisk(symbol **root, FILE *stream);
extern int readfromdisk(symbol **root, FILE *stream);
extern int copytodisk(symbol **root, FILE *stream);
extern void setGlobalPars();
extern int goodGroup(char *group);
extern int goodType(char *type);
extern int exec_shell(char *cmd);
// extern FILE *popen_call(char *cmdstr, char *mode);
extern int  pclose_call(FILE *pfile);
extern char *fgets_nointr(char *datap, int count, FILE *filep );
extern void disp_print(char *t);
extern int setPrinterName(char *name);
extern int setPlotterName(char *name);
extern int is_whitespace(char c, int len, char delimiter[]);


extern char     LprinterPort[128];

#ifdef VNMRJ
extern int VnmrJViewId;
extern int jsendParamMaxMin(int num, char *param, char *tree );
#endif 

#define VAL_LENGTH 128

/*------------------------------------------------------------------------------|
|       flip
|
|       This procedure flips the screen such that the text window is
|       either flipped behind or in front of the graphics window
|       Usage -- flip
|
+----------------------------------------------------------------------------*/

int        textWindowVisable = 0; /* initally text window is visable. Not really */
int   textIsOn = 1;
int   grafIsOn = 1;
int   autotextIsOn = 1;
int   autografIsOn = 1;

int flip(int argc, char *argv[], int retc, char *retv[])
{   
    int save;

   (void) retc;
   (void) retv;
    if (argc > 1)
    {
	if (strcmp(argv[1], "text") == 0)
	{
            if (argc > 2)
            {
	       if (strcmp(argv[2], "off") == 0)
		 textIsOn = 0;
	       else if (strcmp(argv[2], "on") == 0)
		 textIsOn = 1;
	       else if (strcmp(argv[2], "autooff") == 0)
		 autotextIsOn = 0;
	       else if (strcmp(argv[2], "autoon") == 0)
		 autotextIsOn = 1;
            }
            else
            {
               save = textIsOn;
               textIsOn = 1;
               textWindowVisable = 0;
               textOnTop();
               textIsOn = save;
            }
	}
	else if (strcmp(argv[1], "graphics") == 0)
	{
            if (argc > 2)
            {
	       if (strcmp(argv[2], "off") == 0)
		 grafIsOn = 0;
	       else if (strcmp(argv[2], "on") == 0)
		 grafIsOn = 1;
	       else if (strcmp(argv[2], "autooff") == 0)
		 autografIsOn = 0;
	       else if (strcmp(argv[2], "autoon") == 0)
		 autografIsOn = 1;
            }
            else
            {
               save = grafIsOn;
               grafIsOn = 1;
               textWindowVisable = 1;
               graphicsOnTop();
               grafIsOn = save;
            }
	}
	RETURN;
    }
    else if (textWindowVisable)
    {
	TPRINT0("flip: calling graphicsOnTop\n");
        save = grafIsOn;
        grafIsOn = 1;
        graphicsOnTop();
        grafIsOn = save;
    }
    else
    {
	TPRINT0("flip: calling textOnTop\n");
        save = textIsOn;
        textIsOn = 1;
        textOnTop();
        textIsOn = save;
    }
    RETURN;
}
 
/*
   parType = 0 is no pars
   parType = 1 is arrayed parameter
   parType = 2 is string list;
 */
static int parType = 0;
static char parName[MAXSTR];

int getParNameInit(char nameList[])
{
   int i;
   int count;
   char    delimiter[] = " ,\t";
   int length;
   int length2;
   register int ch;
   int firstWord;
   int j;
   symbol **root;
   varInfo *v;

   if (nameList == NULL)
      return(0);
   length = strlen(nameList);
   length2 = strlen(delimiter);
   i = count = 0;
   firstWord = 0; 
   parType = 0;
   while (i < length)
   {
      while (((ch = nameList[i]) != '\0') && (is_whitespace(ch,length2,delimiter)))
         i++;   /* skip white space */
      if (nameList[i] != '\0')
      {
         if ( (count == 0) && (nameList[i] == '$') )
         {
            parType = 1;
            firstWord = i;
            break;
         }
         while (((ch = nameList[i]) != '\0') &&
                 (!is_whitespace(ch,length2,delimiter)))
            i++;   /* skip word */
         count++;
      }
   }

   if ( parType == 0 )
   {
      parType = 2;
      return(count);
   }

   i = firstWord;
   j = 0;
   while (((ch = nameList[i]) != '\0') &&
           (!is_whitespace(ch,length2,delimiter)))
   {
      parName[j++] = ch;
      i++;
   }
   parName[j] = '\0';
   if ((root=selectVarTree(parName)) == NULL)
   {
            Werrprintf("local variable \"%s\" doesn't exist",parName);
            return(0);
   }
   if ((v = rfindVar(parName,root)) == NULL)
   {   Werrprintf("variable \"%s\" doesn't exist",parName);
          return(0);
   }
   return(v->T.size);
}

char *getParName(char nameList[], char name[], int len, int index)
{
   if (parType == 1)
   {
      symbol **root;
      varInfo *v;
      name[0] = '\0';
      if ((root=selectVarTree(parName)) == NULL)
      {
         Werrprintf("local variable \"%s\" doesn't exist",parName);
         return(NULL);
      }
      if ((v = rfindVar(parName,root)) == NULL)
      {   Werrprintf("variable \"%s\" doesn't exist",parName);
          return(NULL);
      }
      else if (v->T.basicType == T_STRING)
      {
       Rval *r;
       int i;

       r = v->R;
       for (i=1; i <= v->T.size; i++ )
       {
           if (r)
           {
               if (i == index )
               {
                  strncpy(name,r->v.s,len);
                  return(name);
               }
               r = r->next;
           }
           else
           {
               i = v->T.size + 1;
           }
       }
      }
      else
      {
          return(NULL);
      }
   }
   else if (parType == 2)
   {
      char    delimiter[] = " ,\t";
      int length;
      int length2;
      int count;
      int i;
      register int ch;
      length = strlen(nameList);
      length2 = strlen(delimiter);
      i = count = 0;
      name[0] = '\0';
      while ( (i < length) && ( count < index) )
      {
         while (((ch = nameList[i]) != '\0') && (is_whitespace(ch,length2,delimiter)))
            i++;   /* skip white space */
         count++;
         if (count == index)
         {
            int j;
            j = 0;
            while (((ch = nameList[i]) != '\0') &&
                    (!is_whitespace(ch,length2,delimiter)) && (j < len) )
            {
               name[j++] = ch;
               i++;
            }
            name[j] = '\0';
            return(name);
         }
         else
         {
            while (((ch = nameList[i]) != '\0') && (! is_whitespace(ch,length2,delimiter)))
               i++;   /* skip word */
         }
      }
   }
   return(NULL);
}

/*------------------------------------------------------------------------------
|
|	rtx
|
|	This procedure reads one or more parameters from a file
|   rtx(filename <, tree <, 'keyword1' <, 'keyword2' >>>)
|   
|   The rtx command retrieves parameters from filename,  based on the setting
|   of the P_LOCK bit and using the rules below.
|   keyword1 may be "keep" or "rt".  Default is keep
|   keyword2 may be "clear" or "noclear".  Default is clear.  keyword2 determines
|   if the P_LOCK bit is cleared after it is rt'ed.
|   
|   Truth table for rtx.
|   
|   Status of P_LOCK       Status of P_LOCK      keyword1      result
|   in current exp.        in filename.
|   
|     on                       off              keep or rt    do not rt
|     on                       on               keep or rt    do not rt
|   
|     off                      on               keep or rt      do rt
|     off                      off                 keep       do not rt
|     off                      off                 rt           do rt
|   
|   <no parameter>             on               keep or rt      do rt
|   <no parameter>             off                 keep       do not rt
|   <no parameter>             off                 rt           do rt
|
|
+-----------------------------------------------------------------------------*/

#define KEEP    1
#define RT      2
#define CLEAR   3
#define NOCLEAR 4
int rtx(int argc, char *argv[], int retc, char *retv[] )
{
   char filepath[MAXPATH];
   char filepath0[MAXPATH];
   int	tree;
   int	key1;
   int  key2;
   char *name;
   extern char *get_cwd();
   char *pathptr;

   (void) retc;
   (void) retv;
   if (argc < 2 || argc > 5) {
   	Werrprintf( "Usage -- %s( file, tree, key1, key2 )", argv[ 0 ] );
   	ABORT;
   }

   key1 = KEEP;
   key2 = CLEAR;
   if (argc == 2)
      tree = getTreeIndex("current");
   else
   {
      tree = getTreeIndex(argv[ 2 ]);
      if (tree == -1)
      {
        Werrprintf( "%s:  '%s' tree doesn't exist", argv[ 0 ], argv[ 2 ] );
        ABORT;
      }
      if (argc > 3)
      {
         if (!strcmp(argv[3],"rt") )
            key1 = RT;
         if (argc > 4)
            if (!strcmp(argv[4],"noclear") )
               key2 = NOCLEAR;
      }
   }

   name = argv[1];
   if (strlen(name) >= (MAXPATH-32))
   {   Werrprintf("file path too long");
       ABORT;
   }
   if (name[0]!='/')
   {
      pathptr = get_cwd();
      strcpy(filepath0, pathptr);
      strcat(filepath0,"/");
      strcat(filepath0,name);
   }
   else
      strcpy(filepath0,name);
   if (find_param_file(filepath0, filepath))
   {
      Werrprintf("cannot find parameters from %s [%s]",argv[1], filepath);
      ABORT;
   }
   if ( P_rtx(tree,filepath, (key1 == RT), (key2 == CLEAR) ) )
   {
      Werrprintf("cannot read parameters from %s [%s]",argv[1], filepath);
      ABORT;
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	readparam
|
|	This procedure reads one or more parameters from a file
|
+-----------------------------------------------------------------------------*/
int readparam(int argc, char *argv[], int retc, char *retv[] )
{
	int	tree;
	int	type;
        char    *currentPar;
        char    par[MAXSTR];
        int     pnum;
        int     pindex;

        (void) retc;
        (void) retv;
	if (argc < 3 || argc > 5) {
		Werrprintf( "Usage -- %s( file, name, tree, type )", argv[ 0 ] );
		ABORT;
	}

        type = 1; /* read type */
	if (argc == 3)
	  tree = getTreeIndex("current");
	else if ( ! strcmp(argv[ 3 ], "list") )
        {
           type = 4;
           tree = 0; /* not used in this case */
        }
	else if ( ! strncmp(argv[ 3 ], "alist ",6) )
        {
           type = 5;
           tree = 3; /* which argv value is used */
           if ( ! strstr(argv[tree],"$") )
           {
	      Werrprintf( "%s:  'alist' option must be folowed by parameter name", argv[ 0 ]);
	      ABORT;
           }
        }
	else
        {
	  tree = getTreeIndex(argv[ 3 ]);
	  if (tree == -1)
          {
	     Werrprintf( "%s:  '%s' tree doesn't exist", argv[ 0 ], argv[ 3 ] );
	     ABORT;
	  }
        }
	if (argc == 5)
        {
           if (strcmp(argv[4], "read") == 0)
              type = 1; /* read type */
           else if (strcmp(argv[4], "replace") == 0)
              type = 2; /* replace type */
           else if (strcmp(argv[4], "add") == 0)
              type = 3; /* add type */
           else if (strcmp(argv[4], "list") == 0)
              type = 4; /* list type */
           else if (strncmp(argv[4], "alist ",6) == 0)
           {
              type = 5; /* arrayed list type */
              tree = 4; /* which argv value is used */
              if ( ! strstr(argv[tree],"$") )
              {
	         Werrprintf( "%s:  'alist' option must be folowed by parameter name", argv[ 0 ]);
	         ABORT;
              }
           }
        }
        if (type == 4)  /* list type */
        {
           currentPar = NULL;
           P_readnames(argv[1], &currentPar);
           if (currentPar == NULL)
           {
	      if (retc > 0)
	         retv[0] = newString("");
	      else
	         Winfoprintf("File %s has no parameters\n",argv[1]);
           }
           else
           {
	      if (retc > 0)
	         retv[0] = newString(currentPar);
	      else
	         Winfoprintf("File %s has '%s' parameters\n",argv[1],currentPar);
              release(currentPar);
           }
           RETURN;
        }
        if (type == 5)  /* arrayed list type */
        {
           int i, j;
           int count;
           char parName[MAXSTR];
           symbol **root;
           varInfo *v;
           int ch;

           i = 5;
           j = 0;
           while ((ch = argv[tree][i]) != '\0' && (ch == ' ') )
              i++;
           while ((ch = argv[tree][i]) != '\0' && (ch != ' ') )
           {
              parName[j++] = ch;
              i++;
           }
           parName[j] = '\0';
           if ((root=selectVarTree(parName)) == NULL)
           {
              Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],parName);
              ABORT;
           }
           if ((v = rfindVar(parName,root)) == NULL)
           {   Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],parName);
               ABORT;
           }
           if (v->T.basicType != T_STRING)
           {   Werrprintf("%s: variable \"%s\" must be a string type",argv[0],parName);
               ABORT;
           }

           currentPar = NULL;
           P_readnames(argv[1], &currentPar);
           if (currentPar == NULL)
           {
	      if (retc > 0)
	         retv[0] = intString(0);
	      else
	         Winfoprintf("File %s has no parameters\n",argv[1]);
           }
           else
           {
              i = 0;
              count = 0;
              while (*(currentPar + i) != '\0')
              {
                 j = 0;
                 while (((ch = *(currentPar + i) ) != '\0') && (ch != ' '))
                 {
                    parName[j++] = ch;
                    i++;
                 }
                 parName[j] = '\0';
                 if (j)
                 {
                    count++;
                    if (count == 1)
                       assignString(parName,v,0);
                    else
                       assignString(parName,v,count);
                 }
                 while (((ch = *(currentPar + i) ) != '\0') && (ch == ' '))
                    i++;
              }

              
	      if (retc > 0)
	         retv[0] = intString(count);
	      else
	         Winfoprintf("File %s has '%d' parameters\n",argv[1],count);
              release(currentPar);
           }
           RETURN;
        }

        P_treereset(TEMPORARY);		/* clear the tree first */
        if ( P_read(TEMPORARY,argv[1]) )
	{  Werrprintf("cannot read parameters from %s",argv[1]);
           P_treereset(TEMPORARY);
	   RETURN;
        }
        pnum = getParNameInit(argv[2]);
        pindex = 0;
        while (pindex < pnum)
        {
           varInfo *v;

           pindex++;
           currentPar = getParName(argv[2], par, MAXSTR-1, pindex);
           if (type == 1)
           {
              /* read type: add currentPar to tree.  If currentPar already exists to
               * the tree, the old one is first deleted before the new one is added.
               */
              P_copyvar(TEMPORARY,tree,currentPar,currentPar);
              appendvarlist(currentPar);
           }
           else if (type == 2)
           {
              /* replace type: add currentPar to tree only if currentPar already exists in
               * the tree. The old one is deleted before the new one is added.
               */
              if ((v = P_getVarInfoAddr(tree,currentPar)) != (varInfo *) -1)
              {
                 P_copyvar(TEMPORARY,tree,currentPar,currentPar);
                 appendvarlist(currentPar);
              }
           }
           else if (type == 3)
           {
              /* add type: add currentPar to tree only if currentPar does not already
               * exist in the tree.
               */
              if ((v = P_getVarInfoAddr(tree,currentPar)) == (varInfo *) -1)
              {
                 P_copyvar(TEMPORARY,tree,currentPar,currentPar);
                 appendvarlist(currentPar);
              }
           }
        }
        P_treereset(TEMPORARY);		/* clear the tree */
	RETURN;
}

/*------------------------------------------------------------------------------
|
|	writeparam
|
|	This procedure writes one or more parameters to a file
|
+-----------------------------------------------------------------------------*/
int writeparam(int argc, char *argv[], int retc, char *retv[] )
{
   symbol  **sWriteP;
   varInfo *v;
   FILE    *stream;
   char    *curtree;
   char    *currentPar;
   int     type;
   char    parList[1024];
   char    *pParList;
   char    par[MAXSTR];
   int     pnum;
   int     pindex;

   (void) retc;
   (void) retv;
   if (argc < 3 || argc > 5) {
      Werrprintf( "Usage -- %s( file, name, tree [,'add' or 'replace'] )", argv[ 0 ] );
      ABORT;
   }

   if ((argc == 3) ||
       ( (argc == 4) && (!strcmp(argv[3],"add") || !strcmp(argv[3],"replace")) ) )
      curtree = "current";
   else
      curtree = argv[ 3 ];

   sWriteP = getTreeRoot( curtree );
   if (sWriteP == NULL) {
      Werrprintf( "%s:  '%s' tree doesn't exist", argv[ 0 ], curtree );
      ABORT;
   }
   type = 1; /* write type */
   if ( ( (argc == 4) && !strcmp(argv[3],"add") ) ||
        ( (argc == 5) && !strcmp(argv[4],"add") ) )
   {
      type = 3; /* add type */
   }
   else if ( ( (argc == 4) && !strcmp(argv[3],"replace") ) ||
             ( (argc == 5) && !strcmp(argv[4],"replace") ) )
   {
      type = 2; /* replace type */
   }
   if (type != 1)
   {
      int doRead = 1;
      if (access(argv[1], F_OK))  
      {
         if (type == 3)
            doRead = 0;                 /* file does not exist. Don't do P_read */
         else
         {
            Werrprintf( "%s:  file '%s' must exist for replace option",
                        argv[0], argv[1]);
            ABORT;
         }
      }
      P_treereset(TEMPORARY);		/* clear the tree first */
      if (doRead && P_read(TEMPORARY,argv[1]) )
      {  Werrprintf("cannot read parameters from %s",argv[1]);
         P_treereset(TEMPORARY);
	 RETURN;
      }
   }
   pParList = argv[2];
   if ((type == 2) && !strcmp(argv[2],""))
   {
      struct stat buf;

      stat(argv[1], &buf);
      if (buf.st_size > 1024)
      {
         pParList = (char *)allocateWithId(buf.st_size,"wpar");
      }
      else
      {
         pParList = parList;
      }
      /* This function returns a space separated list of all the parameters
         in the tree.  The second argument must be large enough to hold the answer.
       */
      P_listnames(TEMPORARY, pParList);
   }
   if ((type == 3) || (type == 2))
   {
      int tree;

      tree = getTreeIndex(curtree);
      pnum = getParNameInit(pParList);
      pindex = 0;
      while (pindex < pnum)
      {
         pindex++;
         currentPar = getParName(pParList, par, MAXSTR-1, pindex);
         v = rfindVar( currentPar, sWriteP );
         if (v != NULL) {
            if (type == 3)
            {
               P_copyvar(tree,TEMPORARY,currentPar,currentPar);
            }
            else
            {
               /* copy into temporary only if it is already present */
               if ((v = P_getVarInfoAddr(TEMPORARY,currentPar)) != (varInfo *) -1)
                  P_copyvar(tree,TEMPORARY,currentPar,currentPar);
            }
         }
      }
      P_save(TEMPORARY, argv[1]);
      P_treereset(TEMPORARY);
      releaseWithId("wpar");
   }
   else if (type == 1)
   {
      stream = fopen( argv[ 1 ], "w" );
      if (stream == NULL) {
         Werrprintf( "%s: can't open %s", argv[ 0 ], argv[ 1 ] );
         ABORT;
      }
      pnum = getParNameInit(argv[2]);
      pindex = 0;
      while (pindex < pnum)
      {
         pindex++;
         currentPar = getParName(argv[2], par, MAXSTR-1, pindex);
         v = rfindVar( currentPar, sWriteP );
         if (v != NULL) {
            storeOnDisk( currentPar, v, stream );
         }
      }
      fclose( stream );
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	fsave
|
|	This procedure write variables to a file and from
|	a tree.  Default tree is current.
|	Usage -- fsave(filename[,tree])
|
+-----------------------------------------------------------------------------*/

int fsave(int argc, char *argv[], int retc, char *retv[])
{   FILE     *stream;
    symbol  **s;
    char     *tmpname;

   (void) retc;
   (void) retv;
    if (argc == 2) {
        s = getTreeRoot("current");
    } else if (argc == 3) {
        if((s = getTreeRoot(argv[2])) == NULL) {
            Werrprintf("fsave: \"%s\" no such tree.",argv[2]);
            ABORT;
        }
    } else {
        Werrprintf("Usage -- fsave(filename[,tree])");
        ABORT;
    }

    if ( (argv[1] == NULL)  || ((int) strlen(argv[1]) < 1) ) {
        Werrprintf("fsave:  Bad filename");
        ABORT;
    }

    /* We write first to a unique tmp file, then rename it as requested.
     * This is to avoid clashes with 2 processes writing same file at same time.
     */
    if ( (stream = fopen(argv[1],"w")) ) {
        /* We will be able to write to final file */
        fclose(stream);
    } else {
        Werrprintf("fsave: Can not write file \"%s\"",argv[1]);
        ABORT;
    }

    tmpname = (char *)malloc(20 + strlen(argv[1]));
    sprintf(tmpname,"%s%d", argv[1], getpid());
    if ( (stream = fopen(tmpname,"w")) ) {
        TPRINT1("fsave:  opened file \"%s\"\n",tmpname);
        copytodisk(s,stream); 
        fclose(stream);
        rename(tmpname, argv[1]);
        free(tmpname);
        RETURN;
    } else {
        /* Cannot write tmp file; write directly to final file */
        free(tmpname);
        if ( (stream = fopen(argv[1],"w")) ) {
            TPRINT1("fsave:  opened file \"%s\"\n",argv[1]);
            copytodisk(s,stream); 
            fclose(stream);
            RETURN;
        } else {
            /* Really don't expect this */
            Werrprintf("fsave: Can not write file \"%s\"!!",argv[1]);
            ABORT;
        }
    }
}

static int dataStation = 1;

int is_datastation()
{
   return(dataStation);
}

void check_datastation()
{
   char        sysname[128];  

   if (P_getstring(SYSTEMGLOBAL,"system",sysname,1,128))
      dataStation = 1;
   else if (sysname[0] == 'd')
      dataStation = 1;
   else
   {
      char filename[MAXPATH];
      char owner[MAXPATH];
      FILE *fd;

      dataStation = 0;
      if (!Bnmr)
      {
         sprintf(filename,"%s/acqqueue/acqreserve", systemdir);
         if ( (fd = fopen(filename, "r")) )
         {
            if ((fscanf(fd,"%s",owner)) != EOF)
            {
               if ( strcmp(owner,UserName) )
               {
                  dataStation = 1;
#ifdef VNMRJ
                  if (VnmrJViewId == 1)
#endif 
                     Werrprintf("Acquisition system reserved by %s", owner);
                  P_setstring(SYSTEMGLOBAL,"system","datastation",1);
                  appendvarlist("datastation");
               }
            }
            fclose(fd);
         }
      }
   }
   set_datastation(dataStation);
}

/*------------------------------------------------------------------------------
|
|	f_read
|
|	This procedure reads in variables from a file and loads
|	them in the current tree.
|	Usage -- fread(filename[,tree])
|
+-----------------------------------------------------------------------------*/

int f_read(int argc, char *argv[], int retc, char *retv[])
{   FILE     *stream;
    int  sys_global = 0;
    int  reset_tree = 0;
    int  value_only = 0;
    int  new_only = 0;
    char *tree;
    int   tIndex;

   (void) retc;
   (void) retv;
    tree = "current";
    if ((argc == 3) || (argc == 4) )
    {
       tree = argv[2];
       sys_global = (strcmp(argv[2],"systemglobal") == 0);
       if ( (argc == 4) && (strcmp(argv[3],"reset") == 0) )
          reset_tree = 1;
       if ( (argc == 4) && (strcmp(argv[3],"value") == 0) )
          value_only = 1;
       if ( (argc == 4) && (strcmp(argv[3],"newonly") == 0) )
          new_only = 1;
    }
    else if (argc != 2)
    {   Werrprintf("Usage -- fread(filename[,tree])");
	ABORT;
    }

    if ((tIndex = getTreeIndex(tree)) < 0)
    {	Werrprintf("fread: \"%s\" no such tree.",tree);
	ABORT;
    }
    /* clear the usertree */
    if ( (argc == 3) && ! strcmp(argv[2],"usertree") && (strlen(argv[1]) < 1) )
    {
        P_treereset(tIndex); /* clear tree */
        RETURN;
    }
    if ( (argv[1] == NULL)  || ((int) strlen(argv[1]) < 1) )
    {	Werrprintf("fread:  Bad filename");
	ABORT;
    }
    if ( (stream = fopen(argv[1],"r")) )
    {	int ret;
        char saveOperator[MAXSTR] = "";


	TPRINT1("fread:  opened file \"%s\"\n",argv[1]);
        if  (strcmp(tree,"global") == 0)
        {
           P_getstring(GLOBAL,"operator",saveOperator,1,MAXSTR-1);
        }
        if (reset_tree)
        {
           P_treereset(tIndex); /* clear tree */
        }
        if (value_only)
	   ret = readvaluesfromdisk(getTreeRootByIndex(tIndex),stream); 
        else if (new_only)
	   ret = readNewVarsfromdisk(getTreeRootByIndex(tIndex),stream); 
        else
	   ret = readfromdisk(getTreeRootByIndex(tIndex),stream); 
	fclose(stream);
        if  (strcmp(tree,"global") == 0)
        {
           setGlobalPars();
           P_setstring(GLOBAL,"operator",saveOperator,1);
        }
        if (sys_global)
          check_datastation();
	RETURN;
    }
    else
    {	Werrprintf("fread: File \"%s\" does not exist",argv[1]);
	ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|    getdgroup/4        getlimit/4      gettype/4
|
|    The Dgroup is an integer whose use is defined by an application.
|    Parameter limits are defined as max, min values, and minimum step size
|    Parameter type (or subtype) is real, string, delay, flag, frequency,
|       pulse, integer (parameter basic type is real or string)
|
|    Usage: getdgroup(name<,tree>)<:$value> - get parameter Dgroup
|       getlimit(name<,tree>)<:$max<,$min<,$step>>> - get parameter limits
|       gettype(name<,tree>)<:$value> - get parameter subtype
|
+-----------------------------------------------------------------------------*/
int gettype(int argc, char *argv[], int retc, char *retv[])
{
	char *tree = NULL;
	symbol **root;
	varInfo *v;

	switch (argc)
	{
	  case 1:
	    if (strcmp(argv[0],"getlimit")==0)
	      Werrprintf("Usage -- %s(parameter<,tree>)<:$max<,$min<,$step>>>",argv[0]);
	    else
	      Werrprintf("Usage -- %s(parameter<,tree>)<:$value>",argv[0]);
	    ABORT;
	    break;
	  case 2:
/*	    tree = "current"; */
	    break;
	  case 3: default:
	    tree = argv[2];
	    break;
	}
	if (tree) {
	    if ((root = getTreeRoot(tree)) == NULL)
	    {
	        Werrprintf("%s: '%s' bad tree name",argv[0],tree);
	        ABORT;
	    }
	} else {
	    int i, ntrees;
	    int itree[] = {CURRENT, GLOBAL, SYSTEMGLOBAL};
	    ntrees = sizeof(itree) / sizeof(int);
	    /* No tree specified; look in all trees (current first) */
	    for (i=0; i<ntrees; i++) {
		root = getTreeRootByIndex(itree[i]);
		if ( (v = rfindVar(argv[1],root)) ) {
		    break;
		}
	    }
	    /* Var not found: will fail again immediately below */
	}
	if ( (v = rfindVar(argv[1],root)) )
	{
	  if (strcmp(argv[0],"getlimit")==0)
	  {
	    double maxv,minv,stepv;
	    if (v->prot & P_MMS)
	    {
	      if (P_getreal( SYSTEMGLOBAL, "parmax", &maxv, (int)(v->maxVal+0.1) ))
	        maxv = 1.0e+30;
	      if (P_getreal( SYSTEMGLOBAL, "parmin", &minv, (int)(v->minVal+0.1) ))
	        minv = -1.0e+30;
	      if (P_getreal( SYSTEMGLOBAL, "parstep", &stepv, (int)(v->step+0.1) ))
	        stepv = 0.0;
	    }
	    else
	    {
	      maxv = v->maxVal;
	      minv = v->minVal;
	      stepv = v->step;
	    }
	    switch (retc)
	    {
	      case 0: default:
	        if (v->prot & P_MMS)
		   Winfoprintf("parameter '%s': max=%g min=%g step=%g (indexed to %d)\n",
		     argv[1],maxv,minv,stepv,(int)(v->maxVal+0.1));
                else
		   Winfoprintf("parameter '%s': max=%g min=%g step=%g\n",
		     argv[1],maxv,minv,stepv);
		break;
	      case 1:
		retv[0] = (char *)realString(maxv);
		break;
	      case 2:
		retv[0] = (char *)realString(maxv);
		retv[1] = (char *)realString(minv);
		break;
	      case 3:
		retv[0] = (char *)realString(maxv);
		retv[1] = (char *)realString(minv);
		retv[2] = (char *)realString(stepv);
		break;
	      case 4:
		retv[0] = (char *)realString(maxv);
		retv[1] = (char *)realString(minv);
		retv[2] = (char *)realString(stepv);
	        if (v->prot & P_MMS)
	 	   retv[3] = (char *)realString(v->maxVal);
                else
	 	   retv[3] = (char *)realString(0.0);
		break;
	    }
	    RETURN;
	  }
	  else if (strcmp(argv[0],"getdgroup")==0)
	  {
	    if (retc > 0)
	      retv[0] = (char *)intString(v->Dgroup);
	    else
	      Winfoprintf("parameter '%s': dgroup = %d\n",argv[1],v->Dgroup);
	    RETURN;
	  }
	  else /* argv[0] is gettype */
	  {
	    if (retc > 0)
            {
	      retv[0] = (char *)intString(v->subtype);
	      if (retc > 1)
	        retv[1] = newString(whatType(v->subtype));
            }
	    else
	    {
               Winfoprintf("parameter '%s': %s (%d)\n",argv[1],
                   whatType(v->subtype),v->subtype);
	    }
	    RETURN;
	  }
	}
	else if (strcmp(argv[0],"gettype")==0)
        {
	    if (retc > 0)
            {
	      retv[0] = intString(ST_UNDEF);
	      if (retc > 1)
	        retv[1] = newString(whatType(ST_UNDEF));
            }
	    else
	    {
               Winfoprintf("parameter '%s': %s (%d)\n",argv[1],
                   whatType(ST_UNDEF),ST_UNDEF);
	    }
	    RETURN;
        }
	else
	{
	  Werrprintf("parameter '%s' doesn't exist in '%s' tree",argv[1],tree);
	  ABORT;
	}
}

/*------------------------------------------------------------------------------
|
|	setdgroup/4
|
|	This command sets the dgroup of a variable.  The Dgroup is an
|	integer whose use is defined by an application.
|	Usage  setgroup(name,value[,tree])
|       For multi-Viewport support, if the tree is global,
|       the command is sent to other viewports with an additional 
|       argument as a signal to not resend to other viewports.
|
+-----------------------------------------------------------------------------*/
int setdgroup(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;

   (void) retc;
   (void) retv;
#ifdef DEBUG
    if (Tflag)
    {	int i;
	
	for (i=0; i<argc ;i++)
	    TPRINT2("setdgroup: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 
    switch (argc)
    { case 3:	tree = "current";
		break;
      case 4:	
      case 5:	
		tree = argv[3];
	    	break;
      default:	Werrprintf("Usage -- setdgroup(name,value[,tree])");
		ABORT;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("setdgroup:  \"%s\"  bad tree name",tree);
	ABORT;
    }
    if ( (v = rfindVar(argv[1],root)) ) 
    {

        if (isReal(argv[2]))
        {
           v->Dgroup = (int) stringReal(argv[2]);
#ifdef VNMRJ
           if ((argc == 4) && (strcmp(tree,"global") == 0))
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
                       sprintf(msg,"VP %d %s('%s','%s','global','vp')\n",index,argv[0],
                                    argv[1],argv[2]);
                       writelineToVnmrJ("vnmrjcmd",msg);
                    }
                 }
              }
           }
           else
#endif 
              if (argc != 5)
                 appendvarlist(argv[1]);
	   RETURN;
        }
	else
	{   Werrprintf("setdgroup: second argument (%s) must be an integer",argv[2]);
	    ABORT;
	}
    }
    else
    {	Werrprintf("variable \"%s\" doesn't exist",argv[1]);
	ABORT; /* variable doesn't exist */
    }
}

/*------------------------------------------------------------------------------
|
|	setgroup/4
|
|	This command sets the group of a variable.  The possible groups
|	are  all, sample, acquisition, processing, display, and spin.
|	Usage  setgroup(name,group[,tree])
|       For multi-Viewport support, if the tree is global,
|       the command is sent to other viewports with an additional 
|       argument as a signal to not resend to other viewports.
|
+-----------------------------------------------------------------------------*/
int setgroup(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;

   (void) retc;
   (void) retv;
#ifdef DEBUG
    if (Tflag)
    {	int i;
	
	for (i=0; i<argc ;i++)
	    TPRINT2("setgroup: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 
    switch (argc)
    { case 3:	tree = "current";
		break;
      case 4:	
      case 5:	
		tree = argv[3];
	    	break;
      default:	Werrprintf("Usage -- setgroup(name,group[,tree])");
		ABORT;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("setgroup:  \"%s\"  bad tree name",tree);
	ABORT;
    }
    if ( (v = rfindVar(argv[1],root)) )
    {   int groupIndex;

        if ((groupIndex = goodGroup(argv[2])) >= 0)
	{   v->Ggroup = groupIndex;
#ifdef VNMRJ
            if ((argc == 4) && (strcmp(tree,"global") == 0))
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
                        sprintf(msg,"VP %d %s('%s','%s','global','vp')\n",index,argv[0],
                                     argv[1],argv[2]);
                        writelineToVnmrJ("vnmrjcmd",msg);
                     }
                  }
               }
            }
            else
#endif 
               if (argc != 5)
	          appendvarlist(argv[1]);
	    RETURN;
	}
	else
	{   Werrprintf("setgroup: '%s' bad group name",argv[2]);
	    ABORT;
	}
    }
    else
    {	Werrprintf("variable \"%s\" doesn't exist",argv[1]);
	ABORT; /* variable doesn't exist */
    }
}

static void clearProtectBits(symbol **root, int bits)
{
   symbol  *p;
   varInfo *v;

   if ( (p=(*root)) )
   {
      if (p->name)
      {
        if ( (v= (varInfo *) (p->val)) )
          v->prot &= ~bits;
      }
      if (p->left)
         clearProtectBits( &(p->left), bits);
      if (p->right)
         clearProtectBits( &(p->right), bits);
   }
}

static char *listString;

static void listProtectBits(symbol **root, int bits, char *tmp)
{
   symbol  *p;
   varInfo *v;

   if ( (p=(*root)) )
   {
      if (p->name)
      {
        if ( (v= (varInfo *) (p->val)) )
        {
           if ((v->prot & bits) == bits)
           {
              sprintf(tmp,"%s ",p->name);
              listString = newCatId(listString,tmp,"sp");
           }
        }
      }
      if (p->left)
         listProtectBits( &(p->left), bits, tmp);
      if (p->right)
         listProtectBits( &(p->right), bits, tmp);
   }

}

static void alistProtectBits(symbol **root, int bits, varInfo *v, int *count)
{
   symbol  *p;
   varInfo *t;

   if ( (p=(*root)) )
   {
      if (p->name)
      {
        if ( (t= (varInfo *) (p->val)) )
        {
           if ((t->prot & bits) == bits)
           {
              *count += 1;
              if (*count == 1)
                 assignString(p->name,v,0);
              else
                 assignString(p->name,v,*count);
           }
        }
      }
      if (p->left)
         alistProtectBits( &(p->left), bits, v, count);
      if (p->right)
         alistProtectBits( &(p->right), bits, v, count);
   }
}

/*------------------------------------------------------------------------------
|
|	setprotect
|
|	This command sets the protection bits of a variable.
|	Usage  setprotect(name,set | on | off,bits[,tree])
|       For multi-Viewport support, if the tree is global,
|       the command is sent to other viewports with an additional
|       argument as a signal to not resend to other viewports.
|
+-----------------------------------------------------------------------------*/

int setprotect(int argc, char *argv[], int retc, char *retv[])
{   const char    *tree;
    symbol **root;
    varInfo *v;
    int      protect=0;
    char    *currentPar;
    char    par[MAXSTR];
    int     pnum;
    int     pindex;

#ifdef DEBUG
    int i;
 
    if (Tflag)
      for (i=0; i<argc ;i++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],i,argv[i]);
#endif 
    if (argc < 3)
    {
      Werrprintf("Usage -- %s(name,set/on/off/list/alist/clear/getval/ison,bits[,tree])",argv[0]);
      ABORT;
    }
    if (strcmp(argv[2],"set") && strcmp(argv[2],"on") && strcmp(argv[2],"off") &&
        strcmp(argv[2],"list") && strcmp(argv[2],"clear") && strcmp(argv[2],"getval") &&
        strcmp(argv[2],"ison") && strncmp(argv[2],"alist ",6) )
    {
      Werrprintf("Usage -- %s(name,set/on/off/list/alist/clear/getval/ison,bits[,tree])",argv[0]);
      ABORT;
    }
    if (strcmp(argv[2],"getval"))
    {
       if (argc >= 4)
          protect = (int) stringReal(argv[3]);
       tree = (argc >= 5) ? argv[4] : "current";
       if ((root = getTreeRoot(tree)) == NULL)
       {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
          ABORT;
       }
    }
    else  // getval has optional bit argument
    {
       tree = "current";
       if (argc == 5)
          tree = argv[4];
       else if ( (argc == 4) && !isReal(argv[3]) )
          tree = argv[3];
       if ((root = getTreeRoot(tree)) == NULL)
       {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
          ABORT;
       }
    }
    if (!strcmp(argv[2],"clear"))
    {
       clearProtectBits(root, protect);
       RETURN;
    }
    else if (!strcmp(argv[2],"list"))
    {
       char  tmp[128];
       listString = newStringId("","sp");
       listProtectBits(root, protect, tmp);
       if (retc)
          retv[0] = newString(listString);
       else
          Winfoprintf("Parameters with bits %d set are %s",protect,listString);
       releaseWithId("sp");
       RETURN;
    }
    else if (!strncmp(argv[2],"alist ",6))
    {
       int i, j;
       int count = 0;
       char parName[MAXSTR];
       symbol **vroot;
       varInfo *v;
       int ch;

       i = 5;
       j = 0;
       while ((ch = argv[2][i]) != '\0' && (ch == ' ') )
          i++;
       while ((ch = argv[2][i]) != '\0' && (ch != ' ') )
       {
          parName[j++] = ch;
          i++;
       }
       parName[j] = '\0';
       if ((vroot=selectVarTree(parName)) == NULL)
       {
          Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],parName);
          ABORT;
       }
       if ((v = rfindVar(parName,vroot)) == NULL)
       {   Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],parName);
           ABORT;
       }
       if (v->T.basicType != T_STRING)
       {   Werrprintf("%s: variable \"%s\" must be a string type",argv[0],parName);
           ABORT;
       }
       alistProtectBits(root, protect, v, &count);
       if (retc)
          retv[0] = intString(count);
       else
          Winfoprintf("%d parameters have bits %d set",count,protect);
       RETURN;
    }
    else if (!strcmp(argv[2],"ison"))
    {
       int res;
       if ((v = rfindVar(argv[1],root)) == NULL)
       {
          res = -1;
          if (!retc)
             Winfoprintf("%s: variable \"%s\" does not exist",argv[0],argv[1]);
       }
       else
       {
          res =  ((v->prot & protect) == protect);
          if (!retc)
             Winfoprintf("%s: variable \"%s\" does%s have protection %d set",argv[0],
                     argv[1], (res) ? "" : " not", protect);
       }
       if (retc)
          retv[0] = intString(res);
       RETURN;
    }
    else if (!strcmp(argv[2],"getval"))
    {
       int res;
       if ((v = rfindVar(argv[1],root)) == NULL)
       {
          res = 0;
          if (!retc)
             Winfoprintf("%s: variable \"%s\" does not exist",argv[0],argv[1]);
       }
       else
       {
          res =  v->prot;
          if (!retc)
             Winfoprintf("%s: variable \"%s\" has protection value %d",argv[0],
                     argv[1], res);
       }
       if (retc)
          retv[0] = intString(res);
       RETURN;
    }
    pnum = getParNameInit(argv[1]);
    pindex = 0;
    while (pindex < pnum)
    {
       pindex++;
       currentPar = getParName(argv[1], par, MAXSTR-1, pindex);
       if ((v = rfindVar(currentPar,root)) == NULL)
       {   Werrprintf("%s: variable \"%s\" does not exist",argv[0],currentPar);
	   ABORT;
       }
       if (v->prot & P_PRO)
       {   Werrprintf("protection setting for %s is disallowed",currentPar);
	   ABORT;
       }
       else
       {
           if (strcmp(argv[2],"set") == 0)
             v->prot = protect;
           else if (strcmp(argv[2],"on") == 0)
             v->prot |= protect;
           else
             v->prot &= ~protect;
#ifdef VNMRJ
           if ((argc == 5) && (strcmp(tree,"global") == 0))
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
                       sprintf(msg,"VP %d %s('%s','%s','%s','global','vp')\n",index,argv[0],
                               argv[1],argv[2],argv[3]);
                       writelineToVnmrJ("vnmrjcmd",msg);
                    }
                 }
              }
           }
           else
#endif 
              if ((argc != 6) && (pnum == 1))
	         appendvarlist(argv[1]);
        }
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	settype
|
|	This command sets the subtype of a variable 
|	Usage  settype(name,type[,tree])
|       types are the same as used by create()
|       For multi-Viewport support, if the tree is global,
|       the command is sent to other viewports with an additional 
|       argument as a signal to not resend to other viewports.
|
+-----------------------------------------------------------------------------*/

int settype(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;
    int      index;
    char    *currentPar;
    char    par[MAXSTR];
    int     pnum;
    int     pindex;

   (void) retc;
   (void) retv;
#ifdef DEBUG
    if (Tflag)
      for (index=0; index<argc ;index++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],index,argv[index]);
#endif 
    if ((argc < 3) || (argc > 5))
    {
      Werrprintf("Usage -- %s(name,type[,tree])",argv[0]);
      ABORT;
    }
    index = 1;
    tree = "current";
    if (argc >= 4)
    {
       tree = argv[3];
    }
    if (!(index = goodType(argv[2])) )
    {
         Werrprintf("Bad type %s for %s",argv[2],argv[0]);
         ABORT;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
	ABORT;
    }
    pnum = getParNameInit(argv[1]);
    pindex = 0;
    while (pindex < pnum)
    {
       pindex++;
       currentPar = getParName(argv[1], par, MAXSTR-1, pindex);
    if ((v = rfindVar(currentPar,root)) == NULL)
    {	if (retc == 0)
           Werrprintf("%s: variable \"%s\" doesn't exist in %s tree",
                    argv[0],currentPar,tree);
	continue;
    }
    else if (v->T.basicType == T_STRING)
    {
       if ((index == ST_FLAG) || (index == ST_STRING))
       {
          v->subtype = index;
       }
       else
       {
           Werrprintf("%s: string variable '%s' can only be set to type 'flag' or 'string'",argv[0], currentPar);
           ABORT;
       }
    }
    else
    {
       if ((index != ST_FLAG) && (index != ST_STRING))
       {
          v->subtype = index;
       }
       else
       {
           Werrprintf("%s: real variable '%s' can not be set to type 'flag' or 'string'",argv[0], currentPar);
           ABORT;
       }
    }
#ifdef VNMRJ
    if ((argc == 4) && (strcmp(tree,"global") == 0))
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
                sprintf(msg,"VP %d %s('%s','%s','global','vp')\n",index,argv[0],
                        currentPar,argv[2]);
                writelineToVnmrJ("vnmrjcmd",msg);
             }
          }
       }
    }
    else
#endif 
       if (argc != 5)
	  appendvarlist(currentPar);

    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	setvalue
|
|	This command sets the value of a variable 
|	Usage  setvalue(name,value[,index][,tree])
|       Use of this procedure bypasses the normal
|       range checking
|
+-----------------------------------------------------------------------------*/

int setvalue(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;
    int      index;

   (void) retv;
#ifdef DEBUG
    if (Tflag)
      for (index=0; index<argc ;index++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],index,argv[index]);
#endif 
    if ((argc < 3) || (argc > 6))
    {
      Werrprintf("Usage -- %s(name,value[,index][,tree])",argv[0]);
      ABORT;
    }
    index = 1;
    tree = "current";
    if (argc >= 5)
    {
       tree = argv[4];
       if (isReal(argv[3]))
       {
          index = (int) stringReal(argv[3]);
       }
       else
       {
         Werrprintf("Usage -- %s(name,value[,index][,tree])",argv[0]);
         ABORT;
       }
    }
    else if ((argc >= 4) && isReal(argv[3]))
    {
       index = (int) stringReal(argv[3]);
    }
    else if (argc >= 4)
    {
       tree = argv[3];
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
	ABORT;
    }
    if ((v = rfindVar(argv[1],root)) == NULL)
    {	
        /* return argument avoids error message */
        if (retc)
        {
           RETURN;
        }
        else
        {
           Werrprintf("%s: variable \"%s\" doesn't exist in %s",
                          argv[0],argv[1],tree);
	   ABORT;
        }
    }
    else if (v->T.basicType == T_STRING)
    {
       P_setstring(getTreeIndex(tree),argv[1],argv[2],index);
    }
    else
    {
       double val;

       if (isReal(argv[2]))
       {
          val = stringReal(argv[2]);
          P_setreal(getTreeIndex(tree),argv[1],val,index);
       }
       else
       {
         Werrprintf("Can't assign STRING value to REAL variable");
         ABORT;
       }
    }
#ifdef VNMRJ
    if ( !(v->prot & P_GLO) && strcmp(argv[argc-1],"vp") &&
         ((strcmp(tree,"global") == 0) || (strcmp(tree,"systemglobal") == 0) ) )
    {
       int vpindex;
       int num;
       int quoteType = 0;
       double dval;
       char msg[MAXSTR];

       if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
       {
          num = (int) (dval+0.1);
          if (v->T.basicType == T_STRING)
          {
             if (strchr(argv[2],'\'') != NULL)
                quoteType = 1;
          }
          for (vpindex=1; vpindex <= num; vpindex++)
          {
             if (vpindex != VnmrJViewId)
             {
                if (v->T.basicType == T_STRING)
                {
                   if (quoteType)  /* use backquote around value */
                      sprintf(msg,"VP %d %s('%s',`%s`,%d,'%s','vp')\n",
                        vpindex,argv[0],argv[1],argv[2],index,tree);
                   else
                      sprintf(msg,"VP %d %s('%s','%s',%d,'%s','vp')\n",
                        vpindex,argv[0],argv[1],argv[2],index,tree);
                }
                else
                {
                   sprintf(msg,"VP %d %s('%s','%s',%d,'%s','vp')\n",
                        vpindex,argv[0],argv[1],argv[2],index,tree);
                }
                writelineToVnmrJ("vnmrjcmd",msg);
             }
          }
       }
    }
#endif 
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	getvalue
|
|	This command gets the value of a variable 
|	Usage  getvalue(name[,index][,tree])
|	Usage  getvalue(name,'size'[,tree])
|       Use of this procedure bypasses the normal
|       range checking
|
+-----------------------------------------------------------------------------*/

int getvalue(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;
    int      index;
    int      doSize=0;

#ifdef DEBUG
    if (Tflag)
      for (index=0; index<argc ;index++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],index,argv[index]);
#endif 
    if ((argc < 2) || (argc > 4))
    {
      Werrprintf("Usage -- %s(name[,index][,tree]) or %s(name,'size'[,tree])",
                 argv[0], argv[0]);
      ABORT;
    }
    index = 1;
    tree = "processed";
    if (argc == 4)
    {
       tree = argv[3];
       if (isReal(argv[2]))
       {
          index = (int) stringReal(argv[2]);
       }
       else if ( ! strcmp(argv[2],"size"))
       {
          doSize = 1;
       }
       else
       {
         Werrprintf("Usage -- %s(name[,index][,tree]) or %s(name,'size'[,tree])",
                 argv[0], argv[0]);
         ABORT;
       }
    }
    else if ((argc == 3) && isReal(argv[2]))
    {
       index = (int) stringReal(argv[2]);
    }
    else if ((argc == 3) && ! strcmp(argv[2],"size"))
    {
       doSize = 1;
    }
    else if (argc == 3)
    {
       tree = argv[2];
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
	ABORT;
    }
    if ((v = rfindVar(argv[1],root)) == NULL)
    {
       if (doSize)
       {
          if (retc>0)
             retv[0] = intString( 0 );
          else
             Winfoprintf("%s: %s does not exist",argv[0], argv[1]);
          RETURN;
       }
       else
       {
          Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[1]);
          ABORT;
       }
    }
    if (doSize)
    {
       if (retc>0)
          retv[0] = intString( v->T.size );
       else
          Winfoprintf("%s has %d elements",argv[1],v->T.size);
    }
    else if (v->T.basicType == T_STRING)
    {
       char tmp[1024];

       if (P_getstring(getTreeIndex(tree),argv[1],tmp,index,1023))
       {
      	  Werrprintf("%s: index %d of variable \"%s\" doesn't exist",
                      argv[0],index,argv[1]);
	  ABORT;
       }
       if (retc>0)
          retv[0] = newString(tmp);
       else if (index>1)
          Winfoprintf("%s[%d] set to %s",argv[1],index,tmp);
       else
          Winfoprintf("%s set to %s",argv[1],tmp);
    }
    else
    {
       double val;

       if (P_getreal(getTreeIndex(tree),argv[1],&val,index))
       {
      	  Werrprintf("%s: index %d of variable \"%s\" doesn't exist",
                      argv[0],index,argv[1]);
	  ABORT;
       }
       if (retc>0)
          retv[0] = realString(val);
       else if (index>1)
          Winfoprintf("%s[%d] set to %g",argv[1],index,val);
       else
          Winfoprintf("%s set to %g",argv[1],val);
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	teststr
|
|	This command tests the values of a string parameter
|	to see if any match the passed argument.
|       The matching index is returned, or a zero if none match.
|
+-----------------------------------------------------------------------------*/

int teststr(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;
    int      index;

#ifdef DEBUG
    if (Tflag)
      for (index=0; index<argc ;index++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],index,argv[index]);
#endif 
    if ((argc < 3) || (argc > 4))
    {
      Werrprintf("Usage -- %s(name,key[,tree])",argv[0]);
      ABORT;
    }
    index = 0;
    tree = "current";
    if (argc == 4)
    {
       tree = argv[3];
    }
    if (strcmp(tree,"local") == 0)
    {
       if ((root=selectVarTree(argv[1])) == NULL)
       {
          Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[1]);
	  ABORT;
       }
    }
    else if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
	ABORT;
    }
    if ((v = rfindVar(argv[1],root)) == NULL)
    {	Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[1]);
	ABORT;
    }
    else if (v->T.basicType == T_STRING)
    {
       Rval *r;
       int i;

       r = v->R;
       for (i=1; i <= v->T.size; i++ )
       {
	   if (r)	
	   {
               if (strcmp(argv[2],r->v.s) == 0)   
               {   
	          index = i;
	          i = v->T.size + 1;
               }
	       r = r->next;
	   }
	   else
	   {
	       i = v->T.size + 1;
	   }
       }
    }
    else
    {
      	Werrprintf("%s: can only be used on string parameters",argv[0]);
	ABORT;
    }
    if (retc>0)
       retv[0] = (char *)intString(index);
    else if (index)
       Winfoprintf("%s matches %s[%d]",argv[2],argv[1],index);
    else
       Winfoprintf("%s does not match any values of %s",argv[2],argv[1]);
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	setenumeral
|
|	This command sets the enumeral types of a string variable 
|	Usage  setenumeral(name,N,enum1,enum2,...,enumN[,tree])
|
+-----------------------------------------------------------------------------*/

int setenumeral(int argc, char *argv[], int retc, char *retv[])
{   const char    *tree;
    symbol **root;
    varInfo *v;
    int      i,num;

   (void) retc;
   (void) retv;
#ifdef DEBUG
    if (Tflag)
      for (i=0; i<argc ;i++)
        TPRINT3("%s: argv[%d] = \"%s\"\n",argv[0],i,argv[i]);
#endif 
    if ((argc < 3) || (!isReal(argv[2])))
    {
      Werrprintf("Usage -- %s(name,N,enum1,enum2,...,enumN[,tree])",argv[0]);
      ABORT;
    }
    num = (int) stringReal(argv[2]);
    if ((num+3 != argc) && (num+4 != argc))
    {
      Werrprintf("Usage -- %s(name,N,enum1,enum2,...,enumN[,tree])",argv[0]);
      ABORT;
    }
    tree = (num+4 == argc) ? argv[argc-1] : "current";
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
	ABORT;
    }
    if ((v = rfindVar(argv[1],root)) == NULL)
    {	Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[1]);
	ABORT;
    }
    if (v->prot & P_ENU)
    {	Werrprintf("enumeral setting for %s is disallowed",argv[1]);
	ABORT;
    }
    else if (v->T.basicType == T_STRING)
    {
        if (v->subtype == ST_FLAG)
           for (i = 0; i < num; i++)
              if (1 != strlen(argv[i+3]))
              {
                 Werrprintf("flag enumerals must be single characters");
                 ABORT;
              }
	if (v->ET.size)
          disposeStringRvals(v->E); /* clear all enumeration values */
	v->ET.size = 0;
	v->E = NULL;
        for (i = 1; i <= num; i++)
      	  assignEString(argv[i+2],v,(i == 1) ? 0 : i);
	appendvarlist(argv[1]);
	RETURN;
    }
    else
    {	Werrprintf("only string variables may have enumerals set");
	ABORT;
    }
}

/*------------------------------------------------------------------------------
|
|	setlimit/4
|
|	This command sets the limits of a variable 
|	Usage  setlimit(name,max,min,step[,tree])
|
+-----------------------------------------------------------------------------*/

int setlimit(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    symbol **root;
    varInfo *v;
    int      indexed;

   (void) retc;
   (void) retv;
#ifdef DEBUG
    if (Tflag)
    {	int i;
	
	for (i=0; i<argc ;i++)
	    TPRINT2("setlimit: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 
    tree = "current";
    switch (argc)
    {
      case 3:	/* default tree value */
                indexed = 1;
		break;

      case 4:	tree = argv[3];
                indexed = 1;
	    	break;

      case 5:	/* default tree value */
                indexed = 0;
		break;

      case 6:	tree = argv[5];
                indexed = 0;
	    	break;

      default:	Werrprintf("Usage -- setlimit(name,max,min,step[,tree]) or setlimit(name,index[,tree])");
		ABORT;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf("setlimit:  \"%s\"  bad tree name",tree);
	ABORT;
    }
    if ( (v = rfindVar(argv[1],root)) ) 
    {
        if (indexed)
        {
           vInfo  info;
           int    index;

           index = (int) ( atof(argv[2]) + 0.5 );
           if (index < 1)
           {
              Werrprintf("setlimit: index must be greater than 1");
              ABORT;
           }
           if (P_getVarInfo(SYSTEMGLOBAL,"parmax",&info))
              info.size = 1;
           if (index > info.size)
           {
              Werrprintf("setlimit: index can not be greater than %d",
                          info.size);
              ABORT;
           }
	   v->step = v->minVal = v->maxVal = (double) index;
           v->prot |= P_MMS;   /* turn on bit to look up max,min,step */
        }
        else
        {
	   v->maxVal = atof(argv[2]);
           v->minVal = atof(argv[3]);
	   v->step   = atof(argv[4]) ;
           v->prot &= ~P_MMS;   /* turn off bit to look up max,min,step */
        }
	appendvarlist(argv[1]);
#ifdef VNMRJ
	jsendParamMaxMin( -1, argv[1], tree );
#endif
	RETURN;
    }
    else
    {	Werrprintf("setlimit: variable \"%s\" doesn't exist",argv[1]);
	ABORT; /* variable doesn't exist */
    }
}

/*------------------------------------------------------------------------------
|
|	on/off/1+
|
|	This command turns on or off a variable and makes it
|	active or inactive.
|
|	Usage  off(name[,tree])
|
+-----------------------------------------------------------------------------*/

int Off(int argc, char *argv[], int retc, char *retv[])
{   char    *tree;
    int     i;
    symbol **root;
    varInfo *v;

#ifdef DEBUG
    if (Tflag)
    {	for (i=0; i<argc ;i++)
	    TPRINT2("Off: argv[%d] = \"%s\"\n",i,argv[i]);
    }
#endif 
    switch (argc)
    { case 2:	tree = "current";
		break;
      case 3:	
		tree = argv[2];
	    	break;
      default:	Werrprintf("Usage -- %s('varname'[,'tree'])",argv[0]);
		if (retc > 0)
		  retv[0] = realString((double)ACT_NOTEXIST);
		RETURN;
    }
    if ((root = getTreeRoot(tree)) == NULL)
    {	Werrprintf(" \"%s\"  bad tree name",tree);
	if (retc > 0)
	  retv[0] = realString((double)ACT_NOTEXIST);
	RETURN;
    }
    if ( (v = rfindVar(argv[1],root)) ) 
    {   if (retc>0)
          { if (retc>1)
              { Werrprintf("Only one value returned in %s",argv[0]);
	 	ABORT;
              }
            i = -1;
            if (strcmp(argv[0],"off") == 0)
              i = (v->active == ACT_OFF);
            else if (strcmp(argv[0],"on") == 0)
              i = (v->active == ACT_ON);
            retv[0] = realString((double)i);
          }
        else
          { if (strcmp(argv[0],"off") == 0)
              v->active = ACT_OFF;
            if (strcmp(argv[0],"on") == 0)
              v->active = ACT_ON;
	    appendvarlist(argv[1]);
          }
	RETURN;
    }
    else
    {	if (retc > 0)
	  retv[0] = realString((double)ACT_NOTEXIST);
	else
	  Werrprintf(" variable \"%s\" doesn't exist in \"%s\" tree",argv[1],tree);
	RETURN; /* variable doesn't exist */
    }
}

/*------------------------------------------------------------------------------
|
|	printon
|
|	This macro creates a temp file (if one does not already exist) and
|       redirects output from the output textsubwindow to this file.
|       This remains in effect until printoff is executed or until the program
|       is exited.
|
+-----------------------------------------------------------------------------*/
char  printpath[MAXPATHL];
int   printFileSeq = 0;
int   printOn      = 0;
FILE *printfile = 0;

int print_on(int argc, char *argv[], int retc, char *retv[])
{
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
   if (!printfile) /* if printfile is not open, open it */
   /* create a unique filename based on pid and sequence number */
   /* place this file in systemdir/tmp  */
   {
#ifdef UNIX
      sprintf(printpath,"%s/tmp/%sPRINT%d%d",
	     systemdir,HostName, ((int) getpid()),printFileSeq++);
#else 
      sprintf(printpath, "%sPRINT%d.%d", userdir, printFileSeq++, ((int) getpid()));
#endif 
      if ( (printfile=fopen(printpath,"w+")) ) /* if successfull */
      {  Winfoprintf("Printing turned on");
         printOn = 1;
	 disp_print("PRINT");
      }
      else
      {  Werrprintf("Unable to open printfile '%s'",printpath);
         ABORT;
      }
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	printoff
|
|	This procedure closes up the printfile and creates a system command
|       to print the output on the current print device.
|
+-----------------------------------------------------------------------------*/
int printoff(int argc, char *argv[], int retc, char *retv[])
{  char        s[MAXPATH];
/*
   extern char PrinterHost[];
   extern char PrinterShared[];
 */
   extern char PrinterName[];
   extern char PrinterType[];
   extern char *OsPrinterName;
   int         toRemove;
   // int         len;
   char        *printer;
   // char        *p;
   // char        data[1024];
   // FILE        *stream;

   (void) retc;
   (void) retv;
   if (OsPrinterName != NULL)
      printer = OsPrinterName;
   else
      printer = PrinterName;

   toRemove = 1;
   if (printfile) /* if it exists */
   {  fclose(printfile);
      printfile = NULL;
      printOn = 0;
      /* create system command to print the printfile */
      /* command structure looks like    vnmrprint filename printername    */
      /* the shell routine plot makes a symbolic link to the printfile */
      /* and deletes it when it is finished with it                   */
      if (argc>1)
      {
         toRemove = 0;
         /* print to a file */
         sprintf(s,"vnmrprint %s \"%s\" \"%s\" %s",
                    printpath,printer,PrinterType,argv[1]);
      }
      else if (!strcmp(PrinterName,"none") || !strcmp(PrinterName,""))
      {
         /* delete the file */
         if (!strcmp(PrinterName,""))
             sprintf(s,"vnmrprint %s", printpath);
         else
             sprintf(s,"vnmrprint %s none none clear", printpath);
      }
      else
      {
         Winfoprintf("Printing turned off");
         sprintf(s,"vnmrprint %s \"%s\" \"%s\"",
                    printpath,printer,PrinterType);
         TPRINT1("printoff:print command '%s'\n",s);
      }
      /********
      else if (!strcmp(PrinterHost,HostName) || (PrinterShared[0] == 'N') ||
          (PrinterShared[0] == 'n'))
      {
         sprintf(s,"vnmrprint %s \"%s\" \"%s\"",
                    printpath,printer,PrinterType);
         TPRINT1("printoff:print command '%s'\n",s);
      }
      else // Remote shared file system,  send a rsh command 
      {  sprintf(s,"rsh %s %s/bin/vnmrprint %s \"%s\" \"%s\"",
            PrinterHost,systemdir,printpath,printer,PrinterType);
         TPRINT1("remote printoff:print command '%s'\n",s);
      }
      ***********/

      // if shell script print or echo some messages before
      //    execute real job, it will be interrupted.
      /****  
      if ((stream = popen_call( s, "r")) != NULL)
      {
         p = fgets_nointr(data,1024,stream);
         pclose_call(stream);
         len = strlen(data);
         if (data[len - 1] == '\n')
            data[len - 1] = '\0';
         Winfoprintf("%s",data);
      }
      ***********/

      exec_shell(s);

      disp_print("     ");

      if (toRemove)
      {
          if (access(argv[1], F_OK))  
          {
             sprintf(s,"rm -f \"%s\"", printpath);
             system(s);
          }
      }

   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	setplotdev
|
|	This macro sets the plotter name.  If there is no argument, it uses the
|       name from the global variables.  It returns the type in retc if 
|       requested.
|
+-----------------------------------------------------------------------------*/
int setplotdev(int argc, char *argv[], int retc, char *retv[])
{  char        plotname[128];  
   extern char PlotterName[];
   extern char PlotterType[];
   extern char PlotterHost[];
   extern double get_ploter_ppmm();
   extern int    get_plotter_raster();

   if (argc == 1) /* no arguments */
   {  if (P_getstring(GLOBAL,"plotter",plotname,1,128))
         strcpy(plotname,"none");
   }
   else
      strcpy(plotname,argv[1]);
   TPRINT1("setplotdev:plotname=%s\n",plotname);
   /* if setting the name fails, set the plotter back to its original name */
   if (!setPlotterName(plotname)) 
      setPlotterName(PlotterName);
   else if (retc == 0)
      Winfoprintf("Plotter set to %s",plotname);
   if (retc >= 1) /* return the type if wanted */
   {
      retv[0] = newString(PlotterType);
      if (retc >= 2) /* return the host if wanted */
      {
         retv[1] = newString(PlotterHost);
         if (retc >= 3) /* return ppmm if wanted */
         {
            retv[2] = (char *)realString( get_ploter_ppmm() );
            if (retc >= 4) /* return raster if wanted */
	       retv[3] = (char *)intString( get_plotter_raster() );
         }
      }
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	setprintdev
|
|	This macro sets the printer name.  If there is no argument, it uses the
|       name from the global variables. It returns the type in the retc if
|       wanted.
|
+-----------------------------------------------------------------------------*/
int setprintdev(int argc, char *argv[], int retc, char *retv[])
{  char        printname[128];  
   extern char PrinterName[];
   extern char PrinterType[];

   if (argc == 1) /* no arguments */
   {  if (P_getstring(GLOBAL,"printer",printname,1,128))
         strcpy(printname,"none");
   }
   else
      strcpy(printname,argv[1]);
   TPRINT1("setprintdev:printname=%s\n",printname);
   /* if setting the name fails, set the printer back to its original name */
   if (!setPrinterName(printname)) 
      setPrinterName(PrinterName);
   else if (retc == 0)
      Winfoprintf("Printer set to %s",printname);
   if (retc >= 1) /* return the type if wanted */
      retv[0] = newString(PrinterType);
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	devicenames
|
|	This procedure prints a list of plotter/printer names and 
|       information from the devicenames file
|
+-----------------------------------------------------------------------------*/
int devicenames(int argc, char *argv[], int retc, char *retv[])
{  char        filepath[MAXPATHL];
   char        Lhost[128];
   char        Lport[128];
   char        Ltype[128];
   char        Lusage[48];
   char       *p;
   char        plotname[128];
   char        s[1024];
   extern char PlotterName[];
   extern char PlotterType[];
   extern char PrinterName[];
   extern char PrinterType[];
   FILE       *namesinfo;
 
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
   /* open the devicenames file */
#ifdef UNIX
   sprintf(filepath,"%s/devicenames",systemdir);
#else 
   sprintf(filepath,"%sdevicenames",systemdir);
#endif 
   if ( (namesinfo=fopen(filepath,"r")) )
   {
      TPRINT1("devicenames: opened file '%s'\n",filepath);
      Wscrprintf("Current Plotter name is '%s'    type is '%s'\n",
		 PlotterName,PlotterType);
      Wscrprintf("Current Printer name is '%s'    type is '%s'\n",
		 PrinterName,PrinterType);
      Wscrprintf("\n");
      Wscrprintf("Plotter Name        Use     Type            Host             Port\n");
      /* find entry in table corresponding to name */
      p = fgets(s,1023,namesinfo);
      while (p)
      {  if (!strncmp(p,"Name",4))/* check if we are at Name */
         {  sscanf(p,"%*s%s",plotname); /* get name from file */
            /* Load in parameters */
            if (!(p = fgets(s,1023,namesinfo)))
            {  Werrprintf("Bad devicenames file");
               fclose(namesinfo);
               ABORT;
            }
            sscanf(p,"%*s%s",Lusage);
            if (!(p = fgets(s,1023,namesinfo)))
            if (!(p = fgets(s,1023,namesinfo)))
            {  Werrprintf("Bad devicenames file");
               fclose(namesinfo);
               ABORT;
            }
            sscanf(p,"%*s%s",Ltype);
            if (!(p = fgets(s,1023,namesinfo)))
            {  Werrprintf("Bad devicenames file");
               fclose(namesinfo);
               ABORT;
            }  
            sscanf(p,"%*s%s",Lhost);
            if (!(p = fgets(s,1023,namesinfo)))
            {  Werrprintf("Bad devicenames file");
               fclose(namesinfo);
               ABORT;
            }  
            sscanf(p,"%*s%s",Lport);
            TPRINT5("devicenames: name=%s use=%s type=%s host=%s port=%s\n",
                      plotname,Lusage,Ltype,Lhost,Lport);
            Wscrprintf("%-19s %-7s %-15s %-16s %-3s\n",
			plotname,Lusage,Ltype,Lhost,Lport);
         }
         p = fgets(s,1023,namesinfo);
      }   
      fclose(namesinfo);
      RETURN;
   }
   else
   {
      TPRINT1("devicenames: trouble opening file '%s'\n",filepath);
      Werrprintf("Could not open devicenames file");
      ABORT;
   }
}

/*------------------------------------------------------------------------------
|
|	clear
|
|	This procedure clears screens, screen 4 (the scrolling one) is default
|	Usage  clear [(screen_num)]
|
+-----------------------------------------------------------------------------*/

int clear(int argc, char *argv[], int retc, char *retv[])
{   
   (void) retc;
   (void) retv;

    if (argc == 1)
    {	Wclear_text();
        Wsettextdisplay("clear");
    }
    else
    {	int screen_num;

        screen_num = atoi(argv[1]);
        Wclear(screen_num);
        if ( (screen_num == 2) || (screen_num == 0)) {
          Wturnoff_buttons();
          Wsetgraphicsdisplay("");
#ifdef VNMRJ
          clearGraphFunc();
#endif
        }
        else if (screen_num == 4)
          Wsettextdisplay("clear");
    }	
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	focus
|
|	This procedure takes the input focus
|
+-----------------------------------------------------------------------------*/
int takeFocus(int argc, char *argv[], int retc, char *retv[])
{   
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
    if (Wissun())
       dgSetTtyFocus();
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	getctype/1
|
|	This routine returns an integer value corresponding to a character
|	subtype.
|
+-----------------------------------------------------------------------------*/

int getctype(char *s)
{   switch(*s)
    { case 'u':	return(ST_UNDEF);
      case 'r':	return(ST_REAL);
      case 's':	return(ST_STRING);
      case 'd':	return(ST_DELAY);
      case 'F':	return(ST_FLAG);
      case 'f':	return(ST_FREQUENCY);
      case 'p':	return(ST_PULSE);
      case 'i':	return(ST_INTEGER);
      default:	return(ST_UNDEF);
    }
}

#ifndef VNMRJ
static char *magicVarList = NULL;
#endif 

void appendMagicVarList()
{
#ifndef VNMRJ
   if (magicVarList)
   {
      appendvarlist(magicVarList);
      release(magicVarList);
      magicVarList = NULL;
   }
#endif 
}

int magicvars(int argc, char *argv[], int retc, char *retv[])
{
#ifndef VNMRJ
   char *ptr, *ptr2;
   char addr[VAL_LENGTH];
   char tmp[VAL_LENGTH];
   int   found, num, i, len;

   if (argc != 4)
   {
      Werrprintf("magicvar requires three arguments");
      ABORT;
   }
   ptr = argv[1];
   len = strlen(ptr);
   num = i = 0;
   while (i<len)
   {
      found = 0;
      while (*ptr == ' ')
      {
         i++;
         ptr++;
      }
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         i++;
         ptr++;
         found = 1;
      }
      if (found)
         num++;
      i++;
      ptr++;
   }
   P_getstring(GLOBAL,"vnmraddr",addr,1, VAL_LENGTH-1);
   getTclInfoFileName(tmp,addr);
   openTclInfo(tmp, num);
   ptr = argv[1];
   if (magicVarList != NULL)
      release(magicVarList);
   magicVarList = newString("");
   for (i=0; i< num; i++)
   {
      varInfo *v;
      double val;

      while (*ptr == ' ')
         ptr++;
      ptr2 = &addr[0];
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         *ptr2++ = *ptr++;
      }
      *ptr2 = '\0';
      ptr++;
      if ((v = P_getVarInfoAddr(CURRENT,addr)) == (varInfo *) -1)
         if ((v = P_getVarInfoAddr(GLOBAL,addr)) == (varInfo *) -1)
            v = P_getVarInfoAddr(SYSTEMGLOBAL,addr);
      if (v != (varInfo *) -1)
         setMagicVar(i, v, addr);
      else
         setMagicVar(i, 0, addr);
      if (i)
         magicVarList = newCatId(magicVarList,"','","appendmagicvar");
      magicVarList = newCatId(magicVarList,addr,"appendmagicvar");
   }
   sendTripleEscToMaster( 'V', argv[2]);
   sendTripleEscToMaster( 'T', argv[3]);
#endif 
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
   RETURN;
}

int tclCmd(int argc, char *argv[], int retc, char *retv[])
{
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
#ifndef VNMRJ
   if (argc != 2)
   {
      Werrprintf("tclcmd requires one argument");
      ABORT;
   }
   if (!Bnmr)
      sendTripleEscToMaster( 'T', argv[1] );
#endif 
   RETURN;
}

int initTclDg(int argc, char *argv[], int retc, char *retv[])
{
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
#ifndef VNMRJ
   if (argc != 2)
   {
      Werrprintf("initTclDg requires port argument");
      ABORT;
   }
   if (!Bnmr)
      sendTripleEscToMaster( 'U', argv[1] );
#endif 
   RETURN;
}


/**************************************************************************
readfile(path, par1, par2, <,cmpstr <,tree> >):num

readfile reads the contents of a file and puts the contents into two
supplied parameters.  The first word on each line in the file is placed in the
first parameter.  The remainder of the line is placed in the second parameter.
An optional fourth argument specifies a string which is used to match the
first word of the line.  For example, if the file contained
H1pw   10
H1pwr  55
C13pw  14
C13pwr 50
and the comparison string was set to H1, only the lines strating with H1
would be put into the parameters.  Namely, H1pw and H1pwr.

Arguments:
 path is the path name of the file to read.
 par1 is the name of the parameter to hold the first word of the line.
 par2 is the name of the parameter to hold the remainder of each line.
 cmpstr is the optional comparison string for matching the first word.
 tree is an optional parameter to select the tree for par1 and par2.
    The possibilities are current, global, and local.  Current is the
    default.  Local is used if the parameters are $ macro parameters.
    If tree is used, the cmpstr must also be supplied.  If cmpstr is
    '', then it is ignored.

The par1 and par2 parameters must already exist. If par1 or par2 are
defined as a real parameter, as opposed to a string parameter, then
if the value does not have a number as the first word, a zero will be
assigned.

num will be set to the number of items in the arrayed params par1 and par2.

Lines which only contain whitspace are not added to the parameters.
Lines which start with a # are not added to the parameters.  Lines
which start with a # can be used as comment lines.
If a line only contains a single word, that word is put into the
first parameter.  The corresponding array element of the second
parameter will be set to an empty string.

The readfile will return the number of lines added to the parameters.

Examples using a prototype file containing the following.

           # A readfile test case

           # Proton values
           H1pw   10
           H1pwr  55

           # Carbon values
           C13pw  14
           C13pwr 50

           H1macro  ft f full aph vsadj
           End

readfile(systemdir+'/probes/testcase','attr','vals')

This sets the attr and vals parameters to arrays of six strings.
attr='H1pw','H1pwr','C13pw','C13pwr','H1macro','End'
vals='10','55','14','50','ft f full aph vsadj',''

readfile(systemdir+'/probes/testcase','attr','vals','H1')

This sets the attr and vals parameters to arrays of three strings.
attr='H1pw','H1pwr','H1macro'
vals='10','55','ft f full aph vsadj'

The readfile command might be used in conjunction with the teststr
command.  The teststr command can be used to search an arrayed
parameter to determine the index of a specified element.
For example,
teststr(attr,'H1pwr'):$e
vals[$e] will be the value of 'H1Pwr'

If the cmpstr is set to JCAMP, then readfile expects the input file to
be a JCAMP file. Lines with # are no longer comment lines. Comments start
with $$. The attr parameter will be set to the Labelled-Data-Record (LDR)
label and the vals parameter will contain the LDR value. LDR's that are
defined between the ## and = (i.e., global LDR) or between ##. and =
(i.e., datatype specific LDR) will be converted according to the JCAMP rules
(all upper case, remove spaces, dashes, slashes, and underlines).
LDR's that are defined between ##$ and = (i.e., private LDR) will be returned
as is. This command only recognizes data sets stored as NTUPLES. It will
store the data as real - imaginary pairs as an ASCII file in the current
experiment with the name jcampData. The jcampData file can be used by the
makefid command to read the data into an experiment.

************************************************************************/



#define JCAMP_GLOBAL    1
#define JCAMP_SPECIFIC  2
#define JCAMP_PRIVATE   3
#define JCAMP_DATA      4
#define JCAMP_ERROR     5

static int *iptrJcamp = NULL;
static int *jptrJcamp = NULL;

static char *getValue(char *buf)
{
   static char tmp[1024];
   int i, j;

   tmp[0] = '\0';
   i = j = 0;
   if ( buf[i] == '=' )
   {
      i++;
      while ( ( buf[i] == ' ' ) || (buf[i] == '\t') )
         i++;
      while ( buf[i] != '\0' )
      {
         tmp[j++] = buf[i++];
      }
   }
   tmp[j] = '\0';
   return(tmp);
}

static int getLDR(char *buf, char *ldr)
{
   int i = 0;
   int j = 0;
   int isCore;

   // skip white space
   ldr[0] = '\0';
   isCore = 1;
   while ( ( buf[i] == ' ' ) || (buf[i] == '\t') )
      i++;
   if ( (buf[i] == '#') && (buf[i+1] == '#') )
   {
      i += 2;
      if (buf[i] == '$')
         isCore = 0;
      if ( (buf[i] == '.') || (buf[i] == '$') )
         i++;
      while ( (buf[i] != '=') && (buf[i] != '\0') )
      {
         if (isCore)
         {
            if ( (buf[i] >= 'a') && (buf[i] <= 'z') )
            {
               ldr[j++] = buf[i] - 'a' + 'A';
            }
            else if ( (buf[i] >= 'A') && (buf[i] <= 'Z') )
            {
               ldr[j++] = buf[i];
            }
            else if ( (buf[i] >= '0') && (buf[i] <= '9') )
            {
               ldr[j++] = buf[i];
            }
         }
         else
         {
            ldr[j++] = buf[i];
         }
         i++;
      }
      ldr[j] = '\0';
      return(i);
   }
   return(-1);
}

static void checkchar(int ch, char *val, int *ycheck, int *dup, int *diff)
{
   *dup = *diff = 0;
   switch (ch)
   {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         sprintf(val,"%c",ch);
         break;
      case '@':
         strcpy(val,"+");
         break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
         sprintf(val,"%c",ch-'A'+'1');
         break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
         sprintf(val,"-%c",ch-'a'+'1');
         break;
      case '%':
         strcpy(val,"+");
         *diff = 1;
         *ycheck = 1;
         break;
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
         sprintf(val,"%c",ch-'J'+'1');
         *diff = 1;
         *ycheck = 1;
         break;
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
         sprintf(val,"-%c",ch-'j'+'1');
         *diff = 1;
         *ycheck = 1;
         break;
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
         sprintf(val,"%c",ch-'S'+'1');
         *dup = 1;
         break;
      case 's':
         strcpy(val,"9");
         *dup = 1;
         break;
   }
}

static int writedata(FILE *oFile, char *buffer, int *xcheck, int *ycheck,
                     int totalPts, int *pts, float factor)
{
   int len;
   int i;
   int j;
   char num[1024];
   static int last = 0;
   int dup;
   int xch;
   int diff;
   int lastnum = 0;
   int lastdiff = 0;
   int doXcheck = 0;

   len = strlen(buffer);
   if ( *xcheck == -1)
      last = 0;

   i = 0;
   // skip white space
   while ( ( buffer[i] == ' ' ) || (buffer[i] == '\t') )
      i++;
   // get xcheck number
   checkchar(buffer[i], num, ycheck, &dup, &diff);
   i++;
   j = strlen(num);
   while ( ((buffer[i] >= '0') && (buffer[i]<= '9')) || (buffer[i] == '.') )

   {
      num[j++] = buffer[i++];
   }
   num[j] = '\0';
   if (doXcheck)
   {
      xch = atoi(num);
      if ( (*xcheck != -1 ) && (xch  != *xcheck))
      {
         Werrprintf("JCAMP xcheck failure");
         return(-1);
      }
   }
   if (*ycheck)
   {
      // skip white space
      while ( ( buffer[i] == ' ' ) || (buffer[i] == '\t') )
         i++;
      // get ycheck number
      checkchar(buffer[i], num, ycheck, &dup, &diff);
      i++;
      j = strlen(num);
      while ((buffer[i] >= '0') && (buffer[i]<= '9') )
      {
         num[j++] = buffer[i++];
      }
      num[j] = '\0';
      xch = atoi(num);
      if (xch != last)
      {
         Werrprintf("JCAMP ycheck failure");
         return(-1);
      }
   }
   while (i < len)
   {
      *xcheck += 1;
      while ( ( buffer[i] == ' ' ) || (buffer[i] == '\t') )
         i++;
      checkchar(buffer[i], num, ycheck, &dup, &diff);
      if (dup)
      {
         i++;
         xch = atoi(num);

         xch--;
         *xcheck += (xch-1);
         while (xch > 0)
         {
            xch--;
            last += lastnum;
            if (factor == 0.0)
            {
               if ( *pts < totalPts)
               {
                  (*pts)++;
                  *iptrJcamp = last;
                  iptrJcamp++;
               }
               else
               {
                  Werrprintf("JCAMP number of points mismatch");
                  return(-1);
               }
            }
            else
            {
               fprintf(oFile,"%-9g %-9g\n", *jptrJcamp * factor, last * factor);
               jptrJcamp++;
            }
         }
      }
      else
      {
      
         i++;
         j = strlen(num);
         while ((buffer[i] >= '0') && (buffer[i]<= '9') )
         {
            num[j++] = buffer[i++];
         }
         num[j] = '\0';
         lastnum = atoi(num);
         lastdiff = diff;
         if (diff)
         {
            xch = lastnum + last;
            last = xch;
         }
         else
         {
            last = xch = lastnum;
         }
         if (factor == 0.0)
         {
            if ( *pts < totalPts)
            {
               (*pts)++;
               *iptrJcamp = last;
               iptrJcamp++;
            }
            else
            {
               Werrprintf("JCAMP number of points mismatch");
               return(-1);
            }
         }
         else
         {
            fprintf(oFile,"%-9g %-9g\n", *jptrJcamp * factor, last * factor);
            jptrJcamp++;
         }
      }

   }
   return(0);
}


static int linetype(char *line)
{
   char *ptr;

   ptr = line;
   while ( ( *ptr == ' ' ) || ( *ptr == '\t' ) )
      ptr++;
   if ( ( *ptr == '#' ) && ( *(ptr+1) == '#') )
   {
      ptr += 2;
      if ( *ptr == '.' )
         return(JCAMP_SPECIFIC);
      if ( *ptr == '$' )
         return(JCAMP_PRIVATE);
      return(JCAMP_GLOBAL);
   }
   return(JCAMP_DATA);
}

static int getlineNoComments(FILE *path, char line[], int limit)
{
  int ch,i;
  int hasComment;
  int done;

  done = 0;
  while ( ! done )
  {
     hasComment = -2;
     ch = line[0] = '\0';
     i = 0;
     while ((i < limit -1) && ((ch = getc(path)) != EOF) && (ch != '\n'))
     {
       line[i++] = ch;
       if ( (hasComment == -2) && (ch == '$') )
       {
          hasComment = -1;
       }
       else if ( (hasComment == -1) && (ch == '$') )
       {
          hasComment = i-1;
       }
       else if (hasComment == -1)
       {
          hasComment = -2;
       }
     }
     line[i] = '\0';
     if (hasComment >= 0)
     {
        i = hasComment - 1;
        while ( (i > 0) && ( (line[i-1] == ' ') || (line[i-1] == '\t') ) )
           i--;
     }
     // check for windows end of line '\r\n'
     while (i && ( (line[i-1] == '\r') || (line[i-1] == '\n')) )
        i--;
     line[i] = '\0';
     if ( (i > 0) || (ch == EOF) )
     {
        done = 1;
     }
  }
  if ((i == 0) && (ch == EOF))
     return(0);
  else
     return(1);
}

static int jcamp(FILE *iFile, varInfo *v1, varInfo *v2)
{
   FILE *oFile = NULL;
   float factor = 0.0;
   int xcheck, ycheck;
   char buffer[2048];
   char outFile[2048];
   int  numRows=0;
   int  numPts=0;
   char ldr[1024];
   int index;
   int error = 0;
   int npx = 0;
   int dataPage = 0;
   int doingData = 0;

   iptrJcamp= NULL;
   jptrJcamp = NULL;

   sprintf(outFile,"%s/jcampData",curexpdir);
   if ( (oFile = fopen(outFile,"w")) == NULL)
   {
      Werrprintf("Cannot open JCAMP data file %s",outFile);
      error = 1;
   }

   xcheck = -1;
   ycheck = 0;
   while ( ! error && getlineNoComments(iFile, buffer, sizeof(buffer)) )
   {
/*
      fprintf(oFile,"%s\n",buffer);
 */
       switch ( linetype(buffer) )
       {
          case JCAMP_GLOBAL:
                xcheck = -1;
                ycheck = 0;
                numPts = 0;
                if (dataPage == 1)
                   dataPage = 2;

                index = getLDR(buffer,ldr);
                if (index > 0)
                {
                   if ( ! strcmp(ldr,"DATACLASS") )
                   {
                      if ( strcmp(getValue(buffer+index),"NTUPLES") )
                      {
                         Werrprintf("Only handles NTUPLES JCAMP data");
                         error = 1;
                      } 
                   }
                   if ( ! strcmp(ldr,"DATATABLE") )
                      doingData = 1;
                   else
                      doingData = 0;
                   if ( ! strcmp(ldr,"VARDIM") )
                      npx = atoi(getValue(buffer+index));
                   if ( ! strcmp(ldr,"FACTOR") )
                   {
                      char *ptr = buffer+index;
                      while ( ( *ptr == ' ' ) || (*ptr == '\t') )
                         ptr++;
                      while ( (*ptr != ',') && (*ptr != '\0') )
                         ptr++;
                      if ( *ptr == ',' )
                      {
                         ptr++;
                         factor = atof(ptr);
                      }
                      else
                      {
                         Werrprintf("Cannot access JCAMP FACTOR parameter");
                         error = 1;
                      }
                   }
                   if ( ! error )
                   {
                      numRows++;
                      assignString(ldr,v1,numRows);
                      assignString(getValue(buffer+index),v2,numRows);
                   }
                }
                break;
          case JCAMP_SPECIFIC:
          case JCAMP_PRIVATE:
                index = getLDR(buffer,ldr);
                if (index > 0)
                {
                   numRows++;
                   assignString(ldr,v1,numRows);
                   assignString(getValue(buffer+index),v2,numRows);
                }
                break;
          case JCAMP_DATA:
                if (doingData)
                {
                   if ( npx == 0)
                   {
                      Werrprintf("Cannot access JCAMP VARDIM parameter");
                      error = 1;
                   }
                   if ( factor == 0.0)
                   {
                      Werrprintf("Cannot access JCAMP FACTOR parameter");
                      error = 1;
                   }
                   if ( ! error )
                   {
                      int ret;
                      if ( iptrJcamp == NULL )
                      {
                         jptrJcamp = iptrJcamp = (int *)allocateWithId(npx * sizeof(int),"jcamp");
                      }
                      if (dataPage == 0)
                         dataPage = 1;
                      if (dataPage == 1)
                         ret = writedata(oFile, buffer, &xcheck, &ycheck,
                                         npx, &numPts, 0.0);
                      else
                         ret = writedata(oFile, buffer, &xcheck, &ycheck,
                                         npx, &numPts, factor);
                      if (ret == -1)
                         error = 1;
                   }
                }
                break;
          default:
                break;
       }
   }
   if (iptrJcamp)
      releaseWithId("jcamp");
   fclose(oFile);
   if (error)
      return(0);
   return(numRows);
}

static int priv_getline(FILE *path, char *line,int limit);

#define LINE_LIMIT 1024
int readfile(int argc, char *argv[], int retc, char *retv[])
{
    char *tree;
    symbol **root;
    varInfo *v1, *v2;
    char line1[LINE_LIMIT], line2[LINE_LIMIT], first[LINE_LIMIT], remain[LINE_LIMIT];
    char c;
    FILE *inputFile;
    int  length, i, numRows=0;
    char *path, *cmpstr=NULL;
    int jcampFlag = 0;

    /* Put args into variables */ 
    /* Must be at least 3 args */
    if(argc < 4) {
        Werrprintf("Usage: readfile(path, par1, par2 <,cmpstr <,tree> >):num");
        ABORT;
    }
    path = argv[1];
    /* Optional 4 arg is for cmpstr */
    if(argc >= 5)
    {
      /* ignore if cmpstr is a null string */
      if (strlen(argv[4]))
      {
        cmpstr = argv[4];
        if ( ! strcmp(cmpstr,"JCAMP"))
           jcampFlag = 1;
      }
    }
    tree = "current";
    if (argc == 6)
    {
       tree = argv[5];
    }
    if (strcmp(tree,"local") == 0)
    {
       if ((root=selectVarTree(argv[2])) == NULL)
       {
          Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[2]);
          ABORT;
       }
       if ((root=selectVarTree(argv[3])) == NULL)
       {
          Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[3]);
          ABORT;
       }
    }
    else if ((root = getTreeRoot(tree)) == NULL)
    {   Werrprintf("%s:  \"%s\"  bad tree name",argv[0],tree);
        ABORT;
    }
    if ((v1 = rfindVar(argv[2],root)) == NULL)
    {   Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[2]);
        ABORT;
    }
    if ((v2 = rfindVar(argv[3],root)) == NULL)
    {   Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[3]);
        ABORT;
    }
    if (jcampFlag)
    {
       if (v1->T.basicType == T_REAL)
       {
          Werrprintf("%s: variable \"%s\" must be string type",argv[0],argv[2]);
          ABORT;
       }
       if (v2->T.basicType == T_REAL)
       {
          Werrprintf("%s: variable \"%s\" must be string type",argv[0],argv[3]);
          ABORT;
       }
    }

    /* Open the input file */
    inputFile = fopen(path, "r");
    /* does file exist? */
    if (inputFile == NULL) {
       if(retc == 1)
          retv[0] = intString(0);
       else
          Werrprintf("%s Not Found.", path);
       RETURN;
    }

    if (v1->T.basicType == T_REAL)
      assignReal(0.0,v1,0);
    else
      assignString("",v1,0);
    if (v2->T.basicType == T_REAL)
      assignReal(0.0,v2,0);
    else
      assignString("",v2,0);

    /* Read each line */
    if (jcampFlag)
    {
        numRows = jcamp(inputFile, v1, v2); 
    }
    else while (priv_getline(inputFile, line1, LINE_LIMIT)) {
        length = strlen(line1);
        if(length == 0)
            continue;
    	/* skip # and empty lines */
        for(i=0; i < length; i++) {
            c = line1[i];
            /* Skip comment lines */
            if(c == '#') {
                /* let code below know we don't want this line */
                i = length;
                break;
            }

            /* skip white space */
            if(c != ' ' && c != '\t') {
                /* break out of for loop, and process the rest of the string */
                break;
            }
        }

        /* Nothing useful in this line, skip on */
        if(i == length)
            continue;

        /* Get the part of the string fillowing any leading white space */
        strcpy(line2, &line1[i]);

        /* Get first word */
        length = strlen(line2);
        for(i=0; i < length; i++) {
            /* Go through chars until white space or end is found */
            c = line2[i];
            if(c == ' ' || c == '\t')
                break;
        }
        /* i should be length of the first word */
        strncpy(first, line2, i);
        /* terminate it */
        first[i] = '\0';

        /* Get remaining string if any */
        for(; i < length; i++) {
            c = line2[i];
            /* Go through chars until non-white space or end is found */
            if(c != ' ' && c != '\t')
                break;
        }
        /* Is there anything left? */
        if(i == length) {
            /* Nothing for second par, terminate it */
            remain[0] = '\0';
        }
        else {
            /* There is something left, keep it. */
            strcpy(remain, &line2[i]);
            length = strlen(remain);
            /* remove trailing whitespace */
            while (--length > 0)
            {
               if ( (remain[length] == ' ') || (remain[length] == '\t') )
                  remain[length] = '\0';
               else
                  length = 0;
            }
        }

        /* Now see if cmpstr is set and if so, weed out any lines that
           don't match */
        if(cmpstr != NULL && cmpstr[0] != '\0') {
            length = strlen(cmpstr);
            if(strncmp(first, cmpstr, length) != 0) {
                /* Not a match, bail out before setting pars */
                continue;
            }
        }

        /* Count up the rows which we kept and this is the param index. */
        numRows++;

        if (v1->T.basicType == T_REAL) {
          assignReal(strtod(first, (char **)NULL), v1, numRows);
        }
        else {
          assignString(first,v1,numRows);
        }
        if (v2->T.basicType == T_REAL) {
          assignReal(strtod(remain, (char **)NULL), v2, numRows);
        }
        else {
          assignString(remain, v2, numRows);
        }
    }
    fclose(inputFile);
    if(retc == 1) {
        retv[0] = (char *)intString(numRows);
    }
    RETURN;
}


/************************************************/
/*  priv_getline returns the next line from a file	*/
/************************************************/
static int priv_getline(FILE *file, char line[], int limit)
{
  int ch,i;

  line[0] = '\0';
  i = 0;
  ch = EOF;
  while ((i < limit -1) && ((ch = getc(file)) != EOF) && (ch != '\n'))
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

#ifdef XXX
static int setString(char *varname, char *value, int index)
{
    if (P_setstring(GLOBAL, varname, value, index)) { /* Try GLOBAL */
	if (P_setstring(CURRENT, varname, value, index)) { /* Try CURRENT */
	    if (P_creatvar(GLOBAL, varname, T_STRING)) { /* Create in GLOBAL */
		return 0;	/* Can this happen? */
	    }
	    P_setprot(GLOBAL, varname, 0x8000); /* Set no-share attribute */
            P_setdgroup(GLOBAL, varname, D_PROCESSING); /* Processing parm */
	    if (P_setstring(GLOBAL, varname, value, index)) {
		return 0;	/* Can this happen? */
	    }
	}
    }
    return 1;
}
#endif

/*
 this function store name:value pair to a string parameter.
 if parameter does not exist, a GLOBAL parameter will be created
 if the same parameter exsits in different trees, search order is 
 CURRENT,GLOBAL,SYSTEMGLOBAL
*/
int setNameValuePair(char *paramName, char *name, char *value)
{
    vInfo paraminfo;
    int tree, i;
    char nameValuePair[MAXSTR], str[MAXSTR];
    char tmpstr[MAXSTR];
    char *strptr, *tokptr;

    tree=CURRENT;
    if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try CURRENT */
     tree=GLOBAL;
     if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try GLOBAL */
        tree=SYSTEMGLOBAL;
        if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try SYSTEMGLOBAL */
            tree=GLOBAL;
            P_creatvar(tree, paramName, T_STRING);  /* Create in GLOBAL */
	    P_setdgroup(tree, paramName, D_PROCESSING); /* Processing parm */
	    P_getVarInfo(tree, paramName,  &paraminfo);
        }
      }
    }

    if(paraminfo.basicType != T_STRING) {
      Werrprintf("Error: Parameter %s should be string type.",paramName);
      ABORT;
    }
 
    strcpy(nameValuePair, name);
    strcat(nameValuePair,":");
    strcat(nameValuePair,value);

    for(i=1; i<=paraminfo.size; i++)
    {
	if(!(P_getstring(tree, paramName, str, i, MAXSTR))) {
	   if(strlen(str) == 0) {
	      P_setstring(tree, paramName, nameValuePair, i);
	      RETURN;
	   }

	   strptr=str;
	   tokptr = (char*) strtok(strptr, ":");
           tmpstr[0]='\0';
	   if(strlen(tokptr) > 0) {
              strcpy(tmpstr, tokptr);
	   }
	   if(strcmp(tmpstr,name) == 0) { 
	      P_setstring(tree, paramName, nameValuePair, i);
	      RETURN;
	   }
        }
    }
    P_setstring(tree, paramName, nameValuePair, paraminfo.size+1);
    RETURN;
}

int setvalue4name(int argc, char *argv[], int retc, char *retv[])
{
    if(argc < 4 || argc > 4)
    {
      Werrprintf("Usage -- %s(paramName,valName,value)",argv[0]);
      ABORT;
    }

    setNameValuePair(argv[1], argv[2], argv[3]);
    RETURN;
}

/*
  this function get value from name:value of a parameter.
  value is empty string if param or name:value pair does not exist.
*/
int getNameValuePair(char *paramName, char *name, char *value)
{
    vInfo paraminfo;
    int tree, i;
    char valName[MAXSTR], str[MAXSTR];
    char *strptr, *tokptr;
    int noName;

    strcpy(value, "");

    tree=CURRENT;
    if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try CURRENT */
     tree=GLOBAL;
     if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try GLOBAL */
        tree=SYSTEMGLOBAL;
        if (P_getVarInfo(tree, paramName,  &paraminfo)) { /* Try SYSTEMGLOBAL */
      	   return(-1);
        }
     }
    }

    if(paraminfo.basicType != T_STRING) {
      Werrprintf("Error: Parameter %s should be string type.",paramName);
      return(-1);
    }
 
    noName=0;
    if(strlen(name) == 0) noName=1;

    for(i=1; i<=paraminfo.size; i++)
    {
	if(!(P_getstring(tree, paramName, str, i, MAXSTR))) {

	   if (!strrchr(str,':')) continue;

	   strptr=str;	
	   tokptr = (char*) strtok(strptr, ":");
	   if(tokptr && strlen(tokptr) > 0) {
              strcpy(valName, tokptr);
	      if(noName) strcpy(name, valName); 
	      if(strcmp(valName,name) == 0) {
		 strptr = (char *) 0;
	         tokptr = (char*) strtok(NULL, ":"); 
	         if(tokptr && strlen(tokptr) > 0) {
                    strcpy(value, tokptr);
	         }
		 if(noName && strlen(value)>0) return(0);
	      }
	   }
        }
    }
    return(0);
}

int getvalue4name(int argc, char *argv[], int retc, char *retv[])
{
    char name[MAXSTR];
    char value[MAXSTR];

    if(argc < 2 || argc > 3)
    {
      Werrprintf("Usage -- %s(paramName,name):$value",argv[0]);
      if(retc > 0) retv[0]=newString("");
      ABORT;
    }

    if(argc > 2) strcpy(name,argv[2]);
    else strcpy(name,"");

    getNameValuePair(argv[1], name, value);
    if(retc > 1) {
      	retv[0]=newString(name);
      	retv[1]=newString(value);
    } else if(retc > 0) 
	retv[0]=newString(value);

    RETURN;
}

int encipher(int argc, char *argv[], int retc, char *retv[])
{
   FILE *txtfile = NULL;
   int binfile;
   int ch;
   int inplace = 0;
   int origIsText = 1;
   char *toFile;

   if ((argc >= 4) && ! strcmp(argv[1],"mac") )
   {
      FILE *outtxtfile = NULL;
      int num = 127;
      if (argc == 5)
         num = -127;
      txtfile = fopen( argv[2], "r" );
      if (txtfile == NULL)
      {
         Werrprintf( "%s: can't open %s", argv[0], argv[2] );
         ABORT;
      }
      outtxtfile = fopen( argv[3], "w" );
      if (outtxtfile == NULL)
      {
         Werrprintf( "%s: can't open %s", argv[0], argv[3] );
         fclose(txtfile);
         ABORT;
      }
      while ( (ch = getc(txtfile)) != EOF)
      {
            putc(ch+num, outtxtfile);
      }
      fclose(txtfile);
      fclose(outtxtfile);
      txtfile = NULL;
      RETURN;
   }
   if ((argc != 4) && (argc != 3))
   {
      Werrprintf("Usage -- %s('bin',infile,outfile) or %s('text',infile,outfile)",
                 argv[0], argv[0]);
      ABORT;
   }

   if ( strcmp(argv[1],"bin") && strcmp(argv[1],"text") )
   {
      Werrprintf("Usage -- %s first argument must be 'bin' or 'text'",argv[0]);
      ABORT;
   }
   if (argc == 3)
      toFile = argv[2];
   else
      toFile = argv[3];
   if ( ! strcmp(argv[2], toFile ) )
      inplace = 1;
   binfile = open( argv[2], O_RDONLY );
   if (binfile < 0)
   {
      Werrprintf( "%s: can't open %s", argv[0], toFile );
      ABORT;
   }
   read( binfile, &ch,  sizeof(int));
   if (ch == 616)
   {
      read( binfile, &ch,  sizeof(int));
      if (ch == 1954)
         origIsText = 0;
   }
   
   if ( origIsText )
   {
      close(binfile);
      txtfile = fopen( argv[2], "r" );
      if (txtfile == NULL)
      {
         Werrprintf( "%s: can't open %s", argv[0], argv[2] );
         ABORT;
      }
   }

   if ( ! strcmp(argv[1],"bin") )
   {
      int outfile;
      int outch;

      if ( ! origIsText )      // it is already binary
      {
         if ( ! inplace)  // just copy
         {
            outfile = open(toFile, O_WRONLY | O_CREAT | O_TRUNC, 0666 );
            if (outfile < 0)
            {
               Werrprintf( "%s: can't open %s", argv[0], toFile );
               close(binfile);
               ABORT;
            }
            ch = 616;
            write(outfile, &ch, sizeof(int) );
            ch = 1954;
            write(outfile, &ch, sizeof(int) );
            while ( read( binfile, &ch,  sizeof(int)) > 0)
            {
               write(outfile, &ch, sizeof(int) );
            }
            close(binfile);
            close(outfile);
         }
      }
      else
      {
         char tmpfile[MAXPATH];
         if (inplace)
         {
            sprintf(tmpfile,"%s.tmp",toFile);
            outfile = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC, 0666 );
         }
         else
            outfile = open(toFile, O_WRONLY | O_CREAT | O_TRUNC, 0666 );
         if (outfile < 0)
         {
            Werrprintf( "%s: can't open %s", argv[0], toFile );
            fclose(txtfile);
            ABORT;
         }
         outch = 616;
         write(outfile, &outch, sizeof(int) );
         outch = 1954;
         write(outfile, &outch, sizeof(int) );
         while ( (ch = getc(txtfile)) != EOF)
         {
            outch = ch + 666;
            write(outfile, &outch, sizeof(int) );
         }
         fclose(txtfile);
         txtfile = NULL;
         close(outfile);
         if (inplace)
         {
            rename(tmpfile, toFile);
         }
      }
   }
   else
   {
      FILE *outfile;

      if ( origIsText )      // it is already text
      {
         if ( ! inplace)  // just copy
         {
            outfile = fopen(toFile, "w" );
            if (outfile == NULL)
            {
               Werrprintf( "%s: can't open %s", argv[0], toFile );
               fclose(txtfile);
               ABORT;
            }
            while ( (ch = getc(txtfile)) != EOF)
            {
               putc(ch, outfile);
            }
            fclose(outfile);
         }
         fclose(txtfile);
         txtfile = NULL;
      }
      else
      {
         char tmpfile[MAXPATH];

         if (inplace)
         {
            sprintf(tmpfile,"%s.tmp",toFile);
            outfile = fopen(tmpfile, "w" );
         }
         else
            outfile = fopen(toFile, "w" );
         if (outfile == NULL)
         {
            Werrprintf( "%s: can't open %s", argv[0], toFile );
            close(binfile);
            ABORT;
         }
         while ( read( binfile, &ch,  sizeof(int)) > 0)
         {
            putc(ch-666, outfile);
         }
         close(binfile);
         fclose(outfile);
         if (inplace)
         {
            rename(tmpfile, toFile);
         }
      }
   }
   if (txtfile != NULL)
      fclose(txtfile);
   RETURN;
}

