/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
*/

#include <math.h>
#include <iostream>
#include "Point3D.h"
#include "Volume.h"
#include "CGLDef.h"

// Box Geometry
//
//       2--------6
//      /|       /|         +Y   +Z
//     / |      / |          | /
//    3--|-----7  |          |/
//    |  1-----|--5   -X ----/---- +X
//    | /      | /          /|
//    |/       |/          / |
//    0--------4         -Z  -Y
//
// VOI box faces

static int edges[12][2]={ // edges of the cube
        0, 1, // 0
        1, 2, // 1
        2, 3, // 2
        3, 0, // 3
        0, 4, // 4
        1, 5, // 5
        2, 6, // 6
        3, 7, // 7
        4, 5, // 8
        5, 6, // 9
        6, 7, // 10
        7, 4  // 11
        };

static int faces[6][4]= { // edges of faces of cube
         0, 1, 2, 3,
         1, 6, 9, 5,
         6, 2, 7, 10,
         7, 3, 4, 11,
         0, 5, 8, 4,
         9, 8, 10, 11
        };

//-------------------------------------------------------------
// constructor
//-------------------------------------------------------------
Volume::Volume(Point3D min, Point3D max) {
	minVolume=min;
	maxVolume=max;

    scale=Point3D(1,1,1);

    setScale(scale);

}

//-------------------------------------------------------------
// set volume scaling factors
//-------------------------------------------------------------
void Volume::setScale(Point3D s) {
	scale=s;
	minCoord=minVolume.mul(scale);
	maxCoord=maxVolume.mul(scale);

	bounds[0].x = bounds[1].x = bounds[2].x = bounds[3].x = minCoord.x;
	bounds[4].x = bounds[5].x = bounds[6].x = bounds[7].x = maxCoord.x;
	bounds[0].y = bounds[1].y = bounds[4].y = bounds[5].y = minCoord.y;
	bounds[2].y = bounds[3].y = bounds[6].y = bounds[7].y = maxCoord.y;
	bounds[0].z = bounds[3].z = bounds[4].z = bounds[7].z = minCoord.z;
	bounds[1].z = bounds[2].z = bounds[5].z = bounds[6].z = maxCoord.z;

    facePoints[PLUS_X][0] = bounds[5];
    facePoints[PLUS_X][1] = bounds[4];
    facePoints[PLUS_X][2] = bounds[7];
    facePoints[PLUS_X][3] = bounds[6];

    facePoints[PLUS_Y][0] = bounds[2];
    facePoints[PLUS_Y][1] = bounds[3];
    facePoints[PLUS_Y][2] = bounds[7];
    facePoints[PLUS_Y][3] = bounds[6];

    facePoints[PLUS_Z][0] = bounds[1];
    facePoints[PLUS_Z][1] = bounds[2];
    facePoints[PLUS_Z][2] = bounds[6];
    facePoints[PLUS_Z][3] = bounds[5];

    facePoints[MINUS_X][0] = bounds[0];
    facePoints[MINUS_X][1] = bounds[1];
    facePoints[MINUS_X][2] = bounds[2];
    facePoints[MINUS_X][3] = bounds[3];

    facePoints[MINUS_Y][0] = bounds[0];
    facePoints[MINUS_Y][1] = bounds[4];
    facePoints[MINUS_Y][2] = bounds[5];
    facePoints[MINUS_Y][3] = bounds[1];

    facePoints[MINUS_Z][0] = bounds[0];
    facePoints[MINUS_Z][1] = bounds[3];
    facePoints[MINUS_Z][2] = bounds[7];
    facePoints[MINUS_Z][3] = bounds[4];


	Point3D pt=maxCoord.add(minCoord);
	center=pt.scale(0.5);
}

//-------------------------------------------------------------
// Return array of 4 points on a face
//-------------------------------------------------------------
void Volume::getFace(int face, Point3D *pts){
    switch(face){
    case MINUS_X:
        pts[0]=bounds[0];
        pts[1]=bounds[1];
        pts[2]=bounds[2];
        pts[3]=bounds[3];
        break;
    case PLUS_X:
        pts[0]=bounds[5];
        pts[1]=bounds[4];
        pts[2]=bounds[7];
        pts[3]=bounds[6];
    case MINUS_Y:
        pts[0]=bounds[0];
        pts[1]=bounds[4];
        pts[2]=bounds[5];
        pts[3]=bounds[1];
        break;
    case PLUS_Y:
        pts[0]=bounds[2];
        pts[1]=bounds[3];
        pts[2]=bounds[7];
        pts[3]=bounds[6];
        break;
    case MINUS_Z:
        pts[0]=bounds[0];
        pts[1]=bounds[3];
        pts[2]=bounds[7];
        pts[3]=bounds[4];
        break;
    case PLUS_Z:
        pts[0]=bounds[1];
        pts[1]=bounds[2];
        pts[2]=bounds[6];
        pts[3]=bounds[5];
        break;
     }
}

