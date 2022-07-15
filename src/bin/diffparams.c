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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "vnmrsys.h"
#include "group.h"
#define VNMRJ
#include "variables.h"
#include "params.h"
#include "pvars.h"

typedef int (*PFI)(char *);           /* Pointer to Function returning int */

static int makeMacro = 0;
static int listValue = 0;
static int listActive = 0;
static int listProtect = 0;

/*------------------------------------------------------------------------
|       isDirectory
|
|       This routine determines if name is a directory
+-----------------------------------------------------------------------*/
static int
isDirectory(char *filename)
{
   int  ival, retval;
   struct stat	buf;

   ival = stat(filename,&buf);
   if (ival != 0)
     retval = 0;
   else
     if (buf.st_mode & S_IFDIR)
       retval = 1;
     else
       retval = 0;
   return( retval );
}

void outputLine(char *name, int v1ok, vInfo v1, int v2ok, vInfo v2)
{
   double val;
   char tmp[512];

   if (makeMacro == 0)
   {
      if (v1ok && v2ok && (v1.subtype != v2.subtype))
      {
         fprintf(stdout,"%s types\t%s\t\t%s\n", name, whatType(v1.subtype), whatType(v2.subtype) );
      }
      if (v1ok && v2ok && ((v1.active != v2.active) && listActive) )
      {
         fprintf(stdout,"%s \t%s\t\t%s\n", name, (v1.active) ? "active" : "inactive",
                                                 (v2.active) ? "active" : "inactive" );
      }
      if (v1ok && v2ok && ((v1.prot != v2.prot) && listProtect) )
      {
         fprintf(stdout,"%s protection\t%d\t\t%d\n", name, v1.prot, v2.prot);
      }
      fprintf(stdout,"%-12s\t",name);
      if (!v1ok)
      {
         fprintf(stdout,"undefined\t");
      }
      else if (v1.size > 1)
      {
         fprintf(stdout,"array of %d %ss\t\t",v1.size,whatType(v1.subtype) );
      }
      else
      {
         switch (v1.basicType)
         {
         case T_REAL:   P_getreal(CURRENT,name,&val,1);
                        fprintf(stdout,"%g\t\t",val);
                        break;
         case T_STRING: P_getstring(CURRENT,name,tmp,1,511);
                        fprintf(stdout,"%s\t\t",tmp);
                        break;
         default:          
                        break;
         }
      }
      if (!v2ok)
      {
         fprintf(stdout,"undefined\n");
      }
      else if (v2.size > 1)
      {
         fprintf(stdout,"array of %d %ss\n",v2.size,whatType(v2.subtype) );
      }
      else
      {
         switch (v2.basicType)
         {
         case T_REAL:   P_getreal(PROCESSED,name,&val,1);
                        fprintf(stdout,"%g\n",val);
                        break;
         case T_STRING: P_getstring(PROCESSED,name,tmp,1,511);
                        fprintf(stdout,"%s\n",tmp);
                        break;
         default:          
                        break;
         }
      }
   }
}

void outputMacro(int what, char *str, ...)
{
   va_list vargs;
   static FILE *macroFile = NULL;
   static char macroName[512];

/* Second argument (what) defines what to do.
 * what == 0 means initialize
 * what == 1 means add line to output file
 * what == 2 means close output file
 */

   if (what == 0)
   {
      strcpy(macroName,str);
      makeMacro = 1;
   }
   else if (makeMacro == 1)
   {
      if (what == 1)
      {
         if (macroFile == NULL)
            macroFile = fopen(macroName,"w");
         va_start(vargs, str);
         vfprintf(macroFile,str,vargs);
         va_end(vargs);
      }
      else if (what == 2)
      {
         if (macroFile != NULL)
            fclose(macroFile);
      }
   }
}

