/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
    Rack Definitions must be specified in column (X) order last
    e.g. (from code_205.grk)
   [Microplate]
      [One Row]
         [Third Row]
            0.0  -13.5  6.0  79.5  119.3
            0.0   -4.5  6.0  79.5  119.3
            0.0    4.5  6.0  79.5  119.3
            0.0   13.5  6.0  79.5  119.3
         [Third Row]
         0.0  -35.9  [Third Row]
         0.0    0.0  [Third Row]
         0.0   35.9  [Third Row]
      [One Row] 
      -31.43  0.0  [One Row]
      -22.45  0.0  [One Row]
      -13.47  0.0  [One Row]
       -4.49  0.0  [One Row]
        4.49  0.0  [One Row]
       13.47  0.0  [One Row]
       22.45  0.0  [One Row]
       31.43  0.0  [One Row]
   [Microplate]

    columns X= -31.43,-22.45,-13.47,etc specified last.
    When rack determines # columns and rows it uses the 1st level to determine columns
     (thus they must be specified last) and rows are recursively calc going down levels
    e.g. [One Row] consistent of 3 entries of [Third Row] of 4 entries thus there are 12 rows.
    if they are NO reference then number of rows is consider 1.
     e.g.:
   	[Vials]
       		-40.84  0.0  12.0  11.0  117.3^M
       		-14.53  0.0  12.0  41.5   89.1^M
        	14.53  0.0  12.0  41.5   89.1^M
        	40.84  0.0  12.0  11.0  117.3^M
   	[Vials]

        4 columns 1 row

*/


/* --------------------- rackObj.c ----------------------------- */

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include  <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "errLogLib.h"
#include "rackObj.h"

/*
modification history
--------------------
2-28-97,gmb  created 

8-12-03, gmb major chang to grk reading runction to better use standard gilson grk files
	 not perfect but much better.
*/

#define M_PI 3.14159265358979323846

#ifdef NEW_TRAY_ROUTINES

BLOCK_ID findBlockReference(BLOCK_LIST *pTheBlockList, char* blockName);

#endif

/*
DESCRIPTION
/* ------------------- Rack Object Structure ------------------- */
RACKOBJ_ID rackCreate(char *filename)
{
  RACKOBJ_ID pRackObj;
  BLOCK_LIST  *blockList;

  /* ------- malloc space for STM Object --------- */
  if ( (pRackObj = (RACK_OBJ *) malloc( sizeof(RACK_OBJ)) ) == NULL )
  {
    errLogSysRet(ErrLogOp,debugInfo,"rackCreate: ");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pRackObj,0,sizeof(RACK_OBJ));

  pRackObj->pSID = "";

#ifdef NEW_TRAY_ROUTINES
  /* new struct */
  /* create block list where all blocks are recorded so we can find them when references by other 
    blocks */
  blockList = (BLOCK_LIST*) malloc( sizeof(BLOCK_LIST));
  memset((char*)blockList,0,sizeof(BLOCK_LIST));
  pRackObj->blockList = blockList;
  if (readtray4(pRackObj,filename) == -1)
  {
     free(pRackObj->blockList);   /* new */
     free(pRackObj);
     return(NULL);
  }
#else
  if (readtray3(pRackObj,filename) == -1)
  {
     free(pRackObj);
     return(NULL);
  }
#endif
  if (DebugLevel > 0)
     rackShow(pRackObj, 1);
  return(pRackObj);
}

readline(FILE *stream,char* textline,int *data,char *label)
{
    char workline[256];
    char eolch;
    char *chrptr,*bracket,*val;
    int len,digit;
    int stat;


    while ( ( chrptr = fgets(textline,256,stream) )  != NULL)
    {
      label[0]='\000';
      len = strlen(textline);
      if (len < 4)
         continue;
      memcpy(workline,textline,len+1);
      DPRINT1(2,"text: '%s' \n",workline);

      /* is there a Label  [label] */
      bracket = (char*) strstr(workline,"[");
      if (bracket != NULL)
      {
        /* Yes Get the Label */
        chrptr = strtok(bracket+1,"]");
	strcpy(label,chrptr);
        DPRINT1(2,"Label: '%s'\n",label);
        /*  If a Comment line then skip and get next line */
        if ( (strcmp(label,"REM") == 0) || (strcmp(label,"Rem") == 0) || (strcmp(label,"rem") == 0) )
        {
	 continue;
	}
      }
      /* is it a data line */
      memcpy(workline,textline,len+1);
      val = strtok(workline," ");
      if (val[0] == '-')
         digit = isdigit(val[1]);
      else
         digit = isdigit(val[0]);
      *data = (digit) ? 1 : 0;

      if (*data && strlen(label))
      {
	  bracket = (char*) strstr(textline,"[");
	  *bracket = '\000';
      }
      break;
     }
     if (chrptr != NULL)
       stat = 0;
     else
       stat = EOF;

    return(stat);
}

extractVals(char *textline, int *values)
{
   char workline[256];
   int len;
   char *cptr,*val;
   int ival;
   double dval;
   int numVals = 0;

   len = strlen(textline);
   memcpy(workline,textline,len+1);
   cptr = workline;
   while ( (val = strtok(cptr," ")) != NULL)
   {
      DPRINT1(2,"val: '%s'\n",val);
      dval = atof(val) * 100.00;
      dval = (dval > 0.0) ? (dval + 0.5) : (dval - 0.5); 
      ival = (int) dval;
      values[numVals] = ival;

      DPRINT1(2,"Ival: %d\n",ival);
      numVals++;
      cptr = NULL;
   }
   return(numVals);
}

rackCenter(RACKOBJ_ID pRackId, int X,int Y)
{
   int i;
   int rackNum;
   int column,row;
   int xmm,ymm;
    int XY[2];

   pRackId->rackCenter[0] = X;
   pRackId->rackCenter[1] = Y;
   return(0);
}

rackCentered(RACKOBJ_ID pRackId)
{
    if (pRackId == NULL)
      return(-1);
    return ((pRackId->rackCenter[0] != 0) && (pRackId->rackCenter[1] != 0));
}

#ifdef NEW_TRAY_ROUTINES

rackWellOrder(RACKOBJ_ID pRackId, int Order)
{
   int Zone,zones;

   if(pRackId == NULL)
      return(-1);

   /* if ( (Order < 0) || (Order > ORDERINGMAX) ) */
   if ( (Order != BACKLEFT2RIGHT) && (Order != LEFTBACK2FRONT) )
       return(-1);

   /* zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries; */
   zones = pRackId->numZoneInfo;
   for(Zone=1; Zone <= zones; Zone++)
   {
     rackZoneWellOrder(pRackId, Zone, Order);
   }
   return(0);
}

