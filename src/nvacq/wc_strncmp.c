/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ***************************************************************************
 *
 *          Copyright 1992-2005 by Pete Wilson All Rights Reserved
 *           50 Staples Street : Lowell Massachusetts 01851 : USA
 *       http://www.pwilson.net/   pete at pwilson dot net   +1 978-454-4547
 *
 * This item is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the 
 * Free Software Foundation; either version 2.1 of the License, or (at your 
 * option) any later version.
 *
 * Pete Wilson prepared this item in the hope that it might be useful, but it 
 * has NO WARRANTY WHATEVER, nor even any implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 *************************************************************************** */

/* ***************************************************************************
 *
 *                          WC_STRNCMP.C
 *
 * Public function to match two strings with or without wildcards:
 *    int wc_strncmp()  return zero if string match, else non-zero.
 *
 * Local private function to match two characters:
 *    int ch_eq()       return zero if char match, else non-zero.       
 *
 * Revision History:
 *
 *     DATE      VER                     DESCRIPTION
 * -----------  ------  ----------------------------------------------------
 *  8-nov-2001   1.06   rewrite
 * 20-jan-2002   1.07   rearrange wc test; general cleanup
 * 14-feb-2002   1.08   fix bug (thanks, steph!): wc test outside ?,* tests
 * 20-feb-2002   1.09   rearrange commentary as many suggested
 * 25-feb-2002   1.10   wc_strlen() return size_t per dt, excise from here
 *
 *************************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif
 
#include <ctype.h>               /* only for tolower() proto */
#include <string.h>              /* only for strlen() proto */

#include "wc_strncmp.h"

static int ch_eq(int c1, int c2, int do_case); /* compare two characters */

/* ***************************************************************************
 *
 * Function:   wc_strncmp(
 *               const char * pattern,   // match this string (can contain ?, *)
 *               const char * candidate, //   against this one.
 *               int count,              // require at least count chars in pattern
 *               int do_case,            // 0 = no case, !0 = cased compare
 *               int do_wildcards)       // 0 = no wc's, !0 = honor ? and *
 *
 * Action:     See if the string pointed by candidate meets the criteria    
 *             given in the string pointed by pattern. The pattern string   
 *             can contain the special wild-card characters:                
 *             * -- meaning match any number of characters, including zero, 
 *               in the candidate string;                                   
 *             ? -- meaning match any single character, which must be present 
 *               to satisfy the match, in the candidate string.
 *
 *             The int arg count tells the minimum length of the pattern
 *             string, after '*' expansion, that will satisfy the string
 *             compare in this call. If count is negative, then forbid
 *             abbreviations: pattern and candidate strings must match exactly
 *             in content and length to give a good compare. If count is 0, 
 *             then an empty pattern string (pattern == "") returns success. 
 *             If count is positive, then must match "count" characters
 *             in the two strings to succeed, except we yield success if:
 *             -- pattern and candidate strings are the same length; or
 *             -- pattern string is shorter than "count"; or
 *             -- do_wildcards > 0 and final pattern char == '*'. 
 * 
 *             If the integer argument do_case == 0, then ignore the case of 
 *             each character: compare the tolower() of each character; if 
 *             do_case != 0, then consider case in character compares.
 *
 *             If the int arg do_wildcards == 0, then treat the wildcard 
 *             characters '*' and '?' just like any others: do a normal 
 *             char-for-char string compare. But if do_wildcards is nonzero, 
 *             then the string compare uses those wildcard characters as 
 *             you'd expect.
 *
 * Returns:    WC_MATCH on successful match. If not WC_MATCH, then the
 *             return val conveys a little more info: see wc_strncmp.h
 *
 * Note on Resynchronization: After a span of one or more pattern-string '*' 
 *             found; and an immediately following span of zero [sic: 0] or 
 *             more '?', we have to resynchronize the candidate string 
 *             with the pattern string: need to find a place in the 
 *             candidate to restart our compares. We've not a clue how many 
 *             chars a span of '*' will match/absorb/represent until we find 
 *             the next matching character in the candidate string. 
 *             For example:
 *       
 * -------------------------------------------------------------------------
 *   patt   cand   resync at 'A'           what's going on?
 *  ------ ------- ------------- ------------------------------------------
 *  "***A" "AbcdA"   "A----"     "***" matches zero characters
 *  "**A"  "aAcdA"   "-A---"     "**" absorbs just one char: "a"
 *  "*??A" "abcdA"   "----A"     "*" absorbs "ab", "??" skips "cd"
 *  "**?A" "abAdA"   "--A--"     "**" absorbs "a", "?" skips "b" 
 * -------------------------------------------------------------------------
 *      
 *             During any resync phase, we'll naturally be looking for the 
 *             end of the candidate string and fail if we see it.
 *   
 *************************************************************************** */
