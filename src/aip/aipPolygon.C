/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <fstream>
using std::ofstream;
using std::ifstream;
#include <cmath>
using std::sqrt;
#include <algorithm>
using std::min;
using std::max;
//#include <vector>
//using std::vector;

#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipPolygon.h"
#include "aipPolyline.h"
#include "aipDataManager.h"

Polygon::Polygon() :
    Roi() {
}

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Polygon::Polygon(spGframe_t gf, int x, int y) :
    Roi() {
    init(gf, 2);
    created_type = ROI_POLYGON;
    closed = true;
    initEdgelist();
    initPix(x, y);
    lastx = (int) basex;
    lasty = (int) basey;
    pntData->name = "Polygon";
    magnetCoords->name="Polygon";
    Roi::create(gf);
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Polygon::Polygon(spGframe_t gf, spCoordVector_t dpts, bool pixflag) :
    Roi() {
    init(gf, dpts->coords.size());
    created_type = ROI_POLYGON;
    closed = true;
    initEdgelist();
    if(pixflag) {
       pntData = dpts;
       setMagnetPntsFromData();
    } else {
       magnetCoords = dpts;
       setDataPntsFromMagnet();
    } 
    setPixPntsFromData(); // Sets pntPix, npnts, min/max
    gf->addRoi(this);
    pntData->name = "Polygon";
    magnetCoords->name="Polygon";
}

void Polygon::initEdgelist() {
    yedgeOOD = true;
    yedge = NULL;
    ydataedge = NULL;
    dist_yedge = 0;
    min_segment_length = 8;
}

/*
 * Destructor.
 */
Polygon::~Polygon(void) {
    if (yedge) {
        // These y_min and y_max values are old values
        Edgelist::free_ybucket(yedge, nyedges);
        delete[] yedge;
        yedge = NULL;
    }
    delete_ydataedge();
}

/*
 * FindDuplicateVertex returns the index of the first vertex that is
 * "near" its successor vertex.  Near is defined to be within the
 * specified "tolerance".
 */
int Polygon::findDuplicateVertex(int tolerance) {
    for (int i = 0; i < npnts; i++) {
        int j = (i + npnts - 1) % npnts; // Previous point
        if (fabs(pntPix[i].x - pntPix[j].x) <= tolerance &&fabs(pntPix[i].y
                - pntPix[j].y) <= tolerance) {
            //printf("duplicate vertex[%d] \n", i);
            return i;
        }
    }
    return -1;
}

/*
 * InsertVertex will insert a vertex after the indicated index
 * position at the location (x,y).  NOTE: You should call build_yedge
 * at some point after inserting a new vertex.  It is not done here
 * because it is a slow operation that should only be done when
 * absolutely needed.
 */
bool Polygon::insertVertex(int index, int x, int y) {
    int i;

    if (index < 0|| npnts < 2|| pntPix.size() != npnts) {
        return false;
    }
    if (closed) {
        // Can insert vertex after last point since this is a closed polygon
        if (index > npnts - 1)
            return false;
    } else {
        // Can only insert a vertex up to the last point
        if (index > npnts - 2)
            return false;
    }

    // NB: Is there a faster way to insert an element into a vector?
    erase();
    Dpoint_t newpt;
    newpt.x = x;
    newpt.y = y;
    pntPix_t::iterator itr = pntPix.begin();
    for (i=0; i<=index; i++, itr++)
        ;
    pntPix.insert(itr, newpt);
    npnts++;
    calc_xyminmax();
    draw();

    if (pntData->coords.size() >= index) {
        pnts3D_t::iterator itr2 = pntData->coords.begin();
        for (i=0; i<=index; i++, itr2++)
            ;
        pntData->coords.insert(itr2, D3Dpoint_t());
    }

    if (magnetCoords->coords.size() >= index) {
        pnts3D_t::iterator itr3 = magnetCoords->coords.begin();
        for (i=0; i<=index; i++, itr3++)
            ;
        magnetCoords->coords.insert(itr3, D3Dpoint_t());
    }

    updateDataCoord(index);

    yedgeOOD = true;

    return true;
}

/*
 * Delete the vertex at the specified index position.
 */
bool Polygon::deleteVertex(int index) {
    int i;

    if (index < 0|| index >= npnts || npnts <= 3|| pntPix.size() != npnts) {
        return false;
    }

    // NB: Is there a faster way to delete an element from a vector?
    pntPix_t::iterator itr = pntPix.begin();
    for (i=0; i<index; i++, itr++)
        ;
    pntPix.erase(itr);
    npnts--;
    calc_xyminmax();
    //draw();

    if (pntData->coords.size() > index) {
        pnts3D_t::iterator itr = pntData->coords.begin();
        for (i=0; i<index; i++, itr++)
            ;
        pntData->coords.erase(itr);
    }

    if (magnetCoords->coords.size() > index) {
        pnts3D_t::iterator itr = magnetCoords->coords.begin();
        for (i=0; i<index; i++, itr++)
            ;
        magnetCoords->coords.erase(itr);
    }

    yedgeOOD = true;

    //printf("npnts = %d\n", npnts);

    return true;
}

/*
 * Update one vertex of a polygon.
 */
ReactionType Polygon::create(short x, short y, short action) {
    int i;

    // Check for the minimum and maximum limit of the graphics area
    //if ((x < 0) || (y < 0) ||
    //    (x > Gdev_Win_Width(gdev)) || (y > Gdev_Win_Height(gdev))
    //    )
    //{
    //    return REACTION_NONE;
    //}

    double dist_x = x - basex; // distance x
    double dist_y = y - basey; // distance y
    double new_x = pntPix[rolloverHandle].x + dist_x;
    double new_y = pntPix[rolloverHandle].y + dist_y;
    keep_point_in_image(&new_x, &new_y);

    if (action == nextPoint) {
        // If we are far enough from the last point, fix the current
        // point in place and start specification of next point.
        if ((fabs(new_x - lastx) < min_segment_length &&fabs(new_y - lasty)
                < min_segment_length)) {
            return REACTION_NONE;
        }
        Dpoint_t tmp;
        tmp.x = new_x;
        tmp.y = new_y;
        pntPix.push_back(tmp);
        ++npnts;
        updateDataCoord(npnts-1);
        setRolloverHandle(rolloverHandle + 1);
        basex = lastx = (int) new_x;
        basey = lasty = (int) new_y;
        //g_draw_line(gdev, x, y, x, y, color);
        roi_set_state(ROI_STATE_CREATE);
        yedgeOOD = true;
        updateSlaves(false);
        someInfo(false);
        return REACTION_NONE;
    }

    if (action == movePoint || action == movePointConstrained) {
        // Just move the current vertex around
        if (new_x == basex && new_y == basey) {
            // Could this even happen?
            return REACTION_NONE; // Same position, do nothing
        }
        erase();
        basex = new_x;
        basey = new_y;
        setPixelCoord(rolloverHandle, new_x, new_y);
        calc_xyminmax();
        draw();
        updateSlaves(true);
        someInfo(true);
    } else if (action == dribblePoints) {
        // If we have moved far enough, create a new point here
        if ((fabs(new_x - lastx) < min_segment_length &&fabs(new_y - lasty)
                < min_segment_length)) {
            return REACTION_NONE;
        }
        basex = lastx = (int) new_x;
        basey = lasty = (int) new_y;
        erase();
        Dpoint_t tmp;
        tmp.x = new_x;
        tmp.y = new_y;
        pntPix.push_back(tmp);
        ++npnts;
        updateDataCoord(npnts-1);
        setRolloverHandle(rolloverHandle + 1);
        calc_xyminmax();
        draw();
        updateSlaves(true);
        someInfo(true);
    } else if (action == deletePoint) {
        deleteVertex(rolloverHandle);
    }

    yedgeOOD = true;
    return REACTION_NONE;
}

/*
 * Checks whether or not the polygon is long enough to exist.
 * Also updates some variables in the class.
 */
ReactionType Polygon::create_done() {
    int vertex;
    while (npnts > 3&&(vertex=findDuplicateVertex(1)) >= 0) {
        deleteVertex(vertex);
    }
    yedgeOOD = true;

    //erase();
    //draw();
    updateSlaves(false);
    RoiManager::get()->clearActiveList();
    someInfo(false);
    return REACTION_NONE;
}

/*
 * Move a polygon to a new location.
 */
ReactionType Polygon::move(short x, short y) {
    int i;

    double dist_x = x - basex; // distance x
    double dist_y = y - basey; // distance y

    keep_roi_in_image(&dist_x, &dist_y);

    // Same position, do nothing
    if ((!dist_x) && (!dist_y)) {
        return REACTION_NONE;
    }

    erase(); // Erase previous polygon

    // Update polygon's pixel, data & magnet coords
    for (i=0; i<npnts; i++) {
        setPixelCoord(i, pntPix[i].x + dist_x, pntPix[i].y + dist_y);
    }
    yedgeOOD = true;

    x_min += dist_x;
    x_max += dist_x;
    y_min += dist_y;
    y_max += dist_y;

    draw(); // Draw new polygon

    basex += dist_x;
    basey += dist_y;

    updateSlaves(true);
    someInfo(true);
    return REACTION_NONE;
}

/************************************************************************
 *                                  *
 *  User just stops moving.                     *
 *                                  */
ReactionType Polygon::move_done(short, short) {
    basex = ROI_NO_POSITION;

    // Assign minmax value for a polygon
    calc_xyminmax();
    updateSlaves(false);
    someInfo(false);
    return REACTION_NONE;
}

/*
 * Insert a new vertex into the polygon.
 * Put it in the side closest to (x, y).
 * Returns index of point added.
 */
int Polygon::add_point(short x, short y) {
    double dist;
    int vertex = closestSide(x, y, dist);
    insertVertex(vertex, x, y); // Insert AFTER this vertex
    ++vertex; // Point to inserted vertex
    basex = lastx = x;
    basey = lasty = y;
    setRolloverHandle(vertex);
    updateDataCoord(vertex);
    updateSlaves(false);
    someInfo(false);
    return vertex;
}

/*
 * Copy this ROI to another Gframe
 */
Roi *Polygon::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Polygon(gframe, magnetCoords);
    return roi;
}

