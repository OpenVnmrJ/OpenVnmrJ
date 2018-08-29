/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/* This is free software: you can redistribute it and/or modify              */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* If not, see <http://www.gnu.org/licenses/>.                               */
/*---------------------------------------------------------------------------*/
/**/

#define MAXIMAGES 20000
#define MAXSL 10000

#include <dirent.h>


typedef struct {
  char name[MAXSTR];
  char type[MAXSTR];
  char string[MAXSTR];
  double value;
} svib_element;

typedef struct {
	int nfiles;
	char names[MAXSL][MAXSTR];
	char volname[MAXSTR];
} vlist;

typedef struct {
	int nvols;
	int ngrps;
	vlist *vlists;
	vlist *glists;
	vlist *misc;
} fileorg;


typedef struct {
  float  rank;
  char   spatial_rank[MAXSTR];
  char   storage[MAXSTR];
  float  bits;
  char   type[MAXSTR];
  char   abscissa[3][MAXSTR];
  char   ordinate[3][MAXSTR];
  char   nucleus[2][MAXSTR];
  float  nucfreq[2];
  char   file[MAXSTR];

  char   sequence[MAXSTR];
  char   studyid[MAXSTR];
  char   position1[MAXSTR];
  char   position2[MAXSTR];

  int    ro_size, pe_size, pe2_size;
  int    datasize;
  float  lro, lpe, lpe2;
  float  pro, ppe, ppe2;

  double pss_array[MAXIMAGES];
  float  pss;
  float  thk;
  float  gap;
  float  psi,phi,theta;

  int    slice_no,    slices;
  int    echo_no,     echoes;
  int    array_index, array_dim;
  int    display_order;

  float  te,tr,ti;
  float  dro,dpe,dsl,bvalue;

  float  Smax, Smin;  // primarily used in AVW header
  
  int    bigendian;
 
  svib_element sviblist[1024]; 
  int   nsvib;
} fdf_header;


