/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* random.h - include function for generating random numbers, Eriks Kupce */
#ifndef __RANDOM_H
#define __RANDOM_H
#define MAXNI 524288     // maximum total number of increments
#define MBIG 1000000000
#define MSEED 161803398
#define FAC (1.0/MBIG)

double ran3(ix)
int *ix;
{
    static int inext, inextp, iff = 0;
    static long ma[56];
    long mj,mk;
    int i,ii,k;

    if (*ix < 0 || iff == 0)
    {
        iff=1;
        mj=MSEED-(*ix < 0 ? -*ix : *ix);
        mj %= MBIG;
        ma[55]=mj;
        mk=1;
        for (i=1;i<=54;i++)
        {
            ii=(21*i) % 55;
            ma[ii]=mk;
            mk=mj-mk;
            if (mk < 0) mk += MBIG;
            mj=ma[ii];
        }

        for (k=1;k<=4;k++)
        {
            for (i=1;i<=55;i++)
            {
                ma[i] -= ma[1+(i+30) % 55];
                if (ma[i] < 0) ma[i] += MBIG;
            }
        }
        inext=0;
        inextp=31;
        *ix=1;
    }

    if (++inext == 56) inext=1;
    if (++inextp == 56) inextp=1;
    mj=ma[inext]-ma[inextp];
    if (mj < 0) mj += MBIG;
    ma[inext]=mj;

    return mj*FAC;
}

#undef MBIG
#undef MSEED
#undef FAC

int insufficient_mem(celem, xnp)
int celem, xnp;
{
    unsigned long buf_size;

    buf_size = celem*(xnp*sizeof(float)+sizeof(BLOCKHEADER));
    if(buf_size > 0.8*available_RAM()) return 1;
    else return 0;
}

int get_nip(xnip, ix, xnp, ky)
int *xnip, ix, xnp, *ky;
{
    int i, j, k, ii;

    i=0;
    while(xnip[i] < ix) i++;

    if(xnip[i]>ix)
    {
        ii=i;
        j=ix-xnip[i-1];
        k=xnip[i]-ix;
        if(k>j)         // check which is closer
        ii = i-1;
        else if(k==j)
        {
            k=*ky;
            if(((int) (0.5 + (double) ran3(&k))) == 0)
            ii = i-1;
            *ky=k;
        }

        return ii;
    }

    j=i;
    while(xnip[j] <= ix) j++;

    ii=j-i-1;
    if(ii)
    {
        k=*ky;
        i += (int) (0.5 + (double) ii*ran3(&k));
        *ky=k;
    }

    return i;
}

char *mk_sch(nsp, xnp, ky)
int nsp, xnp, ky;
{
    char *arr;
    int i, j;
    double tm;

    printf("Creating a sequential sparse sampling schedule\n");

    arr = (char *) calloc(xnp, sizeof(char));
    for(i=0; i<xnp; i++) arr[i] = 0;

    i=1; arr[0]++;  // explicitly set the first sampling point
    while(i<nsp)
    {
        tm = 0.5 + (double) xnp*ran3(&ky);
        j = (int) tm;
        if((j>0) && (j<xnp) && (!arr[j])) arr[j]++, i++;
    }
    return arr;
}

int *mk_rsch(nsp, xnp, ky)
// return a randomized schedule
int nsp, xnp, ky;
{
    char *arr;
    int *sch, i, j;     // j and sch unsigned long int ??? need sch[][][]
    double tm;

    printf("Creating a randomized sparse sampling schedule.\n");

    arr = (char *) calloc(xnp, sizeof(char));
    sch = (int *) calloc(nsp, sizeof(int));
    for(i=0; i<xnp; i++) arr[i] = 0;

    i=1; arr[0]++; sch[0]=0;// explicitly set the first sampling point
    while(i<nsp)
    {
        tm = 0.5 + (double) xnp*ran3(&ky);
        j = (int) tm;
        if((j>0) && (j<xnp) && (arr[j] == 0)) arr[j]++, sch[i]=j, i++;
    }

    return sch;
}

