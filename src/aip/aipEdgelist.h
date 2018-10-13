/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPEDGELIST_H
#define AIPEDGELIST_H

#include "aipRoi.h"		// for pntPix_t

// This class is designed to build an edgelist forf a polygon or
// an oval.
// (Note that not all functions are used by oval)
class Edgelist
{
private:
    static void sort_edge_list(Edgelist **, int, LpointList&);
    static void build_line_points(LpointList&, short, short, short, short);
    static void check_local_minmax(Edgelist **, int, pntPix_t, int);
    static void check_straight_line(Edgelist **, int, pntPix_t, int);
    static void remove_one_edge(Edgelist *&, short);

public:
    short x_edge;
    Edgelist *next;

    Edgelist(short x)
	: x_edge(x), next(NULL) {}

    static void build_ybucket(Edgelist **, int, pntPix_t, int, int);
    static void free_ybucket(Edgelist **, int);
    static void update_ybucket(Edgelist **, int, int);
    static bool point_inside_xedge(Edgelist *, short);
};

#endif /* AIPEDGELIST_H */
