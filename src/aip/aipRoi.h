/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPROI_H
#define AIPROI_H

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>

#include "sharedPtr.h"

#include "aipVnmrFuncs.h"
#include "aipStructs.h"
#include "aipRoiStat.h"
#include "aipMouse.h"
#include "aipStructs.h"
//#include "aipGframeManager.h"
//#include "aipGframe.h"
#include "aipReviewQueue.h"
#include "aipDataManager.h"

class Gframe;
typedef boost::shared_ptr<Gframe> spGframe_t;
extern spGframe_t nullFrame;
class GframeManager;
typedef std::pair<int, int> frameKey_t;
typedef std::map<frameKey_t, spGframe_t> GframeList;

//#include <stream.h>

//#include "stringedit.h"
//#include "macrolist_ib.h"
/* #include "gtools.h" */

// Mark size for marking ROI
#define MARK_SIZE   2

// State used by Roi to indicate that ROI is on the graphics, ROI
// marked, ROI is being created, or ROI existence.  Note that not all
// ROI tools use these bits
// It is accessed through bit by variable 'state' in class Roi
#define ROI_STATE_MARK      1
#define ROI_STATE_EXIST     2
#define ROI_STATE_CREATE    4

#define ROI_NO_POSITION -1

// Types of clip regions that may be set
typedef enum {
    FRAME_NO_CLIP,
    FRAME_CLIP_TO_IMAGE,
    FRAME_CLIP_TO_FRAME
} ClipStyle;

// ROI shape (or type)
typedef enum
{
    ROI_SELECTOR = 0,
    ROI_LINE,
    ROI_POINT,
    ROI_BOX,
    ROI_OVAL,
    ROI_POLYGON,
    ROI_POLYGON_OPEN,
    ROI_TEXT,
    ROI_MATH,
    MARKER_VLINE,
    MARKER_CHAIR,
    MARKER_CIRCLE,
    MARKER_GRID,
    ROI_NUM
} Roitype;

// ROI properties menu
typedef enum {
    ROI_MENU_DELETE,
    ROI_MENU_UNDELETE,
    ROI_MENU_MARK,
    ROI_MENU_LOAD,
    ROI_MENU_SAVE,
    ROI_MENU_APERTURE,
    ROI_MENU_TRACKING,
    ROI_MENU_BIND
} Roi_props_menu;

// ROI event action
typedef enum {
    ROI_NO_ACTION,
    ROI_CREATE,
    ROI_CREATE_DONE,
    ROI_RESIZE,
    ROI_RESIZE_DONE,
    ROI_MOVE,
    ROI_MOVE_DONE,
    ROI_ROTATE, // only apply to Oval
    ROI_ROTATE_DONE // only apply to Oval
} Raction_type;

typedef enum {
    VISIBLE_ALWAYS,
    VISIBLE_WHEN_MOVED,
    VISIBLE_NEVER
} Visibility;

// The following typedef contains possible return values for an action
// that is invoked by an ROI object

typedef enum {
    REACTION_NONE,
    REACTION_CREATE_OBJECT,
    REACTION_DELETE_OBJECT
} ReactionType;

// Indicate the drawing mode for the selected object:
typedef enum {
    ROI_XOR,
    ROI_COPY,
    ROI_NOREFRESH
} RoiDrawMode;

// Graphics drawing mode and color
struct Gmode {
    int color;
    int op;
};

typedef std::vector<D3Dpoint_t> pnts3D_t;

class CoordList {
public:
    long long rev; // Keeps track of updates
    string name; // "Point", "Line", etc.
    pnts3D_t coords;

    explicit CoordList(int n);
    ~CoordList();
};

typedef boost::shared_ptr<CoordList> spCoordVector_t; // "sp" for shared ptr

// An integral location in the plane
class Lpoint {
public:
    short x, y; // point
    Lpoint(short xval, short yval) {
        x=xval;
        y=yval;
    }
    ~Lpoint() {
    }
};

// Linked-list to store a number of integer (short) points
typedef std::list<Lpoint> LpointList;

typedef std::vector<Dpoint_t> pntPix_t; // List of points in pixel coords
typedef std::vector<Dpoint_t> pntData_t; // List of points in data coords


