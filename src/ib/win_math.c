/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/


/* 
 */

#include "stderr.h"
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/wmgr.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <dlfcn.h>		// Dynamic loading

#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "zoom.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"
#include "filelist.h"
#include "win_math.h"
#include "macroexec.h"
#include "parmlist.h"
#include "ddllib.h"

/* TEST FUNCTION /*CMP*/
extern "C" {
int ib_msgline(char *message);
int ib_infomsg(char *message);
int ib_errmsg(char *message);
}
extern void FlushGraphics();/*CMP*/

extern short xview_to_ascii(short);
extern void win_print_msg(char *, ...);
extern Gframe *framehead;

#ifdef LINUX
#define MAXNAMELEN 512
#endif

static Win_math *wm=NULL;

/************************************************************************
*                                                                       *
*  Show the window.
*									*/
void
winpro_math_show(void)
{
    // Create window object only if it is not created
    if (wm == NULL){
	wm = new Win_math;
    }
    wm->show_window();
}

/************************************************************************
*                                                                       *
*  Insert stuff into the expression
*									*/
void
winpro_math_insert(char *text, int isimage)
{
    if (wm){
	wm->math_insert(text, isimage);
    }
}

/************************************************************************
*                                                                       *
*  Create the window
*									*/
Win_math::Win_math(void)
{
    Panel panel;		// panel
    Panel_item item;	// Panle item
    int xpos, ypos;      // window position
    char initname[128];	// init file
    Frame get_base_frame();/*CMP*/

    busy = FALSE;
    //dst_frame2file = NULL;
    //src_frame2file = NULL;

    // Get the initialized file of window position
    (void)init_get_win_filename(initname);

    // Get the position of the control panel
    if (init_get_val(initname, "WINPRO_MATH", "dd", &xpos, &ypos) == NOT_OK){
	xpos = 400;
	ypos = 40;
    }

    frame = get_base_frame();

    popup = (Frame)xv_create(frame, FRAME_CMD,
			     XV_X,		xpos,
			     XV_Y,		ypos,
			     FRAME_LABEL,	"Image Math",
			     FRAME_DONE_PROC,	&Win_math::done_proc,
			     FRAME_CMD_PUSHPIN_IN,	TRUE,
			     FRAME_CMD_DEFAULT_PIN_STATE, FRAME_CMD_PIN_IN,
			     FRAME_SHOW_RESIZE_CORNER, TRUE,
			     NULL);
    
    panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);
    xv_set(panel, PANEL_LAYOUT, PANEL_HORIZONTAL, NULL);

    /*Selection_requestor sel =
    xv_create(panel, SELECTION_REQUESTOR,
	      SEL_REPLY_PROC, &Win_math::selection_proc,
	      NULL);*/

    item = xv_create(panel, PANEL_BUTTON,
		     PANEL_NEXT_ROW, -1,
		     PANEL_LABEL_STRING, "Execute",
		     PANEL_NOTIFY_PROC, &Win_math::execute,
		     NULL);
    xv_set(panel, PANEL_DEFAULT_ITEM, item, NULL);

    Menu menu = xv_create(NULL, MENU,
			  MENU_GEN_PROC, &Win_math::math_menu_load_pullright,
			  NULL);
    item = xv_create(panel, PANEL_BUTTON,
		     PANEL_NEXT_ROW, -1,
		     PANEL_LABEL_STRING, "Expression",
		     PANEL_ITEM_MENU, menu,
		     NULL);

    /*window_fit(panel);*/
    window_fit_height(panel);

    expression_box =
    xv_create(popup, TEXTSW,
	      XV_X, 0,
	      WIN_BELOW, panel,
	      WIN_WIDTH, WIN_EXTEND_TO_EDGE,
	      WIN_ROWS, 3,
	      NULL);

    /*xv_set(panel, WIN_BELOW, expression_box, NULL);*/

    window_fit(popup);
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_math::~Win_math(void)
{
    /*xv_destroy_safe(frame);*/
}

/************************************************************************
*									*
*  Execute an image math expression
*  [MACRO interface]
*  argv[0]: (string) the expression to be executed
*  [STATIC Function]							*
*									*/
int
Win_math::Exec(int argc, char **argv, int, char **)
{
    if (argc != 2){
	msgerr_print("Usage: %s('expression')", *argv);
	ABORT;
    }
    argc--; argv++;
    if (!wm){
	wm = new Win_math;
    }
    exec_string(*argv);
    return PROC_COMPLETE;
}