void init_fdf_header(fdf_header *hdr){
  /* Initializes the fdf header from procpar */
  char   seqcon[MAXSTR],recon[MAXSTR];
  int    np, nv, nv2, ns;
  int    n_ft;
  int    nvals,n,nrefscans;
  double image[MAXIMAGES];

  /* 2D or 3D experiment? */
  getstr("seqcon",seqcon);
  hdr->rank = 2;
  if (seqcon[3] != 'n') hdr->rank = 3;

  if (hdr->rank == 2)
    strcpy(hdr->spatial_rank,"2dfov");
  else if (hdr->rank == 3)
    strcpy(hdr->spatial_rank,"3dfov");

  strcpy(hdr->storage,"float");
  hdr->bits = 32;
  strcpy(hdr->type,"absval");

  getstr("seqfil",hdr->sequence);
  getstr("studyid_",hdr->studyid);
  getstr("position1",hdr->position1);
  getstr("position2",hdr->position2);
  getstr("file",hdr->file);

  getstr("tn",hdr->nucleus[0]);
  getstr("dn",hdr->nucleus[1]);

  hdr->nucfreq[0]  = getval("sfrq");
  hdr->nucfreq[1]  = getval("dfrq");

  np  = (int) getval("np");
  nv  = (int) getval("nv");
  nv2 = (int) getval("nv2");

  /* nv=0 if, for example, whole EPI readout is in a single trace */
  /* If so then try parameters nread and nphase */
  if (nv == 0) {
    np = (int) getval("nread");
    nv = (int) getval("nphase");
  }

  hdr->ro_size  = np/2;
  hdr->pe_size  = nv;
  hdr->pe2_size = nv2;

  /* Internal or external recon? */
  strcpy(recon,"");
  if (getstatus("recon")) getstr("recon",recon);
  if (!strcmp(recon,"") || !strcmp(recon,"internal")) { 
    /* set dimensions to power of 2 */
    n_ft=1;
    while (n_ft<hdr->ro_size)
      n_ft*=2;
    hdr->ro_size=n_ft;

    n_ft=1;
    while (n_ft<hdr->pe_size)
      n_ft*=2;
    hdr->pe_size=n_ft;

    n_ft=1;
    while (n_ft<hdr->pe2_size)
      n_ft*=2;
    hdr->pe2_size=n_ft;
  }

  if (getstatus("fn"))  hdr->ro_size  = (int) getval("fn")/2;
  if (getstatus("fn1")) hdr->pe_size  = (int) getval("fn1")/2;
  if (getstatus("fn2")) hdr->pe2_size = (int) getval("fn2")/2;

  if (hdr->rank < 3) hdr->pe2_size = 1;
  
  hdr->datasize = hdr->ro_size*hdr->pe_size*hdr->pe2_size;

  strcpy(hdr->abscissa[0],"cm");
  strcpy(hdr->abscissa[1],"cm");
  strcpy(hdr->abscissa[2],"cm");
  strcpy(hdr->ordinate[0],"intensity");
  strcpy(hdr->ordinate[1],"intensity");
  strcpy(hdr->ordinate[2],"intensity");
  hdr->lro   = (float) getval("lro");
  hdr->lpe   = (float) getval("lpe");
  hdr->lpe2  = (float) getval("lpe2");
  hdr->pro   = (float) getval("pro");
  hdr->ppe   = (float) getval("ppe");
  hdr->ppe2  = (float) getval("ppe2");
  hdr->thk   = (float) getval("thk")/10;
  hdr->gap   = (float) getval("gap");
  hdr->psi   = (float) getval("psi");
  hdr->phi   = (float) getval("phi");
  hdr->theta = (float) getval("theta");

  ns = getarray("pss",hdr->pss_array);
  quicksort(hdr->pss_array,ns);
  hdr->pss = (float) hdr->pss_array[0];

  hdr->slice_no      = 1;
  hdr->slices        = ns;
  hdr->echo_no       = 1;
  hdr->echoes        = (int) getval("ne");
  hdr->array_index   = 1;
  hdr->array_dim     = (int) getval("arraydim");
  hdr->display_order = 0;

  // Adjust number of slices if seqcon = 's' in slice dim
  if (seqcon[1] == 's') {
    hdr->slices = ns;
    hdr->array_dim /= ns;
  }

  // Adjust arraydim if seqcon = 's' in PE or PE2
  if (seqcon[2] == 's') hdr->array_dim /= nv;
  if (seqcon[3] == 's') hdr->array_dim /= nv2;

  // Adjust arraydim if image exists and there are ref scans
  nvals = getarray("image",image);
  if (nvals > 0) {
    nrefscans = 0;
    for (n = 0; n < nvals; n++) {
      if (image[n] != 1) nrefscans++;
    }
    // What if image is not arrayed in parallel with other arrays????
    hdr->array_dim -= nrefscans;
  }

  hdr->te = (float) getval("te");
  hdr->tr = (float) getval("tr");
  hdr->ti = (float) getval("ti");

  hdr->dro    = (float) getval("dro");
  hdr->dpe    = (float) getval("dpe");
  hdr->dsl    = (float) getval("dsl");
  hdr->bvalue = (float) getval("bvalue");

  hdr->bigendian = 0;
  hdr->nsvib = 0;
}

