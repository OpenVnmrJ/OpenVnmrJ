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

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <dirent.h>
#include <sys/stat.h>

#include "dialog.h"

extern char   systemdir[];  /* vnmr system directory */
extern char   userdir[];    /* user directory */
extern char   UserName[];
extern char   expdir[];
extern char   tmpstr[];
extern int    debug;
extern int    do_preview;

typedef struct  _val_link {
        char    *data;
        struct _val_link *next;
    } VAL_LINK;

static char   *dataPtr, inputData[1024];
static char   retData[256];
static char   tmpData[512];
static char   nameData[256];

static VAL_LINK *subexp_list = NULL;
static FILE   *fin;
ROW_NODE      *exit_row;

static char *token1[] = {"menu", "show", "rows", "cols", "help", "exec",
		    "text", "label", "input", "value", "output", "remark",
		    "choice", "button", "min", "max" };

static int  type1[] = {XMENU, XSHOW, XROWS, XCOLS, XREMARK, XEXEC,
		    XTEXT, XLABEL, XINPUT, XVALUE, XOUTPUT, XREMARK,
		    XCHOICE, XBUTTON, XMIN, XMAX, 0 };
static int  strip1[] = { 1, 0, 1, 1, 0, 0,  0, 0, 0, 1, 0, 0,
			 1, 0, 1, 1 };
static int  margs1[] = { 1, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0,
			 1, 0, 0, 0 };
static int  numType1 = 16;

static char *token2[] = {"rtoutput","menu_value","choice_value","check_button",
		    "check_set_exec", "abbreviate_menu", "check_set_value",
		    "check_unset_exec","check_unset_value","check_set_rtoutput",
		    "check_unset_rtoutput", "choice_exec" };

static int  type2[] = { XRTOUT, XMENVAL, XCHOICEVAL, XRADIO,
			XSETEXEC, XABMENU, XSETVAL, 
			XUNSETEXEC, XUNSETVAL, XSETRT, XUNSETRT, XCHEXEC, 0 };
static int  strip2[] = { 0, 1, 1, 0,  0, 0, 0,
			 0, 0, 0, 0, 1, 0 };
static int  margs2[] = { 0, 1, 1, 0,  0, 1, 0, 
			 0, 0, 0, 0, 1, 0};
static int  numType2 = 12;

extern Arg    args[];


/*  This program will read experiment file.def and create its list */

static char
*get_new_line()
{
	char	*data;
	int	len, stop, num, k;

	while (1)
	{
	    if ((data = fgets(inputData, 1024, fin)) == NULL)
	        return(data);
	    while (*data == ' ' || *data == '\t')
		data++;
            if (*data != '#' && *data != '!')
                break;
	}
        len = strlen(inputData);
        if (len < 2)
	    return(data);
        if (inputData[len - 1] == '\n' && inputData[len - 2] == '\\')
        {
	    if (len > 2)
	    {
		num = 0;
		k = len - 2;
		while (k >= 0)
		{
		     if (inputData[k] == '\\')
			num++;
		     else
			break;
		     k--;
		}
		if ((num % 2) == 0)
	             return(data);
	    }
            inputData[len - 2] = '\0';
	}
	else
	    return(data);
	stop = 0;
	while (fgets(tmpData, 512, fin) != NULL)
	{
             len = strlen(tmpData);
             if (len < 2)
		stop = 1;
             else if (tmpData[len - 1] == '\n' && tmpData[len - 2] == '\\')
             {
		num = 0;
		k = len - 2;
		while (k >= 0)
		{
		     if (tmpData[k] == '\\')
			num++;
		     else
			break;
		     k--;
		}
		if ((num % 2) == 0)
		     stop = 1;
		else
                    tmpData[len - 2] = '\0';
             }
	     else
		stop = 1;
             strcat(inputData, tmpData);
	     if (stop)
		break;
	}
	return(data);
}


static  ROW_NODE
*new_row_node(id)
int	id;
{
	int	   k;
	ROW_NODE  *node;

	node = (ROW_NODE *) calloc(1, sizeof(ROW_NODE));
	if (node != NULL)
	{
    	   node->show = 1;
	}
	return(node);
}


static ITEM_NODE
*new_item_node()
{
	ITEM_NODE  *node;

	node = (ITEM_NODE *) calloc(1, sizeof(ITEM_NODE));
	node->show = 1;
	node->id = -99;
	return(node);
}


int
get_token_type(name)
char  *name;
{
	int	len, k;

	len = strlen(name);
	if (len >= 3)
	{
	     if (len < 8) 
	     {
		for (k = 0; k < numType1; k++)
		{
		    if (strcmp(token1[k], name) == 0)
			return(type1[k]);
		}
	     }
	     else
	     {
		for (k = 0; k < numType2; k++)
		{
		    if (strcmp(token2[k], name) == 0)
			return(type2[k]);
		}
	     }
	}
	return(0);
}


char *get_token_and_id (source, dest, vid, needColon)
char	*source, *dest;
int	*vid, needColon;
{
	char	*retPtr, *vPtr;
	int	sptr, ok;

	*vid = -1;
	sptr = 0;
	ok = 0;
	retPtr = NULL;
	vPtr = NULL;
	if (debug > 1)
	   fprintf(stderr, " get token: %s ", source);
	while (*source == ' ' || *source == '\t')
            source++;
	while ( ok == 0 )
	{
	    switch (*source) {
	      case ':' :
			dest[sptr] = '\0';
			retPtr = source + 1;
			ok = 1;
			break;
	      case ',' :
			if (needColon) /* error syntax */
			{
			    dest[0] = '\0';
			    retPtr = NULL;
			    vPtr = NULL;
			    ok = 1;
			}
			else
			{
			    dest[sptr] = '\0';
			    retPtr = source + 1;
			    ok = 1;
			}
			break;
	      case '(' :
			dest[sptr] = '\0';
			vPtr = source + 1;
			break;
	      case ')' :
			dest[sptr] = '\0';
			*source = '\0';
			break;
	      case ' ' :
	      case '\t':
			dest[sptr] = '\0';
			break;
	      case '\0' :
	      case '\n' :
			dest[0] = '\0';
			retPtr = NULL;
			vPtr = NULL;
			ok = 1;
			break;
	      default:
			if (vPtr == NULL)
			{
			   dest[sptr] = *source;
			   sptr++;
			   if (sptr > 22) /* not correct type */
			   {
			       dest[0] = '\0';
			       retPtr = NULL;
			       vPtr = NULL;
			       ok = 1;
			   }
			}
			break;
	    }
	    source++;
	}
	if (ok && vPtr && (int)strlen(vPtr) > 0)
	{
	    if (debug > 1)
		fprintf(stderr, "  id data: '%s'\n", vPtr);
	    *vid = atoi(vPtr);
	}
	return(retPtr);
}


