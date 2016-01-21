/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <iostream>

#include "CGLRenderer.h" // C++ class header file
#include "GLUtil.h"
#include "CGLProgram.h"

//#define DEBUG_NATIVE
//#define SHOW_GLINFO
//#define DEBUG_MEM

#define DRAW_ROT_AXIS

static int maxPntsPerPixel = 5;

//-------------------------------------------------------------
// CGLRenderer::CGLRenderer()	constructor
//-------------------------------------------------------------
CGLRenderer::CGLRenderer() {
    aspect = 1.0;
    show = 0;
    phasing = usebase = false;
    intensity = 1.0;
    contrast = 0.0;
    threshold = 0;
    limit = 1;
    projection = OBLIQUE;
    sliceplane = Z;
    nclipped = ntotal = 0;
    mode = 0;
    csplineInvalid = true;
    num2Dtexs = 0;
    size2Dtexs = 0;

    newLayout = true;
    newDataGeometry = true;
    newTexMap = true;
    newTexMode = true;
    newShader = true;
    useLighting = false;
    data_size = 0;
    last_trc = 0;

    palette = GRAYCOLS;

    Point3D minCoord(0.0, -0.5, 0.0);
    Point3D maxCoord(1.0, 0.5, 1.0);
    volume = new Volume(minCoord, maxCoord);
    view = new CGLView(volume);

    tex1D = new CGLTex1DMgr(view, this);
    tex2D = new CGLTex2DMgr(view, this);
    tex3D = new CGLTex3DMgr(view, this);
    texmgr = NULL;
    colors = NULL;
    transparency = 0.0;
    alphascale = 1.0;
    shader = NONE;
    mapfile[0] = 0;
    select_mode=false;

#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::CGLRenderer()" << std::endl;
#endif
}

//-------------------------------------------------------------
// CGLRenderer::~CGLRenderer()	destructor
//-------------------------------------------------------------
CGLRenderer::~CGLRenderer() {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::~CGLRenderer()" << std::endl;
#endif
    DELETE(tex1D);
    DELETE(tex2D);
    DELETE(tex3D);
    DELETE(volume);
    DELETE(view);
}

//-------------------------------------------------------------
// CGLRenderer::getDataInfo() return data info
//-------------------------------------------------------------
float *CGLRenderer::getDataInfo(int *size) {
    *size = data_size;
    return vertexData;
}
//-------------------------------------------------------------
// CGLRenderer::init()	initialize openGL environment
//-------------------------------------------------------------
void CGLRenderer::init(int s) {
    float pos[] = { 5.0f, 5.0f, 10.0f, 0.0f };
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::init()" << std::endl;
#endif
    shader = s;
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_LIGHTING);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);            // Clock wise wound is front
    view->init();
#ifdef SHOW_GLINFO
    GLSLProgram program;
    program.showGLInfo();
#endif
}

//-------------------------------------------------------------
// CGLRenderer::setColorArray(float *data, int n)  set color array
//-------------------------------------------------------------
void CGLRenderer::setColorArray(int id, float *data, int n) {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::setColorArray("<<id<<","<<n<<")" << std::endl;
#endif
    int i, j, k;
    switch (id) {
    case STDCOLS:
        for (i = 0; i < n * 4; i++) {
            stdcols[i] = data[i];
        }
        break;
    case ABSCOLS:
        ncolors = NUMABSCOLS;
        colors = abscols;
        for (i = 0; i < ncolors * 4; i++) {
            colors[i] = data[i];
        }
        break;
    case PHSCOLS:
        ncolors = NUMPHSCOLS;
        colors = phscols;
        for (i = 0; i < ncolors * 4; i++) {
            colors[i] = data[i];
        }
        break;

    case CTRCOLS:
        ncolors = NUMCTRCOLS;
        colors = ctrcols;
        for (i = 0; i < ncolors * 4; i++)
            colors[i] = data[i];
        break;

    case GRAYCOLS: {
        ncolors = NUMGRAYS;
        colors = grays;
        float *A = data;
        float *B = data + 4;
        float delta = 1.0f / (ncolors - 1);
        float ampl = 0.0f;
        for (k = 0, i = 0; i < NUMGRAYS; i++) {
            for (j = 0; j < 3; j++) {
                colors[k++] = ampl * B[j] + (1 - ampl) * A[j];
            }
            colors[k++] = 1.0f;
            ampl += delta;
        }
    }
        break;
    }
    if (id != STDCOLS) {
        palette = id;
        newTexMap = true;
        csplineInvalid = true;
    }
}

