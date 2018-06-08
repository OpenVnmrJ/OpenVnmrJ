/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <float.h>
#include <math.h>
#include "AspRoiList.h"
#include "AspFrameMgr.h"
#include "AspUtil.h"

AspRoiList::AspRoiList(int f, bool s) {
    frameID=f;
    roiList = new AspRoiMap;
    roiPath="";
    autoAdjusting=false;
    roiFreqs = new list<double>;
}

AspRoiList::~AspRoiList() {
   delete roiList;
   delete roiFreqs;
}

void AspRoiList::drawRois(spAspCell_t cell) {
   if(cell == nullAspCell || roiList->size()<1) return;

   double pstx, psty, pwd, pht;
   cell->getPixCell(pstx, psty, pwd, pht);
   int yoff = (int)(2*(cell->getCali(VERT)));
   set_clip_region((int)pstx-1,(int)psty+yoff,(int)pwd,(int)pht);

   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	if(roi->show && roi->mouseOver == NOSELECT) roi->draw(cell);
   } 
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	if(roi->show && roi->mouseOver != NOSELECT) roi->draw(cell);
   } 
   set_clip_region(0,0,0,0);
}

int AspRoiList::getNumRois() {
   return roiList->size();
}

// TODO: clear related stuff 
void AspRoiList::removeRois() {
   roiList->clear();
}

spAspRoi_t AspRoiList::getFirstRoi(AspRoiMap::iterator& roiItr) {
    if (roiList && roiList->size() > 0) {
        roiItr = roiList->begin();
        if (roiItr != roiList->end()) {
            return roiItr->second;
        }
    }
    return (spAspRoi_t)NULL;
}

spAspRoi_t AspRoiList::getNextRoi(AspRoiMap::iterator& roiItr) {
    if (++roiItr == roiList->end()) {
        return (spAspRoi_t)NULL;
    }
    return roiItr->second;
}

void AspRoiList::unselectRois() {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	roi->selected=false;
	roi->mouseOver=NOSELECT;
   } 
}

spAspRoi_t AspRoiList::getRoi(int id) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   int i;
   for (roi= getFirstRoi(ri), i=0; roi != nullAspRoi; roi= getNextRoi(ri), i++) {
	if(i == id) return roi;
   } 
   return nullAspRoi;
}

spAspRoi_t AspRoiList::selectRoi(spAspCell_t cell, int x, int y, bool handle) {

   if(cell == nullAspCell) return nullAspRoi;

   AspRoiMap::iterator ri;
   spAspRoi_t selRoi=nullAspRoi;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
        if(roi->select(cell,x,y,handle) && selRoi == nullAspRoi) { 
           selRoi=roi;
        } else roi->mouseOver = NOSELECT;
   }

   return selRoi;
}

void AspRoiList::deleteRoi(spAspRoi_t roi) {
   AspRoiMap::iterator ri;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
        if(ri->second == roi) {
	   roiList->erase(ri->first);
	   return;
        }
   }
}

void AspRoiList::deleteRoi(double c1) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
	roi = ri->second;
        if(roi->cursor1->resonances[0].freq >= c1 && roi->cursor2->resonances[0].freq <= c1) {
	   roiList->erase(ri->first);
	   return;
        }
   }
}

void AspRoiList::deleteRoi(double c1, double c2) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
	roi = ri->second;
        if((roi->cursor1->resonances[0].freq <= c1 || 
		roi->cursor2->resonances[0].freq <= c1) &&
	   (roi->cursor1->resonances[0].freq >= c2 ||
                roi->cursor2->resonances[0].freq >= c2)) {
	   roiList->erase(ri->first);
        }
   }
}

void AspRoiList::addRoi(spAspRoi_t roi) {
   char str[MAXSTR];
   int rank = roi->getRank();
   if(rank==2) {
       sprintf(str,"%s %s %.4f %.4f %.4f %.4f",
               roi->cursor1->resonances[0].name.c_str(),
               roi->cursor1->resonances[1].name.c_str(),
               roi->cursor1->resonances[0].freq,
               roi->cursor2->resonances[0].freq,
               roi->cursor1->resonances[1].freq,
               roi->cursor2->resonances[1].freq);
   } else if(rank==1) {
       sprintf(str,"%s %.4f %.4f",
               roi->cursor1->resonances[0].name.c_str(),
               roi->cursor1->resonances[0].freq,
               roi->cursor2->resonances[0].freq);
   }
   string key = string(str);
   roiList->erase(key);
   roiList->insert(AspRoiMap::value_type(key, roi));
}

