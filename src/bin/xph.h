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
/*
**	Phils X tools 
*/
#define XPH_DIR_ALL		0
#define XPH_DIR_NO_DOT 		1
#define XPH_DIR_NO_DIR		2

#define XPH_DIR_MARK_LOCK	4
#define XPH_DIR_MARK_NOWRITE	8
#define XPH_DIR_LAUNCH		16

Widget xph_obutton();
/*
   one oblong button with callback bCB on select
   args
   Widget parent;   parent widget
   char *tag,	    environment handle
        *label;	    label for button
   void (*bCB)();   button press callback
*/

struct _xph_ex 
{
   Widget xbca;  /* control area */
   Widget xcap;  /* an optional caption field */
   Widget xex;   /* exclusives field */
   Widget *xrectbp;   /* array of buttons */
   int num_but;  /* number of said buttons */
   int selected_but; /* current selection */
};

typedef struct _xph_ex Xph_ex;
typedef struct _xph_ex *Xph_ex_ptr;

Xph_ex_ptr xph_exclusive();
/*
   args
   Widget parent;
   char *tag,*ocapt,**larry;
   int rows,items;
   void (*xCB)();
*/

struct _xph_entry 
{
   /*
   Widget ca;
   */
   Widget cap;
   Widget entry;
};

typedef struct _xph_entry Xph_entry;
typedef struct _xph_entry *Xph_entry_ptr;

Xph_entry_ptr xph_entry();
/*
   parent tag label labelfieldsize init_string maxusersize callcb
   args 
   Widget parent
   char *tag, *label, *init_string;
   int labelfieldsize,maxusersize;
   void (*eCB)();
*/
int xph_ex_set();
/* Xph_ex  enum */

int xph_ex_get();
/* Xph_ex  */

Widget make_form();
/* 
  parent, tag
*/

Widget make_control();
/* 
  parent, tag, nrows
*/

Widget xph_message();
/*
  parent, tag, len, initial value
*/
int xph_message_set();
/*
  msg widget, string
*/
/***********************
layout 1:  this below that +offset left edges same
layout 2:  this beside that +offset tops same
************************/

typedef struct _scrl_ll {
  String         scrlname;
  OlListToken    token;
  struct _scrl_ll *next;
};

struct _xph_dir_list
{
    Widget	dirw;
    Xph_entry_ptr	entry;
    int		action_flag;
    OlListToken (*AddItem)();
    void        (*DeleteItem)();
    void        (*TouchItem)();
    void        (*UpdateView)();
    void        (*ViewItem)();
    struct _scrl_ll 	*head;     
    OlListToken lasttoken;
};
/*
  Use the same structure but change the 
  name to clarify function
*/

typedef struct _xph_dir_list Xph_dir_list;
typedef struct _xph_dir_list *Xph_dir_list_ptr;
typedef struct _xph_dir_list Xph_scr_list;
typedef struct _xph_dir_list *Xph_scr_list_ptr;

Xph_dir_list_ptr xph_dir_list();
/*
Xph_src_list_ptr xph_scr_list();
*/

typedef struct _xph_pd 
{
    Widget ca;
    Widget cap;
    Widget pulld;
    Widget current;
    Widget *bw;
}  Xph_pull_down,*Xph_pull_down_ptr;

Xph_pull_down_ptr
xph_pull_down();
/* 
*/
