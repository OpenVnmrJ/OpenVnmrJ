/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <algorithm>
using std::swap;
#include <cmath>
using std::sqrt;
using std::fabs;
using namespace std;

#include "AspUtil.h"
#include "AspDisInteg.h"
#include "AspMouse.h"
#include "AspFrameMgr.h"
#include "AspDis1D.h"

extern "C" {
}

int AspDisInteg::aspInteg(int argc, char *argv[], int retc, char *retv[]) {

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();

    if(frame == nullAspFrame) RETURN;

    if(argc==1 && retc > 0) { // quary display info
	if(frame->getIntegFlag() == 0) retv[0]=realString(0.0);
	else retv[0]=realString(1.0);
	if(retc>1) retv[1]=realString((double)(frame->getIntegList()->getSize()));
	RETURN;
    }

    spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);
    if(dataInfo == nullAspData) RETURN;

   if(argc>1 && strcasecmp(argv[1],"nli") ==0) {
        nli(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"save") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/integs",curexpdir); 

        save(frame,path);
   } else if(argc>1 && strcasecmp(argv[1],"load") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/integs",curexpdir); 

        load(frame,path,true);
   } else if(argc>1 && strcasecmp(argv[1],"reset") == 0) {
	frame->getIntegList()->resetLabels();
        frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"delete") == 0) {
	 deleteInteg(frame, nullAspInteg);
   } else if(argc>1 && strcasecmp(argv[1],"clear") == 0) {
         frame->getIntegList()->clearList();
   	 frame->initIntegFlag(0);
   	 frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"dpir") == 0) {
	dpir(frame,argc,argv);
   } else if(argc>3 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
           spAspInteg_t integ = frame->getIntegList()->getInteg(atoi(argv[2])-1);
           if(integ != nullAspInteg) {
                retv[0] = newString(integ->getProperty(argv[3]).c_str());
           }
   } else if(argc>2 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
           spAspInteg_t integ = frame->getIntegList()->getSelInteg();
           if(integ != nullAspInteg) {
                retv[0] = newString(integ->getProperty(argv[2]).c_str());
           }
   } else if(argc>4 && strcasecmp(argv[1],"set") == 0) {
           spAspInteg_t integ = frame->getIntegList()->getInteg(atoi(argv[2])-1);
           if(integ != nullAspInteg) {
                integ->setProperty(argv[3],argv[4]);
                frame->displayTop();
           }
         frame->displayTop();
   } else if(argc>3 && strcasecmp(argv[1],"set") == 0) {
           spAspInteg_t integ = frame->getIntegList()->getSelInteg();
           if(integ != nullAspInteg) {
                integ->setProperty(argv[2],argv[3]);
                frame->displayTop();
           }
         frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"integ") == 0) {
	if(argc>2) frame->setIntegFlag(SHOW_INTEG,(atoi(argv[2])>0)); 
	if(retc>0) {
	   int value;
	   if(frame->getIntegFlag() & SHOW_INTEG) value = 1;
	   else value=0;
	   retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strstr(argv[1],"value") != NULL && strstr(argv[1],"vert") != NULL) {
	if(argc>2) frame->setIntegFlag(SHOW_VERT_VALUE,(atoi(argv[2])>0)); 
	if(retc>0) {
	   int value;
	   if(frame->getIntegFlag() & SHOW_VERT_VALUE) value = 1;
	   else value=0;
	   retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"value") == 0) {
	if(argc>2) frame->setIntegFlag(SHOW_VALUE,(atoi(argv[2])>0)); 
	if(retc>0) {
	   int value;
	   if(frame->getIntegFlag() & SHOW_VALUE) value = 1;
	   else value=0;
	   retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strstr(argv[1],"label") != NULL && strstr(argv[1],"vert") != NULL) {
	if(argc>2) frame->setIntegFlag(SHOW_VERT_LABEL,(atoi(argv[2])>0)); 
	if(retc>0) {
	   int value;
	   if(frame->getIntegFlag() & SHOW_VERT_LABEL) value = 1;
	   else value=0;
	   retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"label") == 0) {
	if(argc>2) frame->setIntegFlag(SHOW_LABEL,(atoi(argv[2])>0)); 
	if(retc>0) {
	   int value;
	   if(frame->getIntegFlag() & SHOW_LABEL) value = 1;
	   else value=0;
	   retv[0] = realString((double)value);
	} else frame->displayTop();
   }

   frame->updateMenu();

   RETURN;
}

void AspDisInteg::save(spAspFrame_t frame, char *path) {
   // make sure dir exists
   string tmp = string(path);
   string dir = tmp.substr(0,tmp.find_last_of("/"));

   struct stat fstat;
   if (stat(dir.c_str(), &fstat) != 0) {
       char str[MAXSTR2];
       (void)sprintf(str, "mkdir -p %s \n", dir.c_str());
       (void)system(str);
   }

   FILE *fp;
   if(!(fp = fopen(path, "w"))) {
        Winfoprintf("Failed to create integ file %s.",path);
        return;
   }

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

   AspIntegList *integList = frame->getIntegList();
   int nintegs = integList->getSize();

   fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);
   fprintf(fp,"number_of_integs: %d\n",nintegs);
   fprintf(fp,"normCursor: %f\n",integList->normCursor);
   fprintf(fp,"# index,ppm1 ppm2,absValue,normValue,dataID,label,labelX,labelY,color,fontColor,fontsize,fontName,fontStyle,rotate,disFlag\n");

   int i;
   spAspInteg_t integ;
   for(i=0;i<nintegs;i++) {
	integ = integList->getInteg(i);
	if(integ != nullAspInteg) { 
	   fprintf(fp,"%s\n",integ->toString().c_str());
	}
   }

   fclose(fp);
}

