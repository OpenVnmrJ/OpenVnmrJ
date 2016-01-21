/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include "ddlSymbol.h"
#include "aipSpecDataMgr.h"
#include "aipCommands.h"
#include <limits.h>
#include <float.h>

using namespace aip;

SpecDataMgr *SpecDataMgr::specDataMgr = NULL;

SpecDataMgr::SpecDataMgr() {
    specDataMap = new SpecDataMap;
    selData = NULL;
}

SpecDataMgr *SpecDataMgr::get()
{
    if (!specDataMgr) {
        specDataMgr = new SpecDataMgr();
    }
    return specDataMgr;
}

void SpecDataMgr::removeData(const char *str) {
    
   if( isdigit(str[0]) ) {
      int num = atoi(str);
      int i;
      SpecDataMap::iterator pd;
      for (i=0, pd=specDataMap->begin(); pd != specDataMap->end(); ++pd, ++i) {
        if (i == num) {
            specDataMap->erase(pd->first);
	    break;
        }
      }
      
   } else { 
      string key = string(str);
      if(key == "all") specDataMap->clear();
      else specDataMap->erase(key);
   }
}

SpecDataMap *SpecDataMgr::getSpecDataMap() {
   return specDataMap;
}

// npoints is # of inData points, npt # of outData points after decimation/expansion.
// npt is real points, npoints maybe complex (step=2)
// if step=2, use real part (optionally, may calculate phased data). 
void SpecDataMgr::calcTrace(float *inData, int npoints, int step, double scale, float *outData, int npt) {
  if(!inData || npoints<1) return;
  if(!outData || npt<1) return;
  if(npoints == npt*step) { 
      for(int i=0; i<npt; i++) outData[i] = scale*inData[i*step]; 
  } else {
      double ratio = (double)npt/(double)npoints;
      int ind, j=0, k=0, ne, nd;
      if(npoints>npt) { // decimate: a very crudely appraoch  
         float maxy=-0.1*FLT_MAX, miny=FLT_MAX;
         for(int i=0; i<npoints && j<npt; i++) {
	   ind = i*step;
           if(inData[ind] > maxy) maxy = inData[ind];
           if(inData[ind] < miny) miny = inData[ind];
           if(j == (int)(i * ratio) && j<npt) { // dx is a double smaller than 1.0 
             if(miny == maxy) outData[j]=scale*miny;
	     else outData[j] = 0.5*scale*(miny+maxy);
             j++;
             miny = FLT_MAX;
             maxy = -0.1*FLT_MAX;
           }
         }
      } else { // expand: also a very crudely appraoch
	 j=0;
         for(int i=0; i<(npoints-1) && j<npt; i++) {
	   ind = i*step;
	   outData[j] = scale*inData[ind];
           j++;
           // will use next inData point to add nd points to outData, 
           ind = (i+1)*step;
	   ne = (int)((i+1) * ratio); // ne always >= j because ratio > 1 
	   nd = ne-j;
	   if(nd == 0) continue;
           double d = (scale*inData[ind]-outData[j-1])/(double)(nd+1); 
 	   for(k=0; k<nd; k++) {
	     if(j<npt) {
		outData[j] = outData[j-1] + (k+1)*d/nd;
	        j++;
	     }
	   }
         }
	 outData[npt-1] = scale*inData[npoints-1];
      }
  }
}

// note, outData is allocated by caller. It may not be used 
// if no scaling and no decimation or expansion to srcData. 
float *SpecDataMgr::getTrace(string key, int ind, double scale, float *outData, int npt) {
   if(key == "") return NULL;
   float *srcData = NULL; 
   int step=1, npoints=0;
   if(key == "FID") {
	srcData = aip_GetFid(ind); // get first trace if ind out of range 
	step=2;
        npoints = getFidPoints();
   } else if(key == "SPEC") {
	srcData = aip_GetSpec(ind);
	step=1;
        npoints = getSpecPoints();
   } else if(key == "BASE") {
	srcData = aip_GetBaseline(ind);
	step=1;
        npoints = getSpecPoints();
   } else if(key == "FIT") {
	srcData = aip_GetFit(ind);
	step=1;
        npoints = getSpecPoints();
   } else {

      SpecDataMap::iterator pd = specDataMap->find(key);
      if (pd != specDataMap->end()) { 
         specStruct_t *ss = pd->second->getSpecStruct();
         if(ss != NULL || ss->data != NULL) {
	    srcData = pd->second->getTrace(ind, ss->matrix[0]);
            step = 1;
	    if(strcmp(ss->storage, "complex")==0) step=2;
            npoints = ss->matrix[0];
         }
      }
   }

   if(!srcData && 
	key.find('+',1) == string::npos &&
	key.find('-',1) == string::npos &&
	key.find('*',1) == string::npos &&
	key.find(':',1) == string::npos) {
	return NULL;
   } else if(!srcData) {
	return getTrace4ComboKey(key, ind, scale, npt);
   } else if(npoints == npt && step == 1 && scale == 1) {
	return srcData;
   } else {
	calcTrace(srcData, npoints, step, scale, outData, npt);
	return outData;
   } 
}

