/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************
    sis_initpar.c
**********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "vnmrsys.h"
#ifndef NVPSG
#include "oopc.h"
extern void recon();
#endif
#include "group.h"
#include "variables.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"
#include "abort.h"
#include "pvars.h"
#include "cps.h"

int S_getarray (const char *parname, double array[], int arraysize);

#ifdef NVPSG
typedef struct _RFpattern {
    double  phase;
    double  amp;
    double  time;
} RFpattern;

typedef struct _DECpattern {
    double  tip;
    double  phase;
    double  amp;
    int gate;
} DECpattern;

typedef struct _Gpattern {
    double  amp;
    double  time;
    int ctrl;
} Gpattern;
#endif /* NVPSG */
/**************************************************************
    initparms_img()

    Read in all parameters used by the Imaging 
    SEQD applications package. Build the array 
    of slice positions used in multislice loops.
 *************************************************************/
void initparms_img()
{
    int k;                  /*index for loops*/

/*-------------------------------------------------------------
    INITIALIZE PARAMETERS
-------------------------------------------------------------*/

/*--------------------------------------
    RFPULSES
--------------------------------------*/
    p2 = getvalnwarn("p2");
    p3 = getvalnwarn("p3");
    p4 = getvalnwarn("p4");
    p5 = getvalnwarn("p5");
    pi = getvalnwarn("pi");
    psat = getvalnwarn("psat");
    pmt = getvalnwarn("pmt");
    pwx2 = getvalnwarn("pwx2");
    psl = getvalnwarn("psl");

    getstrnwarn("pwpat",pwpat);
    getstrnwarn("p1pat",p1pat);
    getstrnwarn("p2pat",p2pat);
    getstrnwarn("p3pat",p3pat);
    getstrnwarn("p4pat",p4pat);
    getstrnwarn("p5pat",p5pat);
    getstrnwarn("pipat",pipat);
    getstrnwarn("satpat",satpat);
    getstrnwarn("mtpat",mtpat);
    getstrnwarn("pslpat",pslpat);

    tpwr1 = getvalnwarn("tpwr1");
    tpwr2 = getvalnwarn("tpwr2");
    tpwr3 = getvalnwarn("tpwr3");
    tpwr4 = getvalnwarn("tpwr4");
    tpwr5 = getvalnwarn("tpwr5");
    tpwri = getvalnwarn("tpwri");
    mtpwr = getvalnwarn("mtpwr");
    pwxlvl2 = getvalnwarn("pwxlvl2");
    tpwrsl = getvalnwarn("tpwrsl");

/*--------------------------------------
    DECOUPLER PULSES
--------------------------------------*/
    getstrnwarn("decpat",decpat);
    getstrnwarn("decpat1",decpat1);
    getstrnwarn("decpat2",decpat2);
    getstrnwarn("decpat3",decpat3);
    getstrnwarn("decpat4",decpat4);
    getstrnwarn("decpat5",decpat5);

    dpwr = getvalnwarn("dpwr");
    dpwr1 = getvalnwarn("dpwr1");
    dpwr4 = getvalnwarn("dpwr4");
    dpwr5 = getvalnwarn("dpwr5");

/*--------------------------------------
    GRADIENTS
--------------------------------------*/
    gradunit = getvalnwarn("gradunit");
    gro = getvalnwarn("gro");
    gro2 = getvalnwarn("gro2");
    gro3 = getvalnwarn("gro3");
    gpe = getvalnwarn("gpe");
    gpe2 = getvalnwarn("gpe2");
    gpe3 = getvalnwarn("gpe3");
    gss = getvalnwarn("gss");
    gss2 = getvalnwarn("gss2");
    gss3 = getvalnwarn("gss3");
    gror = getvalnwarn("gror");
    gssr = getvalnwarn("gssr");
    grof = getvalnwarn("grof");
    gssf = getvalnwarn("gssf");
    g0 = getvalnwarn("g0");
    g1 = getvalnwarn("g1");
    g2 = getvalnwarn("g2");
    g3 = getvalnwarn("g3");
    g4 = getvalnwarn("g4");
    g5 = getvalnwarn("g5");
    g6 = getvalnwarn("g6");
    g7 = getvalnwarn("g7");
    g8 = getvalnwarn("g8");
    g9 = getvalnwarn("g9");
    gx = getvalnwarn("gx");
    gy = getvalnwarn("gy");
    gz = getvalnwarn("gz");
    gvox1 = getvalnwarn("gvox1");
    gvox2 = getvalnwarn("gvox2");
    gvox3 = getvalnwarn("gvox3");
    gdiff = getvalnwarn("gdiff");
    gflow = getvalnwarn("gflow");
    gspoil = getvalnwarn("gspoil");
    gspoil2 = getvalnwarn("gspoil2");
    gcrush = getvalnwarn("gcrush");
    gcrush2 = getvalnwarn("gcrush2");
    gtrim = getvalnwarn("gtrim");
    gtrim2 = getvalnwarn("gtrim2");
    gramp = getvalnwarn("gramp");
    gramp2 = getvalnwarn("gramp2");
    gxscale = getvalnwarn("gxscale");
    gyscale = getvalnwarn("gyscale");
    gzscale = getvalnwarn("gzscale");
    gpemult = getvalnwarn("gpemult");

    /* Get gmax or gxmax, gymax, gzmax */
    if ( P_getreal(CURRENT,"gxmax",&gxmax,1) < 0 )
    {
    	if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
    	{
		gmax = 0;
    	}
	gxmax = gmax;
    }
    if ( P_getreal(CURRENT,"gymax",&gymax,1) < 0 )
    {
    	if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
    	{
		gmax = 0;
	}
	gymax = gmax;
    }
    if ( P_getreal(CURRENT,"gzmax",&gzmax,1) < 0 )
    {
    	if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
    	{
		gmax = 0;
	}
	gzmax = gmax;
    }
    if (gxmax < gzmax)
	if (gxmax < gymax)
	   gmax = gxmax;
	else
	   gmax = gymax;
    else
	if (gxmax < gymax)
	   gmax = gxmax;
	else
	   gmax = gymax;

    /* --- Get gradient limit parameters --- */
    if ( P_getreal(CURRENT,"gtotlimit",&gtotlimit,1) < 0 )
	gtotlimit = gxmax + gymax + gzmax;
    if (gtotlimit <= 0.0) gtotlimit = gxmax + gymax + gzmax;

    if ( P_getreal(CURRENT,"gxlimit",&gxlimit,1) < 0 )
	gxlimit = gxmax;
    if (gxlimit <= 0.0) gxlimit = gxmax;

    if ( P_getreal(CURRENT,"gylimit",&gylimit,1) < 0 )
	gylimit = gymax;
    if (gylimit <= 0.0) gylimit = gymax;

    if ( P_getreal(CURRENT,"gzlimit",&gzlimit,1) < 0 )
	gzlimit = gzmax;
    if (gzlimit <= 0.0) gzlimit = gzmax;

    /* --- Get gradstepsz if Present --- */
    /* if gradstepsz is 32767 gradient waveshaping is being used  */
    if ( P_getreal(GLOBAL,"gradstepsz",&gradstepsz,1) >= 0 )
    {
	if (gradstepsz < 1.0)
	{
	    if ((anygradcrwg) || (anygradwg) || (anypfga) || (anypfgw))
		gradstepsz = 32767.0;
	    else
		gradstepsz = 2047.0;
	}
    }
    else
    {
	if ((anygradcrwg) || (anygradwg) || (anypfga) || (anypfgw))
	    gradstepsz = 32767.0;
	else
	    gradstepsz = 2047.0;
    }

    getstrnwarn("gpatup",gpatup);
    getstrnwarn("gpatdown",gpatdown);
    getstrnwarn("gropat",gropat);
    getstrnwarn("gpepat",gpepat);
    getstrnwarn("gsspat",gsspat);
    getstrnwarn("gpat",gpat);
    getstrnwarn("gpat1",gpat1);
    getstrnwarn("gpat2",gpat2);
    getstrnwarn("gpat3",gpat3);
    getstrnwarn("gpat4",gpat4);
    getstrnwarn("gpat5",gpat5);

/*--------------------------------------
    DELAYS
--------------------------------------*/
    tr = getvalnwarn("tr");
    te = getvalnwarn("te");
    ti = getvalnwarn("ti");
    tm = getvalnwarn("tm");
    at = getvalnwarn("at");
    tpe = getvalnwarn("tpe");
    tpe2 = getvalnwarn("tpe2");
    tpe3 = getvalnwarn("tpe3");
    tcrush = getvalnwarn("tcrush");
    tdiff = getvalnwarn("tdiff");
    tdelta = getvalnwarn("tdelta");
    tDELTA = getvalnwarn("tDELTA");
    tflow = getvalnwarn("tflow");
    tspoil = getvalnwarn("tspoil");
    hold = getvalnwarn("hold");
    trise = getvalnwarn("trise");
    if (trise > 10e-3) abort_message("trise: %5.0f usec is excessive!",trise*1e6);

/*--------------------------------------
    FREQUENCIES
--------------------------------------*/
    resto = getvalnwarn("resto");
    wsfrq = getvalnwarn("wsfrq");
    chessfrq = getvalnwarn("chessfrq");
    mtfrq = getvalnwarn("mtfrq");

/*--------------------------------------
    PHYSICAL POSITIONS AND FOV
--------------------------------------*/
    fovunit = getvalnwarn("fovunit");
    pro = getvalnwarn("pro");
    ppe = getvalnwarn("ppe");
    ppe2 = getvalnwarn("ppe2");
    ppe3 = getvalnwarn("ppe3");
    pos1 = getvalnwarn("pos1");
    pos2 = getvalnwarn("pos2");
    pos3 = getvalnwarn("pos3");
    lro = getvalnwarn("lro");
    lpe = getvalnwarn("lpe");
    lpe2 = getvalnwarn("lpe2");
    lpe3 = getvalnwarn("lpe3");
    lss = getvalnwarn("lss");

/*--------------------------------------
    PHYSICAL SIZES
--------------------------------------*/
    thkunit = getvalnwarn("thkunit");
    vox1 = getvalnwarn("vox1");
    vox2 = getvalnwarn("vox2");
    vox3 = getvalnwarn("vox3");
    thk = getvalnwarn("thk");

/*--------------------------------------
    BANDWIDTHS
--------------------------------------*/
    sw1 = getvalnwarn("sw1");
    sw2 = getvalnwarn("sw2");
    sw3 = getvalnwarn("sw3");

/*--------------------------------------
    ORIENTATION PARAMETERS
--------------------------------------*/
    getstrnwarn("orient",orient);
    getstrnwarn("vorient",vorient);
    getstrnwarn("dorient",dorient);
    getstrnwarn("sorient",sorient);
    getstrnwarn("orient2",orient2);
        
    psi = getvalnwarn("psi");
    phi = getvalnwarn("phi");
    theta = getvalnwarn("theta");
    vpsi = getvalnwarn("vpsi");
    vphi = getvalnwarn("vphi");
    vtheta = getvalnwarn("vtheta");
    dpsi = getvalnwarn("dpsi");
    dphi = getvalnwarn("dphi");
    dtheta = getvalnwarn("dtheta");
    spsi = getvalnwarn("spsi");
    sphi = getvalnwarn("sphi");
    stheta = getvalnwarn("stheta");

    offsetx = getvalnwarn("offsetx");
    offsety = getvalnwarn("offsety");
    offsetz = getvalnwarn("offsetz");
    gxdelay = getvalnwarn("gxdelay");
    gydelay = getvalnwarn("gydelay");
    gzdelay = getvalnwarn("gzdelay");

/*--------------------------------------
    COUNTS AND FLAGS
--------------------------------------*/
    nD = getvalnwarn("nD");
    ns = getvalnwarn("ns");
    ne = getvalnwarn("ne");
    ni = getvalnwarn("ni");
    nv = getvalnwarn("nv");
    nv2 = getvalnwarn("nv2");
    nv3 = getvalnwarn("nv3");
    ssc = getvalnwarn("ssc");
    ticks = getvalnwarn("ticks");
        
    getstrnwarn("ir",ir);
    getstrnwarn("ws",ws);
    getstrnwarn("mt",mt);
    getstrnwarn("pilot",pilot);
    getstrnwarn("seqcon",seqcon);
    getstrnwarn("petable",petable);
    getstrnwarn("acqtype",acqtype);
    getstrnwarn("exptype",exptype);
    getstrnwarn("apptype",apptype);
    getstrnwarn("seqfil",seqfil);
    getstrnwarn("rfspoil",rfspoil);
    getstrnwarn("verbose",verbose);

/*--------------------------------------
    Miscellaneous
--------------------------------------*/
    rfphase = getvalnwarn("rfphase");
    B0 = getvalnwarn("B0");
    gpropdelay = getvalnwarn("gpropdelay");
    kzero = getvalnwarn("kzero");
    aqtm = getvalnwarn("aqtm");
    getstrnwarn("volumercv",volumercv);


/*--------------------------------------
    Build slice position array
--------------------------------------*/
    if (ns > MAXSLICE) {
        text_error("ns exceeds maximum limit.\n");
        psg_abort(1);
    }

    for (k = 0; k < MAXSLICE; k++) {
        pss[k] = 0.0;
    }

    if (seqcon[1] == 's')
	pss[0] = getval("pss");
    else if (seqcon[1] == 'c')
    	getarray("pss",pss);
    else
	pss[0] = getvalnwarn("pss");	/* get it anyway but with no warning */


/*--------------------------------------
    Old SISCO Imaging Parameters
--------------------------------------*/
    slcto = getvalnwarn("slcto");    /* slice selection offset */
    delto = getvalnwarn("delto");    /* slice spacing frequency */
    tox = getvalnwarn("tox");
    toy = getvalnwarn("toy");
    toz = getvalnwarn("toz");
    griserate = getvalnwarn("griserate");

}


