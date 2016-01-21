/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * CGLDataMgr.h
 *
 *  Created on: Jan 17, 2010
 *      Author: deans
 */

#ifndef CGLDATAMGR_H_
#define CGLDATAMGR_H_
#include <string.h>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include "CGLDef.h"
#include "Point3D.h"
class CGLDataMgr
{
protected:
    float *vertexData;
    float *mapData;

    char mapfile[256];
public:
    float ymin,ymax;
    double pa, pb,cosd, sind;
    double rp,lp;
    bool complex;
    bool absval;
    bool newDataGeometry;
    int step;
    int ws;
    int dim;
    int data_size;
    int data_type;
    int np,traces,trace,slice,slices;
    int projection;
    int sliceplane;
    bool allpoints;
    bool real,imag;

    CGLDataMgr();
    virtual void setDataPars(int n, int t, int s, int dtype){
        if(np!=n || traces !=t || slices !=s)
            newDataGeometry=true;
        mapData=0;
        np=n;
        traces=t;
        slices=s;
        data_type=dtype;
        dim=data_type&DIM;
        complex=((dtype&COMPLEX)>0)?true:false;
        ws = complex ? 2 : 1;
        data_size=ws*np*traces*slices;
        setReal();
    }
    virtual void setDataSource(){
        int fd=0;
        if(vertexData==0 && mapData==0){
			if(strlen(mapfile)>0)
				fd = open(mapfile, O_RDONLY, 0666 );
			if(fd>0){
				mapData = (float*)mmap((void*)0, data_size*4, PROT_READ, MAP_SHARED, fd, 0);
				close(fd);
			}
			else
				std::cout << "CGLRenderer::setDataSource ERROR opening map file" << std::endl;
		}
    }
    bool dataValid(){
    	return ((vertexData==NULL) && (mapData==0))?false:true;
    }
    void setDataMap(char *path){
        strncpy(mapfile,path,255);
    }
    double vertexValue(int adrs){
        return ((vertexData==0)?mapData[adrs]:vertexData[adrs]);
    }
    double realValue(int adrs){
        return ((vertexData==0)?mapData[adrs]:vertexData[adrs]);
    }
    double imagValue(int adrs){
        return ((vertexData==0)?mapData[adrs+np]:vertexData[adrs+np]);
    }
    Point3D getPoint(int k, int j, int i);
    double vertexValue(int k, int j, int i);
    void initPhaseCoeffs(int start);
    void incrPhaseCoeffs();
    void setReal(){
    	 real=true;
    	 imag=false;
     }
     void setImag(){
    	 imag=true;
    	 real=false;
     }


};
#endif /* CGLDATAMGR_H_ */
