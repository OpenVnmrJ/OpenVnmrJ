/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

 /************************************************************************
 *                                  *
 *  Description                             *
 *  -----------                             *
 *                                  *
 *  This file contains routines related to ROI tool.  They serve an     *
 *  interface between events (user mouse input) and graphics drawing    *
 *  routines.                               *
 *                                  *
 *  This file also contains a base class Roitool routines which serves  *
 *  as general routines.                            *
 *                                  *
 *************************************************************************/
#include <ctype.h>
//#include <math.h>
#include <strings.h>
#include <time.h>
#include <memory.h>
#include <iostream>
#include <fstream>
using std::ifstream;
using std::cout;
using std::endl;
#include <algorithm>
using std::min;
using std::max;
using std::swap;
#include <cmath>
using std::fabs;
using std::sqrt;

#include "aipStderr.h"
#include "aipVnmrFuncs.h"
#include "aipGraphicsWin.h"
#include "aipDataInfo.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipImgInfo.h"
#include "aipRoiManager.h"
#include "aipHistogram.h"
#include "aipRoiStat.h"
#include "aipPoint.h"
#include "aipRoi.h"
#include "aipBox.h"
#include "aipPolygon.h"
#include "aipPolyline.h"
#include "aipLine.h"
#include "aipOval.h"
#include "aipPoint.h"
#include "aipDataManager.h"
#include "aipAxisInfo.h"

extern "C" {
int InsidePolygon(Dpoint_t *polygon,int N,Dpoint_t p);
}

extern short xview_to_ascii(short);

void canvas_repaint_proc(void);

// Initialize static class members
//Gdev *Roi::gdev = 0;          // Graphics device
int Roi::serialNumber = 0;
int Roi::numberOfRois = 0;
int Roi::aperture = 8; // Sensitivity value to resize a corner
int Roi::max_active_tools = 10; // # of tools that can track together
int Roi::copy_color = 1; // Current default for drawing in "copy" mode
int Roi::active_color = 3; // Color used while moving ROI
int Roi::xor_color = 0; // Value used for XOR'ing stuff
int Roi::rollover_color = 3; // Color when mouse is nearby
int Roi::color = 0; // Current color value to use.
bool Roi::bind = false; // ROI binding
spGframe_t Roi::curr_frame = spGframe_t(NULL); // Remembers frame while creating ROI
Gpoint Roi::sel_down = { 0, 0 }; // Remembers where button went down

Raction_type Roi::force_acttype = ROI_NO_ACTION;
Raction_type Roi::acttype = ROI_NO_ACTION;

Roi *roi[ROI_NUM];
Roi *selected_text= NULL; // NOT CURRENTLY USED
Roi *active_tool= NULL;
Roitype roitype; // Type of ROI


// This is a list of all objects on the screen
RoiList* gObjects;
//RoiList* deleteStack;  
int delete_stack_depth_limit = 1;

//RoiList *selected_ROIs;
RoiList *active_tools; // ROIs being created/modified interactively
RoiList *clone_tools; // ROIs being created/modified at completion


CoordList::CoordList(int n) {
    aipDprint(DEBUGBIT_2,"CoordList()\n");
    coords.resize(n);
    //npts = n;
}

CoordList::~CoordList() {
    aipDprint(DEBUGBIT_2,"~CoordList()\n");
}

void redraw_all_rois() {
    spGframe_t frame;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (frame=gfm->getFirstFrame(gfi); frame != nullFrame; frame
            =gfm->getNextFrame(gfi)) {
        Roi *roi;
        for (roi=frame->getFirstRoi(); roi; roi=frame->getNextRoi()) {
            //Gmode mode = roi->setcopy();
            roi->draw();
            //roi->setGmode(mode);
        }
    }
}

/************************************************************************
 *                                  *
 *  Constructor for the Roi base class              *
 *                                  */
Roi::Roi() {
    ++numberOfRois;
    aipDprint(DEBUGBIT_2,"Roi()\n");
    roiNumber = ++serialNumber;
    inCanvasBackup = false;
    my_color = copy_color;
    markable = true;
    basex = basey = ROI_NO_POSITION;
    magnetCoords = spCoordVector_t(new CoordList(1));
    pntData = spCoordVector_t(new CoordList(1));
    minpntData.x=1.0e6;
    minpntData.y=1.0e6;
    minpntData.z=1.0e6;
    maxpntData.x=0.0;
    maxpntData.y=0.0;
    maxpntData.z=0.0;
    labelWd = labelHt = 0;
    rev = 1;
    magnetCoords->rev = 1;
}

/************************************************************************
 *                                  *
 *  Destructor for the Roi base class               *
 *                                  */
Roi::~Roi() {
    pOwnerFrame->setDisplayOOD(true);
    pOwnerFrame->deleteRoi(this);
    RoiManager *roim = RoiManager::get();
    roim->removeSelection(this);
    roim->removeActive(this);
    roim->clearInfoPointers(this);
    if (--numberOfRois == 0) {
        serialNumber = 0;
    }
    aipDprint(DEBUGBIT_2,"~Roi()\n");
}

bool Roi::isVisible() {
    GframeManager *gfm=GframeManager::get();
    spGframe_t gf=gfm->getPtr(pOwnerFrame);
    return gfm->isFrameDisplayed(gf);
}

spCoordVector_t Roi::getMagnetCoords() {
    return magnetCoords;
}

spCoordVector_t Roi::getpntData() {
    return pntData;
}

D3Dpoint_t Roi::getmaxpntData() {
    return maxpntData;
}

D3Dpoint_t Roi::getminpntData() {
    return minpntData;
}

/*
 * Called from contructor (cannot be virtual).
 * Assume gf is valid.
 */
