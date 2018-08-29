/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

public class Volume implements JGLDef
{
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
    public Point3D[][] facePoints = new Point3D[6][];
    static final int[][]     edges = { // edges of the cube 
        /* 0  */{ 0, 1},
        /* 1  */{ 1, 2},
        /* 2  */{ 2, 3},
        /* 3  */{ 3, 0},
        /* 4  */{ 0, 4},
        /* 5  */{ 1, 5},
        /* 6  */{ 2, 6},
        /* 7  */{ 3, 7},
        /* 8  */{ 4, 5},
        /* 9  */{ 5, 6},
        /* 10 */{ 6, 7},
        /* 11 */{ 7, 4}};

    static final int[][]     faces = { // edges of faces of cube
        { 0, 1, 2, 3},
        { 1, 6, 9, 5},
        { 6, 2, 7, 10},
        { 7, 3, 4, 11},
        { 0, 5, 8, 4},
        { 9, 8, 10, 11}};

    
    // The 3D space limits for the volume
    Point3D minVolume = null;
    Point3D maxVolume = null;
    Point3D minCoord = null;
    Point3D maxCoord = null;
    Point3D cpoint;
    Point3D scale=new Point3D(1,1,1);
    public Point3D bounds[]=new Point3D[8];

    public Volume(Point3D min, Point3D max) {
        minVolume=min;
        maxVolume=max;
              
        for(int i = 0; i < 8; i++) {
            bounds[i] = new Point3D();
        }
        for(int i = 0; i < 6; i++) {
            facePoints[i] = new Point3D[4];
        }
        setScale(scale);
        
        // setup the VOI box points

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

    }
    public void setScale(Point3D s) {
    	scale=s;
    	minCoord=minVolume.mul(scale);
    	maxCoord=maxVolume.mul(scale);
    	
        bounds[0].x = bounds[1].x = bounds[2].x = bounds[3].x = minCoord.x;
        bounds[4].x = bounds[5].x = bounds[6].x = bounds[7].x = maxCoord.x;
        bounds[0].y = bounds[1].y = bounds[4].y = bounds[5].y = minCoord.y;
        bounds[2].y = bounds[3].y = bounds[6].y = bounds[7].y = maxCoord.y;
        bounds[0].z = bounds[3].z = bounds[4].z = bounds[7].z = minCoord.z;
        bounds[1].z = bounds[2].z = bounds[5].z = bounds[6].z = maxCoord.z;

        Point3D pt=maxCoord.add(minCoord);
        cpoint=pt.scale(0.5);
    }
  
    public Point3D[] getFace(int face){
        Point3D[] pts=new Point3D[4];
        
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
            break;
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
        return pts;
    }
    public Point3D faceCenter(int face){
        Point3D[] pts=getFace(face);
        Point3D fc=new Point3D();
        for(int i=0;i<4;i++){
            fc=fc.add(pts[i]);
        }
        fc=fc.scale(1.0/4);
        return fc;
    }

    public Point3D faceNormal(int face){
        Point3D[] pts=getFace(face);
        Point3D p1=pts[3].sub(pts[0]);
        Point3D p2=pts[1].sub(pts[0]);
        Point3D p3;
        switch(face){
        case PLUS_Y:
        case PLUS_Z:
            p3=p2.cross(p1);
            break;
        default:
            p3=p1.cross(p2);   
        }
        return p3;
    }
    
    public Point3D center(){
        return cpoint;
    }
    public Point3D Xmax(){
        return new Point3D(maxCoord.x,cpoint.y,cpoint.z);
    }   
    public Point3D Xmin(){
        return new Point3D(minCoord.x,cpoint.y,cpoint.z);
    }
    public Point3D Ymax(){
        return new Point3D(cpoint.x,maxCoord.y,cpoint.z);
    }   
    public Point3D Ymin(){
        return new Point3D(cpoint.x,minCoord.y,cpoint.z);
    }
    public Point3D Zmax(){
        return new Point3D(cpoint.x,cpoint.y,maxCoord.z);
    }   
    public Point3D Zmin(){
        return new Point3D(cpoint.x,cpoint.y,minCoord.z);
    }
    /** Return the slice normal vector
     */
    public Point3D XPlane(){
        return Xmax().sub(center());
    }
    /** Return the slice tangent vector
     */
    public Point3D YPlane(){
        return Ymax().sub(center());
    }
    /** Return the slice cross-product vector
     */
    public Point3D ZPlane(){
        return Zmax().sub(center());
    }

    public double planeDist(Point3D pt, Point3D plane) {
        return pt.x * plane.x + pt.y * plane.y + pt.z * plane.z + plane.w;
    }

    private void exchangePts(Point3D[] edge) {
        Point3D temp = edge[0];
        edge[0] = edge[1];
        edge[1] = temp;
    }

    private void exchangeEdges(Point3D[] edge0, Point3D[] edge1) {
        Point3D temp0 = edge0[0];
        Point3D temp1 = edge0[1];
        edge0[0] = edge1[0];
        edge0[1] = edge1[1];
        edge1[0] = temp0;
        edge1[1] = temp1;
    }

    /** Return the maximum distance of bounding box vertex to plane
     */
    double minPlane(Point3D plane) {
        double max=-1e6;
        for(int i=0;i<8;i++){
            double d=planeDist(bounds[i],plane);
            max=d>max?d:max;
        }
        return max;
    }

    /** Return the minimum distance of bounding box vertex to plane
     */
    double maxPlane(Point3D plane) {
        double min=1e6;
        for(int i=0;i<8;i++){
            double d=planeDist(bounds[i],plane);
            min=d<min?d:min;
        }
        return min;
    }

