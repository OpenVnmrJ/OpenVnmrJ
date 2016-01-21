/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* linedef.h 6.2 10/7/85 */
/* linedef.h 6.1 8/19/85 */
#define ESC 27			/*  Esc character	*/
/*	#define	LINE1	"=  "	*/		/* Program ID message */
#define	LINE2	"=! "		/* Copyright Message  and */
#define	LINE2b	"=!V"		/* memory size in blocks Message */

#define	LINE3	"=\" "		/*    BLANK    */

#define	LINE4	"=# "		/* addresses of lc,acode,data table*/
#define	LINE5	"=$ "		/* addresses of automation structure */
				/* when_mask, sample numbers */

#define	LINE6	"=% "		/*    BLANK    */

#define	LINE7	"=& "		/* STATUS, CT, NT, BSCT, SSCT */

#define	LINE8	"=' "		/*     BLANK    */

#define	LINE9	"=( "		/* FIFO status bit, Output Status register */
#define	LINE9b	"=(8"		/* Input Card Mode (oph,scale) ICMODE      */
				/* Sum-to-Memory Check Word (STMCHK)       */
				/* OVFLSET				   */

#define	LINE10	"=) "		/* Noisechk data */

#define	LINE11	"=* "		/* AUTO or LOAD SHIM */
#define	LINE11b	"=*-"		/* AUTO or LOAD SHIM */

#define	LINE12	"=+ "		/* SPINNER COMMANDS  */
#define LINE12b "=+L"		/* SPINNER ERROR */

#define	LINE13	"=, "		/* SPINSTAT  COMMANDS  */
#define LINE13b "=,5"		/* SPINSTAT ERROR */

#define	LINE14	"=- "		/* SAMPLE CHANGER COMMANDS  */
#define LINE14b "=-A"		/* SAMPLE CHANGER STATUS  */
#define LINE14c "=-V"		/* SAMPLE CHANGER ERROR #  */

#define	LINE15 	"=. "		/* DISK COMMAND & FILE ADDRESS */
#define	LINE15b	"=.`"		/* DISK STATUS */

#define	LINE16	"=/ "		/* DISK ERRORS */

#define	LINE17	"=0 "		/* STM ERRORS */

#define	LINE18	"=1 "		/* BAD OPCODE */

#define	LINE19	"=2 "		/*  */
#define	LINE20	"=3 "		/* automation */
#define	LINE20a	"=3 "		/* */
#define	LINE20b	"=3)"		/* */
#define	LINE20c	"=3V"		/* */
#define	LINE20d	"=3Q"		/* */
#define	LINE21	"=4 "		/* */
#define	LINE21b	"=40"	
#define	LINE21c	"=4G"	
#define	LINE22	"=5 "		/* */
#define	LINE22a	"=5 "		/* */
#define	LINE22b	"=5)"		/* */
#define	LINE22c	"=5V"		/* */
#define	LINE22d	"=5Q"		/* */
#define	LINE23	"=6 "	
#define	LINE23b	"=60"	
#define	LINE23c	"=6G"	
#define	LINE24	"=7 "	
#define	LINE24b	"=70"	
#define	LINE24c	"=7G"	
