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
#include "AspDisAnno.h"
#include "AspMouse.h"
#include "AspFrameMgr.h"
#include "AspPoint.h"
#include "AspText.h"
#include "AspLine.h"
#include "AspBar.h"
#include "AspArrow.h"
#include "AspBox.h"
#include "AspOval.h"
#include "AspPolygon.h"
#include "AspPolyline.h"

string AspDisAnno::clipboardStr="";

extern "C" {
int do_mkdir(const char *dir, int psub, mode_t mode);
char *intString(int i);
}

int AspDisAnno::aspAnno(int argc, char *argv[], int retc, char *retv[]) {

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();


    if(argc==1 && retc > 0) { // quary display info
       if(frame == nullAspFrame)
       {
	  retv[0]=realString(0.0);
	  if(retc>1) retv[1]=realString(0.0);
       }
       else
       {
	  if(frame->getAnnoFlag() == 0) retv[0]=realString(0.0);
	  else retv[0]=realString(1.0);
	  if(retc>1) retv[1]=realString((double)(frame->getAnnoList()->getSize()));
       }
	RETURN;
    }

    if(frame == nullAspFrame) RETURN;

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);
   if(dataInfo == nullAspData) RETURN;

   if(argc>1 && strcasecmp(argv[1],"save") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/annos",curexpdir); 

        save(frame,path);
   } else if(argc>1 && strcasecmp(argv[1],"load") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/annos",curexpdir); 

        load(frame,path,true,retc);
   } else if(argc>1 && strcasecmp(argv[1],"delete") == 0) {
	if(argc>2) frame->getAnnoList()->deleteAnno(atoi(argv[2])-1);
	else frame->getAnnoList()->deleteAnno();
   	frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"clear") == 0) {
         frame->getAnnoList()->clearList();
   	 frame->setAnnoFlag(ANN_ANNO,false);
   	 frame->displayTop();
	 clipboardStr="";
   } else if(argc>1 && strcasecmp(argv[1],"show") == 0) {
	show(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"reset") == 0) {
	 frame->getAnnoList()->resetProperties();
   	 frame->displayTop();
   } else if(argc>3 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
	   AspAnno *anno = frame->getAnnoList()->getAnno(atoi(argv[2])-1);
	   if(anno) {
		retv[0] = newString(anno->getProperty(argv[3]).c_str()); 
	   }
   } else if(argc>2 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
	   AspAnno *anno = frame->getAnnoList()->getSelAnno();
	   if(anno) {
		retv[0] = newString(anno->getProperty(argv[2]).c_str()); 
	   }
   } else if(argc == 2 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
           AspAnnoList *annoList = frame->getAnnoList();
	   if(annoList) {
              if (retc)
		retv[0] = intString(annoList->getSize()); 
              else
                Winfoprintf("Number of annotions: %d", annoList->getSize());
	   }
           else
           {
              if (retc)
		retv[0] = intString(0); 
              else
                Winfoprintf("Number of annotions: 0");
           }
   } else if(argc>4 && strcasecmp(argv[1],"set") == 0) {
      AspAnno *anno;
      int annIndex;
      if ( argv[2][0] == '$')
      {
         int index;
         int num;
         double tmp;

         num = P_getsizeLocal(argv[2]);
         if (num < 0)
         {
          Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[2]);
          ABORT;
         }
            for (index=1; index <= num; index++)
            {
                 P_getrealLocal(argv[2], &tmp, index);
                 annIndex = (int) tmp;
                 if (annIndex == 0)
                 {
                    AspAnnoList *annoList = frame->getAnnoList();
                    AspAnnoMap::iterator itr;
                    for (anno= annoList->getFirstAnno(itr); anno != NULL;
                         anno= annoList->getNextAnno(itr))
                    {
                       int val = 3;
                       while (val < argc)
                       {
         		   anno->setProperty(argv[val],argv[val+1],frame->getFirstCell());
            	           frame->displayTop();
                           val += 2;
                       }
                    }
                    break;
                 }
                 else
                 {
                    anno = frame->getAnnoList()->getAnno(annIndex-1);
	            if(anno) {
                       int val = 3;
                       while (val < argc)
                       {
         		   anno->setProperty(argv[val],argv[val+1],frame->getFirstCell());
            	           frame->displayTop();
                           val += 2;
                       }
	            }
                 }
            }
      }
      else
      {
           char *str, *endptr;

           str = argv[2];
           while (str)
           {
              annIndex = strtol(str, &endptr, 10);
              if (str != endptr)
              {
                 str = endptr;
                 if (annIndex == 0)
                 {
                    AspAnnoList *annoList = frame->getAnnoList();
                    AspAnnoMap::iterator itr;
                    for (anno= annoList->getFirstAnno(itr); anno != NULL;
                         anno= annoList->getNextAnno(itr))
                    {
                       int val = 3;
                       while (val < argc)
                       {
         		   anno->setProperty(argv[val],argv[val+1],frame->getFirstCell());
            	           frame->displayTop();
                           val += 2;
                       }
                    }
                    break;
                 }
                 else
                 {
                    anno = frame->getAnnoList()->getAnno(annIndex-1);
	            if(anno) {
                       int val = 3;
                       while (val < argc)
                       {
         		   anno->setProperty(argv[val],argv[val+1],frame->getFirstCell());
            	           frame->displayTop();
                           val += 2;
                       }
	            }
                 }
              }
              else
                 break;
           }
      }
      frame->displayTop();
   } else if(argc>3 && strcasecmp(argv[1],"set") == 0) {
           AspAnno *anno = frame->getAnnoList()->getSelAnno();
	   if(anno) {
		anno->setProperty(argv[2],argv[3],frame->getFirstCell());
   	   	frame->displayTop();
	   }
   	 frame->displayTop();
   } else if(argc>2 && strcasecmp(argv[1],"copy")==0) {
	AspAnno *anno = frame->getAnnoList()->getAnno(atoi(argv[2])-1);
	if(anno) {
	   clipboardStr = anno->toString();
	}
   } else if(argc>1 && strcasecmp(argv[1],"paste")==0) {
	bool canPaste = (clipboardStr != "");
	if(argc>4) { // x,y are in mm, need to convert to pixels
   	   spAspCell_t cell = frame->getFirstCell();
	   double x =atof(argv[2]);
	   double y =atof(argv[3]);
	   if(cell != nullAspCell) {
		x = cell->mm2pix(HORIZ,x);	
		y = cell->mm2pix(VERT,y);	
	   }
	   paste(frame, string(argv[4]), (int)x,(int)y);
	} else if(canPaste && argc>3) { // x,y are in pixels
	   paste(frame, string(clipboardStr), atoi(argv[2]),atoi(argv[3]));
	} else if(argc>2) { // no x,y, so position specified by annoStr will not be modified.
	   paste(frame, string(argv[2]),0,0);

	}
	if(retc>0) {
	   retv[0] = realString((double)canPaste);
	}
	if(retc>1) {
	   retv[1] = newString(clipboardStr.c_str());
	}
   } else if(((argc == 4) || (argc == 6))  && strcasecmp(argv[1],"is") == 0) {
        testAnno(frame, argc, argv, retc, retv);
   }

   frame->updateMenu();

   RETURN;
}