void Roi::create(spGframe_t gframe) {
#ifdef USE_AIP_TEMPLATE
    createSlaves(this);
#else
    int binding = (int)getReal("aipRoiBind", 0);
    if (binding) {
        bool selectFlag = getReal("aipRoiSelectSlavesOnCreation", 0) != 0;
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        ReviewQueue *rq = ReviewQueue::get();
        RoiManager *roim = RoiManager::get();
        spDataInfo_t dataInfo;
        std::set<string> keylist = rq->getKeyset(binding);
        std::set<string>::iterator itr;
        for (itr = keylist.begin(); itr != keylist.end(); ++itr) {
            gf=gfm->getCachedFrame(*itr);           
            if (gf != nullFrame && gf->getFirstImage() != nullImg){

/* uncomment this if don't want ROI on images of different orient
		if(created_type != ROI_POINT && created_type != ROI_LINE && 
		!gf->parallelPlane(gframe->getFirstView(),gf->getFirstView())) 
		  continue;
*/

                if(binding==DATA_ALL || gfm->isFrameDisplayed(gf))
                if (gf.get() != pOwnerFrame) {
                    Roi *roi=NULL;
                    switch(created_type){
                    case ROI_BOX:
                        roi = new Box(gf, magnetCoords);
                        break;
                    case ROI_OVAL:
                        roi = new Oval(gf, magnetCoords);
                        break;
                    case ROI_LINE:
                        roi = new Line(gf, magnetCoords);
                        break;
                    case ROI_POINT:
                        roi = new Point(gf, magnetCoords);
                        break;
                    case ROI_POLYGON:
                        roi = new Polygon(gf, magnetCoords);
                        break;
                    case ROI_POLYGON_OPEN:
                        roi = new Polyline(gf, magnetCoords);
                        break;
                    }
                    if (selectFlag && roi !=NULL) {
                        roi->select(ROI_COPY, true); // Append selection
                    }
                }
            } else {
                dataInfo = DataManager::get()->getDataInfoByKey(*itr);
                if (dataInfo != (spDataInfo_t)NULL) 
                    dataInfo->addRoi("", magnetCoords, false, selectFlag);
             }
        }
    }
#endif
    activateSlaves();
    someInfo(false);
}

/************************************************************************
 *                                  *
 *  Load ROIs from a file.
 *  Looks for lines giving the name of the ROI type (line, box, etc.)
 *  and calls the load routine for the appropriate type.
 *                                  */
void Roi::load_roi(ifstream &infile) {
    const int buflen = 128;
    char buf[buflen];
    char roi_name[buflen];

    bool pixflag=false;
    while (infile.getline(buf, buflen) ) {
        if (buf[0] == '#' && strstr(buf,"data coordinates") != NULL) {
            pixflag=true;
            continue;
        } else if (buf[0] == '#') { // Ignore comment lines
            continue;
        } else if (strspn(buf, "\t ") == strlen(buf)) { // Ignore blank lines
            continue;
        } else {
            sscanf(buf, "%127s", roi_name); // Expect an ROI type name
            if (strcasecmp(roi_name, "Point") == 0) {
                load(infile,ROI_POINT, pixflag);
            } else if (strcasecmp(roi_name, "Line") == 0) {
                load(infile,ROI_LINE, pixflag);
            } else if (strcasecmp(roi_name, "Box") == 0) {
                load(infile,ROI_BOX, pixflag);
            } else if (strcasecmp(roi_name, "Oval") == 0) {
                load(infile,ROI_OVAL, pixflag);
            } else if (strcasecmp(roi_name, "Polygon") == 0) {
                Polygon::load(infile, pixflag);
            } else if (strcasecmp(roi_name, "Polyline") == 0) {
                Polygon::load(infile, pixflag);
            }
        }
    }
}


/************************************************************************
 *                                  *
 *  Load ROIs from a file.
 *  Looks for lines giving the name of the ROI type (line, box, etc.)
 *  and calls the load routine for the appropriate type.
 *                                  */
void Roi::load(ifstream &infile, int type, bool pixflag) {
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    const char *roi_name="Roi";
    spCoordVector_t temp;
    int num_coords=2;
    switch(type){
    case ROI_OVAL:
        roi_name = "Oval";
        break;
    case ROI_BOX:
        roi_name = "Box";
        break;
    case ROI_LINE:
        roi_name = "Line";
        break;
    case ROI_POINT:
        roi_name = "Point";
        num_coords=1;
        break;
    }
    temp = spCoordVector_t(new CoordList(num_coords));
    if (temp == (spCoordVector_t)NULL)
        return;

    pnts3D_t& dat = temp->coords;

    while ((ndata != num_coords) && infile.getline(buf, buflen)) {
        if (buf[0] == '#' && strstr(buf,"data coordinates") != NULL) {
            pixflag=true;
            continue;
        } else if (buf[0] == '#') {
            continue;
        } else if (strspn(buf, "\t ") == strlen(buf)) { // Ignore blank lines
            continue;
        } else if (pixflag) {
            if (sscanf(buf, "%lf %lf", &(dat[ndata].x), &(dat[ndata].y)) != 2) {
                fprintf(stderr,"ROI %s: Missing data input",roi_name);
                return;
            }
            ndata++;
        } else {
            if (sscanf(buf, "%lf %lf %lf", &(dat[ndata].x), &(dat[ndata].y), &(dat[ndata].z)) != 3) {
                fprintf(stderr,"ROI %s: Missing data input",roi_name);
                return;
            }
            ndata++;
        }
    }

    if (ndata != num_coords) {
        fprintf(stderr,"ROI %s: Incomplete input data points",roi_name);
        return;
    }

    // Put new ROI in the appropriate frames.

    int binding = (int)getReal("aipRoiBind", 0);
    if (binding == 0)
        return;

    bool selectFlag = getReal("aipRoiSelectSlavesOnCreation", 0) != 0;
    spGframe_t gf;
    Roi *roi;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;

    spDataInfo_t dataInfo;

    ReviewQueue *rq = ReviewQueue::get();

    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();

    std::set<string> keylist = rq->getKeyset(binding);
    std::set<string>::iterator itr;
    for (itr = keylist.begin(); itr != keylist.end(); ++itr) {
        gf=gfm->getCachedFrame(*itr);
        if (gf != nullFrame 
            && gf->getViewCount() > 0 
            && (binding==DATA_ALL || gfm->isFrameDisplayed(gf))) {
                roi=NULL;
                switch(type){
                case ROI_OVAL:
                    roi = new Oval(gf, temp, pixflag);
                    break;
                case ROI_BOX:
                    roi = new Box(gf, temp, pixflag);
                    break;
                case ROI_LINE:
                    roi = new Line(gf, temp, pixflag);
                    break;
                case ROI_POINT:
                    roi = new Point(gf, temp, pixflag);
                    break;
                }
                if (roi != NULL && selectFlag)
                    roi->select(ROI_NOREFRESH, true);
            
        } else {
            dataInfo = DataManager::get()->getDataInfoByKey(*itr);
            if (dataInfo != (spDataInfo_t)NULL) {
                dataInfo->addRoi(roi_name, temp, false, selectFlag);
            }
        }
    }
}

/************************************************************************
 *
 *  Update the screen for a particular ROI.
 *  Data is redrawn; unselected tools drawn in solid; selected tools
 *  drawn with handles in ROI_COPY mode, or without in ROI_XOR mode.
 *  (It is not really XOR any more.)
 */