// this will be called by vnmrbg code to get trace to display.
// note, fpt and npt are for real data. So multiple 2 for complex data. 
// selData is a member variable.
float *SpecDataMgr::getTrace(string key, int ind, double scale, int npt) {
    if(selData != NULL) delete[] selData;
    selData = new float[npt];
    return getTrace(key, ind, scale, selData, npt);
}

// this only works for a few simple cases (no space in key):
// key:n, n is index (override ind)
// key*f, scale data by f
// key1+key2, add two data
// key1-key2, subtract two data
// key1 and key2 may contain ":n" or "*f"
float *SpecDataMgr::getTrace4ComboKey(string key, int ind, double scale, int npt) {
    if(key == "") return NULL;
    
    bool add=false;
    bool sub=false;
    size_t pos;
    if((pos = key.find('+',1)) != string::npos) add=true; 
    else if((pos = key.find('-',1)) != string::npos) sub=true;
    if(add || sub) {
	string key1 = key.substr(0,pos);
	string key2 = key.substr(pos+1,key.length() - (pos+1));
        int ind1=ind;
        int ind2=ind;
	double scale1=scale;
	double scale2=scale;
        if((pos = key1.find(':',1)) != string::npos) {
	   string num = key1.substr(pos+1,key1.length() - (pos+1));
	   ind1 = atoi(num.c_str())-1;
	   key1 = key1.substr(0,pos);
	}
        else if((pos = key1.find('*',1)) != string::npos) {
	   string num = key1.substr(pos+1,key1.length() - (pos+1));
	   scale1 = atof(num.c_str());
	   key1 = key1.substr(0,pos);
	}
        if((pos = key2.find(':',1)) != string::npos) {
	   string num = key2.substr(pos+1,key2.length() - (pos+1));
	   ind2 = atoi(num.c_str())-1;
	   key2 = key2.substr(0,pos);
	}
        else if((pos = key2.find('*',1)) != string::npos) {
	   string num = key2.substr(pos+1,key2.length() - (pos+1));
	   scale2 = atof(num.c_str());
	   key2 = key2.substr(0,pos);
	}
	float *trace1 = new float[npt];
	float *trace2 = new float[npt];
        float *data1 = getTrace(key1, ind1, scale1, trace1, npt);
        float *data2 = getTrace(key2, ind2, scale2, trace2, npt);
        if(!data1 && !data2) {
	  Winfoprintf("NULL data for %s and %s",key1.c_str(),key2.c_str());
	  delete[] trace1;
	  delete[] trace2;
	  return NULL;
        } else if(!data1) {
	  Winfoprintf("NULL data for %s",key1.c_str());
	  delete[] trace1;
	  delete[] trace2;
	  return data2;
	} else if(!data2) {
	  Winfoprintf("NULL data for %s",key2.c_str());
	  delete[] trace1;
	  delete[] trace2;
	  return data1;
	}
        if(selData != NULL) delete[] selData;
        selData = new float[npt];
	if(add) {
	  for(int i=0; i<npt; i++) selData[i]=data1[i]+data2[i];
	} else {
	  for(int i=0; i<npt; i++) selData[i]=data1[i]-data2[i];
	}
	   
	delete[] trace1;
	delete[] trace2;

 	return selData;
    } else { 
        int ind1=ind;
	double scale1=scale;
        string key1 = key;
        if((pos = key1.find(':',1)) != string::npos) {
	   string num = key1.substr(pos+1,key1.length() - (pos+1));
	   ind1 = atoi(num.c_str())-1;
	   key1 = key1.substr(0,pos);
	}
        else if((pos = key1.find('*',1)) != string::npos) {
	   string num = key1.substr(pos+1,key1.length() - (pos+1));
	   scale1 = atof(num.c_str());
	   key1 = key1.substr(0,pos);
	}
        if(selData != NULL) delete[] selData;
        selData = new float[npt];
	return getTrace(key1, ind1, scale1, selData, npt);
    }
}