/************************************************************************
*                                                                       *
*  Insert stuff into the expression box
*									*/
void
Win_math::math_insert(char *text, int isimage)
{
    char chr;
    char *cmd;
    int len;
    Textsw_index pnt;

    if (isimage){
	// Check to see if there is a "#" before the insertion point.
	pnt = (Textsw_index)xv_get(expression_box, TEXTSW_INSERTION_POINT);
	if (pnt){
	    xv_get(expression_box, TEXTSW_CONTENTS, pnt-1, &chr, 1);
	}
	if (pnt == 0 || chr != '#'){
	    // Put in a "#"
	    textsw_insert(expression_box, "#", 1);
	}

	// Check for digits after the insertion point
	pnt = (Textsw_index)xv_get(expression_box, TEXTSW_INSERTION_POINT);
	len = (int)xv_get(expression_box, TEXTSW_LENGTH);
	cmd = (char *)malloc(len + 1);
	xv_get(expression_box, TEXTSW_CONTENTS, 0, cmd, len);
	cmd[len] = '\0';
	len = strspn(cmd+pnt, "0123456789");
	if (len){
	    textsw_delete(expression_box, pnt, pnt+len);
	}
	free(cmd);
    }

    // Put in the text
    textsw_insert(expression_box, text, strlen(text));

    if (isimage){
	// Advance cursor past next "#", if there is one
	len = (int)xv_get(expression_box, TEXTSW_LENGTH);
	cmd = (char *)malloc(len + 1);
	xv_get(expression_box, TEXTSW_CONTENTS, 0, cmd, len);
	cmd[len] = '\0';
	pnt = (Textsw_index)xv_get(expression_box, TEXTSW_INSERTION_POINT);
	char *pc = strstr(cmd+pnt, "#");
	if (pc){
	    xv_set(expression_box, TEXTSW_INSERTION_POINT, pc-cmd+1, NULL);
	}
	free(cmd);
    }
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  (STATIC)								*
*									*/
void
Win_math::done_proc(Frame subframe)
{
    xv_set(subframe, XV_SHOW, FALSE, NULL);
    delete wm;
    wm = NULL;
    win_print_msg("Math: Exit");
}

/************************************************************************
*                                                                       *
*  Expression text callback routine.
*  (STATIC)								*
*									*/
void
Win_math::text_proc(Panel_item item, Event *event)
{
    short action = event_action(event);
    short chr = xview_to_ascii(action);
    fprintf(stderr,"char=0x%x\n", chr);
    /*fprintf(stderr,"posn=%d\n",
	    (int)xv_get(item, PANEL_TEXT_CURSOR));*/
    xv_set(item, PANEL_TEXT_SELECT_LINE, NULL);

#ifndef LINUX
    textsw_set_selection(wm->expression_box, 1, 4, 1);
#endif
    xv_set(wm->expression_box, TEXTSW_INSERTION_POINT, 4, NULL);
}


/************************************************************************
*                                                                       *
*  Generate the menu of expressions/functions
*
*  STATIC
*									*/
Menu
Win_math::math_menu_load_pullright(Menu_item mi, Menu_generate op)
{
    if (op == MENU_DISPLAY){
	char fname[MAXPATHLEN];
	(void)init_get_env_name(fname);	/* Get the directory path */
	(void)strcat(fname, "/math/expressions/bin");
	return(filelist_menu_pullright(mi, op, (u_long)math_menu_load, fname));
    }
    return(filelist_menu_pullright(mi, op, NULL, NULL));
}

/************************************************************************
*                                                                       *
*  Load the name of the selected file
*
*  STATIC
*									*/
void
Win_math::math_menu_load(char *dir, char *file)
{
    char *buf = (char *)malloc(strlen(file) + 20);
    sprintf(buf,"# = %s", file);
    xv_set(wm->expression_box,
	   TEXTSW_CONTENTS, buf,
	   TEXTSW_INSERTION_POINT, 1,
	   NULL);
    free(buf);

    /* If the previous cursor was hidden by the pop-up menu, it is not erased,
     * so refresh the window to get rid of it. */
    wmgr_refreshwindow(wm->expression_box);

    Gtools::select_tool(0, GTOOL_MATH);
}

/************************************************************************
*                                                                       *
*  Process math.							*
*  (STATIC)								*
*									*/
void
Win_math::execute(Panel_item item)
{
    /* Get the command string */
    int len = (int)xv_get(wm->expression_box, TEXTSW_LENGTH);
    char *cmd = (char *)malloc(len + 1);
    xv_get(wm->expression_box, TEXTSW_CONTENTS, 0, cmd, len);
    cmd[len] = '\0';
    exec_string(cmd);
}

/************************************************************************
*                                                                       *
*  Utility function: append a string to a possibly existing string.
*  If "oldstr" is non-NULL, it must point to malloc'ed memory.
*  (STATIC)								*
*									*/
char *
Win_math::append_string(char *oldstr, char *newstr)
{
    int len = strlen(newstr);
    if (oldstr){
	len += strlen(oldstr);
    }
    char *pc = (char *)realloc(oldstr, len + 1);
    if (!oldstr){
	*pc = '\0';
    }
    strcat(pc, newstr);
    return pc;
}

/************************************************************************
*                                                                       *
*  Utility function: append a string of given length to a possibly
*  existing string
*  If "oldstr" is non-NULL, it must point to malloc'ed memory.
*  (STATIC)								*
*									*/
char *
Win_math::append_string(char *oldstr, char *newstr, int nchrs)
{
    int len = nchrs;
    if (oldstr){
	len += strlen(oldstr);
    }
    char *pc = (char *)realloc(oldstr, len + 1);
    if (!oldstr){
	*pc = '\0';
    }
    strncat(pc, newstr, nchrs);
    return pc;
}

/************************************************************************
*                                                                       *
*  Utility function: turn a command string into C code
*  (STATIC)								*
*									*/
char *
Win_math::make_c_expression(char *cmd)
{
    int i;
    int n;
    char buf[100];
    char *pc;
    char *pp;
    char *rtn = NULL;
    char frame[20];		// Holds frame nbr parsed from command
    int frameno;

    /*fprintf(stderr,"make_c_expression(\"%s\", \"%s\")\n",
	    cmd, name2frame);/*CMP*/
    // Parse past the "="
    pc = strchr(cmd, '=');
    if (!pc){
	fprintf(stderr,"No equals\n");
	return NULL;
    }
    pc++;

    // Replace all "#<frame>" strings with "img[<name>][indx]"
    for (i=0; pp = strchr(pc, '#'); i++){
	rtn = append_string(rtn, pc, pp-pc);
	pc = pp + 1;		// Skip the '#'
        n = strspn(pc, "0123456789"); // Skip the frame number
	pc += n;
	sprintf(buf,"img[%d][indx]", i);
	rtn = append_string(rtn, buf);
    }
    rtn = append_string(rtn, pc);

    return rtn;
}

/************************************************************************
*                                                                       *
*  Utility function: Turn a user function command into a file name
*  (STATIC)								*
*									*/
char *
Win_math::func2progname(char *cmd)
{
    char *pin;
    char *pout;
    char *rtn = NULL;
    int n;

    // Skip past "="
    pin = strchr(cmd, '=');
    if (!pin){
	fprintf(stderr,"func2progname: no '='\n");
	return rtn;
    }
    pin += strspn(pin, "= \t\n\r");
    n = strcspn(pin, " \t\n\r(");
    if (n){
	rtn = (char *)malloc(n+1);
	if (rtn){
	    strncpy(rtn, pin, n);
	    rtn[n] = '\0';
	}
    }

    return rtn;
}

/************************************************************************
*                                                                       *
*  Utility function: Turn a user function name into a file path
*  (STATIC)								*
*									*/
char *
Win_math::func2path(char *progname)
{
    char *progpath = (char *)malloc(MAXPATHLEN);
    if (!progpath){
	free(progname);
	return NULL;
    }
    (void)init_get_env_name(progpath);	/* Get the directory path */
    strcat(progpath, "/math/functions/bin/");
    strcat(progpath, progname);
    if (access(progpath, X_OK) != 0){
	/*msgerr_print("Math: Function \"%s\" not found in \"%s\"",
		     progname, progpath);*/
	free(progname);
	free(progpath);
	return NULL;
    }

    return progpath;
}

/************************************************************************
*                                                                       *
*  Utility function: Turn an expression command into a file name
*  (STATIC)								*
*									*/
char *
Win_math::expr2progname(char *cmd)
{
    char *pin;
    char *pout;
    char *rtn;
    int n;

    // Skip past "="
    pin = strchr(cmd, '=');
    if (!pin){
	fprintf(stderr,"expr2progname: no '='\n");
	return NULL;
    }
    pin++;

    // For rest of expression, delete whitespace and frame numbers and
    //  convert " / " to " \ "
    pout = rtn = (char *)malloc(strlen(pin) + 1);
    while (*pin){
	if (!strchr(" \t\n", *pin)){ // Skip white space
	    if (*pin == '/'){
		*pout++ = '\\';	// Convert " / " to " \ "
	    }else{
		*pout++ = *pin;	// Write out the character
	    }
	}
	// Increment pin appropriately
	if (*pin == '#'){
	    // Skip frame number
	    n = strspn(++pin, "0123456789");
	    pin += n;
	}else{
	    pin++;
	}
    }
    *pout = '\0';
    if (strlen(rtn) >= MAXNAMELEN){
	msgerr_print("Math: Expression is more than %d characters", MAXNAMELEN);
	free(rtn);
	return NULL;
    }
    return rtn;
}

/************************************************************************
*                                                                       *
*  Utility function: Perform string substitution in a file with
*   output to another file.
*  (STATIC)								*
*									*/
int
Win_math::sub_string_in_file(char *in_file, char *out_file,
			     char *in_sub, char *out_sub)
{
    char buf[1024];
    char *pc;

    FILE *fdin = fopen(in_file, "r");
    if (!fdin){
	msgerr_print("Math: Cannot open %s for reading", in_file);
	return FALSE;
    }
    FILE *fdout = fopen(out_file, "w");
    if (!fdout){
	msgerr_print("Math: Cannot open %s for writing", out_file);
	fclose(fdin);
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fdin)){
	if (buf[strlen(buf)-1] != '\n'){
	    msgerr_print("Math: Input line too long: %.40s...", buf);
	    fclose(fdin);
	    fclose(fdout);
	    return FALSE;
	}
	if (!(pc=strstr(buf, in_sub))){
	    fputs(buf, fdout);
	}else{
	    *pc = '\0';
	    fputs(buf, fdout);
	    fputs(out_sub, fdout);
	    fputs(pc+strlen(in_sub), fdout);
	}
    }
    fclose(fdin);
    fclose(fdout);
    return TRUE;
}