rackZoneWellOrder(RACKOBJ_ID pRackId, int Zone,int Order)
{
   int zones,zoneNum;
   ZONE_INFO_ID zoneInfo;

   if(pRackId == NULL)
      return(-1);

   /* if ( (Order < 0) || (Order > ORDERINGMAX) ) */
   if ( (Order != BACKLEFT2RIGHT) && (Order != LEFTBACK2FRONT) )
       return(-1);

   zones = pRackId->numZoneInfo;

   Zone--;
   if ( (Zone >= 0) && (Zone < zones) )
   {
     zoneInfo = &pRackId->zoneInfo[Zone];
     zoneInfo->Ordering = Order;
   }
   else
   {
     return(-1);
   }
}

rackGetX(RACKOBJ_ID pRackId, int Zone,int sample)
{
  GEO_LINE resultLine;
  rackGetXYLoc2(pRackId, Zone, sample, &resultLine);
  return(resultLine.xval);
}

rackGetY(RACKOBJ_ID pRackId, int Zone,int sample)
{
  GEO_LINE resultLine;
  rackGetXYLoc2(pRackId, Zone, sample, &resultLine);
  return(resultLine.yval);
}

/*******************************************************************
*
*
*   Vol(cm*cm*cm) =  Area(cm*cm) * Z(cm)
*   Vol(ml) = Area(10mm*10mm) * Z(10mm)
*   Vol(1000ul) = 100 * Area(mm*mm) * 10 * Z(mm)
*   1000 * Vol(ul) = 1000 * (Area(mm*mm) * Z(mm))
*   Z(mm) * Area(mm*mm) = Vol(ul)
*   Z(mm) = (Vol(ul) / Area(mm*mm)
*
*	
*     Z(mm) = Vol (ul) / (Pi * square(Diameter(mm)/2.0))
*
*/
double rackVol2ZTrail(RACKOBJ_ID pRackId, double Vol, int Zone, int sample)
{
   double Diam,Ztravel;
   extern double sqrt(double);

   GEO_LINE resultLine;

   rackGetXYLoc2(pRackId, Zone, sample, &resultLine);
   Diam = ((double) resultLine.diameter) / 100.0;

   Ztravel = Vol / (M_PI * pow(Diam /2.0,2.0));

   return(Ztravel);
}

/*******************************************************************
*
* Flow in ml/min, Dia in mm, return speed in mm/sec	
*
*/
double rackFlow2Zspeed(RACKOBJ_ID pRackId, double Flow, int Zone, int sample)
{
    double  mlz;

    mlz = rackVol2ZTrail(pRackId,1000.0,Zone,sample);  /* 1000 uL per ml */
    return((Flow/60.0) * mlz);
}

/*******************************************************************
*
*	
*     Top of Sample Tray 
*
*/
int rackSampTop(RACKOBJ_ID pRackId, int Zone, int sample)
{
   GEO_LINE resultLine;

   rackGetXYLoc2(pRackId, Zone, sample, &resultLine);

   return(resultLine.top/10);
}

/*******************************************************************
*
*	
*     Bottom of Sample Tray 
*
*/
int rackSampBottom(RACKOBJ_ID pRackId, int Zone, int sample)
{
   int bot;
   GEO_LINE resultLine;

   rackGetXYLoc2(pRackId, Zone, sample, &resultLine);

   bot = resultLine.bottom / 10;
   /* if bottom > 1000 mm then do not add safety margin of 1 mm */
   if (bot > 10000)
      bot -= 20000;
   else
       bot += 10;

   return(bot);
}


rackInvalidZone(RACKOBJ_ID pRackId, int zone)
{
   int zones;

   zones = pRackId->numZoneInfo;

   zone--;
   if ( (zone >= 0) && (zone < zones) )
   {
      return(0);
   }
   else
   {
      return(zones);
   }
}

rackInvalidSample(RACKOBJ_ID pRackId, int zone,int sample)
{
   int maxnum;
   ZONE_INFO_ID zoneInfo;

   zoneInfo = &pRackId->zoneInfo[zone-1];
   maxnum = zoneInfo->rows * zoneInfo->columns;

   if ((sample > 0) && (sample <= maxnum))
   {
     return(0);
   }
   else
   {
     return(maxnum);
   }
}

/***********************************************************
*
* read_rack - Read Rack definition file
* Information that must be retain:
*/

#define MAX_TEXT_LEN 256
readtray4(RACKOBJ_ID pRackId,char* filename)
{
    FILE *tray_file,*tmp_file;
    char textline[256];
    char workline[256];
    char label[256];
    int done;
    int dataline;
    int i, ref_num, ref_entries;
    int nVals, values[6];
    int line,axis,index;
    int curLabel,curRefNum,prevEntries;
    int ZoneCnt, curZone;
    int len,stat,rackNum;
    BLOCK_ID block;
    int XYLoc[2];
 
    GEO_LINE_ID resultLine;
    
    tray_file = fopen(filename,"r");
    if (tray_file == NULL)
    {
       errLogSysRet(ErrLogOp,debugInfo,"racktray: couldn't open; '%s':",filename);
       return(-1); 
    }

    line = 0;

    curLabel = 0;

    len = strlen(filename);
    for (i=len; i > 0; i--)
    {
       if (filename[i] == '/')
         break;
    }
    strcpy(pRackId->pRackIdStr,&(filename[i+1]));
    
    readline(tray_file,textline,&dataline,label); /* skip 1st line */

    stat = 0;
    while( stat != EOF)
    {
	/* create new geometry block */
	block = (BLOCK_OBJ*) malloc(sizeof(BLOCK_OBJ));
  	memset(block,0,sizeof(BLOCK_OBJ));
        /* put the block on the blockList */
        pRackId->blockList->geoblocks[pRackId->blockList->nBlocks++] = block;
	stat = readGeometryBlock(tray_file,block,pRackId->blockList);
    }

    /* get upper block where the geo lines are the zones of the tray */
    block = findBlockReference(pRackId->blockList,"RackZones");
    pRackId->ZoneBlock = block;
    genRowColumnInfo(pRackId);
    fclose(tray_file);
}

BLOCK_ID findBlockReference(BLOCK_LIST *pTheBlockList, char* blockName)
{
   int i;
   for (i=0; i < pTheBlockList->nBlocks; i++)
   {
       if (strcmp(pTheBlockList->geoblocks[i]->Label,blockName) == 0)
          return(pTheBlockList->geoblocks[i]);
   }
   return(NULL);
}