//-------------------------------------------------------------
// CGLRenderer::setScale(x,y,z)  set view scale factors
//-------------------------------------------------------------
void CGLRenderer::setScale(double a, double x, double y, double z) {
    view->setScale(a, x, y, z);
}
//-------------------------------------------------------------
// CGLRenderer::setOffset(x,y,z)  set view offset
//-------------------------------------------------------------
void CGLRenderer::setOffset(double x, double y, double z) {
    view->setOffset(x, y, z);
}

//-------------------------------------------------------------
// CGLRenderer::setSpan(x,y,z)  set view span factors
//-------------------------------------------------------------
void CGLRenderer::setSpan(double x, double y, double z) {
    view->setSpan(x, y, z);
}

//-------------------------------------------------------------
// CGLRenderer::setSliceVector(x,y,z)  set slice vector
//-------------------------------------------------------------
void CGLRenderer::setSliceVector(double x, double y, double z, double w) {
    svect = Point3D(x, y, z, w);
}

//-------------------------------------------------------------
// CGLRenderer::setRotation2D(x,y,z)  set view rotation
//-------------------------------------------------------------
void CGLRenderer::setRotation2D(double x, double y) {
    view->setRotation2D(x, y);
}

//-------------------------------------------------------------
// CGLRenderer::setRotation3D(x,y,z)  set view rotation
//-------------------------------------------------------------
void CGLRenderer::setRotation3D(double x, double y, double z) {
    view->setRotation3D(x, y, z);
}
//-------------------------------------------------------------
// CGLRenderer::setObjectRotation(x,y,z)  set object rotation
//-------------------------------------------------------------
void CGLRenderer::setObjectRotation(double x, double y, double z) {
    Point3D vrot(x, y, z);
    view->vrot = vrot;
}
//-------------------------------------------------------------
// CGLRenderer::setSlant(x)  set oblique projection slant
//-------------------------------------------------------------
void CGLRenderer::setSlant(double x, double y) {
    view->setSlant(x, y);
}

//-------------------------------------------------------------
// CGLRenderer::setThreshold(x)  set minimum voxel intensity
//-------------------------------------------------------------
void CGLRenderer::setThreshold(double r) {
    if (r != threshold)
        newTexMap = true;
    threshold = r;
}

//-------------------------------------------------------------
// CGLRenderer::setContours(x)  set contour interval
//-------------------------------------------------------------
void CGLRenderer::setContours(double r) {
    contours = r;
}

//-------------------------------------------------------------
// CGLRenderer::setThreshold(x)  set minimum voxel intensity
//-------------------------------------------------------------
void CGLRenderer::setLimit(double r) {
    if (r != limit)
        newTexMap = true;
    limit = r;
}

//-------------------------------------------------------------
// CGLRenderer::setIntensity(x)  set color intensity
//-------------------------------------------------------------
void CGLRenderer::setIntensity(double r) {
    if (r != intensity)
        newTexMap = true;
    intensity = r;
    yrange = intensity / (ymax - ymin);
}

//-------------------------------------------------------------
// CGLRenderer::setBias(x)  set color bias
//-------------------------------------------------------------
void CGLRenderer::setBias(double r) {
    bias = r;
}

//-------------------------------------------------------------
// CGLRenderer::setContrast(x)  set color contrast
//-------------------------------------------------------------
void CGLRenderer::setContrast(double r) {
    if (r != contrast)
        newTexMap = true;
    contrast = r;
}

//-------------------------------------------------------------
// CGLRenderer::setContrast(x)  Set minimum intensity for alpha>0
//-------------------------------------------------------------
void CGLRenderer::setTransparency(double r) {
    if (r != transparency)
        newTexMap = true;
    transparency = r;
}

