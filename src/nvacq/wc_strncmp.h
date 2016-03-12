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
 *        http://www.pwilson.net/   pete at pwilson dot net   +1 978-454-4547
 *
 * This item is free software: you can redistribute it and/or modify it as 
 * long as you preserve this copyright notice. Pete Wilson prepared this item 
 * hoping it might be useful, but it has NO WARRANTY WHATEVER, not even any 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 *************************************************************************** */

/* ***************************************************************************
 *
 *                            WC_STRNCMP.H
 *
 * Defines for string searches. Revision History:
 *
 *     DATE      VER                     DESCRIPTION
 * -----------  ------  ----------------------------------------------------
 *  8-nov-2001   1.06   rewrite
 * 20-jan-2002   1.07   rearrange wc test; general dust and vacuum
 *
 *************************************************************************** */
#ifndef WC_STRNCMP_H_INCLUDED
#define WC_STRNCMP_H_INCLUDED

#ifdef   __cplusplus
extern "C" {
#endif

int wc_strncmp(const char * pattern,   /* match this str (can contain *,?) */
               const char * candidate, /*   against this one. */
               int count,              /* require count chars in pattern */
               int do_case,            /* 0 = no case, !0 = cased compare */ 
               int do_wildcards);      /* 0 = no wc's, !0 = honor * and ? */

#define WC_QUES '?'
#define WC_STAR '*'

/* string and character match/no-match return codes */

enum {
  WC_MATCH = 0,     /* char/string-match succeed */
  WC_MISMATCH,      /* general char/string-match fail */
  WC_PAT_NULL_PTR,  /* (char *) pattern == NULL */
  WC_CAN_NULL_PTR,  /* (char *) candidate == NULL */
  WC_PAT_TOO_SHORT, /* too few pattern chars to satisfy count */
  WC_CAN_TOO_SHORT  /* too few candidate chars to satisy '?' in pattern */
};

#ifdef   __cplusplus
}
#endif

#endif /* WC_STRNCMP_H_INCLUDED */



