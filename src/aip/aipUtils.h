/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPUTILS_H
#define AIPUTILS_H

#include <string>

void convertData(unsigned char *, float *, int datasize);
void convertData(unsigned short *, float *, int datasize);
void convertData(int *, float *, int datasize);
void convertData(double *, float *, int datasize);
void convertData(float *, unsigned char *, int datasize, 
		 double scale_factor, double offset);
void convertData(float *, unsigned short *, int datasize, 
		 double scale_factor, double offset);
void convertData(float *, short *, int datasize, 
		 double scale_factor, double offset);
void convertData(float *, int *, int datasize, 
		 double scale_factor, double offset);

void deleteOldFiles(const std::string& basename);
void multMat3(double src1[3][3], double src2[3][3], double dst[3][3]);
bool clipLineToRect(double& x0, double& y0, double& x1, double& y1, // Line
                    double rx0, double ry0, double rx1, double ry1); // Rect
/*
bool clipLineToRect(int& x0, int& y0, int& x1, int& y1, // Line
                    int rx0, int ry0, int rx1, int ry1); // Rect
*/

#endif /* AIPUTILS_H */
