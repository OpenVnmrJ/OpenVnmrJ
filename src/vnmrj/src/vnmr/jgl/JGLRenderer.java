/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import javax.media.opengl.*;
import javax.media.opengl.glu.*;

import vnmr.util.DebugOutput;
import vnmr.util.Messages;

import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.nio.*;

/**
 * @author deans
 * 
 */
public class JGLRenderer extends JGLDataMgr implements GLRendererIF {
    static final int maxPntsPerPixel = 5;
    private double aspect;
    private double height, width;
    private GL2 gl = null;
    private int show = 0;
    private int mode = 0;
    double intensity = 0.0;
    double contrast = 0.0;
    double bias = 0.0;
    double threshold = 0.0;
    double contours = 0.01;
    double limit = 1.0;
    double transparency = 0.0;
    double alphascale = 1.0;
    int maxslice = 0;
    int numslices = 0;
    int maxtrace = 0;
    int numtraces = 0;
    boolean revtrc;

    boolean draw_axis = true;
    int shader = NONE;
    // private ByteBuffer mapData=null;

    private boolean csplineInvalid = true;

    boolean newLayout = true;
    boolean newShader = true;
    boolean newTexMap = true;
    boolean newTexMode = false;
    boolean useLighting = false;
    boolean usebase = false;
    boolean select_mode = false;

    int next_trc = 0;
    int max_trc = 0;
    boolean phasing = false;
    int skip = 0;

    private float ctrcols[] = new float[NUMCTRCOLS * 4];
    private float phscols[] = new float[NUMPHSCOLS * 4];
    private float abscols[] = new float[NUMABSCOLS * 4];
    private float grays[] = new float[NUMGRAYS * 4];
    private float stdcols[] = new float[NUMSTDCOLS * 4];

    private float colors[] = null;
    private float cspline[] = null;
    private int ncolors = 0;
    int palette = GRAYCOLS;

    double yrange;
    int nclipped = 0;
    int ntotal = 0;

    double ybase = 0;
    boolean hide = false;

    JGLTexMgr texmgr = null;
    JGLTexMgr tex1D = null;
    JGLTexMgr tex2D = null;
    JGLTexMgr tex3D = null;

    GLU glu = new GLU();

    Point3D minCoord = new Point3D(0.0, -0.5, 0.0);
    Point3D maxCoord = new Point3D(1.0, 0.5, 1.0);

    Volume volume = new Volume(minCoord, maxCoord);

    JGLView view = null;

    Point3D svect = new Point3D();

    public JGLRenderer() {
        aspect = 1.0;
        np = 0;
        traces = 0;
        cspline = new float[NUMCOLORS * 4];
        view = new JGLView(volume);
        tex1D = new JGLTex1DMgr(view, (JGLDataMgr) this);
        tex2D = new JGLTex2DMgr(view, (JGLDataMgr) this);
        tex3D = new JGLTex3DMgr(view, (JGLDataMgr) this);

    }

    // ############### GLRendererIF methods ########################
    public void destroy() {
    }

    /**
     * Set slice vector.
     */
    public void setSliceVector(double x, double y, double z, double w) {
        svect = new Point3D(x, y, z, w);
    }

    /**
     * Set data step.
     */
    public void setStep(int r) {
        step = r;
        step = step < 1 ? 1 : step;
    }

    /**
     * Set minimum intensity for alpha>0.
     */
    public void setThreshold(double r) {
        if (r != threshold)
            newTexMap = true;
        threshold = r;
    }

    /**
     * Set contour intervals
     */
    public void setContours(double r) {
        contours = r;
    }

    /**
     * Set maximum intensity for alpha>0.
     */
    public void setLimit(double l) {
        if (l != limit)
            newTexMap = true;
        limit = l;
    }

    /**
     * Set minimum intensity for alpha>0.
     */
    public void setTransparency(double r) {
        if (r != transparency)
            newTexMap = true;
        transparency = r;
    }

    /**
     * Set alpha super-sampling scaling factor.
     */
    public void setAlphaScale(double r) {
        if (r != alphascale)
            newTexMap = true;
        alphascale = r;
    }

    /**
     * Set intensity factor.
     */
    public void setIntensity(double r) {
        if (r != intensity)
            newTexMap = true;
        intensity = r;
        yrange = intensity / (ymax - ymin);
    }

    /**
     * Set color table bias .
     */
    public void setBias(double r) {
        bias = r;
    }

    /**
     * Set contrast factor.
     */
    public void setContrast(double r) {
        if (r != contrast) {
            contrast = r;
            newTexMap = true;
        }
    }

