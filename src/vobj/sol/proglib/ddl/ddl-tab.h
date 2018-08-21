/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
  double val;
  char *str;
  DDLNode *sym;
} YYSTYPE;
extern YYSTYPE yylval;
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