//-------------------------------------------------------------
// CGLRenderer::setAlphaScale(x) Set alpha super-sampling scaling factor
//-------------------------------------------------------------
void CGLRenderer::setAlphaScale(double r) {
    if (r != alphascale)
        newTexMap = true;
    alphascale = r;
}

//-------------------------------------------------------------
// CGLRenderer::setTrace(int i)  set trace index
//-------------------------------------------------------------
void CGLRenderer::setTrace(int t, int m, int n) {
    trace = t;
    maxtrace = m;
    numtraces = n;
}

//-------------------------------------------------------------
// CGLRenderer::setSlice(int i)  set slice index
//-------------------------------------------------------------
void CGLRenderer::setSlice(int s, int m, int n) {
    slice = s;
    maxslice = m;
    numslices = n;
}

//-------------------------------------------------------------
// CGLRenderer::setStep(int i)  set step size
//-------------------------------------------------------------
void CGLRenderer::setStep(int i) {
    step = i;
    if (step < 1)
        step = 1;
}

//-------------------------------------------------------------
// CGLRenderer::setScale(x,y,z)  set view scale factors
//-------------------------------------------------------------
void CGLRenderer::setDataScale(double mn, double mx) {
    if (mx != ymax)
        newDataGeometry = true;
    ymin = mn;
    ymax = mx;
    view->setDataScale(mn, mx);
    tex1D->setDataScale(ymin, ymax);
    tex2D->setDataScale(ymin, ymax);
    tex3D->setDataScale(ymin, ymax);
}

//-------------------------------------------------------------
// CGLRenderer::setDataPars(int np, ...)  set data parameters
//-------------------------------------------------------------
void CGLRenderer::setDataPars(int n, int t, int s, int dtype) {
    CGLDataMgr::setDataPars(n, t, s, dtype);
    view->setDataPars(n, t, s, dtype);
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::setDataPars("<< np << ","<< traces << ","<< slices << ","<< data_type <<")" << std::endl;
#endif
}

//-------------------------------------------------------------
// CGLRenderer::setData(float *data)  set data array pointer
//-------------------------------------------------------------
void CGLRenderer::setDataPtr(float *data) {
#ifdef DEBUG_NATIVE
    if(data)
    std::cout << "CGLRenderer::setDataPtr(" << (float*)data <<")" << std::endl;
    else
    std::cout << "CGLRenderer::setDataPtr(null)" << std::endl;
#endif
    vertexData = data;
}

//-------------------------------------------------------------
// CGLRenderer::setDataMap(char *path)  set mmap file path
//-------------------------------------------------------------
void CGLRenderer::setDataMap(char *path) {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::setDataMap(" << path <<")" << std::endl;
#endif
    strncpy(mapfile, path, 255);
}

//-------------------------------------------------------------
// CGLRenderer::resize(int w, int h)  resize window
//-------------------------------------------------------------
void CGLRenderer::resize(int w, int h) {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::resize(" << w << "," << h << ")" << std::endl;
#endif
    width = w;
    height = h;
    aspect = height / width;
    newDataGeometry = true;
    newLayout = true;
    view->setResized();
}

//-------------------------------------------------------------
// CGLRenderer::setPhase(int w, int h)  resize window
//-------------------------------------------------------------
void CGLRenderer::setPhase(double r, double l) {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::setPhase(" << r << "," << l << ")" << std::endl;
#endif
    rp = r;
    lp = l;
}

