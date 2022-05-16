/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include "aipInterface.h"
#include "AspUtil.h"
#include "AspMouse.h"
#include "AspDis1D.h"
#include "AspDisPeaks.h"
#include "AspDisAnno.h"
#include "AspDisInteg.h"
#include "AspDisRegion.h"
#include "AspFrameMgr.h"

extern "C" {
  void Jturnoff_aspMouse();
}

int AspMouse::numCursors=0;
int AspMouse::prevX=0;
int AspMouse::prevY=0;
bool AspMouse::creating=false;
char AspMouse::userCursor[128];
char AspMouse::userMacro[128];
AspMouse::mouseState_t AspMouse::state = noState;
AspMouse::mouseState_t AspMouse::origState = noState;
AspMouse::mouseState_t AspMouse::restoreState = noState;
spAspRoi_t AspMouse::roi = nullAspRoi;
AspAnno *AspMouse::anno = NULL;
spAspTrace_t AspMouse::trace = nullAspTrace;
spAspPeak_t AspMouse::peak = nullAspPeak;
spAspInteg_t AspMouse::integ = nullAspInteg;
spAspRegion_t AspMouse::region = nullAspRegion;

/* STATIC VNMRCOMMAND */
int AspMouse::aspSetState(int argc, char *argv[], int retc, char *retv[]) {

    if(argc < 2) {
	Winfoprintf("Usage: aspSetState(mode)");
        RETURN;
    }

    mouseState_t rtn = state;
    if(argc > 2 && !isdigit(argv[2][0])) {
       if(appdirFind(argv[2], "maclib", NULL, "", R_OK) ) {
         
         strcpy(userCursor,argv[1]);
         strcpy(userMacro,argv[2]);
         mouseState_t mode = (mouseState_t)userMode;
         setState(mode);
       } else {
         strcpy(userCursor,"");
         strcpy(userMacro,"");
         mouseState_t mode = state;
         setState(mode);
       }
    } else if(!isdigit(argv[1][0])) {
       if(appdirFind(argv[1], "maclib", NULL, "", R_OK) ) {
         strcpy(userMacro,argv[1]);
       } else {
         strcpy(userMacro,"");
       }
    }  else {
        mouseState_t mode = (mouseState_t)atoi(argv[1]);
        setState(mode);
    }
    if (retc > 0) {
        // TODO: "realString" should be set by Vnmr
        retv[0] = realString((double)rtn);
    }
    //return proc_complete;
    RETURN;
}

/* STATIC */