readGeometryBlock(FILE *tray_file, BLOCK_ID block, BLOCK_LIST *pTheBlockList)
{
   char textline[256],label[256];
   int dataline;
   int labelIndex;
   int nVals, values[6];
   int line,axis,index;
   int stat;
   BLOCK_ID  embeddedBlock;
   GEO_LINE_ID geoline;

   /* block = treeNode->blockId; */
   line = 0;
    while((stat = readline(tray_file,textline,&dataline,label)) != EOF)
    {
	if (!dataline)
	{
	     if (strlen(block->Label) < 2)
             {
	       strncpy(block->Label,label,79);
	     }
	     else if (strcmp(block->Label,label) != 0)
	     {
	       /* create new geometry block */
	       embeddedBlock = (BLOCK_OBJ*) malloc(sizeof(BLOCK_OBJ));
  	       memset(embeddedBlock,0,sizeof(BLOCK_OBJ));
               strncpy(embeddedBlock->Label,label,79);
	       
               /* put the block on the blockList */
               pTheBlockList->geoblocks[pTheBlockList->nBlocks++] = embeddedBlock;
	       readGeometryBlock(tray_file,embeddedBlock,pTheBlockList);
	     }
	     else
	     {
                int i, Xval, Yval;
		int Xconst,Yconst;
                /* calc row or columns, constant X values is a row, constant Y values a column */
		Xval =  block->geoLine[0]->xval;
		Yval =  block->geoLine[0]->yval;
		Xconst = Yconst = 1;
		for (i=1; i < block->nentries; i++)
                {
		   if (Xval != block->geoLine[i]->xval) /* same as 1st values */
			Xconst = 0;
		   if (Yval != block->geoLine[i]->yval)
			Yconst = 0;
	        }
		if ( (Xconst) && (!Yconst) )
		   block->rows = block->nentries;
		else
		   block->rows = 1;
		if ( (Yconst) && (!Xconst) )
		   block->columns = block->nentries;
		else
		   block->columns = 1;

		return;
	     }
	}
	else  /* its a data line */
	{
            geoline = block->geoLine[line] = (GEO_LINE_ID) malloc(sizeof(GEO_LINE)); 
            memset(geoline,0,sizeof(GEO_LINE));
	   /* is there a Label, if not then this must be the full Rack */
	   if (strlen(block->Label) < 2)
             {
	       sprintf(block->Label,"RackZones");
	     }
	   /* is there a reference */
	   if (strlen(label) != 0)
	   {
		strcpy(geoline->blockRef,label);
                geoline->geoBlockRef = findBlockReference(pTheBlockList,label);
	   }
           else
           {
                block->leafGeoLines = 1;   /* true is geoline are leafs (no references) */
           }

	   nVals = extractVals(textline, &values[0]);
	   for (axis = 0; axis < nVals; axis++)
           {
                switch(axis)
                {
		  case 0:
			geoline->xval = values[axis];
			break;
		  case 1:
			geoline->yval = values[axis];
			break;
		  case 2:
			if (geoline->geoBlockRef == NULL)
			   geoline->diameter = values[axis];
			else
			   geoline->zval = values[axis];  /* non leaf 3rd value is Z */
			break;
		  case 3:
			if (geoline->geoBlockRef == NULL)
			   geoline->bottom = values[axis];
			else
			   geoline->thetaAngle = values[axis];  /* non leaf 4rd value is Theta Angle */
			break;
		  case 4:
			geoline->top = values[axis]; 
			break;
		 default:
			DPRINT2(0,"6th and more values ignored: values[%d]=%d\n",axis+1,values[axis]);
              		break;
                }
	   }
	   block->nentries++;
	   line++;
	}
    }
    return(stat);
}

/* rackGetGeoInfo(RACKOBJ_ID pRackId, int Zone, int sample,GEO_LINE_ID  resultLine) */
rackGetXYLoc2(RACKOBJ_ID pRackId, int Zone, int sample,GEO_LINE_ID  resultLine)
{
   int i,zoneNum;
   int rackNum;
   int column,row,orgrow,orgcolumn;
   int xmm,ymm;
   int zones;
   char *pRefLab,*pLabel;
   int XYVals[2];
   int sampeLoc;
   int iterProduct;
   int adjsample;

   BLOCK_ID block;
   GEO_LINE_ID zoneLine,geoline;
   ZONE_INFO_ID zoneInfo;
   char *ref;

   /* select zone, which will be one of the branches of this tree node */
   if (Zone > pRackId->numZoneInfo)
      return;

    zoneInfo = &pRackId->zoneInfo[Zone-1];
    zoneLine = zoneInfo->zoneLine;
    memset(resultLine,0,sizeof(GEO_LINE));
   iterProduct = 1;
   adjsample = getAdjSampleNum(sample,zoneInfo->Ordering,zoneInfo->rows,zoneInfo->columns,zoneInfo->totalSamples);
   getSummedVals(zoneLine->geoBlockRef, adjsample, iterProduct, resultLine);
  /* printf("X; %d, Y: %d \n",resultLine->xval,resultLine->yval); */

  
 sumGeoValues(zoneLine,resultLine);

  xmm = resultLine->xval;
  ymm = resultLine->yval;

  xmm = ((xmm % 10) > 5) ? (xmm + 10)/10 : (xmm / 10);
  ymm = ((ymm % 10) > 5) ? (ymm + 10)/10 : (ymm / 10);

  resultLine->xval = xmm + pRackId->rackCenter[0];
  resultLine->yval = ymm + pRackId->rackCenter[1];

  if (resultLine->zval > 0)
  {
      resultLine->bottom += resultLine->zval;
      resultLine->top += resultLine->zval;
  }
}

getAdjSampleNum(int sample,int Ordering,int rows,int columns, int totalSamples)
{
   int adjsample,val;
   /* int totalSamples = rows * columns; */
   switch(Ordering)
   {
     case 0:
       /* start at one skip every 12 samples, 1,13, 35,... when next is past nsamples start at 2 */
       /* 2, 14, 26, ... 86;  3, 15, 27, ... 87; 4, 16, 28, ... 88, etc... */
       if (rows > 1)
       {
          val = ((sample-1) * rows) + 1;
          adjsample = val % totalSamples + val / totalSamples;   /* div by rows * columns */
       }
       else
       {
	  adjsample = ((sample-1) % totalSamples) + 1;
       }
       break;
     /* not working, not sure how to do this. */
#ifdef XXX
     case 1:
       /* start at 1, skip + or -12, when next is past nsample, start next high and go backwards */
        /* 1, 13, 25, ... 85; 86, 74, 62, ... 2; 3, 15, 27, 39, ... 87; 88, 76 etc.. */
        val = ((sample-1) * columns) + 1;
       adjsample = val % totalSamples + val / totalSamples;   /* div by rows * columns */
       break;
     case 2 - 16:
                break;
#endif           
     case 4:
        adjsample = sample;
        break;
 
     default:
       /* previous way that the 96 well plate was access                */
       /* ((sample - 1) * rows) + 1                                     */
       /* calc initial value, sample 1 = 1-1 * rows + 1 or 1            */
       /*   sample 1 = (1-1) * 12 + 1 = 1,   0 * 12 + 1 = 1             */
       /*   sample 8 = (8-1) * 12 +1 = 85,   7 * 12 = 84, 86 + 1 = 85   */
       /*   sample 25 = (25-1) * 12 + 1 = 289                           */
       /* now the second part                                           */
       /*   289 % 96 + 289 / 96 = 4,  289 % 96 = 1, 289 / 96 = 3, 1+3 = 4 */
       if (rows > 1)
       {
          val = ((sample-1) * rows) + 1;
          adjsample = val % totalSamples + val / totalSamples;   /* div by rows * columns */
       }
       else
       {
	  adjsample = ((sample-1) % totalSamples) + 1;
       }
     break;      
   }             
   return( adjsample );
}                





