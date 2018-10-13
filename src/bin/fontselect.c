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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/DrawingAP.h>
#include <Xm/PushB.h>
#include <Xm/Protocols.h>


#define   MAXPARAM	14
#define   FONTNAME  0
#define   FAMILY    1
#define   WEIGHT    2
#define   SLANT     3
#define   SETWIDTH  4
#define   HEIGHT    6
#define   WIDTH     11
#define   ALIAS     12
#define   FONTNUM   5

Widget    topShell;
Widget    formShell;
Widget    dispWindow;
Widget    butWindow;
Widget    closeButton;
Pixel     winBg, winFg;
Pixel     hiPix;
Arg       args[16];
GC	  gc;
GC	  font_gc;

int	screenWidth, screenHeight;
char **fontList;

Display   *dpy;
Colormap  cmap;


typedef  struct  _mrec {
		char	*val;
		int	scalable;
		int	id;   /* the level number */
		int	fid;  /* font id  */
		int	mid;  /* the order in menu */
		double  hratio;
	        struct  _mrec  *child;
	        struct  _mrec  *parent;
	        struct  _mrec  *next;
	}  MENU_REC;

static MENU_REC  *main_list = NULL;
static MENU_REC  *main_alias_list = NULL;

typedef  struct  _xrec {
		int	id;
		char	*val;
	        struct  _xrec  *next;
	}  SUB_REC;

static SUB_REC  *name_list = NULL;
static SUB_REC  *family_list = NULL;
static SUB_REC  *weight_list = NULL;
static SUB_REC  *slant_list = NULL;
static SUB_REC  *slant2_list = NULL;
static SUB_REC  *swidth_list = NULL;
static SUB_REC  *height_list = NULL;
static SUB_REC  *width_list = NULL;
static SUB_REC  *alias_list = NULL;
static int	name_list_len, family_list_len, weight_list_len;
static int	slant_list_len, height_list_len, width_list_len;
static int	swidth_list_len, alias_list_len;
static int	in_pipe = -1;
static int	out_pipe = -1;
static int	win_width = 0;
static int	cur_id = -1;
static int	rows = 1;
static int	cols = 1;
static int	smallWindow = 1;
static int	def_height = 16;
static int	def_ascent = 12;

typedef  struct  _xfrec {
		int	 id;
		int	 fid;
		int	 scaled;
		Widget   label_wg;
		Widget   name_wg;
		Widget   *name_wg_list;
		Widget   family_wg;
		Widget   *family_wg_list;
		Widget   weight_wg;
		Widget   *weight_wg_list;
		Widget   slant_wg;
		Widget   *slant_wg_list;
		Widget   height_wg;
		Widget   *height_wg_list;
		Widget   width_wg;
		Widget   *width_wg_list;
		Widget   alias_wg;
		Widget   *alias_wg_list;
		int	 item_select[ALIAS+1];
		int	 height_id;
		int	 width_id;
		double	 hratio;
	} FONT_REC;

FONT_REC   font_rec[FONTNUM];

static int    parLens[MAXPARAM] = {120, 120, 32, 32, 32, 12, 12, 
			     	    12, 12, 12, 12, 12, 120, 120};
static char  *array_size[FONTNUM] = {"300", "500", "700", "900", "900+"};
static int   font_sizes[FONTNUM] = {300, 500, 700, 900, 9000};
static char  *pars[MAXPARAM];
static char  *empty = "none";
static char  *zero = "0";
static char  *alias_name = "alias";
static int   font_id;
static int   font_num;
static int   scaleFont = 0;
static int   is_scalable;
static int   no_scale = 1;
static int   has_alias = 0;
static char  *def_fonts[FONTNUM];
static int   font_height[FONTNUM];
static int   font_width[FONTNUM];
static XFontStruct  *xfont_info[FONTNUM];
static XrmDatabase  dbase = NULL;
static XrmDatabase  dbase2 = NULL;
static XrmDatabase  dbase3 = NULL;
static int   diffx = 0;
static int   diffy = 0;
static int   dispWinY = 0;
static int   butWinH = 0;
static double   height_ratio;
static XtInputId      pipeid = 0;

Widget  label_wg[6];
char    *label_vals[6] = {"Name","Family","Weight","Slant","Height","Width"};


static XrmOptionDescRec options[] = {
  {"-fn",      "*fontList",  XrmoptionSepArg,       "8x13"},
};

read_from_vnmr(c_data, fd, id)
 XtPointer  c_data;
 int        *fd;
 XtInputId  id;
{
     int     n, k, v1, v2, v3;
     XEvent  event;

     static char  str[256], cmd[12], nfont[256];

     if (*fd < 0)
        return;
     if ((n = read(*fd, str, 256)) > 1)
     {
        str[n] = '\0';
        if (str[n-1] == '\n')
           str[n-1] = '\0';
	k = strlen(str);
	while (k > 0)
	{
	   if (str[k-1] == ' ')
		str[k-1] = '\0';
	   else
		break;
	   k--;
	}
	if (sscanf(str, "%s", cmd) != 1)
	   return;
	if (strcmp(cmd, "exit") == 0)
	{
	   if (pipeid > 0)
                XtRemoveInput(pipeid);
	   exit(0);
	}
	if (strcmp(cmd, "up") == 0)
	   XMapRaised(dpy, XtWindow(topShell));
	if (strcmp(cmd, "font") == 0)
	{
	   if (sscanf(str, "%s%d%d%d%s", cmd, &v1, &v2, &v3, nfont) != 5)
	   	return;
	   save_proc();
	   smallWindow = v1;
	   rows = v2;
	   cols = v3;
	   strcpy(def_fonts[0], nfont);
	   disp_vnmr_font();
	}
     }
}

static void
WmExitProgram( w, client_data, event )
Widget w;
char *client_data;
void *event;
{
	save_proc();
}


main(argc, argv)
int	argc;
char	**argv;
{
	int	k;
	char	*p;
        char    xfile[256];
	Atom    deleteAtom;

	topShell = XtInitialize("Fontselect", "Fontselect", 
		options, XtNumber(options), &argc, argv);
	dpy = XtDisplay(topShell);
	screenWidth = DisplayWidth (dpy, DefaultScreen(dpy));
	screenHeight = DisplayHeight (dpy, DefaultScreen(dpy));
	dbase = XtDatabase(dpy);
	for ( k = 0; k < argc; k++)
	{
	    if ((argv[k][0] == '-') && ((int) strlen(argv[k]) >= 3) )
	    {
	     	if (strcmp(argv[k], "-scale") == 0)
		    no_scale = 0;
		else
		{
		   switch (argv[k][1]) {
		    case 'h': 
			     out_pipe = 1;
			     initVnmrComm(&argv[k][2]);
			     break;
		    case 'I': 
			     in_pipe = atoi(&argv[k][2]);
			     break;
		    case 'G': 
			     win_width = atoi(&argv[k][2]);
			     break;
		    case 'R': 
			     rows = atoi(&argv[k][2]);
			     break;
		    case 'C': 
			     cols = atoi(&argv[k][2]);
			     break;
		    case 'W': 
			     smallWindow = atoi(&argv[k][2]);
			     break;
		  }
		}
	    }
	}
	if (in_pipe >= 0)
	{
	    pipeid = XtAddInput(in_pipe, (caddr_t)XtInputReadMask,
                (XtInputCallbackProc)read_from_vnmr, (XtPointer)NULL);
	}
	create_font_list();
        XtSetArg(args[0], XmNtitle, "FontSelect");
	XtSetValues(topShell, args, 1);
	p = (char *)getenv("HOME");
	if (p != NULL)
	{
	    sprintf(xfile, "%s/app-defaults/Vnmr", p);
	    if (access (xfile, R_OK) == 0)
		dbase2 = XrmGetFileDatabase(xfile);
	}
	p = (char *)getenv("vnmrsystem");
	if (p != NULL)
	{
	    sprintf(xfile, "%s/app-defaults/Vnmr", p);
	    if (access (xfile, R_OK) == 0)
		dbase3 = XrmGetFileDatabase(xfile);
	}

	create_font_window();
	gc = DefaultGC (dpy, DefaultScreen(dpy));
	XtRealizeWidget (topShell);
	set_def_font();
	font_gc = XCreateGC(dpy, XtWindow(dispWindow), 0, 0);
	set_def_values();
	deleteAtom = XmInternAtom(dpy, "WM_DELETE_WINDOW", FALSE);
	XmAddProtocolCallback(topShell, XM_WM_PROTOCOL_ATOM(topShell),
                        deleteAtom, WmExitProgram, 0);
	sprintf(xfile, "fontselect('SYNCFUNC', 199, 'null')\n");
	if (out_pipe >= 0)
	    sendToVnmr(xfile);
	XtMainLoop();
}