/*----------------------------------------------*/
/* Init receiver on for explicit acquisitions	*/
/*----------------------------------------------*/
void initparms_sis()
{
#ifndef NVPSG    
    recon();
#endif
}


/**************************************************************
    init_RFpattern()

    This routine creates an RF pattern file from an internally
    defined RF pattern structure.

    Usage:  init_RFpattern ( pattern_name, structure_name, no_steps )
**************************************************************/
void init_RFpattern (pat_name, rfpat_struct, nsteps)
char       *pat_name;
RFpattern  *rfpat_struct;
int        nsteps;
{
    int     k;
    char    file_name[MAXSTR];
    FILE    *file_ptr;

/*-------------------------------------------------------------
  Assemble absolute path name for new RF pattern file
-------------------------------------------------------------*/
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_name);
    strcat (file_name, ".RF");
    if (strlen(file_name) > MAXSTR)
    {
        abort_message("Filename for pattern, %s, greater than MAXSTR.\n",
                       pat_name);
    }


/*-------------------------------------------------------------
  Open RF pattern file for writing
-------------------------------------------------------------*/
    if ( (file_ptr = fopen (file_name, "w") ) ==  NULL ) {
        perror("fopen");
        abort_message("init_RFpattern: unable to open output file.\n");
    }

/*-------------------------------------------------------------
  Write RF pattern file
-------------------------------------------------------------*/
    for (k=0; k<nsteps; k++) {
#ifdef NVPSG
	fprintf (file_ptr, "  %.4f        %.4f        %.1f\n",
	  rfpat_struct[k].phase, rfpat_struct[k].amp, rfpat_struct[k].time);
#else
	fprintf (file_ptr, "  %.2f        %.1f        %.1f\n",
	  rfpat_struct[k].phase, rfpat_struct[k].amp, rfpat_struct[k].time);
#endif
    }