    /** Return minimum vertex distance to a Point.
     */
    public double minDistance(Point3D pt) {
        Point3D pc=center();
        double l=pt.distance(pc);
        Point3D plane=pc.plane(pt, 0);
        return minPlane(plane)/l;
    }

    /** Return maximum vertex distance to a Point.
     */
    public double maxDistance(Point3D pt) {
        Point3D pc=center();
        double l=pt.distance(pc);
        Point3D plane=pc.plane(pt, 0);
        return maxPlane(plane)/l;
    }

    /** Return a point on a line passing through the input point and center
     *  that is at the depth of the closest vertex.
     */
    public Point3D minPoint(Point3D pt) {
        double d=minDistance(pt);
        Point3D v=pt.sub(center());
        v=v.normalize();
        Point3D sv=v.scale(d);
        pt=center().add(sv);
        return pt;
    }

    /** Return a point on a line passing through the input point and center
     *  that is at the depth of the farthest vertex.
     */
    public Point3D maxPoint(Point3D pt) {
        double d=maxDistance(pt);
        Point3D v=pt.sub(center());
        v=v.normalize();
        Point3D sv=v.scale(d);
        pt=center().add(sv);
        return pt;
    }

    /** Return a variable length array of points, sorted in clockwise order,
     *  that consist of all intersections of the line segments of the bounds 
     *  box with a plane in object space.
     */
    public Point3D[] boundsPts(Point3D plane) {
    	Point3D[] points = bounds;
        int numOutEdges = 0;
        Point3D[][] outEdges = new Point3D[6][2];
        Point3D wantVtx;
        double[] ptDist = new double[8]; // dist of pt from plane
        boolean[] edgeFlags = new boolean[12]; // edge cut by plane?
        Point3D[] edgeInts = new Point3D[12]; // intersection in edge
        // determine the distance of each point from the plane
        for(int i = 0; i < 8; i++) {
            ptDist[i] = planeDist(points[i], plane);
        }

        // scan each edge, mark the ones that are cut and calc the intersection
        for(int i = 0; i < 12; i++) {
            double dst0 = ptDist[edges[i][0]];
            double dst1 = ptDist[edges[i][1]];
            if((dst0 > 0) ^ (dst1 > 0)) {
                edgeFlags[i] = true;
                double t = dst1 / (dst1 - dst0);
                edgeInts[i] = new Point3D();
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
            boolean anyCut = (edgeFlags[faces[i][0]] | edgeFlags[faces[i][1]]
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
                        exchangePts(outEdges[j]);
                    }
                    if(j != (i + 1)) {
                        exchangeEdges(outEdges[i + 1], outEdges[j]);
                    }
                }
            }
        }

        Point3D[] returnPts = new Point3D[numOutEdges];
        for(int i = 0; i < numOutEdges; i++) {
            returnPts[i] = outEdges[i][0];
        }

        return returnPts;
    }

 
    /** Return an array of points from the intersection of a plane
     *  with an array of points that are assumed to be on a second plane.
     * @param plane         intersection plane (plane point)
     * @param points        input plane (array of points)
     * @param infront       if true, input array points in front of the 
     *                      intersection plane are kept, otherwise input
     *                      array point behind the intersection plane are kept
     * @return
     */
    public Point3D[] intersect(Point3D plane, Point3D[] points, boolean infront) {
        Point3D[] tmp = new Point3D[16];
        int numOutPoints = 0;
        int i;
        for(i = 0; i < points.length; i++) {
            Point3D p0=points[i];
            Point3D p1;
            if(i==(points.length-1))
                p1=points[0];
            else
                p1=points[i+1];
            double d0 = planeDist(p0, plane);
            double d1 = planeDist(p1, plane);
            if(infront){
                d0=-d0;
                d1=-d1;
            }           
            if(d0<=0)
                tmp[numOutPoints++] = new Point3D(p0);
            if((d0 > 0) ^ (d1 > 0)) { // "^" = exclusive or
                double t = d1 / (d1 - d0);
                tmp[numOutPoints++]=p0.interpolate(p1, t);
            }
        }
        Point3D[] outPoints=new Point3D[numOutPoints];
        for(i=0;i<numOutPoints;i++)
            outPoints[i]=tmp[i];
        return outPoints;        
    }

   /** Return an array of points from the intersection of a plane
     *  in object space with the union of the boundary box and a 
     *  front and back slice plane.
     * @param plane          plane along the eye vector
     * @param front          front plane along the slice vector
     * @param back           back plane along the slice vector
     * @return               array of points (size=0 if plane is completely clipped)
     */
    public Point3D[] slicePts(Point3D plane, Point3D front, Point3D back) {
        Point3D[] outPoints = new Point3D[16];
        
        Point3D[] points = boundsPts(plane);
        
        outPoints=intersect(front,points,false);
        outPoints=intersect(back,outPoints,true);

        int numOutPoints = 0;
        Point3D[][] outEdges;
        Point3D wantVtx;
        numOutPoints=outPoints.length;
        outEdges = new Point3D[numOutPoints][2];
        for(int i = 0; i < numOutPoints; i++) {
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
                        exchangePts(outEdges[j]);
                    }
                    if(j != (i + 1)) {
                        exchangeEdges(outEdges[i + 1], outEdges[j]);
                    }
                }
            }
        }
        Point3D[] returnPts = new Point3D[numOutPoints];
        for(int i = 0; i < numOutPoints; i++) {
            returnPts[i] = outEdges[i][0];
        }
        return returnPts;
    }
}

