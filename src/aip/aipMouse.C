/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include "aipOrthoSlices.h"
#include "aipBox.h"
#include "aipCommands.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipGraphicsWin.h"
#include "aipInterface.h"
#include "aipLine.h"
#include "aipMouse.h"
#include "aipOval.h"
#include "aipPoint.h"
#include "aipPolygon.h"
#include "aipPolyline.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipStderr.h"
#include "aipVnmrFuncs.h"
#include "aipVsInfo.h"
#include "aipWinMath.h"
#include "aipMovie.h"
#include "aipWinMovie.h"
#include "aipVolData.h"
#include "aipAxisInfo.h"

using namespace aip;

extern "C" {
void graph_batch(int on);
int onActivePlan(int x, int y);
extern int aip_mnumypnts;
extern int graf_height;
}

char Mouse::userCursor[128];
char Mouse::userMacro[128];
int Mouse::mouseFrameID;
Mouse::mouseState_t Mouse::state = noState;
Roi *Mouse::roi= NULL;

/* STATIC VNMRCOMMAND */
int Mouse::aipSetState(int argc, char *argv[], int retc, char *retv[]) {
/*
    if (argc > 2) {
        fprintf(stderr,"Usage: %s<(new_state)><:old_state>\n", argv[0]);
        return proc_error;
    }
*/
    mouseState_t rtn = state;
    if (argc > 2) {
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
    }  else if (argc > 1) {
        mouseState_t mode = (mouseState_t)atoi(argv[1]);
        setState(mode);
    }
    if (retc > 0) {
        // TODO: "realString" should be set by Vnmr
        retv[0] = realString((double)rtn);
    }
    return proc_complete;
}

/* STATIC */
bool Mouse::creatingPoint() {
   return (state == createPoint);
}

bool Mouse::creatingLine() {
   return (state == createLine);
}

Mouse::mouseState_t Mouse::setState(mouseState_t newState) {
    mouseState_t oldState = state;

    if (newState == previous) {
        // Call with newState=previous to reassert same internal state
        newState = state;
    }
    if (newState != notOwner) {
        // Set new internal state ...
        state = newState;
        if (newState <= 0|| newState >= lastExternalState) {
            // ... but don't change external state in these cases
            newState = oldState;
        }
    }

    // Set cursor
    string curType;
    switch (newState) {
    case select:
        curType = "pointer";
        break;
    case vs:
        curType = "intensity";
        break;
    case createPoint:
    case createLine:
    case createBox:
    case createOval:
    case createPolygon:
    case createPolyline:
    case modifyPoint:
    case modifyLine:
    case modifyBox:
    case modifyOval:
    case modifyPolygon:
    case modifyPolyline:
        curType = "pencil";
        break;
    case zoom:
    case pan: // At least for now
        curType = "zoom";
        break;
    case imageMath:
        curType = "math";
        break;
    case notOwner:
        curType = "pointer";
        break;
    case selectCSIvox:
        curType = "selectCSIvox";
        break;
    case userMode:
        curType = userCursor;
        break;
    default:
        //state = select;
        curType = "pointer";
        break;
    }
    if (newState != notOwner && !aipHasScreen()) {
        // Put something on the screen (and grab mouse/screen)
        GframeManager::get()->draw();
    }

    setReal("aipMode", newState, true);
    GraphicsWin::setCursor(curType);
    bool oldLock = aipLockScreen();
    if (newState != notOwner) {
        // NB: Sometimes Wturnoff_buttons() releases mouse, so lock screen
        oldLock = aipLockScreen(true);
    }
    Wturnoff_buttons(); // Update the button highlighting
    aipLockScreen(oldLock);

    return oldState;
}

