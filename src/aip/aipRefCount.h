/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPREFCOUNT_H
#define AIPREFCOUNT_H

/*
 * The refCount needs to be kept updated to the number of pointers to
 * the structure.  On creation, it is set to 1.  Each time a pointer
 * is assigned to it refCount is incremented.  When a pointer gets
 * cleared refCount is decremented; if the result is 0, the referenced
 * instance of the class is deleted.
 */
class RefCount {
  public:
    int refCount;

    RefCount() {
	refCount = 1;
    }
};

#define refDup(dst, src)		\
    if (src)				\
	(src)->refCount++;		\
    (dst) = (src)

#define refClr(dst)				\
    if ((dst) && --((dst)->refCount) == 0)	\
        delete (dst);				\
    (dst) = NULL

#endif /* AIPREFCOUNT_H */

