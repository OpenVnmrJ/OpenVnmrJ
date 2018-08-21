/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* %Z%%M% %I% %G% Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved
 */
#ifndef VOLUME_H_
#define VOLUME_H_

#include <math.h>
#include "Point3D.h"

class Volume
{
public:
	// box faces
    Point3D facePoints[6][4];

    // The 3D space limits for the volume
    Point3D minVolume;
    Point3D maxVolume;
    Point3D scale;
    Point3D minCoord;
    Point3D maxCoord;
	Point3D center;
    Point3D bounds[8];

    Volume(Point3D min, Point3D max);

    void setScale(Point3D s);
    void getFace(int face, Point3D *pts);
    double minDistance(Point3D pt);
    double maxDistance(Point3D pt);
    Point3D minPoint(Point3D pt);
    Point3D maxPoint(Point3D pt);
    double minPlane(Point3D plane);
    double maxPlane(Point3D plane);
    int boundsPts(Point3D plane, Point3D *returnPts);
    int intersect(Point3D plane, Point3D *point, int inpnts, bool infront, Point3D *returnPts);
    int slicePts(Point3D plane, Point3D front, Point3D back, Point3D *returnPts);
    double planeDist(Point3D pt, Point3D plane);

    Point3D Xmax(){
        return Point3D(maxCoord.x,center.y,center.z);
    }
    Point3D Xmin(){
        return Point3D(minCoord.x,center.y,center.z);
    }
    Point3D Ymax(){
        return Point3D(center.x,maxCoord.y,center.z);
    }
    Point3D Ymin(){
        return Point3D(center.x,minCoord.y,center.z);
    }
    Point3D Zmax(){
        return Point3D(center.x,center.y,maxCoord.z);
    }
    Point3D Zmin(){
        return Point3D(center.x,center.y,minCoord.z);
    }
};
#endif