void AspDisInteg::load(spAspFrame_t frame, char *path, bool show) {

   struct stat fstat;
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return;
   }

   // make sure at least one cell exists
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;
   
   FILE *fp;
   if(!(fp = fopen(path, "r"))) {
        Winfoprintf("Failed to open session file %s.",path);
        return;
   }

   char  buf[MAXSTR], words[MAXWORDNUM][MAXSTR], *tokptr;
   int nw=0;

   AspIntegList *integList = frame->getIntegList();
   integList->clearList();

   int nintegs=0;
   int count=0;
   while (fgets(buf,sizeof(buf),fp)) {
      if(strlen(buf) < 1 || buf[0] == '#') continue;
          // break buf into tok of parameter names

      nw=0;
      tokptr = strtok(buf, " \n");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, " \n");
      }

      if(nw < 2) continue;
      if(strcasecmp(words[0],"number_of_integs:")==0) {
	nintegs = atoi(words[1]); 
      } else if(strcasecmp(words[0],"normCursor")==0 && nw>10) {
	integList->normCursor = atof(words[1]);
      } else if(nintegs>0 && nw>4) {
	spAspInteg_t integ = spAspInteg_t(new AspInteg(words,nw));
	integList->addInteg(integ->index, integ);	
	count++;
      }
   }
   fclose(fp);

   if(count == 0) { 
	Winfoprintf("0 integral loaded.");
	return;	
   }

   if(nintegs != count) Winfoprintf("Warning: number of integs does not match %d %d",nintegs,count);

   // calculate integrals according to insref, ins parameters.
   integList->update();

   if(!frame->getIntegFlag()) frame->initIntegFlag(SHOW_INTEG | SHOW_VALUE);
   if(show) frame->displayTop();
}

// display integral from lifrq and liamp parameters
void AspDisInteg::nli(spAspFrame_t frame, int argc, char *argv[]) {
    
   int nintegs = AspUtil::getParSize("lifrq");
   if(nintegs < 1) {
	Winfoprintf("%d integrals are defined by lifrq.",nintegs);
	return;
   }

   if(nintegs != AspUtil::getParSize("liamp")) { // calculate liamp
	execString("nli\n");
   }
   nintegs = (nintegs-1)/2;

   AspIntegList *integList = frame->getIntegList();
   integList->clearList();

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   double reffrq = dataInfo->haxis.scale; // in MHz
   double rflrfp = dataInfo->haxis.minfirst; // in ppm 
   int i, count=0;
   double freq1,freq2,amp;
   for(i=0;i<nintegs;i++) {
	freq1=AspUtil::getReal("lifrq",i*2+1,0.0) / reffrq + rflrfp; // lifrq is in Hz 
	freq2=AspUtil::getReal("lifrq",i*2+2,0.0) / reffrq + rflrfp; // lifrq is in Hz 
	amp=AspUtil::getReal("liamp",i*2+2,0.0);	
  	spAspInteg_t integ = spAspInteg_t(new AspInteg(count,freq1,freq2,amp));
	integ->dataID = string("SPEC:0");
	integList->addInteg(count, integ);
	count++;
   }

   if(!frame->getIntegFlag()) frame->initIntegFlag(SHOW_INTEG | SHOW_VALUE);
   frame->displayTop();
}

