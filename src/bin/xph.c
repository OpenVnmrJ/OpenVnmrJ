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
** Phil's window conveniences
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/PopupWindo.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/TextField.h>
#include <Xol/Exclusives.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/Stub.h>
#include <Xol/Form.h>
#include <Xol/RubberTile.h>
#include "xph.h"

/* if you eliminate the XtNlabel field in the function 
   call user may change labels
   aint gonna do it now
   don't know how to put user label field into callback
*/

Widget 
xph_obutton(parent,tag,label,bCB)
Widget parent;
char *tag,*label;
void (*bCB)();
{

    Widget pwdgt;

    pwdgt = XtVaCreateManagedWidget(tag,
		oblongButtonGadgetClass,
		parent, 
   		XtNlabel,	(XtArgVal) label,
		NULL); 

    if (bCB)
       XtAddCallback(pwdgt, XtNselect, bCB,label);
    return(pwdgt);
}

Xph_ex_ptr 
xph_exclusive(parent,tag,ocapt,rows,larry,items,xCB)
Widget parent;
char *tag,*ocapt,**larry;
int rows,items;
void (*xCB)();
{
   char *xp,tmpstr[200];
   int i,nrows;
   Xph_ex_ptr big_p;

   big_p = (Xph_ex_ptr) XtMalloc(sizeof(Xph_ex));
   nrows = 1;
   /* to make label on top
   if (ocapt)
     nrows++;
   */
   big_p->num_but = items;
   big_p->selected_but = 0;
   big_p ->xbca = XtVaCreateManagedWidget(tag,
		controlAreaWidgetClass,
		parent,
   		XtNlayoutType, 		OL_FIXEDROWS,
		XtNmeasure, 		nrows, 
		NULL);

   if (ocapt)
   {
     /* OL_BOTTOM looks even more dorky */
     strcpy(tmpstr,tag);
     strcat(tmpstr,"_c");
     big_p->xcap = XtVaCreateManagedWidget(tmpstr, 
    		captionWidgetClass,
		big_p->xbca, 
		XtNlabel,		ocapt, 
		XtNspace,		20, 
		XtNalignment,		OL_CENTER,
		NULL);
   }

   strcpy(tmpstr,tag);
   strcat(tmpstr,"_x");
   big_p->xex = XtVaCreateManagedWidget(tmpstr,
		exclusivesWidgetClass,
		big_p->xbca,
		XtNlayoutType, (XtArgVal) OL_FIXEDROWS,
		XtNmeasure,    (XtArgVal) rows,
		NULL);

   big_p->xrectbp = (Widget *) XtMalloc(sizeof(Widget)*items);

   for (i=0; i < items; i++) 
   {
        xp = *(larry+i);
	*(big_p->xrectbp + i) = XtVaCreateManagedWidget(xp,
		rectButtonWidgetClass,
		big_p->xex,
  		XtNxVaryOffset, (XtArgVal) TRUE,
  		XtNyVaryOffset, (XtArgVal) TRUE,
		NULL);
	
	if (xCB)
        XtAddCallback(*(big_p->xrectbp+i), XtNselect, xCB, xp);
/*
        XtAddCallback(*(big_p->xrectbp+i), XtNselect, xCB, xp, NULL);
*/
   }
   return(big_p);
}

xph_ex_set(xph_ex,which)
Xph_ex  xph_ex;
int which;
{
   if ((which >= xph_ex.num_but) || (which < 0))
   {
     fprintf(stderr,"setting default out of range\n");
     return(-1);
   }
   xph_ex.selected_but = which;
   XtVaSetValues(*(xph_ex.xrectbp+which), XtNset, TRUE, NULL);
}

xph_ex_get(xph_ex)
Xph_ex xph_ex;
{
   Boolean j;
   int i,k;
   i = 0;
   k = -1;
   while (i < xph_ex.num_but)
   {
       XtVaGetValues(*(xph_ex.xrectbp+i), XtNset, &j, NULL);
       if (j != 0)
	 k = i;
       i++;
   }
   return(k);
}

Xph_entry_ptr xph_entry(parent,tag,label,lfz,istring,mxusz,eCB)
Widget parent;
char *tag,*label,*istring;
int lfz,mxusz;
void (*eCB)();
{
   char tmp[80];
   Xph_entry_ptr p;
   /* did not need ca? */

   p = (Xph_entry_ptr) XtMalloc(sizeof(Xph_entry));

   p->cap = XtVaCreateManagedWidget(tag, captionWidgetClass,
		parent, 
		XtNlabel, 		label,
		XtNmaximumSize, 	lfz,
		XtNspace,		2, 
		NULL);

   strcpy(tmp,tag);
   strcat(tmp,"_e");
   p->entry = XtVaCreateManagedWidget(tmp, 
		textFieldWidgetClass, 		p->cap, 
		XtNmaximumSize,			mxusz,
		/*
		XtNborderWidth,			1,
		*/
		XtNstring,			istring,	

		NULL);
 
   if (eCB)
     XtAddCallback(p->entry,XtNverification,eCB,p);
/*
     XtAddCallback(p->entry,XtNverification,eCB,p,NULL);
*/
   return(p);
}

