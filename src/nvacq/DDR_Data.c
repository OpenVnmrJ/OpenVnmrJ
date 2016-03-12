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
//=========================================================================
// FILE: DDR_Data.c
//=========================================================================
//   I. functional responsibilities of module
//     1. contains functions to get data from C67
//=========================================================================
#include <stdlib.h>
#ifdef INSTRUMENT
#include "wvLib.h"          /* wvEvent() */
#endif
#include "DDR_Common.h"
#include "crc32.h"
#include <stdio.h>
#include <math.h>
#include "data.h"
#include "DDR_Globals.h"
#include "logMsgLib.h"
#include "DDR_Acq.h"
#include "dmaDrv.h"

MSG_Q_ID pTestDmaRdyMsgQ=0;  // for stand-alone tests

//#define DEBUG_DATA_XFER    // use Makefile

#define DATA_TSK_NAME "tDataTSK"
extern int ddrTask(char *name, void* task, int priority);
extern unsigned int ddr_mode(); // in DDR_Acq.c
extern void markTime(long long *dat);
extern double diffTime(long long m2,long long m1);

char *ddrfidname="fid";

int data_stats=0;

void swap2(char  *src)
{
	char b=src[1];
	src[1]=src[0];
	src[0]=b;
}

void swap4(char  *src)
{
	char b=src[3];
	src[3]=src[0];
	src[0]=b;
	b=src[2];
	src[2]=src[1];
	src[1]=b;
}

//=========================================================================
// checkData: test data checksum 
//=========================================================================
unsigned int checkData(float *data, int n, int dsize)
{
    unsigned int cs=0,i;
    double sum=0;
    int mode=ddr_mode(); 
    if(mode & MODE_NOCS)
        cs=0;
    else 
        cs=addbfcrcinc((char*)data,dsize*n,0);
    return cs;
}

//###################  vnmr data file io  #################################
//=========================================================================
// readFidFileHeader: read dfilehead header from a file 
//=========================================================================
int readFidFileHeader(FILE *fp,dfilehead *header)
{
    int nbytes=fread(header, sizeof(dfilehead), 1, fp);
    if (nbytes <=0)
        return 0;
    return nbytes;
}

//=========================================================================
// readFidBlockHeader: read dblockhead header from a file 
//=========================================================================
int readFidBlockHeader(FILE *fp,dblockhead *header)
{
    int nbytes=fread(header, sizeof(dblockhead), 1, fp);
    if (nbytes <=0)
        return 0;
    return nbytes;
}
  
//=========================================================================
// writeFidFileHeader: write dfilehead header to a file 
//=========================================================================
int writeFidFileHeader(FILE *fp,dfilehead *header)
{
    int nbytes=fwrite(header, sizeof(dfilehead), 1, fp);
    if (nbytes <=0)
        return 0;
    return nbytes;
}

//=========================================================================
// writeFidBlockHeader: write dblockhead header to a file 
//=========================================================================
int writeFidBlockHeader(FILE *fp,dblockhead *header)
{
    int nbytes=fwrite(header, sizeof(dblockhead), 1, fp);
    if (nbytes <=0)
        return 0;
    return nbytes;
}

//=========================================================================
// readFidData: read nmr data from a file 
//=========================================================================
int readFidData(FILE *fp,short *data, int size)
{

    int i;
    int word;
    int index=0;
    for(i=0;i<size;i++){
        if(fread(&word, 4, 1, fp)<1)
            return 0;
        data[index++]=word>>1;
        if(fread(&word, 4, 1, fp)<1)
            return 0; 
        data[index++]=word>>1;
    }
    return index;
}

//=========================================================================
// writeFidData: write nmr data to a file 
//=========================================================================
int writeFidData(FILE *fp,int *data, int size, int esize)
{
    int nbytes=fwrite(data, esize, size, fp);
    if (nbytes <=0)
        return 0;
    return nbytes;
}

