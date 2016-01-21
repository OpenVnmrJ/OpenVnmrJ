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
 *    design_flowcomp.h 
 *
 * DESCRIPTION:
 *   This function calculates flow compensation waveform attributes from
 *   the attributes of a primary waveform (slice, read, etc.).
 *   All pertinent attributes are stored in an FCgrp struct.
 *
 *   The basic method is a generalization of the method of Bernstein
 *   (JMRI 1992; 2:583-588).  The compensation waveform lobes are
 *   numbered 1 & 2 as shown in the figure below, so w1 is the width
 *   of lobe 1, m01 the zeroth moment onf lobe 1, etc. (Note: that this
 *   waveform can be time reversed (ie. readout) and still yield the
 *   correct result as described by Bernstein.)
 *
 *       t=0   (Bernstein Method)
 *   ___  |         __
 *      \ |        /  \
 *   _p_ \|_ _ _ _/_2_ \_ _
 *        \  1   /
 *        |\____/
 *
 *    The current code will correctly null both moments in the case where lobe 2
 *    is triangular.  If lobe 1 is triangular the zero moment will be
 *    correct, but some degredation may occur in the first moment nulling.
 *
 * MODIFICATION HISTORY:
 *    Revision 2.2  2003/09/29 17:37:11  erickson
 *    Updated file header.
 *
 *********************************************************************/

typedef struct FCattrib { /*Flow Comp attributes*/
   double m0,m1;        /*primary moments*/
   double amp1,amp2;          /*magnitude of compensation lobes*/
   double w1,w2;        /*total pulse duration for each lobe*/
   double r1,r2;        /*ramp durations for each lobe*/
} FCgrp;
void design_flowcomp(FCgrp *in,double amp,double sr)
{
    double m0, m1;
    double hr, h, r;
    double m01,m02;     /*intermediate moments*/
    double tmp;
    m0 = fabs(in->m0);
    m1 = fabs(in->m1);
    in->amp1 = in->amp2 = h = fabs(amp);
    in->r1 = in->r2 = r = sr*h;
    hr = h*r;
    m02 = (-hr + sqrt(hr*hr+2.0*(hr*m0+m0*m0+2.0*h*m1)))/2.0;
    if (m02 < hr){/*If lobe 2 is triangular*/
       tmp = sqrt(m0*(m0+hr) + 2.0*m1*h);
       m02 = (hr + 2.0*tmp - hr*sqrt(1.0 + (4.0/hr)*tmp))/2.0;
       in->w2 = 2.0*sqrt(m02*r/h);
       in->r2 = in->w2/2.0;
       /*in->amp2 = in->r2/sr;*/
       /*Command amps to h and turnaround midstream*/
       in->amp2 = h;
    }else{
       in->w2 = m02/h + r;
       in->r2 = r;
    }
    m01 = m0 + m02;
    if (m01 < hr){/*If lobe 1 is triangular*/
       in->w1 = 2.0*sqrt(m01*r/h);
       in->r1 = in->w1/2.0;
       /*in->amp1 = in->r1/sr;*/
       /*Command amps to h and turnaround midstream*/
       in->amp1 = h;
    }else{
       in->w1 = m01/h + r;
       in->r1 = r;
    }
}