//-------------------------------------------------------------
// Return a variable length array of points, sorted in clockwize order,
// that consist of all intersections of the line segments of the bounds
// box with a given plane in object space.
//-------------------------------------------------------------
int Volume::boundsPts(Point3D plane, Point3D *returnPts) {
    int numOutEdges = 0;
    Point3D outEdges[6][12];
    Point3D wantVtx;

    Point3D *points = bounds;
    double ptDist[8]; // dist of pt from plane
    bool edgeFlags[12] ; // edge cut by plane?
    Point3D edgeInts[12] ; // intersection in edge
    // determine the distance of each point from the plane
    for(int i = 0; i < 8; i++) {
        ptDist[i] = planeDist(points[i], plane);
    }

    // scan each edge, mark the ones that are cut and calc the
    // intersection
    for(int i = 0; i < 12; i++) {
        double dst0 = ptDist[edges[i][0]];
        double dst1 = ptDist[edges[i][1]];
        if((dst0 > 0) ^ (dst1 > 0)) {
            edgeFlags[i] = true;
            double t = dst1 / (dst1 - dst0);
            edgeInts[i].x = t * points[edges[i][0]].x + (1 - t)
                    * points[edges[i][1]].x;
            edgeInts[i].y = t * points[edges[i][0]].y + (1 - t)
                    * points[edges[i][1]].y;
            edgeInts[i].z = t * points[edges[i][0]].z + (1 - t)
                    * points[edges[i][1]].z;
        } else {
            edgeFlags[i] = false;
        }
    }

    // scan each face, if it is cut by the plane, make an edge across
    // the face
    for(int i = 0; i < 6; i++) {
        bool anyCut = (edgeFlags[faces[i][0]] | edgeFlags[faces[i][1]]
                | edgeFlags[faces[i][2]] | edgeFlags[faces[i][3]]);
        if(anyCut) {
            int edgePt = 0;
            if(edgeFlags[faces[i][0]]) {
                outEdges[numOutEdges][edgePt++] = edgeInts[faces[i][0]];
            }
            if(edgeFlags[faces[i][1]]) {
                outEdges[numOutEdges][edgePt++] = edgeInts[faces[i][1]];
            }
            if(edgeFlags[faces[i][2]]) {
                outEdges[numOutEdges][edgePt++] = edgeInts[faces[i][2]];
            }
            if(edgeFlags[faces[i][3]]) {
                outEdges[numOutEdges][edgePt++] = edgeInts[faces[i][3]];
            }
            numOutEdges++;
        }
    }

    // sort the edges, matching the endpoints to make a loop
    for(int i = 0; i < numOutEdges; i++) {
        wantVtx = outEdges[i][1];
        for(int j = i + 1; j < numOutEdges; j++) {
            if((outEdges[j][0] == wantVtx) || (outEdges[j][1] == wantVtx)) {
                if(outEdges[j][1] == wantVtx) {
                    Point3D temp = outEdges[j][0];
                    outEdges[j][0] = outEdges[j][1];
                    outEdges[j][1] = temp;
                 }
                if(j != (i + 1)) {
                    Point3D temp0 = outEdges[i + 1][0];
                    Point3D temp1 = outEdges[i + 1][1];
                    outEdges[i + 1][0] = outEdges[j][0];
                    outEdges[i + 1][1] = outEdges[j][1];
                    outEdges[j][0] = temp0;
                    outEdges[j][1] = temp1;
                }
            }
        }
    }

    for(int i = 0; i < numOutEdges; i++) {
        returnPts[i] = outEdges[i][0];
    }
    return numOutEdges;
}

//-------------------------------------------------------------
// Return an array of points from the intersection of a plane
// with an array of points that are assumed to be on a second plane.
//-------------------------------------------------------------
int Volume::intersect(Point3D plane, Point3D *points, int numInPoints,
        bool infront, Point3D *outPoints) {
    Point3D tmp[16];
    int numOutPoints = 0;
    int i;
    for (i = 0; i < numInPoints; i++) {
        Point3D p0=points[i];
        Point3D p1;
        if (i==(numInPoints-1))
            p1=points[0];
        else
            p1=points[i+1];
        double d0 = planeDist(p0, plane);
        double d1 = planeDist(p1, plane);
        if (infront) {
            d0=-d0;
            d1=-d1;
        }
        if (d0<=0)
            tmp[numOutPoints++] = Point3D(p0);
        if ((d0 > 0) ^ (d1 > 0)) { // "^" = exclusive or
            double t = d1 / (d1 - d0);
            tmp[numOutPoints++]=p0.interpolate(p1, t);
        }
    }
    for (i=0; i<numOutPoints; i++)
        outPoints[i]=tmp[i];
    return numOutPoints;
}