/*-------------------------------------------------------------
  Close RF pattern file
-------------------------------------------------------------*/
    fclose(file_ptr);
}


/**************************************************************
    init_DECpattern()

    This routine creates an DEC pattern file from an internally
    defined DEC pattern structure.

    Usage:  init_DECpattern( pattern_name, structure_name, no_steps )
**************************************************************/
void init_DECpattern(pat_name, decpat_struct, nsteps)
char       *pat_name;
DECpattern  *decpat_struct;
int        nsteps;
{
    int     k, errflag, gateflag;
    char    file_name[MAXSTR];
    FILE    *file_ptr;

    errflag = 0;
    gateflag = 0;
/*-------------------------------------------------------------
  Check fields of DEC pattern 
  - Check to make sure cntrl values are valid.
  - Set gateflag if gate field is used.
-------------------------------------------------------------*/
    for (k=0; k<nsteps; k++) {
	if ((decpat_struct[k].gate < 0) || (decpat_struct[k].gate > 3)) {
	    errflag = 1;
	    break;
	}
	if (decpat_struct[k].gate != 0) gateflag = 1;
    }
    
    if (errflag) {
	text_error("init_decpattern: Check gate field of DECpattern exists and is set.");
	psg_abort(1);
    }

/*-------------------------------------------------------------
  Assemble absolute path name for new DEC pattern file
-------------------------------------------------------------*/
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_name);
    strcat (file_name, ".DEC");
    if (strlen(file_name) > MAXSTR)
    {
        abort_message("Filename for pattern, %s, greater than MAXSTR.\n",
                       pat_name);
    }


