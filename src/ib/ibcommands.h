/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _IBCOMMANDS_H
#define _IBCOMMANDS_H

//
//  Copyright (c) Varian Assoc., Inc.  All Rights Reserved.
//

#include "macroexec.h"		// For "Cmd" typedef

// Declare all the functions
#include "graphics.h"
#include "gframe.h"
#include "gtools.h"
#include "voldata.h"
#include "vs.h"
#include "win_math.h"
#include "win_rotation.h"
#include "win_stat.h"
#include "zoom.h"
int info_write(int argc, char **argv, int retc, char **retv);

Cmd MacroExec::cmd_table[] = {
    {"data_header_set",		Frame_data::SetHdr},
    {"data_load",		Frame_routine::Load},
    {"data_load_all",		Frame_routine::LoadAll},
    {"data_save",		Frame_routine::Save},
    {"display_contrast",	Vscale::Contrast},
    {"display_saturation",	Vscale::Saturate},
    {"display_vs",		Vscale::Vs},
    {"display_vs_bind",		Vscale::Bind},
    {"frame_clear",		Gframe::Clear},
    {"frame_delete",		Gframe::Delete},
    {"frame_load",		Gframe::Load},
    {"frame_save",		Gframe::Save},
    {"frame_select",		Gframe::Select},
    {"frame_split",		Frame_select::Split},
    {"info_save",		info_write},
    {"math",			Win_math::Exec},
    {"memory_warning_threshold", Frame_routine::MemThreshold},
    {"roi_bind",		Roi_routine::Bind},
    {"roi_delete",		Roi_routine::Delete},
    {"roi_load",		Roi_routine::Load},
    {"roi_save",		Roi_routine::Save},
    {"rotate",			Win_rot::Rotate},
    {"stat_bins",		Win_stat::Bins},
    {"stat_dump",		Win_stat::Dump},
    {"stat_print",		Win_stat::Print},
    {"stat_update",		Win_stat::Update},
    {"stat_xcoord",		Win_stat::Xcoord},
    {"stat_ycoord",		Win_stat::Ycoord},
    {"suspend",			MacroExec::Suspend},
    {"tool",			Gtools::Tool},
    {"vol_extract",		VolData::Extract},
    {"vol_mip",			VolData::Mip},
    {"zoom_factor",		Zoomf::Factor},
    {0, 0}
};

#endif /* _IBCOMMANDS_H */