/* procedure to get and set entries */
xph_entry_set(xx,newstr)
Xph_entry xx;
char *newstr;
{
   XtVaSetValues(xx.entry,XtNstring,newstr,NULL);
}

int
xph_entry_get(xx,answer)
Xph_entry *xx;
char *answer;
{
  OlTextFieldCopyString(xx->entry,answer);
}

/* form and controlarea facilities */ 

Widget make_form(p,tag)
Widget p;
char *tag;
{
    return(XtVaCreateManagedWidget(tag, formWidgetClass, p, 
	/*
	XtNborderWidth,  1,
	*/
	NULL));
}			

Widget make_control(p,tag,nrows)
Widget p;
char *tag;
int nrows;
{
   return(XtVaCreateManagedWidget(tag,controlAreaWidgetClass,
			p,
   			XtNlayoutType, 		OL_FIXEDROWS,
			XtNmeasure, 		nrows, 
			/*
			XtNborderWidth,		2,
			*/
			NULL));
}

Widget xph_message(p,tag,len,init_string)
Widget p;
char *tag,*init_string;
int len;
{
   return(XtVaCreateManagedWidget(tag,staticTextWidgetClass,
	p, XtNmaximumSize, len, XtNstring, init_string, NULL));
}

xph_message_set(msg,msg_content)
Widget msg;
char *msg_content;
{
   XtVaSetValues(msg,XtNstring,msg_content,NULL);
}
/* everything has been verified to here */

xph_f_pos(w,x,xa,xo,y,ya,yo)
Widget w,x,y;
Boolean xa,ya;
int xo,yo;
{
   XtVaSetValues(w,
      XtNxRefWidget,		x,
      XtNyRefWidget,		y,
      XtNxAddWidth,		xa,
      XtNyAddHeight,		ya,
      XtNxOffset,		xo,
      XtNyOffset,		yo,
      NULL);
}

xph_f_rightedge(w)
Widget w;
{
   XtVaSetValues(w, XtNxAttachRight,  TRUE,
		 NULL);
}

static void current_callback();

Xph_dir_list_ptr
xph_dir_list(p,tag,vitems,dir,flags,lCB,ee)
Widget p;
char *tag,*dir;
int vitems,flags;
void (*lCB)();
Xph_entry_ptr ee;
{
   Xph_dir_list_ptr q;

   q = (Xph_dir_list_ptr) malloc(sizeof(struct _xph_dir_list));
   if (q == NULL)
   {
     perror("Out of memory");
     exit(-1);
   }

   q->action_flag = flags;

   q->dirw =
   XtVaCreateManagedWidget(tag, 
		scrollingListWidgetClass,		p,
  		XtNviewHeight, 				vitems,
  		XtNselectable, 				FALSE,
		NULL);

  if (q->dirw == NULL)
   {
     perror("Out of memory");
     exit(-1);
   }

  XtVaGetValues(q->dirw,
  		XtNapplAddItem,    			&(q->AddItem),
  		XtNapplTouchItem,  			&(q->TouchItem),
  		XtNapplUpdateView, 			&(q->UpdateView),
  		XtNapplDeleteItem, 			&(q->DeleteItem),
  		XtNapplViewItem,   			&(q->ViewItem),
		NULL);

  q->lasttoken = NULL;
  q->entry = ee;
  q->head =  NULL;

  XtAddCallback(q->dirw, XtNuserMakeCurrent, current_callback, q);
  if (lCB)
    XtAddCallback(q->dirw, XtNuserMakeCurrent, lCB, NULL);

  if (dir[0] == '\0')
   dirList(q,".",flags);
  else
   dirList(q,dir,flags);
  initList(q);
  return(q);
}

dirList(lw, directory,toggle)
Xph_dir_list_ptr lw;
String directory;
int toggle;
{
  DIR           *dirp;
  struct dirent *dp;
  struct _scrl_ll         *filep, *prevp;
  struct stat statb;
  char tstr[80];
  int j;

  if(directory == NULL)
    directory = ".";
  dirp = opendir(directory);
  if(dirp == NULL) 
  {
     filep = (struct _scrl_ll *) malloc(sizeof(struct _scrl_ll));
     if (filep == NULL)
     {
       perror("Out of memory");
       exit(-1);
     }
     filep->next = NULL;
     filep->scrlname = XtNewString("[No access to directory]");
     filep->token = NULL;
     lw->head = filep;
     return;
  }
  for (dp = readdir(dirp); (dp != NULL) ; dp = readdir(dirp)) 
  {
    /* should use SYMBOLIC FLAG */
    if ((toggle & XPH_DIR_NO_DOT) && (dp->d_name[0] == '.'))
     continue;
    strcpy(tstr,directory);
    strcat(tstr,"/");
    strcat(tstr,dp->d_name);
    j = stat(tstr,&statb);
    if ((toggle & XPH_DIR_NO_DIR) && (S_ISDIR(statb.st_mode)))
     continue;
    filep = (struct _scrl_ll *) malloc(sizeof(struct _scrl_ll));
    if (filep == NULL)
      exit(-4);
    if(lw->head == NULL)
      lw->head = prevp = filep;
    else 
    {
      prevp->next = filep;
      prevp = filep;
    }
    filep->next = NULL;
    filep->scrlname = XtNewString(dp->d_name);
    filep->token = NULL;
  }
  closedir(dirp);
}