sumGeoValues(GEO_LINE_ID source, GEO_LINE_ID result)
{
      result->xval += source->xval;
      result->yval += source->yval;
      result->diameter += source->diameter;
      result->bottom += source->bottom;
      result->top += source->top;
      result->zval += source->zval;
      result->thetaAngle += source->thetaAngle;
}
/* OK we can go depth 1st or level 1st */
getSummedVals(BLOCK_ID blockId, int sample, int iterProduct, GEO_LINE_ID ResultLine)
{

   int nBranches,branchIndex,i;
   int sampleHigh;
   int index;
   int nGeoLines;
   int max,min,line,result;
   int presentIterProd;

   GEO_LINE_ID geoline;

   nGeoLines = blockId->nentries;
   if ( nGeoLines > 0)
   {
      for (i=0; i < nGeoLines; i++)
      { 
          geoline = blockId->geoLine[i];
          if ((blockId->leafGeoLines == 0) && (geoline->geoBlockRef != NULL) )
          {
              max = iterProduct * blockId->nentries;
              min = max - blockId->nentries + 1;
              /* printf("iterProd: %d, max: %d, min: %d\n",*iterProduct,max,min); */
              presentIterProd = min + i;
              result = getSummedVals(geoline->geoBlockRef, sample, presentIterProd, ResultLine);
              if (result == 1)
              {
                sumGeoValues(geoline,ResultLine);
		return(result);
              }
          }
          else if (blockId->leafGeoLines == 1)
          {
              max = iterProduct * blockId->nentries;
              min = max - blockId->nentries + 1;
             /* printf("iterProd: %d, max: %d, min: %d\n",iterProduct,max,min);  */
              if ((sample <= max) && (sample >= min))
	      {
		 /* calc geoline */
                 /* line = min % (blockId->nentries + 1); /* value must be 0 - nentries */
                 line = sample - min;
                 sumGeoValues(blockId->geoLine[line],ResultLine);
                 return 1;
              }
              else
              {
		return 0;
              }
          }

      }
   } 
   return 0;
}

genRowColumnInfo(RACKOBJ_ID pRackId)
{
   BLOCK_ID block,zblock;
   int i,rows,columns,total;
   int rtc;
   block = pRackId->ZoneBlock;
   for(i=0; i < block->nentries; i++)
   {
      rows = columns = 1;
      total = 0;
      zblock = block->geoLine[i]->geoBlockRef;
      getRowsColumns(zblock, &rows, &columns);
      getTotalSamples(zblock, &total );
      rows = ( rows <= 0) ? 1 : rows;
      columns = ( columns <= 0) ? 1 : columns;
      rtc = rows * columns;
      if ( rtc != total )
      {
	 /* Ok, row * columns should equal total */
	 /* typically rows has been set properly
	    so if rows divides into total evenly then
	    we will say that's the number of columns
             (e.g. code_200.grk)
	    otherwise will make rows equal total
	     (e.g. code_211.grk )
            We only do this if one is equal to 1,
            if rows * columsn != total and rows & columns
	    are both greater than 1 then, Houston we have a problem.
         */
	 if ( (rows > 1) && (columns == 1) )
	 {
             if (total % rows)
		rows = total;
             else
		columns = total / rows;
         }
	 else if ( (columns > 1) && (rows == 1) )
	 {
             if (total % columns)
		columns = total;
             else
		rows = total / columns;
         }
   
      }
      pRackId->zoneInfo[i].rows = rows;
      pRackId->zoneInfo[i].columns = columns;
      pRackId->zoneInfo[i].totalSamples = total;
      pRackId->zoneInfo[i].zoneLine = block->geoLine[i];
      pRackId->numZoneInfo++;
   } 
}

getRowsColumns(BLOCK_ID block, int *rows, int *columns)
{
   BLOCK_ID blockref;
   /* based on 1st entry (i.e. geoLine[0]), assuming all lines ref same block */
   blockref = block->geoLine[0]->geoBlockRef; 
   if (blockref != NULL)
   {
       getRowsColumns(blockref,rows,columns);
   }
   if (block->leafGeoLines == 1)
   {
     *rows = block->rows;
     *columns = block->columns;
   }
   else
   {
    /* if *rows,*columns zero then don't mult by zero (wrong answer) just set to the values */
    /* also if the block row or columsn is zero then don't multiply either */
    if (*rows == 0)
       *rows = block->rows;
    else
       *rows = (block->rows > 0) ? ( *rows * block->rows) : *rows;

    if (*columns == 0)
       *columns = block->columns;
    else
       *columns = (block->columns > 0) ? ( *columns * block->columns) : *columns;

   }
   return 0;
}

getTotalSamples(BLOCK_ID block, int *total )
{
   BLOCK_ID blockref;
   int i,entries;
   entries =  block->nentries; 
   for ( i=0; i < entries; i++)
   {
       if (block->leafGeoLines != 1)
       {
         blockref = block->geoLine[i]->geoBlockRef; 
         if (blockref != NULL)
         {
            getTotalSamples(blockref,total);
         }
       }
       else
       {
         *total = *total + block->nentries;
	 break;
       }
   }
}


printTray(RACKOBJ_ID pRackObj)
{
 int nblocks,i,level;
 BLOCK_ID block;
 BLOCK_LIST *pTheBlockList = pRackObj->blockList;
 nblocks = pTheBlockList->nBlocks;
  
    block = findBlockReference(pTheBlockList,"RackZones");
    printBlockRef(block,0);
}

printBlockRef(BLOCK_ID block, int level)
{
     int i;
     BLOCK_ID blockref,prevBlock;
     printBlock(block,level);
     
     for(i=0; i < block->nentries; i++)
     {
        blockref = block->geoLine[i]->geoBlockRef;
        if (blockref != NULL)
        {
	   if ( blockref != prevBlock)
	   {
	       printBlockRef(blockref,level+4);
               prevBlock = blockref;
           }
        }
    }
}