class Stats;

// Base tool class.  This class is a base class for ROI routines
class Roi {
    friend class RoiManager;

protected:

    // To keep from drawing ROIs outside of image data
    bool getImageBoundsInPixels(double& imgX0, double& imgY0, double& imgX1,
            double& imgY1);
    void keep_point_in_image(short *x, short *y);
    void keep_point_in_image(int *x, int *y);
    void keep_point_in_image(double *x, double *y);

    static int serialNumber; // Running index of ROI creation order
    static int numberOfRois; // Number of ROIs in existence
    int roiNumber; // Number of this ROI

    // The type of this object at creation time
    Roitype created_type;

    // Determines the visibity of the ROI
    Visibility visibility;

    // Temporary variables used by the member functions in this class.
    // Variables 'basex' and 'basey' are served as initialized point
    // (base point for moving/creating/resizing)
    double basex;
    double basey;
    // Desired position - cursor position:
    int off_x;
    int off_y;

    bool active;
    bool creating;

    static Raction_type force_acttype; // Force selection for move, rotate
    //static Gdev *gdev;        // Graphics device
    static int max_active_tools; // # of tools to adjust in real time
    static int copy_color; // Current default for drawing in "copy" mode
    static int rollover_color; // Color when mouse is nearby
    static int active_color; // Color used while moving ROI
    static int xor_color; // Color for XOR'ing stuff (Not used?)
    static int color; // Normal color value to use. (Redundant?)
    static bool bind; // Bind ROI modifications

    bool rolloverFlag; // Indicates mouse is near ROI
    int rolloverHandle;
    int my_color; // Normal (copy) color for this ROI
    // TODO: eliminate npnts (get from pntPix.size())
    long long rev; // Track pntPix updates
    int npnts; // Number of points
    pntPix_t pntPix; // List of points in pixel coords
    D3Dpoint_t maxpntData; // max point in data coords
    D3Dpoint_t minpntData; // max point in data coords
    spCoordVector_t pntData; // List of points in data coords
    // Formerly: pntData_t
    spCoordVector_t magnetCoords;// List of points in magnet coords
    short state; // State of ROI
    static spGframe_t curr_frame; // Remembers frame while creating ROI
    static Gpoint sel_down; // Remembers where button went down
    bool inCanvasBackup; // True if bkg for this ROI is stored in pixmap
    //short pmstx, pmsty;   // Location of ul corner of pixmap on canvas
    //short pmendx, pmendy; // Location of lr corner of pixmap on canvas

    double x_min, y_min; // Upper left corner of ROI bounding rectangle 
    double x_max, y_max; // Lower right corner of ROI bounding rectangle
    int labelX, labelY; // Position of label
    int labelWd, labelHt; // Size of label
    unsigned short resizable : 1;
    unsigned short drawable : 1;
    unsigned short markable : 1;

public:
    enum {
        nextPoint,
        movePoint,
        movePointConstrained,
        deletePoint,
        dribblePoints
    };
    static int aperture; // Sensitivity value to resize a corner

    Roi();
    virtual ~Roi();

    // NB: Use a "Gframe *" here instead of shared pointer "spGframe_t".
    // Since the pointer is contained in the Gframe (Gframe contains a
    // list of ROIs, which all have pointers back to the Gframe), making
    // this a shared pointer prevents a Gframe with ROIs from ever being
    // automatically deleted (unless you delete its ROIs first).
    Gframe *pOwnerFrame; // Frame containing this ROI

    static double distanceFromLine(double x, double y, double x1, double y1,
            double x2, double y2, double far);
    static Roi *get_selected_tool();
    static int frame_has_a_selected_tool(spGframe_t);
    Roitype GetType() {
        return created_type;
    }
    int getFirstLast(double &x1, double &y1, double &x2, double &y2);
    int getFirstPoint(double &x, double &y);
    bool getCenterPoint(double &x, double &y);

    ; // What type of ROI this is
    bool redisplay_bkg(int, int, int, int); // Refresh the background image
    bool save_bkg(); // Create pixmap of ROI's background
    void forget_bkg(); // Mark stored background invalid
    static bool allocate_bkg(); // Get pixmap to store ROI backgrounds
    //static void release_bkg();    // Release pixmap of ROI backgrounds
    //static XID set_drawable(XID); // Set the default drawable for display
    spCoordVector_t getpntData();
    D3Dpoint_t getmaxpntData();
    D3Dpoint_t getminpntData();

