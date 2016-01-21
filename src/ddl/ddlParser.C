/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
//#include <stream.h>
#include <string.h>
#include <math.h>
#ifdef SOLARIS
#include <sunmath.h>
#endif 

#include "ddlParser.h"
#include "ddlSymbol.h"
#include "ddlGrammar.h"
#include "ddlScanner.h"
//#include "ddllib.h"
//#include "ddl-tab.h"

//double Log(double x);
//double Log10(double x);
//double Exp(double x);
//double Sqrt(double x);
//double Pow(double x, double y);
//double integer(double x);

DDLSymbolTable *symlist;

static struct {
  const char *name;
  int token;
} keywords[] = {
  {"typedef",   TYPEDEF},
  {"struct",    STRUCT},
  {"float",     DDL_FLOAT},
  {"void",      VOID},
  {"char",      DDL_CHAR},
  {"int",       DDL_INT},
  {0,           0},
};

 
DDLSymbolTable *ParseDDLFile(const char *infile)
{
  int i;
  const double NaN_double = strtod("Nan",NULL); // 0.0/0.0 or  quiet_nan(0L)

  symlist = new DDLSymbolTable(infile);

  
  symlist->Install("NaN", BUILTIN, NaN_double);
  symlist->Install("NULL", BUILTIN, 0.0);
  for (i = 0; keywords[i].name; i++)
    symlist->Install(keywords[i].name, keywords[i].token);

  if (ddlInitscanner()) {
      //char *magicnumber = GetMagicNumber();
    //cout << "DDL Parser \n" << "\n";
    //cout << "Magic Number = " << magicnumber << "\n";
      //delete magicnumber;
    //cout << "Calling yyparse()..." << "\n";
    ddl_yyparse();
    ddlClosescanner();
    return symlist;
  } else {
    cerr << "Error: Cannot open " << infile << endl;
    return 0;
  }
}