void AspDisInteg::dpir(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   argc--;
   argv++;
   if(argc>1) {
      frame->initIntegFlag(0);
      frame->getIntegList()->resetLabels();
      if(strstr(argv[1],"integ") != NULL) frame->setIntegFlag(SHOW_INTEG,true);
      if(strstr(argv[1],"value") != NULL) frame->setIntegFlag(SHOW_VALUE,true);
      if(strstr(argv[1],"label") != NULL) frame->setIntegFlag(SHOW_LABEL,true);
      if(strstr(argv[1],"vert") != NULL && strstr(argv[1],"value") != NULL) 
	frame->setIntegFlag(SHOW_VERT_VALUE,true);
      if(strstr(argv[1],"vert") != NULL && strstr(argv[1],"label") != NULL) 
	frame->setIntegFlag(SHOW_VERT_LABEL,true);
   }
   
   int nintegs = frame->getIntegList()->getSize();
   if(!nintegs) {
	Winfoprintf("0 integ is picked or loaded.");
	return;
   }

   frame->displayTop();
}

spAspInteg_t AspDisInteg::createInteg(spAspFrame_t frame, int x,int y,int prevX, int prevY) {
   if(frame == nullAspFrame) return nullAspInteg;
   spAspCell_t cell = frame->selectCell(x,y);
   if(cell == nullAspCell) return nullAspInteg;

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return nullAspInteg;

   double freq1 = cell->pix2val(HORIZ,prevX); 
   double freq2 = cell->pix2val(HORIZ,x); 
   double amp = trace->getInteg(freq1,freq2);

   AspIntegList *integList = frame->getIntegList();
   
   spAspInteg_t integ = spAspInteg_t(new AspInteg(integList->getSize(),freq1,freq2,amp));
   integ->dataID = trace->getKeyInd();
   integList->addInteg(integ->index, integ);	
   integList->update();
   
   if(!frame->getIntegFlag()) frame->initIntegFlag(SHOW_INTEG | SHOW_VALUE);
   frame->displayTop();

   integ->select(cell,x,y);
   return integ;
}

void AspDisInteg::modifyInteg(spAspFrame_t frame, spAspInteg_t integ, int x, int y, int prevX, int prevY, int mask) {
   if(integ == nullAspInteg) return;
   if(frame == nullAspFrame) return;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return;

   if(mask & ctrl) integ->modifyVert(cell,x,y,prevX,prevY);
   else integ->modify(cell,x,y,prevX,prevY); 
   frame->getIntegList()->update();
   frame->displayTop();
}

void AspDisInteg::deleteInteg(spAspFrame_t frame, spAspInteg_t integ) {
   AspIntegList *integList = frame->getIntegList();
   if(integList == NULL) return;

   if(integ == nullAspInteg) integList->deleteInteg(); 
   else integList->deleteInteg(integ->index); 
   integList->update();
   frame->displayTop();
}

spAspInteg_t AspDisInteg::selectInteg(spAspFrame_t frame, int x, int y) {
   if(frame == nullAspFrame) return nullAspInteg;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return nullAspInteg;
   if(frame->getIntegList()->getSize() < 1) return nullAspInteg;

   bool changeFlag=false;
   
   spAspInteg_t integ = frame->getIntegList()->selectInteg(cell,x,y,changeFlag);

   if(changeFlag) frame->displayTop();

   return integ;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspDisInteg::display(spAspFrame_t frame) {
   if(frame == nullAspFrame) return;

   if(frame->getIntegList()->getSize() < 1) return;

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   int integFlag = frame->getIntegFlag();
   int specFlag = frame->getSpecFlag();
   if(integFlag == 0) return;

   // for now only display it in first cell
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   spAspInteg_t integ = frame->getIntegList()->getInteg(0);
   if(integ == nullAspInteg) return;

   // make sure there is enough vp space to display integ values
   double ycali = cell->getCali(VERT);
   double vpos = dataInfo->getVpos()*ycali;
   if(integFlag & SHOW_VERT_VALUE) {
          char str[MAXSTR];
          sprintf(str,"%.2f",100.00);
          int cht, ascent, descent;
          GraphicsWin::getTextExtents(str, 14, &ascent, &descent, &cht);
	  string value;
          AspUtil::getDisplayOption("IntegralMarkThickness",value);
          int thickness = atoi(value.c_str());
          double h = cht + 5*thickness;
        if(h > vpos) {
            AspUtil::setReal("vp", (double)h/ycali, true);
	    frame->draw();
	    return;
        }
   } else if(integFlag & SHOW_VALUE) {
          char str[MAXSTR];
          sprintf(str,"%.2f",100.00);
          int cwd, cht, ascent, descent;
          GraphicsWin::getTextExtents(str, 14, &ascent, &descent, &cwd);
	  cht = ascent + descent;
	  string value;
          AspUtil::getDisplayOption("IntegralMarkThickness",value);
          int thickness = atoi(value.c_str());
          double h = 1.5*cht + 15*thickness;
        if(h > vpos) {
            AspUtil::setReal("vp", (double)h/ycali, true);
	    frame->draw();
	    return;
        }
   }

   frame->getIntegList()->display(cell, frame->getSelTraceList(), dataInfo, integFlag, specFlag);
}