char *mk_wsch(nip, ni2p, ni3p, nsp, nimax, ni2max, ni3max, ky)
int *nip, *ni2p, *ni3p, nsp, nimax, ni2max, ni3max, ky;
{
    char *arr;
    int i, j, k, ii, jj, kk, xx, xnp, kmax;   // max 4D
    double tm;

    printf("Creating weighted sequential sparse sampling schedule.\n");
    if((nimax==0) || (ni2max==0) || (ni3max==0)) vn_error("mk_wsch: code error.");

    xx = nimax*ni2max;
    xnp = xx*ni3max;
    kmax = 20*xnp;// maximum # of iterations to make sure a bad schedule is terminated

    arr = (char *) calloc(xnp, sizeof(char));
    for(i=0; i<xnp; i++) arr[i] = 0;

    k=0; i=1; arr[0]++;// explicitly set the first sampling point xxx
    while((i<nsp) && (k < kmax))
    {
        tm = 0.5 + (double) xnp*ran3(&ky);
        j = (int) tm;
        if(j<xnp)
        {
            ii = j%nimax;
            jj = (j/nimax)%ni2max;
            kk = j/xx;
            if(nip != NULL) ii = get_nip(nip, ii, nimax, &ky);
            if(ni2p != NULL) jj = get_nip(ni2p, jj, ni2max, &ky);
            if(ni3p != NULL) kk = get_nip(ni3p, kk, ni3max, &ky);
            j = kk*xx + jj*nimax + ii;
            if((j>0) && (j<xnp) && (!arr[j])) arr[j]++, i++;
        }
        k++;
    }
    printf("mk_wsch: expected:%d iterations:%d\n",nsp,k);
    return arr;
}

int *mk_wrsch(nip, ni2p, ni3p, nsp, nimax, ni2max, ni3max, ky)
// return a randomized schedule
int *nip, *ni2p, *ni3p, nsp, nimax, ni2max, ni3max, ky;
{
    char *arr;
    int *sch, i, j, k, ii, jj, kk, xx, xnp, kmax; // j and sch unsigned long int ??? need sch[][][]
    double tm;

    printf("Creating weighted randomized sparse sampling schedule.\n");
    if((nimax==0) || (ni2max==0) || (ni3max==0)) vn_error("mk_wsch: code error.");

    xx = nimax*ni2max;
    xnp = xx*ni3max;
    kmax = 20*xnp;// make sure a bad schedule is terminated

    arr = (char *) calloc(xnp, sizeof(char));
    sch = (int *) calloc(nsp, sizeof(int));
    for(i=0; i<xnp; i++) arr[i] = 0;

    k=0; i=1; arr[0]++; sch[0]=0;// explicitly set the first sampling point xxx
    while((i<nsp) && (k < kmax))
    {
        tm = 0.5 + (double) xnp*ran3(&ky);
        j = (int) tm;
        if(j<xnp)
        {
            ii = j%nimax;
            jj = (j/nimax)%ni2max;
            kk = j/xx;
            if(nip != NULL) ii = get_nip(nip, ii, nimax, &ky);
            if(ni2p != NULL) jj = get_nip(ni2p, jj, ni2max, &ky);
            if(ni3p != NULL) kk = get_nip(ni3p, kk, ni3max, &ky);
            j = kk*xx + jj*nimax + ii;
            if((j>0) && (j<xnp) && (!arr[j])) arr[j]++, sch[i]=j, i++;
        }
        k++;
    }

    return sch;
}

int write_sch(xfnm, sch, xnp, ni, ni2, ni3, nimax, ni2max, ni3max)
char *xfnm, *sch;
int xnp, ni, ni2, ni3, nimax, ni2max, ni3max;
{
    FILE *trg;
    int i, j, k;

    if((trg = fopen(xfnm, "w")) == NULL)
    vn_error("mkCSsch : problem opening the output file. ");

    j = nimax*ni2max;
    for(i=0, k=0; i<xnp; i++)
    {
        if(sch[i])
        {
            if(nimax>ni) fprintf(trg, "%4d ", i%nimax);
            if(ni2max>ni2) fprintf(trg, "%4d ", (i/nimax)%ni2max);
            if(ni3max>ni3) fprintf(trg, "%4d ", i/j);
            fprintf(trg, "\n"); k++;
        }
    }
    fclose(trg);

    return k;
}