void AspRoiList::loadRois(spAspCell_t cell, char *path) {
   struct stat fstat;
   if(stat(path, &fstat) != 0) {
	Winfoprintf("Error: cannot find %s.",path); 
	return;
   }

   // roiList->clear(); // let user clear explicitly
   FILE *fp;
   char  buf[MAXSTR], words[MAXWORDNUM][MAXWORDLEN], *tokptr;
   int count;
   double px,py,pw,ph;
   if((fp = fopen(path, "r"))) {
	while (fgets(buf,sizeof(buf),fp)) {	
	  if(strlen(buf) < 1 || buf[0] == '#') continue;
	    
          // break buf into tok of parameter names
          count=0;
          tokptr = strtok(buf, ", \t\n");
          while(tokptr != NULL) {
            if(strlen(tokptr) > 0) {
              strcpy(words[count], tokptr);
              count++;
            }
            tokptr = strtok(NULL, ", \t\n");
          }

	  if(count < 4) continue;

	  int rank = atoi(words[0]);
	  if(rank == 2 && count > 6) { // rank 2 Roi (a box)
	     // rank xfreq1 xfreq2 yfreq1 yfreq2 label color opaque
	     double f1 = atof(words[3]);
	     double f2 = atof(words[4]);
	     double f3 = atof(words[5]);
	     double f4 = atof(words[6]);
	     // make sure c1 is downfield cursor (bigger ppm, and lower left)
	     // c2 is upfield cursor (smaller ppm, and upper right)
             double freq1 = f1>f2 ? f1:f2;
             double freq2 = f3>f4 ? f3:f4;
	     spAspCursor_t c1 = spAspCursor_t(new AspCursor(freq1,freq2,string(words[1]),string(words[2])));
             freq1 = f1>f2 ? f2:f1;
             freq2 = f3>f4 ? f4:f3;
	     spAspCursor_t c2 = spAspCursor_t(new AspCursor(freq1,freq2,string(words[1]),string(words[2])));
	     spAspRoi_t roi = spAspRoi_t(new AspRoi(c1,c2,roiList->size())); 
	     if(count > 7) roi->label=string(words[7]);
	     if(count > 8) roi->setColor(words[8]);
	     if(count > 9) roi->opaque = atoi(words[9]);

	     // only add roi if it is valid in the given cell.
	     if(roi->getRoiBox(cell,px,py,pw,ph)) addRoi(roi);

	  } else if(rank == 1 && count > 3) { // rank 1 Roi (a band)
	     // rank freq1 freq2 label color opaque height
	     double f1 = atof(words[2]);
	     double f2 = atof(words[3]);
             double freq = f1>f2 ? f1:f2;
	     spAspCursor_t c1 = spAspCursor_t(new AspCursor(freq,string(words[1])));
             freq = f1>f2 ? f2:f1;
	     spAspCursor_t c2 = spAspCursor_t(new AspCursor(freq,string(words[1])));
	     spAspRoi_t roi = spAspRoi_t(new AspRoi(c1,c2,roiList->size())); 
	     if(count > 4) roi->label=string(words[4]);
	     if(count > 5) roi->setColor(words[5]);
	     if(count > 6) roi->opaque = atoi(words[6]);
	     if(count > 7) roi->height = atoi(words[7]);

	     if(roi->getRoiBox(cell,px,py,pw,ph)) addRoi(roi);
	  }

	}
      fclose(fp);
      roiPath = string(path);
   }

}

