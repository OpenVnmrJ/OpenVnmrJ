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
# define NLSTATE magic_yyprevious=YYNEWLINE
# define BEGIN magic_yybgin = magic_yysvec + 1 +
# define INITIAL 0
# define YYLERR magic_yysvec
# define YYSTATE (magic_yyestate-magic_yysvec-1)
# define YYOPTIM 1
# ifndef YYLMAX 
# define YYLMAX BUFSIZ
# endif 
#ifndef __cplusplus
# define output(c) (void)putc(c,magic_yyout)
#else
# define lex_output(c) (void)putc(c,magic_yyout)
#endif

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
	int magic_yyback(int *, int);
	int magic_yyinput(void);
	int magic_yylook(void);
	void magic_yyoutput(int);
	int yyracc(int);
	int yyreject(void);
	void magic_yyunput(int);
	int magic_yylex(void);
#ifdef YYLEX_E
	void yywoutput(wchar_t);
	wchar_t yywinput(void);
#endif
#ifndef yyless
	int yyless(int);
#endif
#ifndef magic_yywrap
	int magic_yywrap(void);
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
# define unput(c) {magic_yytchar= (c);if(magic_yytchar=='\n')magic_yylineno--;*magic_yysptr++=magic_yytchar;}
# define yymore() (magic_yymorfg=1)
#ifndef __cplusplus
# define input() (((magic_yytchar=magic_yysptr>magic_yysbuf?U(*--magic_yysptr):getc(magic_yyin))==10?(magic_yylineno++,magic_yytchar):magic_yytchar)==EOF?0:magic_yytchar)
#else
# define lex_input() (((magic_yytchar=magic_yysptr>magic_yysbuf?U(*--magic_yysptr):getc(magic_yyin))==10?(magic_yylineno++,magic_yytchar):magic_yytchar)==EOF?0:magic_yytchar)
#endif
#define ECHO fprintf(magic_yyout, "%s",magic_yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int magic_yyleng;
#define YYISARRAY
char magic_yytext[YYLMAX];
int magic_yymorfg;
extern char *magic_yysptr, magic_yysbuf[];
int magic_yytchar;
FILE *magic_yyin = {(FILE *)0}, *magic_yyout = {(FILE *)1};
extern int magic_yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *magic_yyestate;
extern struct yysvf magic_yysvec[], *magic_yybgin;

# line 2 "magical_lex.l"

# line 3 "magical_lex.l"
/*
*/

# line 10 "magical_lex.l"
/*-------------------------------------------------------------------------
|
|       magic.lex.l
|
|       This is the description of the lexical tokens used in the system.
|       The magic.lex.l file is used by LEX to produce a lexer that feeds
|       the parser.  
|
|       NOTE:   The magicFileName (pointer) placed in magic_yylval (one per token)
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
#include "magical_io.h"


# line 35 "magical_lex.l"
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
#define returnToken(T)  { magic_yylval.tval.token           = T; \
			  magic_yylval.tval.location.line   = magicLineNumber; \
			  magic_yylval.tval.location.column = magicColumnNumber; \
			  magic_yylval.tval.location.file   = magicFileName; \
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
#else   DEBUG
#define LPRINT0(level,str)
#define LPRINT1(level,str, arg1)
#define LPRINT2(level,str, arg1, arg2)
#define LPRINT3(level,str, arg1, arg2, arg3)
#define returnToken(T)  { magic_yylval.tval.token           = T; \
			  magic_yylval.tval.location.line   = magicLineNumber; \
			  magic_yylval.tval.location.column = magicColumnNumber; \
			  magic_yylval.tval.location.file   = magicFileName; \
			  return(T); \
			}
#endif  DEBUG


# line 74 "magical_lex.l"
/*static char     fileName[1025];
static int      columnNumber;
static int      lineNumber;*/
static int oldColumnNumber;

static YYSTYPE  magic_yylval;
static int (*inputFunc)() = 0;
static void (*outputFunc)() = 0;
static void (*unputFunc)() = 0;

int magicFromString = 0;
int magicFromFile = 0;

int      ignoreEOL;
static int input();

# define YYNEWLINE 10
magic_yylex(){
int nstr; extern int magic_yyprevious;
#ifdef __cplusplus
/* to avoid CC and lint complaining yyfussy not being used ...*/
static int __lex_hack = 0;
if (__lex_hack) goto yyfussy;
#endif
while((nstr = magic_yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(magic_yywrap()) return(0); break;
case 1:

# line 91 "magical_lex.l"
                           { LPRINT0(1,"lex: exhausted, suspending!\n");
				  return(-1);
				}
break;
case 2:

# line 94 "magical_lex.l"
                           { LPRINT0(1,"lex: got \"^D\", ENDFILE\n");
				  returnToken(ENDFILE);
				}
break;
case 3:

# line 97 "magical_lex.l"
                           { LPRINT0(1,"lex: got \"{\", LC\n");
				  returnToken(DOLLAR);
				}
break;
case 4:

# line 100 "magical_lex.l"
                             { LPRINT0(1,"lex: got \"{\", LC\n");
				  returnToken(LC);
				}
break;
case 5:

# line 103 "magical_lex.l"
                             { LPRINT0(1,"lex: got \"}\", RC\n");
				  returnToken(RC);
				}
break;
case 6:

# line 106 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", CL\n",magic_yytext);
				  returnToken(CL);
				}
break;
case 7:

# line 109 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", CM\n",magic_yytext);
				  returnToken(CM);
				}
break;
case 8:

# line 112 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", EQ\n",magic_yytext);
				  returnToken(EQ);
				}
break;
case 9:

# line 115 "magical_lex.l"
                           { LPRINT1(1,"lex: got \"%s\", NE\n",magic_yytext);
				  returnToken(NE);
				}
break;
case 10:

# line 118 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", LT\n",magic_yytext);
				  returnToken(LT);
				}
break;
case 11:

# line 121 "magical_lex.l"
                           { LPRINT1(1,"lex: got \"%s\", LE\n",magic_yytext);
				  returnToken(LE);
				}
break;
case 12:

# line 124 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", GT\n",magic_yytext);
				  returnToken(GT);
				}
break;
case 13:

# line 127 "magical_lex.l"
                           { LPRINT1(1,"lex: got \"%s\", GE\n",magic_yytext);
				  returnToken(GE);
				}
break;
case 14:

# line 130 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", PLUS\n",magic_yytext);
				  returnToken(PLUS);
				}
break;
case 15:

# line 133 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", MINUS\n",magic_yytext);
				  returnToken(MINUS);
				}
break;
case 16:

# line 136 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", MULT\n",magic_yytext);
				  returnToken(MULT);
				}
break;
case 17:

# line 139 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", DIV\n",magic_yytext);
				  returnToken(DIV);
				}
break;
case 18:

# line 142 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", LP\n",magic_yytext);
				  returnToken(LP);
				}
break;
case 19:

# line 145 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", LB\n",magic_yytext);
				  returnToken(LB);
				}
break;
case 20:

# line 148 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", RB\n",magic_yytext);
				  returnToken(RB);
				}
break;
case 21:

# line 151 "magical_lex.l"
                            { LPRINT1(1,"lex: got \"%s\", RP\n",magic_yytext);
				  returnToken(RP);
				}
break;
case 22:

# line 154 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", ABORT\n",magic_yytext);
				  returnToken(BOMB);
				}
break;
case 23:

# line 157 "magical_lex.l"
	{ LPRINT1(1,"lex: got \"%s\",ABORTOFF\n",
					  magic_yytext);
				  returnToken(IGNORE);
				}
break;
case 24:

# line 161 "magical_lex.l"
                { LPRINT1(1,"lex: got \"%s\", ABORTON\n",
					  magic_yytext);
				  returnToken(DONT);
				}
break;
case 25:

# line 165 "magical_lex.l"
                        { LPRINT1(1,"lex: got \"%s\", AND\n",magic_yytext);
				  returnToken(AND);
				}
break;
case 26:

# line 168 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", BREAK\n",magic_yytext);
				  returnToken(BREAK);
				}
break;
case 27:

# line 171 "magical_lex.l"
                          { LPRINT1(1,"lex: got \"%s\", DO\n",magic_yytext);
				  returnToken(DO);
				}
break;
case 28:

# line 174 "magical_lex.l"
                      { LPRINT1(1,"lex: got \"%s\", ELSE\n",magic_yytext);
				  returnToken(ELSE);
				}
break;
case 29:

# line 177 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", FI\n",magic_yytext);
				  returnToken(FI);
				}
break;
case 30:

# line 180 "magical_lex.l"
              { LPRINT1(1,"lex: got \"%s\", OD\n",magic_yytext);
				  returnToken(OD);
				}
break;
case 31:

# line 183 "magical_lex.l"
                  { LPRINT1(1,"lex: got \"%s\", RETURN\n",magic_yytext);
				  returnToken(EXIT);
				}
break;
case 32:

# line 186 "magical_lex.l"
                          { LPRINT1(1,"lex: got \"%s\", IF\n",magic_yytext);
				  returnToken(IF);
				}
break;
case 33:

# line 189 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", MOD\n",magic_yytext);
				  returnToken(MOD);
				}
break;
case 34:

# line 192 "magical_lex.l"
                        { LPRINT1(1,"lex: got \"%s\", NOT\n",magic_yytext);
				  returnToken(NOT);
				}
break;
case 35:

# line 195 "magical_lex.l"
                          { LPRINT1(1,"lex: got \"%s\", OR\n",magic_yytext);
				  returnToken(OR);
				}
break;
case 36:

# line 198 "magical_lex.l"
                  { LPRINT1(1,"lex: got \"%s\", REPEAT\n",magic_yytext);
				  returnToken(REPEAT);
				}
break;
case 37:

# line 201 "magical_lex.l"
                             { LPRINT1(1,"lex: got \"%s\", SHOW\n",magic_yytext);
				  returnToken(SHOW);
				}
break;
case 38:

# line 204 "magical_lex.l"
                      { LPRINT1(1,"lex: got \"%s\", SIZE\n",magic_yytext);
				  returnToken(SIZE);
				}
break;
case 39:

# line 207 "magical_lex.l"
                      { LPRINT1(1,"lex: got \"%s\", SQRT\n",magic_yytext);
				  returnToken(SQRT);
				}
break;
case 40:

# line 210 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", TRUNC\n",magic_yytext);
				  returnToken(TRUNC);
				}
break;
case 41:

# line 213 "magical_lex.l"
                  { LPRINT1(1,"lex: got \"%s\", TYPEOF\n",magic_yytext);
				  returnToken(TYPEOF);
				}
break;
case 42:

# line 216 "magical_lex.l"
                      { LPRINT1(1,"lex: got \"%s\", THEN\n",magic_yytext);
				  returnToken(THEN);
				}
break;
case 43:

# line 219 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", UNTIL\n",magic_yytext);
				  returnToken(UNTIL);
				}
break;
case 44:

# line 222 "magical_lex.l"
                    { LPRINT1(1,"lex: got \"%s\", WHILE\n",magic_yytext);
				  returnToken(WHILE);
				}
break;
case 45:

# line 225 "magical_lex.l"
   {
				  LPRINT1(1,"lex: got \"%s\", ID\n",magic_yytext);
				  returnToken(ID);
				}
break;
case 46:

# line 229 "magical_lex.l"
{
				  LPRINT1(1,"lex: got \"%s\", REAL\n",magic_yytext);
				  returnToken(REAL);
				}
break;
case 47:

# line 233 "magical_lex.l"
{
				  LPRINT1(1,"lex: got \"%s\", UNIT\n",magic_yytext);
				  returnToken(UNIT);
				}
break;
case 48:

# line 237 "magical_lex.l"
                             { int c,notDone,slashed;

				  slashed = 0;
				  notDone = 1;
				  while (notDone)
				  {  c = input();
				     if (c < ' ' && c != '\t')
				     {  magic_yyleng    = 1;
					magic_yytext[0] = '\n';
					magic_yytext[1] = '\0';
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
					magic_yytext[magic_yyleng++] = c;
					magic_yytext[magic_yyleng]   = '\0';
				     }
				     else
				     {  if (c == '\\')
					   slashed = 1;
					else
					{  magic_yytext[magic_yyleng++] = c;
					   magic_yytext[magic_yyleng]   = '\0';
					   if (c == '\'')
					      notDone = 0;
					}
				     }
				  }
				  LPRINT1(1,"lex: got \"%s\", STRING\n",magic_yytext);
				  returnToken(STRING);
				}
break;
case 49:

# line 282 "magical_lex.l"
        /*"*/                { int c;

				  while ((c=input()) != '"')
				      if (c < ' ' && c != '\t')
				      {    magic_yyleng    = 1;
					   magic_yytext[0] = '\n';
					   magic_yytext[1] = '\0';    
					   returnToken(EOL);
				      }
				  LPRINT0(1,"lex: got comment!\n");
				}
break;
case 50:

# line 293 "magical_lex.l"
                           ;
break;
case 51:

# line 294 "magical_lex.l"
                             { if (ignoreEOL)
				  {   LPRINT0(1,"lex: ignored \"\\n\", EOL\n");
				  }
				  else
				  {   LPRINT0(1,"lex: got \"\\n\", EOL\n");
				      returnToken(EOL);
				  }
				}
break;
case 52:

# line 302 "magical_lex.l"
                          ;
break;
case 53:

# line 303 "magical_lex.l"
                              returnToken(BAD);
break;
case -1:
break;
default:
(void)fprintf(magic_yyout,"bad switch magic_yylook %d",nstr);
} return(0); }
/* end of magic_yylex */

# line 305 "magical_lex.l"

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
	if (magic_yysptr > magic_yysbuf){
	    magic_yytchar = *(--magic_yysptr);
	}else{
	    magic_yytchar = getc(magic_yyin);
	}
	if (magic_yytchar == '\n'){
	    magic_yylineno++;
	}else if (magic_yytchar == EOF){
	    magic_yytchar = 0;
	}
	return magic_yytchar;
    }
}
#endif /* NOTDEFINED */

static int
input()
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
#endif DEBUG
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
#endif   DEBUG
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
	magic_yytchar = c;
	if (magic_yytchar == '\n'){
	    magic_yylineno--;
	}
	*magic_yysptr++ = magic_yytchar;
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

