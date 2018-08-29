/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipCFuncs.h"
#include "aipCommands.h"
#include "aipVnmrFuncs.h"
#include "aipDataManager.h"
#include "aipGframeManager.h"
#include "aipInterface.h"
#include "aipMouse.h"
#include "aipRoiManager.h"
#include "aipRoiStat.h"
#include "aipStderr.h"
#include "aipWinMath.h"
#include "aipWinRotation.h"
#include "aipWinMovie.h"
#include "aipMovie.h"
#include "aipWriteData.h"
#include "aipVolData.h"
#include "aipVsInfo.h"
#include "aipReviewQueue.h"
#include "aipOrthoSlices.h"
#include "aipSpecDataMgr.h"
#include "aipSpecViewMgr.h"
#include "AspFrameMgr.h"
#include "AspMouse.h"
#include "AspDis1D.h"
#include "AspDisPeaks.h"
#include "AspDisInteg.h"
#include "AspDisAnno.h"
#include "AspDisRegion.h"

using namespace aip;

typedef struct {
    const char *useName;
    int (*func)(int argc, char *argv[], int retc, char *retv[]);
    int graphcmd;
    int redoType;
} cmd_t;

namespace {
    // List of AIP commands:
    cmd_t cmds[] = {
	{"aipBigFrame",		GframeManager::aipFullScreen, NO_REEXEC, 0},
	{"aipClearFrames",	GframeManager::aipClearFrames, NO_REEXEC, 0},
	{"aipDeleteData", 	DataManager::aipDeleteData, NO_REEXEC, 0},
	{"aipDeleteFrames",	GframeManager::aipDeleteFrames, NO_REEXEC, 0},
	{"aipDeleteRois",	RoiManager::aipDeleteRois, NO_REEXEC, 0},
	{"aipDisplay",	 	DataManager::aipDisplay, NO_REEXEC, 0},
	{"aipDupFrame",		GframeManager::aipDupFrame, NO_REEXEC, 0},
    {"aipShow3Planes",  VolData::show3Planes, NO_REEXEC, 0},
	{"aipExtract",	 	VolData::Extract, NO_REEXEC, 0},
	{"aipExtractObl",	VolData::ExtractObl, NO_REEXEC, 0},
	{"aipStartExtract",	OrthoSlices::StartExtract, NO_REEXEC, 0},
    {"aipSetg3dpnt",    OrthoSlices::Setg3dpnt, NO_REEXEC, 0},
    {"aipSetg3drot",    OrthoSlices::Setg3drot, NO_REEXEC, 0},
    {"aipSetg3df",      OrthoSlices::Setg3dflt, NO_REEXEC, 0},
    {"aipRedrawMip",    OrthoSlices::RedrawMip, NO_REEXEC, 0},
    {"aipShow3PCursors",    OrthoSlices::Show3PCursors, NO_REEXEC, 0},
	{"aip2Dto3D",	 	DataManager::aip2Dto3D, NO_REEXEC, 0},	
	{"aipExtractMip",	VolData::Mip, NO_REEXEC, 0},
	{"aipFlip",	 	    WinRotation::aipRotate, NO_REEXEC, 0},
	{"aipLoadDir",		DataManager::aipLoadDir, NO_REEXEC, 0},
	{"aipLoadFile",	 	DataManager::aipLoadDir, NO_REEXEC, 0},
	{"aipMathExecute",	WinMath::aipMathExecute, NO_REEXEC, 0},
	{"aipPrintImage",	GframeManager::aipPrintImage, NO_REEXEC, 0},
	{"aipRedisplay",	GframeManager::aipRedisplay, REEXEC, 1},
	{"aipRotate",	 	WinRotation::aipRotate, NO_REEXEC, 0},
	{"aipSaveHeaders",	DataManager::aipSaveHeaders, NO_REEXEC, 0},
	{"aipSaveVs",	        VsInfo::aipSaveVs, NO_REEXEC, 0},
	{"aipSegment",	 	RoiStat::aipSegment, NO_REEXEC, 0},
	{"aipSelectFrames",	GframeManager::aipSelectFrames, NO_REEXEC, 0},
	{"aipSelectRois",	RoiManager::aipSelectRois, NO_REEXEC, 0},
	{"aipSetDebug",	 	aipSetDebug, NO_REEXEC, 0},
	{"aipSetExpression", 	WinMath::aipSetExpression, NO_REEXEC, 0},
	{"aipSetState",	 	Mouse::aipSetState, NO_REEXEC, 0},
	{"aipSetVsFunction", 	VsInfo::aipSetVsFunction, NO_REEXEC, 0},
	{"aipSomeInfoUpdate", 	RoiManager::aipSomeInfoUpdate, NO_REEXEC, 0},
	{"aipSplitWindow",	GframeManager::aipSplitWindow, NO_REEXEC, 0},
	{"aipStatPrint",	RoiStat::aipStatPrint, NO_REEXEC, 0},
	{"aipStatUpdate",	RoiStat::aipStatUpdate, NO_REEXEC, 0},
	{"aipTestListener",	aipTestDisplayListener, NO_REEXEC, 0},
	{"aipTestMouseListener", aipTestMouseListener, NO_REEXEC, 0},
	{"aipWriteData", 	WriteData::writeData, NO_REEXEC, 0},
	{"startMovie",	 	WinMovie::startMovie, NO_REEXEC, 0},
	{"continueMovie",	WinMovie::continueMovie, NO_REEXEC, 0},
	{"stepMovie",	 	WinMovie::stepMovie, NO_REEXEC, 0},
	{"stopMovie",	 	WinMovie::stopMovie, NO_REEXEC, 0},
	{"resetMovie",	 	WinMovie::resetMovie, NO_REEXEC, 0},
	{"aipGetPrintFrames",   GframeManager::aipGetPrintFrames, NO_REEXEC, 0},
	{"aipSaveRois",         RoiManager::aipSaveRois, NO_REEXEC, 0},
	{"aipLoadRois",         RoiManager::aipLoadRois, NO_REEXEC, 0},
	{"aipGetHeaderParam",   DataManager::aipGetHeaderParam, NO_REEXEC, 0},
	{"aipGetHeaderString",  DataManager::aipGetHeaderString, NO_REEXEC, 0},
	{"aipGetHeaderInt",     DataManager::aipGetHeaderInt, NO_REEXEC, 0},
	{"aipGetHeaderReal",    DataManager::aipGetHeaderReal, NO_REEXEC, 0},
	{"aipNumOfImgs",        DataManager::aipNumOfImgs, NO_REEXEC, 0},
	{"aipNumOfCopies",      DataManager::aipNumOfCopies, NO_REEXEC, 0},
	{"aipLoadImgList",	DataManager::aipLoadImgList, NO_REEXEC, 0},
	{"aipGetFrame",		GframeManager::aipGetFrame, NO_REEXEC, 0},
	{"aipGetDataKey",	GframeManager::aipGetDataKey, NO_REEXEC, 0},
	{"aipGetFrameToStart",	GframeManager::aipGetFrameToStart, NO_REEXEC, 0},
	{"aipRQcommand",     	ReviewQueue::aipRQcommand, NO_REEXEC, 4},
	{"aipShowSpec",         SpecViewMgr::aipShowSpec, NO_REEXEC, 0},
	{"aipShowCSIData",         SpecViewMgr::aipShowSpec, NO_REEXEC, 0},
	{"aipLoadSpec",            SpecDataMgr::aipLoadSpec, NO_REEXEC, 0},
	{"aipRemoveSpec",            SpecDataMgr::aipRemoveSpec, NO_REEXEC, 0},
	{"aipGetSelectedKeys",  GframeManager::aipGetSelectedKeys, NO_REEXEC, 0},
	{"aipGetImgKey",        DataManager::aipGetImgKey, NO_REEXEC, 0},
	{"aipScreen",           aipScreen, NO_REEXEC, 0},
	{"aipIsIplanObj",       GframeManager::aipIsIplanObj, NO_REEXEC, 0},
	{"aipMovie",            Movie::doIt, NO_REEXEC, 0},
	{"aipSetColormap",      GframeManager::aipSetColormap, NO_REEXEC, 0},
	{"aipSaveColormap",     GframeManager::aipSaveColormap, NO_REEXEC, 0},
	{"aipSetTransparency",      GframeManager::aipSetTransparency, NO_REEXEC, 0},
	{"aipViewLayers",      GframeManager::aipViewLayers, NO_REEXEC, 0},
	{"aipOverlayFrames",      GframeManager::aipOverlayFrames, NO_REEXEC, 0},
	{"aipMakeMaps",      SpecViewMgr::aipMakeMaps, NO_REEXEC, 0},
	{"aipMoveOverlay",      GframeManager::aipMoveOverlay, NO_REEXEC, 0},
	{"aipOverlayGroup",      GframeManager::aipOverlayGroup, NO_REEXEC, 0},
	{"aspFrame",      AspFrameMgr::aspFrame, NO_REEXEC, 0},
	{"aspRoi",      AspFrameMgr::aspRoi, NO_REEXEC, 0},
	{"asp1D",      AspDis1D::asp1D, REEXEC, 0},
	{"aspSetState",      AspMouse::aspSetState, NO_REEXEC, 0},
	{"aspSession",      AspFrameMgr::aspSession, NO_REEXEC, 0},
	{"aspPeaks",      AspDisPeaks::aspPeaks, NO_REEXEC, 0},
	{"aspInteg",      AspDisInteg::aspInteg, NO_REEXEC, 0},
	{"aspAnno",      AspDisAnno::aspAnno, NO_REEXEC, 0},
	{"aspRegion",      AspDisRegion::aspRegion, NO_REEXEC, 0},
	{NULL,			NULL, NO_REEXEC, 0}
    };
}

void *
aipGetCommandTable()
{
    return (void *)cmds;
}
