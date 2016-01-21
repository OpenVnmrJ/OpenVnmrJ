################
# Notes
#################
# I. Applicability
#   1. this Makefile is targeted for Linux or Windows (SFU)
# II. Linux setup
#   1. In .cshrc set LD_LIBRARY_PATH to /vnmr/lib. 
#
#   note: failure to set LD_LIBRARY_PATH results in "cannot connect
#         vnmrbg to error output"  and vnmrbg doesn't start.
#
# III. Windows setup
#    A. SFU <inttypes.h>
#      1. In sfu, inttypes.h does not exist (despite the fact that this
#         file was added to the "must support" list for ansi c in 1999)
#        - As a workaround, copy the "sfutypes.h" file in git-repo/scripts 
#          to C:/SFU/usr/include/inttypes.h
#    B. Eclipse setup on Windows for SFU
#      1. define the following Windows environment variables
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
#        - set value to: ${MAKEPATH}${PATH}
#       o  "Setting"
#        - choose "PE Windows Parser" 
#      3. Project properties -> C/C++ General 
#       o "Paths and Symbols - Includes"
#        - delete all paths for Assembly, GNU C and GNU C++
#
#################
OSTYPE      = `uname -s`
THIS        = vnmrbg
FILES       =
DIR         =
SFX         = .c
SHOW        = y
SHELL       = /bin/sh
MAKEFILE    = Makefile
#VNMR        = ../../../vnmr
VNMR        = /vnmr
#################
#SFU_SYSROOT = /dev/fs/C/SFU
#SFU_GCCDIR  = /dev/fs/C/SFU/opt/gcc.3.3/bin/
SFU_SYSROOT = $(INTERIX_ROOT)
SFU_GCCDIR  = $(INTERIX_ROOT)/opt/gcc.3.3/bin/
LNX_SYSROOT = 
LNX_GCCDIR  = 
GCCDIR      = 
SYSROOT     = 
#################
SHELL		=  /bin/sh
CC		=  $(GCCDIR)gcc
CCC		=  $(GCCDIR)g++

REV_DATE	=  $(VNMR_REV_DATE)
COMPDATE:sh	=  date '+Compiled: %m/%d/%y %H:%M'
M32         = -m32
CFLAGS 		= -w -g $(M32)

INCS        = \
			-I../vnmr \
			-I../3D \
			-I../psg \
			-I../acqproc \
			-I../ncomm \
			-I../expproc \
			-I../nacqi \
			-I../xracq \
			-I../vwacq \
			-I../vnmrbg \
			-I../magic \
			-I../aip \
			-I../ddl \
			-I../xwin

CCFLAGS  	= $(CFLAGS) 

INCLUDES    = -I./ -I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/X11R6/include $(INCS)

SFU_DEFINES  = -DLINUX -DVNMRJ -D_ALL_SOURCE -DUSE_HTONS -DBSDACQ -DNESSIE -DSUN -DX11 -DNOMOTIF -DWINBRIDGE -DNO_WIN_MMAP
#SFU_DEFINES  = -DLINUX -DVNMRJ -D_ALL_SOURCE -DUSE_HTONS -DBSDACQ -DNESSIE -DSUN -DX11 -DNOMOTIF -DWINBRIDGE
LNX_DEFINES  = -DLINUX -DVNMRJ -D_ALL_SOURCE -DUSE_HTONS -DBSDACQ -DNESSIE -DSUN -DX11 -DNOMOTIF