int
ib_msgline(char *message)
{
    win_print_msg(message);
    return 0;
}

int
ib_infomsg(char *message)
{
    msginfo_print(message);
    return 0;
}

int
ib_errmsg(char *message)
{
    msgerr_print(message);
    return 0;
}

/************************************************************************
*                                                                       *
*  Process the Image Math string "cmd".
*  (STATIC)								*
*									*/
void
Win_math::exec_string(char *cmd)
{
    int i;
    int n;
    int err = FALSE;
    char *pc;

    // The following get pointed to mallocated memory
    ParmList dst_frames = NULL;
    ParmList dst_ddls = NULL;
    ParmList src_ddls = NULL;
    ParmList src_ddl_vecs = NULL;
    ParmList src_strings = NULL;
    ParmList src_constants = NULL;
    ParmList parmtree = NULL;
    char *exec_path = NULL;

    win_print_msg("Math: Parsing...");

    /* Change New-Lines to Spaces */
    while (pc=strchr(cmd, '\n')){
	*pc = ' ';
    }

    /* Parse the left hand side */
    int nout = parse_lhs(cmd, &dst_frames);
    if (!nout){
	err = TRUE;
    }
    /*printParm(dst_frames);/*CMP*/

    /* Parse images on right hand side */
    int nin = parse_rhs(cmd, &src_ddls, &src_ddl_vecs,
			&src_strings, &src_constants);
    if (!nin){
	err = TRUE;
    }
    /*printParm(src_ddls);/*CMP*/
    /*printParm(src_strings);/*CMP*/

    /* Get the executable, compiling if necessary */
    if (!err){
	win_print_msg("Math: Compiling program...");
	if (!(exec_path = get_program(cmd))){
	    err = TRUE;
	}
    }
    /*fprintf(stderr,"exec_path=%s\n", exec_path);/*CMP*/

    /* Execute the program */
    parmtree = allocParm("parmtree", PL_PARM, 5);
    setParmParm(parmtree, src_ddls, 0);
    setParmParm(parmtree, dst_frames, 1);
    setParmParm(parmtree, src_strings, 2);
    setParmParm(parmtree, src_constants, 3);
    setParmParm(parmtree, src_ddl_vecs, 4);
    /*printParm(parmtree);/*CMP*/
    if (!err){
	win_print_msg("Math: Executing program...");
	if (!exec_program(exec_path, parmtree, &dst_ddls)){
	    err = TRUE;
	}
    }

    /* Display the results */
    /*printParm(dst_ddls);/*CMP*/
    void *vst;
    int frame;
    Gframe *gf;
    n = countParm(dst_ddls);
    for (i=0; i<n; i++){
	getPtrParm(dst_ddls, &vst, i);
	DDLSymbolTable *st = (DDLSymbolTable *)vst;
	getIntParm(dst_frames, &frame, i);
	/*fprintf(stderr,"st=0x%x, frame=%d\n", st, frame);/*CMP*/
	if (st && frame){
	    char *fname;
	    st->GetValue("filename", fname);
	    char *newname = (char *)malloc(strlen(fname) + 20);
	    sprintf(newname,"%s-mathout#%d", fname, i+1);
	    st->SetValue("filename", newname);
	    free(newname);
	    gf = Gframe::get_frame_by_number(frame);
	    int display_data = TRUE;
	    int math_result = TRUE;
	    Frame_data::load_ddl_data(gf, NULL, NULL, &display_data, TRUE,
				      (DDLSymbolTable *)st, math_result);
	}
    }

    /* Free memory */
    free(parmtree);		// Also frees params under it
    free(dst_ddls);

    win_print_msg("Math: Done.");
}

