/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <sys/types.h>
#include <dlfcn.h>
/*
* SOlaris does not provide with a static library of dlxxx routines this they hand dynamic loading
* however to build a staticly link app these my be referenced but never called,
* since we're attempting to build a statically linked
* binary, the varous dl*() routines should never be called in the
* first place.  This means we need own set of stub routines and
* link them into our binary.  
*     Greg Brissey   8/21/06
*/
void *dlopen(const char *str, int x) {}
void *dlsym(void *ptr, const char *str) {}
int dlclose(void *ptr) {}
char *dlerror() {}
void *dlmopen(Lmid_t a, const char *str, int x) {}
int dladdr(void *ptr1, Dl_info *ptr2) {} 
int dldump(const char *str1, const char *str2, int x) {}
int dlinfo(void *ptr1, int x, void *ptr2) {}
void *_dlopen(const char *str, int x) {}
void *_dlsym(void *ptr, const char *str) {}
int _dlclose(void *ptr) {}
char *_dlerror() {}
void *_dlmopen(Lmid_t a, const char *str, int x) {}
int _dladdr(void *ptr1, Dl_info *ptr2) {} 
int _dldump(const char *str1, const char *str2, int x) {}
int _dlinfo(void *ptr1, int x, void *ptr2) {}