void makeVar(char *name, vInfo *v)
{
   int index;
   double val;
   char tmp[512];
#include "init.h"

   outputMacro(1, "exists('%s','parameter'):$e\n",name );
   outputMacro(1, "if ($e < 0.5) then\n");
   outputMacro(1, "  create('%s','%s')\n",name, whatType(v->subtype) );
   outputMacro(1, "endif\n");
   outputMacro(1, "setlimit('%s',%g,%g,%g)\n",name, v->maxVal, v->minVal, v->step );
   if (v->group != preset[v->subtype].Ggroup)
      outputMacro(1, "setgroup('%s','%s')\n",name, whatGroup(v->group) );
   if (v->Dgroup != preset[v->subtype].Dgroup)
      outputMacro(1, "setdgroup('%s',%d)\n",name, v->Dgroup );
   if (v->prot != preset[v->subtype].Prot)
      outputMacro(1, "setprotect('%s','set',%d)\n",name, v->prot );
   if (v->active == ACT_OFF)
   {
      outputMacro(1, "off('%s')\n",name );
   }
   index = 1;
   while (index <= v->size)
   {
      char *ch_ptr;
      switch (v->basicType)
      {
      case T_REAL:   P_getreal(PROCESSED,name,&val,index);
                     outputMacro(1, "setvalue('%s',%g,%d)\n",name,val,index );
                     break;
      case T_STRING: P_getstring(PROCESSED,name,tmp,index,511);
                     if ((ch_ptr = strchr(tmp,'\'')) == NULL)
                     {
                        outputMacro(1, "setvalue('%s','%s',%d)\n",name,tmp,index);
                     }
                     else
                     {
                        outputMacro(1, "setvalue('%s',`%s`,%d)\n",name,tmp,index);
                     }
                     break;
      default:          
                     break;
      }
      index++;
   }
}

int editVar(char *name)
{
   vInfo v1, v2;
   double val, val2;
   char tmp[512], tmp2[512];
   int index;

   P_getVarInfo(CURRENT,name,&v1);
   P_getVarInfo(PROCESSED,name,&v2);
   if (listActive)
   {
      if (v1.active != v2.active)
      {
         if (listValue)
         {
            fprintf(stdout,"%s ",name);
         }
         else
         {
            outputLine(name, 1, v1, 1, v2);
         }
      }
      return(0);
   }
   if (listProtect)
   {
      if (v1.prot != v2.prot)
      {
         if (listValue)
         {
            fprintf(stdout,"%s ",name);
         }
         else
         {
            outputLine(name, 1, v1, 1, v2);
         }
      }
      return(0);
   }
   if ( (v1.maxVal != v2.maxVal) ||  (v1.minVal != v2.minVal) || (v1.step != v2.step) )
   {
      outputMacro(1, "setlimit('%s',%g,%g,%g)\n",name, v2.maxVal, v2.minVal, v2.step );
   }
   if (v1.prot != v2.prot)
   {
      outputMacro(1, "setprotect('%s','set',%d)\n",name, v2.prot);
   }
   if (v1.active != v2.active)
   {
      outputMacro(1, "%s('%s')\n",(v2.active) ? "on" : "off", name);
   }
   if (v1.group != v2.group)
   {
      outputMacro(1, "setgroup('%s','%s')\n",name, whatGroup(v2.group) );
   }
   if (v1.Dgroup != v2.Dgroup)
   {
      outputMacro(1, "setdgroup('%s',%d)\n",name, v2.Dgroup );
   }
   if (v1.size != v2.size)
   {
     char *ch_ptr;

     if (listValue)
     {
        fprintf(stdout,"%s ",name);
        return(0);
     }
     index = 1;
     outputLine(name, 1, v1, 1, v2);
     while (index <= v2.size)
     {
      switch (v2.basicType)
      {
      case T_REAL:   P_getreal(PROCESSED,name,&val,index);
                     outputMacro(1, "setvalue('%s',%g,%d)\n",name,val,(index == 1) ? 0 : index);
                     break;
      case T_STRING: P_getstring(PROCESSED,name,tmp,index,511);
                     if ((ch_ptr = strchr(tmp,'\'')) == NULL)
                     {
                        outputMacro(1, "setvalue('%s','%s',%d)\n",name,tmp,index);
                     }
                     else
                     {
                        outputMacro(1, "setvalue('%s',`%s`,%d)\n",name,tmp,index);
                     }
                     break;
      default:          
                     break;
      }
      index++;
     }
   }
   else
   {
     index = 1;
     while (index <= v2.size)
     {
      char *ch_ptr;

      switch (v2.basicType)
      {
      case T_REAL:      P_getreal(CURRENT,name,&val,index);
                        P_getreal(PROCESSED,name,&val2,index);
                        if (val != val2)
                        {
                           if (listValue)
                           {
                              fprintf(stdout,"%s ",name);
                              return(0);
                           }
                           outputMacro(1, "setvalue('%s',%g,%d)\n",name,val2,index);
                           outputLine(name, 1, v1, 1, v2);
                        }
                        break;
      case T_STRING:    P_getstring(CURRENT,name,tmp,index,511);
                        P_getstring(PROCESSED,name,tmp2,index,511);
                        if (strcmp(tmp,tmp2))
                        {
                           if (listValue)
                           {
                              fprintf(stdout,"%s ",name);
                              return(0);
                           }
                           if ((ch_ptr = strchr(tmp2,'\'')) == NULL)
                           {
                              outputMacro(1, "setvalue('%s','%s',%d)\n",
                                 name,tmp2,index);
                           }
                           else
                           {
                              outputMacro(1, "setvalue('%s',`%s`,%d)\n",
                                 name,tmp2,index);
                           }
                           outputLine(name, 1, v1, 1, v2);
                        }
                        break;
      default:          
                        break;
      }
      index++;
     }
   }
   return(0);
}