//=========================================================================
// readFid: read a vnmr data file
//=========================================================================
short *readFid(char *fidname, int *points)
{
    FILE *fp;
    dfilehead dfh;
    dblockhead dbh;
    short *data=0;
    int nbytes=0;

    if ((fp = fopen(fidname, "rb")) == NULL){
        PRINT0("ERROR readFid: couldn't open file");
        return 0;
    }

    nbytes=readFidFileHeader(fp,&dfh);
    if (!nbytes){
        PRINT0("ERROR readFid: error reading file header");
        fclose(fp);
        return 0;
    }
    nbytes=readFidBlockHeader(fp,&dbh);
    if (!nbytes){
        PRINT0("ERROR readFid: error reading block header");
        fclose(fp);
        return 0;
    }
    // Write data to output file

    data=(short*) malloc (2*dfh.np*sizeof(short));
    
    if(!data){
        PRINT0("ERROR readFid: unable to malloc memory for data file");
        fclose(fp);
        return 0;
    }
    nbytes=readFidData(fp,data,dfh.np/2);

    if (nbytes!=dfh.np){
        PRINT2("ERROR readFid: read %d expected %d",nbytes,dfh.np);
        fclose(fp);
        return 0;
    }
        
    fclose(fp);
    *points=dfh.np;
    return data;
}


