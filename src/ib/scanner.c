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
    return "@(#)scanner.c 18.1 03/21/08 (c)1991 SISCO";
}

 /*
  *  scanner.c:  This file contains the lexical scanner used by the DDL
  *              parser.
  *
  */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#ifdef LINUX
#include "generic.h"
#include <iostream>
#include <strstream>
#else
#include <generic.h>
#include <iostream.h>
#include <strstream.h>
#endif
#include <math.h>
#include <sys/param.h>
#ifdef LINUX 
#elif SOLARIS
#include <sunmath.h>
#endif
#include "ddllib.h"
#include "ddl-tab.h"
#include <iomanip>

#define SYM_NAME_LENGTH MAXPATHLEN

//#define scannerdebug name2(/,/)
#define scannerdebug cout

/*static ofstream debug("/dev/tty", ios::out);*/
#define debug name2(/,/)

#define isvalidpunct(c) (( c == '_' || c == '$' ) ? TRUE : FALSE)

static int lineno = 1;

/*jmp_buf begin;*/
static int indef;
char* infile;
extern DDLSymbolTable* symlist;
static int c;
static int column;


int  backslash(int c);
int follow(int expect, int ifyes, int ifno);
void warning(char *s, char *t);
void EatComment();

int ddlgetc()
{
  int c = symlist->ddlin->get();
  if (c != EOF) {
    column++;
    if (c == '\n') {
      lineno++;
      column = 1;
    }
  }
  return c;
}

void GetDirective()
{
  int c = ddlgetc();

  //cout << "Skipping: ";
  
  while (c != EOF && c != '\n' ) {
    c = ddlgetc();
    //cout << (char) c;
  }
  //cout << endl;
}

int EatWhiteSpace()
{
  while ((c = ddlgetc()) == ' ' || c == '\t' || c == '\n' || c == '\f') ;
  return c;
}
  
int yylex()
{
  
  while ((c = EatWhiteSpace()) != EOF) {
    
    if (c == '#' && column == 1) {
      GetDirective();
      continue;
    }

    // - Pseudo End of File
    if (c == 0) {
      //scannerdebug << "Pseudo EOF encountered at line " << lineno;
      //scannerdebug << " byte number " << symlist->ddlin->gcount() << "\n";
      return EOF;
    }
    
    //-- REAL
    if (c == '.' || isdigit(c)) {  
      double d;
      symlist->ddlin->putback(c);
      (*symlist->ddlin) >> d;
      yylval.val = d;
      //scannerdebug << "scanner.c: REAL = " << d << "\n";
      return DDL_REAL;
    }
    
    //-- VAR
    if (isalpha(c)) {
      DDLNode *s;
      char sbuf[SYM_NAME_LENGTH], *p = sbuf;

      sbuf[SYM_NAME_LENGTH-1] = 0;
      
      do {
	if (p < sbuf + sizeof(sbuf) - 1) {
	  *p++ = c;
	}
      } while ((c = ddlgetc()) != EOF &&
	       (isalnum(c) || isvalidpunct(c)));
      symlist->ddlin->putback(c);
      *p = 0;
      if (strcmp(sbuf, "NaN") == 0){
#ifdef LINUX
	  yylval.val = 0.0 / 0.0;
#else
	  yylval.val = quiet_nan(0L);
#endif
	  return DDL_REAL;
      }
      if ((s = symlist->Lookup(sbuf)) == NullDDL) {
	s = symlist->Install(sbuf, UNDEFINED);
      }
      yylval.sym = s;
      switch (s->symtype) {
      case STRUCT:
	return s->symtype;
      }
      return s->symtype == UNDEFINED ? VAR : s->symtype;
    }
    
    /*
      //-- ARGUMENT
      if (c == '$') {
      int n = 0;
      
      while (isdigit(c = ddlgetc())) {
      n = 10 * n + c - '0';
      }
      symlist->ddlin->putback(c);
      if (n == 0) execerror("strange $...");
      yylval.narg = n;
      return ARG;
      }
      */
    
    //-- QUOTED STRING
    if (c == '\"') {
      char sbuf[SYM_NAME_LENGTH], *p;
      
      for (p = sbuf; (c = ddlgetc()) != '\"'; p++) {
	if (c == EOF) execerror("missing quote", "");
	if (p >= sbuf + sizeof(sbuf) - 1) {
	  *p = 0;
	  execerror("string too long", sbuf);
	}
	*p = backslash(c);
      }
      *p = 0;
      yylval.str = emalloc(strlen(sbuf)+1);
      strcpy(yylval.str, sbuf);
      return DDL_STRING;
    }
    
    switch(c) {
      
    case '>':
      return follow('=', GE, GT);
      
    case '<':
      return follow('=', LE, LT);
      
    case '=':
      return follow('=', EQ, '=');
      
    case '!':
      return follow('=', NE, NOT);
      
    case '|':
      return follow('|', OR, '|');
      
    case '&':
      return follow('&', AND, '&');
      
    case '/':
      if (c = ddlgetc() == '*') {
	//scannerdebug << "COMMENT: " << '/' << '*';
	EatComment();
	//scannerdebug << '*' << '/' << "\n";
	continue;
      } else {
	symlist->ddlin->putback(c);
	return c;
      }
      
    case '\n':
      lineno++;
      return '\n';
      
    default:
      return c;
    }

  }
  return EOF;
}