/************************************************************************
*                                                                       *
*  Parse the LHS of math expression "cmd".
*  Returns number of frames in list.
*  (STATIC)								*
*									*/
int
Win_math::parse_lhs(char *cmd, ParmList *framelist)
{
    int nframes = 0;
    char *tbuf;
    int pid = getpid();

    tbuf = strdup(cmd);
    if (!tbuf) return 0;

    char *token = strtok(tbuf, "=");
    if (token){
	// Where to put the results
	*framelist = get_framevector("dst_frames", &token);
	nframes = countParm(*framelist);
	if (nframes < 1){
	    msgerr_print("Math: Illegal expression: no output frames.");
	    return 0;
	}
    }
    free(tbuf);
    return nframes;
}

/************************************************************************
*                                                                       *
*  Parse the RHS of math expression "cmd".
*  Returns number of source images
*  (STATIC)								*
*									*/
int
Win_math::parse_rhs(char *cmd,
		    ParmList *ddls,
		    ParmList *ddlvecs,
		    ParmList *strings,
		    ParmList *constants)
{
    int i;
    int j;
    int m;
    int n;
    char *pc;
    char *tbuf;
    ParmList frames;
    ParmList pl;
    void *pv;
    Imginfo *img;

    pc = strchr(cmd, '=');
    if (!pc){
	return 0;
    }

    tbuf = strdup(++pc);	// tbuf contains the RHS
    if (!tbuf) return 0;

    *ddlvecs = get_imagevector_list("src_ddlvecs", tbuf); // frame nbrs stripped

    *ddls = allocParm("src_ddls", PL_PTR, 0);
    n = countParm(*ddlvecs);
    for (i=0; i<n; i++){
	getParmParm(*ddlvecs, &pl, i);
	m = countParm(pl);
	for (j=0; j<m; j++){
	    getPtrParm(pl, &pv, j);
	    *ddls = appendPtrParm(*ddls, pv);
	}
    }
    /*fprintf(stderr,"cmd w/o frames = \"%s\"\n", tbuf);/*CMP*/

    *strings = get_stringlist("src_strings", tbuf); // strings stripped off
    /*fprintf(stderr,"cmd w/o strings = \"%s\"\n", tbuf);/*CMP*/

    *constants = get_constlist("src_constants", tbuf); // tbuf unchanged
    /*fprintf(stderr,"cmd w/ constants = \"%s\"\n", tbuf);/*CMP*/

    n = countParm(*ddls);
    if (!n){
	msgerr_print("Math: no input image(s) specified");
	freeParms(*ddls);
	*ddls = NULL;
	return 0;
    }
    free(tbuf);
    return n;
}

