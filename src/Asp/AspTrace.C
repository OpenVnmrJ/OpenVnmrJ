/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "AspTrace.h"
#include "graphics.h"
#include "aipSpecData.h"
#include "aipSpecDataMgr.h"

extern "C" {
int getSpecPoints();
int writeFDFSpecfile_spectrum(char *fdfpath, int msg);
int colorindex(char *colorname, int *index);
}
extern "C" int nblocks;
extern "C" int specperblock;

spAspTrace_t nullAspTrace = spAspTrace_t(NULL);

AspTrace::AspTrace(int traceIndex, string key, string fpath, int index, int np, double min, double max) {

  initTrace();

  dataKey = key;
  path = fpath;
  dataInd = index;
  mminX = minX = input_minX = min;
  mmaxX = maxX = input_maxX = max;
  npts = np;
  first=0;
  last=np-1;
  traceInd = traceIndex;
  cmd="load";
  rootPath="";
  shift=0.0;
  dcMargin=0.0;
}

AspTrace::AspTrace(char *str) {
      char words[MAXWORDNUM][MAXSTR2], *tokptr;
      tokptr = strtok(str, ", \n");
      int nw=0;
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, ", \n");
      }


      initTrace();
      makeTrace(words,nw);
}

AspTrace::AspTrace(char words[MAXWORDNUM][MAXSTR2], int nw) {
      initTrace();
      makeTrace(words,nw);
}

void AspTrace::makeTrace(char words[MAXWORDNUM][MAXSTR2], int nw) {

      if(nw < 12) return;

        //e.g. load 1 /tmp/spec.fdf spec1 spec1 0 -2.022703 14.022636 1.000000 0.000000 11 0
        //e.g. load 2 /tmp/spec.fdf spec1 spec1 1 -2.022703 14.022636 1.000000 0.000000 11 0
        // each line corresponds to a AspTrace  
	cmd=string(words[0]); // such as "load, "add" or "sub", or "load_dc",...
        traceInd=atoi(words[1])-1;
        path = string(words[2]);
        int count = 3;
   	if(nw>count) {
         int ln = strlen(words[count]);
         if(words[count][0]=='|' && words[count][ln-1]=='|') {
           string str = string(words[count]); count++;
           label=str.substr(1,str.find_last_of("|")-1);
         } else if(words[count][0]=='|') {
           string str = string(words[count]); count++;
           bool endW = false;
           while(!endW && nw>count) {
                endW = (strstr(words[count],"|") != NULL);
                str += " ";
                str += string(words[count]); count++;
           }
	   if(endW) label=str.substr(1,str.find_last_of("|")-1);
	   else label=str.substr(1,str.find_last_of("|"));
         } else {
           label=string(words[count]); count++; 
         }
   	}
   	if(nw>count) { dataKey = string(words[count]); count++; }
        if(nw>count) { dataInd =atoi(words[count])-1; count++; }
        if(nw>count) { minX = atof(words[count]); count++; }
        if(nw>count) { maxX = atof(words[count]); count++; }
        if(nw>count) { scale = atof(words[count]); count++; }
        if(nw>count) { vp = atof(words[count]); count++; }
        if(nw>count) { colorStr =string(words[count]); count++; }
        if(nw>count) { labelFlag =atoi(words[count]); count++; }
        if(nw>count) { shift = atof(words[count]); count++; }
        if(nw>count) { dcMargin = atof(words[count]); count++; }

  	if(dataInd < 0) dataInd = 0;

        // make sure fdf data for dataKey is loaded
    int maxind = 0;
    if(dataKey != "SPEC") {
        spSpecData_t sd = SpecDataMgr::get()->getDataByKey(dataKey);
        if(sd == (spSpecData_t)NULL) {
           if(!SpecDataMgr::get()->loadSpec(dataKey, words[2]))
           {
             Winfoprintf("Error loading %s",words[2]);
             return;
           }
        }

        sd = SpecDataMgr::get()->getDataByKey(dataKey);
        specStruct_t *ss = NULL;
        if(sd != (spSpecData_t)NULL) ss = sd->getSpecStruct();
        if(ss == NULL) {
           Winfoprintf("Error loading %s",words[2]);
           return;
        }

        npts = ss->matrix[0];
        mminX = (ss->upfield[0]/ss->reffrq[0]);
        mmaxX = (ss->sw[0]/ss->reffrq[0]) + mminX;

        maxind = ss->matrix[1];
    } else {
	npts=getSpecPoints();
	mminX=minX;
	mmaxX=maxX;
        maxind = nblocks * specperblock;
	if(maxind<1) maxind=1;
    }

    if(dataInd >= maxind) {
	Winfoprintf("Warning: data %s index %d out bound %d. Set to %d.",dataKey.c_str(),dataInd,maxind,maxind); 
	dataInd = maxind-1;
    }
    if(minX < mminX) minX = mminX;
    if(maxX > mmaxX) maxX = mmaxX;

	setShift(shift);
        setMinX(minX); // calculate last point 
        setMaxX(maxX); // calculate first point

	if(strstr(words[0],"_dc") != NULL) dodc();

}

