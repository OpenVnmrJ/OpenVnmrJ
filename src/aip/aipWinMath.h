/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPWINMATH_H
#define AIPWINMATH_H

#include "aipParmList.h"
#include "ddlSymbol.h"

// Class used to create math controller
class WinMath
{
  public:
    WinMath(void);
    ~WinMath(void);
    static int Exec(int argc, char **argv, int, char **);
    void math_insert(char *, int isimage);
    static WinMath *get();
    static int aipMathExecute(int argc, char *argv[], int retc, char *retv[]);
    static int aipSetExpression(int argc, char *argv[], int retc, char *retv[]);
    static int aipIMVarNames(int argc, char *argv[], int retc, char *retv[]);

    char frameClicked[32];


  private:
    static int mathIdx;

    string	expression;
    char 	expressionInVar[64];
    char 	expressionOutVar[64];
    int		expCaretPos;
    char	expCaretInVar[64];
    char	expCaretOutVar[64];

    int busy;

    static WinMath *winMath;  // support one instance
    static void execute();
    static char *append_string(char *oldstring, const char *newstuff);
    static char *append_string(char *oldstring, const char *newstuff,int nchrs);
    static char *make_c_expression(const char *cmd);
    static char *expr2progname(const char *cmd);
    static char *func2progname(const char *cmd);
    static char *func2path(char *name);
    static int sub_string_in_file(const char *infile, char *outfile,
				  const char *insub, char *outsub);
    static void exec_string(const char*);
    static int parse_lhs(const char *, ParmList *frames);
    static int parse_rhs(const char *, ParmList *ddls, ParmList *ddlvecs,
			 ParmList *strings, ParmList *constants);
    static char *get_program(const char *);
    static int write_images(char *);
    static int exec_program(const char *, ParmList in, ParmList *out);
    static void remove_files(char *);

    static ParmList get_stringlist(const char *name, char *cmd);
    static ParmList get_constlist(const char *name, char *cmd);
    static ParmList get_imagevector_list(const char *name, char *cmd);
    static ParmList get_framevector(const char *name, char **cmd);
    static ParmList parse_frame_spec(char *str, ParmList pl);
};

#endif /* AIPWINMATH_H */