static
char *get_argument(indata, restrict)
 char  *indata;
 int    restrict;
{
	int    ptr, dbQuote;
	int    goback;
	static char *buffer;
	char   *tdata;

     if (indata != NULL)
        buffer = indata;
     while (*buffer == ' ' || *buffer == ',' || *buffer == '\t')
             buffer++;

     dbQuote = 0;
     ptr = 0;
     goback = 0;
     for(;;)
     {
	switch (*buffer)  {
	 case '\0':
	 case '\n':
		   retData[ptr] = '\0';
		   goback = 1;
		   break;
	 case '"':
		   if (ptr > 0)
		   {
		       if (retData[ptr-1] == '\\')
			   retData[ptr++] = *buffer;
		       else
		           dbQuote++;
		   }
		   else
		       dbQuote++;
		   if (dbQuote == 2)
		       goback = 1;
		   buffer++;
		   break;
	 case ' ':
	 case '\t':
	 case ',':
		   if (dbQuote || !restrict)
			retData[ptr++] = *buffer;
		   else
		   {
			retData[ptr] = '\0';
			goback = 1;
		   }
		   buffer++;
		   break;
	 case '#':
	 case '!':
		   if (dbQuote)
		   {
			retData[ptr++] = *buffer;
		        buffer++;
		   }
		   else
			goback = 1;
		   break;
	 default:
		   retData[ptr++] = *buffer;
		   buffer++;
		   break;
	}
	if (goback)
	{
	     retData[ptr] = '\0';
	     if (ptr > 0 && !dbQuote)
	     {
		  ptr--;
		  tdata = retData + ptr;
		  while (*tdata == ' ' || *tdata == '\t')
		  {
			*tdata = '\0';
			if (ptr <= 0)
			   break;
			ptr--;
			tdata--;
		  }
		  return(&retData[0]);
	     }
	     if (dbQuote)
		  return(&retData[0]);
	     else
		  return ((char *) NULL);
	}
     }
}



static ITEM_NODE
*set_item_multi_attribute (rownode, type, id, data)
ROW_NODE   *rownode;
int	   type, id;
char	   *data;
{
	char	     *arg;
	int	     subexp, expflag;
	ITEM_NODE    *tnode, *retnode;
	XPANE_NODE   *pane;
	XCHOICE_NODE *choice, *nchoice;
	XVALUE_NODE  *vnode, *nvnode;

	arg = get_argument(data, 1);
	retnode = NULL;
	pane = NULL;
	if (arg == NULL)
	   return(retnode);
	tnode = (ITEM_NODE *) rownode->item_list;
	while (tnode != NULL)
	{
	   if (tnode->type == type && tnode->id == id)
		break;
	   tnode = tnode->next;
	}
	if (tnode == NULL)
	{
	     tnode = new_item_node();
	     tnode->type = type;
	     tnode->id = id;
	     tnode->row_node = (caddr_t) rownode;
	     retnode = tnode;
	}
	switch (type) {
	  case XMENU:
	  case XABMENU:
	  case XCHOICE:
		if (tnode->data_node == NULL)
		{
		    pane = (XPANE_NODE *) calloc(1, sizeof(XPANE_NODE));
		    pane->val = 1;
		    pane->subexp = 0;
		    pane->wlist = NULL;
		    pane->lwidget = NULL;
		    pane->abWidget = NULL;
		    tnode->data_node = (caddr_t) pane;
		}
		else
		    pane = (XPANE_NODE *) tnode->data_node;
		choice = pane->wlist;
		while (choice && choice->next != NULL)
		    choice = choice->next;
		break;

	  case XVALUE:
	  case XMENVAL:
	  case XCHOICEVAL:
	  case XCHEXEC:
		vnode = (XVALUE_NODE *) tnode->data_node;
		while (vnode && vnode->next != NULL)
		    vnode = vnode->next;
		break;
	  default:
		if (debug)
		   fprintf(stderr, " parse(2): unknown type => %d ...\n", type);
		return (NULL);
		break;
	}

	while (arg != NULL)
	{
	   subexp = 0;
	   expflag = 0;

	   switch (type) {
	     case XMENU:
	     case XABMENU:
	     case XCHOICE:
		    nchoice = (XCHOICE_NODE *) calloc(1, sizeof(XCHOICE_NODE));
		    nchoice->pnode = (caddr_t)pane;
		    nchoice->data = NULL;
		    nchoice->bid = 1;
		    nchoice->subexp = subexp;
		    nchoice->widget = NULL;
		    nchoice->next = NULL;
		    if (!subexp)
		    {
		    	nchoice->label = (char *) malloc(strlen(arg)+1);
		    	strcpy (nchoice->label, arg);
		    }
		    else
			nchoice->label = NULL;
		    if (choice == NULL)
			pane->wlist = nchoice;
		    else
			choice->next = nchoice;
		    choice = nchoice;
		    break;

	     case XVALUE:
	     case XMENVAL:
	     case XCHOICEVAL:
	     case XCHEXEC:
		    nvnode = (XVALUE_NODE *) calloc(1, sizeof(XVALUE_NODE));
		    nvnode->next = NULL;
		    nvnode->subexp = subexp;
		    if (!subexp)
		    {
		    	    nvnode->data = (char *) malloc(strlen(arg)+1);
		    	    strcpy (nvnode->data, arg);
		    }
		    else
			    nvnode->data = NULL;
		    if (vnode == NULL)
		            tnode->data_node = (caddr_t) nvnode;
		    else
			    vnode->next = nvnode;
		    vnode = nvnode;
		    break;
	   }
	   arg = get_argument(NULL, 1);
	}
	return(retnode);
}


