/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "aipVolData.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipImgInfo.h"
#include "aipStderr.h"
#include "aipDataManager.h"
#include "aipCommands.h"
#include "aipDataInfo.h"
#include "group.h"
#include "aipReviewQueue.h"
#include "aipCInterface.h"
#include "aipOrthoSlices.h"

#define DEG_TO_RAD (M_PI/180.)
#define D360 360.
#define D180 180.
#define D90 90.
#define D270 270.
const float eps = 1.0e-4;
using namespace aip;

VolData VolData::volData;

extern "C" {
int mSync(MFILE_ID md);
void set3Pmode(int m);
void rotateu2m(float3 cor, float* d);
void rotatem2u(float3 cor, float* d);
}
VolData::VolData(void) {
    volImagePath[0] = 0;
    volMapFile[0]=0;
    dataInfo = (spDataInfo_t)NULL;
    mobj=(MFILE_ID)NULL;
    data_valid=false;
    overlayFlg=false;
}

VolData::~VolData() {
    freeMapData();
}


bool VolData::showingObliquePlanesPanel() {
    double value=0;
    P_getreal(GLOBAL,"showObliquePlanesPanel",&value,1);
    if (value>0 && data_valid)
        return true;
    return false;
}
/*
 * Returns the one VolData instance.
 */
/* PUBLIC STATIC */
VolData *VolData::get() {
    return &volData;
}
/* routines stolen from iplan.c */
void snapAngle(float* a)
{
    float angle, delta = 1.0;
    angle = *a;
    if ((angle > -delta && angle < delta) || (angle > D360-delta&& angle
            < D360+delta))
        angle = 0.0;
    else if (angle > D90-delta&& angle < D90+delta)
        angle = 90;
    else if (angle > -D90-delta && angle < -D90+delta)
        angle = -90;
    else if (angle > D180-delta&& angle < D180+delta)
        angle = 180;
    else if (angle > -D180-delta && angle < -D180+delta)
        angle = -180;
    else if (angle > D270-delta&& angle < D270+delta)
        angle = 270;
    else if (angle > -D270-delta && angle < -D270+delta)
        angle = -270;

    *a = angle;
}

void multiplyAB(double* a, double* b, double* c, int row, int col)
/******************/
/*   C = A.B  */
{
    int i, j, k;
    int l = 0;
    double* d;

    d = (double*)malloc(sizeof(double)*row*col);

    for (i=0; i<row; i++) {
        for (j=0; j<col; j++) {
            d[l] = 0.0;
            for (k=0; k<row; k++) {
                d[l] += a[i*row+k]*b[k*col+j];
            }
            l++;
        }
    }
    for (i=0; i<row*col; i++)
        c[i] = d[i];
    for (i=0; i<row*col; i++)
        if (fabs(c[i]) > 1.0)
            c[i] = c[i]/fabs(c[i]);
    free(d);
}

void VolData::setMatrixLimits() {
    if (dataInfo == (spDataInfo_t)NULL)
        return;
    DDLSymbolTable *st = dataInfo->st;
    int ret = 1;
    if (st)
        st->GetValue("matrix", ret, 2);
    P_setreal(GLOBAL, "aipXYlast", (double)(ret-1), 0);
    ret = 1;
    if (st)
        st->GetValue("matrix", ret, 1);
    P_setreal(GLOBAL, "aipXZlast", (double)(ret-1), 0);
    ret = 1;
    if (st)
        st->GetValue("matrix", ret, 0);
    P_setreal(GLOBAL, "aipYZlast", (double)(ret-1), 0);
}

void VolData::showObliquePlanesPanel(bool show) {
    double value=0;
    bool same_val=false;

    if (P_getreal(GLOBAL,  "showObliquePlanesPanel", &value, 1 ) == 0){
        if(value==0 && !show)
            same_val=true;
        if(value>0 && show)
            same_val=true;
    }

    if (!same_val) {
        if (show) {
            P_setreal(GLOBAL, "showObliquePlanesPanel", (double)1.0, 0);
        } else
            P_setreal(GLOBAL, "showObliquePlanesPanel", (double)0.0, 0);
        if(!show)
            set3Pmode(show);
    }
    if(show)
        set3Pmode(show);
    //OrthoSlices::get()->setShowCursors(show);

    //setMatrixLimits();

    // Execute pnew
    // char pnewcmd1[32];
    // strcpy(pnewcmd1, "pnew 5 ");
    char pnewcmd2[80];
    strcpy(pnewcmd2, "aipXYlast aipXZlast aipYZlast showObliquePlanesPanel");
    // writelineToVnmrJ(pnewcmd1, pnewcmd2);
    appendJvarlist(pnewcmd2);
}

/************************************************************************
 *                                                                       *
 *  Show the slice extraction panel.  Have the panel use the vnmr variable
 *  'enableExtractPanel' to enable and disable.
 *
 *                                  */
void VolData::enableExtractSlicesPanel(bool show) {
    // Set the vnmr variable enableExtractPanel to 1 or 0.
    if (show) {
        P_setreal(GLOBAL, "enableExtractPanel", (double)1.0, 0);
        //setMatrixLimits();
    } else {
        P_setreal(GLOBAL, "enableExtractPanel", (double)0.0, 0);
    }
    // Execute pnew on enableExtractPanel
    // char pnewcmd1[32];
    // strcpy(pnewcmd1, "pnew 4 ");
    char pnewcmd2[64];
    strcpy(pnewcmd2, "enableExtractPanel aipXYlast aipXZlast aipYZlast");
    // writelineToVnmrJ(pnewcmd1, pnewcmd2);
    appendJvarlist(pnewcmd2);
}

int VolData::ExtractObl(int argc, char **argv, int, char **) {
    char *name = *argv;
    VolData *vdat = VolData::get();
    argc--;
    argv++;
    double rx,ry,rz;

    if (!vdat || vdat->volImagePath[0] == '\0') {
        // No volume data loaded yet
        STDERR_1("%s: No 3D data set available", name);
        return proc_error;
    }
    char str[300];
    char usage[100];
    int error;

    sprintf(
            usage,
            "usage: %s(orientation number, first_slice, last_slice, [incr, [MIP_flag]])",
            name);
    if (!argc) {
        ib_errmsg(usage);
        return proc_error;
    }

    if ((argc < 1) || (argc > 7)) {
        ib_errmsg(usage);
        return proc_error;
    }
    int slices[40];
    int ni = vdat->getIntArgs(argc, argv, slices, argc);
    if (ni != argc) {
        ib_errmsg(usage);
        return proc_error;
    }
    float rots[]= { 0., 0., 0. };
    // get rotation angles from the 3D panel
    if(OrthoSlices::get()->Getg3drot(rx,ry,rz)>0){
        rots[0]=rx;
        rots[1]=ry;
        rots[2]=rz;
   }

    float angles[]={1.,1.,1.};
    if (ni>=1) {
        angles[0]=angles[1]=angles[2]=(float)slices[0]; // use first "slice" as the frame number
    }

    // figure out closest orientation of rotated slices
    rx= fabs(sin(DEG_TO_RAD*angles[0])*sin(DEG_TO_RAD*angles[2]));
    ry= fabs(cos(DEG_TO_RAD*angles[0])*sin(DEG_TO_RAD*angles[2]));
    rz= fabs(cos(DEG_TO_RAD*angles[2]));

    double rmax = rz;
    int lmax = vdat->dataInfo->getSlow();
    int orient=FRONT_PLANE;

    if (rx>rmax) {
        rmax=rx;
        orient=SIDE_PLANE;
        lmax = vdat->dataInfo->getFast();
    }
    if (ry>rmax) {
        rmax=ry;
        orient=TOP_PLANE;
        lmax = vdat->dataInfo->getMedium();
    }
    lmax=lmax-1;

    int first, last, incr, mip_flag;

    first=1;
    if (ni >= 2)
        first=slices[1];

    last=first;
    if (ni >= 3)
        last=slices[2];

    incr=1;
    if (ni >= 4)
        incr = slices[3];

    mip_flag=FALSE;
    if (ni >= 5) {
        mip_flag=slices[4];
        if ((mip_flag!=TRUE)&&(mip_flag!=FALSE)) {
            (void)sprintf(str, "Invalid MIP_flag: %d \n", mip_flag);
            ib_errmsg(str);
            ib_errmsg(usage);
            return proc_error;
        }
    }
    else
    {
        error=OrthoSlices::get()->MipMode();
        if (error>=0) {
            if (error>0)
                mip_flag=TRUE;
            else
                mip_flag=FALSE;
        }
    }



    if (incr < 1)
        incr = 1;
    if (first<0)
        first=0;
    if (first>lmax)
        first=lmax;
    if (last<0)
        last=lmax;
    if (last>lmax)
        last=lmax;

    // Trap for last < first and fix it.
    if (last < first)
        last = first;

    int slicelist[512];
    int i, nslices;

    nslices=0;
    for (i=first; i<=last; i+=incr) {
        slicelist[nslices]=i;
        nslices++;
    }

    //vdat->extract_oblplanes(angles, rots, nslices, slicelist, mip_flag, 1);
    vdat->extract_oblplanes(angles, rots, nslices, slicelist, mip_flag, 0);

    ReviewQueue::get()->displayPlanes();

    return proc_complete;
}

int VolData::show3Planes(int argc, char **argv, int, char **) {
    int show; // Normalized coordinates

    if (argc < 1)
        return 0;

    show = atoi(argv[1]);
    VolData::get()->showObliquePlanesPanel((bool)show);
    return 1;
}