void AspDisAnno::testAnno(spAspFrame_t frame, int argc, char *argv[], int retc, char *retv[]) {
   AspAnnoList *annoList = frame->getAnnoList();
   int nannos = annoList->getSize();
   if(nannos < 1) {
       if (retc)
       {
	   retv[0] = intString(0); 
           if (retc > 1)
	      retv[1] = newString(""); 
       }
       else
           Winfoprintf("Annotations of type %s with value %s: 0", argv[2], argv[3]);
       return;
   }
   AspAnno *anno;
   AspAnnoMap::iterator itr;
   char ids[MAXSTR2];
   char id[MAXSTR];

   ids[0] = '\0';
   int inx = 0;
   for (anno= annoList->getFirstAnno(itr); anno != NULL; anno= annoList->getNextAnno(itr)) {
         if ( ! strcmp(anno->getProperty(argv[2]).c_str(), argv[3]) )
         {
            if (argc == 6)
            {
               if ( ! strcmp(anno->getProperty(argv[4]).c_str(), argv[5]) )
               {
                  inx++;
                  if (ids[0] == '\0')
                     sprintf(id,"%d",anno->index + 1);
                  else
                     sprintf(id," %d",anno->index + 1);
                  strcat(ids,id);
               }
            }
            else
            {
               inx++;
               if (ids[0] == '\0')
                  sprintf(id,"%d",anno->index + 1);
               else
                  sprintf(id," %d",anno->index + 1);
               strcat(ids,id);
            }
         }
   }
   if (retc)
   {
	   retv[0] = intString(inx); 
           if (retc > 1)
	      retv[1] = newString(ids); 
   }
   else
           Winfoprintf("Annotations of type %s with value %s: %d", argv[2], argv[3], inx);
   return;
}

