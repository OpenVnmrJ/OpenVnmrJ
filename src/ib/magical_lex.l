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
/*-------------------------------------------------------------------------
|
|       magic.lex.l
|
|       This is the description of the lexical tokens used in the system.
|       The magic.lex.l file is used by LEX to produce a lexer that feeds
|       the parser.  
|
|       NOTE:   The magicFileName (pointer) placed in yylval (one per token)
|               in parser stack) is not allocated (since we never get to
|               release it).  When this info is used to make nodes, this
|               name will be copied to allocated area (since nodes are
|               eventually released).
|
+---------------------------------------------------------------------------*/
%{
#undef input
#undef unput
#undef output

#include "node.h"               /* <-- magic.gram.h needs this */
#include "magic.gram.h"
#include "stack.h"
#include "magical_io.h"

/*#define DEBUG*/
static int      Lflag = 1;

#ifdef  DEBUG
#define LPRINT0(level,str) \
        if (Lflag >= level) fprintf(stderr,str)
#define LPRINT1(level,str, arg1) \
        if (Lflag >= level) fprintf(stderr,str,arg1)
#define LPRINT2(level,str, arg1, arg2) \
        if (Lflag >= level) fprintf(stderr,str,arg1,arg2)
#define LPRINT3(level,str, arg1, arg2, arg3) \
        if (Lflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define returnToken(T)  { yylval.tval.token           = T; \
			  yylval.tval.location.line   = magicLineNumber; \
			  yylval.tval.location.column = magicColumnNumber; \
			  yylval.tval.location.file   = magicFileName; \
			  if (Lflag) \
			     if (magicFileName) \
				LPRINT3(1,"lex: line %d, col %d of file %s\n" \
					,magicLineNumber \
					,magicColumnNumber \
					,magicFileName); \
			     else \
				LPRINT0(1,"lex: not in a file\n"); \
			  return(T); \
			}
#else
#define LPRINT0(level,str)
#define LPRINT1(level,str, arg1)
#define LPRINT2(level,str, arg1, arg2)
#define LPRINT3(level,str, arg1, arg2, arg3)
#define returnToken(T)  { yylval.tval.token           = T; \
			  yylval.tval.location.line   = magicLineNumber; \
			  yylval.tval.location.column = magicColumnNumber; \
			  yylval.tval.location.file   = magicFileName; \
			  return(T); \
			}
#endif

/*static char     fileName[1025];
static int      columnNumber;
static int      lineNumber;*/
static int oldColumnNumber;

static YYSTYPE  yylval;
static int (*inputFunc)() = 0;
static void (*outputFunc)() = 0;
static void (*unputFunc)() = 0;
static int input();

int magicFromString = 0;
int magicFromFile = 0;

int      ignoreEOL;

%}

%%
\001                            { LPRINT0(1,"lex: exhausted, suspending!\n");
				  return(-1);
				}
\004                            { LPRINT0(1,"lex: got \"^D\", ENDFILE\n");
				  returnToken(ENDFILE);
				}