AspTrace::~AspTrace() {
   delete sumTraceList;
   if(sumData) delete[] sumData;
   sumData=NULL;
   if(bcModel) delete[] bcModel;
   bcModel=NULL;
}

void AspTrace::initTrace() {

  	dataKey = "";
  	path = "";
  	rootPath = "";
  	dataInd = 0;
  	sumData = NULL;
  	bcModel = NULL;
  	mminX = minX = input_minX = 0.0;
  	mmaxX = maxX = input_maxX = 0.0;
  	npts = 0;
  	selected = false;
  	vp=0.0;
	scale = 1.0;
  	label="";
  	labelFlag=0;
  	first=0;
  	last=0;
  	traceInd = 0;
 	sumTraceList = new list<spAspTrace_t>;
	cmd="";
	shift=0.0;
	dcMargin=0.0;
        doneBC=false;
        colorStr="";
	return;
}

string AspTrace::getKeyInd() {
  char str[MAXSTR];
  sprintf(str,"%s:%d",dataKey.c_str(),dataInd);
  return string(str);   
}

string AspTrace::getIndex() {
  char str[MAXSTR];
  sprintf(str,"%d",traceInd+1);
  return string(str);   
}

string AspTrace::getLabel(spAspDataInfo_t dataInfo) {

  string newStr = "";
  if(labelFlag & SPEC_INDEX) newStr = getIndex();

  if((labelFlag & SPEC_LABEL) && label != "") {
    if(label != "") newStr = newStr+" "+ label;
  
  } else if((labelFlag & SPEC_LABEL)) {
    char str[MAXSTR];
    spSpecData_t sd = SpecDataMgr::get()->getDataByKey(dataKey);
    if(sd != (spSpecData_t)NULL) {
	int ntraces = sd->getNumTraces();
	string lb = sd->getLabel();
	if(lb == "") lb = dataKey;
	if(ntraces > 1) sprintf(str,"%s_%d",lb.c_str(),dataInd+1);
	else sprintf(str,"%s",lb.c_str());
    } else {
	int ntraces = dataInfo->vaxis.npts; 
	if(ntraces > 1) sprintf(str,"%d",dataInd+1);
	else sprintf(str,"%s",path.c_str());
    }
    newStr = newStr+" "+string(str); 
  }
  
  return newStr;
}

float AspTrace::getAmp(double freq) {
   int pos = val2dpt(freq);
   float *data = getFirstDataPtr();
   if(!data) return 0;
   else return *(data + pos);
}

int AspTrace::setColor() {
   if(colorStr=="ACTIVE_COLOR") return ACTIVE_COLOR;
   else if(colorStr=="") return SPEC_COLOR;
   else {
      int color;
      if(colorindex((char *)colorStr.c_str(), &color)) return color;
      else { 
      	set_anno_color((char *)colorStr.c_str());
        return -1;
      } 
   }
}