char *
get_x_resource(class, value)
char  *class, *value;
{
        char       info_str[128];
        char       *str_type;
        XrmValue   rval;

        if (dbase == NULL && dbase2 == NULL  && dbase3 == NULL)
           return((char *) NULL);
        sprintf(info_str, "%s*%s", class, value);
        if (XrmGetResource(dbase, info_str, class, &str_type, &rval))
           return((char *) rval.addr);
        if (XrmGetResource(dbase, value, class, &str_type, &rval))
           return((char *) rval.addr);
	if (dbase2 != NULL)
	{
	   if (XrmGetResource(dbase2, info_str, class, &str_type, &rval))
              return((char *) rval.addr);
           if (XrmGetResource(dbase2, value, class, &str_type, &rval))
              return((char *) rval.addr);
	}
	if (dbase3 != NULL)
	{
           if (XrmGetResource(dbase3, info_str, class, &str_type, &rval))
              return((char *) rval.addr);
           if (XrmGetResource(dbase3, value, class, &str_type, &rval))
              return((char *) rval.addr);
	}
        return((char *) NULL);
}



int
set_font_args (fname, modify)
char	*fname;
int	modify;
{
	int	k, m;
	char	*data;
	char	*source;

	k = 0;
	is_scalable = 0;
	source = fname;
	while (*source == ' ' || *source == '\t')
	    source++;
	k = 0;
	data = pars[0];
	*data = '\0';
	while (*source != '\0')
	{
	    if (*source == '-')
	    {
		*data = '\0';
		if (k >= MAXPARAM)
		    return(-1);
	        data = pars[k];
		k++;
	    }
	    else
	    {
		*data = *source;
		data++;
	    }
	    source++;
	}
	if (*(data-1) == '\n')
	    *(data-1) = '\0';
	*data = '\0';
	if (k == MAXPARAM)
	{
	    if (modify && (strlen(pars[FAMILY]) != 0))
	    {
		if (strcmp(pars[FAMILY], "symbol") == 0)
		   return(0);
	    }
	    if (modify && (strlen(pars[FONTNAME]) == 0))
	       strcpy(pars[FONTNAME], empty);
	    add_to_attr (pars[FONTNAME], FONTNAME);
	    if (modify && (strlen(pars[FAMILY]) == 0))
	       strcpy(pars[FAMILY], empty);
	    add_to_attr (pars[FAMILY], FAMILY);
	    if (modify && (strlen(pars[WEIGHT]) == 0))
	       strcpy(pars[WEIGHT], empty);
	    add_to_attr (pars[WEIGHT], WEIGHT);
	    add_to_attr (pars[SLANT], SLANT);
	    add_to_attr (pars[SETWIDTH], SETWIDTH);
	    if (no_scale == 0)
	    {
	   	if (atoi(pars[HEIGHT]) == 0)
	   	{
	    	    if ((atoi(pars[HEIGHT+1]) == 0) && (atoi(pars[WIDTH]) == 0))
	    	    {
		    	is_scalable = 1;
		    	scaleFont = 1;
	    	    }
	   	}
	   	else
	    	    height_ratio = (double)atoi(pars[HEIGHT+1]) / (double)atoi(pars[HEIGHT]);
	    }
	    if (modify && (strlen(pars[WIDTH]) > 1))
	    	pars[WIDTH][strlen(pars[WIDTH])-1] = '\0';
	    if (modify && (is_scalable == 0))
	    {
	    	add_sort_attr(pars[HEIGHT], HEIGHT);
	    	add_sort_attr(pars[WIDTH], WIDTH);
	    }
	}
	if (k == 0)
	{
	    if (strlen(pars[0]) > 0)
	    {
		k = 1;
		add_to_attr (pars[0], ALIAS);
		has_alias = 1;
	    }
	}
	return(k);
}

append_font_attr(pnode, num)
MENU_REC   *pnode;
int	   num;
{
	MENU_REC   *node, *new_node;
	char	   *name;
	int	   ret;

	node = pnode->child;
	name = pars[num];
	while (node != NULL)
	{
	    if ((node->val[0] == name[0]) && (strcmp(node->val, name) == 0))
		break;
	    node = node->next;
	}
	if (node == NULL)
	{
	    new_node = (MENU_REC *) calloc(1, sizeof(MENU_REC));
	    new_node->val = (char *) malloc(strlen(name) + 1);
	    strcpy(new_node->val, name);
	    new_node->parent = pnode;
	    new_node->id = num;
	    new_node->mid = -1;
	    new_node->hratio = 10.0;
	    new_node->fid = font_id;
	    if (pnode->child == NULL)
		pnode->child = new_node;
	    else
	    {
		node = pnode->child;
		while (node->next != NULL)
		     node = node->next;
		node->next = new_node;
	    }
	    node = new_node;
	}
	switch (num) {
	    case FAMILY:
		    append_font_attr(node, WEIGHT);
		    break;	
	    case WEIGHT:
		    append_font_attr(node, SLANT);
		    break;	
	    case SLANT:
		    if (is_scalable)
			node->scalable = 1;
		    append_font_attr(node, SETWIDTH);
		    break;	
	    case SETWIDTH:
		    if (is_scalable && node->scalable)
			return;
		    if (is_scalable)
			node->scalable = 1;
		    else
		        node->hratio = height_ratio;
		    append_font_attr(node, HEIGHT);
		    break;	
	    case HEIGHT:
		    node->scalable = is_scalable;
		    append_font_attr(node, WIDTH);
		    break;	
	    case WIDTH:
		    node->scalable = is_scalable;
		    break;	
	}
}


add_new_font()
{
	MENU_REC   *node, *new_node;
	char	   *name;
	int	   name_num;

	node = main_list;
	name_num = 0;
	name = pars[FONTNAME];
	while (node != NULL)
	{
	    if ((node->val[0] == name[0]) && (strcmp(node->val, name) == 0))
		break;
	    name_num++;
	    node = node->next;
	}
	if (node == NULL)
	{
	    new_node = (MENU_REC *) calloc(1, sizeof(MENU_REC));
	    new_node->val = (char *) malloc(strlen(name) + 1);
	    strcpy(new_node->val, name);
	    new_node->id = FONTNAME;
	    new_node->mid = name_num;
	    if (main_list == NULL)
		main_list = new_node;
	    else
	    {
		node = main_list;
		while (node->next != NULL)
		     node = node->next;
		node->next = new_node;
	    }
	    node = new_node;
	}
	append_font_attr(node, FAMILY);
}