/*
 * Draw the polygon, including closing segment.
 */
void Polygon::draw() {
    if (pntPix[0].x == ROI_NO_POSITION || !drawable || !isVisible()) {
        return;
    }
    // Draw closing segment
    if (visibility != VISIBLE_NEVER) {
        setClipRegion(FRAME_CLIP_TO_IMAGE);
        GraphicsWin::drawLine(pntPix[npnts-1].x, pntPix[npnts-1].y,
                pntPix[0].x, pntPix[0].y, my_color);
        setClipRegion(FRAME_NO_CLIP);
    }
    // Now draw the rest
    Roi::draw();
}

/*
 * Get the distance from a given point to any side of this polygon.
 * If "far" >= 0, can return "far" if actual distance is > "far".
 */
double Polygon::distanceFrom(int x, int y, double far) {
    if (far > 0&&(x > x_max + far || x < x_min - far ||y > y_max + far || y
            < y_min - far)) {
        return far+1; // Return something farther away
    }

    double distance;
    closestSide(x, y, distance);
    return distance;
}

int Polygon::closestSide(int x, int y, double& distance) {
    double d2;
    int i, j;

    distance = distanceFromLine(x, y, pntPix[0].x, pntPix[0].y,
            pntPix[1%npnts].x, pntPix[1%npnts].y, -1);
    int minside = 0;
    int np = closed ? npnts : npnts - 1;
    for (i=1; i<np; i++) {
        int j = (i + 1) % npnts;
        d2 = distanceFromLine(x, y, pntPix[i].x, pntPix[i].y, pntPix[j].x,
                pntPix[j].y, -1);
        if (distance > d2) {
            distance = d2;
            minside = i;
        }
    }
    return minside;
}