string AspTrace::getColor() {
   if(selected) return "ACTIVE_COLOR";
   else return colorStr;
}

double AspTrace::getMminX() {
  return mminX;
}

double AspTrace::getMmaxX() {
  return mmaxX;
}

int AspTrace::getTotalpts() {
  return npts;
}

// scale parameter is not used here, it will be used in AspCell.
float *AspTrace::getFirstDataPtr() {
  if(npts<1) return NULL;
  else if(sumData != NULL) return sumData;
  else if(sumTraceList->size() > 0) {
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   for (itr=sumTraceList->begin(); itr != sumTraceList->end(); ++itr) {
        trace = (*itr);
 	if(trace->cmd.find("sub") != string::npos) sumTrace(*itr,SUB_TRACE);
 	else if(trace->cmd.find("rep") != string::npos) sumTrace(*itr,REP_TRACE);
	else if(trace->cmd.find("add") != string::npos) sumTrace(*itr,ADD_TRACE);
   }
  }
  if(sumTraceList->size() > 0 && sumData != NULL) return sumData;
  else return SpecDataMgr::get()->getTrace(dataKey, dataInd, 1.0, npts);
}

double AspTrace::getMinX() {
  return minX;
}

double AspTrace::getMaxX() {
  return maxX;
}

int AspTrace::getNpts() {
  return last-first+1;
}

// scale parameter is not used here, it will be used in AspCell.
float *AspTrace::getData() {
  if(npts<1) return NULL;
  else if(sumData != NULL) return sumData + first;
  else if(sumTraceList->size() > 0) {
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   for (itr=sumTraceList->begin(); itr != sumTraceList->end(); ++itr) {
        trace = (*itr);
 	if(trace->cmd.find("sub") != string::npos) sumTrace(*itr,SUB_TRACE);
 	else if(trace->cmd.find("rep") != string::npos) sumTrace(*itr,REP_TRACE);
	else if(trace->cmd.find("add") != string::npos) sumTrace(*itr,ADD_TRACE);
   }
  }
  if(sumTraceList->size() > 0 && sumData != NULL) return sumData + first;
  else {
     float *data = SpecDataMgr::get()->getTrace(dataKey, dataInd, 1.0, npts);
     if(!data) return NULL;
     else return data + first;
  }
}

void AspTrace::setMinX(double val) {
   minX=val;
   last=(int)(npts*(mmaxX-minX)/(mmaxX-mminX)) - 1;
   if(last>(npts-1)) last = npts-1;
}

void AspTrace::setMaxX(double val) {
   maxX=val;
   first=(int)(npts*(mmaxX-maxX)/(mmaxX-mminX));
   if(first>last) first=last-1;
   if(first<0) first=0;
}

string AspTrace::toString(string root) {
   if(root == "" || root != rootPath) return toString();

   // use relative path
   string rpath;
   if(root.find_last_of("/") == root.length()-1) 
	rpath = path.substr(root.length());
   else rpath = path.substr(root.length()+1);

   char str[MAXSTR2];
   string labelStr;
   if(label=="") labelStr = dataKey;
   else labelStr = label;
   double min,max,vscale;
   if(sumData) {
	min = input_minX;
	max = input_maxX;
	vscale = input_scale;
   } else {
	min = minX;
	max = maxX;
	vscale = scale;
   }
   sprintf(str,"%s %d %s %s %s %d %.4f %.4f %.4f %.4f %s %d %.4f %.4f",
	cmd.c_str(),traceInd+1,rpath.c_str(),labelStr.c_str(),dataKey.c_str(),
	dataInd+1,min,max,vscale,vp,colorStr.c_str(),labelFlag,shift,dcMargin);
   return string(str);
}