spSpecData_t
SpecDataMgr::getDataByKey(string key) {

    if(key == "") {
         return (spSpecData_t)NULL;
    }

    SpecDataMap::iterator pd = specDataMap->find(key);
    if (pd == specDataMap->end()) {
        return (spSpecData_t)NULL;
    }
    return pd->second;
}

spSpecData_t
SpecDataMgr::getDataByNumber(int num) {
// num starts from zero
   SpecDataMap::iterator pd;
    int i;
    for (i=0, pd=specDataMap->begin(); pd != specDataMap->end(); ++pd, ++i) {
        if (i == num) {
            return pd->second;
        }
    }
    return (spSpecData_t)NULL; 
}

spSpecData_t
SpecDataMgr::getDataByType(string type, int index) {
   SpecDataMap::iterator pd;
    int i;
    int count = 0;
    string dataType;
    for (i=0, pd=specDataMap->begin(); pd != specDataMap->end(); ++pd, ++i) {
        dataType = pd->second->getDataType();
	if( dataType == type) {
	   count++;
	   if(count == index) return pd->second;
        }
    }
    return (spSpecData_t)NULL; 
}

int
SpecDataMgr::aipLoadSpec(int argc, char *argv[], int retc, char *retv[])
{
   if(argc < 2) {
      Winfoprintf("Usage: aipLoadSpec(fullpath<,key>)");
      return proc_complete;
   }

   // check path
   if(access(argv[1], R_OK) != 0) {
      Winfoprintf("%s: %s does not exist", argv[0], argv[1]);
      return proc_error;
   }

   string key;
   if(argc > 2) key = string(argv[2]);
   else {
/*
	// file name as key
        key = string(argv[1]);
	string::size_type first = key.find_last_of("/") + 1;
	string::size_type  last = key.find_last_of(".");
        key = key.substr(first,last-first);
*/
	key = SpecDataMgr::get()->getNextKey();
   }

   if(SpecDataMgr::get()->loadSpec(key, argv[1]))
     return proc_complete;
   else
     return proc_error;
}

string SpecDataMgr::getNextKey() {
	int nspec = 1 + getSpecDataMap()->size();
        char str[16];
        sprintf(str,"spec%d",nspec);
        return string(str);
}

int SpecDataMgr::loadSpec(string key, char *path) {

    DDLSymbolTable *st = ParseDDLFile(path);// Get header
    if (!st) return 0; 

    char *mallocError = st->MallocData();
    if(mallocError[0] != 0) {
	Winfoprintf("%s", mallocError);
	delete st;
        return 0;	
    }

    specStruct_t *ss = new specStruct_t;
    if(!(SpecData::fillSpecStruct(st,ss))) {
	delete st;
	delete ss;
        return 0;	
    }

    spSpecData_t specData = spSpecData_t(new SpecData(key, path, st, ss));
    if(specData == (spSpecData_t)NULL) {
	Winfoprintf("SpecData is NULL.");
	delete st;
	delete ss;
        return 0;	
    }
	
    specDataMap->erase(key);        // Delete any old version.
    specDataMap->insert(SpecDataMap::value_type(key, specData));

    return specData->getNumTraces();
}

int
SpecDataMgr::aipRemoveSpec(int argc, char *argv[], int retc, char *retv[])
{
   if(argc < 2) {
      Winfoprintf("Usage: aipRemoveSpec(key or index), index starts from 0");
      return proc_complete;
   }

   SpecDataMgr::get()->removeData(argv[1]);
      
   return proc_complete;
}