//-------------------------------------------------------------
// CGLRenderer::setOptions() set render pass options
//-------------------------------------------------------------
void CGLRenderer::setOptions(int indx, int newshow) {
    mode = 0;
    int last;
    int texflags = 0;

    if ((newshow & SHOWTEXOPTS) != (show & SHOWTEXOPTS)) {
        newTexMode = true;
        newTexMap = true;
        csplineInvalid = true;
    }
    nclipped = ntotal = 0;

    select_mode = selecting();

    last = projection;
    switch (newshow & SHOWPTYPE) {
    default:
    case SHOW1D:
        projection = ONETRACE;
        break;
    case SHOW1DSP:
        projection = OBLIQUE;
        break;
    case SHOW2D:
        projection = TWOD;
        break;
    case SHOW2DSP:
        projection = SLICES;
        break;
    case SHOW3D:
        projection = THREED;
        break;
    }
    if (last != projection)
        newDataGeometry = true;
    last = sliceplane;

    if (dim < 2)
        sliceplane = Z;
    else
        switch (newshow & SHOWAXIS) {
        case SHOWY:
            sliceplane = Y;
            break;
        case SHOWX:
            sliceplane = X;
            break;
        case SHOWZ:
            sliceplane = Z;
            break;
        }

    mode |= sliceplane;

    if (last != sliceplane && projection == SLICES)
        newDataGeometry = true;
    if (((newshow & ABS) == ABS) || !complex)
        absval = true;
    else
        absval = false;

    if ((newshow & SHOWBLEND) > 0)
        mode = setBit(mode, BLEND);
    if ((newshow & SHOWCLAMP) > 0)
        mode = setBit(mode, CLAMP);
    if ((newshow & SHOWXPARANCY) > 0)
        mode = setBit(mode, XPARANCY);
    if ((newshow & SHOWCONTOURS) > 0)
        mode = setBit(mode, CONTOURS);
    if ((newshow & SHOWGRID) > 0)
        mode = setBit(mode, GRID);

    useLighting = ((newshow & SHOWLIGHTING) != 0);
    if (show == 0 || ((show & SHOWLIGHTING) != (newshow & SHOWLIGHTING))) {
        if (projection == TWOD && !select_mode)
            newShader = true;
    }

    glDisable(GL_TEXTURE_1D);

    if ((mode & BLEND) > 0)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

    switch (newshow & SHOWCTYPE) {
    case SHOWONECOL:
        mode = setBits(mode, COLTYPE, COLONE);
        break;
    case SHOWIDCOL:
        mode = setBits(mode, COLTYPE, COLIDX);
        glEnable(GL_TEXTURE_1D);
        break;
    case SHOWHTCOL:
        glEnable(GL_TEXTURE_1D);
        mode = setBits(mode, COLTYPE, COLHT);
        if ((show & SHOWCTYPE) != (newshow & SHOWCTYPE))
            newShader = true;
        break;
    }
    if (show == 0 || (show & SHOWCTYPE) != (newshow & SHOWCTYPE)) {
        csplineInvalid = true;
        newTexMap = true;
        if (projection == TWOD)
            newShader = true;
    }
    if (projection == TWOD && !select_mode)
        tex1D->setUseShader(true);
    else
        tex1D->setUseShader(false);

    CGLTexMgr *lastmgr = texmgr;
    texmgr = tex1D;
    if (csplineInvalid) {
        makeCSpline(mode);
        texflags |= NEWCTABLE;
    }

    switch (projection) {
    case THREED:
    case SLICES:
        if ((newshow & SHOWLTYPE) == SHOWPOLYGONS) {
            if (projection == SLICES)
                texmgr = tex2D;
            else
                texmgr = tex3D;
        }

        if ((show & SHADERMODE) != (newshow & SHADERMODE))
            newShader = true;
        mode = clrBit(mode, CLIPLOW | CLIPHIGH);
        if ((newshow & SHADERMODE) == MIP) {
            mode = setBits(mode, SHADERTYPE, MIPSHADER);
            mode = clrBit(mode, XPARANCY);
            if ((newshow & SHOWXPARANCY) > 0) {
                if ((newshow & SHOWCLIPLOW) > 0)
                    mode = setBit(mode, CLIPLOW);
                if ((newshow & SHOWCLIPHIGH) > 0)
                    mode = setBit(mode, CLIPHIGH);
            }
        } else {
            if ((newshow & SHOWCLIPLOW) > 0)
                mode = setBit(mode, CLIPLOW);
            if ((newshow & SHOWCLIPHIGH) > 0)
                mode = setBit(mode, CLIPHIGH);
        }
        if ((show & CLIPMODE) != (newshow & CLIPMODE))
            newTexMap = true;
        break;
    }
    if ((lastmgr != NULL) && (texmgr != lastmgr))
        lastmgr->free();

    mode |= projection;
    mode |= sliceplane;

    texmgr->setShaderType(shader);
    texmgr->setIntensity(intensity);
    texmgr->setThreshold(threshold);
    texmgr->setBias(bias);
    texmgr->setContours(contours);
    texmgr->setLimit(limit);
    texmgr->setTransparency(transparency);
    texmgr->setAlphaScale(alphascale);
    texmgr->setContrast(contrast);
    texmgr->setColorTable(cspline, NUMCOLORS, palette | texflags);
    texmgr->setBgColor(getStdColor(BGCOLOR));
    texmgr->setGridColor(getStdColor(GRIDCOLOR));

    show = newshow;
}