/*-------------------------------------------------------------
  Open DEC pattern file for writing
-------------------------------------------------------------*/
    if ( (file_ptr = fopen (file_name, "w") ) ==  NULL ) {
        perror("fopen");
        printf("init_DECpattern: unable to open output file.\n");
        psg_abort(1);
    }

/*-------------------------------------------------------------
  Write DEC pattern file
    if gateflag is set write pattern with a transmitter flag.
-------------------------------------------------------------*/
    if (gateflag) {
	for (k=0; k<nsteps; k++) {
#ifdef NVPSG
	  fprintf (file_ptr, "  %.1f        %.4f        %.4f   %2d\n",
	      decpat_struct[k].tip, decpat_struct[k].phase, 
	                   decpat_struct[k].amp,decpat_struct[k].gate);
#else
fprintf (file_ptr, "  %.1f        %.2f        %.1f   %2d\n",
	      decpat_struct[k].tip, decpat_struct[k].phase, 
	                   decpat_struct[k].amp,decpat_struct[k].gate);
#endif
	}
    }
    else {
	for (k=0; k<nsteps; k++) {
	  fprintf (file_ptr, "  %.1f        %.2f        %.1f \n",
	   decpat_struct[k].tip, decpat_struct[k].phase, decpat_struct[k].amp);
	}
    }