/*
 *  Check whether a polygon is selected or not.
 *  There are two possiblities:
 *      1. Positioning a cursor at either corner will RESIZE a polygon.
 *  2. Positioning a cursor inside a polygon will MOVE a polygon.
 *  If a polygon is selected, it will set the 'acttype' variable.
 *  Return TRUE or FALSE.
 */
bool Polygon::is_selected(short x, short y) {
    // ----- Check to RESIZE a polygon -----
    // Check for the corner of a polygon.  If the cursor is close enough to
    // the end points, we will assign the opposite position of the point 
    // to variables 'basex' and 'basey' because this is the way we
    // interactively create a polygon.

    /* NOT USED ???? **************

     if ((force_acttype != ROI_MOVE) && resizable) {
     if (Corner_Check(x, y, pntPix[0].x, pntPix[0].y, aperture))   {
     // Upper-left
     basex = pntPix[1].x; basey = pntPix[1].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[1].x, pntPix[0].y, aperture))    {
     // Upper-right
     basex = pntPix[0].x; basey = pntPix[1].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[1].x, pntPix[1].y, aperture)) {
     // Bottom-right
     basex = pntPix[0].x; basey = pntPix[0].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[0].x, pntPix[1].y, aperture))  {
     // Bottom-left
     basex = pntPix[1].x; basey = pntPix[0].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     }
     }
     if (com_point_in_rect(x,y, pntPix[0].x,pntPix[0].y, pntPix[1].x,pntPix[1].y))  {
     // ----- Check to MOVE a polygon ------
     acttype = ROI_MOVE;
     return(TRUE);
     }
     */

    // ----- Polygon is not selected ------
    return (false);
}