/************************************************************************
*                                                                       *
*  Get the name of the executable file, compiling if necessary.
*  Return file path, or NULL on failure.
*  (STATIC)								*
*									*/
char *
Win_math::get_program(char *cmd)
{
    int i;

    char *progpath = NULL;
    char *progname = NULL;

    char *dir = "/tmp";		// Directory for temp image files
    int pid = getpid();

    if ((progname=func2progname(cmd)) && (progpath=func2path(progname))){
	/* We got a user program */
    }else if ( (progname=expr2progname(cmd)) == NULL){
	msgerr_print("Math: Could not make prog name from \"%s\"", cmd);
	return NULL;
    }else{
	// This was an expression
	/*fprintf(stderr,"progname=\"%s\"\n", progname);/*CMP*/

	// See if program already exists
	progpath = (char *)malloc(MAXPATHLEN);
	if (!progpath){
	    free(progname);
	    return NULL;
	}
	(void)init_get_env_name(progpath);	/* Get the directory path */
	strcat(progpath, "/math/expressions/bin/");
	strcat(progpath, progname);
	int tstfd;
	// NB: *** NEED TO CHECK IF PROGRAM IS UP TO DATE
	if (access(progpath, X_OK) == 0){
	    // Already have the program
	    /*fprintf(stderr,"prog exists\n");/*CMP*/
	}else{
	    // Need to compile the program
	    char *c_expr = NULL;
	    win_print_msg("Math: Compiling...");

	// Turn the given expression into C code
	/*fprintf(stderr,"cmd=\"%s\"\n", cmd);/*CMP*/
	/*fprintf(stderr,"&src_name2frame=0x%x\n", src_name2frame);/*CMP*/
	/*fprintf(stderr,"src_name2frame=\"%s\"\n", src_name2frame);/*CMP*/
	    c_expr = make_c_expression(cmd);
	    if (!c_expr){
		msgerr_print("Math: Could not decode expression: \"%s\"", cmd);
		free(progname);
		return NULL;
	    }
	    /*fprintf(stderr,"cmd=\"%s\"\nc_expr=\"%s\"\n", cmd, c_expr);/*CMP*/

	// Insert the expression into the canned program
	    char protopath[MAXPATHLEN];
	    (void)init_get_env_name(protopath);
	    strcat(protopath, "/math/expressions/src/mathproto.c");
	    char srcpath[MAXPATHLEN];
	    sprintf(srcpath,"%s/%d_c_code.c", dir, pid);
	    if (!sub_string_in_file(protopath, srcpath,
				    "IB_EXPRESSION", c_expr)){
		msgerr_print("Math: Could not write C program");
		free(progname);
		free(c_expr);
		return NULL;
	    }
	    free(c_expr);

	// Compile program into math directory
	    char cccmd[2048];
	    char vnmrlib[1024];
	    char gnuinc[1024];
	    char srcinc[1024];
	    char ccprog[1024];
	    char libs[] = "-lddl -lnsl -lC -lm";
	    char *pc;
	    int no_gcc;

	    (void)init_get_env_name(srcinc);
	    strcat(srcinc,"/math/expressions/src");
	    if (!(pc=getenv("vnmrsystem"))){
		pc = "/vnmr";
	    }
	    sprintf(vnmrlib,"%s/lib", pc);
	    sprintf(ccprog,"%s/gnu/bin/cc", pc); // Use Gnu cc if we have it ...
	    if ((no_gcc = access(ccprog, X_OK)) != 0){
		strcpy(ccprog, "cc"); // ... otherwise, whatever we find.
	    }
	    if (!(pc=getenv("GCC_EXEC_PREFIX"))){
		pc = "./";
		char *msg="Environment variable \"GCC_EXEC_PREFIX\" undefined";
		if (!no_gcc){
		    msgerr_print(msg);
		}
	    }
	    sprintf(gnuinc,"%sinclude", pc);

	    char makeexpr[1024];
	    char makestr[1024];
	    sprintf(makeexpr,"%s/makeexpr", srcinc);
	    FILE *fd = fopen(makeexpr, "r");
	    makestr[0] = '\0';
	    if (fd){
		if (fgets(makestr, sizeof(makestr), fd)){
		    if ((i = strlen(makestr)) > 0){
			if (makestr[i-1] == '\n'){
			    makestr[i-1] = '\0';
			}
		    }
		}
		fclose(fd);
	    }

	    sprintf(cccmd,
		    "%s %s -s -G -I%s -I%s -L. -L%s -R. -R%s %s -o '%s' %s",
		    ccprog, makestr, srcinc, gnuinc,
		    vnmrlib, vnmrlib, srcpath, progpath, libs);
	    /*fprintf(stderr,"cccmd=\"%s\"\n", cccmd);/*CMP*/
	    i = system(cccmd);	// Compile
	    /*fprintf(stderr,"COMPILE=0x%x\n", i);/*CMP*/

	// Clean up
	    unlink(srcpath);	// Delete source
	    if (i){
		// Compile failed
		unlink(progpath);
		msgerr_print("Math: Program did not compile");
		free(progname);
		return NULL;
	    }
	}
    }
    free(progname);
    return progpath;
}

