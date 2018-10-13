/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include <string>
using std::string;
//using namespace std;
#include <cmath>
using std::fabs;

#include "aipCommands.h"
#include "aipStderr.h"
#include "aipWinRotation.h"
#include "aipGframeManager.h"
#include "aipGframe.h"
#include "aipDataInfo.h"
#include "aipDataManager.h"
#include "aipUtils.h"
#include "aipRoi.h"

using namespace aip;

WinRotation *WinRotation::winRotation = NULL;


WinRotation::WinRotation()
{
}

/* Keep only one instance of WinMovie.  Allow anyone who needs it to
 * get it via this static member call.
 */
/* STATIC */
WinRotation *
WinRotation::get()
{
    if (!winRotation) {
	winRotation = new WinRotation();
    }
    return winRotation;
}

/* STATIC */
int
WinRotation::aipRotate(int argc, char *argv[], int retc, char *retv[])
{
    if(argc < 2) {
	if(strcmp(argv[0], "aipRotate") == 0) {
	    STDERR("Usage: aipRotate 90/180/270/-90\n");
	}
	else if(strcmp(argv[0], "aipFlip") == 0) {
	    STDERR("Usage: aipFlip 0/45/90/135\n");
	}
	return proc_error;
    }
    if(strcmp(argv[0], "aipRotate") == 0) {
	// If aipRotate command, just send the arg which should be a number.
	WinRotation::rotate(argv[1]);
    }
    else if(strcmp(argv[0], "aipFlip") == 0) {
	// If aipFlip, send arg prepended with the string 'flip'.
	char str[64];
	sprintf(str, "flip%s", argv[1]);
	WinRotation::rotate(str);
    }

    return proc_complete;
}


/************************************************************************
 *                                                                       *
 *  Process image rotation/reflection command.
 *  Performs operation on images in all selected frames.
 *  LIMITATIONS:
 *  Does not handle overlay images.
 *  Only deals with "2dfov" data.
 */
/* STATIC */
void
WinRotation::rotate(char * rottype)
{
    // should not use VsMode, so commented out all cases except last one. 
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    spDataInfo_t dataInfo;
    GframeList::iterator gfi;
    DataManager *dm = DataManager::get();
    int mode = VsInfo::getVsMode();
    set<string> keys;
    switch (mode) {
    default:
    case VS_UNIFORM:
    case VS_DISPLAYED:
    case VS_SELECTEDFRAMES:
    case VS_INDIVIDUAL:
    case VS_OPERATE:
    case VS_GROUP:
    case VS_HEADER:
        keys = dm->getKeys(DATA_SELECTED_FRAMES);
        if(keys.size() <= 0) keys = dm->getKeys(DATA_DISPLAYED);
        break;
    }

    set<string>::iterator itr;
    for(itr=keys.begin(); itr!=keys.end(); ++itr) {
        spGframe_t gf = gfm->getCachedFrame(*itr);
        if (gf == nullFrame) continue;

        spViewInfo_t view = gf->getSelView();
        if (view == nullView) {
            continue;           // If no image, skip this frame
        }
        int rotation = view->getRotation();
        spImgInfo_t img = view->imgInfo;

	// If frame contains an image, rotate (or flip) it.
	if(strcmp(rottype, "90") == 0) {
            rotation = ( ((rotation & 4) ^ 4) // Toggle transpose bit
                         | (((rotation & 1) ^ 1) << 1) // y <-- -x
                         | ((rotation & 2) >> 1)); // x <-- y
	} else if (strcmp(rottype, "180") == 0) {
            rotation ^= 3;      // x <-- -x, y <-- -y
	} else if (strcmp(rottype, "270") == 0 || strcmp(rottype, "-90") == 0) {
            rotation = ( ((rotation & 4) ^ 4) // Toggle transpose bit
                         | ((rotation & 1) << 1) // y <-- x
                         | (((rotation & 2) ^ 2) >> 1)); // x <-- -y
	} else if (strcmp(rottype, "flip0") == 0) {
            rotation ^= 1;      // x <-- -x
	} else if (strcmp(rottype, "flip90") == 0) {
            rotation ^= 2;     // y <-- -y
	} else if (strcmp(rottype, "flip135") == 0) {
            rotation = ( ((rotation & 4) ^ 4) // Toggle transpose bit
                         | (((rotation & 1) ^ 1) << 1) // y <-- -x
                         | (((rotation & 2) ^ 2) >> 1)); // x <-- -y
	} else if (strcmp(rottype, "flip45") == 0) {
            rotation = ( ((rotation & 4) ^ 4) // Toggle transpose bit
                         | ((rotation & 1) << 1) // y <-- x
                         | ((rotation & 2) >> 1)); // x <-- y
	} else {
            // No change to picture
	    fprintf(stderr,"WinRotation: Illegal operation: %s\n", rottype);
            return;
	}

        view->setRotation(rotation);
        gf->setZoom(img->datastx + img->datawd / 2,
                    img->datasty + img->dataht / 2,
                    gf->pixelsPerCm);
    }
}

