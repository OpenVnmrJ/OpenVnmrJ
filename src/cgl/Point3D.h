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
#ifndef POINT3D_H_
#define POINT3D_H_

#include <math.h>

#define RNDERR 1e-12
#define DEQ(x,y) (fabs((x)-(y))<RNDERR)

class Point3D
{
public:
   double x,y,z,w;

 	Point3D() {x=y=z=w=0;}

    Point3D(const Point3D &a){
        x=a.x;y=a.y;z=a.z;w=a.w;
    }

    Point3D(double _x, double _y, double _z){
        x=_x;y=_y;z=_z;w=1;
    }
    Point3D(double _x, double _y, double _z, double _w){
        x=_x;y=_y;z=_z;w=_w;
    }
    double dot(Point3D a){
        return a.x*x+a.y*y+a.z*z;
    }
    Point3D cross(Point3D p){
        return Point3D(y*p.z-z*p.y,z*p.x-x*p.z,x*p.y-y*p.x);
    }

    double distance(Point3D &a){
        return sqrt((a.x-x)*(a.x-x)+(a.y-y)*(a.y-y)+(a.z-z)*(a.z-z));
    }
    Point3D sub(Point3D &a){
        return  Point3D(x-a.x,y-a.y,z-a.z);
    }
    Point3D add(Point3D &a){
        return Point3D(a.x+x,a.y+y,a.z+z);
    }
    Point3D mul(Point3D &a){
        return Point3D(a.x*x,a.y*y,a.z*z);
    }
	Point3D minus(){
		return Point3D(-x,-y,-z);
	}
    Point3D div(Point3D a){
        return Point3D(x/a.x,y/a.y,z/a.z);
    }
    Point3D invert(){
        return Point3D(1.0/x,1.0/y,1.0/z);
    }
    Point3D scale(double s){
        return Point3D(x*s,y*s,z*s);
    }
    Point3D plane(Point3D &eye, Point3D &ref){
        Point3D pt=eye.sub(ref);
        pt.w = -(pt.x * ref.x +pt.y * ref.y +pt.z * ref.z);
        return pt;
    }
    Point3D interpolate(Point3D &p1, double t){
        Point3D p;
        p.x = t * x + (1 - t)* p1.x;
        p.y = t * y + (1 - t)* p1.y;
        p.z = t * z + (1 - t)* p1.z;
        return p;
    }

    double length(){
        return sqrt(x*x+y*y+z*z);
    }

    Point3D normalize(){
        double d=length();
        return Point3D(x/d,y/d,z/d);
    }

    Point3D plane(Point3D &eye, Point3D &ref, double d){
        Point3D v=ref.sub(eye);
        Point3D p=Point3D(eye.x+d*v.x,eye.y+d*v.y,eye.z+d*v.z);
        v.w=-(v.x * p.x +v.y * p.y +v.z * p.z);
        return v;
    }

    Point3D plane(Point3D &ref, double d){
        Point3D v=ref.sub(*this);
        Point3D p=Point3D(x+d*v.x,y+d*v.y,z+d*v.z);
        v.w=-(v.x * p.x +v.y * p.y +v.z * p.z);
        return v;
    }
    int operator==(Point3D&p)		{ return DEQ(x,p.x)&&DEQ(y,p.y)&&DEQ(z,p.z)&&DEQ(w,p.w);}
};
#endif