//-------------------------------------------------------------
// CGLRenderer::render2DPoint() render a single point
//-------------------------------------------------------------
void CGLRenderer::render2DPoint(int pt, int trc, int dtype) {
    if (dtype == 0)
        setReal();
    else
        setImag();
    initPhaseCoeffs(pt);
    renderVertex(0, trc, pt, 0);
}

//-------------------------------------------------------------
// CGLRenderer::render() render the scene
//-------------------------------------------------------------
void CGLRenderer::render(int rflags) {
#ifdef DEBUG_NATIVE
    std::cout << "CGLRenderer::render()" << std::endl;
#endif
    int options = mode;
    int i;

    if ((rflags & LOCKAXIS) > 0)
        options |= FIXAXIS;
    if ((rflags & FROMFRNT) > 0)
        options |= REVDIR;
    if ((rflags & INVALIDVIEW) > 0)
        view->setReset();
    view->setSlice(slice, maxslice, numslices);
    view->setSliceVector(svect.x, svect.y, svect.z);
    view->setOptions(options);

    if (!select_mode)
        view->setView();

    int flags = 0;
    if (newTexMap)
        flags |= NEWTEXMAP;
    if (newTexMode)
        flags |= NEWTEXMODE;
    if (newDataGeometry)
        flags |= NEWTEXGEOM;
    if (newLayout)
        flags |= NEWLAYOUT;
    if (newShader)
        flags |= NEWSHADER;
    if (useLighting) {
        flags |= USELIGHTING;
        options |= LIGHTING;
    }

    newDataGeometry = false;
    newTexMap = false;
    newTexMode = false;
    newLayout = false;
    newShader = false;
    useLighting = false;

    int start = slice;
    int end = 0;
    setDataSource();
    if (texmgr == tex1D) {
        tex3D->invalidate();
        tex2D->invalidate();
    } else if (texmgr == tex2D) {
        tex3D->invalidate();
    } else if (texmgr == tex3D) {
        tex2D->invalidate();
    }

    glEnable(GL_MULTISAMPLE);
    switch (projection) {
    case THREED:
        texmgr->init(mode, flags);
        for (i = 0; i < maxslice; i += step)
            draw2D(i, options);
        texmgr->finish();
        break;
    case SLICES:
        texmgr->init(mode, flags);
        if ((rflags & FROMFRNT) == 0) {
            start = slice + numslices;
            start = start > maxslice ? maxslice : start;
            end = slice;
        } else {
            start = slice;
            end = slice - numslices;
            end = end < 0 ? 0 : end;
        }
        if (view->sliceIsFrontFacing() && !view->sliceIsEyeplane()) {
            for (int i = start; i > end; i -= step)
                draw2D(i, options);
        } else {
            for (int i = end; i <= start; i += step)
                draw2D(i, options);
        }
        texmgr->finish();
        break;
    default:
        if (((show & SHOWLTYPE) == SHOWLINES) && ((show & LINESMOOTH) == 0))
            glDisable(GL_MULTISAMPLE);
        texmgr->init(mode, flags);
        draw2D(slice, options);
        texmgr->finish();
        break;
    }
}

