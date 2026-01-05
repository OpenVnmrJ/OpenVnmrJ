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

#include <sys/param.h>

#define IMAGEMATH_MAIN
#include "imagemath.h"
#undef IMAGEMATH_MAIN

extern int mathfunc(void);

/*
 * Memory allocation management
 */
typedef struct _MemItem{
    void *item;
    struct _MemItem *next;
} MemItem;

static MemItem *memhead = NULL;

static void
addto_memlist(void *adr)
{
    MemItem *p;

    p = memhead;
    memhead = (MemItem *)malloc(sizeof(MemItem));
    memhead->item = adr;
    memhead->next = p;
}

static void
release_memlist()
{
    MemItem *p;
    MemItem *pnext;
    for (p=memhead; p; p=pnext){
	if (p->item) free(p->item);
	pnext = p->next;
	free(p);
    }
    memhead = NULL;
}

void *
getmem(size_t size)
{
    void *rtn;

    rtn = malloc(size);
    if (rtn){
	addto_memlist(rtn);
    }
    return rtn;
}

void
release_memitem(void *adr)
{
    MemItem *p;
    MemItem **ppnext;		/* Pointer to Previous Next */

    if (!adr) return;
    ppnext = &memhead;
    for (p=memhead; p; p=p->next){
	if (p->item == adr){
	    *ppnext = p->next;
	    free(p->item);
	    free(p);
	    break;
	}
	ppnext = &(p->next);
    }
}

/*
 * End memory allocation management
 */

static char **outpaths;
static int *out_frame;

int
mathexpr(ParmList inparms, ParmList *outparms)
{
    int i, j, k;
    int len;
    int indx;
    int ifile;
    int iname;
    int nfiles;
    int nnames;
    int nsrcs;
    int ok;
    float val;
    char *pc;
    char fname[MAXPATHLEN];
    FDFptr st;
    ParmList src_ddls;
    ParmList src_ddlvecs;
    ParmList src_strings;
    ParmList src_constants;
    ParmList dst_frames;
    ParmList dst_ddls;
    ParmList ddlvec;

    FDFptr *filehandle;
    float **srcimg;

    /*
     * Parse the arguments
     */
    src_ddls = findParm(inparms, "src_ddls");
    if (!src_ddls){
	ib_errmsg("MATH: \"src_ddls\" not passed");
	return FALSE;
    }
    /*printParm(src_ddls); */ /*CMP*/
    src_ddlvecs = findParm(inparms, "src_ddlvecs");
    if (!src_ddlvecs){
	ib_errmsg("MATH: \"src_ddlvecs\" not passed");
	return FALSE;
    }
    /*printParm(src_ddlvecs); */ /*CMP*/

    /*
     * Allocate memory
     */
    nbr_image_vecs = countParm(src_ddlvecs);
    in_vec_len = (int *)getmem(nbr_image_vecs * sizeof(int));
    vecindx = (int *)getmem(nbr_image_vecs * sizeof(int));
    for (i=nsrcs=0; i<nbr_image_vecs; i++){
	getParmParm(src_ddlvecs, &ddlvec, i);
	in_vec_len[i] = countParm(ddlvec);
	nsrcs += in_vec_len[i];
    }
    vecindx[0] = 0;
    for (i=1; i<nbr_image_vecs; i++){
	vecindx[i] = vecindx[i-1] + in_vec_len[i-1];
    }
    nbr_infiles = nsrcs;
    if (nsrcs){
	/* Source data pointers */
	in_data = (float **)getmem(nsrcs * sizeof(float *));
	/* FDF file handles */
	in_object = (DDLSymbolTable **)getmem(nsrcs*sizeof(DDLSymbolTable *));
	/* Image-size arrays */
	in_width = (int *)getmem(nsrcs * sizeof(int));
	in_height = (int *)getmem(nsrcs * sizeof(int));
	in_depth = (int *)getmem(nsrcs * sizeof(int));
	in_size = (int *)getmem(nsrcs * sizeof(int));
	if (!in_data || !in_object
	    || !in_width || !in_height || !in_depth || !in_size)
	{
	    fprintf(stderr,"MATH: memory allocation failure\n");
	    return FALSE;
	}
    }
    /*fprintf(stderr,"nsrcs=%d, memory allocated\n", nsrcs); */ /*CMP*/
    img_width = img_height = img_depth = 0;

    /* Store the input strings */
    src_strings = findParm(inparms, "src_strings");
    nbr_strings = countParm(src_strings);
    if (nbr_strings){
	in_strings = (char **)getmem(nbr_strings * sizeof(char *));
	for (i=0; i<nbr_strings; i++){
	    getStrParm(src_strings, &pc, i);
	    in_strings[i] = (char *)strdup(pc);
	    addto_memlist(in_strings[i]);
	}
    }

    /* Store the input parameters */
    src_constants = findParm(inparms, "src_constants");
    nbr_params = countParm(src_constants);
    if (nbr_params){
	in_params = (float *)getmem(nbr_params * sizeof(float));
	for (i=0; i<nbr_params; i++){
	    getFloatParm(src_constants, &val, i);
	    in_params[i] = val;
	}
    }

    /*
     * Load the input data
     */
    input_sizes_differ = FALSE;
    img_width = img_height = img_depth = 0;
    for (indx=0; indx<nsrcs; indx++){
	getPtrParm(src_ddls, &st, indx);

	/* Check image size */
	i = get_image_width(st);
	j = get_image_height(st);
	k = get_image_depth(st);
	if (!i || !j){
	    fprintf(stderr,"MATH: image size is %d x %d\n", i, j);
	    return FALSE;
	}
	if ((img_width && i != img_width)
	    || (img_height && j != img_height)
	    || (img_depth && k != img_depth))
	{
	    fprintf(stderr,"MATH: Warning: images are different sizes\n");
	    input_sizes_differ = TRUE;
	}
	img_width = in_width[indx] = i;
	img_height = in_height[indx] = j;
	img_depth = in_depth[indx] = k;
	img_size = in_size[indx] = i * j * k;
	in_object[indx] = st;
	in_data[indx] = get_ddl_data(st);
    }

    /*
     * Output files
     */
    dst_frames = findParm(inparms, "dst_frames");
    if (!dst_frames){
	ib_errmsg("MATH: \"dst_frames\" not passed");
	return FALSE;
    }
    /*printParm(dst_frames); */ /*CMP*/
    nbr_outfiles = countParm(dst_frames);
    out_object = (FDFptr *)getmem(nbr_outfiles * sizeof(FDFptr));
    out_data = (float **)getmem(nbr_outfiles * sizeof(float *));
    out_frame = (int *)getmem(nbr_outfiles * sizeof(int));
    out_width = (int *)getmem(nbr_outfiles * sizeof(int));
    out_height = (int *)getmem(nbr_outfiles * sizeof(int));
    out_depth = (int *)getmem(nbr_outfiles * sizeof(int));
    out_size = (int *)getmem(nbr_outfiles * sizeof(int));
    /* Initialize output file sizes */
    for (i=0; i<nbr_outfiles; i++){
	out_width[i] = img_width;
	out_height[i] = img_height;
	out_depth[i] = img_depth;
	out_size[i] = img_size;
	getIntParm(dst_frames, &out_frame[i], i);
	out_object[i] = NULL;
	out_data[i] = NULL;
    }
    /*fprintf(stderr,"nbr_image_vecs=%d", nbr_image_vecs);
    if (nbr_image_vecs > 1){
	fprintf(stderr,", in_vec_len[1]=%d, vecindx[1]=%d\n",
		in_vec_len[1], vecindx[1]);
    }else{
	fprintf(stderr,"\n");
    } */ /*CMP*/

    /* Call user function */
    ok = mathfunc();

    if (!ok){
	fprintf(stderr,"MATH: user \"mathfunc\" failed\n");
    }else{
	/* Pass back the output data */
	*outparms = allocParm("dst_ddls", PL_PTR, nbr_outfiles);
	for (i=0; i<nbr_outfiles; i++){
	    if (want_output(i)){
		setPtrParm(*outparms, out_object[i], i);
	    }
	}
    }

    /* After return, we will be unloaded--last chance to release memory. */
    release_memlist();
    return ok;
}

