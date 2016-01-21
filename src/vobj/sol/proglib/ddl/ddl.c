/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

# line 3 "ddl.y"
#ifdef LINUX
#include "generic.h"
#else
#include <generic.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "parser.h"
extern void free(void *);

# line 15 "ddl.y"
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

#include <values.h>

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef yyerror
#if defined(__cplusplus)
	void yyerror(const char *);
#endif
#endif
#ifndef yylex
	int yylex(void);
#endif
	int yyparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 76 "ddl.y"

  
	 
static const yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 7,
	259, 14,
	61, 16,
	-2, 20,
	};
# define YYNPROD 30
# define YYLAST 240
static const yytabelem yyact[]={

    11,    47,    10,    40,    11,    25,    10,    27,    26,    39,
    18,    28,    35,    24,    49,    11,     2,    10,    22,    42,
     2,    34,    21,    20,    23,    19,    12,     5,    29,     1,
    33,    32,     8,     3,     4,    30,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    31,     0,     0,     0,
    36,     0,     0,     0,     0,     0,     0,     0,    37,     0,
     0,     0,    41,     0,     0,    45,     0,    44,     8,     3,
    46,     0,    48,     0,     0,     0,    50,     0,     0,     0,
    18,     0,    43,     0,    18,     0,     0,     0,     0,     0,
    38,     0,     0,     0,     0,    18,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     6,     9,    14,     7,     6,     9,    14,
     7,    13,    15,    16,    17,    13,    15,    16,    17,     9,
    14,     7,     0,     0,     0,     0,    13,    15,    16,    17 };
static const yytabelem yypact[]={

-10000000,   -39,-10000000,   -34,   -36,   -37,   -41,   -78,  -254,-10000000,
  -249,  -250,-10000000,  -112,-10000000,-10000000,-10000000,   -14,   -28,-10000000,
-10000000,-10000000,-10000000,   -40,   -81,   -78,-10000000,-10000000,-10000000,-10000000,
   -35,-10000000,-10000000,  -256,   -28,-10000000,   -42,   -43,-10000000,   -28,
   -78,-10000000,   -28,  -258,-10000000,   -42,-10000000,   -78,   -47,  -113,
-10000000 };
static const yytabelem yypgo[]={

     0,    27,    31,    26,    35,    29,    34,    24,    30 };
static const yytabelem yyr1[]={

     0,     5,     5,     5,     5,     5,     5,     2,     2,     6,
     8,     8,     8,     8,     8,     7,     7,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     3,     4,     4,     4 };
static const yytabelem yyr2[]={

     0,     1,     5,     7,     7,     7,     7,     9,    11,     7,
     2,     2,     2,     4,     2,     4,     0,     3,     5,     5,
     3,     3,     3,    13,    17,     3,     7,     1,     3,     7 };
static const yytabelem yychk[]={

-10000000,    -5,    59,    -2,    -6,    -1,   256,   259,    -8,   257,
    45,    43,    -3,   264,   258,   265,   266,   267,   123,    59,
    59,    59,    59,    -7,    91,   259,   257,   257,   123,    42,
    -4,    -1,    -2,    -8,    61,    93,    -7,    -5,   125,    44,
   259,    -1,    61,   125,    -1,    -7,    -1,   259,    -7,    61,
    -3 };
static const yytabelem yydef[]={

     1,    -2,     2,     0,     0,     0,     0,    -2,     0,    17,
     0,     0,    22,     0,    25,    10,    11,    12,    27,     3,
     4,     5,     6,     0,     0,    16,    18,    19,     1,    13,
     0,    28,    21,     0,     0,    15,     9,     0,    26,     0,
    16,     7,     0,     0,    29,     0,     8,    16,    23,     0,
    24 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
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

char * yyreds[] =
{
	"-no such reduction-",
	"statement : /* empty */",
	"statement : statement ';'",
	"statement : statement asgn ';'",
	"statement : statement decl ';'",
	"statement : statement expr ';'",
	"statement : statement error ';'",
	"asgn : VAR arrayspec '=' expr",
	"asgn : typename VAR arrayspec '=' expr",
	"decl : typename VAR arrayspec",
	"typename : DDL_FLOAT",
	"typename : DDL_INT",
	"typename : DDL_CHAR",
	"typename : DDL_CHAR '*'",
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
	"expr : DDL_STRING",
	"array : '{' elements '}'",
	"elements : /* empty */",
	"elements : expr",
	"elements : elements ',' expr",
};
#endif /* YYDEBUG */
# line	1 "/usr/ccs/bin/yaccpar"
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)yaccpar	6.15	97/12/08 SMI"

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
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
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
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
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
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
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
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
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
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

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
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long yyps_index = (yy_ps - yys);
			long yypv_index = (yy_pv - yyv);
			long yypvt_index = (yypvt - yyv);
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
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register const int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
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
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
			skip_init:
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
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
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
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
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 1:
# line 38 "ddl.y"
{ yyval.sym = 0; } break;
case 2:
# line 39 "ddl.y"
{ yyval.sym = yypvt[-1].sym;} break;
case 3:
# line 40 "ddl.y"
{ yyval.sym = yypvt[-2].sym;} break;
case 4:
# line 41 "ddl.y"
{ yyval.sym = yypvt[-2].sym;} break;
case 5:
# line 42 "ddl.y"
{ yyval.sym = ExprStatement(yypvt[-2].sym, yypvt[-1].sym);} break;
case 6:
# line 43 "ddl.y"
{ yyerrok; yyval.sym = yypvt[-2].sym;} break;
case 7:
# line 46 "ddl.y"
{yyval.sym = TypelessAsgn(yypvt[-3].sym, yypvt[-0].sym);} break;
case 8:
# line 47 "ddl.y"
{yyval.sym = TypedAsgn(yypvt[-3].sym, yypvt[-0].sym);} break;
case 9:
# line 50 "ddl.y"
{ yyval.sym = yypvt[-1].sym; } break;
case 17:
# line 57 "ddl.y"
{yyval.sym = RealExpr(yypvt[-0].val);} break;
case 18:
# line 58 "ddl.y"
{yyval.sym = RealExpr(-yypvt[-0].val);} break;
case 19:
# line 59 "ddl.y"
{yyval.sym = RealExpr(yypvt[-0].val);} break;
case 20:
# line 60 "ddl.y"
{yyval.sym = yypvt[-0].sym;} break;
case 21:
# line 61 "ddl.y"
{yyval.sym = yypvt[-0].sym;} break;
case 22:
# line 62 "ddl.y"
{yyval.sym = yypvt[-0].sym;} break;
case 23:
# line 63 "ddl.y"
{yyval.sym = StructExpr();} break;
case 24:
# line 64 "ddl.y"
{yyval.sym = StructExprInit();} break;
case 25:
# line 65 "ddl.y"
{yyval.sym = StringExpr(yypvt[-0].str);} break;
case 26:
# line 68 "ddl.y"
{yyval.sym = ArrayStatement(yypvt[-1].sym);} break;
case 27:
# line 71 "ddl.y"
{yyval.sym = ElementsDone();} break;
case 28:
# line 72 "ddl.y"
{yyval.sym = ExprElements(yypvt[-0].sym);} break;
case 29:
# line 73 "ddl.y"
{yyval.sym = AppendElement(yypvt[-2].sym, yypvt[-0].sym);} break;
# line	531 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}

