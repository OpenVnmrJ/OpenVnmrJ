/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPORTHOSLICES_H
#define AIPORTHOSLICES_H
enum OrthoPlanes {
    XPLANE=2,
    YPLANE=3,
    ZPLANE=1
};
enum {X_PLUS, X_MINUS, Y_PLUS, Y_MINUS, Z_PLUS, Z_MINUS};
enum {AXIAL, SAGITTAL, CORONAL};


class OrthoSlices {
public:
    OrthoSlices(void);
    static OrthoSlices *get();
    static int StartExtract(int argc, char **argv, int, char **);
    static int MipMode();
    static int WidthReversed();
    static int WidthEnabled();
    static int WidthValue(double &w);
    static int Getg3dpnt(double &x, double &y, double &z);
    static int Getg3drot(double &x, double &y, double &z);
    static int Setg3dpnt(int argc, char **argv, int, char **);
    static int Setg3drot(int argc, char **argv, int, char **);
    static int Setg3dflt(int argc, char **argv, int, char **);
    static int RedrawMip(int argc, char **argv, int, char **);
    static int Show3PCursors(int argc, char **argv, int, char **);
    static void updateCursors();
    static void updateDisplayMode();
    void drawThreePlanes(int sliceX, int sliceY, int sliceZ);
    bool extractCursorMoved(int x, int y);
    bool startCursorMoved(int x, int y);
    bool endCursorMoved(int x, int y);
    static bool getShowCursors();
    static void setShowCursors(bool b);
    void setNext3Ppnt(int,int);

private:
    static OrthoSlices *orthoSlices; // support one instance
    void drawThreeCursors(int x, int y, int activeFrame);
    void extractPlane(int plane, int index);
    void redraw();
    static bool showCursors;
};

#endif /*AIPORTHOSLICES_H */