void Roi::refresh(RoiDrawMode mode) // ROI_XOR or ROI_COPY
{
    if (mode == ROI_XOR) {
        roi_clear_state(ROI_STATE_MARK);
    } else if (is_selected()) {
        roi_set_state(ROI_STATE_MARK);
    }

    bool visible=isVisible();
    
    if(!visible)
       return;
    
    // Refresh my picture
    if (mode != ROI_NOREFRESH && visible) {
        if (mode == ROI_XOR) {
            drawable = false; // Do not draw myself
        }
        pOwnerFrame->draw();
        drawable = true;
    }

    if (mode == ROI_XOR && visible) {
        // Draw myself without handles
        roi_clear_state(ROI_STATE_MARK);
        pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
        draw();
        pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
        if (is_selected()) {
            roi_set_state(ROI_STATE_MARK);
        }
    }

    if (mode != ROI_NOREFRESH) {
        // Refresh all other pictures
        GframeManager::get()->updateViews();
    }
}

/************************************************************************
 *                                                                       *
 *  Select a particular Roi and update the screen accordingly.
 *  Data is redrawn; unselected tools drawn in solid; selected tool
 *  drawn in XOR mode or COPY mode, depending on value of "mode".
 */
void Roi::select(RoiDrawMode mode, // ROI_XOR or ROI_COPY
        bool appendFlag) // Other ROIs remain selected
{
    bool alreadySelected = false;
    RoiManager *roim = RoiManager::get();

    if (appendFlag) {
        alreadySelected = is_selected();
    } else {
        // Empty the selected list
        roim->clearSelectedList(); // TODO: This should set any OOD flags
        pOwnerFrame->setDisplayOOD(true);
    }

    if (!alreadySelected) {
        roim->addSelection(this); // Select new ROI
        pOwnerFrame->setDisplayOOD(true); // Set Out Of Date flag
    }
    refresh(mode);
}

/*
 * Get some information about the roi.
 * Used for point info and line profiles.  See aipPoint.C and aipLine.C.
 */
void Roi::someInfo(bool, bool) {
}

/* STATIC */
double Roi::distanceFromLine(double x, double y, double x1, double y1,
        double x2, double y2, double far) {
    double xpa = x - x1; // distance from x1 to x
    double ypa = y - y1; // distance from y1 to y
    double xba = x2 - x1; // distance from x1 to x2
    double yba = y2 - y1; // distance from y1 to y2
    double len2 = xba * xba + yba * yba; // Square of line length

    if (len2 == 0) {
        // degenerate line
        return sqrt(xpa * xpa + ypa * ypa);
    }

    double temp = xpa * xba + ypa * yba; // dot product
    double parm = temp / len2;
    if (parm <= 0) {
        // We're closest to first end of line
        return sqrt(xpa * xpa + ypa * ypa);
    } else if (parm >= 1) {
        // Closest to other end of line
        return sqrt((x-x2) * (x-x2)+ (y-y2) * (y-y2));
    } else {
        // Perpendicular distance to line
        return sqrt(fabs((xpa * xpa + ypa * ypa) - (temp * parm)));
    }
}

/************************************************************************
 *                                                                       *
 *  Return the latest selected ROI
 *  STATIC
 *                                                                       */
Roi *Roi::get_selected_tool() {
    Roi *tool;
    RoiManager *roim = RoiManager::get();

    if (roim->numberSelected() ) {
        return roim->getFirstSelected();
    } else {
        return NULL;
    }
}

/************************************************************************
 *                                                                       *
 *  Determine if a gframe has any selected tools in it.
 *  STATIC
 *                                                                       */
//int
//Roi::frame_has_a_selected_tool(spGframe_t frame)
// Moved to Gframe::hasASelectedRoi()


void Roi::deselect() {
    roi_clear_state(ROI_STATE_MARK);
    RoiManager::get()->removeSelection(this);
}

/************************************************************************
 *                                  *
 *  Update x_min, y_min, x_max, y_max
 *                                  */
void Roi::update_xyminmax(int x, int y) {
    if (x < x_min) {
        x_min = x;
    } else if (x > x_max) {
        x_max = x;
    }
    if (y < y_min) {
        y_min = y;
    } else if (y > y_max) {
        y_max = y;
    }
}

/************************************************************************
 *                                  *
 *  Update x_min, y_min, x_max, y_max
 *                                  */
void Roi::calc_xyminmax() {
    if (npnts == 0) {
        x_min = y_min = x_max = y_max = 0;
    } else {
        x_min = x_max = pntPix[0].x;
        y_min = y_max = pntPix[0].y;
        int i;
        for (i=1; i<npnts; i++) {
            if (pntPix[i].x < x_min) {
                x_min = pntPix[i].x;
            } else if (pntPix[i].x > x_max) {
                x_max = pntPix[i].x;
            }
            if (pntPix[i].y < y_min) {
                y_min = pntPix[i].y;
            } else if (pntPix[i].y > y_max) {
                y_max = pntPix[i].y;
            }
        }
    }
}

void Roi::setRolloverHandle(int handle) {
    if (handle >= 0&& (rolloverFlag || (rolloverHandle != handle))) {
        rolloverFlag = false;
        my_color = copy_color;
        rolloverHandle = handle;
        refresh(ROI_COPY);
    } else if (handle < 0&& rolloverHandle >= 0) {
        rolloverHandle = handle;
        refresh(ROI_COPY);
    }
}

bool Roi::setRollover(bool onFlag) {
    bool rtn = rolloverFlag;
    bool change = false;
    if (rolloverFlag != onFlag) {
        my_color = onFlag ? rollover_color : copy_color;
        rolloverFlag = onFlag;
        change = true;
    }
    if (rolloverHandle >= 0) {
        rolloverHandle = -1;
        change = true;
    }
    if (change) {
        refresh(ROI_COPY);
    }
    return rtn;
}

void Roi::setBase(int x, int y) {
    basex = x;
    basey = y;
}

/************************************************************************
 *                                  *
 *  Return the order of a given tool in the active list.
 *  First is 0.  If not in list, return -1.
 *
 *  STATIC
 *                                  */
int Roi::position_in_active_list(Roi *roi) {
    RoiManager *roim = RoiManager::get();
    Roi *r;
    int i;

    for (i=0, r=roim->getFirstActive(); r; r=roim->getNextActive(), i++) {
        if (r == roi) {
            return i;
        }
    }
    return -1;
}

/************************************************************************
 *                                                                       *
 *  Find the min and max values in the ROI
 *                                  */
bool Roi::getMinMax(double& dmin, double& dmax) {
    float *data;
    if ((data = firstPixel()) == NULL) {
        // No pixels in this ROI.
        return false;
    }
    dmin = dmax = *data;
    for (data=nextPixel(); data; data=nextPixel()) {
        if (*data > dmax) {
            dmax = *data;
        } else if (*data < dmin) {
            dmin = *data;
        }

    }
    return true;
}

