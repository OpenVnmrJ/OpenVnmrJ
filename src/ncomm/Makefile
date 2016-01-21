#################
#
# MODULE    $Id: Makefile,v 1.1 2010-06-28 16:00:53 dean Exp $
#
# DESCRIPTION   Makefile for nvpsg
# Notes:
# A. Changes needed for 64-bit linux
#   Makefiles
#    1. add -m32 to CFLAGS
#    2. add -m32 to LFLAGS
#    3. CC  = gcc
#    4. CCC = g++
#    5. LD  = g++
# B. sgl library changes
#    1. moved most functions in sgl.c to sglCommon.c (new)
#    2. sgl.c now contains only pulse-sequence globals and
#       parameter initialization functions
#    3. sgl.c is not built into the nvpsg library but
#       is compiled in with the user sequence using: 
#       include "sgl.c"
#################
# Notes
#################
# I. Applicability
#   1. this Makefile is targeted for Linux or Windows (SFU)
# II. Windows setup
#    A. Eclipse setup on Windows for SFU
#      1. define the following Eclipse environment variables
#        - MAKEDIR:  C:/SFU/bin/
#        - MAKEPATH: C:/SFU/bin/;C:/SFU/usr/bin;C:/SFU/opt/gcc.3.3/bin;
#      2. Project->properties->C/C++->Build 
#       o (top level treenode)
#        - In "Build settings" folder
#            - uncheck "Use default build command"
#            - in Build Command field enter :
#              "${MAKEDIR}make " (no quotes, space at end)
#        - In "Behavior" folder - Workbench build behavior box:
#            - Change the Build target from "all" to "build"
#       o  "Discovery options"
#        - uncheck "Automatic discovery of paths and symbols"
#       o  "Environment"
#        - add new variable "PATH" 
#        - set value to: ${MAKEPATH}
#       o  "Setting"
#        - choose "PE Windows Parser" 
#      3. Project properties -> C/C++ General 
#       o "Paths and Symbols - Includes"
#        - delete all paths for Assembly, GNU C and GNU C++
#################
# History
#################

OSTYPE      = `uname -s`
MAKEFILE    = Makefile
FILES       =
DIR         =
SFX         = .c
SHELL       = /bin/sh
VNMR        = /vnmr
#################
SFU_SYSROOT = $(INTERIX_ROOT)
SFU_GCCDIR  = $(INTERIX_ROOT)/opt/gcc.3.3/bin/
LNX_SYSROOT = 
LNX_GCCDIR  = 
GCCDIR      = 
SYSROOT     = 

NVFLAGS     = -Dcodeint=int -Dcodelong=int
CDEFINE     = -DULONG -D_ALL_SOURCE -DUSE_HTONS -DSUN -DLINUX -DNESSIE -DNO_WIN_MMAP
M32         = -m32

INCS        = \
            -I.,/ncomm \
			-I../vnmr \
			-I../expproc \
			-I../vwacq \
			-I../psg

INCLUDES    = -I./ -I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/X11R6/include $(INCS)

CFLAGS      = -O $(CDEFINE) -w -Wall $(INCLUDES) $(M32)

LDOPTIONS   = -O
LDFLAGS     = $(M32)

CC          = $(GCCDIR)gcc
CCC			= $(GCCDIR)g++
LD          = $(GCCDIR)g++

SFU_LIBS    = -lstdc++ -lsocket -lm $(PSGLIBS)
LNX_LIBS    = -lm -lnsl -lpthread $(PSGLIBS)

NCOMM_LIBS  = libncomm.a libacqcomm.a

COMPILE.c   = $(CC)  $(CFLAGS) $(NVFLAGS) -c
COMPILE.cc  = $(CCC) $(CFLAGS) $(CPPDEFINE) -c
LINK.cc     = $(CCC) $(LDFLAGS)
LINK.c      = $(CC) $(LDFLAGS) 

#################
# ncomm files 
#################
NCOMM_HDR=  \
         errLogLib.h	\
         semLib.h	\
         sockets.h	\
         shrMLib.h	\
         mfileObj.h
 
NCOMM_SRC=			\
		 mfileObj.c \
         errLogLib.c \
         hhashLib.c \
         ipcKeyDbm.c \
         ipcMsgQLib.c \
         semLib.c \
         msgQLib.c \
         asyncIo.c \
         eventHandler.c \
         eventQueue.c \
         listObj.c \
         sendAsync.c \
         shrMLib.c \
		 chanLib.c \
         sockets.c \
         signals.c \
         fdmask.c

NCOMM_OBJ= $(NCOMM_SRC:.c=.o)
NCOMM_FILES = $(NCOMM_HDR:%.h=../ncomm/%.h) $(NCOMM_SRC:%.c=../ncomm/%.c)

#################	    
# expproc files
#################
EXPPROC_HDR=        \
		expQfuncs.h \
        hostMsgChannels.h \
        shrstatinfo.h \
        shrexpinfo.h