//-------------------------------------------------------------
// CGLRenderer::draw2D() draw a slice
//-------------------------------------------------------------
void CGLRenderer::draw2D(int slc, int options) {
    int t1 = 0, nt = traces, i, trc;
    int dmode = 0;
    bool show_imag = false;
    bool show_real = true;

    bool reversed = (show & SHOWHIDE) > 0;
    if (reversed)
        options = setBit(options, REVERSED);
    else
        options = clrBit(options, REVERSED);

    if (texmgr != tex1D) {
        texmgr->drawSlice(slc, options);
        return;
    }
    texmgr->beginShader(slc, options);

    if (complex) {
        dmode = show & SHOWDTYPE;
        if (dmode == SHOWREAL) {
            show_imag = false;
            show_real = true;
        } else if (dmode == SHOWIMAG) {
            show_real = false;
            show_imag = true;
        } else
            show_real = show_imag = true;
    }

    hide = false;
    if (((show & SHOWLTYPE) != SHOWPOLYGONS) && (projection == OBLIQUE)
            && ((show & SHOWHIDE) > 0) && traces > 1 && !select_mode)
        hide = true;

    int rcolor = phasing ? PHSREALCOLOR : REALCOLOR;
    int icolor = IMAGCOLOR;

    switch (sliceplane) {
    case Y:
        nt = slices;
        break;
    case X:
        nt = traces;
        break;
    case Z:
    default:
        nt = traces;
        break;
    }

    if (projection == ONETRACE) {
        t1 = trace;
        nt = 1;
    }
    bool revtrc = ((projection == TWOD || projection == OBLIQUE)
            && (view->twist < 90 || view->twist >= 270));
    max_trc = revtrc ? t1 : t1 + nt - 1;
    for (i = t1; i <= t1 + nt - step; i += step) {
        // set trace order for transparency (render traces back to front)
        if (revtrc) {
            trc = nt - i - 1;
            next_trc = trc - 1;
        } else {
            trc = i;
            next_trc = i + 1;
        }

        if (hide) {
            glPushAttrib(GL_ENABLE_BIT);
            glDisable(GL_TEXTURE_1D);
            //glDisable(GL_BLEND);
            //setStdColor(BGCOLOR);
            if ((show & SHOWXPARANCY) > 0) {
                glEnable(GL_BLEND);
                float *A = getStdColor(BGCOLOR);
                glColor4f(A[0], A[1], A[2], 0.5f);
            } else {
                glDisable(GL_BLEND);
                setStdColor(BGCOLOR);
            }

            if (show_imag) {
                setImag();
                draw1D(slc, trc, ZMASK | options);
            }
            if (show_real) {
                setReal();
                draw1D(slc, trc, ZMASK | options);
            }
            glPopAttrib();
            glDisable(GL_BLEND);
        }
        switch (show & SHOWLTYPE) {
        case SHOWLINES:
            if (show_real) {
                setStdColor(rcolor);
                setReal();
                draw1D(slc, trc, LINES | options);
            }
            if (show_imag) {
                setStdColor(icolor);
                setImag();
                draw1D(slc, trc, LINES | options);
            }
            break;
        case SHOWPOINTS:
            if (show_real) {
                setStdColor(rcolor);
                setReal();
                draw1D(slc, trc, POINTS | options);
            }
            if (show_imag) {
                setStdColor(icolor);
                setImag();
                draw1D(slc, trc, POINTS | options);
            }
            break;
        case SHOWPOLYGONS:
            glPushAttrib(GL_ENABLE_BIT);
            if ((options & XPARANCY) > 0)
                glEnable(GL_BLEND);
            if (show_imag) {
                setStdColor(icolor);
                setImag();
                draw1D(slc, trc, POLYGONS | options);
            }
            if (show_real) {
                setStdColor(rcolor);
                setReal();
                draw1D(slc, trc, POLYGONS | options);
            }
            glPopAttrib();
            break;
        }
    }
    texmgr->endShader();
}

