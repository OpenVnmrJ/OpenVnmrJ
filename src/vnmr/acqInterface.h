/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef NESSIE
#define  ACQHARDWARE	"sethw"
#define  READACQHW	"readhw"
#define  QUEQUERY	"queuequery"
#define  ACCESSQUERY	"accessquery"
#define  ACQABORT	"abort"
#define  ACQSTOP	"stop"
#define  ACQSUPERABORT	"superabort"
#define  ACQDEBUG	"debug"
#define  ACQIPCTST	"ipctst"
#define  ACQHALT	"halt"

#define  DELIMITER_1	' '
#define  DELIMITER_2	'\n'
#define  DELIMITER_3	','
#else
#define  ACQHARDWARE	"16"
#define  READACQHW	"17"
#define  QUEQUERY	"18"
#define  ACCESSQUERY	"19"
#define  ACQABORT	"4"
#define  ACQSTOP	"6"
#define  ACQSUPERABORT	"7"
#define  ACQDEBUG	"8"
#define  ACQIPCTST	"9"
#define  ACQHALT	"15"

#define  DELIMITER_1	','
#define  DELIMITER_2	','
#define  DELIMITER_3	','
#endif
