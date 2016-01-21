/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Copyright (c) Varian Assoc., Inc.  All Rights Reserved. */
/*
* Varian Assoc.,Inc. All Rights Reserved.
* This software contains proprietary and confidential
* information of Varian Assoc., Inc. and its contributors.
* Use, disclosure and reproduction is prohibited without
* prior consent.
*/
%{

#include <stdio.h>

#include "node.h"
#include "stack.h"
#include "magical_io.h"

extern char    fileName[];
extern char   *newStringId();
extern char    yytext[];
extern double  atof();
extern int     columnNumber;
extern int     ignoreEOL;
extern int     lineNumber;
extern int     yychar;
extern node   *newNode();
int            envLevel = 0;
int            loopLevel = 0;

extern char   *tempID;
extern node   *doingNode;

/*#define DEBUG*/

#ifdef DEBUG
static int Pflag = 2;
#define PPRINT0(str) \
	if (Pflag) fprintf(stderr,str)
#define PPRINT1(str,arg1) \
	if (Pflag) fprintf(stderr,str,arg1)
#define PSHOWTREE(arg1) \
        if (2 <= Pflag) showTree(0,"psr: ",arg1)
#else  DEBUG
#define PPRINT0(str)
#define PPRINT1(str,arg1)
#define PSHOWTREE(arg1)
#endif DEBUG
        
%}

%token <tval> AND BAD BOMB BREAK CL CM CMLIST DIV DO DONT DOLLAR ELSE ENDFILE EOL EQ
%token <tval> EXIT FI GE GT ID IF IGNORE LB LC LE LP LT MINUS MOD MULT NE NOT OD
%token <tval> OR PLUS POP PUSH RB RC REAL REPEAT RP SHOW SIZE SQRT STRING THEN
%token <tval> TRUNC TYPEOF UNIT UNTIL WHILE

%token <tval> _Car _Ca_ _C_r _C__ _NAME
%token <tval> _NEG

%type  <tval> if fi while od repeat until lb rb lp rp

%type  <nval> start stmnt sList expList exp lTerm lFact lAtom
%type  <nval> order arith term factor atom lVal rVal lValList name

%type  <dval> real
%type  <cval> string
%type  <cval> unit

%start start

%%

start   : sList EOL
                        ={
                            PPRINT0("psr: start      : sList EOL\n");
                            magicCodeTree = $1;
                            YYACCEPT;
                         }
        | EOL
                        ={
                            PPRINT0("psr: start      : EOL\n");
                            magicCodeTree = newNode(EOL,&($1.location));
                            YYACCEPT;
                         }
        | ENDFILE
                        ={
                            PPRINT0("psr: start      : ENDFILE\n");
                            magicCodeTree = newNode(ENDFILE,&($1.location));
                            YYACCEPT;
                         }
        | error EOL
                        ={
                            PPRINT0("psr: start      : error EOL\n");
                            magicCodeTree = NULL;
                            YYABORT;
                         }
        | error ENDFILE
                        ={
                            PPRINT0("psr: start      : error ENDFILE\n");
                            magicCodeTree = NULL;
                            YYABORT;
                         }
        ;