set_item_one_attribute (node, data, strip)
ITEM_NODE  *node;
char	   *data;
int	   strip;
{
	char	     *arg;
	XVALUE_NODE  *vnode;
	XINPUT_NODE  *in_node;
	XBUTTON_NODE *bnode;
	XRADIO_NODE  *rdnode;
	XTEXT_NODE   *txnode;

	arg = get_argument(data, strip);
	switch (node->type) {
	  case XLABEL:
	  case XOUTPUT:
	  case XRTOUT:
	  case XUNSETRT:
	  case XUNSETEXEC:
	  case XUNSETVAL:
	  case XREMARK:
	  case XEXEC:
	  case XSHOW:
	  case XSETRT:
	  case XSETEXEC:
	  case XSETVAL:
	  case XROWS:
	  case XCOLS:
	  case XMIN:
	  case XMAX:
		    vnode = (XVALUE_NODE *) calloc(1, sizeof(XVALUE_NODE));
		    if (arg)
		    {
			vnode->data = (char *) malloc(strlen(arg) + 1);
			strcpy(vnode->data, arg);
		    }
		    node->data_node = (caddr_t) vnode;
		    break;

	  case XINPUT:
		    in_node = (XINPUT_NODE *) calloc(1, sizeof(XINPUT_NODE));
		    if (arg)
		    {
			in_node->data = (char *) malloc(strlen(arg) + 1);
			strcpy(in_node->data, arg);
		    }
		    in_node->rows = 1;
		    in_node->cols = 6;
		    node->data_node = (caddr_t) in_node;
		    break;
	  case XBUTTON:
		    bnode = (XBUTTON_NODE *) calloc(1, sizeof(XBUTTON_NODE));
		    if (arg)
		    {
			bnode->label = (char *) malloc (strlen(arg) + 1);
			strcpy(bnode->label, arg);
		    }
		    node->data_node = (caddr_t) bnode;
		    break;
	  case XRADIO:
		    rdnode = (XRADIO_NODE *) calloc(1, sizeof(XRADIO_NODE));
		    if (arg)
		    {
			rdnode->label = (char *) malloc (strlen(arg) + 1);
			strcpy(rdnode->label, arg);
		    }
		    node->data_node = (caddr_t) rdnode;
		    break;
	  case XTEXT:
		    txnode = (XTEXT_NODE *) calloc(1, sizeof(XTEXT_NODE));
		    if (arg)
		    {
			txnode->data = (char *) malloc(strlen(arg) + 1);
			strcpy(txnode->data, arg);
		    }
		    node->data_node = (caddr_t) txnode;
		    break;
	  default:
		    if (debug)
			fprintf(stderr, " Parse(1): unknown type => %d ...\n", node->type);
		    node->data_node = (caddr_t) NULL;
		    break;
	}
}


static ROW_NODE
*parse_def_input(dentry)
 subWin_entry  *dentry;
{
    char   *arg, token[32];
    char   *argData;
    int    tokenLen, item_id;
    int    type, margs, strip, k;
    float	min, max, def_min, def_max;
    ROW_NODE    *rownode;
    ITEM_NODE   *tnode, *ptnode, *cnode;

    rownode = new_row_node();
    if (rownode == NULL)
	return(rownode);
	
    while (dataPtr != NULL)
    {
	while (*dataPtr == ' ' || *dataPtr == '\t')
	    dataPtr++;
	if (*dataPtr == '\0' || *dataPtr == '\n' || *dataPtr == '#')
	{
	    dataPtr = get_new_line();
	    if (dataPtr == NULL)
	        return(rownode);
	    if (debug > 2)
		fprintf(stderr, " %s\n", dataPtr);
	    continue;
	}
	if (*dataPtr == '}')
	    return(rownode);
	argData = get_token_and_id (dataPtr, token, &item_id, 1);
	if (argData == NULL)
	{
	    if (debug > 1)
		fprintf(stderr, " item resource is NULL ...\n");
	    return(rownode);
	}
	if (debug > 1)
	{
	   fprintf(stderr, "  get_token_and_id:  ok\n");
	   fprintf(stderr, "%s(%d)  %s", token, item_id, argData);
	}
	tokenLen = strlen (token);
	type = 0;
	margs = 0;
	tnode = NULL;
	arg = NULL;
	strip = 1;
	if (tokenLen >= 3)
	{
	     if (tokenLen < 8) 
	     {
		for (k = 0; k < numType1; k++)
		{
		    if (strcmp(token1[k], token) == 0)
		    {
			type = type1[k];
			strip = strip1[k];
			margs = margs1[k];
			break;
		    }
		}
	     }
	     else
	     {
		for (k = 0; k < numType2; k++)
		{
		    if (strcmp(token2[k], token) == 0)
		    {
			type = type2[k];
			strip = strip2[k];
			margs = margs2[k];
			break;
		    }
		}
	     }
	}
	if (type == 0 && debug)
	     fprintf(stderr, " parser: unknown type '%s' \n", token);
	if (debug > 1)
	    fprintf(stderr, " type: %d args: %d  strip: %d\n", type, margs, strip);
	if (type && margs == 0)
	{
	     tnode = new_item_node();
	     tnode->type = type;
	     tnode->id = item_id;
	     tnode->row_node = (caddr_t) rownode;
	     set_item_one_attribute (tnode, argData, strip);
	}
	else if (type)
	     tnode = set_item_multi_attribute (rownode, type, item_id, argData);
	if (tnode)
	{
	    if (rownode->item_list == NULL)
		rownode->item_list = tnode;
	    else
	    {
	        cnode = rownode->item_list;
	        while (cnode->next != NULL)
		    cnode = cnode->next;
		cnode->next = tnode;
	    }
	}
	dataPtr = get_new_line();
	if (debug > 2 && dataPtr)
	    fprintf(stderr, " %s\n", dataPtr);
    }
    return(rownode);
}


parse_new_def(def_entry, fname)
 subWin_entry  *def_entry;
 char   *fname;
{
	int       new_line, k;
	caddr_t   entry_addr;
	ROW_NODE  *retnode, *rownode, *prnode;
	
	if (def_entry == NULL)
	    return(NULL);

	if ((fin = fopen (fname, "r")) == NULL)
	    return(NULL);

	exit_row = NULL;
	entry_addr = (caddr_t) def_entry;
	while ((dataPtr = get_new_line()) != NULL)
	{
	   if (debug > 2)
	        fprintf(stderr, " %s\n", dataPtr);
	   while (*dataPtr == ' ' || *dataPtr == '\t')
		dataPtr++;
	   if ((int)strlen(dataPtr) > 0)
	   {
	      new_line = 0;
	      k = 0;
	      while (1)
	      {
	     	   switch (*dataPtr) {
		   case '#':  
		   case '!':  
		   case '\0':  
		   case '\n':  
			if (k > 0)
			    nameData[k] = '\0';
			new_line = 1;
			break;
		   case '{':
			dataPtr++;
			if (k > 0)
			    nameData[k] = '\0';
			retnode = parse_def_input(def_entry);
			if (retnode)
			{
			    if (retnode->subexp)
				def_entry->subexp = 1;
			    retnode->win_entry = entry_addr;
			    if (def_entry->vnode == NULL)
				def_entry->vnode = retnode;
			    else
			    {
			        rownode = def_entry->vnode;
				while (rownode->next != NULL)
				    rownode = rownode->next;
				rownode->next = retnode;
			    }
			    k = strlen(nameData) - 1;
			    while (k >= 0)
			    {
				if (nameData[k] == ' ' || nameData[k] == '\t')
				    nameData[k] = '\0';
				else
				    break;
				k--;
			    }
			    if ((int)strlen(nameData) > 0)
			    {
			        retnode->name = malloc (strlen(nameData) + 1);
				strcpy(retnode->name, nameData);
			    }
			}
			nameData[0] = '\0';
			break;
		   default :
			nameData[k] = *dataPtr;
			k++;
			dataPtr++;
			break;
	          }
	          if (new_line)
		  	break;
	      }
	   }
	}
	fclose(fin);
	if (def_entry->subexp)
	     update_subexp_data(def_entry);
	rownode = def_entry->vnode;
	k = 0;
	while (rownode != NULL)
	{
	     assign_row_value(rownode);
             k++;
             rownode = rownode->next;
        }
	if (exit_row == NULL)
	{
	     append_exit_button(def_entry);
	     k++;
	}
	else if (k > 3)
	{
	     prnode = def_entry->vnode;
	     while (prnode->next != NULL)
	     {
		 if (prnode->next == exit_row)
		     prnode->next = exit_row->next;	
		 if (prnode->next)
		     prnode = prnode->next;
		 else
		     break;
	     }
	     prnode->next = exit_row;
	     exit_row->next = NULL;
	}
	if (do_preview)
	     append_preview_button(exit_row);
	def_entry->itemNum = k;
	if (debug > 1)
	   print_entry (def_entry);
}