void AspRoiList::saveRois(char *path) {

   // make sure dir exists
   string tmp = string(path);
   string dir = tmp.substr(0,tmp.find_last_of("/")); 
   
   struct stat fstat;
   if (stat(dir.c_str(), &fstat) != 0) {
       char str[256];
       (void)sprintf(str, "mkdir -p %s \n", dir.c_str());
       (void)system(str);
   }

   FILE *fp;
   if((fp = fopen(path, "w"))) {

      time_t clock;
      char *tdate;
      char datetim[26];
      clock = time(NULL);
      tdate = ctime(&clock);
      if (tdate != NULL) {
        strcpy(datetim,tdate);
        datetim[24] = '\0';
      } else {
        strcpy(datetim,"???");
      }
 
      fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);
      fprintf(fp,"# in PPM\n");
      fprintf(fp,"# rank name ppm1 ppm2 ... color\n");

      spAspRoi_t roi;
      char name[64];

      AspRoiMap::iterator itr1;
      double freq1, freq2;
      int i=0;
      for (itr1 = roiList->begin(); itr1 != roiList->end(); ++itr1, i++) {
	freq1 = itr1->second->cursor1->resonances[0].freq;
	freq2 = itr1->second->cursor2->resonances[0].freq;

	roi = getRoi(freq1,freq2);
	if(roi == nullAspRoi)
        {
           continue;
        }

	if(roi->label == "") sprintf(name,"Roi%d",i);
	else strcpy(name,roi->label.c_str());
	int rank = roi->getRank();
        if(rank == 2) {
	   fprintf(fp,"2 %s %s\t%.4f\t%.4f\t%.4f\t%.4f\t%s %d %d\n",
		roi->cursor1->resonances[0].name.c_str(),
		roi->cursor1->resonances[1].name.c_str(),
		roi->cursor1->resonances[0].freq,
		roi->cursor2->resonances[0].freq,
		roi->cursor1->resonances[1].freq,
		roi->cursor2->resonances[1].freq,name,roi->color,roi->opaque);
	} else if(rank == 1) {
	   fprintf(fp,"1 %s\t%.4f\t%.4f\t%s %d %d %d\n",
		roi->cursor1->resonances[0].name.c_str(),
		roi->cursor1->resonances[0].freq,
		roi->cursor2->resonances[0].freq,
		name, roi->color, roi->opaque, roi->height);
	}
      } 
      roiPath = string(path);
      fclose(fp);
   }
   
}

void AspRoiList::setRoiColor(char *name) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	roi->setColor(name);
   }
}

void AspRoiList::setRoiOpaque(int op) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	roi->opaque = op;
   }
}

void AspRoiList::setRoiHeight(int h) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	roi->height = h;
   }
}

void AspRoiList::hideRoi(int ind) {
   AspRoiMap::iterator ri;
   int i;
   for (ri = roiList->begin(),i=0;  ri != roiList->end(); ++ri,++i) {
        if(i == ind) {
	   ri->second->selected=false;
	   ri->second->show=false;
	   return;
        }
   }
}

void AspRoiList::deleteRoi(int ind) {
   AspRoiMap::iterator ri;
   int i;
   for (ri = roiList->begin(),i=0;  ri != roiList->end(); ++ri,++i) {
        if(i == ind) {
	   roiList->erase(ri->first);
	   return;
        }
   }
}

void AspRoiList::setShowRois(bool b) {
   AspRoiMap::iterator ri;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
	   ri->second->selected=false;
	   ri->second->show=b;
   }
}

void AspRoiList::deleteSelRoi() {
   AspRoiMap::iterator ri;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
        if(ri->second->selected)
	   roiList->erase(ri->first);
   }
}

void AspRoiList::hideSelRoi() {
   AspRoiMap::iterator ri;
   for (ri = roiList->begin();  ri != roiList->end(); ++ri) {
        if(ri->second->selected) {
	   ri->second->selected=false;
	   ri->second->show=false;
	}
   }
}