printBlock(BLOCK_ID pBlock,int indent)
{
  char spaces[40];
  int i,k,axis;

  int value;
  GEO_LINE_ID geoline;

  for (i=0;i<indent;i++) spaces[i] = ' '; 
  spaces[i]='\0'; 

 diagPrint(0,"%s Block Label: '%s', Rows: %d, Columns: %d, %d entries\n",spaces,pBlock->Label,
		pBlock->rows, pBlock->columns,pBlock->nentries);

  if ( pBlock->nentries < 1)
	return;

  geoline = pBlock->geoLine[0];
  if (geoline->geoBlockRef == NULL)
     diagPrint(0,"%s        X         Y     Diameter  Bottom     Top (units 12230 = 122.3 mm)\n",spaces);
  else
     diagPrint(0,"%s        X         Y         Z    Theta Angle\n",spaces);


  for (i=0;i<pBlock->nentries; i++)
  {
     diagPrint(0,"%s   ",spaces);

         geoline = pBlock->geoLine[i];
         for (axis=0; axis < 5; axis++)
         {
                switch(axis)
                {
                  case 0:
                        value = geoline->xval;
                        break;
                  case 1:
                        value = geoline->yval;
                        break;
                  case 2:
                        if (geoline->geoBlockRef == NULL)
                           value = geoline->diameter;
                        else
                           value = geoline->zval;  /* non leaf 3rd value is Z */
			break;
                  case 3:
                        if (geoline->geoBlockRef == NULL)
                           value = geoline->bottom;
                        else
                           value = geoline->thetaAngle;  /* non leaf 4rd value is Theta Angle */
			break;
                  case 4:
                        value = geoline->top;
                        break;
                }
                diagPrint(0,"%6d    ", value);
          }
     if ( strlen(geoline->blockRef) > 2)
	diagPrint(0," Reference: '%s'",geoline->blockRef);
     diagPrint(0,"\n");
  }
}

#else   // Original routines

/***********************************************************
*
* read_rack - Read Rack definition file
* Information that must be retain:
*/

#define MAX_TEXT_LEN 256
readtray3(RACKOBJ_ID pRackId,char* filename)
{
    FILE *tray_file,*tmp_file;
    char textline[256];
    char workline[256];
    char label[256];
    int done;
    int dataline;
    int i, ref_num, ref_entries;
    int nVals, values[6];
    int line,axis,index;
    int curLabel,curRefNum,prevEntries;
    int ZoneCnt, curZone;
    int len,stat,rackNum;
    
    tray_file = fopen(filename,"r");
    if (tray_file == NULL)
    {
       errLogSysRet(ErrLogOp,debugInfo,"racktray: couldn't open; '%s':",filename);
       return(-1); 
    }

    line = 0;

    curLabel = 0;

    len = strlen(filename);
    for (i=len; i > 0; i--)
    {
       if (filename[i] == '/')
         break;
    }
    strcpy(pRackId->pRackIdStr,&(filename[i+1]));
    
    readline(tray_file,textline,&dataline,label); /* skip 1st line */

    stat = 0;
    while( stat != EOF)
    {
        pRackId->numZones++;
	sprintf(label,"Zone%d",pRackId->numZones);
        curZone = pRackId->numZones - 1;
        pRackId->pZones[curZone] = (ZONE_OBJ *) calloc(1,sizeof(ZONE_OBJ));
        pRackId->pZones[curZone]->Ordering = NW_2_E;
	strncpy(pRackId->pZones[curZone]->ZoneIdStr,label,79);
	DPRINT2(1,"Num Zones(s): %d, %s \n",
		pRackId->numZones,pRackId->pZones[curZone]->ZoneIdStr);
	pRackId->pZones[curZone]->pLabelId = (LABEL_OBJ*) calloc(1,sizeof(LABEL_OBJ));
	stat = readLabel(pRackId->pZones[curZone]->pLabelId,tray_file,0);
    }

    for (i=0; i < pRackId->numZones; i++)
    {
	if (strcmp(pRackId->pZones[i]->pLabelId->Label,"Rack") == 0)
        {
	  rackNum = i;
	  break;
	}
    }
    for (i=0; i < rackNum; i++)
    {
	pRackId->pZones[i]->Columns = pRackId->pZones[i]->pLabelId->nentries;

        if (pRackId->pZones[i]->pLabelId->RefLabel == NULL)
	   /* pRackId->pZones[i]->Rows = pRackId->pZones[i]->pLabelId->nentries; */
	   pRackId->pZones[i]->Rows = 1;
        else
	   pRackId->pZones[i]->Rows = getRows(pRackId->pZones[i]->pLabelId->RefLabel);
    }

    fclose(tray_file);
}

readLabel(LABEL_OBJ *pLabel,FILE *tray_file, int indx )
{
   char textline[256],label[256];
   int dataline;
   int labelIndex;
   int nVals, values[6];
   int line,axis,index;
   int stat;

   line = 0;
    while((stat = readline(tray_file,textline,&dataline,label)) != EOF)
    {
	if (!dataline)
	{
	     if (strlen(pLabel->Label) < 2)
             {
	       strncpy(pLabel->Label,label,79);
	     }
	     else if (strcmp(pLabel->Label,label) != 0)
	     {
	       pLabel->RefLabel = (LABEL_OBJ*) calloc(1,sizeof(LABEL_OBJ));
	       strncpy(pLabel->RefLabel->Label,label,79);
	       readLabel(pLabel->RefLabel,tray_file,indx );
	     }
	     else
	     {
		return;
	     }
	}
	else  /* its a data line */
	{

	   /* is there a Label, if not then this must be the full Rack */
	   if (strlen(pLabel->Label) < 2)
             {
	       strncpy(pLabel->Label,"Rack",79);
	     }
	   /* is there a reference */
	   if (strlen(label) != 0)
	   {
		strcpy(&pLabel->RefLab[line][0],label);
	   }

	   nVals = extractVals(textline, &values[0]);
	   for (axis = 0; axis < nVals; axis++)
           {
	      pLabel->xyloc[line][axis] = values[axis];
	   }
	    pLabel->nentries++;
	   line++;
	}
    }
    return(stat);
}

getRows(LABEL_OBJ* pLabelId)
{
   int rows = 0;
   if (pLabelId->RefLabel != NULL)
   {
     rows =  getRows(pLabelId->RefLabel);
     rows = rows * pLabelId->nentries;
   }
   else
     rows = pLabelId->nentries;

   return(rows);
}

rackWellOrder(RACKOBJ_ID pRackId, int Order)
{
   int Zone,zones;

   if(pRackId == NULL)
      return(-1);

   if ( (Order < 0) || (Order > ORDERINGMAX) )
       return(-1);

   zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries;
   for(Zone=1; Zone <= zones; Zone++)
   {
     rackZoneWellOrder(pRackId, Zone, Order);
   }
   return(0);
}

rackZoneWellOrder(RACKOBJ_ID pRackId, int Zone,int Order)
{
   int i,zones,zoneNum;
   char *pRefLab,*pLabel;

   LABEL_OBJ* pZonesTrays;

   if(pRackId == NULL)
      return(-1);

   if ( (Order < 0) || (Order > ORDERINGMAX) )
       return(-1);

   zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries;

   Zone--;
   if ( (Zone >= 0) && (Zone < zones) )
   {
     pZonesTrays = pRackId->pZones[pRackId->numZones-1]->pLabelId; 
     for (i=0; i < pRackId->numZones; i++)
     {
	pRefLab = &(pZonesTrays->RefLab[Zone][0]);
        pLabel = pRackId->pZones[i]->pLabelId->Label;
	if (strcmp(pRefLab,pLabel) == 0)
        {
	   zoneNum = i;
           break;
        }
     }
     pRackId->pZones[zoneNum]->Ordering = Order;
  }
  else
  {
    return(-1);
  }
}