void EatComment()
{
  char c;

  while (1) {
    c = ddlgetc();
    if (c == EOF) {
      symlist->ddlin->putback(c);
      return;
    }
    if (c != '*') {
      if (c == EOF) {
	symlist->ddlin->putback(c);
	return;
      }
      //scannerdebug << c;
      continue;
    }

    if ((c = ddlgetc()) == '/') {
      return;
    } else {
      symlist->ddlin->putback(c);
      continue;
    }
  }
}

  
	 
   /*
    *
    *  backslash(c) get's the next character with \'s interpreted
    *
    */
     
int  backslash(int c)
{
  
  static char transtab[] = "b\bf\fn\nr\rt\t";
  
  if (c != '\\') return c;
  
  c = ddlgetc();
  if (islower(c) && strchr(transtab, c)) return strchr(transtab, c)[1];
  return c;
}

 /*
  *
  *  follow looks ahead for >=, etc
  *
  */

int follow(int expect, int ifyes, int ifno)
{

  int c = ddlgetc();

  if (c == expect) return ifyes;
  symlist->ddlin->putback(c);
//  ungetc(c,ddlin);
  return ifno;
}

 /*
  *
  * warns if illegal definition encountered
  *
  */

void defonly(char *s)
{
  if (!indef) execerror(s, "used outside definition");
}


void yyerror(const char *s)
{
  warning((char*)s, (char*)0);
}

void execerror(char *s, char *t)
{
  warning(s, t);
//  fseek(ddlin, 0L, 2);
//  longjmp(begin, 0);
  exit(1);
}

void fpecatch()
{
  execerror("floating point exception", (char*)0);
}

char *GetMagicNumber()
{
  //  char *magic = new char[SYM_NAME_LENGTH];
  ostrstream magic;

  //  symlist->ddlin->getline(magic, SYM_NAME_LENGTH);
  magic << symlist->ddlin;
  return magic.str();
}


int initscanner()
{
  lineno = 1;
  column = 0;
  return TRUE;
}

void closescanner()
{
    symlist->ddlin->close();
    symlist->st_debug->close();
    return;
}


void warning(char *s, char *t)
{
  cerr << s;
  if (t) {
    cerr << " " << t;
  }
  if (infile) {
    cerr << " in " << infile;
  }
  cerr << " in FDF header near line " << lineno << endl;

  while (c != '\n' && c != EOF) c = ddlgetc();
  if (c == '\n') lineno++;
}


