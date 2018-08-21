/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPCINTERFACE_H
#define AIPCINTERFACE_H

#include "aipCStructs.h"
#include "aipSpecStruct.h"

/*
 * Functions to interface to the Advanced Image Processing interface.
 *
 * In addition to the following functions, there are Vnmr commands to
 * load data into AIP, display data, refresh the display, etc.
 */
#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Display Listener Functions.
 */

/*
 * Register a function to be called whenever the AIP display is
 * updated.  Whenever one or more images are redrawn or deleted by
 * AIP, the registered PDLF_t function(s) will be called with
 * the following arguments:
 * "len" is the length of the following arrays.
 * "ids" is an array of integer IDs specifying the images that have
 * been redrawn.  The ID is used to get further information about
 * the image.
 * "changeFlags" indicate whether the image has changed or just been
 * refreshed.  If a changeFlag is "true", the ID will need to be
 * used to get further information about the image.  If the
 * "changeFlag" is false, just redraw whatever was drawn before.
 */
/* Pointer to Display Listener Function */
typedef void (*PDLF_t)(int len, int *ids, int *changeFlags);
void aipRegisterDisplayListener(PDLF_t func);
void aipUnregisterDisplayListener(PDLF_t func);

/*
 * Return number of images being displayed
 */
int aipGetNumberOfImages();

/*
 * Return the IDs of displayed images in the caller provided array "ids".
 * "len" is the length of the "ids" array.
 * Returns the number of image IDs loaded into the array.
 */
int aipGetImageIds(int len, int *ids);

/*
 * Get the full filepath for a given image.
 * "id" is the ID of the image.
 * "buf" is a buffer where the result is returned.
 * "buflen" is the size of the buffer.
 * Returns the original length of the path, or a zero length string if
 * the ID refers to an empty frame.  The path in the buffer may
 * be truncated.  The string in the buffer will be null terminated, so
 * its maximum length is (buflen - 1).
 * Buffer overflow can be checked as  follows:
 *
 *   if (aipGetImagePath(id, buf, buflen) >= buflen)
 *	 return -1;
 */
int aipGetImagePath(int id, char *buf, int buflen);

/*
 * Get the image description of an image identitied by "id".
 * Caller supplies a CImgInfo_t buffer to put the info in.
 * Returns "true" on success, "false" if the CImgInfo_t was not set.
 * A "false" return means that this image does not exist, so the
 * caller should not try to write on it.
 */
int aipGetImageInfo(int id, CImgInfo_t *imgInfo);
int aipGetFrameInfo(int id, CFrameInfo_t *cinfo);

/*
 * Force AIP to redraw the image that corresponds to a given ID.
 * Returns "true" on success, "false" if requested image does not exist.
 */
int aipRefreshImage(int id);
 
/*
 * Mouse Listener Functions.
 */
typedef void (*PMLF_t)(int x, int y, int button, int mask, int dummy);
typedef void (*PMQF_t)();
void aipRegisterMouseListener(PMLF_t func, PMQF_t quit);
void aipUnregisterMouseListener(PMLF_t func, PMQF_t quit);

/*
 * Check if AIP is responsible for keeping screen updated.
 */
int aipOwnsScreen();

/*
 * Read an FDF file and display it in a specified frame.
 *
 * path: The full file path.
 * frame: The index number of the frame, starting from 0.  If
 *        frame < 0, AIP will choose a frame.
 *
 * return: 1 on success, 0 on failure.
 */
int aipDisplayFile(char *path, int frame);

/* Auto Scale */
void aipAutoScale();

  /* mrk */
int aipJavaFile(char *path, int *nx, int *ny, int *ns, float *sx, float *sy, float *sz, float **jdata );

/*
 * ky is fullpath +' '+ fdf filename
 * nm is param name in fdf header
 * idx is -1 for nonarrayed params, 0,1,2... for arrayed params
 * value is an ptr to the value of the param
*/
int aipGetHdrReal(char *ky, char *nm, int idx, double *value);
int aipGetHdrInt(char *ky, char *nm, int idx, int *value);
int aipGetHdrStr(char *ky, char *nm, int idx, char **value);

void aipDeleteFrames();

void aipUpdateRQ();
void aipSetReconDisplay(int nimages);
void aipMouseWheel(int clicks, double factor);
specStruct_t *aipGetSpecStruct(char *key);
void aipSpecViewUpdate();
float *aipGetTrace(char *key, int ind, double scale, int npt);
void aipSelectViewLayer(int imgId, int order, int mapId);
int aspFrame(char *keyword, int frameID, int x, int y, int w, int h);
void aspMouseWheel(int clicks, double factor);
int loadFdfSpec(char *path, char *key);

#ifdef	__cplusplus
}
#endif

#endif /* AIPCINTERFACE_H */