bool Roi::getSum(double &sum, int &count) {
    float *data;
    if ((data = firstPixel()) == NULL) {
        // No pixels in this ROI.
        return false;
    }
    count = 1;
    sum = *data;
    for (data=nextPixel(); data; data=nextPixel()) {
        count++;
        sum += *data;
    }
    return true;
}

/************************************************************************
 *                                                                       *
 *  Find the histogram and statistics within the ROI.
 *  Only pixels with data between the "bot" and "top" values are considered
 *  in calculating the statistics.  This will also be the range spanned
 *  by the histogram.
 *  Return true if successful, otherwise false.
 *                                  */
bool Roi::histostats(int nbins, double bot, double top, RoiData *stats) {
    register int index;
    double thickness;
    double z_location;
    spImgInfo_t img = pOwnerFrame->getFirstImage();

    register float *data;
    if (nbins <= 0|| (data = firstPixel()) == 0) {
        // aipStatNumBins zero, or no pixels in this ROI.
        return false;
    }

    // Calculate the max, min, and mean from all data in ROI that
    // is within the bot and top limits.
    int npixels = 0;
    double mean = 0;
    float min = *data;
    float max = *data;
    for (; data; data=nextPixel() ) {
        if (*data >= bot && *data <= top) {
            if (*data > max) {
                max = *data;
            } else if (*data < min) {
                min = *data;
            }
            mean += *data;
            npixels++;
        }
    }
    if (npixels) {
        mean /= npixels;

        // Construct the histogram and calculate standard deviation.
        stats->histogram = new Histogram;
        stats->histogram->nbins = nbins;
        stats->histogram->bottom = bot;
        stats->histogram->top = top;
        int *hist = stats->histogram->counts = new int[nbins];
        for (index=0; index<nbins; index++) {
            hist[index] = 0;
        }

        double sdv = 0;
        int low_bin = 0; // Hold counts of number of off scale bins.
        int high_bin = 0; // ...A good idea, but we don't use them.
        float offset = bot;
        if (top == bot) {
            // Degenerate case; put all counts in middle bin
            hist[nbins/2] = npixels;
            sdv = 0;
            stats->median = mean;
        } else {
            float scale = nbins / (top - bot);
            for (data=firstPixel(); data; data=nextPixel() ) {
                index = (int)(scale * (*data - offset));
                if (*data == top) {
                    // Include border case in top bin
                    // (otherwise always lose top pixel when we auto-scale)
                    index = nbins - 1;
                }
                if (*data < bot) {
                    low_bin++;
                } else if (*data > top) {
                    high_bin++;
                } else {
                    sdv += (*data - mean) * (*data - mean);
                    hist[index]++;
                }
            }
            if (npixels > 1) {
                sdv = sqrt(sdv / (npixels - 1));
            }

            // Calculate approximate median from the histogram.
            int ipixel = 0;
            for (index=0; index<nbins; index++) {
                ipixel += hist[index];
                if ((ipixel * 2) > npixels) {
                    break;
                }
            }
            stats->median = bot + (index + 0.5) * (top - bot)/ nbins;
        }

        stats->min = min;
        stats->max = max;
        stats->mean = mean;
        stats->sdv = sdv;
    }

    spDataInfo_t dataInfo = img->getDataInfo();
    // Calculate and load the area in sq cm.
    // The "span" gives the distance
    // from one edge of the picture to the other, rather than the
    // distance between centers of the opposite edge pixels.
    stats->area = (npixels* (dataInfo->getSpan(0) / dataInfo->getFast())
            * (dataInfo->getSpan(1) / dataInfo->getMedium()));

    // Get some additional stuff to put in
    // stats->frameNumber = pOwnerFrame->getGframeNumber();
    // it really mean data number
    stats->frameNumber = pOwnerFrame->getDataNumber();
    stats->roiNumber = getRoiNumber();
    stats->npixels = npixels;
    double location[3];
    dataInfo->getLocation(location);
    stats->z_location = location[2];
    stats->thickness = dataInfo->getRoi(2);
    stats->volume = stats->area * stats->thickness;

    return true;
}

/************************************************************************
 *                                  *
 *  Zeros out all pixels that are inside the ROI and are outside the
 *  intensity range (min <= data <= max).
 *                                  */
void Roi::zero_out(double min, double max) {
    float *data = firstPixel();
    for (; data; data=nextPixel() ) {
        if ((*data > max) || (*data < min)) {
            *data = 0.0;
        }
    }
}

/************************************************************************
 *                                  *
 *  These are the default "iterator" functions used by those ROI objects
 *  for which no valid histogram can be computed.
 *                                  */
float *Roi::firstPixel() {
    return 0;
}

float *Roi::nextPixel() {
    return 0;
}

bool Roi::setPixelCoord(int index, double x, double y) {
    //double dx, dy, dz;
    if (index >= npnts) {
        return false;
    }
    pntPix[index].x = x;
    pntPix[index].y = y;
    updateDataCoord(index);
}

bool Roi::updateDataCoord(int index) {
    if (index >= npnts) {
        return false;
    }
    // Set magnet coordinate
    if (magnetCoords->coords.size() <= index) {
        // TODO: use vector for magnetCoords->coords
        magnetCoords->coords.resize(index+1);
    }
    D3Dpoint_t *magCoords = &magnetCoords->coords[index];
    spViewInfo_t view = pOwnerFrame->getFirstView();

    view->pixToMagnet(pntPix[index].x, pntPix[index].y, 0, magCoords->x,
            magCoords->y, magCoords->z);

    magnetCoords->rev = 1;

    // Set data coordinate
    if (pntData->coords.size() <= index) {
        // Make sure vector is big enough
        pntData->coords.resize(index+1);
    }
    view->pixToData(pntPix[index].x, pntPix[index].y, pntData->coords[index].x,
            pntData->coords[index].y);
    pntData->rev = rev;

}

/*
 * Set pntPix, npnts, x_min, x_max, y_min, y_max, rev from data coords
 */
void Roi::setPixPntsFromData() {
    pnts3D_t data = pntData->coords;
    npnts = data.size();
    if (npnts < 1) {
        return;
    }
    pntPix.resize(npnts);
    spViewInfo_t view = pOwnerFrame->getFirstView();
    double x, y;
    view->dataToPix(data[0].x, data[0].y, x, y);
    pntPix[0].x = x;
    pntPix[0].y = y;
    x_min = x_max = pntPix[0].x;
    y_min = y_max = pntPix[0].y;
    for (int i=1; i<npnts; i++) {
        view->dataToPix(data[i].x, data[i].y, x, y);
        pntPix[i].x = x;
        pntPix[i].y = y;
        double sx = pntPix[i].x;
        double sy = pntPix[i].y;
        if (x_min > sx) {
            x_min = sx;
        } else if (x_max < sx) {
            x_max = sx;
        }
        if (y_min > sy) {
            y_min = sy;
        } else if (y_max < sy) {
            y_max = sy;
        }
    }
    //rev = pntData->rev;

    setMinMaxDataPnts();
}