    /**
     * Set complex data phase factors.
     */
    public void setPhase(double r, double l) {
        rp = r;
        lp = l;
    }

    /**
     * Set view scale factors.
     */
    public void setScale(double a, double x, double y, double z) {
        view.setScale(a, x, y, z);
    }

    /**
     * Set span factors.
     */
    public void setSpan(double x, double y, double z) {
        view.setSpan(x, y, z);
    }

    /**
     * Set linear view offsets.
     */
    public void setOffset(double x, double y, double z) {
        view.setOffset(x, y, z);
    }

    /**
     * Set rotation angles for 3D projections.
     */
    public void setRotation3D(double x, double y, double z) {
        view.setRotation3D(x, y, z);
    }

    /**
     * Set rotation angles for 3D projections.
     */
    public void setRotation2D(double x, double y) {
        view.setRotation2D(x, y);
    }

    /**
     * Set Object rotation angles for 3D projections.
     */
    public void setObjectRotation(double x, double y, double z) {
        view.vrot = new Point3D(x, y, z);
    }

    /**
     * Set slant for oblique projection.
     */
    public void setSlant(double x, double y) {
        view.setSlant(x, y);
    }

    /**
     * Set current trace.
     */
    public void setTrace(int t, int max, int num) {
        trace = t;
        maxtrace = max;
        numtraces = num;
    }

    /**
     * Set current slice.
     */
    public void setSlice(int s, int max, int num) {
        slice = s;
        maxslice = max;
        numslices = num;
    }

    public void setDataScale(double mn, double mx) {
        if (mx != ymax)
            newDataGeometry = true;
        ymax = mx;
        ymin = mn;
        scale = 1.0f / (ymax - ymin);
        view.setDataScale(mn, mx);
        tex1D.setDataScale(ymin, ymax);
        tex2D.setDataScale(ymin, ymax);
        tex3D.setDataScale(ymin, ymax);
    }

    /**
     * Import vertex data.
     */
    public void setDataPars(int n, int t, int s, int dtype) {
        super.setDataPars(n, t, s, dtype);
        view.setDataPars(np, traces, slices, dtype);
    }

    public void render2DPoint(int pt, int trc, int dtype) {
        if (dtype == 0)
            setReal();
        else
            setImag();
        initPhaseCoeffs(pt);
        renderVertex(0, trc, pt, 0);
    }

