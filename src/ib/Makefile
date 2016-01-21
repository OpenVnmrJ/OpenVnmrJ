# Copyright (c) Varian Assoc., Inc.  All Rights Reserved.

CC=gcc
TARGET_ARCH=
CFLAGS=-Wall -ansi -c -DANSI_C
COMPILE.c=$(CC) $(CFLAGS) $(CPPFLAGS)
DEBUG=-DDEBUG
DEBUG_ALLOC=-DDEBUG_ALLOC
CLOCKTIME=-DCLOCKTIME

PARAMS_LIB=libparams.so.1.1
DEBUG_LIB=libparams_dbg.so.1.1

.c.o:
	$(COMPILE.c) $(DEBUG) $(DEBUG_ALLOC) $(CLOCKTIME) $<

.PRECIOUS: $(PARAMS_LIB)

#------------------------- Macros for various programs ------------------------

TPT_SRCS=tp.c params.c tree.c stack.c getreply.c nfgets.c error.c debug_alloc.c
TPT_OBJS=$(TPT_SRCS:.c=.o)

TPH_SRCS=tp.c params.c hash.c getprime.c getreply.c nfgets.c error.c \
         debug_alloc.c
TPH_OBJS=$(TPH_SRCS:.c=.o)

SORT_SRCS=sortparms.c params.c tree.c stack.c error.c debug_alloc.c
SORT_OBJS=$(SORT_SRCS:.c=.o)

TREE_SRCS=testtree.c tree.c stack.c getreply.c nfgets.c error.c debug_alloc.c
TREE_OBJS=$(TREE_SRCS:.c=.o)

HASH_SRCS=testhash.c hash.c getprime.c getreply.c nfgets.c error.c debug_alloc.c
HASH_OBJS=$(HASH_SRCS:.c=.o)

LIB_SRCS=params.c tree.c stack.c error.c
LIB_OBJS=$(LIB_SRCS:.c=.o)

FORCE:
 
#------------------------- Tool programs --------------------------------------

tpt: $(TPT_OBJS)
	gcc -o $(@) $(TPT_OBJS)
 
tph: $(TPH_OBJS)
	gcc -o $(@) $(TPH_OBJS) -lm

tptt: $(TPT_OBJS) clocktime.o
	gcc -o $(@) $(TPT_OBJS) clocktime.o -lm
 
tpht: $(TPH_OBJS) clocktime.o
	gcc -o $(@) $(TPH_OBJS) clocktime.o -lm
 
tp.o: tp.c tp.h params.h boolean.h error.h debug_alloc.h clocktime.h

params.o: params.c params.h p_struct.inc p_static.inc storage.h \
          boolean.h error.h debug_alloc.h

sortparms: $(SORT_OBJS)
	gcc -o $(@) $(SORT_OBJS)

sortparms.o: sortparms.c params.h boolean.h error.h debug_alloc.h

#--- for testing AVL tree functions

testtree: $(TREE_OBJS)
	gcc -o $(@) $(TREE_OBJS)

testtree.o: testtree.c storage.h tree.h boolean.h error.h
 
tree.o: tree.c storage.h tree.h stack.h error.h

#--- for testing hash table functions
 
testhash: $(HASH_OBJS)
	gcc -o $(@) $(HASH_OBJS) -lm
 
testhash.o: testhash.c storage.h boolean.h error.h
 
hash.o: hash.c storage.h getprime.h error.h
 
#------------------------- Utility modules ------------------------------------

stack.o: stack.c stack.h error.h debug_alloc.h
 
getreply.o: getreply.c boolean.h error.h

getprime.o: getprime.c getprime.h boolean.h error.h

nfgets.o: nfgets.c

clocktime.o: clocktime.c clocktime.h error.h

error.o: error.c error.h

debug_alloc.o: debug_alloc.c debug_alloc.h boolean.h error.h

#------------------------- Portability check ----------------------------------

cc_tpt: FORCE
	cc -o tpt $(DEBUG_ALLOC) $(TPT_SRCS)
#rm $(TPT_OBJS)

cc_tph: FORCE
	cc -o tph $(DEBUG_ALLOC) $(TPH_SRCS) -lm
	rm $(TPH_OBJS)

cc_sortparms: FORCE
	cc -o sortparms $(DEBUG_ALLOC) $(SORT_SRCS)
	rm $(SORT_OBJS)

#------------------------- library build --------------------------------------

$(PARAMS_LIB): FORCE
	cc -pic -O -c $(LIB_SRCS)
	ld -assert pure-text $(LIB_OBJS) -o $@
	rm $(LIB_OBJS)

$(DEBUG_LIB): FORCE
	cc -pic -g -c $(DEBUG) $(DEBUG_ALLOC) $(LIB_SRCS) debug_alloc.c
	ld -lolgx -assert pure-text $(LIB_OBJS) debug_alloc.o -o $@
	rm $(LIB_OBJS) debug_alloc.o

libparams.a: FORCE
	cc -O -c $(LIB_SRCS)
	ar rv $@ $(LIB_OBJS)
	ranlib $@
	rm $(LIB_OBJS)

libparams_dbg.a: FORCE
	cc -g -c $(DEBUG) $(DEBUG_ALLOC) $(LIB_SRCS) debug_alloc.c
	ar rv $@ $(LIB_OBJS) debug_alloc.o
	ranlib $@
	rm $(LIB_OBJS)

#------------------------- lint build -----------------------------------------
lint_tpt:
	lint -u $(DEBUG) $(TPT_SRCS)

lint_tph:
	lint -u $(DEBUG) $(TPH_SRCS)

lint_tree:
	lint -u $(DEBUG) $(TREE_SRCS)
 
lint_hash:
	lint -u $(DEBUG) $(HASH_SRCS)