AspMouse::mouseState_t AspMouse::setState(mouseState_t newState) {
    mouseState_t oldState = state;
    state = newState;

    // Set cursor
    string curType;
    switch (newState) {
    case select:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  hover over annotation    [LEFT] drag to move or resize    [RIGHT] click to bring up menu."); 
        curType = "pointer";
        break;
    case vs:
        curType = "intensity";
        break;
    case createBox:
    case createBand:
    case createPeak:
    case createXBar:
    case createYBar:
    case createOval:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  [LEFT] drag to create annotation."); 
        curType = "pencil";
         break;
    case createLine:
    case createLineTR:
    case createArrow:
    case createArrowTR:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  [LEFT] drag to create annotation. [RIGHT] to exit annotations."); 
        curType = "pencil";
        break;
    case createRegion:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  [LEFT] click or drag to define baseline."); 
        curType = "pencil";
        break;
    case createPoint:
    case createPointTR:
    case createText:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  [LEFT] click to create annotation. [RIGHT] to exit annotations."); 
        curType = "pencil";
        break;
    case createPolygon:
    case createPolyline:
	writelineToVnmrJ("vnmrjcmd","writeToFrameTitle mouse:  [LEFT] click to add N-1 points    [RIGHT] click to add last point."); 
    	writelineToVnmrJ("vnmrjcmd","closepopup file:AnnoProperties.xml");
        curType = "pencil";
        break;
    case modifyBox:
    case modifyBand:
    case modifyPeak:
    case modifyPoint:
    case modifyLine:
    case modifyXBar:
    case modifyYBar:
    case modifyArrow:
    case modifyOval:
    case modifyPolygon:
    case modifyPolyline:
    case modifyText:
    case createInteg:
    case modifyInteg:
        curType = "pencil";
        break;
    case cursor1:
        curType = "pointer";
        break;
    case cursor2:
        curType = "pointer";
        break;
    case zoom:
        curType = "zoom";
        break;
    case pan: 
        curType = "upDownCursor";
        break;
    case array: 
        curType = "pointer";
        break;
    case phasing: 
        curType = "pointer";
        break;
    case userMode:
        curType = userCursor;
        break;
    default:
        curType = "pointer";
        break;
    }

    switch (newState) {
      case createBand:
      case modifyBand:
        break;
      case cursor1:
	numCursors=1;
        break;
      case cursor2:
	numCursors=2;
        break;
      default:
	numCursors=0;
        break;
    }

    setReal("aspMode",newState,true);
    GraphicsWin::setCursor(curType);
    if(newState>0) {
	grabMouse();
    } else if(state != oldState) {
	releaseMouse();
    }

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();
    if(frame == nullAspFrame) return state;

    // redraw frame to remove cursor(s)
    if(state != noState && (oldState == cursor1 || oldState == cursor2)) {
	frame->draw();
    }

    if(state != oldState)
    {
       frame->updateMenu();
       if (state == noState)
       {
	   char str[20];
           Wgetgraphicsdisplay(str, 20);
	   if( ! strcmp(str,"dconi") ) execString("dconi('restart')\n");
	   else if( ! strcmp(str,"ds") ) execString("ds('restart')\n");
       }
    }
    return state;
}

/* STATIC */
void AspMouse::wheelEvent(int clicks, double factor) {
    char cmd[MAXSTR];

    switch (state) {

      case userMode:
	sprintf(cmd,"%s('wheel',%d,%d)\n",userMacro,clicks,0);
	execString(cmd);
        break;

      case noState:
      case select:
      case vs:
      case zoom:
      case pan:
      default:
    	spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();
    	if(frame != nullAspFrame) frame->specVS(clicks,factor);
	break;
     } 
}

