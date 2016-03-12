/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SIMONET_H
#define SIMONET_H

#include "ficl.h"

extern void simon_hook_vm_init(void (*hook)(ficlVm * vm));
extern ficlVm* simon_vm_alloc();
extern void simon_vm_destroy(ficlVm* vm);
extern void simon_vm_init(ficlVm* vm, int filedes);
#endif /* SIMONET_H */
