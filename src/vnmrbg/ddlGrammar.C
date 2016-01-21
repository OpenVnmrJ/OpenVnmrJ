/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

# line 10 "ddlGrammar.y"
    /*
#include <generic.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
    */
#include "ddlParser.h"
    /*extern void free(void *);*/

# line 20 "ddlGrammar.y"
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
  double val;
  char *str;
  DDLNode *sym;
} YYSTYPE;
# define DDL_REAL 257
# define DDL_STRING 258
# define VAR 259
# define BUILTIN 260
# define UNDEFINED 261
# define CODE 262
# define KEYWORD 263
# define STRUCT 264
# define DDL_FLOAT 265
# define DDL_INT 266
# define DDL_CHAR 267
# define VOID 268
# define TYPEDEF 269
# define OR 270
# define AND 271
# define GT 272
# define GE 273
# define LT 274
# define LE 275
# define EQ 276
# define NE 277
# define UNARYMINUS 278
# define UNARYPLUS 279
# define NOT 280

#include <inttypes.h>

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#else
#include <malloc.h>
#include <memory.h>
#endif


#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef ddl_yyerror
#if defined(__cplusplus)
	void ddl_yyerror(const char *);
#endif
#endif
#ifndef ddl_yylex
	int ddl_yylex(void);
#endif
	int ddl_yyparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define yyclearin ddl_yychar = -1
#define yyerrok ddl_yyerrflag = 0
extern int ddl_yychar;
extern int ddl_yyerrflag;
YYSTYPE ddl_yylval;
YYSTYPE ddl_yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *ddl_yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *ddl_yyv = yy_yyv;
#else	/* user does initial allocation */
int *ddl_yys;
YYSTYPE *ddl_yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 96 "ddlGrammar.y"

  
	 
static const yytabelem ddl_yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 9,
	259, 14,
	61, 16,
	-2, 20,
	};
# define YYNPROD 35
# define YYLAST 239
static const yytabelem ddl_yyact[]={

    13,    55,    12,    51,    13,    42,    12,    35,    27,    29,
    28,    61,    41,    18,    30,    13,     2,    12,    37,    26,
     2,    52,    25,    59,    45,    49,    36,    14,    23,    22,
    21,    24,    20,    19,     5,    34,    33,    10,     4,     1,
    31,    57,    53,     3,     6,     0,     0,     0,     0,     0,
    38,     0,     0,     0,     0,     0,     0,     0,    43,     0,
     0,     0,     0,    32,     0,    48,     0,     0,     0,     0,
    39,     0,     0,     0,    56,    10,     4,    58,     0,     0,
    18,    44,    46,    63,    18,     0,    47,    62,     0,     0,
    50,     0,    60,    40,     0,    18,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    54,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     7,    11,     0,     9,     7,    11,     0,
     9,    15,    16,    17,     8,    15,    16,    17,     8,    11,
     0,     9,     0,     0,     0,     0,    15,    16,    17 };
static const yytabelem ddl_yypact[]={

-10000000,   -39,-10000000,   -26,   -27,   -29,   -30,   -31,   -11,   -72,
  -251,-10000000,  -247,  -248,-10000000,  -109,-10000000,-10000000,   -28,-10000000,
-10000000,-10000000,-10000000,-10000000,  -252,   -35,   -75,   -72,-10000000,-10000000,
-10000000,   -32,-10000000,-10000000,  -254,   -72,   -28,-10000000,   -37,   -43,
-10000000,   -28,   -72,   -36,-10000000,   -28,  -256,-10000000,   -37,  -122,
-10000000,   -72,-10000000,-10000000,-10000000,  -122,   -38,   -33,-10000000,  -110,
-10000000,  -122,-10000000,-10000000 };
static const yytabelem ddl_yypgo[]={

     0,    21,    44,    43,    36,    42,    27,    41,    40,    39,
    34,    22,    35 };
static const yytabelem ddl_yyr1[]={

     0,     9,     9,     9,     9,     9,     9,     9,     3,     4,
     4,    10,    12,    12,    12,    11,    11,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     1,     6,     5,     8,
     8,     8,     7,     7,     7 };
