/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
   these headers are prepended to each acode, 
   pattern, table, lc etc. emitted by PSG
   acode compression info is provided but not
   implemented here.
*/
/* these are IDENTIFIERS for the fields */
/* id and number |31 - 28| are ID Fields 
                 |27 - 24| reserved 
                 |23 -  0| number 
*/

#ifndef INC_COMBOHEADER_H
#define INC_COMBOHEADER_H

#define INITIALHEADER   0x10000000
#define ACODEHEADER     0x20000000
#define PATTERNHEADER   0x30000000
#define TABLEHEADER     0x40000000
#define POSTEXPHEADER   0x50000000
#define UNDEFINEDHEADER 0x0

#define LC_FORMAT_STR  	    "%slc%d"
#define ACODE_FORMAT_STR    "%sf%u"
#define PATTERN_FORMAT_STR  "%sp%d"
#define TABLE_FORMAT_STR    "%st%d"
#define POSTEXP_FORMAT_STR  "%spost%d"
 
#define COMBOID_NUM_WORD 101

typedef struct _ACODEHEADER_ {
    int compressed_size;
    int full_size;
    int key;
} AcodeHeader;

typedef struct _TABLEHEADER_ {
    int auto_inc_flag;
    int numberElements;
    int divn_factor;
    int autoIndex;
} TableHeader;

typedef struct _PATTERNHEADER_ { 
    int pattern_element_size;
    int numberElements;
} PatternHeader;

typedef struct _COMBOHEADER_ {
  int comboID_and_Number;
  int sizeInBytes;
  union {  AcodeHeader AC;
           TableHeader TBL; 
           PatternHeader PAT;
         } details ;
} ComboHeader ;

#endif

/* syntax example 
int main(int *argc, char **argv)
{
  struct _COMBOHEADER_ ch;
  ch.comboID_and_Number =  0xA0000001;
  ch.sizeInBytes = 10000;
  ch.AC.compressed_size = 1000;
}
*/