add_alias_fonts()
{
	MENU_REC   *node, *new_node;
	SUB_REC    *snode, *new_snode;
	int	   num;

	new_node = (MENU_REC *) calloc(1, sizeof(MENU_REC));
	new_node->val = (char *) malloc(8);
	strcpy(new_node->val, alias_name);
	new_node->id = FONTNAME;
	new_node->child = main_alias_list;
	node = main_alias_list;
	while (node != NULL)
	{
	     node->parent = new_node;
	     node = node->next;
	}
	main_alias_list->parent = new_node;
	new_node->next = main_list;
	main_list = new_node;
	node = main_list;
	num = 0;
	while (node != NULL)
	{
	    node->mid = num;
	    num++;
	    node = node->next;
	}
	new_snode = (SUB_REC *) calloc(1, sizeof(SUB_REC));
        new_snode->val = (char *) malloc(8);
        strcpy(new_snode->val, alias_name);
	new_snode->next = name_list;
	name_list = new_snode;
	name_list_len++;
	num = 0;
	while (new_snode != NULL)
	{
	    new_snode->id = num;
	    num++;
	    new_snode = new_snode->next;
	}
}


create_font_list()
{ 
    int    field, count;
    char	exname[6];
    SUB_REC     *s1, *s2, *s3;

    fontList =
        XListFonts(dpy, "*", 32767, &font_num);
    scaleFont = 0;
    if (font_num <= 0)
    {
	fprintf(stderr, " Error: could not get fontlist from this server.\n");
	exit(0);
    }
    for (count = 0; count < MAXPARAM; count++)
	pars[count] = (char *) malloc (parLens[count]);

    for (font_id = 0; font_id < font_num; font_id++)
    {
	field = set_font_args(fontList[font_id], 1);
	if (field == MAXPARAM)
	{
	     add_new_font();
	}
    }
    if (main_alias_list != NULL)
	add_alias_fonts();
    if (scaleFont)
    {
	count = 6;
	while (count < 40)
	{
	     sprintf(exname, "%d", count);
	     add_sort_attr (exname, WIDTH);
	     add_sort_attr (exname, HEIGHT);
	     count += 2;
	}
    }

    cal_list_length();
    s1 = slant_list;
    while (s1 != NULL)
    {
	s2 = (SUB_REC *) calloc(1, sizeof(SUB_REC));
	if (slant2_list == NULL)
	    slant2_list = s2;
	else
	    s3->next = s2;
	s3 = s2;
	if ((strcmp(s1->val, "o") == 0) || (strcmp(s1->val, "O") == 0))
	{
	    s2->val = (char *) malloc(8);
	    strcpy(s2->val, "Oblique");
	}
	else if (!strcmp(s1->val, "r") || !strcmp(s1->val, "R"))
	{
	    s2->val = (char *) malloc(6);
	    strcpy(s2->val, "Roman");
	}
	else if (!strcmp(s1->val, "i") || !strcmp(s1->val, "I"))
	{
	    s2->val = (char *) malloc(8);
	    strcpy(s2->val, "Italic");
	}
	else if (!strcmp(s1->val, "ri") || !strcmp(s1->val, "RI"))
	{
	    s2->val = (char *) malloc(18);
	    strcpy(s2->val, "Reverse Italic");
	}
	else if (!strcmp(s1->val, "ro") || !strcmp(s1->val, "RO"))
	{
	    s2->val = (char *) malloc(18);
	    strcpy(s2->val, "Reverse Oblique");
	}
	else if (!strcmp(s1->val, "ot") || !strcmp(s1->val, "OT"))
	{
	    s2->val = (char *) malloc(8);
	    strcpy(s2->val, "Other");
	}
	else
	{
	    s2->val = (char *) malloc(strlen(s1->val)+1);
	    strcpy(s2->val, s1->val);
	}
	s1 = s1->next;
    }
}

add_sort_attr(name, which)
char  *name;
int   which;
{
	SUB_REC   *snode, *pnode, *cnode, *nnode;
	int	  v1, v2;

	v1 = atoi(name);
	if (v1 > 0 && v1 < 5)
	    return;

	switch (which) {
	 case  HEIGHT:
		 snode = height_list;
		 break;
	 case  WIDTH:
		 snode = width_list;
		 break;
	}
	pnode = snode;
	while (pnode != NULL)
	{
	    if (strcmp(pnode->val, name) == 0)
		return;
	    pnode = pnode->next;
	}
	nnode = (SUB_REC *) calloc(1, sizeof(SUB_REC));
	nnode->val = (char *) malloc(strlen(name) + 2);
	strcpy(nnode->val, name);
	if (snode == NULL)
	{
	    switch (which) {
		case HEIGHT:
			height_list = nnode;
			break;
		case WIDTH:
			width_list = nnode;
			break;
	    }
	    return;
	}
	pnode = snode;
	v2 = atoi(pnode->val);
	if (v2 > v1)
	{
	    nnode->next = pnode;
	    if (which == 6)
		height_list = nnode;
	    else
		width_list = nnode;
	    return;
	}
	cnode = snode->next;
	while (cnode != NULL)
	{
	    v2 = atoi(cnode->val);
	    if (v2 > v1)
	    {
		pnode->next = nnode;
		nnode->next = cnode;
		return;
	    }
	    pnode = cnode;
	    cnode = pnode->next;
	}
	pnode->next = nnode;
}


add_to_attr(name, which)
char  *name;
int   which;
{
	SUB_REC   *snode, *pnode, *nnode;
	MENU_REC  *alist, *anode;
	int	  m;

	switch (which) {
	 case  FONTNAME:
		 snode = name_list;
		 break;
	 case  FAMILY:
		 snode = family_list;
		 break;
	 case  WEIGHT:
		 snode = weight_list;
		 break;
	 case  SLANT:
		 snode = slant_list;
		 break;
	 case  SETWIDTH:
		 snode = swidth_list;
		 break;
	 case  ALIAS:
		 snode = alias_list;
		 break;
	 default:
		 return;
		 break;
	}
	pnode = snode;
	while (pnode != NULL)
	{
	    if (strcmp(pnode->val, name) == 0)
		return;
	    pnode = pnode->next;
	}
	nnode = (SUB_REC *) calloc(1, sizeof(SUB_REC));
	nnode->val = (char *) malloc(strlen(name) + 2);
	strcpy(nnode->val, name);
	if (which == ALIAS)
	{
	    anode = (MENU_REC *) calloc(1, sizeof(MENU_REC));
            anode->val = (char *) malloc(strlen(name) + 1);
            strcpy(anode->val, name);
            anode->id = which;
            anode->fid = font_id;
            anode->mid = 0;
	    if (main_alias_list == NULL)
		main_alias_list = anode;
	    else
	    {
	    	alist = main_alias_list;
	    	while (alist->next != NULL)
		    alist = alist->next;
            	anode->mid = alist->mid + 1;
	    	alist->next = anode;
	    }
	}

	if (snode == NULL)
	{
	    switch (which) {
		case FONTNAME:
			name_list = nnode;
			break;
		case FAMILY:
			family_list = nnode;
			break;
		case WEIGHT:
			weight_list = nnode;
			break;
		case SLANT:
			slant_list = nnode;
			break;
		case SETWIDTH:
			swidth_list = nnode;
			break;
		case ALIAS:
			alias_list = nnode;
			break;
	    }
	    return;
	}
	pnode = snode;
	while (pnode->next != NULL)
		pnode = pnode->next;
	pnode->next = nnode;
}


int
count_list (list)
SUB_REC  *list;
{
	int	k;

	k = 0;
	while (list != NULL)
	{
	    list->id = k;
	    k++;
	    list = list->next;
	}
	return(k);
}