int
want_output(int n)
{
    int rtn;

    rtn = n>=0 && n<nbr_outfiles && out_frame[n] >= 0 ? TRUE : FALSE;
    return rtn;
}

int
create_output_files(int n, FDFptr cloner)
{
    int i;

    if (n < 0){
	n = nbr_outfiles;
    }
    for (i=0; i<n; i++){
	if (want_output(i)){
	    out_object[i] = clone_ddl(cloner, 0);
	    init_ddl(out_object[i], out_width[i], out_height[i], out_depth[i]);
	    out_data[i] = get_ddl_data(out_object[i]);
	}
    }
    return TRUE;
}

int
write_output_files()
{
    return TRUE;
}

#ifndef LINUX
int
gethostname(char *name, int buflen)
{
    int i;
    *name = '\0';
    i = sysinfo(SI_HOSTNAME, name, (long)buflen);
    return i >= 0 ? 0 : -1;
}
#endif

/* take from win_math.c & msgprt.c in ib */
/* 
* At one point I'm guessing these function were with in one of the ib libraries created but no more,
*  since they contain quite a bit of X code in hopes of provided a simple solttuion just have the
*  message output to stderr rather some X windows which did seem to happen anyway.
*     Greg B.  9/18/08
*/
int ib_errmsg(char *message)
{
    fprintf(stderr,"%s\n",message);
    return 0;
}

int ib_msgline(char *message)
{
    fprintf(stderr,"%s\n",message);
    return 0;
}

// actually in interrupt.c in ib only used in fit.c
/*
* these appeared to use the X window signal to catch some sort of windows signal from the
* user? maybe. But the interrupt_registered was never called which make me don't this was 
* actually used at all, so I've dummied it out here.. Greg B.  9/18/08
*/
int interrupt() { }
int interrupt_begin() { }
int interrupt_end() { }
