/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#include <Xol/TextEdit.h>
#define SYSTEM	1
#define USER	2


#define OPEN   		1
#define CLOSE   	0
#define ON   		1
#define OFF   		0
#define VERTICAL 	1
#define HORIZONTAL   	0
#define BUTCOLS  6

#define GLOBAL  1
#define CUREXP  2
#define USERDIR 3
#define SYSEXP  4

#define MAXPATHL     128

#define LEFT    1
#define RIGHT   2
#define CENTER  3

/* types of items of file x.def */

#define XLABEL		1
#define XINPUT		2
#define XMENU		3
#define XCHOICE		4
#define XCHOICEVAL	5
#define XOUTPUT		6
#define XREMARK		7
#define XRTOUT		8
#define XMENVAL		9
#define XSHOW		10
#define XROWS		11
#define XCOLS		12
#define XMAX		13
#define XMIN		14
#define SUBEXP		15
#define XTEXT		16
#define XBUTTON		17
#define XRADIO		18
#define XABMENU		19
#define XEXEC		20
#define XVALUE		21
#define XSETRT		22
#define XUNSETRT	23
#define XSETEXEC	24
#define XUNSETEXEC	25
#define XSETVAL		26
#define XUNSETVAL	27
#define XCHEXEC		28

/* execs of button */
#define BUT_CLOSE	41
#define BUT_CALL	42
#define BUT_DEF		43
#define BUT_DO		44
#define BUT_EXIT	45
#define BUT_MASK	46
#define BUT_MENU	47
#define BUT_OPEN	48
#define BUT_RESET	49
#define BUT_SHOW	50
#define BUT_UNMASK	51
#define BUT_EXEC	52
#define BUT_VEXEC	53
#define BUT_SETVAL	54
#define BUT_OPENDEF	55
#define BUT_SAVE	56
#define BUT_VIEW	57

typedef struct _value_node {
	int	  subexp;
	char      *data;
	struct _value_node  *next;
       } XVALUE_NODE;

typedef struct _help_node {
	int	  len;
	int	  width;
	int	  height;
	char      *data;
	struct _help_node  *next;
       } XHELP_NODE;

typedef struct _input_node {
	char      *data;  /* data */
        float     min, max;
        int       rows, cols;
	Widget    widget;
       } XINPUT_NODE;

typedef struct _choice_node {
	int	  bid;   /* id for this button */
	int	  subexp;
	char      *label;
	char	  *data;
	char	  *exec;
	Widget    widget;
	caddr_t   pnode;   /* XPANE_NODE */
	struct _choice_node  *next;
	struct _choice_node  *subnode;  /* for subexp */
       } XCHOICE_NODE;

typedef struct _but_node {
	char      *label;
	Widget    widget;
       } XBUTTON_NODE;

typedef struct _radio_node {
	char      *label;
	int	  set;
	int	  init_set;
        int       set_exec;
        int       unset_exec;
        int       set_rtout;
        int       unset_rtout;
        int       set_val;
        int       unset_val;
	Widget    widget;
       } XRADIO_NODE;

typedef struct _text_node {
	char      *data;
	Widget    widget;
       } XTEXT_NODE;

typedef struct _pane_node {
	int	  val;      /* selected id */
	int	  subexp;
	Widget    abWidget; /* abbrevMenuButton */
	Widget    lwidget;  /* label  */
	caddr_t   pnode;    /* ITEM_NODE  */
	XCHOICE_NODE   *wlist;
       } XPANE_NODE;


typedef struct _item_node {
	int	  type;
	int	  id;
	int	  subexp;
	int	  show; /* for item */
	int	  checkShow;
        int       rtcmd;
	int	  execmd;
	char      *remark;
	caddr_t   data_node;
	struct _item_node  *next;
	struct _item_node  *prev;
	caddr_t   row_node;
       } ITEM_NODE;

typedef struct _row_node {
        int       show;
        int       rtcmd;
        int       output;
        int       execmd;
	int	  subexp;
	int	  checkShow;
	int	  checkItem;
	int	  choiceExec;
	int	  mask;
	char      *name;
        char      *label;
        char      *remark;
	Widget    rowidget;
	Widget    lwidget; /* label widget */
	caddr_t   win_entry;
        ITEM_NODE *item_list;
	struct _row_node  *next;
	struct _row_node  *dnext;
       } ROW_NODE;

typedef struct  _button_node {
        int     id;
        int     show;
        int     num;
        int     type;
        int     used;
        char    *name;
        char    *label;
        char    *exec;
        char    *proc;
        char    *vnmr_proc;
        char    *help;
	Widget  swidget;
        struct _button_node     *prev;
        struct _button_node     *next;
    } BUTTON_NODE;

typedef struct  _subWin_entry {
	int     id;
	int     sid;
	int	which;
	int	changed;
	int	subexp;
	int	alignment;
	int	itemNum;
	int	maxRow;
        char    *name;
        char    *def;
   	time_t  def_mtime;  /* the time of def last modification */
	Widget  shell;
        Widget  footer;
        Widget  buttonPanel;
	ROW_NODE    *vnode;
	BUTTON_NODE *button_node;
   	struct _xiconInfo *icon_info;
	} subWin_entry;