//-------------------------------------------------------------
// Return an array of points from the intersection of a plane
// in object space with the union of the boundary box and a
// sliceplane.
//-------------------------------------------------------------
int Volume::slicePts(Point3D plane, Point3D front, Point3D back, Point3D *returnPts) {
    Point3D points[12];
    int numInPts=boundsPts(plane,points);
    int i;
    int numOutPoints = 0;
    Point3D outPoints[16];
    Point3D outEdges[16][2];
    Point3D wantVtx;

    numOutPoints=intersect(front,points,numInPts,false,outPoints);
    numOutPoints=intersect(back,outPoints,numOutPoints,true,outPoints);

    for(i = 0; i < numOutPoints; i++) {
        Point3D p0=outPoints[i];
        Point3D p1;
        if(i==(numOutPoints-1))
            p1=outPoints[0];
        else
            p1=outPoints[i+1];
        outEdges[i][0]=p0;
        outEdges[i][1]=p1;

    }
    // sort the edges, matching the endpoints to make a loop
    for(int i = 0; i < numOutPoints; i++) {
        wantVtx = outEdges[i][1];
        for(int j = i + 1; j < numOutPoints; j++) {
            if((outEdges[j][0] == wantVtx) || (outEdges[j][1] == wantVtx)) {
                if(outEdges[j][1] == wantVtx) {
                    Point3D temp = outEdges[j][0];
                    outEdges[j][0] = outEdges[j][1];
                    outEdges[j][1] = temp;
                }
                if(j != (i + 1)) {
                    Point3D temp0 = outEdges[i + 1][0];
                    Point3D temp1 = outEdges[i + 1][1];
                    outEdges[i + 1][0] = outEdges[j][0];
                    outEdges[i + 1][1] = outEdges[j][1];
                    outEdges[j][0] = temp0;
                    outEdges[j][1] = temp1;
                }
            }
        }
    }
    for(i = 0; i < numOutPoints; i++) {
        returnPts[i] = outEdges[i][0];
    }
    return numOutPoints;
}

//-------------------------------------------------------------
// Return minimum vertex distance to a Point.
//-------------------------------------------------------------
double Volume::minDistance(Point3D pt) {
    double l=pt.distance(center);
    Point3D plane=center.plane(pt, 0);
    return minPlane(plane)/l;
}

//-------------------------------------------------------------
// Return maximum vertex distance to a Point.
//-------------------------------------------------------------
double Volume::maxDistance(Point3D pt) {
    double l=pt.distance(center);
    Point3D plane=center.plane(pt, 0);
    return maxPlane(plane)/l;
}

//-------------------------------------------------------------
// Return the distance between a point and a plane.
//-------------------------------------------------------------
double Volume::planeDist(Point3D pt, Point3D plane) {
    return pt.x * plane.x + pt.y * plane.y + pt.z * plane.z + plane.w;
}

//-------------------------------------------------------------
//  Return the maximum distance of bounding box vertex to plane
//-------------------------------------------------------------
double Volume::minPlane(Point3D plane) {
    double max=-1e6;
    for(int i=0;i<8;i++){
        double d=planeDist(bounds[i],plane);
        max=d>max?d:max;
    }
    return max;
}

//-------------------------------------------------------------
/// Return the minimum distance of bounding box vertex to plane
//-------------------------------------------------------------
double Volume::maxPlane(Point3D plane) {
    double min=1e6;
    for(int i=0;i<8;i++){
        double d=planeDist(bounds[i],plane);
        min=d<min?d:min;
    }
    return min;
}

//-------------------------------------------------------------
// Return a point on a line passing through the input point and center
// that is at the depth of the closest vertex
//-------------------------------------------------------------
Point3D Volume::minPoint(Point3D pt) {
    double d=minDistance(pt);
    Point3D v=pt.sub(center);
    v=v.normalize();
    Point3D sv=v.scale(d);
    pt=center.add(sv);
    return pt;
}

//-------------------------------------------------------------
// Return a point on a line passing through the input point and center
// that is at the depth of the farthest vertex.
//-------------------------------------------------------------
Point3D Volume::maxPoint(Point3D pt) {
    double d=maxDistance(pt);
    Point3D v=pt.sub(center);
    v=v.normalize();
    Point3D sv=v.scale(d);
    pt=center.add(sv);
    return pt;
}
