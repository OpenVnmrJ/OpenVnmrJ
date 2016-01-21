/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acqparms.h"
#include "pvars.h"
#include "abort.h"

char ws[MAXSTR];
extern int bgflag;
extern void genPower(double power, int rfch);
extern void genOffset(double offset, int rfch);

void Winfoprintf(char *format, ...) { }

void Wscrprintf(char *format, ...) { }

void WerrprintfWithPos(const char *format, ...) { }

int getparmd(char *varname, char *vartype, int tree, double *varaddr, int size)
{
    int ret;
    char mess[MAXSTR];

    if ( (strcmp(vartype,"REAL") == 0) || (strcmp(vartype,"real") == 0) )
    {
        if ((ret = P_getreal(tree,varname,(double *)varaddr,1)) < 0)
        {    sprintf(mess,"Cannot get parameter: %s\n",varname);
             text_error(mess);
             if (bgflag)
                 P_err(ret,varname,": ");
             return(1);
        }
    }
    return(0);
}

int PSG_abort(int a)
{
  exit(EXIT_FAILURE);
}

int getFirstActiveRcvr()
{
  return(1);
}

double getGain()
{
  return(6.0);
}

int lcsample() 
{ 
   return(0);
}
/* 
   these are here because a macro definition 
   confuses those who use offset and power as
   variables.
*/
int power(double po, int dev)
{
   genPower(po,dev);
   return(0);
}

int offset(double off, int dev) 
{
   genOffset(off, dev);
   return(0);
}

int Tflag;

void Wprintfpos(int window, int line, int column, char *format, ...) 
{ printf("Wprintfpos  dead\n");}

void Werrprintf(char *format, ...) 
{ printf("Werrprintf  dead\n");}

/* 
  needs HSgate stub here for SolidsPack for VnmrJ 3.0
*/
void HSgate(int ch, int state)
{ }
