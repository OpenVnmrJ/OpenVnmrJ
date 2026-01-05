/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <dlfcn.h>		// Dynamic loading

#include "graphics.h"
#include "params.h"
#include "process.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipImgInfo.h"
#include "aipWinMath.h"
#include "aipParmList.h"
#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "aipInitStart.h"
#include "group.h"
#include "aipDataManager.h"
#include "ddlCInterface.h"      // TODO: Needed?
#include "ddlSymbol.h"          // TODO: Needed?
#include "vnmrsys.h"


//#include "ddllib.h"
//#include "msgprt.h"
//#include "gtools.h"
//#include "zoom.h"
//#include "convert.h"
//#include "filelist.h"
//#include "macroexec.h"

typedef int (*MathFunc_t)(ParmList, ParmList *);

#include <iostream>
#include <string>
using namespace std;


extern void FlushGraphics();

extern short xview_to_ascii(short);

WinMath* WinMath::winMath = NULL;
int WinMath::mathIdx = 0;

/* The vnmrj panel for driving WinMath must have the following:
   - filemenu item:  
   	Selection variable: aipMselect
	Content variable: aipMupdate
	Value of item: $VALUE=aipMselect
	Vnmr command: aipSetExpression('$VALUE')
	Menu source: IBMATH
	Menu type: directory
   - centry item:
   	Vnmr variables: aip2JExp aip2JCaret
	Text query: $VALUE=aip2JExp
	Text command: aip2DExp='$VALUE'
	Caret query: $VALUE=aip2JCaret
	Caret command: aip2CCaret=$VALUE
   - button item:
   	Label of item: Execute
	Vnmr variables: aipMode
	Vnmr command: aipMathExecute('aip2CExp', 'aipMupdate')
   - a radio item in with the other adv image processing (AIP) mode controls
   	Label of item: Image Math
	Vnmr variables: aipMode
	Value of item: $VALUE=(aipMode=100)
	Vnmr Command: aipSetState(100) aipMode=100
 */

WinMath::WinMath(void) {

    // Init to empty string.
    expression =  string("");
    expCaretPos = 1;

    // I tried to have these defined by the panel and set when the
    // mode was set to imageMath.  Unfortunately, when the mode was left
    // in imageMath mode when vnmrj was exited, it then restarts with
    // that same mode, and these params have not been set in vnmrbg.
    // I don't know another answer other that hard coding them here.
    strcpy(expressionInVar, "aip2CExp");
    strcpy(expCaretInVar, "aip2CCaret");
    strcpy(expressionOutVar, "aip2JExp");
    strcpy(expCaretOutVar, "aip2JCaret");
}

/*
 * Returns the one WinMath instance.  Creates one if it has
 * not been created yet.
 */
/* PUBLIC STATIC */
WinMath *WinMath::get()
{
    if (!winMath) {
	winMath = new WinMath();
    }
    return winMath;
}

/************************************************************************
*                                                                       *
*  Insert stuff into the expression
*									*/
void
winpro_math_insert(char *text, int isimage)
{
	WinMath::get()->math_insert(text, isimage);
}


/************************************************************************
*									*
*  Execute an image math expression
*  [MACRO interface]
*  argv[0]: (string) the expression to be executed
*  [STATIC Function]							*
*  Return 1 => Error,  0 => okay
*									*/
int
WinMath::Exec(int argc, char **argv, int, char **)
{
    if (argc != 2){
        char ebuf[1024];
	sprintf(ebuf, "Usage: %s('expression')", *argv);
        ib_errmsg(ebuf);
	return 1;
    }
    argc--; argv++;

    exec_string(*argv);
    return 0;
}