int crVar(char *name)
{
   vInfo v1, v2;
   int ret = 0;

   if (P_getVarInfo(CURRENT,name,&v1))
   {
      P_getVarInfo(PROCESSED,name,&v2);
      if (!listValue && !listActive && !listProtect)
      {
         makeVar(name, &v2);
         outputLine(name, 0, v1, 1, v2);
      }
      else if (listValue)
      {
         fprintf(stdout,"%s ",name);
      }
      ret = 1;
   }
   return(ret);
}

int delVar(char *name)
{
   vInfo v1, v2;
   int ret = 0;

   P_getVarInfo(CURRENT,name,&v1);
   if (P_getVarInfo(PROCESSED,name,&v2))
   {
      outputMacro(1, "destroy('%s')\n",name);
      if (!listValue && !listActive && !listProtect)
         outputLine(name, 1, v1, 0, v2);
      ret = 1;
   }
   else
   {
      if (v1.subtype != v2.subtype)
      {
        if (!listValue && !listActive && !listProtect)
        {
           outputMacro(1, "destroy('%s')\n",name);
           makeVar(name, &v2);
        }
        if (!listValue && !listActive && !listProtect)
           outputLine(name, 1, v1, 1, v2);
        if (listValue)
        {
           fprintf(stdout,"%s ",name);
        }
        P_deleteVar(PROCESSED,name);
        ret = 1;
      }
   }
/*
   if (strcmp(name,"pw") == 0)
   {
      printf("active: %d\n",v2.active);
      printf("Dgroup: %d\n",v2.Dgroup);
      printf("group: %d\n",v2.group);
      printf("basicType: %d\n",v2.basicType);
      printf("size: %d\n",v2.size);
      printf("subtype: %d\n",v2.subtype);
      printf("Esize: %d\n",v2.Esize);
      printf("prot: %d\n",v2.prot);
      printf("minVal: %g\n",v2.minVal);
      printf("maxVal: %g\n",v2.maxVal);
      printf("step: %g\n",v2.step);
   }
 */
   return(ret);
}

void walkTree(symbol **tp, PFI func)
{   symbol  *p;

    if ( (p=(*tp)) )   /* check if there is at least something in tree */
    {	if (p->left)
	    walkTree(&(p->left),func);
	if (p->right)
	    walkTree(&(p->right),func);
	if (p->name)
	{
	   if (func(p->name))
              delName(tp,p->name);
	}
    }
}