/*-------------------------------------------------------------
  Close DEC pattern file
-------------------------------------------------------------*/
    fclose(file_ptr);
}



/**************************************************************
    init_Gpattern()

    This routine creates a gradient pattern file from an internally
    defined Gpattern structure.

    Usage:  init_Gpattern ( pattern_name, gradpat_struct, no_steps )
**************************************************************/
void init_Gpattern (char *pat_name, Gpattern *gradpat_struct, int nsteps)
{
    int     k;
    char    file_name[MAXSTR];
    FILE    *file_ptr;

/*-------------------------------------------------------------
  Assemble absolute path name for new gradient pattern file
-------------------------------------------------------------*/
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_name);
    strcat (file_name, ".GRD");
    if (strlen(file_name) > MAXSTR)
    {
        abort_message("Filename for pattern, %s, greater than MAXSTR.\n",
                       pat_name);
    }

/*-------------------------------------------------------------
  Open gradient pattern file for writing
-------------------------------------------------------------*/
    if ( (file_ptr = fopen (file_name, "w") ) ==  NULL ) {
        perror("fopen");
        abort_message("init_Gpattern: unable to open output file.\n");
    }

/*-------------------------------------------------------------
  Write gradient pattern file
-------------------------------------------------------------*/
    for (k=0; k<nsteps; k++) {
	fprintf (file_ptr, "  %.1f        %.1f\n",
	         gradpat_struct[k].amp, gradpat_struct[k].time);
    }