void write_fdf_header(char *filename, fdf_header *hdr) {
  double cospsi, cosphi,costheta, sinpsi, sinphi, sintheta, deg2rad;
  double or0=0.0,or1=0.0,or2=0.0,or3=0.0,or4=0.0,or5=0.0,or6=0.0,or7=0.0,or8=0.0;
  double phi, psi, theta;
  int    i;
  FILE *fp;

  if ((fp = fopen(filename,"w")) == NULL) {
    fprintf(stderr,"Can't open file %s\n",filename);
    exit(0);
  }

  /* Calculate some cos & sin to the angles, used for orientation */
  deg2rad  = M_PI/180.0;
  psi      = hdr->psi;
  phi      = hdr->phi;
  theta    = hdr->theta;
  cospsi   = cos(deg2rad*psi);
  sinpsi   = sin(deg2rad*psi);
  cosphi   = cos(deg2rad*phi);
  sinphi   = sin(deg2rad*phi);
  costheta = cos(deg2rad*theta);
  sintheta = sin(deg2rad*theta);


  fprintf(fp,"#!/usr/local/fdf/startup\n");
  fprintf(fp,"float  rank = %.f;\n",hdr->rank);
  fprintf(fp,"char  *spatial_rank = \"%s\";\n",hdr->spatial_rank);
  fprintf(fp,"char  *storage = \"%s\";\n",hdr->storage);
  fprintf(fp,"float  bits = %.f;\n",hdr->bits);
  fprintf(fp,"char  *type = \"%s\";\n",hdr->type);
  fprintf(fp,"char  *nucleus[] = {\"%s\",\"%s\"};\n",hdr->nucleus[0],hdr->nucleus[1]);
  fprintf(fp,"float  nucfreq[] = {%f, %f};\n",hdr->nucfreq[0],hdr->nucfreq[1]);
  fprintf(fp,"char  *sequence = \"%s\";\n",hdr->sequence);
  fprintf(fp,"char  *studyid = \"%s\";\n",hdr->studyid);
  fprintf(fp,"char  *position1 = \"%s\";\n",hdr->position1);
  fprintf(fp,"char  *position2 = \"%s\";\n",hdr->position2);
  fprintf(fp,"char  *file = \"%s\";\n",hdr->file);

  fprintf(fp,"int    ro_size = %d;\n",hdr->ro_size);
  fprintf(fp,"int    pe_size = %d;\n",hdr->pe_size);

  if (hdr->rank == 2) {
    fprintf(fp,"float  matrix[] = {%d, %d};\n",hdr->pe_size,hdr->ro_size);
    fprintf(fp,"char  *abscissa[] = {\"%s\",\"%s\"};\n",hdr->abscissa[0],hdr->abscissa[1]);
    fprintf(fp,"char  *ordinate[] = {\"%s\",\"%s\"};\n",hdr->ordinate[0],hdr->ordinate[1]);
    fprintf(fp,"float  span[] = {%f, %f};\n",        hdr->lpe,hdr->lro);
    fprintf(fp,"float  roi[] = {%.6f,%.6f,%.6f};\n",hdr->lpe,hdr->lro,hdr->thk);
    fprintf(fp,"float  origin[] = {%f, %f};\n",          -hdr->ppe,hdr->pro);
    fprintf(fp,"float  location[] = {%.6f,%.6f,%.6f};\n",-hdr->ppe,hdr->pro,hdr->pss);
    fprintf(fp,"float  gap = %f;\n",hdr->gap);

    fprintf(fp,"int    slice_no = %d;\n",hdr->slice_no);
    fprintf(fp,"int    slices = %d;\n",hdr->slices);
    fprintf(fp,"int    echo_no = %d;\n",hdr->echo_no);
    fprintf(fp,"int    echoes = %d;\n",hdr->echoes);
    fprintf(fp,"int    array_index = %d;\n",hdr->array_index);
    fprintf(fp,"float  array_dim = %d;\n",hdr->array_dim);
    fprintf(fp,"int    display_order = %d;\n",hdr->display_order);

    fprintf(fp,"float  psi = %f;\n",hdr->psi);
    fprintf(fp,"float  phi = %f;\n",hdr->phi);
    fprintf(fp,"float  theta = %f;\n",hdr->theta);

    or0 = -1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or1 = -1*cosphi*sinpsi + sinphi*costheta*cospsi;
    or2 = -1*sinphi*sintheta;
    or3 = -1*sinphi*cospsi + cosphi*costheta*sinpsi;
    or4 = -1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or5 =    cosphi*sintheta;
    or6 = -1*sintheta*sinpsi;
    or7 =    sintheta*cospsi;
    or8 =    costheta;

  }
  else if (hdr->rank == 3) {
    fprintf(fp,"int    pe2_size = %d;\n",hdr->pe2_size);
    fprintf(fp,"float  matrix[] = {%d, %d, %d};\n",
      hdr->ro_size,hdr->pe_size,hdr->pe2_size);
    fprintf(fp,"char  *abscissa[] = {\"%s\",\"%s\",\"%s\"};\n",hdr->abscissa[0],hdr->abscissa[1],hdr->abscissa[2]);
    fprintf(fp,"char  *ordinate[] = {\"%s\",\"%s\",\"%s\"};\n",hdr->ordinate[0],hdr->ordinate[1],hdr->ordinate[2]);
    fprintf(fp,"float  span[] = {%f, %f, %f};\n",    hdr->lro,hdr->lpe,hdr->lpe2);
    fprintf(fp,"float  roi[] = {%.6f,%.6f,%.6f};\n",hdr->lro,hdr->lpe,hdr->lpe2);
    fprintf(fp,"float  origin[] = {%f, %f, %f};\n",
      -hdr->lro/2-hdr->pro,-hdr->lpe/2+hdr->pro,-hdr->lpe2/2+hdr->ppe2);
    fprintf(fp,"float  location[] = {%.6f,%.6f,%.6f};\n",-hdr->pro,hdr->ppe,hdr->ppe2);

    or0 = -1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or1 =    sinphi*cospsi - cosphi*costheta*sinpsi;
    or2 =    sintheta*cosphi;
    or3 =    cosphi*sinpsi - sinphi*costheta*cospsi;
    or4 = -1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or5 =    sintheta*sinphi;
    or6 =    cospsi*sintheta;
    or7 =    sinpsi*sintheta;
    or8 =    costheta;

  }

  fprintf(fp,"float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
    or0,or1,or2,or3,or4,or5,or6,or7,or8);

  fprintf(fp,"float  te = %f;\n",hdr->te);
  fprintf(fp,"float  TE = %f;\n",hdr->te*1000);
  fprintf(fp,"float  tr = %f;\n",hdr->tr);
  fprintf(fp,"float  TR = %f;\n",hdr->tr*1000);
  fprintf(fp,"float  ti = %f;\n",hdr->ti);
  fprintf(fp,"float  TI = %f;\n",hdr->ti*1000);

  for (i = 0; i < hdr->nsvib; i++) { 
    if (!strcmp(hdr->sviblist[i].type,"float")) {
      fprintf(fp,"float  %s = %f;\n",hdr->sviblist[i].name,hdr->sviblist[i].value);
    }
    else if (!strcmp(hdr->sviblist[i].type,"int")) {
      fprintf(fp,"int    %s = %d;\n",hdr->sviblist[i].name,(int)hdr->sviblist[i].value);
    }
    else if (!strcmp(hdr->sviblist[i].type,"char")) {
      fprintf(fp,"char  *%s = \"%s\";\n",hdr->sviblist[i].name,hdr->sviblist[i].string);
    }
  }


  fprintf(fp,"int bigendian = %d;\n",hdr->bigendian);
  fprintf(fp,"int checksum = 1291708713;\n");
  fclose(fp);
}


