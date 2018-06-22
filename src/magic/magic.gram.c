// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

# line 10 "magic.gram.y"

#include "node.h"
#include "stack.h"
#include "wjunk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Warray-bounds"

extern int yylex(void);
extern int yyparse(void);

# define YYLMAX BUFSIZ

extern char   *tempID;
extern char    fileName[];
extern char   *newStringId(char *, char *);
extern char    yytext[];
extern int     columnNumber;
extern int     ignoreEOL;
extern int     lineNumber;
extern int     yychar;
extern int     yyleng;
extern node   *codeTree;
extern node   *doingNode;
int            envLevel = 0;
int            loopLevel = 0;

#ifdef DEBUG
extern int      Pflag;
#define PPRINT0(str) \
	if (Pflag) fprintf(stderr,str)
#define PSHOWTREE(arg1) \
        if (2 <= Pflag) showTree(0,"psr: ",arg1)
#else 
#define PPRINT0(str)
#define PSHOWTREE(arg1)
#endif 
        
# define AND 257
# define BAD 258
# define BOMB 259
# define BREAK 260
# define CL 261
# define CM 262
# define CMLIST 263
# define DIV 264
# define DO 265
# define DONT 266
# define DOLLAR 267
# define ELSE 268
# define ELSEIF 269
# define ENDFILE 270
# define EOL 271
# define EQ 272
# define EXIT 273
# define FI 274
# define GE 275
# define GT 276
# define ID 277
# define IF 278
# define IGNORE 279
# define LB 280
# define LC 281
# define LE 282
# define LP 283
# define LT 284
# define MINUS 285
# define MOD 286
# define MULT 287
# define NE 288
# define NOT 289
# define OD 290
# define OR 291
# define PLUS 292
# define POP 293
# define PUSH 294
# define RB 295
# define RC 296
# define REAL 297
# define REPEAT 298
# define RP 299
# define SHOW 300
# define SIZE 301
# define SQRT 302
# define STRING 303
# define THEN 304
# define TRUNC 305
# define TYPEOF 306
# define UNIT 307
# define UNTIL 308
# define WHILE 309
# define _Car 310
# define _Ca_ 311
# define _C_r 312
# define _C__ 313
# define _NAME 314
# define _NEG 315
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 300
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 810 "magic.gram.y"