COMPILE.c	= $(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c
COMPILE.cc	= $(CCC) $(CCFLAGS) $(INCLUDES) $(DEFINES) -c

SFU_LIBS	= -ldl -lm -lX11 -lXt -lXp -lXext -lSM -lICE -ll -lsocket 
LNX_LIBS	= -ldl -lm -lX11 -lXt -lnsl

SFU_LFLAGS	= $(M32) -O -L$(SYSROOT)/usr/X11R6/lib -L.
LNX_LFLAGS	= $(M32) -O -rdynamic -L$(SYSROOT)/usr/X11R6/lib -L.

# Set default to Linux build

LFLAGS		= $(LNX_LFLAGS)
LIBS		= $(LNX_LIBS)
DEFINES	    = $(LNX_DEFINES)

LINK.c      = $(CCC) $(LFLAGS)

#------------------------------------------------------------------------
# vnmrbg files
#------------------------------------------------------------------------
UNMRBG_SRC_COM  = \
		dcon.c 		\
		dconi.c 	\
		decc.c		\
		dfid.c		\
		dpcon.c 	\
		fdatap.c    \
		iplan.c		\
		iplan_graphics.c	\
		iplan_params.c \
		locki.c		\
		main.c		\
		master.c	\
        pipeRead.c \
        smagic.c 	\
		crosshair.c 	\
		mouseHandler.c 	\
		frameManager.c 	\
		wjunk.c

UNMR_BG_HDR  = \
		vjXdef.h

WIN_BG_SRC  =	\
		graphics_win.c	\
		master_win.c    \
		smagic_win.c

VNMRBG_SRC = $(UNMRBG_SRC_COM) $(WIN_BG_SRC)
VNMRBG_OBJ = $(VNMRBG_SRC:.c=.o)

VNMRBG_CC_SRC  = graphics3D.C
VNMRBG_CC_OBJ = $(VNMRBG_CC_SRC:.C=.o)
#------------------------------------------------------------------------
# expproc files
#------------------------------------------------------------------------
EXPPROC_SRC  =	\
		expQfuncs.c	\
		statfuncs.c

EXPPROC_OBJ  = $(EXPPROC_SRC:.c=.o)

EXPPROC_HDR  =	\
		expQfuncs.h	\
		shrexpinfo.h	\
		hostMsgChannels.h	\
		shrstatinfo.h

#------------------------------------------------------------------------
# vwacq files
#------------------------------------------------------------------------
VWACQ_HDR  = \
	acqcmds.h	\
	hostAcqStructs.h

#------------------------------------------------------------------------
# nacqi files
#------------------------------------------------------------------------
NACQI_HDR  = acqInterface.h
NACQI_SRC  = msgqueue.c
NACQI_OBJ  = $(NACQI_SRC:.c=.o)

#------------------------------------------------------------------------
# vnmr files
#------------------------------------------------------------------------
UNMRLIB_SRC_COM    = \
		acqfuncs.c 	\
		acqhwcmd.c 	\
		addsub.c 	\
		ai.c 		\
		allocate.c 	\
		asm.c		\
		asmfuncs.c	\
		assign.c	\
		bgvars.c 	\
		bootup.c 	\
		builtin.c 	\
		builtin1.c 	\
		buttons.c	\
		calcq.c		\
		continprepare.c \
		continread.c \
		createparams.c	\
		data.c 		\
		ddf.c 		\
		ddif.c 		\
		ddph.c 		\
		debug.c 	\
		df2d.c		\
		dfww.c 		\
		dg.c 		\
		disp3Dmap.c     \
		dll.c		\
		dosyfit.c	\
		dosyfit3D.c     \
		dpf.c		\
		dps.c		\
		ds.c		\
		dscale.c	\
		dsn.c		\
		dsp.c		\
		dsww.c 		\
		element.c 	\
		emouse.c 	\
		execcmd.c 	\
		fiddle.c	\
		files.c 	\
		flashc.c	\
		ft.c		\
		ft2d.c 		\
		ftinit.c	\
		full.c 		\
		gdevsw.c	\
		glide.c 	\
		go.c 		\
		gradfit.c \
		graphics.c 	\
		graphics_util.c \
		gxyzcalib.c \
		gxyzcalib2.c \
		gxyzmap.c \
		gzfit.c 	\
		handler.c 	\
		help.c 		\
		history.c 	\
		hpa.c 		\
		ht.c 		\
		init2d.c 	\
		init_display.c 	\
		init_proc.c 	\
		integ.c 	\
		io.c 		\
		jexp.c 		\
		ll2d.c 		\
		lockfreqfunc.c	\
		locksys.c	\
		lookup.c 	\
		lstring.c 	\
		macro.c 	\
		magnetom.c	\
		makeslice.c	\
		md5c.c		\
		mfdata.c	\
		ops.c 		\
		p11Tools.c	\
		part11.c	\
		pcmap.c		\
		phase_fid.c \
		plot_handlers.c	\
		plot_construct.c \
		proc2d.c 	\
		pvars.c		\
		recon_all.c	\
		recon3D.c \
		recon_mm.c	\
		readlk.c 	\
		rtvarcmds.c	\
		savretphf.c	\
		set3dproc.c	\
		shellcmds.c	\
		shims.c		\
		sky.c		\
		specfreq.c	\
		splmodprepare.c \
		splmodread.c \
		sread.c 	\
		swrite.c 	\
		symtab.c 	\
		table.c 	\
		tempstuff.c 	\
		terminal.c 	\
		text.c 		\
		tools.c 	\
		variables1.c 	\
		vcolor.c 	\
		vdialog.c 	\
		vfilesys.c 	\
		weight.c	\
		annotate.c	\
		wti.c \
		acqhdl.c \
                displayops.c    \
                foldj.c

# -- special compile options required
UNMR_SPC_SRC  =  \
#		acqhdl.c \
#                displayops.c    \
#                foldj.c

UNMR_COM_OBJ  = $(UNMRLIB_SRC_COM:.c=.o)
UNMR_BG_OBJ   = $(UNMRBG_SRC_COM:.c=.o)
UNMR_SPC_OBJ  = $(UNMR_SPC_SRC:.c=.o)

UNMRLIB_HDR      = \
		acqerrmsges.h   \
		acquisition.h   \
		allocate.h      \
		asm.h		\
		bgvars.h \
		buttons.h \
		data.h \
		disp.h          \
		displayops.h \
		dps_menu.icon   \
		dpsdef.h        \
		dscale.h \
		element.h       \
		epi_recon.h     \
		errorcodes.h    \
		md5global.h     \
		md5.h		\
		part11.h        \
		fft.h \
		ftpar.h         \
		graphics.h      \
		graphics_util.h \
		group.h         \
		init.h          \
		init_display.h \
		init_proc.h \
		init2d.h        \
		iplan.h         \
		iplan_graphics.h  \
		locksys.h       \
		node.h          \
		params.h        \
		phase_correct.h \
		process.h       \
		pvars.h         \
		shims.h         \
		sky.h         \
		sockinfo.h \
		stack.h         \
		symtab.h        \
		tools.h         \
		variables.h     \
		vfilesys.h      \
		vnmrsys.h       \
		whenmask.h      \
		wjunk.h

ACQCOM_HDR  =	comm.h

ACQCOM_SRC  =	socket1.c	\
		comm.c          \
		socket.c        

ACQCOM_OBJ  =   $(ACQCOM_SRC:.c=.o)

MAGIC_COM_SRC  = \
		exec.c	\
		dli.c	\
		fft.c 	\
		lexjunk.c	\
		lpz.c	\
		matrix.c 	\
		node.c

MAGIC_SPC_SRC  = \
		downsample.c	\
		lpqld.c

MAGIC_COM_OBJ  = $(MAGIC_COM_SRC:.c=.o)
MAGIC_SPC_OBJ  = $(MAGIC_SPC_SRC:.c=.o)

VNMR_SRC= \
	$(UNMRLIB_SRC_COM) \
	$(ACQCOM_OBJ) \
	$(MAGIC_COM_SRC) \
	$(MAGIC_SPC_SRC) \

VNMR_OBJ  = $(VNMR_SRC:.c=.o)

VNMR_HDR= \
	$(UNMRLIB_HDR) \
	$(ACQCOM_HDR) \
	
VNMR_FILES = $(VNMR_SRC) $(VNMR_HDR) 

#------------------------------------------------------------------------
# ddl files
#------------------------------------------------------------------------
DDL_HDR  = \
		ddlList.h		\
		ddlNode.h		\
		ddlParser.h		\
		ddlScanner.h		\
		ddlSymbol.h

DDL_SRC  = \
		ddlAction.C		\
		ddlNode.C		\
		ddlParser.C		\
		ddlScanner.C		\
		ddlSymbolTable.C

DDL_OBJ     = $(DDL_SRC:.C=.o)

YACC_DDL_SRC   = ddlGrammar.y
YACC_DDL_OBJ   = ddlGrammar.o

#------------------------------------------------------------------------
# aip files
#------------------------------------------------------------------------
AIP_GNU_HDR = generic.h  
AIP_HDR     = \
		aipBox.h		\
		aipCFuncs.h		\
		aipCInterface.h		\
		aipCommands.h		\
		aipCStructs.h		\
		aipDataInfo.h		\
		aipDataManager.h	\
		aipDataStruct.h		\
		aipEdgelist.h		\
		aipGframe.h		\
		aipGframeManager.h	\
		aipGraphics.h		\
		aipGraphicsWin.h	\
		aipHistogram.h		\
		aipImgInfo.h		\
		aipInitStart.h		\
		aipInterface.h		\
		aipInterpolation.h	\
		aipLine.h		\
		aipMouse.h		\
		aipOrthoSlices.h \
		aipOval.h		\
		aipParmList.h		\
		aipPoint.h		\
		aipPolygon.h		\
		aipPolyline.h		\
		aipRoi.h		\
		aipRoiManager.h		\
		aipRoiStat.h		\
		aipStderr.h		\
		aipStructs.h		\
		aipUtils.h		\
		aipViewInfo.h		\
		aipVnmrFuncs.h		\
		aipVolData.h		\
		aipVsInfo.h		\
		aipWinMath.h		\
		aipWinMovie.h		\
		aipMovie.h \
		aipWinRotation.h	\
		aipWriteData.h		\
		aipReviewQueue.h	\
		aipRQparser.h		\
		aipRQnode.h		\
		aipRQimage.h		\
		aipRQgroup.h		\
		aipRQstudy.h		\
		ddlCInterface.h		\
		sharedPtr.h

AIP_SRC      =	\
		aipBox.C		\
		aipCommands.C		\
		aipDataInfo.C		\
		aipDataManager.C	\
		aipEdgelist.C		\
		aipGframe.C		\
		aipGframeManager.C	\
		aipGraphicsWin.C	\
		aipHistogram.C		\
		aipImgInfo.C		\
		aipInitStart.C		\
		aipInterface.C		\
		aipInterpolation.C	\
		aipLine.C		\
		aipMouse.C		\
		aipOrthoSlices.C		\
		aipOval.C		\
		aipParmList.C		\
		aipPoint.C		\
		aipPolygon.C		\
		aipPolyline.C		\
		aipRoi.C		\
		aipRoiManager.C		\
		aipRoiStat.C		\
		aipStderr.C		\
		aipUtils.C		\
		aipViewInfo.C		\
		aipVnmrFuncs.C		\
		aipVolData.C		\
		aipVsInfo.C		\
		aipWinMath.C		\
		aipWinMovie.C		\
        aipMovie.C          \
		aipWinRotation.C	\
		aipWriteData.C		\
		aipReviewQueue.C \
		aipRQparser.C		\
		aipRQnode.C		\
		aipRQimage.C		\
		aipRQgroup.C		\
		aipRQstudy.C		\
		ddlCInterface.C

FORCE_DOT_O =	ddlCInterface.o

AIP_OBJ  =   $(AIP_SRC:.C=.o)
#------------------------------------------------------------------------
# xwin files
#------------------------------------------------------------------------
XWIN_COM_ICON  = \
		master.icon	\
		vnmr_text.icon  \
		vnmr.icon

XWIN_C_SRC   =	banner.c \
		ras_dump.c

XWIN_OBJ      = $(XWIN_C_SRC:.c=.o) 

XWIN_SRC     = $(XWIN_COM_ICON) $(XWIN_C_SRC)

#------------------------------------------------------------------------
# xracq files
#------------------------------------------------------------------------
XRACQ_HDR=			\
		ACQ_HAL.h	\
		ACQ_SUN.h	\
		STAT_DEFS.h	\
		acodes.h	

#------------------------------------------------------------------------
# psg files
#------------------------------------------------------------------------

PSG_SRC  = \
		ap_device.c 	\
		device.c 	\
		ecc_setup.c	\
		ecc_worstCase.c	\
		freq_device.c\
		freqfuncs.c	\
		objerror.c

PSG_OBJ  =$(PSG_SRC:.c=.o)

PSG_HDR  = \
		REV_NUMS.h	\
		Pbox_HT.h	\
		ap_device.p	\
		common.p	\
		device.p	\
		ecc.h	\
		freq_device.p\
		objerror.h	\
		oopc.h	\
		rfconst.h
#------------------------------------------------------------------------
# 3D files
#------------------------------------------------------------------------
FT3D_HDR  = \
		lock3D.h	\
		struct3d.h

FT3D_SRC  = \
		lock3Dfiles.c

FT3D_OBJ  = $(FT3D_SRC:.c=.o)

#------------------------------------------------------------------------
# ncomm files
#------------------------------------------------------------------------
NCOMM_HDR   =   \
                sockets.h       \
		chanLib.h       \
                mfileObj.h      \
		errLogLib.h     \
                hhashLib.h      \
                ipcKeyDbm.h     \
                ipcMsgQLib.h    \
                semLib.h        \
                msgQLib.h       \
		eventHandler.h  \
		eventQueue.h    \
		eventQueueP.h   \
		listObj.h       \
		sendAsync.h     \
                shrMLib.h

LNX_NCOMM_SRC  = \
		asyncIo.c       \
		errLogLib.c     \
		eventHandler.c  \
		eventQueue.c    \
		fdmask.c        \
		hhashLib.c      \
		ipcKeyDbm.c     \
		ipcMsgQLib.c    \
		listObj.c       \
		mfileObj.c      \
		msgQLib.c       \
		semLib.c        \
		sendAsync.c     \
		shrMLib.c       \
		sockets.c       \

LNX_NCOMM_OBJ  = $(LNX_NCOMM_SRC:.c=.o) 

SFU_NCOMM_SRC  = \
		asyncIo.c       \
		errLogLib.c     \
		eventHandler.c  \
		eventQueue.c    \
		fdmask.c        \
		hhashLib.c      \
		listObj.c       \
		mfileObj.c      \
		semLib.c        \
		sendAsync.c     \
		shrMLib.c       \
		sockets.c       \

SFU_NCOMM_OBJ  = $(SFU_NCOMM_SRC:.c=.o) 

NCOMM_SRC  = $(LNX_NCOMM_SRC)
NCOMM_OBJ  = $(NCOMM_SRC:.c=.o) 

#------------------------------------------------------------------------
# magic files
#------------------------------------------------------------------------
# NOTE: Using lex(flex) and yacc(bison) from Linux the magic parser
#       builds okay but hangs when envoked. The current nightly build
#       uses an older Solaris variant of lex and yacc which include arguments
#       that don't seem to be supported in the Linux version.
#       Until these problems are resolved this project will use the 
#       set of C files produced from the most current Solaris build and
#       copied into this directory and renamed (this requires manual
#       synchronization if changes are made to the magic source files).
# Original file names 
#MAGIC_GRAM_OBJ   =	magic.gram.o
#MAGIC_GRAM_C     =	magic.gram.c
#MAGIC_LEX_C      =	magic.lex.c
#MAGIC_LEX_OBJ    =	magic.lex.o

# New file names 
MAGIC_GRAM_C     =	magic_gram.c
MAGIC_LEX_C      =	magic_lex.c

MAGIC_SRC = $(MAGIC_GRAM_C) $(MAGIC_LEX_C) 
MAGIC_OBJ = $(MAGIC_SRC:.c=.o)  

#------------------------------------------------------------------------
#---- All Sources and headers required for compilation -----
#------------------------------------------------------------------------

ALL_OBJ  =  \
	$(PSG_OBJ) \
	$(UNMR_SPC_OBJ) \
	$(MAGIC_OBJ) \
	$(VNMRBG_OBJ) \
	$(VNMRBG_CC_OBJ) \
	$(NCOMM_OBJ) \
	$(VNMR_OBJ) \
	$(FT3D_OBJ) \
	$(EXPPROC_OBJ) \
	$(NACQI_OBJ) \
	$(AIP_OBJ) \
	$(DDL_OBJ) \
	$(YACC_DDL_OBJ) \
	$(XWIN_OBJ) \

############################################################
# BUILD Targets 
############################################################
all: build install
build:;	@echo "+++++++++++++++  $@ --------------------------"
	@if(test -f ../psg/vnmrsys.h); then \
	    mv ../psg/vnmrsys.h ../psg/vnmrsys2.h; \
	fi;

	@if(test $(OSTYPE) = Linux); then \
		$(MAKE) -f $(MAKEFILE) TARGET=Vnmrbg lnx; \
	elif(test $(OSTYPE) = Interix); then \
		$(MAKE) -f $(MAKEFILE) TARGET=Vnmrbg sfu; \
	fi;
	@if(test -f ../psg/vnmrsys2.h); then \
	    mv ../psg/vnmrsys2.h ../psg/vnmrsys.h; \
	fi;
	@echo "DONE target $@ --------------------------"

lnx:;	@echo "+++++++++++++++  $@ --------------------------"
	@$(MAKE) -f $(MAKEFILE) \
	LIBS="$(LNX_LIBS)" \
	DEFINES="$(LNX_DEFINES)" \
	LFLAGS="$(LNX_LFLAGS)" \
	NCOMM_SRC="$(LNX_NCOMM_SRC)" \
	SYSROOT="$(LNX_SYSROOT)" \
	GCCDIR="$(LNX_GCCDIR)" \
	$(TARGET);
	@echo "DONE target $@ --------------------------"

sfu:;	@echo "+++++++++++++++  $@ --------------------------"
	@$(MAKE) -f $(MAKEFILE) \
	LIBS="$(SFU_LIBS)" \
	DEFINES="$(SFU_DEFINES)" \
	LFLAGS="$(SFU_LFLAGS)" \
	NCOMM_SRC="$(SFU_NCOMM_SRC)" \
	SYSROOT="$(SFU_SYSROOT)" \
	GCCDIR="$(SFU_GCCDIR)" \
	$(TARGET);
	@echo "DONE target $@ --------------------------"

Vnmrbg:  revdate.o $(ALL_OBJ) 
	(umask 2; $(LINK.c) -o $@  revdate.o  $(ALL_OBJ) $(LIBS) )

#--- Object Targets ---------------------------
#
$(MAGIC_OBJ) : $(@:%.o=../magic/%.c)
	$(COMPILE.c) $(@:%.o=../magic/%.c);
$(NCOMM_OBJ) : $(@:%.o=../ncomm/%.c)
	$(COMPILE.c) $(@:%.o=../ncomm/%.c);
$(FT3D_OBJ) : $(@:%.o=../3D/%.c)
	$(COMPILE.c) $(@:%.o=../3D/%.c);
$(EXPPROC_OBJ) : $(@:%.o=../expproc/%.c)
	$(COMPILE.c) $(@:%.o=../expproc/%.c);
$(PSG_OBJ) : $(@:%.o=../psg/%.c)
	$(COMPILE.c) $(@:%.o=../psg/%.c);
$(VNMR_OBJ) : $(@:%.o=../vnmr/%.c)
	$(COMPILE.c) $(@:%.o=../vnmr/%.c);
$(XWIN_OBJ) : $(@:%.o=../xwin/%.c)
	$(COMPILE.c) $(@:%.o=../xwin/%.c);
$(NACQI_OBJ) : $(@:%.o=../nacqi/%.c)
	$(COMPILE.c) $(@:%.o=../nacqi/%.c);

$(DDL_OBJ) : $(@:%.o=../ddl/%.C)
	$(COMPILE.cc) $(@:%.o=../ddl/%.C);

$(YACC_DDL_OBJ) : $(@:%.o=%.C)
	$(COMPILE.cc) $(@:%.o=%.C);

$(AIP_OBJ) : $(@:%.o=../aip/%.C)
	$(COMPILE.cc) $(@:%.o=../aip/%.C);

$(VNMRBG_OBJ) : $(@:%.o=%.c)
	$(COMPILE.c) $(@:%.o=%.c);

$(VNMRBG_CC_OBJ) : $(@:%.o=%.C)
	$(COMPILE.cc) $(@:%.o=%.C);

#--- Special Object Targets ----------------------
# revdate.o 
# This object is always compiled, VNMR_REV_ID & VNMR_REV_DATE are environment
# variables and COMPDATE is a makefile define passed to the makefile via a recusive
# make call or from vmake itself.
# REVISION_ID typically - "VERSION 4.2 REVISION A"
# REVISION_DATE typically - "Aug. 17, 1993"
# COMPDATE typically - "Compiled: 8/17/92 15:45"
#
revdate.o:;
	(umask 2; $(COMPILE.c) -DREVISION_ID='"$(VNMR_REV_ID)"' -DREVISION_DATE='"$(REV_DATE)"' -DCOMPDATE='"$(COMPDATE)"' $(@:%.o=../vnmr/%.c) )

acqhdl.o : $(@:%.o=../vnmr/%.c)
		(umask 2; $(COMPILE.c) -DACQPROC_LC $(@:%.o=../vnmr/%.c))

#################
install:;
	@if(test -d $(VNMR)); then \
		if(test ! -d $(VNMR)/bin); then \
			mkdir $(VNMR)/bin; \
		fi;\
		if(test -f Vnmrbg); then \
			echo "installing Vnmrbg in $(VNMR)/bin"; \
			cp Vnmrbg $(VNMR)/bin; \
		else \
			echo "could not install Vnmrbg <file not present in working directory>"; \
		fi; \
	else \
		echo "could not install Vnmrbg <vnmr directory not accessable> ";\
	fi; \
	echo "DONE target $@ --------------------------"
#################
clean:
	rm -f *.o *.so Vnmrbg
	@echo "DONE target $@ --------------------------"
clobber:
	rm -f *.c *.C *.h *.o *.p *.icon *.y *.l *.parser *.output yytoddl
	rm -f Vnmrbg
	@echo "DONE target $@ --------------------------"
#################
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
	-@(echo VNMR="$(VNMR)"; )
	-@(echo MAKEDIR="$(MAKEDIR)"; )
	-@(echo PLATFORM="$(PLATFORM)"; )

none:;
############################################################
# dependencies
############################################################
# 1. special problem with vnmrsys.h 
#  - different files with same name in psg and vnmr directories
#  - If psg/vnmrsys.h is included first ecc_setup.c build fails 
#    because "VNMR" is not defined.
# 
#   workaround:
#  - before running depend target rename vnmrsys.h in psg to something 
#    else (e.g. vnmrsys2.h).
#  - run "make depend" (sets vnmrsys dependencies to vnmr file)
#  - restore name in psg directory

# AUTO-DEPENDENCY GENERATION ALGORITHM
# target "depend"
# 1. runs "makedepend" for source files in current directory.
#    - removes Makefile text following "DO NOT DELETE .." line
#    - sets dependencies for local ".h" files (e.g. file.o: file.h)
#    - sets dependencies for remote ".h" files (e.g. file.o: ../dir/file.h)
# 2. runs "makedepend" for source files in remote directories
#    - generates dependencies like ../dir/file.o: ../dir/file.h
#    - pass through awk script to strip off ../dir/ in ".o" strings so that
#      object files are specified in current directory (e.g. file.o vs. ../dir/file.o)
#    - append remote dependencies to makedepend output from (1) 
# 3. runs "make lists" to generate explicit "object:source" dependencies for all files
#    - needed on Linux because "implicit" dependencies (e.g. file.o:file.c)
#      don't seem to always work
#    - append "make lists" dependencies to output from (2)
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
        makedepend -Y./ $(INCS) $(VNMRBG_SRC) $(VNMRBG_CC_SRC) 2>/dev/null; \
        make -s FILES="$(VNMRBG_SRC:%.c=%)" list_depends | sort >>Makefile; \
        make -s FILES="$(VNMRBG_CC_SRC:%.C=%)" SFX=".C" list_depends | sort >>Makefile; \
        make -s FILES="$(NACQI_SRC:%.c=%)" SRC="$(NACQI_SRC:%.c=../nacqi/%.c)" DIR="nacqi" dirlist >>lists; \
        make -s FILES="$(NCOMM_SRC:%.c=%)" SRC="$(NCOMM_SRC:%.c=../ncomm/%.c)" DIR="ncomm" dirlist >>lists; \
        make -s FILES="$(EXPPROC_SRC:%.c=%)" SRC="$(EXPPROC_SRC:%.c=../expproc/%.c)" DIR="expproc" dirlist >>lists; \
        make -s FILES="$(PSG_SRC:%.c=%)" SRC="$(PSG_SRC:%.c=../psg/%.c)" DIR="psg" dirlist >>lists; \
        make -s FILES="$(FT3D_SRC:%.c=%)" SRC="$(FT3D_SRC:%.c=../3D/%.c)" DIR="3D" dirlist >>lists; \
        make -s FILES="$(VNMR_SRC:%.c=%)" SRC="$(VNMR_SRC:%.c=../vnmr/%.c)" DIR="vnmr" dirlist >>lists; \
        make -s FILES="$(XWIN_SRC:%.c=%)" SRC="$(XWIN_SRC:%.c=../xwin/%.c)" DIR="xwin" dirlist >>lists; \
        make -s FILES="$(AIP_SRC:%.C=%)" SRC="$(AIP_SRC:%.C=../aip/%.C)" SFX=".C" DIR="aip" dirlist >>lists; \
        make -s FILES="$(DDL_SRC:%.C=%)" SRC="$(DDL_SRC:%.C=../ddl/%.C)" SFX=".C" DIR="ddl" dirlist >>lists; \
        cat lists >> $(MAKEFILE); \
        rm lists tmp;
        
# DO NOT DELETE THIS LINE -- make depend depends on it.

dcon.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/graphics.h ../vnmr/group.h
dcon.o: ../vnmr/init2d.h ../vnmr/sky.h ../vnmr/variables.h ../vnmr/vnmrsys.h
dcon.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/allocate.h ../vnmr/buttons.h
dcon.o: ../vnmr/dscale.h ../vnmr/init_display.h ../vnmr/init_proc.h
dconi.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/data.h ../vnmr/disp.h
dconi.o: ../vnmr/init2d.h ../vnmr/graphics.h ../vnmr/group.h ../vnmr/sky.h
dconi.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/buttons.h ../vnmr/dscale.h
dconi.o: ../vnmr/init_display.h
decc.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/pvars.h ../vnmr/tools.h
dfid.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h ../vnmr/graphics.h
dfid.o: ../vnmr/group.h ../vnmr/init2d.h ../vnmr/sky.h ../vnmr/tools.h
dfid.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/allocate.h
dfid.o: ../vnmr/buttons.h ../vnmr/displayops.h ../vnmr/dscale.h
dfid.o: ../vnmr/init_display.h ../vnmr/wjunk.h
dpcon.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/graphics.h ../vnmr/group.h
dpcon.o: ../vnmr/init2d.h ../vnmr/sky.h ../vnmr/vnmrsys.h ../vnmr/allocate.h
dpcon.o: ../vnmr/pvars.h ../vnmr/wjunk.h
iplan.o: ../vnmr/iplan.h ../vnmr/vnmrsys.h ../vnmr/group.h
iplan.o: ../vnmr/variables.h ../vnmr/data.h ../vnmr/graphics.h
iplan.o: ../vnmr/init2d.h ../vnmr/iplan_graphics.h ../vnmr/pvars.h
iplan.o: ../aip/aipCInterface.h ../aip/aipCStructs.h
iplan_graphics.o: ../vnmr/iplan.h ../vnmr/vnmrsys.h ../vnmr/group.h
iplan_graphics.o: ../vnmr/variables.h ../vnmr/data.h ../vnmr/graphics.h
iplan_graphics.o: ../vnmr/init2d.h ../vnmr/iplan_graphics.h
iplan_graphics.o: ../aip/aipCInterface.h ../aip/aipCStructs.h ../vnmr/pvars.h
iplan_graphics.o: ../vnmr/tools.h ../vnmr/wjunk.h
iplan_params.o: ../vnmr/vnmrsys.h ../vnmr/pvars.h ../vnmr/variables.h
iplan_params.o: ../vnmr/group.h ../vnmr/iplan.h ../vnmr/data.h
iplan_params.o: ../vnmr/graphics.h ../vnmr/init2d.h
locki.o: ../vnmr/data.h ../vnmr/group.h ../xracq/ACQ_SUN.h ../vnmr/vnmrsys.h
locki.o: ../expproc/shrexpinfo.h ../expproc/expQfuncs.h ../ncomm/mfileObj.h
locki.o: ../expproc/shrstatinfo.h ../vwacq/hostAcqStructs.h
locki.o: ../vwacq/acqcmds.h ../vnmr/graphics.h ../vnmr/allocate.h
locki.o: ../vnmr/pvars.h ../vnmr/wjunk.h
main.o: ../vnmr/vnmrsys.h
master.o: ../vnmr/locksys.h ../vnmr/acquisition.h ../vnmr/allocate.h
master.o: ../ncomm/sockets.h ../vnmr/sockinfo.h ../vnmr/wjunk.h
smagic.o: ../vnmr/vnmrsys.h ../vnmr/locksys.h ../vnmr/allocate.h
smagic.o: ../vnmr/wjunk.h ../vnmr/acquisition.h ../ncomm/sockets.h
smagic.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/params.h
smagic.o: ../vnmr/vfilesys.h ../vnmr/comm.h ../aip/aipCFuncs.h
smagic.o: ../aip/aipDataStruct.h ../vnmr/tools.h ../vnmr/pvars.h
smagic.o: ../vnmr/buttons.h
crosshair.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
crosshair.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/init2d.h
crosshair.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/wjunk.h
crosshair.o: ../vnmr/variables.h
mouseHandler.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/pvars.h
frameManager.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
frameManager.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/init2d.h
frameManager.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/wjunk.h
frameManager.o: ../vnmr/variables.h
wjunk.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/node.h ../vnmr/group.h
wjunk.o: ../vnmr/pvars.h ../vnmr/tools.h ../vnmr/wjunk.h ../vnmr/variables.h
graphics_win.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/pvars.h
graphics_win.o: ../vnmr/graphics.h ../aip/aipGraphics.h vjXdef.h
graphics_win.o: ../aip/aipCStructs.h ../aip/aipCFuncs.h
graphics_win.o: ../aip/aipDataStruct.h ../vnmr/iplan.h ../vnmr/group.h
graphics_win.o: ../vnmr/variables.h ../vnmr/data.h ../vnmr/init2d.h
graphics_win.o: ../vnmr/wjunk.h ../vnmr/buttons.h ../vnmr/symtab.h
master_win.o: vjXdef.h ../vnmr/acquisition.h
smagic_win.o: ../vnmr/vnmrsys.h ../vnmr/locksys.h ../vnmr/tools.h vjXdef.h
graphics3D.o: ../aip/aipCommands.h ../aip/aipDataInfo.h ../aip/sharedPtr.h
graphics3D.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
graphics3D.o: ../ncomm/mfileObj.h ../aip/aipStructs.h ../aip/aipCStructs.h
graphics3D.o: ../aip/aipVsInfo.h ../aip/aipMouse.h ../aip/aipHistogram.h
graphics3D.o: ../aip/aipDataStruct.h ../aip/aipDataManager.h
graphics3D.o: ../aip/aipVolData.h ../aip/aipOrthoSlices.h ../vnmr/group.h
graphics3D.o: ../vnmr/pvars.h ../vnmr/params.h ../vnmr/data.h
graphics3D.o: ../vnmr/vnmrsys.h ../vnmr/variables.h ../vnmr/wjunk.h
crosshair.o: crosshair.c
dconi.o: dconi.c
dcon.o: dcon.c
decc.o: decc.c
dfid.o: dfid.c
dpcon.o: dpcon.c
frameManager.o: frameManager.c
graphics_win.o: graphics_win.c
iplan_graphics.o: iplan_graphics.c
iplan.o: iplan.c
iplan_params.o: iplan_params.c
locki.o: locki.c
main.o: main.c
master.o: master.c
pipeRead.o: pipeRead.c
master_win.o: master_win.c
mouseHandler.o: mouseHandler.c
smagic.o: smagic.c
smagic_win.o: smagic_win.c
wjunk.o: wjunk.c
graphics3D.o: graphics3D.C
#========== nacqi ======== 
msgqueue.o: ../nacqi/acqInterface.h ../vnmr/vnmrsys.h ../vnmr/comm.h
msgqueue.o: ../vnmr/group.h
msgqueue.o: ../nacqi/msgqueue.c
#========== ncomm ======== 
asyncIo.o: ../ncomm/eventHandler.h ../ncomm/errLogLib.h
eventHandler.o: ../ncomm/errLogLib.h ../ncomm/listObj.h
eventHandler.o: ../ncomm/eventQueue.h ../ncomm/eventHandler.h
eventQueue.o: ../ncomm/eventQueueP.h ../ncomm/eventQueue.h
hhashLib.o: ../ncomm/errLogLib.h ../ncomm/hhashLib.h
ipcKeyDbm.o: ../ncomm/errLogLib.h ../ncomm/shrMLib.h
ipcKeyDbm.o: ../ncomm/mfileObj.h ../ncomm/ipcKeyDbm.h
ipcMsgQLib.o: ../ncomm/errLogLib.h ../ncomm/ipcMsgQLib.h
listObj.o: ../ncomm/listObj.h
mfileObj.o: ../ncomm/errLogLib.h ../ncomm/mfileObj.h
msgQLib.o: ../ncomm/errLogLib.h ../expproc/hostMsgChannels.h
msgQLib.o: ../ncomm/chanLib.h ../ncomm/eventHandler.h
msgQLib.o: ../ncomm/mfileObj.h ../ncomm/msgQLib.h
msgQLib.o: ../ncomm/ipcMsgQLib.h ../ncomm/shrMLib.h
semLib.o: ../ncomm/errLogLib.h ../ncomm/semLib.h ../ncomm/mfileObj.h
sendAsync.o: ../ncomm/errLogLib.h ../ncomm/sockets.h
shrMLib.o: ../ncomm/errLogLib.h ../ncomm/shrMLib.h
shrMLib.o: ../ncomm/mfileObj.h ../ncomm/semLib.h
sockets.o: ../ncomm/errLogLib.h ../ncomm/sockets.h
asyncIo.o: ../ncomm/asyncIo.c
errLogLib.o: ../ncomm/errLogLib.c
eventHandler.o: ../ncomm/eventHandler.c
eventQueue.o: ../ncomm/eventQueue.c
fdmask.o: ../ncomm/fdmask.c
hhashLib.o: ../ncomm/hhashLib.c
ipcKeyDbm.o: ../ncomm/ipcKeyDbm.c
ipcMsgQLib.o: ../ncomm/ipcMsgQLib.c
listObj.o: ../ncomm/listObj.c
mfileObj.o: ../ncomm/mfileObj.c
msgQLib.o: ../ncomm/msgQLib.c
semLib.o: ../ncomm/semLib.c
sendAsync.o: ../ncomm/sendAsync.c
shrMLib.o: ../ncomm/shrMLib.c
sockets.o: ../ncomm/sockets.c
#========== expproc ======== 
expQfuncs.o: ../ncomm/errLogLib.h ../ncomm/mfileObj.h
expQfuncs.o: ../ncomm/shrMLib.h ../expproc/shrexpinfo.h
expQfuncs.o: ../expproc/expQfuncs.h
statfuncs.o: ../ncomm/errLogLib.h ../vwacq/hostAcqStructs.h
statfuncs.o: ../ncomm/mfileObj.h ../ncomm/shrMLib.h
statfuncs.o: ../expproc/shrstatinfo.h
expQfuncs.o: ../expproc/expQfuncs.c
statfuncs.o: ../expproc/statfuncs.c
#========== psg ======== 
ap_device.o: ../xracq/acodes.h ../psg/oopc.h ../psg/ap_device.p
ap_device.o: ../psg/device.p ../psg/common.p
device.o: ../psg/oopc.h ../psg/device.p ../psg/common.p
ecc_setup.o: ../vnmr/vnmrsys.h ../xracq/acodes.h ../vnmr/group.h
ecc_setup.o: ../psg/ecc.h ../vnmr/pvars.h
ecc_worstCase.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../psg/ecc.h
ecc_worstCase.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/wjunk.h
freq_device.o: ../psg/rfconst.h ../xracq/acodes.h ../psg/oopc.h
freq_device.o: ../psg/freq_device.p ../psg/ap_device.p ../psg/device.p
freq_device.o: ../psg/common.p
freqfuncs.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/params.h
freqfuncs.o: ../vnmr/pvars.h
objerror.o: ../psg/oopc.h ../psg/objerror.h
ap_device.o: ../psg/ap_device.c
device.o: ../psg/device.c
ecc_setup.o: ../psg/ecc_setup.c
ecc_worstCase.o: ../psg/ecc_worstCase.c
freq_device.o: ../psg/freq_device.c
freqfuncs.o: ../psg/freqfuncs.c
objerror.o: ../psg/objerror.c
#========== 3D ======== 
lock3Dfiles.o: ../3D/lock3D.h ../vnmr/wjunk.h
lock3Dfiles.o: ../3D/lock3Dfiles.c
#========== vnmr ======== 
acqfuncs.o: ../vnmr/vnmrsys.h ../vnmr/bgvars.h ../vnmr/sockinfo.h
acqfuncs.o: ../vnmr/variables.h ../vnmr/group.h ../vnmr/params.h
acqfuncs.o: ../vnmr/tools.h ../vnmr/locksys.h ../vnmr/pvars.h
acqfuncs.o: ../vnmr/vfilesys.h ../vnmr/wjunk.h ../xracq/STAT_DEFS.h
acqfuncs.o: ../xracq/ACQ_HAL.h ../xracq/ACQ_SUN.h ../vnmr/shims.h
acqfuncs.o: ../vnmr/acquisition.h
acqhwcmd.o: ../vnmr/vnmrsys.h ../xracq/ACQ_SUN.h ../xracq/STAT_DEFS.h
acqhwcmd.o: ../vnmr/group.h ../vnmr/params.h ../vnmr/locksys.h
acqhwcmd.o: ../vnmr/variables.h ../vnmr/shims.h ../vnmr/acquisition.h
acqhwcmd.o: ../vnmr/pvars.h ../vnmr/sockinfo.h ../vnmr/tools.h
acqhwcmd.o: ../vnmr/wjunk.h ../ncomm/sockets.h
addsub.o: ../vnmr/vnmrsys.h ../vnmr/disp.h ../vnmr/data.h
addsub.o: ../vnmr/group.h ../vnmr/init2d.h ../vnmr/allocate.h
addsub.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/sky.h
addsub.o: ../vnmr/wjunk.h ../vnmr/variables.h
ai.o: ../vnmr/group.h ../vnmr/data.h ../vnmr/init2d.h
ai.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/wjunk.h
ai.o: ../vnmr/vnmrsys.h
asm.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/acquisition.h
asm.o: ../vnmr/pvars.h ../vnmr/wjunk.h
asmfuncs.o: ../vnmr/asm.h
assign.o: ../vnmr/group.h ../vnmr/params.h ../vnmr/variables.h
assign.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/tools.h
assign.o: ../vnmr/wjunk.h
bgvars.o: ../vnmr/group.h ../vnmr/symtab.h ../vnmr/variables.h
bootup.o: ../vnmr/vnmrsys.h ../vnmr/locksys.h ../vnmr/data.h
bootup.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/init2d.h
bootup.o: ../vnmr/acquisition.h ../vnmr/pvars.h ../vnmr/tools.h
bootup.o: ../vnmr/wjunk.h
builtin.o: ../vnmr/symtab.h ../vnmr/variables.h ../vnmr/vnmrsys.h
builtin.o: ../vnmr/group.h ../vnmr/pvars.h ../vnmr/allocate.h
builtin.o: ../vnmr/buttons.h ../vnmr/tools.h ../vnmr/wjunk.h
builtin1.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/params.h
builtin1.o: ../vnmr/symtab.h ../vnmr/variables.h ../vnmr/pvars.h
builtin1.o: ../vnmr/wjunk.h ../vnmr/allocate.h ../vnmr/tools.h
buttons.o: ../vnmr/vnmrsys.h ../vnmr/tools.h ../vnmr/wjunk.h
buttons.o: ../vnmr/allocate.h
calcq.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/data.h
calcq.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/wjunk.h
calcq.o: ../vnmr/init2d.h
continprepare.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
continprepare.o: ../vnmr/init2d.h ../vnmr/graphics.h ../vnmr/group.h
continprepare.o: ../vnmr/pvars.h ../vnmr/wjunk.h
continread.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
continread.o: ../vnmr/init2d.h ../vnmr/graphics.h ../vnmr/group.h
continread.o: ../vnmr/wjunk.h
createparams.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/vfilesys.h
createparams.o: ../vnmr/wjunk.h
data.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
data.o: ../vnmr/allocate.h ../vnmr/wjunk.h
ddf.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
ddf.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/allocate.h
ddf.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/vfilesys.h
ddif.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/data.h
ddif.o: ../vnmr/variables.h ../vnmr/vnmrsys.h ../vnmr/pvars.h
ddif.o: ../vnmr/wjunk.h ../vnmr/allocate.h
ddph.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/data.h
ddph.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/init2d.h
ddph.o: ../vnmr/pvars.h ../vnmr/wjunk.h
debug.o: ../vnmr/vnmrsys.h ../vnmr/wjunk.h
df2d.o: ../vnmr/data.h ../vnmr/ftpar.h ../vnmr/process.h
df2d.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/vnmrsys.h
df2d.o: ../vnmr/sky.h ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/wjunk.h
dfww.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/graphics.h
dfww.o: ../vnmr/group.h ../vnmr/init2d.h ../vnmr/tools.h
dfww.o: ../vnmr/variables.h ../vnmr/dscale.h ../vnmr/buttons.h
dfww.o: ../vnmr/init_display.h ../vnmr/pvars.h ../vnmr/wjunk.h
dg.o: ../vnmr/allocate.h ../vnmr/group.h ../vnmr/variables.h
dg.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/buttons.h
dg.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/tools.h
disp3Dmap.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/data.h
disp3Dmap.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/init2d.h
disp3Dmap.o: ../vnmr/pvars.h ../vnmr/wjunk.h
dll.o: ../vnmr/allocate.h ../vnmr/data.h ../vnmr/disp.h
dll.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/init2d.h
dll.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/vnmrsys.h
dll.o: ../vnmr/pvars.h ../vnmr/wjunk.h
dosyfit.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
dosyfit.o: ../vnmr/pvars.h ../vnmr/init2d.h ../vnmr/graphics.h
dosyfit.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/wjunk.h
dosyfit3D.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
dosyfit3D.o: ../vnmr/init2d.h ../vnmr/graphics.h ../vnmr/group.h
dosyfit3D.o: ../vnmr/pvars.h ../vnmr/wjunk.h
dpf.o: ../vnmr/data.h ../vnmr/graphics.h ../vnmr/group.h
dpf.o: ../vnmr/init2d.h ../vnmr/tools.h ../vnmr/variables.h
dpf.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/buttons.h
dpf.o: ../vnmr/pvars.h ../vnmr/wjunk.h
dps.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/dpsdef.h
dps.o: ../vnmr/vfilesys.h ../vnmr/pvars.h ../vnmr/wjunk.h
dps.o: ../vnmr/group.h ../vnmr/symtab.h ../vnmr/tools.h
dps.o: ../vnmr/variables.h ../psg/REV_NUMS.h ../vnmr/allocate.h
dps.o: ../vnmr/dps_menu.icon
ds.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
ds.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/init2d.h
ds.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/wjunk.h
ds.o: ../vnmr/variables.h ../vnmr/allocate.h ../vnmr/buttons.h
ds.o: ../vnmr/init_display.h ../vnmr/dscale.h
dscale.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/data.h
dscale.o: ../vnmr/disp.h ../vnmr/init2d.h ../vnmr/allocate.h
dscale.o: ../vnmr/sky.h ../vnmr/pvars.h ../vnmr/tools.h
dscale.o: ../vnmr/wjunk.h ../vnmr/init_display.h ../vnmr/init_proc.h
dsn.o: ../vnmr/data.h ../vnmr/graphics.h ../vnmr/group.h
dsn.o: ../vnmr/init2d.h ../vnmr/tools.h ../vnmr/variables.h
dsn.o: ../vnmr/vnmrsys.h ../vnmr/buttons.h ../vnmr/pvars.h
dsn.o: ../vnmr/wjunk.h
dsp.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
dsp.o: ../vnmr/init2d.h ../vnmr/variables.h ../vnmr/tools.h
dsp.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/sky.h
dsp.o: ../vnmr/wjunk.h
dsww.o: ../vnmr/data.h ../vnmr/allocate.h ../vnmr/disp.h
dsww.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/init2d.h
dsww.o: ../vnmr/tools.h ../vnmr/pvars.h ../vnmr/variables.h
dsww.o: ../vnmr/wjunk.h ../vnmr/vnmrsys.h ../vnmr/dscale.h
dsww.o: ../vnmr/init_display.h ../vnmr/buttons.h
element.o: ../vnmr/vnmrsys.h ../vnmr/element.h ../vnmr/tools.h
element.o: ../vnmr/allocate.h ../vnmr/wjunk.h
emouse.o: ../vnmr/vnmrsys.h ../vnmr/wjunk.h
execcmd.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/tools.h
execcmd.o: ../vnmr/wjunk.h
fiddle.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/data.h
fiddle.o: ../vnmr/variables.h ../vnmr/vnmrsys.h ../vnmr/pvars.h
fiddle.o: ../vnmr/wjunk.h ../vnmr/fft.h ../vnmr/displayops.h
files.o: ../vnmr/vnmrsys.h ../vnmr/element.h ../vnmr/tools.h
flashc.o: ../vnmr/data.h ../vnmr/group.h ../ncomm/mfileObj.h
flashc.o: ../vnmr/symtab.h ../vnmr/vnmrsys.h ../vnmr/variables.h
flashc.o: ../vnmr/pvars.h ../vnmr/wjunk.h
ft.o: ../vnmr/data.h ../vnmr/ftpar.h ../vnmr/process.h
ft.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/vnmrsys.h
ft.o: ../vnmr/allocate.h ../vnmr/buttons.h ../vnmr/displayops.h
ft.o: ../vnmr/fft.h ../vnmr/pvars.h ../vnmr/sky.h ../vnmr/tools.h
ft.o: ../vnmr/params.h ../vnmr/wjunk.h
ft2d.o: ../vnmr/data.h ../vnmr/ftpar.h ../vnmr/process.h
ft2d.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/vnmrsys.h
ft2d.o: ../vnmr/sky.h ../vnmr/tools.h ../vnmr/allocate.h
ft2d.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/displayops.h
ft2d.o: ../vnmr/fft.h
ftinit.o: ../vnmr/data.h ../vnmr/allocate.h ../vnmr/ftpar.h
ftinit.o: ../vnmr/process.h ../vnmr/group.h ../vnmr/variables.h
ftinit.o: ../vnmr/vfilesys.h ../vnmr/vnmrsys.h ../vnmr/pvars.h
ftinit.o: ../vnmr/wjunk.h ../vnmr/tools.h
full.o: ../vnmr/disp.h ../vnmr/group.h ../vnmr/tools.h
full.o: ../vnmr/vnmrsys.h ../vnmr/pvars.h ../vnmr/wjunk.h
full.o: ../vnmr/init_display.h
gdevsw.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/wjunk.h
glide.o: ../vnmr/wjunk.h
go.o: ../vnmr/vnmrsys.h ../ncomm/sockets.h ../vnmr/data.h
go.o: ../vnmr/group.h ../vnmr/params.h ../vnmr/bgvars.h
go.o: ../vnmr/symtab.h ../vnmr/tools.h ../vnmr/variables.h
go.o: ../psg/REV_NUMS.h ../vnmr/locksys.h ../xracq/STAT_DEFS.h
go.o: ../vnmr/acquisition.h ../vnmr/whenmask.h ../vnmr/vfilesys.h
go.o: ../vnmr/pvars.h ../vnmr/wjunk.h ../vnmr/sockinfo.h
gradfit.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/data.h
gradfit.o: ../vnmr/variables.h ../vnmr/vnmrsys.h ../vnmr/pvars.h
gradfit.o: ../vnmr/wjunk.h
graphics.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/group.h
graphics.o: ../vnmr/pvars.h ../vnmr/wjunk.h
graphics_util.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h
graphics_util.o: ../vnmr/graphics_util.h ../vnmr/wjunk.h
gxyzcalib.o: ../vnmr/tools.h ../vnmr/wjunk.h
gxyzcalib2.o: ../vnmr/tools.h ../vnmr/wjunk.h
gxyzmap.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/data.h
gxyzmap.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/init2d.h
gxyzmap.o: ../vnmr/pvars.h ../vnmr/wjunk.h
gzfit.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
gzfit.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/pvars.h
gzfit.o: ../vnmr/wjunk.h
handler.o: ../vnmr/buttons.h ../vnmr/wjunk.h
help.o: ../vnmr/vnmrsys.h ../vnmr/vfilesys.h ../vnmr/buttons.h
help.o: ../vnmr/wjunk.h
history.o: ../vnmr/allocate.h ../vnmr/tools.h
hpa.o: ../vnmr/data.h ../vnmr/graphics.h ../vnmr/group.h
hpa.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/vnmrsys.h
hpa.o: ../vnmr/pvars.h ../vnmr/wjunk.h
ht.o: ../vnmr/ftpar.h ../vnmr/data.h ../vnmr/process.h
ht.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/vnmrsys.h
ht.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/sky.h
ht.o: ../vnmr/wjunk.h ../psg/Pbox_HT.h
init2d.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/graphics.h
init2d.o: ../vnmr/group.h ../vnmr/init_display.h ../vnmr/init_proc.h
init2d.o: ../vnmr/variables.h ../vnmr/vnmrsys.h ../vnmr/pvars.h
init2d.o: ../vnmr/vfilesys.h ../vnmr/wjunk.h
init_display.o: ../vnmr/disp.h ../vnmr/group.h ../vnmr/variables.h
init_display.o: ../vnmr/pvars.h ../vnmr/wjunk.h
init_proc.o: ../vnmr/disp.h ../vnmr/group.h ../vnmr/variables.h
init_proc.o: ../vnmr/process.h ../vnmr/pvars.h ../vnmr/wjunk.h
init_proc.o: ../vnmr/init_display.h
integ.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/init2d.h
integ.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/allocate.h
integ.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/wjunk.h
io.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/node.h
io.o: ../magic/magic.gram.h ../vnmr/allocate.h ../vnmr/wjunk.h
jexp.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
jexp.o: ../vnmr/init2d.h ../vnmr/locksys.h ../vnmr/variables.h
jexp.o: ../vnmr/pvars.h ../vnmr/tools.h ../vnmr/vfilesys.h
jexp.o: ../vnmr/shims.h ../vnmr/wjunk.h ../vnmr/buttons.h
jexp.o: ../vnmr/params.h
ll2d.o: ../vnmr/data.h ../vnmr/init2d.h ../vnmr/pvars.h
ll2d.o: ../vnmr/vnmrsys.h ../vnmr/disp.h ../vnmr/tools.h
ll2d.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/allocate.h
ll2d.o: ../vnmr/buttons.h ../vnmr/init_display.h ../vnmr/init_proc.h
ll2d.o: ../vnmr/dscale.h ../vnmr/wjunk.h
lockfreqfunc.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/allocate.h
lockfreqfunc.o: ../vnmr/pvars.h
locksys.o: ../vnmr/vnmrsys.h ../vnmr/locksys.h ../vnmr/tools.h
locksys.o: ../vnmr/wjunk.h
lookup.o: ../vnmr/tools.h ../vnmr/vnmrsys.h ../vnmr/group.h
lookup.o: ../vnmr/pvars.h ../vnmr/wjunk.h
lstring.o: ../vnmr/allocate.h ../vnmr/tools.h ../vnmr/wjunk.h
macro.o: ../vnmr/node.h ../vnmr/symtab.h ../vnmr/vnmrsys.h
macro.o: ../vnmr/allocate.h ../vnmr/wjunk.h
magnetom.o: ../vnmr/vnmrsys.h ../vnmr/variables.h ../vnmr/group.h
magnetom.o: ../vnmr/element.h ../vnmr/data.h ../vnmr/init2d.h
magnetom.o: ../vnmr/pvars.h ../vnmr/wjunk.h
makeslice.o: ../vnmr/group.h ../vnmr/tools.h ../vnmr/data.h
makeslice.o: ../vnmr/variables.h ../vnmr/vnmrsys.h ../vnmr/disp.h
makeslice.o: ../vnmr/pvars.h ../vnmr/wjunk.h
md5c.o: ../vnmr/md5global.h ../vnmr/md5.h
mfdata.o: ../ncomm/mfileObj.h ../vnmr/data.h ../vnmr/group.h
mfdata.o: ../vnmr/tools.h ../vnmr/vnmrsys.h ../vnmr/variables.h
ops.o: ../vnmr/tools.h ../vnmr/variables.h ../vnmr/wjunk.h
part11.o: ../vnmr/data.h ../vnmr/part11.h ../vnmr/vnmrsys.h
part11.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/tools.h
part11.o: ../vnmr/shims.h ../vnmr/md5global.h ../vnmr/md5.h
part11.o: ../vnmr/locksys.h ../vnmr/allocate.h ../vnmr/buttons.h
part11.o: ../vnmr/pvars.h ../vnmr/wjunk.h
pcmap.o: ../ncomm/mfileObj.h ../vnmr/data.h ../vnmr/group.h
pcmap.o: ../vnmr/tools.h ../vnmr/vnmrsys.h ../vnmr/variables.h
pcmap.o: ../vnmr/phase_correct.h ../vnmr/pvars.h ../vnmr/wjunk.h
phase_fid.o: ../vnmr/buttons.h ../vnmr/data.h ../vnmr/graphics.h
phase_fid.o: ../vnmr/group.h ../vnmr/init2d.h ../ncomm/mfileObj.h
phase_fid.o: ../vnmr/symtab.h ../vnmr/vnmrsys.h ../vnmr/variables.h
phase_fid.o: ../vnmr/process.h ../vnmr/epi_recon.h
phase_fid.o: ../vnmr/phase_correct.h ../vnmr/allocate.h
phase_fid.o: ../vnmr/pvars.h ../vnmr/wjunk.h
plot_handlers.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/pvars.h
plot_handlers.o: ../vnmr/tools.h ../vnmr/wjunk.h ../vnmr/group.h
plot_construct.o: ../vnmr/group.h ../vnmr/pvars.h ../vnmr/wjunk.h
plot_construct.o: ../vnmr/vnmrsys.h ../vnmr/variables.h
plot_construct.o: ../vnmr/graphics.h ../ncomm/sockets.h
proc2d.o: ../vnmr/group.h ../vnmr/data.h ../vnmr/allocate.h
proc2d.o: ../vnmr/disp.h ../vnmr/displayops.h ../vnmr/init2d.h
proc2d.o: ../vnmr/init_display.h ../vnmr/init_proc.h
proc2d.o: ../vnmr/vnmrsys.h ../vnmr/process.h ../vnmr/pvars.h
proc2d.o: ../vnmr/wjunk.h
pvars.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/symtab.h
pvars.o: ../vnmr/params.h ../vnmr/variables.h ../vnmr/tools.h
pvars.o: ../vnmr/wjunk.h
recon_all.o: ../vnmr/data.h ../vnmr/group.h ../ncomm/mfileObj.h
recon_all.o: ../vnmr/symtab.h ../vnmr/vnmrsys.h ../vnmr/variables.h
recon_all.o: ../vnmr/fft.h ../vnmr/process.h ../vnmr/epi_recon.h
recon_all.o: ../vnmr/phase_correct.h ../vnmr/vfilesys.h
recon_all.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/wjunk.h
recon3D.o: ../vnmr/data.h ../vnmr/group.h ../ncomm/mfileObj.h
recon3D.o: ../vnmr/symtab.h ../vnmr/vnmrsys.h ../vnmr/variables.h
recon3D.o: ../vnmr/fft.h ../vnmr/process.h ../vnmr/epi_recon.h
recon3D.o: ../vnmr/phase_correct.h ../vnmr/vfilesys.h
recon3D.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/wjunk.h
recon_mm.o: ../vnmr/data.h ../vnmr/group.h ../ncomm/mfileObj.h
recon_mm.o: ../vnmr/symtab.h ../vnmr/vnmrsys.h ../vnmr/variables.h
recon_mm.o: ../vnmr/fft.h ../vnmr/process.h ../vnmr/epi_recon.h
recon_mm.o: ../vnmr/phase_correct.h ../vnmr/allocate.h
recon_mm.o: ../vnmr/pvars.h ../vnmr/wjunk.h
readlk.o: ../vnmr/vnmrsys.h ../vnmr/tools.h ../vnmr/acquisition.h
savretphf.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
savretphf.o: ../vnmr/group.h ../vnmr/init2d.h ../vnmr/pvars.h
savretphf.o: ../vnmr/wjunk.h
set3dproc.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/process.h
set3dproc.o: ../vnmr/group.h ../3D/struct3d.h ../vnmr/ftpar.h
set3dproc.o: ../vnmr/tools.h ../3D/lock3D.h ../vnmr/variables.h
set3dproc.o: ../vnmr/allocate.h ../vnmr/params.h ../vnmr/pvars.h
set3dproc.o: ../vnmr/wjunk.h
shellcmds.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/tools.h
shellcmds.o: ../vnmr/pvars.h ../vnmr/wjunk.h
shims.o: ../vnmr/group.h ../vnmr/pvars.h
specfreq.o: ../vnmr/vnmrsys.h ../vnmr/variables.h ../vnmr/group.h
specfreq.o: ../vnmr/params.h ../vnmr/pvars.h ../vnmr/tools.h
specfreq.o: ../vnmr/wjunk.h ../xracq/ACQ_HAL.h ../xracq/ACQ_SUN.h
specfreq.o: ../psg/oopc.h ../psg/rfconst.h
splmodprepare.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
splmodprepare.o: ../vnmr/pvars.h ../vnmr/init2d.h ../vnmr/graphics.h
splmodprepare.o: ../vnmr/group.h ../vnmr/wjunk.h
splmodread.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/pvars.h
splmodread.o: ../vnmr/disp.h ../vnmr/init2d.h ../vnmr/graphics.h
splmodread.o: ../vnmr/group.h ../vnmr/wjunk.h
sread.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
sread.o: ../vnmr/params.h ../vnmr/tools.h ../vnmr/variables.h
sread.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/wjunk.h
swrite.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/group.h
swrite.o: ../vnmr/init2d.h ../vnmr/symtab.h ../vnmr/tools.h
swrite.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/wjunk.h
symtab.o: ../vnmr/symtab.h ../vnmr/allocate.h ../vnmr/tools.h
table.o: ../vnmr/tools.h ../vnmr/vnmrsys.h ../vnmr/wjunk.h
terminal.o: ../vnmr/vnmrsys.h ../vnmr/wjunk.h ../vnmr/buttons.h
text.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/buttons.h
text.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/group.h
text.o: ../vnmr/graphics.h ../vnmr/init2d.h ../vnmr/tools.h
text.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/wjunk.h
text.o: ../vnmr/vfilesys.h ../vnmr/init_display.h
tools.o: ../vnmr/vnmrsys.h ../vnmr/tools.h ../vnmr/allocate.h
variables1.o: ../vnmr/graphics.h ../vnmr/group.h ../vnmr/params.h
variables1.o: ../vnmr/symtab.h ../vnmr/variables.h ../vnmr/init.h
variables1.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/tools.h
variables1.o: ../vnmr/wjunk.h
vcolor.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/group.h
vcolor.o: ../vnmr/pvars.h ../vnmr/tools.h ../vnmr/wjunk.h
vdialog.o: ../vnmr/group.h ../vnmr/vnmrsys.h ../vnmr/variables.h
vdialog.o: ../vnmr/vglide.h ../vnmr/allocate.h ../vnmr/pvars.h
vdialog.o: ../vnmr/wjunk.h
vfilesys.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/group.h
vfilesys.o: ../vnmr/pvars.h ../vnmr/tools.h ../vnmr/wjunk.h
weight.o: ../vnmr/data.h ../vnmr/disp.h ../vnmr/group.h
weight.o: ../vnmr/vnmrsys.h ../vnmr/pvars.h ../vnmr/sky.h
weight.o: ../vnmr/wjunk.h
annotate.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h ../vnmr/group.h
annotate.o: ../aip/aipCStructs.h ../vnmr/graphics_util.h
annotate.o: ../vnmr/allocate.h ../vnmr/pvars.h ../vnmr/tools.h
annotate.o: ../vnmr/wjunk.h vjXdef.h
wti.o: ../vnmr/data.h ../vnmr/ftpar.h ../vnmr/process.h
wti.o: ../vnmr/init2d.h ../vnmr/graphics.h ../vnmr/group.h
wti.o: ../vnmr/vnmrsys.h ../vnmr/variables.h ../vnmr/pvars.h
wti.o: ../vnmr/sky.h ../vnmr/allocate.h ../vnmr/buttons.h
wti.o: ../vnmr/displayops.h ../vnmr/dscale.h ../vnmr/tools.h
wti.o: ../vnmr/fft.h ../vnmr/wjunk.h
acqhdl.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/errorcodes.h
acqhdl.o: ../vnmr/variables.h ../vnmr/group.h ../vnmr/tools.h
acqhdl.o: ../vnmr/locksys.h ../vnmr/acqerrmsges.h ../vnmr/shims.h
acqhdl.o: ../vnmr/whenmask.h ../vnmr/acquisition.h ../vnmr/pvars.h
acqhdl.o: ../vnmr/wjunk.h ../vnmr/sockinfo.h ../xracq/ACQ_HAL.h
acqhdl.o: ../xracq/ACQ_SUN.h
displayops.o: ../vnmr/group.h ../vnmr/pvars.h ../vnmr/wjunk.h
foldj.o: ../vnmr/vnmrsys.h ../vnmr/data.h ../vnmr/disp.h
foldj.o: ../vnmr/init2d.h ../vnmr/tools.h ../vnmr/variables.h
foldj.o: ../vnmr/group.h ../vnmr/allocate.h ../vnmr/buttons.h
foldj.o: ../vnmr/displayops.h ../vnmr/init_display.h
foldj.o: ../vnmr/init_proc.h ../vnmr/pvars.h ../vnmr/sky.h
foldj.o: ../vnmr/wjunk.h
exec.o: ../vnmr/vnmrsys.h ../vnmr/group.h ../vnmr/node.h
exec.o: ../vnmr/params.h ../vnmr/allocate.h ../vnmr/variables.h
exec.o: ../magic/magic.gram.h ../vnmr/pvars.h ../vnmr/tools.h
exec.o: ../vnmr/wjunk.h
dli.o: ../vnmr/data.h ../vnmr/allocate.h ../vnmr/disp.h
dli.o: ../vnmr/group.h ../vnmr/init2d.h ../vnmr/tools.h
dli.o: ../vnmr/pvars.h ../vnmr/variables.h ../vnmr/wjunk.h
dli.o: ../vnmr/vnmrsys.h
fft.o: ../vnmr/allocate.h ../vnmr/wjunk.h
lexjunk.o: ../vnmr/vnmrsys.h ../vnmr/node.h ../vnmr/variables.h
lexjunk.o: ../magic/magic.gram.h ../vnmr/allocate.h ../vnmr/tools.h
lexjunk.o: ../vnmr/wjunk.h
lpz.o: ../vnmr/vnmrsys.h ../vnmr/allocate.h ../vnmr/group.h
lpz.o: ../vnmr/variables.h ../vnmr/pvars.h ../vnmr/wjunk.h
lpz.o: ../vnmr/data.h ../vnmr/process.h
matrix.o: ../vnmr/vnmrsys.h
node.o: ../vnmr/vnmrsys.h ../vnmr/node.h ../magic/magic.gram.h
node.o: ../vnmr/stack.h ../vnmr/allocate.h
downsample.o: ../vnmr/allocate.h ../vnmr/data.h ../vnmr/ftpar.h
downsample.o: ../vnmr/group.h ../vnmr/variables.h ../vnmr/vnmrsys.h
downsample.o: ../vnmr/pvars.h ../vnmr/tools.h ../vnmr/wjunk.h
lpqld.o: ../vnmr/ftpar.h ../vnmr/process.h
acqfuncs.o: ../vnmr/acqfuncs.c
acqhdl.o: ../vnmr/acqhdl.c
acqhwcmd.o: ../vnmr/acqhwcmd.c
addsub.o: ../vnmr/addsub.c
ai.o: ../vnmr/ai.c
allocate.o: ../vnmr/allocate.c
annotate.o: ../vnmr/annotate.c
asmfuncs.o: ../vnmr/asmfuncs.c
asm.o: ../vnmr/asm.c
assign.o: ../vnmr/assign.c
bgvars.o: ../vnmr/bgvars.c
bootup.o: ../vnmr/bootup.c
builtin1.o: ../vnmr/builtin1.c
builtin.o: ../vnmr/builtin.c
buttons.o: ../vnmr/buttons.c
calcq.o: ../vnmr/calcq.c
comm.o.o: ../vnmr/comm.o.c
continprepare.o: ../vnmr/continprepare.c
continread.o: ../vnmr/continread.c
createparams.o: ../vnmr/createparams.c
data.o: ../vnmr/data.c
ddf.o: ../vnmr/ddf.c
ddif.o: ../vnmr/ddif.c
ddph.o: ../vnmr/ddph.c
debug.o: ../vnmr/debug.c
df2d.o: ../vnmr/df2d.c
dfww.o: ../vnmr/dfww.c
dg.o: ../vnmr/dg.c
disp3Dmap.o: ../vnmr/disp3Dmap.c
displayops.o: ../vnmr/displayops.c
dli.o: ../vnmr/dli.c
dll.o: ../vnmr/dll.c
dosyfit3D.o: ../vnmr/dosyfit3D.c
dosyfit.o: ../vnmr/dosyfit.c
downsample.o: ../vnmr/downsample.c
dpf.o: ../vnmr/dpf.c
dps.o: ../vnmr/dps.c
dscale.o: ../vnmr/dscale.c
dsn.o: ../vnmr/dsn.c
ds.o: ../vnmr/ds.c
dsp.o: ../vnmr/dsp.c
dsww.o: ../vnmr/dsww.c
element.o: ../vnmr/element.c
emouse.o: ../vnmr/emouse.c
execcmd.o: ../vnmr/execcmd.c
exec.o: ../vnmr/exec.c
fft.o: ../vnmr/fft.c
fiddle.o: ../vnmr/fiddle.c
files.o: ../vnmr/files.c
flashc.o: ../vnmr/flashc.c
foldj.o: ../vnmr/foldj.c
ft2d.o: ../vnmr/ft2d.c
ftinit.o: ../vnmr/ftinit.c
ft.o: ../vnmr/ft.c
full.o: ../vnmr/full.c
gdevsw.o: ../vnmr/gdevsw.c
glide.o: ../vnmr/glide.c
go.o: ../vnmr/go.c
gradfit.o: ../vnmr/gradfit.c
graphics.o: ../vnmr/graphics.c
graphics_util.o: ../vnmr/graphics_util.c
gxyzcalib2.o: ../vnmr/gxyzcalib2.c
gxyzcalib.o: ../vnmr/gxyzcalib.c
gxyzmap.o: ../vnmr/gxyzmap.c
gzfit.o: ../vnmr/gzfit.c
handler.o: ../vnmr/handler.c
help.o: ../vnmr/help.c
history.o: ../vnmr/history.c
hpa.o: ../vnmr/hpa.c
ht.o: ../vnmr/ht.c
init2d.o: ../vnmr/init2d.c
init_display.o: ../vnmr/init_display.c
init_proc.o: ../vnmr/init_proc.c
integ.o: ../vnmr/integ.c
io.o: ../vnmr/io.c
jexp.o: ../vnmr/jexp.c
lexjunk.o: ../vnmr/lexjunk.c
ll2d.o: ../vnmr/ll2d.c
lockfreqfunc.o: ../vnmr/lockfreqfunc.c
locksys.o: ../vnmr/locksys.c
lookup.o: ../vnmr/lookup.c
lpqld.o: ../vnmr/lpqld.c
lpz.o: ../vnmr/lpz.c
lstring.o: ../vnmr/lstring.c
macro.o: ../vnmr/macro.c
magnetom.o: ../vnmr/magnetom.c
makeslice.o: ../vnmr/makeslice.c
matrix.o: ../vnmr/matrix.c
md5c.o: ../vnmr/md5c.c
mfdata.o: ../vnmr/mfdata.c
node.o: ../vnmr/node.c
ops.o: ../vnmr/ops.c
p11Tools.o: ../vnmr/p11Tools.c
part11.o: ../vnmr/part11.c
pcmap.o: ../vnmr/pcmap.c
phase_fid.o: ../vnmr/phase_fid.c
plot_construct.o: ../vnmr/plot_construct.c
plot_handlers.o: ../vnmr/plot_handlers.c
proc2d.o: ../vnmr/proc2d.c
pvars.o: ../vnmr/pvars.c
readlk.o: ../vnmr/readlk.c
recon3D.o: ../vnmr/recon3D.c
recon_all.o: ../vnmr/recon_all.c
recon_mm.o: ../vnmr/recon_mm.c
rtvarcmds.o: ../vnmr/rtvarcmds.c
savretphf.o: ../vnmr/savretphf.c
set3dproc.o: ../vnmr/set3dproc.c
shellcmds.o: ../vnmr/shellcmds.c
shims.o: ../vnmr/shims.c
sky.o: ../vnmr/sky.c
socket1.o.o: ../vnmr/socket1.o.c
socket.o.o: ../vnmr/socket.o.c
specfreq.o: ../vnmr/specfreq.c
splmodprepare.o: ../vnmr/splmodprepare.c
splmodread.o: ../vnmr/splmodread.c
sread.o: ../vnmr/sread.c
swrite.o: ../vnmr/swrite.c
symtab.o: ../vnmr/symtab.c
table.o: ../vnmr/table.c
tempstuff.o: ../vnmr/tempstuff.c
terminal.o: ../vnmr/terminal.c
text.o: ../vnmr/text.c
tools.o: ../vnmr/tools.c
variables1.o: ../vnmr/variables1.c
vcolor.o: ../vnmr/vcolor.c
vdialog.o: ../vnmr/vdialog.c
vfilesys.o: ../vnmr/vfilesys.c
weight.o: ../vnmr/weight.c
wti.o: ../vnmr/wti.c
#========== xwin ======== 
banner.o: ../vnmr/vnmrsys.h ../vnmr/graphics.h
banner.o: ../xwin/banner.c
master.icon.o: ../xwin/master.icon.c
ras_dump.o: ../xwin/ras_dump.c
vnmr.icon.o: ../xwin/vnmr.icon.c
vnmr_text.icon.o: ../xwin/vnmr_text.icon.c
#========== aip ======== 
aipBox.o: ../aip/aipUtils.h ../aip/aipStructs.h ../aip/aipCStructs.h
aipBox.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipBox.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h vjXdef.h
aipBox.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipBox.o: ../aip/aipGframe.h ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipBox.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipBox.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipBox.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h ../vnmr/vnmrsys.h
aipBox.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipBox.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipBox.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipBox.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipBox.o: ../aip/aipBox.h ../aip/aipOval.h
aipCommands.o: ../aip/aipCFuncs.h ../aip/aipDataStruct.h
aipCommands.o: ../aip/aipCommands.h ../aip/aipVnmrFuncs.h vjXdef.h
aipCommands.o: ../vnmr/vnmrsys.h ../aip/aipDataManager.h
aipCommands.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipCommands.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipCommands.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipCommands.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipCommands.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipCommands.o: ../aip/aipGframeManager.h ../aip/aipGframe.h
aipCommands.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipCommands.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipCommands.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipCommands.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipCommands.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipCommands.o: ../aip/aipInterface.h ../aip/aipCInterface.h
aipCommands.o: ../aip/aipRoiManager.h ../aip/aipStderr.h
aipCommands.o: ../aip/aipWinMath.h ../aip/aipParmList.h
aipCommands.o: ../aip/aipWinRotation.h ../aip/aipWinMovie.h
aipCommands.o: ../aip/aipMovie.h ../aip/aipWriteData.h
aipCommands.o: ../aip/aipVolData.h ../aip/aipOrthoSlices.h
aipDataInfo.o: ../aip/aipStderr.h ../aip/aipVnmrFuncs.h vjXdef.h
aipDataInfo.o: ../vnmr/vnmrsys.h ../aip/aipUtils.h
aipDataInfo.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipDataInfo.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipDataInfo.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipDataInfo.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipDataInfo.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipDataInfo.o: ../aip/aipDataStruct.h ../aip/aipLine.h
aipDataInfo.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipDataInfo.o: ../aip/aipImgInfo.h ../aip/aipInterpolation.h
aipDataInfo.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipDataInfo.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipDataInfo.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipDataInfo.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipDataInfo.o: ../aip/aipBox.h ../aip/aipOval.h ../aip/aipPolygon.h
aipDataInfo.o: ../aip/aipEdgelist.h ../aip/aipPolyline.h
aipDataManager.o: ../aip/aipStderr.h ../aip/aipVnmrFuncs.h vjXdef.h
aipDataManager.o: ../vnmr/vnmrsys.h ../aip/aipCommands.h
aipDataManager.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipDataManager.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipDataManager.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipDataManager.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipDataManager.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipDataManager.o: ../aip/aipDataStruct.h ../aip/aipCFuncs.h
aipDataManager.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipDataManager.o: ../aip/aipImgInfo.h ../aip/aipInterpolation.h
aipDataManager.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipDataManager.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipDataManager.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipDataManager.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipDataManager.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipDataManager.o: ../aip/aipVolData.h ../aip/aipOrthoSlices.h
aipDataManager.o: ../aip/aipWinMovie.h ../aip/aipWinRotation.h
aipDataManager.o: ../vnmr/group.h ../aip/aipWinMath.h
aipDataManager.o: ../aip/aipParmList.h
aipEdgelist.o: ../aip/aipRoi.h ../aip/sharedPtr.h
aipEdgelist.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipEdgelist.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipEdgelist.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipEdgelist.o: ../aip/aipHistogram.h ../aip/aipRoiStat.h
aipEdgelist.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipEdgelist.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipEdgelist.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipEdgelist.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipEdgelist.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipEdgelist.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipEdgelist.o: ../aip/aipEdgelist.h
aipGframe.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipGframe.o: ../aip/aipStderr.h ../aip/sharedPtr.h ../aip/aipRoi.h
aipGframe.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipGframe.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipGframe.o: ../aip/aipHistogram.h ../aip/aipRoiStat.h
aipGframe.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipGframe.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipGframe.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipGframe.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipGframe.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipGframe.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipGframe.o: ../aip/aipRoiManager.h ../aip/aipGframe.h
aipGframe.o: ../aip/aipViewInfo.h ../aip/aipInterpolation.h
aipGframe.o: ../aip/aipGraphicsWin.h ../aip/aipGraphics.h
aipGframe.o: ../aip/aipGframeManager.h ../aip/aipInterface.h
aipGframe.o: ../aip/aipCInterface.h ../vnmr/group.h
aipGframe.o: ../aip/aipWinMovie.h ../aip/aipMovie.h
aipGframe.o: ../aip/aipVolData.h ../aip/aipOrthoSlices.h
aipGframeManager.o: ../aip/sharedPtr.h ../aip/aipStderr.h
aipGframeManager.o: ../aip/aipCommands.h ../aip/aipGraphicsWin.h
aipGframeManager.o: vjXdef.h ../aip/aipGraphics.h ../aip/aipCStructs.h
aipGframeManager.o: ../aip/aipStructs.h ../aip/aipVsInfo.h
aipGframeManager.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipGframeManager.o: ../aip/aipInterpolation.h ../aip/aipGframe.h
aipGframeManager.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipGframeManager.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h
aipGframeManager.o: ../ddl/ddlNode.h ../ddl/ddlList.h
aipGframeManager.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipGframeManager.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h
aipGframeManager.o: ../vnmr/vnmrsys.h ../aip/aipRoiStat.h
aipGframeManager.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipGframeManager.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipGframeManager.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipGframeManager.o: ../aip/aipGframeManager.h ../aip/aipMovie.h
aipGframeManager.o: ../aip/aipWinMovie.h ../aip/aipOrthoSlices.h
aipGframeManager.o: ../aip/aipVolData.h ../vnmr/group.h
aipGraphicsWin.o: vjXdef.h ../aip/aipUtils.h ../aip/aipVnmrFuncs.h
aipGraphicsWin.o: ../vnmr/vnmrsys.h ../aip/aipStderr.h
aipGraphicsWin.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipGraphicsWin.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h
aipGraphicsWin.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipGraphicsWin.o: ../aip/aipGraphics.h ../aip/aipGraphicsWin.h
aipGraphicsWin.o: ../aip/aipInterpolation.h ../aip/aipGframeManager.h
aipGraphicsWin.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipGraphicsWin.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipGraphicsWin.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipGraphicsWin.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipGraphicsWin.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipGraphicsWin.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipGraphicsWin.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipGraphicsWin.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipGraphicsWin.o: aipJFuncs.h
aipHistogram.o: ../aip/aipHistogram.h
aipImgInfo.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipImgInfo.o: ../aip/aipStderr.h ../aip/aipVsInfo.h ../aip/sharedPtr.h
aipImgInfo.o: ../aip/aipCStructs.h ../aip/aipMouse.h
aipImgInfo.o: ../aip/aipHistogram.h ../aip/aipImgInfo.h
aipImgInfo.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipImgInfo.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipImgInfo.o: ../aip/aipDataStruct.h ../aip/aipGraphics.h
aipImgInfo.o: ../aip/aipGraphicsWin.h ../aip/aipInterpolation.h
aipInitStart.o: ../aip/aipStderr.h
aipInterface.o: ../aip/aipCommands.h ../aip/aipDataInfo.h
aipInterface.o: ../aip/sharedPtr.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipInterface.o: ../ddl/ddlList.h ../ncomm/mfileObj.h
aipInterface.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipInterface.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipInterface.o: ../aip/aipHistogram.h ../aip/aipDataStruct.h
aipInterface.o: ../aip/aipDataManager.h ../aip/aipImgInfo.h
aipInterface.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipInterface.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipInterface.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipInterface.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipInterface.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipInterface.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipInterface.o: ../aip/aipGframeManager.h ../aip/aipInterface.h
aipInterface.o: ../aip/aipCInterface.h
aipInterpolation.o: ../aip/aipStderr.h ../aip/aipInterpolation.h
aipLine.o: ../aip/aipUtils.h ../aip/aipVnmrFuncs.h vjXdef.h
aipLine.o: ../vnmr/vnmrsys.h ../aip/aipStructs.h ../aip/aipCStructs.h
aipLine.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipLine.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h
aipLine.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipLine.o: ../aip/aipGframe.h ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipLine.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipLine.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipLine.o: ../aip/aipRoi.h ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipLine.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipLine.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipLine.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipLine.o: ../aip/aipLine.h
aipMouse.o: ../aip/aipOrthoSlices.h ../aip/aipBox.h ../aip/aipGframe.h
aipMouse.o: ../aip/sharedPtr.h ../aip/aipViewInfo.h
aipMouse.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipMouse.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipMouse.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipMouse.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h ../aip/aipMouse.h
aipMouse.o: ../aip/aipHistogram.h ../aip/aipDataStruct.h
aipMouse.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipMouse.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipMouse.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipMouse.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipMouse.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipMouse.o: ../aip/aipCommands.h ../aip/aipGframeManager.h
aipMouse.o: ../aip/aipGraphicsWin.h ../aip/aipGraphics.h
aipMouse.o: ../aip/aipInterface.h ../aip/aipCInterface.h
aipMouse.o: ../aip/aipLine.h ../aip/aipOval.h ../aip/aipPoint.h
aipMouse.o: ../aip/aipPolygon.h ../aip/aipEdgelist.h
aipMouse.o: ../aip/aipPolyline.h ../aip/aipRoiManager.h
aipMouse.o: ../aip/aipStderr.h ../aip/aipWinMath.h
aipMouse.o: ../aip/aipParmList.h ../aip/aipMovie.h
aipMouse.o: ../aip/aipWinMovie.h ../aip/aipVolData.h
aipOrthoSlices.o: ../aip/aipGframe.h ../aip/sharedPtr.h
aipOrthoSlices.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipOrthoSlices.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h
aipOrthoSlices.o: ../ddl/ddlNode.h ../ddl/ddlList.h
aipOrthoSlices.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipOrthoSlices.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipOrthoSlices.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipOrthoSlices.o: ../aip/aipDataStruct.h ../aip/aipInterpolation.h
aipOrthoSlices.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h vjXdef.h
aipOrthoSlices.o: ../vnmr/vnmrsys.h ../aip/aipRoiStat.h
aipOrthoSlices.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipOrthoSlices.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipOrthoSlices.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipOrthoSlices.o: ../aip/aipGframeManager.h ../aip/aipStderr.h
aipOrthoSlices.o: ../aip/aipVolData.h ../aip/aipGraphicsWin.h
aipOrthoSlices.o: ../aip/aipGraphics.h ../aip/aipOrthoSlices.h
aipOrthoSlices.o: ../vnmr/group.h ../vnmr/graphics.h
aipOval.o: ../aip/aipUtils.h ../aip/aipStructs.h ../aip/aipCStructs.h
aipOval.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipOval.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h vjXdef.h
aipOval.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipOval.o: ../aip/aipGframe.h ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipOval.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipOval.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipOval.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h ../vnmr/vnmrsys.h
aipOval.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipOval.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipOval.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipOval.o: ../aip/aipGframeManager.h ../aip/aipBox.h ../aip/aipOval.h
aipParmList.o: ../aip/aipParmList.h
aipPoint.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipPoint.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipPoint.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipPoint.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h
aipPoint.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipPoint.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipPoint.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipPoint.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipPoint.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h ../aip/aipRoi.h
aipPoint.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipPoint.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipPoint.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipPoint.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipPoint.o: ../aip/aipPoint.h
aipPolygon.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipPolygon.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipPolygon.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h vjXdef.h
aipPolygon.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipPolygon.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipPolygon.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipPolygon.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipPolygon.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipPolygon.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h ../vnmr/vnmrsys.h
aipPolygon.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipPolygon.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipPolygon.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipPolygon.o: ../aip/aipDataManager.h ../aip/aipGframeManager.h
aipPolygon.o: ../aip/aipRoiManager.h ../aip/aipPolygon.h
aipPolygon.o: ../aip/aipEdgelist.h ../aip/aipPolyline.h
aipPolyline.o: ../aip/aipUtils.h ../aip/aipVnmrFuncs.h vjXdef.h
aipPolyline.o: ../vnmr/vnmrsys.h ../aip/aipStructs.h
aipPolyline.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipPolyline.o: ../aip/sharedPtr.h ../aip/aipMouse.h
aipPolyline.o: ../aip/aipHistogram.h ../aip/aipGraphicsWin.h
aipPolyline.o: ../aip/aipGraphics.h ../aip/aipInterpolation.h
aipPolyline.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipPolyline.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipPolyline.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipPolyline.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipPolyline.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipPolyline.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipPolyline.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipPolyline.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipPolyline.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipPolyline.o: ../aip/aipLine.h ../aip/aipPolygon.h
aipPolyline.o: ../aip/aipEdgelist.h ../aip/aipPolyline.h
aipRoi.o: ../aip/aipStderr.h ../aip/aipVnmrFuncs.h vjXdef.h
aipRoi.o: ../vnmr/vnmrsys.h ../aip/aipGraphicsWin.h
aipRoi.o: ../aip/aipGraphics.h ../aip/aipCStructs.h
aipRoi.o: ../aip/aipStructs.h ../aip/aipVsInfo.h ../aip/sharedPtr.h
aipRoi.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipRoi.o: ../aip/aipInterpolation.h ../aip/aipDataInfo.h
aipRoi.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipRoi.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipRoi.o: ../aip/aipGframe.h ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipRoi.o: ../aip/aipRoi.h ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipRoi.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipRoi.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipRoi.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipRoi.o: ../aip/aipPoint.h ../aip/aipBox.h ../aip/aipPolygon.h
aipRoi.o: ../aip/aipEdgelist.h ../aip/aipPolyline.h ../aip/aipLine.h
aipRoi.o: ../aip/aipOval.h
aipRoiManager.o: ../aip/aipRoi.h ../aip/sharedPtr.h
aipRoiManager.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipRoiManager.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipRoiManager.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipRoiManager.o: ../aip/aipHistogram.h ../aip/aipRoiStat.h
aipRoiManager.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipRoiManager.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipRoiManager.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipRoiManager.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipRoiManager.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipRoiManager.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipRoiManager.o: ../aip/aipPoint.h ../aip/aipGframe.h
aipRoiManager.o: ../aip/aipViewInfo.h ../aip/aipInterpolation.h
aipRoiManager.o: ../aip/aipLine.h ../aip/aipRoiManager.h
aipRoiManager.o: ../aip/aipGframeManager.h ../aip/aipCommands.h
aipRoiManager.o: ../aip/aipInitStart.h
aipRoiStat.o: ../aip/aipUtils.h ../aip/aipGframe.h ../aip/sharedPtr.h
aipRoiStat.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipRoiStat.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipRoiStat.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipRoiStat.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipRoiStat.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipRoiStat.o: ../aip/aipDataStruct.h ../aip/aipInterpolation.h
aipRoiStat.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h vjXdef.h
aipRoiStat.o: ../vnmr/vnmrsys.h ../aip/aipRoiStat.h
aipRoiStat.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipRoiStat.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipRoiStat.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipRoiStat.o: ../aip/aipGframeManager.h ../aip/aipRoiManager.h
aipRoiStat.o: ../aip/aipCommands.h
aipStderr.o: ../aip/aipStderr.h ../aip/aipCommands.h
aipViewInfo.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipViewInfo.o: ../aip/aipStderr.h ../aip/sharedPtr.h
aipViewInfo.o: ../aip/aipGraphicsWin.h ../aip/aipGraphics.h
aipViewInfo.o: ../aip/aipCStructs.h ../aip/aipStructs.h
aipViewInfo.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipViewInfo.o: ../aip/aipHistogram.h ../aip/aipInterpolation.h
aipViewInfo.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipViewInfo.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipViewInfo.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipViewInfo.o: ../aip/aipInterface.h ../aip/aipCInterface.h
aipViewInfo.o: ../aip/aipWinRotation.h ../aip/aipViewInfo.h
aipVnmrFuncs.o: ../vnmr/variables.h ../vnmr/group.h
aipVnmrFuncs.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipVnmrFuncs.o: ../aip/aipMouse.h ../aip/aipGframeManager.h
aipVnmrFuncs.o: ../aip/aipGframe.h ../aip/sharedPtr.h
aipVnmrFuncs.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipVnmrFuncs.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h
aipVnmrFuncs.o: ../ddl/ddlNode.h ../ddl/ddlList.h ../ncomm/mfileObj.h
aipVnmrFuncs.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipVnmrFuncs.o: ../aip/aipVsInfo.h ../aip/aipHistogram.h
aipVnmrFuncs.o: ../aip/aipDataStruct.h ../aip/aipInterpolation.h
aipVnmrFuncs.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipVnmrFuncs.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipVnmrFuncs.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipVnmrFuncs.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipVolData.o: ../aip/aipVolData.h ../aip/aipDataInfo.h
aipVolData.o: ../aip/sharedPtr.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipVolData.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipVolData.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipVolData.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipVolData.o: ../aip/aipDataStruct.h ../aip/aipGframe.h
aipVolData.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipVolData.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipVolData.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipVolData.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipVolData.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipVolData.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipVolData.o: ../aip/aipDataManager.h ../aip/aipGframeManager.h
aipVolData.o: ../aip/aipStderr.h ../aip/aipCommands.h ../vnmr/group.h
aipVolData.o: ../aip/aipCInterface.h ../aip/aipOrthoSlices.h
aipVsInfo.o: ../aip/aipUtils.h ../aip/aipVnmrFuncs.h vjXdef.h
aipVsInfo.o: ../vnmr/vnmrsys.h ../aip/aipCommands.h ../aip/aipStderr.h
aipVsInfo.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h
aipVsInfo.o: ../aip/aipCStructs.h ../aip/aipMouse.h
aipVsInfo.o: ../aip/aipHistogram.h ../aip/aipDataManager.h
aipVsInfo.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipVsInfo.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipVsInfo.o: ../aip/aipDataStruct.h ../aip/aipGframe.h
aipVsInfo.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipVsInfo.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipVsInfo.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipVsInfo.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipVsInfo.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipVsInfo.o: ../aip/aipGframeManager.h
aipWinMath.o: ../vnmr/graphics.h ../vnmr/params.h ../vnmr/process.h
aipWinMath.o: ../aip/aipGframe.h ../aip/sharedPtr.h
aipWinMath.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipWinMath.o: ../aip/aipDataInfo.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipWinMath.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipWinMath.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipWinMath.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipWinMath.o: ../aip/aipDataStruct.h ../aip/aipInterpolation.h
aipWinMath.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h vjXdef.h
aipWinMath.o: ../vnmr/vnmrsys.h ../aip/aipRoiStat.h
aipWinMath.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipWinMath.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipWinMath.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipWinMath.o: ../aip/aipGframeManager.h ../aip/aipWinMath.h
aipWinMath.o: ../aip/aipParmList.h ../aip/aipStderr.h
aipWinMath.o: ../aip/aipInitStart.h ../vnmr/group.h
aipWinMath.o: ../aip/ddlCInterface.h
aipWinMovie.o: vjXdef.h ../aip/aipCommands.h ../aip/aipDataInfo.h
aipWinMovie.o: ../aip/sharedPtr.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipWinMovie.o: ../ddl/ddlList.h ../ncomm/mfileObj.h
aipWinMovie.o: ../aip/aipStructs.h ../aip/aipCStructs.h
aipWinMovie.o: ../aip/aipVsInfo.h ../aip/aipMouse.h
aipWinMovie.o: ../aip/aipHistogram.h ../aip/aipDataStruct.h
aipWinMovie.o: ../aip/aipCFuncs.h ../aip/aipGframeManager.h
aipWinMovie.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipWinMovie.o: ../aip/aipImgInfo.h ../aip/aipInterpolation.h
aipWinMovie.o: ../aip/aipRoi.h ../aip/aipVnmrFuncs.h ../vnmr/vnmrsys.h
aipWinMovie.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipWinMovie.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipWinMovie.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipWinMovie.o: ../aip/aipDataManager.h ../aip/aipWinMovie.h
aipWinMovie.o: ../aip/aipMovie.h aipJFuncs.h
aipMovie.o: vjXdef.h ../aip/aipGraphicsWin.h ../aip/aipGraphics.h
aipMovie.o: ../aip/aipCStructs.h ../aip/aipStructs.h
aipMovie.o: ../aip/aipVsInfo.h ../aip/sharedPtr.h ../aip/aipMouse.h
aipMovie.o: ../aip/aipHistogram.h ../aip/aipInterpolation.h
aipMovie.o: ../aip/aipCommands.h ../aip/aipDataInfo.h
aipMovie.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipMovie.o: ../ncomm/mfileObj.h ../aip/aipDataStruct.h
aipMovie.o: ../aip/aipCFuncs.h ../aip/aipGframeManager.h
aipMovie.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipMovie.o: ../aip/aipImgInfo.h ../aip/aipRoi.h ../aip/aipVnmrFuncs.h
aipMovie.o: ../vnmr/vnmrsys.h ../aip/aipRoiStat.h
aipMovie.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipMovie.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipMovie.o: ../aip/aipDataManager.h ../aip/aipWinMovie.h
aipMovie.o: ../aip/aipMovie.h aipJFuncs.h
aipWinRotation.o: ../aip/aipCommands.h ../aip/aipStderr.h
aipWinRotation.o: ../aip/aipWinRotation.h ../aip/aipImgInfo.h
aipWinRotation.o: ../aip/sharedPtr.h ../aip/aipDataInfo.h
aipWinRotation.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipWinRotation.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipWinRotation.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipWinRotation.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipWinRotation.o: ../aip/aipDataStruct.h ../aip/aipGframeManager.h
aipWinRotation.o: ../aip/aipGframe.h ../aip/aipViewInfo.h
aipWinRotation.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipWinRotation.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipWinRotation.o: ../aip/aipRoiStat.h ../aip/aipReviewQueue.h
aipWinRotation.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipWinRotation.o: ../aip/aipRQgroup.h ../aip/aipRQimage.h
aipWinRotation.o: ../aip/aipDataManager.h ../aip/aipUtils.h
aipWriteData.o: ../aip/aipCommands.h ../aip/aipVnmrFuncs.h vjXdef.h
aipWriteData.o: ../vnmr/vnmrsys.h ../aip/aipUtils.h ../aip/aipGframe.h
aipWriteData.o: ../aip/sharedPtr.h ../aip/aipViewInfo.h
aipWriteData.o: ../aip/aipImgInfo.h ../aip/aipDataInfo.h
aipWriteData.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipWriteData.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipWriteData.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipWriteData.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipWriteData.o: ../aip/aipDataStruct.h ../aip/aipInterpolation.h
aipWriteData.o: ../aip/aipRoi.h ../aip/aipRoiStat.h
aipWriteData.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipWriteData.o: ../aip/aipRQnode.h ../aip/aipRQgroup.h
aipWriteData.o: ../aip/aipRQimage.h ../aip/aipDataManager.h
aipWriteData.o: ../aip/aipGframeManager.h ../aip/aipWriteData.h
aipReviewQueue.o: ../aip/aipRQimage.h ../aip/aipRQnode.h
aipReviewQueue.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipReviewQueue.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipReviewQueue.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipReviewQueue.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipReviewQueue.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipReviewQueue.o: ../aip/aipDataStruct.h ../aip/aipRQgroup.h
aipReviewQueue.o: ../aip/aipRQstudy.h ../aip/aipReviewQueue.h
aipReviewQueue.o: ../aip/aipRQparser.h ../aip/aipDataManager.h
aipReviewQueue.o: ../aip/aipGframeManager.h ../aip/aipGframe.h
aipReviewQueue.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipReviewQueue.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipReviewQueue.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipReviewQueue.o: ../aip/aipRoiStat.h ../aip/aipCommands.h
aipReviewQueue.o: ../aip/aipWinRotation.h ../aip/aipWinMovie.h
aipReviewQueue.o: ../aip/aipVolData.h
aipRQparser.o: ../aip/aipRQparser.h ../aip/aipVnmrFuncs.h vjXdef.h
aipRQparser.o: ../vnmr/vnmrsys.h
aipRQnode.o: ../aip/aipRQnode.h ../aip/aipDataInfo.h
aipRQnode.o: ../aip/sharedPtr.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
aipRQnode.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ../aip/aipStructs.h
aipRQnode.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h ../aip/aipMouse.h
aipRQnode.o: ../aip/aipHistogram.h ../aip/aipDataStruct.h
aipRQimage.o: ../aip/aipRQimage.h ../aip/aipRQnode.h
aipRQimage.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipRQimage.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipRQimage.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipRQimage.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipRQimage.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipRQimage.o: ../aip/aipDataStruct.h ../aip/aipRQgroup.h
aipRQimage.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipRQimage.o: ../aip/aipDataManager.h ../aip/aipVolData.h
aipRQgroup.o: ../aip/aipRQgroup.h ../aip/aipRQnode.h
aipRQgroup.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipRQgroup.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipRQgroup.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipRQgroup.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipRQgroup.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipRQgroup.o: ../aip/aipDataStruct.h ../aip/aipRQimage.h
aipRQgroup.o: ../aip/aipReviewQueue.h ../aip/aipRQstudy.h
aipRQgroup.o: ../aip/aipRQparser.h ../aip/aipDataManager.h
aipRQgroup.o: ../aip/aipGframeManager.h ../aip/aipGframe.h
aipRQgroup.o: ../aip/aipViewInfo.h ../aip/aipImgInfo.h
aipRQgroup.o: ../aip/aipInterpolation.h ../aip/aipRoi.h
aipRQgroup.o: ../aip/aipVnmrFuncs.h vjXdef.h ../vnmr/vnmrsys.h
aipRQgroup.o: ../aip/aipRoiStat.h
aipRQstudy.o: ../aip/aipRQstudy.h ../aip/aipRQnode.h
aipRQstudy.o: ../aip/aipDataInfo.h ../aip/sharedPtr.h
aipRQstudy.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
aipRQstudy.o: ../ncomm/mfileObj.h ../aip/aipStructs.h
aipRQstudy.o: ../aip/aipCStructs.h ../aip/aipVsInfo.h
aipRQstudy.o: ../aip/aipMouse.h ../aip/aipHistogram.h
aipRQstudy.o: ../aip/aipDataStruct.h ../aip/aipRQgroup.h
aipRQstudy.o: ../aip/aipRQimage.h ../aip/aipReviewQueue.h
ddlCInterface.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
ddlCInterface.o: ../ncomm/mfileObj.h ../aip/ddlCInterface.h
aipBox.o: ../aip/aipBox.C
aipCommands.o: ../aip/aipCommands.C
aipDataInfo.o: ../aip/aipDataInfo.C
aipDataManager.o: ../aip/aipDataManager.C
aipEdgelist.o: ../aip/aipEdgelist.C
aipGframeManager.o: ../aip/aipGframeManager.C
aipGframe.o: ../aip/aipGframe.C
aipGraphicsWin.o: ../aip/aipGraphicsWin.C
aipHistogram.o: ../aip/aipHistogram.C
aipImgInfo.o: ../aip/aipImgInfo.C
aipInitStart.o: ../aip/aipInitStart.C
aipInterface.o: ../aip/aipInterface.C
aipInterpolation.o: ../aip/aipInterpolation.C
aipLine.o: ../aip/aipLine.C
aipMouse.o: ../aip/aipMouse.C
aipMovie.o: ../aip/aipMovie.C
aipOrthoSlices.o: ../aip/aipOrthoSlices.C
aipOval.o: ../aip/aipOval.C
aipParmList.o: ../aip/aipParmList.C
aipPoint.o: ../aip/aipPoint.C
aipPolygon.o: ../aip/aipPolygon.C
aipPolyline.o: ../aip/aipPolyline.C
aipReviewQueue.o: ../aip/aipReviewQueue.C
aipRoiManager.o: ../aip/aipRoiManager.C
aipRoi.o: ../aip/aipRoi.C
aipRoiStat.o: ../aip/aipRoiStat.C
aipRQgroup.o: ../aip/aipRQgroup.C
aipRQimage.o: ../aip/aipRQimage.C
aipRQnode.o: ../aip/aipRQnode.C
aipRQparser.o: ../aip/aipRQparser.C
aipRQstudy.o: ../aip/aipRQstudy.C
aipStderr.o: ../aip/aipStderr.C
aipUtils.o: ../aip/aipUtils.C
aipViewInfo.o: ../aip/aipViewInfo.C
aipVnmrFuncs.o: ../aip/aipVnmrFuncs.C
aipVolData.o: ../aip/aipVolData.C
aipVsInfo.o: ../aip/aipVsInfo.C
aipWinMath.o: ../aip/aipWinMath.C
aipWinMovie.o: ../aip/aipWinMovie.C
aipWinRotation.o: ../aip/aipWinRotation.C
aipWriteData.o: ../aip/aipWriteData.C
ddlCInterface.o: ../aip/ddlCInterface.C
#========== ddl ======== 
ddlAction.o: ../ddl/ddlParser.h ../ddl/ddlNode.h ../ddl/ddlList.h
ddlAction.o: ddlGrammar.h
ddlNode.o: ../ddl/ddlList.h ../ddl/ddlNode.h
ddlParser.o: ../ddl/ddlParser.h ../ddl/ddlSymbol.h ../ddl/ddlNode.h
ddlParser.o: ../ddl/ddlList.h ../ncomm/mfileObj.h ddlGrammar.h
ddlParser.o: ../ddl/ddlScanner.h
ddlScanner.o: ../ddl/ddlScanner.h ../ddl/ddlParser.h
ddlScanner.o: ../ddl/ddlSymbol.h ../ddl/ddlNode.h ../ddl/ddlList.h
ddlScanner.o: ../ncomm/mfileObj.h ddlGrammar.h
ddlSymbolTable.o: ../vnmr/vnmrsys.h ../ddl/ddlNode.h ../ddl/ddlList.h
ddlSymbolTable.o: ddlGrammar.h ../ddl/ddlSymbol.h ../ncomm/mfileObj.h
ddlSymbolTable.o: ../ddl/ddlParser.h
ddlAction.o: ../ddl/ddlAction.C
ddlNode.o: ../ddl/ddlNode.C
ddlParser.o: ../ddl/ddlParser.C
ddlScanner.o: ../ddl/ddlScanner.C
ddlSymbolTable.o: ../ddl/ddlSymbolTable.C