/************************************************************************
*                                                                       *
*  Insert stuff into the expression string
*									*/
void
WinMath::math_insert(char *text, int isimage)
{
    int status, pos;
    double fpos;
    char newexp[128];
    // Get the current value of the expression 
    status = P_getstring(GLOBAL, expressionInVar, newexp, 1, 128);
    if(status == 0) {
	// Free up the previous expression
	//winMath->expression.~string();
	// Put the expression arg into 'expression'
	winMath->expression = string(newexp);
    }


    // Get the current value of the Caret position from the
    // variable whose name is in expCaretVar
    status = P_getreal(GLOBAL, expCaretInVar, &fpos, 1);
    if(status == 0)
	expCaretPos = (int)fpos;

    if (isimage){

	// Trap for caret position beyond end.  This can happen is spaces are
	// at the end of the string.
	if(expCaretPos > (int) expression.length())
	    expCaretPos = expression.length();

	// Check to see if there is a "#" before the insertion point,
	// If not, add one.
	if (expCaretPos == 0 || expression[expCaretPos-1] != '#') {
	    expression.insert(expCaretPos, 1, '#');
	    expCaretPos++;
	}
	// Check for digits after the insertion point, if so delete it.
        pos = expCaretPos;
	while(isdigit(expression[pos])) {
	   pos++;
	}	
	if(isdigit(expression[expCaretPos]))
	    expression.erase(expCaretPos, pos-expCaretPos);
    }

    // Insert the text
    expression.insert(expCaretPos, text);

    if (isimage){
	// Advance cursor past next "#", if there is one
	if (expression.find('#', expCaretPos+1) != expression.npos) {
	    expCaretPos = expression.find('#', expCaretPos+1) +1;

	}
    }

    // Set the new expression value into the variable name given
    P_setstring(GLOBAL, expressionOutVar, expression.c_str(), 0);
    P_setstring(GLOBAL, expressionInVar, expression.c_str(), 0);
    // Set the new caret position
    P_setreal(GLOBAL, expCaretInVar, (double)expCaretPos, 0);
    P_setreal(GLOBAL, expCaretOutVar, (double)expCaretPos, 0);


    // Execute pnew on expressionOutVar, this will take care of the caret also
    // char pnewcmd1[32];
    // strcpy(pnewcmd1, "pnew 1 ");
    // writelineToVnmrJ(pnewcmd1, expressionOutVar);
    appendJvarlist(expressionOutVar);
}


/*
 * Set the 'expression' string using an input arg which is the filename.
 * That simply means to add '#=' to the front of the string given.
 * Put the result into 'expression' and set the variable.
 * Return 1 => Error,  0 => okay
 */
int
WinMath::aipSetExpression(int argc, char *argv[], int retc, char *retv[])
{
    int status = 0;

    if(argc < 2) {
        ib_errmsg("usage: aipSetExpression expression");
	return 1;
    }

    // Instanciate winMath if needed.
    WinMath::get();
    // Free up the previous expression
    //winMath->expression.~string();
    // Put the expression arg into 'expression'
    winMath->expression = string(argv[1]);

    // prepend the '#='
    winMath->expression.insert(0, "#=");

    // Set the new expression value into the variable name given
    // TODO: Rewrite these with setString/setReal
    status = P_setstring(GLOBAL, winMath->expressionOutVar, 
			 winMath->expression.c_str(), 0);
    status = P_setstring(GLOBAL, winMath->expressionInVar, 
			 winMath->expression.c_str(), 0);
    P_setreal(GLOBAL, winMath->expCaretInVar, 1.0, 0);
    P_setreal(GLOBAL, winMath->expCaretOutVar, 1.0, 0);

    // The value does not seem to get to vnmrj unless I force a pnew.
    // char pnewcmd[32];
    // strcpy(pnewcmd, "pnew 1 ");	
    // writelineToVnmrJ(pnewcmd, winMath->expressionOutVar);
    appendJvarlist(winMath->expressionOutVar);

    return status;
}



/*
 * Execute the image math expression.
 * If an arg is given use it.  If no arg, use WinMath::expression
 * Return 0 => okay
 */
int
WinMath::aipMathExecute(int argc, char *argv[], int retc, char *retv[])
{
    const int  maxlen = 256;
    char exp[maxlen];

    // No arg given, let execute use WinMath::expression
    if(argc == 1) 
 	execute();
    else {
	// Use the arg given as a vnmr variable and get its value.
	P_getstring(GLOBAL, argv[1], exp, 1, maxlen);
        if (isDebugBit(DEBUGBIT_8)) {
            fprintf(stderr, "aipMathExecute: Executing: %s\n", exp);
        }
	exec_string(exp);
    }

    // Second arg will be variable name to cause menu to be updated.
    // We need to get the current value, and increment it so that a change
    // is registered.
    if(argc == 3) {
	char pnewcmd[32];
	strcpy(pnewcmd, "pnew 1");	
	// writelineToVnmrJ(pnewcmd, argv[2]);
        appendJvarlist(argv[2]);
    }

    return 0;
}


/************************************************************************
*                                                                       *
*  Process math.							*
*  (STATIC)								*
*									*/
void
WinMath::execute()
{
    // Instanciate winMath if needed.
    WinMath::get();

    exec_string(winMath->expression.c_str());
}