void AspDisAnno::save(spAspFrame_t frame, char *path) {

   AspAnnoList *annoList = frame->getAnnoList();
   int nannos = annoList->getSize();
   if(nannos < 1) {
        Winfoprintf("Abort: no annotions to save.");
        return;
   }

   // make sure dir exists
   string tmp = string(path);
   string dir = tmp.substr(0,tmp.find_last_of("/"));

   struct stat fstat;
   if (stat(dir.c_str(), &fstat) != 0) {
       int ival __attribute__((unused));
       ival = do_mkdir(dir.c_str(), 1, 0777);
   }

   FILE *fp;
   if(!(fp = fopen(path, "w"))) {
        Winfoprintf("Failed to create anno file %s.",path);
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
   fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);
   fprintf(fp,"number_of_annos: %d\n",nannos);

   AspAnno *anno;
   AspAnnoMap::iterator itr;

   int inx = 0;
   for (anno= annoList->getFirstAnno(itr); anno != NULL; anno= annoList->getNextAnno(itr)) {
      anno->index = inx;
      inx++;
      fprintf(fp,"%s\n",anno->toString().c_str());
   }
/*
   for(i=0;i<nannos;i++) {
	anno = annoList->getAnno(i);
	if(anno != NULL) { 
	   fprintf(fp,"%s\n",anno->toString().c_str());
	}
   }
 */

   fclose(fp);
}

void AspDisAnno::load(spAspFrame_t frame, char *path, bool show, int retc) {

   struct stat fstat;
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return;
   }

   // make sure at least one cell exists.
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   FILE *fp;
   if(!(fp = fopen(path, "r"))) {
        Winfoprintf("Failed to open session file %s.",path);
        return;
   }

   char  buf[MAXSTR], words[MAXWORDNUM][MAXSTR], *tokptr;
   int nw=0;

   AspAnnoList *annoList = frame->getAnnoList();
   annoList->clearList();

   int nannos=0;
   int count=0;
   while (fgets(buf,sizeof(buf),fp)) {
      if(strlen(buf) < 1 || buf[0] == '#') continue;
          // break buf into tok of parameter names

      nw=0;
      tokptr = strtok(buf, " \n");
      while(tokptr != NULL && nw < MAXWORDNUM) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, " \n");
      }

      if(nw < 2) continue;
      if(strcasecmp(words[0],"number_of_annos:")==0) {
	nannos = atoi(words[1]); 
      } else if(strcasecmp(words[0],"???")==0 && nw>10) {
      } else {
        AspAnno *anno = NULL;
	if(strcasecmp(words[1],"POINT") == 0) {
	   anno = new AspPoint(words,nw);
	} else if(strcasecmp(words[1],"Text") == 0) {
	   anno = new AspText(words,nw);
	} else if(strcasecmp(words[1],"Line") == 0) {
	   anno = new AspLine(words,nw);
	} else if(strcasecmp(words[1],"XBar") == 0) {
	   anno = new AspBar(words,nw);
	   anno->setType(ANNO_XBAR);
	} else if(strcasecmp(words[1],"YBar") == 0) {
	   anno = new AspBar(words,nw);
	   anno->setType(ANNO_YBAR);
	} else if(strcasecmp(words[1],"Arrow") == 0) {
	   anno = new AspArrow(words,nw);
	} else if(strcasecmp(words[1],"Box") == 0) {
	   anno = new AspBox(words,nw);
	} else if(strcasecmp(words[1],"Oval") == 0) {
	   anno = new AspOval(words,nw);
	} else if(strcasecmp(words[1],"Polygon") == 0) {
	   anno = new AspPolygon(words,nw);
	} else if(strcasecmp(words[1],"Polyline") == 0) {
	   anno = new AspPolyline(words,nw);
	}
	if(anno != NULL && anno->getType() != ANNO_NONE) {
	  annoList->addAnno(anno);	
	  count++;
	}
      }
   }
   fclose(fp);

   if(count == 0) { 
	Winfoprintf("0 annotation loaded.");
	return;	
   }

   if ((nannos>0) && (nannos != count) && (retc == 0) ) 
	Winfoprintf("Warning: number of annos does not match %d %d",nannos,count);

   frame->setAnnoFlag(ANN_ANNO,true);
   if(show) frame->displayTop();
}

