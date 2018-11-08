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
#include <inttypes.h>
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# ifndef YYLMAX 
# define YYLMAX BUFSIZ
# endif 
#ifndef __cplusplus
# define output(c) (void)putc(c,yyout)
#else
# define lex_output(c) (void)putc(c,yyout)
#endif

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
	int yyback(int *, int);
	int yyinput(void);
	int yylook(void);
	void yyoutput(int);
	int yyracc(int);
	int yyreject(void);
	void yyunput(int);
	int yylex(void);
#ifdef YYLEX_E
	void yywoutput(wchar_t);
	wchar_t yywinput(void);
#endif
#ifndef yyless
	int yyless(int);
#endif
#ifndef yywrap
	int yywrap(void);
#endif
#ifdef LEXDEBUG
	void allprint(char);
	void sprint(char *);
#endif
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
	void exit(int);
#ifdef __cplusplus
}
#endif

#endif
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
#ifndef __cplusplus
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#else
# define lex_input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#endif
#define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng;
#define YYISARRAY
char yytext[YYLMAX];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = (FILE *) 0, *yyout = (FILE *) 0;
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;

# line 2 "magic.lex.l"

# line 3 "magic.lex.l"
/*
*/
#pragma GCC diagnostic ignored "-Wmissing-braces"

# line 10 "magic.lex.l"
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
#undef input
#undef unput
#undef output

#include "node.h"               /* <-- magic.gram.h needs this */
#include "magic.gram.h"
#include "stack.h"

extern int input(void);
extern void unput(int);
extern void output(int);

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

# define YYNEWLINE 10
int yylex(){
int nstr;
#ifdef LINUX
  if ( ! yyin)
    yyin = stdin;
  if ( ! yyout)
    yyout = stdout;
#endif
while((nstr = yylook()) >= 0)
 switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:

# line 92 "magic.lex.l"
                           { LPRINT0("lex: exhausted, suspending!\n");
				  return(-1);
				}
break;
case 2:

# line 95 "magic.lex.l"
                           { LPRINT0("lex: got \"^D\", ENDFILE\n");
				  returnToken(ENDFILE);
				}
break;
case 3:

# line 98 "magic.lex.l"
                           { LPRINT0("lex: got \"{\", LC\n");
				  returnToken(DOLLAR);
				}
break;
case 4:

# line 101 "magic.lex.l"
                             { LPRINT0("lex: got \"{\", LC\n");
				  returnToken(LC);
				}
break;
case 5:

# line 104 "magic.lex.l"
                             { LPRINT0("lex: got \"}\", RC\n");
				  returnToken(RC);
				}
break;
case 6:

# line 107 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", CL\n",yytext);
				  returnToken(CL);
				}
break;
case 7:

# line 110 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", CM\n",yytext);
				  returnToken(CM);
				}
break;
case 8:

# line 113 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", EQ\n",yytext);
				  returnToken(EQ);
				}
break;
case 9:

# line 116 "magic.lex.l"
                           { LPRINT1("lex: got \"%s\", NE\n",yytext);
				  returnToken(NE);
				}
break;
case 10:

# line 119 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", LT\n",yytext);
				  returnToken(LT);
				}
break;
case 11:

# line 122 "magic.lex.l"
                           { LPRINT1("lex: got \"%s\", LE\n",yytext);
				  returnToken(LE);
				}
break;
case 12:

# line 125 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", GT\n",yytext);
				  returnToken(GT);
				}
break;
case 13:

# line 128 "magic.lex.l"
                           { LPRINT1("lex: got \"%s\", GE\n",yytext);
				  returnToken(GE);
				}
break;
case 14:

# line 131 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", PLUS\n",yytext);
				  returnToken(PLUS);
				}
break;
case 15:

# line 134 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", MINUS\n",yytext);
				  returnToken(MINUS);
				}
break;
case 16:

# line 137 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", MULT\n",yytext);
				  returnToken(MULT);
				}
break;
case 17:

# line 140 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", DIV\n",yytext);
				  returnToken(DIV);
				}
break;
case 18:

# line 143 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", LP\n",yytext);
				  returnToken(LP);
				}
break;
case 19:

# line 146 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", LB\n",yytext);
				  returnToken(LB);
				}
break;
case 20:

# line 149 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", RB\n",yytext);
				  returnToken(RB);
				}
break;
case 21:

# line 152 "magic.lex.l"
                            { LPRINT1("lex: got \"%s\", RP\n",yytext);
				  returnToken(RP);
				}
break;
case 22:

# line 155 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", ABORT\n",yytext);
				  returnToken(BOMB);
				}
break;
case 23:

# line 158 "magic.lex.l"
	{ LPRINT1("lex: got \"%s\",ABORTOFF\n",yytext);
				  returnToken(IGNORE);
				}