int read_fdf_data(char *filename, float *data, fdf_header *hdr) {
  FILE *fp;

  char  ch;
  int   n;
    
  if ((fp = fopen(filename,"r")) == NULL) {
    fprintf(stderr,"Can't open file %s for reading\n",filename);
    exit(0);
  }

  /* Locate null character */
  do {
    if (fscanf(fp, "%c", &ch) != 1) {
      fprintf(stdout,"Can't find checksum\n");
      exit(0);
    }
  } while (ch != '\0');



  /* Really should check type also */
  n = fread(data,sizeof(float),hdr->datasize,fp);
  fclose(fp);

  if (n != hdr->datasize) {
    printf("Problem reading enough data from %s, only %d of %d\n",filename, n,hdr->datasize);
    exit(0);
  }
  
  return(n);

}



int get_file_lists(const char *inputpath, fileorg *fo)
{
    int i,j;
	int nfiles;
	int iv, ig;
	int nvols, ngrps;
	char matchstr[MAXSTR];
	int sl;
	int *inlist;

	struct dirent **namelist;

	nfiles = scandir(inputpath, &namelist, 0, alphasort);
	if (nfiles < 0) {
		fprintf(stderr, "error using scandir \n");
		return(1);
	}

	// check for volumes that contain the same slice number
	nvols = 0;
	ngrps = 0;
	for (i = 0; i < nfiles; i++) {
		if (strstr(namelist[i]->d_name, "slice001") != NULL)
			nvols++;
		else if ((strstr(namelist[i]->d_name, "001.fdf") != NULL) && (strstr(namelist[i]->d_name, "slab") == NULL))
			ngrps++;
	}

	fo->nvols=nvols;
	fo->ngrps=ngrps;
	fo->vlists = malloc(nvols * sizeof(vlist));
	fo->glists = malloc(ngrps * sizeof(vlist));
	fo->misc = malloc(sizeof(vlist));
	inlist = malloc(nfiles * sizeof(int));

	for(i = 0; i< nfiles; i++)
		*(inlist+i)=0;

	iv=0;
	for (i = 0; i < nfiles; i++) {
		fo->vlists[iv].nfiles = 0;
		if (strstr(namelist[i]->d_name, "slice001") != NULL) {
			strcpy(matchstr, namelist[i]->d_name + 8);
			// make the name for the volume
			strcpy(fo->vlists[iv].volname,"vol_");
			sl=strlen(matchstr);
			sl=sl-4;  // not the extension
			strncat(fo->vlists[iv].volname,matchstr,sl);
			// find matches and save names
			for (j = 0; j < nfiles; j++) {
				if (strstr(namelist[j]->d_name, matchstr) != NULL){
					strcpy(fo->vlists[iv].names[fo->vlists[iv].nfiles++],
							namelist[j]->d_name);
					*(inlist+j)=1;
				}
			}
			iv++;
		}
	}

	ig=0;
	for (i = 0; i < nfiles; i++) {
		fo->glists[ig].nfiles = 0;
		if ((!*(inlist + i)) && strstr(namelist[i]->d_name, "001.fdf")&& (strstr(namelist[i]->d_name, "slab") == NULL)) {
			sl = strlen(namelist[i]->d_name);
			sl = sl - 7;
			strncpy(matchstr, namelist[i]->d_name, sl);
			// make the name for the volume
			strcpy(fo->glists[ig].volname, "vol_");
			strcat(fo->glists[ig].volname, matchstr);
			// find matches and save names
			for (j = 0; j < nfiles; j++) {
				if (strstr(namelist[j]->d_name, matchstr) != NULL) {
					strcpy(fo->glists[ig].names[fo->glists[ig].nfiles++],
							namelist[j]->d_name);
					*(inlist + j) = 1;
				}
			}
			ig++;
		}
	}

	// find any miscellaneous fdfs to convert one-by-one
	fo->misc->nfiles = 0;
	for (i = 0; i < nfiles; i++) {
		if((!*(inlist+i)) && strstr(namelist[i]->d_name,".fdf") ){
			strcpy(fo->misc->names[fo->misc->nfiles++],
					namelist[i]->d_name);
		}
	}


	(void)free(inlist);

	return(0);

}