cal_list_length()
{
	SUB_REC   *snode;
	int	  k;

	name_list_len = count_list(name_list);
	family_list_len = count_list(family_list);
	weight_list_len = count_list(weight_list);
	slant_list_len = count_list(slant_list);
	swidth_list_len = count_list(swidth_list);
	height_list_len = count_list(height_list);
	width_list_len = count_list(width_list);
	alias_list_len = count_list(alias_list);
/*
	for (k = 0; k < FONTNUM; k++)
*/
	for (k = 0; k < 1; k++)
	{
	    font_rec[k].name_wg_list = 
			(Widget *) malloc(sizeof(Widget) * name_list_len);
	    if (family_list_len > 0)
	        font_rec[k].family_wg_list = 
			(Widget *) malloc(sizeof(Widget) * family_list_len);
	    else
	        font_rec[k].family_wg_list = NULL;
	    if (weight_list_len > 0)
	        font_rec[k].weight_wg_list = 
			(Widget *) malloc(sizeof(Widget) * weight_list_len);
	    else
	        font_rec[k].weight_wg_list = NULL;
	    if (slant_list_len > 0)
	        font_rec[k].slant_wg_list = 
			(Widget *) malloc(sizeof(Widget) * slant_list_len);
	    else
	        font_rec[k].slant_wg_list = NULL;
	    if (height_list_len > 0)
	        font_rec[k].height_wg_list = 
			(Widget *) malloc(sizeof(Widget) * height_list_len);
	    else
	        font_rec[k].height_wg_list = NULL;
	    if (width_list_len > 0)
	        font_rec[k].width_wg_list = 
			(Widget *) malloc(sizeof(Widget) * width_list_len);
	    else
	        font_rec[k].width_wg_list = NULL;
	    if (alias_list_len > 0)
	        font_rec[k].alias_wg_list = 
			(Widget *) malloc(sizeof(Widget) * alias_list_len);
	    else
	        font_rec[k].alias_wg_list = NULL;
	}
}


save_proc()
{
	char  spath[512];
	char  dpath[512];
	char  data[512];
	char  data2[512];
	char  *pdata;
	char  *dir;
	struct stat     f_stat;
	FILE  *fin, *fout;
	int    k, do_copy;

	if (cols <= 0 || rows <= 0)
	    return;
	if ((def_fonts[0] == NULL) || ((int) strlen(def_fonts[0]) < 2))
	    return;

	dir = (char *)getenv("HOME");
	if (dir == NULL)
	    return;
	sprintf(spath, "%s/app-defaults", dir);
	{
	    if (access (spath, R_OK) != 0)
            {
		sprintf(dpath, "mkdir -p %s", spath);
		system(dpath);
            }
	}
	sprintf(spath, "%s/app-defaults/Vnmr", dir);
	sprintf(dpath, "%s", tempnam("/tmp", "Vnmr"));
	fin = NULL;
	if (stat(spath, &f_stat) == 0)
        {
	    if (access (spath, W_OK) != 0)
	    {
		sprintf(data2, "chmod +rw %s", spath);
		system(data2);
	    }
	    if ((fin = fopen(spath, "r")) == NULL)
		return;
	}
	else
	{
	    dir = (char *)getenv("vnmrsystem");
	    if (dir != NULL)
	    {
		sprintf(data, "%s/app-defaults/Vnmr", dir);
	        fin = fopen(data, "r");
	    }
	}
	if ((fout = fopen(dpath, "w")) == NULL)
	{
	    if (fin)
		fclose(fin);
	    return;
	}
	if (smallWindow)
	    sprintf(data2, "*Vnmr*graphics*small%dx%d*font", rows, cols);
	else
	    sprintf(data2, "*Vnmr*graphics*large%dx%d*font", rows, cols);
	k = strlen(data2);
	do_copy = 1;
	if (fin != NULL)
	{
	    while (fgets(data, 500, fin) != NULL)
	    {
		if (strncmp(data, data2, k) == 0)
		{
		    fprintf(fout, "%s:  %s\n", data2, def_fonts[0]);
		    do_copy = 0;
		    break;
		}
		else
		    fprintf(fout, "%s", data);
	    }
	    while (fgets(data, 500, fin) != NULL)
	    {
		fprintf(fout, "%s", data);
	    }
	    fclose(fin);
	}
	if (do_copy)
	    fprintf(fout, "%s:  %s\n", data2, def_fonts[0]);
	fclose(fout);
	fsync(fileno(fout));
	sprintf (data2, "mv -f %s %s", dpath, spath);
	system (data2);
}


void
close_proc()
{
	save_proc();
	exit(0);
}

void win_repaint()
{
	draw_font_info();
}


void win_resize(w,data,event)
Widget    w;
caddr_t   data;
XEvent    *event;
{
   int         n, x, y, pw;
   Dimension	bw, bw2;
   XWindowAttributes attr, attr2;
   XWindowChanges    newdim;

   if(event->type != ConfigureNotify)
	return;
   if (!XGetWindowAttributes(dpy,XtWindow(topShell), &attr))
   	return;
   if (butWinH <= 0)
   {
	set_label_position();
        if (XGetWindowAttributes(dpy,XtWindow(butWindow), &attr2))
	{
	    butWinH = attr2.height;
	    pw = attr2.width;
	    XtSetArg(args[0], XmNwidth, &bw);
	    XtGetValues(closeButton, args, 1);
	    x = (pw - (int)bw ) / 2;
	    if (x > 0)
	    {
            	XtSetArg(args[0], XmNmarginWidth, x);
		XtSetValues(butWindow, args, 1);
	    }
	}
   }

   if (diffy <= 0)
   {
        if (!XGetWindowAttributes(dpy,XtWindow(dispWindow), &attr2))
   	    return;
	diffy = attr.height - attr2.height;
	diffx = attr.width - attr2.width;
	dispWinY = attr2.y;
	return;
   }
   newdim.height = attr.height - diffy;
   XConfigureWindow (dpy, XtWindow(dispWindow), CWHeight, &newdim);
   if (butWinH > 0)
   {
       newdim.height = butWinH;
       newdim.y = attr.height - butWinH;
       XtConfigureWidget (butWindow, 0, newdim.y, attr.width - diffx, butWinH, 0);
   }
   
}


Widget
create_rc_widget(parent, topwidget, vertical, pad, space)
int     vertical;
int     pad, space;
{
        Widget   rcwidget;
	int	 n;

        n =0;
        XtSetArg(args[n], XmNborderWidth, 0);  n++;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNresizable, TRUE);  n++;

        if (topwidget != NULL)
        {
            XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
            XtSetArg (args[n], XmNtopWidget, topwidget);  n++;
        }
        else
        {
            XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
        }
        if (vertical)
        {
            XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
            XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);  n++;
            XtSetArg (args[n], XmNmarginHeight, 1);  n++;
        }
        else
        {
            XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	    if (has_alias)
               XtSetArg(args[n], XmNpacking, XmPACK_NONE);
	    else
               XtSetArg(args[n], XmNpacking, XmPACK_TIGHT);
	    n++;
            XtSetArg(args[n], XmNspacing, space);  n++;
            XtSetArg(args[n], XmNmarginWidth, pad);  n++;
        }
        rcwidget = (Widget)XmCreateRowColumn(parent, "", args, n);
        XtManageChild (rcwidget);
        return(rcwidget);
}