void AspRoiList::addRois(string nucname, int argc, char *argv[]) {

    if(argc < 4) {
	Winfoprintf("No ROI added.");
	return;
    }
    // skip cmd name and first keyword
    argc -= 2;
    argv += 2;
      
    int color=-1,opaque=-1,height=-1;
    while (argc > 1 && (strcasecmp(argv[argc-2],"setColor")==0 ||
	strcasecmp(argv[argc-2],"setOpaque")==0 || strcasecmp(argv[argc-2],"setHeight")==0) ) {
	if(strcasecmp(argv[argc-2],"setColor") == 0) {
	   if(isdigit(argv[argc-1][0])) color = atoi(argv[argc-1]);
	   else if(!colorindex(argv[argc-1], &color)) color=-1;
	   argc -= 2;
	} else if(strcasecmp(argv[argc-2],"setOpaque") == 0) {
	   opaque =  atoi(argv[argc-1]);
	   argc -= 2;
	} else if(strcasecmp(argv[argc-2],"setHeight") == 0) {
	   height =  atoi(argv[argc-1]);
	   argc -= 2;
	}
    }

    double c1,c2;
    while (argc > 1) {
	c1=atof(argv[0]); argc--; argv++;	
	c2=atof(argv[0]); argc--; argv++;	

    	if(c1<c2) {double tmp=c1; c1=c2; c2=tmp;}

    	spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(c1,nucname));
    	spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(c2,nucname));
    	spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
	if(color > 0) roi->color = color;
	if(opaque >=0) roi->opaque=opaque;
	if(height > 0) roi->height=height;
	addRoi(roi);
    }
}