/*
 *  Save the current polygon vertices into the following format:
 *
 *      # <comments>
 *   polygon
 *       n
 *       X1 Y1
 *       X2 Y2
 *       X3 Y3
 *       .....
 *       Xn Yn
 *  where
 *       # indicates comments
 *       n number of vertices, and
 *       X1 Y1    is the first vertex
 *   . . .
 *   Xn Yn    is the last vertex
 *
 *  If the polygon is closed, the first and last points are the same,
 *  and n is the number of vertices + 1.
 */
void Polygon::save(ofstream &outfile, bool pixflag) {
    int i;

    outfile << name() << "\n";
    if (closed) {
        outfile << npnts+1<< "\n";
    } else {
        outfile << npnts << "\n";
    }

  if(pixflag) {
    for (i=0; i<npnts; i++) {
        outfile << pntData->coords[i].x<< " "<< pntData->coords[i].y<< "\n";
    }
    if (closed) {
        outfile << pntData->coords[0].x<< " "<< pntData->coords[0].y<< "\n";
    }
  } else {
    for (i=0; i<npnts; i++) {
        outfile << magnetCoords->coords[i].x<< " "<< magnetCoords->coords[i].y<< " "<< magnetCoords->coords[i].z <<"\n";
    }
    if (closed) {
        outfile << magnetCoords->coords[0].x<< " "<< magnetCoords->coords[0].y<< " "<< magnetCoords->coords[0].z <<"\n";
    }
  }
}

/*
 *  Load ROI polygon from a file which has a format described in "save"
 *  routine.
 */
