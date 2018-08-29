/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPCSTRUCTS_H
#define AIPCSTRUCTS_H

typedef struct _palette {
    char label[32];		/* E.g. "Fixed colors", "Grayscale" */
    int id;
    int firstColor;		/* Index of 1st color in Vnmr cmap */
    int numColors;		/* Nbr of colors in this palette */
    int zeroColor;		/* Index of color corresponding to 0, or -1 */
    long mtime;               /* the file last modified time */
    char **names;		/* Null terminated list (optional) */
    char *mapName;		/* colormap name */
    char *fileName;		/* colormap file path */
} palette_t;

typedef enum {
   NAMED_COLORS = 0,
   CODED_COLORS,
   ABSVAL_COLORS,
   SIGNED_COLORS,
   GRAYSCALE_COLORS
} colormapSegment_t;

#define MAXRANK 4		/* Max dimensions in data */
#define RFCHANS 6
typedef struct {
    int id;			/* An ID that uniquely identifies this image. */
    double location[3];		/* Position of data volume in magnet (cm);
				 * orientation of coords is "orientation" */
    double roi[3];		/* Size of data volume (cm) */
    double euler[3];

    int framestx, framesty;
    int framewd, frameht;

    /*
     * The window area occupied by the image.  These are integers
     * because we don't "split pixels" on the screen.  A pixel is
     * either entirely within the image or entirely out of it.
     * Note that the screen distance between the first and last
     * pixel in a row is (pixwd - 1).
     * For 1-d data, pixht is still the height of the area drawn on.
     */
    int pixstx, pixsty;		/* Upper-left corner of image on window */
    int pixwd, pixht;		/* Number of pixels along edges of image */

    /*
     * Transformation matrices for converting between pixel location
     * in the window and position in the magnet.  E.g., matrix
     * multiply:
     *
     *  ( X )   ( p2m[0][0] p2m[0][1] p2m[0][2] p2m[0][3] )   ( x )
     *  ( Y ) = ( p2m[1][0] p2m[1][1] p2m[1][2] p2m[1][3] ) * ( y )
     *  ( Z )   ( p2m[2][0] p2m[2][1] p2m[2][2] p2m[2][3] )   ( z )
     *  ( 1 )   (     0         0         0         1     )   ( 1 )
     *
     * where lower case is the pixel coordinate and upper case is the
     * magnet coordinate.  This is an affine transformation, where the
     * p2m[i][3] are the offsets.  Note that the last row of the
     * matrix is not actually stored, but only implied.  All pixels on
     * the screen have z=0; z>0 is behind the screen, z<0 in front.
     * The x coordinate increases to the right on the screen, while y
     * increases downwards.
     *
     * The matrix m2p is the inverse of p2m, for converting from
     * magnet to pixel coordinates.  It's bottom row is the same as
     * that of p2m, and it also is not stored.
     *
     * Note that:
     * m2p[i][0]**2 + m2p[i][1]**2 + m2p[i][2]**2 = pixelsPerCm**2  (i=0,1,2)
     * Dividing the upper-left-hand 3x3 submatrix of m2p by
     * pixelsPerCm yields the rotation matrix for going from magnet to
     * screen coordinates.
     */
    double p2m[3][4];		/* Pixel address to magnet coord conversion */
    double m2p[3][4];		/* Magnet coord to pixel address conversion */
    double pixelsPerCm;		/* For convenience */
    double b2m[3][3];		/* Pixel address to magnet coord conversion */
    int coils;          /* number of coils for multi-mouse */
    int coil;           /* coil id for multi-mouse */
    double origin[3];           /* origin of the coil */

} CImgInfo_t;

typedef struct {
    int framestx, framesty;
    int framewd, frameht;
} CFrameInfo_t;

#undef MAXRANK
#undef RFCHANS

#endif /* AIPCSTRUCTS_H */