assign_menu_id (list)
MENU_REC  *list;
{
	SUB_REC   *slist, *splist;
	MENU_REC  *chlist;
	int	  fid;

	switch (list->id) {
	  case  FONTNAME:
		   splist = name_list;
		   break;
	  case  FAMILY:
		   splist = family_list;
		   break;
	  case  WEIGHT:
		   splist = weight_list;
		   break;
	  case  SLANT:
		   splist = slant_list;
		   break;
	  case SETWIDTH:
		   splist = swidth_list;
		   break;
	  case  HEIGHT:
		   splist = height_list;
		   break;
	  case  WIDTH:
		   splist = width_list;
		   break;
	  case  ALIAS:
		   splist = alias_list;
		   break;
	  default:
		   return;
	}
	while (list != NULL)
	{
	   slist = splist;
	   while (slist != NULL)
	   {
	      if (list->val[0] == slist->val[0])
	      {
		  if (strcmp(list->val, slist->val) == 0)
		  {
		      list->mid = slist->id;
		      break;
		  }
	      }
	      slist = slist->next;
	   }
	   if (list->id == SETWIDTH || list->id == HEIGHT)
	   {
	      if (list->scalable)
	      {
		 chlist = list->child;
		 while (chlist != NULL)
                 {
		    if (chlist->scalable <= 0)
		    {
		        fid = chlist->fid;
			break;
		    }
		    chlist = chlist->next;
		 }
		 chlist = list->child;
		 while (chlist != NULL)
		 {
		    if (chlist->scalable && chlist->mid < 0)
		    {
			chlist->fid = fid;
			chlist->mid = 0;
		        chlist->hratio = list->hratio;
		    }
		    chlist = chlist->next;
		 }
	      }
	   }
	   list = list->next;
	}
}


int
set_menu_get_fontid (mlist, frec)
MENU_REC  *mlist;
FONT_REC  *frec;
{
	MENU_REC    *tlist;
	Widget      *wlist, *w2list, menu_wg;
	int	    w_len, m, item, change_item;
	int	    ret, scalable;

	if (mlist == NULL)
	    return(-1);
	if (mlist->mid < 0)
	    assign_menu_id(mlist);
	change_item = 1;
	switch (mlist->id) {
	  case  FONTNAME:
		   wlist = frec->name_wg_list;
		   w_len = name_list_len;
		   menu_wg = frec->name_wg;
		   break;
	  case  FAMILY:
		   wlist = frec->family_wg_list;
		   w_len = family_list_len;
		   menu_wg = frec->family_wg;
		   break;
	  case  WEIGHT:
		   wlist = frec->weight_wg_list;
		   w_len = weight_list_len;
		   menu_wg = frec->weight_wg;
		   break;
	  case  SLANT:
		   wlist = frec->slant_wg_list;
		   menu_wg = frec->slant_wg;
		   w_len = slant_list_len;
		   break;
	  case SETWIDTH:
		   wlist = NULL;
		   w_len = 0;
	     	   change_item = 0;
		   ret = set_menu_get_fontid (mlist->child, frec);
		   return(ret);
		   break;
	  case  HEIGHT:
		   wlist = frec->height_wg_list;
		   menu_wg = frec->height_wg;
		   w_len = height_list_len;
		   break;
	  case  WIDTH:
		   wlist = frec->width_wg_list;
		   menu_wg = frec->width_wg;
		   w_len = width_list_len;
		   break;
	  case  ALIAS:
		   wlist = frec->alias_wg_list;
		   menu_wg = frec->alias_wg;
		   w_len = alias_list_len;
	     	   change_item = 0;
		   break;
	  default:
		   return (-1);
	}
	item = frec->item_select[mlist->id];
	scalable = 0;
	if (wlist == NULL || menu_wg == NULL)
	     return (-1);
	if (mlist->id == HEIGHT || mlist->id == WIDTH)
	{
	    tlist = mlist->parent;
	    if (mlist->id == WIDTH)
		tlist = tlist->parent;
	    scalable = tlist->scalable;
	}
	
	if (scalable)
	{
	     XtSetArg( args[ 0 ], XtNsensitive, True );
	     for (m = 0; m < w_len; m++)
		XtSetValues (wlist[m], args, 1);
	     change_item = 0;
	}
	else if (wlist != NULL && mlist->id != ALIAS)
	{
	     XtSetArg( args[ 0 ], XtNsensitive, False );
	     for (m = 0; m < w_len; m++)
		XtSetValues (wlist[m], args, 1);
	     XtSetArg( args[ 0 ], XtNsensitive, True );
	     tlist = mlist;
	     while (tlist != NULL)
	     {
	        if (tlist->mid >= 0 && tlist->mid < w_len)
		    XtSetValues (wlist[tlist->mid], args, 1);
		if (tlist->mid == item)
		    change_item = 0;
		tlist = tlist->next;
	     }
	}
	if (change_item)
	{
	     tlist = mlist;
	     while (tlist != NULL)
	     {
		if (tlist->mid >= 0 && tlist->mid < w_len)
		{
		    frec->item_select[mlist->id] = tlist->mid;
		    XtSetArg(args[0], XmNmenuHistory, wlist[tlist->mid]);
		    XtSetValues(menu_wg, args, 1);
		    break;
		}
		tlist = tlist->next;
	     }
	     if (tlist != NULL)
		mlist = tlist;
	}
	if (mlist->id != WIDTH)
	{
	    tlist = mlist;
	    while (tlist != NULL)
	    {
	        if (tlist->mid == frec->item_select[mlist->id])
		    break;
	        tlist = tlist->next;
	    }
	    if (tlist != NULL)
		mlist = tlist;
	}
	if (mlist->id >= WIDTH)
	{
	    frec->fid = mlist->fid;
	    frec->scaled = scalable;
	    frec->hratio = mlist->hratio;
	    return(mlist->fid);
	}
	if (mlist == NULL)
	    return(-1);
	ret = set_menu_get_fontid (mlist->child, frec);
	return(ret);
}


int
select_font(uid, frec)
int	uid;
FONT_REC  *frec;
{
	int	  fid, scalable;
	MENU_REC  *mlist, *plist;


	mlist = main_list;
	while (mlist != NULL && mlist->id < uid)
	{
	    if (mlist->mid < 0)
	        assign_menu_id(mlist);
	    scalable = 0;
	    if (mlist->id == HEIGHT || mlist->id == WIDTH)
	    {
		plist = mlist->parent;
		if (mlist->id == WIDTH)
		   plist = plist->parent;
		if (plist->scalable)
			scalable = 1;
	    }
	    plist = mlist;
	    while (mlist != NULL)
	    {
		if (mlist->mid == frec->item_select[mlist->id])
			break;
		mlist = mlist->next;
	    }
	    if (mlist == NULL)
	    {
		if (scalable)
		    mlist = plist;
		else
		    return;
	    }
	    mlist = mlist->child;
	}
	if (mlist == NULL)
	{
	    return(-1);
	}
	if (mlist->mid < 0)
	     assign_menu_id(mlist);
	scalable = 0;
	if (mlist->id >= HEIGHT)
	{
		plist = mlist->parent;
		if (mlist->id == WIDTH)
		   plist = plist->parent;
		scalable = plist->scalable;
	}
	plist = mlist;
	while (mlist != NULL)
	{
		if (mlist->mid == frec->item_select[mlist->id])
			break;
		mlist = mlist->next;
	}
	if (mlist == NULL)
	{
		if (scalable)
		    mlist = plist;
		else
		    return(-1);
	}
	if (mlist->id == FONTNAME)
	{
		if (strcmp(mlist->val, alias_name) == 0)
		{
		    XtUnmapWidget(frec->width_wg);
		    XtUnmapWidget(frec->height_wg);
		    XtUnmapWidget(frec->slant_wg);
		    XtUnmapWidget(frec->weight_wg);
		    XtUnmapWidget(frec->family_wg);
		    XtMapWidget(frec->alias_wg);
		}
		else
		{
		    XtUnmapWidget(frec->alias_wg);
		    XtMapWidget(frec->family_wg);
		    XtMapWidget(frec->weight_wg);
		    XtMapWidget(frec->slant_wg);
		    XtMapWidget(frec->height_wg);
		    XtMapWidget(frec->width_wg);
		}
	}
	
	if (uid >= WIDTH)
	{
	    fid = mlist->fid;
	    frec->scaled = scalable;
	    frec->hratio = mlist->hratio;
	    if (fid >= 0 && fid < font_num)
	    {
	        frec->fid = fid;
	        return(fid);
	    }
	    else
	        return(-1);
	}
	else
	    fid = set_menu_get_fontid (mlist->child, frec);
	return(fid);
}


