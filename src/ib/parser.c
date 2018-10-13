/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
static char *sccsID(){
    return "@(#)parser.c 18.1 03/21/08 (c)1991 SISCO";
}
/*
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef LINUX 
#include <iostream>
#include <strstream>
#include <fstream>
#elif SOLARIS
#include <stream.h>
#include <iostream.h>
#include <strstream.h>
#include <fstream.h>
#include <sunmath.h>
#endif
#include "ddllib.h"
#include "ddl-tab.h"

double Log(double x);
double Log10(double x);
double Exp(double x);
double Sqrt(double x);
double Pow(double x, double y);
double integer(double x);

DDLSymbolTable *symlist;

static struct {
  char *name;
  int token;
} keywords[] = {
  "typedef",   TYPEDEF,
  "struct",    STRUCT,
  "float",     DDL_FLOAT,
  "void",      VOID,
  "char",      DDL_CHAR,
  "int",       DDL_INT,
  0,           0,
};

 
DDLSymbolTable *ParseDDLFile(char *infile)
{
  int i;
#ifdef LINUX
  double NaN_double = 0.0 / 0.0;
#else
  double NaN_double = quiet_nan(0L);
#endif

  symlist = new DDLSymbolTable(infile);

  
  symlist->Install("NaN", BUILTIN, NaN_double);
  symlist->Install("NULL", BUILTIN, 0.0);
  for (i = 0; keywords[i].name; i++)
    symlist->Install(keywords[i].name, keywords[i].token);

  if (initscanner()) {
    char *magicnumber = GetMagicNumber();
    //cout << "DDL Parser \n" << "\n";
    //cout << "Magic Number = " << magicnumber << "\n";
    delete magicnumber;
    //cout << "Calling yyparse()..." << "\n";
    yyparse();
    closescanner();
    return symlist;
  } else {
    cerr << "Error: Cannot open " << infile << endl;
    return 0;
  }
}