void AspDisAnno::paste(spAspFrame_t frame, string str, int x, int y) {
   char  words[MAXWORDNUM][MAXSTR], *tokptr;
   int nw=0;
   char buf[MAXSTR];
   strcpy(buf,str.c_str());
      tokptr = strtok(buf, " \n");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, " \n");
      }

      if(nw < 2) return;

        AspAnno *anno = NULL;
	if(strcasecmp(words[1],"POINT") == 0) {
	   anno = new AspPoint(words,nw);
	} else if(strcasecmp(words[1],"Text") == 0) {
	   anno = new AspText(words,nw);
	} else if(strcasecmp(words[1],"Line") == 0) {
	   anno = new AspLine(words,nw);
	} else if(strcasecmp(words[1],"XBar") == 0) {
	   anno = new AspBar(words,nw);
	   anno->setType(ANNO_XBAR);
	} else if(strcasecmp(words[1],"YBar") == 0) {
	   anno = new AspBar(words,nw);
	   anno->setType(ANNO_YBAR);
	} else if(strcasecmp(words[1],"Arrow") == 0) {
	   anno = new AspArrow(words,nw);
	} else if(strcasecmp(words[1],"Box") == 0) {
	   anno = new AspBox(words,nw);
	} else if(strcasecmp(words[1],"Oval") == 0) {
	   anno = new AspOval(words,nw);
	} else if(strcasecmp(words[1],"Polygon") == 0) {
	   anno = new AspPolygon(words,nw);
	} else if(strcasecmp(words[1],"Polyline") == 0) {
	   anno = new AspPolyline(words,nw);
	}
	if(anno != NULL && anno->getType() != ANNO_NONE) {
	  frame->getAnnoList()->addAnno(anno);	
   	  frame->setAnnoFlag(ANN_ANNO,true);
	  if(x>0 && y>0) {
	     // have to display before modify because pCoord is calculated by display
	     anno->selected = ROI_SELECTED;
	     frame->displayTop(); 
	     modifyAnno(frame, anno, x,y, x,y);
	  } else {
	     frame->displayTop(); 
	  }
	}
}

void AspDisAnno::show(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   argc--;
   argv++;
   if(argc>1 && atoi(argv[1]) == 0) frame->setAnnoFlag(ANN_ANNO, false);
   else frame->setAnnoFlag(ANN_ANNO, true);
   
   int nannos = frame->getAnnoList()->getSize();
   if(!nannos) {
	Winfoprintf("0 anno is defined.");
	return;
   }

   frame->displayTop();
}

