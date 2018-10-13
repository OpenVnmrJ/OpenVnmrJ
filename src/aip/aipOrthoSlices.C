/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <netinet/in.h>

#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipImgInfo.h"
#include "aipStderr.h"
#include "aipDataManager.h"
#include "aipDataInfo.h"
#include "aipViewInfo.h"
#include "aipVolData.h"
#include "aipGraphicsWin.h"
#include "aipOrthoSlices.h"

extern "C" {
int graphToVnmrJ(char*, int);
void set3Pcursor(int i, int j, int x1, int y1, int x2, int y2);
void update3PCursors();
int onActivePlan(int x, int y);
int onVoxelPlan(int x, int y);
void unselectAndRedrawPlan();
#include "group.h"
#include "graphics.h"
}
void vnmrj_dstring(int, int, int, int, int, char *);

#ifdef LINUX
extern void sendVpntToVnmrj(float x, float y, float z, int f);
extern void setVpnt(float x, float y, float z, float f);
extern void setVrot(float x, float y, float z);
extern void setVflt(int index, float value);
#endif

// Current crosshair cursor positions
static double curDataX=0, curDataY=0, curDataZ=0;
// Dimensionality of current data
static int fast=0, med=0, slow=0;
static double xmax, ymax, zmax;
static float eulers[3][3];
static float rotangles[3];
static int mipflag=0;
static int dframes=0;
static double initX=0, initY=0;
static int gf_frame=0;
static double imwd, imht;
static spGframe_t gf_selected;
static bool ydrag=false;
static bool xdrag=false;
static bool dragging=false;
static bool need_refresh=false;
static int frameCursors[4][2];
static int neuro;
static int radio;
static int pixel_order;
static int x_xaxis, x_yaxis, y_xaxis, y_yaxis, z_xaxis, z_yaxis;
bool OrthoSlices::showCursors;
#define X 0
#define Y 1

// extern C function called from graphics_win.c after screen is resized
void update3PCursors() {
    VolData *vdat=VolData::get();
    if (vdat->showingObliquePlanesPanel() && vdat->validData())
        OrthoSlices::get()->updateCursors();
}

OrthoSlices* OrthoSlices::orthoSlices= NULL;

OrthoSlices::OrthoSlices(void) {
    // Set default euler angles
    // order is axial, sagittal, coronal which corresponds to sliceZ, slice X, sliceY

    eulers[0][0] = 0.0;
    eulers[0][1] = 0.0;
    eulers[0][2] = 0.0;
    eulers[1][0] = 90.0;
    eulers[1][1] = 90.0;
    eulers[1][2] = 90.0;
    eulers[2][0] = 180.0;
    eulers[2][1] = 0.0;
    eulers[2][2] = 90.0;
    
    eulers[0][0] = 1.0;
	eulers[0][1] = 1.0;
	eulers[0][2] = 1.0;
	eulers[1][0] = 2.0;
	eulers[1][1] = 2.0;
	eulers[1][2] = 2.0;
	eulers[2][0] = 3.0;
	eulers[2][1] = 3.0;
	eulers[2][2] = 3.0;


    rotangles[0]=0.0;
    rotangles[1]=0.0;
    rotangles[2]=0.0;

    // We want to be able to tell if these have been set
    fast = med = slow = 0;
    dframes=0;
    showCursors=true;
}

//-------------------------------------------------------------
// OrthoSlices *OrthoSlices::get()
//-------------------------------------------------------------
// Returns the one and only OrthoSlices instance.  
// (Creates one if it has not been created yet).
// - PUBLIC STATIC
//-------------------------------------------------------------
OrthoSlices *OrthoSlices::get() {
    if (!orthoSlices) {
        orthoSlices = new OrthoSlices();
    }
    return orthoSlices;
}

//-------------------------------------------------------------
// OrthoSlices::updateCursors()
//-------------------------------------------------------------
// Update cursors. 
//-------------------------------------------------------------
void OrthoSlices::updateCursors() {
    OrthoSlices::get()->drawThreeCursors((int) curDataX, (int) curDataY, (int) curDataZ);
}

