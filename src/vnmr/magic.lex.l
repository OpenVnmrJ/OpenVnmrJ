/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|
|       magic.lex.l
|
|       This is the description of the lexical tokens used in the system.
|       The magic.lex.l file is used by LEX to produce a lexer that feeds
|       the parser.  
|
|       NOTE:   The fileName (pointer) placed in yylval (one per token)
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

extern int input(void);
extern void unput(int);

#ifdef LINUX
#ifndef YYLMAX
#define YYLMAX 500
#endif
#endif

#ifdef  DEBUG
#define LPRINT0(str) \
        if (Lflag) fprintf(stderr,str)
#define LPRINT1(str, arg1) \
        if (Lflag) fprintf(stderr,str,arg1)
#define LPRINT3(str, arg1, arg2, arg3) \
        if (Lflag) fprintf(stderr,str,arg1,arg2,arg3)
#define returnToken(T)  { yylval.tval.token           = T; \
			  yylval.tval.location.line   = lineNumber; \
			  yylval.tval.location.column = columnNumber; \
			  yylval.tval.location.file   = fileName; \
			  if (Lflag) \
			     if (fileName) \
				LPRINT3("lex: line %d, column %d of file %s\n" \
				       ,lineNumber,columnNumber,fileName); \
			     else \
				LPRINT0("lex: not in a file\n"); \
			  return(T); \
			}
#else 
#define LPRINT0(str)
#define LPRINT1(str, arg1)
#define LPRINT3(str, arg1, arg2, arg3)
#define returnToken(T)  { yylval.tval.token           = T; \
			  yylval.tval.location.line   = lineNumber; \
			  yylval.tval.location.column = columnNumber; \
			  yylval.tval.location.file   = fileName; \
			  return(T); \
			}
#endif 

extern char     fileName[];
extern int      columnNumber;
extern int      lineNumber;
extern int      ignoreEOL;
extern int      fromFile;
extern int      Lflag;
extern YYSTYPE  yylval;

%}

%%
%{
#ifdef LINUX
  if ( ! yyin)
    yyin = stdin;
  if ( ! yyout)
    yyout = stdout;
#endif
%}
\001                            { LPRINT0("lex: exhausted, suspending!\n");
				  return(-1);
				}
\004                            { LPRINT0("lex: got \"^D\", ENDFILE\n");
				  returnToken(ENDFILE);
				}