free_ch_subnode (node)
XCHOICE_NODE  *node;
{
	XCHOICE_NODE  *node1, *node2;

	node1 = node;
	while (node1 != NULL)
	{
	     node2 = node1->next;
	     if (node1->subexp && node1->label)
	     {
		free(node1->label);
		node1->label = NULL;
	     }
	     if (node1->subexp > 1)
	     {
		node->next = node2;
	        free(node1);
	     }
	     else
	     {
		node1->data = NULL;
		node = node1;
	     }
	     node1 = node2;
	}
}


free_xv_subnode (node)
XVALUE_NODE  *node;
{
	XVALUE_NODE  *node1, *node2;

	
	node1 = node;
	while (node1 != NULL)
	{
	     node2 = node1->next;
	     if (node1->subexp && node1->data)
	     {
		free(node1->data);
		node1->data = NULL;
	     }
	     if (node1->subexp > 1)
	     {
		node->next = node2;
	        free(node1);
	     }
	     else
		node = node1;
	     node1 = node2;
	}
}


free_subexp_data(pnode)
ITEM_NODE  *pnode;
{
	XPANE_NODE    *pane;
	XCHOICE_NODE  *chnode;
	XVALUE_NODE   *xvnode;

	switch (pnode->type) {
	  case XMENU:
	  case XABMENU:
	  case XCHOICE:
		    pane = (XPANE_NODE *) pnode->data_node;
		    chnode = (XCHOICE_NODE *)pane->wlist;
		    if (chnode != NULL)
			free_ch_subnode(chnode);
		    break;
	  case XVALUE:
	  case XMENVAL:
	  case XCHOICEVAL:
	  case XCHEXEC:
		    xvnode = (XVALUE_NODE *) pnode->data_node;
		    if (xvnode != NULL)
			free_xv_subnode(xvnode);
		    break;
	}
}


XCHOICE_NODE
*add_ch_subnode (pnode)
XCHOICE_NODE  *pnode;
{
	VAL_LINK      *vlink;
	XCHOICE_NODE  *cnode, *node2;

	vlink = subexp_list;
	node2 = pnode->next;
	if (vlink != NULL)
	{
	     pnode->label = (char *) malloc( strlen(vlink->data) + 1);
	     strcpy(pnode->label, vlink->data);
	     vlink = vlink->next;
	}
	while (vlink != NULL)
	{
	     cnode = (XCHOICE_NODE *)calloc (1, sizeof(XCHOICE_NODE));
	     cnode->label = (char *) malloc( strlen(vlink->data) + 1);
	     strcpy(cnode->label, vlink->data);
	     cnode->subexp = 2;
	     cnode->bid = 1;
	     cnode->next = node2;
	     pnode->next = cnode;
	     pnode = cnode;
	     vlink = vlink->next;
	}
	return(node2);
}


XVALUE_NODE
*add_xv_subnode (pnode)
XVALUE_NODE  *pnode;
{
	VAL_LINK     *vlink;
	XVALUE_NODE  *cnode, *node2;

	vlink = subexp_list;
	node2 = pnode->next;
	if (vlink != NULL)
	{
	     pnode->data = (char *) malloc( strlen(vlink->data) + 1);
	     strcpy(pnode->data, vlink->data);
	     vlink = vlink->next;
	}
	while (vlink != NULL)
	{
	     cnode = (XVALUE_NODE *)calloc (1, sizeof(XVALUE_NODE));
	     cnode->data = (char *) malloc( strlen(vlink->data) + 1);
	     strcpy(cnode->data, vlink->data);
	     cnode->subexp = 2;
	     cnode->next = node2;
	     pnode->next = cnode;
	     pnode = cnode;
	     vlink = vlink->next;
	}
	return(node2);
}


add_subexp_data(pnode)
ITEM_NODE  *pnode;
{
	XPANE_NODE    *pane;
	XCHOICE_NODE  *chnode;
	XVALUE_NODE   *xvnode;

	switch (pnode->type) {
	  case XMENU:
	  case XABMENU:
	  case XCHOICE:
		    pane = (XPANE_NODE *) pnode->data_node;
		    chnode = (XCHOICE_NODE *)pane->wlist;
		    while (chnode != NULL)
		    {
			if (chnode->subexp == 1)
			    chnode = add_ch_subnode(chnode);
			else
			    chnode = chnode->next;
		    }
		    break;
	  case XVALUE:
	  case XMENVAL:
	  case XCHOICEVAL:
	  case XCHEXEC:
		    xvnode = (XVALUE_NODE *) pnode->data_node;
		    while (xvnode != NULL)
		    {
			if (xvnode->subexp == 1)
			    xvnode = add_xv_subnode(xvnode);
			else
			    xvnode = xvnode->next;
		    }
		    break;
	}
}


create_subexp_link()
{
	DIR             *dirp;
        struct dirent   *dp;
	struct stat     status;
	VAL_LINK  	*vlink, *plink;

	vlink = subexp_list;
	while (vlink != NULL)
	{
	     plink = vlink;
	     vlink = plink->next;
	     if (plink->data)
		free(plink->data);
	     free(plink);
	}
	subexp_list = NULL;
	sprintf(tmpstr, "%s/subexp", expdir);
        dirp = opendir(tmpstr);
        if(dirp == NULL)
	     return;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
        {
           if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                continue;
	   sprintf(tmpstr, "%s/subexp/%s", expdir, dp->d_name);
           if (stat(tmpstr, &status) != 0)
                continue;
           if ((status.st_mode & S_IFDIR) == 0)
                continue;
	   vlink = (VAL_LINK *) calloc(1, sizeof(VAL_LINK));
	   vlink->data = (char *) malloc(sizeof (strlen(dp->d_name) +1));
	   strcpy(vlink->data, dp->d_name);
	   if (subexp_list == NULL)
		subexp_list = vlink;
	   else
		plink->next = vlink;
	   plink = vlink;
	}
	closedir(dirp);
}