rackGetX(RACKOBJ_ID pRackId, int Zone,int sample)
{
  int XY[2];
  rackGetXYLoc(pRackId, Zone,sample,XY);
    return(XY[0]);
}

rackGetY(RACKOBJ_ID pRackId, int Zone,int sample)
{
  int XY[2];
  rackGetXYLoc(pRackId, Zone,sample,XY);
    return(XY[1]);
}

/**************************************************************************
*  
* rackGetColRow - Get the COlumn and Row of Sample for a Zone
*
*   Returns:
*	Zone Reference Index
*
*/
int rackGetColRow(RACKOBJ_ID pRackId, int Zone, int sample, int *Col, int *Row)
{
   int i,zoneNum;
   int rackNum;
   int column,row,orgrow,orgcolumn;
   int xmm,ymm;
   int zones;
   char *pRefLab,*pLabel;

   LABEL_OBJ* pZonesTrays;
   zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries;

   Zone--;
   if ( (Zone >= 0) && (Zone < zones) )
   {
     pZonesTrays = pRackId->pZones[pRackId->numZones-1]->pLabelId; 
     for (i=0; i < pRackId->numZones; i++)
     {
	pRefLab = &(pZonesTrays->RefLab[Zone][0]);
        pLabel = pRackId->pZones[i]->pLabelId->Label;
        DPRINT2(2,"rackGetColRow(): pRefLab: '%s', pLabel: '%s'\n",pRefLab,pLabel);
	if (strcmp(pRefLab,pLabel) == 0)
        {
	   zoneNum = i;
           break;
        }
     }
     switch ( pRackId->pZones[zoneNum]->Ordering )
     {
      case BACKLEFT2RIGHT:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKLEFT2RIGHTZIGZAG:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* odd rows go backwards */
	if ( row % 2)
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKRIGHT2LEFT:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKRIGHT2LEFTZIGZAG:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* even rows go forwards */
	if ( !(row % 2) )
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTBACK2FRONT:
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTBACK2FRONTZIGZAG:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* odd columns go backwards */
	if ( column % 2)
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTFRONT2BACK:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTFRONT2BACKZIGZAG:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* even columns go backwards */
	if ( !(column % 2) )
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;


      case FRONTLEFT2RIGHT:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTLEFT2RIGHTZIGZAG:
	orgrow = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - orgrow - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* odd rows go backwards */
	if ( orgrow % 2)
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTRIGHT2LEFT:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTRIGHT2LEFTZIGZAG:
	orgrow = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - orgrow - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* even rows go forwards */
	if ( !(orgrow % 2) )
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;



      case RIGHTBACK2FRONT:
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTBACK2FRONTZIGZAG:
	orgcolumn = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - orgcolumn - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* odd columns go backwards */
	if ( orgcolumn % 2)
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTFRONT2BACK:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTFRONT2BACKZIGZAG:
	orgcolumn = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - orgcolumn - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* even columns go backwards */
	if ( !(orgcolumn % 2) )
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      default:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
        /* printf("Column: %d, Row: %d\n",column,row); */
	break;
     }
     *Col = column;
     *Row = row;
     DPRINT3(2,"Columns: %d, Rows: %d, zoneIndex: %d\n",pRackId->pZones[zoneNum]->Columns,
		pRackId->pZones[zoneNum]->Rows,zoneNum);
     return(zoneNum);
   }
   else
     return(-1);
}

rackGetXYLoc(RACKOBJ_ID pRackId, int Zone, int sample,int *XY)
{
   int i,zoneNum;
   int rackNum;
   int column,row,orgrow,orgcolumn;
   int xmm,ymm;
   int zones;
   char *pRefLab,*pLabel;

   LABEL_OBJ* pZonesTrays;

   zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries;

   Zone--;
   if ( (Zone >= 0) && (Zone < zones) )
   {
     pZonesTrays = pRackId->pZones[pRackId->numZones-1]->pLabelId; 
     for (i=0; i < pRackId->numZones; i++)
     {
	pRefLab = &(pZonesTrays->RefLab[Zone][0]);
        pLabel = pRackId->pZones[i]->pLabelId->Label;
	if (strcmp(pRefLab,pLabel) == 0)
        {
	   zoneNum = i;
           break;
        }
     }
     switch ( pRackId->pZones[zoneNum]->Ordering )
     {
      case BACKLEFT2RIGHT:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKLEFT2RIGHTZIGZAG:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* odd rows go backwards */
	if ( row % 2)
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKRIGHT2LEFT:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case BACKRIGHT2LEFTZIGZAG:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* even rows go forwards */
	if ( !(row % 2) )
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTBACK2FRONT:
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTBACK2FRONTZIGZAG:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* odd columns go backwards */
	if ( column % 2)
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTFRONT2BACK:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case LEFTFRONT2BACKZIGZAG:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* even columns go backwards */
	if ( !(column % 2) )
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;


      case FRONTLEFT2RIGHT:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTLEFT2RIGHTZIGZAG:
	orgrow = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - orgrow - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* odd rows go backwards */
	if ( orgrow % 2)
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTRIGHT2LEFT:
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case FRONTRIGHT2LEFTZIGZAG:
	orgrow = (sample-1) / pRackId->pZones[zoneNum]->Columns;
	row = pRackId->pZones[zoneNum]->Rows - orgrow - 1;
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	/* even rows go forwards */
	if ( !(orgrow % 2) )
	   column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;



      case RIGHTBACK2FRONT:
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTBACK2FRONTZIGZAG:
	orgcolumn = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - orgcolumn - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* odd columns go backwards */
	if ( orgcolumn % 2)
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTFRONT2BACK:
	column = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - column - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;

      case RIGHTFRONT2BACKZIGZAG:
	orgcolumn = (sample-1) / pRackId->pZones[zoneNum]->Rows;
	column = pRackId->pZones[zoneNum]->Columns - orgcolumn - 1;
	row = (sample-1) % pRackId->pZones[zoneNum]->Rows;
	/* even columns go backwards */
	if ( !(orgcolumn % 2) )
	   row = pRackId->pZones[zoneNum]->Rows - row - 1;
        /* printf("Column: %d, Row: %d\n",column,row); */
		break;



      default:
	column = (sample-1) % pRackId->pZones[zoneNum]->Columns;
	row = (sample-1) / pRackId->pZones[zoneNum]->Columns;
        /* printf("Column: %d, Row: %d\n",column,row); */
	break;
     }
     xmm = pRackId->pZones[zoneNum]->pLabelId->xyloc[column][0];
     if ( pRackId->pZones[zoneNum]->pLabelId->RefLabel != NULL)
       ymm = getYmm(pRackId->pZones[zoneNum]->pLabelId->RefLabel,&row);
     else
       ymm = 0;
     ymm += pRackId->pZones[zoneNum]->pLabelId->xyloc[column][1];

     xmm = xmm + pRackId->pZones[pRackId->numZones-1]->pLabelId->xyloc[Zone][0];
     ymm = ymm + pRackId->pZones[pRackId->numZones-1]->pLabelId->xyloc[Zone][1];
     xmm = ((xmm % 10) > 5) ? (xmm + 10)/10 : (xmm / 10);
     ymm = ((ymm % 10) > 5) ? (ymm + 10)/10 : (ymm / 10);
     /* printf("Xmm: %ld, Ymm: %ld\n",xmm,ymm); */
  
     xmm = xmm + pRackId->rackCenter[0];
     ymm = ymm + pRackId->rackCenter[1];
     /* printf("Xmm: %ld, Ymm: %ld\n",xmm,ymm); */
     XY[0] = xmm;
     XY[1] = ymm;
   }
   return(0); 
}