void
change_font(wg, c_data, b_data)
Widget   wg;
caddr_t  c_data, b_data;
{
	int	  id, uid, k, fid;
	FONT_REC  *frec;
	SUB_REC   *slist;
	int	  list_len;
	Widget    *wlist;
	MENU_REC  *mlist;


	id = (int) c_data;
	frec = (FONT_REC *) &font_rec[id];
        XtSetArg(args[0], XmNuserData, &uid);
	XtGetValues(wg, args, 1);
	list_len = 0;
	switch (uid) {
	  case  FONTNAME:
		   wlist = frec->name_wg_list;
		   slist = name_list;
		   list_len = name_list_len;
		   break;
	  case  FAMILY:
		   wlist = frec->family_wg_list;
		   slist = family_list;
		   list_len = family_list_len;
		   break;
	  case  WEIGHT:
		   wlist = frec->weight_wg_list;
		   slist = weight_list;
		   list_len = weight_list_len;
		   break;
	  case  SLANT:
		   wlist = frec->slant_wg_list;
		   slist = slant_list;
		   list_len = slant_list_len;
		   break;
	  case  HEIGHT:
		   wlist = frec->height_wg_list;
		   slist = height_list;
		   list_len = height_list_len;
		   break;
	  case  WIDTH:
		   wlist = frec->width_wg_list;
		   slist = width_list;
		   list_len = width_list_len;
		   break;
	  case  ALIAS:
		   wlist = frec->alias_wg_list;
		   slist = alias_list;
		   list_len = alias_list_len;
		   break;
	}
	for (k = 0; k < list_len; k++)
	{
	    if (wg == wlist[k])
		break;
	}
	if (k >= list_len)
	    return;
	if (uid == HEIGHT)
	    frec->height_id = k;
	else if (uid == WIDTH)
	    frec->width_id = k;

	frec->item_select[uid] = k;
	select_font(uid, frec);
	disp_new_font_info(id);
}


Widget
create_menu_item (list, row, id, uid)
SUB_REC   *list;
Widget    row;
int	  id, uid;
{
        Widget   pulldown;
        Widget   retwidget;
        Widget   *wlist;
        Widget   pwg;
        XmString xmstr;
	int	 n, inum, len;

	switch (uid) {
	  case  FONTNAME:
		   wlist = font_rec[id].name_wg_list;
		   len = name_list_len;
		   pwg = font_rec[id].label_wg;
		   break;
	  case  FAMILY:
		   wlist = font_rec[id].family_wg_list;
		   len = family_list_len;
		   pwg = font_rec[id].name_wg;
		   break;
	  case  WEIGHT:
		   wlist = font_rec[id].weight_wg_list;
		   len = weight_list_len;
		   pwg = font_rec[id].family_wg;
		   break;
	  case  SLANT:
		   wlist = font_rec[id].slant_wg_list;
		   len = slant_list_len;
		   pwg = font_rec[id].weight_wg;
		   break;
	  case  HEIGHT:
		   wlist = font_rec[id].height_wg_list;
		   len = height_list_len;
		   pwg = font_rec[id].slant_wg;
		   break;
	  case  WIDTH:
		   wlist = font_rec[id].width_wg_list;
		   len = width_list_len;
		   pwg = font_rec[id].height_wg;
		   break;
	  case  ALIAS:
		   wlist = font_rec[id].alias_wg_list;
		   len = alias_list_len;
		   pwg = font_rec[id].label_wg;
		   break;
	}
	if (wlist == NULL)
	    return(NULL);
	n = 0;
	if (len > 30)
	{
	  inum = len / 30 + 1;
          XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
          XtSetArg(args[n], XmNnumColumns, inum); n++;
	}
        pulldown = (Widget)XmCreatePulldownMenu(row, "pulldown", args, n);
	inum = 0;
	while (list != NULL)
        {
	   xmstr = XmStringCreateLocalized(list->val);
	   n = 0;
           XtSetArg(args[n], XmNlabelString, xmstr); n++;
           XtSetArg(args[n], XmNmarginBottom, 1); n++;
           XtSetArg(args[n], XmNmarginTop, 1); n++;
           XtSetArg(args[n], XmNuserData, uid); n++;
           wlist[inum] = (Widget)XmCreatePushButtonGadget(pulldown, " ", args, n);
           XtAddCallback(wlist[inum], XmNactivateCallback, change_font, 
			(XtPointer) id);
           XmStringFree(xmstr);
           XtManageChild(wlist[inum]);
	   inum++;
	   list = list->next;
        }
	n = 0;
	if (uid == ALIAS)
	{
            XtSetArg(args[n], XmNmappedWhenManaged, False); n++;
	}
        XtSetArg(args[n], XmNsubMenuId, pulldown); n++;
        XtSetArg(args[n], XmNmenuHistory, wlist[0]); n++;
        retwidget = (Widget)XmCreateOptionMenu(row, "", args, n);
        XtManageChild(retwidget);
	return (retwidget);
}



Widget
create_font_menu (id, row_widget, label)
int	id;
Widget  row_widget;
char	*label;
{
	FONT_REC   *cur_rec;
	XmString   xmstr;
	int	   n;

	cur_rec = (FONT_REC *) &font_rec[id];
	xmstr = XmStringCreateLocalized(label);
	n = 0;
        XtSetArg(args[n], XmNlabelString, xmstr); n++;
        cur_rec->label_wg = XmCreateLabel(row_widget, "label", args, n);
	XtManageChild (cur_rec->label_wg);
        XmStringFree(xmstr);
	cur_rec->name_wg = create_menu_item (name_list,row_widget,id,FONTNAME);
	cur_rec->family_wg = create_menu_item (family_list,row_widget,id,FAMILY);
	cur_rec->weight_wg = create_menu_item (weight_list,row_widget,id,WEIGHT);
	cur_rec->slant_wg = create_menu_item (slant2_list,row_widget, id, SLANT);
	cur_rec->height_wg = create_menu_item (height_list,row_widget, id,HEIGHT);
	cur_rec->width_wg = create_menu_item (width_list,row_widget, id,WIDTH);
	if (alias_list != NULL)
	   cur_rec->alias_wg = create_menu_item (alias_list,row_widget, id,ALIAS);
	for (n = 0; n <= ALIAS; n++)
	    cur_rec->item_select[n] = 0;
}