/************************************************************************
*                                                                       *
*  
*  (STATIC)								*
*									*/
int
Win_math::write_images(char *src_frame2file)
{
    int i;
    int frameno;
    char *token;
    char tbuf[MAXPATHLEN];
    Gframe *gptr;

    win_print_msg("Math: Sending input data...");

    if (src_frame2file){
	/* Write out the source images */
	char *tmp = strdup(src_frame2file); // Parse it in tmp buffer
	if (!tmp){
	    return FALSE;
	}
	token = strtok(tmp, " ");

	while (token){
	    if ((i=sscanf(token,"%d = %s", &frameno, tbuf)) != 2){
		fprintf(stderr,"Bad token: i=%d, token=\"%s\"\n", i, token);
		return FALSE;
	    }else{
		for (i=1, gptr= Gframe::get_first_frame();
		     i<frameno && gptr;
		     i++, gptr= Gframe::get_next_frame(gptr))
		{ }

		/*fprintf(stderr,"Write frame %d to \"%s\"\n",
			frameno, tbuf);/*CMP*/
		gptr->imginfo->ddldata_write(tbuf);
	    }
	    token = strtok(NULL, " ");
	}
	free(tmp);
    }
    return TRUE;
}

/************************************************************************
*                                                                       *
*  
*  (STATIC)								*
*									*/
int
Win_math::exec_program(char *path, ParmList parmtree, ParmList *out)
{
    /*fprintf(stderr,"Exec users program\n");/*CMP*/
    int rtn = TRUE;
    int (*func)(ParmList, ParmList *);
    void *dlhandle;

    dlhandle = dlopen(path, RTLD_LAZY);
    if (!dlhandle){
	msgerr_print("%s", dlerror());
	return FALSE;
    }
    func = (int (*)(ParmList, ParmList *))dlsym(dlhandle, "mathexpr");
    if (!func){
	msgerr_print("%s", dlerror());
	dlclose(dlhandle);
	return FALSE;
    }
    rtn = (*func)(parmtree, out);

    dlclose(dlhandle);
    return rtn;
}