/* STATIC */
void Mouse::wheelEvent(int clicks, double factor) {
    char cmd[MAXSTR];
    if (!aipHasScreen()) {
      setState(notOwner);
      releaseMouse();
      return;
    }

    switch (state) {

      case userMode:
	sprintf(cmd,"%s('wheel',%d,%d)\n",userMacro,clicks,mouseFrameID);
	execString(cmd);
        break;

      case select:

	// only if in 3plane display mode
        if (!VolData::get()->showingObliquePlanesPanel()) return;

	// turn off 3plane cursors display.
  	// OrthoSlices::get()->setShowCursors(false);

        // return if 3-plane cursors are displayed 
        // if(OrthoSlices::get()->getShowCursors()) return;

  	// check mouseFrameID (set by mouse move)
  	if(mouseFrameID < 1 || mouseFrameID > 3) return; 

  	// foreward or backward 1 image at a time regardless "clicks". ignore "factor".
	// if(clicks == 1 || clicks == -1) // ignore clicks=2 or -2 (fast scroll)
	  OrthoSlices::get()->setNext3Ppnt(mouseFrameID, clicks);
	
	break;
     } 
}

void Mouse::event(int x, int y, int button, int mask, int dummy) {
    // Remember these
    static int handle = -1;
    static spGframe_t gframe = nullFrame;
    static spGframe_t vsframe = nullFrame;
    static bool vsChanged = false; // Prevent spurious VS mods to bound frames
    //static Point *point = NULL;
    //static Line *line = NULL;
    //static Box *box = NULL;
    //static Polygon *polygon = NULL;
    //static Polyline *polyline = NULL;

    /* Mask uses four bytes to represent the mouse event.
     * bit0 to bit7: Number of clicks.
     * bit8: button_1, bit9: button_2, bit10: button_3,
     * bit16: button_press, but17: button_release, but18: button_click,
     * bit19: drag. bit20: mmove.
     * bit24: SHIFT, bit25: CTRL, bit26: ALT, bit27: META
     */

    // mask: 27   26   25   24 . 20   19   18   17   16 . 10   9   8 . 7-0
    //       meta alt  ctrl shft mmove drag clik up   down b3   b2  b1  clicks

    /* if ((mask & mmove) == 0) {
     fprintf(stderr,"mask=0x%08x, button=%d\n", mask, button);
     }/*CMP*/

    if (!aipHasScreen()) {
        // Someone else started using the screen w/o controlling the mouse.
        setState(notOwner);
        releaseMouse();
        //GraphicsWin::setCursor("pointer");
        return;
    }

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

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = gfm->getGframeFromCoords(x, y);
    spViewInfo_t view = nullView;
    if (gf != nullFrame) {
        view = gf->getSelView();
    }
    RoiManager *roim = RoiManager::get();

    // If state has not been set yet, then get the value from aipMode
    // and set it.  If aipMode does not exist, default to select mode.
    if (state == 0) {
        mouseState_t mode = (mouseState_t)getReal("aipMode", (double)select);
        if (mode == 0) {
            mode = select;
        }
        setState(mode);
    }
    if (button == 0) {
        if (aip_mnumypnts < (graf_height - 10))
            aip_mnumypnts = graf_height - 4;
    }

    OrthoSlices *oslice = OrthoSlices::get();

    bool mouseHandled = false;
    static mouseState_t restoreState = select;
    char cmd[MAXSTR];

    bool showROIlabel = (getReal("aipShowPosition",0) > 0);
    bool showOver = (showROIlabel && getReal("aipShowROIPos",0) == 0);
    bool showIntensity = (getReal("aipShowROIOpt",0) > 0);

    // save this for mouseWheel
    mouseFrameID = gf->getGframeNumber(); 

    strcpy(cmd,"");
    switch (state) {

    case userMode:
        switch (mask) { // only set some of the event
	case b1+drag: 
	  sprintf(cmd,"%s('b1+drag',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b1+ctrl+shift+click+1: 
	  sprintf(cmd,"%s('b1+ctrl+shift+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b1+shift+click+1: 
	  sprintf(cmd,"%s('b1+shift+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b1+ctrl+click+1: 
	  sprintf(cmd,"%s('b1+ctrl+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b1+click+1: 
	  sprintf(cmd,"%s('b1+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b2+click+1: 
	  sprintf(cmd,"%s('b2+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b3+click+1: 
	  sprintf(cmd,"%s('b3+click+1',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b1+up: 
	  sprintf(cmd,"%s('b1+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b2+up: 
	  sprintf(cmd,"%s('b2+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
        case b3+up: 
	  sprintf(cmd,"%s('b3+up',%d,%d,%d)\n",userMacro,x,y,mouseFrameID);
	     break;
	}
        if(strlen(cmd)>0) execString(cmd);
	break;
    case select:
        switch (mask) {

        default:
            if (!roi && gf != nullFrame && isIplanObj(x, y, gf->id)) {
                // Iplan has something here; deselect any gframe
                if (gframe != nullFrame) {
                    gframe->setHighlight(false);
                    gframe = nullFrame;
                }
            }
            break;

        case shift+mmove:
        case ctrl+mmove:
        case mmove: /* Update visual cues */
            // NB: Sets the "roi" for other "select" actions
            // NB: handle is modified
        {
            bool ignoreHandles = (mask & shift);
            bool deselectGframe = false;
            roi = roim->highlightRoi(x, y, ignoreHandles, roi, handle);
            if (roi) {
                if(gf != nullFrame && showOver) {
		  gf->draw();
		  Roitype type = roi->GetType();
                  if(showIntensity) {
		     roi->showIntensity(false);
                  } else if(handle != -1) {
/*
                     if(type == ROI_LINE) {
		       double x1,y1,x2,y2;
		       if(roi->getFirstLast(x1,y1,x2,y2) == 2)
		       AxisInfo::showDistance(gf,(int)x1, (int)y1, (int)x2, (int)y2);
		     } else {
*/
		        double x1,y1;
		        if(roi->getHandlePoint(roi->getHandle(x,y), x1,y1) == 1) 
		          AxisInfo::showPosition(gf,(int)x1,(int)y1,true,false);
//		     }
		  } else if(type == ROI_POINT) {
		     double x1,y1;
		     if(roi->getHandlePoint(0, x1,y1) == 1) 
		     AxisInfo::showPosition(gf,(int)x1,(int)y1,true,false);
		  } else if(type == ROI_LINE) {
		     double x1,y1,x2,y2;
		     if(roi->getFirstLast(x1,y1,x2,y2) == 2) 
		     AxisInfo::showDistance(gf,(int)x1, (int)y1, (int)x2, (int)y2);
		  }
		}
                mouseHandled = true;
                deselectGframe = true;
            } else {
                if (gf != nullFrame) { // Only check the frame we are in
                    if (onActivePlan(x, y)) {
                        // Iplan has something here; deselect any gframe
                        deselectGframe = true;
                    } else {
                        // No Iplan object, check for gframe
                        gframe = gf->highlight(x, y, gframe);
                        if (gframe != nullFrame) {
                            mouseHandled = true;
                        }
                    }
                } else {
                    deselectGframe = true;
                }
            }
            if (deselectGframe && gframe != nullFrame) {
                gframe->setHighlight(false);
                gframe = nullFrame;
            }
        }
            break;

        case b1+down: /* Activate ROI (ready for move) */
        case b1+shift+down: // Because we can drag it this way
        case b1+ctrl+shift+down: // Because we can drag it this way
	{
            bool ignoreHandles = (mask & shift);
            if(ignoreHandles) // select whole roi if shift key is held down. 
              roi = roim->highlightRoi(x, y, ignoreHandles, roi, handle);
            if (!(mask & shift) && VolData::get()->showingObliquePlanesPanel() 
		&& OrthoSlices::getShowCursors()) {
                if(oslice->startCursorMoved(x, y)){
                    mouseHandled = true;
                    break;
                }
            }
            if(roi = roim->activateRoi(x, y)) {
                graph_batch(1);
                mouseHandled = true;
                roi->setBase(x, y);
                roi->someInfo(false);
                roi->activateSlaves();
                graph_batch(0);
            }
            if(gf != nullFrame) gfm->setActiveGframe(gf);
	}
            break;
        case b1+ctrl+up: // Because we can drag it this way
            if (gframe != nullFrame) {
                mouseHandled = true;
                if (!WinMovie::get()->movieRunning() && !Movie::get()->movieRunning()) {
                    gframe->setSelect(!gframe->selected, true);
                    VsInfo::setVsHistogram();
                }
	    }
            break;
        case b1+ctrl+shift+click+1: /* Add a point to a polygon */
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                roi->activateSlaves();
                handle = roi->add_point(x, y);
                roi->create_done(); // Deletes any duplicate polygon points
                RoiStat::get()->calculate(false);
                graph_batch(0);
            }
            break;

        case b1+up: /* Clean up any ROI we were working on */
        case b1+shift+up: // Because we can drag it this way
        case b1+ctrl+shift+up: // Because we can drag it this way
            if (VolData::get()->showingObliquePlanesPanel() 
		&& OrthoSlices::getShowCursors()) {
                 if(oslice->endCursorMoved(x, y)){
                     mouseHandled = true;
                     break;
                 }
            }
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                roi->create_done(); // Deletes any duplicate polygon points
                roi->someInfo(false);
                RoiStat::get()->calculate(false);
                graph_batch(0);
            }
            if (gf != nullFrame) {
		gf->setPositionInfo(x, y);
            }
            break;

        case b1+click+1: /* Select ROI or Gframe (exclusively) */
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                roi->select(ROI_COPY, false); // Dont append
                RoiStat::get()->calculate(false); // Selection may change calc
                graph_batch(0);
            } else if (gframe != nullFrame) {
                mouseHandled = true;
                if (!WinMovie::get()->movieRunning() && !Movie::get()->movieRunning()) {
                    gframe->setSelect(true, false);
                    gfm->setFrameToLoad(gframe);
                    VsInfo::setVsHistogram();
	            gframe->sendImageInfo();
                }
            }
            break;

        case b1+shift+click+1: /* Select/Deselect all ROIs/Gframes */
        case b1+shift+click+2:
        case b1+shift+click+3:
        case b1+shift+click+0:
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                roim->selectDeselectAllRois(roi);
                RoiStat::get()->calculate(false); // Selection may change calc
                graph_batch(0);
            } else if (gframe != nullFrame) {
                mouseHandled = true;
                gfm->shiftSelectFrames(gframe);
                gfm->setFrameToLoad(0);
                VsInfo::setVsHistogram();
            }
            break;

        case b1+ctrl+click+1: /* Select/Deselect all bound ROIs */
        case b1+ctrl+click+2:
        case b1+ctrl+click+3:
        case b1+ctrl+click+0:
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                roi->activateSlaves();
                roim->selectDeselectBoundRois(roi);
                RoiStat::get()->calculate(false); // Selection may change calc
                gfm->draw(); // Redraw all images
                roim->clearActiveList();
                graph_batch(0);
 	    }

            break;

        case b1+click+2: /* Toggle frame small <--> full-screen */
            if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setHighlight(false);
                gfm->toggleFullScreen(gframe);
                gframe = nullFrame;
                event(x, y, 0, mmove, 0); // Force update of frame selection
            }
            break;

        case b2+click+1: /* Toggle ROI or Gframe selection (or VS) */
        case b2+click+2:
        case b2+click+3:
        case b2+click+0:
            //case b1+ctrl+click+1:
            //case b1+ctrl+click+2:
            //case b1+ctrl+click+3:
            //case b1+ctrl+click+0:
            if (roi) {
                graph_batch(1);
                mouseHandled = true;
                if (roi->is_selected()) {
                    roi->deselect();
                    roi->refresh(ROI_COPY);
                } else {
                    roi->select(ROI_COPY, true); // Append
                }
                RoiStat::get()->calculate(false);
                graph_batch(0);
            } else if (mask == b2+click+1&& gf != nullFrame &&view != nullView) {
                graph_batch(1);
                mouseHandled = true;
                gf->quickVs(x, y, true);
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        case b2+ctrl+click+1: /* Auto VS */
            if (gf != nullFrame && view != nullView) {
                graph_batch(1);
                mouseHandled = true;
                gf->autoVs();
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        case b2+down:
        case b2+ctrl+down:
            // Prepare for interactive VS adjustment
            if (/*gframe == nullFrame &&*/gf != nullFrame && view != nullView) {
                if ((gframe == nullFrame && roi == NULL) || (mask & ctrl)) {
                    GraphicsWin::setCursor("intensity");
                }
                vsframe = gf;
                vsChanged = false;
                mouseHandled = true;
                gf->startInteractiveVs(x, y);
            }
            break;

        case b2+drag:
        case b2+ctrl+drag:
            // Do interactive VS adjustment
            if (vsframe != nullFrame) {
                if (GraphicsWin::getCursor() != "intensity2") {
                    GraphicsWin::setCursor("intensity2");
                }
                graph_batch(1);
                vsChanged = true;
                mouseHandled = true;
                ReviewQueue::get()->setDragging(true);
                vsframe->doInteractiveVs(x, y);
                graph_batch(0);
            }
            break;

        case b2+up:
        case b2+ctrl+up:
            // Complete interactive VS adjustment
            GraphicsWin::setCursor("pointer");
            if (vsframe != nullFrame && vsChanged) {
                graph_batch(1);
                mouseHandled = true;
                ReviewQueue::get()->setDragging(false);
                vsframe->finishInteractiveVs();
                VsInfo::setVsDifference();
                graph_batch(0);
                vsframe = nullFrame;
            }
            break;

        case b1+drag: // ** Move or modify ROI or control crosshair cursor
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
            if (!(mask & shift) && VolData::get()->showingObliquePlanesPanel() 
		&& OrthoSlices::getShowCursors()) {
                 if(oslice->extractCursorMoved(x, y)){
                     mouseHandled = true;
                     break;
                 }
            }           
            if (roi && handle < 0&& (mask & shift)) {
                roi->setRollover(true);
                //handle = -1;
            }
            if (roi) {
                mouseHandled = true;
                if (handle >= 0) {
                    int op = ((mask & shift) ? Roi::movePointConstrained
                            : Roi::movePoint);
                    roi->create(x, y, op);
                } else {
                    roi->move(x, y);
                }
                RoiStat::get()->calculate(true);
                if(gf != nullFrame && showOver) {
		  gf->draw();
		  Roitype type = roi->GetType();
                  if(showIntensity) {
		     roi->showIntensity();
                  } else if(handle != -1) {
                     if(type == ROI_LINE) {
		       double x1,y1,x2,y2;
		       if(roi->getFirstLast(x1,y1,x2,y2) == 2)
		       AxisInfo::showDistance(gf,(int)x1, (int)y1, (int)x2, (int)y2);
		     } else {
		        //double x1,y1;
		        //if(roi->getHandlePoint(roi->getHandle(x,y), x1,y1) == 1) 
		        //AxisInfo::showPosition(gf,(int)x1,(int)y1,true,false);
		        AxisInfo::showPosition(gf,x,y,true,false);
		     }
		  } else if(type == ROI_POINT) {
		    double x1,y1;
		    if(roi->getHandlePoint(0, x1,y1) == 1) 
		    AxisInfo::showPosition(gf,(int)x1,(int)y1,true,false);
		  } else if(type == ROI_LINE) {
		    double x1,y1,x2,y2;
		    if(roi->getFirstLast(x1,y1,x2,y2) == 2)
		    AxisInfo::showDistance(gf,(int)x1, (int)y1, (int)x2, (int)y2);
		  }
		}
            } 
            break;

        case b3+click+1: /* Delete ROI */
            if (gf != nullFrame && roi) {
                graph_batch(1);
                if (handle < 0) {
                    bool b = roim->isLastDisplayedBoundRoi(roi);
                    roim->remove(roi);
                    if (b)
                        roim->removeNotDisplayedBoundRoi(roi);
                    roi = NULL;
                    gf->draw(); // Redraw just this image
                } else {
                    roi->activateSlaves();
                    roi->create(x, y, Roi::deletePoint);
                    roi->create_done(); // Deletes any duplicate polygon points

                    // NB: following makes "roi" out-of-date (maybe null)
                    event(x, y, 0, mmove, 0); // Force update of selection

                }
                graph_batch(0);
                RoiStat::get()->calculate();
            }
            break;

        case b3+shift+click+1: /* Delete all ROIs */
            if (gf != nullFrame && roi) {
                graph_batch(1);
                if (handle < 0) {
                    roim->removeAllRois();
                    roi = NULL;
                }
                RoiStat::get()->calculate();
                gfm->draw(); // Redraw all images
                graph_batch(0);
            }
            break;

        case b3+ctrl+click+1: /* Delete all bound ROIs */
            if (gf != nullFrame && roi) {
                if (handle < 0) {
                    graph_batch(1);
                    roi->activateSlaves();
                    roim->addActive(roi);
                    roim->removeActiveRois();
                    roi = NULL;
                    RoiStat::get()->calculate();
                    gfm->draw(); // Redraw all images
                    graph_batch(0);
                }
            }
            break;
        }
        break;

    case vs:
        switch (mask) {
        case mmove: 
          GraphicsWin::setCursor("intensity");
          break;
        case b1+click+1: /* Set max intensity */
        case b2+click+1:
            if (gf != nullFrame && view != nullView) {
                graph_batch(1);
                mouseHandled = true;
                gf->quickVs(x, y, true);
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        case b2+ctrl+click+1: /* Auto VS */
        case b1+ctrl+click+1:
            if (gf != nullFrame && view != nullView) {
                graph_batch(1);
                mouseHandled = true;
                gf->autoVs();
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        case b3+click+1: /* Set min intensity */
            if (gf != nullFrame && view != nullView) {
                graph_batch(1);
                mouseHandled = true;
                gf->quickVs(x, y, false);
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        case b2+down:
        case b1+down:
            // Prepare for interactive VS adjustment
            if (gf != nullFrame && view != nullView) {
                graph_batch(1);
                gframe = gf;
                mouseHandled = true;
                gf->startInteractiveVs(x, y);
                graph_batch(0);
            }
            break;

        case b2+drag:
        case b1+drag:
            // Do interactive VS adjustment
            if (gframe != nullFrame) {
                if (GraphicsWin::getCursor() != "intensity2") {
                    GraphicsWin::setCursor("intensity2");
                }
                graph_batch(1);
                mouseHandled = true;
                ReviewQueue::get()->setDragging(true);
                gframe->doInteractiveVs(x, y);
                graph_batch(0);
            }
            break;

        case b2+up:
        case b1+up:
            // Complete interactive VS adjustment
            GraphicsWin::setCursor("intensity");
            if (gframe != nullFrame) {
                graph_batch(1);
                mouseHandled = true;
                ReviewQueue::get()->setDragging(false);
                gframe->finishInteractiveVs();
                VsInfo::setVsDifference();
                graph_batch(0);
            }
            break;

        }
        break;

    case zoom:
        switch (mask) {
        case mmove: 
          GraphicsWin::setCursor("zoom");
          break;
        case b1+click+1:
        case b1+click+2:
        case b1+click+3:
        case b1+click+4:
            if (gf != nullFrame) {
                graph_batch(1);
                mouseHandled = true;
                double zoomFactor = getReal("aipZoomFactor", 1.41421356);
                gf->quickZoom(x, y, zoomFactor);
                graph_batch(0);
                if(view != nullView) VsInfo::setVsHistogram();
            }
            break;

        case b3+click+1:
        case b3+click+2:
        case b3+click+3:
        case b3+click+0:
            if (gf != nullFrame) {
                graph_batch(1);
                mouseHandled = true;
                double zoomFactor = getReal("aipZoomFactor", 1.41421356);
                gf->quickZoom(x, y, 1/zoomFactor);
                graph_batch(0);
                if(view != nullView) VsInfo::setVsHistogram();
            }
            break;

        case b2+shift+down:
        case b2+down:
        case b1+shift+down:
            if (gf != nullFrame) {
                mouseHandled = true;
                setState(pan);
                gframe = gf;
                if(view != nullView) gf->setBase(x, y);
            }
            break;

        case b1+ctrl+click+1:
        case b2+ctrl+click+1:
        case b3+ctrl+click+1:
            if (gf != nullFrame) {
                graph_batch(1);
                mouseHandled = true;
                gf->resetZoom();
                graph_batch(0);
            }
            break;
        }
        break;

    case pan:
        switch (mask & ~(ctrl+alt+meta)) { // Ignore these modifier keys
        case mmove: 
          GraphicsWin::setCursor("zoom");
          break;
        case b2+shift+drag:
            if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setOverlayPan(x, y, true);
            }
            break;

        case b2+shift+up:
            if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setOverlayPan(x, y, false);
                gframe = nullFrame;
                setState(zoom);
                VsInfo::setVsHistogram();
            }
            break;

        case b2+drag:
        case b1+shift+drag:
        case b1+drag:
            if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setPan(x, y, true);
            }
            break;

        case b2+up:
        case b1+shift+up:
        case b1+up:
            if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setPan(x, y, false);
                gframe = nullFrame;
                setState(zoom);
                VsInfo::setVsHistogram();
            }
            break;
        }
        break;

    case createPoint:
    case createLine:
    case createBox:
    case createOval:
    case createPolygon:
    case createPolyline:
        switch (mask) {
        case mmove: // display position or distance 
          GraphicsWin::setCursor("pencil");
          if(gf != nullFrame && showROIlabel && showIntensity) {
	      gfm->draw(); // cleanup 
	      AxisInfo::showIntensity(gf,x,y,true);
          } else if(gf != nullFrame && showROIlabel) {
	      gfm->draw(); // cleanup 
	      AxisInfo::showPosition(gf,x,y,true,true);
	  } else if(gf != nullFrame && showOver) {
	      gfm->draw(); // cleanup
          }
          break;
        case b1+down:
        case b1+ctrl+down:
        case b1+shift+down:
        case b1+ctrl+shift+down:
            restoreState = (mask & ctrl) ? state : select;
            if (gf != nullFrame && view != nullView) {
                mouseHandled = true;
                switch (state) {
                case createPoint:
                    roi = new Point(gf, x, y);
                    setState(modifyPoint);
                    break;
                case createLine:
                    roi = new Line(gf, x, y);
                    setState(modifyLine);
                    break;
                case createBox:
                    roi = new Box(gf, x, y);
                    setState(modifyBox);
                    break;
                case createOval:
                    roi = new Oval(gf, x, y);
                    setState(modifyOval);
                    break;
                case createPolygon:
                    roi = new Polygon(gf, x, y);
                    setState(modifyPolygon);
                    break;
                case createPolyline:
                    roi = new Polyline(gf, x, y);
                    setState(modifyPolyline);
                    break;
                }
                if (roi != NULL) {
                    roi->setRolloverHandle(1);
                    RoiStat::get()->calculate(true);
                }
            }
            break;
        }
        break;

    case modifyPoint:
    case modifyLine:
    case modifyBox:
    case modifyOval:
        switch (mask) {
        case b1+drag:
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
            if (roi) {
                mouseHandled = true;
                int op = ((mask & shift) ? Roi::movePointConstrained
                        : Roi::movePoint);
                roi->create(x, y, op);
                RoiStat::get()->calculate(true);
                if(gf != nullFrame && showROIlabel && !showIntensity) {
		   gf->draw();
		   Roitype type = roi->GetType();
		   if(type == ROI_LINE) {
		      double x1,y1,x2,y2;
		      if(roi->getFirstLast(x1,y1,x2,y2) == 2)
		      AxisInfo::showDistance(gf,(int)x1, (int)y1, (int)x2, (int)y2);
		   } else {
                      AxisInfo::showPosition(gf,x,y,true,false);
		   }
                }
            }
            break;

        case b1+up:
        case b1+ctrl+up:
        case b1+shift+up:
        case b1+ctrl+shift+up:
            if (roi) {
                mouseHandled = true;
                roi->setRollover(false);
                roi->create_done();
                roi->someInfo(false);
                RoiStat::get()->calculate(false);
            }
            roi = NULL;
            setState(restoreState);
            break;

        }
        break;

    case modifyPolygon:
    case modifyPolyline:
        switch (mask) {

        default:
        case b1+mmove: // ** Move the current point around
        case b1+ctrl+mmove:
        case b1+shift+mmove:
        case b1+ctrl+shift+mmove:
            if (roi && x >= 0) { // Negative x is actually an exit event
                mouseHandled = true;
                roi->create(x, y, Roi::movePoint);
                RoiStat::get()->calculate(true);
                if(gf != nullFrame && showROIlabel && !showIntensity) {
		  gf->draw();
                  AxisInfo::showPosition(gf,x,y,true,false);
		}
            }
            break;

        case b1+drag: // ** Create new polygon points as needed
        case b1+ctrl+drag:
        case b1+shift+drag:
        case b1+ctrl+shift+drag:
            if (roi) {
                mouseHandled = true;
                roi->create(x, y, Roi::dribblePoints);
                RoiStat::get()->calculate(true);
                if(gf != nullFrame && showROIlabel && !showIntensity) {
		  gf->draw();
                  AxisInfo::showPosition(gf,x,y,true,false);
		}
            }
            break;

        case b1+down: // ** Tack down the point we're moving
        case b1+ctrl+down:
        case b1+shift+down:
        case b1+ctrl+shift+down:
            if (roi) {
                mouseHandled = true;
                roi->create(x, y, Roi::nextPoint);
                roi->someInfo(false);
                RoiStat::get()->calculate(false);
            }
            break;

        case b1+click+2: // ** Finish off polygon creation
        case b1+ctrl+click+2:
        case b1+shift+click+2:
        case b1+ctrl+shift+click+2:
        case b2+click+1:
        case b2+ctrl+click+1:
        case b2+shift+click+1:
        case b2+ctrl+shift+click+1:
            if (roi) {
                mouseHandled = true;
                roi->create_done();
                roi->setRolloverHandle(-1);
                roi->someInfo(false);
                RoiStat::get()->calculate(false);
            }
            roi = NULL;
            setState(restoreState);
            break;

        }
        break;

    case imageMath:
        switch (mask) {
        case mmove:
            if (gf != nullFrame) { // Highlight the frame we are in
                mouseHandled = true;
                if (gframe != nullFrame && gframe != gf) {
                    gframe->setHighlight(false);
                }
                gframe = gf;
                gframe->setHighlight(true);
            } else if (gframe != nullFrame) {
                mouseHandled = true;
                gframe->setHighlight(false);
                gframe = nullFrame;
            }
            break;

        case b1+up:
            int frame = gf->getGframeNumber();
            if (frame < 0)
                break;

            mouseHandled = true;
            char str[32];
            sprintf(str, "%d", frame);
            WinMath::get()->math_insert(str, true);
            break;
        }
        break;
    case selectCSIvox:
        mouseHandled = true;
        switch (mask) {
        case b1+down:
        case b1+ctrl+down:
        case b1+shift+down:
        case b1+ctrl+shift+down:
            if (gf != nullFrame) {
		gf->setPositionInfo(x, y);
            }
            if (gf != nullFrame) gf->selectSpecView(x, y, mask);
            break;
        case b1+up:
            break;
        }
        break;
    }

    // Modeless stuff that can happen if nobody else did anything
    if (!mouseHandled) {
        switch (mask) {
        case b1+ctrl+shift+click+1:
        case b1+ctrl+shift+click+2:
        case b1+ctrl+shift+click+3:
        case b1+ctrl+shift+click+0:
            if (gf != nullFrame) {
                mouseHandled = true;
                gf->setSelect(true, false);
                //WinMovie::get()->stepMovie(-1);
                VsInfo::setVsHistogram();
            }
            break;

        case b3+ctrl+shift+click+1:
        case b3+ctrl+shift+click+2:
        case b3+ctrl+shift+click+3:
        case b3+ctrl+shift+click+0:
            if (gf != nullFrame) {
                mouseHandled = true;
                gf->setSelect(true, false);
                //WinMovie::get()->stepMovie(1);
                VsInfo::setVsHistogram();
            }
            break;
        }
    }

    // See if anyone else is interested
    if (!mouseHandled && state == select) {
        aipCallMouseListeners(x, y, button, mask, dummy);
    }
}

/* STATIC */
void Mouse::reset() {
    static int reentryCount = 0;
    roi = NULL;
    if (++reentryCount <= 1) {
        if (aipLockScreen()) {
            // Don't let anyone release the mouse
            // NB: grabMouse() may end up calling this, only avoid infinite
            // recursion here because the "mouseReturn" pointer always gets
            // nulled out before the function is called.
            // --- Added a recursion counter for safety - CMP: 2002 July 9
            grabMouse();
        } else {
            // Let the mouse go; switch cursor to default
            setState(notOwner);
        }
    }
    --reentryCount;
}