//=========================================================================
// writeFid: write a vnmr data file
//=========================================================================
int writeFid(char *fidname, int points, void *data, int dtype)
{
    FILE *fp;
    int nbytes=0;
    dfilehead dfh;
    dblockhead dbh;
    int esize;

    if((dtype==DATA_FLOAT)||(dtype==DATA_DOUBLE)){
        dfh.status=S_DATA|S_FLOAT|S_COMPLEX|FID_FILE;
        esize=4;
    }
    else{
        dfh.status=S_DATA|S_COMPLEX|FID_FILE; // packed short
        esize=2;
    }

    dfh.ebytes=esize;

    dfh.ntraces=1;
    dfh.nblocks=1;
    dfh.np=points*2;
    dfh.tbytes=dfh.np*esize;
    dfh.bbytes=dfh.tbytes+sizeof(dblockhead);
    dfh.vers_id=0;
    dfh.nbheaders=0;

    dbh.scale=0;
    dbh.index=1;
    dbh.mode=0;
    dbh.ctcount=1;
    dbh.lpval=0;
    dbh.rpval=0;
    dbh.lvl=0;
    dbh.tlt=0;
    dbh.status=dfh.status;

    if ((fp = fopen(fidname, "wb")) == NULL)
    {
        PRINT0("ERROR writeFid: couldn't open file");
        return 0;
    }
    
    nbytes=writeFidFileHeader(fp,&dfh);
    if (!nbytes){
        PRINT0("ERROR writeFid: error writing file header");
        fclose(fp);
        return 0;
    }

    nbytes=writeFidBlockHeader(fp,&dbh);
    if (!nbytes){
        PRINT0("ERROR writeFid: error writing block header");
        fclose(fp);
        return 0;
    }
 
    // Write data to output file

    nbytes=writeFidData(fp,(int*)data,points,2*esize);

    if (!nbytes){
        PRINT0("ERROR writeFid: error writing fid data");
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

//=========================================================================
// writeFid: write a raw data file (no header)
//=========================================================================
int writeData(char *fidname, int points, void *data, int dtype)
{
    int esize;
    FILE *fp;
    int nbytes=0;

    if ((fp = fopen(fidname, "wb")) == NULL){
        PRINT0("ERROR writeFid: couldn't open file");
        return 0;
    }

    if((dtype==DATA_FLOAT)||(dtype==DATA_DOUBLE))
        esize=4;
    else
        esize=2;
 
    // Write data to output file

    nbytes=writeFidData(fp,(int*)data,points,2*esize);

    if (!nbytes){
        PRINT0("ERROR writeFid: error writing data");
        fclose(fp);
        return 0;
    }
    fclose(fp);   
    return 1;
}

//###################  Nirvana DDR data io  ###############################

//=========================================================================
// setDDRFilename: set base name for fid files 
//=========================================================================
void setDDRFilename(char *fn)
{
    ddrfidname=fn;
}

//=========================================================================
// getDDRData copy data from C67 to 405 memory (programmed io) 
//=========================================================================
void getDDRData(int adrs, unsigned int pts, float *dest, int dtype)
{
    int i,n;
    float *fptr;    
    int stride=2;

    if((dtype==DATA_FLOAT)||(dtype==DATA_DOUBLE))
        stride=2;
    else
        stride=1;

    fptr=(float*)setHPIBase(1, adrs, 1);
  
    n=stride*pts;
    for (i=0; i<n; i++){
        dest[i] = fptr[i];
    }
}

//=========================================================================
// showDDRFloatData: screen dump data points 
//=========================================================================
void showDDRData(int adrs, unsigned int pts, int dtype)
{
    u32 *data;
    int i;
    int stride;
    
    if(dtype==DATA_SHORT)
        stride=1;
    else
        stride=2;

    data=(u32*)malloc (pts*stride*4); 
    if(!data){
        printf("showDDRData Error, unable to malloc memory\n");
        return;
    }
    if(stride==1)
        getDDRData(adrs,pts,(float*)data,DATA_SHORT);
    else
        getDDRData(adrs,pts,(float*)data,DATA_FLOAT);
    switch(dtype){
    case DATA_SHORT:
        for (i=0; i<pts; i++)
            printf("%-5d  0x%0.8X\n",i, data[i].i);
        break;
    case DATA_FLOAT:    
        for (i=0; i<2*pts; i+=2)
            printf("%-5d  %-5g %-5g\n",i, data[i].f,data[i+1].f);
        break;
    }    
    free(data);
}

//=========================================================================
// writeDDRFile: create a binary data file 
//=========================================================================
void writeDataFile(char *fn, unsigned int adrs, int pts, int dtype, int bigendian)
{
    float *data;
    int i;
    int bytes;
 
    int stride;
    
    if(dtype==DATA_SHORT)
        stride=1;
    else
        stride=2;

    bytes=pts*stride*4;
    data=(float*)malloc(bytes); 
       
    if(ddr_debug & DDR_DEBUG_DATA)
         PRINT3("writing raw data file adrs:0x%0.8X pts:%d type:%d",adrs,pts,dtype);

    if(!data){
        PRINT1("Error: unable to malloc data memory %d bytes",pts*stride*4);
        return;
    }
    if(stride==1){
        getDDRData(adrs,pts,(float*)data,DATA_SHORT);      
        if(!bigendian){
	        for(i=0;i<pts*4;i+=2){
	        	swap2(((char*)data)+i);
	        }
        }
    }
    else{    	
        getDDRData(adrs,pts,(float*)data,DATA_FLOAT);
        if(!bigendian){
        	for(i=0;i<pts*4;i+=4){
        		swap4(((char*)data)+i);
        	}
        }
    }
    writeData(fn, pts, data, dtype);  
    free(data);
}

//=========================================================================
// writeDDRFile: create a vnmr "fid" data file  
//=========================================================================
void writeDDRFile(char *fn, unsigned int adrs, int pts, int type)
{
    float *data;
    int dbytes;
    int i;
    
    type&=DATA_TYPE;
    if(type==DATA_SHORT)
        dbytes=4;
    else
        dbytes=8;
        
    if(ddr_debug & DDR_DEBUG_DATA)
         PRINT3("writing FID file adrs:0x%0.8X pts:%d type:%d",adrs,pts,type);

    data=(float*)malloc(pts*dbytes); 
    if(!data){
        PRINT1("Error: unable to malloc data memory %d bytes",pts*dbytes);
        return;
    }
    getDDRData(adrs,pts,data,type);     
    writeFid(fn, pts, data, type);  
    free(data);
}

//=========================================================================
// writeLastAcq: create a binary data file from data in last acq buffer
//=========================================================================
void writeLastAcqData(char *fn,int pts, int bigendian){
	writeDataFile(fn, 0x81000000, pts, DATA_SHORT, bigendian);
}

//=========================================================================
// writeLastAcq: create a binary data file from data in last acm buffer
//=========================================================================
void writeLastAcmData(char *fn,int pts, int bigendian){
	writeDataFile(fn, 0x90000000, pts, DATA_FLOAT, bigendian);
}

//=========================================================================
// writeLastAcq: create a .fid file from data in last acm buffer
//=========================================================================
void writeLastFid(char *fn,int pts){
	writeDDRFile(fn, 0x90000000, pts, DATA_FLOAT);
}

//-------------------------------------------------------------
// ddrShowDataStats() do statistics on data
//-------------------------------------------------------------
ddrShowDataStats(void *vdata, int dtype, int np)
{
    int i;
    float *fdata;
    short *sdata;
    double maxr=-1e6;
    double maxi=-1e6;
    double maxa=-1e6;
    double minr=1e6;
    double mini=1e6;
    double mina=1e6;
    double sumr=0;
    double sumi=0;
    double suma=0;
    double sumd=0;
    double sumdd=0;
    double ave=0;
    double R,I,A,delta,var;
    double maxvalue=(double)0x10000;
    
    fdata=(float*)vdata;
    sdata=(short*)vdata;
 
     for(i=0;i<2*np;i+=2){
        if((dtype==DATA_FLOAT)||(dtype==DATA_DOUBLE)){
            R=fdata[i];
            I=fdata[i+1];
        }
        else{
            R=sdata[i];
            I=sdata[i+1];
        }
        A=sqrt(R*R+I*I);
        sumr+=R;
        sumi+=I;
        suma+=A;
        delta=R-I;
        sumd+=delta-ave;
        sumdd+=delta*delta;
        maxr=R>maxr?R:maxr;
        minr=R<minr?R:minr;
        maxi=I>maxi?I:maxi;
        mini=I<mini?I:mini;
        maxa=A>maxa?A:maxa;
        mina=A<mina?A:mina;
    }
    ave=sumd/np;
    var=sumdd - np*ave*ave;
    PRINT3("Real max:%-2.6f min:%-2.6f ave:%g",maxr, minr, sumr/np);
    PRINT3("Imag max:%-2.6f min:%-2.6f ave:%g",maxi, mini, sumi/np);
    PRINT3("Ampl max:%-2.6f min:%-2.6f ave:%g",maxa, mina, suma/np);
    PRINT2("Channel imbalance %g +- %g ",ave,sqrt(var)/np);
}

//-------------------------------------------------------------
// ddr_data_task() emulates NDDS data xfer task
//-------------------------------------------------------------
void ddr_data_task()
{
    data_msg msg;
    int stat;
    float *data=0;
    tcrc cs=0;
    int mode;
    int dsize;
    int size; // number of ints
    int n;
    static long long m1;
    static long long m2;
    double dt;
    int rate;
    int stride=2;
    int dtype;
    int err;
    int cntdown;
    static int dmaChannel=-1;
    static double totaltm=0;
    
    FOREVER {
        stat=msgQReceive(pTestDataRdyMsgQ,(char*)&msg,sizeof(data_msg), WAIT_FOREVER);
        if(stat==ERROR){
             PRINT0("VXWORKS ERROR: msgQReceive in ddr_data_task");
             continue;
        }
        dtype=(ddr_mode()&DATA_PACK)?DATA_SHORT:DATA_FLOAT;
        if(ddr_debug & DDR_DEBUG_XFER){
            // - transfer data using programmed IO or DMA
            // - do checksum test
            // - measure and report transfer speed
            if(msg.src==0){
                totaltm=0;
            }
            if(dtype==DATA_SHORT){
                stride=1;
            	dsize=4;
            }
            else {
                stride=2;
            	dsize=8;
            }
            size=msg.np*dsize/4;
            err=0;
            data = (float*)malloc(msg.np*dsize); // complex data (float)
            if(!data){
                PRINT0("ERROR could not malloc memory for data buffer");
                continue;
            }
            markTime(&m1);  

            mode=ddr_mode(); 

            if(mode & MODE_DMA){ // use DMA
                if(dmaChannel ==  -1){
                    PRINT0("=====>>>  opening data DMA channel <<<========");
                    dmaFreeChannel(3);
                    dmaChannel = dmaGetChannel(3); // no device pacing with cnnl 0 or 1
                }
                if(dmaChannel == -1){
                    PRINT0("=====>>>  DMA channel not available <<<========");
                    err=1;
                }
                else{
                    dmaXfer(dmaChannel, MEMORY_TO_MEMORY, 
                        NO_SG_LIST,(UINT32) msg.adrs, (UINT32) data, size, NULL, pTestDmaRdyMsgQ);
                    msgQReceive(pTestDmaRdyMsgQ,(char*)&stat,4, WAIT_FOREVER); // wait for complete
                }
            }
            else{ // use programmed io
                getDDRData(msg.adrs, msg.np, data, dtype);
            }
            markTime(&m2);
            if(err){
                PRINT4("XFER FAILED src:%d adrs:0x%0.8X np:%d id:%d",
                   msg.src,msg.adrs,msg.np,msg.id);
            }
            else{
                dt=diffTime(m2,m1);
                totaltm+=dt;
                rate=(int)(stride*4*msg.np/dt);
                if(ddr_debug & DDR_DEBUG_DATA){
                    PRINT6("XFER TEST src:%d adrs:0x%0.8X np:%d id:%d tm:%d ms (%d MBs)",
                        msg.src,msg.adrs,msg.np,msg.id,(int)dt/1000,(int)rate);
                }        
                if(!(mode & MODE_NOCS)){
                    cs=addbfcrcinc((char*)data,dsize*msg.np,0);
                    if(msg.cs!=cs){
                        PRINT5("CHECKSUM ERROR src:%d adrs:0x%0.8X n:%d 405:0x%0.8X C67:0x%0.8X ",
                            msg.src,msg.adrs,msg.np,cs,msg.cs);
                    }
                }
            }
            n=ddr_nacms();
            if(msg.src==n-1){
                dt=totaltm/n;
                rate=(int)(stride*4*msg.np/dt);
                if(mode & MODE_DMA){
                    PRINT4("%d DMA xfers of %d cplx pts %d ms <%d MBs> ave",
                        msg.src+1,msg.np,(int)dt/1000,rate);
                }
                else{
                    PRINT4("%d PIO xfers of %d cplx pts %d ms <%d MBs> ave",
                        msg.src+1,msg.np,(int)dt/1000,rate);
                }
            }
            if(data_stats)
                ddrShowDataStats(data,dtype,msg.np);
            free(data);
       }
        else if(ddr_debug & DDR_DEBUG_DATA)
            PRINT4("DATA MSG TEST src:%d adrs:0x%0.8X np:%d id:%d",
                   msg.src,msg.adrs,msg.np,msg.id);
        ddr_unlock_acm(msg.src, 0);
        if(msg.status & ACM_LAST){
            PRINT0("last data set transferred: ddr ready for new exp");
            set_data_ready();
        }
    }
}
#define MAX_DATA_MSGS       1000
//=========================================================================
// initDDRData: initialize data capture thread
//=========================================================================
void initDDRData()
{
    if(ddr_run_mode==STAND_ALONE){ // stand-alone mode
        pTestDataRdyMsgQ=msgQCreate(MAX_DATA_MSGS,sizeof(data_msg),MSG_Q_FIFO);
        ddrTask(DATA_TSK_NAME, (void*)ddr_data_task, 110);
        pTestDmaRdyMsgQ=msgQCreate(2,sizeof(int),MSG_Q_FIFO);
     }
 }