int SpecDataMgr::getYminmax(string nucleus,double &ymin, double &ymax) {
   if(specDataMap->size() <1) {
        return 0; // in this case, the caller should get ntraces from vnmrbg code. 
   }

   int n=0;
   specStruct_t *ss;
   SpecDataMap::iterator pd;
    for (pd=specDataMap->begin(); pd != specDataMap->end(); ++pd) {
        spSpecData_t sd = pd->second;
        ss = sd->getSpecStruct();
        if(ss == NULL || ss->data == NULL) continue;

        if(string(ss->nucleus[0]) != nucleus) continue; // for now only deal with 1D data       

	int npnt = ss->matrix[0];
	for(int i=1; i<(ss->rank); i++) {
	   npnt *= ss->matrix[i];
	} 
	register float  *data = ss->data;
	register float maxval = -0.1*FLT_MAX;
	register float minval = FLT_MAX;
	register float tmp;
	while (--npnt)
	{
	   tmp = *data++;
	   if (tmp > maxval) maxval = tmp;
	   if (tmp < minval) minval = tmp;
	}
	ymin = minval;	
	ymax = maxval;	
	n++;
    }
    return n;
}

// get total number of traces
int SpecDataMgr::getNtraces(string nucleus) {
   if(specDataMap->size() <1) {
        return 0; // in this case, the caller should get ntraces from vnmrbg code. 
   }

   int n=0;
   specStruct_t *ss;
   SpecDataMap::iterator pd;
    for (pd=specDataMap->begin(); pd != specDataMap->end(); ++pd) {
	spSpecData_t sd = pd->second;
        ss = sd->getSpecStruct();
	if(ss == NULL || ss->data == NULL) continue;

	if(string(ss->nucleus[0]) != nucleus) continue; // for now only deal with 1D data	
        
	int i=1;
	while(i < (ss->rank)) {
	   n += ss->matrix[i];
	   i++;
	} 
    }
    return n; 
}

// Note, totalInd is for traces of given nucleus.
// minX is rfp-rfl, maxX is minX+sw, all in ppm.
// if key is not empty, only return npts,minX,maxX
// otherwise figure out key and traceInd based on totalInd.
void SpecDataMgr::getTraceInfo(int totalInd, string &key, string &path, int &traceInd, int &npts, double &minX, double &maxX, string nucleus) {

   npts=0;
   minX=0;
   maxX=0;
   // from current vnmrbg data
   if(specDataMap->size() <1 || key == "SPEC") {
      key="SPEC";
      if(totalInd >=0) {
	traceInd = totalInd;
      }
      npts = getSpecPoints();
      double ref = getReal("reffrq",1.0);
      minX = (getReal("rfp",0.0) - getReal("rfl",0.0))/ref; 
      maxX = getReal("sw",0.0)/ref + minX; 
      return;
   }

   // get npts,minX,maxX for specified key
   specStruct_t *ss;
   SpecDataMap::iterator pd;
   if(key != "") {
     for (pd=specDataMap->begin(); pd != specDataMap->end(); ++pd) {
        spSpecData_t sd = pd->second;
        ss = sd->getSpecStruct();
        if(ss == NULL || ss->data == NULL) continue;
	if(key == pd->first) {
           path = sd->getPath(); 
           npts = ss->matrix[0];
	   minX = ss->upfield[0]/ss->reffrq[0]; 
	   maxX = ss->sw[0]/ss->reffrq[0] + minX;
	   return;
	}
      }
      return;
   }
 
   // get info based on totalInd for traces of given nucleus. 
   int ind=0;
   for (pd=specDataMap->begin(); pd != specDataMap->end(); ++pd) {
        spSpecData_t sd = pd->second;
        ss = sd->getSpecStruct();
        if(ss == NULL || ss->data == NULL) continue;
	
	if(string(ss->nucleus[0]) != nucleus) continue; // for now only deal with 1D data	

	int i=1, ntraces=0;
	while(i < (ss->rank)) {
	   ntraces += ss->matrix[i];
	   i++;
	} 
	for(i=0; i<ntraces; i++) {	
	   if(ind==totalInd) {
		key=pd->first;
                path = sd->getPath(); 
		traceInd = i;
           	npts = ss->matrix[0];
	   	minX = ss->upfield[0]/ss->reffrq[0]; 
	   	maxX = ss->sw[0]/ss->reffrq[0] + minX;
		return; 
	   }
	   ind++;
	}
   }
}