\$\{                            { LPRINT0("lex: got \"{\", LC\n");
				  returnToken(DOLLAR);
				}
\{                              { LPRINT0("lex: got \"{\", LC\n");
				  returnToken(LC);
				}
\}                              { LPRINT0("lex: got \"}\", RC\n");
				  returnToken(RC);
				}
":"                             { LPRINT1("lex: got \"%s\", CL\n",yytext);
				  returnToken(CL);
				}
","                             { LPRINT1("lex: got \"%s\", CM\n",yytext);
				  returnToken(CM);
				}
"="                             { LPRINT1("lex: got \"%s\", EQ\n",yytext);
				  returnToken(EQ);
				}
"<>"                            { LPRINT1("lex: got \"%s\", NE\n",yytext);
				  returnToken(NE);
				}
"<"                             { LPRINT1("lex: got \"%s\", LT\n",yytext);
				  returnToken(LT);
				}
"<="                            { LPRINT1("lex: got \"%s\", LE\n",yytext);
				  returnToken(LE);
				}
">"                             { LPRINT1("lex: got \"%s\", GT\n",yytext);
				  returnToken(GT);
				}
">="                            { LPRINT1("lex: got \"%s\", GE\n",yytext);
				  returnToken(GE);
				}
"+"                             { LPRINT1("lex: got \"%s\", PLUS\n",yytext);
				  returnToken(PLUS);
				}
"-"                             { LPRINT1("lex: got \"%s\", MINUS\n",yytext);
				  returnToken(MINUS);
				}
"*"                             { LPRINT1("lex: got \"%s\", MULT\n",yytext);
				  returnToken(MULT);
				}
"/"                             { LPRINT1("lex: got \"%s\", DIV\n",yytext);
				  returnToken(DIV);
				}
"("                             { LPRINT1("lex: got \"%s\", LP\n",yytext);
				  returnToken(LP);
				}
"["                             { LPRINT1("lex: got \"%s\", LB\n",yytext);
				  returnToken(LB);
				}
"]"                             { LPRINT1("lex: got \"%s\", RB\n",yytext);
				  returnToken(RB);
				}
")"                             { LPRINT1("lex: got \"%s\", RP\n",yytext);
				  returnToken(RP);
				}
ABORT|abort                     { LPRINT1("lex: got \"%s\", ABORT\n",yytext);
				  returnToken(BOMB);
				}
ABORTOFF|abortoff		{ LPRINT1("lex: got \"%s\",ABORTOFF\n",yytext);
				  returnToken(IGNORE);
				}
ABORTON|aborton                 { LPRINT1("lex: got \"%s\", ABORTON\n",yytext);
				  returnToken(DONT);
				}
AND|and                         { LPRINT1("lex: got \"%s\", AND\n",yytext);
				  returnToken(AND);
				}
BREAK|break                     { LPRINT1("lex: got \"%s\", BREAK\n",yytext);
				  returnToken(BREAK);
				}
DO|do                           { LPRINT1("lex: got \"%s\", DO\n",yytext);
				  returnToken(DO);
				}
ELSE|else                       { LPRINT1("lex: got \"%s\", ELSE\n",yytext);
				  returnToken(ELSE);
				}
ELSEIF|elseif                   { LPRINT1("lex: got \"%s\", ELSEIF\n",yytext);
				  returnToken(ELSEIF);
				}
ENDIF|endif                     { LPRINT1("lex: got \"%s\", FI\n",yytext);
				  returnToken(FI);
				}
ENDWHILE|endwhile               { LPRINT1("lex: got \"%s\", OD\n",yytext);
				  returnToken(OD);
				}
RETURN|return                   { LPRINT1("lex: got \"%s\", RETURN\n",yytext);
				  returnToken(EXIT);
				}
IF|if                           { LPRINT1("lex: got \"%s\", IF\n",yytext);
				  returnToken(IF);
				}
MOD|mod|"%"                     { LPRINT1("lex: got \"%s\", MOD\n",yytext);
				  returnToken(MOD);
				}
NOT|not                         { LPRINT1("lex: got \"%s\", NOT\n",yytext);
				  returnToken(NOT);
				}
OR|or                           { LPRINT1("lex: got \"%s\", OR\n",yytext);
				  returnToken(OR);
				}
REPEAT|repeat                   { LPRINT1("lex: got \"%s\", REPEAT\n",yytext);
				  returnToken(REPEAT);
				}
\?                              { LPRINT1("lex: got \"%s\", SHOW\n",yytext);
				  returnToken(SHOW);
				}
SIZE|size                       { LPRINT1("lex: got \"%s\", SIZE\n",yytext);
				  returnToken(SIZE);
				}
SQRT|sqrt                       { LPRINT1("lex: got \"%s\", SQRT\n",yytext);
				  returnToken(SQRT);
				}
TRUNC|trunc                     { LPRINT1("lex: got \"%s\", TRUNC\n",yytext);
				  returnToken(TRUNC);
				}
TYPEOF|typeof                   { LPRINT1("lex: got \"%s\", TYPEOF\n",yytext);
				  returnToken(TYPEOF);
				}
THEN|then                       { LPRINT1("lex: got \"%s\", THEN\n",yytext);
				  returnToken(THEN);
				}
UNTIL|until                     { LPRINT1("lex: got \"%s\", UNTIL\n",yytext);
				  returnToken(UNTIL);
				}
WHILE|while                     { LPRINT1("lex: got \"%s\", WHILE\n",yytext);
				  returnToken(WHILE);
				}
[\$a-zA-Z_][\$a-zA-Z0-9_\#]*    {
				  LPRINT1("lex: got \"%s\", ID\n",yytext);
				  returnToken(ID);
				}
(([0-9]+(\.[0-9]*)?)|([0-9]*\.[0-9]+))(([Ee][-+]?[0-9]+)?) {
				  LPRINT1("lex: got \"%s\", REAL\n",yytext);
				  returnToken(REAL);
				}
(([0-9]+(\.[0-9]*)?)|([0-9]*\.[0-9]+))(([Ee][-+]?[0-9]+)?)[a-zA-Z]+ {
				  LPRINT1("lex: got \"%s\", UNIT\n",yytext);
				  returnToken(UNIT);
				}
\'                              { int c,notDone,slashed;

				  slashed = 0;
				  notDone = 1;
				  while (notDone)
				  {  c = input();
				     if ((c < ' ') && (c != '\t') && (c != '\n') && (c != '\r'))
				     {  yyleng    = 1;
					yytext[0] = '\n';
					yytext[1] = '\0';
					LPRINT0("lex: unclosed string!\n");
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
					  case '\n': c = ' ';
						    break;
					  default:  break;
					}
                                        if (yyleng < YYLMAX - 1)
                                        {
					   yytext[yyleng++] = c;
					   yytext[yyleng]   = '\0';
                                        }
                                        else
                                        {
				           yyleng    = 2;
					   yytext[0] = '\n';
					   yytext[1] = '\n';
					   yytext[2] = '\0';
					   LPRINT0("lex: unclosed string!\n");
					   returnToken(EOL);
                                        }
				     }
				     else
				     {  if (c == '\\')
					   slashed = 1;
					else
					{
                                           if (yyleng < YYLMAX-1)
                                           {
                                              yytext[yyleng++] = c;
					      yytext[yyleng]   = '\0';
                                           }
                                           else
                                           {
				              yyleng    = 2;
					      yytext[0] = '\n';
					      yytext[1] = '\n';
					      yytext[2] = '\0';
					      LPRINT0("lex: unclosed string!\n");
					      returnToken(EOL);
                                           }
					   if (c == '\'')
					      notDone = 0;
					}
				     }
				  }
				  LPRINT1("lex: got \"%s\", STRING\n",yytext);
				  returnToken(STRING);
				}