\$\{                            { LPRINT0(1,"lex: got \"{\", LC\n");
				  returnToken(DOLLAR);
				}
\{                              { LPRINT0(1,"lex: got \"{\", LC\n");
				  returnToken(LC);
				}
\}                              { LPRINT0(1,"lex: got \"}\", RC\n");
				  returnToken(RC);
				}
":"                             { LPRINT1(1,"lex: got \"%s\", CL\n",yytext);
				  returnToken(CL);
				}
","                             { LPRINT1(1,"lex: got \"%s\", CM\n",yytext);
				  returnToken(CM);
				}
"="                             { LPRINT1(1,"lex: got \"%s\", EQ\n",yytext);
				  returnToken(EQ);
				}
"<>"                            { LPRINT1(1,"lex: got \"%s\", NE\n",yytext);
				  returnToken(NE);
				}
"<"                             { LPRINT1(1,"lex: got \"%s\", LT\n",yytext);
				  returnToken(LT);
				}
"<="                            { LPRINT1(1,"lex: got \"%s\", LE\n",yytext);
				  returnToken(LE);
				}
">"                             { LPRINT1(1,"lex: got \"%s\", GT\n",yytext);
				  returnToken(GT);
				}
">="                            { LPRINT1(1,"lex: got \"%s\", GE\n",yytext);
				  returnToken(GE);
				}
"+"                             { LPRINT1(1,"lex: got \"%s\", PLUS\n",yytext);
				  returnToken(PLUS);
				}
"-"                             { LPRINT1(1,"lex: got \"%s\", MINUS\n",yytext);
				  returnToken(MINUS);
				}
"*"                             { LPRINT1(1,"lex: got \"%s\", MULT\n",yytext);
				  returnToken(MULT);
				}
"/"                             { LPRINT1(1,"lex: got \"%s\", DIV\n",yytext);
				  returnToken(DIV);
				}
"("                             { LPRINT1(1,"lex: got \"%s\", LP\n",yytext);
				  returnToken(LP);
				}
"["                             { LPRINT1(1,"lex: got \"%s\", LB\n",yytext);
				  returnToken(LB);
				}
"]"                             { LPRINT1(1,"lex: got \"%s\", RB\n",yytext);
				  returnToken(RB);
				}
")"                             { LPRINT1(1,"lex: got \"%s\", RP\n",yytext);
				  returnToken(RP);
				}
ABORT|abort                     { LPRINT1(1,"lex: got \"%s\", ABORT\n",yytext);
				  returnToken(BOMB);
				}
ABORTOFF|abortoff		{ LPRINT1(1,"lex: got \"%s\",ABORTOFF\n",
					  yytext);
				  returnToken(IGNORE);
				}
ABORTON|aborton                 { LPRINT1(1,"lex: got \"%s\", ABORTON\n",
					  yytext);
				  returnToken(DONT);
				}
AND|and                         { LPRINT1(1,"lex: got \"%s\", AND\n",yytext);
				  returnToken(AND);
				}
BREAK|break                     { LPRINT1(1,"lex: got \"%s\", BREAK\n",yytext);
				  returnToken(BREAK);
				}
DO|do                           { LPRINT1(1,"lex: got \"%s\", DO\n",yytext);
				  returnToken(DO);
				}
ELSE|else                       { LPRINT1(1,"lex: got \"%s\", ELSE\n",yytext);
				  returnToken(ELSE);
				}
ENDIF|endif                     { LPRINT1(1,"lex: got \"%s\", FI\n",yytext);
				  returnToken(FI);
				}
ENDWHILE|endwhile               { LPRINT1(1,"lex: got \"%s\", OD\n",yytext);
				  returnToken(OD);
				}
RETURN|return                   { LPRINT1(1,"lex: got \"%s\", RETURN\n",yytext);
				  returnToken(EXIT);
				}
IF|if                           { LPRINT1(1,"lex: got \"%s\", IF\n",yytext);
				  returnToken(IF);
				}
MOD|mod|"%"                     { LPRINT1(1,"lex: got \"%s\", MOD\n",yytext);
				  returnToken(MOD);
				}
NOT|not                         { LPRINT1(1,"lex: got \"%s\", NOT\n",yytext);
				  returnToken(NOT);
				}
OR|or                           { LPRINT1(1,"lex: got \"%s\", OR\n",yytext);
				  returnToken(OR);
				}
REPEAT|repeat                   { LPRINT1(1,"lex: got \"%s\", REPEAT\n",yytext);
				  returnToken(REPEAT);
				}
\?                              { LPRINT1(1,"lex: got \"%s\", SHOW\n",yytext);
				  returnToken(SHOW);
				}
SIZE|size                       { LPRINT1(1,"lex: got \"%s\", SIZE\n",yytext);
				  returnToken(SIZE);
				}
SQRT|sqrt                       { LPRINT1(1,"lex: got \"%s\", SQRT\n",yytext);
				  returnToken(SQRT);
				}
TRUNC|trunc                     { LPRINT1(1,"lex: got \"%s\", TRUNC\n",yytext);
				  returnToken(TRUNC);
				}
TYPEOF|typeof                   { LPRINT1(1,"lex: got \"%s\", TYPEOF\n",yytext);
				  returnToken(TYPEOF);
				}
THEN|then                       { LPRINT1(1,"lex: got \"%s\", THEN\n",yytext);
				  returnToken(THEN);
				}
UNTIL|until                     { LPRINT1(1,"lex: got \"%s\", UNTIL\n",yytext);
				  returnToken(UNTIL);
				}
WHILE|while                     { LPRINT1(1,"lex: got \"%s\", WHILE\n",yytext);
				  returnToken(WHILE);
				}
[\$a-zA-Z_][\$a-zA-Z0-9_\#]*    {
				  LPRINT1(1,"lex: got \"%s\", ID\n",yytext);
				  returnToken(ID);
				}
(([0-9]+(\.[0-9]*)?)|([0-9]*\.[0-9]+))(([Ee][-+]?[0-9]+)?) {
				  LPRINT1(1,"lex: got \"%s\", REAL\n",yytext);
				  returnToken(REAL);
				}
(([0-9]+(\.[0-9]*)?)|([0-9]*\.[0-9]+))(([Ee][-+]?[0-9]+)?)[a-zA-Z]+ {
				  LPRINT1(1,"lex: got \"%s\", UNIT\n",yytext);
				  returnToken(UNIT);
				}
\'                              { int c,notDone,slashed;

				  slashed = 0;
				  notDone = 1;
				  while (notDone)
				  {  c = input();
				     if (c < ' ' && c != '\t')
				     {  yyleng    = 1;
					yytext[0] = '\n';
					yytext[1] = '\0';
					LPRINT0(1,"lex: unclosed string!\n");
					returnToken(EOL);
				     } 
				     if (slashed)
				     {  slashed = 0;
					switch (c)
					{ case 'b': c = '\b';
						    break;
					  case 'f': c = '\f';
						    break;
					  case 'n': c = '\n';
						    break;
					  case 'r': c = '\r';
						    break;
					  case 't': c = '\t';
						    break;
					  default:  break;
					}
					yytext[yyleng++] = c;
					yytext[yyleng]   = '\0';
				     }
				     else
				     {  if (c == '\\')
					   slashed = 1;
					else
					{  yytext[yyleng++] = c;
					   yytext[yyleng]   = '\0';
					   if (c == '\'')
					      notDone = 0;
					}
				     }
				  }
				  LPRINT1(1,"lex: got \"%s\", STRING\n",yytext);
				  returnToken(STRING);
				}
