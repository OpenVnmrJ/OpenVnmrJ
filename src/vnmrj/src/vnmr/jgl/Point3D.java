/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import java.text.DecimalFormat;

public class Point3D
{
    public double x,y,z,w;
    private static DecimalFormat df = new DecimalFormat("0.000");
    
    static public double RPD=2.0*Math.PI/360.0;
    
    public Point3D(){x=y=z=w=0;}
    public Point3D(Point3D a){
        x=a.x;y=a.y;z=a.z;w=a.w;
    }

    public Point3D(double _x, double _y){
        x=_x;y=_y;z=0;w=0;
    }
    public Point3D(double _x, double _y, double _z){
        x=_x;y=_y;z=_z;w=0;
    }
    public Point3D(double _x, double _y, double _z, double _w){
        x=_x;y=_y;z=_z;w=_w;
    }
    public double distance(Point3D a){
        return Math.sqrt((a.x-x)*(a.x-x)+(a.y-y)*(a.y-y)+(a.z-z)*(a.z-z));
    }
    public double dot(Point3D a){
        return a.x*x+a.y*y+a.z*z;
    }
    public Point3D cross(Point3D p){ 
        return new Point3D(y*p.z-z*p.y,z*p.x-x*p.z,x*p.y-y*p.x);
    }
    public double length(){
        return Math.sqrt(x*x+y*y+z*z);
    }
    public Point3D normalize(){
        double d=length();
        return new Point3D(x/d,y/d,z/d);
    }
    public Point3D sub(Point3D a){
        return new Point3D(x-a.x,y-a.y,z-a.z);
    }
    public Point3D add(Point3D a){
        return new Point3D(a.x+x,a.y+y,a.z+z);
    }
    public Point3D minus(){
        return new Point3D(-x,-y,-z);
    }    
    public Point3D mul(Point3D a){
        return new Point3D(a.x*x,a.y*y,a.z*z);
    }
    public Point3D div(Point3D a){
        return new Point3D(x/a.x,y/a.y,z/a.z);
    }
    public Point3D invert(){
        return new Point3D(1.0/x,1.0/y,1.0/z);
    }
    public Point3D scale(double s){
        return new Point3D(x*s,y*s,z*s);
    }

    public double planeDist(Point3D plane) {
        return x * plane.x + y * plane.y + z * plane.z + plane.w;
    }

    public Point3D plane(Point3D ref, double d){
        Point3D v=ref.sub(this);
        Point3D p=new Point3D(x+d*v.x,y+d*v.y,z+d*v.z);
        v.w=-(v.x * p.x +v.y * p.y +v.z * p.z);
        return v;
    }
    public Point3D interpolate(Point3D p1, double t){
        Point3D p=new Point3D();
        p.x = t * x + (1 - t)* p1.x;
        p.y = t * y + (1 - t)* p1.y;
        p.z = t * z + (1 - t)* p1.z;
        return p;
    }
    public boolean equals(Point3D ref){
        if(ref.x==x && ref.y==y && ref.x==z)
            return true;
        return false;
    }
    public Point3D polarToRectangular(){
        double t=RPD*x;
        double p=RPD*y;
        double f=z*Math.cos(p);
        return new Point3D(-f*Math.cos(t),z*Math.sin(p),f*Math.sin(t));
    }
    public Point3D rectangularToPolar(){
        double t,p,r;
        r=Math.sqrt(x*x+y*y+z*z);
        p=Math.asin(y/r)/RPD;
        t=Math.atan2(-z,x)/RPD;
        return new Point3D(t,p,r);
    }

    /** this conversion uses NASA standard aeroplane conventions as described on page:
     *   http://www.euclideanspace.com/maths/geometry/rotations/euler/index.htm
     *   Coordinate System: right hand
     *   Positive angle: right hand
     *   Order of euler angles: heading first, then attitude, then bank
     *   matrix row column ordering:
     *   [m00 m01 m02]
     *   [m10 m11 m12]
     *   [m20 m21 m22]*/
     public Point3D  rotate(Point3D v, double heading, double attitude, double bank) {
         // Assuming the angles are in degrees.
         double ch = Math.cos(RPD*heading);
         double sh = Math.sin(RPD*heading);
         double ca = Math.cos(RPD*attitude);
         double sa = Math.sin(RPD*attitude);
         double cb = Math.cos(RPD*bank);
         double sb = Math.sin(RPD*bank);
         double D[]=new double[9];

         D[0] = ch * ca;   D[3] = sh*sb - ch*sa*cb;   D[6] = ch*sa*sb + sh*cb;
         D[1] = sa;        D[4] = ca*cb;              D[7] = -ca*sb;
         D[2] = -sh*ca;    D[5] = sh*sa*cb + ch*sb;   D[8] = -sh*sa*sb + ch*cb;
         
         Point3D p=new Point3D();
  
         p.x=v.x*D[0]+v.y*D[3]+v.z*D[6];
         p.y=v.x*D[1]+v.y*D[4]+v.z*D[7];
         p.z=v.x*D[2]+v.y*D[5]+v.z*D[8];

         return p;
     }
     public Point3D translate(Point3D v, double x, double y, double z){
         Point3D p=new Point3D();
         p.x=v.x+x;
         p.y=v.y+y;
         p.z=v.z+z;
         return p;
     }

     public String toString(){
         if(w != 1.0)
             return new String("P("+df.format(x)+","+df.format(y)+","+df.format(z)+","+df.format(w)+")");
         else
             return new String("P("+df.format(x)+","+df.format(y)+","+df.format(z)+")");
     }

}