static void yyerror(const char *m)
{   extern int  ignoreEOL;
    node        N;
    node       *oldNode;

    N.location = yylval.tval.location;
    oldNode    = doingNode;
    doingNode  = &N;
    ignoreEOL  = 0;
    if (strlen(yytext)==1)
    {
        if (yytext[0] == '\n')
            WerrprintfWithPos("Syntax error! Unexpected end of line");
        else if ((' '<=yytext[0]) && (yytext[0]<='~'))
            WerrprintfWithPos("Syntax error! Unexpected '%c'",yytext[0]);
        else if (yytext[0] == '\004')
            WerrprintfWithPos("Syntax error! Unexpected end of file");
        else
            WerrprintfWithPos("Syntax error! Unexpected '\\%03o'",yytext[0]);
    }
    else if ( (strlen(yytext)==2) && (yytext[0] == '\n') && (yytext[1] == '\n') )
        WerrprintfWithPos("Syntax error! String is too long. Maximun is %d characters", YYLMAX);
    else if ( (yyleng==2) && (yytext[0] == '\n') && (yytext[1] == '\004') )
        WerrprintfWithPos("Syntax error! Unclosed comment using /* ");
    else
        WerrprintfWithPos("Syntax error! Unexpected \"%s\"",yytext);
    doingNode = oldNode;
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 90
# define YYLAST 423
short yyact[]={

   8,  13,  72,  72,  72,  72,  72,  17,  23, 135,
 134,  99,  94, 102,  12, 137, 153,  71,  22,  19,
  16, 101,  24,  72, 101,  82,   5,  72,  98,   8,
  13, 120,  81,  33,  15,  14,  17,  23,  72,  21,
   4,   3, 137,  12,  52,  30, 131,  22,  19,  16,
  20,  24,   8,  13, 103,  84,  31, 120, 102,  17,
  23, 156, 157,  15,  14, 101,  12,  33,  21,  75,
  22,  19,  16,  41,  24,   8,  13,  85,  83,  20,
  28,  27,  17,  23,  32,  76,  15,  14,  73,  12,
 137,  21,  44,  22,  19,  16, 144,  24,   8,  13,
  68,  23,  20, 146, 119,  17,  23,  39,  61,  15,
  14,  22,  12,  36,  21,  24,  22,  19,  16,  38,
  24, 136,  35, 126,  46,  20,  56,   8,  13, 150,
  55,  54,  15,  14,  17,  23,  86,  21,   2,  51,
   1,  12, 127,  63,  11,  22,  19,  16,  20,  24,
 149,   8,  13,  10,   9, 113, 114,  74,  17,  23,
   0,  15,  14,  25,   0,  12,  21,   0,   0,  22,
  19,  16,   0,  24,   8,  13, 128,  20,  93,   0,
   0,  17,  23, 107, 108,  15,  14, 106,  12,   0,
  21,   0,  22,  19,  16, 105,  24,   0,  80,  79,
  95,  20, 130,  42,  23,  78,   0,  77,  15,  14,
 104,   0,   0,  21,  22,  23,   6,  52,  24,  26,
  33, 100,  43,  40,  20,  22,  37,   0,  52,  24,
 142,  33, 145,  43,  57,   0,   0,   0,  49,  47,
  58,   0,  48,  50,  59,  57,  23,  67,   7,  49,
  47,  58,   0,  48,  50,  59,  22,  34,  60,  52,
  24, 159,  33,  53,  18, 143,  18,  66, 152,   0,
 154,  64,  65,   0, 148,   0,  57,   0,   0,  69,
  49,  47,  58,  45,  48,  50,  59, 115, 116, 117,
   0,  29, 155,  87,  18, 158,  62,   0,   0,  70,
   0, 109, 110, 111, 112,   0,   0,   0,   0, 118,
   0,  97,  26,   0,  88,   0,   0,   0, 129,   0,
   0,  26,   0,   0,   0,  18,   0,   0,   0,   0,
  96,  89,  90,  91,  92,  18,   0, 121, 122, 123,
 124, 125,   0, 138, 139, 140, 141,   0,   0, 132,
   0,   0, 133,   0,   0,   0,   0,   0,   0,  18,
  26,   0,   0,   0,   0,  26,   0,  18,  18,   0,
   0,   0,  26,   0,   0,  26,   0,   0,   0,   0,
  69,   0, 147,   0,   0,   0,   0,   0,   0,   0,
  18,   0, 151,   0,   0,  18,   0,   0,   0,  18,
   0,   0,   0,   0,   0, 147,   0,  18,   0,   0,
   0,   0,  18,   0,   0,   0,   0,  18,   0,  18,
  18,   0,  18 };
short yypact[]={

-230,-1000,-108,-1000,-1000,-190,-1000,-216,-1000, -63,
 -63,-1000,-250,-1000,-1000,-1000,-1000,-1000,-236,-1000,
-1000,-1000,-1000, -63, -63,-1000,-1000,-1000,-1000, -63,
-166, -63,-1000,-1000,-287,-169,-1000, -52,-203, -77,
-260,-209,-1000, -21,-1000, -63, -63,-250,-250,-250,
-250,-1000,-1000,-236,-1000,-1000,-1000,-1000,-1000,-1000,
-253, -85, -63, -63,-268,-285,-241,-289,-208,-1000,
-197, -85, -63, -63,-203, -52, -52, -52, -52, -52,
 -52, -52, -52, -52, -52, -52,-1000,-286,-238, -63,
 -63, -63, -63, -63,-1000,-132,-241,-264,-1000,-1000,
-215, -63,-1000,-166,-259,-169,-1000, -77, -77,-260,
-260,-260,-260,-209,-209,-1000,-1000,-1000,-1000,-1000,
-1000,-286,-286,-286,-286,-264, -85,-1000,-1000,-1000,
-1000,-166,-289,-1000, -63, -85,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-161, -63,-208,-232,-288,-184,-1000,
-1000,-289,-1000, -85,-1000,-207, -85, -63, -85,-1000 };
short yypgo[]={

   0, 154, 121, 153, 150, 144, 142, 124, 104, 283,
 221, 140, 216, 138, 103, 267, 247, 122, 113, 119,
 107, 223,  73, 203,  92, 248, 139, 100, 263, 131,
 130, 126, 123, 108,  96 };
short yyr1[]={

   0,  11,  11,  11,  11,  11,  12,  12,  12,  12,
  12,  12,  12,  12,  12,  12,  32,  12,  33,  34,
  12,  12,  12,  12,  12,  12,  12,  12,  14,  14,
  14,  13,  13,  15,  15,  16,  16,  17,  17,  18,
  18,  19,  19,  19,  20,  20,  20,  20,  20,  21,
  21,  21,  22,  22,  22,  22,  23,  23,  24,  24,
  24,  24,  24,  24,  24,  25,  25,  26,  26,  26,
  26,  26,  27,  27,  28,  28,  28,  29,  30,  31,
   1,   2,   3,   4,   5,   6,   7,   8,   9,  10 };
short yyr2[]={

   0,   2,   1,   1,   2,   2,   6,   4,   3,   1,
   3,   1,   2,   7,   7,   5,   0,   6,   0,   0,
   6,   4,   1,   1,   1,   1,   1,   1,   3,   5,
   5,   2,   1,   3,   1,   3,   1,   3,   1,   2,
   1,   3,   3,   1,   3,   3,   3,   3,   1,   3,
   3,   1,   3,   3,   3,   1,   2,   1,   3,   3,
   4,   4,   4,   4,   1,   4,   1,   4,   1,   1,
   1,   1,   3,   1,   1,   3,   3,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1 };
short yychk[]={

-1000, -11, -13, 271, 270, 256, -12, -25, 259,  -1,
  -3,  -5, 273, 260, 294, 293, 279, 266, -28, 278,
 309, 298, 277, 267, 281, 271, -12, 271, 270,  -9,
 261, 272, 300, 283, -16, -17, -18, 289, -19, -20,
 -21, -22, -23, 285, -24,  -9,  -7, 302, 305, 301,
 306, -26, 280, -28, -29, -30, -31, 297, 303, 307,
 -16, -33,  -9,  -7, -16, -16, -15, -16, -27, -25,
 -15, 304, 291, 257, -19, 272, 288, 284, 282, 276,
 275, 292, 285, 287, 264, 286, -24, -16, -15,  -9,
  -9,  -9,  -9,  -7, 265, -13, -15, -16, 296, 296,
 -10, 262, 299, 262, -13, -17, -18, -20, -20, -21,
 -21, -21, -21, -22, -22, -23, -23, -23, -10,  -8,
 295, -16, -16, -16, -16, -16, -32,  -6, 308, -10,
  -8, 261, -16, -25, 269, 268,  -2, 274, -10, -10,
 -10, -10,  -8, -13, -34, -27, -14, -16, -13,  -4,
 290, -16,  -2, 304,  -2, -13, 268, 269, -13, -14 };
short yydef[]={

   0,  -2,   0,   2,   3,   0,  32,   9,  11,   0,
   0,  18,  22,  23,  24,  25,  26,  27,  66,  80,
  82,  84,  74,   0,   0,   1,  31,   4,   5,   0,
   0,   0,  12,  88,   0,  36,  38,   0,  40,  43,
  48,  51,  55,   0,  57,   0,   0,   0,   0,   0,
   0,  64,  86,  68,  69,  70,  71,  77,  78,  79,
   0,   0,   0,   0,   0,   0,   0,  34,   8,  73,
  10,   0,   0,   0,  39,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  56,   0,   0,   0,
   0,   0,   0,   0,  16,   0,   0,   0,  75,  76,
   7,   0,  89,   0,   0,  35,  37,  41,  42,  44,
  45,  46,  47,  49,  50,  52,  53,  54,  58,  59,
  87,   0,   0,   0,   0,   0,   0,  19,  85,  21,
  65,   0,  33,  72,   0,   0,  15,  81,  60,  61,
  62,  63,  67,   0,   0,   6,   0,   0,   0,  17,
  83,  20,  13,   0,  14,  28,   0,   0,  29,  30 };
/*
*/
#define YYFLAG    -1000
#define YYACCEPT  return(0)
#define YYABORT   return(1)

/*******************************************************************************
*                                                                              *
*       yyparse/0                                                              *
*                                                                              *
*       A restartable version of the yacc parser engine.  A bit slower than    *
*       the regular engine, but keeps state in static area so parsing may      *
*       be resumed if suspended.                                               *
*                                                                              *
*       yyparse returns...                                                     *
*                                                                              *
*           0    If "accepting"                                                *
*           1    If not "accepting"                                            *
*           2    If suspending                                                 *
*                                                                              *
*******************************************************************************/
/*   The suspend feature causes problems with acquisition messages.  For example, if
 *   a close parenthesis is omitted from the command line, an acquisition message
 *   can be added as an argument to the command.  Also, if someone types
 *   "while (condition) do stuff" and forgets the endwhile.  All future comamnd
 *   will be added to the body of the while, with no execution until an endwhile
 *   is entered.  For some, this might be a feature, but for most this is a bug.
 *   Note that for macros, multiline commands are okay.  Macros must contain
 *   complete commands.
 *   The suspend feature of the parser has been left in the code.  It can be
 *   reactivated with the SUSP ifdef.
 */

#ifdef YYDEBUG
int             yydebug = 0;		/* set to 1 for debugging             */
#endif 
YYSTYPE         yyv[YYMAXDEPTH];	/* where the values are stored        */
int             yychar  = -1;		/* current input token number         */
int             yynerrs = 0;		/* number of errors                   */
int             yyEntry = 0;		/* where suspended (0, 1 or 2)        */
short           yyerrflag = 0;		/* error recovery flag                */
short           yys[YYMAXDEPTH];	/* state stack                        */

#ifdef SUSP
static short    yySVstate,
	       *yySVps,	
		yySVn;
static short   *yySVxi;
static YYSTYPE *yySVpvt;
static YYSTYPE *yySVpv;
#endif 

int yyparse()
{  short             yyj, yym;
   register short    yystate,
	            *yyps,
		     yyn;
   register short   *yyxi;
   register YYSTYPE *yypvt;
   register YYSTYPE *yypv;

#ifdef SUSP
   if (yyEntry == 0)			/* suspended?                         */
   {
#endif 
      yychar    = -1;
      yynerrs   = 0;
      yyerrflag = 0;
      yyps      = &yys[-1];
      yypv      = &yyv[-1];
      yystate   = 0;			/* no:  initialize and go             */
#ifdef SUSP
   }
   else
   {  yystate = yySVstate;		/* yes: restore state info and resume */
      yyps    = yySVps;
      yyn     = yySVn;
      yyxi    = yySVxi;
      yypvt   = yySVpvt;
      yypv    = yySVpv;
      if (yyEntry == 1)
      {  yyEntry = 0;
         goto yySusp1;
      }
      else
      {  yyEntry = 0;
         goto yySusp2;
      }
   }
#endif 

yystack:				/* push state and value */

#ifdef YYDEBUG
   if (yydebug)
      printf("state %d, char 0%o\n",yystate,yychar);
#endif 
#ifdef MACOS
   ++yyps;
#else
   if (&yys[YYMAXDEPTH-4] < ++yyps)
   {  yyerror( "yacc stack overflow" );
      WerrprintfWithPos("Macro too long and complicated");
      return(1);
   }
#endif
   *yyps = yystate;
   yypv += 1;
   *yypv = yyval;

yynewstate:

   yyn = yypact[yystate];
   if (yyn <= YYFLAG )
      goto yydefault;			/* simple state */
   if (yychar < 0)
#ifdef SUSP
yySusp1:
#endif
         if ((yychar=yylex()) < 0)	/* go to lexer... */
         {
#ifdef SUSP
            yyEntry   = 1;		/* suspended! note where and return */
	    yySVstate = yystate;
	    yySVps    = yyps;
	    yySVn     = yyn;
	    yySVxi    = yyxi;
	    yySVpvt   = yypvt;
	    yySVpv    = yypv;
	    return(2);
#else 
            yytext[0] = '\n';
            yytext[1] = '\0';
            yyerror( "\n" );
	    return(1);
#endif 
         }
   if ((yyn += yychar)<0 || YYLAST <= yyn)
      goto yydefault;
   if (yychk[yyn=yyact[yyn]] == yychar)	/* valid shift */
   {  yychar  = -1;
      yyval   = yylval;
      yystate = yyn;
      if (0 < yyerrflag)
	--yyerrflag;
      goto yystack;
   }

yydefault:				/* default state action */

   if ((yyn=yydef[yystate]) == -2)
   {  if (yychar<0)
#ifdef SUSP
yySusp2:
#endif 
         if ((yychar=yylex()) < 0)	/* go to lexer... */
         {
#ifdef SUSP
            yyEntry   = 2;		/* suspended! note where and return */
	    yySVstate = yystate;
	    yySVps    = yyps;
	    yySVn     = yyn;
	    yySVxi    = yyxi;
	    yySVpvt   = yypvt;
	    yySVpv    = yypv;
	    return(2);
#else 
            yytext[0] = '\n';
            yytext[1] = '\0';
            yyerror( "\n" );
	    return(1);
#endif 
         }

		/* look through exception table */

      for (yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 )
	 ;
      while (0 <= *(yyxi+=2))
      {  if (*yyxi == yychar )
	    break;
      }
      if ((yyn = yyxi[1]) < 0 )
	 return(0);			/* accept */
   }

   if (yyn == 0 )			/* error */
   {  switch (yyerrflag)		/* ... attempt to resume parsing */
      { case 0:   yyerror("syntax error");/* brand new error */
	          yynerrs += 1;
	case 1:
	case 2:   yyerrflag = 3;	/* incompletely recovered error ... try again */

			/* find a state where "error" is a legal shift action */

		  while (yys <= yyps)
		  {  yyn = yypact[*yyps] + YYERRCODE;
		     if (0 <= yyn && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE)
		     {  yystate = yyact[yyn];  /* simulate a shift of "error" */
			goto yystack;
		     }
		     yyn = yypact[*yyps]; /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
		     if (yydebug )
			printf("error recovery pops state %d, uncovers %d\n",*yyps,yyps[-1]);
#endif 
		     yyps -= 1;
		     yypv -= 1;
		  }

			

        yyabort:  return(1);			/* no pushed state with 'error' ... abort */
	case 3:   /* no shift yet; clobber input char */

#ifdef YYDEBUG
		  if (yydebug )
		     printf("error recovery discards char %d\n",yychar);
#endif 
		  if (yychar == 0 )
		     goto yyabort;		/* don't discard EOF, quit */
		  yychar = -1;
		  goto yynewstate;		/* try again in the same state */

      }
   }

	/* reduction by production yyn */

#ifdef YYDEBUG
   if (yydebug )
      printf("reduce %d\n",yyn);
#endif 
   yyps -= yyr2[yyn];
   yypvt = yypv;
   yypv -= yyr2[yyn];
   yyval = yypv[1];
   yym   = yyn;
   yyn   = yyr1[yyn];			/* consult goto table for next state */
   yyj   = yypgo[yyn] + *yyps + 1;
   if (YYLAST<=yyj || yychk[yystate=yyact[yyj]] != -yyn )
      yystate = yyact[yypgo[yyn]];
   switch (yym)				/* ready... ACTION! */
   {
			
case 1:
# line 71 "magic.gram.y"
{
                            PPRINT0("psr: start      : sList EOL\n");
                            codeTree = yypvt[-1].nval;
                            YYACCEPT;
                         } break;
case 2:
# line 77 "magic.gram.y"
{
                            PPRINT0("psr: start      : EOL\n");
                            codeTree = newNode(EOL,&(yypvt[-0].tval.location));
                            YYACCEPT;
                         } break;
case 3:
# line 83 "magic.gram.y"
{
                            PPRINT0("psr: start      : ENDFILE\n");
                            codeTree = newNode(ENDFILE,&(yypvt[-0].tval.location));
                            YYACCEPT;
                         } break;
case 4:
# line 89 "magic.gram.y"
{
                            PPRINT0("psr: start      : error EOL\n");
                            codeTree = NULL;
                            YYABORT;
                         } break;
case 5:
# line 95 "magic.gram.y"
{
                            PPRINT0("psr: start      : error ENDFILE\n");
                            codeTree = NULL;
                            YYABORT;
                         } break;
case 6:
# line 103 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP CL lValList\n");
                            p = newNode(_Car,&(yypvt[-4].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-5].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 7:
# line 114 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP\n");
                            p = newNode(_Ca_,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 8:
# line 124 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal CL lValList\n");
                            p = newNode(_C_r,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 9:
# line 134 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal\n");
                            p = newNode(_C__,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 10:
# line 143 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal EQ expList\n");
                            p = newNode(EQ,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 11:
# line 153 "magic.gram.y"
{  yyval.nval = newNode(BOMB,&(yypvt[-0].tval.location));
                            PSHOWTREE(yyval.nval);
                         } break;
case 12:
# line 157 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal SHOW \n");
                            p = newNode(SHOW,&(yypvt[-0].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 13:
# line 166 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList ELSEIF sList2 FI\n");
                            p = newNode(ELSE,&(yypvt[-6].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-5].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 14:
# line 177 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList ELSE sList FI\n");
                            p = newNode(ELSE,&(yypvt[-6].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-5].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 15:
# line 188 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList FI\n");
                            p = newNode(THEN,&(yypvt[-4].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 16:
# line 198 "magic.gram.y"
{  loopLevel += 1;
                         } break;
case 17:
# line 201 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : WHILE exp DO sList OD\n");
                            loopLevel -= 1;
                            p          = newNode(WHILE,&(yypvt[-5].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-4].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 18:
# line 212 "magic.gram.y"
{  loopLevel += 1;
                         } break;
case 19:
# line 215 "magic.gram.y"
{  loopLevel -= 1;
                         } break;
case 20:
# line 218 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : REPEAT sList UNTIL exp\n");
                            p = newNode(REPEAT,&(yypvt[-5].tval.location));
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 21:
# line 228 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: stmnt       : EXIT LP expList RP\n");
                            p = newNode(EXIT,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 22:
# line 237 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : EXIT\n");
                            yyval.nval = newNode(EXIT,&(yypvt[-0].tval.location));
                            PSHOWTREE(yyval.nval);
                         } break;
case 23:
# line 243 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : BREAK\n");
                            if (loopLevel)
                                yyval.nval = newNode(BREAK,&(yypvt[-0].tval.location));
                            else
                            {   WerrprintfWithPos("Syntax error! BREAK outside loop");
                                yyval.nval = NULL;
                            }
                            PSHOWTREE(yyval.nval);
                         } break;
case 24:
# line 254 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : PUSH\n");
                            envLevel += 1;
                            yyval.nval        = newNode(PUSH,&(yypvt[-0].tval.location));
                            PSHOWTREE(yyval.nval);
                         } break;
case 25:
# line 261 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : POP\n");
                            if (envLevel)
                            {   envLevel -= 1;
                                yyval.nval        = newNode(POP,&(yypvt[-0].tval.location));
                            }
                            else
                            {   fprintf(stderr,"magic: POP without PUSH\n");
                                yyval.nval = NULL;
                            }
                            PSHOWTREE(yyval.nval);
                         } break;
case 26:
# line 274 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : IGNORE\n");
                            yyval.nval = newNode(IGNORE,&(yypvt[-0].tval.location));
                            PSHOWTREE(yyval.nval);
                         } break;
case 27:
# line 280 "magic.gram.y"
{
                            PPRINT0("psr: stmnt      : DONT\n");
                            yyval.nval = newNode(DONT,&(yypvt[-0].tval.location));
                            PSHOWTREE(yyval.nval);
                         } break;
case 28:
# line 288 "magic.gram.y"
{  node *p;
                            PPRINT0("psr: sList2      : exp THEN sList\n");

                            p = newNode(THEN,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
			} break;
case 29:
# line 298 "magic.gram.y"
{  node *p;
                            PPRINT0("psr: sList2      : exp THEN sList ELSE sList\n");
                            p = newNode(ELSE,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            addLeftSon(p,yypvt[-4].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 30:
# line 308 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: sList2      : exp THEN sList ELSEIF sList2\n");
                            p = newNode(ELSE,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            addLeftSon(p,yypvt[-4].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 31:
# line 320 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: sList      : sList stmnt\n");
                            if (yypvt[-1].nval)
                                if (yypvt[-0].nval)
                                {   p = newNode(CM,&(yypvt[-0].nval->location));
                                    addLeftSon(p,yypvt[-0].nval);
                                    addLeftSon(p,yypvt[-1].nval);
                                    yyval.nval = p;
                                }
                                else
                                    yyval.nval = yypvt[-1].nval;
                            else
                                yyval.nval = yypvt[-0].nval;
                            PSHOWTREE(p);
                         } break;
case 32:
# line 337 "magic.gram.y"
{
                            PPRINT0("psr: sList      : stmnt\n");
                            yyval.nval = yypvt[-0].nval;
                         } break;
case 33:
# line 344 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: expList    : expList CM exp\n");
                            p = newNode(CM,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 34:
# line 354 "magic.gram.y"
{
                            PPRINT0("psr: expList    : exp\n");
                            yyval.nval = yypvt[-0].nval;
                         } break;
case 35:
# line 361 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: exp        : exp OR lTerm\n");
                            p = newNode(OR,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 36:
# line 371 "magic.gram.y"
{
                            PPRINT0("psr: exp        : lTerm\n");
                            yyval.nval = yypvt[-0].nval;
                         } break;
case 37:
# line 378 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lTerm      : lTerm AND lFact\n");
                            p = newNode(AND,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 38:
# line 388 "magic.gram.y"
{
                            PPRINT0("psr: lTerm      : lFact\n");
                            yyval.nval = yypvt[-0].nval;
                         } break;
case 39:
# line 395 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lFact      : NOT lAtom\n");
                            p = newNode(NOT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 40:
# line 404 "magic.gram.y"
{
                            PPRINT0("psr: lFact      : lAtom\n");
                         } break;
case 41:
# line 410 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lAtom      : lAtom EQ order\n");
                            p = newNode(EQ,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 42:
# line 420 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lAtom      : lAtom NE order\n");
                            p = newNode(NE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 43:
# line 430 "magic.gram.y"
{
                            PPRINT0("psr: lAtom      : order\n");
                         } break;
case 44:
# line 436 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: order      : order LT arith\n");
                            p = newNode(LT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 45:
# line 446 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: order      : order LE arith\n");
                            p = newNode(LE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 46:
# line 456 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: order      : order GT arith\n");
                            p = newNode(GT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 47:
# line 466 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: order      : order GE arith\n");
                            p = newNode(GE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 48:
# line 476 "magic.gram.y"
{
                            PPRINT0("psr: order       : arith\n");
                         } break;
case 49:
# line 482 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: arith      : arith PLUS term\n");
                            p = newNode(PLUS,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 50:
# line 492 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: arith      : arith MINUS term\n");
                            p = newNode(MINUS,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 51:
# line 502 "magic.gram.y"
{
                            PPRINT0("psr: arith      : term\n");
                         } break;
case 52:
# line 508 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: term       : term MULT factor\n");
                            p = newNode(MULT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 53:
# line 518 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: term       : term DIV factor\n");
                            p = newNode(DIV,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 54:
# line 528 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: term       : term MOD factor\n");
                            p = newNode(MOD,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 55:
# line 538 "magic.gram.y"
{
                            PPRINT0("psr: term       : factor\n");
                         } break;
case 56:
# line 544 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: factor     : MINUS atom\n");
                            p = newNode(_NEG,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 57:
# line 553 "magic.gram.y"
{
                            PPRINT0("psr: factor     : atom\n");
                         } break;
case 58:
# line 559 "magic.gram.y"
{
                            PPRINT0("psr: atom       : LP exp RP\n");
                            yyval.nval = yypvt[-1].nval;
                         } break;
case 59:
# line 564 "magic.gram.y"
{  node *p;
                            PPRINT0("psr: atom       : LB expList RB\n");
                            p = newNode(CMLIST,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 60:
# line 572 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: atom       : SQRT LP exp RP\n");
                            p = newNode(SQRT,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 61:
# line 581 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: atom       : TRUNC LP exp RP\n");
                            p = newNode(TRUNC,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 62:
# line 590 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: atom       : SIZE LP exp RP\n");
                            p = newNode(SIZE,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 63:
# line 599 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: atom       : TYPEOF LP exp RP\n");
                            p = newNode(TYPEOF,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 64:
# line 608 "magic.gram.y"
{
                            PPRINT0("psr: atom       : rVal\n");
                            yyval.nval = yypvt[-0].nval;
                         } break;
case 65:
# line 615 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lVal       : name LB exp RB\n");
                            p = newNode(LB,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 66:
# line 625 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lVal       : name\n");
                            p = newNode(ID,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 67:
# line 636 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: rVal       : name LB exp RB\n");
                            p = newNode(LB,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 68:
# line 646 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: rVal       : name\n");
                            p = newNode(ID,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 69:
# line 655 "magic.gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : REAL\n");
                            p      = newNode(REAL,&(yypvt[-0].dval.location));
                            p->v.r = yypvt[-0].dval.value;
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 70:
# line 664 "magic.gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : string\n");
                            p      = newNode(STRING,&(yypvt[-0].cval.location));
                            p->v.s = yypvt[-0].cval.value;
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 71:
# line 673 "magic.gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : unit\n");
                            p      = newNode(UNIT,&(yypvt[-0].cval.location));
                            p->v.s = yypvt[-0].cval.value;
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 72:
# line 684 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: lValList   : lValList CM lVal\n");
                            p = newNode(CM,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 73:
# line 694 "magic.gram.y"
{
                            PPRINT0("psr: lValList   : lVal\n");
                         } break;
case 74:
# line 700 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: name       : ID\n");
                            p      = newNode(_NAME,&(yypvt[-0].tval.location));
                            p->v.s = newStringId(yytext,tempID);
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 75:
# line 709 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: name       : DOLLAR exp RC\n");
                            p = newNode(DOLLAR,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 76:
# line 718 "magic.gram.y"
{  node *p;

                            PPRINT0("psr: name       : LC exp RC\n");
                            p = newNode(LC,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 77:
# line 729 "magic.gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: real        : REAL\n");
                            tmp.dval.value    = atof(yytext);
                            tmp.dval.location = yypvt[-0].tval.location;
                            yyval.dval                = tmp.dval;
                         } break;
case 78:
# line 739 "magic.gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: string     : STRING\n");
                            yytext[strlen(yytext)-1] = '\0';
                            tmp.cval.value           = newStringId(&(yytext[1]),tempID);
                            tmp.cval.location        = yypvt[-0].tval.location;
                            yyval.cval                       = tmp.cval;
                         } break;
case 79:
# line 750 "magic.gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: unit     : UNIT\n");
                            yytext[strlen(yytext)] = '\0';
                            tmp.cval.value           = newStringId(&(yytext[0]),tempID);
                            tmp.cval.location        = yypvt[-0].tval.location;
                            yyval.cval                       = tmp.cval;
                         } break;
case 80:
# line 761 "magic.gram.y"
{  ignoreEOL += 1;
                         } break;
case 81:
# line 766 "magic.gram.y"
{  ignoreEOL -= 1;
                         } break;
case 82:
# line 771 "magic.gram.y"
{  ignoreEOL += 1;
                         } break;
case 83:
# line 776 "magic.gram.y"
{  ignoreEOL -= 1;
                         } break;
case 84:
# line 781 "magic.gram.y"
{  ignoreEOL += 1;
                         } break;
case 85:
# line 786 "magic.gram.y"
{  ignoreEOL -= 1;
                         } break;
case 86:
# line 791 "magic.gram.y"
{  ignoreEOL += 1;
                         } break;
case 87:
# line 796 "magic.gram.y"
{  ignoreEOL -= 1;
                         } break;
case 88:
# line 801 "magic.gram.y"
{  ignoreEOL += 1;
                         } break;
case 89:
# line 806 "magic.gram.y"
{  ignoreEOL -= 1;
                         } break;
   }
   goto yystack;  /* stack new state and value */
}