\"         /*"*/                { int c;

				  while ((c=input()) != '"')
				      if (c < ' ' && c != '\t')
				      {    yyleng    = 1;
					   yytext[0] = '\n';
					   yytext[1] = '\0';    
					   returnToken(EOL);
				      }
				  LPRINT0(1,"lex: got comment!\n");
				}
\\\n                            ;
\n                              { if (ignoreEOL)
				  {   LPRINT0(1,"lex: ignored \"\\n\", EOL\n");
				  }
				  else
				  {   LPRINT0(1,"lex: got \"\\n\", EOL\n");
				      returnToken(EOL);
				  }
				}
[ \t]                           ;
.                               returnToken(BAD);
%%

void
setMagicLflag(val)
  int val;
{
    Lflag = val;
}

static void
output(c)
  int c;
{
    if (outputFunc){
	(*outputFunc)(c);
    }else{
	fprintf(stderr,"%c", c);
    }
}

#ifdef NOTDEFINED
static int
input()
{
    if (inputFunc){
	return (*inputFunc)();
    }else{
	if (yysptr > yysbuf){
	    yytchar = *(--yysptr);
	}else{
	    yytchar = getc(yyin);
	}
	if (yytchar == '\n'){
	    yylineno++;
	}else if (yytchar == EOF){
	    yytchar = 0;
	}
	return yytchar;
    }
}
#endif /* NOTDEFINED */