int write_fdf(char *filename, float *data, fdf_header *hdr) {
  FILE *fp;
  int   n;
    
  write_fdf_header(filename,hdr);

  if ((fp = fopen(filename,"a")) == NULL) {
    fprintf(stderr,"Can't open file %s for writing\n",filename);
    exit(0);
  }


  /* Write null character */
  fprintf(fp,"\n%c",'\0');

  /* Really should check type */
  n = fwrite(data,sizeof(float),hdr->datasize,fp);
  fclose(fp);

  if (n != hdr->datasize) {
    printf("Problem writing enough data, only %d of %d\n",n,hdr->datasize);
    exit(0);
  }

  return(n);
}


void printf_fdf_header(fdf_header *hdr) {
  double cospsi, cosphi,costheta, sinpsi, sinphi, sintheta, deg2rad;
  double or0=0.0,or1=0.0,or2=0.0,or3=0.0,or4=0.0,or5=0.0,or6=0.0,or7=0.0,or8=0.0;
  double phi, psi, theta;

  /* Calculate some cos & sin to the angles, used for orientation */
  deg2rad  = M_PI/180.0;
  psi      = hdr->psi;
  phi      = hdr->phi;
  theta    = hdr->theta;
  cospsi   = cos(deg2rad*psi);
  sinpsi   = sin(deg2rad*psi);
  cosphi   = cos(deg2rad*phi);
  sinphi   = sin(deg2rad*phi);
  costheta = cos(deg2rad*theta);
  sintheta = sin(deg2rad*theta);

  printf("#!/usr/local/fdf/startup\n");
  printf("float  rank = %.f;\n",hdr->rank);
  printf("char  *spatial_rank = \"%s\";\n",hdr->spatial_rank);
  printf("char  *storage = \"%s\";\n",hdr->storage);
  printf("float  bits = %.f;\n",hdr->bits);
  printf("char  *type = \"%s\";\n",hdr->type);
  printf("char  *nucleus[] = {\"%s\",\"%s\"};\n",hdr->nucleus[0],hdr->nucleus[1]);
  printf("float  nucfreq[] = {%f, %f};\n",hdr->nucfreq[0],hdr->nucfreq[1]);
  printf("char  *sequence = \"%s\";\n",hdr->sequence);
  printf("char  *studyid = \"%s\";\n",hdr->studyid);
  printf("char  *position1 = \"%s\";\n",hdr->position1);
  printf("char  *position2 = \"%s\";\n",hdr->position2);
  printf("char  *file = \"%s\";\n",hdr->file);


  printf("int    ro_size = %d;\n",hdr->ro_size);
  printf("int    pe_size = %d;\n",hdr->pe_size);

  if (hdr->rank == 2) {
    printf("float  matrix[] = {%d, %d};\n",hdr->pe_size,hdr->ro_size);
    printf("char  *abscissa[] = {\"%s\",\"%s\"};\n",hdr->abscissa[0],hdr->abscissa[1]);
    printf("char  *ordinate[] = {\"%s\",\"%s\"};\n",hdr->ordinate[0],hdr->ordinate[1]);
    printf("float  span[] = {%f, %f};\n",        hdr->lpe,hdr->lro);
    printf("float  roi[] = {%.6f,%.6f,%.6f};\n",hdr->lpe,hdr->lro,hdr->thk);
    printf("float  origin[] = {%f, %f};\n",          -hdr->ppe,hdr->pro);
    printf("float  location[] = {%.6f,%.6f,%.6f};\n",-hdr->ppe,hdr->pro,hdr->pss);
    printf("float  gap = %f;\n",hdr->gap);

    printf("int    slice_no      = %d;\n",hdr->slice_no);
    printf("int    slices        = %d;\n",hdr->slices);
    printf("int    echo_no       = %d;\n",hdr->echo_no);
    printf("int    echoes        = %d;\n",hdr->echoes);
    printf("int    array_index   = %d;\n",hdr->array_index);
    printf("float  array_dim     = %d;\n",hdr->array_dim);
    printf("int    display_order = %d;\n",hdr->display_order);

    printf("float  psi   = %f;\n",hdr->psi);
    printf("float  phi   = %f;\n",hdr->phi);
    printf("float  theta = %f;\n",hdr->theta);

    or0 = -1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or1 = -1*cosphi*sinpsi + sinphi*costheta*cospsi;
    or2 = -1*sinphi*sintheta;
    or3 = -1*sinphi*cospsi + cosphi*costheta*sinpsi;
    or4 = -1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or5 =    cosphi*sintheta;
    or6 = -1*sintheta*sinpsi;
    or7 =    sintheta*cospsi;
    or8 =    costheta;

  }
  else if (hdr->rank == 3) {
    printf("int    pe2_size = %d;\n",hdr->pe2_size);
    printf("float  matrix[] = {%d, %d, %d};\n",
      hdr->ro_size,hdr->pe_size,hdr->pe2_size);
    printf("char  *abscissa[] = {\"%s\",\"%s\",\"%s\"};\n",hdr->abscissa[0],hdr->abscissa[1],hdr->abscissa[2]);
    printf("char  *ordinate[] = {\"%s\",\"%s\",\"%s\"};\n",hdr->ordinate[0],hdr->ordinate[1],hdr->ordinate[2]);
    printf("float  span[] = {%f, %f, %f};\n",    hdr->lro,hdr->lpe,hdr->lpe2);
    printf("float  roi[] = {%.6f,%.6f,%.6f};\n",hdr->lro,hdr->lpe,hdr->lpe2);
    printf("float  origin[] = {%f, %f, %f};\n",
      -hdr->lro/2-hdr->pro,-hdr->lpe/2+hdr->pro,-hdr->lpe2/2+hdr->ppe2);
    printf("float  location[] = {%.6f,%.6f,%.6f};\n",-hdr->pro,hdr->ppe,hdr->ppe2);

    or0 = -1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or1 =    sinphi*cospsi - cosphi*costheta*sinpsi;
    or2 =    sintheta*cosphi;
    or3 =    cosphi*sinpsi - sinphi*costheta*cospsi;
    or4 = -1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or5 =    sintheta*sinphi;
    or6 =    cospsi*sintheta;
    or7 =    sinpsi*sintheta;
    or8 =    costheta;

  }
  




  printf("float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
    or0,or1,or2,or3,or4,or5,or6,or7,or8);




  printf("float  te = %f;\n",hdr->te);
  printf("float  TE = %f;\n",hdr->te*1000);
  printf("float  tr = %f;\n",hdr->tr);
  printf("float  TR = %f;\n",hdr->tr*1000);
  printf("float  ti = %f;\n",hdr->ti);
  printf("float  TI = %f;\n",hdr->ti*1000);

/*
  for (i = 0; i < hdr->nsvib; i++) { 
    if (!strcmp(hdr->sviblist[i].type,"float")) {
      printf("float  %s = %f;\n",hdr->sviblist[i].name,hdr->sviblist[i].value);
    }
    else if (!strcmp(hdr->sviblist[i].type,"int")) {
      printf("int    %s = %d;\n",hdr->sviblist[i].name,(int)hdr->sviblist[i].value);
    }
    else if (!strcmp(hdr->sviblist[i].type,"char")) {
      printf("char  *%s = \"%s\";\n",hdr->sviblist[i].name,hdr->sviblist[i].string);
    }
  }
*/

  printf("int bigendian = %d;\n",hdr->bigendian);
  printf("int checksum = 1291708713;\n");
}