    // Set attribute (registered to be a call-back function)
    // Used to set color value and aperture (sensitivity)
    static int set_attr(int id, char *);
    //static void roi_menu_color(Menu, Menu_item);

    // Initialize points. It is enough to initialize x of the first point
    void init_point(void) {
        pntPix[0].x = basex = ROI_NO_POSITION;
    }

    // Routine related to state
    short roi_state(short s) {
        return (state & (s));
    }
    void roi_clear_state(short s) {
        state &= ~(s);
    }
    void roi_set_state(short s) {
        state |= (s);
    }

    // Action of graphics event such as create/move/resize
    static Raction_type acttype;

    // To create/move/resize 
    //ReactionType action(short x, short y, short action = NULL);
    // Create/move/resize all active ROIs
    //ReactionType active_action(short x, short y, short action = NULL);

    // Make every tool in the active_tools list like the first one
    static void clone_first_active();

    static int position_in_active_list(Roi *);

    // Text handler
    virtual int handle_text(char c) {
        return printf("%c", c);
    }

    // Set drawing mode (and corresponding color)
    /*
     Gmode setxor();
     Gmode setcopy();
     Gmode setactive();
     Gmode setGmode(Gmode);
     */

    bool contains(int x, int y);
    bool isActive();
    void setActive(bool on_off);
    bool isVisible();
    void setNotDisplayedActive(bool on_off) {
        active = on_off;
    }
    void activateSlaves();

    // Basic ROI functions
    virtual const char *name(void) = 0;
    virtual void flagUpdate() {
    }
    virtual ReactionType create(short, short, short) = 0;
    virtual ReactionType create_done();
    virtual ReactionType resize(short, short) = 0;
    virtual ReactionType resize_done() {
        return REACTION_NONE;
    }
    virtual ReactionType move(short, short) = 0;
    virtual ReactionType move_done(short, short) = 0;
    virtual int add_point(short x, short y) {
        return -1;
    }
    virtual Roi *copy(spGframe_t);
    virtual void draw(void);
    virtual void erase(void);
    virtual void mark();
    void refresh(RoiDrawMode copymode=ROI_XOR);
    virtual void select(RoiDrawMode copymode=ROI_XOR, bool appendflag=false);
    virtual void deselect();
    virtual double distanceFrom(int x, int y, double far) = 0;
    virtual bool is_selected(short, short) = 0; // Is cursor in ROI?
    virtual bool is_selectable(short x, short y) {
        return is_selected(x, y);
    } // "is_selectable()" does not modify the ROI
    bool is_selected(); // Is ROI in selection list?
    virtual int getHandle(int x, int y);
    virtual int getHandlePoint(int handle, double &x, double &y);
    void draw_mark(int x, int y); // Draw ROI marker at (x, y)
    void update_xyminmax(int x, int y); // Update x_min, y_min, x_max, y_max
    void calc_xyminmax(); // Calculate x_min, etc. from pnt[]
    void setRolloverHandle(int n); // Draw n'th vertex highlighted
    bool setRollover(bool); // Draw highlighted if rollover is on
    virtual void setBase(int x, int y); // Base position for moves
    void updateSlaves(bool inMotion);
    void init(spGframe_t gf, int np);
    void initPix(int x, int y);
    int getRoiNumber();
    void drawRoiNumber();
    void eraseRoiNumber();

    void create(spGframe_t gf);
    static void load(std::ifstream &infile, int type, bool pixflag=false);

    // FocusIn tells the roi that it should be the focus of input and it
    // should take appropriate actions (such as highlighting or other init)
    virtual int Focus_In() {
        return false;
    }
    // FocusOut tells the roi that it is no longer the input focus
    // and should take appropriate action (especially in undoing the effects
    // of FocusIn()).  Both FocusIn and FocusOut return true if they care
    // about being the input focus, false otherwise.
    virtual int Focus_Out() {
        return false;
    }