//-------------------------------------------------------------
// OrthoSlices::StartExtract(int argc, char **argv, int, char **)
//-------------------------------------------------------------
// Begin 3 Orthoganal Planes extraction. 
//-------------------------------------------------------------
int OrthoSlices::StartExtract(int argc, char **argv, int, char **) {
    frameCursors[0][X] = 0;
    char usage[100];
    char str[100];
    double rot[9];
    double rmax;
    int base_orient;
    int rot_90=FALSE;
    
    VolData *vdat = VolData::get();
    OrthoSlices *os = OrthoSlices::get();
    mipflag=-1;

    if (vdat->dataInfo == (spDataInfo_t)NULL) { // No volume data loaded yet
        sprintf(usage, "No Volume Data Loaded");
        ib_errmsg(usage);
        return 0;
    }
    slow=vdat->dataInfo->getSlow();
    med=vdat->dataInfo->getMedium();
    fast=vdat->dataInfo->getFast();
    xmax=fast-1;
    ymax=med-1;
    zmax=slow-1;
    
    // get the orientation of the 3D acquisition
    (void)vdat->dataInfo->GetOrientation(rot);
    base_orient=AXIAL;
    if((fabs(rot[1]) > fabs(rot[0])) && (fabs(rot[1]) > fabs(rot[2])))
        rot_90=TRUE;
 
    rmax=fabs(rot[8]);
    if(fabs(rot[6])>rmax)
    {
            base_orient=CORONAL;
            if((fabs(rot[1]) > fabs(rot[0])) && (fabs(rot[1]) > fabs(rot[2])))
                    rot_90=TRUE;            
            rmax=fabs(rot[6]);
    }
    if(fabs(rot[7])>rmax)
    {
              base_orient=SAGITTAL;
              if((fabs(rot[0]) > fabs(rot[1])) && (fabs(rot[0]) > fabs(rot[2])))
                      rot_90=TRUE;              
              rmax=fabs(rot[7]);
    } 

    neuro=FALSE;
    radio=FALSE;
    pixel_order=FALSE;
    if (P_getstring(CURRENT, "aipRotationPolicy", str, 1, 100) < 0)
           if (P_getstring(GLOBAL, "aipRotationPolicy",str, 1, 100) < 0)
           {             
                   ib_errmsg("aipStartExtract cannot read aipRotationPolicy");
                   return 0;               
           }
    
    if (strstr(str, "neuro"))
        neuro=TRUE;
    else if (strstr(str, "radio"))
        radio=TRUE;
    else if (strstr(str, "none"))
        pixel_order=TRUE;
    else {
        ib_errmsg("aipStartExtract: invalid aipRotationPolicy");
        return 0;
    }
             
   //this section determines how everything behaves!!        
    // Note: base_orient (the cases) indicate orientation of the original 3D acquisition
    // parameters such as (eg) z_xaxis indicates display color/direction
    //  of the xaxis in the z image
    // z means "front plane", x means "side plane" and y means "top plane"
   switch (base_orient) {
    case AXIAL:
        if (rot_90) {
            if (neuro) {
                z_xaxis = X_MINUS;
				z_yaxis = Y_PLUS;
				x_xaxis = Y_PLUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_MINUS;
				y_yaxis = Z_PLUS;
			} else if (radio) {
				z_xaxis = X_PLUS;
				z_yaxis = Y_PLUS;
				x_xaxis = Y_PLUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_PLUS;
				y_yaxis = Z_PLUS;

			} else if (pixel_order) {
				z_xaxis = X_PLUS;
				z_yaxis = Y_PLUS;
				x_xaxis = Y_PLUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_PLUS;
				y_yaxis = Z_PLUS;
			}
        } else {
            if (neuro) {
				z_xaxis = Y_MINUS;
				z_yaxis = X_MINUS;
				x_xaxis = Y_MINUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_MINUS;
				y_yaxis = Z_PLUS;
			} else if (radio) {
				z_xaxis = Y_PLUS;
				z_yaxis = X_PLUS;
				x_xaxis = Y_PLUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_PLUS;
				y_yaxis = Z_PLUS;
			} else if (pixel_order) {
				z_xaxis = X_PLUS;
				z_yaxis = Y_MINUS;
				x_xaxis = Y_PLUS;
				x_yaxis = Z_PLUS;
				y_xaxis = X_PLUS;
				y_yaxis = Z_PLUS;
            }
        }
        break;

    case SAGITTAL:
        if (rot_90) {
            if (neuro) {
                z_xaxis=X_MINUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Z_PLUS;
                x_yaxis=Y_PLUS;
                y_xaxis=Z_PLUS;
                y_yaxis=X_MINUS;
            } else if (radio) {
                z_xaxis=X_MINUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Z_MINUS;
                x_yaxis=Y_PLUS;
                y_xaxis=Z_MINUS;
                y_yaxis=X_MINUS;
            } else if (pixel_order) {
                z_xaxis=X_PLUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Y_PLUS;
                x_yaxis=Z_PLUS;
                y_xaxis=X_PLUS;
                y_yaxis=Z_PLUS;
            }
        } else {
            if (neuro) {
                z_xaxis=Y_MINUS;
                z_yaxis=X_MINUS;
                x_xaxis=Z_MINUS;
                x_yaxis=Y_MINUS;
                y_xaxis=Z_MINUS;
                y_yaxis=X_MINUS;
            } else if (radio) {
                z_xaxis=Y_MINUS;
                z_yaxis=X_MINUS;
                x_xaxis=Z_MINUS;
                x_yaxis=Y_MINUS;
                y_xaxis=Z_MINUS;
                y_yaxis=X_MINUS;
            } else if (pixel_order) {
                z_xaxis=X_PLUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Y_PLUS;
                x_yaxis=Z_PLUS;
                y_xaxis=X_PLUS;
                y_yaxis=Z_PLUS;
            }
        }
        break;

    case CORONAL:
        if (rot_90) {
            if (neuro) {
                z_xaxis=X_MINUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Z_MINUS;
                x_yaxis=Y_PLUS;
                y_xaxis=X_MINUS;
                y_yaxis=Z_MINUS;
            } else if (radio) {
                z_xaxis=X_PLUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Z_MINUS;
                x_yaxis=Y_PLUS;
                y_xaxis=X_PLUS;
                y_yaxis=Z_MINUS;
            } else if (pixel_order) {
                z_xaxis=X_PLUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Y_PLUS;
                x_yaxis=Z_PLUS;
                y_xaxis=X_PLUS;
                y_yaxis=Z_PLUS;
            }
        } else {
            if (neuro) {
                z_xaxis=Y_MINUS;
                z_yaxis=X_MINUS;
                x_xaxis=Y_MINUS;
                x_yaxis=Z_MINUS;
                y_xaxis=Z_MINUS;
                y_yaxis=X_MINUS;
            } else if (radio) {
                z_xaxis=Y_PLUS;
                z_yaxis=X_MINUS;
                x_xaxis=Y_PLUS;
                x_yaxis=Z_MINUS;
                y_xaxis=Z_MINUS;
                y_yaxis=X_MINUS;
            } else if (pixel_order) {
                z_xaxis=X_PLUS;
                z_yaxis=Y_PLUS;
                x_xaxis=Y_PLUS;
                x_yaxis=Z_MINUS;
                y_xaxis=X_PLUS;
                y_yaxis=Z_PLUS;
            }
        }
        break;
    }
         
       
     
    
    // If not data has been loaded, we need to bail out
    if (slow <= 0)
        return 0;

    if (argc != 1 && argc !=2 && argc != 10 && argc != 11) {
        sprintf(usage, "usage: %s", "aipStartExtract (9 angles [MIPflag] )");
        ib_errmsg(usage);
        return 0;
    }
    // Get euler angles from args
    if (argc >= 10) {
        for (int i=0; i < 3; i++) {
            for (int k=0; k < 3; k++) {
                // skip argv[0] and get the next nine values
                eulers[i][k] = atof(argv[i*3 + k + 1]);
            }
        }
        if (argc==11)
            mipflag=atoi(argv[10]);
    } else if (argc == 2) {
        mipflag=atoi(argv[1]);
    }

    if (mipflag<0) { // mip flag not passed in, get it from g3di[10]
        /* g3di[10] controls MIP in 3D graphics window */
        double temp=0;
        mipflag=0;
        if (P_getreal(GLOBAL, "g3di", &temp, 10)==0) {
            mipflag=temp>0 ? 1 : 0;
        }
    }

    // deleteData clear DataMap
    ReviewQueue::get()->deleteData("all");

    // Display the first series of planes
    // **Need to find center slices and use here **
    // Set the initial slices to the center of each dimension
    double x=0.5, y=0.5, z=0.5;
    os->Getg3dpnt(x, y, z);

    curDataX = x*xmax;
    curDataY = y*ymax;
    curDataZ = z*zmax;
    
    vdat->showObliquePlanesPanel(true);

	GframeManager::get()->splitWindow(1,3);
   
	ydrag=false;
	xdrag=false;
	dragging=false;
	need_refresh=true;
	vdat->extractRotations[0]=-1;
	vdat->extractRotations[1]=-1;
	vdat->extractRotations[2]=-1;
	dframes=0;
	os->extractPlane(ZPLANE, (int) curDataZ);
	os->extractPlane(XPLANE, (int) curDataX);
	os->extractPlane(YPLANE, (int) curDataY);

	dframes=GframeManager::get()->getNumberOfFrames();
	os->drawThreeCursors((int) curDataX, (int) curDataY, (int) curDataZ);
        return 0;
}

