/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ASPDISInteg_H
#define ASPDISInteg_H

#include <list>
#include "AspFrame.h"

class AspDisInteg {

public:

    static int aspInteg(int argc, char *argv[], int retc, char *retv[]);
    static void display(spAspFrame_t frame); 
    
    static spAspInteg_t selectInteg(spAspFrame_t frame, int x, int y);
    static void deleteInteg(spAspFrame_t frame, spAspInteg_t integ);

    static void save(spAspFrame_t frame, char *path = NULL);
    static void load(spAspFrame_t frame, char *path = NULL, bool show=false);

    static spAspInteg_t createInteg(spAspFrame_t frame, int x,int y,int prevX, int prevY);
    static void modifyInteg(spAspFrame_t frame, spAspInteg_t integ, int x, int y, 
		int prevX, int prevY, int mask);

    static double noisemult;

private:

    static void nli(spAspFrame_t frame, int argc, char *argv[]);
    static void dpir(spAspFrame_t frame, int argc, char *argv[]);
};

#endif /* ASPDISInteg_H */