    /**
     * Initialize color table
     */
    public void setColorArray(int id, float[] data) {
        int i, j, k;
        switch (id) {
        case STDCOLS:
            for (i = 0; i < NUMSTDCOLS * 4; i++)
                stdcols[i] = data[i];
            break;
        case ABSCOLS:
            ncolors = NUMABSCOLS;
            colors = abscols;
            for (i = 0; i < ncolors * 4; i++)
                colors[i] = data[i];
            break;
        case PHSCOLS:
            ncolors = NUMPHSCOLS;
            colors = phscols;
            for (i = 0; i < ncolors * 4; i++)
                colors[i] = data[i];
            break;
        case CTRCOLS:
            ncolors = NUMCTRCOLS;
            colors = ctrcols;
            for (i = 0; i < ncolors * 4; i++)
                colors[i] = data[i];
            break;
        case GRAYCOLS: {
            colors = grays;
            ncolors = NUMGRAYS;
            float[] A = new float[4];
            float[] B = new float[4];
            j = 0;
            for (k = 0; k < 4; k++, j++)
                A[k] = data[j];
            for (k = 0; k < 4; k++, j++)
                B[k] = data[j];
            float delta = 1.0f / (NUMGRAYS - 1);
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

    /**
     * Initialize openGL
     */
    public void init(int s) {
        shader = s & SHADER;
        float pos[] = { 5.0f, 5.0f, 10.0f, 0.0f };
        gl = GLU.getCurrentGL().getGL2();
        gl.glDisable(GL2.GL_LINE_SMOOTH);
        gl.glLightfv(GL2.GL_LIGHT0, GL2.GL_POSITION, pos, 0);
        gl.glDisable(GL2.GL_CULL_FACE);
        gl.glEnable(GL2.GL_DEPTH_TEST);
        gl.glEnable(GL2.GL_POINT_SMOOTH);
        gl.glDepthFunc(GL2.GL_LEQUAL);
        gl.glHint(GL2.GL_POINT_SMOOTH_HINT, GL2.GL_NICEST);
        gl.glDisable(GL2.GL_BLEND);
        gl.glBlendFunc(GL2.GL_SRC_ALPHA, GL2.GL_ONE_MINUS_SRC_ALPHA);
        gl.glShadeModel(GL2.GL_SMOOTH);
        gl.glDisable(GL2.GL_LIGHTING);
        float A[] = getStdColor(BGCOLOR);
        //gl = GLU.getCurrentGL().getGL2();
        gl.glClearColor(A[0], A[1], A[2], 0.0f);
        //gl.glClear(GL2.GL_COLOR_BUFFER_BIT | GL2.GL_DEPTH_BUFFER_BIT);
        view.init();
    }

    /**
     * Resize window
     */
    public void resize(int w, int h) {
        width = w;
        height = h;
        aspect = height / width;
        newDataGeometry = true;
        newLayout = true;
        view.setResized();
    }

    /**
     * Set render pass options
     */
    public void setOptions(int id, int newshow) {
        mode = 0;
        int last;
        int texflags = 0;
        if ((newshow & SHOWTEXOPTS) != (show & SHOWTEXOPTS)) {
            newTexMap = true;
            newTexMode = true;
            csplineInvalid = true;
        }
        gl = GLU.getCurrentGL().getGL2();

        select_mode = selecting();

        nclipped = ntotal = 0;

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

        gl.glDisable(GL2.GL_TEXTURE_1D);

        if ((mode & BLEND) > 0)
            gl.glShadeModel(GL2.GL_SMOOTH);
        else
            gl.glShadeModel(GL2.GL_FLAT);

        switch (newshow & SHOWCTYPE) {
        case SHOWONECOL:
            mode = setBits(mode, COLTYPE, COLONE);
            break;
        case SHOWIDCOL:
            mode = setBits(mode, COLTYPE, COLIDX);
            gl.glEnable(GL2.GL_TEXTURE_1D);
            break;
        case SHOWHTCOL:
            gl.glEnable(GL2.GL_TEXTURE_1D);
            mode = setBits(mode, COLTYPE, COLHT);
            break;
        }

        if (show == 0 || (show & SHOWCTYPE) != (newshow & SHOWCTYPE)) {
            csplineInvalid = true;
            newTexMap = true;
            if (projection == TWOD)
                newShader = true;
        }
        if (projection == TWOD && !select_mode)
            tex1D.setUseShader(true);
        else
            tex1D.setUseShader(false);

        JGLTexMgr lastmgr = texmgr;
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
        if ((lastmgr != null) && (texmgr != lastmgr))
            lastmgr.free();

        mode |= sliceplane;
        mode |= projection;

        texmgr.setShaderType(shader);
        texmgr.setIntensity(intensity);
        texmgr.setThreshold(threshold);
        texmgr.setBias(bias);
        texmgr.setContours(contours);
        texmgr.setLimit(limit);
        texmgr.setTransparency(transparency);
        texmgr.setAlphaScale(alphascale);
        texmgr.setContrast(contrast);
        texmgr.setColorTable(cspline, palette | texflags);
        texmgr.setBgColor(getStdColor(BGCOLOR));
        texmgr.setGridColor(getStdColor(GRIDCOLOR));

        show = newshow;
    }

    /**
     * Render data content
     */
    public void render(int rflags) {
        int options = mode;
        int i;

        if ((rflags & LOCKAXIS) > 0)
            options |= FIXAXIS;
        if ((rflags & FROMFRNT) > 0)
            options |= REVDIR;
        if ((rflags & INVALIDVIEW) > 0)
            view.setReset();
        view.setSlice(slice, maxslice, numslices);
        view.setSliceVector(svect.x, svect.y, svect.z);
        view.setOptions(options);

        if (!select_mode)
            view.setView();

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
            tex3D.invalidate();
            tex2D.invalidate();
        } else if (texmgr == tex2D) {
            tex3D.invalidate();
        } else if (texmgr == tex3D) {
            tex2D.invalidate();
        }
        gl.glEnable(GL2.GL_MULTISAMPLE);
        switch (projection) {
        case THREED:
            texmgr.init(mode, flags);
            for (i = 0; i < maxslice; i += step) {
                draw2D(i, options);
            }
            texmgr.finish();
            break;
        case SLICES:
            texmgr.init(mode, flags);
            if ((rflags & FROMFRNT) == 0) {
                start = slice + numslices;
                start = start > maxslice ? maxslice : start;
                end = slice;
            } else {
                start = slice;
                end = slice - numslices;
                end = end < 0 ? 0 : end;
            }
            if (view.sliceIsFrontFacing() && !view.sliceIsEyeplane()) {
                for (i = start; i > end; i -= step)
                    draw2D(i, options);
            } else {
                for (i = end; i <= start; i += step)
                    draw2D(i, options);
            }
            texmgr.finish();
            break;
        default:
            if (((show & SHOWLTYPE) == SHOWLINES) && ((show & LINESMOOTH) == 0))
                gl.glDisable(GL2.GL_MULTISAMPLE);
            texmgr.init(mode, flags);
            draw2D(slice, options);
            texmgr.finish();

            break;
        }
    }

    // ############### private methods ########################

    /**
     * Draw a 1D trace or a single 2D plane of data points
     * 
     * @param slc
     *            constant for <=TWO2
     * @param options
     */
    private void draw2D(int slc, int options) {
        int t1 = 0, nt = traces, i, trc;
        int dmode = 0;
        boolean show_imag = false;
        boolean show_real = true;
        boolean reversed = (show & SHOWHIDE) > 0;
        if (reversed)
            options = setBit(options, REVERSED);
        else
            options = clrBit(options, REVERSED);

        if (texmgr != tex1D) {
            texmgr.drawSlice(slc, options);
            return;
        }

        texmgr.beginShader(slc, options);

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

        switch (options & SLICEPLANE) {
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

        revtrc = ((projection == TWOD || projection == OBLIQUE) && (view.twist < 90 || view.twist >= 270));
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
                gl.glPushAttrib(GL2.GL_ENABLE_BIT);
                gl.glDisable(GL2.GL_TEXTURE_1D);
                if ((show & SHOWXPARANCY) > 0) {
                    gl.glEnable(GL2.GL_BLEND);
                    float A[] = getStdColor(BGCOLOR);
                    gl.glColor4f(A[0], A[1], A[2], 0.5f);
                } else {
                    gl.glDisable(GL2.GL_BLEND);
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
                gl.glPopAttrib();
                gl.glDisable(GL2.GL_BLEND);
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
                gl.glPushAttrib(GL2.GL_ENABLE_BIT);
                if ((options & XPARANCY) > 0)
                    gl.glEnable(GL2.GL_BLEND);

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
                gl.glPopAttrib();
                break;
            }
        }
        texmgr.endShader();
    }

    /**
     * Draw a single trace of data - called from "draw2D" for all projections
     * <=TWOD - 3D projections call JGLTexMgr.drawSlice instead
     * 
     * @param slc
     *            constant for <=TWO2
     * @param trc
     *            incrementing 0..max(sliceplane)
     * @param options
     */
    private void draw1D(int slc, int trc, int options) {
        int n;
        int drawmode = options & DRAWMODE;

        int opts = options;

        boolean polytrace = false;

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
        if (drawmode == POLYGONS)
            polytrace = true;
        ybase = 0;
        usebase = false;
        next_trc = trc + step;
        switch (projection) {
        case ONETRACE:
        case OBLIQUE:
            if (!select_mode && (polytrace || drawmode == ZMASK)) {
                if (absval)
                    ybase = 0;
                else
                    ybase = -0.5 * view.ascale / view.dscale;
                if (view.dely <= 0)
                    ybase = -ybase;
                usebase = true;
            }
            break;
        }
        int start = 0;
        int end = np;
        boolean clipped = false;

        if (!allpoints && !absval) {
            Point3D P1 = getPoint(slc, trc, 0);
            Point3D P2 = getPoint(slc, trc, n);
            Point3D L = view.testBounds(P1, P2);
            if (L.w > 0)
                clipped = true;
            else {
                double m = 4;
                start = (int) (L.x * n);
                end = (int) (L.y * n);
                if (projection == TWOD && trc != max_trc) {
                    Point3D P3 = getPoint(slc, next_trc, 0);
                    Point3D P4 = getPoint(slc, next_trc, n);
                    Point3D L2 = view.testBounds(P3, P4);
                    double d1 = L2.x - L.x;
                    double d2 = L2.y - L.y;

                    int m1 = (int) Math.abs(d1 * n);
                    int m2 = (int) Math.abs(d2 * n);
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
        int glmode = GL2.GL_QUAD_STRIP;
        switch (drawmode) {
        case ZMASK:
        case POLYGONS:
            gl.glPolygonMode(GL2.GL_FRONT, GL2.GL_FILL);
            break;
        case LINES:
            glmode = GL2.GL_LINE_STRIP;
            break;
        case POINTS:
            glmode = GL2.GL_POINTS;
            break;
        }

        if (select_mode) {
            for (int i = start; i < end - step; i += step) {
                int id = (i + trc * n) * 2;
                if (imag)
                    id++;
                gl.glLoadName(id);

                switch (drawmode) {
                case POINTS:
                    gl.glBegin(GL2.GL_POINTS);
                    renderVertex(slc, trc, i, opts);
                    gl.glEnd();
                    incrPhaseCoeffs();
                    break;
                case LINES:
                    gl.glBegin(GL2.GL_LINES);
                    renderVertex(slc, trc, i, opts);
                    incrPhaseCoeffs();
                    renderVertex(slc, trc, i + step, opts);
                    gl.glEnd();
                    break;
                default:
                    if (trc == max_trc)
                        break;
                    gl.glBegin(GL2.GL_TRIANGLE_FAN);
                    renderVertex(slc, trc, i, opts);
                    renderVertex(slc, next_trc, i, opts);
                    renderVertex(slc, next_trc, i + step, opts);
                    renderVertex(slc, trc, i + step, opts);
                    gl.glEnd();
                    incrPhaseCoeffs();
                    break;
                }
            }
            return;
        }

        gl.glPushAttrib(GL2.GL_ENABLE_BIT);
        gl.glBegin(glmode);

        for (int i = start; i <= end - step; i += step) {
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
                if (trc == max_trc)
                    break;
                //if ((options & LIGHTING) != 0 && i >= end - step - 1)
                //    break;
                renderVertex(slc, trc, i, opts);
                renderVertex(slc, next_trc, i, opts);
                break;
            }
            incrPhaseCoeffs();
        }
        gl.glEnd();
        gl.glPopAttrib();
    }

    /**
     * Set OpenGL Vertex attributes
     * 
     * @param Point3D
     *            p Vertex coordinates
     * @param options
     *            color mode
     */
    private void renderVertex(Point3D p, int options) {
        switch (options & COLTYPE) {
        case COLHT:
        case COLIDX:
            if ((options & DRAWMODE) == ZMASK)
                break;
            if ((options & XPARANCY) > 0)
                gl.glColor4d(0, 0, 0, tex1D.getAlphaValue(p.w));
            gl.glTexCoord1d(p.w);
            break;
        }
        gl.glVertex3d(p.x, p.y, p.z);
    }

    /**
     * Draw a vertex at the specified coordinates
     * 
     * @param k
     *            slice incrementing(3D) fixed (1D,2D)
     * @param j
     *            trace incrementing(2D) fixed (1D)
     * @param i
     *            point incrementing(ALL)
     * @param options
     *            color mode
     */
    private void renderVertex(int k, int j, int i, int options) {
        double s = 0; // texture coordinate
        double v = 0; // vertex value
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
                gl.glNormal3d(N.x, N.y, N.z);
            }
            break;
        }
        renderVertex(V, options);
    }

    // Helper functions

    /**
     * fill a fixed length color array from a set of input colors
     * 
     * @param options
     */
    private void makeCSpline(int options) {
        int i, j, indx;
        double m, f, rem;
        m = (ncolors - 1.0) / (NUMCOLORS - 1.0) / 4.0;
        int n = 4 * NUMCOLORS;
        boolean blend = (options & BLEND) > 0;
        // float A[]=getStdColor(BGCOLOR);
        for (i = 0; i < n; i += 4) {
            f = m * i;
            indx = (int) (f);
            rem = (double) (f - indx);
            if (rem == 0 || !blend) {
                for (j = 0; j < 4; j++)
                    cspline[i + j] = (float) (colors[indx * 4 + j]);
            } else if (indx < (ncolors - 1)) {
                for (j = 0; j < 4; j++)
                    cspline[i + j] = (float) ((1 - rem) * colors[indx * 4 + j] + rem
                            * colors[indx * 4 + 4 + j]);
            }
        }
        csplineInvalid = false;
    }

    private void setStdColor(int i) {
        int indx = i * 4;
        gl.glColor4f(stdcols[indx], stdcols[indx + 1], stdcols[indx + 2],
                stdcols[indx + 3]);
    }

    private float[] getStdColor(int i) {
        float A[] = new float[4];
        int indx = i * 4;
        A[0] = stdcols[indx];
        A[1] = stdcols[indx + 1];
        A[2] = stdcols[indx + 2];
        A[3] = stdcols[indx + 3];
        return A;
    }

    // Utilities

    private int setBit(int w, int b) {
        return w | b;
    }

    private int clrBit(int w, int b) {
        return w & (~b);
    }

    private int setBits(int w, int m, int b) {
        int r = w & (~m);
        return r | b;
    }

    private boolean bitIsSet(int i, int b) {
        if ((i & b) == 0)
            return false;
        return true;
    }

    private boolean selecting() {
        IntBuffer intbuffer = IntBuffer.allocate(1);
        gl.glGetIntegerv(GL2.GL_RENDER_MODE, intbuffer);
        if (intbuffer.get(0) == GL2.GL_SELECT)
            return true;
        return false;
    }

};