//-------------------------------------------------------------
// OrthoSlices::updateDisplayMode()
//-------------------------------------------------------------
void OrthoSlices::updateDisplayMode() {
    GframeManager *gfm = GframeManager::get();
    VolData *vdat=VolData::get();
    int nframes=gfm->getNumberOfFrames();
    if (vdat->showingObliquePlanesPanel()) {
        if (dframes>0 && (nframes<3 || nframes>4))vdat->showObliquePlanesPanel(false);
    }
}

//-------------------------------------------------------------
// OrthoSlices::extractPlane(int type, int index)
//-------------------------------------------------------------
// Extract the single plane specified by index
//-------------------------------------------------------------
void OrthoSlices::extractPlane(int type, int index) {
    VolData *vdat = VolData::get();
    int slicelist[1];
    slicelist[0]=index;
    vdat->extract_oblplanes(eulers[type-1], rotangles, 1, slicelist, mipflag, 0);
}

//-------------------------------------------------------------
// OrthoSlices::drawThreePlanes(int sliceX, int sliceY, int sliceZ)
//-------------------------------------------------------------
// Extract and draw the three slices as specified
//-------------------------------------------------------------
void OrthoSlices::drawThreePlanes(int sliceX, int sliceY, int sliceZ) {
    //char str[60];
    GframeManager* gfm = GframeManager::get();
	gfm->clearFrameCache();

    extractPlane(ZPLANE, sliceZ);
    extractPlane(XPLANE, sliceX);
    extractPlane(YPLANE, sliceY);


    drawThreeCursors(sliceX, sliceY, sliceZ);
    //sprintf(str,"# cached frames:%d",gfm->getNumberOfCachedFrames());
    //ib_errmsg(str);
    need_refresh=false;
}

//-------------------------------------------------------------
// OrthoSlices::redraw()
//-------------------------------------------------------------
// redraw ortho planes
//-------------------------------------------------------------
void OrthoSlices::redraw() {
    VolData *vdat=VolData::get();
    if (vdat->showingObliquePlanesPanel()) {
        drawThreePlanes((int) curDataX, (int) curDataY, (int) curDataZ);
    }
}

