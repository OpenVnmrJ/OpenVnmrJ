/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID


#else

#ifndef STACK_H_DEFINED
#define STACK_H_DEFINED

typedef void *Stack;

#ifdef __STDC__
Stack Stack_init (unsigned count, unsigned size);
Stack Stack_adjust (Stack stack, unsigned count);
int   Stack_push (Stack stack, void *entry);
int   Stack_pop (Stack stack, void *entry);
int   Stack_top (Stack stack, void *entry);
#else
Stack Stack_init( /* unsigned count, unsigned size */ );
Stack Stack_adjust( /* Stack stack, unsigned count */ );
int   Stack_push( /* Stack stack, void *entry */ );
int   Stack_pop( /* Stack stack, void *entry */ );
int   Stack_top( /* Stack stack, void *entry */ );
#endif

#endif

#endif