\`                              { int c,notDone,slashed;

				  slashed = 0;
				  notDone = 1;
				  while (notDone)
				  {  c = input();
				     if ((c < ' ') && (c != '\t') && (c != '\n') && (c != '\r'))
				     {  yyleng    = 1;
					yytext[0] = '\n';
					yytext[1] = '\0';
					LPRINT0("lex: unclosed string!\n");
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
					  case '\n': c = ' ';
						    break;
					  default:  break;
					}
                                        if (yyleng < YYLMAX - 1)
                                        {
					   yytext[yyleng++] = c;
					   yytext[yyleng]   = '\0';
                                        }
                                        else
                                        {
				           yyleng    = 2;
					   yytext[0] = '\n';
					   yytext[1] = '\n';
					   yytext[2] = '\0';
					   LPRINT0("lex: unclosed string!\n");
					   returnToken(EOL);
                                        }
				     }
				     else
				     {  if (c == '\\')
					   slashed = 1;
					else
					{
                                           if (yyleng < YYLMAX-1)
                                           {
                                              yytext[yyleng++] = c;
					      yytext[yyleng]   = '\0';
                                           }
                                           else
                                           {
				              yyleng    = 2;
					      yytext[0] = '\n';
					      yytext[1] = '\n';
					      yytext[2] = '\0';
					      LPRINT0("lex: unclosed string!\n");
					      returnToken(EOL);
                                           }
					   if (c == '`')
					      notDone = 0;
					}
				     }
				  }
				  LPRINT1("lex: got \"%s\", STRING\n",yytext);
				  returnToken(STRING);
				}
\"                              { int c;

				  while ((c=input()) != '"')
				      if (c < ' ' && c != '\t')
				      {    yyleng    = 1;
					   yytext[0] = '\n';
					   yytext[1] = '\0';    
					   returnToken(EOL);
				      }
				  LPRINT0("lex: got comment!\n");
				}
"//"                            { int c;
				  while ((c=input()) != '\n')
                                     ;
				  LPRINT0("lex: got comment!\n");
                                  if (!ignoreEOL)
				     returnToken(EOL);
				}
"/*"                            { int c;
                                  int done = 0;
                                  
                                  if (fromFile)
                                  {
                                    while (!done)
                                    {
				     while ((c=input()) != '*')
				      if (c == '\004')
				      {    yyleng    = 2;
					   yytext[0] = '\n';
					   yytext[1] = '\004';
					   yytext[2] = '\0';    
					   returnToken(BAD);
				      }
				     while ((c=input()) == '*')
                                      ;
				     if (c == '/')
                                        done = 1;
                                    }
                                  }
                                  else
                                  {
                                    while (!done)
                                    {
				      while ( ((c=input()) != '*') && (c != '\n') )
				        ;
                                      if (c == '\n')
                                      {
                                        done = 1;
				        returnToken(EOL);
                                      }
                                      else
                                      {
				        while ((c=input()) == '*')
                                         ;
				        if (c == '/')
                                           done = 1;
                                        else if (c == '\n')
                                        {
                                          done = 1;
				          returnToken(EOL);
                                        }
                                      }
                                    }
                                  }
				  LPRINT0("lex: got comment!\n");
				}
\\\n                            ;
\n                              { if (ignoreEOL)
				  {   LPRINT0("lex: ignored \"\\n\", EOL\n");
				  }
				  else
				  {   LPRINT0("lex: got \"\\n\", EOL\n");
				      returnToken(EOL);
				  }
				}
[ \t\015]                       ;
.                               returnToken(BAD);