set_node_value(head, snode)
ITEM_NODE  *head, *snode;
{
	ITEM_NODE   *node, *dnode;
	XVALUE_NODE *vnode;
	XINPUT_NODE *in_node;
	XTEXT_NODE  *txnode;
	XPANE_NODE  *pane;
	XCHOICE_NODE  *chnode;
	int	   type, id;

	node = head;
	type = snode->type;
	id = snode->id;
	while (node != NULL)
	{
	   dnode = NULL;
	   switch (node->type) {
	     case XMENU:
	     case XABMENU:
		    if (type == XVALUE || type == XMENVAL)
		    {
			if (node->id == id)
			    dnode = node;
		    }
		    break;
	     case XCHOICE:
		    if (type == XVALUE || type == XCHOICEVAL || type == XCHEXEC)
		    {
			if (node->id == id)
			    dnode = node;
		    }
		    break;
	     case XINPUT:
		    if ((type == XVALUE) && id > 0)
		    {
			if (node->id == id)
			{
	    	   	    vnode = (XVALUE_NODE *) snode->data_node;
			    in_node = (XINPUT_NODE *) node->data_node;
			    in_node->data = vnode->data;
			}
		    }
		    break;
	     case XTEXT:
		    if ((type == XVALUE) && id > 0)
		    {
			if (node->id == id)
			{
	    	   	    vnode = (XVALUE_NODE *) snode->data_node;
			    txnode = (XTEXT_NODE *) node->data_node;
			    txnode->data = vnode->data;
			}
		    }
		    break;
	   }
	   if (dnode)
	   {
	    	vnode = (XVALUE_NODE *) snode->data_node;
		pane = (XPANE_NODE *) dnode->data_node;
		chnode = (XCHOICE_NODE *)pane->wlist;
		while (chnode != NULL && vnode != NULL)
		{
		   while (chnode != NULL && chnode->label == NULL)
			chnode = chnode->next;
		   if (chnode == NULL)
			break;
		   while (vnode != NULL && vnode->data == NULL)
			vnode = vnode->next;
		   if (vnode == NULL)
			break;
		   if (type == XCHEXEC)
		       chnode->exec = vnode->data;
		   else
		       chnode->data = vnode->data;
		   chnode = chnode->next;
		   vnode = vnode->next;
		}
	   }
	   node = node->next;
	}
}

set_item_attribute(hnode, node)
ITEM_NODE     *hnode, *node;
{
	ITEM_NODE     *cnode;
	XVALUE_NODE   *vnode;
	XRADIO_NODE   *rdnode;
	ROW_NODE      *rownode;
	XVALUE_NODE   *xvnode;
	int	      id, type;

 	vnode = (XVALUE_NODE *) node->data_node;
	cnode = hnode;
	id = node->id;
	type = node->type;
	rownode = (ROW_NODE *) hnode->row_node;
	while (cnode != NULL)
	{
	   switch (cnode->type) {
	     case XMENU:
	     case XABMENU:
	     case XCHOICE:
	     case XBUTTON:
	     case XRADIO:
		      if (cnode->id != id)
			  break;
		      if (type == XEXEC)
			  cnode->execmd = 1;
		      else if (type == XRTOUT)
			  cnode->rtcmd = 1;
		      else if (type == XSHOW)
		      {
			  cnode->checkShow = 1;
			  rownode->checkItem = 1;
		      }
		      else if (type == XREMARK)
			  cnode->remark = vnode->data;
		      else if (cnode->type == XRADIO)
		      {
			  rdnode = (XRADIO_NODE *) cnode->data_node;
			  switch (type) {
			   case XUNSETRT:
			         	rdnode->unset_rtout = 1;
			      		cnode->rtcmd = 1;
					break;
			   case XUNSETEXEC:
			         	rdnode->unset_exec = 1;
			      		cnode->execmd = 1;
					break;
			   case XUNSETVAL:
			         	rdnode->unset_val = 1;
					break;
			   case XSETEXEC:
			         	rdnode->set_exec = 1;
			      		cnode->execmd = 1;
					break;
			   case XSETRT:
			         	rdnode->set_rtout = 1;
			      		cnode->rtcmd = 1;
					break;
			   case XSETVAL:
			         	rdnode->set_val = 1;
					break;
			   }
		      }
		      break;
	     case XINPUT:
	     case XTEXT:
		      if (cnode->id == id)
		      {
			  if (type == XSHOW)
			  {
			      cnode->checkShow = 1;
			      rownode->checkItem = 1;
			  }
			  else if (type == XREMARK)
			      cnode->remark = vnode->data;
		      }
		      break;
	    }
	    cnode = cnode->next;
	}
}