initList(lw)
Xph_dir_list_ptr lw;
{
  OlListItem     item;
  int          count = 0;
  struct _scrl_ll         *filep, *prevp;

  item.label_type = OL_STRING;
  item.mnemonic = NULL;
  (*(lw->UpdateView))(lw->dirw, FALSE);
  filep = lw->head;
  while (filep != NULL)
  {
    item.attr = count;
    item.label = filep->scrlname;
    filep->token = (*(lw->AddItem))(lw->dirw, 0, 0, item);
    filep = filep->next;
    count++;
  }
  (*(lw->UpdateView))(lw->dirw, TRUE);
}

deleteList(lw)
Xph_dir_list_ptr lw;
{
  int          count = 0;
  struct _scrl_ll  *filep, *wipe;

  (*(lw->UpdateView))(lw->dirw, FALSE);
  /* start at the front of the list */
  filep = lw->head;
  while (filep != NULL)
  {
    (*(lw->DeleteItem))(lw->dirw, filep->token);
    wipe = filep;
    filep = filep->next;
    free(wipe);
  }
  lw->head = NULL;
  lw->lasttoken = NULL;
  (*(lw->UpdateView))(lw->dirw, TRUE);
}



static void
current_callback(w, client_data, call_data)
Widget w;
Xph_dir_list_ptr client_data;
XtPointer call_data;
{
  OlListToken token = (OlListToken)call_data;
  OlListItem *newItem = OlListItemPointer(token);
  OlListItem *lastItem;

  /* unset */
  if(client_data->lasttoken == token) 
  {      
    newItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*(client_data->TouchItem))(client_data->dirw, token);
    client_data->lasttoken = NULL;
    return;
  }

  if(client_data->lasttoken) 
  {
    lastItem = OlListItemPointer(client_data->lasttoken);
    if(lastItem->attr & OL_LIST_ATTR_CURRENT)
      lastItem->attr &= ~OL_LIST_ATTR_CURRENT;
    (*(client_data->TouchItem))(client_data->dirw, client_data->lasttoken);
  }
  newItem->attr |= OL_LIST_ATTR_CURRENT;
  (*(client_data->TouchItem))(client_data->dirw, token);
  client_data->lasttoken = token;
  if (client_data->entry)
    xph_entry_set(client_data->entry,newItem->label); 
}

static void
ab_CB(w,client_data,call_data)
Widget w;
caddr_t client_data,call_data;
{
   char *choice; 
   XtVaGetValues(w, XtNlabel, &choice, NULL);
   XtVaSetValues(client_data, XtNstring, choice, NULL);
}

Xph_pull_down_ptr
xph_pull_down(p,tag,cpt,barry,nb,rows,cCB)
Widget p;
char *tag,*cpt;
char **barry;
int nb,rows;
void (*cCB)();
{
  Xph_pull_down_ptr q;
  Widget menu_pane,*lbw;
  char tmp[64];
  int i;

  q = (Xph_pull_down_ptr) malloc(sizeof(struct _xph_pd));

  strcpy(tmp,tag);
  strcat(tmp,"_ca");
  q->ca = XtVaCreateManagedWidget(tmp,
    		formWidgetClass,
		p,
		NULL);

  if (cpt != NULL)
  {
    strcpy(tmp,tag);
    strcat(tmp,"_c");

    q->cap = XtVaCreateManagedWidget(tmp,
    		captionWidgetClass,
		q->ca,
		XtNlabel,		cpt, 
		NULL);
  }
  else
    q->cap = NULL;

  strcpy(tmp,tag);
  strcat(tmp,"_e");

  q->current = XtVaCreateManagedWidget(tmp,
		staticTextWidgetClass, q->ca,
		XtNstring,	*(barry),
		NULL);

  q->pulld = XtVaCreateManagedWidget(tag,
		abbrevMenuButtonWidgetClass, q->ca,
		XtNpreviewWidget,	q->current,
		NULL);

  if (q->cap != NULL)
     xph_f_pos(q->pulld,q->cap,TRUE,5,q->cap,FALSE,0);
  else
     xph_f_pos(q->pulld,q->ca,FALSE,0,q->ca,FALSE,0);

  xph_f_pos(q->current,q->pulld,TRUE,5,q->pulld,FALSE,0);

  XtVaGetValues(q->pulld,XtNmenuPane,&menu_pane,NULL);

  lbw = (Widget *) XtMalloc(nb*sizeof(Widget));

  q->bw = lbw;

  for (i = 0; i < nb; i++)
  {
     *(lbw+i) = XtVaCreateManagedWidget(*(barry+i),
		oblongButtonWidgetClass, menu_pane, NULL,NULL);

     XtAddCallback(*(lbw+i), XtNselect, ab_CB, q->current);
/*
     XtAddCallback(*(lbw+i), XtNselect, ab_CB, q->current, NULL);
*/
  }

  return(q);
}