/* STATIC */
int
WinRotation::calcRotation(spDataInfo_t di)
{
    int i, j, k;
    int rotation = 0;

    string policy = getString("aipRotationPolicy", "none");
    if (policy == "none") {
        rotation = 0;
    } else if (policy == "radiological" || policy == "neurological") {
        // Rotate to std image orientation

        // Get body position in magnet
        char *p1 = (char *)"";
        char *p2 = (char *)"";
        di->st->GetValue("position1", p1);
        di->st->GetValue("position2", p2);
        //string position1 = p1;
        //string position2 = p2;
        int pos1, pos2;
        if (strcasecmp(p1, "head first") == 0) {
            pos1 = 'h';
        } else if (strcasecmp(p1, "feet first") == 0) {
            pos1 = 'f';
        } else {
            pos1 = 'h'; //return 0;
        }
        if (strcasecmp(p2, "supine") == 0) {
            pos2 = 's';
        } else if (strcasecmp(p2, "prone") == 0) {
            pos2 = 'p';
        } else if (strcasecmp(p2, "left") == 0) {
            pos2 = 'l';
        } else if (strcasecmp(p2, "right") == 0) {
            pos2 = 'r';
        } else {
            pos2 = 's'; //return 0;
        }

        double m2b[3][3];       // Magnet -> body rotation
        calcMagnetToBodyRotation(pos1, pos2, m2b);

        // d2m is the 3X3 data to magnet rotation matrix; just
        // the transpose of orientation[9].
        // Note that di->d2m is different.
        double d2m[3][3];       // Data -> magnet rotation
        double orientation[9];
        di->getOrientation(orientation);
        for (i=0, j=0; j<3; j++) {
            for (k=0; k<3; k++, i++) {
                d2m[k][j] = orientation[i]; // Load TRANSPOSE of rotation matrix
            }
        }
        
        double snap_d2m[3][3];  // Data -> magnet (snap to nearest 90 deg)
        snapRotationTo90(d2m, snap_d2m);

        double d2b[3][3];      // Data -> body rotation
        multMat3(m2b, snap_d2m, d2b);

        // See where the Z data direction comes from to get the type of slice
        // NB: This assumes we have snapped to 90 degrees; only one of
        // d2b[0][2], d2b[1][2], d2b[2][2] is non-zero.
        int slice;
        if (d2b[0][2] != 0) {
            slice = 's';        // X body comes from Z data
        } else if (d2b[1][2] != 0) {
            slice = 'c';        // Y body comes from Z data
        } else {
            slice = 't';        // Z body comes from Z data
        }
        
        double b2p[3][3];       // Body -> pix rotation
        calcBodyToPixRotation(slice, policy[0], b2p);

        double d2p[3][3];       // Data -> pix rotation
        multMat3(b2p, d2b, d2p);

        rotation = 0;
        if (fabs(d2p[0][0]) < 0.5) {
            rotation |= 4;      // Set transpose bit
            if (d2p[0][1] < 0) {
                rotation |= 1;  // X pixels are backward
            }
            if (d2p[1][0] < 0) {
                rotation |= 2;  // Y pixels are backward
            }
        } else {
            if (d2p[0][0] < 0) {
                rotation |= 1;  // X pixels are backward
            }
            if (d2p[1][1] < 0) {
                rotation |= 2;  // Y pixels are backward
            }
        }
    }
    return rotation;
}