int get_params(char *cname, char *argv[] )
{
	int	ival;
        char  path[MAXPATH];

        strcpy(path,argv[0]);
        if (isDirectory(path))
           strcat(path,"/procpar");
	ival = access( path, R_OK );
	if (ival != 0) {
		fprintf( stderr, "%s: cannot access %s\n", cname, path );
		perror( "reason" );
		return( -1 );
	} 
	ival = P_read( CURRENT, path );
	if (ival != 0) {
		fprintf( stderr, "error reading parameters from %s\n", path );
		return( -1 );
	}

        strcpy(path,argv[1]);
        if (isDirectory(path))
           strcat(path,"/procpar");
	ival = access( path, R_OK );
	if (ival != 0) {
		fprintf( stderr, "%s: cannot access %s\n", cname, path );
		perror( "reason" );
		return( -1 );
	} 
	ival = P_read( PROCESSED, path );
	if (ival != 0) {
		fprintf( stderr, "error reading parameters from %s\n", path );
		return( -1 );
	}

	return( 0 );
}

int main(int argc, char *argv[] )
{
   int	ival;
   symbol **root1;
   PFI    cmd;

   if (argc < 3)
   {
      fprintf(stdout,"Usage: diffparams <-list> <-active> file1 file2 <macro>\n");
      exit(-1);
   }

   listValue = (!strcmp(argv[1],"-list") || !strcmp(argv[2],"-list") );
   listActive = (!strcmp(argv[1],"-active") || !strcmp(argv[2],"-active") );
   listProtect = (!strcmp(argv[1],"-protection") || !strcmp(argv[2],"-protection") );
   if (!listValue && !listActive && !listProtect && (argc == 4))
      outputMacro(0, argv[3]);
   ival = get_params( argv[ 0 ], &argv[ 1+listValue+listActive+listProtect ] );
   if (ival < 0)
      exit(-1);

/*
   foreach var in file1
     if not in file2
       issue delete and delete from file1
     else if different types
       issue delete and create and delete from file1 and file2
   foreach var in file2
     if not in file1
       issue create and delete from file2
   foreach var1 in file1
     get var2 from file2
     if var1 != var2
       issue stvalue
 */
   cmd = delVar;
   root1 = getTreeRootByIndex(CURRENT);
   walkTree(root1,cmd);
   balance(root1);  /* rebalance tree */

   cmd = crVar;
   root1 = getTreeRootByIndex(PROCESSED);
   walkTree(root1,cmd);
   balance(root1);  /* rebalance tree */

   cmd = editVar;
   root1 = getTreeRootByIndex(CURRENT);
   walkTree(root1,cmd);

   if (!listValue && !listActive && !listProtect)
      outputMacro(2, "");
   else
      fprintf(stdout,"\n");
   exit(0);
}

/*  Below are various symbols required by routines in magiclib.a
    or unmrlib.a which are needed by the ``xdcvt'' program.  They
    are defined here to prevent the ``ld'' program from bringing
    in the VNMR module which defines them, as the VNMR version
    includes references to SUN libraries which we would rather not
    load.								*/

/*  This represents all the VNMR software symbols which need to be
    defined somehow so the application program can read a parameter
    set (CURPAR, PROCPAR, GLOBAL, CONPAR, etc.) and work with the
    parameters in that set.  (VNMR Source file pvars.c)			*/

int	Dflag = 0;

/*  The next two actually do something.  */

char *skymalloc(int memsize)
{
	return( (char *) malloc(memsize) );
}

void skyfree(unsigned *memptr)
{
	free(memptr);
	return;
}

void unsetMagicVar(int addr)
{
}

void WerrprintfWithPos()
{
	printf( "WerrprintfWithPos called\n" );
}

void Werrprintf()
{
	printf( "Werrprintf called\n" );
}

void Wscrprintf()
{
	printf( "Wscrprintf called\n" );
}

void Wprintfpos()
{
	printf( "Wprintfpos called\n" );
}

void Winfoprintf()
{
	printf( "Winfoprintf called\n" );
}