void Polygon::load(ifstream &infile, bool pixflag) {
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    bool isclosed;

    while (infile.getline(buf, buflen)) {
        if ((buf[0] != '#') && // Ignore comments
                strspn(buf, "\t ") != strlen(buf)) // Ignore blank line
        {
            if (sscanf(buf, "%d", &ndata) != 1) {
                fprintf(stderr,"ROI polygon: Missing number of vertices");
                return;
            } else {
                break;
            }
        }
    }
    if (ndata < 3) {
        fprintf(stderr,"ROI polygon. Number of vertices must be at least 3");
        return;
    }
    spCoordVector_t temp = spCoordVector_t(new CoordList(ndata));
    pnts3D_t& dat = temp->coords;

    // Read in the data
    int i = 0;
    while ((i != ndata) && infile.getline(buf, buflen)) {
        if ((buf[0] != '#') && // Ignore comments
                strspn(buf, "\t ") != strlen(buf)) // Ignore blank line
        {
          if(pixflag) {
            if (sscanf(buf, "%lf %lf", &(dat[i].x), &(dat[i].y)) != 2) {
                fprintf(stderr,"ROI Polygon: Missing data input");
                return;
            }
            i++;
	  } else {
            if (sscanf(buf, "%lf %lf %lf", &(dat[i].x), &(dat[i].y),&(dat[i].z)) != 3) {
                fprintf(stderr,"ROI Polygon: Missing data input");
                return;
            }
            i++;
 	  }
        }
    }

    if (ndata != i) {
        fprintf(stderr,"ROI Polygon: Incomplete input data points");
        return;
    }

    // Check if polygon is closed
    if (pixflag && dat[0].x == dat[ndata-1].x&& dat[0].y == dat[ndata-1].y) {
        ndata--;
        isclosed = true;
    } else if (dat[0].x == dat[ndata-1].x&& dat[0].y == dat[ndata-1].y
	&& dat[0].z == dat[ndata-1].z) {
        ndata--;
        isclosed = true;
    } else {
        isclosed = false;
    }

    // Put new ROI in the appropriate frames.

    int binding = (int) getReal("aipRoiBind", 0);
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
        if (gf != nullFrame && gf->getViewCount() > 0 
                && (binding==DATA_ALL || gfm->isFrameDisplayed(gf))) {
            if (isclosed) 
                roi = new Polygon(gf, temp, pixflag);
            else 
                roi = new Polyline(gf, temp, pixflag);
            
            if (roi != NULL&& selectFlag) 
                roi->select(ROI_COPY, true);
            
        } else {
            dataInfo = DataManager::get()->getDataInfoByKey(*itr);
            if (dataInfo != (spDataInfo_t)NULL) {
                if (isclosed) 
                    dataInfo->addRoi("Polygon", temp, false, selectFlag);
                 else 
                    dataInfo->addRoi("Polyline", temp, false, selectFlag);
            }
        }
    }
}

/*
 * Returns the address of the first data pixel in this ROI.
 * Initializes variables needed by NextPixel() to step through all the
 * pixels in the ROI.
 */
float *Polygon::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg) {
        return NULL;
    }

    if (yedgeOOD) {
        build_ydataedge();
    }

    spDataInfo_t di = img->getDataInfo();
    data_width = di->getFast();

    beg_of_row = di->getData() + (int)toprow * data_width;
    row = 0;
    roi_height = (int)botrow - (int)toprow + 1;
    edge_ptr = ydataedge[0];
    if (!edge_ptr || npnts < 2) {
        return NULL;
    }
    int x0 = (int)(0.5 + min(pntData->coords[0].x, pntData->coords[1].x));
    int x1 = (int)(0.5 + max(pntData->coords[0].x, pntData->coords[1].x));
    if(roi_height < 1 || (x1 - x0) < 1) {
        return NULL;
    }
    end_of_segment = beg_of_row + edge_ptr->next->x_edge;
    data = beg_of_row + edge_ptr->x_edge;
    return data;
}

/*
 * After initialization by calling FirstPixel(), each call to NextPixel()
 * returns the address of the next data pixel that is inside this ROI.
 * Successive calls to NextPixel() step through all the data in the ROI.
 * If no pixels are left, returns 0.
 */
float *Polygon::nextPixel() {
    if (data < end_of_segment) {
        return ++data;
    } else if ((edge_ptr=edge_ptr->next->next) && edge_ptr->next) {
        end_of_segment = beg_of_row + edge_ptr->next->x_edge;
        data = beg_of_row + edge_ptr->x_edge;
        return data;
    } else if (++row < roi_height && (edge_ptr=ydataedge[row]) != NULL) {
        beg_of_row += data_width;
        end_of_segment = beg_of_row + edge_ptr->next->x_edge;
        data = beg_of_row + edge_ptr->x_edge;
        return data;
    } else {
        return 0;
    }
}