string AspTrace::toString() {
   char str[MAXSTR2];
   string labelStr;
   if(label=="") labelStr = dataKey;
   else labelStr = label;
   double min,max,vscale;
   if(sumData) {
	min = input_minX;
	max = input_maxX;
	vscale = input_scale;
   } else {
	min = minX;
	max = maxX;
	vscale = scale;
   }
   sprintf(str,"%s %d %s %s %s %d %.4f %.4f %.4f %.4f %s %d %.4f,%.4f",
	cmd.c_str(),traceInd+1,path.c_str(),labelStr.c_str(),dataKey.c_str(),
	dataInd+1,min,max,vscale,vp,colorStr.c_str(),labelFlag,shift,dcMargin);
   return string(str);
}

void AspTrace::sumTrace(spAspTrace_t trace, int flag) {
   
   if(trace->getNpts() < 2) {
        Winfoprintf("Error adding trace %s: %d data points.",trace->getKeyInd().c_str(), trace->getNpts());
        return;
   }

   trace->traceInd = traceInd;

   if(!sumData) {
      sumData = new float[npts]; 
      float *data = SpecDataMgr::get()->getTrace(dataKey, dataInd, 1.0, npts);
      if(!data) {
	Winfoprintf("Cannot add traces: null data %s",dataKey.c_str());
	return;
      }
      memcpy(sumData,data,npts*sizeof(float));
      for(int i=0; i<first-1; i++) *(sumData+i) = 0.0;	
      for(int i=first; i<last; i++) *(sumData+i) *= scale;	
      for(int i=last+1; i<npts; i++) *(sumData+i) *= 0.0;	
      // save these because they may be modified
      input_minX=minX;
      input_maxX=maxX;
      input_scale = scale;
      scale = 1.0;
   } 
   
   int n1 = getNpts();
   int n2 = trace->getNpts();
   if(n1<1 or n2<1) {
	Winfoprintf("Cannot add traces: zero trace points %d %d",n1,n2);
	return;
   }

   double eps = 1.0e-4;
   // make sure the same resolution 
   int npts1 = npts;
   int npts2 = trace->getTotalpts();
   double shift2=trace->getShift();
   double mminX2=trace->getMminX();
   double mmaxX2=trace->getMmaxX();
   double minX2 = trace->getMinX() + shift2;
   double maxX2 = trace->getMaxX() + shift2;
   double ppp=getPPP(); // ppm per point 
   double ppp2=trace->getPPP();
   if(ppp == 0 || ppp2 == 0) {
	Winfoprintf("Cannot add traces: zero data point.");
	return;
   }

   int first2=trace->getFirst();
   int last2=trace->getLast();
   // recalculate first, last for new trace if needed 
   if((maxX2 - mmaxX) > eps) {
	first2=(int)(npts2*(mmaxX2-mmaxX+shift2)/(mmaxX2-mminX2)); 
	maxX2=mmaxX;
   	if((first2+n2+1) > npts2) {
	   last2 = npts2-1; 
	   minX2 = mminX2+shift2;
	}
   }
   if((minX2 - mminX) < -eps) {
        last2=(int)(npts2*(mmaxX2-mminX+shift2)/(mmaxX2-mminX2)) - 1;
	minX2=mminX;
   }

   if(first2 <0 || first2 >= npts2 || last2 <= first2 || last2 >= npts2) {
	Winfoprintf("Cannot add traces: new data do not overlay %d %d %d",first2,last2,npts2);
	return; 
   }

   int first1=(int)(npts1*(mmaxX-maxX2)/(mmaxX-mminX)); 
   int last1=(int)(npts1*(mmaxX-minX2)/(mmaxX-mminX)) - 1;
   if(first1 <0 || first1 >= npts1 || last1 <= first1 || last1 >= npts1) {
	Winfoprintf("Cannot add traces: data does not overlay %d %d %d",first1,last1,npts1);
	return; 
   }

   npts1=1+(int)((maxX2-minX2)/ppp);
   npts2=1+(int)((maxX2-minX2)/ppp2);
   
   if(npts2<1 || npts1<1) return;

   if(minX2<minX) setMinX(minX2);
   if(maxX2>maxX) setMaxX(maxX2);

   trace->setMinX(minX2-shift2);
   trace->setMaxX(maxX2-shift2);

   double vscale = trace->scale;
   if(flag == SUB_TRACE) vscale = -vscale;

   sumTraceList->push_back(trace);

   float *data = getFirstDataPtr() + first1;
   float *data2 = trace->getFirstDataPtr() + first2;
   if(!data || !data2) {
	Winfoprintf("Error: sum of invalid data.");
	return;
   }
   //if(npts2==npts1) {
   if(ppp2==ppp && flag == REP_TRACE) {
     double off = *(data) - *(data2);
     for(int i=0; i<=(last2-first2); i++) *(data+i) = (*(data2+i))*vscale + off;	
   } else if(ppp2==ppp) {
     for(int i=0; i<=(last2-first2); i++) *(data+i) += (*(data2+i))*vscale;	
   } else if(flag == REP_TRACE) {
	double ratio = (double)npts2/(double)npts1;
        double off = *(data) - *(data2);
	int j;
        for(int i=0; i<npts1; i++) {
	   j=(int)(i*ratio);
	   if(j<npts2) *(data+i) = (*(data2+j))*vscale + off; 
	}
   } else { 
	double ratio = (double)npts2/(double)npts1;
	int j;
        for(int i=0; i<npts1; i++) {
	   j=(int)(i*ratio);
	   if(j<npts2) *(data+i) += (*(data2+j))*vscale; 
	}
   }
}