//-------------------------------------------------------------
// OrthoSlices::drawThreeCursors(int dataX, int dataY, int dataZ)
//-------------------------------------------------------------
// Draw the three crosshair cursors given the xyz position of the point. 
// x, y and z are in data points units
// TODO: set colors based on aipRotationPolicy
//       (current values correspond to 'none' case)
//-------------------------------------------------------------
void OrthoSlices::drawThreeCursors(int dataX, int dataY, int dataZ) {

    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    spGframe_t gf;
    spImgInfo_t img;
    int frame, row, col;
    double pixX, pixY, pixZ;
    double newX, newY, newZ;
    int xframe, yframe;
    int xst, xend, yst, yend;
    int xcolor=1, ycolor=2;

    if (gfm->getNumberOfFrames()<3) return;

    for (int i=1; i <= 3; i++) {
        if (i == 1)
            gf = gfm->getFirstFrame(gfi);
        else
            gf = gfm->getNextFrame(gfi);
        frame = gf->getGframeNumber();
        spViewInfo_t view = gf->getFirstView();
        if(view==nullView) continue;
        img = view->imgInfo;
        if(img==nullImg) continue;

        imwd = img->getDatawd();
        imht = img->getDataht();

        switch (frame) {
        case ZPLANE:
        {

            switch(z_xaxis)
            {
            case X_PLUS:
                newX=dataX*(imwd/fast);
                xcolor=YPLANE_COLOR;
                break;
            case X_MINUS:
                newX=(fast-dataX)*(imwd/fast);
                xcolor=YPLANE_COLOR;
                break;
            case Y_PLUS:
                 newX=dataY*(imwd/med);
                 xcolor=XPLANE_COLOR;
                 break;
             case Y_MINUS:
                 newX=(med-dataY)*(imwd/med);
                 xcolor=XPLANE_COLOR;
                 break;
             default:
                 newX=dataX*(imwd/fast);
                 xcolor=YPLANE_COLOR;
                 break;                 
            }
            switch(z_yaxis)
            {
            case X_PLUS:
                newY=dataX*(imht/fast);
                ycolor=YPLANE_COLOR;
                break;
            case X_MINUS:
                newY=(fast-dataX)*(imht/fast);
                ycolor=YPLANE_COLOR;
                break;
            case Y_PLUS:
                 newY=dataY*(imht/med);
                 ycolor=XPLANE_COLOR;
                 break;
             case Y_MINUS:
                 newY=(med-dataY)*(imht/med);
                 ycolor=XPLANE_COLOR;
                 break;
             default:
                 newY=dataX*(imht/fast);
                 ycolor=YPLANE_COLOR;
                 break;                 
            }
        }
        break;
            
        case XPLANE:
        {


             switch(x_xaxis)
             {
             case Y_PLUS:
                 newX=dataY*(imwd/med);
                 xcolor=ZPLANE_COLOR;
                 break;
             case Y_MINUS:
                 newX=(med-dataY)*(imwd/med);
                 xcolor=ZPLANE_COLOR;
                 break;
             case Z_PLUS:
                  newX=dataZ*(imwd/slow);
                  xcolor=YPLANE_COLOR;
                  break;
              case Z_MINUS:
                  newX=(slow-dataZ)*(imwd/slow);
                  xcolor=YPLANE_COLOR;
                  break;
              default:
                  newX=dataY*(imwd/med);
                  xcolor=ZPLANE_COLOR;
                  break;                 
             }
             switch(x_yaxis)
             {
             case Y_PLUS:
                   newY=dataY*(imht/med);
                   ycolor=ZPLANE_COLOR;
                   break;
               case Y_MINUS:
                   newY=(med-dataY)*(imht/med);
                   ycolor=ZPLANE_COLOR;
                   break;
               case Z_PLUS:
                    newY=dataZ*(imht/slow);
                    ycolor=YPLANE_COLOR;
                    break;
                case Z_MINUS:
                    newY=(slow-dataZ)*(imht/slow);
                    ycolor=YPLANE_COLOR;
                    break;
                default:
                    newY=dataY*(imht/med);
                    ycolor=ZPLANE_COLOR;
                    break;                      
             }
        }
        break;
             
        case YPLANE:
        {

             switch(y_xaxis)
             {
             case X_PLUS:
                 newX=dataX*(imwd/fast);
                 xcolor=ZPLANE_COLOR;
                 break;
             case X_MINUS:
                 newX=(fast-dataX)*(imwd/fast);
                 xcolor=ZPLANE_COLOR;
                 break;
             case Z_PLUS:
                  newX=dataZ*(imwd/slow);
                  xcolor=XPLANE_COLOR;
                  break;
              case Z_MINUS:
                  newX=(slow-dataZ)*(imwd/slow);
                  xcolor=XPLANE_COLOR;
                  break;
              default:
                  newX=dataX*(imwd/fast);
                  xcolor=ZPLANE_COLOR;
                  break;                 
             }
             switch(y_yaxis)
             {
             case X_PLUS:
                   newY=dataX*(imht/fast);
                   ycolor=ZPLANE_COLOR;
                   break;
               case X_MINUS:
                   newY=(fast-dataX)*(imht/fast);
                   ycolor=ZPLANE_COLOR;
                   break;
               case Z_PLUS:
                    newY=dataZ*(imht/slow);
                    ycolor=XPLANE_COLOR;
                    break;
                case Z_MINUS:
                    newY=(slow-dataZ)*(imht/slow);
                    ycolor=XPLANE_COLOR;
                    break;
                default:
                    newY=dataX*(imht/fast);
                    ycolor=ZPLANE_COLOR;
                    break;                 
               }   
            }   
        break;
        
        } // end of switch on frame
    
        int temprot=view->getRotation();
        view->setRotation(0);
        view->updateScaleFactors();        
        view->dataToPix((double)newX, (double)newY, pixX, pixY);
        view->setRotation(temprot);
        view->updateScaleFactors();
        
        frameCursors[0][X] = xframe = (int) pixX;
        frameCursors[0][Y] = yframe = (int) pixY;
        
        
         
        // Get the position of this frame in the viewport
        row = gf->row;
        col = gf->col;

        // Values in gf are frame sizes, values in view are data sizes
        // Calc the start and end of x
        xst = gf->pixwd * col +1;
        // End is number of col * frame size plus data (view) size
        //xend = gf->pixwd * col + view->pixwd-1;
        xend = xst + gf->pixwd - 1;
        // Calc the start and end of y
        yst = gf->pixht * row +1;
        // Don't draw ht of frame, draw ht of data
        //yend = gf->pixht * (row)-1 + view->pixht;
        yend = yst + gf->pixht - 1;
	if(showCursors) {
          set3Pcursor((frame-1)*2 + 0, xcolor, xst, yframe, xend, yframe);
          set3Pcursor((frame-1)*2 + 1, ycolor, xframe, yst, xframe, yend);
	} else {
          set3Pcursor((frame-1)*2 + 0, xcolor, 0,0,0,0);
          set3Pcursor((frame-1)*2 + 1, ycolor, 0,0,0,0);
	}
    }
}