void AspMouse::event(int x, int y, int button, int mask, int dummy) {
    char cmd[MAXSTR];

    if ((mask & up) || (mask & click)) {
        // "button" arg is set.  Insert the bit of the button that changed.
        mask |= 0x100 << button;
    }
    /*
     if ((mask & b1) && (mask & b3)) {
     // Buttons 1 and 3 used together, make like it's button 2
     mask |= b2;
     mask &= ~(b1 | b3);     // Turn off buttons 1 and 3
     }
     */
    mask &= 0xffffff03; // Fold click count at 3 (0 means 4, 8, ...)

    // Note, frame is not selected by AspMouse. It selected by java code
    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();
    if(frame == nullAspFrame) return; 

/*
    // Note, cell is created by frame, and cannot be adjusted by AspMouse.
    spAspCell_t cell = frame->selectCell(x,y);
    // a cell has to be selected by x,y.
    if(cell == nullAspCell) return;
*/

    if(strlen(userMacro)>0) {
      switch (mask) {
	case b1+down:
      	   origState=state;
           sprintf(cmd,"%s('b1+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b1+ctrl+down:
	case b1+shift+down:
      	   origState=state;
           sprintf(cmd,"%s('b1+ctrl+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b2+down:
      	   origState=state;
           sprintf(cmd,"%s('b2+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b2+ctrl+down:
	case b2+shift+down:
      	   origState=state;
           sprintf(cmd,"%s('b2+ctrl+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b3+down:
      	   origState=state;
           sprintf(cmd,"%s('b3+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b3+ctrl+down:
	case b3+shift+down:
      	   origState=state;
           sprintf(cmd,"%s('b3+ctrl+down',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	default:
	   break;	  	
      }
    }

    switch (state) {
      case select:
	restoreState = select;
        switch (mask) {
	   case shift+mmove:
	   case ctrl+mmove:
           case mmove:
	     AspDis1D::unselectTraces(frame);
	     roi = frame->selectRoi(x,y,!((mask & shift) || (mask & ctrl)) );
	       if(roi == nullAspRoi) {
		region = AspDisRegion::selectRegion(frame,x,y);
		if(region == nullAspRegion) {
		  peak = AspDisPeaks::selectPeak(frame,x,y);
		  if(peak == nullAspPeak) {
		    integ = AspDisInteg::selectInteg(frame,x,y);
		    if(integ == nullAspInteg) {
		      anno = AspDisAnno::selectAnno(frame,x,y);
		      if(anno == NULL) {
		        if(frame->doDs()) AspDis1D::selectThresh(frame,x,y);
		        if(!(frame->threshSelected)) {
		  	  trace = AspDis1D::selectTrace(frame,x,y);
			  if(trace == nullAspTrace) {
			    frame->select(x,y);
			  }
			}
		      }
		    }
		 }
		}
	      }
	      break;	  	
	   default:
	      break;	  	

	   case b1+down:
	   {
    		writelineToVnmrJ("vnmrjcmd","closepopup file:AnnoProperties.xml");
		if(roi != nullAspRoi) {
		  frame->selectRoiHandle(roi,x,y,true);
		  if(roi->getRank() > 1) setState(modifyBox);
		  else setState(modifyBand);
		} else if(region != nullAspRegion) {
		  setState(modifyRegion);
		} else if(peak != nullAspPeak) {
		  setState(modifyPeak);
		} else if(integ != nullAspInteg) {
		  setState(modifyInteg);
		} else if(anno != NULL) {
		  AnnoType_t type = anno->getType();
		  switch (type) {
		     case ANNO_POINT:
			setState(modifyPoint);
			break;
		     case ANNO_TEXT:
			setState(modifyText);
			break;
		     case ANNO_LINE:
			setState(modifyLine);
			break;
		     case ANNO_XBAR:
			setState(modifyXBar);
			break;
		     case ANNO_YBAR:
			setState(modifyYBar);
			break;
		     case ANNO_ARROW:
			setState(modifyArrow);
			break;
		     case ANNO_BOX:
			setState(modifyBox);
			break;
		     case ANNO_OVAL:
			setState(modifyOval);
			break;
		     case ANNO_POLYGON:
			setState(modifyPolygon);
			break;
		     case ANNO_POLYLINE:
			setState(modifyPolyline);
			break;
		     default:
			break;
		  }
		} 
	   }
	      break;
	   case b1+ctrl+down:
	   case b1+shift+down:
	   {
		if(trace != nullAspTrace) {
		  setState(pan);	
		} 
	   }
	      break;
	   case b3+down:
	   {
    	      writelineToVnmrJ("vnmrjcmd","closepopup file:AnnoProperties.xml");
	      if(region != nullAspRegion) AspDisRegion::deleteRegion(frame,region);
	      else
       	      if(appdirFind("aspCmd", "maclib", NULL, "", R_OK) ) {
		int frameSel=frame->getSelect();
		strcpy(cmd,"");
		if(roi != nullAspRoi) {
		  roi->selected=true;
           	  sprintf(cmd,"aspCmd('roi',%d,%d,%d)\n",roi->id,x,y);
		} else if(peak != nullAspPeak) {
           	  sprintf(cmd,"aspCmd('peak',%d,%d,%d)\n",peak->getIndex(),x,y);
		} else if(integ != nullAspInteg) {
           	  sprintf(cmd,"aspCmd('integ',%d,%d,%d)\n",integ->index,x,y);
		} else if(anno != NULL) {
           	  sprintf(cmd,"aspCmd('anno',%d,%d,%d,%d)\n",anno->index,x,y,(int)(anno->getType()));
		} else if(frame->doDs() && frame->threshSelected) {
           	  sprintf(cmd,"aspCmd('thresh',%d,%d,%d)\n",0,x,y);
		} else if(trace != nullAspTrace) {
		  trace->selected=true;
           	  sprintf(cmd,"aspCmd('trace',%d,%d,%d)\n",trace->traceInd,x,y);
		//} else if(frameSel >=LINE1 && frameSel <= LINE4) { // frame border selected
		} else if(frameSel) { // frame selected
           	  sprintf(cmd,"aspCmd('frame',%d,%d,%d)\n",frame->id,x,y);
		}
	        if(strlen(cmd)>0) execString(cmd);
	   	break;
	      } else {
		if(roi != nullAspRoi) frame->deleteRoi(roi);
		else if(peak != nullAspPeak) AspDisPeaks::deletePeak(frame,peak);
		else if(integ != nullAspInteg) AspDisInteg::deleteInteg(frame,integ);
		else if(anno != NULL) AspDisAnno::deleteAnno(frame,anno);
		else if(frame->doDs() && frame->threshSelected) {
		   frame->setDisFlag(SPEC_THRESH, false);
		   frame->draw();
		} else if(trace != nullAspTrace) {
		   AspDis1D::deleteTrace(frame, trace);
		} 	
	      }
	   }
	      break;
	   case b3+drag:
	   {
		//frame->deleteRoi(x,y);
	   }
	      break;
	   case b1+drag: // may drag thresh
	   {
		int frameSel=frame->getSelect();
		if(frame->doDs() && frame->threshSelected) {
		  AspDis1D::moveThresh(frame,x,y,prevX,prevY);
		} else if(trace != nullAspTrace) {
		  AspDis1D::moveTrace(frame,trace,y,prevY);
		} else if(frameSel >=HANDLE1 && frameSel <= HANDLE4) { // frame corner selected
		  frame->resizeFrame(x,y,prevX,prevY);
		} else if(frameSel >=LINE1 && frameSel <= LINE4) { // frame border selected
		  frame->moveFrame(x,y,prevX,prevY);
		}
	   }
	      break;
           case b1+ctrl+click+2:
	    if(anno != NULL && !creating && anno->selected == HANDLE_SELECTED) {
		AspDisAnno::deletePoint(frame, anno, x, y);
	    } else if(anno != NULL && !creating) {
		AspDisAnno::addPoint(frame, anno, x, y,true);
	    } 
	    frame->displayTop();
            break;
	}
	break;      
      case cursor1:
        switch (mask) {
	   case shift+mmove:
	   case ctrl+mmove:
           case mmove: // TODO: show cross hair
	      if(frame->doDs()) AspDis1D::selectThresh(frame,x,y);
	      break;	  	
	   default:
	      break;	  	

	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+shift+down:
	   {
		if(frame->doDs() && frame->threshSelected) {
		} else frame->showCursor(x,y,1,1);
	   }
	      break;
	   case b3+down:
	   {
		if(frame->doDs() && frame->threshSelected) {
		} else {
		  setState(cursor2);
		  frame->showCursor(x,y,2,2);
		}
	   }
	      break;
	   case b1+drag: // may drag thresh
	   {
		if(frame->doDs() && frame->threshSelected) {
		  AspDis1D::moveThresh(frame,x,y,prevX,prevY);
		}
	   }
	      break;
	}
	break;      
      case cursor2:
        switch (mask) {
	   case shift+mmove:
	   case ctrl+mmove:
           case mmove: // TODO: show cross hair
	      if(frame->doDs()) AspDis1D::selectThresh(frame,x,y);
	      break;	  	
	   default:
	      break;	  	

	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+shift+down:
	   {
		if(frame->doDs() && frame->threshSelected) {
		} else frame->showCursor(x,y,2,1);
	   }
	      break;
	   case b3+down:
	   {
		setState(cursor2);
		if(frame->doDs() && frame->threshSelected) {
		} else {
		  frame->showCursor(x,y,2,2);
		}
	   }
	      break;
	   case b1+drag: // may drag thresh
	   {
		if(frame->doDs() && frame->threshSelected) {
		  AspDis1D::moveThresh(frame,x,y,prevX,prevY);
		}
	   }
	      break;
	}
	break;      
      case zoom:
        switch (mask) {
	   case shift+mmove:
	   case ctrl+mmove:
           case mmove: // TODO: show cross hair
	   {
	   }
	      break;	  	
	   //case b1+click+2: 
	   case b1+ctrl+click+1: // zoom out to full 
	   case b1+shift+click+1: // zoom out to full 
	   {
		frame->zoomSpec(x,y,FULL);
	   }
	      break;
	   case b1+click+1: // center and zoom in
	   {
		frame->zoomSpec(x,y,ZOOM_IN);
	   }
	      break;
	   case b3+click+1: // center and zoom out
	   {
		frame->zoomSpec(x,y,ZOOM_OUT);
	   }
	      break;
	   case b1+down:
	   {
		startDrag(frame,x,y); // shaded rubber band or box
	   }
	      break;
	   default:
	      break;	  	
	}
	break;      
      case pan:
        switch (mask) {
	   //case b1+click+2: 
	   case b1+ctrl+click+1: // reset vp to zero
	   case b1+shift+click+1: // reset vp to zero
	   {
		frame->panSpec(x,y,prevX,prevY,RESET);
	   }
	      break;
	   case b1+click+1: // center
	   {
		frame->panSpec(x,y,prevX,prevY,CENTER);
	   }
	      break;
	   case b1+drag: // pan both horizontally and vertically
	   {
		frame->panSpec(x,y,prevX,prevY,PAN_2D);
	   }
	      break;
	   case b3+drag: // drag up to zoom out, drag down to zoom in
	   {
		frame->panSpec(x,y,prevX,prevY,PAN_ZOOM);
	   }
	      break;
           case b1+ctrl+drag: // pan horizontally
           case b1+shift+drag: // pan horizontally
	   {
		frame->panSpec(x,y,prevX,prevY,PAN_1D);
	   }
	      break;
           case b1+up: // reset mode ??? 
	   {
		if(trace != nullAspTrace && restoreState == select) { 
		   setState(select);
		}
	   }
	      break;
	   default:
	      break;	  	
	}
	break;      
      case phasing:
        switch (mask) {
	   case b1+drag: // zero order 
	   {
		AspDis1D::setPhase0(frame,x,y,prevX,prevY,COARSE);
	   }
	      break;
	   case b3+drag: // fine zero order 
	   {
		AspDis1D::setPhase0(frame,x,y,prevX,prevY,FINE);
	   }
	      break;
	   case b1+ctrl+click+1: // auto 
	   case b1+shift+click+1: // auto 
	   {
		AspDis1D::setPhase0(frame,x,y,prevX,prevY,AUTO);
	   }
	      break;
	}
	break;      
      case array:
        switch (mask) {
	   //case b1+click+2: 
	   case b1+ctrl+click+1: // reset vo, ho to zero
	   case b1+shift+click+1: // reset vo, ho to zero
	   {
		AspDis1D::setArray(frame,x,y,prevX,prevY,ARRAY_RESET);
	   }
	      break;
	   case b1+ctrl+click+2: // reset vo, ho, vp to zero
	   case b1+shift+click+2: // reset vo, ho, vp to zero
	   {
		AspDis1D::setArray(frame,x,y,prevX,prevY,ARRAY_RESETALL);
	   }
	      break;
	   case b1+drag: // change vo or vp 
	   {
		AspDis1D::setArray(frame,x,y,prevX,prevY,ARRAY_OFFSET);
	   }
	      break;
	   case b1+ctrl+drag: // change ho 
	   case b1+shift+drag: // change ho 
	   {
		AspDis1D::setArray(frame,x,y,prevX,prevY,ARRAY_VP);
	   }
	      break;
	   default:
	      break;	  	
	}
	break;      
      case createBand:
      case createBox:
      case createPoint:
      case createPointTR:
      case createLine:
      case createLineTR:
      case createXBar:
      case createYBar:
      case createArrow:
      case createArrowTR:
      case createOval:
      case createPolygon:
      case createPolyline:
      case createText:
      {
        switch (mask) {
	   case mmove:
               break;
	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+down:
	   case b1+ctrl+shift+down:
	   {
		if(state == createBand && numCursors ==1)
		  restoreState = ((mask & ctrl) || (mask & shift)) ? state : cursor1;
		else if(state == createBand && numCursors ==2)
		  restoreState = ((mask & ctrl) || (mask & shift)) ? state : cursor2;
		else if ( (state == createPolygon) || (state == createPolyline) )
		  restoreState = ((mask & ctrl) || (mask & shift)) ? state : noState;
		else
		  restoreState = state;
		//mouseHandled = true;
                switch (state) {
/*
                  case createBox:
    		  	roi = frame->addRoi(ROI_BOX,x,y);
			if(roi != nullAspRoi) {
		  	   setState(modifyBox);
	    		   creating=true;
			}
			break;
*/
		  case createBand:
    		  	roi = frame->addRoi(ROI_BAND,x,y);
			if(roi != nullAspRoi) {
		  	   setState(modifyBand);
	    		   creating=true;
			}
			break;
		  case createPoint:
		  case createPointTR:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_POINT,
                                           (state == createPointTR) );
			if(anno != NULL) {
		  	   setState(restoreState);
	    		   creating=true;
			}
			break;
		  case createText:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_TEXT);
			if(anno != NULL) {
		  	   setState(restoreState);
	    		   creating=true;
			   strcpy(cmd,"");
           	  	   sprintf(cmd,"aspCmd('annoText',%d,%d,%d,%d)\n",anno->index,x,y,ANNO_TEXT);
	        	   execString(cmd);
			}
			break;
		  case createLine:
		  case createLineTR:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_LINE,
                                           (state == createLineTR) );
			if(anno != NULL) {
		  	   setState(modifyLine);
	    		   creating=true;
			}
			break;
		  case createXBar:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_XBAR);
			if(anno != NULL) {
		  	   setState(modifyXBar);
	    		   creating=true;
			}
			break;
		  case createYBar:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_YBAR);
			if(anno != NULL) {
		  	   setState(modifyYBar);
	    		   creating=true;
			}
			break;
		  case createArrow:
		  case createArrowTR:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_ARROW,
                                           (state == createArrowTR) );
			if(anno != NULL) {
		  	   setState(modifyArrow);
	    		   creating=true;
			}
			break;
		  case createBox:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_BOX);
			if(anno != NULL) {
		  	   setState(modifyBox);
	    		   creating=true;
			}
			break;
		  case createOval:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_OVAL);
			if(anno != NULL) {
		  	   setState(modifyOval);
	    		   creating=true;
			}
			break;
		  case createPolygon:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_POLYGON);
			if(anno != NULL) {
		  	   setState(modifyPolygon);
	    		   creating=true;
			}
			break;
		  case createPolyline:
			anno = AspDisAnno::createAnno(frame, x, y, ANNO_POLYLINE);
			if(anno != NULL) {
		  	   setState(modifyPolyline);
	    		   creating=true;
			}
			break;
		  default:
			break;
		}
		break;
	   }
	   case b3+up:
	   {
		setState(noState);
		break;
           }
		break;
	   default:
		break;
	}
	break;      
      }
      case modifyBand:
      case modifyBox:
      case modifyPoint:
      case modifyLine:
      case modifyXBar:
      case modifyYBar:
      case modifyArrow:
      case modifyOval:
      case modifyText:
        switch (mask) {
	case b1+drag:
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
	    if( (state == modifyBand) && (roi != nullAspRoi)) {
	        if(creating) frame->selectRoiHandle(roi,x,y,!((mask & shift) || (mask & ctrl)));
	        creating=false;
		frame->modifyRoi(roi,x,y);
	    } else if(anno != NULL) {
                if (((state == modifyPoint) ||
                     (state == modifyLine) ||
                     (state == modifyArrow)) &&
                          (mask == b1+drag))
		   AspDisAnno::modifyAnno(frame, anno, x, y, prevX, prevY, true);
                else
		   AspDisAnno::modifyAnno(frame, anno, x, y, prevX, prevY, false);
	    }
            break;
        case b1+up:
	    if( (state == modifyBand) && roi != nullAspRoi &&
               frame->getRoiList()->autoAdjusting) {
	       frame->getRoiList()->autoAdjust(roi);
	    }
            if((state == modifyBand) && roi != nullAspRoi)
            {
                roi->selected=false;
		frame->unselectRois();
            }
            if(anno != NULL) {
                if ((state == modifyBox) ||
                     (state == modifyOval) )
                {
		   AspDisAnno::checkAnno(frame, anno, x, y);
                }
         
		anno->selected=0;
		anno->selectedHandle=0;
           	sprintf(cmd,"aspCmd('annoModified',%d,%d,%d)\n",anno->index,x,y);
	        execString(cmd);
	    }
            setState(restoreState);
	    creating=false;
	    frame->displayTop();
            break;
        case b1+ctrl+up:
        case b1+shift+up:
        case b1+ctrl+shift+up:
	{
	    if( (state == modifyBand) && roi != nullAspRoi &&
                 frame->getRoiList()->autoAdjusting) {
	       frame->getRoiList()->autoAdjust(roi);
	    }
            if(roi != nullAspRoi) roi->selected=false;
	    if(anno != NULL) {
		anno->selected=0;
		anno->selectedHandle=0;
	    }
            setState(restoreState);
	    creating=false;
	    frame->displayTop();
            break;
	}
	default:
            break;
	}
        break;
      case modifyPolygon:
      case modifyPolyline:
        switch (mask) {
	case mmove:
	case mmove + ctrl:
	    if(anno != NULL && creating) {
		AspDisAnno::modifyAnno(frame, anno, x, y, prevX, prevY);
	    }
            break;
	case b1+drag:
	case b1+ctrl+drag:
	    if(anno != NULL) {
		AspDisAnno::modifyAnno(frame, anno, x, y, prevX, prevY);
	    }
            break;
        case b1+up:
        case b1+ctrl+up:
        case b3+down:
	    if(anno != NULL && creating) {
		AspDisAnno::addPoint(frame, anno, x, y);
	    } else if(anno != NULL) {
		anno->selected=0;
		anno->selectedHandle=0;
                setState(restoreState);
	        creating=false;
           	sprintf(cmd,"aspCmd('annoModified',%d,%d,%d)\n",anno->index,x,y);
	        execString(cmd);
	        frame->displayTop();
	    }
            break;
        case b3+up:
        case b3+ctrl+up:
            setState(restoreState);
	    creating=false;
	    frame->displayTop();
            break;
	default:
            break;
	}
        break;
      case createRegion:
        switch (mask) {
	   case mmove:
		break;
	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+down:
	   case b1+ctrl+shift+down:
	   {
		restoreState = ((mask & ctrl) || (mask & shift)) ? state : select;
		startDrag(frame,x,y);
	   }
		break;
	   case b3+down:
	   {
	      if(region != nullAspRegion) {
		AspDisRegion::deleteRegion(frame,region);
	      }

              break;
	   }
	   default:
		break;
	}
	break;      
      case modifyRegion:
        switch (mask) {
	case b1+drag:
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
	    if(region != nullAspRegion) {
		AspDisRegion::modifyRegion(frame,region,x,y,prevX,prevY,mask);
	    }
            break;
        case b1+up:
        case b1+ctrl+up:
        case b1+shift+up:
        case b1+ctrl+shift+up:
	{
            setState(select);
            break;
	}
	default:
            break;
	}
	break;      
      case createPeak:
        switch (mask) {
	   case mmove:
		break;
	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+down:
	   case b1+ctrl+shift+down:
	   {
		restoreState = ((mask & ctrl) || (mask & shift)) ? state : select;
		startDrag(frame,x,y);
	   }
		break;
	   case b3+down:
	   {
	   if(appdirFind("aspCmd", "maclib", NULL, "", R_OK) ) {
		if(peak != nullAspPeak) { 
           	  sprintf(cmd,"aspCmd('peak',%d,%d,%d)\n",peak->getIndex(),x,y);
           	  execString(cmd);
		}
	      } else if(peak != nullAspPeak) {
		AspDisPeaks::deletePeak(frame,peak);
	      }

              break;
	   }
	   default:
		break;
	}
	break;      
      case modifyPeak:
        switch (mask) {
	case b1+drag:
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
	    if(peak != nullAspPeak) {
		AspDisPeaks::modifyPeak(frame,peak,x,y);
	    }
            break;
        case b1+up:
        case b1+ctrl+up:
        case b1+shift+up:
        case b1+ctrl+shift+up:
	{
            setState(select);
            break;
	}
	default:
            break;
	}
      case createInteg:
        switch (mask) {
	   case mmove:
		break;
	   case b1+down:
	   case b1+shift+down:
	   case b1+ctrl+down:
	   case b1+ctrl+shift+down:
	   {
		restoreState = ((mask & ctrl) || (mask & shift)) ? state : select;
		startDrag(frame,x,y);
	   }
		break;
	   case b3+down:
	   {
	   if(appdirFind("aspCmd", "maclib", NULL, "", R_OK) ) {
		if(integ != nullAspInteg) { 
           	  sprintf(cmd,"aspCmd('integ',%d,%d,%d)\n",integ->index,x,y);
           	  execString(cmd);
		}
	      } else if(integ != nullAspInteg) {
		AspDisInteg::deleteInteg(frame,integ);
	      }

              break;
	   }
	   default:
		break;
	}
	break;      
      case modifyInteg:
        switch (mask) {
	case b1+drag:
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
	    if(integ != nullAspInteg) {
		AspDisInteg::modifyInteg(frame,integ,x,y,prevX,prevY,mask);
	    }
            break;
        case b1+up:
        case b1+ctrl+up:
        case b1+shift+up:
        case b1+ctrl+shift+up:
	{
            setState(select);
            break;
	}
	default:
            break;
	}
	break;      
      default:
	break;      
    }

    if(strlen(userMacro)>0) {
      switch (mask) {
	case b1+up:
           sprintf(cmd,"%s('b1+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b1+ctrl+up:
	case b1+shift+up:
           sprintf(cmd,"%s('b1+ctrl+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b2+up:
           sprintf(cmd,"%s('b2+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b2+ctrl+up:
	case b2+shift+up:
           sprintf(cmd,"%s('b2+ctrl+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b3+up:
           sprintf(cmd,"%s('b3+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	case b3+ctrl+up:
	case b3+shift+up:
           sprintf(cmd,"%s('b3+ctrl+up',%d,%d,%d)\n",userMacro,origState,x,y);
           execString(cmd);
	   break;
	default:
	   break;	  	
      }
    }
    prevX=x;
    prevY=y;
}

/* STATIC */
void AspMouse::reset() {
}

void AspMouse::grabMouse()
{
    setReal("buttonMode",0,true);
    Jactivate_mouse(NULL, NULL, NULL, AspMouse::event, AspMouse::reset);
}

void AspMouse::releaseMouse()
{
    Jturnoff_aspMouse();
}

// start shaded rubber band or box
void AspMouse::startDrag(spAspFrame_t frame, int x, int y) {
    switch (state) {
      case zoom:
	frame->startZoom(x,y); 
	break;
      case createPeak:
	writelineToVnmrJ("vnmrjcmd", "canvas rubberbox");
        break;
      case createInteg:
      case createRegion:
	writelineToVnmrJ("vnmrjcmd", "canvas rubberarea");
        break;
      default:
        break;
    }
}

void AspMouse::endDrag(spAspFrame_t frame, int x,int y,int prevX, int prevY) {
    switch (state) {
      case zoom:
	frame->setZoom(x,y,prevX,prevY);
	break;
      case createPeak:
        AspDisPeaks::peakPicking(frame,x,y,prevX,prevY);
	setState(restoreState);
        break;
      case createInteg:
        AspDisInteg::createInteg(frame,x,y,prevX,prevY);
	setState(restoreState);
        break;
      case createRegion:
        AspDisRegion::createRegion(frame,x,y,prevX,prevY);
	setState(restoreState);
        break;
      default:
        break;
    }
}