void Roi::setMinMaxDataPnts() {
    pnts3D_t data = pntData->coords;
    npnts = data.size();
    if (npnts < 1) {
        return;
    }
    maxpntData.x=data[0].x;
    maxpntData.y=data[0].y;
    maxpntData.z=data[0].z;
    minpntData.x=data[0].x;
    minpntData.y=data[0].y;
    minpntData.z=data[0].z;
    for (int i=1; i<npnts; i++) {
        if(data[i].x > maxpntData.x) maxpntData.x = data[i].x;
        if(data[i].y > maxpntData.y) maxpntData.y = data[i].y;
        if(data[i].z > maxpntData.z) maxpntData.z = data[i].z;
        if(data[i].x < minpntData.x) minpntData.x = data[i].x;
        if(data[i].y < minpntData.y) minpntData.y = data[i].y;
        if(data[i].z < minpntData.z) minpntData.z = data[i].z;
    }
}

/*
 * Set magnetCoords, npnts, rev from data coords
 */
void Roi::setMagnetPntsFromData() {
    pnts3D_t data = pntData->coords;
    npnts = data.size();
    if (npnts < 1) {
        return;
    }
    magnetCoords->coords.resize(npnts);
    //pnts3D_t mag = magnetCoords->coords;
    spViewInfo_t view = pOwnerFrame->getFirstView();
    if(view == nullView) return;
 
    //di->updateScaleFactors();
    for (int i=0; i<npnts; i++) {
        double x, y, z;
        double px, py;
        view->dataToPix(data[i].x, data[i].y, px, py);
        view->pixToMagnet(px, py, 0.0, x, y, z);
        magnetCoords->coords[i].x = x;
        magnetCoords->coords[i].y = y;
        magnetCoords->coords[i].z = z;
    }
    //rev = pntData->rev;
}

ReactionType Roi::create_done() {
    updateSlaves(false);
    RoiManager::get()->clearActiveList();
    return REACTION_NONE;
}

Roi *Roi::copy(spGframe_t gframe) {
    return NULL;
}

// show mean or sum of ROI intensity
void Roi::showIntensity(bool updateSlaves) {
   double sum, cx, cy;
   int count;
   if(getSum(sum, count) && getCenterPoint(cx, cy)) {
     if(count > 1 && getReal("aipShowROIOpt", 0) == 1) 
	sum = sum/count;

     spGframe_t gf = GframeManager::get()->getPtr(pOwnerFrame);
     AxisInfo::showIntensity(gf, (int)cx, (int)cy, false, sum); 
   }

   if(!updateSlaves) return;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf=gfm->getNextFrame(gfi)) {
        if (gf.get() != pOwnerFrame) {
            Roi *roi;
            for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
                if (roi->magnetCoords == magnetCoords) {
                    // ROI is bound to this one if has same data points
                    roi->showIntensity(false);
                    break; // Only one per frame!
                }
            }
        }
    }
}

/************************************************************************
 *                                  *
 *  Draw the ROI
 *                                  */
void Roi::draw(void) {
    if (pntPix[0].x == ROI_NO_POSITION || !drawable || !isVisible()) {
        return;
    }

    setClipRegion(FRAME_CLIP_TO_IMAGE);
    calc_xyminmax();

    if (visibility != VISIBLE_NEVER) {
        /*XSetLineAttributes(gdev->xdpy, gdev->xgc,
         0, LineSolid, CapButt, JoinBevel);*/
        GraphicsWin::drawPolyline(&pntPix[0], npnts, my_color);
    }
    roi_set_state(ROI_STATE_EXIST);

    if (roi_state(ROI_STATE_MARK) || rolloverHandle >= 0) {
        mark();
    }

    setClipRegion(FRAME_NO_CLIP);

    bool show = (getReal("aipShowROIPos", 0)>0);

    if(show && getReal("aipShowROIOpt", 0) > 0 && npnts > 0) {
      showIntensity();
    } else if(show && npnts> 1 && created_type == ROI_LINE) {
      double dis = 0.0;
      double a,b;
      string units = getString("aipUnits","cm");
      if(units.find("pix") != string::npos) {
	a = pntData->coords[0].x-pntData->coords[1].x;
	b = pntData->coords[0].y-pntData->coords[1].y;
	dis = sqrt(a*a+b*b);
      } else {
	a = magnetCoords->coords[0].x-magnetCoords->coords[1].x;
	b = magnetCoords->coords[0].y-magnetCoords->coords[1].y;
	dis = sqrt(a*a+b*b);
      }
      spGframe_t gf = GframeManager::get()->getPtr(pOwnerFrame);
      AxisInfo::showDistance(gf,(int)pntPix[0].x,(int)pntPix[0].y,
		(int)pntPix[1].x,(int)pntPix[1].y, dis);
    }

    drawRoiNumber();

}

int Roi::getHandlePoint(int i, double &x, double &y) {
   if(npnts < 1 || i < 0 || i >= npnts) return 0;
   x=pntPix[i].x;
   y=pntPix[i].y;
   return 1;
}

int Roi::getFirstPoint(double &x, double &y) {
   if(npnts < 1) return 0;
   x=pntPix[0].x;
   y=pntPix[0].y;
   return npnts;
}

int Roi::getFirstLast(double &x1, double &y1, double &x2, double &y2) {
   if(npnts < 2) return 0;
   x1=pntPix[0].x;
   y1=pntPix[0].y;
   x2=pntPix[npnts-1].x;
   y2=pntPix[npnts-1].y;
   return npnts;
}
 
bool Roi::getCenterPoint(double &x, double &y) {
   if(npnts < 1) return false;
   x=0; y=0;
   for(int i=0; i<npnts; i++) {
     x += pntPix[i].x; 
     y += pntPix[i].y; 
   }
   x = x/npnts; 
   y = y/npnts; 
   return true;
}

int Roi::getRoiNumber() {
    return roiNumber;
}