/************************************************************************
*                                                                       *
*  Utility function: append a string to a possibly existing string.
*  If "oldstr" is non-NULL, it must point to malloc'ed memory.
*  (STATIC)								*
*									*/
char *
WinMath::append_string(char *oldstr, const char *newstr)
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
WinMath::append_string(char *oldstr, const char *newstr, int nchrs)
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
WinMath::make_c_expression(const char *cmd)
{
    int i;
    int n;
    char buf[100];
    const char *pc;
    const char *pp;
    char *rtn = NULL;

    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("make_c_expression(\"%s\")\n", cmd);
    }
    // Parse past the "="
    pc = strchr(cmd, '=');
    if (!pc){
	ib_errmsg("No equals\n");
	return NULL;
    }
    pc++;

    // Replace all "#<frame>" strings with "img[<name>][indx]"
    for (i=0; (pp = strchr(pc, '#')); i++){
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
WinMath::func2progname(const char *cmd)
{
    const char *pin;
    char *rtn = NULL;
    int n;

    // Skip past "="
    pin = strchr(cmd, '=');
    if (!pin){
	ib_errmsg("func2progname: no '='\n");
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
WinMath::func2path(char *progname)
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
WinMath::expr2progname(const char *cmd)
{
    const char *pin;
    char *pout;
    char *rtn;
    int n;

    // Skip past "="
    pin = strchr(cmd, '=');
    if (!pin){
	ib_errmsg("expr2progname: no '='\n");
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
    if (strlen(rtn) >= MAXPATH){
        char ebuf[1024];
        sprintf(ebuf,"Math: Expression is more than %d characters", MAXPATH);
        ib_errmsg(ebuf);
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
WinMath::sub_string_in_file(const char *in_file, char *out_file,
			     const char *in_sub, char *out_sub)
{
    char buf[1024];
    char *pc;

    FILE *fdin = fopen(in_file, "r");
    if (!fdin){
        char ebuf[1024];
        sprintf(ebuf,"Math: Cannot open %s for reading", in_file);
        ib_errmsg(ebuf);
	return FALSE;
    }
    FILE *fdout = fopen(out_file, "w");
    if (!fdout){
        char ebuf[1024];
        sprintf(ebuf,"Math: Cannot open %s for writing", out_file);
        ib_errmsg(ebuf);
	fclose(fdin);
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fdin)){
	if (buf[strlen(buf)-1] != '\n'){
            char ebuf[1024];
            sprintf(ebuf,"Math: Input line too long: %.40s...", buf);
            ib_errmsg(ebuf);
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



/************************************************************************
*                                                                       *
*  Process the Image Math string "cmd".
*  (STATIC)								*
*									*/
void
WinMath::exec_string(const char *incmd)
{
    int i;
    int n;
    int err = FALSE;
    char *pc;
    char cmd[512];

    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("exec_string(\"%s\")\n", incmd);
    }

    // Code below may modify the input command string, so make a copy.
    strcpy(cmd, incmd);

    // The following get pointed to mallocated memory
    ParmList dst_frames = NULL;
    ParmList dst_ddls = NULL;
    ParmList src_ddls = NULL;
    ParmList src_ddl_vecs = NULL;
    ParmList src_strings = NULL;
    ParmList src_constants = NULL;
    ParmList parmtree = NULL;
    char *exec_path = NULL;

    ib_msgline("Math: Parsing...\n");

    /* Change New-Lines to Spaces */
    while ( (pc=strchr(cmd, '\n')) ){
	*pc = ' ';
    }

    /* Parse the left hand side */
    int nout = parse_lhs(cmd, &dst_frames);
    if (!nout){
	err = TRUE;
    }
    if (isDebugBit(DEBUGBIT_8)) {
        printParm(dst_frames);
    }

    /* Parse images on right hand side */
    int nin = parse_rhs(cmd, &src_ddls, &src_ddl_vecs,
			&src_strings, &src_constants);
    if (!nin){
	err = TRUE;
    }
    if (isDebugBit(DEBUGBIT_8)) {
        printParm(src_ddls);
        printParm(src_strings);
    }

    /* Get the executable, compiling if necessary */
    if (!err){
	if (!(exec_path = get_program(cmd))){
	    err = TRUE;
	}
    }
    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("exec_path=%s\n", exec_path);
    }

    /* Execute the program */
    parmtree = allocParm("parmtree", PL_PARM, 5);
    setParmParm(parmtree, src_ddls, 0);
    setParmParm(parmtree, dst_frames, 1);
    setParmParm(parmtree, src_strings, 2);
    setParmParm(parmtree, src_constants, 3);
    setParmParm(parmtree, src_ddl_vecs, 4);
    if (isDebugBit(DEBUGBIT_8)) {
        printParm(parmtree);
    }
    if (!err){
	ib_msgline("Math: Executing program...\n");
	if (!exec_program(exec_path, parmtree, &dst_ddls)){
	    err = TRUE;
	}
    }

    /* Display the results */
    if (isDebugBit(DEBUGBIT_8)) {
        printParm(dst_ddls);
    }
    void *vst;
    int frame;
    spGframe_t gf;
    n = countParm(dst_ddls);
    for (i=0; i<n; i++){
	getPtrParm(dst_ddls, &vst, i);
	DDLSymbolTable *st = (DDLSymbolTable *)vst;
	getIntParm(dst_frames, &frame, i);
	if (isDebugBit(DEBUGBIT_8)) {
	    STDERR_2("st=%p, frame=%d\n", st, frame);
	}
	if (st && frame){
	    char *fname;
	    bool status = st->GetValue("filename", fname);
 	    char *newname;
 	    if(status) {
		newname = (char *)malloc(strlen(fname) + 20);
		sprintf(newname,"%s.mathout#%d", fname, ++mathIdx);
	    }
	    else {
		newname = (char *)malloc(20);
		sprintf(newname,"mathout#%d", ++mathIdx);
	    }
	    st->SetValue("filename", newname);
            st->SetValue("display_order", -1); // Force use of filename in key
	    gf = GframeManager::get()->getFrameByNumber(frame);
	    DataManager *dm = DataManager::get();
	    string key = dm->loadFile(newname, st);
	    // Display into the correct frame.
	    dm->displayData(key, frame - 1);
	    free(newname);
	}
    }

    /* Free memory */
    free(parmtree);		// Also frees params under it
    free(dst_ddls);

    ib_msgline("Math: Done.\n");
}

/************************************************************************
*                                                                       *
*  Parse the LHS of math expression "cmd".
*  Returns number of frames in list.
*  (STATIC)								*
*									*/
int
WinMath::parse_lhs(const char *cmd, ParmList *framelist)
{
    int nframes = 0;
    char *tbuf;

    tbuf = strdup(cmd);
    if (!tbuf) return 0;

    char *token = strtok(tbuf, "=");
    if (token){
	// Where to put the results
	*framelist = get_framevector("dst_frames", &token);
	nframes = countParm(*framelist);
	if (nframes < 1){
	    ib_errmsg("Math: Specified output frames do not exist.");
    	    free(tbuf);
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
WinMath::parse_rhs(const char *cmd,
		    ParmList *ddls,
		    ParmList *ddlvecs,
		    ParmList *strings,
		    ParmList *constants)
{
    int i;
    int j;
    int m;
    int n;
    const char *pc;
    char *tbuf;
    ParmList pl;
    void *pv;
    spImgInfo_t img;

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
    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("cmd w/o frames = \"%s\"\n", tbuf);
    }

    *strings = get_stringlist("src_strings", tbuf); // strings stripped off
    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("cmd w/o strings = \"%s\"\n", tbuf);
    }

    *constants = get_constlist("src_constants", tbuf); // tbuf unchanged
    if (isDebugBit(DEBUGBIT_8)) {
        STDERR_1("cmd w/ constants = \"%s\"\n", tbuf);
    }

    n = countParm(*ddls);
    if (!n){
	ib_errmsg("Math: no input image(s) specified");
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
WinMath::get_program(const char *cmd)
{
    int i;

    char *progpath = NULL;
    char *progname = NULL;

    const char *dir = "/tmp";		// Directory for temp image files
    int pid = getpid();

    if ((progname=func2progname(cmd)) && (progpath=func2path(progname))){
	/* We got a user program */
    }else if ( (progname=expr2progname(cmd)) == NULL){
        char ebuf[1024];
        sprintf(ebuf,"Math: Could not make prog name from \"%s\"", cmd);
        ib_errmsg(ebuf);
	return NULL;
    }else{
	// This was an expression
        if (isDebugBit(DEBUGBIT_8)) {
	    STDERR_1("progname=\"%s\"\n", progname);
	}

	// See if program already exists
	progpath = (char *)malloc(MAXPATHLEN);
	if (!progpath){
	    free(progname);
	    return NULL;
	}
	(void)init_get_env_name(progpath);	/* Get the directory path */
	strcat(progpath, "/math/expressions/bin/");
	strcat(progpath, progname);
	// NB: *** NEED TO CHECK IF PROGRAM IS UP TO DATE
	if (access(progpath, X_OK) == 0){
	    // Already have the program
	    if (isDebugBit(DEBUGBIT_8)) {
	        STDERR("program already exists\n");
	    }
	}else{
	    // Need to compile the program
	    char *c_expr = NULL;

            ib_msgline("Math: Compiling program...\n");
	    // Turn the given expression into C code
            if (isDebugBit(DEBUGBIT_8)) {
            }
	    c_expr = make_c_expression(cmd);
            if (isDebugBit(DEBUGBIT_8)) {
		STDERR_1("cmd=\"%s\"\n", cmd);
                STDERR_1("c_expr=\"%s\"\n", c_expr);
            }
	    if (!c_expr){
                char ebuf[1024];
		sprintf(ebuf,"Math: Could not decode expression: \"%s\"", cmd);
                ib_errmsg(ebuf);
		free(progname);
		return NULL;
	    }

	    // Insert the expression into the canned program
	    char protopath[MAXPATHLEN];
	    (void)init_get_env_name(protopath);
	    strcat(protopath, "/math/expressions/src/mathproto.c");
	    char srcpath[128];
	    sprintf(srcpath,"%s/%d_c_code.c", dir, pid);
	    if (!sub_string_in_file(protopath, srcpath,
				    "IB_EXPRESSION", c_expr)){
		ib_errmsg("Math: Could not write C program");
		free(progname);
		free(c_expr);
		return NULL;
	    }
	    free(c_expr);

	    // Compile program into math directory
	    char cccmd[2048*2];
	    char gnuinc[16];
	    char srcinc[512];
	    char ccprog[8];
	    char libs[] = "-lm";

	    (void)init_get_env_name(srcinc);
	    strcat(srcinc,"/math/expressions/src");
		 strcpy(ccprog, "cc");
	    strcpy(gnuinc,"./include");

	    char makeexpr[512+16];
	    char makestr[512];
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
		    "%s %s -s -shared -I%s -I%s -fPIC %s -o '%s' %s",
		    ccprog, makestr, srcinc, gnuinc,
		    srcpath, progpath, libs);
	    if (isDebugBit(DEBUGBIT_8)) {
	        STDERR_1("cccmd=\"%s\"\n", cccmd);
	    }
	    i = system(cccmd);  // Compile

	    if (i){
		// Compile failed
		unlink(progpath);
		ib_errmsg("Math: Program did not compile");
		free(progname);
		return NULL;
	    } else {
                unlink(srcpath);	// Delete source
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
WinMath::exec_program(const char *path, ParmList parmtree, ParmList *out)
{
    if (isDebugBit(DEBUGBIT_8)) {
        STDERR("Exec users program\n");
    }
    int rtn = TRUE;
    MathFunc_t func;
    void *dlhandle;


    dlhandle = dlopen(path, RTLD_NOW);

    if (!dlhandle){
	ib_errmsg(dlerror());
	return FALSE;
    }
    // NB: Compiler does not like casts from "pointer-to-void" to
    // "pointer-to-function".  Hence, first cast to "long", then to
    // "pointer-to-function"!
    func = (MathFunc_t) dlsym(dlhandle, "mathexpr");    
    if (!func){
	ib_errmsg(dlerror());
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
void
WinMath::remove_files(char *frame2file)
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
WinMath::get_imagevector_list(const char *name, char *cmd)
{
    int i;
    int j;
    int n;
    ParmList plp;		// List of parms (each one a framevector)
    ParmList plf;		// List of frame numbers (framevector)
    ParmList pli;		// List of images (imagevector)
    spDataInfo_t di;
    spGframe_t gf;

    plp = allocParm(name, PL_PARM, 0);
    while ( (plf=get_framevector("framevec", &cmd)) ){ // "cmd" pointer gets updated
	// Convert framevector to imagevector
	n = countParm(plf);	// n will be positive
	pli = allocParm("imagevec", PL_PTR, n);
	for (i=0; i<n; i++){
	    getIntParm(plf, &j, i);
	    spImgInfo_t img;
	    if ((gf = GframeManager::get()->getFrameByNumber(j)) == nullFrame ||
		(img = gf->getFirstImage()) == nullImg)
	    {
                char ebuf[1024];
		sprintf(ebuf,"Math: no image in input frame #%d.", j);
                ib_errmsg(ebuf);
		freeParms(plp);
		freeParms(plf);
		return 0;
	    }
	    di = img->getDataInfo();
	    setPtrParm(pli, di->st, i);
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
WinMath::get_framevector(const char *name, char **cmd)
{
    int j;
    int brange;
    int erange;
    char *p;
    char *pp;
    char *ppp;
    ParmList pl;

    /* Load up the list of frames */
    pl = 0;
    if ( (p=strchr(*cmd, '#')) ){	// p --> first #
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
		if (GframeManager::get()->getFrameByNumber(j) == nullFrame) {
                    char ebuf[1024];
                    sprintf(ebuf,"Math: frame #%d does not exist", j);
                    ib_errmsg(ebuf);
		    return NULL;
		}
		pl = appendIntParm(pl, j);
	    }
	}else if (*p == '('){
	    pl = parse_frame_spec(p, pl);
	    if ( (ppp=strchr(p, ')')) ){
		p = ppp;
	    }else{
		p += strlen(p);
	    }
	}else if (sscanf(p,"%d",&j) == 1){
	    p += strspn(p, "0123456789");
	    if (GframeManager::get()->getFrameByNumber(j) == nullFrame) {
                char ebuf[1024];
                sprintf(ebuf,"Math: frame #%d does not exist", j);
                ib_errmsg(ebuf);
		return NULL;
	    }
	    pl = appendIntParm(pl, j);
	}
	/* Consolidate string--replacing image spec with "#" */
	ppp = pp;
	while ( (*pp++ = *p++) );
	*cmd = ppp;

	if (countParm(pl) == 0){
	    freeParms(pl);
	    pl = 0;
	}
        if (isDebugBit(DEBUGBIT_8)) {
	    printParm(pl);
	}
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
WinMath::parse_frame_spec(char *str, ParmList pl)
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
	    if (GframeManager::get()->getFrameByNumber(j) == nullFrame) {
                char ebuf[1024];
                sprintf(ebuf,"Math: frame #%d does not exist", j);
                ib_errmsg(ebuf);
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
WinMath::get_stringlist(const char *name, char *cmd)
{



    int i;
    char *pc;
    char *pcc;
    char term;
    char *pccc;
    ParmList strings;

    strings = allocParm(name, PL_STRING, 0);
    pcc = cmd;

    // Skip over the first argument (the function name)
    i = strspn(pcc, " "); // Skip any blanks
    pcc += i;
    pcc = strchr(pcc, ' '); // Find next blank after token

    while (pcc && strlen(pcc) > 0) {
        i = strspn(pcc, " "); // Skip blanks
        pcc += i;
        if (*pcc != '"') {
            // maybe a non-quoted string, maybe a constant, maybe a "#"
            //i = strspn(pcc + 1, " ");
            //pc = pcc + 1 + strspn(pcc + 1, " "); // Skip contiguous blanks
            pc = strchr(pcc, ' '); // Find closing blank
            if (!pc) {
                pc = cmd + strlen(cmd); // ... points to terminating NULL
            }
            term = *pc; //
            *pc++ = '\0'; // Replace terminator with NULL
            i = (int)strtod(pcc, &pccc); // Test numeric conversion
            if (*pcc == '#' || ( (int) (pccc - pcc) == (int) strlen(pcc))) {

                // It's a number or "#"
                if (term == '\0') {
                    *--pc = term;
                    pcc = pc; // We're done
                } else {
                    *--pc = term;
                    pcc = pc;
                }
            } else {
                // Not a "#" or a number
                strings = appendStrParm(strings, pcc); // Copy string into the list
                if (term == '\0') {
                    *pcc = '\0'; // We're done
                } else {
                    *pcc = term; // Restore terminating character
                    pccc = pcc;
                    while ((*pccc++ = *pc++) != '\0'); // Move down stuff after closing quote
                }
            }
        } else {
            // Found a quoted string
            pc = strchr(pcc + 1, '"'); // Find closing quote
            if (!pc){
                ib_errmsg("MATH WARNING: unmatched quotes (\") in expression");
                break;
            }
            pcc++; // Skip open quote
            *pc++ = '\0';           // Replace with NULL
            strings = appendStrParm(strings, pcc); // Copy string into the list
            *pcc++ = '"';           // Restore closing quote
            pccc = pcc;
            while ((*pccc++ = *pc++) != 0); // Move down stuff after closing quote
        }
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
WinMath::get_constlist(const char *name, char *cmd)
{
    unsigned int n;
    char *pc;
    char *pcc;
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
    if (isDebugBit(DEBUGBIT_8)) {
        printParm(consts);
    }
    return consts;
}

