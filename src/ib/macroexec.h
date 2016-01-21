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

#ifndef _MACROEXEC_H
#define _MACROEXEC_H

#define PROC_COMPLETE 0
#define PROC_ERROR 1
#define ABORT return PROC_ERROR

typedef struct{
    char *name;
    int (*func)(int, char **, int, char **);
} Cmd;

class MacroExec
{
  private:
    static Cmd cmd_table[];
    int recording;		// Recording in progress;
    Frame frame;		// Parent
    Frame popup;		// Popup frame
    Panel_item record_widget;
    Panel_item exec_widget;
    Panel_item list_widget;
    Panel_item name_widget;
    Panel_item listing_widget;
    static void record_callback(Panel_item, int value, Event *);
    static void list_callback(Panel_item, int value, Event *);
    static void resume_callback(Panel_item, int value, Event *);
    static void exec_callback(Panel_item, int value, Event *);
    static Panel_setting name_callback(Panel_item, Event *);

  public:
    MacroExec(void);
    void record(char *format, ...);
    static void execMacro(char *);
    static void execProc(char *cmd);
    static int (*getFunc(char *))(int, char **, int, char **);
    static int (*getFuncAndArgs(char *, char***, int*))(int, char **, int, char **);
    static int getDoubleArgs(int, char **, double *, int);
    static int getIntArgs(int, char **, int *, int);
    static void show_window();
    static int Suspend(int argc, char **, int, char **);
};

extern MacroExec *macroexec;

extern void browser_loop();	// Defined in win_graphics.c(!)

#endif /* _MACROEXEC_H */