/* STATIC */
bool
WinRotation::calcBodyToPixRotation(int slice, int policy, double b2p[3][3])
{
    // NB: The Y pixel direction is DOWN
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            b2p[i][j] = 0;
        }
    }
    switch (slice) {
      case 't':                 // Transverse
        b2p[1][1] = -1;         // Nose up
        if (policy == 'n') {
            // Neurological
            b2p[0][0] = 1;      // Right on right
            b2p[2][2] = -1;     // Keep coord system rt handed
        } else {
            // Radiological
            b2p[0][0] = -1;     // Right on left
            b2p[2][2] = 1;      // Keep coord system rt handed
        }
        return true;

      case 's':                 // Sagittal
        b2p[0][1] = -1;         // Nose left
        b2p[1][2] = -1;         // Head up
        b2p[2][0] = 1;          // Keep coord system rt handed
        return true;

      case 'c':                 // Coronal
        b2p[1][2] = -1;         // Head up
        if (policy == 'n') {
            // Neurological
            b2p[0][0] = 1;      // Right on right
            b2p[2][1] = 1;      // Keep coord system rt handed
        } else {
            // Radiological
            b2p[0][0] = -1;     // Right on left
            b2p[2][1] = -1;     // Keep coord system rt handed
        }
        return true;
    }
    b2p[0][0] = b2p[1][1] = b2p[2][2] = 1;
    return false;
}

/* STATIC */
bool
WinRotation::calcMagnetToBodyRotation(int pos1, int pos2, double m2b[3][3])
{
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            m2b[i][j] = 0;
        }
    }
    if (pos1 == 'f') {
        m2b[2][2] = 1;          // Head at +Z
        if (pos2 == 'l') {
            m2b[1][0] = -1;     // Nose at -X
        } else if (pos2 == 'r') {
            m2b[1][0] = 1;      // Nose at +X
        } else if (pos2 == 'p') {
            m2b[0][0] = -1;      // Rt at -X
        } else { /* 's' */
            m2b[0][0] = 1;     // Rt at +X
        }
    } else /* 'h' */ {
        m2b[2][2] = -1;         // Head at -Z
        if (pos2 == 'l') {
            m2b[1][0] = 1;      // Nose at +X
        } else if (pos2 == 'r') {
            m2b[1][0] = -1;     // Nose at -X
        } else if (pos2 == 'p') {
            m2b[0][0] = 1;      // Rt at +X
        } else { /* 's' */
            m2b[0][0] = -1;     // Rt at -X
        }
    }
    if (pos2 == 'l') {
        m2b[0][1] = 1;          // Rt at +Y
    } else if (pos2 == 'r') {
        m2b[0][1] = -1;         // Rt at -Y
    } else if (pos2 == 'p') {
        m2b[1][1] = -1;         // Nose at -Y
    } else { /* 's' */
        m2b[1][1] = 1;          // Nose at +Y
    }
    if ((pos1 != 'h' && pos1 != 'f') ||
        (pos2 != 'l' && pos2 != 'r' && pos2 != 'p' && pos2 != 's'))
    {
        return false;
    }
    return true;
}

/* STATIC */
bool
WinRotation::snapRotationTo90(double src[3][3], double dst[3][3])
{
    int i, j;
    for (i=0; i<3; ++i) {
        for (j=0; j<3; ++j) {
            dst[i][j] = 0;
        }
    }
    int bigx, bigy, bigz;
    if (fabs(src[0][0]) < fabs(src[0][1])) {
        if (fabs(src[0][1]) < fabs(src[0][2])) {
            bigx = 2;
        } else {
            bigx = 1;
        }
    } else if (fabs(src[0][0]) < fabs(src[0][2])) {
        bigx = 2;
    } else {
        bigx = 0;
    }
    i = (bigx + 1) % 3;
    j = (i + 1) % 3;
    bigy = fabs(src[1][i]) < fabs(src[1][j]) ? j : i;
    bigz = bigy == i ? j : i;
    dst[0][bigx] = src[0][bigx] > 0 ? 1 : -1;
    dst[1][bigy] = src[1][bigy] > 0 ? 1 : -1;
    dst[2][bigz] = src[2][bigz] > 0 ? 1 : -1;
    return true;
}