EXPPROC_SRC=               \
		expQfuncs.c \
        statfuncs.c

EXPPROC_OBJ= $(EXPPROC_SRC:.c=.o)
EXPPROC_FILES = $(EXPPROC_HDR:%.h=../expproc/%.h) $(EXPPROC_SRC:%.c=../expproc/%.c)
#################	    
# vnmr files
#################
VNMR_HDR=        \
		acquisition.h \
        comm.h \
        variables.h
VNMR_SRC=               \
		comm.c \
        socket.c \
        socket1.c

VNMR_OBJ= $(VNMR_SRC:.c=.o)
VNMR_FILES = $(VNMR_HDR:%.h=../vnmr/%.h) $(VNMR_SRC:%.c=../vnmr/%.c)
#################
# psg files
#################
PSG_HDR=                \

PSG_SRC=                \
        convertapbout.c
PSG_OBJ= $(PSG_SRC:.c=.o)
PSG_FILES = $(PSG_HDR:%.h=../psg/%.h) $(PSG_SRC:%.c=../psg/%.c)

#################
# vwacq files
#################
VWACQ_HDR=                \
		hostAcqStructs.h
VWACQ_FILES = $(VWACQ_HDR:%.h=../vwacq/%.h) $(VWACQ_SRC:%.c=../vwacq/%.c)

#################
ALL_OBJS  = $(NCOMM_OBJ) $(EXPPROC_OBJ) $(PSG_OBJ) $(VNMR_OBJ)
ALL_FILES = \
       $(PSG_FILES) \
       $(VNMR_FILES) \
       $(EXPPROC_FILES) \
       $(VWACQ_FILES) \
       $(NCOMM_FILES) \

#################
# build targets
#################
all: build install
build:;	@echo "+++++++++++++++  $@ --------------------------"
	if(test $(OSTYPE) = Linux); then \
		$(MAKE) -f $(MAKEFILE) TARGET=build_libs lnx; \
	elif(test $(OSTYPE) = Interix); then \
		$(MAKE) -f $(MAKEFILE) TARGET=build_libs sfu; \
	fi;

lnx:;	@echo "+++++++++++++++  $@ --------------------------"
	@$(MAKE) -f $(MAKEFILE) \
	SYSROOT="$(LNX_SYSROOT)" \
	GCCDIR="$(LNX_GCCDIR)" \
	$(TARGET);
	@echo "DONE target $@ --------------------------"

sfu:;	@echo "+++++++++++++++  $@ --------------------------"
	@$(MAKE) -f $(MAKEFILE) \
	SYSROOT="$(SFU_SYSROOT)" \
	GCCDIR="$(SFU_GCCDIR)" \
	$(TARGET);
	@echo "DONE target $@ --------------------------"

#build_libs: libncomm.a libncomm.so libacqcomm.a libacqcomm.so
build_libs: $(NCOMM_LIBS)
	@echo "+++++++++ DONE $@ +++++++";

#--- Object Targets ---------------------------
#
$(NCOMM_OBJ) : $(@:%.o=../ncomm/%.c)
	$(COMPILE.c) $(@:%.o=../ncomm/%.c);

$(PSG_OBJ) : $(@:%.o=../psg/%.c)
	$(COMPILE.c) $(@:%.o=../psg/%.c);

$(EXPPROC_OBJ) : $(@:%.o=../expproc/%.c)
	$(COMPILE.c) $(@:%.o=../expproc/%.c);

$(VNMR_OBJ) : $(@:%.o=../vnmr/%.c)
	$(COMPILE.c) $(@:%.o=../vnmr/%.c);

libncomm.a: $(NCOMM_OBJ)
	@rm -f $@; \
	ar rv $@ $(NCOMM_OBJ);

libacqcomm.a: $(ALL_OBJS)
	@rm -f $@;
	ar rv $@ $(ALL_OBJS);

install:;
	@if(test -d $(VNMR)); then \
		if(test ! -d $(VNMR)/lib); then \
			mkdir $(VNMR)/lib; \
		fi;\
		if(test -f libncomm.a); then \
			echo "installing ncomm libraries in $(VNMR)/lib"; \
			cp $(NCOMM_LIBS) $(VNMR)/lib; \
		else \
			echo "could not install ncomm libraries <not present in working directory>"; \
		fi; \
	else \
		echo "could not install ncomm libraries <vnmr directory not accessable> ";\
	fi; \
	echo "DONE target $@ --------------------------"

#################
clean:;     @rm -fr *.o *.so* *.a *~
showenv:;	@echo "+++++++++++++++  $@ --------------------------"
	@if(test $(OSTYPE) = Linux); then \
		$(MAKE) -f $(MAKEFILE) TARGET=_showenv lnx; \
	elif(test $(OSTYPE) = Interix); then \
		$(MAKE) -f $(MAKEFILE) TARGET=_showenv sfu; \
	fi;
	@echo "DONE target $@ --------------------------"