getYmm(LABEL_OBJ* pLabelId,int *row)
{
  int ymm,xmm,mmIndex;

  if (pLabelId->RefLabel != NULL)
  {
     ymm = getYmm(pLabelId->RefLabel,row);
     mmIndex = *row % pLabelId->nentries;
     ymm += pLabelId->xyloc[mmIndex][1];
  }
  else
  {
    mmIndex = *row % pLabelId->nentries;
    ymm = pLabelId->xyloc[mmIndex][1];
    *row = *row / pLabelId->nentries;
  }
  return(ymm) ;
}

/*******************************************************************
*
*
*   Vol(cm*cm*cm) =  Area(cm*cm) * Z(cm)
*   Vol(ml) = Area(10mm*10mm) * Z(10mm)
*   Vol(1000ul) = 100 * Area(mm*mm) * 10 * Z(mm)
*   1000 * Vol(ul) = 1000 * (Area(mm*mm) * Z(mm))
*   Z(mm) * Area(mm*mm) = Vol(ul)
*   Z(mm) = (Vol(ul) / Area(mm*mm)
*
*	
*     Z(mm) = Vol (ul) / (Pi * square(Diameter(mm)/2.0))
*
*/
double rackVol2ZTrail(RACKOBJ_ID pRackId, double Vol, int Zone, int sample)
{
   int i,zoneIndex,mmIndex;
   int column,row;
   int diam,bot,top;
   double Diam,Ztravel;
   int ztravel;
   extern double sqrt(double);

   LABEL_OBJ* pZonesTrays;

   zoneIndex = rackGetColRow(pRackId, Zone, sample, &column, &row);



   if ( pRackId->pZones[zoneIndex]->pLabelId->RefLabel != NULL)
   {
      getSampDims(pRackId->pZones[zoneIndex]->pLabelId->RefLabel,&row,&diam,&bot,&top);
   }
   else
   {
      mmIndex = row % pRackId->pZones[zoneIndex]->pLabelId->nentries;
      diam = pRackId->pZones[zoneIndex]->pLabelId->xyloc[mmIndex][2];
   }

   Diam = ((double) diam) / 100.0;

   Ztravel = Vol / (M_PI * pow(Diam /2.0,2.0));

   return(Ztravel);
}

/*******************************************************************
*
* Flow in ml/min, Dia in mm, return speed in mm/sec	
*
*/
double rackFlow2Zspeed(RACKOBJ_ID pRackId, double Flow, int Zone, int sample)
{
    double  mlz;

    mlz = rackVol2ZTrail(pRackId,1000.0,Zone,sample);  /* 1000 uL per ml */
    return((Flow/60.0) * mlz);
}

/*******************************************************************
*
*	
*     Top of Sample Tray 
*
*/
int rackSampTop(RACKOBJ_ID pRackId, int Zone, int sample)
{
   int i,zoneIndex,mmIndex;
   int column,row;
   int diam,bot,top;

   LABEL_OBJ* pZonesTrays;

   zoneIndex = rackGetColRow(pRackId, Zone, sample, &column, &row);

   if ( pRackId->pZones[zoneIndex]->pLabelId->RefLabel != NULL)
      getSampDims(pRackId->pZones[zoneIndex]->pLabelId->RefLabel,&row,&diam,&bot,&top);
     else
     {
      mmIndex = row % pRackId->pZones[zoneIndex]->pLabelId->nentries;
      top = pRackId->pZones[zoneIndex]->pLabelId->xyloc[mmIndex][4];
     }

   return(top/10);
}

/*******************************************************************
*
*	
*     Bottom of Sample Tray 
*
*/
int rackSampBottom(RACKOBJ_ID pRackId, int Zone, int sample)
{
   int i,zoneIndex,mmIndex;
   int column,row;
   int diam,bot,top;

   LABEL_OBJ* pZonesTrays;

   zoneIndex = rackGetColRow(pRackId, Zone, sample, &column, &row);


   if ( pRackId->pZones[zoneIndex]->pLabelId->RefLabel != NULL)
      getSampDims(pRackId->pZones[zoneIndex]->pLabelId->RefLabel,&row,&diam,&bot,&top);
     else
     {
      mmIndex = row % pRackId->pZones[zoneIndex]->pLabelId->nentries;
      bot = pRackId->pZones[zoneIndex]->pLabelId->xyloc[mmIndex][3];
     }

   bot /= 10;
   /* if bottom > 1000 mm then do not add safety margin of 1 mm */
   if (bot > 10000)
      bot -= 20000;
   else
       bot += 10;

   return(bot);
}


/**************************************************************************
*
*  getSampDims - Obtains Sample Dimensions
*
*	Note: values need to be divided by 100 to obtain mm units
*
*/
getSampDims(LABEL_OBJ* pLabelId,int *row,int *Diameter, int *Bottom, int *Top)
{
  int ymm,xmm,mmIndex;

  if (pLabelId->RefLabel != NULL)
  {
     getSampDims(pLabelId->RefLabel,row,Diameter,Bottom,Top);
     mmIndex = *row % pLabelId->nentries;
     *Diameter = (pLabelId->xyloc[mmIndex][2] != 0) ? pLabelId->xyloc[mmIndex][2] : *Diameter;
     *Bottom = (pLabelId->xyloc[mmIndex][3] != 0) ? pLabelId->xyloc[mmIndex][3] : *Bottom;
     *Top = (pLabelId->xyloc[mmIndex][4] != 0) ? pLabelId->xyloc[mmIndex][4] : *Top;
     ymm += pLabelId->xyloc[mmIndex][1];
  }
  else
  {
    mmIndex = *row % pLabelId->nentries;
    *Diameter = pLabelId->xyloc[mmIndex][2];
    *Bottom = pLabelId->xyloc[mmIndex][3];
    *Top = pLabelId->xyloc[mmIndex][4];
    *row = *row / pLabelId->nentries;
  }
  return(0);
}

