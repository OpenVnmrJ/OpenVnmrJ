/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.util.*;

/********************************************************** <pre>
 * Summary: Container for static strings used for shuffler.
 *
 </pre> **********************************************************/

public class Shuf {

    /* DB table names */
    public static final String DB_FILE = "file";
    public static final String DB_VNMR_DATA = "vnmr_data";
    public static final String DB_VNMR_RECORD = "vnmr_record";
    public static final String DB_VNMR_REC_DATA = "vnmr_rec_data";
    public static final String DB_VNMR_PAR = "vnmr_par";
    public static final String DB_WORKSPACE = "workspace";
    public static final String DB_PANELSNCOMPONENTS = "panels_n_components";
    public static final String DB_PARAM = "parameter";
    public static final String DB_SHIMS = "shims";
    public static final String DB_PPGM = "pulse_sequences";
    public static final String DB_STUDY = "study";
    public static final String DB_LCSTUDY = "lcstudy";
    public static final String DB_STUDY_DATA = "study_data";
    public static final String DB_PROTOCOL = "protocol";
    public static final String DB_AUTODIR = "automation";
    public static final String DB_AVAIL_SUB_TYPES = "avail_sub_types";
    public static final String DB_PRESCRIPTION = "prescription";
    public static final String DB_GRADIENTS = "gradients";
    public static final String DB_PROCESSING = "processing";
    public static final String DB_IMAGE_DIR = "image_dir";
    public static final String DB_COMPUTED_DIR = "computed_dir";
    public static final String DB_IMAGE_FILE = "image_file";
    public static final String DB_MOLECULE = "molecule";
    public static final String DB_VFS = "vfs";
    public static final String DB_CRFT = "craft";
    public static final String DB_ICON = "icon";
    public static final String DB_TRASH = "trash";
    public static final String DB_VERSION = "version";
    public static final String DB_LINK_MAP = "sym_link_map";
    // DB_MACRO is the ending string from DB_COMMAND_MACRO and DB_PPGM_MACRO
    public static final String DB_MACRO = "_macro";
    public static final String DB_COMMAND_MACRO = "command_n_macro";
    public static final String DB_PPGM_MACRO = "pulse_sequence_macro";
    public static final String[] OBJTYPE_LIST = {DB_PROTOCOL,
                                                 DB_VNMR_PAR,
                                                 DB_VNMR_DATA, 
                                                 DB_VNMR_RECORD,
                                                 DB_VNMR_REC_DATA,
                                                 DB_WORKSPACE,
                                                 DB_PANELSNCOMPONENTS,
                                                 DB_SHIMS, 
                                                 DB_COMMAND_MACRO, 
                                                 DB_PPGM_MACRO,
                                                 DB_STUDY,
                                                 DB_LCSTUDY,
                                                 DB_AVAIL_SUB_TYPES,
                                                 DB_AUTODIR,
                                                 DB_IMAGE_DIR,
                                                 DB_TRASH};
                                

    /** Prefixes and Suffixes */
    public static final String DB_FID_SUFFIX = ".fid";
    public static final String DB_SPC_SUFFIX = ".spc";
    public static final String DB_PAR_SUFFIX = ".par";
    public static final String DB_REC_SUFFIX = ".REC";
    public static final String DB_IMG_DIR_SUFFIX = ".img";
    public static final String DB_CMP_DIR_SUFFIX = ".cmp";
    public static final String DB_IMG_FILE_SUFFIX = ".fdf";
    public static final String DB_PANELSNCOMPONENTS_SUFFIX = ".xml";
    public static final String DB_PROTOCOL_SUFFIX = ".xml";
    public static final String DB_VFS_SUFFIX = ".vfs";
    public static final String DB_CRFT_SUFFIX = ".crft";
    public static final String DB_STUDY_SUFFIX = "";
    public static final String DB_LCSTUDY_SUFFIX = "";
    public static final String DB_WKS_PREFIX = "exp";

    /** If date is not set, then it equals DB_DATE_ALL */
    public static final String DB_DATE_ALL = "date_all";
    public static final String DB_DATE_SINCE = "date_since";
    public static final String DB_DATE_BEFORE = "date_before";
    public static final String DB_DATE_ON = "date_on";
    public static final String DB_DATE_BETWEEN = "date_between";

    public static final String DB_TIME_RUN = "time_run";
    public static final String DB_TIME_IMPORTED = "time_imported";
    public static final String DB_TIME_SAVED = "time_saved";
    public static final String DB_TIME_ARCHIVED = "time_archived";
    public static final String DB_TIME_PUBLISHED = "time_published";

    public static final String DB_SORT_DESCENDING = "DESC";
    public static final String DB_SORT_ASCENDING = "ASC";

    public static final String SEPARATOR = "separator";

    // For access to Vector returned from MountPaths
    public static final int HOST = 0;
    public static final int PATH = 1;
    

    public static ArrayList newlySavedFile=new ArrayList();


}