static int input()
{
   LPRINT0(2,"input: starting...\n");
   if (magicFromFile)
   {
      LPRINT1(2,"input: ...from file \"%s\"\n",magicFileName);
      while (*magicMacroBp == '\0')
      {  register char *p;
	 register int   c;

	 LPRINT0(2,"input: ...out of characters!\n");
	 p = magicMacroBuffer;
	 while (1)
	    if ((c=getc(magicMacroFile)) == EOF)
	       if (p == magicMacroBuffer)
	       {
		  LPRINT0(2,"input: ...return '^D'\n");
		  return('\004');
	       }
	       else
	       {  *p = '\0';
		  LPRINT0(2,"input:    ...<done>\n");
		  break;
	       }
	    else
	    {  *p++ = c;
	       LPRINT1(2,"input:    ...'%c'\n",c);
	       if (c == '\n' || c == '\0')
	       {  *p = '\0';
		  LPRINT0(2,"input:    ...<done>\n");
		  break;
	       }
	    }
	 magicMacroBp = magicMacroBuffer;
      }
#ifdef DEBUG
      if (2 <= Lflag)
	 if (*magicMacroBp == '\n')
	    LPRINT0(2,"input: ...return '\\n'");
	 else if (' ' <= *magicMacroBp && *magicMacroBp <= '~')
	    LPRINT1(2,"input: ...return '%c'",*magicMacroBp);
	 else
	    LPRINT1(2,"input: ...return '\\%03o'",*magicMacroBp);
#endif
      if (*magicMacroBp == '\n')
      {  oldColumnNumber = magicColumnNumber;
	 magicColumnNumber    = 1;
	 magicLineNumber     += 1;
      }
      else
	 magicColumnNumber += 1;
      LPRINT2(2," (line %d, col %d)\n",magicLineNumber,magicColumnNumber);
      return(*magicMacroBp++);
   }
   else
   {  if (*magicBp == '\0')
      {  if (magicFromString)
	 {
	    LPRINT0(2,"input: ...end of string!\n");
	    return('\004');
	 }
	 else
	 {
	    LPRINT0(2,"input: ...suspending!\n");
	    return('\001');
	 }
      }
      else
      {
#ifdef   DEBUG
         if (2 <= Lflag)
	    if (magicFromString)
	    {  if (*magicBp == '\n')
		  LPRINT0(2,"input: ...return '\\n' (from string)\n");
	       else if (' ' <= *magicBp && *magicBp <= '~')
		  LPRINT1(2,"input: ...return '%c' (from string)\n",*magicBp);
	       else
		  LPRINT1(2,"input: ...return '\\%03o' (from string)\n",*magicBp);
	    }
	    else
	    {  if (*magicBp == '\n')
		  LPRINT0(2,"input: ...return '\\n' (from terminal)\n");
	       else if (' ' <= *magicBp && *magicBp <= '~')
		  LPRINT1(2,"input: ...return '%c' (from terminal)\n",*magicBp);
	       else
		  LPRINT1(2,"input: ...return '\\%03o' (from terminal)\n",*magicBp);
	    }
#endif
	 return(*magicBp++);
      }
   }
}

#ifdef NOTDEFINED
static void
unput(c)
  int c;
{
    if (unputFunc){
	(*unput)(c);
    }else{
	yytchar = c;
	if (yytchar == '\n'){
	    yylineno--;
	}
	*yysptr++ = yytchar;
    }
}
#endif /* NOTDEFINED */

static void
unput(c)
  int c;
{
    if (c != '\001'){
	if (magicFromFile){
	    magicColumnNumber -= 1;
	    if (c == '\n'){
		magicColumnNumber = oldColumnNumber;
		magicLineNumber  -= 1;
	    }else{
		magicColumnNumber -= 1;
	    }
	    *(--magicMacroBp) = c;
	}else{
	    *(--magicBp) = c;
	}
    }
}

int yywrap()
{  return(1);
}