void AspTrace::dodc() {

   if(!sumData) {
      sumData = new float[npts]; 
      float *data = SpecDataMgr::get()->getTrace(dataKey, dataInd, 1.0, npts);
      if(!data) {
	Winfoprintf("Cannot do DC: null data %s",dataKey.c_str());
	return;
      }
      memcpy(sumData,data,npts*sizeof(float));
      for(int i=0; i<first-1; i++) *(sumData+i) = 0.0;	
      for(int i=first; i<last; i++) *(sumData+i) *= scale;	
      for(int i=last+1; i<npts; i++) *(sumData+i) *= 0.0;	
      // save these because they may be modified
      input_minX=minX;
      input_maxX=maxX;
      input_scale = scale;
      scale = 1.0;
   }

   double saveminX = minX;
   double savemaxX = maxX;

   minX -= dcMargin;
   maxX += dcMargin;
   if(minX < mminX) minX=mminX;
   if(maxX < mmaxX) maxX=mmaxX;

   float *data = getData();
   int np = getNpts();
   if(np<1) {
	Winfoprintf("Cannot do dc: zero trace points %d",np);
	return;
   }

   float sum=0.0;
   for(int i=0; i<(int)(np*0.05); i++) {
        sum += (*(data+i))*scale;
   }
   for(int i=(int)(np*0.95); i<np; i++) {
        sum += (*(data+i))*scale;
   }
   sum /= (0.1*np);
//Winfoprintf("dc %d %f",np,sum);

   for(int i=0; i<np; i++) {
	*(data+i) -= sum;
   }

   minX=saveminX;
   maxX=savemaxX;
}

bool AspTrace::ismultiColor() {
   if(sumTraceList->size() < 1) return false;
   list<spAspTrace_t>::iterator itr;
   for (itr=sumTraceList->begin(); itr != sumTraceList->end(); ++itr) {
        if((*itr)->colorStr != colorStr) return true;
   }
   return false;
}

list<spAspTrace_t> *AspTrace::getSumTraceList() {
   if(sumTraceList->size()<2) return sumTraceList;

   // sort list
   double min;
   list<spAspTrace_t>::iterator itr;
   list<spAspTrace_t>::iterator itrmin;
   list<spAspTrace_t> *tmp = new list<spAspTrace_t>;
   while(sumTraceList->size()>0) {
     min = mmaxX;
     itrmin = sumTraceList->begin();
     for (itr=sumTraceList->begin(); itr != sumTraceList->end(); ++itr) {
	if((*itr)->minX < min) {
	   min = (*itr)->minX;
	   itrmin=itr;
	}
     }
     tmp->push_back(*itrmin);
     sumTraceList->erase(itrmin);
   }
   for (itr=tmp->begin(); itr != tmp->end(); ++itr) {
	sumTraceList->push_back(*itr);
   }
   delete tmp;
   return sumTraceList;
}

