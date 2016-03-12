/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* disp3Dmap.c 			 							*/

/* usage : disp3Dmap  or  disp3Dmap('filename')   or  disp3Dmap('filename',n1,n1,n3 ...)	*/
/* usage : plot3Dmap  or  plot3Dmap('filename')   or  plot3Dmap('filename',n1,n1,n3 ...)	*/
/* usage : disp1Dmap('filename',xbar,ybar)   or  disp1Dmap('filename',xbar,ybar,n1,n1,n3 ...)	*/
/* usage : plot1Dmap('filename',xbar,ybar)   or  plot3Dmap('filename',xbar,ybar,n1,n1,n3 ...)	*/

/* Display or plot a 2D representation of a 3D magnetic field or shim map			*/
/* If 'filename' contains multiple maps, plot first only (default) or numbers n1, n2 etc.	*/
/* Use map layout information in <filename>.dat if available			GAM 11xi02	*/
/* Correct gxyzstr to start at [0]						GAM 11xi02	*/
/* Assume first entry on each line of map is floating point not integer       	GAM 13xi02	*/
/* Strip .amp extension before searching for .dat file				GAM 14i03	*/
/* Fix crash if too few arguments for 1D display				GAM 14i03	*/
/* Display title of map, if given on line 3 of input file, and display mapname  GAM 10ii03	*/

/* Neatened up by Dan Iverson for VnmrJ compatibility 25 vii 03					*/
/* Allow up to 100 maps, change y0 to -10, reduce xstep by 1			GAM 13viii03	*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "vnmrsys.h"
#include "group.h"
#include "data.h"
#include "tools.h"
#include "variables.h"
#include "init2d.h"
#include "pvars.h"
#include "wjunk.h"
#include "shims.h"

#define Pi 3.14159265359
#define twoPi 2.0*Pi
#define MAX 256

extern int nmr_draw(int argc, char *argv[], int retc, char *retv[]);
extern int nmr_write(int argc, char *argv[], int retc, char *retv[]);


FILE *map,*datin;
static char mapname[MAX],path[MAXPATH],datapath[MAXPATH];
static char device[9];
static char str[80],maptitle[80],dumch[80];
static int ncols=6;
static int debug=0;

static void nmrWrite(double x, double y, char *string)
{
	int cargc,cretc;
	char *cargv[10],*cretv[10];
	char arg0[80]; 
	char arg1[80]; 
	char arg2[80]; 
	char arg3[80]; 

	cargc=5;
        cretc=0;
	strcpy(arg0,"write");
	strcpy(arg1,device);
	sprintf(arg2,"%8.2f",x);
	sprintf(arg3,"%8.2f",y);
	cargv[0]=arg0;
	cargv[1]=arg1;
	cargv[2]=arg2;
	cargv[3]=arg3;
	cargv[4]=string;
	/*
	for (i=0;i<=4;i++)
	{
	printf("argument number %d is  %s\n",i,cargv[i]);
	}
	*/
	nmr_write(cargc,cargv,cretc,cretv);
}

static void move(double x, double y)
{
	int cargc,cretc;
	char *cargv[10],*cretv[10];
	char arg0[80]; 
	char arg1[80]; 
	char arg2[80]; 
	char arg3[80]; 

	cargc=4;
        cretc=0;
	strcpy(arg0,"move");
	strcpy(arg1,device);
	sprintf(arg2,"%8.2f",x);
	sprintf(arg3,"%8.2f",y);
	cargv[0]=arg0;
	cargv[1]=arg1;
	cargv[2]=arg2;
	cargv[3]=arg3;
	/*
	for (i=0;i<=3;i++)
	{
	printf("argument number %d is  %s\n",i,cargv[i]);
	}
	*/
	nmr_draw(cargc,cargv,cretc,cretv);
}

static void draw(double x, double y)
{
	int cargc,cretc;
	char *cargv[10],*cretv[10];
	char arg0[80]; 
	char arg1[80]; 
	char arg2[80]; 

	cargc=3;
        cretc=0;
	strcpy(arg0,"draw");
	sprintf(arg1,"%8.2f",x);
	sprintf(arg2,"%8.2f",y);
	cargv[0]=arg0;
	cargv[1]=arg1;
	cargv[2]=arg2;
	/*
	for (i=0;i<=2;i++)
	{
	printf("argument number %d is  %s\n",i,cargv[i]);
	}
	*/
	nmr_draw(cargc,cargv,cretc,cretv);
	/*
	*/
}

static void pen(int penarg)
{
	int cargc,cretc;
	char *cargv[10],*cretv[10];
	char arg0[80]; 
	char arg1[80]; 

	cargc=2;
        cretc=0;
	strcpy(arg0,"pen");
        if (penarg == 10)
           strcpy(arg1,"envelope");
        else
           sprintf(arg1,"Spectrum%d",penarg);
	cargv[0]=arg0;
	cargv[1]=arg1;

	/*
	for (i=0;i<=1;i++)
	{
	printf("argument number %d is  %s\n",i,cargv[i]);
	}
	*/
	nmr_draw(cargc,cargv,cretc,cretv);
	/*
	(void) getchar();
	*/
}

