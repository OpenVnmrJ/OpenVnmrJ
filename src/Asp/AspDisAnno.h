/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ASPDISANNO_H
#define ASPDISANNO_H

#include <list>
#include "AspFrame.h"

class AspDisAnno {

public:

    static int aspAnno(int argc, char *argv[], int retc, char *retv[]);
    static void display(spAspFrame_t frame); 
    
    static AspAnno *selectAnno(spAspFrame_t frame, int x, int y);
    static void deleteAnno(spAspFrame_t frame, AspAnno *anno);

    static void save(spAspFrame_t frame, char *path = NULL);
    static void load(spAspFrame_t frame, char *path = NULL, bool show=false, int retc=1);

    static AspAnno *createAnno(spAspFrame_t frame, int x, int y, AnnoType_t type,
                               bool trCase=false);
    static void modifyAnno(spAspFrame_t frame, AspAnno *anno, int x, int y, int prevX, int prevY,
                           bool trCase = false);
    static void addPoint(spAspFrame_t frame, AspAnno *anno, int x, int y, bool insert=false);
    static void deletePoint(spAspFrame_t frame, AspAnno *anno, int x, int y);

    static void show(spAspFrame_t frame, int argc, char *argv[]); 

    static void paste(spAspFrame_t frame, string str, int x, int y);
    static string clipboardStr;
    static void testAnno(spAspFrame_t frame, int argc, char *argv[],
                     int retc, char *retv[]);

private:

};

#endif /* ASPDISANNO_H */