create_font_window()
{
	int	n;
	Widget  row1, row2;
	XmString   xmstr;
	char    tmp[32];

	n = 0;
	formShell = (Widget) XmCreateForm(topShell, "", args, n);
        XtManageChild (formShell);
	row2 = create_rc_widget(formShell, NULL, 0, 0, 0);
	create_label_widgets(row2);
	row1 = create_rc_widget(formShell, row2, 0, 0, 0);
	sprintf(tmp, " Font for %dx%d windows: ", rows, cols);
	create_font_menu(0, row1, " ");
	n =0;
        XtSetArg (args[n], XmNborderWidth, 1);  n++;
        XtSetArg (args[n], XmNmarginHeight, 0);  n++;
        XtSetArg (args[n], XmNmarginWidth, 0);  n++;
        XtSetArg (args[n], XmNwidth, 300);  n++;
        XtSetArg (args[n], XmNheight, 100);  n++;
        XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg (args[n], XmNtopWidget, row1);  n++;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNresizable, TRUE);  n++;
        dispWindow = XtCreateManagedWidget("text", xmDrawingAreaWidgetClass,
                         formShell, args, n);
	n =0;
        XtSetArg (args[n], XmNforeground, &winFg);  n++;
        XtSetArg (args[n], XmNbackground, &winBg);  n++;
        XtSetArg (args[n], XmNhighlightColor, &hiPix);  n++;
        XtGetValues(dispWindow, args, n);

        n =0;
        XtSetArg(args[n], XmNborderWidth, 0);  n++;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg (args[n], XmNtopWidget, dispWindow);  n++;
        XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
/*
        XtSetArg (args[n], XmNpacking, XmPACK_COLUMN);  n++;
        XtSetArg (args[n], XmNspacing, 60);  n++;
        XtSetArg (args[n], XmNresizable, FALSE);  n++;
*/
        XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER);  n++;
/*
        XtSetArg(args[n], XmNmarginWidth, 180);  n++;
*/
        butWindow = (Widget)XmCreateRowColumn(formShell, "", args, n);
        XtManageChild (butWindow);
	xmstr = XmStringCreateLocalized("Close");
	n = 0;
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
        XtSetArg (args[n], XmNtraversalOn, FALSE);  n++;
        closeButton = (Widget)XmCreatePushButton(butWindow, "button", args, n);
        XtManageChild (closeButton);
        XmStringFree(xmstr);
	XtAddCallback(closeButton, XmNactivateCallback, close_proc, NULL);

	XtAddEventHandler(dispWindow,ExposureMask, False, win_repaint, NULL);
	XtAddEventHandler(topShell,StructureNotifyMask,False,win_resize,NULL);
	n = 0;
        XtSetArg (args[n], XmNx, 100);  n++;
        XtSetArg (args[n], XmNy, 60);  n++;
	XtSetValues(topShell, args, n);
}


set_def_select(slist, frec, item)
SUB_REC   *slist;
FONT_REC  *frec;
int	  item;
{
	char   *name;
	Widget pane, *w_list;
	int	w_len;

	name = pars[item];
	switch (item) {
	  case  FONTNAME:
			  pane = frec->name_wg;
			  w_list = frec->name_wg_list;
			  w_len = name_list_len;
			  break;
	  case  FAMILY:
			  pane = frec->family_wg;
			  w_list = frec->family_wg_list;
			  w_len = family_list_len;
			  break;
	  case  WEIGHT:
			  pane = frec->weight_wg;
			  w_list = frec->weight_wg_list;
			  w_len = weight_list_len;
			  break;
	  case  SLANT:
			  pane = frec->slant_wg;
			  w_list = frec->slant_wg_list;
			  w_len = slant_list_len;
			  break;
	  case  HEIGHT:
			  pane = frec->height_wg;
			  w_list = frec->height_wg_list;
			  w_len = height_list_len;
			  break;
	  case  WIDTH:
			  pane = frec->width_wg;
			  w_list = frec->width_wg_list;
			  w_len = width_list_len;
			  break;
	  case  ALIAS:
			  pane = frec->alias_wg;
			  w_list = frec->alias_wg_list;
			  w_len = alias_list_len;
			  name = pars[0];
			  break;
	}
	if (strlen(name) <= 0)
	{
	    if (item == FONTNAME)
		name = empty;
	    else
		name = zero;
	}
	while (slist != NULL)
	{
	   if (name[0] == slist->val[0])
	   {
		if (strcmp(name, slist->val) == 0)
		{
		      frec->item_select[item] = slist->id;
		      break;
	        }
	   }
	   slist = slist->next;
	}
	if (frec->item_select[item] >= 0 && frec->item_select[item] < w_len)
	{
	    XtSetArg(args[0], XmNmenuHistory, w_list[frec->item_select[item]]);
	    XtSetValues(pane, args, 1);
	    if (item == HEIGHT)
		frec->height_id = frec->item_select[item];
	    else if (item == WIDTH)
		frec->width_id = frec->item_select[item];
	}
}

set_def_values()
{
	char    *label, *font, *res, *res2;
        char    attrs[32];
	int	num, argnum;
	SUB_REC *slist;

	for (num = 0; num < 1; num++)
	{
	     xfont_info[num] = NULL;
	     font_rec[num].fid = 0;
	     font_rec[num].id = num;
	     font_rec[num].scaled = 0;
	     font_rec[num].height_id = 0;
	     font_rec[num].width_id = 0;
	     def_fonts[num] = (char *) malloc(256);
	     if (smallWindow)
	       sprintf(attrs, "graphics*small%dx%d*font", rows, cols);
	     else
	       sprintf(attrs, "graphics*large%dx%d*font", rows, cols);
	     if ((res = get_x_resource("Vnmr", attrs)) != NULL)
		strcpy (def_fonts[num], res);
	     else
		strcpy (def_fonts[num], "9x15");
	    argnum = set_font_args(def_fonts[num], 0);
	    if (argnum == MAXPARAM)
	    {
	        set_def_select(name_list, &font_rec[num], FONTNAME);
	        set_def_select(family_list, &font_rec[num], FAMILY);
	        set_def_select(weight_list, &font_rec[num], WEIGHT);
	        set_def_select(slant_list, &font_rec[num], SLANT);
	        set_def_select(height_list, &font_rec[num], HEIGHT);
	        set_def_select(width_list, &font_rec[num], WIDTH);
	    }
	    else if (argnum == 1)
	    {
	        set_def_select(alias_list, &font_rec[num], ALIAS);
	    }
	    select_font(FONTNAME, &font_rec[num]);
	    if (cur_id < 0)
	    {
		if (win_width < font_sizes[num])
		    cur_id = num;
	    }
	}
	disp_default_font_info();
}

disp_vnmr_font()
{
	char    *label, *font, *res, *res2;
        char    attrs[32];
	int	num, argnum;
	SUB_REC *slist;

	num = 0;
	xfont_info[num] = NULL;
	font_rec[num].fid = 0;
	argnum = set_font_args(def_fonts[num], 0);
	if (argnum == MAXPARAM)
	{
	        set_def_select(name_list, &font_rec[num], FONTNAME);
	        set_def_select(family_list, &font_rec[num], FAMILY);
	        set_def_select(weight_list, &font_rec[num], WEIGHT);
	        set_def_select(slant_list, &font_rec[num], SLANT);
	        set_def_select(height_list, &font_rec[num], HEIGHT);
	        set_def_select(width_list, &font_rec[num], WIDTH);
	}
	else if (argnum == 1)
	{
	       set_def_select(alias_list, &font_rec[num], ALIAS);
	}
	select_font(FONTNAME, &font_rec[num]);
	if (cur_id < 0)
	{
		if (win_width < font_sizes[num])
		    cur_id = num;
	}
	disp_default_font_info();
}

create_label_widgets(parent)
Widget   parent;
{
	int	n, k;
	XmString   xmstr;

	n = 0;
        XtSetArg(args[n], XmNpacking, XmPACK_NONE);  n++;
	XtSetValues(parent, args, n);
	for (k = 0; k < 6; k++)
	{
	    xmstr = XmStringCreateLocalized(label_vals[k]);
	    n = 0;
            XtSetArg(args[n], XmNlabelString, xmstr);  n++;
	    label_wg[k] = XtCreateManagedWidget ("", xmLabelWidgetClass, 
			parent, args, n);
            XmStringFree(xmstr);
	}
}