/*-------------------------------------------------------------
  Close gradient pattern file
-------------------------------------------------------------*/
    fclose(file_ptr);
}

/**************************************************************
    init_Bpattern()

    This routine creates a gradient pattern file from an internally
    defined Gpattern structure.
    Same as init_Gpattern, except it also writes out the control words
    (currently used for the gradient Booster).

    Usage:  init_Bpattern ( pattern_name, gradpat_struct, no_steps )
**************************************************************/
void init_Bpattern (char *pat_name, Gpattern *gradpat_struct, int nsteps)
{
    int     k;
    char    file_name[MAXSTR];
    FILE    *file_ptr;

/*-------------------------------------------------------------
  Assemble absolute path name for new gradient pattern file
-------------------------------------------------------------*/
    strcpy (file_name, userdir);
    strcat (file_name, "/shapelib/");
    strcat (file_name, pat_name);
    strcat (file_name, ".GRD");
    if (strlen(file_name) > MAXSTR)
    {
        abort_message("Filename for pattern, %s, greater than MAXSTR.\n",
                       pat_name);
    }

/*-------------------------------------------------------------
  Open gradient pattern file for writing
-------------------------------------------------------------*/
    if ( (file_ptr = fopen (file_name, "w") ) ==  NULL ) {
        perror("fopen");
        abort_message("init_Bpattern: unable to open output file.\n");
    }

/*-------------------------------------------------------------
  Write gradient pattern file
-------------------------------------------------------------*/
    for (k=0; k<nsteps; k++) {
	fprintf (file_ptr, "  %.1f        %.1f  %2d\n",
	         gradpat_struct[k].amp, gradpat_struct[k].time,
		 gradpat_struct[k].ctrl);
    }

/*-------------------------------------------------------------
  Close gradient pattern file
-------------------------------------------------------------*/
    fclose(file_ptr);
}



/**************************************************************
    S_getarray(parname, array, arraysize)

    This routine retrieves all values of an arrayed parameter
    from the parameter set.  It requires the name of the parameter,
    a base array address to put the values in, and the size of the
    array to be used as a check against the number of arrayed
    elements in the parameter.  The number of elements in the
    arrayed parameter is determined with the P_getVarInfo call, and
    returned by the function as an integer.

    Usage:  size = S_getarray ( parameter name, array )
    (See the macro getarray in macros.h)
**************************************************************/
int S_getarray (const char *parname, double array[], int arraysize)
{
    int      size,r,i;
    vInfo    varinfo;

    if ( (r = P_getVarInfo(CURRENT, parname, &varinfo)) ) {
        printf("getarray: could not find the parameter \"%s\"\n",parname);
	psg_abort(1);
    }
    if ((int)varinfo.basicType != 1) {
        printf("getarray: \"%s\" is not an array of reals.\n",parname);
	psg_abort(1);
    }

    size = (int)varinfo.size;
    if (arraysize/sizeof(double) < size) {
        printf("getarray: number of parameter elements > array dimension.\n");
	psg_abort(1);
    }

    for (i=0; i<size; i++) {
        if ( P_getreal(CURRENT,parname,&array[i],i+1) ) {
	    printf("getarray: problem getting array element %d.\n",i+1);
	    psg_abort(1);
	}
    }
    return(size);
}