// this is called when mouse released from dragging rubber band 
// threshold th will be used if x=y=prevX=prevY=0
AspAnno *AspDisAnno::createAnno(spAspFrame_t frame, int x, int y, AnnoType_t type,
                                bool trCase) {

    if(frame == nullAspFrame) return NULL; 
    spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
    if(dataInfo == nullAspData) return NULL;
    spAspCell_t cell;
    if(x > 0) cell = frame->selectCell(x,y);
    else {
	cell = frame->getFirstCell();
    }
    if(cell == nullAspCell) return NULL;

    AspAnnoList *annoList = frame->getAnnoList();

    AspAnno *anno = NULL;
    switch (type) {
	case ANNO_POINT:
	   anno = new AspPoint(cell, x, y, trCase);
	   break;
	case ANNO_TEXT:
	   anno = new AspText(cell, x, y);
	   break;
	case ANNO_LINE:
	   anno = new AspLine(cell, x, y, trCase);
	   break;
	case ANNO_XBAR:
	   anno = new AspBar(cell, x, y);
	   anno->setType(ANNO_XBAR);
	   break;
	case ANNO_YBAR:
	   anno = new AspBar(cell, x, y);
	   anno->setType(ANNO_YBAR);
	   break;
	case ANNO_ARROW:
	   anno = new AspArrow(cell, x, y, trCase);
	   break;
	case ANNO_BOX:
	   anno = new AspBox(cell, x, y);
	   break;
	case ANNO_OVAL:
	   anno = new AspOval(cell, x, y);
	   break;
	case ANNO_POLYGON:
	   anno = new AspPolygon(cell, x, y);
	   break;
	case ANNO_POLYLINE:
	   anno = new AspPolyline(cell, x, y);
	   break;
	default:
	   break;
    }

    if(anno == NULL) return NULL;

    annoList->addAnno(anno);

   frame->setAnnoFlag(ANN_ANNO,true);
   frame->displayTop();

   // select anno so it can be modified.
   anno->select(x,y);
   return anno;
}

void AspDisAnno::modifyAnno(spAspFrame_t frame, AspAnno *anno, int x, int y,
                            int prevX, int prevY, bool trCase) {
   if(anno == NULL) return;
   if(frame == nullAspFrame) return;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return;

   if (trCase)
      anno->modifyTR(cell,x,y,prevX,prevY,trCase); 
   else
      anno->modify(cell,x,y,prevX,prevY); 
   frame->displayTop();
}

void AspDisAnno::checkAnno(spAspFrame_t frame, AspAnno *anno, int x, int y)
{
   if(anno == NULL) return;
   if(frame == nullAspFrame) return;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return;

   anno->check(cell); 
   frame->displayTop();
}

void AspDisAnno::deleteAnno(spAspFrame_t frame, AspAnno *anno) {
   AspAnnoList *annoList = frame->getAnnoList();
   if(annoList == NULL) return;

   if(anno == NULL) annoList->deleteAnno(); 
   else annoList->deleteAnno(anno->index); 
   frame->displayTop();
}

AspAnno *AspDisAnno::selectAnno(spAspFrame_t frame, int x, int y) {
//Winfoprintf("selectAnno");
   if(frame == nullAspFrame) return NULL;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return NULL;
   if(frame->getAnnoList()->getSize() < 1) return NULL;

   bool changeFlag=false;
   
   AspAnno *anno = frame->getAnnoList()->selectAnno(x,y,changeFlag);

   if(changeFlag) frame->displayTop();

   return anno;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspDisAnno::display(spAspFrame_t frame) {
   if(frame == nullAspFrame) return;

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   // for now only display it in first cell
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   frame->getAnnoList()->display(cell, dataInfo);
}

void AspDisAnno::addPoint(spAspFrame_t frame, AspAnno *anno, int x, int y, bool insert) {
  AnnoType_t type = anno->getType();
  if(type == ANNO_POLYGON || type == ANNO_POLYLINE) {
    if(frame == nullAspFrame) return;
    spAspCell_t cell =  frame->selectCell(x,y);

    if(cell == nullAspCell) return;
    if(insert) anno->insertPoint(cell,x,y);
    else anno->addPoint(cell,x,y);
  }
}

void AspDisAnno::deletePoint(spAspFrame_t frame, AspAnno *anno, int x, int y) {
  AnnoType_t type = anno->getType();
  if(type == ANNO_POLYGON || type == ANNO_POLYLINE) {
    if(frame == nullAspFrame) return;
    spAspCell_t cell =  frame->selectCell(x,y);

    if(cell == nullAspCell) return;
    anno->deletePoint(cell,x,y);
  }
}