double AspTrace::getPPP() {
   return (mmaxX-mminX)/(double)(npts-1); // ppm per point
}

void AspTrace::setShift(double s) {
    shift = s;
}

int AspTrace::val2dpt(double freq) {
   if(freq>maxX || freq<minX) return 0;
   double d = (double)(mmaxX - freq)/(double) (mmaxX-mminX);
   int pos = (int) (npts * d);
   if(pos > (npts-1)) pos=npts-1;
   if(pos<0) pos=0;
   return pos;
}

double AspTrace::dpt2val(int dpt) {
   if(dpt<0) dpt=0;
   if(dpt>(npts-1)) dpt=npts-1;

   return mmaxX - dpt*(mmaxX-mminX)/npts; 
}

double AspTrace::getInteg(double freq1, double freq2) {
   float *data = getFirstDataPtr();
   int np = getNpts();
   if(np<1) {
	Winfoprintf("Cannot do dc: zero trace points %d",np);
	return 0.0;
   }

   int p1 = val2dpt(freq1);
   int p2 = val2dpt(freq2);
   double sum = 0.0;
   for(int i=p1;i<=p2;i++) sum += (*(data+i));

   return (sum - *(data+p1));
}

void AspTrace::save(string dir) {
    char str[MAXSTR];
    rootPath=string(dir);
    sprintf(str,"%s/%s.fdf",dir.c_str(),dataKey.c_str());
    path = string(str);

    struct stat fstat;
    if (stat(str, &fstat) == 0) unlink(str); // remove existing file. 

    if(dataKey == "SPEC") {
	writeFDFSpecfile_spectrum(str, 0);
    } else {
        spSpecData_t sd = SpecDataMgr::get()->getDataByKey(dataKey);
        if(sd == (spSpecData_t)NULL) {
           if(!SpecDataMgr::get()->loadSpec(dataKey, (char *)path.c_str()))
           {
             Winfoprintf("Error loading %s",path.c_str());
             return;
           }
        }

        sd = SpecDataMgr::get()->getDataByKey(dataKey);
        specStruct_t *ss = NULL;
        if(sd != (spSpecData_t)NULL) ss = sd->getSpecStruct();
        if(ss == NULL) {
           Winfoprintf("Error loading %s",path.c_str());
           return;
        }

	DDLSymbolTable *cp = (DDLSymbolTable *)sd->st->CloneList();	
	if(sumData != NULL) {
	   cp->SetData(getFirstDataPtr(), sizeof(float)*npts);
	   cp->SetValue("matrix", 1, 1);
	   cp->SetValue("spec_matrix", 1, 1);
	   if(scale == 0.0) scale=1.0;
	   if(input_scale == 0.0) input_scale=1.0;
	   dataInd=0;
	} 
	cp->SetValue("filename", str);
	cp->SaveSymbolsAndData(str);
    }
}

void AspTrace::setBCModel(float *model, int n) {
      if(n != npts) return;
      undoBC();
      if(bcModel) delete[] bcModel;
      bcModel = new float[npts];
      memcpy(bcModel,model,npts*sizeof(float));
}

// Note, BC changes origional data
void AspTrace::doBC() {
      if(!bcModel) return;
      if(doneBC) undoBC();
      doneBC=true;
      float *data = getFirstDataPtr();
      for(int i=0; i<npts; i++) *(data+i) -= *(bcModel+i);
}

void AspTrace::undoBC() {
      if(!bcModel || !doneBC) return;
      doneBC=false;
      float *data = getFirstDataPtr();
      for(int i=0; i<npts; i++) *(data+i) += *(bcModel+i);
}