/************************************************************************
*                                                                       *
*  
*  (STATIC)								*
*									*/
int
Win_math::read_images(char *dst_frame2file)
{
    int i;
    int frameno;
    char fname[MAXPATHLEN];
    Gframe *gptr;

    // Get the gframe
    sscanf(dst_frame2file,"%d=%s", &frameno, fname);
    for (i=1, gptr= Gframe::get_first_frame();
	 i<frameno && gptr;
	 i++, gptr= Gframe::get_next_frame(gptr))
    { }
    if (i != frameno){
	msgerr_print("Math: no frame #%d to store result in.", frameno);
	return FALSE;
    }

    // Read the data
    int tstfd;
    if ((tstfd=open(fname, O_RDONLY)) == -1){
	msgerr_print("Math: procedure failed--no output.");
	return FALSE;
    }
    close(tstfd);
    int display_data = TRUE;
    Frame_data::load_ddl_data(gptr, "", fname, &display_data);

    return TRUE;
}

/************************************************************************
*                                                                       *
*  
*  (STATIC)								*
*									*/
void
Win_math::remove_files(char *frame2file)
{
    char *tmpstr;
    char *token;

    if (frame2file && (tmpstr = strdup(frame2file))){
	// Remove source files
	token = strtok(tmpstr, "=");
	while (token){
	    token = strtok(NULL, " ");
	    if (token){
		unlink(token);
	    }
	    token = strtok(NULL, "=");
	}
	free(tmpstr);
    }
}

/************************************************************************
*                                                                       *
*  Frame numbers are stripped out of the command string.
*  Each image vector is repllaced with just a "#".
*  
*  (STATIC)								*
*									*/
ParmList
Win_math::get_imagevector_list(char *name, char *cmd)
{
    int i;
    int j;
    int n;
    ParmList plp;		// List of parms (each one a framevector)
    ParmList plf;		// List of frame numbers (framevector)
    ParmList pli;		// List of images (imagevector)
    Imginfo *img;

    plp = allocParm(name, PL_PARM, 0);
    while (plf=get_framevector("framevec", &cmd)){ // "cmd" pointer gets updated
	// Convert framevector to imagevector
	n = countParm(plf);	// n will be positive
	pli = allocParm("imagevec", PL_PTR, n);
	for (i=0; i<n; i++){
	    getIntParm(plf, &j, i);
	    img = Gframe::get_frame_by_number(j)->imginfo;
	    if (!img){
		msgerr_print("Math: no image in input frame #%d.", j);
		freeParms(plp);
		freeParms(plf);
		return 0;
	    }
	    setPtrParm(pli, img->st, i);
	}
	plp = appendParmParm(plp, pli);
    }
    return plp;
}