//-------------------------------------------------------------
// OrthoSlices::startCursorMoved(int x, int y)
//-------------------------------------------------------------
// Called from aipMouse following a left mouse button press 
// event in one of the three ortho frames.  
// - Determine what frame we are in. 
//   (keep same frame while drag is active)
// - Get the initial horizontal/vertical crosshair position based
//   on which frame is selected and the value of g3dpnt.
// - If the initial mouse position is close to the horizontal 
//   or vertical crosshair position set the xdrag and/or ydrag 
//   flag true. 
//-------------------------------------------------------------
bool OrthoSlices::startCursorMoved(int x, int y) {
    double gx, gy, gz;
    double maxpixX=0;
    double maxpixY=0;
    int pix = 10;

    // Get the frameManager and find out from it which frame
    // is at the xy position where the mouse is located
    GframeManager *gfm = GframeManager::get();
    gf_selected = gfm->getGframeFromCoords(x, y);

    // Get the frame number. Starts at 1
    gf_frame = gf_selected->getGframeNumber();

    if ((gf_frame > 3)||(gf_frame<1)) {
        gf_frame=0;
        return false;
    }

    if(onVoxelPlan(x, y)) {
    	ydrag=false;
    	xdrag=false;
    	dragging=false;
	return false; 
    } else if(onActivePlan(x, y)) pix /= 2;

    Getg3dpnt(gx, gy, gz);

    curDataX = gx*xmax;
    curDataY = gy*ymax;
    curDataZ = gz*zmax;

   // gx*=xmax;
  //  gy*=ymax;
  //  gz*=zmax;
    // Get the new data row and column from the xy pixel info
    spViewInfo_t view = gf_selected->getFirstView();
    if(view==nullView) return false;
    spImgInfo_t img =view->imgInfo;
    if(img==nullImg) return false;
    imwd = img->getDatawd();
    imht = img->getDataht();
    
    double dx, dy;
    switch (gf_frame) {
    case ZPLANE:
        switch (z_xaxis){
        case X_PLUS:
            dx=gx; break;
        case X_MINUS:
             dx=1.0-gx; break;
        case Y_PLUS:
             dx=gy; break;
        case Y_MINUS:
             dx=1.0-gy; break;            
        }
        switch (z_yaxis){
        case X_PLUS:
            dy=gx; break;
        case X_MINUS:
             dy=1.0-gx; break;
        case Y_PLUS:
             dy=gy; break;
        case Y_MINUS:
             dy=1.0-gy; break;            
        }
        break;
    case XPLANE:
        switch (x_xaxis){
           case Y_PLUS:
               dx=gy; break;
           case Y_MINUS:
                dx=1.0-gy; break;
           case Z_PLUS:
                dx=gz; break;
           case Z_MINUS:
                dx=1.0-gz; break;            
           }
           switch (x_yaxis){
           case Y_PLUS:
               dy=gy; break;
           case Y_MINUS:
                dy=1.0-gy; break;
           case Z_PLUS:
                dy=gz; break;
           case Z_MINUS:
                dy=1.0-gz; break;            
           }
           break;
    case YPLANE:
         switch (y_xaxis){
            case X_PLUS:
                dx=gx; break;
            case X_MINUS:
                 dx=1.0-gx; break;
            case Z_PLUS:
                 dx=gz; break;
            case Z_MINUS:
                 dx=1.0-gz; break;            
            }
            switch (y_yaxis){
            case X_PLUS:
                dy=gx; break;
            case X_MINUS:
                 dy=1.0-gx; break;
            case Z_PLUS:
                 dy=gz; break;
            case Z_MINUS:
                 dy=1.0-gz; break;            
            }
            break;
    }                    
         
    
    int temprot=view->getRotation();
    view->setRotation(0);
    view->updateScaleFactors();
    view->dataToPix(dx*imwd, dy*imht, initX, initY);
    view->setRotation(temprot);
    view->updateScaleFactors();     
    
    
    ydrag=false;
    xdrag=false;
    //need_refresh=true;
    dragging=false;

    int delx=abs(x-(int)initX);
    int dely=abs(y-(int)initY);
    if (dely<pix)
        ydrag=true;
    if (delx<pix)
        xdrag=true;

    if (ydrag || xdrag)
        dragging=true;
    
    if(dragging) unselectAndRedrawPlan();

    return dragging;
}