int VolData::Extract(int argc, char **argv, int, char **) {
    char *name = *argv;
    VolData *vdat = VolData::get();
    argc--;
    argv++;

    if (!vdat || vdat->volImagePath[0] == '\0') {
        // No volume data loaded yet
        STDERR_1("%s: No 3D data set available", name);
        return proc_error;
    }

    char usage[100];
    sprintf(usage,
            "usage: %s(['xy'|'yz'|'xz'], first_slice [, last_slice [, incr]])",
            name);
    if (!argc) {
        STDERR(usage);
        return proc_error;
    }
    int orient = FRONT_PLANE;
    if (strcasecmp(*argv, "xy") == 0) {
        orient = FRONT_PLANE;
        argc--;
        argv++;
    } else if (strcasecmp(*argv, "xz") == 0) {
        orient = TOP_PLANE;
        argc--;
        argv++;
    } else if (strcasecmp(*argv, "yz") == 0) {
        orient = SIDE_PLANE;
        argc--;
        argv++;
    }

    if ((argc < 1) || (argc > 3)) {
        STDERR(usage);
        return proc_error;
    }
    int slices[3];
    int ni = vdat->getIntArgs(argc, argv, slices, argc);
    if (ni != argc) {
        STDERR(usage);
        return proc_error;
    }
    int first = slices[0];
    int last = first;
    int incr = 1;
    if (ni >= 2) {
        last = slices[1];
    }
    if (ni == 3) {
        incr = slices[2];
    }
    // Trap for incr less than one and correct it
    if (incr < 1)
        incr = 1;

    // last may be -1 to show all images of selected plane.
    if (last < 0&& vdat->dataInfo != (spDataInfo_t)NULL) {
        if (orient == FRONT_PLANE)
            last = vdat->dataInfo->getSlow() -1;
        if (orient == TOP_PLANE)
            last = vdat->dataInfo->getMedium() -1;
        if (orient == SIDE_PLANE)
            last = vdat->dataInfo->getFast() -1;
    }

    // Trap for last < first and fix it.
    if (last < first)
        last = first;

    vdat->extract_planes(orient, first, last, incr);

    // Display all of the new frames
    //DataManager *dm = DataManager::get();
    //dm->displayAll();
    ReviewQueue::get()->displayPlanes();

    return proc_complete;
}

/************************************************************************
 *                                  *
 *  Extract Maximum Intensity Projections from a 3D data set.
 *  [MACRO interface]
 *  argv[0]: See "usage" string, below.
 *  argv[1]: ...
 *  [STATIC Function]                           *
 *                                  */
int VolData::Mip(int argc, char **argv, int, char **) {
    char *name = *argv;
    VolData *vdat = VolData::get();

    argc--;
    argv++;
    if (!vdat || vdat->volImagePath[0] == '\0') {
        // No volume data loaded yet
        STDERR_1("%s: No 3D data set available", name);
        return proc_error;
    }

    char usage[100];
    sprintf(usage,
            "usage: %s(['xy'|'yz'|'xz'], first_slice [, last_slice [, incr]])",
            name);
    if (!argc) {
        STDERR(usage);
        return proc_error;
    }

    int orient = FRONT_PLANE;
    if (strcasecmp(*argv, "xy") == 0) {
        orient = FRONT_PLANE;
        argc--;
        argv++;
    } else if (strcasecmp(*argv, "xz") == 0) {
        orient = TOP_PLANE;
        argc--;
        argv++;
    } else if (strcasecmp(*argv, "yz") == 0) {
        orient = SIDE_PLANE;
        argc--;
        argv++;
    }

    if ((argc < 1) || (argc > 3)) {
        STDERR(usage);
        return proc_error;
    }
    int slices[3];
    int ni = vdat->getIntArgs(argc, argv, slices, argc);
    if (ni != argc) {
        STDERR(usage);
        return proc_error;
    }
    int first = slices[0];
    int last = first;
    int incr = 1;
    if (ni >= 2) {
        last = slices[1];
    }
    if (ni == 3) {
        incr = slices[2];
    }
    // Trap for incr less than one and correct it
    if (incr < 1)
        incr = 1;

    // Trap for last < first and fix it.
    if (last < first)
        last = first;

    vdat->extract_mip(orient, first, last, incr);

    // Display all of the new frames
    //DataManager *dm = DataManager::get();
    //dm->displayAll();
    ReviewQueue::get()->displayPlanes();

    return proc_complete;
}

// Set the volumn image path for use by subsequent commands.
void VolData::setVolImagePath(const char *newPath) {
    VolData *vdat = VolData::get();
    size_t len = sizeof(vdat->volImagePath);
    strncpy(vdat->volImagePath, newPath, len);
    vdat->volImagePath[len-1] = '\0';
}

char *VolData::getVolImagePath() {
    VolData *vdat = VolData::get();
    return vdat->volImagePath;
}

bool VolData::validVolImagePath() {
    return (volImagePath[0]==0) ? false : true;
}

// Set the volumn image path for use by subsequent commands.
bool VolData::setMapFile(const char *mapfile) {
    int fd = open(mapfile, O_CREAT | O_TRUNC| O_RDWR, 0666);
    if (fd == -1) {
        return false;
    }
    close(fd);
    size_t len = sizeof(volMapFile);
    strncpy(volMapFile, mapfile, len);
    volMapFile[len-1] = '\0';
    return true;
}

void VolData::setDfltMapFile() {
    char str[PATH_MAX];
    char path[PATH_MAX];
    int vp = (int)getReal("jviewport", 1);
    str[0]=0;
    P_getstring(GLOBAL, "volmapdir", str, 1, 256);
    if (strlen(str)==0) {
        size_t len = sizeof(str);
        strncpy(str,userdir, len);
        str[len-1] = '\0';
    }
    freeMapData();
    if (strlen(str)>0) {
        int fd = open(str, O_RDWR, 0666);
        if (fd != -1) {
            close(fd);
            size_t len = sizeof(path);
            snprintf(path, len, "%s/volmap%d", str, vp);
            path[len-1] = '\0';
            setMapFile(path);
            return;
        }
     }
     volMapFile[0]=0;
}

char *VolData::getMapFile() {
    return volMapFile;
}

bool VolData::validMapFile() {
    double value=0;
    if(volMapFile[0]==0)
        return false;
    if(P_getreal(GLOBAL,"g3ds",&value,5)==0){
        int test=(int)value;
        return ((test & 64)>0)? false : true;
    }
    return false;
}

bool VolData::validData() {
    return data_valid;
}

bool VolData::volDataIsMapped() {
    return mobj==(MFILE_ID)0 ? false : true;
}
void VolData::freeData() {
    freeMapData();
    data_valid=false;
}
void VolData::freeMapData() {
    if (mobj!=(MFILE_ID)NULL) {
        DataInfo *oldinfo=dataInfo.get();
        DDLSymbolTable *st=oldinfo->st;
        st->ClrData();
        mClose(mobj);
        mobj=(MFILE_ID)NULL;
        oldinfo->dataStruct->data=NULL;
        dataInfo = (spDataInfo_t)NULL;
    }
    if (volMapFile[0]!=0) {
        remove(volMapFile);
    }
    volMapFile[0]=0;
}

char *VolData::mapVolData(DDLSymbolTable *st) {
    char *err=st->MapData(volMapFile, &mobj);
    return err;
}

char *VolData::setVolData(DDLSymbolTable *st) {
    char *err=0;
    if(validMapFile())
        err=mapVolData(st);
    if(err==0 || err[0]!=0)
        err=st->MallocData();
    data_valid=(err[0]==0)?true:false;
    if(!err)
    	setMatrixLimits();
    return err;
}

/************************************************************************
 *                                                                       *
 * Extract slices in given (orthogonal) plane.
 */
void VolData::extract_planes(int orientation, int first, int last, int incr) {
    int i;
    //char macrocmd[100];
    int planelist[] = { FRONT_PLANE, TOP_PLANE, SIDE_PLANE };
    //const char *planenames[] = { "xy", "xz", "yz" };

    VolData *vdat = VolData::get();

    vdat->showObliquePlanesPanel(false);

    first = vdat->clip_slice(orientation, first);
    last = vdat->clip_slice(orientation, last);

    for (i=first; i<=last; i+=incr) {
        vdat->extract_plane(planelist[orientation], 1, &i, first, last);
    }

    //    sprintf(macrocmd,"vol_extract('%s', %d, %d, %d)\n",
    //      planenames[orientation], first, last, incr);
    //    macroexec->record(macrocmd);
}

/************************************************************************
 *                                                                       *
 * Extract MIP in an orthogonal plane.
 */
void VolData::extract_mip(int orientation, int first, int last, int incr) {
    int i;
    int j;
    //char macrocmd[100];
    //const char *planenames[] = { "xy", "xz", "yz" };
    VolData *vdat = VolData::get();

    first = vdat->clip_slice(orientation, first);
    last = vdat->clip_slice(orientation, last);
    int nslices = (last - first + incr) / incr;
    nslices = nslices < 1 ? 1 : nslices;
    int *slicelist = new int[nslices];
    for (i=first, j=0; i<=last; i+=incr, j++) {
        slicelist[j] = i;
    }
    vdat->extract_plane(orientation, nslices, slicelist, first, last);
    delete[] slicelist;

    //    sprintf(macrocmd,"vol_mip('%s', %d, %d, %d)\n",
    //      planenames[orientation], first, last, incr);
    //    macroexec->record(macrocmd);
}