//-------------------------------------------------------------
// CGLRenderer::draw1D(...)  draw a trace
//-------------------------------------------------------------
void CGLRenderer::draw1D(int slc, int trc, int options) {
    int n = np;
    int drawmode = options & DRAWMODE;

    int opts = options;

    bool polytrace = false;

    switch (options & SLICEPLANE) {
    case Y:
        n = np;
        break;
    case X:
        n = slices;
        break;
    default:
    case Z:
        n = np;
        break;
    }
    usebase = false;
    if (drawmode == POLYGONS)
        polytrace = true;

    next_trc = trc + step;
    ybase = 0;
    switch (projection) {
    case ONETRACE:
    case OBLIQUE:
        if (!select_mode && (polytrace || drawmode == ZMASK)) {
            if (absval)
                ybase = 0;
            else
                ybase = -0.5 * view->ascale / view->dscale;
            if (view->dely <= 0)
                ybase = -ybase;
            usebase = true;
        }
        break;
    }

    bool clipped = false;
    int start = 0;
    int end = np;

    if (!allpoints && !absval) {
        Point3D P1 = getPoint(slc, trc, 0);
        Point3D P2 = getPoint(slc, trc, n);
        Point3D L = view->testBounds(P1, P2);
        if (L.w > 0)
            clipped = true;
        else {
            double m = 4;
            start = (int) (L.x * n);
            end = (int) (L.y * n);
            if (projection == TWOD && trc != max_trc) {
                Point3D P3 = getPoint(slc, next_trc, 0);
                Point3D P4 = getPoint(slc, next_trc, n);
                Point3D L2 = view->testBounds(P3, P4);
                double d1 = L2.x - L.x;
                double d2 = L2.y - L.y;

                int m1 = (int) fabs(d1 * n);
                int m2 = (int) fabs(d2 * n);
                start -= m1;
                end += m2;
            }
            start -= m;
            end += m;
            start = start < 0 ? 0 : start;
            end = end > n ? n : end;
        }
    }

    if (clipped) {
        ntotal += n;
        nclipped += n;
        return;
    }
    start = start - start % (step);
    start = start < 0 ? 0 : start;
    end = end + end % (step);
    if (end > n)
        end = n;
    ntotal += n;
    nclipped += n - (end - start);

    initPhaseCoeffs(start);
    int glmode = GL_QUAD_STRIP;
    switch (drawmode) {
    case ZMASK:
    case POLYGONS:
        glPolygonMode(GL_FRONT, GL_FILL);
        break;
    case LINES:
        glmode = GL_LINE_STRIP;
        break;
    case POINTS:
        glmode = GL_POINTS;
        break;
    }

    if (select_mode) {
        for (int i = start; i < end - step; i += step) {
            int id = (i + trc * n) * 2;
            if (imag)
                id++;
            glLoadName(id);

            switch (drawmode) {
            case POINTS:
                glBegin(GL_POINTS);
                renderVertex(slc, trc, i, opts);
                glEnd();
                incrPhaseCoeffs();
                break;
            case LINES:
                glBegin(GL_LINES);
                renderVertex(slc, trc, i, opts);
                incrPhaseCoeffs();
                renderVertex(slc, trc, i + step, opts);
                glEnd();
                break;
            default:
                if (trc == max_trc)
                    break;
                glBegin(GL_TRIANGLE_FAN);
                renderVertex(slc, trc, i, opts);
                renderVertex(slc, next_trc, i, opts);
                renderVertex(slc, next_trc, i + step, opts);
                renderVertex(slc, trc, i + step, opts);
                glEnd();
                incrPhaseCoeffs();
                break;
            }
        }
        return;
    }

    glPushAttrib(GL_ENABLE_BIT);
    glBegin(glmode);

    for (int i = start; i <= end - step; i += step) {
        last_trc = trc - step;
        switch (projection) {
        case ONETRACE:
        case OBLIQUE:
            if (usebase) { // hidden lines removal
                renderVertex(slc, trc, i, opts); // render masking polygons
                usebase = false;
                renderVertex(slc, trc, i, opts); // render lines or points
                usebase = true;
            } else
                renderVertex(slc, trc, i, opts);
            break;
        case TWOD:
        default:
            if (last_trc < 0)
                break;
            renderVertex(slc, last_trc, i, opts);
            renderVertex(slc, trc, i, opts);
            break;
        }
        incrPhaseCoeffs();
    }
    glEnd();
    glPopAttrib();
}