int 
wc_strncmp(const char * pattern, 
           const char * candidate, 
           int count, 
           int do_case, 
           int do_wildcards) 
{
  const char * can_start;
  unsigned int star_was_prev_char;
  unsigned int ques_was_prev_char;
  unsigned int retval;

  if (pattern == NULL)           /* avoid any pesky coredump, please */
  {
    return WC_PAT_NULL_PTR;
  }

  if (candidate == NULL) 
  {
    return WC_CAN_NULL_PTR;
  }

  /* match loop runs, comparing pattern and candidate strings char by char,
   until (1) the end of the pattern string is found or (2) somebody found
   some kind of mismatch condition. we deal with four cases in this loop: 
     -- pattern char is '?'; or 
     -- pattern char is '*'; or 
     -- candidate string is exhausted; or
     -- none of the above, pattern char is just a normal char.  */

  can_start = candidate;         /* to calc n chars compared at exit time */
  star_was_prev_char = 0;        /* show previous character was not '*' */
  ques_was_prev_char = 0;        /* and it wasn't '?', either */
  retval = WC_MATCH;             /* assume success */

  while (retval == WC_MATCH && *pattern != '\0') 
  {

    if (do_wildcards != 0)       /* honoring wildcards? */
    { 

      /* first: pattern-string character == '?' */

      if (*pattern == WC_QUES)   /* better be another char in candidate */
      { 
        ques_was_prev_char = 1;  /* but we don't care what it is */
      }
    
      /* second: pattern-string character == '*' */

      else if (*pattern == WC_STAR) 
      {  
        star_was_prev_char = 1;  /* we'll need to resync later */
        ++pattern;
        continue;                /* rid adjacent stars */ 
      }
    }

    /* third: any more characters in candidate string? */

    if (*candidate == '\0') 
    {
      retval = WC_CAN_TOO_SHORT;
    }

    else if (ques_was_prev_char > 0) /* all set if we just saw ques */
    {
      ques_was_prev_char = 0;        /* reset and go check next pair */
    }

    /* fourth: plain old char compare; but resync first if necessary */

    else 
    {
      if (star_was_prev_char > 0) 
      { 
        star_was_prev_char = 0;

        /* resynchronize (see note in header) candidate with pattern */

        while (WC_MISMATCH == ch_eq(*pattern, *candidate, do_case)) 
        {
          if (*candidate++ == '\0') 
          {
            retval = WC_CAN_TOO_SHORT;
            break;
          }
        }
      }           /* end of re-sync, resume normal-type scan */

      /* finally, after all that rigamarole upstairs: the main char compare */

      else  /* if star or ques was not last previous character */
      {
        retval = ch_eq(*pattern, *candidate, do_case);
      }
    }

    ++candidate;  /* point next chars in candidate and pattern strings */
    ++pattern;    /*   and go for next compare */

  }               /* while (retval == WC_MATCH && *pattern != '\0') */


  if (retval == WC_MATCH) 
  {

  /* we found end of pattern string, so we've a match to this point; now make
     sure "count" arg is satisfied. we'll deem it so and succeed the call if: 
     - we matched at least "count" chars; or
     - we matched fewer than "count" chars and:
       - pattern is same length as candidate; or
       - we're honoring wildcards and the final pattern char was star. 
     spell these tests right out /completely/ in the code */

    int min;
    int nmatch;

    min = (count < 0) ?                  /* if count negative, no abbrev: */
           strlen(can_start) :           /*   must match two entire strings */
           count;                        /* but count >= 0, can abbreviate */

    nmatch = candidate - can_start;      /* n chars we did in fact match */

    if (nmatch >= min)                   /* matched enough? */
    {                                    /* yes; retval == WC_MATCH here */
      ;                                  
    }
    else if (*candidate == '\0')         /* or cand same length as pattern? */
    {                                    /* yes, all set */
      ;                                  
    }
    else if (star_was_prev_char != 0)    /* or final char was star? */
    {                                    /* yes, succeed that case, too */
      ;                                  
    }                          

    else                                 /* otherwise, fail */
    {
      retval = WC_PAT_TOO_SHORT;           
    }
  }

  return retval;
}

/* ***************************************************************************
 *
 * Function:   int ch_eq(int c1, int c2, int do_case)                  
 *
 * Action:     Compare two characters, c1 and c2, and return WC_MATCH if     
 *             chars are equal, WC_MISMATCH if chars are unequal. If the 
 *             integer arg do_case == 0, treat upper-case same as lower-case.
 *
 * Returns:    WC_MATCH or WC_MISMATCH
 *
 *************************************************************************** */
static int   
ch_eq(int c1, 
      int c2, 
      int do_case) 
{
  if (do_case == 0)      /* do_case == 0, ignore  case */
  {                
    c1 = tolower(c1);
    c2 = tolower(c2);
  }
  return (c1 == c2) ? WC_MATCH : WC_MISMATCH;
}

#ifdef __cplusplus
}
#endif


