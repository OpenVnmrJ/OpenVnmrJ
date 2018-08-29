/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>		// For unlink()
#include <signal.h>		// For kill()
#include <dirent.h>		// Directory info
#include <math.h>               // For rint()
#include <stdlib.h>               // For rint()
#include <string>
using std::string;

void
convertData(unsigned char *indata, float *cdata, int datasize)
{
    while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
convertData(unsigned short *indata, float *cdata, int datasize)
{
    while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
convertData(int *indata, float *cdata, int datasize)
{
    while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
convertData(double *indata, float *cdata, int datasize)
{
    while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
convertData(float *cdata, unsigned char *outdata, int datasize,
	    double scale_factor, double offset)
{
    double temp;
    const double max = 0xff;
    while (datasize-- > 0)
    {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max) temp = max;
	if ( temp < 0.0 ) temp = 0.0;
   	*outdata++ = (unsigned char) temp;
    }
}

void
convertData(float *cdata, unsigned short *outdata, int datasize,
	    double scale_factor, double offset)
{
    double temp;
    const double max = 0xffff;
    while (datasize-- > 0)
    {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max) temp = max;
	if ( temp < 0.0 ) temp = 0.0;
   	*outdata++ = (unsigned short) temp;
    }
}

void
convertData(float *cdata, short *outdata, int datasize,
	    double scale_factor, double offset)
{
    double temp;
    const double max = 0x7fff;
    const double min = -max;

    while (datasize-- > 0)
    {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max){
	    temp = max;
	}else if ( temp < min ){
	    temp = min;
	}
   	*outdata++ = (short)temp;
    }
}

void
convertData(float *cdata, int *outdata, int datasize,
	    double scale_factor, double offset)
{
    double temp;
    const double max = 0x7fffffff;
    const double min = -max;

    while (datasize-- > 0)
    {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max){
	    temp = max;
	}else if ( temp < min ){
	    temp = min;
	}
   	*outdata++ = (int)temp;
    }
}

/*
 * Deletes files with names of the form "baseFilenamePIDxxx",
 * where "baseFilename" is an absolute (or maybe relative) path plus
 * file prefix, "PID" is the PID of the process that uses the file,
 * and "xxx" is any file suffix.
 * Files will be deleted iff they match the form AND the "PID" does
 * not match any running process.
 */
void
deleteOldFiles(const string& baseFilename)
{
    DIR *dirp;			/* directory pointer */
    struct dirent *dp;		/* directory pointer for each file */

    // Get directory name and file pfx
    string::size_type pos = baseFilename.rfind('/');
    string dir;
    string filepfx;
    if (pos == string::npos) {
	dir = ".";
	filepfx = baseFilename;
    } else {
	dir = baseFilename.substr(0, pos);
	filepfx = baseFilename.substr(pos+1);
    }
    string::size_type pfxlen = filepfx.size();
    // Get directory info
    if (!(dirp = opendir(dir.c_str()))) {
	return;
    }
    // Check each file in directory
    while ( (dp = readdir(dirp)) ) {
	string name(dp->d_name);
	if (name.compare(0, pfxlen, filepfx) == 0) {
	    // It has the right prefix
	    int pid = atoi(name.substr(pfxlen).c_str());
	    if (kill(pid, 0) == -1) {
		// File belongs to a process that has exited.
		unlink((dir + "/" + name).c_str());
	    }
	}
    }
    closedir(dirp);
}

/*
 * Multiply 2 3x3 matrices: dst = src1 * src2
 * First index is row number [0:2].
 */
void
multMat3(double src1[3][3], double src2[3][3], double dst[3][3])
{
    int i, j, k;
    for (i=0; i<3; ++i) {
        for (j=0; j<3; ++j) {
	    dst[i][j] = 0;
	    for (k=0; k<3; k++){
		dst[i][j] += src1[i][k] * src2[k][j];
	    }
	}
    }
}

/*
 * For use by clipLineToRect().
 */
static bool
clipParms(double denom, double num, double& tEnter, double& tExit)
{
    double t;
    bool accept = true;
    if (denom > 0) {
        // Possible Enter event
        t = num / denom;
        if (t > tExit) {
            accept = false;
        } else if (t > tEnter) {
            tEnter = t;
        }
    } else if (denom < 0) {
        // Possible Exit event
        t = num / denom;
        if (t < tEnter) {
            accept = false;
        } else if (t < tExit) {
            tExit = t;
        }
    } else if (num > 0) {
        accept = false;
    }
    return accept;
}

/*
bool
clipLineToRect(int& x0, int& y0, int& x1, int& y1, // Line
               int rx0, int ry0, int rx1, int ry1) // Rect
{
    double dx0 = x0;
    double dy0 = y0;
    double dx1 = x1;
    double dy1 = y1;
    double drx0 = rx0;
    bool rtn = clipLineToRect(dx0, dy0, dx1, dy1,
                              drx0, (double)ry0,
                              (double)rx1, (double)ry1);
    x0 = (int)dx0;
    y0 = (int)dy0;
    x1 = (int)dx1;
    y1 = (int)dy1;
    return rtn;
}
*/

/*
 * Clip a line segment against a rectangle.
 * Returns true if line is at least partially visible, and modifies the
 * line endpoints.
 * Returns false if line is entirely invisible.
 * Liang-Barsky algorithm: See Computer Graphics, 2nd Ed. Foley, et. al. 1990
 * Uses: clipParms().
 */
bool
clipLineToRect(double& x0, double& y0, double& x1, double& y1, // Line segment
               double rx0, double ry0, double rx1, double ry1) // Rect corners
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    bool visible = false;
    if (dx == 0 && dy == 0) {
        if (x0 >= rx0 && x0 <= rx1 && y0 >= ry0 && y0 <= ry1) {
            // Degenerate line, entirely within rectangle
            visible = true;
        }
    } else {
        double tEnter = 0;      // Init parameter values for where line enters
        double tExit = 1;       // ... and leaves rectangle.
        if (clipParms(dx, rx0 - x0, tEnter, tExit)) {
            if (clipParms(-dx, x0 - rx1, tEnter, tExit)) {
                if (clipParms(dy, ry0 - y0, tEnter, tExit)) {
                    if (clipParms(-dy, y0 - ry1, tEnter, tExit)) {
                        visible = true;
                        if (tExit < 1) {
                            x1 = (int)rint(x0 + tExit * dx);
                            y1 = (int)rint(y0 + tExit * dy);
                        }
                        if (tEnter > 0) {
                            x0 = (int)rint(x0 + tEnter * dx);
                            y0 = (int)rint(y0 + tEnter * dy);
                        }
                    }
                }
            }
        }
    }
    return visible;
}