void Roi::drawRoiNumber() {
    int show = (int)getReal("aipNumberRois", 1);
    if (!show) {
        labelWd = labelHt = 0;
        return;
    }

    // setClipRegion(FRAME_CLIP_TO_FRAME);
    Gframe *gf = pOwnerFrame;
    char buf[1000];
    sprintf(buf, "%d", getRoiNumber());
    int ascent = 0;
    int descent = 0;
    int width = 0;
    int x = (int) x_max + 2;
    int y = (int) y_min;
    int x1 = gf->minX();
    int y1 = gf->minY();
    int x2 = gf->maxX();
    int y2 = gf->maxY();
    GraphicsWin::getTextExtents(buf, 14, &ascent, &descent, &width);
    if (!width || !(ascent + descent)) {
        return; // Can't figure out the size
    }
    spViewInfo_t vinfo;
    if ((vinfo=pOwnerFrame->getFirstView()) != nullView) {
        x1 = vinfo->pixstx;
        y1 = vinfo->pixsty;
        x2 = vinfo->pixstx + vinfo->pixwd;
        y2 = vinfo->pixsty + vinfo->pixht;
    }

    /*
     if (x + width > gf->maxX() && x_max <= gf->maxX()) {
     x = gf->maxX() - width;
     }
     if (y - ascent < gf->minY() && y_min >= gf->minY()) {
     y = gf->minY() + ascent;
     }
     */
    if (width >= (gf->maxX() - gf->minX()) / 4|| (descent + ascent)
            >= (gf->maxY() + gf->minY()) / 4) {
        // Frame too small for numbers
        labelWd = labelHt = 0;
        return;
    }

    labelWd = width + 2;
    labelHt = ascent + descent + 2;
    if (x + labelWd > x2) {
        x = x2 - labelWd;
    } else if (x < 0)
        x = 0;
    if (y - ascent < y1) {
        y = y1 + ascent;
    }
    if (y + labelHt > y2)
        y = y2 - labelHt;
    if (show < 2) {
        // Don't clear background
        GraphicsWin::drawString(buf, x, y, my_color, 0, 0, 0);
    } else {
        GraphicsWin::drawString(buf, x, y, my_color, width, ascent, descent);
    }

    // Dirty part of window; allow extra pixel for text background rectangle
    labelX = x - 1;
    labelY = y - ascent - 1;

    // setClipRegion(FRAME_NO_CLIP);
}

void Roi::eraseRoiNumber() {
    if (labelWd == 0|| labelHt == 0) {
        return;
    }
    setClipRegion(FRAME_CLIP_TO_FRAME);
    // Sometimes font extends beyond expected rectangle -- so...
    int pad = labelWd > labelHt ? labelWd : labelHt;
    redisplay_bkg(labelX - pad, labelY - pad, labelX + labelWd + pad, labelY
            + labelHt + pad);
    setClipRegion(FRAME_NO_CLIP);
}

bool Roi::isActive() {
    return active;
}

void Roi::setActive(bool b) {
    if(!isVisible()) { // don't call save_bkg()
        active = b;
	return;
    }

    if (b) {
        active = b;
        save_bkg();
    } else {
        if (active) {
            active = b;
            save_bkg();
        }
        forget_bkg();
    }
    active = b;
}

void Roi::activateSlaves() {
    RoiManager *roim = RoiManager::get();
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeCache_t::iterator gfi;
    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf=gfm->getNextCachedFrame(gfi)) {
        if (gf.get() != pOwnerFrame) {
            Roi *roi;
            for (roi=gf->getFirstRoi(); roi; roi=gf->getNextRoi()) {
                if (roi->magnetCoords == magnetCoords) {
                    // ROI is bound to this one if has same data points
                    roim->addActive(roi);
                    break; // Only one per frame!
                }
            }
        }
    }
}

//void
//Roi::updateSlaves(bool inMotion, spCoordVector_t newData) {
//    pntData = newData;
//    updateSlaves(inMotion);
//}

/* For test */
/*
 static int
 printCoords(pnts3D_t coords)
 {
 int np = coords.size();
 for (int i=0; i<np; ++i) {
 fprintf(stderr,"%.6f   %.6f   %.6f\n",
 coords[i].x, coords[i].y, coords[i].z);
 }
 return 0;
 }
 */

void Roi::updateSlaves(bool inMotion) {
    int ns=-1;
    if (inMotion && (ns=(int)getReal("aipRoiMaxActiveSlaves", 10)) == 0) {
        return;
    }
    // Assumes slaves have been activated!
    RoiManager *roim = RoiManager::get();
    if (ns < 0|| !inMotion) {
        ns = roim->numberActive();
    }
    Roi *roi;
    int i = 0;
    for (roi=roim->getFirstActive(); roi && i<ns; roi=roim->getNextActive()) {
        if (roi != this) {
            roi->erase();
	    roi->setDataPntsFromMagnet();
            roi->setPixPntsFromData();
            //roi->setMagnetPntsFromData();
            roi->flagUpdate();
            roi->draw();
            i++;
        }
    }
    for (roi=roim->getFirstSelected(); roi ; roi=roim->getNextSelected()) {
        if (roi != this) {
            if (!roi->isActive()) {
	        roi->setDataPntsFromMagnet();
                roi->setPixPntsFromData();
               // roi->setMagnetPntsFromData();
                roi->flagUpdate();
            }
        }
    }

    if (creating)
        createSlavesForData();
}

void Roi::createSlavesForData() {
    creating = false;

    int binding = (int)getReal("aipRoiBind", 0);
    if (binding == 0|| binding == DATA_SELECTED_FRAMES|| binding
           == DATA_DISPLAYED || binding==DATA_ALL)
      //  == DATA_DISPLAYED)
        return;

    spDataInfo_t dataInfo;
    ReviewQueue *rq = ReviewQueue::get();

    bool selectFlag = getReal("aipRoiSelectSlavesOnCreation", 0) != 0;
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    DataManager *dm = DataManager::get();
    DataMap *dataMap = dm->getDataMap();

    std::set<string> keylist = rq->getKeyset(binding);
    std::set<string>::iterator itr;
    for (itr = keylist.begin(); itr != keylist.end(); ++itr) {
        gf=gfm->getCachedFrame(*itr);
        if (gf == nullFrame || !gfm->isFrameDisplayed(gf) ||gf->getFirstImage()
                == nullImg) {

            dataInfo = DataManager::get()->getDataInfoByKey(*itr);
            if (dataInfo != (spDataInfo_t)NULL) {
                dataInfo->addRoi(pntData->name.c_str(), magnetCoords, false,
                        selectFlag);
            }
        }
    }
}

void Roi::init(spGframe_t gf, int np) {
    if (np) {
        npnts = np;
        pntPix.resize(np);
        pntData->coords.resize(np);
    }
    state = 0;
    visibility = VISIBLE_ALWAYS;
    drawable = true;
    resizable = true;
    pOwnerFrame = gf.get();
    setRollover(false);
}

void Roi::initPix(int x, int y) {
    short sx = x;
    short sy = y;
    keep_point_in_image(&sx, &sy);
    x_min = x_max = sx;
    y_min = y_max = sy;
    setBase(sx, sy);
    for (int i=0; i<npnts; i++) {
        setPixelCoord(i, sx, sy);
    }
    pOwnerFrame->addRoi(this);
    if (getReal("aipRoiSelectOnCreation", 1) != 0) {
        select(ROI_COPY, true); // Append selection
    }
    RoiManager *roim = RoiManager::get();
    roim->clearActiveList();
    roim->addActive(this);
    creating = true;
}