//-------------------------------------------------------------
// CGLRenderer::renderVertex(x,..)  set ogl attributes for a vertex
//-------------------------------------------------------------
void CGLRenderer::renderVertex(Point3D p, int options) {
    switch (options & COLTYPE) {
    case COLHT:
    case COLIDX:
        if ((options & DRAWMODE) == ZMASK)
            break;
        if ((options & XPARANCY) > 0)
            glColor4d(0, 0, 0, tex1D->getAlphaValue(p.w));
        glTexCoord1d(p.w);
        break;
    }
    glVertex3d(p.x, p.y, p.z);
}

//-------------------------------------------------------------
// CGLRenderer::renderVertex(k,..)  render vertex from index values
//-------------------------------------------------------------
void CGLRenderer::renderVertex(int k, int j, int i, int options) {
    double s = 0;   // texture coordinate
    double v = 0;   // vertex value
    int coltype = options & COLTYPE;
    int id;

    v = (usebase) ? ybase : vertexValue(k, j, i);

    switch (coltype) {
    case COLONE:
        s = 0;
        break;
    case COLIDX:
        id = projection > TWOD ? k : j;
        s = intensity * ((double) id) / ncolors + bias;
        break;
    case COLHT:
        s = yrange * v + bias;
        break;
    }

    Point3D V = getPoint(k, j, i);
    V.w = s;

    switch (projection) {
    case ONETRACE:
    case OBLIQUE:
        V.y = v;
        break;
    case TWOD:
        V.y = v;
        if ((options & LIGHTING) != 0) {
            double v1 = vertexValue(k, j, i + 1);
            double v2 = vertexValue(k, j + 1, i);
            Point3D V1 = getPoint(k, j, i + 1);
            Point3D V2 = getPoint(k, j + 1, i);
            V1.y = v1;
            V2.y = v2;
            V1 = V1.sub(V);
            V2 = V2.sub(V);
            Point3D N = V1.cross(V2);
            N = N.normalize();
            glNormal3d(N.x, N.y, N.z);
        }
        break;
    }
    renderVertex(V, options);
}

//-------------------------------------------------------------
// CGLRenderer::makeCSpline(int)  make color array
//-------------------------------------------------------------
void CGLRenderer::makeCSpline(int options) {
    int i, j, indx;
    double m, f, rem;
    m = (ncolors - 1.0) / (NUMCOLORS - 1.0) / 4.0;
    bool blend = (options & BLEND) > 0;
    int n = 4 * NUMCOLORS;
    float *A = getStdColor(BGCOLOR);

    for (i = 0; i < n; i += 4) {
        f = m * i;
        indx = (int) (f);
        rem = (double) (f - indx);
        if (rem == 0 || !blend) {
            for (j = 0; j < 4; j++)
                cspline[i + j] = (float) (colors[indx * 4 + j]);
        } else if (indx < (ncolors - 1)) {
            for (j = 0; j < 4; j++)
                cspline[i + j] = (float) ((1 - rem) * colors[indx * 4 + j]
                        + rem * colors[indx * 4 + 4 + j]);
        }
    }
    csplineInvalid = false;
}

//-------------------------------------------------------------
// CGLRenderer::getStdColor(int)  return color from stdcols array
//-------------------------------------------------------------
float *CGLRenderer::getStdColor(int i) {
    return stdcols + i * 4;
}

//-------------------------------------------------------------
// CGLRenderer::setStdColor(int)  set color from stdcols array
//-------------------------------------------------------------
void CGLRenderer::setStdColor(int i) {
    int indx = i * 4;
    glColor4f(stdcols[indx], stdcols[indx + 1], stdcols[indx + 2],
            stdcols[indx + 3]);
}

//-------------------------------------------------------------
// CGLRenderer::setColor(int)  set color from color array
//-------------------------------------------------------------
void CGLRenderer::setColor(float *array, int i) {
    int indx = i * 4;
    glColor4f(array[indx], array[indx + 1], array[indx + 2], array[indx + 3]);
}

//-------------------------------------------------------------
// CGLRenderer::selecting()  return true if in GL_SELECT mode
//-------------------------------------------------------------
bool CGLRenderer::selecting() {
    int intbuffer[2];
    glGetIntegerv(GL_RENDER_MODE, intbuffer);
    if (intbuffer[0] == GL_SELECT)
        return true;
    return false;
}
