/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

# line 10 "magical_gram.y"

#include <stdio.h>

#include "node.h"
#include "stack.h"
#include "magical_io.h"

extern char    fileName[];
extern char   *newStringId();
extern char    magic_yytext[];
extern double  atof();
extern int     columnNumber;
extern int     ignoreEOL;
extern int     lineNumber;
extern int     magic_yychar;
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
        
#include "magic.gram.h"

#define yyclearin magic_yychar = -1
#define yyerrok magic_yyerrflag = 0
extern int magic_yychar;
extern short magic_yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE magic_yylval, magic_yyval;
# define YYERRCODE 256

# line 760 "magical_gram.y"


void
magic_yyerror(m)
  char *m;
{
    node        N;
    node       *oldNode;

    N.location = magic_yylval.tval.location;
    oldNode    = doingNode;
    doingNode  = &N;
    ignoreEOL  = 0;
    if (strlen(magic_yytext)==1)
        if (magic_yytext[0] == '\n')
            WerrprintfWithPos("Syntax error! Unexpected end of line");
        else if ((' '<=magic_yytext[0]) && (magic_yytext[0]<='~'))
            WerrprintfWithPos("Syntax error! Unexpected '%c'",magic_yytext[0]);
        else if (magic_yytext[0] == '\004')
            WerrprintfWithPos("Syntax error! Unexpected end of file");
        else
            WerrprintfWithPos("Syntax error! Unexpected '\\%03o'",magic_yytext[0]);
    else
        WerrprintfWithPos("Syntax error! Unexpected \"%s\"",magic_yytext);
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

	yaccrtn = magic_yyparse();
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
*       magic_yyparse (suspended) for dealing with the notifier driven environment.   
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
   switch (magic_yyparse())
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
short magic_yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 85
# define YYLAST 381
short magic_yyact[]={

   5,  71,  71,   8,  13,  71,  33,  30,  71, 100,
  17,  23, 127,   4,   3,  70,  12,  31,  71, 101,
  22,  19,  16,  97,  24,   8,  13,  62,  33,  99,
  71,  99,  17,  23, 131,  96,  15,  14,  12, 133,
 128,  21,  22,  19,  16,  32,  24,   8,  13,  81,
  92,  83,  20,  72,  17,  23,  80, 132,  15,  14,
  12, 133,  74,  21,  22,  19,  16, 100,  24,   8,
  13,  39,  84,  82,  20,  71,  17,  23,  75, 126,
  15,  14,  12,  28,  27,  21,  22,  19,  16, 140,
  24,  36,  61,   8,  13,  23,  20,  67,   2, 144,
  17,  23,  15,  14,  22,  59,  12,  21,  24, 122,
  22,  19,  16,  41,  24,  79,  78,  54,  20,  44,
  53,  38,  77,  52,  76,   6,  15,  14,  26,  50,
  35,  21,   8,  13,   1, 123,  11, 143,  10,  17,
  23, 124,  20,  25,  91,  12, 105, 106,   9,  22,
  19,  16,   7,  24,   8,  13,   0,   0,  93,  73,
   0,  17,  23,  85, 104,  15,  14,  12,   0, 102,
  21,  22,  19,  16,  98,  24,  23,  51,  18,   0,
  18,  20,   0,  68,   0,  22,   0,  15,  14,  24,
  42,  33,  21,  43, 111, 112,   0,  37,   0,   0,
 146, 138, 103,  20,   0,  55,  65,  23,  18,  48,
  46,  56,   0,  47,  49,  57,  22,   0,  23,  26,
  24, 139,  33,   0,  43,  40, 141,  22,  26,   0,
 142,  24,   0,  33,   0,   0,  55,  18,  69,  66,
  48,  46,  56,   0,  47,  49,  57,  55,  18,  34,
  58,  48,  46,  56, 130,  47,  49,  57,   0,   0,
   0, 116,   0,  63,  64,  26,   0,  94,  26, 125,
   0,  18,   0, 113, 114, 115,  45,   0,   0,  18,
  18,  68,   0,   0,  29,  86,   0,   0,   0,  60,
   0,   0, 134, 135, 136, 137,   0,   0,   0,   0,
  18,  95, 107, 108, 109, 110,  18,   0,   0,  18,
   0,   0,   0,   0,   0,   0,   0,  18,   0,   0,
  18,   0,   0,  87,  88,  89,  90, 117, 118, 119,
 120, 121,   0,   0,   0,   0,   0,   0,   0, 129,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 145 };
short magic_yypact[]={

-256,-1000,-127,-1000,-1000,-186,-1000,-254,-1000, -91,
 -91,-1000,-276,-1000,-1000,-1000,-1000,-1000,-252,-1000,
-1000,-1000,-1000, -91, -91,-1000,-1000,-1000,-1000, -91,
-172, -91,-1000,-1000,-288,-204,-1000, -60,-209,-159,
-235,-213,-1000, -49,-1000, -91,-276,-276,-276,-276,
-1000,-252,-1000,-1000,-1000,-1000,-1000,-1000,-215,-105,
 -91, -91,-1000,-260,-272,-231,-285,-243,-1000,-233,
-105, -91, -91,-209, -60, -60, -60, -60, -60, -60,
 -60, -60, -60, -60, -60,-1000,-289, -91, -91, -91,
 -91, -91,-1000,-166,-231,-282,-1000,-1000,-221, -91,
-1000,-172,-234,-204,-1000,-159,-159,-235,-235,-235,
-235,-213,-213,-1000,-1000,-1000,-1000,-289,-289,-289,
-289,-282,-105,-1000,-1000,-1000,-1000,-1000,-172,-285,
-1000,-105,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-190,
 -91,-243,-212,-1000,-1000,-285,-1000 };
short magic_yypgo[]={

   0, 148,  57, 138, 137, 136, 135,  92,  79, 276,
 174, 134, 125,  98, 206, 239, 130,  91, 121,  71,
 225, 113, 190, 119, 152, 129,  97, 177, 123, 120,
 117, 109, 105,  89 };
short magic_yyr1[]={

   0,  11,  11,  11,  11,  11,  12,  12,  12,  12,
  12,  12,  12,  12,  12,  31,  12,  32,  33,  12,
  12,  12,  12,  12,  12,  12,  12,  13,  13,  14,
  14,  15,  15,  16,  16,  17,  17,  18,  18,  18,
  19,  19,  19,  19,  19,  20,  20,  20,  21,  21,
  21,  21,  22,  22,  23,  23,  23,  23,  23,  23,
  24,  24,  25,  25,  25,  25,  25,  26,  26,  27,
  27,  27,  28,  29,  30,   1,   2,   3,   4,   5,
   6,   7,   8,   9,  10 };
short magic_yyr2[]={

   0,   2,   1,   1,   2,   2,   6,   4,   3,   1,
   3,   1,   2,   7,   5,   0,   6,   0,   0,   6,
   4,   1,   1,   1,   1,   1,   1,   2,   1,   3,
   1,   3,   1,   3,   1,   2,   1,   3,   3,   1,
   3,   3,   3,   3,   1,   3,   3,   1,   3,   3,
   3,   1,   2,   1,   3,   4,   4,   4,   4,   1,
   4,   1,   4,   1,   1,   1,   1,   3,   1,   1,
   3,   3,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1 };
short magic_yychk[]={

-1000, -11, -13, 270, 269, 256, -12, -24, 259,  -1,
  -3,  -5, 272, 260, 293, 292, 278, 266, -27, 277,
 308, 297, 276, 267, 280, 270, -12, 270, 269,  -9,
 261, 271, 299, 282, -15, -16, -17, 288, -18, -19,
 -20, -21, -22, 284, -23,  -9, 301, 304, 300, 305,
 -25, -27, -28, -29, -30, 296, 302, 306, -15, -32,
  -9,  -7, 279, -15, -15, -14, -15, -26, -24, -14,
 303, 290, 257, -18, 271, 287, 283, 281, 275, 274,
 291, 284, 286, 264, 285, -23, -15,  -9,  -9,  -9,
  -9,  -7, 265, -13, -14, -15, 295, 295, -10, 262,
 298, 262, -13, -16, -17, -19, -19, -20, -20, -20,
 -20, -21, -21, -22, -22, -22, -10, -15, -15, -15,
 -15, -15, -31,  -6, 307, -10,  -8, 294, 261, -15,
 -24, 268,  -2, 273, -10, -10, -10, -10,  -8, -13,
 -33, -26, -13,  -4, 289, -15,  -2 };
short magic_yydef[]={

   0,  -2,   0,   2,   3,   0,  28,   9,  11,   0,
   0,  17,  21,  22,  23,  24,  25,  26,  61,  75,
  77,  79,  69,   0,   0,   1,  27,   4,   5,   0,
   0,   0,  12,  83,   0,  32,  34,   0,  36,  39,
  44,  47,  51,   0,  53,   0,   0,   0,   0,   0,
  59,  63,  64,  65,  66,  72,  73,  74,   0,   0,
   0,   0,  81,   0,   0,   0,  30,   8,  68,  10,
   0,   0,   0,  35,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  52,   0,   0,   0,   0,
   0,   0,  15,   0,   0,   0,  70,  71,   7,   0,
  84,   0,   0,  31,  33,  37,  38,  40,  41,  42,
  43,  45,  46,  48,  49,  50,  54,   0,   0,   0,
   0,   0,   0,  18,  80,  20,  60,  82,   0,  29,
  67,   0,  14,  76,  55,  56,  57,  58,  62,   0,
   0,   6,   0,  16,  78,  19,  13 };
/*
*/
#define YYFLAG    -1000
#define YYERROR   goto yyerrlab
#define YYACCEPT  return(0)
#define YYABORT   return(1)

/*******************************************************************************
*                                                                              *
*       magic_yyparse/0                                                              *
*                                                                              *
*       A restartable version of the yacc parser engine.  A bit slower than    *
*       the regular engine, but keeps state in static area so parsing may      *
*       be resumed if suspended.                                               *
*                                                                              *
*       magic_yyparse returns...                                                     *
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
int             magic_yydebug = 0;		/* set to 1 for debugging             */
#endif 
YYSTYPE         magic_yyv[YYMAXDEPTH];	/* where the values are stored        */
int             magic_yychar  = -1;		/* current input token number         */
int             magic_yynerrs = 0;		/* number of errors                   */
int             yyEntry = 0;		/* where suspended (0, 1 or 2)        */
short           magic_yyerrflag = 0;		/* error recovery flag                */
short           magic_yys[YYMAXDEPTH];	/* state stack                        */

#ifdef SUSP
static short    yySVstate,
	       *yySVps,	
		yySVn;
static short   *yySVxi;
static YYSTYPE *yySVpvt;
static YYSTYPE *yySVpv;
#endif 

int magic_yyparse()
{  short             yyj, yym;
   register short    magic_yystate,
	            *magic_yyps,
		     yyn;
   register short   *yyxi;
   register YYSTYPE *yypvt;
   register YYSTYPE *magic_yypv;

#ifdef SUSP
   if (yyEntry == 0)			/* suspended?                         */
   {
#endif 
      magic_yychar    = -1;
      magic_yynerrs   = 0;
      magic_yyerrflag = 0;
      magic_yyps      = &magic_yys[-1];
      magic_yypv      = &magic_yyv[-1];
      magic_yystate   = 0;			/* no:  initialize and go             */
#ifdef SUSP
   }
   else
   {  magic_yystate = yySVstate;		/* yes: restore state info and resume */
      magic_yyps    = yySVps;
      yyn     = yySVn;
      yyxi    = yySVxi;
      yypvt   = yySVpvt;
      magic_yypv    = yySVpv;
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
   if (magic_yydebug)
      printf("state %d, char 0%o\n",magic_yystate,magic_yychar);
#endif 
   if (&magic_yys[YYMAXDEPTH] < ++magic_yyps)
   {  magic_yyerror( "yacc stack overflow" );
      return(1);
   }
   *magic_yyps = magic_yystate;
   magic_yypv += 1;
   *magic_yypv = magic_yyval;

yynewstate:

   yyn = magic_yypact[magic_yystate];
   if (yyn <= YYFLAG )
      goto yydefault;			/* simple state */
   if (magic_yychar < 0)
yySusp1:
         if ((magic_yychar=magic_yylex()) < 0)	/* go to lexer... */
         {
#ifdef SUSP
            yyEntry   = 1;		/* suspended! note where and return */
	    yySVstate = magic_yystate;
	    yySVps    = magic_yyps;
	    yySVn     = yyn;
	    yySVxi    = yyxi;
	    yySVpvt   = yypvt;
	    yySVpv    = magic_yypv;
	    return(2);
#else 
            magic_yytext[0] = '\n';
            magic_yytext[1] = '\0';
            magic_yyerror( "\n" );
	    return(1);
#endif 
         }
   if ((yyn += magic_yychar)<0 || YYLAST <= yyn)
      goto yydefault;
   if (magic_yychk[yyn=magic_yyact[yyn]] == magic_yychar)	/* valid shift */
   {  magic_yychar  = -1;
      magic_yyval   = magic_yylval;
      magic_yystate = yyn;
      if (0 < magic_yyerrflag)
	--magic_yyerrflag;
      goto yystack;
   }

yydefault:				/* default state action */

   if ((yyn=magic_yydef[magic_yystate]) == -2)
   {  if (magic_yychar<0)
yySusp2:
         if ((magic_yychar=magic_yylex()) < 0)	/* go to lexer... */
         {
#ifdef SUSP
            yyEntry   = 2;		/* suspended! note where and return */
	    yySVstate = magic_yystate;
	    yySVps    = magic_yyps;
	    yySVn     = yyn;
	    yySVxi    = yyxi;
	    yySVpvt   = yypvt;
	    yySVpv    = magic_yypv;
	    return(2);
#else 
            magic_yytext[0] = '\n';
            magic_yytext[1] = '\0';
            magic_yyerror( "\n" );
	    return(1);
#endif 
         }

		/* look through exception table */

      for (yyxi=magic_yyexca; (*yyxi!= (-1)) || (yyxi[1]!=magic_yystate) ; yyxi += 2 )
	 ;
      while (0 <= *(yyxi+=2))
      {  if (*yyxi == magic_yychar )
	    break;
      }
      if ((yyn = yyxi[1]) < 0 )
	 return(0);			/* accept */
   }

   if (yyn == 0 )			/* error */
   {  switch (magic_yyerrflag)		/* ... attempt to resume parsing */
      { case 0:   magic_yyerror("syntax error");/* brand new error */
	yyerrlab: magic_yynerrs += 1;
	case 1:
	case 2:   magic_yyerrflag = 3;	/* incompletely recovered error ... try again */

			/* find a state where "error" is a legal shift action */

		  while (magic_yys <= magic_yyps)
		  {  yyn = magic_yypact[*magic_yyps] + YYERRCODE;
		     if (0 <= yyn && yyn < YYLAST && magic_yychk[magic_yyact[yyn]] == YYERRCODE)
		     {  magic_yystate = magic_yyact[yyn];  /* simulate a shift of "error" */
			goto yystack;
		     }
		     yyn = magic_yypact[*magic_yyps]; /* the current magic_yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
		     if (magic_yydebug )
			printf("error recovery pops state %d, uncovers %d\n",*magic_yyps,magic_yyps[-1]);
#endif 
		     magic_yyps -= 1;
		     magic_yypv -= 1;
		  }

			

        yyabort:  return(1);			/* no pushed state with 'error' ... abort */
	case 3:   /* no shift yet; clobber input char */

#ifdef YYDEBUG
		  if (magic_yydebug )
		     printf("error recovery discards char %d\n",magic_yychar);
#endif 
		  if (magic_yychar == 0 )
		     goto yyabort;		/* don't discard EOF, quit */
		  magic_yychar = -1;
		  goto yynewstate;		/* try again in the same state */

      }
   }

	/* reduction by production yyn */

#ifdef YYDEBUG
   if (magic_yydebug )
      printf("reduce %d\n",yyn);
#endif 
   magic_yyps -= magic_yyr2[yyn];
   yypvt = magic_yypv;
   magic_yypv -= magic_yyr2[yyn];
   magic_yyval = magic_yypv[1];
   yym   = yyn;
   yyn   = magic_yyr1[yyn];			/* consult goto table for next state */
   yyj   = magic_yypgo[yyn] + *magic_yyps + 1;
   if (YYLAST<=yyj || magic_yychk[magic_yystate=magic_yyact[yyj]] != -yyn )
      magic_yystate = magic_yyact[magic_yypgo[yyn]];
   switch (yym)				/* ready... ACTION! */
   {
			
case 1:
# line 72 "magical_gram.y"
{
                            PPRINT0("psr: start      : sList EOL\n");
                            magicCodeTree = yypvt[-1].nval;
                            YYACCEPT;
                         } break;
case 2:
# line 78 "magical_gram.y"
{
                            PPRINT0("psr: start      : EOL\n");
                            magicCodeTree = newNode(EOL,&(yypvt[-0].tval.location));
                            YYACCEPT;
                         } break;
case 3:
# line 84 "magical_gram.y"
{
                            PPRINT0("psr: start      : ENDFILE\n");
                            magicCodeTree = newNode(ENDFILE,&(yypvt[-0].tval.location));
                            YYACCEPT;
                         } break;
case 4:
# line 90 "magical_gram.y"
{
                            PPRINT0("psr: start      : error EOL\n");
                            magicCodeTree = NULL;
                            YYABORT;
                         } break;
case 5:
# line 96 "magical_gram.y"
{
                            PPRINT0("psr: start      : error ENDFILE\n");
                            magicCodeTree = NULL;
                            YYABORT;
                         } break;
case 6:
# line 104 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP CL lValList\n");
                            p = newNode(_Car,&(yypvt[-4].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-5].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 7:
# line 115 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal LP expList RP\n");
                            p = newNode(_Ca_,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 8:
# line 125 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal CL lValList\n");
                            p = newNode(_C_r,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 9:
# line 135 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal\n");
                            p = newNode(_C__,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 10:
# line 144 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal EQ expList\n");
                            p = newNode(EQ,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 11:
# line 154 "magical_gram.y"
{  magic_yyval.nval = newNode(BOMB,&(yypvt[-0].tval.location));
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 12:
# line 158 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : lVal SHOW \n");
                            p = newNode(SHOW,&(yypvt[-0].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 13:
# line 167 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList ELSE sList FI\n");
                            p = newNode(ELSE,&(yypvt[-6].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-5].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 14:
# line 178 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : IF exp THEN sList FI\n");
                            p = newNode(THEN,&(yypvt[-4].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 15:
# line 188 "magical_gram.y"
{  loopLevel += 1;
                         } break;
case 16:
# line 191 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : WHILE exp DO sList OD\n");
                            loopLevel -= 1;
                            p          = newNode(WHILE,&(yypvt[-5].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-4].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 17:
# line 202 "magical_gram.y"
{  loopLevel += 1;
                         } break;
case 18:
# line 205 "magical_gram.y"
{  loopLevel -= 1;
                         } break;
case 19:
# line 208 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt      : REPEAT sList UNTIL exp\n");
                            p = newNode(REPEAT,&(yypvt[-5].tval.location));
                            addLeftSon(p,yypvt[-3].nval);
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 20:
# line 218 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: stmnt       : EXIT LP expList RP\n");
                            p = newNode(EXIT,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 21:
# line 227 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : EXIT\n");
                            magic_yyval.nval = newNode(EXIT,&(yypvt[-0].tval.location));
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 22:
# line 233 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : BREAK\n");
                            if (loopLevel)
                                magic_yyval.nval = newNode(BREAK,&(yypvt[-0].tval.location));
                            else
                            {   WerrprintfWithPos("Syntax error! BREAK outside loop");
                                magic_yyval.nval = NULL;
                            }
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 23:
# line 244 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : PUSH\n");
                            envLevel += 1;
                            magic_yyval.nval        = newNode(PUSH,&(yypvt[-0].tval.location));
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 24:
# line 251 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : POP\n");
                            if (envLevel)
                            {   envLevel -= 1;
                                magic_yyval.nval        = newNode(POP,&(yypvt[-0].tval.location));
                            }
                            else
                            {   fprintf(stderr,"magic: POP without PUSH\n");
                                magic_yyval.nval = NULL;
                            }
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 25:
# line 264 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : IGNORE\n");
                            magic_yyval.nval = newNode(IGNORE,&(yypvt[-0].tval.location));
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 26:
# line 270 "magical_gram.y"
{
                            PPRINT0("psr: stmnt      : DONT\n");
                            magic_yyval.nval = newNode(DONT,&(yypvt[-0].tval.location));
                            PSHOWTREE(magic_yyval.nval);
                         } break;
case 27:
# line 278 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: sList      : sList stmnt\n");
                            if (yypvt[-1].nval)
                                if (yypvt[-0].nval)
                                {   p = newNode(CM,&(yypvt[-0].nval->location));
                                    addLeftSon(p,yypvt[-0].nval);
                                    addLeftSon(p,yypvt[-1].nval);
                                    magic_yyval.nval = p;
                                }
                                else
                                    magic_yyval.nval = yypvt[-1].nval;
                            else
                                magic_yyval.nval = yypvt[-0].nval;
                            PSHOWTREE(p);
                         } break;
case 28:
# line 295 "magical_gram.y"
{
                            PPRINT0("psr: sList      : stmnt\n");
                            magic_yyval.nval = yypvt[-0].nval;
                         } break;
case 29:
# line 302 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: expList    : expList CM exp\n");
                            p = newNode(CM,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 30:
# line 312 "magical_gram.y"
{
                            PPRINT0("psr: expList    : exp\n");
                            magic_yyval.nval = yypvt[-0].nval;
                         } break;
case 31:
# line 319 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: exp        : exp OR lTerm\n");
                            p = newNode(OR,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 32:
# line 329 "magical_gram.y"
{
                            PPRINT0("psr: exp        : lTerm\n");
                            magic_yyval.nval = yypvt[-0].nval;
                         } break;
case 33:
# line 336 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lTerm      : lTerm AND lFact\n");
                            p = newNode(AND,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 34:
# line 346 "magical_gram.y"
{
                            PPRINT0("psr: lTerm      : lFact\n");
                            magic_yyval.nval = yypvt[-0].nval;
                         } break;
case 35:
# line 353 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lFact      : NOT lAtom\n");
                            p = newNode(NOT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 36:
# line 362 "magical_gram.y"
{
                            PPRINT0("psr: lFact      : lAtom\n");
                         } break;
case 37:
# line 368 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lAtom      : lAtom EQ order\n");
                            p = newNode(EQ,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 38:
# line 378 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lAtom      : lAtom NE order\n");
                            p = newNode(NE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 39:
# line 388 "magical_gram.y"
{
                            PPRINT0("psr: lAtom      : order\n");
                         } break;
case 40:
# line 394 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: order      : order LT arith\n");
                            p = newNode(LT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 41:
# line 404 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: order      : order LE arith\n");
                            p = newNode(LE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 42:
# line 414 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: order      : order GT arith\n");
                            p = newNode(GT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 43:
# line 424 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: order      : order GE arith\n");
                            p = newNode(GE,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 44:
# line 434 "magical_gram.y"
{
                            PPRINT0("psr: order       : arith\n");
                         } break;
case 45:
# line 440 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: arith      : arith PLUS term\n");
                            p = newNode(PLUS,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 46:
# line 450 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: arith      : arith MINUS term\n");
                            p = newNode(MINUS,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 47:
# line 460 "magical_gram.y"
{
                            PPRINT0("psr: arith      : term\n");
                         } break;
case 48:
# line 466 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: term       : term MULT factor\n");
                            p = newNode(MULT,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 49:
# line 476 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: term       : term DIV factor\n");
                            p = newNode(DIV,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 50:
# line 486 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: term       : term MOD factor\n");
                            p = newNode(MOD,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 51:
# line 496 "magical_gram.y"
{
                            PPRINT0("psr: term       : factor\n");
                         } break;
case 52:
# line 502 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: factor     : MINUS atom\n");
                            p = newNode(_NEG,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 53:
# line 511 "magical_gram.y"
{
                            PPRINT0("psr: factor     : atom\n");
                         } break;
case 54:
# line 517 "magical_gram.y"
{
                            PPRINT0("psr: atom       : LP exp RP\n");
                            magic_yyval.nval = yypvt[-1].nval;
                         } break;
case 55:
# line 522 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: atom       : SQRT LP exp RP\n");
                            p = newNode(SQRT,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 56:
# line 531 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: atom       : TRUNC LP exp RP\n");
                            p = newNode(TRUNC,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 57:
# line 540 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: atom       : SIZE LP exp RP\n");
                            p = newNode(SIZE,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 58:
# line 549 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: atom       : TYPEOF LP exp RP\n");
                            p = newNode(TYPEOF,&(yypvt[-3].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 59:
# line 558 "magical_gram.y"
{
                            PPRINT0("psr: atom       : rVal\n");
                            magic_yyval.nval = yypvt[-0].nval;
                         } break;
case 60:
# line 565 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lVal       : name LB exp RB\n");
                            p = newNode(LB,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 61:
# line 575 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lVal       : name\n");
                            p = newNode(ID,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 62:
# line 586 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: rVal       : name LB exp RB\n");
                            p = newNode(LB,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            addLeftSon(p,yypvt[-3].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 63:
# line 596 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: rVal       : name\n");
                            p = newNode(ID,&(yypvt[-0].nval->location));
                            addLeftSon(p,yypvt[-0].nval);
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 64:
# line 605 "magical_gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : REAL\n");
                            p      = newNode(REAL,&(yypvt[-0].dval.location));
                            p->v.r = yypvt[-0].dval.value;
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 65:
# line 614 "magical_gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : string\n");
                            p      = newNode(STRING,&(yypvt[-0].cval.location));
                            p->v.s = yypvt[-0].cval.value;
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 66:
# line 623 "magical_gram.y"
{  node *p;
                        
                            PPRINT0("psr: rVal       : unit\n");
                            p      = newNode(UNIT,&(yypvt[-0].cval.location));
                            p->v.s = yypvt[-0].cval.value;
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 67:
# line 634 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: lValList   : lValList CM lVal\n");
                            p = newNode(CM,&(yypvt[-1].tval.location));
                            addLeftSon(p,yypvt[-0].nval);
                            addLeftSon(p,yypvt[-2].nval);
                            magic_yyval.nval = p;
                            PSHOWTREE(p);
                         } break;
case 68:
# line 644 "magical_gram.y"
{
                            PPRINT0("psr: lValList   : lVal\n");
                         } break;
case 69:
# line 650 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: name       : ID\n");
                            p      = newNode(_NAME,&(yypvt[-0].tval.location));
                            p->v.s = newStringId(magic_yytext,tempID);
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 70:
# line 659 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: name       : DOLLAR exp RC\n");
                            p = newNode(DOLLAR,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 71:
# line 668 "magical_gram.y"
{  node *p;

                            PPRINT0("psr: name       : LC exp RC\n");
                            p = newNode(LC,&(yypvt[-2].tval.location));
                            addLeftSon(p,yypvt[-1].nval);
                            magic_yyval.nval     = p;
                            PSHOWTREE(p);
                         } break;
case 72:
# line 679 "magical_gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: real        : REAL\n");
                            tmp.dval.value    = atof(magic_yytext);
                            tmp.dval.location = yypvt[-0].tval.location;
                            magic_yyval.dval                = tmp.dval;
                         } break;
case 73:
# line 689 "magical_gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: string     : STRING\n");
                            magic_yytext[strlen(magic_yytext)-1] = '\0';
                            tmp.cval.value           = newStringId(&(magic_yytext[1]),tempID);
                            tmp.cval.location        = yypvt[-0].tval.location;
                            magic_yyval.cval                       = tmp.cval;
                         } break;
case 74:
# line 700 "magical_gram.y"
{  YYSTYPE tmp;

                            PPRINT0("psr: unit     : UNIT\n");
                            magic_yytext[strlen(magic_yytext)] = '\0';
                            tmp.cval.value           = newStringId(&(magic_yytext[0]),tempID);
                            tmp.cval.location        = yypvt[-0].tval.location;
                            magic_yyval.cval                       = tmp.cval;
                         } break;
case 75:
# line 711 "magical_gram.y"
{  ignoreEOL += 1;
                         } break;
case 76:
# line 716 "magical_gram.y"
{  ignoreEOL -= 1;
                         } break;
case 77:
# line 721 "magical_gram.y"
{  ignoreEOL += 1;
                         } break;
case 78:
# line 726 "magical_gram.y"
{  ignoreEOL -= 1;
                         } break;
case 79:
# line 731 "magical_gram.y"
{  ignoreEOL += 1;
                         } break;
case 80:
# line 736 "magical_gram.y"
{  ignoreEOL -= 1;
                         } break;
case 81:
# line 741 "magical_gram.y"
{  ignoreEOL += 1;
                         } break;
case 82:
# line 746 "magical_gram.y"
{  ignoreEOL -= 1;
                         } break;
case 83:
# line 751 "magical_gram.y"
{  ignoreEOL += 1;
                         } break;
case 84:
# line 756 "magical_gram.y"
{  ignoreEOL -= 1;
                         } break;
   }
   goto yystack;  /* stack new state and value */
}