/************************************************************************
 *                                                                       *
 *  Extract a slice of data or the MIP of a list of slices.
 *  A simple extraction is just a MIP over one slice.
 *  Set "orientation" to FRONT_PLANE, TOP_PLANE, or SIDE_PLANE.
 *  Set "slicelist" to the array of slice indices to do and "nslices"
 *  to the number of slices in the array.
 *
 * Note: this is extended to accomodate a set of euler angles, which are
 * rotations from the plane described by orientation - M. Kritzer 1/25/2007
 *
 *                                                                      */
void VolData::extract_oblplanes(float *eulers, float *rotangles, int nslices,
        int *slicelist, int mip, int mag_frame) {

    // NB: the secret to euler angles:
    //     with (psi, phi, theta)
    //     (0,0,0) gives axial extraction (ROxPE)
    //     (0,0,90) gives coronal (ROxPE2)
    //     (90,0,90) gives sagittal (PExPE2)

    int i;
    int nx, ny, nz;
    int nx0, ny0, nz0;
    int islice;
    int ix, iy;
    int xn, yn, zn;
    int xm, ym, zm;
    int pmx, pmy, pmz;
    int xc, yc, zc;
    int xc0, yc0, zc0;
    int slice;
    int first, last;
    int inpoints;
    int trilinear=TRUE;
    int orientation;
    int msize=3;
    int error;
    int *tslicelist=slicelist;

    double rx0, ry0, rz0;
    double rx1, ry1, rz1;
    double xres, yres, zres;
    double xres0, yres0, zres0;
    double xfov, yfov, zfov;
    double tx0, ty0, tz0;
    double tx1, ty1, tz1;
    double zscale;
    double rot[9];
    double frot[9];
    double arot[9];
    double newrot[9];
    double trot[9];
    double srot[9];
    double r[9]= { 0., -1., 0., 1., 0., 0., 0., 0., 1. };
    //double rt[9]= { 0., 1., 0., -1., 0., 0., 0., 0., 1. };
    double rotX[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double rotY[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double rotZ[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double hrotX[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double hrotY[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double hrotZ[9]= { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
    double atheta, aphi, apsi;
    double stheta;
    double dtemp;
    double psi, phi, theta;
    double cospsi, cosphi, costheta;
    double sinpsi, sinphi, sintheta;
    double acospsi, acosphi, acostheta;
    double asinpsi, asinphi, asintheta;
    double angX, angY, angZ;
    double rx, ry, rz, rmax;

    float i1, i2, j1, j2, w1, w2;
    float dx, dy, dz;
    float *data;
    float bufpt, mippt;

    spDataInfo_t datainfo; // New 2D datainfo, dataInfo is orig 3D version
    char floatStr[16];

    mippt = 0;
    if (dataInfo == (spDataInfo_t)NULL)
        return;

    data = (float *)dataInfo->getData(); // we will need all of it


    strcpy(floatStr, "float");

    psi=(double) *(eulers);
    phi=(double) *(eulers+1);
    theta=(double) *(eulers+2);

    theta = DEG_TO_RAD*theta;
    psi = DEG_TO_RAD*psi;
    phi = DEG_TO_RAD*phi;

    cospsi=cos(psi);
    sinpsi=sin(psi);
    cosphi=cos(phi);
    sinphi=sin(phi);
    costheta=cos(theta);
    sintheta=sin(theta);

    angX=-1*DEG_TO_RAD*rotangles[1];
    angY=-1*DEG_TO_RAD*rotangles[0];
    angZ=-1*DEG_TO_RAD*rotangles[2];


    /* figure out simple rotations from rotangles, which came from 3D graphics */
    rotX[4]=cos(angX);
    rotX[5]=-1*sin(angX);
    rotX[7]=sin(angX);
    rotX[8]=cos(angX);

    rotY[0]=cos(angY);
    rotY[2]=sin(angY);
    rotY[6]=-1*sin(angY);
    rotY[8]=cos(angY);

    rotZ[0]=cos(angZ);
    rotZ[1]=-1*sin(angZ);
    rotZ[3]=sin(angZ);
    rotZ[4]=cos(angZ);

    // put opposite rotation into fdf header for planning
    // angX *= -1;
    angY *= -1;
    angZ *= -1;

    hrotX[4]=cos(angX);
    hrotX[5]=-1*sin(angX);
    hrotX[7]=sin(angX);
    hrotX[8]=cos(angX);

    hrotY[0]=cos(angY);
    hrotY[2]=sin(angY);
    hrotY[6]=-1*sin(angY);
    hrotY[8]=cos(angY);

    hrotZ[0]=cos(angZ);
    hrotZ[1]=-1*sin(angZ);
    hrotZ[3]=sin(angZ);
    hrotZ[4]=cos(angZ);



    if (TRUE) // if euler angles are with respect to magnet frame, not acq. frame
    {
        /* solve for acquisition angles */
        (void)dataInfo->GetOrientation(frot);
        if (fabs(frot[8] - 1.0) < eps ) /* theta =0 */
        {
            atheta = 0.0;
            aphi = 0.0;
            apsi = atan2(frot[3], -frot[0]);
            if (fabs(apsi) < eps)
                apsi = 0.0;
            if (apsi < 0.0)
                apsi += 2*M_PI;
        } else if (fabs(frot[8] + 1.0) < eps) {
            /* theta = pi, cos(theta) = -1, sin(theta) = 0, z-axis flipped */
            atheta = M_PI;
            aphi = 0.0;
            apsi = atan2(frot[3], frot[0]);
            if (fabs(apsi) < eps)
                apsi = 0.0;
            if (apsi < 0.0)
                apsi += 2*M_PI;
        } else /* sin of theta is not zero */
        {
            atheta=acos(frot[8]);
            if (fabs(atheta) < eps)
                atheta = 0.0;
            stheta=sin(atheta);
            apsi=atan2(frot[7]/stheta, (frot[6])/stheta);
            aphi=atan2(frot[5]/stheta, frot[2]/stheta);
            if (fabs(aphi) < eps)
                aphi = 0.0;
            if (aphi<0.0)
                aphi += 2*M_PI;
            if (fabs(apsi) < eps)
                apsi = 0.0;
            if (apsi<0.0)
                apsi += 2*M_PI;
        }

        acospsi=cos(apsi);
        asinpsi=sin(apsi);
        acosphi=cos(aphi);
        asinphi=sin(aphi);
        acostheta=cos(atheta);
        asintheta=sin(atheta);

        /* acq rotation in 2D recon form */
        /*
         frot[0]=-1*acosphi*acospsi - asinphi*acostheta*asinpsi;
         frot[1]=-1*acosphi*asinpsi + asinphi*acostheta*acospsi;
         frot[2]=-1*asinphi*asintheta;
         frot[3]=-1*asinphi*acospsi + acosphi*acostheta*asinpsi;
         frot[4]=-1*asinphi*asinpsi - acosphi*acostheta*acospsi;
         frot[5]=acosphi*asintheta;
         frot[6]=-1*asintheta*asinpsi;
         frot[7]=asintheta*acospsi;
         frot[8]=acostheta;
         */

        /* classic euler form */
        /*
         frot[0]= acosphi*acospsi - asinphi*acostheta*asinpsi;
         frot[1]= acosphi*asinpsi + asinphi*acostheta*acospsi;
         frot[2]= asinphi*asintheta;
         frot[3]=-1*asinphi*acospsi - acosphi*acostheta*asinpsi;
         frot[4]=-1*asinphi*asinpsi + acosphi*acostheta*acospsi;
         frot[5]= acosphi*asintheta;
         frot[6]= asintheta*asinpsi;
         frot[7]=-1*asintheta*acospsi;
         frot[8]= acostheta;
         */

        /* M2L form */
        frot[0]=asinphi*acospsi - acosphi*acostheta*asinpsi;
        frot[1]=-1*asinphi*asinpsi - acosphi*acostheta*acospsi;
        frot[2]= acosphi*asintheta;
        frot[3]= -1*acosphi*acospsi - asinphi*acostheta*asinpsi;
        frot[4]= acosphi*asinpsi - asinphi*acostheta*acospsi;
        frot[5]= asinphi*asintheta;
        frot[6]= asintheta*asinpsi;
        frot[7]=asintheta*acospsi;
        frot[8]= acostheta;

        /* invert the original acquisition matrix (frot) */
        arot[0]=frot[4]*frot[8]-frot[5]*frot[7];
        arot[1]=frot[2]*frot[7]-frot[1]*frot[8];
        arot[2]=frot[1]*frot[5]-frot[2]*frot[4];
        arot[3]=frot[5]*frot[6]-frot[3]*frot[8];
        arot[4]=frot[0]*frot[8]-frot[2]*frot[6];
        arot[5]=frot[2]*frot[3]-frot[0]*frot[5];
        arot[6]=frot[3]*frot[7]-frot[4]*frot[6];
        arot[7]=frot[1]*frot[6]-frot[0]*frot[7];
        arot[8]=frot[0]*frot[4]-frot[1]*frot[3];

    } // end if angles are with respect to magnet frame

    /*      (void)multiplyAB(arot,r,arot,msize,msize);       */

    // make up rotation matrix

    /* classic Euler rotation */

    rot[0]= cosphi*cospsi - sinphi*costheta*sinpsi;
    rot[1]= cosphi*sinpsi + sinphi*costheta*cospsi;
    rot[2]= sinphi*sintheta;
    rot[3]=-1*sinphi*cospsi - cosphi*costheta*sinpsi;
    rot[4]=-1*sinphi*sinpsi + cosphi*costheta*cospsi;
    rot[5]= cosphi*sintheta;
    rot[6]= sintheta*sinpsi;
    rot[7]=-1*sintheta*cospsi;
    rot[8]= costheta;

    if(*eulers == 1.0)
    {
    rot[0]=1;
    rot[1]=0;
    rot[2]=0;
    rot[3]=0;
    rot[4]=1;
    rot[5]=0;
    rot[6]=0;
    rot[7]=0;
    rot[8]=1;
    }
    else if(*eulers == 2.0)
    {
    rot[0]=0;
	rot[1]=0;
	rot[2]=1;
	rot[3]=0;
	rot[4]=1;
	rot[5]=0;
	rot[6]=1;
	rot[7]=0;
	rot[8]=0;

	rot[0]=0;
	rot[1]=1;
	rot[2]=0;
	rot[3]=0;
	rot[4]=0;
	rot[5]=1;
	rot[6]=1;
	rot[7]=0;
	rot[8]=0;
    }
    else
{
	rot[0]=1;
	rot[1]=0;
	rot[2]=0;
	rot[3]=0;
	rot[4]=0;
	rot[5]=1;
	rot[6]=0;
	rot[7]=1;
	rot[8]=0;
}

    // figure out closest orientation of new slices
    rx= fabs(rot[6]);
    ry= fabs(rot[7]);
    rz= fabs(rot[8]);

    rmax = rz;
    orientation=FRONT_PLANE;
    if (rx>rmax) {
        rmax=rx;
        orientation=SIDE_PLANE;
    }
    if (ry>rmax) {
        rmax=ry;
        orientation=TOP_PLANE;
    }

    // account for rotation from 3D graphics window
    (void)multiplyAB(rotX, rotY, rotX, msize, msize);
    (void)multiplyAB(rotZ, rotX, rotX, msize, msize);
    (void)multiplyAB(rot, rotX, trot, msize, msize);

    if (mag_frame) {
        // (void)multiplyAB(r, rot, rot, msize, msize);
        (void)multiplyAB(rot, r, rot, msize, msize);
        (void)multiplyAB(rot, arot, trot, msize, msize);
    }


    /* make new orientation for 2D */

    /*
    newrot[0]= acosphi*acospsi - asinphi*acostheta*asinpsi;
    newrot[1]= acosphi*asinpsi + asinphi*acostheta*acospsi;
    newrot[2]= asinphi*asintheta;
    newrot[3]=-1*asinphi*acospsi - acosphi*acostheta*asinpsi;
    newrot[4]=-1*asinphi*asinpsi + acosphi*acostheta*acospsi;
    newrot[5]= acosphi*asintheta;
    newrot[6]= asintheta*asinpsi;
    newrot[7]=-1*asintheta*acospsi;
    newrot[8]= acostheta;
    */


    // recon 2D style

//    newrot[0]= -1*acosphi*acospsi - asinphi*acostheta*asinpsi;
//    newrot[1]= -1*acosphi*asinpsi + asinphi*acostheta*acospsi;
//    newrot[2]= -1*asinphi*asintheta;
//    newrot[3]=-1*asinphi*acospsi + acosphi*acostheta*asinpsi;
//    newrot[4]=-1*asinphi*asinpsi - acosphi*acostheta*acospsi;
//    newrot[5]= acosphi*asintheta;
//    newrot[6]= -1*asintheta*asinpsi;
//    newrot[7]=-asintheta*acospsi;
//    newrot[8]= acostheta;

    (void)dataInfo->GetOrientation(newrot); // get original 3D rotation


   // accounts for swap of X and Y in fdf data
    srot[0]= 0.;
    srot[1]= -1;
    srot[2]= 0;
    srot[3]=-1;
    srot[4]=0;
    srot[5]= 0;
    srot[6]= 0;
    srot[7]=0;
    srot[8]= 1;

    /*

    sprintf(str,"newrot = %f %f %f\,",newrot[0],newrot[1],newrot[2]);
        ib_errmsg(str);
        sprintf(str,"newrot = %f %f %f\,",newrot[3],newrot[4],newrot[5]);
        ib_errmsg(str);
        sprintf(str,"newrot = %f %f %f\,",newrot[6],newrot[7],newrot[8]);
        ib_errmsg(str);

        sprintf(str," rot = %f %f %f\,",rot[0],rot[1],rot[2]);
        ib_errmsg(str);
        sprintf(str,"rot = %f %f %f\,",rot[3],rot[4],rot[5]);
        ib_errmsg(str);
        sprintf(str,"rot = %f %f %f\,",rot[6],rot[7],rot[8]);
        ib_errmsg(str);

        */

    // rotation from graphics window - for the fdf header
    (void)multiplyAB(hrotX, hrotY, hrotX, msize, msize);
    (void)multiplyAB(hrotZ, hrotX, hrotX, msize, msize);

    (void)multiplyAB(rot, hrotX, rot, msize, msize);

   // (void)multiplyAB(srot, newrot, newrot, msize, msize);
  //  (void)multiplyAB(rot, newrot, newrot, msize, msize);
//    (void)multiplyAB(newrot, hrotX, newrot, msize, msize);
    (void)multiplyAB(hrotX, newrot, newrot, msize, msize);

    if (fabs(newrot[0])<eps)
        newrot[0]=0.;
    if (fabs(newrot[1])<eps)
        newrot[1]=0.; rot[4]=1;
        rot[5]=0;
        rot[6]=0;
        rot[7]=0;
        rot[8]=1;
    if (fabs(newrot[2])<eps)
        newrot[2]=0.;
    if (fabs(newrot[3])<eps)
        newrot[3]=0.;
    if (fabs(newrot[4])<eps)
        newrot[4]=0.;
    if (fabs(newrot[5])<eps)
        newrot[5]=0.;
    if (fabs(newrot[6])<eps)
        newrot[6]=0.;
    if (fabs(newrot[7])<eps)
        newrot[7]=0.;
    if (fabs(newrot[8])<eps)
        newrot[8]=0.;



    /* spatial information for "front plane" */
    nx = dataInfo->getFast();
    ny = dataInfo->getMedium();
    nz = dataInfo->getSlow();
    xres=dataInfo->getRoi(0)/nx;
    yres=dataInfo->getRoi(1)/ny;
    zres=dataInfo->getRoi(2)/nz;
    xc=nx/2-1;
    yc=ny/2-1;
    zc=nz/2-1;

    switch (orientation) {
    case FRONT_PLANE:
    default:
        nx0 = dataInfo->getFast();
        ny0 = dataInfo->getMedium();
        nz0 = dataInfo->getSlow();
        xfov=dataInfo->getRoi(0);
        yfov=dataInfo->getRoi(1);
        zfov=dataInfo->getRoi(2);
        break;
    case SIDE_PLANE:
    	/*
        nx0 = dataInfo->getSlow();
        ny0 = dataInfo->getMedium();
		nz0 = dataInfo->getFast();
		xfov=dataInfo->getRoi(2);
		yfov=dataInfo->getRoi(1);
		zfov=dataInfo->getRoi(0);
*/
		nx0 = dataInfo->getMedium();
		ny0 = dataInfo->getSlow();
		nz0 = dataInfo->getFast();
		xfov=dataInfo->getRoi(1);
		yfov=dataInfo->getRoi(2);
		zfov=dataInfo->getRoi(0);

        break;
    case TOP_PLANE:
        nx0 = dataInfo->getFast();
        ny0 = dataInfo->getSlow();
        nz0 = dataInfo->getMedium();
        xfov=dataInfo->getRoi(0);
        yfov=dataInfo->getRoi(2);
        zfov=dataInfo->getRoi(1);
        break;
    }

    // find resolutions
    xres0 = xfov/nx0;
    yres0 = yfov/ny0;
    zres0 = zfov/nz0;

    // centers of dimensions
    xc0=nx0/2-1;
    yc0=ny0/2-1;
    zc0=nz0/2-1;

    zscale=zres;
    zscale /=dataInfo->getRoi(2);
    zscale *= dataInfo->getSlow();

    // allocate data buffers
    float *buf = new float[nx0*ny0];
    if (buf == NULL) {
        STDERR("VolData: allocate memory returned NULL pointer.");
        return;
    }
    float *mipbuf = new float[nx0*ny0];



    if (mip &&(mipbuf == NULL)) {
        STDERR("VolData: allocate memory returned NULL pointer.");
        delete[] buf;
        return;
    }

    first=*slicelist;
    last=*(slicelist+nslices-1);

    if (mip && nslices<2) {
         error=OrthoSlices::get()->WidthEnabled();
         if (error>0) {
             error=OrthoSlices::get()->WidthValue(dtemp);
             if(error<0) {
                 STDERR("VolData: Error getting Mip Width\n");
                 delete[] buf;
                 delete[] mipbuf;
                 return;
             }
             else
             {
                 nslices=(int)(nz0*dtemp);
                 error=OrthoSlices::get()->WidthReversed();
                 if(error>0)
                 {
                     if(first+nslices-1>=nz0)
                         nslices=nz0-first;
                     tslicelist = new int[nslices];
                     for (i=0; i<nslices; i++)
                     *(tslicelist+i)=first + i;
                 }
                 else if(error==0) {
                     if((first-nslices+1)<0)
                         nslices=first+1;
                     tslicelist = new int[nslices];
                     for (i=0; i<nslices; i++)
                     *(tslicelist+i)=(first-nslices+1) + i;
                 }
             }
         }
         else {   // no width enabled, default behavior
             nslices=last;
             tslicelist = new int[nslices];
             for (i=0; i<nslices; i++)
             *(tslicelist+i)=i;
         }
     }


    (void)memset(buf, 0, nx0*ny0 * sizeof(float));
    if (mip)
        (void)memset(mipbuf, 0, nx0*ny0 * sizeof(float));

    for (islice=0; islice<nslices; islice++) {
        inpoints=0;
        slice = *(tslicelist+islice); // Prepare to get slice
        // slice = nz0-1-slice;
        if (slice<0)
            slice=0;
        if (slice>=nz0)
            slice=nz0-1;
        rz0=0;

        // find translation vector
        tx0=0.;
        ty0=0.;

        tz0=(double)(slice-zc0);
        tz0 *= zres0;
        //tz0 *= zscale;


        tx1=tx0*trot[0]+ty0*trot[3]+tz0*trot[6];
        ty1=tx0*trot[1]+ty0*trot[4]+tz0*trot[7];
        tz1=tx0*trot[2]+ty0*trot[5]+tz0*trot[8];

        for (iy=0; iy<ny0; iy++) {
            for (ix=0; ix<nx0; ix++) {
                rx0=(ix-xc0);
                ry0=(iy-yc0);
                rx0*=xres0;
                ry0*=yres0;

                if (mip)
		  mippt = mipbuf[nx0*iy + ix];

		//         mippt=mipbuf[nx0*(ny0-1-iy) + ix];

                // rz0=tz0;

                // rotate to new coordinates
                /*
                 rx1=rx0*trot[0]+ry0*trot[1]+rz0*trot[2];
                 ry1=rx0*trot[3]+ry0*trot[4]+rz0*trot[5];
                 rz1=rx0*trot[6]+ry0*trot[7]+rz0*trot[8];
                 */

                /*
                 rx1=rx0*rot[0]+ry0*rot[1]+rz0*rot[2];
                 ry1=rx0*rot[3]+ry0*rot[4]+rz0*rot[5];
                 rz1=rx0*rot[6]+ry0*rot[7]+rz0*rot[8];
                 */

                /*
                 rx1=rx0*rot[0]+ry0*rot[3]+rz0*rot[6];
                 ry1=rx0*rot[1]+ry0*rot[4]+rz0*rot[7];
                 rz1=rx0*rot[2]+ry0*rot[5]+rz0*rot[8];
                 */

                rx1=rx0*trot[0]+ry0*trot[3]+rz0*trot[6];
                ry1=rx0*trot[1]+ry0*trot[4]+rz0*trot[7];
                rz1=rx0*trot[2]+ry0*trot[5]+rz0*trot[8];


                rx1 += tx1;
                ry1 += ty1;
                rz1 += tz1;

                /* convert to original coordinates */
                dx=rx1/xres+xc;
                dy=ry1/yres+yc;
                dz=rz1/zres+zc;

                // find nearest neighbor
                if (trilinear) {
                    xn=(int)(dx);
                    yn=(int)(dy);
                    zn=(int)(dz);
                    dx=dx-xn;
                    dy=dy-yn;
                    dz=dz-zn;
                    pmx=1;
                    pmy=1;
                    pmz=1;

                    if (dx<0.0) {
                        pmx=-1;
                        dx=-1*dx;
                    }
                    if (dy<0.0) {
                        pmy=-1;
                        dy=-1*dy;
                    }
                    if (dz<0.0) {
                        pmz=-1;
                        dz=-1*dz;
                    }
                } else {
                    xn=(int)(dx+0.5);
                    yn=(int)(dy+0.5);
                    zn=(int)(dz+0.5);
                }
                bufpt=0.0;
                if ((xn>=0)&&(xn<nx)&&(yn>=0)&&(yn<ny)&&(zn>=0)&&(zn<nz)) {
                    bufpt=data[zn*nx*ny+yn*nx+xn];
                    if (trilinear) {
                        xm=xn+pmx;
                        ym=yn+pmy;
                        zm=zn+pmz;

                        if ((xm<0)||(xm>=nx))
                            xm=xn;
                        if ((ym<0)||(ym>=ny))
                            ym=yn;
                        if ((zm<0)||(zm>=nz))
                            zm=zn;

                        // interp along z
                        i1=(1-dz)*data[zn*nx*ny+yn*nx+xn];
                        i1+=(dz)*data[zm*nx*ny+yn*nx+xn];
                        i2=(1-dz)*data[zn*nx*ny+ym*nx+xn];
                        i2+=(dz)*data[zm*nx*ny+ym*nx+xn];
                        j1=(1-dz)*data[zn*nx*ny+yn*nx+xm];
                        j1+=(dz)*data[zm*nx*ny+yn*nx+xm];
                        j2=(1-dz)*data[zn*nx*ny+ym*nx+xm];
                        j2+=(dz)*data[zm*nx*ny+ym*nx+xm];

                        // interp along y
                        w1=i1*(1-dy)+ i2*(dy);
                        w2=j1*(1-dy)+ j2*(dy);

                        // interp along x
                        bufpt = w1*(1-dx)+ w2*(dx);

                    } // end if trilinear
                    inpoints++;
                    buf[nx0*iy + ix]=bufpt;
                   //  buf[nx0*(ny0-1-iy) + ix]=bufpt;
                  //  buf[nx0*iy + (nx0-1-ix)]=bufpt;
  //                buf[nx0*(ny0-1-iy) + (nx0-1-ix)]=bufpt;

                } // end of point is inside
                if (mip) {
                    if (bufpt>mippt)
		      //     mipbuf[nx0*(ny0-1-iy) + ix]=bufpt;
                    mipbuf[nx0*iy + ix]=bufpt;
                }
            } // end loop on x
        } // end loop on y

        if (!mip) {
            dataStruct_t *ds2d = new dataStruct_t;
            if(!ds2d) {
                delete[] buf;
                delete[] mipbuf;
                return;
            }
            memcpy(ds2d, dataInfo->dataStruct, sizeof(*ds2d));
            ds2d->data = NULL;
            ds2d->auxparms = NULL;

            DDLSymbolTable *st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
            DDLSymbolTable *st2 = new DDLSymbolTable();
            datainfo = spDataInfo_t(new DataInfo(ds2d, st, st2));
            if(dataInfo == (spDataInfo_t)NULL)
            {
                delete ds2d;
                delete st2;
                delete[] buf;
                delete[] mipbuf;
                STDERR("VolData: allocate memory returned NULL pointer.");
                return;
            }
            datainfo->initializeSymTab(2, 32, floatStr, nx0, ny0, 1, 1);
            datainfo->st->SetData((float *)buf,
            sizeof(float)*nx0*ny0);

            // new rotation matrix for 2D
            /*
            datainfo->st->SetValue("orientation", newrot[0], 0);
            datainfo->st->SetValue("orientation", newrot[1], 1);
            datainfo->st->SetValue("orientation", newrot[2], 2);
            datainfo->st->SetValue("orientation", newrot[3], 3);
            datainfo->st->SetValue("orientation", newrot[4], 4);
            datainfo->st->SetValue("orientation", newrot[5], 5);
            datainfo->st->SetValue("orientation", newrot[6], 6);
            datainfo->st->SetValue("orientation", newrot[7], 7);
            datainfo->st->SetValue("orientation", newrot[8], 8);newrot
            */

             extract_plane_header_obl(datainfo->st, orientation, nslices, tslicelist,newrot);
           //  extract_plane_header(datainfo->st, orientation, nslices, slicelist);

            // Create a name for this new slicenewrot
            char orientStr[16];
            double maxPlanes = 1;
            if(orientation == FRONT_PLANE)
            {
                strcpy(orientStr, "xy");
                maxPlanes = dataInfo->getSlow();
            }
            else if(orientation == TOP_PLANE)
            {
                strcpy(orientStr, "xz");
                maxPlanes = dataInfo->getMedium();
            }
            else if(orientation == SIDE_PLANE)
            {
                strcpy(orientStr, "yz");
                maxPlanes = dataInfo->getFast();
            }
            int digits = maxPlanes > 1 ? 1 + (int)log10(maxPlanes) : 1;
            char newpath[256];
            sprintf(newpath, "%s_%s_%0*d_%d_%d_%d_%d_%d_%d_EXTRA",
            volImagePath, orientStr, digits, *(tslicelist+islice),
            (int)*eulers,(int)*(eulers+1),(int)*(eulers+2),
            (int)*(rotangles),(int)*(rotangles+2),(int)*(rotangles+1) );
            datainfo->st->SetValue("filename", newpath);
            DataManager *dm = DataManager::get();
            dm->erasePlane(orientStr);
            string key = dm->loadFile(newpath, datainfo->st, datainfo->st2,orientStr);
            ReviewQueue::get()->disImagePlane(dataInfo, key, nslices, tslicelist[islice], first, last);

            datainfo->st = NULL;
        }
    } // end loop on slices
    delete[] buf;
    if(mip && mipbuf)
    {
        dataStruct_t *dsmip = new dataStruct_t;
        if(!dsmip) {
            delete[] mipbuf;
            delete[] tslicelist;
            return;
        }
        memcpy(dsmip, dataInfo->dataStruct, sizeof(*dsmip));
        dsmip->data = NULL;
        dsmip->auxparms = NULL;

        DDLSymbolTable *st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        DDLSymbolTable *st2 = new DDLSymbolTable();
        datainfo = spDataInfo_t(new DataInfo(dsmip, st, st2));
        if(dataInfo == (spDataInfo_t)NULL)
        {
            delete dsmip;
            delete st2;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        datainfo->initializeSymTab(2, 32, floatStr, nx0, ny0, 1, 1);
        datainfo->st->SetData((float *)mipbuf,sizeof(float)*nx0*ny0);

        // new rotation matrix for MIP
        /*
        datainfo->st->SetValue("orientation", newrot[0], 0);
        datainfo->st->SetValue("orientation", newrot[1], 1);
        datainfo->st->SetValue("orientation", newrot[2], 2);
        datainfo->st->SetValue("orientation", newrot[3], 3);
        datainfo->st->SetValue("orientation", newrot[4], 4);
        datainfo->st->SetValue("orientation", newrot[5], 5);
        datainfo->st->SetValue("orientation", newrot[6], 6);
        datainfo->st->SetValue("orientation", newrot[7], 7);
        datainfo->st->SetValue("orientation", newrot[8], 8);
        */

        //extract_plane_header(datainfo->st, orientation, nslices, (tslicelist+islice));
        extract_plane_header_obl(datainfo->st, orientation, nslices, tslicelist, newrot);

        // Create a name for this new slice
        char orientStr[16];
        double maxPlanes = 1;
        if(orientation == FRONT_PLANE)
        {
            strcpy(orientStr, "xy");
            maxPlanes = dataInfo->getSlow();
        }
        else if(orientation == TOP_PLANE)
        {
            strcpy(orientStr, "xz");
            maxPlanes = dataInfo->getMedium();
        }
        else if(orientation == SIDE_PLANE)
        {
            strcpy(orientStr, "yz");
            maxPlanes = dataInfo->getFast();
        }
        int digits = maxPlanes > 1 ? 1 + (int)log10(maxPlanes) : 1;
        char newpath[256];
        /*
        sprintf(newpath, "%s_%s_%0*d_%d_%d_%d_%d_%d_%d_EXTRA",
        volImagePath, orientStr, digits, *(tslicelist+islice),
        (int)*eulers,(int)*(eulers+1),(int)*(eulers+2),
        (int)*(rotangles),(int)*(rotangles+1),(int)*(rotangles+2) );
        datainfo->st->SetValue("filename", newpath);
        */
        sprintf(newpath, "%s_%s_%0*d_%d_%d_%d_%d_%d_%d_EXTRA",
        volImagePath, orientStr, digits, *(slicelist),
        (int)*eulers,(int)*(eulers+1),(int)*(eulers+2),
        (int)*(rotangles),(int)*(rotangles+1),(int)*(rotangles+2) );
        datainfo->st->SetValue("filename", newpath);
        DataManager *dm = DataManager::get();
        dm->erasePlane(orientStr);
        string key = dm->loadFile(newpath, datainfo->st, datainfo->st2,orientStr);
        ReviewQueue::get()->disImagePlane(dataInfo, key, 1, 0, 0, 0);

        datainfo->st = NULL;
        delete[] mipbuf;
        delete[] tslicelist;
    }
}

void VolData::extract_plane(int orientation, int nslices, int *slicelist,
        int first, int last) {
    int i;
    int j;
    int k;
    int k0;
    int k1;
    int nx;
    int ny;
    float *data;
    int slice;
    int nfast;
    int nmedium;
    int nslow;
    spDataInfo_t datainfo; // New 2D datainfo, dataInfo is orig 3D version
    char floatStr[16];



    if (dataInfo == (spDataInfo_t)NULL) {
        return;
    }
    strcpy(floatStr, "float");

    nfast = dataInfo->getFast();
    nmedium = dataInfo->getMedium();
    nslow = dataInfo->getSlow();

    /*
     * NB: We are about to extract a single 2D data set from the 3D data.
     * We need to make a copy of the "dataStruct" of the 3D data.
     * We don't need to copy the data, because the data pointer will
     * get reset to the new, extracted data.  But, just to make sure,
     * we initialize the "data" pointer to NULL, so that deleting the
     * copy won't delete the original data.  We don't deal with
     * "auxparms" yet.
     */
    dataStruct_t *ds2d = new dataStruct_t;
    if(!ds2d) return;
    memcpy(ds2d, dataInfo->dataStruct, sizeof(*ds2d));

    ds2d->data = NULL;
    ds2d->auxparms = NULL;

    slice = *slicelist; // Prepare to get first slice
    if (orientation == FRONT_PLANE)
    {
        float *buf = new float[nfast * nmedium];
        if(buf == NULL)
        {
            delete ds2d;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        DDLSymbolTable *st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        DDLSymbolTable *st2 = new DDLSymbolTable();
        datainfo = spDataInfo_t(new DataInfo(ds2d, st, st2));
        if(datainfo == (spDataInfo_t)NULL)
        {
            delete ds2d;
            delete st2;
	    delete[] buf;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        //datainfo->st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        datainfo->initializeSymTab(2, 32, floatStr, nfast, nmedium, 1, 1);
        // Extract the first slice

        data = (float *)dataInfo->getData() + slice * nfast * nmedium;
        for (i=0; i<nfast * nmedium; i++)
        {
            buf[i] = data[i];
        }
        // Do the MIP of the data
        for (k=1; k<nslices; k++)
        {
            slice = slicelist[k];
            data = (float *)dataInfo->getData() + slice * nfast * nmedium;
            for (i=0; i<nfast * nmedium; i++)
            {
                if (buf[i] < data[i])
                {
                    buf[i] = data[i];
                }
            }
        }
        datainfo->st->SetData((float *)buf,
        sizeof(float) * nfast * nmedium);
        nx = nfast;
        ny = nmedium;
        delete[] buf;
    }
    else if (orientation == TOP_PLANE)
    {
        float *buf = new float[nfast * nslow];
        if(buf == NULL)
        {
            delete ds2d;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        DDLSymbolTable *st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        DDLSymbolTable *st2 = new DDLSymbolTable();
        datainfo = spDataInfo_t(new DataInfo(ds2d, st, st2));
        if(datainfo == (spDataInfo_t)NULL)
        {
            delete ds2d;
            delete st2;
	    delete[] buf;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        //datainfo->st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        datainfo->initializeSymTab(2, 32, floatStr, nfast, nslow, 1, 1);
        // Extract the first slice
        data = (float *)dataInfo->getData() + slice * nfast;
        for (j=0; j<nslow; j++)
        {
            k0 = j * nfast * nmedium;
            k1 = j * nfast;
            for (i=0; i<nfast; i++)
            {
                buf[i + k1] = data[i + k0];
            }
        }
        // Do the MIP of the data
        for (k=1; k<nslices; k++)
        {
            slice = slicelist[k];
            data = (float *)dataInfo->getData() + slice * nfast;
            for (j=0; j<nslow; j++)
            {
                k0 = j * nfast * nmedium;
                k1 = j * nfast;
                for (i=0; i<nfast; i++)
                {
                    if (buf[i + k1] < data[i + k0])
                    {
                        buf[i + k1] = data[i + k0];
                    }
                }
            }
        }
        datainfo->st->SetData((float *)buf,
        sizeof(float) * nfast * nslow);
        nx = nfast;
        ny = nslow;
        delete[] buf;
    }
    else if (orientation == SIDE_PLANE)
    {
        float *buf = new float[nmedium * nslow];
        if(buf == NULL)
        {
            delete ds2d;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        DDLSymbolTable *st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        DDLSymbolTable *st2 = new DDLSymbolTable();
        datainfo = spDataInfo_t(new DataInfo(ds2d, st,st2));
        if(datainfo == (spDataInfo_t)NULL)
        {
            delete ds2d;
            delete st2;
	    delete[] buf;
            STDERR("VolData: allocate memory returned NULL pointer.");
            return;
        }
        //datainfo->st = (DDLSymbolTable *)dataInfo->st->CloneList(false);
        datainfo->initializeSymTab(2, 32, floatStr, nmedium, nslow, 1, 1);

        // Extract the first slice
        data = (float *)dataInfo->getData() + slice;
        for (j=0; j<nslow; j++)
        {
            k0 = j * nfast * nmedium;
            k1 = j * nmedium;
            for (i=0; i<nmedium; i++)
            {
                buf[i + k1] = data[i*nfast + k0];
            }
        }


        // Do the MIP of the data
        for (k=1; k<nslices; k++)
        {
            slice = slicelist[k];
            data = (float *)dataInfo->getData() + slice;
            for (j=0; j<nslow; j++)
            {
                k0 = j * nfast * nmedium;
                k1 = j * nmedium;
                for (i=0; i<nmedium; i++)
                {
                    if (buf[i + k1] < data[i*nfast + k0])
                    {
                        buf[i + k1] = data[i*nfast + k0];
                    }
                }
            }
        }
        datainfo->st->SetData((float *)buf,
        sizeof(float) * nmedium * nslow);
        nx = nmedium;
        ny = nslow;
        delete[] buf;
    }
    else
    {
        fprintf(stderr, "VolData::extract_plane(): Internal error %d\n",
        orientation);
        return;
    }

    extract_plane_header(datainfo->st, orientation, nslices, slicelist);

    // Create a name for this new slice
    // Use the path of the 3D followed by:
    // "_", orientation, "_", sliceNumber, "_", number of slices
    char orientStr[16];
    double maxPlanes = 1;
    if(orientation == FRONT_PLANE)
    {
        strcpy(orientStr, "xy");
        maxPlanes = dataInfo->getSlow();
    }
    else if(orientation == TOP_PLANE)
    {
        strcpy(orientStr, "xz");
        maxPlanes = dataInfo->getMedium();
    }
    else if(orientation == SIDE_PLANE)
    {
        strcpy(orientStr, "yz");
        maxPlanes = dataInfo->getFast();
    }
    int digits = maxPlanes > 1 ? 1 + (int)log10(maxPlanes) : 1;
    char newpath[256];
    sprintf(newpath, "%s_%s_MIP_%0*d_%0*d",
    volImagePath, orientStr, digits, *slicelist, digits, nslices);

    datainfo->st->SetValue("filename", newpath);
    //string key = DataInfo::getKey(datainfo->st);
    DataManager *dm = DataManager::get();
    string key = dm->loadFile(newpath, datainfo->st, datainfo->st2,orientStr);
    if(nslices>1) // Mip
      ReviewQueue::get()->addImagePlane(dataInfo, key, 1, slicelist[0], 1, 1);
    else
      ReviewQueue::get()->addImagePlane(dataInfo, key, nslices, slicelist[0], first, last);

    // datainfo->st now belongs to loadFile(); unreference it here, so
    // that it won't get deleted with the rest of the datainfo
    // structure when datainfo goes out of scope.
    datainfo->st = NULL;

}

/* this version of extract_plane_header gets orientation as an argument */
void VolData::extract_plane_header_obl(DDLSymbolTable *ist, int slice_orient,
        int nslices, int *slicelist, double *orientation) {
    int i;

    DDLSymbolTable *vst = dataInfo->st;
    double slice_offset;
    int minslice = slicelist[0];
    int maxslice = slicelist[0];
    for (i=1; i<nslices; i++) {
        if (minslice > slicelist[i]) {
            minslice = slicelist[i];
        } else if (maxslice < slicelist[i]) {
            maxslice = slicelist[i];
        }
    }
    float fslice = (maxslice + minslice) / 2.0;
    nslices = maxslice - minslice + 1; // NB: redefine nslices!

    // Get all relevant values from 3D data set
    int matrix[3];
    vst->GetValue("matrix", matrix[0], 0);
    vst->GetValue("matrix", matrix[1], 1);
    vst->GetValue("matrix", matrix[2], 2);
    double span[3];
    vst->GetValue("span", span[0], 0);
    vst->GetValue("span", span[1], 1);
    vst->GetValue("span", span[2], 2);
    double location[3];
    vst->GetValue("location", location[0], 0);
    vst->GetValue("location", location[1], 1);
    vst->GetValue("location", location[2], 2);
    double origin[3];
    vst->GetValue("origin", origin[0], 0);
    vst->GetValue("origin", origin[1], 1);
    vst->GetValue("origin", origin[2], 2);
    double roi[3];
    vst->GetValue("roi", roi[0], 0);
    vst->GetValue("roi", roi[1], 1);
    vst->GetValue("roi", roi[2], 2);

// rotate location to new orientation:
// rotate old position to magnet frame using old orientation matrix, 
// then rotate it to new position using new orientation matrix.
// Note, x,y are swapped and inverted. 
    double vOrient[9];
    vst->GetValue("orientation", vOrient[0], 0);
    vst->GetValue("orientation", vOrient[1], 1);
    vst->GetValue("orientation", vOrient[2], 2);
    vst->GetValue("orientation", vOrient[3], 3);
    vst->GetValue("orientation", vOrient[4], 4);
    vst->GetValue("orientation", vOrient[5], 5);
    vst->GetValue("orientation", vOrient[6], 6);
    vst->GetValue("orientation", vOrient[7], 7);
    vst->GetValue("orientation", vOrient[8], 8);

    float loc[3];
    loc[0]=location[0];
    loc[1]=location[1];
    loc[2] = location[2];

    float orient[9];
    orient[0]=vOrient[1];
    orient[1]=-vOrient[0];
    orient[2]=-vOrient[2];
    orient[3]=-vOrient[4];
    orient[4]=vOrient[3];
    orient[5]=vOrient[5];
    orient[6]=-vOrient[7];
    orient[7]=vOrient[6];
    orient[8]=vOrient[8];

    rotateu2m(loc,orient);

    orient[0]=orientation[1];
    orient[1]=-orientation[0];
    orient[2]=-orientation[2];
    orient[3]=-orientation[4];
    orient[4]=orientation[3];
    orient[5]=orientation[5];
    orient[6]=-orientation[7];
    orient[7]=orientation[6];
    orient[8]=orientation[8];

    rotatem2u(loc,orient);

    location[0]=loc[0];
    location[1]=loc[1];
    location[2] = loc[2];

    char fov[16];
    strcpy(fov, "2dfov");
    ist->SetValue("subrank", fov);

	 char *p1 = (char *)"head_first";
	 char *p2 = (char *)"supine";
	vst->GetValue("position1", p1);
	vst->GetValue("position2", p2);
	ist->SetValue("position1", p1);
	ist->SetValue("position2", p2);


    if (slice_orient == FRONT_PLANE) {
        ist->SetValue("span", span[0], 0);
        ist->SetValue("span", span[1], 1);
        ist->SetValue("location", location[0], 0);
        ist->SetValue("location", location[1], 1);
        slice_offset = ((fslice - (matrix[2] - 1.0) / 2.0)
                * (roi[2] / matrix[2]));
        ist->SetValue("location", location[2] + slice_offset, 2);
        ist->SetValue("origin", origin[0], 0);
        ist->SetValue("origin", origin[1], 1);
        ist->SetValue("roi", roi[0], 0);
        ist->SetValue("roi", roi[1], 1);
        ist->SetValue("roi", (nslices * roi[2]) / matrix[2], 2);

        ist->SetValue("orientation", orientation[1], 0);
         ist->SetValue("orientation", -orientation[0], 1);
         ist->SetValue("orientation", -orientation[2], 2);
         ist->SetValue("orientation", -orientation[4], 3);
         ist->SetValue("orientation", orientation[3], 4);
         ist->SetValue("orientation", orientation[5], 5);

         ist->SetValue("orientation", -orientation[7], 6);
         ist->SetValue("orientation", orientation[6], 7);
         ist->SetValue("orientation", orientation[8], 8);


    } else if (slice_orient == TOP_PLANE) {
        ist->SetValue("span", span[0], 0);
        ist->SetValue("span", span[2], 1);
        ist->SetValue("location", location[0], 0);
        ist->SetValue("location", location[2], 1);
        slice_offset = ((fslice - (matrix[1] - 1.0) / 2.0)
                * (roi[1] / matrix[1]));
        ist->SetValue("location", location[1] + slice_offset, 2);
        ist->SetValue("origin", origin[0], 0);
        ist->SetValue("origin", origin[2], 1);
        ist->SetValue("roi", roi[0], 0);
        ist->SetValue("roi", roi[2], 1);
        ist->SetValue("roi", (nslices * roi[1]/ matrix[1]), 2);

        ist->SetValue("orientation", orientation[1], 0);
            ist->SetValue("orientation", -orientation[0], 1);
            ist->SetValue("orientation", -orientation[2], 2);
            ist->SetValue("orientation", -orientation[7], 3);
            ist->SetValue("orientation", orientation[6], 4);
            ist->SetValue("orientation", orientation[8], 5);

            ist->SetValue("orientation", -orientation[4], 6);
            ist->SetValue("orientation", orientation[3], 7);
            ist->SetValue("orientation", orientation[5], 8);


    } else if (slice_orient == SIDE_PLANE) {
        ist->SetValue("span", span[1], 0);
        ist->SetValue("span", span[2], 1);
        ist->SetValue("location", location[1], 0);
        ist->SetValue("location", location[2], 1);
        slice_offset = ((fslice - (matrix[0] - 1.0) / 2.0)
                * (roi[0] / matrix[0]));
        ist->SetValue("location", location[0] + slice_offset, 2);
        ist->SetValue("origin", origin[1], 0);
        ist->SetValue("origin", origin[2], 1);
        ist->SetValue("roi", roi[1], 0);
        ist->SetValue("roi", roi[2], 1);
        ist->SetValue("roi", (nslices * roi[0]/ matrix[0]), 2);


          ist->SetValue("orientation", -orientation[4], 0);
          ist->SetValue("orientation", orientation[3], 1);
          ist->SetValue("orientation", orientation[5], 2);
          ist->SetValue("orientation", -orientation[7], 3);
          ist->SetValue("orientation", orientation[6], 4);
          ist->SetValue("orientation", orientation[8], 5);
          ist->SetValue("orientation", orientation[1], 6);
          ist->SetValue("orientation", -orientation[0], 7);
          ist->SetValue("orientation", -orientation[2], 8);


    } else {
        fprintf(stderr, "VolData::extract_plane_header(): Internal error %d\n",
        slice_orient);
    }
}

void VolData::extract_plane_header(DDLSymbolTable *ist, int slice_orient,
        int nslices, int *slicelist) {
    int i;

    DDLSymbolTable *vst = dataInfo->st;
    double slice_offset;
    int minslice = slicelist[0];
    int maxslice = slicelist[0];
    for (i=1; i<nslices; i++) {
        if (minslice > slicelist[i]) {
            minslice = slicelist[i];
        } else if (maxslice < slicelist[i]) {
            maxslice = slicelist[i];
        }
    }
    float fslice = (maxslice + minslice) / 2.0;
    nslices = maxslice - minslice + 1; // NB: redefine nslices!

    // Get all relevant values from 3D data set
    int matrix[3];
    vst->GetValue("matrix", matrix[0], 0);
    vst->GetValue("matrix", matrix[1], 1);
    vst->GetValue("matrix", matrix[2], 2);
    double span[3];
    vst->GetValue("span", span[0], 0);
    vst->GetValue("span", span[1], 1);
    vst->GetValue("span", span[2], 2);
    double location[3];
    vst->GetValue("location", location[0], 0);
    vst->GetValue("location", location[1], 1);
    vst->GetValue("location", location[2], 2);
    double origin[3];
    vst->GetValue("origin", origin[0], 0);
    vst->GetValue("origin", origin[1], 1);
    vst->GetValue("origin", origin[2], 2);
    double roi[3];
    vst->GetValue("roi", roi[0], 0);
    vst->GetValue("roi", roi[1], 1);
    vst->GetValue("roi", roi[2], 2);
    double orientation[9];
    vst->GetValue("orientation", orientation[0], 0);
    vst->GetValue("orientation", orientation[1], 1);
    vst->GetValue("orientation", orientation[2], 2);
    vst->GetValue("orientation", orientation[3], 3);
    vst->GetValue("orientation", orientation[4], 4);
    vst->GetValue("orientation", orientation[5], 5);
    vst->GetValue("orientation", orientation[6], 6);
    vst->GetValue("orientation", orientation[7], 7);
    vst->GetValue("orientation", orientation[8], 8);

    char fov[16];
    strcpy(fov, "2dfov");
    ist->SetValue("subrank", fov);




	 char *p1 = (char *)"head_first";
	 char *p2 = (char *)"supine";
    vst->GetValue("position1", p1);
    vst->GetValue("position2", p2);
    ist->SetValue("position1", p1);
    ist->SetValue("position2", p2);



    if (slice_orient == FRONT_PLANE) {
        ist->SetValue("span", span[0], 0);
        ist->SetValue("span", span[1], 1);
        ist->SetValue("location", location[0], 0);
        ist->SetValue("location", location[1], 1);
        slice_offset = ((fslice - (matrix[2] - 1.0) / 2.0)
                * (roi[2] / matrix[2]));
        ist->SetValue("location", location[2] + slice_offset, 2);
        ist->SetValue("origin", origin[0], 0);
        ist->SetValue("origin", origin[1], 1);
        ist->SetValue("roi", roi[0], 0);
        ist->SetValue("roi", roi[1], 1);
        ist->SetValue("roi", (nslices * roi[2]) / matrix[2], 2);
        /* no correction
         ist->SetValue("orientation", orientation[0], 0);
         ist->SetValue("orientation", orientation[1], 1);
         ist->SetValue("orientation", orientation[2], 2);
         ist->SetValue("orientation", orientation[3], 3);
         ist->SetValue("orientation", orientation[4], 4);
         ist->SetValue("orientation", orientation[5], 5);
         ist->SetValue("orientation", orientation[6], 6);
         ist->SetValue("orientation", orientation[7], 7);
         ist->SetValue("orientation", orientation[8], 8);
         */
        /* The orientation matrix calculated by macro ft3D is different
         from that in DataInfo by a rotation of R, i.e., M(IB)=R*M(3D)*R'
         where R={0,-1,0,1,0,0,0,0,1}, R'={0,1,0,-1,0,0,0,0,1}.
         This rotation should be applied to the planes.

         The 3D data also seems to have x, y swabbed and the axies
         inverted. The correction will be a {0,-1,0,-1,0,0,0,0,-1}
         rotation.

         The total rotation will be

         M(IB) = {0,-1,0,-1,0,0,0,0,-1}*R*M(3D)*R'
         */
        ist->SetValue("orientation", orientation[1], 0);
        ist->SetValue("orientation", -orientation[0], 1);
        ist->SetValue("orientation", -orientation[2], 2);
        ist->SetValue("orientation", -orientation[4], 3);
        ist->SetValue("orientation", orientation[3], 4);
        ist->SetValue("orientation", orientation[5], 5);
        /* need to multiply -1 ???
         ist->SetValue("orientation", orientation[7], 6);
         ist->SetValue("orientation", -orientation[6], 7);
         ist->SetValue("orientation", -orientation[8], 8);
         */
        ist->SetValue("orientation", -orientation[7], 6);
        ist->SetValue("orientation", orientation[6], 7);
        ist->SetValue("orientation", orientation[8], 8);

    } else if (slice_orient == TOP_PLANE) {
        ist->SetValue("span", span[0], 0);
        ist->SetValue("span", span[2], 1);
        ist->SetValue("location", location[0], 0);
        ist->SetValue("location", location[2], 1);
        slice_offset = ((fslice - (matrix[1] - 1.0) / 2.0)
                * (roi[1] / matrix[1]));
        ist->SetValue("location", location[1] + slice_offset, 2);
        ist->SetValue("origin", origin[0], 0);
        ist->SetValue("origin", origin[2], 1);
        ist->SetValue("roi", roi[0], 0);
        ist->SetValue("roi", roi[2], 1);
        ist->SetValue("roi", (nslices * roi[1]/ matrix[1]), 2);
        /* swab y and z (y->z, z->y)
         ist->SetValue("orientation", orientation[0], 0);
         ist->SetValue("orientation", orientation[1], 1);
         ist->SetValue("orientation", orientation[2], 2);
         ist->SetValue("orientation", orientation[6], 3);
         ist->SetValue("orientation", orientation[7], 4);
         ist->SetValue("orientation", orientation[8], 5);
         ist->SetValue("orientation", orientation[3], 6);
         ist->SetValue("orientation", orientation[4], 7);
         ist->SetValue("orientation", orientation[5], 8);
         */
        /* total correction will be
         M(IB) = {0,-1,0,-1,0,0,0,0,-1}*R*{1,0,0,0,0,1,0,1,0}*M(3D)*R'
         */
        ist->SetValue("orientation", orientation[1], 0);
        ist->SetValue("orientation", -orientation[0], 1);
        ist->SetValue("orientation", -orientation[2], 2);
        ist->SetValue("orientation", -orientation[7], 3);
        ist->SetValue("orientation", orientation[6], 4);
        ist->SetValue("orientation", orientation[8], 5);
        /* need to multiply -1 ???
         ist->SetValue("orientation", orientation[4], 6);
         ist->SetValue("orientation", -orientation[3], 7);
         ist->SetValue("orientation", -orientation[5], 8);
         */
        ist->SetValue("orientation", -orientation[4], 6);
        ist->SetValue("orientation", orientation[3], 7);
        ist->SetValue("orientation", orientation[5], 8);

    } else if (slice_orient == SIDE_PLANE) {
        ist->SetValue("span", span[1], 0);
        ist->SetValue("span", span[2], 1);
        ist->SetValue("location", location[1], 0);
        ist->SetValue("location", location[2], 1);
        slice_offset = ((fslice - (matrix[0] - 1.0) / 2.0)
                * (roi[0] / matrix[0]));
        ist->SetValue("location", location[0] + slice_offset, 2);
        ist->SetValue("origin", origin[1], 0);
        ist->SetValue("origin", origin[2], 1);
        ist->SetValue("roi", roi[1], 0);
        ist->SetValue("roi", roi[2], 1);
        ist->SetValue("roi", (nslices * roi[0]/ matrix[0]), 2);
        /* x->y, y->z, z->x
         ist->SetValue("orientation", orientation[3], 0);
         ist->SetValue("orientation", orientation[4], 1);
         ist->SetValue("orientation", orientation[5], 2);
         ist->SetValue("orientation", orientation[6], 3);
         ist->SetValue("orientation", orientation[7], 4);
         ist->SetValue("orientation", orientation[8], 5);
         ist->SetValue("orientation", orientation[0], 6);
         ist->SetValue("orientation", orientation[1], 7);
         ist->SetValue("orientation", orientation[2], 8);
         */
        /* It seems x needs to be inverted in addition to the swab
         and rotations.

         Total correction will be
         M(IB) = {0,-1,0,-1,0,0,0,0,-1}*R*{0,-1,0,0,0,1,1,0,0}*M(3D)*R'
         */
        ist->SetValue("orientation", -orientation[4], 0);
        ist->SetValue("orientation", orientation[3], 1);
        ist->SetValue("orientation", orientation[5], 2);
        ist->SetValue("orientation", -orientation[7], 3);
        ist->SetValue("orientation", orientation[6], 4);
        ist->SetValue("orientation", orientation[8], 5);
        ist->SetValue("orientation", orientation[1], 6);
        ist->SetValue("orientation", -orientation[0], 7);
        ist->SetValue("orientation", -orientation[2], 8);

    } else {
        fprintf(stderr, "VolData::extract_plane_header(): Internal error %d\n",
        slice_orient);
    }
}

/************************************************************************
 *
 *  Make sure a slice number is valid
 */
int VolData::clip_slice(int slice_orient, int slice) {
    int max = 0;
    if (slice_orient == FRONT_PLANE) {
        max = dataInfo->getSlow() - 1;
    } else if (slice_orient == TOP_PLANE) {
        max = dataInfo->getMedium() - 1;
    } else if (slice_orient == SIDE_PLANE) {
        max = dataInfo->getFast() - 1;
    }
    slice = slice < 0 ? 0 : slice > max ? max : slice;
    return slice;
}

/************************************************************************
 *                                  *
 *  Get a bunch of integer values from an argv list
 *  Returns number of values decoded.
 *                                  */
int VolData::getIntArgs(int argc, // Argument count
        char **argv, // Argument array
        int *values, // Decoded values
        int nvals // Number of values to get
) {
    int err;
    int i;
    int rtn = 0;
    for (i=0; i<nvals && i<argc; i++) {
        err = sscanf(*argv, "%i", &values[i]);
        if (err != 1) {
            break;
        }
        rtn++;
        argv++;
    }
    return rtn;
}

void VolData::setOverlayFlg(bool b) {
   overlayFlg=b;
}

bool VolData::getOverlayFlg() {
   return overlayFlg;
}