int disp3Dmap(int argc, char *argv[], int retc, char *retv[])
{
/*	main										*/
	char gxyzstr[MAX];
	int flag3D,datinexists=0,nptdat,nlines,mapno,mapsused,i,j,k,l,ni,nmaps,np,npt,ntr,
	nchosen,nvalid,ii,kk,i1,i2,i3,i4;
	int chosen[11]={0,0,0,0,0,0,0,0,0,0,0},xbar,ybar;
	int used[MAX_SHIMS+1];
	double Hzpp,xst,range,amax,amin,max,min,x,y,dx,dth,th,r,rni,wcmax,wc2max,x0,y0,cth,sth;
	double *data;  
	int found;
	int flagamp=0;

        (void) retc;
        (void) retv;

	if (argv[0][0]=='d')
	{
		strcpy(device,"graphics");
	}
	else
	{
		strcpy(device,"plotter");
	}

	flag3D=1;
	if (argv[0][4]=='1')
	{
		flag3D=0;
	}
	if (debug) printf("device is  %s\n",device);


	if ((!flag3D)&&(argc<4))
	{
		Werrprintf("At least 3 arguments needed for this command \n");
		ABORT;
	}
        for (i=0; i < MAX_SHIMS+1; i++)
           used[i] = 0;
        xbar = ybar = 0;
	if (!flag3D) xbar=atof(argv[2]);
	if (!flag3D) ybar=atof(argv[3]);
	/* ###### read relevant parameters, from current experiment or from .dat file where available ###### */

	P_getreal(GLOBAL,"wcmax",&wcmax,1);
	P_getreal(GLOBAL,"wc2max",&wc2max,1);
        P_getstring(CURRENT,"mapname",mapname,1,MAX);

/* 	Find whether target file, if any, exists in curent directory or in 3Dshimlib/shimmaps/mapname	*/
	if (argc>1) 
	{
	found=0;
	strcpy(path, argv[1]);
	if ((datin = fopen(path,"r")) != NULL)
	{
		found=1;
		strcpy(datapath,"");
		fclose(datin);
	}
	if (!found)
	{
	strcpy(datapath,userdir);
	strcat(datapath,"/3Dshimlib/shimmaps/");
	strcat(datapath,mapname);
	strcat(datapath,"/");
	strcpy(path,datapath);
	strcat(path,argv[1]);
	if ((datin = fopen(path,"r")) != NULL)
	{
		found=1;
		fclose(datin);
	}
	}

	if (!found)
	{
		Werrprintf("Requested map file was not found in current directory or in 3Dshimlib/shimmaps/%s \n",mapname);
		ABORT;
	}
	}

	if (argc==1)  /* No input file specified, so assume fieldmap */
	{
		strcpy(path, userdir);
		strcat(path, "/3Dshimlib/data/fieldmap.dat");
	}
	else
	{
		strcpy(path,datapath);
		strcat(path, argv[1]);
		if(strstr(argv[1],".amp"))
		{
			flagamp=1;
			path[strlen(argv[1])-4]='\0';
		}
		strcat(path, ".dat");
	}
	if ((datin = fopen(path,"r")) == NULL)
	{
		if (P_getreal(CURRENT,"ni",&rni,1))
		{
			Werrprintf("Parameter ni not found in current experiment\n");
			ABORT;
		}
		ni = (int)rni;
		if (P_getstring(CURRENT,"gxyzcode",gxyzstr,1,MAX))
		{
			Werrprintf("Parameter gxyzcode not found in current experiment\n");
			ABORT;
		}
	}
	else
	{
		datinexists=1;
		if ((fscanf(datin,"ni = %d\n",&ni) == EOF)
		    || (fscanf(datin,"gxyzcode =  %s\n",gxyzstr) == EOF)
		    || (fscanf(datin,"Hzpp =  %lf\n",&Hzpp) == EOF)
		    || (fscanf(datin,"npt = %d\n",&nptdat) == EOF))
		{
			Werrprintf("disp3Dmap: reached premature end of file %s", path);
			fclose(datin);
			ABORT;
		}
		rni=(double) ni;
		fclose(datin);
	}

	if (((int) strlen(gxyzstr)) != (ni*ni))
	{
		Werrprintf("gxyzcode length is %d, not %d;  command cancelled \n",(int) strlen(gxyzstr),(int) (rni*rni));
		ABORT;
	}

	if (debug)
	{
		printf("wcmax =  %f\n",wcmax);
		printf("wc2max =  %f\n",wc2max);
		printf("gxyzcode is \n%s\n  ",gxyzstr);
		printf("ni = %d\n  ",ni);
	}

	/* Check size of map file */

	if (argc==1)
	{
		strcpy(path, userdir);
		strcat(path, "/3Dshimlib/data/fieldmap");
		if ((map = fopen(path,"r")) == NULL)
		{
			Werrprintf("Cannot open input map file");
			ABORT; 
		}
	}
	else
	{
		strcpy(path,datapath);
		strcat(path, argv[1]);
		if ((map = fopen(path,"r")) == NULL)
		{
			Werrprintf("Cannot open input map file");
			ABORT; 
		}
	}

	nlines=0;
	while (fgets(dumch,80,map)) nlines++;

	/* Rewind map file and start drawing */
	rewind(map);

	Wclear(2);
	Wsetgraphicsdisplay("");

	x0=wcmax-wc2max-3;  
	y0=-10;
	move(x0,y0);
	/*
	*/

	/* draw frame, circle and lattice */

	pen(8);
	draw(x0,y0+wc2max);
	draw(x0+wc2max,y0+wc2max);
	draw(x0+wc2max,y0);
	draw(x0,y0);
	dx=wc2max/rni;
	move(x0+wc2max/2,y0+dx/2);
	if (flag3D)
	{
		th=0.0;
		r=(wc2max-dx)/2.0;
		dth=twoPi/360.0;
		for (i=0; i<=360; i++)
		{
			th=th+dth;
			cth=cos(th);  
			sth=sin(th);
			draw(x0+dx/2+r*(1-sth),y0+dx/2+r*(1-cth));
		}
		x=x0; 
		y=y0;
		pen(9);
		for (i=1; i<ni; i++)
		{
			x=x+dx;
			move(x,y);
			draw(x,y+wc2max);
		}

		x=x0; 
		y=y0;
		for (i=1; i<ni; i++)
		{
			y=y+dx;
			move(x,y);
			draw(x+wc2max,y);
		}
	}
	else
	{
		pen(9);
		move(x0+wc2max,y0+wc2max/2);
		draw(x0,y0+wc2max/2);
	}


	/* input map header */

	fscanf(map,"%s %s\n",dumch,maptitle);
	fscanf(map,"%d %d\n",&nmaps,&np);
	fgets(maptitle,80,map);
	if (debug)
	{
		printf("Map title: %s\n",maptitle);
		printf("Number of maps :  %d\n",nmaps);
		printf("Number of points :  %d\n",np);
	}


	/* Check size of map file */

	if (nlines!=(3+nmaps*(np+2)))
	{
		Werrprintf("Map file is corrupt:  %d  lines long, not  %d  as expected\n",nlines,3+nmaps*(np+2));
		ABORT;
	}

	/* check how many bars are to be used */

	ntr=0;
	for (i=0;i<ni*ni;i++)
	{
		if (gxyzstr[i]=='1') ntr++;
	}

	npt=np/ntr;
	if (datinexists)
	{
		if(nptdat!=npt)
		{
			Werrprintf("Warning:  map file format is not consistent with the information in the .dat file\n");
		}
	}

	nchosen=1;
	chosen[1]=1;
	if (debug)
	{
		printf("Number of bars :  %d\n",ntr);
		printf("Number of points per trace :  %d\n",npt);
	}

	if (argc>2)
	{
		if (flag3D)
		{
			nchosen=argc-2;
		}
		else
		{
			nchosen=argc-4;
		}
		if (nchosen>10)
		{
			nchosen=10;
			Werrprintf("A maximum of 10 maps may be chosen;  any more are ignored\n");
		}
		ii=2;
		if (flag3D) ii=0;
		for (i=2;i<(nchosen+2);i++)
		{
			if (argc>i+ii) chosen[i-1]=atoi(argv[i+ii]);
			if (debug) printf("Number of map chosen :  %d\n",chosen[i-1]);
		}
	}
	if (!flag3D&&(nchosen==0))
	{
		nchosen=1; chosen[1]=1;
	}

	if (debug) printf("Number of maps chosen :  %d\n",nchosen);
	nvalid=nchosen;
	for (i=1;i<=nchosen;i++)
	{
		if ((chosen[i]<1)||(chosen[i]>nmaps))
		{
			nvalid=nvalid-1;
			Werrprintf("Map number %d does not exist - ignored\n",chosen[i]);
		}
	}

	if (nvalid<1)
	{
		Werrprintf("No valid maps chosen, so none drawn\n");
		fclose(map);
		ABORT;
	}

	/* Check which maps to use */
	if (debug) printf("Check which of the %d  maps to use\n",nmaps);


	for (i=1;i<=nmaps;i++)
	{
		used[i]=1;
		if (nchosen>0)
		{
			used[i]=0;
			for (j=1;j<=nchosen;j++)
			{
				if (i==chosen[j]) used[i]=1;
			}
		}
	}

	/* Allocate memory for map data */

	for (i=1;i<=nmaps;i++) if (debug) printf("Use map number %d ?  - %d\n",i,used[i]);

	if ((data=(double *)malloc(sizeof(double)*np*nmaps))==0)
	{
		Werrprintf("could not allocate memory\n");
		fclose(map);
                ABORT;
	}

	/* Read in map data */

	kk=0;
	max=-99999; 
	min=99999;
	for (i=1;i<=nmaps;i++)
	{
		fscanf(map,"%d %d %d %d",&i1,&i2,&i3,&i4);  /* skip to start of map data */
		for (j=1;j<=np;j++)
		{
			fscanf(map,"%s %lf",dumch,&data[kk]);
			if (data[kk]>max) max=data[kk];
			if (data[kk]<min) min=data[kk];
			kk++;
		}
	}

	if (debug) printf("Maximum is  %f  and minimum  %f\n",max,min);
	amax=fabs(max); 
	amin=fabs(min);
	if (amax<amin) { 
		max=amin; 
	} else { 
		max=amax;
	}
	min=-max;
	if (flagamp) min=0;
	range=max-min;
	if (debug) printf("Maximum is  %f  and minimum  %f\n",max,min);

	/* Draw scale bar */

	pen(10);
	if (flagamp)
	{
		sprintf(str,"%6.4f",range);
	}
	else
	{
		sprintf(str,"%6.2f Hz",range);
	}

/*	Display mapname and (1st 20 characters of) map title	*/
	x=10;
	y=y0+wc2max-10.0;
	nmrWrite(x,y,mapname);
	y=y0+wc2max-20.0;

	strcpy(dumch,maptitle);
	if (strlen(maptitle)>19)
	{
		strncpy(dumch,maptitle,20);
		dumch[20]='\0';
		strcpy(maptitle,dumch);
	}
	if (strchr(dumch,'\n')!=NULL) 
	{
		strcpy(maptitle," ");
		strncpy(maptitle,dumch,strchr(dumch,'\n')-dumch);
		maptitle[strchr(dumch,'\n')-dumch]='\0';
	}

	if (strncmp(maptitle,"index",5)!=0) nmrWrite(x,y,maptitle);

	x=wcmax-wc2max-10;
	if (flag3D)
	{
		y=y0+wc2max/2.0-dx/2.0;
		move(x-2,y);
		draw(x,y);
		draw(x,y+dx);
		draw(x-2,y+dx);
		nmrWrite(x-50,y+dx/2,str);
	}
	else
	{
		y=y0;
		move(x-2,y);
		draw(x,y);
		draw(x,y+wc2max);
		draw(x-2,y+wc2max);
		nmrWrite(x-50,y+wc2max/2,str);
	}
	if (debug) printf("device is  %s\n",device);

	/* Draw map */

	mapsused=0;
	kk=0;
	for (mapno=1;mapno<=nmaps;mapno++)		/* for each map */
	{
		pen(((mapno-1) % ncols) + 2);
		if (flag3D)
		{
			k=0;
			y=y0+dx/2;
			x=x0;
			xst=dx/(npt);
			for (i=1;i<=ni;i++)		/* step through x coordinate */
			{
				for (j=1;j<=ni;j++)	/* step through y coordinate */
				{

					if (gxyzstr[k]=='1')
					{
						x=x0+(j-1)*dx+xst/2.0;
						y=y0+(ni-i)*dx;
						move(x,y+((data[kk]-min)/range)*dx);
						kk++;
						x=x+xst;
						for (l=2;l<=npt;l++)
						{
							if (used[mapno]==1) draw(x,y+((data[kk]-min)/range)*dx);
							x=x+xst;
							kk++;
						}
					}
					k++;
				}
			}
		}
		else
		{
			k=0;
			xst=wc2max/(npt-1);
			for (i=1;i<=ni;i++)		/* step through x coordinate */
			{
				for (j=1;j<=ni;j++)	/* step through y coordinate */
				{

					if (gxyzstr[k]=='1')
					{
						x=x0;
						y=y0;
						move(x,y+((data[kk]-min)/range)*wc2max);
						kk++;
						x=x+xst;
						for (l=2;l<=npt;l++)
						{
							if ((used[mapno]==1)&(i==xbar)&(j==ybar)) draw(x,y+((data[kk]-min)/
							    range)*wc2max);
							x=x+xst;
							kk++;
						}
					}
					k++;
				}
			}
		}
		if (used[mapno]==1) mapsused++;
	}

	/*
	for (j=0;j<=npt;j++) if (debug) printf("data point  %d  =  %lf\n",j,data[j]);
	*/

	if (debug) printf("End of disp3Dmap\n");



	free(data);
	fclose(map);
	RETURN;
}