//-------------------------------------------------------------
// OrthoSlices::extractCursorMoved(int x, int y)
//-------------------------------------------------------------
// The cursor has been dragged after a left mouse button press event 
// in one of the three ortho frames. 
// - Use the frame id determined by startCursorMoved above
// - Depending on what initial frame is selected, if ydrag is true 
//   one of the componants of g3dpnt tracks the vertical cursor.
// - If xdrag is true one of the 3 componants of g3dpnt tracks the 
//   vertical cursor.
//   Note: If the initial mouse position is close to the crosshair 
//         intersection point both xdrag and ydrag are set to true.
//
// - sendVpntToVnmrj in graphics3D.C passes the value of the newly
//   determined g3dpnt through the graphics socket to JGLComMgr.java.
//   JGLComMgr sets the slice planes in the 3D display (if open) and
//   then calls Setg3dpnt (though aipSetg3dpnt) which calls
//   drawThreePlanes, which draws the 2D graphics frames.
//-------------------------------------------------------------
bool OrthoSlices::extractCursorMoved(int xpos, int ypos) {
    double localDataX=0.0, localDataY=0.0;
    double gx, gy, gz;

    if ((gf_frame > 3)||(gf_frame<1) || !dragging)
        return false;

    int x=xpos;
    int y=ypos;

    Getg3dpnt(gx, gy, gz);

    if (!ydrag)
        y= (int) initY;
    if (!xdrag)
        x= (int) initX;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // Get the new data row and column from the xy pixel info
    spViewInfo_t view = gf_selected->getFirstView();
    if(view==nullView) return false;
    spImgInfo_t img =view->imgInfo;
    if(img==nullImg) return false;
    imwd = img->getDatawd();
    imht = img->getDataht();
 
    int temprot=view->getRotation();
    view->setRotation(0);
    view->updateScaleFactors();
    view->pixToData((double)x, (double)y, localDataX, localDataY);
    view->setRotation(temprot);
    view->updateScaleFactors();         
        
    localDataX /= imwd;
    localDataY /= imht;
    
    switch (gf_frame) {
    case ZPLANE:
        if (xdrag) {
            switch (z_xaxis) {
            case X_PLUS:
                gx=localDataX;
                break;
            case X_MINUS:
                gx=(1.0-localDataX);
                break;
            case Y_PLUS:
                gy=localDataX;
                break;
            case Y_MINUS:
                gy=(1.0-localDataX);
                break;
            }
        }
        if (ydrag) {
            switch (z_yaxis) {
            case X_PLUS:
                gx=localDataY;
                break;
            case X_MINUS:
                gx=(1.0-localDataY);
                break;
            case Y_PLUS:
                gy=localDataY;
                break;
            case Y_MINUS:
                gy=(1.0-localDataY);
                break;
            }
        }
        break;
    case XPLANE:
        if (xdrag) {
            switch (x_xaxis) {
            case Y_PLUS:
                gy=localDataX;
                break;
            case Y_MINUS:
                gy=(1.0-localDataX);
                break;
            case Z_PLUS:
                gz=localDataX;
                break;
            case Z_MINUS:
                gz=(1.0-localDataX);
                break;
            }
        }
        if (ydrag) {
            switch (x_yaxis) {
            case Y_PLUS:
                gy=localDataY;
                break;
            case Y_MINUS:
                gy=(1.0-localDataY);
                break;
            case Z_PLUS:
                gz=localDataY;
                break;
            case Z_MINUS:
                gz=(1.0-localDataY);
                break;
            }
        }
        break;
    case YPLANE:
        if (xdrag) {
            switch (y_xaxis) {
            case X_PLUS:
                gx=localDataX;
                break;
            case X_MINUS:
                gx=(1.0-localDataX);
                break;
            case Z_PLUS:
                gz=localDataX;
                break;
            case Z_MINUS:
                gz=(1.0-localDataX);
                break;
            }
        }
        if (ydrag) {
            switch (y_yaxis) {
            case X_PLUS:
                gx=localDataY;
                break;
            case X_MINUS:
                gx=(1.0-localDataY);
                break;
            case Z_PLUS:
                gz=localDataY;
                break;
            case Z_MINUS:
                gz=(1.0-localDataY);
                break;
            }
        }
        break;
    }

    gx=gx>1.0 ? 1.0 : gx;
    gy=gy>1.0 ? 1.0 : gy;
    gz=gz>1.0 ? 1.0 : gz;

    gx=gx<0.0 ? 0.0 : gx;
    gy=gy<0.0 ? 0.0 : gy;
    gz=gz<0.0 ? 0.0 : gz;

#ifdef LINUX
    sendVpntToVnmrj(gx, gy, gz, gf_frame);
#endif
    return true;
}

//-------------------------------------------------------------
// OrthoSlices::endCursorMoved(int x, int y) 
//-------------------------------------------------------------
// Called from aipMouse following a left mouse button up 
// event in one of the three ortho frames.  
// - Clear dragging flag
//-------------------------------------------------------------
bool OrthoSlices::endCursorMoved(int x, int y) {
    bool was_dragging=dragging;
    dragging=false;
    return was_dragging;
}