assign_row_value(rnode)
ROW_NODE   *rnode;
{
	ITEM_NODE     *head_node, *cnode, *dnode;
	XVALUE_NODE   *vnode;
	XINPUT_NODE   *in_node;
	int	      exFlag, butFlag;

	head_node = (ITEM_NODE *) rnode->item_list;
	cnode = head_node;
	exFlag = 0;
	butFlag = 0;
	while (cnode != NULL)
	{
	   switch (cnode->type) {
	      case XVALUE:
	      case XMENVAL:
	      case XCHOICEVAL:
	      case XCHEXEC:
		       set_node_value(head_node, cnode);
		       break;
	      case XROWS:
	      case XCOLS:
			dnode = head_node;
	    	   	vnode = (XVALUE_NODE *) cnode->data_node;
			while (dnode != NULL)
			{
			     if (dnode->type == XINPUT && dnode->id == cnode->id)
			     {
			        in_node = (XINPUT_NODE *) dnode->data_node;
				if (vnode->data != NULL)
				{
				    if (cnode->type == XROWS)
			                in_node->rows = atoi(vnode->data);
				    else
			                in_node->cols = atoi(vnode->data);
				}
			     }
			     dnode = dnode->next;
			}
		       break;
	      case XLABEL:
			vnode = (XVALUE_NODE *) cnode->data_node;
			if (vnode && vnode->data)
			{
			   rnode->label = vnode->data;
			   exFlag = 2;
			}
		       break;
	      case XOUTPUT:
			rnode->output = 1;
			exFlag = 2;
		       break;
	      case XRTOUT:
			if (cnode->id < 0)
			{
			   vnode = (XVALUE_NODE *) cnode->data_node;
			   if (vnode != NULL && vnode->data != NULL)
				rnode->rtcmd = 1;
			}
			else
			   set_item_attribute(head_node, cnode);
			break;
	      case XUNSETRT: /* check_set_rtoutput */
	      case XUNSETEXEC: /* check_unset_exec */
	      case XSETRT: /* check_set_rtoutput */
	      case XSETEXEC: /* check_set_exec */
			set_item_attribute(head_node, cnode);
			break;
	      case XREMARK:
			if (cnode->id < 0)
			{
			    vnode = (XVALUE_NODE *) cnode->data_node;
			    rnode->remark = vnode->data;
			}
			else
			   set_item_attribute(head_node, cnode);
			break;
	      case XEXEC:
			if (cnode->id < 0)
			{
			   vnode = (XVALUE_NODE *) cnode->data_node;
			   if (vnode != NULL && vnode->data != NULL)
				rnode->execmd = 1;
			}
			else
			   set_item_attribute(head_node, cnode);
			if (exFlag == 0)
			{
			   vnode = (XVALUE_NODE *) cnode->data_node;
			   while (vnode != NULL)
			   {
				if (vnode->data)
				{
				    if (strstr(vnode->data, "EXIT") != NULL)
				    {
					 exFlag = 1;
					 break;
				    }
				}
				vnode = vnode->next;
			   }
			}
			break;
	      case XSHOW:
			if (cnode->id < 0)
			{
			   vnode = (XVALUE_NODE *) cnode->data_node;
			   if (vnode != NULL && vnode->data != NULL)
				rnode->checkShow = 1;
			}
			else
			   set_item_attribute(head_node, cnode);
			exFlag = 2;
			break;
	      case XMIN:
	      case XMAX:
		       break;
	     case XMENU:
	     case XABMENU:
	     case XCHOICE:
	     case XRADIO:
	     case XINPUT:
	     case XTEXT:
			exFlag = 2;
			break;
	     case XBUTTON:
			butFlag = 1;
			break;
	    }
	    cnode = cnode->next;
	}
	if (butFlag && exFlag == 1)
	    exit_row = rnode;
}


assign_item_value( head_node )
ITEM_NODE  *head_node;
{
	ITEM_NODE     *cnode, *dnode;
	XVALUE_NODE   *vnode;
	XINPUT_NODE   *in_node;

	cnode = head_node;
	while (cnode != NULL)
	{
	   switch (cnode->type) {
	      case XVALUE:
	      case XMENVAL:
	      case XCHOICEVAL:
	      case XCHEXEC:
		       set_node_value(head_node, cnode);
		       break;
	      case XROWS:
	      case XCOLS:
			dnode = head_node;
	    	   	vnode = (XVALUE_NODE *) cnode->data_node;
			while (dnode != NULL)
			{
			     if (dnode->type == XINPUT && dnode->id == cnode->id)
			     {
			        in_node = (XINPUT_NODE *) dnode->data_node;
				if (vnode->data != NULL)
				{
				    if (cnode->type == XROWS)
			                in_node->rows = atoi(vnode->data);
				    else
			                in_node->cols = atoi(vnode->data);
				}
			     }
			     dnode = dnode->next;
			}
		       break;
	    }
	    cnode = cnode->next;
	}
	     
}


free_subexp_nodes(def_entry)
 subWin_entry  *def_entry;
{
	ROW_NODE   *rnode;
	ITEM_NODE  *cnode;

	rnode = def_entry->vnode;
	while (rnode != NULL)
	{
	     if (rnode->subexp)
	     {
		  cnode = (ITEM_NODE *)rnode->item_list;
		  while (cnode != NULL)
		  {
			if (cnode->subexp)
			   free_subexp_data(cnode);
			cnode = cnode->next;
		  }
	     }
	     rnode = rnode->next;
	}
}


update_subexp_data(dentry)
 subWin_entry  *dentry;
{
	ROW_NODE   *rnode;
	ITEM_NODE  *cnode;

	create_subexp_link();
	rnode = dentry->vnode;
	while (rnode != NULL)
	{
	     if (rnode->subexp)
	     {
		  cnode = (ITEM_NODE *)rnode->item_list;
		  while (cnode != NULL)
		  {
			if (cnode->subexp)
			     add_subexp_data(cnode);
			cnode = cnode->next;
		  }
	     }
	     rnode = rnode->next;
	}
}


check_update_subexp(dentry)
subWin_entry  *dentry;
{
	ROW_NODE   *rnode;
	ITEM_NODE  *cnode;

	free_subexp_nodes(dentry);
	create_subexp_link();
	rnode = dentry->vnode;
	while (rnode != NULL)
	{
	     if (rnode->subexp)
	     {
		  cnode = (ITEM_NODE *)rnode->item_list;
		  while (cnode != NULL)
		  {
			if (cnode->subexp)
			     add_subexp_data(cnode);
			cnode = cnode->next;
		  }
	          assign_item_value((ITEM_NODE *)rnode->item_list);
	     }
	     rnode = rnode->next;
	}
}


static
print_output (node, type, id)
ITEM_NODE  *node;
int	   type, id;
{
	XVALUE_NODE *xvnode;
	int	    row;

	row = 0;
	while (node != NULL)
	{
	   if (node->type == type && node->id == id)
	   {
	      if (row > 0)
		  fprintf(stderr, "\n      (%d):", row);
	      xvnode = (XVALUE_NODE *) node->data_node;
	      while (xvnode != NULL)
	      {
		  if (xvnode->data != NULL)
		      fprintf(stderr, " %s,", xvnode->data);
		  xvnode = xvnode->next;
	      }
	      row++;
	   }
	   node = node->next;
	}
	fprintf(stderr, "\n");
}