int write_rsch(xfnm, sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max)
char *xfnm;
int *sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max;
{
    FILE *trg;
    int i, j, k;

    if((trg = fopen(xfnm, "w")) == NULL)
    vn_error("write_rsch : problem opening the output file. ");

    j = nimax*ni2max;
    for(i=0, k=0; i<nsp; i++)
    {
        if(nimax>ni) fprintf(trg, "%4d ", sch[i]%nimax);
        if(ni2max>ni2) fprintf(trg, "%4d ", (sch[i]/nimax)%ni2max);
        if(ni3max>ni3) fprintf(trg, "%4d ", sch[i]/j);
        fprintf(trg, "\n"); k++;
    }
    fclose(trg);

    return k;
}

void save_rsch(xfnm, sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max)
char *xfnm;
int *sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max;
{
    char fnm[MAXSTR];

    strcpy(fnm, xfnm);
    strcat(fnm,"/sampling.sch");
    if(write_rsch(fnm, sch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max) != nsp)
    vn_error("mkCSsch error: incomplete sampling schedule. ## 1\n");

    return;
}

int *get_pros(xname, nsp)
// sampling.pro file must exist in curexp
char xname[MAXSTR];
int nsp;
{
    FILE *src;
    int *pro, i;
    double tm, sum, scf;

    if(nsp < 2) return NULL;

    if((src = open_file(xname, "r")) == NULL)
    return NULL;

    i=0; sum=0.0;
    while(fscanf(src, "%lf", &tm) == 1) sum += tm, i++;  // count & integrate
    if(i != nsp) vn_abort(src, "mkCSsch error: inconsistent number of points in .pro file");
    if(sum == 0.0) vn_abort(src, "mkCSsch error: zero probabilities in .pro file. ");

    pro = calloc_i1d(nsp);

    fseek(src, 0, 0);
    i=0; scf = (double) (nsp-1)/sum; sum = 0.0;
    while((i<nsp) && (fscanf(src, "%lf", &tm)))// read, integrate and normalize
    pro[i++] = (int) (0.5 + (sum += scf*tm));
    fclose(src);

    return pro;
}

void make_sch(xfnm, ni, ni2, ni3, nimax, ni2max, ni3max, ky, ws)
char *xfnm;
int ni, ni2, ni3, nimax, ni2max, ni3max, ky, ws;
{
    char *sch, *ch, fnm[MAXSTR], fpro[MAXSTR];
    int *rsch, *nip=0, *ni2p=0, *ni3p=0, i, nsp, xnp;

    i=0; sch=NULL; rsch=NULL;
    nsp = ni*ni2*ni3;
    xnp = nimax*ni2max*ni3max;

    //printf("make_sch ni:%d ni2:%d ni3:%d nimax:%d ni2max:%d ni3max:%d\n",ni, ni2, ni3, nimax, ni2max, ni3max);

    if(xnp > MAXNI) vn_error("max # of points exceeds 524288. ");

    if(ws)// weighted schedule
    {
        strcpy(fnm, xfnm);
        if((ch = strrchr(fnm, '/')) != NULL)
        {
            *ch='\0';
            strcat(fnm,"/sampling_ni");
        }
        else
        strcpy(fnm, "sampling_ni");

        strcpy(fpro, fnm); strcat(fpro,".pro");
        if((nip = get_pros(fpro, nimax)) != NULL) i++;
        strcpy(fpro, fnm); strcat(fpro,"2.pro");
        if((ni2p = get_pros(fpro, ni2max)) != NULL) i++;
        strcpy(fpro, fnm); strcat(fpro,"3.pro");
        if((ni3p = get_pros(fpro, ni3max)) != NULL) i++;
        if(i)
        {
            if(ky < 0) // randomized schedule
            rsch = mk_wrsch(nip, ni2p, ni3p, nsp, nimax, ni2max, ni3max, -ky);
            else// sequential schedule
            sch = mk_wsch(nip, ni2p, ni3p, nsp, nimax, ni2max, ni3max, ky);
        }
        else
        vn_error("mkCSsch: sampling probability profile not found. \n");
    }
    else
    {
        if(ky < 0) rsch = mk_rsch(nsp, xnp, -ky);  // randomized schedule
        else sch = mk_sch(nsp, xnp, ky);// sequential schedule
    }

    if((ky<0) && (rsch != NULL))
    i = write_rsch(xfnm, rsch, nsp, ni, ni2, ni3, nimax, ni2max, ni3max);
    else if (sch != NULL)
    i = write_sch(xfnm, sch, xnp, ni, ni2, ni3, nimax, ni2max, ni3max);

    if(i < nsp) vn_error("mkCSsch error: incomplete sampling schedule. ## 2\n");

    return;
}