//-------------------------------------------------------------
// OrthoSlices::MipMode() 
//-------------------------------------------------------------
// return:
// 1 if in MIP mode
// 0 if in VOL mode
//-1 if param error
//-------------------------------------------------------------
int OrthoSlices::MipMode() {
    double value=0;
    if (P_getreal(GLOBAL, "g3di", &value, 10)>=0) {
        return (value>0) ? 1 : 0;
    }
    return -1;
}

//-------------------------------------------------------------
// OrthoSlices::WidthEnabled() 
//-------------------------------------------------------------
// return:
// 1 if slab width is enabled
// 0 if slab width is disabled
//-1 if param error
//-------------------------------------------------------------
int OrthoSlices::WidthEnabled() {
    double value=0;
    if (P_getreal(GLOBAL, "g3ds", &value, 4)>=0) {
        int test=(int)value;
        return ((test & 0x40)>0) ? 1 : 0;
    }
    return -1;
}

//-------------------------------------------------------------
// OrthoSlices::WidthReversed() 
//-------------------------------------------------------------
// return:
// 0 if slab width direction is "normal"
// 1 if slab width direction is "reversed"
//-1 if param error
//-------------------------------------------------------------
int OrthoSlices::WidthReversed() {
    double value=0;
    if (P_getreal(GLOBAL, "g3ds", &value, 1)>=0) {
        int test=(int)value;
        return ((test & 0x04)>0) ? 0 : 1;
    }
    return -1;
}

//-------------------------------------------------------------
// OrthoSlices::WidthValue(double &w) 
//-------------------------------------------------------------
// return the value of slab width (-1 if param error)
//-------------------------------------------------------------
int OrthoSlices::WidthValue(double &w) {
    double value=0;
    if (P_getreal(GLOBAL, "g3df", &value, 8)>=0) {
        w=value;
        return 1;
    }
    return -1;
}

//-------------------------------------------------------------
// OrthoSlices::Getg3dpnt(double &x, double &y, double &z)
//-------------------------------------------------------------
// return the value of g3dpnt (-1 if param error)
//-------------------------------------------------------------
int OrthoSlices::Getg3dpnt(double &x, double &y, double &z) {
    double value=0;
    if (P_getreal(GLOBAL, "g3dpnt", &value, 1)>=0) {
        P_getreal(GLOBAL, "g3dpnt", &x, 1);
        P_getreal(GLOBAL, "g3dpnt", &y, 2);
        P_getreal(GLOBAL, "g3dpnt", &z, 3);
        return 1;
    }
    return -1;
}

//-------------------------------------------------------------
// OrthoSlices::Getg3drot(double &x, double &y, double &z)
//-------------------------------------------------------------
// return the value of g3drot (-1 if param error) 
//-------------------------------------------------------------
int OrthoSlices::Getg3drot(double &x, double &y, double &z) {
    double value=0;
    if (P_getreal(GLOBAL, "g3drot", &value, 1)>=0) {
        P_getreal(GLOBAL, "g3drot", &x, 1);
        P_getreal(GLOBAL, "g3drot", &y, 2);
        P_getreal(GLOBAL, "g3drot", &z, 3);
        return 1;
    }
    return -1;
}

bool OrthoSlices::getShowCursors() {return showCursors;}
void OrthoSlices::setShowCursors(bool b) {
  showCursors = b;
  VolData *vdat=VolData::get();
  if (vdat->showingObliquePlanesPanel() && vdat->validData())
        OrthoSlices::get()->updateCursors();
  else {
	showCursors=false;
        OrthoSlices::get()->updateCursors();
  }
}

//-------------------------------------------------------------
// OrthoSlices::Show3PCursors(int argc, char **argv, int, char **) 
//-------------------------------------------------------------
int OrthoSlices::Show3PCursors(int argc, char **argv, int, char **) 
{
   if(argc>1 && atoi(argv[1]) <= 0) setShowCursors(false); 
   else if(argc>1) setShowCursors(true); 
   else if(showCursors) setShowCursors(false);
   else setShowCursors(true);
   return 0;
}

void OrthoSlices::setNext3Ppnt(int frame, int step) {
    char cmd[MAXSTR];
    if(frame == 1)
      sprintf(cmd,"vnmrjcmd('g3dpnt',3,%f)\n",(curDataZ+step)/zmax);
    else if(frame == 2)
      sprintf(cmd,"vnmrjcmd('g3dpnt',1,%f)\n",(curDataX+step)/xmax);
    else if(frame == 3)
      sprintf(cmd,"vnmrjcmd('g3dpnt',2,%f)\n",(curDataY+step)/ymax);
    else return;
    execString(cmd);
}