static
print_item (hnode, node)
ITEM_NODE  *hnode, *node;
{
	XVALUE_NODE *xvnode;
	XPANE_NODE  *pane;
	XTEXT_NODE  *txnode;
	XINPUT_NODE *in_node;
	XCHOICE_NODE  *chnode;
	XBUTTON_NODE  *xbutton;
	XRADIO_NODE   *rdbutton;

	if (node->remark)
	    fprintf(stderr, "   remark: %s\n", node->remark);
	if (node->rtcmd)
	{
	    fprintf(stderr, "   rtoutput: ");
	    print_output(hnode, XRTOUT, node->id);
	}
	if (node->execmd)
	{
	    fprintf(stderr, "   exec: ");
	    print_output(hnode, XEXEC, node->id);
	}
	if (node->checkShow)
	{
	    fprintf(stderr, "   show: ");
	    print_output(hnode, XSHOW, node->id);
	}
	if (node->subexp)
	    fprintf(stderr, "   SUBEXP...\n");
	switch (node->type) {
	 case XINPUT:
		    in_node = (XINPUT_NODE *) node->data_node;
		    if (in_node->data)
		       fprintf(stderr, "   data: %s\n", in_node->data);
		    else
		       fprintf(stderr, "   data: NULL\n");
		    fprintf(stderr, "   row, col: %d, %d\n", in_node->rows, in_node->cols);
		    break;
	 case XBUTTON:
		    xbutton = (XBUTTON_NODE *) node->data_node;
		    if (xbutton->label)
		       fprintf(stderr, "   label: %s\n", xbutton->label);
		    else
		       fprintf(stderr, "   label: NULL\n");
		    break;
	 case XRADIO:
		    rdbutton = (XRADIO_NODE *) node->data_node;
		    if (rdbutton->label)
		       fprintf(stderr, "   label: %s\n", rdbutton->label);
		    else
		       fprintf(stderr, "   label: NULL\n");
		    if (rdbutton->set_exec)
		    {
			fprintf(stderr, "   set_exec: ");
			print_output(hnode, XSETEXEC, node->id);
		    }
		    if (rdbutton->unset_exec)
		    {
			fprintf(stderr, "   unset_exec: ");
			print_output(hnode, XUNSETEXEC, node->id);
		    }
		    if (rdbutton->set_rtout)
		    {
			fprintf(stderr, "   set_rtout: ");
			print_output(hnode, XSETRT, node->id);
		    }
		    if (rdbutton->unset_rtout)
		    {
			fprintf(stderr, "   unset_rtout: ");
			print_output(hnode, XUNSETRT, node->id);
		    }
		    if (rdbutton->set_val)
		    {
			fprintf(stderr, "   set_val: ");
			print_output(hnode, XSETVAL, node->id);
		    }
		    if (rdbutton->unset_val)
		    {
			fprintf(stderr, "   unset_val: ");
			print_output(hnode, XUNSETVAL, node->id);
		    }
		    break;
	 case XTEXT:
		    txnode = (XTEXT_NODE *) node->data_node;
		    if (txnode->data)
		       fprintf(stderr, "   data: %s\n", txnode->data);
		    else
		       fprintf(stderr, "   data: NULL\n");
		    break;
	 case XMENU:
  	 case XABMENU:
  	 case XCHOICE:
		    pane = (XPANE_NODE *) node->data_node;
		    if (pane->subexp)
		       fprintf(stderr, "   SUBEXP...\n");
		    chnode = (XCHOICE_NODE *) pane->wlist;
		    fprintf(stderr, "   label: ");
		    while (chnode != NULL)
		    {
			if (chnode->label)
		           fprintf(stderr, "%s, ", chnode->label);
			else
		           fprintf(stderr, "??, ");
			chnode = chnode->next;
		    }
		    chnode = (XCHOICE_NODE *) pane->wlist;
		    fprintf(stderr, "\n    value: ");
		    while (chnode != NULL)
		    {
			if (chnode->data)
		           fprintf(stderr, "%s, ", chnode->data);
			else
		           fprintf(stderr, "??, ");
			chnode = chnode->next;
		    }
		    fprintf(stderr, "\n");
		    break;
	}
}


print_entry(dentry)
subWin_entry  *dentry;
{
	ROW_NODE   *rnode;
	ITEM_NODE  *cnode, *dnode;

	rnode = dentry->vnode;
	while (rnode != NULL)
	{
	    cnode = (ITEM_NODE *)rnode->item_list;
	    if (rnode->name)
		fprintf(stderr, "name: %s\n", rnode->name);
	    if (rnode->label)
		fprintf(stderr, "label: %s\n", rnode->label);
	    else
		fprintf(stderr, "label: none\n");
	    if (rnode->remark)
		fprintf(stderr, "remark: %s\n", rnode->remark);
	    if (rnode->output)
	    {
		fprintf(stderr, "output: ");
		print_output(cnode, XOUTPUT, -1);
	    }
	    if (rnode->rtcmd)
	    {
		fprintf(stderr, "rtoutput: ");
		print_output(cnode, XRTOUT, -1);
	    }
	    if (rnode->execmd)
	    {
		fprintf(stderr, "exec: ");
		print_output(cnode, XEXEC, -1);
	    }
	    if (rnode->checkShow)
	    {
		fprintf(stderr, "show: ");
		print_output(cnode, XSHOW, -1);
	    }
	    if (rnode->subexp)
		fprintf(stderr, "SUBEXP...\n");
	    dnode = cnode;
	    while (dnode != NULL)
	    {
	        switch (dnode->type) {
		 case XINPUT:
			if (dnode->id > 0)
			    fprintf(stderr, "  input (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  input \n");
			print_item (cnode, dnode);
			break;
		 case XBUTTON:
			if (dnode->id > 0)
			    fprintf(stderr, "  button (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  button \n");
			print_item (cnode, dnode);
			break;
		 case XRADIO:
			if (dnode->id > 0)
			    fprintf(stderr, "  check button (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  check button \n");
			print_item (cnode, dnode);
			break;
		 case XTEXT:
			if (dnode->id > 0)
			    fprintf(stderr, "  text (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  text \n");
			print_item (cnode, dnode);
			break;
		 case XMENU:
			if (dnode->id > 0)
			    fprintf(stderr, "  menu (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  menu \n");
			print_item (cnode, dnode);
			break;
	  	 case XABMENU:
			if (dnode->id > 0)
			    fprintf(stderr, "  ab menu (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  ab menu \n");
			print_item (cnode, dnode);
			break;
	  	 case XCHOICE:
			if (dnode->id > 0)
			    fprintf(stderr, "  choice (%d)\n", dnode->id);
			else
			    fprintf(stderr, "  choice \n");
			print_item (cnode, dnode);
			break;
		}
		dnode = dnode->next;
	    }
	    rnode = rnode->next;
	}
}


static
copy_one_row_text(fd, w)
FILE     *fd;
Widget   w;
{
        char    *data;

        XtSetArg (args[0], XtNstring, &data);
        XtGetValues(w, args, 1);
        if (data != NULL)
            fprintf(fd, "%s", data);
        XtFree(data);
}

static
copy_multi_row_text(fd, w)
FILE     *fd;
Widget   w;
{
        char    *text, *data;

        OlTextEditCopyBuffer((TextEditWidget)w, &text);
        if (text != NULL)
        {
            data = text;
            while (*data != '\0')
            {
                if (*data == '\n')
                   fprintf(fd, " \\\n");
                else
                   fprintf(fd, "%c", *data);
                data++;
            }
        }
        XtFree(text);
}


char
*get_type_by_name(type)
int	type;
{
	int	k;

	for (k = 0; k < numType1; k++)
	{
	    if (type == type1[k])
		return(token1[k]);
	}

	for (k = 0; k < numType2; k++)
	{
	    if (type == type2[k])
		return(token2[k]);
	}
	return((char *) NULL);
}