int magic_yywrap()
{  return(1);
}
int magic_yyvstop[] = {
0,

1,
53,
0, 

53,
0, 

2,
53,
0, 

52,
53,
0, 

51,
0, 

49,
53,
0, 

45,
53,
0, 

33,
53,
0, 

48,
53,
0, 

18,
53,
0, 

21,
53,
0, 

16,
53,
0, 

14,
53,
0, 

7,
53,
0, 

15,
53,
0, 

53,
0, 

17,
53,
0, 

46,
53,
0, 

6,
53,
0, 

10,
53,
0, 

8,
53,
0, 

12,
53,
0, 

37,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

19,
53,
0, 

53,
0, 

20,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

45,
53,
0, 

4,
53,
0, 

5,
53,
0, 

45,
0, 

3,
0, 

46,
0, 

46,
0, 

46,
0, 

47,
0, 

47,
0, 

11,
0, 

9,
0, 

13,
0, 

45,
0, 

45,
0, 

45,
0, 

27,
45,
0, 

45,
0, 

45,
0, 

32,
45,
0, 

45,
0, 

45,
0, 

35,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

50,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

46,
0, 

45,
0, 

25,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

33,
45,
0, 

34,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

28,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

38,
45,
0, 

39,
45,
0, 

42,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

22,
45,
0, 

26,
45,
0, 

29,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

40,
45,
0, 

45,
0, 

43,
45,
0, 

44,
45,
0, 

22,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

45,
0, 

36,
45,
0, 

31,
45,
0, 

41,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

24,
45,
0, 

45,
0, 

45,
0, 

45,
0, 

23,
45,
0, 

30,
45,
0, 
0};
# define YYTYPE unsigned char
struct yywork { YYTYPE verify, advance; } magic_yycrank[] = {
0,0,	0,0,	1,3,	1,4,	
1,4,	1,5,	1,4,	1,4,	
1,4,	1,4,	1,6,	1,7,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	1,4,	1,4,	1,4,	
1,4,	0,0,	1,4,	1,8,	
1,4,	1,9,	1,10,	1,4,	
1,11,	1,12,	1,13,	1,14,	
1,15,	1,16,	1,17,	1,18,	
1,19,	1,20,	41,86,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	1,21,	
1,4,	1,22,	1,23,	1,24,	
1,25,	1,4,	1,26,	1,27,	
1,28,	1,29,	1,30,	1,28,	
1,28,	1,28,	1,31,	1,28,	
1,28,	1,28,	1,32,	1,33,	
1,34,	1,28,	1,28,	1,35,	
1,36,	1,37,	1,38,	1,28,	
1,39,	1,28,	1,28,	1,28,	
1,40,	1,41,	1,42,	1,4,	
1,28,	1,4,	1,43,	1,44,	
1,28,	1,45,	1,46,	1,28,	
1,28,	1,28,	1,47,	1,28,	
1,28,	1,28,	1,48,	1,49,	
1,50,	1,28,	1,28,	1,51,	
1,52,	1,53,	1,54,	1,28,	
1,55,	1,28,	1,28,	1,28,	
1,56,	1,4,	1,57,	1,4,	
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
22,65,	22,66,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	2,4,	
2,4,	2,4,	2,4,	24,67,	
2,4,	2,8,	0,0,	0,0,	
2,10,	2,4,	2,11,	2,12,	
2,13,	2,14,	0,0,	2,16,	
2,17,	2,18,	2,19,	18,60,	
18,60,	18,60,	18,60,	18,60,	
18,60,	18,60,	18,60,	18,60,	
18,60,	2,21,	2,4,	2,22,	
2,23,	2,24,	2,25,	2,4,	
0,0,	2,27,	2,28,	2,29,	
0,0,	2,28,	2,28,	2,28,	
2,31,	2,28,	2,28,	2,28,	
2,32,	2,33,	2,34,	2,28,	
2,28,	2,35,	2,36,	2,37,	
2,38,	2,28,	2,39,	2,28,	
2,28,	2,28,	2,40,	2,41,	
2,42,	2,4,	2,28,	2,4,	
2,43,	2,44,	2,28,	2,45,	
2,46,	2,28,	2,28,	2,28,	
2,47,	2,28,	2,28,	2,28,	
2,48,	2,49,	2,50,	2,28,	
2,28,	2,51,	2,52,	2,53,	
2,54,	2,28,	2,55,	2,28,	
2,28,	2,28,	2,56,	2,4,	
2,57,	2,4,	2,4,	2,4,	
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
2,4,	2,4,	2,4,	9,58,	
9,58,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	27,58,	28,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	27,58,	28,58,	
0,0,	0,0,	27,70,	0,0,	
0,0,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	0,0,	
0,0,	0,0,	0,0,	9,58,	
0,0,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,58,	
9,58,	9,58,	9,58,	9,59,	
20,61,	29,58,	20,62,	20,62,	
20,62,	20,62,	20,62,	20,62,	
20,62,	20,62,	20,62,	20,62,	
0,0,	29,58,	29,71,	0,0,	
0,0,	0,0,	0,0,	20,63,	
20,63,	20,63,	20,63,	20,64,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	20,63,	
20,63,	20,63,	20,63,	20,64,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	20,63,	20,63,	20,63,	
20,63,	26,58,	26,58,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
32,58,	0,0,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
32,58,	32,75,	0,0,	0,0,	
0,0,	0,0,	0,0,	26,58,	
26,68,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,69,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	0,0,	0,0,	0,0,	
0,0,	26,58,	0,0,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	26,58,	26,58,	26,58,	
26,58,	30,58,	31,58,	33,58,	
0,0,	34,58,	31,74,	35,58,	
0,0,	0,0,	35,78,	30,72,	
38,58,	30,73,	31,58,	33,58,	
33,76,	34,58,	36,58,	35,58,	
37,58,	34,77,	39,58,	43,58,	
38,84,	36,79,	37,81,	44,58,	
39,85,	45,58,	36,58,	0,0,	
37,58,	36,80,	39,58,	43,58,	
37,82,	46,58,	48,58,	44,58,	
47,58,	45,58,	49,58,	37,83,	
50,58,	51,58,	52,58,	54,58,	
53,58,	46,58,	48,58,	55,58,	
47,58,	58,58,	49,58,	43,87,	
50,58,	51,58,	52,58,	54,58,	
53,58,	70,58,	0,0,	55,58,	
70,106,	58,58,	69,58,	43,88,	
69,105,	0,0,	0,0,	0,0,	
71,58,	70,58,	45,71,	44,89,	
47,74,	0,0,	69,58,	46,90,	
51,94,	46,91,	68,58,	48,92,	
71,58,	52,95,	53,97,	49,93,	
73,58,	55,101,	73,108,	54,100,	
50,77,	52,96,	68,58,	68,104,	
53,98,	0,0,	0,0,	0,0,	
73,58,	0,0,	72,58,	53,99,	
60,60,	60,60,	60,60,	60,60,	
60,60,	60,60,	60,60,	60,60,	
60,60,	60,60,	72,58,	0,0,	
0,0,	0,0,	0,0,	72,107,	
0,0,	60,63,	60,63,	60,63,	
60,63,	60,64,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	60,63,	60,63,	60,63,	
60,63,	60,64,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	60,63,	
60,63,	60,63,	60,63,	61,61,	
61,61,	61,61,	61,61,	61,61,	
61,61,	61,61,	61,61,	61,61,	
61,61,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	63,63,	
63,63,	63,63,	63,63,	64,102,	
0,0,	64,102,	74,58,	0,0,	
64,103,	64,103,	64,103,	64,103,	
64,103,	64,103,	64,103,	64,103,	
64,103,	64,103,	74,58,	75,58,	
76,58,	75,109,	77,58,	78,58,	
79,58,	0,0,	0,0,	80,58,	
82,58,	0,0,	81,58,	75,58,	
76,58,	81,115,	77,58,	78,58,	
79,58,	78,111,	76,110,	80,58,	
82,58,	78,112,	81,58,	80,114,	
83,58,	84,58,	0,0,	82,116,	
79,113,	87,58,	88,58,	0,0,	
85,58,	89,58,	90,58,	91,58,	
83,58,	84,58,	83,117,	85,119,	
93,58,	87,58,	88,58,	84,118,	
85,58,	89,58,	90,58,	91,58,	
92,58,	94,58,	95,58,	97,58,	
93,58,	96,58,	98,58,	101,58,	
0,0,	0,0,	99,58,	100,58,	
92,58,	94,58,	95,58,	97,58,	
88,105,	96,58,	98,58,	101,58,	
89,121,	91,123,	99,58,	100,58,	
104,58,	105,58,	87,120,	107,58,	
106,134,	106,58,	107,135,	109,58,	
110,58,	112,58,	92,109,	90,122,	
104,58,	105,58,	97,128,	107,58,	
104,133,	106,58,	93,110,	109,58,	
110,58,	112,58,	101,132,	94,124,	
115,58,	0,0,	0,0,	94,125,	
112,139,	96,127,	0,0,	0,0,	
99,130,	98,129,	95,126,	0,0,	
115,142,	100,131,	102,103,	102,103,	
102,103,	102,103,	102,103,	102,103,	
102,103,	102,103,	102,103,	102,103,	
103,103,	103,103,	103,103,	103,103,	
103,103,	103,103,	103,103,	103,103,	
103,103,	103,103,	108,58,	111,58,	
113,58,	116,58,	111,138,	113,140,	
114,58,	108,136,	117,58,	0,0,	
118,58,	117,144,	108,58,	111,58,	
113,58,	116,143,	119,58,	118,145,	
114,58,	120,58,	117,58,	108,137,	
118,58,	121,58,	114,141,	122,58,	
119,146,	123,58,	119,58,	124,58,	
125,58,	120,58,	127,58,	126,58,	
128,58,	121,58,	130,58,	122,58,	
131,58,	123,58,	129,58,	124,58,	
125,58,	132,58,	127,58,	126,58,	
128,58,	133,58,	130,58,	134,58,	
131,58,	135,58,	129,58,	0,0,	
121,148,	132,58,	138,161,	138,58,	
134,158,	133,58,	122,135,	134,58,	
0,0,	135,58,	124,151,	133,157,	
123,149,	120,147,	126,140,	138,58,	
0,0,	130,154,	0,0,	0,0,	
139,58,	136,58,	140,58,	131,155,	
128,142,	136,159,	123,150,	125,152,	
127,141,	137,58,	129,153,	132,156,	
139,58,	136,58,	140,58,	137,160,	
139,162,	141,58,	142,58,	143,58,	
143,163,	137,58,	144,58,	0,0,	
0,0,	146,58,	145,58,	147,58,	
146,166,	141,58,	142,58,	143,58,	
148,58,	149,58,	144,58,	144,164,	
145,165,	146,58,	145,58,	147,58,	
150,58,	151,58,	152,58,	153,58,	
148,58,	149,58,	154,58,	155,58,	
156,58,	157,58,	0,0,	158,58,	
150,58,	151,58,	152,58,	153,58,	
159,58,	0,0,	154,58,	155,58,	
156,58,	157,58,	157,172,	158,58,	
0,0,	0,0,	0,0,	160,58,	
159,58,	149,159,	161,58,	0,0,	
151,169,	148,158,	160,173,	162,58,	
153,163,	147,167,	150,168,	160,58,	
163,58,	164,58,	161,58,	156,166,	
165,58,	164,176,	166,58,	162,175,	
161,174,	155,165,	152,170,	154,171,	
163,58,	164,58,	167,58,	168,58,	
165,58,	169,58,	166,58,	170,58,	
171,58,	172,58,	0,0,	174,58,	
173,58,	172,179,	167,58,	168,58,	
175,58,	169,58,	176,58,	170,58,	
171,58,	172,180,	173,181,	174,58,	
173,58,	177,58,	178,58,	179,58,	
175,58,	180,58,	176,58,	179,184,	
0,0,	181,58,	182,58,	0,0,	
181,185,	177,58,	178,58,	179,58,	
183,58,	180,58,	168,178,	0,0,	
171,176,	181,58,	182,58,	167,177,	
184,58,	0,0,	185,58,	170,175,	
183,58,	0,0,	0,0,	169,174,	
0,0,	0,0,	0,0,	0,0,	
184,58,	177,182,	185,58,	0,0,	
0,0,	0,0,	0,0,	0,0,	
178,183,	177,180,	182,184,	0,0,	
0,0,	0,0,	0,0,	183,185,	
0,0};
struct yysvf magic_yysvec[] = {
0,	0,	0,
magic_yycrank+-1,	0,		0,	
magic_yycrank+-255,	magic_yysvec+1,	0,	
magic_yycrank+0,	0,		magic_yyvstop+1,
magic_yycrank+0,	0,		magic_yyvstop+4,
magic_yycrank+0,	0,		magic_yyvstop+6,
magic_yycrank+0,	0,		magic_yyvstop+9,
magic_yycrank+0,	0,		magic_yyvstop+12,
magic_yycrank+0,	0,		magic_yyvstop+14,
magic_yycrank+476,	0,		magic_yyvstop+17,
magic_yycrank+0,	0,		magic_yyvstop+20,
magic_yycrank+0,	0,		magic_yyvstop+23,
magic_yycrank+0,	0,		magic_yyvstop+26,
magic_yycrank+0,	0,		magic_yyvstop+29,
magic_yycrank+0,	0,		magic_yyvstop+32,
magic_yycrank+0,	0,		magic_yyvstop+35,
magic_yycrank+0,	0,		magic_yyvstop+38,
magic_yycrank+0,	0,		magic_yyvstop+41,
magic_yycrank+255,	0,		magic_yyvstop+44,
magic_yycrank+0,	0,		magic_yyvstop+46,
magic_yycrank+554,	0,		magic_yyvstop+49,
magic_yycrank+0,	0,		magic_yyvstop+52,
magic_yycrank+203,	0,		magic_yyvstop+55,
magic_yycrank+0,	0,		magic_yyvstop+58,
magic_yycrank+226,	0,		magic_yyvstop+61,
magic_yycrank+0,	0,		magic_yyvstop+64,
magic_yycrank+642,	0,		magic_yyvstop+67,
magic_yycrank+456,	magic_yysvec+26,	magic_yyvstop+70,
magic_yycrank+457,	magic_yysvec+26,	magic_yyvstop+73,
magic_yycrank+535,	magic_yysvec+26,	magic_yyvstop+76,
magic_yycrank+699,	magic_yysvec+26,	magic_yyvstop+79,
magic_yycrank+700,	magic_yysvec+26,	magic_yyvstop+82,
magic_yycrank+622,	magic_yysvec+26,	magic_yyvstop+85,
magic_yycrank+701,	magic_yysvec+26,	magic_yyvstop+88,
magic_yycrank+703,	magic_yysvec+26,	magic_yyvstop+91,
magic_yycrank+705,	magic_yysvec+26,	magic_yyvstop+94,
magic_yycrank+716,	magic_yysvec+26,	magic_yyvstop+97,
magic_yycrank+718,	magic_yysvec+26,	magic_yyvstop+100,
magic_yycrank+710,	magic_yysvec+26,	magic_yyvstop+103,
magic_yycrank+720,	magic_yysvec+26,	magic_yyvstop+106,
magic_yycrank+0,	0,		magic_yyvstop+109,
magic_yycrank+40,	0,		magic_yyvstop+112,
magic_yycrank+0,	0,		magic_yyvstop+114,
magic_yycrank+721,	magic_yysvec+26,	magic_yyvstop+117,
magic_yycrank+725,	magic_yysvec+26,	magic_yyvstop+120,
magic_yycrank+727,	magic_yysvec+26,	magic_yyvstop+123,
magic_yycrank+735,	magic_yysvec+26,	magic_yyvstop+126,
magic_yycrank+738,	magic_yysvec+26,	magic_yyvstop+129,
magic_yycrank+736,	magic_yysvec+26,	magic_yyvstop+132,
magic_yycrank+740,	magic_yysvec+26,	magic_yyvstop+135,
magic_yycrank+742,	magic_yysvec+26,	magic_yyvstop+138,
magic_yycrank+743,	magic_yysvec+26,	magic_yyvstop+141,
magic_yycrank+744,	magic_yysvec+26,	magic_yyvstop+144,
magic_yycrank+746,	magic_yysvec+26,	magic_yyvstop+147,
magic_yycrank+745,	magic_yysvec+26,	magic_yyvstop+150,
magic_yycrank+749,	magic_yysvec+26,	magic_yyvstop+153,
magic_yycrank+0,	0,		magic_yyvstop+156,
magic_yycrank+0,	0,		magic_yyvstop+159,
magic_yycrank+751,	magic_yysvec+26,	magic_yyvstop+162,
magic_yycrank+0,	0,		magic_yyvstop+164,
magic_yycrank+820,	0,		magic_yyvstop+166,
magic_yycrank+895,	magic_yysvec+60,	magic_yyvstop+168,
magic_yycrank+0,	magic_yysvec+20,	magic_yyvstop+170,
magic_yycrank+888,	0,		magic_yyvstop+172,
magic_yycrank+968,	magic_yysvec+63,	magic_yyvstop+174,
magic_yycrank+0,	0,		magic_yyvstop+176,
magic_yycrank+0,	0,		magic_yyvstop+178,
magic_yycrank+0,	0,		magic_yyvstop+180,
magic_yycrank+780,	magic_yysvec+26,	magic_yyvstop+182,
magic_yycrank+764,	magic_yysvec+26,	magic_yyvstop+184,
magic_yycrank+759,	magic_yysvec+26,	magic_yyvstop+186,
magic_yycrank+770,	magic_yysvec+26,	magic_yyvstop+188,
magic_yycrank+800,	magic_yysvec+26,	magic_yyvstop+191,
magic_yycrank+786,	magic_yysvec+26,	magic_yyvstop+193,
magic_yycrank+948,	magic_yysvec+26,	magic_yyvstop+195,
magic_yycrank+961,	magic_yysvec+26,	magic_yyvstop+198,
magic_yycrank+962,	magic_yysvec+26,	magic_yyvstop+200,
magic_yycrank+964,	magic_yysvec+26,	magic_yyvstop+202,
magic_yycrank+965,	magic_yysvec+26,	magic_yyvstop+205,
magic_yycrank+966,	magic_yysvec+26,	magic_yyvstop+207,
magic_yycrank+969,	magic_yysvec+26,	magic_yyvstop+209,
magic_yycrank+972,	magic_yysvec+26,	magic_yyvstop+211,
magic_yycrank+970,	magic_yysvec+26,	magic_yyvstop+213,
magic_yycrank+986,	magic_yysvec+26,	magic_yyvstop+215,
magic_yycrank+987,	magic_yysvec+26,	magic_yyvstop+217,
magic_yycrank+994,	magic_yysvec+26,	magic_yyvstop+219,
magic_yycrank+0,	0,		magic_yyvstop+221,
magic_yycrank+991,	magic_yysvec+26,	magic_yyvstop+223,
magic_yycrank+992,	magic_yysvec+26,	magic_yyvstop+225,
magic_yycrank+995,	magic_yysvec+26,	magic_yyvstop+227,
magic_yycrank+996,	magic_yysvec+26,	magic_yyvstop+229,
magic_yycrank+997,	magic_yysvec+26,	magic_yyvstop+231,
magic_yycrank+1010,	magic_yysvec+26,	magic_yyvstop+233,
magic_yycrank+1002,	magic_yysvec+26,	magic_yyvstop+235,
magic_yycrank+1011,	magic_yysvec+26,	magic_yyvstop+237,
magic_yycrank+1012,	magic_yysvec+26,	magic_yyvstop+239,
magic_yycrank+1015,	magic_yysvec+26,	magic_yyvstop+241,
magic_yycrank+1013,	magic_yysvec+26,	magic_yyvstop+243,
magic_yycrank+1016,	magic_yysvec+26,	magic_yyvstop+245,
magic_yycrank+1020,	magic_yysvec+26,	magic_yyvstop+247,
magic_yycrank+1021,	magic_yysvec+26,	magic_yyvstop+249,
magic_yycrank+1017,	magic_yysvec+26,	magic_yyvstop+251,
magic_yycrank+1090,	0,		0,	
magic_yycrank+1100,	magic_yysvec+63,	magic_yyvstop+253,
magic_yycrank+1034,	magic_yysvec+26,	magic_yyvstop+255,
magic_yycrank+1035,	magic_yysvec+26,	magic_yyvstop+257,
magic_yycrank+1039,	magic_yysvec+26,	magic_yyvstop+260,
magic_yycrank+1037,	magic_yysvec+26,	magic_yyvstop+262,
magic_yycrank+1092,	magic_yysvec+26,	magic_yyvstop+264,
magic_yycrank+1041,	magic_yysvec+26,	magic_yyvstop+266,
magic_yycrank+1042,	magic_yysvec+26,	magic_yyvstop+269,
magic_yycrank+1093,	magic_yysvec+26,	magic_yyvstop+272,
magic_yycrank+1043,	magic_yysvec+26,	magic_yyvstop+274,
magic_yycrank+1094,	magic_yysvec+26,	magic_yyvstop+276,
magic_yycrank+1098,	magic_yysvec+26,	magic_yyvstop+278,
magic_yycrank+1058,	magic_yysvec+26,	magic_yyvstop+280,
magic_yycrank+1095,	magic_yysvec+26,	magic_yyvstop+282,
magic_yycrank+1100,	magic_yysvec+26,	magic_yyvstop+284,
magic_yycrank+1102,	magic_yysvec+26,	magic_yyvstop+286,
magic_yycrank+1108,	magic_yysvec+26,	magic_yyvstop+288,
magic_yycrank+1111,	magic_yysvec+26,	magic_yyvstop+290,
magic_yycrank+1115,	magic_yysvec+26,	magic_yyvstop+292,
magic_yycrank+1117,	magic_yysvec+26,	magic_yyvstop+294,
magic_yycrank+1119,	magic_yysvec+26,	magic_yyvstop+296,
magic_yycrank+1121,	magic_yysvec+26,	magic_yyvstop+298,
magic_yycrank+1122,	magic_yysvec+26,	magic_yyvstop+300,
magic_yycrank+1125,	magic_yysvec+26,	magic_yyvstop+302,
magic_yycrank+1124,	magic_yysvec+26,	magic_yyvstop+304,
magic_yycrank+1126,	magic_yysvec+26,	magic_yyvstop+306,
magic_yycrank+1132,	magic_yysvec+26,	magic_yyvstop+308,
magic_yycrank+1128,	magic_yysvec+26,	magic_yyvstop+310,
magic_yycrank+1130,	magic_yysvec+26,	magic_yyvstop+312,
magic_yycrank+1135,	magic_yysvec+26,	magic_yyvstop+314,
magic_yycrank+1139,	magic_yysvec+26,	magic_yyvstop+316,
magic_yycrank+1141,	magic_yysvec+26,	magic_yyvstop+318,
magic_yycrank+1143,	magic_yysvec+26,	magic_yyvstop+320,
magic_yycrank+1167,	magic_yysvec+26,	magic_yyvstop+323,
magic_yycrank+1175,	magic_yysvec+26,	magic_yyvstop+325,
magic_yycrank+1149,	magic_yysvec+26,	magic_yyvstop+327,
magic_yycrank+1166,	magic_yysvec+26,	magic_yyvstop+329,
magic_yycrank+1168,	magic_yysvec+26,	magic_yyvstop+331,
magic_yycrank+1183,	magic_yysvec+26,	magic_yyvstop+334,
magic_yycrank+1184,	magic_yysvec+26,	magic_yyvstop+337,
magic_yycrank+1185,	magic_yysvec+26,	magic_yyvstop+340,
magic_yycrank+1188,	magic_yysvec+26,	magic_yyvstop+342,
magic_yycrank+1192,	magic_yysvec+26,	magic_yyvstop+344,
magic_yycrank+1191,	magic_yysvec+26,	magic_yyvstop+346,
magic_yycrank+1193,	magic_yysvec+26,	magic_yyvstop+348,
magic_yycrank+1198,	magic_yysvec+26,	magic_yyvstop+350,
magic_yycrank+1199,	magic_yysvec+26,	magic_yyvstop+352,
magic_yycrank+1206,	magic_yysvec+26,	magic_yyvstop+354,
magic_yycrank+1207,	magic_yysvec+26,	magic_yyvstop+356,
magic_yycrank+1208,	magic_yysvec+26,	magic_yyvstop+358,
magic_yycrank+1209,	magic_yysvec+26,	magic_yyvstop+360,
magic_yycrank+1212,	magic_yysvec+26,	magic_yyvstop+362,
magic_yycrank+1213,	magic_yysvec+26,	magic_yyvstop+364,
magic_yycrank+1214,	magic_yysvec+26,	magic_yyvstop+366,
magic_yycrank+1215,	magic_yysvec+26,	magic_yyvstop+368,
magic_yycrank+1217,	magic_yysvec+26,	magic_yyvstop+371,
magic_yycrank+1222,	magic_yysvec+26,	magic_yyvstop+374,
magic_yycrank+1233,	magic_yysvec+26,	magic_yyvstop+377,
magic_yycrank+1236,	magic_yysvec+26,	magic_yyvstop+379,
magic_yycrank+1241,	magic_yysvec+26,	magic_yyvstop+381,
magic_yycrank+1246,	magic_yysvec+26,	magic_yyvstop+383,
magic_yycrank+1247,	magic_yysvec+26,	magic_yyvstop+386,
magic_yycrank+1250,	magic_yysvec+26,	magic_yyvstop+388,
magic_yycrank+1252,	magic_yysvec+26,	magic_yyvstop+391,
magic_yycrank+1260,	magic_yysvec+26,	magic_yyvstop+394,
magic_yycrank+1261,	magic_yysvec+26,	magic_yyvstop+397,
magic_yycrank+1263,	magic_yysvec+26,	magic_yyvstop+399,
magic_yycrank+1265,	magic_yysvec+26,	magic_yyvstop+401,
magic_yycrank+1266,	magic_yysvec+26,	magic_yyvstop+403,
magic_yycrank+1267,	magic_yysvec+26,	magic_yyvstop+405,
magic_yycrank+1270,	magic_yysvec+26,	magic_yyvstop+407,
magic_yycrank+1269,	magic_yysvec+26,	magic_yyvstop+409,
magic_yycrank+1274,	magic_yysvec+26,	magic_yyvstop+412,
magic_yycrank+1276,	magic_yysvec+26,	magic_yyvstop+415,
magic_yycrank+1283,	magic_yysvec+26,	magic_yyvstop+418,
magic_yycrank+1284,	magic_yysvec+26,	magic_yyvstop+420,
magic_yycrank+1285,	magic_yysvec+26,	magic_yyvstop+422,
magic_yycrank+1287,	magic_yysvec+26,	magic_yyvstop+424,
magic_yycrank+1291,	magic_yysvec+26,	magic_yyvstop+427,
magic_yycrank+1292,	magic_yysvec+26,	magic_yyvstop+429,
magic_yycrank+1298,	magic_yysvec+26,	magic_yyvstop+431,
magic_yycrank+1306,	magic_yysvec+26,	magic_yyvstop+433,
magic_yycrank+1308,	magic_yysvec+26,	magic_yyvstop+436,
0,	0,	0};
struct yywork *magic_yytop = magic_yycrank+1399;
struct yysvf *magic_yybgin = magic_yysvec+1;
char magic_yymatch[] = {
  0,   1,   1,   1,   1,   1,   1,   1, 
  1,   9,  10,   1,   1,   1,   1,   1, 
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
char magic_yyextra[] = {
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

#pragma ident	"@(#)ncform	6.12	97/12/08 SMI"

int magic_yylineno =1;
# define YYU(x) x
# define NLSTATE magic_yyprevious=YYNEWLINE
struct yysvf *magic_yylstate [YYLMAX], **magic_yylsp, **magic_yyolsp;
char magic_yysbuf[YYLMAX];
char *magic_yysptr = magic_yysbuf;
int *magic_yyfnd;
extern struct yysvf *magic_yyestate;
int magic_yyprevious = YYNEWLINE;
#if defined(__cplusplus) || defined(__STDC__)
int magic_yylook(void)
#else
magic_yylook()
#endif
{
	register struct yysvf *magic_yystate, **lsp;
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
	if (!magic_yymorfg)
		yylastch = magic_yytext;
	else {
		magic_yymorfg=0;
		yylastch = magic_yytext+magic_yyleng;
		}
	for(;;){
		lsp = magic_yylstate;
		magic_yyestate = magic_yystate = magic_yybgin;
		if (magic_yyprevious==YYNEWLINE) magic_yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(magic_yyout,"state %d\n",magic_yystate-magic_yysvec-1);
# endif
			yyt = magic_yystate->yystoff;
			if(yyt == magic_yycrank && !yyfirst){  /* may not be any transitions */
				yyz = magic_yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == magic_yycrank)break;
				}
#ifndef __cplusplus
			*yylastch++ = yych = input();
#else
			*yylastch++ = yych = lex_input();
#endif
#ifdef YYISARRAY
			if(yylastch > &magic_yytext[YYLMAX]) {
				fprintf(magic_yyout,"Input string too long, limit %d\n",YYLMAX);
				exit(1);
			}
#else
			if (yylastch >= &magic_yytext[ yytextsz ]) {
				int	x = yylastch - magic_yytext;

				yytextsz += YYTEXTSZINC;
				if (magic_yytext == yy_tbuf) {
				    magic_yytext = (char *) malloc(yytextsz);
				    memcpy(magic_yytext, yy_tbuf, sizeof (yy_tbuf));
				}
				else
				    magic_yytext = (char *) realloc(magic_yytext, yytextsz);
				if (!magic_yytext) {
				    fprintf(magic_yyout,
					"Cannot realloc magic_yytext\n");
				    exit(1);
				}
				yylastch = magic_yytext + x;
			}
#endif
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(magic_yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (uintptr_t)yyt > (uintptr_t)magic_yycrank){
				yyt = yyr + yych;
				if (yyt <= magic_yytop && yyt->verify+magic_yysvec == magic_yystate){
					if(yyt->advance+magic_yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = magic_yystate = yyt->advance+magic_yysvec;
					if(lsp > &magic_yylstate[YYLMAX]) {
						fprintf(magic_yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((uintptr_t)yyt < (uintptr_t)magic_yycrank) {	/* r < magic_yycrank */
				yyt = yyr = magic_yycrank+(magic_yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(magic_yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= magic_yytop && yyt->verify+magic_yysvec == magic_yystate){
					if(yyt->advance+magic_yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = magic_yystate = yyt->advance+magic_yysvec;
					if(lsp > &magic_yylstate[YYLMAX]) {
						fprintf(magic_yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				yyt = yyr + YYU(magic_yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(magic_yyout,"try fall back character ");
					allprint(YYU(magic_yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= magic_yytop && yyt->verify+magic_yysvec == magic_yystate){
					if(yyt->advance+magic_yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = magic_yystate = yyt->advance+magic_yysvec;
					if(lsp > &magic_yylstate[YYLMAX]) {
						fprintf(magic_yyout,"Input string too long, limit %d\n",YYLMAX);
						exit(1);
					}
					goto contin;
					}
				}
			if ((magic_yystate = magic_yystate->yyother) && (yyt= magic_yystate->yystoff) != magic_yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(magic_yyout,"fall back to state %d\n",magic_yystate-magic_yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(magic_yyout,"state %d char ",magic_yystate-magic_yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(magic_yyout,"stopped at %d with ",*(lsp-1)-magic_yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > magic_yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (magic_yyfnd= (*lsp)->yystops) && *magic_yyfnd > 0){
				magic_yyolsp = lsp;
				if(magic_yyextra[*magic_yyfnd]){		/* must backup */
					while(magic_yyback((*lsp)->yystops,-*magic_yyfnd) != 1 && lsp > magic_yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				magic_yyprevious = YYU(*yylastch);
				magic_yylsp = lsp;
				magic_yyleng = yylastch-magic_yytext+1;
				magic_yytext[magic_yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(magic_yyout,"\nmatch ");
					sprint(magic_yytext);
					fprintf(magic_yyout," action %d\n",*magic_yyfnd);
					}
# endif
				return(*magic_yyfnd++);
				}
			unput(*yylastch);
			}
		if (magic_yytext[0] == 0  /* && feof(magic_yyin) */)
			{
			magic_yysptr=magic_yysbuf;
			return(0);
			}
#ifndef __cplusplus
		magic_yyprevious = magic_yytext[0] = input();
		if (magic_yyprevious>0)
			output(magic_yyprevious);
#else
		magic_yyprevious = magic_yytext[0] = lex_input();
		if (magic_yyprevious>0)
			lex_output(magic_yyprevious);
#endif
		yylastch=magic_yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
#if defined(__cplusplus) || defined(__STDC__)
int magic_yyback(int *p, int m)
#else
magic_yyback(p, m)
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
int magic_yyinput(void)
#else
magic_yyinput()
#endif
{
#ifndef __cplusplus
	return(input());
#else
	return(lex_input());
#endif
	}
#if defined(__cplusplus) || defined(__STDC__)
void magic_yyoutput(int c)
#else
magic_yyoutput(c)
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
void magic_yyunput(int c)
#else
magic_yyunput(c)
   int c; 
#endif
{
	unput(c);
	}