// ************************************************************************
//
// ************************************************************************
/*
 void Polygon::update_screen_coords()
 {
 pntPix[0].x = x_min = data_to_xpix(pntData->coords[0].x);
 pntPix[0].y = y_min = data_to_ypix(pntData->coords[0].y);
 pntPix[1].x = x_max = data_to_xpix(pntData->coords[1].x);
 pntPix[1].y = y_max = data_to_ypix(pntData->coords[1].y);
 }
 */

// ************************************************************************
//
// ************************************************************************
/*
 void Polygon::update_data_coords()
 {
 pntData->coords[0].x = xpix_to_data(pntPix[0].x);
 pntData->coords[0].y = ypix_to_data(pntPix[0].y);
 pntData->coords[1].x = xpix_to_data(pntPix[1].x);
 pntData->coords[1].y = ypix_to_data(pntPix[1].y);
 }
 */

/************************************************************************
 *                                                                      *
 *  delete_yedge frees and resets the yedge list to NULL.  Look at  *
 *  Edgelist::build_ybucket for more details.               *
 *                                  */
void Polygon::delete_yedge(void) {

    if (yedge) {
        // These y_min and y_max values are old values
        Edgelist::free_ybucket(yedge, nyedges);
        delete[] yedge;
        yedge = NULL;
    }
}

/************************************************************************
 *                                                                      *
 *  delete_ydataedge frees and resets the ydataedge list to NULL.  Look at
 *  Edgelist::build_ybucket for more details.               *
 *                                  */
void Polygon::delete_ydataedge(void) {

    if (ydataedge) {
        // These toprow and botrow values are old values
        Edgelist::free_ybucket(ydataedge, (int)botrow - (int)toprow + 1);
        delete[] ydataedge;
        ydataedge = NULL;
    }
}

/************************************************************************
 *                                                                       *
 *  Build 'yedge' which consists of polygon edges.  Look at         *
 *  Edgelist::build_ybucket for more details.               *
 *                                  */
void Polygon::build_yedge(void) {
    int i;

    // Free previous polygon ybucket.  Note that this statement should
    // come before "find_minmax" which will change the value of y_min
    // and y_max.
    delete_yedge();

    // Find the current minimum and maximum polygon boundary points
    calc_xyminmax();

    // Allocate memory for a polygon ybucket 'yedge'
    nyedges = (int) (y_max - y_min) + 1;
    yedge = new Edgelist* [nyedges];

    // Clear all allocation to zero
    for (i=0; i<nyedges; yedge[i++]=NULL)
        ;

    // Build polygon ybucket. That is, each yedge[] consists of a list
    // of x's values which forms polygon's edges.
    Edgelist::build_ybucket(yedge, nyedges, pntPix, npnts, (int) y_min);
}

/************************************************************************
 *                                                                       *
 *  Build 'ydataedge' which consists of polygon edges.  Look at
 *  Edgelist::build_ybucket for more details.               *
 *                                  */
void Polygon::build_ydataedge(void) {
    int i;

    // Free previous polygon ybucket.
    delete_ydataedge();

    // Round and copy the list of data-space vertices to a "Gpoint" array
    // Find the current top and bottom polygon boundary points
    pntPix_t ipoints(npnts);
    //ipoints.reserve(npnts);
    toprow = botrow = pntData->coords[0].y;
    for (i=0; i<npnts; i++) {
        ipoints[i].x = (int)pntData->coords[i].x;
        ipoints[i].y = (int)pntData->coords[i].y;
        if (pntData->coords[i].y > botrow) {
            botrow = pntData->coords[i].y;
        } else if (pntData->coords[i].y < toprow) {
            toprow = pntData->coords[i].y;
        }
    }

    // Allocate memory for a polygon ybucket 'ydataedge' and initialize to 0.
    int nrows = (int)botrow - (int)toprow + 1;
    ydataedge = new Edgelist* [nrows];
    for (i=0; i<nrows; ydataedge[i++] = NULL)
        ;

    // Build polygon ybucket. That is, each ydataedge[] consists of a list
    // of x's values which forms polygon's edges.
    Edgelist::build_ybucket(ydataedge, nrows, ipoints, npnts, (int)toprow);
    yedgeOOD = false;
}