// remove overlaps
void AspRoiList::autoAdjust(spAspRoi_t newRoi) {
	
   if(getNumRois() < 1) return;
   double minWidth = AspUtil::getReal("aspPref",5,0.08); // 0.08ppm or about 32Hz
   double eps = 1.0e-4;
   
   AspRoiMap::iterator itr1;
   AspRoiMap::iterator itr2;
   double p11,p12,p21,p22;
   string nucname = roiList->begin()->second->cursor1->resonances[0].name;
   for (itr1 = roiList->begin(); itr1 != roiList->end(); ++itr1) {
	if(newRoi != nullAspRoi && newRoi != itr1->second) continue;
	for (itr2 = roiList->begin(); itr2 != roiList->end(); ++itr2) {
	   if(itr2 == itr1) continue;
	   p11 = itr1->second->cursor1->resonances[0].freq;
	   p12 = itr1->second->cursor2->resonances[0].freq;
	   p21 = itr2->second->cursor1->resonances[0].freq;
	   p22 = itr2->second->cursor2->resonances[0].freq;
	   if(p11 < p21 && p11 > p22 && p12 < p21 && p12 > p22 ) { // itr1 inside itr2, remove itr1
		roiList->erase(itr1->first);
		itr1=roiList->begin();
		itr2=roiList->begin();
	   } else if(p21 < p11 && p21 > p12 && p22 < p11 && p22 > p12 ) { // itr2 inside itr1, remove itr2
		roiList->erase(itr2->first);
		itr1=roiList->begin();
		itr2=roiList->begin();
	   } else if((p21-p11)>eps && (p11-p22)>eps) {
		if(fabs(p22-p12) < minWidth && fabs(p21-p11) < minWidth) { // split in moddile
		  double width=(p21-p12)/2;
		  roiList->erase(itr1->first);
		  roiList->erase(itr2->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p21,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p21-width,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		  cursor1 = spAspCursor_t(new AspCursor(p21-width,nucname));
        	  cursor2 = spAspCursor_t(new AspCursor(p12,nucname));
        	  roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		
		} else if(fabs(p11-p12) > fabs(p21-p22)) { // adjust itr1
		  roiList->erase(itr1->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p22,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p12,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		} else { // adjust itr2
		  roiList->erase(itr2->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p21,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p11,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		}
	   } else if((p21-p11) > eps && (p12-p22) > eps) {
		if(fabs(p22-p12) < minWidth && fabs(p21-p11) < minWidth) {
		  double width=(p21-p12)/2;
		  roiList->erase(itr1->first);
		  roiList->erase(itr2->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p21,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p21-width,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		  cursor1 = spAspCursor_t(new AspCursor(p21-width,nucname));
        	  cursor2 = spAspCursor_t(new AspCursor(p12,nucname));
        	  roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		
		} else if(fabs(p11-p12) > fabs(p21-p22)) {
		  roiList->erase(itr1->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p11,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p21,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		} else {
		  roiList->erase(itr2->first);
		  itr1=roiList->begin();
		  itr2=roiList->begin();
		  spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(p12,nucname));
        	  spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(p22,nucname));
        	  spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
		  addRoi(roi);
		}
	   }
	}	
   }
} 

void AspRoiList::autoDefine(AspTraceList *traceList, spAspDataInfo_t dataInfo) {
   double scale = dataInfo->haxis.scale; // reffrq
   double f1 = AspUtil::getReal("cr",0.0)/scale;
   double f2 = f1 - AspUtil::getReal("delta",0.0)/scale;
   autoDefine(traceList,dataInfo,f1,f2);
}

// auto find ROI regions
void AspRoiList::autoDefine(AspTraceList *traceList, spAspDataInfo_t dataInfo,
	double f1, double f2) {
   if(f1==f2 || traceList == NULL || traceList->getSize() < 1 ||
	dataInfo == nullAspData) return;

   if(f1<f2) { // make sure f1>f2
	double tmp=f1;
	f1=f2;
	f2=tmp;
   }
  
   double scale = dataInfo->haxis.scale; // reffrq 
   if(scale <=0) scale=1.0;
   double swppm = AspUtil::getReal("sw",0,0)/scale;
   double roiWidth = AspUtil::getReal("aspPref",4,0.4); // 0.4ppm or about 160Hz
   double minWidth = AspUtil::getReal("aspPref",5,0.08); // 0.08ppm or about 32Hz
   double noiseMult = AspUtil::getReal("aspPref",6,3.0); 
   if(roiWidth < 1.2*minWidth) roiWidth = 1.2*minWidth;
   if(roiWidth > swppm/64) roiWidth = swppm/64;
   double margin = roiWidth*0.05;

   int nRois = (int)(0.5+(f1-f2)/(double)roiWidth);  

//Winfoprintf("scale,swppm,roiWidth,minwidth,nRois %f %f %f %f %f %f %f %d",f1,f2,scale,swppm,roiWidth,minWidth,margin,nRois);
   
   char traceStr[MAXSTR2];
   spAspTrace_t tr = traceList->getTrace(0);
   if(tr == nullAspTrace) return;
   strcpy(traceStr, tr->toString().c_str());
   spAspTrace_t trace = spAspTrace_t(new AspTrace(traceStr));
   // add all traces
   for(int i=1; i<traceList->getSize(); i++) {
   	strcpy(traceStr, traceList->getTrace(i)->toString().c_str());
   	spAspTrace_t sumtrace = spAspTrace_t(new AspTrace(traceStr));
	if(sumtrace == nullAspTrace) continue;
	trace->sumTrace(sumtrace);
   }

   float *data = trace->getFirstDataPtr();
   int npts = trace->getTotalpts();
   double mminX = trace->getMminX();
   double mmaxX = trace->getMmaxX();
   
   float sum=0.0;
   double noise = 0.0;
   for(int i=0; i<(int)(npts*0.05); i++) {
        sum += fabs(*(data+i));
   }
   for(int i=(int)(npts*0.95); i<npts; i++) {
        sum += fabs(*(data+i));
   }
   sum /= (0.1*npts);
   noise = sum;
   double th = AspUtil::getReal("th",0.0)/dataInfo->getVscale();

   int p1,p2,p;
   float amp,miny,maxy;
   double x,x1,x2;
   double *roiFreqs = new double[4*nRois];
   int count=0;
   for(int i=0; i<=nRois; i++) {
	x=f1-i*roiWidth;
      	x1=x+5*margin;
      	x2=x-5*margin;
	p=(int)(npts*(mmaxX-x)/(mmaxX-mminX));	
	p1=(int)(npts*(mmaxX-x1)/(mmaxX-mminX));	
	p2=(int)(npts*(mmaxX-x2)/(mmaxX-mminX));	
	if(i>0 && i<nRois) {
	   float minsum=FLT_MAX;
	   int mink=0;
	   miny=FLT_MAX;
	   double step = 10.0*margin/5.0;
	   for(int k=1; k<5; k++) {
		p1=(int)(npts*(mmaxX-(x1-(k-1)*step))/(mmaxX-mminX));	
		p2=(int)(npts*(mmaxX-(x1-k*step))/(mmaxX-mminX));	
		sum=0.0;
	   	maxy=-1.0*FLT_MAX;
		for(int j=p1; j<p2; j++) {
		  amp = fabs(*(data+j));
		  sum += amp; 
		  if(amp>maxy) maxy=amp;
		}
		if(sum < minsum && maxy < miny) {
		  minsum=sum;
		  miny=maxy;
		  mink = k; 
		}
	    }
	    if(mink > 0) {
	       x1=x1-(mink-1)*step;
	       x2=x1-mink*step;
	       p1=(int)(npts*(mmaxX-x1)/(mmaxX-mminX));
               p2=(int)(npts*(mmaxX-x2)/(mmaxX-mminX));
	    }
	}
	miny=FLT_MAX;
	maxy=0.0;
	p=p1;
	sum=0.0;
	for(int j=p1; j<p2; j++) {
            amp = fabs(*(data+j));
	    sum += amp;
            if(amp < miny) {
               p=j;
               miny = amp;
            }
	    if(amp > maxy) maxy=amp;
        }
	sum /= (p2-p1+1);
	if(noiseMult > 0 && sum < noise*noiseMult) p=(p2+p1)/2; 
	else if(noiseMult == 0 && maxy < th) p=(p2+p1)/2;
	p1=p2=p;
	if(i==0) {
            roiFreqs[count]=mmaxX - p1*(mmaxX-mminX)/npts;
            count++;
	} else if(i == nRois) {
            roiFreqs[count]=mmaxX - p2*(mmaxX-mminX)/npts;
            count++;
	} else { 
	    roiFreqs[count]=mmaxX - p1*(mmaxX-mminX)/npts;
	    count++;	
	    roiFreqs[count]=mmaxX - p2*(mmaxX-mminX)/npts;
	    count++;	
	}	
   }

   string nucname = dataInfo->haxis.name;
   for(int i=0;i<count;i++) { 
       if(i%2 == 1) {
	    p1 = (int)(npts*(mmaxX-roiFreqs[i-1])/(mmaxX-mminX));
	    p2 = (int)(npts*(mmaxX-roiFreqs[i])/(mmaxX-mminX));
	    if(noiseMult>0) {
		sum=0.0;
	    	for(int j=p1; j<p2; j++) {
		   sum += fabs(*(data+j));
	    	}
		maxy = sum/(p2-p1+1);
		miny=noise*noiseMult;
	    } else {
    		maxy=0.0;
                for(int j=p1; j<p2; j++) {
                   sum = fabs(*(data+j));
		   if(sum>maxy) maxy=sum;
            	}
        	miny=th; 
	    }
	    if(maxy>miny) {
    	      spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(roiFreqs[i-1],nucname));
    	      spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(roiFreqs[i],nucname));
    	      spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,getNumRois()));
	      addRoi(roi);
	    }
       }
   }
}

list<double> *AspRoiList::getRoiFreqs(int dim) {
   roiFreqs->clear();
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	if(roi->cursor1->rank>1 && dim==VERT) {
                roiFreqs->push_back(roi->cursor1->resonances[1].freq);
                roiFreqs->push_back(roi->cursor2->resonances[1].freq);
	} else {
		roiFreqs->push_back(roi->cursor1->resonances[0].freq);
		roiFreqs->push_back(roi->cursor2->resonances[0].freq);
        } 
   } 
   roiFreqs->sort();

   return roiFreqs;    
}

spAspRoi_t AspRoiList::getRoi(double freq1, double freq2) {
   AspRoiMap::iterator ri;
   spAspRoi_t roi;
   for (roi= getFirstRoi(ri); roi != nullAspRoi; roi= getNextRoi(ri)) {
	if((roi->cursor1->resonances[0].freq == freq1 &&
		roi->cursor2->resonances[0].freq == freq2) ||
	  (roi->cursor1->resonances[0].freq == freq2 &&
                roi->cursor2->resonances[0].freq == freq1) ) return roi;
   }
   return nullAspRoi;
}