int *get_sch(xfnm, nsp, xni, xdim, xnb)
char *xfnm;
int *nsp, *xni, xdim, xnb;
{
    FILE *src;
    char cmd[MAXSTR];
    short s[4];
    int i, j, k, *sch;

    strcpy(cmd, xfnm);
    strcat(cmd,"/sampling.sch");
    if((src = fopen(cmd, "r")) == NULL)
    {
        printf("get_sch : problem opening the sampling schedule file\n %s \n", cmd);
        return NULL;
    }

    i=0; s[0] = 0; s[1] = 0; s[2] = 0;
    k = 0;
    while ( (k == 0) && fgets(cmd, sizeof(cmd), src) )
    {
       if (cmd[0] != '#')
          k = 1;
    }
    if (k == 1)
       k = sscanf(cmd, "%hd %hd %hd", &s[0], &s[1], &s[2]);
    if((k!=xdim) || (k < 1) || (k > 4))
    {
        printf("get_sch : failed to read the sampling schedule\n %s/sampling.sch \n", xfnm);
        return NULL;
    }

    sch = (int *) calloc(xnb, sizeof(int));

    i=0; sch[i] = 0;
    for(j=0; j<xdim; j++)
    sch[i] += (int) xni[j]*s[j];

    i = 1;
    while((fgets(cmd, sizeof(cmd), src)) && (i<xnb))
    {
      if (cmd[0] != '#')
      {
        j = sscanf(cmd, "%hd %hd %hd", &s[0], &s[1], &s[2]);
        if(j<xdim) break;
        sch[i] = 0;
        for(j=0; j<xdim; j++)
        sch[i] += (int) xni[j]*s[j];
        i++;
      }
    }
    fclose(src);

    if(i<xnb)
    printf("WARNING: schedule size %d is smaller than expected (%d)\n", i, xnb);
    *nsp = i;

    return sch;
}

int not_sequential(xsch, nsp)
int *xsch, nsp;
{
    int i;

    i = 1;
    while((xsch[i]>xsch[i-1]) && (i<nsp)) i++;
    return (nsp-i);
}

short ishs(f, jf, N)
// shell sort
int *f[], *jf[], N;
{
    int i, j, j0, k0, ix;

    if(N < 2) return 0;

    i=1;
    while((i<N) &&((*f)[i]>=(*f)[i-1])) i++;
    if(i==N) return i;

    ix=1;
    do
    {
        ix *=3;
        ix++;
    }while (ix < N);

    do
    {
        ix /= 3;
        for(i=ix+1; i<N; i++)
        {
            k0=(*f)[i]; j0=(*jf)[i];
            j=i;
            while((*f)[j-ix] > k0)
            {
                (*f)[j]=(*f)[j-ix]; (*jf)[j]=(*jf)[j-ix];
                j-=ix;
                if(j<ix) break;
            }
            (*f)[j]=k0; (*jf)[j]=j0;
        }
    }
    while (ix>0); /* ??? */

    i=1;
    while((i<N) &&((*f)[i]>=(*f)[i-1])) i++;

    if(i==N)
    return 1;
    else
    return 0;
}

int *sort_me_out(xsch, nsp)
int *xsch[], nsp;
{
    int *seq, i;

    seq = (int *) calloc(nsp, sizeof(int));
    for(i=0; i<nsp; i++) seq[i] = i;

    if(!ishs(xsch, &seq, nsp)) i = ishs(xsch, &seq, nsp);
    if(!i) return NULL;
    else return seq;
}
#endif