rackInvalidZone(RACKOBJ_ID pRackId, int zone)
{
   int zones;

   LABEL_OBJ* pZonesTrays;

   zones = pRackId->pZones[pRackId->numZones-1]->pLabelId->nentries;

   zone--;
   if ( (zone >= 0) && (zone < zones) )
   {
      return(0);
   }
   else
   {
      return(zones);
   }
}

rackInvalidSample(RACKOBJ_ID pRackId, int zone,int sample)
{
   int zoneIndex,column,row,maxnum;

   zoneIndex = rackGetColRow(pRackId, zone, sample, &column, &row);
   maxnum = pRackId->pZones[zoneIndex]->Columns * pRackId->pZones[zoneIndex]->Rows;
   if ((sample > 0) && (sample <= maxnum))
   {
     return(0);
   }
   else
   {
     return(maxnum);
   }
}

printEntries(LABEL_OBJ* pLabelId,int indent)
{
  char spaces[40];
  int i,k,axis;

  for (i=0;i<indent;i++) spaces[i] = ' '; 
  spaces[i]='\0'; 

  diagPrint(0,"%s Label: '%s', %d entries\n",spaces,pLabelId->Label,
		pLabelId->nentries);

  for (i=0;i<pLabelId->nentries; i++)
  {
     diagPrint(0,"%s   ",spaces);
     for (axis=0; axis < 5; axis++)
     {
	if ( (axis > 1) && (pLabelId->xyloc[i][axis] == 0))
	   continue;
        diagPrint(0,"%6d    ", pLabelId->xyloc[i][axis]);
     }
     if ( strlen(&pLabelId->RefLab[i][0]) > 2)
	diagPrint(0," Reference: '%s'",&pLabelId->RefLab[i][0]);
     diagPrint(0,"\n");
  }
  if (pLabelId->RefLabel != NULL)
     printEntries(pLabelId->RefLabel,indent + 4);
}


#endif  /* NEW_TRAY_ROUTINES & ORIGINAL ROUTINES */

rackShow(RACKOBJ_ID pRackId, int level)
{
   int i,k,axis;
   int entries;
   LABEL_OBJ* pLabelId;

   DPRINT2(0,"\n------------- rackShow Obj: 0x%p, level: %d   \n\n",
			pRackId,level);
   diagPrint(NULL,"RackObj: 0x%p\n",pRackId);
   diagPrint(NULL,"Rack Id: '%s'\n",pRackId->pRackIdStr);
   diagPrint(NULL,"SCCS ID: '%s'\n",pRackId->pSID);
   diagPrint(NULL,"Rack Center: X: %d, Y: %d \n",pRackId->rackCenter[0],
		pRackId->rackCenter[1]);
#ifdef NEW_TRAY_ROUTINES
   diagPrint(NULL,"\n");
   diagPrint(NULL,"Rack: '%s',   %d Zones.\n",pRackId->pRackIdStr,pRackId->numZoneInfo);
   diagPrint(NULL,"\n");
   for( i=0; i < pRackId->numZoneInfo; i++)
   {
      diagPrint(NULL," Zone %d, %d Rows, %d Columns, %d Total Samples\n",
		i+1, pRackId->zoneInfo[i].rows, pRackId->zoneInfo[i].columns,
		pRackId->zoneInfo[i].totalSamples);
   }
   diagPrint(NULL,"\n\n");
   printTray(pRackId);
#else
   pLabelId = pRackId->pZones[pRackId->numZones-1]->pLabelId;
   entries = pLabelId->nentries;
   diagPrint(NULL,"Rack Zones: %d \n",entries);
   for (i=0;i<entries; i++)
   {
     for (axis=0; axis < 5; axis++)
     {
	if ( (axis > 1) && (pLabelId->xyloc[i][axis] == 0))
	   continue;
        diagPrint(NULL,"%6d    ", pLabelId->xyloc[i][axis]);
     }
     if ( strlen(&pLabelId->RefLab[i][0]) > 2)
	diagPrint(NULL," Reference: '%s'",&pLabelId->RefLab[i][0]);
     diagPrint(NULL,"\n");
   }
	
   diagPrint(NULL,"\n %d Lables\n",pRackId->numZones-1);
   for (i=0;i<pRackId->numZones-1; i++)
   {
        pLabelId = pRackId->pZones[i]->pLabelId;
	diagPrint(NULL,"Columns: %d, Rows: %d\n",pRackId->pZones[i]->Columns,pRackId->pZones[i]->Rows);
	printEntries(pLabelId,4);
   }
#endif 
   diagPrint(NULL,"\n\n");
}


#ifdef XXX
#define ZONE1 1
#define ZONE2 2
#define ZONE3 3
main (int argc, char *argv[])
{
  int i,top,bot;
  int xAxis,yAxis;
  RACKOBJ_ID inject,rack,rack2,rack3;
  char buffer[256];
  double Ztravel,Zspeed;
  inject = rackCreate("/usr24/greg/projects/Gilson/racks/m215_inj.grk");
  rackCenter(inject, (545 + (1200 * 4) + 220 ),0);  /* 5345 */
  rackShow(inject, 1);
  top = rackSampTop(inject,ZONE1,1);
  bot = rackSampBottom(inject,ZONE1,1);

  rack = rackCreate("/usr24/greg/projects/Gilson/racks/code_205.grk");
  rackCenter(rack, 545,1870);
  rackShow(rack, 1);
  top = rackSampTop(rack,ZONE1,1);
  bot = rackSampBottom(rack,ZONE1,1);
/*
  rack2 = rackCreate("/usr24/greg/projects/Gilson/racks/code_204.grk");
  rackCenter(rack2, 1745,1870);
  rackShow(rack2, 1);
*/
/*
  rack3 = rackCreate("/usr24/greg/projects/Gilson/racks/code_209.grk");
  rackCenter(rack3, 545,1870);
  rackShow(rack3, 1);
*/
/*
  rackGetX(rack,  ZONE1,27);
  rackGetY(rack,  ZONE1,27);
  rackGetX(rack2,  ZONE1,5);
  rackGetY(rack2,  ZONE1,5);
  rackGetX(rack3,  ZONE1,5);
  rackGetY(rack3,  ZONE1,5);
*/
/*
  rackGetXYLoc(rack, "Microplate", 27);
  rackGetXYLoc(rack, "Microplate", 43);
  rackGetXYLoc(rack, "Microplate", 8);
*/
 
  Ztravel =  rackVol2ZTrail(rack, 40.0 /*ul*/, ZONE1, 1);
  /* Flow = ml/min */
  Zspeed = rackFlow2Zspeed(rack, 1.0 /*ml/min*/, ZONE1, 1);
  for(i=1;i<20;i++)
  {
      xAxis = rackGetX(rack3, ZONE1 ,i);
      yAxis = rackGetY(rack3, ZONE1, i);
      fprintf(stderr,"xAxis: %d, yAxis: %d\n",xAxis,yAxis);
  }
    
}
#endif

