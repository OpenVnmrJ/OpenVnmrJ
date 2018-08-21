/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************
 *
 * NAME:
 *    fcprint.c
 *
 * DESCRIPTION:
 *
 * MODIFICATION HISTORY:
 *    Revision 1.3  2003/09/29 17:37:11  erickson
 *    Updated file header.
 *
 *********************************************************************/

void fcprint(FCgrp *in,char *name)
{
   printf("%s ******\n",name);
   printf("m0 = %e  m1 = %e\n",in->m0,in->m1);
   printf("rlobe: amp = %f, w = %e, r = %e\n",in->amp1,in->w1,in->r1);
   printf("flobe: amp = %f, w = %e, r = %e\n",in->amp2,in->w2,in->r2);
}