stmnt   : lVal lp expList rp CL lValList
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP CL lValList\n");
                            p = newNode(_Car,&($2.location));
                            addLeftSon(p,$6);
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lVal lp expList rp
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP\n");
                            p = newNode(_Ca_,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lVal               CL lValList
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal CL lValList\n");
                            p = newNode(_C_r,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lVal
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal\n");
                            p = newNode(_C__,&($1->location));
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lVal EQ expList
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal EQ expList\n");
                            p = newNode(EQ,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | BOMB
                        ={  $$ = newNode(BOMB,&($1.location));
                            PSHOWTREE($$);
                         }
        | lVal SHOW
                        ={  node *p;

                            PPRINT0("psr: stmnt      : lVal SHOW \n");
                            p = newNode(SHOW,&($2.location));
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | if exp THEN sList ELSE sList fi
                        ={  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList ELSE sList FI\n");
                            p = newNode(ELSE,&($1.location));
                            addLeftSon(p,$6);
                            addLeftSon(p,$4);
                            addLeftSon(p,$2);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | if exp THEN sList              fi
                        ={  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList FI\n");
                            p = newNode(THEN,&($1.location));
                            addLeftSon(p,$4);
                            addLeftSon(p,$2);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | while exp DO
                         {  loopLevel += 1;
                         }
          sList od
                        ={  node *p;

                            PPRINT0("psr: stmnt      : WHILE exp DO sList OD\n");
                            loopLevel -= 1;
                            p          = newNode(WHILE,&($1.location));
                            addLeftSon(p,$5);
                            addLeftSon(p,$2);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | repeat
                         {  loopLevel += 1;
                         }
          sList until
                         {  loopLevel -= 1;
                         }
          exp
                        ={  node *p;

                            PPRINT0("psr: stmnt      : REPEAT sList UNTIL exp\n");
                            p = newNode(REPEAT,&($1.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$6);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | EXIT lp expList rp
                        ={  node *p;

                            PPRINT0("psr: stmnt       : EXIT LP expList RP\n");
                            p = newNode(EXIT,&($1.location));
                            addLeftSon(p,$3);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | EXIT
                        ={
                            PPRINT0("psr: stmnt      : EXIT\n");
                            $$ = newNode(EXIT,&($1.location));
                            PSHOWTREE($$);
                         }
        | BREAK
                        ={
                            PPRINT0("psr: stmnt      : BREAK\n");
                            if (loopLevel)
                                $$ = newNode(BREAK,&($1.location));
                            else
                            {   WerrprintfWithPos("Syntax error! BREAK outside loop");
                                $$ = NULL;
                            }
                            PSHOWTREE($$);
                         }
        | PUSH
                        ={
                            PPRINT0("psr: stmnt      : PUSH\n");
                            envLevel += 1;
                            $$        = newNode(PUSH,&($1.location));
                            PSHOWTREE($$);
                         }
        | POP
                        ={
                            PPRINT0("psr: stmnt      : POP\n");
                            if (envLevel)
                            {   envLevel -= 1;
                                $$        = newNode(POP,&($1.location));
                            }
                            else
                            {   fprintf(stderr,"magic: POP without PUSH\n");
                                $$ = NULL;
                            }
                            PSHOWTREE($$);
                         }
        | IGNORE
                        ={
                            PPRINT0("psr: stmnt      : IGNORE\n");
                            $$ = newNode(IGNORE,&($1.location));
                            PSHOWTREE($$);
                         }
        | DONT
                        ={
                            PPRINT0("psr: stmnt      : DONT\n");
                            $$ = newNode(DONT,&($1.location));
                            PSHOWTREE($$);
                         }
        ;

sList   : sList stmnt
                        ={  node *p;

                            PPRINT0("psr: sList      : sList stmnt\n");
                            if ($1)
                                if ($2)
                                {   p = newNode(CM,&($2->location));
                                    addLeftSon(p,$2);
                                    addLeftSon(p,$1);
                                    $$ = p;
                                }
                                else
                                    $$ = $1;
                            else
                                $$ = $2;
                            PSHOWTREE(p);
                         }
        | stmnt
                        ={
                            PPRINT0("psr: sList      : stmnt\n");
                            $$ = $1;
                         }
        ;

expList : expList CM exp
                        ={  node *p;

                            PPRINT0("psr: expList    : expList CM exp\n");
                            p = newNode(CM,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | exp
                        ={
                            PPRINT0("psr: expList    : exp\n");
                            $$ = $1;
                         }
        ;

exp     : exp OR lTerm
                        ={  node *p;

                            PPRINT0("psr: exp        : exp OR lTerm\n");
                            p = newNode(OR,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lTerm
                        ={
                            PPRINT0("psr: exp        : lTerm\n");
                            $$ = $1;
                         }
        ;

lTerm   : lTerm AND lFact
                        ={  node *p;

                            PPRINT0("psr: lTerm      : lTerm AND lFact\n");
                            p = newNode(AND,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lFact
                        ={
                            PPRINT0("psr: lTerm      : lFact\n");
                            $$ = $1;
                         }
        ;

lFact   : NOT lAtom
                        ={  node *p;

                            PPRINT0("psr: lFact      : NOT lAtom\n");
                            p = newNode(NOT,&($1.location));
                            addLeftSon(p,$2);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lAtom
                        ={
                            PPRINT0("psr: lFact      : lAtom\n");
                         }
        ;

lAtom   : lAtom EQ order
                        ={  node *p;

                            PPRINT0("psr: lAtom      : lAtom EQ order\n");
                            p = newNode(EQ,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lAtom NE order
                        ={  node *p;

                            PPRINT0("psr: lAtom      : lAtom NE order\n");
                            p = newNode(NE,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | order
                        ={
                            PPRINT0("psr: lAtom      : order\n");
                         }
        ;

order   : order LT arith
                        ={  node *p;

                            PPRINT0("psr: order      : order LT arith\n");
                            p = newNode(LT,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | order LE arith
                        ={  node *p;

                            PPRINT0("psr: order      : order LE arith\n");
                            p = newNode(LE,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | order GT arith
                        ={  node *p;

                            PPRINT0("psr: order      : order GT arith\n");
                            p = newNode(GT,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | order GE arith
                        ={  node *p;

                            PPRINT0("psr: order      : order GE arith\n");
                            p = newNode(GE,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | arith
                        ={
                            PPRINT0("psr: order       : arith\n");
                         }
        ;

arith   : arith PLUS term
                        ={  node *p;

                            PPRINT0("psr: arith      : arith PLUS term\n");
                            p = newNode(PLUS,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | arith MINUS term
                        ={  node *p;

                            PPRINT0("psr: arith      : arith MINUS term\n");
                            p = newNode(MINUS,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | term
                        ={
                            PPRINT0("psr: arith      : term\n");
                         }
        ;

term    : term MULT factor
                        ={  node *p;

                            PPRINT0("psr: term       : term MULT factor\n");
                            p = newNode(MULT,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | term DIV factor
                        ={  node *p;

                            PPRINT0("psr: term       : term DIV factor\n");
                            p = newNode(DIV,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | term MOD factor
                        ={  node *p;

                            PPRINT0("psr: term       : term MOD factor\n");
                            p = newNode(MOD,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | factor
                        ={
                            PPRINT0("psr: term       : factor\n");
                         }
        ;

factor  : MINUS atom
                        ={  node *p;

                            PPRINT0("psr: factor     : MINUS atom\n");
                            p = newNode(_NEG,&($1.location));
                            addLeftSon(p,$2);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | atom
                        ={
                            PPRINT0("psr: factor     : atom\n");
                         }
        ;

atom    : lp exp rp
                        ={
                            PPRINT0("psr: atom       : LP exp RP\n");
                            $$ = $2;
                         }
        | SQRT lp exp rp
                        ={  node *p;

                            PPRINT0("psr: atom       : SQRT LP exp RP\n");
                            p = newNode(SQRT,&($1.location));
                            addLeftSon(p,$3);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | TRUNC lp exp rp
                        ={  node *p;

                            PPRINT0("psr: atom       : TRUNC LP exp RP\n");
                            p = newNode(TRUNC,&($1.location));
                            addLeftSon(p,$3);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | SIZE lp exp rp
                        ={  node *p;

                            PPRINT0("psr: atom       : SIZE LP exp RP\n");
                            p = newNode(SIZE,&($1.location));
                            addLeftSon(p,$3);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | TYPEOF lp exp rp
                        ={  node *p;

                            PPRINT0("psr: atom       : TYPEOF LP exp RP\n");
                            p = newNode(TYPEOF,&($1.location));
                            addLeftSon(p,$3);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | rVal
                        ={
                            PPRINT0("psr: atom       : rVal\n");
                            $$ = $1;
                         }
        ;

lVal    : name lb exp rb
                        ={  node *p;

                            PPRINT0("psr: lVal       : name LB exp RB\n");
                            p = newNode(LB,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | name
                        ={  node *p;

                            PPRINT0("psr: lVal       : name\n");
                            p = newNode(ID,&($1->location));
                            addLeftSon(p,$1);
                            $$     = p;
                            PSHOWTREE(p);
                         }
        ;

rVal    : name lb exp rb
                        ={  node *p;

                            PPRINT0("psr: rVal       : name LB exp RB\n");
                            p = newNode(LB,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | name
                        ={  node *p;

                            PPRINT0("psr: rVal       : name\n");
                            p = newNode(ID,&($1->location));
                            addLeftSon(p,$1);
                            $$     = p;
                            PSHOWTREE(p);
                         }
        | real
                        ={  node *p;
                        
                            PPRINT0("psr: rVal       : REAL\n");
                            p      = newNode(REAL,&($1.location));
                            p->v.r = $1.value;
                            $$     = p;
                            PSHOWTREE(p);
                         }
        | string
                        ={  node *p;
                        
                            PPRINT0("psr: rVal       : string\n");
                            p      = newNode(STRING,&($1.location));
                            p->v.s = $1.value;
                            $$     = p;
                            PSHOWTREE(p);
                         }
        | unit
                        ={  node *p;
                        
                            PPRINT0("psr: rVal       : unit\n");
                            p      = newNode(UNIT,&($1.location));
                            p->v.s = $1.value;
                            $$     = p;
                            PSHOWTREE(p);
                         }
        ;

lValList : lValList CM lVal
                        ={  node *p;

                            PPRINT0("psr: lValList   : lValList CM lVal\n");
                            p = newNode(CM,&($2.location));
                            addLeftSon(p,$3);
                            addLeftSon(p,$1);
                            $$ = p;
                            PSHOWTREE(p);
                         }
        | lVal
                        ={
                            PPRINT0("psr: lValList   : lVal\n");
                         }
        ;

name    : ID
                        ={  node *p;

                            PPRINT0("psr: name       : ID\n");
                            p      = newNode(_NAME,&($1.location));
                            p->v.s = newStringId(yytext,tempID);
                            $$     = p;
                            PSHOWTREE(p);
                         }
        | DOLLAR exp RC
                        ={  node *p;

                            PPRINT0("psr: name       : DOLLAR exp RC\n");
                            p = newNode(DOLLAR,&($1.location));
                            addLeftSon(p,$2);
                            $$     = p;
                            PSHOWTREE(p);
                         }
        | LC exp RC
                        ={  node *p;

                            PPRINT0("psr: name       : LC exp RC\n");
                            p = newNode(LC,&($1.location));
                            addLeftSon(p,$2);
                            $$     = p;
                            PSHOWTREE(p);
                         }
        ;

real    : REAL
                        ={  YYSTYPE tmp;

                            PPRINT0("psr: real        : REAL\n");
                            tmp.dval.value    = atof(yytext);
                            tmp.dval.location = $1.location;
                            $$                = tmp.dval;
                         }
        ;

string  : STRING
                        ={  YYSTYPE tmp;

                            PPRINT0("psr: string     : STRING\n");
                            yytext[strlen(yytext)-1] = '\0';
                            tmp.cval.value           = newStringId(&(yytext[1]),tempID);
                            tmp.cval.location        = $1.location;
                            $$                       = tmp.cval;
                         }
        ;

unit    : UNIT
                        ={  YYSTYPE tmp;

                            PPRINT0("psr: unit     : UNIT\n");
                            yytext[strlen(yytext)] = '\0';
                            tmp.cval.value           = newStringId(&(yytext[0]),tempID);
                            tmp.cval.location        = $1.location;
                            $$                       = tmp.cval;
                         }
        ;

if      : IF
                        ={  ignoreEOL += 1;
                         }
        ;

fi      : FI
                        ={  ignoreEOL -= 1;
                         }
        ;

while   : WHILE
                        ={  ignoreEOL += 1;
                         }
        ;

od      : OD
                        ={  ignoreEOL -= 1;
                         }
        ;

repeat  : REPEAT
                        ={  ignoreEOL += 1;
                         }
        ;

until   : UNTIL
                        ={  ignoreEOL -= 1;
                         }
        ;

lb      : LB
                        ={  ignoreEOL += 1;
                         }
        ;

rb      : RB
                        ={  ignoreEOL -= 1;
                         }
        ;

lp      : LP
                        ={  ignoreEOL += 1;
                         }
        ;

rp      : RP
                        ={  ignoreEOL -= 1;
                         }
        ;

%%

void
yyerror(m)
  char *m;
{
    node        N;
    node       *oldNode;

    N.location = yylval.tval.location;
    oldNode    = doingNode;
    doingNode  = &N;
    ignoreEOL  = 0;
    if (strlen(yytext)==1)
        if (yytext[0] == '\n')
            WerrprintfWithPos("Syntax error! Unexpected end of line");
        else if ((' '<=yytext[0]) && (yytext[0]<='~'))
            WerrprintfWithPos("Syntax error! Unexpected '%c'",yytext[0]);
        else if (yytext[0] == '\004')
            WerrprintfWithPos("Syntax error! Unexpected end of file");
        else
            WerrprintfWithPos("Syntax error! Unexpected '\\%03o'",yytext[0]);
    else
        WerrprintfWithPos("Syntax error! Unexpected \"%s\"",yytext);
    doingNode = oldNode;
}

/******************************************************************************
 *
 *	This routine should attempt to execute the string as if it was entered
 *	on the terminal.  The string must end with a newline.
 *		--from lexjunk.c
 ******************************************************************************/
int
execString(buffer)
  char *buffer;
{
    int yaccrtn;
    char *prevTempID;
    int   returnCode;
   char *oldbp;
   int   oldFromFile;
   int   oldFromString;
   node *oldcodeTree;
   int   oldworking;
   int   oldinteruption;

    if (0 < strlen(buffer) && buffer[strlen(buffer)-1] == '\n'){
	PPRINT0("execString: starting...\n");
      oldFromFile   = magicFromFile;		/* push existing file info */
      oldFromString = magicFromString;
      oldbp         = magicBp;
      oldcodeTree   = magicCodeTree;
      oldinteruption= magicInterrupt;
      oldworking    = magicWorking;

	prevTempID = tempID;
	tempID = newTempName("tmpExecStringID");

	yaccrtn = yyparse();
	PPRINT0("execString: ...parser gives code ");
	switch (yaccrtn){
	  case 0:
	    PPRINT0("0, go do it!\n");
	    if (magicCodeTree && magicCodeTree->flavour != ENDFILE){
		if (execS(magicCodeTree)){
		    returnCode = 0;
		}else{
		    returnCode = 1;
		}
	    }
	    break;
	  case 1:
	    PPRINT0("1, drop everything!\n");
	    ignoreEOL  = 0;		/* had a syntax error, drop it        */
	    returnCode = 1;
	    break;
	  case 2:
	    PPRINT0("2, incomplete expression, abort\n");
	    ignoreEOL  = 0;		/* had a syntax error, drop it       */
	    returnCode = 1;
	    break;
	}
	magicCodeTree = NULL;

	releaseWithId(tempID);
	free(tempID);
	tempID = prevTempID;


      magicFromFile   = oldFromFile; /* restore old file/kbd/string state*/
      magicFromString = oldFromString;
      magicBp         = oldbp;
      magicCodeTree   = oldcodeTree;
      magicInterrupt= oldinteruption;
      magicWorking    = oldworking;
	PPRINT0("execString: finishing...\n");
    }else{
	fprintf(stderr,"execString: string (=\"%s\") does not end with \\n\n",
		buffer);
	returnCode = 1;
    }
    return(returnCode);
}

/*******************************************************************************
*                                                                              *
*       This routine is triggered by terminal_main_loop when a carriage 
*       return is intercepted.  Notice the special return from 
*       yyparse (suspended) for dealing with the notifier driven environment.   
*		--from lexjunk.c
*******************************************************************************/
int
loadAndExec(buffer)
  char *buffer;
{
    int returnCode;

   PPRINT1("loadAndExec: have... \"%s\" starting...\n",buffer);
   magicFromFile     = 0;
   magicFromString   = 0;
   tempID       = "loadAndExec";
   magicBp           = buffer;
   switch (yyparse())
   { case 0:
	     PPRINT0("loadAndExec: ...parser gives code 0, go do it!\n");
	     if (magicCodeTree)
	     {
		if (magicCodeTree->flavour != ENDFILE)
		{  if (execS(magicCodeTree))
		      returnCode = 0;
		   else
		      returnCode = 1;
		   /*autoRedisplay();*/ /* if variable changed re-display */ 
		}
		magicCodeTree = NULL;
	     }
	     releaseTempCache();	/* of macro code trees */
	     releaseWithId(tempID);
	     releaseWithId("newMacro");
	     break;
     case 1:
	     PPRINT0("loadAndExec: ...parser gives code 1, drop everything!\n");
	     ignoreEOL  = 0;		/* had a syntax error, drop it        */
	     magicCodeTree   = NULL;
	     returnCode = 1;
	     releaseTempCache();
	     releaseWithId(tempID);
	     releaseWithId("newMacro");
	     break;
     case 2:
	     PPRINT0("loadAndExec: ...parser gives code 2, back for more\n");
	     returnCode = 0;
	     break;			/* suspended, back for more           */
   }
   return(returnCode);
}