break;
case 24:

# line 161 "magic.lex.l"
                { LPRINT1("lex: got \"%s\", ABORTON\n",yytext);
				  returnToken(DONT);
				}
break;
case 25:

# line 164 "magic.lex.l"
                        { LPRINT1("lex: got \"%s\", AND\n",yytext);
				  returnToken(AND);
				}
break;
case 26:

# line 167 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", BREAK\n",yytext);
				  returnToken(BREAK);
				}
break;
case 27:

# line 170 "magic.lex.l"
                          { LPRINT1("lex: got \"%s\", DO\n",yytext);
				  returnToken(DO);
				}
break;
case 28:

# line 173 "magic.lex.l"
                      { LPRINT1("lex: got \"%s\", ELSE\n",yytext);
				  returnToken(ELSE);
				}
break;
case 29:

# line 176 "magic.lex.l"
                  { LPRINT1("lex: got \"%s\", ELSEIF\n",yytext);
				  returnToken(ELSEIF);
				}
break;
case 30:

# line 179 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", FI\n",yytext);
				  returnToken(FI);
				}
break;
case 31:

# line 182 "magic.lex.l"
              { LPRINT1("lex: got \"%s\", OD\n",yytext);
				  returnToken(OD);
				}
break;
case 32:

# line 185 "magic.lex.l"
                  { LPRINT1("lex: got \"%s\", RETURN\n",yytext);
				  returnToken(EXIT);
				}
break;
case 33:

# line 188 "magic.lex.l"
                          { LPRINT1("lex: got \"%s\", IF\n",yytext);
				  returnToken(IF);
				}
break;
case 34:

# line 191 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", MOD\n",yytext);
				  returnToken(MOD);
				}
break;
case 35:

# line 194 "magic.lex.l"
                        { LPRINT1("lex: got \"%s\", NOT\n",yytext);
				  returnToken(NOT);
				}
break;
case 36:

# line 197 "magic.lex.l"
                          { LPRINT1("lex: got \"%s\", OR\n",yytext);
				  returnToken(OR);
				}
break;
case 37:

# line 200 "magic.lex.l"
                  { LPRINT1("lex: got \"%s\", REPEAT\n",yytext);
				  returnToken(REPEAT);
				}
break;
case 38:

# line 203 "magic.lex.l"
                             { LPRINT1("lex: got \"%s\", SHOW\n",yytext);
				  returnToken(SHOW);
				}
break;
case 39:

# line 206 "magic.lex.l"
                      { LPRINT1("lex: got \"%s\", SIZE\n",yytext);
				  returnToken(SIZE);
				}
break;
case 40:

# line 209 "magic.lex.l"
                      { LPRINT1("lex: got \"%s\", SQRT\n",yytext);
				  returnToken(SQRT);
				}
break;
case 41:

# line 212 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", TRUNC\n",yytext);
				  returnToken(TRUNC);
				}
break;
case 42:

# line 215 "magic.lex.l"
                  { LPRINT1("lex: got \"%s\", TYPEOF\n",yytext);
				  returnToken(TYPEOF);
				}
break;
case 43:

# line 218 "magic.lex.l"
                      { LPRINT1("lex: got \"%s\", THEN\n",yytext);
				  returnToken(THEN);
				}
break;
case 44:

# line 221 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", UNTIL\n",yytext);
				  returnToken(UNTIL);
				}
break;
case 45:

# line 224 "magic.lex.l"
                    { LPRINT1("lex: got \"%s\", WHILE\n",yytext);
				  returnToken(WHILE);
				}
break;
case 46:

# line 227 "magic.lex.l"
   {
				  LPRINT1("lex: got \"%s\", ID\n",yytext);
				  returnToken(ID);
				}
break;
case 47:

# line 231 "magic.lex.l"
{
				  LPRINT1("lex: got \"%s\", REAL\n",yytext);
				  returnToken(REAL);
				}
break;
case 48:

# line 235 "magic.lex.l"
{
				  LPRINT1("lex: got \"%s\", UNIT\n",yytext);
				  returnToken(UNIT);
				}
break;
case 49:

# line 239 "magic.lex.l"
                             {
                                  int notDone,slashed;
                                  unsigned int c;

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
break;
case 50:

# line 311 "magic.lex.l"
                             {
                                  int notDone,slashed;
                                  unsigned int c;

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
break;
case 51:

# line 383 "magic.lex.l"
                             { int c;

				  while ((c=input()) != '"')
                                      if (c == '\n')
                                      {
                                         if (!ignoreEOL)
                                         {
				            returnToken(EOL);
                                         }
                                         else
                                         {
                                            break;
                                         }
                                      }
				      else if (c < ' ' && c != '\t')
				      {    yyleng    = 1;
					   yytext[0] = '\n';
					   yytext[1] = '\0';    
					   returnToken(EOL);
				      }
				  LPRINT0("lex: got comment!\n");
				}
break;
case 52:

# line 394 "magic.lex.l"
                           { int c;
				  while ((c=input()) != '\n')
                                     ;
				  LPRINT0("lex: got comment!\n");
                                  if (!ignoreEOL)
				     returnToken(EOL);
				}
break;
case 53:

# line 401 "magic.lex.l"
                           { int c;
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
break;
case 54:

# line 449 "magic.lex.l"
                           ;
break;
case 55:

# line 450 "magic.lex.l"
                             { if (ignoreEOL)
				  {   LPRINT0("lex: ignored \"\\n\", EOL\n");
				  }
				  else
				  {   LPRINT0("lex: got \"\\n\", EOL\n");
				      returnToken(EOL);
				  }
				}
break;
case 56:

# line 458 "magic.lex.l"
                      ;
break;
case 57:

# line 459 "magic.lex.l"
                              returnToken(BAD);
break;
case -1:
break;
default:
(void)fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
int yyvstop[] = {
0,

1,
57,
0, 

57,
0, 

2,
57,
0, 

56,
57,
0, 

55,
0, 

51,
57,
0, 

46,
57,
0, 

34,
57,
0, 

49,
57,
0, 

18,
57,
0, 

21,
57,
0, 

16,
57,
0, 

14,
57,
0, 

7,
57,
0, 

15,
57,
0, 

57,
0, 

17,
57,
0, 

47,
57,
0, 

6,
57,
0, 

10,
57,
0, 

8,
57,
0, 

12,
57,
0, 

38,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

19,
57,
0, 

57,
0, 

20,
57,
0, 

50,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

46,
57,
0, 

4,
57,
0, 

5,
57,
0, 

46,
0, 

3,
0, 

47,
0, 

53,
0, 

52,
0, 

47,
0, 

47,
0, 

48,
0, 

48,
0, 

11,
0, 

9,
0, 

13,
0, 

46,
0, 

46,
0, 

46,
0, 

27,
46,
0, 

46,
0, 

46,
0, 

33,
46,
0, 

46,
0, 

46,
0, 

36,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

54,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

47,
0, 

46,
0, 

25,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

34,
46,
0, 

35,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

28,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

39,
46,
0, 

40,
46,
0, 

43,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

28,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

22,
46,
0, 

26,
46,
0, 

46,
0, 

30,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

41,
46,
0, 

46,
0, 

44,
46,
0, 

45,
46,
0, 

22,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

46,
0, 

29,
46,
0, 

46,
0, 

37,
46,
0, 

32,
46,
0, 

42,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

24,
46,
0, 

46,
0, 

46,
0, 

46,
0, 

23,
46,
0, 

31,
46,
0, 
0};
# define YYTYPE unsigned char
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	1,4,	
1,4,	1,5,	1,4,	1,4,	
1,4,	1,4,	1,6,	1,7,	
1,4,	1,4,	0,0,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	0,0,	1,4,	1,8,	
1,4,	1,9,	1,10,	1,4,	
1,11,	1,12,	1,13,	1,14,	
1,15,	1,16,	1,17,	1,18,	
1,19,	1,20,	41,89,	19,62,	
0,0,	0,0,	0,0,	0,0,	
19,63,	0,0,	0,0,	1,21,	
1,4,	1,22,	1,23,	1,24,	
1,25,	1,4,	1,26,	1,27,	
1,28,	1,29,	1,30,	1,28,	
1,28,	1,28,	1,31,	1,28,	
1,28,	1,28,	1,32,	1,33,	
1,34,	1,28,	1,28,	1,35,	
1,36,	1,37,	1,38,	1,28,	
1,39,	1,28,	1,28,	1,28,	
1,40,	1,41,	1,42,	1,4,	
1,28,	1,43,	1,44,	1,45,	
1,28,	1,46,	1,47,	1,28,	
1,28,	1,28,	1,48,	1,28,	
1,28,	1,28,	1,49,	1,50,	
1,51,	1,28,	1,28,	1,52,	
1,53,	1,54,	1,55,	1,28,	
1,56,	1,28,	1,28,	1,28,	
1,57,	1,4,	1,58,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	2,4,	2,4,	2,5,	
2,4,	2,4,	2,4,	2,4,	
22,68,	22,69,	2,4,	2,4,	
24,70,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	0,0,	
2,4,	2,8,	0,0,	0,0,	
2,10,	2,4,	2,11,	2,12,	
2,13,	2,14,	0,0,	2,16,	
2,17,	2,18,	2,19,	18,61,	
18,61,	18,61,	18,61,	18,61,	
18,61,	18,61,	18,61,	18,61,	
18,61,	2,21,	2,4,	2,22,	
2,23,	2,24,	2,25,	2,4,	
0,0,	2,27,	2,28,	2,29,	
0,0,	2,28,	2,28,	2,28,	
2,31,	2,28,	2,28,	2,28,	
2,32,	2,33,	2,34,	2,28,	
2,28,	2,35,	2,36,	2,37,	
2,38,	2,28,	2,39,	2,28,	
2,28,	2,28,	2,40,	2,41,	
2,42,	2,4,	2,28,	2,43,	
2,44,	2,45,	2,28,	2,46,	
2,47,	2,28,	2,28,	2,28,	
2,48,	2,28,	2,28,	2,28,	
2,49,	2,50,	2,51,	2,28,	
2,28,	2,52,	2,53,	2,54,	
2,55,	2,28,	2,56,	2,28,	
2,28,	2,28,	2,57,	2,4,	
2,58,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	9,59,	
9,59,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	27,59,	28,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	27,59,	28,59,	
0,0,	0,0,	27,73,	0,0,	
0,0,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	0,0,	
0,0,	0,0,	0,0,	9,59,	
0,0,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,59,	
9,59,	9,59,	9,59,	9,60,	
20,64,	29,59,	20,65,	20,65,	
20,65,	20,65,	20,65,	20,65,	
20,65,	20,65,	20,65,	20,65,	
0,0,	29,59,	29,74,	0,0,	
0,0,	0,0,	0,0,	20,66,	
20,66,	20,66,	20,66,	20,67,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	20,66,	
20,66,	20,66,	20,66,	20,67,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	20,66,	20,66,	20,66,	
20,66,	26,59,	26,59,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
32,59,	0,0,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
32,59,	32,78,	0,0,	0,0,	
0,0,	0,0,	0,0,	26,59,	
26,71,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,72,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	0,0,	0,0,	0,0,	
0,0,	26,59,	0,0,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	26,59,	26,59,	26,59,	
26,59,	30,59,	31,59,	33,59,	
0,0,	34,59,	31,77,	35,59,	
0,0,	0,0,	35,81,	30,75,	
38,59,	30,76,	31,59,	33,59,	
33,79,	34,59,	36,59,	35,59,	
37,59,	34,80,	39,59,	44,59,	
38,87,	36,82,	37,84,	45,59,	
39,88,	46,59,	36,59,	0,0,	
37,59,	36,83,	39,59,	44,59,	
37,85,	47,59,	49,59,	45,59,	
48,59,	46,59,	50,59,	37,86,	
51,59,	52,59,	53,59,	55,59,	
54,59,	47,59,	49,59,	56,59,	
48,59,	59,59,	50,59,	44,90,	
51,59,	52,59,	53,59,	55,59,	
54,59,	73,59,	0,0,	56,59,	
73,109,	59,59,	72,59,	44,91,	
72,108,	0,0,	0,0,	0,0,	
74,59,	73,59,	46,74,	45,92,	
48,77,	0,0,	72,59,	47,93,	
52,97,	47,94,	71,59,	49,95,	
74,59,	53,98,	54,100,	50,96,	
76,59,	56,104,	76,111,	55,103,	
51,80,	53,99,	71,59,	71,107,	
54,101,	0,0,	0,0,	0,0,	
76,59,	0,0,	75,59,	54,102,	
61,61,	61,61,	61,61,	61,61,	
61,61,	61,61,	61,61,	61,61,	
61,61,	61,61,	75,59,	0,0,	
0,0,	0,0,	0,0,	75,110,	
0,0,	61,66,	61,66,	61,66,	
61,66,	61,67,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	61,66,	61,66,	61,66,	
61,66,	61,67,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	61,66,	
61,66,	61,66,	61,66,	64,64,	
64,64,	64,64,	64,64,	64,64,	
64,64,	64,64,	64,64,	64,64,	
64,64,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	66,66,	
66,66,	66,66,	66,66,	67,105,	
0,0,	67,105,	77,59,	0,0,	
67,106,	67,106,	67,106,	67,106,	
67,106,	67,106,	67,106,	67,106,	
67,106,	67,106,	77,59,	78,59,	
79,59,	78,112,	80,59,	81,59,	
82,59,	0,0,	0,0,	83,59,	
85,59,	0,0,	84,59,	78,59,	
79,59,	84,118,	80,59,	81,59,	
82,59,	81,114,	79,113,	83,59,	
85,59,	81,115,	84,59,	83,117,	
86,59,	87,59,	0,0,	85,119,	
82,116,	90,59,	91,59,	0,0,	
88,59,	92,59,	93,59,	94,59,	
86,59,	87,59,	86,120,	88,122,	
96,59,	90,59,	91,59,	87,121,	
88,59,	92,59,	93,59,	94,59,	
95,59,	97,59,	98,59,	100,59,	
96,59,	99,59,	101,59,	104,59,	
0,0,	0,0,	102,59,	103,59,	
95,59,	97,59,	98,59,	100,59,	
91,108,	99,59,	101,59,	104,59,	
92,124,	94,126,	102,59,	103,59,	
107,59,	108,59,	90,123,	110,59,	
109,137,	109,59,	110,138,	112,59,	
113,59,	115,59,	95,112,	93,125,	
107,59,	108,59,	100,131,	110,59,	
107,136,	109,59,	96,113,	112,59,	
113,59,	115,59,	104,135,	97,127,	
118,59,	0,0,	0,0,	97,128,	
115,142,	99,130,	0,0,	0,0,	
102,133,	101,132,	98,129,	0,0,	
118,145,	103,134,	105,106,	105,106,	
105,106,	105,106,	105,106,	105,106,	
105,106,	105,106,	105,106,	105,106,	
106,106,	106,106,	106,106,	106,106,	
106,106,	106,106,	106,106,	106,106,	
106,106,	106,106,	111,59,	114,59,	
116,59,	119,59,	114,141,	116,143,	
117,59,	111,139,	120,59,	0,0,	
121,59,	120,147,	111,59,	114,59,	
116,59,	119,146,	122,59,	121,148,	
117,59,	123,59,	120,59,	111,140,	
121,59,	124,59,	117,144,	125,59,	
122,149,	126,59,	122,59,	127,59,	
128,59,	123,59,	130,59,	129,59,	
131,59,	124,59,	133,59,	125,59,	
134,59,	126,59,	132,59,	127,59,	
128,59,	135,59,	130,59,	129,59,	
131,59,	136,59,	133,59,	137,59,	
134,59,	140,59,	132,59,	0,0,	
124,151,	135,59,	0,0,	140,165,	
137,162,	136,59,	125,152,	137,59,	
138,59,	140,59,	127,155,	136,161,	
126,153,	123,150,	129,143,	138,163,	
0,0,	133,158,	0,0,	0,0,	
138,59,	139,59,	0,0,	134,159,	
131,145,	139,164,	126,154,	128,156,	
130,144,	142,59,	132,157,	135,160,	
143,59,	139,59,	141,166,	141,59,	
144,59,	145,59,	146,59,	146,168,	
147,59,	142,59,	0,0,	150,59,	
143,59,	142,167,	148,59,	141,59,	
144,59,	145,59,	146,59,	149,59,	
147,59,	147,169,	149,171,	150,59,	
148,170,	151,59,	148,59,	153,59,	
152,59,	155,59,	154,59,	149,59,	
156,59,	158,59,	159,59,	160,59,	
157,59,	151,59,	161,59,	153,59,	
152,59,	155,59,	154,59,	162,59,	
156,59,	158,59,	159,59,	160,59,	
157,59,	0,0,	161,59,	161,178,	
163,59,	164,59,	0,0,	162,59,	
163,179,	0,0,	0,0,	166,59,	
155,175,	150,172,	167,59,	153,164,	
163,59,	164,59,	151,162,	152,173,	
154,174,	157,168,	160,171,	166,59,	
165,59,	168,59,	167,182,	170,59,	
159,170,	166,181,	158,177,	165,180,	
156,176,	171,59,	169,59,	172,59,	
165,59,	168,59,	169,183,	170,59,	
173,59,	175,59,	174,59,	176,59,	
179,59,	171,59,	169,59,	172,59,	
177,59,	0,0,	181,59,	182,59,	
173,59,	175,59,	174,59,	176,59,	
179,59,	178,59,	180,59,	183,59,	
177,59,	178,186,	181,59,	182,59,	
0,0,	184,59,	185,59,	187,59,	
180,188,	178,187,	180,59,	183,59,	
0,0,	0,0,	0,0,	0,0,	
173,179,	184,59,	185,59,	187,59,	
172,184,	174,185,	186,59,	189,59,	
177,183,	188,59,	186,191,	176,182,	
188,192,	0,0,	190,59,	175,181,	
191,59,	192,59,	186,59,	189,59,	
0,0,	188,59,	0,0,	0,0,	
0,0,	184,189,	190,59,	0,0,	
191,59,	192,59,	0,0,	0,0,	
185,190,	184,187,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	189,191,	
0,0,	0,0,	0,0,	0,0,	
0,0,	190,192,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+-1,	0,		0,	
yycrank+-255,	yysvec+1,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+4,
yycrank+0,	0,		yyvstop+6,
yycrank+0,	0,		yyvstop+9,
yycrank+0,	0,		yyvstop+12,
yycrank+0,	0,		yyvstop+14,
yycrank+476,	0,		yyvstop+17,
yycrank+0,	0,		yyvstop+20,
yycrank+0,	0,		yyvstop+23,
yycrank+0,	0,		yyvstop+26,
yycrank+0,	0,		yyvstop+29,
yycrank+0,	0,		yyvstop+32,
yycrank+0,	0,		yyvstop+35,
yycrank+0,	0,		yyvstop+38,
yycrank+0,	0,		yyvstop+41,
yycrank+255,	0,		yyvstop+44,
yycrank+9,	0,		yyvstop+46,
yycrank+554,	0,		yyvstop+49,
yycrank+0,	0,		yyvstop+52,
yycrank+203,	0,		yyvstop+55,
yycrank+0,	0,		yyvstop+58,
yycrank+207,	0,		yyvstop+61,
yycrank+0,	0,		yyvstop+64,
yycrank+642,	0,		yyvstop+67,
yycrank+456,	yysvec+26,	yyvstop+70,
yycrank+457,	yysvec+26,	yyvstop+73,
yycrank+535,	yysvec+26,	yyvstop+76,
yycrank+699,	yysvec+26,	yyvstop+79,
yycrank+700,	yysvec+26,	yyvstop+82,
yycrank+622,	yysvec+26,	yyvstop+85,
yycrank+701,	yysvec+26,	yyvstop+88,
yycrank+703,	yysvec+26,	yyvstop+91,
yycrank+705,	yysvec+26,	yyvstop+94,
yycrank+716,	yysvec+26,	yyvstop+97,
yycrank+718,	yysvec+26,	yyvstop+100,
yycrank+710,	yysvec+26,	yyvstop+103,
yycrank+720,	yysvec+26,	yyvstop+106,
yycrank+0,	0,		yyvstop+109,
yycrank+40,	0,		yyvstop+112,
yycrank+0,	0,		yyvstop+114,
yycrank+0,	0,		yyvstop+117,
yycrank+721,	yysvec+26,	yyvstop+120,
yycrank+725,	yysvec+26,	yyvstop+123,
yycrank+727,	yysvec+26,	yyvstop+126,
yycrank+735,	yysvec+26,	yyvstop+129,
yycrank+738,	yysvec+26,	yyvstop+132,
yycrank+736,	yysvec+26,	yyvstop+135,
yycrank+740,	yysvec+26,	yyvstop+138,
yycrank+742,	yysvec+26,	yyvstop+141,
yycrank+743,	yysvec+26,	yyvstop+144,
yycrank+744,	yysvec+26,	yyvstop+147,
yycrank+746,	yysvec+26,	yyvstop+150,
yycrank+745,	yysvec+26,	yyvstop+153,
yycrank+749,	yysvec+26,	yyvstop+156,
yycrank+0,	0,		yyvstop+159,
yycrank+0,	0,		yyvstop+162,
yycrank+751,	yysvec+26,	yyvstop+165,
yycrank+0,	0,		yyvstop+167,
yycrank+820,	0,		yyvstop+169,
yycrank+0,	0,		yyvstop+171,
yycrank+0,	0,		yyvstop+173,
yycrank+895,	yysvec+61,	yyvstop+175,
yycrank+0,	yysvec+20,	yyvstop+177,
yycrank+888,	0,		yyvstop+179,
yycrank+968,	yysvec+66,	yyvstop+181,
yycrank+0,	0,		yyvstop+183,
yycrank+0,	0,		yyvstop+185,
yycrank+0,	0,		yyvstop+187,
yycrank+780,	yysvec+26,	yyvstop+189,
yycrank+764,	yysvec+26,	yyvstop+191,
yycrank+759,	yysvec+26,	yyvstop+193,
yycrank+770,	yysvec+26,	yyvstop+195,
yycrank+800,	yysvec+26,	yyvstop+198,
yycrank+786,	yysvec+26,	yyvstop+200,
yycrank+948,	yysvec+26,	yyvstop+202,
yycrank+961,	yysvec+26,	yyvstop+205,
yycrank+962,	yysvec+26,	yyvstop+207,
yycrank+964,	yysvec+26,	yyvstop+209,
yycrank+965,	yysvec+26,	yyvstop+212,
yycrank+966,	yysvec+26,	yyvstop+214,
yycrank+969,	yysvec+26,	yyvstop+216,
yycrank+972,	yysvec+26,	yyvstop+218,
yycrank+970,	yysvec+26,	yyvstop+220,
yycrank+986,	yysvec+26,	yyvstop+222,
yycrank+987,	yysvec+26,	yyvstop+224,
yycrank+994,	yysvec+26,	yyvstop+226,
yycrank+0,	0,		yyvstop+228,
yycrank+991,	yysvec+26,	yyvstop+230,
yycrank+992,	yysvec+26,	yyvstop+232,
yycrank+995,	yysvec+26,	yyvstop+234,
yycrank+996,	yysvec+26,	yyvstop+236,
yycrank+997,	yysvec+26,	yyvstop+238,
yycrank+1010,	yysvec+26,	yyvstop+240,
yycrank+1002,	yysvec+26,	yyvstop+242,
yycrank+1011,	yysvec+26,	yyvstop+244,
yycrank+1012,	yysvec+26,	yyvstop+246,
yycrank+1015,	yysvec+26,	yyvstop+248,
yycrank+1013,	yysvec+26,	yyvstop+250,
yycrank+1016,	yysvec+26,	yyvstop+252,
yycrank+1020,	yysvec+26,	yyvstop+254,
yycrank+1021,	yysvec+26,	yyvstop+256,
yycrank+1017,	yysvec+26,	yyvstop+258,
yycrank+1090,	0,		0,	
yycrank+1100,	yysvec+66,	yyvstop+260,
yycrank+1034,	yysvec+26,	yyvstop+262,
yycrank+1035,	yysvec+26,	yyvstop+264,
yycrank+1039,	yysvec+26,	yyvstop+267,
yycrank+1037,	yysvec+26,	yyvstop+269,
yycrank+1092,	yysvec+26,	yyvstop+271,
yycrank+1041,	yysvec+26,	yyvstop+273,
yycrank+1042,	yysvec+26,	yyvstop+276,
yycrank+1093,	yysvec+26,	yyvstop+279,
yycrank+1043,	yysvec+26,	yyvstop+281,
yycrank+1094,	yysvec+26,	yyvstop+283,
yycrank+1098,	yysvec+26,	yyvstop+285,
yycrank+1058,	yysvec+26,	yyvstop+287,
yycrank+1095,	yysvec+26,	yyvstop+289,
yycrank+1100,	yysvec+26,	yyvstop+291,
yycrank+1102,	yysvec+26,	yyvstop+293,
yycrank+1108,	yysvec+26,	yyvstop+295,
yycrank+1111,	yysvec+26,	yyvstop+297,
yycrank+1115,	yysvec+26,	yyvstop+299,
yycrank+1117,	yysvec+26,	yyvstop+301,
yycrank+1119,	yysvec+26,	yyvstop+303,
yycrank+1121,	yysvec+26,	yyvstop+305,
yycrank+1122,	yysvec+26,	yyvstop+307,
yycrank+1125,	yysvec+26,	yyvstop+309,
yycrank+1124,	yysvec+26,	yyvstop+311,
yycrank+1126,	yysvec+26,	yyvstop+313,
yycrank+1132,	yysvec+26,	yyvstop+315,
yycrank+1128,	yysvec+26,	yyvstop+317,
yycrank+1130,	yysvec+26,	yyvstop+319,
yycrank+1135,	yysvec+26,	yyvstop+321,
yycrank+1139,	yysvec+26,	yyvstop+323,
yycrank+1141,	yysvec+26,	yyvstop+325,
yycrank+1154,	yysvec+26,	yyvstop+327,
yycrank+1167,	yysvec+26,	yyvstop+330,
yycrank+1143,	yysvec+26,	yyvstop+332,
yycrank+1181,	yysvec+26,	yyvstop+334,
yycrank+1175,	yysvec+26,	yyvstop+336,
yycrank+1178,	yysvec+26,	yyvstop+338,
yycrank+1182,	yysvec+26,	yyvstop+341,
yycrank+1183,	yysvec+26,	yyvstop+344,
yycrank+1184,	yysvec+26,	yyvstop+347,
yycrank+1186,	yysvec+26,	yyvstop+349,
yycrank+1192,	yysvec+26,	yyvstop+351,
yycrank+1197,	yysvec+26,	yyvstop+353,
yycrank+1189,	yysvec+26,	yyvstop+355,
yycrank+1203,	yysvec+26,	yyvstop+357,
yycrank+1206,	yysvec+26,	yyvstop+359,
yycrank+1205,	yysvec+26,	yyvstop+362,
yycrank+1208,	yysvec+26,	yyvstop+364,
yycrank+1207,	yysvec+26,	yyvstop+366,
yycrank+1210,	yysvec+26,	yyvstop+368,
yycrank+1214,	yysvec+26,	yyvstop+370,
yycrank+1211,	yysvec+26,	yyvstop+372,
yycrank+1212,	yysvec+26,	yyvstop+374,
yycrank+1213,	yysvec+26,	yyvstop+376,
yycrank+1216,	yysvec+26,	yyvstop+378,
yycrank+1221,	yysvec+26,	yyvstop+381,
yycrank+1230,	yysvec+26,	yyvstop+384,
yycrank+1231,	yysvec+26,	yyvstop+386,
yycrank+1250,	yysvec+26,	yyvstop+389,
yycrank+1237,	yysvec+26,	yyvstop+391,
yycrank+1240,	yysvec+26,	yyvstop+393,
yycrank+1251,	yysvec+26,	yyvstop+395,
yycrank+1260,	yysvec+26,	yyvstop+398,
yycrank+1253,	yysvec+26,	yyvstop+400,
yycrank+1259,	yysvec+26,	yyvstop+403,
yycrank+1261,	yysvec+26,	yyvstop+406,
yycrank+1266,	yysvec+26,	yyvstop+409,
yycrank+1268,	yysvec+26,	yyvstop+411,
yycrank+1267,	yysvec+26,	yyvstop+413,
yycrank+1269,	yysvec+26,	yyvstop+415,
yycrank+1274,	yysvec+26,	yyvstop+417,
yycrank+1283,	yysvec+26,	yyvstop+419,
yycrank+1270,	yysvec+26,	yyvstop+421,
yycrank+1284,	yysvec+26,	yyvstop+424,
yycrank+1276,	yysvec+26,	yyvstop+426,
yycrank+1277,	yysvec+26,	yyvstop+429,
yycrank+1285,	yysvec+26,	yyvstop+432,
yycrank+1291,	yysvec+26,	yyvstop+435,
yycrank+1292,	yysvec+26,	yyvstop+437,
yycrank+1308,	yysvec+26,	yyvstop+439,
yycrank+1293,	yysvec+26,	yyvstop+441,
yycrank+1311,	yysvec+26,	yyvstop+444,
yycrank+1309,	yysvec+26,	yyvstop+446,
yycrank+1316,	yysvec+26,	yyvstop+448,
yycrank+1318,	yysvec+26,	yyvstop+450,
yycrank+1319,	yysvec+26,	yyvstop+453,
0,	0,	0};
struct yywork *yytop = yycrank+1417;
struct yysvf *yybgin = yysvec+1;
char yymatch[] = {
  0,   1,   1,   1,   1,   1,   1,   1, 
  1,   9,  10,   1,   1,   9,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  9,   1,   1,  35,  36,   1,   1,   1, 
  1,   1,   1,  43,   1,  43,   1,   1, 
 48,  48,  48,  48,  48,  48,  48,  48, 
 48,  48,   1,   1,   1,   1,   1,   1, 
  1,  65,  65,  65,  65,  69,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,   1,   1,   1,   1,  36, 
  1,  65,  65,  65,  65,  69,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,  65,  65,  65,  65,  65, 
 65,  65,  65,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
  1,   1,   1,   1,   1,   1,   1,   1, 
0};
char yyextra[] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/*	Copyright (c) 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
char yysbuf[YYLMAX];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
#if defined(__cplusplus) || defined(__STDC__)
int yylook(void)
#else
yylook()
#endif
{
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych, yyfirst;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank && !yyfirst){  /* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
#ifndef __cplusplus
			*yylastch++ = yych = input();
#else
			*yylastch++ = yych = lex_input();
#endif
#ifdef YYISARRAY
			if(yylastch > &yytext[YYLMAX]) {
				fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
				exit(1);
			}
#else
			if (yylastch >= &yytext[ yytextsz ]) {
				int	x = yylastch - yytext;

				yytextsz += YYTEXTSZINC;
				if (yytext == yy_tbuf) {
				    yytext = (char *) malloc(yytextsz);
				    memcpy(yytext, yy_tbuf, sizeof (yy_tbuf));
				}
				else
				    yytext = (char *) realloc(yytext, yytextsz);
				if (!yytext) {
				    fprintf(yyout,
					"Cannot realloc yytext\n");
				    exit(1);
				}
				yylastch = yytext + x;
			}
#endif
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (uintptr_t)yyt > (uintptr_t)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((uintptr_t)yyt < (uintptr_t)yycrank) {	/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					if(lsp > &yylstate[YYLMAX]) {
						fprintf(yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
#ifndef __cplusplus
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
#else
		yyprevious = yytext[0] = lex_input();
		if (yyprevious>0)
			lex_output(yyprevious);
#endif
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
#if defined(__cplusplus) || defined(__STDC__)
int yyback(int *p, int m)
#else
yyback(p, m)
	int *p;
#endif
{
	if (p==0) return(0);
	while (*p) {
		if (*p++ == m)
			return(1);
	}
	return(0);
}
	/* the following are only used in the lex library */
#if defined(__cplusplus) || defined(__STDC__)
int yyinput(void)
#else
yyinput()
#endif
{
#ifndef __cplusplus
	return(input());
#else
	return(lex_input());
#endif
	}
#if defined(__cplusplus) || defined(__STDC__)
void yyoutput(int c)
#else
yyoutput(c)
  int c; 
#endif
{
#ifndef __cplusplus
	output(c);
#else
	lex_output(c);
#endif
	}
#if defined(__cplusplus) || defined(__STDC__)
void yyunput(int c)
#else
yyunput(c)
   int c; 
#endif
{
	unput(c);
	}