/************************************************************************
 *                                  *
 *  Mark stored background invalid
 *                                  */
void Roi::forget_bkg() {
    inCanvasBackup = false;
}

/************************************************************************
 *                                  *
 *  Save a copy of the "background" image.  This is everything that is
 *  displayed in the area that the ROI can move around in, except the
 *  ROI itself.
 *
 *  Returns true if it's saved, otherwise returns false.
 *                                  */
bool Roi::save_bkg() {
    inCanvasBackup = pOwnerFrame->saveCanvasBackup(this);
    return inCanvasBackup;
}

/************************************************************************
 *                                  *
 *  Refresh the "background" image, overwriting the ROI
 *                                  */
bool Roi::redisplay_bkg(int x1, int y1, int x2, int y2) {
    bool ok;
    ok = pOwnerFrame->drawCanvasBackup(x1, y1, x2, y2);
    /*
     if (!ok) {
     save_bkg();
     ok = pOwnerFrame->drawCanvasBackup(x1, y1, x2, y2);
     }
     */
    return ok;
}

/************************************************************************
 *                                  *
 *  Erase an ROI
 *                                  */
void Roi::erase() {
    if(!isVisible())
        return;
    pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
    if(getReal("aipShowROIPos", 0)>0) pOwnerFrame->draw();
    redisplay_bkg((int) x_min - MARK_SIZE, (int) y_min - MARK_SIZE, (int) x_max + MARK_SIZE,
            (int) y_max + MARK_SIZE);
    pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
    roi_clear_state(ROI_STATE_EXIST);
    eraseRoiNumber();
}

/************************************************************************
 *                                  *
 *  Mark all the vertices.                          *
 *                                  */
void Roi::mark(void) {
    if (!markable)
        return;
    int i;
    int saveColor = my_color;
    if (!roi_state(ROI_STATE_MARK) && rolloverHandle >= 0) {
        // Draw only the active mark
        my_color = rollover_color;
        draw_mark((int) pntPix[rolloverHandle].x, (int) pntPix[rolloverHandle].y);
        my_color = saveColor;
    } else {
        // Draw all the marks
        for (i=0; i<npnts; i++) {
            if (rolloverHandle == i) {
                my_color = rollover_color;
            }
            draw_mark((int) pntPix[i].x, (int) pntPix[i].y);
            my_color = saveColor;
        }
    }
}


/************************************************************************
 *                                  *
 * Used after ROI creation/modification to set where the ROI is relative
 * to the data.
 * VIRTUAL
 *                                  */
void Roi::update_data_coords() {
    cout << "update_data_coords() not implemented"<< endl;
}

/************************************************************************
 *                                  *
 * Used after image rotation to rotate the ROI.
 *                                  */
void Roi::rot90_data_coords(int datawidth) {
    int i;
    double t;

    for (i=0; i<npnts; i++) {
        t = pntData->coords[i].x;
        pntData->coords[i].x = pntData->coords[i].y;
        pntData->coords[i].y = datawidth - t;
    }
    
    // Mark Roi out of date 
    flagUpdate();
}

/************************************************************************
 *                                  *
 * Used after image flipping to flip the ROI.
 * VIRTUAL
 *                                  */
void Roi::flip_data_coords(int datawidth) {
    int i;

    for (i=0; i<npnts; i++) {
        pntData->coords[i].x = datawidth - pntData->coords[i].x;
    }
    // Mark Roi out of date 
    flagUpdate();
}

/************************************************************************
 *                                  *
 * Used after window move/resize/zoom to update our idea of where the
 * is on the screen, which we need to know if we want to know if the
 * mouse is on it.
 *                                  */
void Roi::update_screen_coords() {
    cout << "update_screen_coords() not implemented"<< endl;
}

void Roi::setMagnetCoordsFromPixels() {
    if (magnetCoords->coords.size() != npnts) {
        magnetCoords->coords.resize(npnts);
    }
    spViewInfo_t view = pOwnerFrame->getFirstView();
    for (int i=0; i<npnts; i++) {
        view->pixToMagnet(pntPix[i].x, pntPix[i].y, 0,
                magnetCoords->coords[i].x, magnetCoords->coords[i].y,
                magnetCoords->coords[i].z);
    }
}

void Roi::keep_roi_in_image(double *x_motion, double *y_motion) {
    double x_limit;
    double y_limit;
    spViewInfo_t vinfo;

    if ((vinfo=pOwnerFrame->getFirstView()) == nullView) {
        return;
    }

    if ( (*x_motion >= 0) && (*y_motion >= 0)) {
        x_limit = x_max + *x_motion;
        y_limit = y_max + *y_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *x_motion = x_limit - x_max;
        *y_motion = y_limit - y_max;

    } else if ( (*x_motion <= 0) && (*y_motion >= 0)) {
        x_limit = x_min + *x_motion;
        y_limit = y_max + *y_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *x_motion = x_limit - x_min;
        *y_motion = y_limit - y_max;

    } else if ( (*y_motion <= 0) && (*x_motion >= 0)) {
        y_limit = y_min + *y_motion;
        x_limit = x_max + *x_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *y_motion = y_limit - y_min;
        *x_motion = x_limit - x_max;

    } else if ( (*y_motion <= 0) && (*x_motion <= 0)) {
        y_limit = y_min + *y_motion;
        x_limit = x_min + *x_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *y_motion = y_limit - y_min;
        *x_motion = x_limit - x_min;
    }
}

void Roi::keep_roi_in_image(short *x_motion, short *y_motion) {
    double x_limit;
    double y_limit;
    spViewInfo_t vinfo;

    if ((vinfo=pOwnerFrame->getFirstView()) == nullView) {
        return;
    }

    if ( (*x_motion >= 0) && (*y_motion >= 0)) {
        x_limit = x_max + *x_motion;
        y_limit = y_max + *y_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *x_motion = (short)(x_limit - x_max);
        *y_motion = (short)(y_limit - y_max);

    } else if ( (*x_motion <= 0) && (*y_motion >= 0)) {
        x_limit = x_min + *x_motion;
        y_limit = y_max + *y_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *x_motion = (short)(x_limit - x_min);
        *y_motion = (short)(y_limit - y_max);

    } else if ( (*y_motion <= 0) && (*x_motion >= 0)) {
        y_limit = y_min + *y_motion;
        x_limit = x_max + *x_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *y_motion = (short)(y_limit - y_min);
        *x_motion = (short)(x_limit - x_max);

    } else if ( (*y_motion <= 0) && (*x_motion <= 0)) {
        y_limit = y_min + *y_motion;
        x_limit = x_min + *x_motion;
        vinfo->keepPointInData(x_limit, y_limit);
        *y_motion = (short)(y_limit - y_min);
        *x_motion = (short)(x_limit - x_min);
    }
}