static const yytabelem ddl_yyr2[]={

     0,     1,     5,     7,     7,     7,     7,     7,    13,     9,
    11,     7,     2,     2,     2,     4,     0,     3,     5,     5,
     3,     3,     3,    13,    17,     3,     3,     7,     7,     1,
     3,     7,     1,     3,     7 };
static const yytabelem ddl_yychk[]={

-10000000,    -9,    59,    -3,    -4,   -10,    -2,   256,   267,   259,
   -12,   257,    45,    43,    -6,   264,   265,   266,   123,    59,
    59,    59,    59,    59,    42,   -11,    91,   259,   257,   257,
   123,    -8,    -2,    -4,   -12,   259,    61,    93,   -11,    -9,
   125,    44,   259,   -11,    -2,    61,   125,    -2,   -11,    61,
    -2,   259,    -1,    -5,   258,   123,   -11,    -7,    -1,    61,
   125,    44,    -6,    -1 };
static const yytabelem ddl_yydef[]={

     1,    -2,     2,     0,     0,     0,     0,     0,     0,    -2,
     0,    17,     0,     0,    22,     0,    12,    13,    29,     3,
     4,     5,     6,     7,     0,     0,     0,    16,    18,    19,
     1,     0,    30,    21,     0,    16,     0,    15,    11,     0,
    27,     0,    16,     0,     9,     0,     0,    31,     0,     0,
    10,    16,     8,    25,    26,    32,    23,     0,    33,     0,
    28,     0,    24,    34 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype ddl_yytoks[] =
{
	"DDL_REAL",	257,
	"DDL_STRING",	258,
	"VAR",	259,
	"BUILTIN",	260,
	"UNDEFINED",	261,
	"CODE",	262,
	"KEYWORD",	263,
	"STRUCT",	264,
	"DDL_FLOAT",	265,
	"DDL_INT",	266,
	"DDL_CHAR",	267,
	"VOID",	268,
	"TYPEDEF",	269,
	"=",	61,
	"OR",	270,
	"AND",	271,
	"GT",	272,
	"GE",	273,
	"LT",	274,
	"LE",	275,
	"EQ",	276,
	"NE",	277,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"UNARYMINUS",	278,
	"UNARYPLUS",	279,
	"NOT",	280,
	"^",	94,
	"-unknown-",	-1	/* ends search */
};

char * ddl_yyreds[] =
{
	"-no such reduction-",
	"statement : /* empty */",
	"statement : statement ';'",
	"statement : statement sasgn ';'",
	"statement : statement asgn ';'",
	"statement : statement decl ';'",
	"statement : statement expr ';'",
	"statement : statement error ';'",
	"sasgn : DDL_CHAR '*' VAR arrayspec '=' sexpr",
	"asgn : VAR arrayspec '=' expr",
	"asgn : typename VAR arrayspec '=' expr",
	"decl : typename VAR arrayspec",
	"typename : DDL_FLOAT",
	"typename : DDL_INT",
	"typename : VAR",
	"arrayspec : '[' ']'",
	"arrayspec : /* empty */",
	"expr : DDL_REAL",
	"expr : '-' DDL_REAL",
	"expr : '+' DDL_REAL",
	"expr : VAR",
	"expr : asgn",
	"expr : array",
	"expr : STRUCT '{' statement '}' VAR arrayspec",
	"expr : STRUCT '{' statement '}' VAR arrayspec '=' array",
	"sexpr : sarray",
	"sexpr : DDL_STRING",
	"array : '{' elements '}'",
	"sarray : '{' selements '}'",
	"elements : /* empty */",
	"elements : expr",
	"elements : elements ',' expr",
	"selements : /* empty */",
	"selements : sexpr",
	"selements : selements ',' sexpr",
};
#endif /* YYDEBUG */
# line	1 "/usr/ccs/bin/yaccpar"
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( ddl_yychar >= 0 || ( ddl_yyr2[ ddl_yytmp ] >> 1 ) != 1 )\
	{\
		ddl_yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	ddl_yychar = newtoken;\
	ddl_yystate = *ddl_yyps;\
	ddl_yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!ddl_yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yymaxdepth * sizeof (type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int ddl_yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *ddl_yypv;			/* top of value stack */
int *ddl_yyps;			/* top of state stack */

int ddl_yystate;			/* current state */
int ddl_yytmp;			/* extra var (lasts between blocks) */

int ddl_yynerrs;			/* number of errors */
int ddl_yyerrflag;			/* error recovery flag */
int ddl_yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(ddl_yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		ddl_yylex()
#endif/*!YYNMBCHARS*/

/*
** ddl_yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int ddl_yyparse(void)
#else
int ddl_yyparse()
#endif
{
	register YYSTYPE *yypvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside
	switch should never be executed
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto yyerrlab;
		case 2: goto yynewstate;
	}
#endif

	/*
	** Initialize externals - ddl_yyparse may be called more than once
	*/
	ddl_yypv = &ddl_yyv[-1];
	ddl_yyps = &ddl_yys[-1];
	ddl_yystate = 0;
	ddl_yytmp = 0;
	ddl_yynerrs = 0;
	ddl_yyerrflag = 0;
	ddl_yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			ddl_yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
	goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = ddl_yypv;
		yy_ps = ddl_yyps;
		yy_state = ddl_yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = ddl_yypv;
		yy_ps = ddl_yyps;
		yy_state = ddl_yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( ddl_yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( ddl_yychar == 0 )
				printf( "end-of-file\n" );
			else if ( ddl_yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; ddl_yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( ddl_yytoks[yy_i].t_val == ddl_yychar )
						break;
				}
				printf( "%s\n", ddl_yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &ddl_yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long yyps_index = (yy_ps - ddl_yys);
			long yypv_index = (yy_pv - ddl_yyv);
			long yypvt_index = (yypvt - ddl_yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					ddl_yys = YYCOPY(newyys, ddl_yys, int);
					ddl_yyv = YYCOPY(newyyv, ddl_yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				ddl_yys = YYENLARGE(ddl_yys, int);
				ddl_yyv = YYENLARGE(ddl_yyv, YYSTYPE);
				if (ddl_yys == 0 || ddl_yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				ddl_yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = ddl_yys + yyps_index;
			yy_pv = ddl_yyv + yypv_index;
			yypvt = ddl_yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = ddl_yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = ddl_yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		ddl_yytmp = ddl_yychar < 0;
#endif
		if ( ( ddl_yychar < 0 ) && ( ( ddl_yychar = YYLEX() ) < 0 ) )
			ddl_yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( ddl_yydebug && ddl_yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( ddl_yychar == 0 )
				printf( "end-of-file\n" );
			else if ( ddl_yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; ddl_yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( ddl_yytoks[yy_i].t_val == ddl_yychar )
						break;
				}
				printf( "%s\n", ddl_yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += ddl_yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( ddl_yychk[ yy_n = ddl_yyact[ yy_n ] ] == ddl_yychar )	/*valid shift*/
		{
			ddl_yychar = -1;
			ddl_yyval = ddl_yylval;
			yy_state = yy_n;
			if ( ddl_yyerrflag > 0 )
				ddl_yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = ddl_yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			ddl_yytmp = ddl_yychar < 0;
#endif
			if ( ( ddl_yychar < 0 ) && ( ( ddl_yychar = YYLEX() ) < 0 ) )
				ddl_yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( ddl_yydebug && ddl_yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( ddl_yychar == 0 )
					printf( "end-of-file\n" );
				else if ( ddl_yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						ddl_yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( ddl_yytoks[yy_i].t_val
							== ddl_yychar )
						{
							break;
						}
					}
					printf( "%s\n", ddl_yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register const int *yyxi = ddl_yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != ddl_yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( ddl_yyerrflag )
			{
			case 0:		/* new error */
				ddl_yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = ddl_yypv;
				yy_ps = ddl_yyps;
				yy_state = ddl_yystate;
			skip_init:
				ddl_yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				ddl_yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= ddl_yys )
				{
					yy_n = ddl_yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						ddl_yychk[ddl_yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = ddl_yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( ddl_yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( ddl_yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( ddl_yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( ddl_yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							ddl_yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( ddl_yytoks[yy_i].t_val
								== ddl_yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							ddl_yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( ddl_yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				ddl_yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( ddl_yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, ddl_yyreds[ yy_n ] );
#endif
		ddl_yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If ddl_yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = ddl_yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				ddl_yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = ddl_yypgo[ yy_n = ddl_yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					ddl_yychk[ yy_state =
					ddl_yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = ddl_yyact[ ddl_yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			ddl_yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = ddl_yypgo[ yy_n = ddl_yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				ddl_yychk[ yy_state = ddl_yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = ddl_yyact[ ddl_yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		ddl_yystate = yy_state;
		ddl_yyps = yy_ps;
		ddl_yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( ddl_yytmp )
	{
		
case 1:
# line 43 "ddlGrammar.y"
{ ddl_yyval.sym = 0; } break;
case 2:
# line 44 "ddlGrammar.y"
{ ddl_yyval.sym = yypvt[-1].sym;} break;
case 3:
# line 45 "ddlGrammar.y"
{ ddl_yyval.sym = yypvt[-2].sym;} break;
case 4:
# line 46 "ddlGrammar.y"
{ ddl_yyval.sym = yypvt[-2].sym;} break;
case 5:
# line 47 "ddlGrammar.y"
{ ddl_yyval.sym = yypvt[-2].sym;} break;
case 6:
# line 48 "ddlGrammar.y"
{ ddl_yyval.sym = ExprStatement(yypvt[-2].sym, yypvt[-1].sym);} break;
case 7:
# line 49 "ddlGrammar.y"
{ yyerrok; ddl_yyval.sym = yypvt[-2].sym;} break;
case 8:
# line 52 "ddlGrammar.y"
{ddl_yyval.sym = TypedAsgn(yypvt[-3].sym, yypvt[-0].sym);} break;
case 9:
# line 55 "ddlGrammar.y"
{ddl_yyval.sym = TypelessAsgn(yypvt[-3].sym, yypvt[-0].sym);} break;
case 10:
# line 56 "ddlGrammar.y"
{ddl_yyval.sym = TypedAsgn(yypvt[-3].sym, yypvt[-0].sym);} break;
case 11:
# line 59 "ddlGrammar.y"
{ ddl_yyval.sym = yypvt[-1].sym; } break;
case 17:
# line 66 "ddlGrammar.y"
{ddl_yyval.sym = RealExpr(yypvt[-0].val);} break;
case 18:
# line 67 "ddlGrammar.y"
{ddl_yyval.sym = RealExpr(-yypvt[-0].val);} break;
case 19:
# line 68 "ddlGrammar.y"
{ddl_yyval.sym = RealExpr(yypvt[-0].val);} break;
case 20:
# line 69 "ddlGrammar.y"
{ddl_yyval.sym = yypvt[-0].sym;} break;
case 21:
# line 70 "ddlGrammar.y"
{ddl_yyval.sym = yypvt[-0].sym;} break;
case 22:
# line 71 "ddlGrammar.y"
{ddl_yyval.sym = yypvt[-0].sym;} break;
case 23:
# line 72 "ddlGrammar.y"
{ddl_yyval.sym = StructExpr();} break;
case 24:
# line 73 "ddlGrammar.y"
{ddl_yyval.sym = StructExprInit();} break;
case 25:
# line 76 "ddlGrammar.y"
{ddl_yyval.sym = yypvt[-0].sym;} break;
case 26:
# line 77 "ddlGrammar.y"
{ddl_yyval.sym = StringExpr(yypvt[-0].str);} break;
case 27:
# line 80 "ddlGrammar.y"
{ddl_yyval.sym = ArrayStatement(yypvt[-1].sym);} break;
case 28:
# line 83 "ddlGrammar.y"
{ddl_yyval.sym = ArrayStatement(yypvt[-1].sym);} break;
case 29:
# line 86 "ddlGrammar.y"
{ddl_yyval.sym = ElementsDone();} break;
case 30:
# line 87 "ddlGrammar.y"
{ddl_yyval.sym = ExprElements(yypvt[-0].sym);} break;
case 31:
# line 88 "ddlGrammar.y"
{ddl_yyval.sym = AppendElement(yypvt[-2].sym, yypvt[-0].sym);} break;
case 32:
# line 91 "ddlGrammar.y"
{ddl_yyval.sym = ElementsDone();} break;
case 33:
# line 92 "ddlGrammar.y"
{ddl_yyval.sym = ExprElements(yypvt[-0].sym);} break;
case 34:
# line 93 "ddlGrammar.y"
{ddl_yyval.sym = AppendElement(yypvt[-2].sym, yypvt[-0].sym);} break;
# line	531 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}