/************************************************************************
*                                                                       *
*  The "cmd" pointer is moved past the stuff that gets parsed.
*  The string itself is not changed.
*  Parses stuff like #1 or ##1-10 or #(1,2,4-6,10) or #(1-)
*  
*  (STATIC)								*
*									*/
ParmList
Win_math::get_framevector(char *name, char **cmd)
{
    int i;
    int j;
    int n;
    int brange;
    int erange;
    char *p;
    char *pp;
    char *ppp;
    ParmList pl;

    /* Load up the list of frames */
    pl = 0;
    if (p=strchr(*cmd, '#')){	// p --> first #
	pl = allocParm(name, PL_INT, 0);
	pp = ++p;		// Mark spot to delete frame numbers
	if (*p == '#'){
	    /* Specified a range of frames, like ##20-30 */
	    p++;
	    sscanf(p,"%u", &brange);
	    erange = brange;
	    p += strspn(p, "0123456789");
	    switch (*p){
	      case '-':
		p++;
		sscanf(p,"%u", &erange);
		p += strspn(p, "0123456789");
		break;
	    }
	    for (j=brange; j<=erange; j++){
		if (!Gframe::get_frame_by_number(j)){
		    msgerr_print("Math: frame #%d does not exist", j);
		    return NULL;
		}
		pl = appendIntParm(pl, j);
	    }
	}else if (*p == '('){
	    pl = parse_frame_spec(p, pl);
	    if (ppp=strchr(p, ')')){
		p = ppp;
	    }else{
		p += strlen(p);
	    }
	}else if (sscanf(p,"%d",&j) == 1){
	    p += strspn(p, "0123456789");
	    if (!Gframe::get_frame_by_number(j)){
		msgerr_print("Math: frame #%d does not exist", j);
		return NULL;
	    }
	    pl = appendIntParm(pl, j);
	}
	/* Consolidate string--replacing image spec with "#" */
	ppp = pp;
	while (*pp++ = *p++);
	*cmd = ppp;

	if (countParm(pl) == 0){
	    freeParms(pl);
	    pl = 0;
	}
	/*printParm(pl);/*CMP*/
    }
    return pl;
}

/************************************************************************
*                                                                       *
*  The string "cmd" is MODIFIED by removing all image specs
*  
*  (STATIC)								*
*									*/
ParmList
Win_math::parse_frame_spec(char *str, ParmList pl)
{
    int j;
    int brange;
    int erange;

    while (*str && *str != ')'){
	str++;
	erange = brange = (int)strtoul(str, &str, 10);
	if (*str == '-'){
	    str++;
	    erange = (int)strtoul(str, &str, 10);
	}
	for (j=brange; j<=erange; j++){
	    if (!Gframe::get_frame_by_number(j)){
		msgerr_print("Math: frame #%d does not exist", j);
		freeParms(pl);
		return NULL;
	    }
	    pl = appendIntParm(pl, j);
	}
    }
    return pl;
}

/************************************************************************
*                                                                       *
*  Parse the expression "cmd", looking for strings (text enclosed in quotes).
*  Returns list.
*  The "cmd" string is MODIFIED by replacing all strings with ""
*  (STATIC)								*
*									*/
ParmList
Win_math::get_stringlist(char *name, char *cmd)
{
    int i;
    int j;
    //int n;
    char *pc;
    char *pcc;
    char *pccc;
    char *str;
    char msg[256];
    ParmList strings;

    pc = cmd;

    /* Count up number of quotes */
    /*for (n=0, pcc=pc; pcc=strchr(pcc,'"'); n++, pcc++);
    if (n%2){
	ib_errmsg("MATH WARNING: unmatched quotes (\") in expression:");
	sprintf(msg,"\t%.250s", cmd);
	ib_errmsg(msg);
    }
    n /= 2;			// Number of strings*/

    strings = allocParm(name, PL_STRING, 0);
    for (i=0, pcc=pc; /*i<n,*/ pcc=strchr(pcc,'"'); i++){
	pc = strchr(++pcc, '"'); // pc -> ending quote
	if (!pc){
	    ib_errmsg("MATH WARNING: unmatched quotes (\") in expression");
	    break;
	}
	//pc = pcc + j;		// pc -> ending quote
	*pc++ = '\0';		// Replace with NULL
	strings = appendStrParm(strings, pcc); // Copy string into the list
	*pcc++ = '"';		// Restore ending quote
	pccc = pcc;
	while (*pccc++ = *pc++); // Move down stuff after ending quote
    }

    return strings;
}

/************************************************************************
*                                                                       *
*  Parse the expression "cmd", looking for numerical constants
*  Returns list.
*  The "cmd" string is UNMODIFIED
*  (STATIC)								*
*									*/
ParmList
Win_math::get_constlist(char *name, char *cmd)
{
    int i;
    int j;
    int n;
    char *pc;
    char *pcc;
    char *pccc;
    char *str;
    char msg[256];
    float value;
    ParmList consts;

    pc = cmd;

    consts = allocParm(name, PL_FLOAT, 0);
    while ((n=strcspn(pc,"0123456789-.")) != strlen(pc)){
	pc += n;		// Points to start of possible number
	value = strtod(pc, &pcc);
	if (pc == pcc){
	    pc++;		// No number, keep trying
	}else{
	    // Got a value
	    consts = appendFloatParm(consts, value);
	    pc = pcc;
	}
    }
    /*printParm(consts);/*CMP*/
    return consts;
}