/*
 *
 */
bool Roi::getImageBoundsInPixels(double& imgX0, double& imgY0, double& imgX1,
        double& imgY1) {
    spViewInfo_t vi;
    if ((vi=pOwnerFrame->getFirstView()) == nullView) {
        return false;
    }
    spDataInfo_t di = vi->imgInfo->getDataInfo();
    vi->dataToPix(0, 0, imgX0, imgY0);
    vi->dataToPix(di->getFast() - 1, di->getMedium() - 1, imgX1, imgY1);
    if (imgX0 > imgX1) {
        swap(imgX0, imgX1);
    }
    if (imgY0 > imgY1) {
        swap(imgY0, imgY1);
    }
    return true;
}

/************************************************************************
 *                                  *
 * Clip a point's coordinates to keep it inside the image
 *                                  */
void Roi::keep_point_in_image(short *x, short *y) {
    spViewInfo_t vinfo;
    if (created_type == ROI_SELECTOR ||(vinfo=pOwnerFrame->getFirstView())
            == nullView) {
        return;
    }

    double dx = *x;
    double dy = *y;
    vinfo->keepPointInData(dx, dy);
    //fprintf(stderr,"dx=%f\n", dx);/*CMP*/
    *x = (short)dx;
    *y = (short)dy;
}

/************************************************************************
 *                                  *
 * Clip a point's coordinates to keep it inside the image
 *                                  */
void Roi::keep_point_in_image(int *x, int *y) {
    spViewInfo_t vinfo;
    if (created_type == ROI_SELECTOR ||(vinfo=pOwnerFrame->getFirstView())
            == nullView) {
        return;
    }

    double dx = *x;
    double dy = *y;
    vinfo->keepPointInData(dx, dy);
    *x = (int)dx;
    *y = (int)dy;
}

/************************************************************************
 *                                  *
 * Clip a point's coordinates to keep it inside the image
 *                                  */
void Roi::keep_point_in_image(double *x, double *y) {
    spViewInfo_t vinfo;
    if (created_type == ROI_SELECTOR ||(vinfo=pOwnerFrame->getFirstView())
            == nullView) {
        return;
    }

    vinfo->keepPointInData(*x, *y);
}

/************************************************************************
 *                                  *
 * Set the clip region to be the image display area.
 * "style" is FRAME_CLIP_TO_IMAGE or FRAME_NO_CLIP (to turn off clipping).
 *                                  */
void Roi::setClipRegion(ClipStyle style) {
    pOwnerFrame->setClipRegion(style);
}

/************************************************************************
 *                                  *
 * Redraw the ROI in copy mode to make it solid
 *                                  */
void Roi::draw_solid() {
    pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
    draw();
    pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
}

/************************************************************************
 *                                  *
 * Check the list of selected ROIs to see if we're selected.
 *                                  */
bool Roi::is_selected() {
    return RoiManager::get()->isSelected(this);
}

/*
 * Figure out which handle, if any, we are near.
 */
int Roi::getHandle(int x, int y) {
    if (npnts < 1) {
        return -1;
    }
    double dx = pntPix[0].x - x;
    double dy = pntPix[0].y - y;
    double minDist = dx * dx + dy * dy;
    int minHandle = 0;
    double dist;
    int i;
    for (i=1; i<npnts; ++i) {
        dx = pntPix[i].x - x;
        dy = pntPix[i].y - y;
        dist = dx * dx + dy * dy;
        if (dist <= minDist) {
            minDist = dist;
            minHandle = i;
        }
    }
    if (minDist >= aperture * aperture) {
        return -1;
    } else {
        return minHandle;
    }
}

/************************************************************************
 *                                  *
 * Draw an ROI marker at the specified position, (x, y).
 *                                  */
void Roi::draw_mark(int x, int y) {
    Gpoint mpnt[4];
    // Note that g_fill_polygon() works asymetrically, which is why
    //  we need to subtract one less than the "MARK_SIZE", but add the
    //  whole "MAKK_SIZE".
    mpnt[0].x = x - MARK_SIZE+ 1;
    mpnt[0].y = y - MARK_SIZE+ 1;
    mpnt[1].x = x + MARK_SIZE;
    mpnt[1].y = y - MARK_SIZE+ 1;
    mpnt[2].x = x + MARK_SIZE;
    mpnt[2].y = y + MARK_SIZE;
    mpnt[3].x = x - MARK_SIZE+ 1;
    mpnt[3].y = y + MARK_SIZE;
    GraphicsWin::fillPolygon(mpnt, 4, my_color);
}

typedef float float2[2];
bool Roi::contains(int x, int y) {
   bool yes=false;
   Dpoint_t p, poly[64];
   int npt;
   float r;
   pnts3D_t data;
   int i;
   double a,b;
   
   switch(created_type){
   case ROI_BOX:
      if(x >= minpntData.x && x <= maxpntData.x 
	&& y >= minpntData.y && y <= maxpntData.y) yes = true;
      break;
   case ROI_OVAL:
      a = (maxpntData.x-minpntData.x)/2;
      b = (maxpntData.y-minpntData.y)/2;
      x = x - (maxpntData.x+minpntData.x)/2;
      y = y - (maxpntData.y+minpntData.y)/2;
      if(a>0 && b>0 && ((x*x)/(a*a) + (y*y)/(b*b)) < 1) yes = true;
      break;
   case ROI_POLYGON:
   case ROI_POLYGON_OPEN:
      p.x=x;
      p.y=y;
      data = pntData->coords;
      npt = npnts;
      if(npt>64) npt=64;
      for(i=0; i<npt; i++) {
       poly[i].x=data[i].x;
       poly[i].y=data[i].y;
      }
      if(InsidePolygon(poly,npt,p)==0) yes = true;
      break;
   case ROI_LINE:
   case ROI_POINT:
      break;
   }
   return yes;
}

void Roi::setDataPntsFromMagnet() {
    pnts3D_t data = magnetCoords->coords;
    npnts = data.size();
    if (npnts < 1) {
        return;
    }

    pntData->coords.resize(npnts);

    spViewInfo_t view = pOwnerFrame->getFirstView();
    if(view == nullView) return;

    for (int i=0; i<npnts; i++) {
        double x, y, z;
        double px, py, pz;
        view->magnetToPix(data[i].x, data[i].y, data[i].z, px, py, pz);
        view->pixToData(px, py, x, y);
        pntData->coords[i].x = x;
        pntData->coords[i].y = y;
        pntData->coords[i].z = 0.0;
    }
    pntData->rev = rev;
    magnetCoords->rev = 1;
}