//-------------------------------------------------------------
// OrthoSlices::Setg3dpnt(int argc, char **argv, int, char **) 
//-------------------------------------------------------------
// Set the slices to be displayed.  
// - called from aipSetg3dpnt in aipCommands.C
// - aipSetg3dpnt will be called from JGLComMgr as a result of:
//   1. a slider change in a vnmr panel
//   2. moving a curser in a 2D ortho frame
//   3. moving a slice plane in the 3D graphics window
//-------------------------------------------------------------
int OrthoSlices::Setg3dpnt(int argc, char **argv, int, char **) {
    float NX, NY, NZ, slice; // Normalized coordinates
    // char str[256];
    double tol=1e-5;
    if (argc < 5)
        return 0;

    NX = atof(argv[1]);
    NY = atof(argv[2]);
    NZ = atof(argv[3]);
    slice = atof(argv[4]); // current sliceplane position

//#ifdef LINUX
    setVpnt(NX, NY, NZ, slice); // set the g3dpnt[] variable
//#endif

    // Bail out if we have never been initialized or wrong args
    if (fast == 0|| slow == 0)
        return 0;

    // Sanity test
    if (NX< 0.0 || NX> 1.0 ||NY < 0.0 || NY> 1.0 ||NZ < 0.0 || NZ> 1.0 )return 0;

    // Convert normalized to data point form and save'

    VolData *vdat=VolData::get();
    if(vdat->showingObliquePlanesPanel()) {
        // Extract and draw the slices
        OrthoSlices *os = OrthoSlices::get();
        double newDataX=NX * xmax;
        double newDataY=NY * ymax;
        double newDataZ=NZ * zmax;
        if(need_refresh) {
            os->drawThreePlanes( (int) newDataX, (int) newDataY, (int) newDataZ);
            return 1;
        }
        double delx=fabs(newDataX-curDataX)/xmax;
        double dely=fabs(newDataY-curDataY)/ymax;
        double delz=fabs(newDataZ-curDataZ)/zmax;
        if(delx>tol)
        os->extractPlane(XPLANE, (int) newDataX);
        if(dely>tol)
        os->extractPlane(YPLANE, (int) newDataY);
        if(delz>tol)
        os->extractPlane(ZPLANE, (int) newDataZ);

        //sprintf(str," dx:%g dy:%g dz:%g",delx,dely,delz);
        //ib_errmsg(str);

        if(!dragging) {
            curDataX = newDataX;
            curDataY = newDataY;
            curDataZ = newDataZ;
        }
        else {
            switch (gf_frame) {
                case ZPLANE:
                if (xdrag)
                {
                    if((z_xaxis==X_PLUS)||(z_xaxis==X_MINUS))
                            curDataX = newDataX;
                    else    
                        curDataY = newDataY;
                }
                if (ydrag)
                { 
                    if((z_yaxis==X_PLUS)||(z_yaxis==X_MINUS)) 
                            curDataX = newDataX;
                    else
                        curDataY = newDataY;
                }        
                break;
                
                case XPLANE:
                    if (xdrag)
                    {
                        if((x_xaxis==Y_PLUS)||(x_xaxis==Y_MINUS))
                                curDataY = newDataY;
                        else    
                            curDataZ = newDataZ;
                    }
                    if (ydrag)
                    { 
                        if((x_yaxis==Y_PLUS)||(x_yaxis==Y_MINUS))  
                                curDataY = newDataY;
                        else
                            curDataZ = newDataZ;
                    }                      
                    break;
                    
                case YPLANE:
                    if (xdrag)
                     {
                         if((y_xaxis==X_PLUS)||(y_xaxis==X_MINUS))
                                 curDataX = newDataX;
                         else    
                             curDataZ = newDataZ;
                     }
                     if (ydrag)
                     { 
                         if((y_yaxis==X_PLUS)||(y_yaxis==X_MINUS))  
                                 curDataX = newDataX;
                         else
                             curDataZ = newDataZ;
                     }                      
                     break;                    
            }
        }
        os->drawThreeCursors((int) curDataX, (int) curDataY, (int) curDataZ);
    }
    return 1;
}

//-------------------------------------------------------------
// OrthoSlices::Setg3drot(int argc, char **argv, int, char **) 
//-------------------------------------------------------------
// Set the rotation (Euler angles) to use for Setg3drot
//-------------------------------------------------------------
int OrthoSlices::Setg3drot(int argc, char **argv, int, char **) {
    float H, A, B; // 

    if (argc != 4)
        return 0;

    H = atof(argv[1]);
    A = atof(argv[2]);
    B = atof(argv[3]);

#ifdef LINUX
    setVrot(H, A, B);
#endif
    rotangles[0]=H;
    rotangles[1]=A;
    rotangles[2]=B;

    VolData *vdat=VolData::get();
    if (vdat->showingObliquePlanesPanel()) {
        // Extract and draw the slices
        OrthoSlices *os = OrthoSlices::get();
        os->drawThreePlanes((int) curDataX, (int) curDataY, (int) curDataZ);
    }
    return 1;
}

//-------------------------------------------------------------
// OrthoSlices::Setg3dflt(int argc, char **argv, int, char **) 
//-------------------------------------------------------------
// Set float parameter value
//-------------------------------------------------------------
int OrthoSlices::Setg3dflt(int argc, char **argv, int, char **) {
    float value;
    int index;
    index = atoi(argv[1]);
    value = atof(argv[2]);

#ifdef LINUX
    setVflt(index, value);
#endif
    if (index==8 && mipflag)
        OrthoSlices::get()->redraw();
    return 1;
}

//-------------------------------------------------------------
// OrthoSlices::RedrawMip()
//-------------------------------------------------------------
// redraw ortho planes
//-------------------------------------------------------------
int OrthoSlices::RedrawMip(int argc, char **argv, int, char **) {
    VolData *vdat=VolData::get();
    if (mipflag) {
        OrthoSlices::get()->redraw();
    }
    return 1;
}