set_label_position()
{
	Position   x, y, lx;
	int	   n, m, gap;
	Dimension  w;
	FONT_REC   *cur_rec;
	Arg        xargs[2];      

/*
	cur_rec = (FONT_REC *) &font_rec[FONTNUM - 1];
*/
	cur_rec = (FONT_REC *) &font_rec[0];
	if (has_alias)
	{
	    n = 0;
            XtSetArg(args[n], XmNx, &x);  n++;
            XtSetArg(args[n], XmNwidth, &w);  n++;
	    XtGetValues(cur_rec->label_wg, args, n);
            XtSetArg(args[0], XmNy, &y);
	    XtGetValues(cur_rec->name_wg, args, 1);
	    lx = x + w + 8;
	    x = lx;
	    XtSetArg(xargs[0], XmNwidth, &w);
	    XtGetValues(cur_rec->name_wg, xargs, 1);
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
		XtMoveWidget(font_rec[m].name_wg, x, y);
	    x += (Position) w;
	    XtGetValues(cur_rec->family_wg, xargs, 1);
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
	    {
		XtMoveWidget(font_rec[m].family_wg, x, y);
		XtMoveWidget(font_rec[m].alias_wg, x, y);
	    }
	    x += (Position) w;
	    XtGetValues(cur_rec->weight_wg, xargs, 1);
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
		XtMoveWidget(font_rec[m].weight_wg, x, y);
	    x += (Position) w;
	    XtGetValues(cur_rec->slant_wg, xargs, 1);
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
		XtMoveWidget(font_rec[m].slant_wg, x, y);
	    x += (Position) w;
	    XtGetValues(cur_rec->height_wg, xargs, 1);
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
		XtMoveWidget(font_rec[m].height_wg, x, y);
	    x += (Position) w;
	    XtGetValues(cur_rec->width_wg, xargs, 1);
	    w = w + (Dimension) x + 12;
	    XtSetArg(args[0], XmNx, x);
	    for (m = 0; m < 1; m++)
		XtMoveWidget(font_rec[m].width_wg, x, y);
	    XtSetArg(args[0], XmNwidth, w);
	    XtSetValues(topShell, args, 1);
	}
	cur_rec = (FONT_REC *) &font_rec[0];
	gap = 8;
	n = 0;
        XtSetArg(args[n], XmNx, &x);  n++;
        XtSetArg(args[n], XmNy, &y);  n++;
	XtGetValues(cur_rec->width_wg, args, n);
	XtMoveWidget(label_wg[5], x+gap, y);
	XtGetValues(cur_rec->height_wg, args, n);
	XtMoveWidget(label_wg[4], x+gap, y);
	XtGetValues(cur_rec->slant_wg, args, n);
	XtMoveWidget(label_wg[3], x+gap, y);
	XtGetValues(cur_rec->weight_wg, args, n);
	XtMoveWidget(label_wg[2], x+gap, y);
	XtGetValues(cur_rec->family_wg, args, n);
	XtMoveWidget(label_wg[1], x+gap, y);
	XtGetValues(cur_rec->name_wg, args, n);
	XtMoveWidget(label_wg[0], x+gap, y);
}

set_def_font()
{
	XFontStruct *fInfo;

	if ((fInfo = XLoadQueryFont(dpy, "10x20")) == NULL)
        {
	   fInfo = XLoadQueryFont(dpy, "9x15");
	}
	if (fInfo == NULL)
	   return;
	def_ascent = fInfo->ascent;
	def_height = def_ascent + fInfo->descent + 12;
	XSetFont(dpy, gc, fInfo->fid);
}


int
open_font_info(font_name, num)
char   *font_name;
int	num;
{
	XFontStruct *newInfo;

	if ((newInfo = XLoadQueryFont(dpy, font_name)) == NULL)
	{
	    return (0);
	}
	if (xfont_info[num] != NULL)
	{
	    XUnloadFont(dpy, xfont_info[num]->fid);
            XFreeFontInfo(NULL, xfont_info[num], 1);
	}
	xfont_info[num] = newInfo;
	strcpy(def_fonts[num], font_name);
	return (1);
}


char *adjust_font_name(fname, num, mult)
char	*fname;
int	num;
int	mult;
{
	static char    font_name[256];
	SUB_REC *slist;
	double  psize;
	int	m;

	strcpy(font_name, fname);
	if (set_font_args(fname, 0) != MAXPARAM)
	    return(font_name);
	if (is_scalable || font_rec[num].scaled)
	{
	    slist = height_list;
	    m = font_rec[num].height_id;
	    while (slist != NULL)
	    {
		if (slist->id == m)
		{
	        	strcpy(pars[HEIGHT], slist->val);
			break;
		}
		slist = slist->next;
	    }
	    if (mult)
	    {
	       psize = (double)(atoi(pars[HEIGHT])) * font_rec[num].hratio;
	       sprintf(pars[HEIGHT+1], "%d", (int) psize);
	    }
	    else
	       strcpy(pars[HEIGHT+1], "0");
	    slist = width_list;
	    m = font_rec[num].width_id;
	    while (slist != NULL)
	    {
		if (slist->id == m)
		{
		    if (*slist->val != '0')
	                sprintf(pars[WIDTH], "%s0",  slist->val);
		    break;
		}
		slist = slist->next;
	    }
	    font_name[0] = '\0';
	    for (m = 0; m < MAXPARAM; m++)
	    {
		strcat (font_name, "-");
		strcat (font_name, pars[m]);
	    }
	}
	return (font_name);
}

draw_font_info()
{
	int	k, px, py, h;
	Window  win;
	char    data[256];

	win = XtWindow(dispWindow);
	XClearWindow (dpy, win);
	py = 32;
	k = 0;
	if (xfont_info[k] != NULL)
	{
	       if (xfont_info[k]->max_bounds.ascent > 0)
		   py = xfont_info[k]->max_bounds.ascent + def_height;
		XSetFont(dpy, font_gc, xfont_info[k]->fid);
	} 
	if (smallWindow)
	        sprintf(data, "Font for small %dx%d windows is ", rows, cols);
	else
	        sprintf(data, "Font for large %dx%d windows is ", rows, cols);
	XSetForeground(dpy, font_gc, winFg);
	XDrawString(dpy, win, gc, 8, def_ascent + 6, data, strlen(data));
	XDrawString(dpy, win, font_gc, 12, py, def_fonts[k], strlen(def_fonts[k]));
}

disp_default_font_info()
{
	int	k;
	char    *new_name;

	for (k = 0; k < 1; k++)
	{
	    new_name = adjust_font_name(def_fonts[k], k, 1);
	    if (!open_font_info (new_name, k))
	    {
		new_name = adjust_font_name(def_fonts[k], k, 0);
		open_font_info (new_name, k);
	    }
	}
	draw_font_info();
}

disp_new_font_info(num)
int	num;
{
	char    *new_name;
	char    mess[512];

	new_name = adjust_font_name(fontList[font_rec[num].fid], num, 1);
	if (strcmp(def_fonts[num], new_name) == 0)
	    return;
	if (!open_font_info(new_name, num))
	{
	    new_name = adjust_font_name(fontList[font_rec[num].fid], num, 0);
	    open_font_info(new_name, num);
	}
	draw_font_info();
	if (out_pipe < 0)
	    return;
	if ((def_fonts[0] == NULL) || ((int) strlen(def_fonts[0]) < 2))
	    return;
	sprintf(mess, "fontselect('SYNCFUNC', 0, '%s')\n",def_fonts[0]);
	sendToVnmr(mess);
	sprintf(mess, "fontselect('SYNCFUNC', 99, 'null')\n");
	sendToVnmr(mess);
}

