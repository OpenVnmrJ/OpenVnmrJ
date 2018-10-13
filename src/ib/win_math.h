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

#ifndef _WIN_MATH_H
#define _WIN_MATH_H

#include "parmlist.h"

// Class used to create math controller
class Win_math
{
  public:
    Win_math(void);
    ~Win_math(void);
    static int Exec(int argc, char **argv, int, char **);
    void math_insert(char *, int isimage);
    void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }

  private:
    Frame frame;	// Parent
    Frame popup;	// Popup frame (subframe)
    Textsw expression_box;

    int busy;
    static void done_proc(Frame);
    static void text_proc(Panel_item, Event *);
    static Menu math_menu_load_pullright(Menu_item, Menu_generate op);
    static void math_menu_load(char *dir, char *file);
    static void execute(Panel_item);
    static char *append_string(char *oldstring, char *newstuff);
    static char *append_string(char *oldstring, char *newstuff, int nchrs);
    static char *make_c_expression(char *cmd);
    static char *expr2progname(char *cmd);
    static char *func2progname(char *cmd);
    static char *func2path(char *name);
    static int sub_string_in_file(char *infile, char *outfile,
				  char *insub, char *outsub);
    static void exec_string(char*);
    static int parse_lhs(char *, ParmList *frames);
    static int parse_rhs(char *, ParmList *ddls, ParmList *ddlvecs,
			 ParmList *strings, ParmList *constants);
    static char *get_program(char *);
    static int write_images(char *);
    static int exec_program(char *, ParmList in, ParmList *out);
    static int read_images(char *);
    static void remove_files(char *);

    static ParmList get_stringlist(char *name, char *cmd);
    static ParmList get_constlist(char *name, char *cmd);
    static ParmList get_imagevector_list(char *name, char *cmd);
    static ParmList get_framevector(char *name, char **cmd);
    static ParmList parse_frame_spec(char *str, ParmList pl);
};

#endif /* _WIN_MATH_H */
