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


#include "vjNative.h"
#include "jawt_md.h"


JNIEXPORT jint JNICALL Java_vnmr_ui_VNMRFrame_getXwinId
  (JNIEnv *env, jobject obj, jobject source)
{
        JAWT awt;
        JAWT_DrawingSurface *ds;
        JAWT_DrawingSurfaceInfo *dsi;
        JAWT_X11DrawingSurfaceInfo *dsi_win;
        Display* dpy;
	jint lock, wid;

        wid = 0;
	awt.version = JAWT_VERSION_1_4;
	if (JAWT_GetAWT(env, &awt) == JNI_FALSE) {
	    printf(" JAVA AWT not found \n");
	    return (wid);
	}

	ds = awt.GetDrawingSurface(env, source);
	if (ds == NULL) {
	    printf(" JAVA Window not found \n");
	    return (wid);
	}

	lock = ds->Lock(ds);
	if ((lock & JAWT_LOCK_ERROR) != 0) {
	    printf(" JAVA Error on locking window \n");
	    awt.FreeDrawingSurface(ds);
	    return (wid);
	}

	dsi = ds->GetDrawingSurfaceInfo(ds);
	if (dsi != NULL) {
	    dsi_win = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo;
	    if (dsi_win != NULL) {
		wid = (jint) dsi_win->drawable;
	    }
	    ds->FreeDrawingSurfaceInfo(dsi);
	}
	ds->Unlock(ds);
	awt.FreeDrawingSurface(ds);
	return (wid);
}

JNIEXPORT jint JNICALL Java_vnmr_ui_VNMRFrame_syncXwin
  (JNIEnv *env, jobject obj, jobject source)
{
        JAWT awt;
        JAWT_DrawingSurface *ds;
        JAWT_DrawingSurfaceInfo *dsi;
        JAWT_X11DrawingSurfaceInfo *dsi_win;
        Display* dpy;
        Drawable xwin;
        GC       gc;
        jint     ret;
	jint lock;

        ret = 0;
	awt.version = JAWT_VERSION_1_4;
	if (JAWT_GetAWT(env, &awt) == JNI_FALSE) {
	    printf(" JAVA AWT not found \n");
	    return (ret);
	}

	ds = awt.GetDrawingSurface(env, source);
	if (ds == NULL) {
	    printf(" JAVA Window not found \n");
	    return (ret);
	}

	lock = ds->Lock(ds);
	if ((lock & JAWT_LOCK_ERROR) != 0) {
	    printf(" JAVA Error on locking window \n");
	    awt.FreeDrawingSurface(ds);
	    return (ret);
	}

	dsi = ds->GetDrawingSurfaceInfo(ds);
	if (dsi != NULL) {
	    dsi_win = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo;
	    if (dsi_win != NULL) {
                xwin = dsi_win->drawable;
                dpy = dsi_win->display;
                XSynchronize(dpy, 1);
                gc = XCreateGC(dpy, xwin, 0, 0);
                XSetForeground(dpy, gc, 2);
                XFreeGC(dpy, gc);
                XSynchronize(dpy, 0);
		ret = 1;
	    }
	    ds->FreeDrawingSurfaceInfo(dsi);
	}
	ds->Unlock(ds);
	awt.FreeDrawingSurface(ds);
	return (ret);
}