copy_def_row_item (fout, rownode)
FILE       *fout;
ROW_NODE  *rownode;
{
        ITEM_NODE *node, *hnode;
	int	  type, k;
	char	    *label;
	XVALUE_NODE *xvnode;
	XPANE_NODE  *pane;
	XTEXT_NODE  *txnode;
	XINPUT_NODE *innode;
	XCHOICE_NODE  *chnode;
	XBUTTON_NODE  *xbutton;
	XRADIO_NODE   *rdbutton;

        hnode = (ITEM_NODE *) rownode->item_list;
        node = hnode;
        while (node != NULL)
        {
	    label = get_type_by_name(node->type);
	    if (label == NULL)
	    {
		node = node->next;
		continue;
	    }
            fprintf(fout, " %s: ", label);
            switch (node->type) {
             case XLABEL:
             case XOUTPUT:
             case XRTOUT:
	     case XUNSETRT:
	     case XUNSETEXEC:
	     case XUNSETVAL:
	     case XREMARK:
	     case XEXEC:
	     case XSHOW:
	     case XSETRT:
	     case XSETEXEC:
	     case XSETVAL:
	     case XROWS:
	     case XCOLS:
	     case XMIN:
	     case XMAX:
			xvnode = (XVALUE_NODE *) node->data_node;
			while (xvnode != NULL)
			{
                           if (xvnode->data)
                                fprintf(fout, "\"%s\" ", xvnode->data);
			   xvnode = xvnode->next;
		        }
			break;
             case XINPUT:
			innode = (XINPUT_NODE *) node->data_node;
                        if (innode->widget)
                        {
                            if (innode->rows <= 1)
                                copy_one_row_text(fout, innode->widget);
                            else
                                copy_multi_row_text(fout, innode->widget);
                        }
             		break;
             case XBUTTON:
			xbutton = (XBUTTON_NODE *) node->data_node;
                    	if (xbutton->label)
			    fprintf(fout, "\"%s\" ", xbutton->label);
             		break;
             case XRADIO:
			rdbutton = (XRADIO_NODE *) node->data_node;
                    	if (rdbutton->label)
                       	    fprintf(fout, "\"%s\" ", rdbutton->label);
             		break;
             case XTEXT:
			txnode = (XTEXT_NODE *) node->data_node;
                        if (txnode->data)
                            fprintf(fout, "\"%s\"", txnode->data);
             		break;
             case XMENU:
             case XABMENU:
             case XCHOICE:
			pane = (XPANE_NODE *) node->data_node;
                    	chnode = (XCHOICE_NODE *) pane->wlist;
			while (chnode != NULL)
			{
			    if (chnode->subexp)
			    {
			        if (chnode->subexp == 1)
				    fprintf(fout, "SUBEXP ");
			    }
			    else if (chnode->label)
				fprintf(fout, "\"%s\" ", chnode->label);
			    chnode = chnode->next;
			}
             		break;
             case XVALUE:
             case XMENVAL:
             case XCHOICEVAL:
             case XCHEXEC:
			xvnode = (XVALUE_NODE *) node->data_node;
			while (xvnode != NULL)
			{
			    if (xvnode->subexp)
			    {
			        if (xvnode->subexp == 1)
				    fprintf(fout, "SUBEXP ");
			    }
                            else if (xvnode->data)
                                fprintf(fout, "\"%s\" ", xvnode->data);
			    xvnode = xvnode->next;
		        }
             		break;
            }
	    fprintf(fout, "\n");
            node = node->next;
        }
}

static char *blabels[] = {"Ok", "Reset", "Exit", "Preview" };
static char *bexecs[] = {"DO", "RESET", "EXIT", "PREVIEW" };

append_exit_button(entry)
 subWin_entry  *entry;
{
	int	     count, k;
	XBUTTON_NODE *bnode;
	XVALUE_NODE  *vnode;
        ROW_NODE     *row1, *row2;
        ITEM_NODE    *node1, *node2;

        row1 = entry->vnode;
	if (row1 == NULL)
	    return;
        while (row1->next != NULL)
	    row1 = row1->next;
	row2 = (ROW_NODE *) calloc(1, sizeof(ROW_NODE));
	row2->show = 1;
	row2->win_entry = (caddr_t) entry;
	row1->next = row2;
	exit_row = row2;
	k = 3;
	for (count = 0; count < k; count++)
	{
	   node2 = (ITEM_NODE *) calloc(1, sizeof(ITEM_NODE));
	   node2->row_node = (caddr_t) row2;
	   node2->show = 1;
	   node2->type = XBUTTON;
	   node2->execmd = 1;
	   node2->id = count + 1;
	   if (row2->item_list == NULL)
		row2->item_list = node2;
	   else
		node1->next = node2;
	   node1 = node2;
	   bnode = (XBUTTON_NODE *) calloc(1, sizeof(XBUTTON_NODE));
	   bnode->label = blabels[count];
	   node2->data_node = (caddr_t) bnode;

	   node2 = (ITEM_NODE *) calloc(1, sizeof(ITEM_NODE));
	   node2->row_node = (caddr_t) row2;
	   node2->show = 1;
	   node2->type = XEXEC;
	   node2->id = count + 1;
	   node1->next = node2;
	   node1 = node2;
	   vnode = (XVALUE_NODE *) calloc(1, sizeof(XVALUE_NODE));
	   vnode->data = bexecs[count];
	   node2->data_node = (caddr_t) vnode;
	}
}


append_preview_button(rnode)
ROW_NODE *rnode;
{
	XBUTTON_NODE *bnode;
	XVALUE_NODE  *vnode;
        ITEM_NODE    *node1, *node2;
	int	     id;

	node1 = rnode->item_list;
	id = 2;
	while (node1 != NULL)
	{
	   if (node1->id >= id)
		id = node1->id + 1;
	   if (node1->next == NULL)
		break;
	   node1 = node1->next;
	}
	node2 = (ITEM_NODE *) calloc(1, sizeof(ITEM_NODE));
	node2->row_node = (caddr_t) rnode;
	node2->show = 1;
	node2->type = XBUTTON;
	node2->execmd = 1;
	node2->id = id;
	node1->next = node2;
	node1 = node2;
	bnode = (XBUTTON_NODE *) calloc(1, sizeof(XBUTTON_NODE));
	bnode->label = blabels[3];
	node2->data_node = (caddr_t) bnode;

	node2 = (ITEM_NODE *) calloc(1, sizeof(ITEM_NODE));
	node2->row_node = (caddr_t) rnode;
	node2->show = 1;
	node2->type = XEXEC;
	node2->id = id;
	node1->next = node2;
	vnode = (XVALUE_NODE *) calloc(1, sizeof(XVALUE_NODE));
	vnode->data = bexecs[3];
	node2->data_node = (caddr_t) vnode;
}