    // Recreate creates a new instance of this roi
    // It allows subclasses to create new instances of the proper subclass
    // without the code having to know which subclass is being created.
    //virtual Roi* recreate() {return this;}

    // Only apply to Oval
    virtual ReactionType rotate(short, short) {
        printf("Can't rotate this object\n");
        return REACTION_NONE;
    }
    virtual ReactionType rotate_done(short, short) // Only apply to Oval
    {
        return REACTION_NONE;
    }

    // ROI I/O functions
    virtual void save(std::ofstream &, bool) = 0;
    //virtual void load(std::ifstream &) = 0;
    static void load_roi(std::ifstream &);

    // Only apply to Polygon
    virtual ReactionType mouse_middle(void) {
        return REACTION_NONE;
    }
    virtual ReactionType mouse_right(void) {
        return REACTION_NONE;
    }

    // For Histogram statistics
    bool getMinMax(double& min, double& max);
    bool histostats(int nbins, double min, double max, RoiData *stats);
    bool getSum(double &sum, int &count);

    void showIntensity(bool updateSlaves=false);

    /*
     virtual void histostats(int *hist, int nbins, double *min, double *max,
     double *median, double *mean, double *sdv,
     double *area);
     */
    virtual void someInfo(bool ifmoving=false, bool clear=false); // look in point.c and line.c

    // Used for image segmentation
    void zero_out(double min, double max);

    //Used to run through all data pixels in ROI
    virtual float *firstPixel();
    virtual float *nextPixel();

    // Functions to convert from pixel to data coords and back
    float xpix_to_data(int xp);
    float ypix_to_data(int yp);
    int data_to_xpix(float x);
    int data_to_ypix(float y);
    void setMagnetCoordsFromPixels();
    bool setPixelCoord(int index, double x, double y);
    bool updateDataCoord(int index);
    void setPixPntsFromData();
    void setMagnetPntsFromData();
    void setMinMaxDataPnts();
    void setDataPntsFromMagnet();
    spCoordVector_t getMagnetCoords();

    // Used after ROI creation/mod
    virtual void update_data_coords();

    // Used after image rotation/reflection
    void rot90_data_coords(int datawidth);
    void flip_data_coords(int datawidth);

    // Used after window move/resize/zoom
    virtual void update_screen_coords();

    // Clips ROI move parameters to keep ROI on image
    void keep_roi_in_image(short *x_motion, short *y_motion);
    void keep_roi_in_image(double *x_motion, double *y_motion);

    // Sets the display clip region
    void setClipRegion(ClipStyle style);

    // Redraw the ROI in copy mode to make it solid
    void draw_solid();

    // Returns true is specified ROI is "similar" to this one
    // in type, shape, and location.
    bool matches(Roi *);

    /* This template compiles with older gnu compilers, but not with version g++4. */
#ifdef USE_AIP_TEMPLATE
    template <class T>
    void createSlaves(const T* type) {
        int binding = (int)getReal("aipRoiBind", 0);
        if (binding == 0) return;

        bool selectFlag = getReal("aipRoiSelectSlavesOnCreation", 0) != 0;
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        GframeList::iterator gfi;

        ReviewQueue *rq = ReviewQueue::get();

        spDataInfo_t dataInfo;
        DataManager *dm = DataManager::get();
        DataMap *dataMap = dm->getDataMap();

        std::set<string> keylist = rq->getKeyset(binding);
        std::set<string>::iterator itr;
        for (itr = keylist.begin(); itr != keylist.end(); ++itr) {
            gf=gfm->getCachedFrame(*itr);
            if (gf != nullFrame && gfm->isFrameDisplayed(gf) &&
                    gf->getFirstImage() != nullImg) {
                if(gf.get() != pOwnerFrame) {
                    Roi *roi = new T(gf, pntData);
                    if (selectFlag) {
                        roi->select(ROI_COPY, true); // Append selection
                    }
                }
            } else {
                dataInfo = DataManager::get()->getDataInfoByKey(*itr);
                if(dataInfo != (spDataInfo_t)NULL) {
                    dataInfo->addRoi("", pntData, false, selectFlag);
                }
            }
        }
    }
#endif

    void createSlavesForData();
};

typedef std::list<Roi *> RoiList;

#endif /* AIPROI_H */