_showenv:;
	-@(echo OSTYPE="$(OSTYPE)"; )
	-@(echo SYSROOT="$(SYSROOT)"; )
	-@(echo GCCDIR="$(GCCDIR)"; )
	-@(echo INCLUDES="$(INCLUDES)"; )
	-@(echo LIBS="$(LIBS)"; )
	-@(echo DEFINES="$(DEFINES)"; )
	-@(echo PATH="$(PATH)"; )

############################################################
# dependencies
############################################################
# AUTO-DEPENDENCY GENERATION using target 'make depend' ###
# 1. runs "makedepend" for source files in current directory.
#    - removes Makefile text following "DO NOT DELETE .." line
#    - sets dependencies for local ".h" files (e.g. file.o: file.h)
#    - sets dependencies for remote ".h" files (e.g. file.o: ../dir/file.h)
# 2. runs "makedepend" for source files in remote directories
#    - generates dependencies like ../dir/file.o: ../dir/file.h
#    - passes output through awk script to strip off ../dir/ in ".o" strings so that
#      object files are specified in current directory (e.g. file.o vs. ../dir/file.o)
#    - appends remote dependencies output from (1) 
# 3. runs "make list_depends" to generate explicit "object:source" dependencies for all files
#    - needed on Linux because "implicit" dependencies (e.g. file.o:file.c)
#      don't seem to always work
#    - appends "make list_depends" dependencies to output from (2)
#################
list_depends:;  @for file in $(FILES); do \
		echo "$$file.o: $(DIR)$$file$(SFX)"; \
	done

dirlist:; echo "#========== $(DIR) ======== " ;\
        makedepend  -Y -ftmp $(INCS)  $(SRC)  2>/dev/null; \
        awk '/^\.\.\/$(DIR)\// {printf("%s\n", substr($$0,length("$(DIR)")+5));next;} /\#/ {next;} /./ {print;}' tmp; \
        make -s FILES="$(FILES)" DIR="../$(DIR)/" list_depends | sort; \

depend:; @rm -f lists; \
	    touch tmp; \
        makedepend -Y./ $(INCS) $(NCOMM_SRC)2>/dev/null; \
        make -s FILES="$(PSG_SRC:%.c=%)" SRC="$(PSG_SRC:%.c=../psg/%.c)" DIR="psg" dirlist >>lists; \
        make -s FILES="$(EXPPROC_SRC:%.c=%)" SRC="$(EXPPROC_SRC:%.c=../expproc/%.c)" DIR="expproc" dirlist >>lists; \
        make -s FILES="$(VNMR_SRC:%.c=%)" SRC="$(VNMR_SRC:%.c=../vnmr/%.c)" DIR="vnmr" dirlist >>lists; \
        cat lists >> $(MAKEFILE); \
        rm lists tmp;
        
# DO NOT DELETE THIS LINE -- make depend depends on it.

mfileObj.o: errLogLib.h mfileObj.h
hhashLib.o: errLogLib.h hhashLib.h
ipcKeyDbm.o: errLogLib.h shrMLib.h mfileObj.h ipcKeyDbm.h
ipcMsgQLib.o: errLogLib.h ipcMsgQLib.h
semLib.o: errLogLib.h semLib.h mfileObj.h
msgQLib.o: errLogLib.h ../expproc/hostMsgChannels.h chanLib.h eventHandler.h
msgQLib.o: mfileObj.h msgQLib.h ipcMsgQLib.h shrMLib.h
asyncIo.o: eventHandler.h errLogLib.h
eventHandler.o: errLogLib.h listObj.h eventQueue.h eventHandler.h
eventQueue.o: eventQueueP.h eventQueue.h
listObj.o: listObj.h
sendAsync.o: errLogLib.h sockets.h
shrMLib.o: errLogLib.h shrMLib.h mfileObj.h semLib.h
chanLib.o: errLogLib.h chanLibP.h sockets.h chanLib.h listObj.h
chanLib.o: eventHandler.h
sockets.o: errLogLib.h sockets.h
#========== psg ======== 
convertapbout.o: ../psg/convertapbout.c
#========== expproc ======== 
expQfuncs.o: errLogLib.h mfileObj.h shrMLib.h
expQfuncs.o: ../expproc/shrexpinfo.h ../expproc/expQfuncs.h
statfuncs.o: errLogLib.h ../vwacq/hostAcqStructs.h mfileObj.h
statfuncs.o: shrMLib.h ../expproc/shrstatinfo.h
expQfuncs.o: ../expproc/expQfuncs.c
statfuncs.o: ../expproc/statfuncs.c
#========== vnmr ======== 
comm.o: ../vnmr/comm.h ../vnmr/acquisition.h errLogLib.h
socket.o: mfileObj.h ../vnmr/acquisition.h ../vnmr/comm.h
socket.o: ../vnmr/variables.h
socket1.o: ../vnmr/acquisition.h ../vnmr/comm.h
comm.o: ../vnmr/comm.c
socket.o: ../vnmr/socket.c
socket1.o: ../vnmr/socket1.c
