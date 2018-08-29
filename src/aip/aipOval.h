/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPOVAL_H
#define AIPOVAL_H

#include "aipBox.h"

class Oval : public Box
{
public:
    Oval(spGframe_t gf, int x, int y); // Init degenerate oval
    Oval(spGframe_t gf, spCoordVector_t dpts, bool pixflag=false); // Init to points on data

    virtual Roi *copy(spGframe_t gframe);
    virtual void draw();
    virtual double distanceFrom(int x, int y, double far);
    virtual float *firstPixel();
    virtual float *nextPixel();
    virtual const char *name(void) { return ("Oval"); }

private:
    // Used to iterate through an ellipse (also uses some Box variables)
    double m_a;                   // Horizontal semi-axis
    double m_b;                   // Vertical semi-axis
    double m_xc;                  // Center of ellipse
    double m_yc;
    int m_last_row;
    float *m_beg_of_data;

};

#endif /* AIPOVAL_H */
