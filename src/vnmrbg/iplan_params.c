/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// This file contains code to handle parameters for imaging planning.
//
// setPlanValue(char* paramName, char *value) to set plan parameter
// sendPlanParams(prescription* plans) to update parameters based on plan overlays
//
// Parameter names for different plan types are defined in file 
// templates/vnmrj/choicefiles/planParams. This file is searched based
// on "appdir" order. If this file does not exist, default parameter
// names will be used (filled by setDefaultTags). 
// The parameter sets are maintained in memory as paramTags, i.e.,
// an arrayed struct of planParams
// 
// The parameter set is uniquely identified by planType.
// There can be multiple parameter set (parameter tag) for the same overlayType. 
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vnmrsys.h"
#include "pvars.h"
#include "variables.h"
#include "group.h"
#include "iplan.h"
#include "wjunk.h"

#define  R_OK   4

extern int getIplanType();
extern int getCubeFOV();
extern int majorOrient(float theta, float psi, float phi);
extern int getActStackInd();

void createPlanParams(int planType);
void updateParams(planParams *tag, int ind);
void getArrayStr(int planType, int nstacks, int nslices, int is3P, char *arrayStr);
void setArray(char *arrayStr);
static planParams *paramTags = NULL;
static int ntags = 0; // number of tags (parameter sets)

// call this only if planParams file does not exist.
// set tags for five plan types: REGULAR, CSI2D, VOLUME, VOXEL, SATBAND
void setDefaultTags()
{
  int i;
  ntags = 5;
  if(paramTags == NULL) paramTags = (planParams *)malloc(sizeof(planParams)*ntags);  
  else paramTags = (planParams *)realloc(paramTags, sizeof(planParams)*ntags);
  
  // REGULAR
  strcpy(paramTags[0].planName,"slices");
  paramTags[0].planType = REGULAR; 
  paramTags[0].planColor = CYAN; 
  paramTags[0].overlayType = REGULAR; 
  strcpy(paramTags[0].orient.name, "orient");
  strcpy(paramTags[0].theta.name, "theta");
  strcpy(paramTags[0].psi.name, "psi");
  strcpy(paramTags[0].phi.name, "phi");
  strcpy(paramTags[0].dim1.name, "lpe");
  strcpy(paramTags[0].dim2.name, "lro");
  strcpy(paramTags[0].dim3.name, "lpe2");
  strcpy(paramTags[0].pos1.name, "ppe");
  strcpy(paramTags[0].pos2.name, "pro");
  strcpy(paramTags[0].pos3.name, "pss0");
  strcpy(paramTags[0].pss.name, "pss");
  strcpy(paramTags[0].thk.name, "thk");
  strcpy(paramTags[0].gap.name, "gap");
  strcpy(paramTags[0].ns.name, "ns");
  // CSI2D
  strcpy(paramTags[1].planName,"CSI2D");
  paramTags[0].planType = CSI2D; 
  paramTags[0].planColor = CYAN; 
  paramTags[1].overlayType = REGULAR; 
  strcpy(paramTags[1].orient.name, "orient");
  strcpy(paramTags[1].theta.name, "theta");
  strcpy(paramTags[1].psi.name, "psi");
  strcpy(paramTags[1].phi.name, "phi");
  strcpy(paramTags[1].dim1.name, "lpe");
  strcpy(paramTags[1].dim2.name, "lpe2");
  strcpy(paramTags[1].dim3.name, "thk");
  strcpy(paramTags[1].pos1.name, "ppe");
  strcpy(paramTags[1].pos2.name, "ppe2");
  strcpy(paramTags[1].pos3.name, "pss0");
  strcpy(paramTags[1].pss.name, "pss");
  strcpy(paramTags[1].thk.name, "thk");
  strcpy(paramTags[1].gap.name, "gap");
  strcpy(paramTags[1].ns.name, "ns");
  // VOLUME
  strcpy(paramTags[2].planName,"volume");
  paramTags[0].planType = VOLUME; 
  paramTags[0].planColor = ORANGE; 
  paramTags[2].overlayType = VOLUME; 
  strcpy(paramTags[2].orient.name, "orient");
  strcpy(paramTags[2].theta.name, "theta");
  strcpy(paramTags[2].psi.name, "psi");
  strcpy(paramTags[2].phi.name, "phi");
  strcpy(paramTags[2].dim1.name, "lpe");
  strcpy(paramTags[2].dim2.name, "lro");
  strcpy(paramTags[2].dim3.name, "lpe2");
  strcpy(paramTags[2].pos1.name, "ppe");
  strcpy(paramTags[2].pos2.name, "pro");
  strcpy(paramTags[2].pos3.name, "ppe2");
  strcpy(paramTags[2].pss.name, "pss0");
  strcpy(paramTags[2].thk.name, "thk");
  strcpy(paramTags[2].gap.name, "");
  strcpy(paramTags[2].ns.name, "");
  // VOXEL
  strcpy(paramTags[3].planName,"voxel");
  paramTags[0].planType = VOXEL; 
  paramTags[0].planColor = GREEN; 
  paramTags[3].overlayType = VOXEL; 
  strcpy(paramTags[3].orient.name, "vorient");
  strcpy(paramTags[3].theta.name, "vtheta");
  strcpy(paramTags[3].psi.name, "vpsi");
  strcpy(paramTags[3].phi.name, "vphi");
  strcpy(paramTags[3].dim1.name, "vox1");
  strcpy(paramTags[3].dim2.name, "vox2");
  strcpy(paramTags[3].dim3.name, "vox3");
  strcpy(paramTags[3].pos1.name, "pos1");
  strcpy(paramTags[3].pos2.name, "pos2");
  strcpy(paramTags[3].pos3.name, "pos3");
  strcpy(paramTags[3].pss.name, "");
  strcpy(paramTags[3].thk.name, "");
  strcpy(paramTags[3].gap.name, "");
  strcpy(paramTags[3].ns.name, "");
  // SATBAND 
  strcpy(paramTags[4].planName,"satband");
  paramTags[0].planType = SATBAND; 
  paramTags[0].planColor = WHITE; 
  paramTags[4].overlayType = SATBAND; 
  strcpy(paramTags[4].orient.name, "sorient");
  strcpy(paramTags[4].theta.name, "stheta");
  strcpy(paramTags[4].psi.name, "spsi");
  strcpy(paramTags[4].phi.name, "sphi");
  strcpy(paramTags[4].dim1.name, "");
  strcpy(paramTags[4].dim2.name, "");
  strcpy(paramTags[4].dim3.name, "");
  strcpy(paramTags[4].pos1.name, "");
  strcpy(paramTags[4].pos2.name, "");
  strcpy(paramTags[4].pos3.name, "satpos");
  strcpy(paramTags[4].pss.name, "");
  strcpy(paramTags[4].thk.name, "satthk");
  strcpy(paramTags[4].gap.name, "");
  strcpy(paramTags[4].ns.name, "");

  // init useParam flag to 1.
  for(i=0;i<ntags;i++) {
    paramTags[i].orient.use=1;
    paramTags[i].theta.use=1;
    paramTags[i].psi.use=1;
    paramTags[i].phi.use=1;
    paramTags[i].dim1.use=1;
    paramTags[i].dim2.use=1;
    paramTags[i].dim3.use=1;
    paramTags[i].pos1.use=1;
    paramTags[i].pos2.use=1;
    paramTags[i].pos3.use=1;
    paramTags[i].pss.use=1;
    paramTags[i].thk.use=1;
    paramTags[i].gap.use=1;
    paramTags[i].ns.use=1;
  }
}

// conver user entered color name to defined colors
int getColorByName(char *colorName) {
   if(strcasecmp(colorName,"BLACK") == 0) return BLACK;
   else if(strcasecmp(colorName,"RED") == 0) return RED;
   else if(strcasecmp(colorName,"YELLOW") == 0) return YELLOW;
   else if(strcasecmp(colorName,"GREEN") == 0) return GREEN;
   else if(strcasecmp(colorName,"CYAN") == 0) return CYAN;
   else if(strcasecmp(colorName,"BLUE") == 0) return BLUE;
   else if(strcasecmp(colorName,"MAGENTA") == 0) return MAGENTA;
   else if(strcasecmp(colorName,"WHITE") == 0) return WHITE;
   else if(strcasecmp(colorName,"ORANGE") == 0) return ORANGE;
   else if(strcasecmp(colorName,"GRAY") == 0) return GRAY_COLOR;
   else if(strcasecmp(colorName,"PINK") == 0) return PINK_COLOR;

   // somehow strcmp or strcasecmp does not like strptr=buf+n;
   // so try strstr
   if(strstr(colorName,"black")) return BLACK;
   else if(strstr(colorName,"red")) return RED;
   else if(strstr(colorName,"yellow")) return YELLOW;
   else if(strstr(colorName,"green")) return GREEN;
   else if(strstr(colorName,"cyan")) return CYAN;
   else if(strstr(colorName,"blue")) return BLUE;
   else if(strstr(colorName,"magenta")) return MAGENTA;
   else if(strstr(colorName,"white")) return WHITE;
   else if(strstr(colorName,"orange")) return ORANGE;
   else if(strstr(colorName,"gray")) return GRAY_COLOR;
   else if(strstr(colorName,"pink")) return PINK_COLOR;
   else return CYAN;
}

// read tags from templates/vnmrj/choicefiles/planParams
// using appdir search order.
void readTags()
{
  char fullpath[MAXSTR], buf[MAXSTR];
  FILE* fp;
  char *tokptr;
  char words[64][64];
  int count, i;
  int colorFlg=0;

  if( appdirFind("planParams", "templates/vnmrj/choicefiles", fullpath, "", R_OK) ) {
    if((fp = fopen(fullpath, "r"))) {
      ntags = 0;
      while (fgets(buf,sizeof(buf),fp)) {
	if(strlen(buf) > 1 && buf[0] == '#') {
	  if(strstr(buf, "planColor") != NULL) colorFlg=1;
	} else if(strlen(buf) > 1 && strstr(buf,"COLOR") == buf) {
	  if(strstr(buf,"highlight")) highlight=getColorByName(buf+16);
	  else if(strstr(buf,"handle")) handleColor=getColorByName(buf+13);
	  else if(strstr(buf,"grid")) meshColor=getColorByName(buf+11);
	  else if(strstr(buf,"axis")) axisColor=getColorByName(buf+11);
	  else if(strstr(buf,"mark")) markColor=getColorByName(buf+11);
	  else if(strstr(buf,"onPlanOverlay")) onPlaneColor=getColorByName(buf+20);
	  else if(strstr(buf,"offPlanOverlay")) offPlaneColor=getColorByName(buf+21);
	} else if(strlen(buf) > 1 && buf[0] != '#') {
          ntags++;
	  if(paramTags == NULL) paramTags = (planParams *)malloc(sizeof(planParams)*ntags);  
	  else paramTags = (planParams *)realloc(paramTags, sizeof(planParams)*ntags);
	  
	  // break buf into tok of parameter names
	  count=0;
	  tokptr = strtok(buf, ", \t\n");
	  while(tokptr != NULL) {
	    if(strlen(tokptr) > 0) {
              strcpy(words[count], tokptr);
              count++;
            }
	    tokptr = strtok(NULL, ", \t\n");
          }

	  i = ntags-1;
          if(!colorFlg) { // old planParams file format with hard coded color
	    if(count>0) strcpy(paramTags[i].planName,words[0]);
	    if(count>1) paramTags[i].planType = atoi(words[1]); 
	    if(count>2) paramTags[i].overlayType = atoi(words[2]); 
	    if(paramTags[i].overlayType == VOLUME) paramTags[i].planColor = ORANGE;
	    else if(paramTags[i].overlayType == VOXEL) paramTags[i].planColor = GREEN;
	    else if(paramTags[i].overlayType == SATBAND) paramTags[i].planColor = WHITE;
	    else paramTags[i].planColor = CYAN;
	    if(count>3) strcpy(paramTags[i].orient.name, words[3]);
	    else strcpy(paramTags[i].orient.name, ""); 
	    if(count>4) strcpy(paramTags[i].theta.name, words[4]);
	    else strcpy(paramTags[i].theta.name, ""); 
	    if(count>5) strcpy(paramTags[i].psi.name, words[5]);
	    else strcpy(paramTags[i].psi.name, ""); 
	    if(count>6) strcpy(paramTags[i].phi.name, words[6]);
	    else strcpy(paramTags[i].phi.name, ""); 
	    if(paramTags[i].overlayType == SATBAND) {
	      strcpy(paramTags[i].dim1.name, ""); 
	      strcpy(paramTags[i].dim2.name, ""); 
	      strcpy(paramTags[i].dim3.name, ""); 
	      strcpy(paramTags[i].pos1.name, ""); 
	      strcpy(paramTags[i].pos2.name, ""); 
	      if(count>7) strcpy(paramTags[i].pos3.name, words[7]);
	      else strcpy(paramTags[i].pos3.name, ""); 
	      strcpy(paramTags[i].pss.name, ""); 
	      if(count>8) strcpy(paramTags[i].thk.name, words[8]);
	      else strcpy(paramTags[i].thk.name, ""); 
	      strcpy(paramTags[i].gap.name, ""); 
	      strcpy(paramTags[i].ns.name, ""); 
            } else {
	      if(count>7) strcpy(paramTags[i].dim1.name, words[7]);
	      else strcpy(paramTags[i].dim1.name, ""); 
	      if(count>8) strcpy(paramTags[i].dim2.name, words[8]);
	      else strcpy(paramTags[i].dim2.name, ""); 
	      if(count>9) strcpy(paramTags[i].dim3.name, words[9]);
	      else strcpy(paramTags[i].dim3.name, ""); 
	      if(count>10) strcpy(paramTags[i].pos1.name, words[10]);
	      else strcpy(paramTags[i].pos1.name, ""); 
	      if(count>11) strcpy(paramTags[i].pos2.name, words[11]);
	      else strcpy(paramTags[i].pos2.name, ""); 
	      if(count>12) strcpy(paramTags[i].pos3.name, words[12]);
	      else strcpy(paramTags[i].pos3.name, ""); 
	      if(count>13) strcpy(paramTags[i].pss.name, words[13]);
	      else strcpy(paramTags[i].pss.name, ""); 
	      if(count>14) strcpy(paramTags[i].thk.name, words[14]);
	      else strcpy(paramTags[i].thk.name, ""); 
	      if(count>15) strcpy(paramTags[i].gap.name, words[15]);
	      else strcpy(paramTags[i].gap.name, ""); 
	      if(count>16) strcpy(paramTags[i].ns.name, words[16]);
	      else strcpy(paramTags[i].ns.name, ""); 
	    }

	  } else { // new planParams file format with plancolor added and
		   // overlayType and planType swapped.

	    if(count>0) strcpy(paramTags[i].planName,words[0]);
	    if(count>1) paramTags[i].overlayType = atoi(words[1]); 
	    if(count>2) paramTags[i].planType = atoi(words[2]); 
	    if(count>3) paramTags[i].planColor = getColorByName(words[3]); 
	    if(count>4) strcpy(paramTags[i].orient.name, words[4]);
	    else strcpy(paramTags[i].orient.name, ""); 
	    if(count>5) strcpy(paramTags[i].theta.name, words[5]);
	    else strcpy(paramTags[i].theta.name, ""); 
	    if(count>6) strcpy(paramTags[i].psi.name, words[6]);
	    else strcpy(paramTags[i].psi.name, ""); 
	    if(count>7) strcpy(paramTags[i].phi.name, words[7]);
	    else strcpy(paramTags[i].phi.name, ""); 
	    if(paramTags[i].overlayType == SATBAND) {
	      strcpy(paramTags[i].dim1.name, ""); 
	      strcpy(paramTags[i].dim2.name, ""); 
	      strcpy(paramTags[i].dim3.name, ""); 
	      strcpy(paramTags[i].pos1.name, ""); 
	      strcpy(paramTags[i].pos2.name, ""); 
	      if(count>8) strcpy(paramTags[i].pos3.name, words[8]);
	      else strcpy(paramTags[i].pos3.name, ""); 
	      strcpy(paramTags[i].pss.name, ""); 
	      if(count>9) strcpy(paramTags[i].thk.name, words[9]);
	      else strcpy(paramTags[i].thk.name, ""); 
	      strcpy(paramTags[i].gap.name, ""); 
	      strcpy(paramTags[i].ns.name, ""); 
            } else {
	      if(count>8) strcpy(paramTags[i].dim1.name, words[8]);
	      else strcpy(paramTags[i].dim1.name, ""); 
	      if(count>9) strcpy(paramTags[i].dim2.name, words[9]);
	      else strcpy(paramTags[i].dim2.name, ""); 
	      if(count>10) strcpy(paramTags[i].dim3.name, words[10]);
	      else strcpy(paramTags[i].dim3.name, ""); 
	      if(count>11) strcpy(paramTags[i].pos1.name, words[11]);
	      else strcpy(paramTags[i].pos1.name, ""); 
	      if(count>12) strcpy(paramTags[i].pos2.name, words[12]);
	      else strcpy(paramTags[i].pos2.name, ""); 
	      if(count>13) strcpy(paramTags[i].pos3.name, words[13]);
	      else strcpy(paramTags[i].pos3.name, ""); 
	      if(count>14) strcpy(paramTags[i].pss.name, words[14]);
	      else strcpy(paramTags[i].pss.name, ""); 
	      if(count>15) strcpy(paramTags[i].thk.name, words[15]);
	      else strcpy(paramTags[i].thk.name, ""); 
	      if(count>16) strcpy(paramTags[i].gap.name, words[16]);
	      else strcpy(paramTags[i].gap.name, ""); 
	      if(count>17) strcpy(paramTags[i].ns.name, words[17]);
	      else strcpy(paramTags[i].ns.name, ""); 
	    }
          }

	  // init "use" to default
    	  paramTags[i].orient.use=1;
    	  paramTags[i].theta.use=1;
    	  paramTags[i].psi.use=1;
    	  paramTags[i].phi.use=1;
    	  paramTags[i].dim1.use=1;
    	  paramTags[i].dim2.use=1;
    	  paramTags[i].dim3.use=1;
    	  paramTags[i].pos1.use=1;
/*
	  if(paramTags[i].overlayType == REGULAR || 
	     paramTags[i].overlayType == VOLUME) paramTags[i].pos2.use=0;
	  else
*/
    	     paramTags[i].pos2.use=1;
    	  paramTags[i].pos3.use=1;
    	  paramTags[i].pss.use=1;
    	  paramTags[i].thk.use=1;
    	  paramTags[i].gap.use=1;
    	  paramTags[i].ns.use=1;
	}	
      }
    }
  }
}

void initTags()
{
  readTags();
  if(paramTags == NULL || ntags < 1) setDefaultTags();
}

// return tag by name
planParams *getPlanTag(int planType) 
{
   int i;
   if(paramTags == NULL) initTags();
   if(planType < 0) return NULL;

   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == planType) {
	return (&paramTags[i]);
      }
   } 
   return NULL; 
}

// similar to getPlanTag, but parameter values are updated.
planParams *getCurrPlanParamsForInd(int planType, int ind) {

   planParams *tag = getPlanTag(planType);
   if(tag != NULL) updateParams(tag, ind);
   else Winfoprintf("Parameters are not defined for plan type %d.",planType);
   return tag;
}

planParams *getCurrPlanParams(int planType) {
   createPlanParams(planType);
   return getCurrPlanParamsForInd(planType, 1);
}

int getUseParam(int planType, char *paramName) {
   planParams *tag = getPlanTag(planType);
   if(tag == NULL || paramName == NULL) return 0;

   if(strcmp(paramName, tag->orient.name) == 0) return tag->orient.use;
   if(strcmp(paramName, tag->theta.name) == 0) return tag->theta.use;
   if(strcmp(paramName, tag->psi.name) == 0) return tag->psi.use;
   if(strcmp(paramName, tag->phi.name) == 0) return tag->phi.use;
   if(strcmp(paramName, tag->dim1.name) == 0) return tag->dim1.use;
   if(strcmp(paramName, tag->dim2.name) == 0) return tag->dim2.use;
   if(strcmp(paramName, tag->dim3.name) == 0) return tag->dim3.use;
   if(strcmp(paramName, tag->pos1.name) == 0) return tag->pos1.use;
   if(strcmp(paramName, tag->pos2.name) == 0) return tag->pos2.use;
   if(strcmp(paramName, tag->pos3.name) == 0) return tag->pos3.use;
   if(strcmp(paramName, tag->pss.name) == 0) return tag->pss.use;
   if(strcmp(paramName, tag->thk.name) == 0) return tag->thk.use;
   if(strcmp(paramName, tag->gap.name) == 0) return tag->gap.use;
   if(strcmp(paramName, tag->ns.name) == 0) return tag->ns.use;
   else return 0;
}

// set flag to fix parameter value (not to allow the value to be changed)
void setUseParam(int planType, char *paramName, int mode)
{
   planParams *tag = getPlanTag(planType);
   if(tag == NULL || paramName == NULL) return;

   if(strcmp(paramName, tag->orient.name) == 0) tag->orient.use=mode;
   if(strcmp(paramName, tag->theta.name) == 0) tag->theta.use=mode;
   if(strcmp(paramName, tag->psi.name) == 0) tag->psi.use=mode;
   if(strcmp(paramName, tag->phi.name) == 0) tag->phi.use=mode;
   if(strcmp(paramName, tag->dim1.name) == 0) tag->dim1.use=mode;
   if(strcmp(paramName, tag->dim2.name) == 0) tag->dim2.use=mode;
   if(strcmp(paramName, tag->dim3.name) == 0) tag->dim3.use=mode;
   if(strcmp(paramName, tag->pos1.name) == 0) tag->pos1.use=mode;
   if(strcmp(paramName, tag->pos2.name) == 0) tag->pos2.use=mode;
   if(strcmp(paramName, tag->pos3.name) == 0) tag->pos3.use=mode;
   if(strcmp(paramName, tag->pss.name) == 0) tag->pss.use=mode;
   if(strcmp(paramName, tag->thk.name) == 0) tag->thk.use=mode;
   if(strcmp(paramName, tag->gap.name) == 0) tag->gap.use=mode;
   if(strcmp(paramName, tag->ns.name) == 0) tag->ns.use=mode;
}

int getPlanColor(int planType)
{
   int i;
   int color = CYAN;
   if(paramTags == NULL) initTags();

   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == planType) color=paramTags[i].planColor;
   }
   return color;
}

int getPlanColorByOrient(int planType, float theta, float psi, float phi)
{
   int i;
   int color = CYAN;
   char orient[MAXSTR];
   if(paramTags == NULL) initTags();

   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == planType) color=paramTags[i].planColor;
   }
   if(planType == REGULAR) {
       P_getstring(CURRENT, "orient", orient, 1, MAXSTR);
       if(strcmp(orient,"3orthogonal") == 0) { // hard coded colors for 3-plane
         i = majorOrient(theta, psi, phi);
         if(i == 2 || i == 5) color = MAGENTA;  /*sag*/
         else if(i == 1 || i == 4) color = BLUE;  /*cor*/
         else color = CYAN;  /*trans*/
       } 
   } 
   return color;
}

int getOverlayType(int planType)
{
   int i;
   if(paramTags == NULL) initTags();

   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == planType) return paramTags[i].overlayType;
   }
   if(planType == CSI2D) return REGULAR;
   if(planType == CSIVOXEL) return VOXEL;
   if(planType == CSI3D) return VOLUME;
   else return planType;
}

// Note, this is planType
int getTypeByTagName(char *planName)
{
   int i;
   if(paramTags == NULL) initTags();

   for(i=0; i<ntags; i++) {
      if(strcasecmp(paramTags[i].planName, planName) == 0) return paramTags[i].planType;
   }
   if(strcasecmp(planName, "slices")) return REGULAR;
   else if(strcasecmp(planName, "volume")) return VOLUME;
   else if(strcasecmp(planName, "voxel")) return VOXEL;
   else if(strcasecmp(planName, "satband")) return SATBAND;
   else if(strcasecmp(planName, "CSI2D")) return CSI2D;
   else if(strcasecmp(planName, "CSI3D")) return CSI3D;
   else if(strcasecmp(planName, "CSIvoxel")) return CSIVOXEL;
   else return 0;
}

// Note, this is planType
// called by iplan.c when new plan of given type is created
char *getTagNameByType(int type)
{
   int i;
   if(paramTags == NULL) initTags();

   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == type) return paramTags[i].planName;
   }
   // this happens only if tags are not defined in the file.
   if(type==REGULAR) return "slices";
   else if(type==VOLUME) return "volume";
   else if(type==VOXEL) return "voxel";
   else if(type==SATBAND) return "satband";
   else if(type==CSI2D) return "CSI2D";
   else if(type==CSI3D) return "CSI3D";
   else if(type==CSIVOXEL) return "CSIvoxel";
   else return "slices";
}

// return current parameter values for given tag. 
// If a parameter is arrayed, only the first element is returned.
// parameter ptr can be NULL if not need to be returned.
// this function also return a pnew string that is ready to be sent.
// returned thk is in cm (it is in mm as a paramater). 
void updateParams(planParams *tag, int ind)
{
   if(tag == NULL) return;

     if(strlen(tag->orient.name)<1 || P_getstring(CURRENT,tag->orient.name,tag->orient.strValue,1,64)) 
	strcpy(tag->orient.strValue,"orient");
     if(strlen(tag->theta.name)<1 || P_getreal(CURRENT,tag->theta.name,&(tag->theta.value),ind))
	tag->theta.value=0;
     if(strlen(tag->psi.name)<1 || P_getreal(CURRENT,tag->psi.name,&(tag->psi.value),ind))
	tag->psi.value=0;
     if(strlen(tag->phi.name)<1 || P_getreal(CURRENT,tag->phi.name,&(tag->phi.value),ind))
	tag->phi.value=0;
     if(strlen(tag->dim1.name)<1 || P_getreal(CURRENT,tag->dim1.name,&(tag->dim1.value),ind))
	tag->dim1.value=10;
     if(strlen(tag->dim2.name)<1 || P_getreal(CURRENT,tag->dim2.name,&(tag->dim2.value),ind))
	tag->dim2.value=10;
     if(strlen(tag->dim3.name)<1 || P_getreal(CURRENT,tag->dim3.name,&(tag->dim3.value),ind))
	tag->dim3.value=10;
     if(strlen(tag->pos1.name)<1 || P_getreal(CURRENT,tag->pos1.name,&(tag->pos1.value),ind))
	tag->pos1.value=0;
     if(strlen(tag->pos2.name)<1 || P_getreal(CURRENT,tag->pos2.name,&(tag->pos2.value),ind))
	tag->pos2.value=0;
     if(strlen(tag->pos3.name)<1 || P_getreal(CURRENT,tag->pos3.name,&(tag->pos3.value),ind))
	tag->pos3.value=0;
     if(strlen(tag->pss.name)<1 || P_getreal(CURRENT,tag->pss.name,&(tag->pss.value),1))
	tag->pss.value=0;
     if(strlen(tag->thk.name)<1 || P_getreal(CURRENT,tag->thk.name,&(tag->thk.value),1))
	tag->thk.value=4;
     if(strlen(tag->gap.name)<1 || P_getreal(CURRENT,tag->gap.name,&(tag->gap.value),1))
	tag->gap.value=1;
     if(strlen(tag->ns.name)<1 || P_getreal(CURRENT,tag->ns.name,&(tag->ns.value),1))
	tag->ns.value=5;
   // NOTE, thk and voxel dimensions are converted from mm to cm.
   // (FOV and position are already in cm)
   tag->thk.value *= 0.1;
   if(tag->overlayType == VOXEL) {
      tag->dim1.value *= 0.1;
      tag->dim2.value *= 0.1;
      tag->dim3.value *= 0.1;
   }
/*
   if(tag->overlayType == CSI3D || tag->overlayType == VOLUME) {
     if(tag->dim3.value <= 1) { // dim3 = gap*(ns-1) + ns*thk
        planParam gap,thk,ns;
        getPlanParams(REGULAR,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
             NULL,NULL,NULL,NULL,&thk,&gap,&ns,1);
	tag->dim3.value = gap.value*(ns.value-1) + ns.value*thk.value;
     }
   }
*/
}

void getPlanParamNames(int planType, char *parlist) {

   int i;
   if(paramTags == NULL) initTags();

   strcpy(parlist,"");
   for(i=0; i<ntags; i++) {
      if(paramTags[i].planType == planType) {
	if(strlen(paramTags[i].orient.name) > 0) { 
	  strcat(parlist,paramTags[i].orient.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].theta.name) > 0) { 
	  strcat(parlist,paramTags[i].theta.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].psi.name) > 0) { 
	  strcat(parlist,paramTags[i].psi.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].phi.name) > 0) { 
	  strcat(parlist,paramTags[i].phi.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].dim1.name) > 0) { 
	  strcat(parlist,paramTags[i].dim1.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].dim2.name) > 0) { 
	  strcat(parlist,paramTags[i].dim2.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].dim3.name) > 0) { 
	  strcat(parlist,paramTags[i].dim3.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].pos1.name) > 0) { 
	  strcat(parlist,paramTags[i].pos1.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].pos2.name) > 0) { 
	  strcat(parlist,paramTags[i].pos2.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].pos3.name) > 0) { 
	  strcat(parlist,paramTags[i].pos3.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].pss.name) > 0) { 
	  strcat(parlist,paramTags[i].pss.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].thk.name) > 0) { 
	  strcat(parlist,paramTags[i].thk.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].gap.name) > 0) { 
	  strcat(parlist,paramTags[i].gap.name);strcat(parlist,","); 
	}
	if(strlen(paramTags[i].ns.name) > 0) { 
	  strcat(parlist,paramTags[i].ns.name);strcat(parlist,","); 
	}
      }
   }
}

void sendPnew(planParam *orient, planParam *theta, planParam *psi, planParam *phi, 
		planParam *dim1, planParam *dim2, planParam *dim3, planParam *pos1, planParam *pos2, 
		planParam *pos3, planParam *pss, planParam *thk, planParam *gap, planParam *ns, int ind) {
   int i=0;
   char str[MAXSTR];
//   char pnewString[MAXSTR];
   char strValue[64];
   double d;

   strcpy(str,"");

   if(orient != NULL && strlen(orient->name) > 0 && !P_getstring(CURRENT,orient->name,strValue,1,64)
	&& strcmp(orient->strValue,strValue) != 0) {i++; strcat(str," "); strcat(str,orient->name);}
   if(theta != NULL && strlen(theta->name) > 0 && !P_getreal(CURRENT,theta->name,&d,ind)
	&& theta->value != d) {i++; strcat(str," "); strcat(str,theta->name);}
   if(psi != NULL && strlen(psi->name) > 0 && !P_getreal(CURRENT,psi->name,&d,ind)
	&& psi->value != d) {i++; strcat(str," "); strcat(str,psi->name);}
   if(phi != NULL && strlen(phi->name) > 0 && !P_getreal(CURRENT,phi->name,&d,ind)
	&& phi->value != d) {i++; strcat(str," "); strcat(str,phi->name);}
   if(dim1 != NULL && strlen(dim1->name) > 0 && !P_getreal(CURRENT,dim1->name,&d,ind)
	&& dim1->value != d) {i++; strcat(str," "); strcat(str,dim1->name);}
   if(dim2 != NULL && strlen(dim2->name) > 0 && !P_getreal(CURRENT,dim2->name,&d,ind)
	&& dim2->value != d) {i++; strcat(str," "); strcat(str,dim2->name);}
   if(dim3 != NULL && strlen(dim3->name) > 0 && !P_getreal(CURRENT,dim3->name,&d,ind)
	&& dim3->value != d) {i++; strcat(str," "); strcat(str,dim3->name);}
   if(pos1 != NULL && strlen(pos1->name) > 0 && !P_getreal(CURRENT,pos1->name,&d,ind)
	&& pos1->value != d) {i++; strcat(str," "); strcat(str,pos1->name);}
   if(pos2 != NULL && strlen(pos2->name) > 0 && !P_getreal(CURRENT,pos2->name,&d,ind)
	&& pos2->value != d) {i++; strcat(str," "); strcat(str,pos2->name);}
   if(pos3 != NULL && strlen(pos3->name) > 0 && !P_getreal(CURRENT,pos3->name,&d,ind)
	&& pos3->value != d) {i++; strcat(str," "); strcat(str,pos3->name);}
   if(pss != NULL && strlen(pss->name) > 0 && !P_getreal(CURRENT,pss->name,&d,1)
	&& pss->value != d) {i++; strcat(str," "); strcat(str,pss->name);}
   if(thk != NULL && strlen(thk->name) > 0 && !P_getreal(CURRENT,thk->name,&d,1)
	&& thk->value != 0.1*d) {i++; strcat(str," "); strcat(str,thk->name);}
   if(gap != NULL && strlen(gap->name) > 0 && !P_getreal(CURRENT,gap->name,&d,1)
	&& gap->value != d) {i++; strcat(str," "); strcat(str,gap->name);}
   if(ns != NULL && strlen(ns->name) > 0 && !P_getreal(CURRENT,ns->name,&d,1)
	&& (int)(ns->value) != (int)d) {i++; strcat(str," "); strcat(str,ns->name);}
 
   // sprintf(pnewString,"%d %s",i,str);
   // writelineToVnmrJ("pnew",pnewString); 
   appendJvarlist(str);
}
 
// return parameter names for given tag
void getPlanParams(int planType, planParam *orient, planParam *theta, planParam *psi, planParam *phi, 
		planParam *dim1, planParam *dim2, planParam *dim3, planParam *pos1, planParam *pos2, 
		planParam *pos3, planParam *pss, planParam *thk, planParam *gap, planParam *ns, int ind) 
{
   planParams *tag = getCurrPlanParamsForInd(planType, ind);
   if(tag == NULL) return;

   if(orient != NULL) *orient=(tag->orient);
   if(theta != NULL) *theta=(tag->theta);
   if(psi != NULL) *psi=(tag->psi);
   if(phi != NULL) *phi=(tag->phi);
   if(dim1 != NULL) *dim1=(tag->dim1);
   if(dim2 != NULL) *dim2=(tag->dim2);
   if(dim3 != NULL) *dim3=(tag->dim3);
   if(pos1 != NULL) *pos1=(tag->pos1);
   if(pos2 != NULL) *pos2=(tag->pos2);
   if(pos3 != NULL) *pos3=(tag->pos3);
   if(pss != NULL) *pss=(tag->pss);
   if(thk != NULL) *thk=(tag->thk);
   if(gap != NULL) *gap=(tag->gap);
   if(ns != NULL) *ns=(tag->ns);
}

// print out what's current plan parameters.
void printCurrTag()
{
   planParam orient, theta, psi, phi, dim1, dim2, dim3,
        pos1, pos2, pos3, pss, thk, gap, ns;
   char planName[64];

   int planType = getIplanType();
   strcpy(planName,getTagNameByType(planType));
   if(planName == NULL || strlen(planName) < 1) {
     Winfoprintf("plan type is not selected.");
     return;
   }

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,1);

   Winfoprintf("planName planType %s %d", planName, planType);
   Winfoprintf("orient %s %s %d", orient.name,orient.strValue,orient.use);
   Winfoprintf("theta %s %f %d", theta.name,theta.value,theta.use);
   Winfoprintf("psi %s %f %d", psi.name,psi.value,psi.use);
   Winfoprintf("phi %s %f %d", phi.name,phi.value,phi.use);
   Winfoprintf("dim1 %s %f %d", dim1.name,dim1.value,dim1.use);
   Winfoprintf("dim2 %s %f %d", dim2.name,dim2.value,dim2.use);
   Winfoprintf("dim3 %s %f %d", dim3.name,dim3.value,dim3.use);
   Winfoprintf("pos1 %s %f %d", pos1.name,pos1.value,pos1.use);
   Winfoprintf("pos2 %s %f %d", pos2.name,pos2.value,pos2.use);
   Winfoprintf("pos3 %s %f %d", pos3.name,pos3.value,pos3.use);
   Winfoprintf("pss %s %f %d", pss.name,pss.value,pss.use);
   Winfoprintf("thk %s %f %d", thk.name,thk.value,thk.use);
   Winfoprintf("gap %s %f %d", gap.name,gap.value,gap.use);
   Winfoprintf("ns %s %f %d", ns.name,ns.value,ns.use);
}

// called by setPlanValue when parameters affecting slices are changed.
void updateSlices(char *spss, int ns, double pos3, double thk, double gap)
{
   double shift, pss; 
   int i;

   if(ns%2 == 0) shift = (ns)/2.0 - 0.5;
   else shift = (ns-1.0)/2.0;
   clearVar(spss);
   for(i=0; i<ns; i++) {
      pss = pos3 + (i-shift)*(thk+gap);
      P_setreal(CURRENT, spss,pss,i+1);
   }
}

int checkPlanTypeForParam(char *paramName, int planType) {
   planParams *tag = getPlanTag(planType);
   if(tag == NULL) return 0;
   
   if(strcmp(paramName, tag->orient.name) == 0) return 1;
   if(strcmp(paramName, tag->theta.name) == 0) return 1;
   if(strcmp(paramName, tag->psi.name) == 0) return 1;
   if(strcmp(paramName, tag->phi.name) == 0) return 1;
   if(strcmp(paramName, tag->dim1.name) == 0) return 1;
   if(strcmp(paramName, tag->dim2.name) == 0) return 1;
   if(strcmp(paramName, tag->dim3.name) == 0) return 1;
   if(strcmp(paramName, tag->pos1.name) == 0) return 1;
   if(strcmp(paramName, tag->pos2.name) == 0) return 1;
   if(strcmp(paramName, tag->pos3.name) == 0) return 1;
   if(strcmp(paramName, tag->pss.name) == 0) return 1;
   if(strcmp(paramName, tag->thk.name) == 0) return 1;
   if(strcmp(paramName, tag->gap.name) == 0) return 1;
   if(strcmp(paramName, tag->ns.name) == 0) return 1;
   return 0;
}
 
int getPlanTypeForParam(char *paramName) {
   vInfo           paraminfo;
   double d;
   int i, n;
   int planType = getIplanType();
   if(checkPlanTypeForParam(paramName, planType)) return planType; 

   if(!(P_getVarInfo(CURRENT, "iplanDefaultType", &paraminfo))) {
        n = paraminfo.size;
   } else n = 0;

   for(i=0; i<n; i++) {
      if(!(P_getreal(CURRENT, "iplanDefaultType", &d, i+1))) {
	planType = (int)d;
	if(checkPlanTypeForParam(paramName, planType)) return planType;
      }
   }
   for(i=0; i<ntags; i++) {
     planType = paramTags[i].planType;
     if(checkPlanTypeForParam(paramName, planType)) return planType;
   }
   return -1;
}

// get major orientation for euler angles
void getOrient(float theta, float psi, float phi, char *orient) {
      int intValue = majorOrient(theta, psi, phi);
      if(intValue == 0) strcpy(orient, "trans");
      else if(intValue == 1) strcpy(orient, "cor");
      else if(intValue == 2) strcpy(orient, "sag");
      else if(intValue == 3) strcpy(orient, "trans90");
      else if(intValue == 4) strcpy(orient, "cor90");
      else if(intValue == 5) strcpy(orient, "sag90");
      else strcpy(orient, "oblique");
}

void getNewPos(float3 oldEuler, float3 oldPos, float3 newEuler, float3 newPos)
{
      float orientation[9];
      newPos[0]=oldPos[0];
      newPos[1]=oldPos[1];
      newPos[2]=oldPos[2];
      euler2tensor(oldEuler[0],oldEuler[1],oldEuler[2], orientation);
      rotateu2m(newPos, orientation);
      euler2tensor(newEuler[0],newEuler[1],newEuler[2], orientation);
      rotatem2u(newPos, orientation);
}

void getNewDim(float3 oldEuler, float3 oldDim, float3 newEuler, float3 newDim)
{
      getNewPos(oldEuler, oldDim, newEuler, newDim);
      newDim[0]=fabs(newDim[0]);
      newDim[1]=fabs(newDim[1]);
      newDim[2]=fabs(newDim[2]);
}

int getDecimal() {
     int dec;
     double d;
     if(P_getreal(GLOBAL,"planDecimal",&d,1)) dec = 3;
     else dec = d; // dec is digits after decimal point. 
     if(dec < 1) dec = 1;
     return pow(10, dec); // return 100 if dec=2.
}

// 
void updateSlab(int planType) {
   char slabctr[4];
   vInfo           paraminfo;
   double d, value;
   int n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   if(planType != REGULAR && planType != VOLUME) return;
   if((P_getVarInfo(CURRENT, "slabctr", &paraminfo) == -2)) return;

   if(P_getstring(CURRENT, "slabctr", slabctr, 1, 4)) strcpy(slabctr, "y");
   if(slabctr[0] != 'y') return;

   planParam sdim3,spos3,spss,sthk,sgap,sns;
   planParam vdim3,vpos3;
   getPlanParams(REGULAR,NULL,NULL,NULL,NULL,NULL,NULL,&sdim3,
             NULL,NULL,&spos3,&spss,&sthk,&sgap,&sns,1);
   getPlanParams(VOLUME,NULL,NULL,NULL,NULL,NULL,NULL,&vdim3,
             NULL,NULL,&vpos3,NULL,NULL,NULL,NULL,1);

   double thk;
   if((P_getVarInfo(CURRENT, "slabfract", &paraminfo) == -2) || 
		P_getreal(CURRENT,"slabfract",&thk,1)) thk=1.0; 
   thk=thk*vdim3.value;
   value = floor(n*(10.0*thk + d))/n; // note, value is in mm
   P_setreal(CURRENT, sthk.name, value, 1);
   int nslices = 1;
   if(!P_getVarInfo(CURRENT, spss.name, &paraminfo) ) nslices = paraminfo.size;
   if(planType == REGULAR) {
      value = floor(n*(spos3.value + d))/n;
      P_setreal(CURRENT,vpos3.name, value, 1);
      // update slab pss
      updateSlices(spss.name, nslices, spos3.value, thk, sgap.value);
      sendPnew(NULL,NULL,NULL,NULL,NULL,NULL,NULL,
             NULL,NULL,&vpos3,NULL,&sthk,NULL,NULL,1);
   } else {
      value = floor(n*(vpos3.value + d))/n;
      P_setreal(CURRENT,spos3.name, value, 1);
      // update slab pss
      updateSlices(spss.name, nslices, vpos3.value, thk, sgap.value);
      sendPnew(NULL,NULL,NULL,NULL,NULL,NULL,NULL,
             NULL,NULL,&spos3,NULL,&sthk,NULL,NULL,1);
   }
}

// called by gplan('setValue',...) 
// This function set plan parameter values based on current parametr tag.
// It calls getCurrentStacks at the end to load plan(s) based on parameters.
void setPlanValue(char* paramName, char *value)
{
   vInfo           paraminfo;
   planParam orient0, theta0, psi0, phi0, dim10, dim20, dim30,
        pos10, pos20, pos30, pss0, thk0, gap0, ns0;
   planParam orient, theta, psi, phi, dim1, dim2, dim3,
        pos1, pos2, pos3, pss, thk, gap, ns;
   planParam orient2, theta2, psi2, phi2, dim12, dim22, dim32,
        pos12, pos22, pos32, pss2, thk2, gap2, ns2;
   int intValue, rotatePos=0;
   float3 oldpos, olddim, oldeuler, newpos, newdim, neweuler;
   double d, dvalue, newVal;
   int planType, ind;

   if(paramName == NULL || strlen(paramName) < 1) return;
   if(value == NULL || strlen(value) < 1) return;

   planType = getPlanTypeForParam(paramName);
   if(planType < 0) {
      Winfoprintf("Error: %s is not a plan parameter.",paramName);
      return;
   }

   // get current parameters
   // Note, this is done before setting the value.
   // Note, changing these returned planParam does not change original tags. 
   getPlanParams(planType,&orient0,&theta0,&psi0,&phi0,&dim10,&dim20,&dim30,
        &pos10,&pos20,&pos30,&pss0,&thk0,&gap0,&ns0,1);

   // set the parameter
   dvalue = atof(value);
   ind=getActStackInd();
   if(!(P_getVarInfo(CURRENT, paramName, &paraminfo))) {
     if(ind>paraminfo.size) ind = paraminfo.size;
     if(paraminfo.basicType == T_STRING)
        P_setstring(CURRENT, paramName, value, ind);
     else
        P_setreal(CURRENT, paramName, dvalue, ind);

     if(ind>1 || paraminfo.size > 1) {
       if(currentStacks != NULL && currentStacks->numOfStacks > 0)
         getCurrentStacks();
       return;
     }
   } else return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,1);

   // need the second set of parameters for CSI
   if(planType == CSI2D || planType == CSI3D) {
   	getPlanParams(CSIVOXEL, &orient2,&theta2,&psi2,&phi2,&dim12,&dim22,
	&dim32,&pos12,&pos22,&pos32,&pss2,&thk2,&gap2,&ns2,1);
   }

   neweuler[0]=0;
   neweuler[1]=0;
   neweuler[2]=0;
   // set new euler if euler or orient parameter is changed.
   // do not modify theta.value, psi.value, phi.value to orient,
   // they are needed for rotatePos.
   if((strcmp(paramName,orient.name)==0 && strcmp(value,"oblique") == 0) 
	|| strcmp(paramName,theta.name)==0 
	|| strcmp(paramName,psi.name)==0 || strcmp(paramName,phi.name)==0) {
      rotatePos=1;
      if(!P_getreal(CURRENT,theta.name,&d,1)) neweuler[0]=d;
      if(!P_getreal(CURRENT,psi.name,&d,1)) neweuler[1]=d;
      if(!P_getreal(CURRENT,phi.name,&d,1)) neweuler[2]=d;
      // set orient parameter according to new euler
      getOrient(neweuler[0],neweuler[1],neweuler[2],orient.strValue);
      P_setstring(CURRENT, orient.name, orient.strValue, 1);
   } else if(strcmp(paramName,orient.name)==0 && 
	strcmp(value,"3orthogonal") == 0) {
      P_setreal(CURRENT, theta.name, 0, 1);
      P_setreal(CURRENT, psi.name, 0, 1);
      P_setreal(CURRENT, phi.name, 0, 1);
      P_setreal(CURRENT, theta.name, 90, 2);
      P_setreal(CURRENT, psi.name, 0, 2);
      P_setreal(CURRENT, phi.name, 0, 2);
      P_setreal(CURRENT, theta.name, 90, 3);
      P_setreal(CURRENT, psi.name, 90, 3);
      P_setreal(CURRENT, phi.name, 0, 3);
   } else if(strcmp(paramName,orient.name)==0) {
      rotatePos=1;
      if(strcmp(value,"sag") == 0) {
        neweuler[0]=90;
        neweuler[1]=90;
        neweuler[2]=0;
      } else if(strcmp(value,"cor") == 0) {
        neweuler[0]=90;
        neweuler[1]=0;
        neweuler[2]=0;
      } else if(strcmp(value,"trans") == 0) {
        neweuler[0]=0;
        neweuler[1]=0;
        neweuler[2]=0;
      } else if(strcmp(value,"sag90") == 0) {
        neweuler[0]=90;
        neweuler[1]=90;
        neweuler[2]=90;
      } else if(strcmp(value,"cor90") == 0) {
        neweuler[0]=90;
        neweuler[1]=0;
        neweuler[2]=90;
      } else if(strcmp(value,"trans90") == 0) {
        neweuler[0]=0;
        neweuler[1]=0;
        neweuler[2]=90;
      }
      clearVar(theta.name);
      clearVar(psi.name);
      clearVar(phi.name);
      P_setreal(CURRENT, theta.name, neweuler[0], 1);
      P_setreal(CURRENT, psi.name, neweuler[1], 1);
      P_setreal(CURRENT, phi.name, neweuler[2], 1);
   }

   // rotate position (pos1,2,3) according to new euler (new orientation)
/*  // currently does not work for orient.
   if(rotatePos && mcoils > 1) {
      isoRotationForMM(paramName, value);
   } else 
*/
   if(rotatePos) {
      // Note, swamp index 0 and 1 because of rotation matrix. 
      oldeuler[0]=theta.value;
      oldeuler[1]=psi.value;
      oldeuler[2]=phi.value;
      oldpos[1]=pos1.value;
      oldpos[0]=pos2.value;
      oldpos[2]=pos3.value;
      getNewPos(oldeuler, oldpos, neweuler, newpos);
      pos1.value=newpos[1];
      pos2.value=newpos[0];
      pos3.value=newpos[2];
      if(strlen(pos1.name)>0) P_setreal(CURRENT, pos1.name, pos1.value, 1);
      if(strlen(pos2.name)>0) P_setreal(CURRENT, pos2.name, pos2.value, 1);
      if(strlen(pos3.name)>0) P_setreal(CURRENT, pos3.name, pos3.value, 1);

// the following are special cases.
      // For CSI, the slice and voxel orietation should be the same.
      // So need to rotate the second set of position parameters.
      if(strcmp(paramName,orient.name)==0 && 
	(planType == CSI2D || planType == CSI3D)) {
   	oldeuler[0]=theta2.value;
   	oldeuler[1]=psi2.value;
   	oldeuler[2]=phi2.value;
        oldpos[1]=pos12.value;
        oldpos[0]=pos22.value;
        oldpos[2]=pos32.value;
        getNewPos(oldeuler, oldpos, neweuler, newpos);
        P_setreal(CURRENT, pos12.name, newpos[1], 1);
        P_setreal(CURRENT, pos22.name, newpos[0], 1);
        P_setreal(CURRENT, pos32.name, newpos[2], 1);
      }
      // this is a specially case for voxel. It won't work for oblique.
      // When voxel orientation changes, only the acqusition order is changed.
      // So dim1,2,3 should rotate with pos1,2,3 so the voxel remains the same. 
      if(strcmp(paramName,orient.name)==0 && getOverlayType(planType) == VOXEL 
	&& (strstr(orient.strValue,"trans") != NULL
	|| strstr(orient.strValue,"cor") != NULL 
	|| strstr(orient.strValue,"sag") != NULL) ) { 
   	oldeuler[0]=theta.value;
   	oldeuler[1]=psi.value;
   	oldeuler[2]=phi.value;
        olddim[1]=dim1.value;
        olddim[0]=dim2.value;
        olddim[2]=dim3.value;
	getNewDim(oldeuler, olddim, neweuler, newdim);
        dim1.value=newdim[1];
        dim2.value=newdim[0];
        dim3.value=newdim[2];
        P_setreal(CURRENT, dim1.name, 10.0*dim1.value, 1);
        P_setreal(CURRENT, dim2.name, 10.0*dim2.value, 1);
        P_setreal(CURRENT, dim3.name, 10.0*dim3.value, 1);
      }
      // update pss
      if(strlen(pss.name) > 0 && strlen(ns.name) > 0) {
	vInfo paraminfo;
        int nslices = 1.0;
        if(!P_getVarInfo(CURRENT, pss.name, &paraminfo) ) nslices = paraminfo.size;
	updateSlices(pss.name, nslices, pos3.value, thk.value, gap.value);
      } 
      theta.value=neweuler[0];
      psi.value=neweuler[1];
      phi.value=neweuler[2];
   }

   // if CSI, force orient, pos3 and dim3 the same for the slice and voxel.
   if(planType == CSI2D) {
	P_setstring(CURRENT, orient2.name, orient.strValue, 1);
        P_setreal(CURRENT, theta2.name, theta.value, 1);
        P_setreal(CURRENT, psi2.name, psi.value, 1);
        P_setreal(CURRENT, phi2.name, phi.value, 1);
        P_setreal(CURRENT, pos32.name, pos3.value, 1);
	P_setreal(CURRENT, dim32.name, 10.0*thk.value, 1); // dim32 is vox3
   	sendPnew(&orient2,&theta2,&psi2,&phi2,&dim12,&dim22,
	&dim32,&pos12,&pos22,&pos32,&pss2,&thk2,&gap2,&ns2,1);
   } else if(planType == CSI3D) {
	P_setstring(CURRENT, orient2.name, orient.strValue, 1);
        P_setreal(CURRENT, theta2.name, theta.value, 1);
        P_setreal(CURRENT, psi2.name, psi.value, 1);
        P_setreal(CURRENT, phi2.name, phi.value, 1);
   	sendPnew(&orient2,&theta2,&psi2,&phi2,&dim12,&dim22,
	&dim32,&pos12,&pos22,&pos32,&pss2,&thk2,&gap2,&ns2,1);
   }

   if (planType == VOLUME && strcmp(paramName,pss.name) == 0) {
      P_setreal(CURRENT, pos3.name, dvalue, 1);
      updateSlab(planType);
   } else if(strcmp(paramName,dim3.name) == 0 || 
	strcmp(paramName,pos3.name) == 0) {
      updateSlab(planType);
   }

   // set slices if related parameters are changed.
   if(strlen(pss.name) > 0 && strlen(ns.name) > 0) {
     vInfo paraminfo;
     int nslices = 1.0;
     if(!P_getVarInfo(CURRENT, pss.name, &paraminfo) ) nslices = paraminfo.size;
     if(strcmp(paramName,ns.name)==0) {
       intValue = (int)dvalue;
       if (intValue < 1) {
	 intValue = 1;
	 P_setreal(CURRENT, ns.name, 1.0, 1);
       }
       updateSlices(pss.name, intValue, pos3.value, thk.value, gap.value);
     }
     if(strcmp(paramName,pos3.name)==0) {
       updateSlices(pss.name, nslices, dvalue, thk.value, gap.value);
     }
     if(strcmp(paramName,gap.name)==0 && nslices > 1 ) {
       updateSlices(pss.name, nslices, pos3.value, thk.value, dvalue);
     }
     if(strcmp(paramName,thk.name)==0 && nslices > 1 ) {
       d = (nslices-1)*(thk0.value+gap.value); // note, thk0 is the original thk.
       newVal = d/(nslices - 1) - 0.1*dvalue; // d is the new gap 
       P_setreal(CURRENT, gap.name, newVal, 1);
     }
   }

   // set FOV as a cube (all sides are the same) if cubeFOV is set.
   if(getCubeFOV()) {
     if(strcmp(paramName,dim1.name) == 0) { 
	P_setreal(CURRENT,dim2.name,dvalue, 1); 
	P_setreal(CURRENT,dim3.name,dvalue, 1); 
     }
     if(strcmp(paramName,dim2.name) == 0) { 
	P_setreal(CURRENT,dim1.name,dvalue, 1); 
	P_setreal(CURRENT,dim3.name,dvalue, 1); 
     }
     if(strcmp(paramName,dim3.name) == 0) { 
	P_setreal(CURRENT,dim1.name,dvalue, 1); 
	P_setreal(CURRENT,dim2.name,dvalue, 1); 
     }
   }

   // write pnew to update the panel
   sendPnew(&orient0,&theta0,&psi0,&phi0,&dim10,&dim20,&dim30,
        &pos10,&pos20,&pos30,&pss0,&thk0,&gap0,&ns0,1);
   // load plan to graphics display.
   intValue = 1;
   if(strstr(paramName,"orient") != NULL || 
	(currentStacks != NULL && currentStacks->numOfStacks > 0)) {
	intValue = getCurrentStacks();
   }
   if(intValue > 0) { // returned without graphics plan
     // set array parameter.
     char arrayStr[MAXSTR];
     vInfo paraminfo;
     int nslices = 1.0;
     if(!P_getVarInfo(CURRENT, pss.name, &paraminfo) ) nslices = paraminfo.size;
     if(strcmp(orient.strValue,"3orthogonal") == 0) 
	getArrayStr(planType, 3, nslices, 1, arrayStr); 
     else getArrayStr(planType, 1, nslices, 0, arrayStr); 
     setArray(arrayStr);
   }
}

int isPlanParam(char *name) {
  int i;
  if(name == NULL || strlen(name)<1) return 0;

  for(i=0;i<ntags;i++) {
    if(strlen(paramTags[i].theta.name)>0 && strstr(name,paramTags[i].theta.name) != NULL) return 1;
    if(strlen(paramTags[i].psi.name)>0 && strstr(name,paramTags[i].psi.name) != NULL) return 1;
    if(strlen(paramTags[i].phi.name)>0 && strstr(name,paramTags[i].phi.name) != NULL) return 1;
    if(strlen(paramTags[i].dim1.name)>0 && strstr(name,paramTags[i].dim1.name) != NULL) return 1;
    if(strlen(paramTags[i].dim2.name)>0 && strstr(name,paramTags[i].dim2.name) != NULL) return 1;
    if(strlen(paramTags[i].dim3.name)>0 && strstr(name,paramTags[i].dim3.name) != NULL) return 1;
    if(strlen(paramTags[i].pos1.name)>0 && strstr(name,paramTags[i].pos1.name) != NULL) return 1;
    if(strlen(paramTags[i].pos2.name)>0 && strstr(name,paramTags[i].pos2.name) != NULL) return 1;
    if(strlen(paramTags[i].pos3.name)>0 && strstr(name,paramTags[i].pos3.name) != NULL) return 1;
    if(strlen(paramTags[i].pss.name)>0 && strstr(name,paramTags[i].pss.name) != NULL) return 1;
    if(strlen(paramTags[i].thk.name)>0 && strstr(name,paramTags[i].thk.name) != NULL) return 1;
    if(strlen(paramTags[i].gap.name)>0 && strstr(name,paramTags[i].gap.name) != NULL) return 1;
  }
  return 0;
}

void setArray(char *arrayStr) {
   char array[MAXSTR], str[MAXSTR];
   char *tokptr;
   int found=0; // found plan param in array.

   // remove plan parameters from current array.
   str[0] = '\0';
   P_getstring(CURRENT, "array", array, 1, MAXSTR); 
   if(strlen(array) > 0) {
     tokptr = strtok(array,",");
     strcpy(str,"");
     while(tokptr != NULL) {
       if(!isPlanParam(tokptr) ) {
         if(strlen(str) > 0) strcat(str,",");
         strcat(str,tokptr);
       } else found=1;
       tokptr = strtok(NULL, ",");
     }
   }

   if(!found && strlen(arrayStr) < 1) return;

   //found or arrayStr<>''
   if(strlen(arrayStr) < 1) {
 	P_setstring(CURRENT, "array", str, 1);
   } else if(strlen(str) >0) {
	strcat(str,",");
	strcat(str,arrayStr);
        P_setstring(CURRENT, "array", str, 1);
   } else { 
   	P_setstring(CURRENT, "array", arrayStr, 1);
   }
}

// note, count starts from 1 (alrady added by the caller).
// order also starts from 1. if order == 0, slice is not used.
void sendPss(char *pssName, iplan_stack *stack, int count) {
   int i,j,k;
   double d, value;
   int n = getDecimal();
   d = 0.5/n;

   j = 0; // count slices that order > 0
   clearVar(pssName);
   for(i=0; i<stack->ns; i++) {
      if(stack->order[i] > 0) {
        k = stack->order[i] - 1;
        value = stack->pss0 + stack->pss[k][0] + stack->pss[k][1];
	value = floor(n*(value + d))/n;
        P_setreal(CURRENT, pssName, value, j+count);
	j++;
      }
   }
}

void getArrayStr(int planType, int nstacks, int nslices, int is3P, char *arrayStr) {

   planParams *tag;
   int compressMode, type;

   strcpy(arrayStr,"");
   if(nstacks == 0) return;

   tag = getPlanTag(planType);
   if(tag == NULL) return;

   compressMode = getCompressMode(); // 1 is compressed, 2 is not compressed 
   type = getOverlayType(planType);

// array should never contains satband parameters.
/*
   if(type == SATBAND && nstacks > 1) {
     sprintf(arrayStr, "(%s,%s,%s,%s,%s)",
	tag->theta.name,tag->psi.name,tag->phi.name,tag->pos3.name,tag->thk.name);
   }  
*/
   if(type == VOXEL && nstacks > 1) {
     sprintf(arrayStr, "(%s,%s,%s,%s,%s,%s,%s,%s,%s)",
	tag->theta.name,tag->psi.name,tag->phi.name,tag->dim1.name,tag->dim2.name,tag->dim3.name,
	tag->pos1.name,tag->pos2.name,tag->pos3.name);
   } else if(type == VOLUME && nstacks > 1) {
     sprintf(arrayStr, "(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",
	tag->theta.name,tag->psi.name,tag->phi.name,tag->dim1.name,tag->dim2.name,tag->dim3.name,
	tag->pos1.name,tag->pos2.name,tag->pos3.name,tag->pss.name);
   } else if(type == REGULAR && is3P && nstacks > 1) {
     if(nslices > 1 && compressMode == 2)
	sprintf(arrayStr, "(%s,%s,%s),%s",tag->theta.name,tag->psi.name,tag->phi.name,tag->pss.name);
     else 
	sprintf(arrayStr, "(%s,%s,%s)",tag->theta.name,tag->psi.name,tag->phi.name);
   } else if(type == REGULAR) {
     if(nstacks > 1 && nslices > 1 && compressMode == 2) 
   	sprintf(arrayStr, "(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s),%s",
	tag->theta.name,tag->psi.name,tag->phi.name,tag->dim1.name,tag->dim2.name,tag->dim3.name,
	tag->pos1.name,tag->pos2.name,tag->pos3.name,tag->thk.name,tag->gap.name,tag->pss.name);
     else if(nstacks > 1) 
	sprintf(arrayStr, "(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",
	tag->theta.name,tag->psi.name,tag->phi.name,tag->dim1.name,tag->dim2.name,tag->dim3.name,
	tag->pos1.name,tag->pos2.name,tag->pos3.name,tag->thk.name,tag->gap.name);
     else if(nslices > 1 && compressMode == 2) 
	sprintf(arrayStr,"%s",tag->pss.name);
   }
}

// send satband parameters. Note, thk*10
void sendPlanParamsForSatband(prescription* plans, int planType, char *arrayStr) {
   int i, count;
   char orientStr[64]; 
   planParam orient, theta, psi, phi, pos3, thk;
   double d, value;
   int n;

   if(plans == NULL || plans->numOfStacks < 1) return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,NULL,NULL,NULL,
        NULL,NULL,&pos3,NULL,&thk,NULL,NULL,1);

   n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   count = 0;
   clearVar(theta.name); 
   clearVar(psi.name); 
   clearVar(phi.name); 
   clearVar(pos3.name); 
   clearVar(thk.name); 
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
	count++;
	if(count == 1) { // should orient be arrayed??
	  getOrient(plans->stacks[i].theta, plans->stacks[i].psi, 
		plans->stacks[i].phi, orientStr);
   	  P_setstring(CURRENT, orient.name, orientStr, count);
	} else getPlanParams(planType,&orient,&theta,&psi,&phi,NULL,NULL,NULL,
          NULL,NULL,&pos3,NULL,&thk,NULL,NULL,count);

	value = floor(n*(plans->stacks[i].theta + d))/n;
        P_setreal(CURRENT, theta.name, value, count);
	value = floor(n*(plans->stacks[i].psi + d))/n;
        P_setreal(CURRENT, psi.name, value, count);
	value = floor(n*(plans->stacks[i].phi + d))/n;
        P_setreal(CURRENT, phi.name, value, count);
	value = floor(n*(plans->stacks[i].pss0 + d))/n;
        P_setreal(CURRENT, pos3.name, value, count);
	value = floor(n*(plans->stacks[i].thk*10 + d))/n;
        P_setreal(CURRENT, thk.name, value, count);

        sendPnew(&orient,&theta,&psi,&phi,NULL,NULL,NULL,
          NULL,NULL,&pos3,NULL,&thk,NULL,NULL,count);
      }
   }
   if(count == 0) return;

   getArrayStr(planType, count, 1, 0, arrayStr); 
}

// send voxel parameters. Note, vox1=lro*10, vox2=lpe*10, vox3=lpe2*10
void sendPlanParamsForVoxel(prescription* plans, int planType, char *arrayStr) {
   int i, count;
   char orientStr[64]; 
   planParam orient, theta, psi, phi, dim1, dim2, dim3, pos1, pos2, pos3;
   double d, value;
   int n;

   if(plans == NULL || plans->numOfStacks < 1) return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,NULL,NULL,NULL,NULL,1);

   n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   count = 0;
   clearVar(theta.name); 
   clearVar(psi.name); 
   clearVar(phi.name); 
   clearVar(dim1.name); 
   clearVar(dim2.name); 
   clearVar(dim3.name); 
   clearVar(pos1.name); 
   clearVar(pos2.name); 
   clearVar(pos3.name); 
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
	count++;
	if(count == 1) { // should orient be arrayed??
	  getOrient(plans->stacks[i].theta, plans->stacks[i].psi, 
		plans->stacks[i].phi, orientStr);
   	  P_setstring(CURRENT, orient.name, orientStr, count);
	} else getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
           &pos1,&pos2,&pos3,NULL,NULL,NULL,NULL,count);

	value = floor(n*(plans->stacks[i].theta + d))/n;
        P_setreal(CURRENT, theta.name, value, count);
	value = floor(n*(plans->stacks[i].psi + d))/n;
        P_setreal(CURRENT, psi.name, value, count);
	value = floor(n*(plans->stacks[i].phi + d))/n;
        P_setreal(CURRENT, phi.name, value, count);
	value = floor(n*(plans->stacks[i].lpe*10 + d))/n;
        P_setreal(CURRENT, dim1.name, value, count);
	value = floor(n*(plans->stacks[i].lro*10 + d))/n;
        P_setreal(CURRENT, dim2.name, value, count);
	value = floor(n*(plans->stacks[i].lpe2*10 + d))/n;
        P_setreal(CURRENT, dim3.name, value, count);
	value = floor(n*(plans->stacks[i].ppe + d))/n;
        P_setreal(CURRENT, pos1.name, value, count);
	value = floor(n*(plans->stacks[i].pro + d))/n;
        P_setreal(CURRENT, pos2.name, value, count);
	value = floor(n*(plans->stacks[i].pss0 + d))/n;
        P_setreal(CURRENT, pos3.name, value, count);

        sendPnew(&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
           &pos1,&pos2,&pos3,NULL,NULL,NULL,NULL,count);
      }
   }
   if(count == 0) return;

   getArrayStr(planType, count, 1, 0, arrayStr); 
}

// send parameters for volume. 
void sendPlanParamsForVolume(prescription* plans, int planType, char *arrayStr) {
   int i, count;
   char orientStr[64]; 
   planParam orient, theta, psi, phi, dim1, dim2, dim3, pos1, pos2, pos3, pss;
   double d, value;
   int n;

   if(plans == NULL || plans->numOfStacks < 1) return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,NULL,NULL,NULL,1);

   n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   count = 0;
   clearVar(theta.name); 
   clearVar(psi.name); 
   clearVar(phi.name); 
   clearVar(dim1.name); 
   clearVar(dim2.name); 
   clearVar(dim3.name); 
   clearVar(pos1.name); 
   clearVar(pos2.name); 
   clearVar(pos3.name); 
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
	count++;
	if(count == 1) { // should orient be arrayed??
	  getOrient(plans->stacks[i].theta, plans->stacks[i].psi, 
		plans->stacks[i].phi, orientStr);
   	  P_setstring(CURRENT, orient.name, orientStr, count);
	} else getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
          &pos1,&pos2,&pos3,&pss,NULL,NULL,NULL,count);

	value = floor(n*(plans->stacks[i].theta + d))/n;
        P_setreal(CURRENT, theta.name, value, count);
	value = floor(n*(plans->stacks[i].psi + d))/n;
        P_setreal(CURRENT, psi.name, value, count);
	value = floor(n*(plans->stacks[i].phi + d))/n;
        P_setreal(CURRENT, phi.name, value, count);
	value = floor(n*(plans->stacks[i].lpe + d))/n;
        P_setreal(CURRENT, dim1.name, value, count);
	value = floor(n*(plans->stacks[i].lro + d))/n;
        P_setreal(CURRENT, dim2.name, value, count);
	value = floor(n*(plans->stacks[i].lpe2 + d))/n;
        P_setreal(CURRENT, dim3.name, value, count);
	value = floor(n*(plans->stacks[i].ppe + d))/n;
        P_setreal(CURRENT, pos1.name, value, count);
	value = floor(n*(plans->stacks[i].pro + d))/n;
        P_setreal(CURRENT, pos2.name, value, count);
	value = floor(n*(plans->stacks[i].pss0 + d))/n;
        P_setreal(CURRENT, pos3.name, value, count);
        updateSlab(planType);
        sendPnew(&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,NULL,NULL,NULL,count);

      }
   }
   if(count == 0) return;

   getArrayStr(planType, count, 1, 0, arrayStr); 
}

void sendPlanParamsFor3Planes(prescription* plans, int planType, char *arrayStr) {
   int i, count, compressMode, nslices = 1;
   planParam orient, theta, psi, phi, dim1, dim2, dim3,
        pos1, pos2, pos3, pss, thk, gap, ns;
   double d, value;
   int n;

   if(plans == NULL || plans->numOfStacks < 1) return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,1);

   n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   P_setstring(CURRENT, orient.name, "3orthogonal", 1);

   compressMode = getCompressMode(); // 1 is compressed, 2 is not compressed 

   count = 0;
   clearVar(theta.name); 
   clearVar(psi.name); 
   clearVar(phi.name); 
   clearVar(dim1.name); 
   clearVar(dim2.name); 
   //clearVar(dim3.name); 
   clearVar(pos1.name); 
   clearVar(pos2.name); 
   clearVar(pos3.name); 
   clearVar(thk.name); 
   clearVar(gap.name); 
   clearVar(ns.name); 
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
	count++;
        if(count>1)
        getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
           &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,count);

	value = floor(n*(plans->stacks[i].theta + d))/n;
        P_setreal(CURRENT, theta.name, value, count);
	value = floor(n*(plans->stacks[i].psi + d))/n;
        P_setreal(CURRENT, psi.name, value, count);
	value = floor(n*(plans->stacks[i].phi + d))/n;
        P_setreal(CURRENT, phi.name, value, count);
	value = floor(n*(plans->stacks[i].lpe + d))/n;
        P_setreal(CURRENT, dim1.name, value, 1);
	value = floor(n*(plans->stacks[i].lro + d))/n;
        P_setreal(CURRENT, dim2.name, value, 1);
	//value = floor(n*(plans->stacks[i].lpe2 + d))/n;
        //P_setreal(CURRENT, dim3.name, value, 1);
	value = floor(n*(plans->stacks[i].ppe + d))/n;
        P_setreal(CURRENT, pos1.name, value, 1);
	value = floor(n*(plans->stacks[i].pro + d))/n;
        P_setreal(CURRENT, pos2.name, value, 1);
	value = floor(n*(plans->stacks[i].pss0 + d))/n;
        P_setreal(CURRENT, pos3.name, value, 1);
	sendPss(pss.name, &(plans->stacks[i]), 1);
	value = floor(n*(plans->stacks[i].thk*10 + d))/n;
        P_setreal(CURRENT, thk.name, value, 1);
	value = floor(n*(plans->stacks[i].gap + d))/n;
        P_setreal(CURRENT, gap.name, value, 1);
  	if(compressMode == 2) 
          P_setreal(CURRENT, ns.name, 1, 1);
	else
          P_setreal(CURRENT, ns.name, plans->stacks[i].ns, 1);
	if(count == 1) nslices = plans->stacks[i].ns;

        sendPnew(&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
           &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,count);
      }
   }
   if(count == 0) return;

   getArrayStr(planType, count, nslices, 1, arrayStr);
}

int is3Plane(prescription* plans, int planType) {
   int i, count;
   if(planType != REGULAR) return 0;
   count = 0;
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
        count++;
      }
   }
   if(count == 3) return 1;
   else return 0; 
}

void sendPlanParamsForSlices(prescription* plans, int planType, char *arrayStr) {
   int i, stackCount, sliceCount, nslices, compressMode;
   planParam orient, theta, psi, phi, dim1, dim2, dim3,
        pos1, pos2, pos3, pss, thk, gap, ns;
   char orientStr[64]; 
   double d, value;
   int n;

   if(plans == NULL || plans->numOfStacks < 1) return;

   getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
        &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,1);

   n = getDecimal();
   d = 0.5/n; // d is half of the precision. If dec=2, precision=0.01, d = 0.005

   if(is3Plane(plans, planType)) {
	sendPlanParamsFor3Planes(plans, planType, arrayStr);
	return;
   }

   compressMode = getCompressMode(); // 1 is compressed, 2 is not compressed 

   stackCount = 0;
   sliceCount = 0;
   clearVar(theta.name); 
   clearVar(psi.name); 
   clearVar(phi.name); 
   clearVar(dim1.name); 
   clearVar(dim2.name); 
   //clearVar(dim3.name); 
   clearVar(pos1.name); 
   clearVar(pos2.name); 
   clearVar(pos3.name); 
   clearVar(thk.name); 
   clearVar(gap.name); 
   clearVar(ns.name); 
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) {
	stackCount++;

	if(stackCount == 1) { // should orient be arrayed??
	  getOrient(plans->stacks[i].theta, plans->stacks[i].psi, 
		plans->stacks[i].phi, orientStr);
   	  P_setstring(CURRENT, orient.name, orientStr, stackCount);
	} else getPlanParams(planType,&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
          &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,stackCount);

	value = floor(n*(plans->stacks[i].theta + d))/n;
        P_setreal(CURRENT, theta.name, value, stackCount);
	value = floor(n*(plans->stacks[i].psi + d))/n;
        P_setreal(CURRENT, psi.name, value, stackCount);
	value = floor(n*(plans->stacks[i].phi + d))/n;
        P_setreal(CURRENT, phi.name, value, stackCount);
	value = floor(n*(plans->stacks[i].lpe + d))/n;
        P_setreal(CURRENT, dim1.name, value, stackCount);
	value = floor(n*(plans->stacks[i].lro + d))/n;
        P_setreal(CURRENT, dim2.name, value, stackCount);
	//value = floor(n*(plans->stacks[i].lpe2 + d))/n;
        //P_setreal(CURRENT, dim3.name, value, stackCount);
	value = floor(n*(plans->stacks[i].ppe + d))/n;
        P_setreal(CURRENT, pos1.name, value, stackCount);
	value = floor(n*(plans->stacks[i].pro + d))/n;
        P_setreal(CURRENT, pos2.name, value, stackCount);
	value = floor(n*(plans->stacks[i].pss0 + d))/n;
        P_setreal(CURRENT, pos3.name, value, stackCount);
	sendPss(pss.name, &(plans->stacks[i]), sliceCount+1);
	value = floor(n*(plans->stacks[i].thk*10 + d))/n;
        P_setreal(CURRENT, thk.name, value, stackCount);
	value = floor(n*(plans->stacks[i].gap + d))/n;
        P_setreal(CURRENT, gap.name, value, stackCount);
  	if(compressMode == 2) 
          P_setreal(CURRENT, ns.name, 1, stackCount);
	else
          P_setreal(CURRENT, ns.name, plans->stacks[i].ns, stackCount);
	sliceCount += plans->stacks[i].ns; 
        updateSlab(planType);
        sendPnew(&orient,&theta,&psi,&phi,&dim1,&dim2,&dim3,
          &pos1,&pos2,&pos3,&pss,&thk,&gap,&ns,stackCount);

      }
   }
   if(stackCount == 0) return;

   nslices = sliceCount/stackCount; 
   getArrayStr(planType, stackCount, nslices, 0, arrayStr);
}

int hasType(prescription* plans, int planType) {
   int i;
   if(plans == NULL || plans->numOfStacks < 1) return 0;
   for(i=0; i<plans->numOfStacks; i++) {
      if(plans->stacks[i].planType == planType) return 1;
   }
   return 0;
}

void sendPlanParamsForType(prescription* plans, int planType, char *arrayStr) {
   if(!hasType(plans, planType)) return;
   int type = getOverlayType(planType);
   if(type == VOXEL) sendPlanParamsForVoxel(plans, planType, arrayStr);
   else if(type == SATBAND) sendPlanParamsForSatband(plans, planType, arrayStr);
   else if(type == VOLUME) sendPlanParamsForVolume(plans, planType, arrayStr);
   else if(type == REGULAR) sendPlanParamsForSlices(plans, planType, arrayStr);
}

void sendActiveOrient(prescription* plans, int act) {
   char orientStr[64];
//   char pnewString[64]; 
   planParams *tag;
   if(plans == NULL || plans->numOfStacks < 1) return;
   if(act < 0 || act >= plans->numOfStacks) return;
   getOrient(plans->stacks[act].theta, plans->stacks[act].psi, 
	plans->stacks[act].phi, orientStr);
   
   tag = getPlanTag(plans->stacks[act].planType);
   if(tag == NULL) return;

   P_setstring(CURRENT, tag->orient.name, orientStr, 1);
   // sprintf(pnewString,"1 %s",tag->orient.name); 
   // writelineToVnmrJ("pnew",pnewString); 
   appendJvarlist(tag->orient.name);
}

void sendPlanParams(prescription* plans) {
   int i;
   char arrayStr[MAXSTR];
   char str[MAXSTR];
   if(plans == NULL || plans->numOfStacks < 1) return;

   if(paramTags == NULL) initTags();

   strcpy(str,"");
   for(i=0; i<ntags; i++) {
      strcpy(arrayStr,"");
      sendPlanParamsForType(plans, paramTags[i].planType, arrayStr);
      if(strlen(arrayStr) > 0 && strlen(str) > 0) {
	strcat(str,",");
	strncat(str,arrayStr,strlen(arrayStr));
      } else if(strlen(arrayStr) > 0) strcpy(str,arrayStr);
   }
   setArray(str);
}

// this function is called by gplan('setDefaultType').
// it creates parameters for a given planType.
// iplanDefaultType, iplanType are created by gplan('setDefaultType').
// If a parameter is used, but not created by iplan (such as "slabctr"), 
// it is necessary to check existence of this parameter. 
void createPlanParams(int planType) {
//   char pnewString[64]; 
   vInfo paraminfo;
   planParams *tag = getPlanTag(planType);
   if(tag == NULL) return;

   if(tag->overlayType == VOLUME && P_getVarInfo(CURRENT, "slabctr", &paraminfo) == -2) {
	   P_creatvar(CURRENT, "slabctr", ST_STRING);
	   P_setstring(CURRENT, "slabctr", "y", 1);
   }
   if(strlen(tag->orient.name) > 0 && P_getVarInfo(CURRENT, tag->orient.name, &paraminfo) == -2) {
	   P_creatvar(CURRENT, tag->orient.name, ST_STRING);
	   P_setstring(CURRENT, tag->orient.name, "trans", 1);
           // sprintf(pnewString,"1 %s",tag->orient.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->orient.name);
   }
   if(strlen(tag->theta.name) > 0 && P_getVarInfo(CURRENT, tag->theta.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->theta.name, ST_REAL);
           P_setreal(CURRENT, tag->theta.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->theta.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->theta.name);
   }
   if(strlen(tag->psi.name) > 0 && P_getVarInfo(CURRENT, tag->psi.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->psi.name, ST_REAL);
           P_setreal(CURRENT, tag->psi.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->psi.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->psi.name);
   }
   if(strlen(tag->phi.name) > 0 && P_getVarInfo(CURRENT, tag->phi.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->phi.name, ST_REAL);
           P_setreal(CURRENT, tag->phi.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->phi.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->phi.name);
   }
   if(strlen(tag->dim1.name) > 0 && P_getVarInfo(CURRENT, tag->dim1.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->dim1.name, ST_REAL);
           P_setreal(CURRENT, tag->dim1.name, (double)10, 1);
           // sprintf(pnewString,"1 %s",tag->dim1.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->dim1.name);
   }
   if(strlen(tag->dim2.name) > 0 && P_getVarInfo(CURRENT, tag->dim2.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->dim2.name, ST_REAL);
           P_setreal(CURRENT, tag->dim2.name, (double)10, 1);
           // sprintf(pnewString,"1 %s",tag->dim2.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->dim2.name);
   }
   if(strlen(tag->dim3.name) > 0 && P_getVarInfo(CURRENT, tag->dim3.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->dim3.name, ST_REAL);
           P_setreal(CURRENT, tag->dim3.name, (double)10, 1);
           // sprintf(pnewString,"1 %s",tag->dim3.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->dim3.name);
   }
   if(strlen(tag->pos1.name) > 0 && P_getVarInfo(CURRENT, tag->pos1.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->pos1.name, ST_REAL);
           P_setreal(CURRENT, tag->pos1.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->pos1.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->pos1.name);
   }
   if(strlen(tag->pos2.name) > 0 && P_getVarInfo(CURRENT, tag->pos2.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->pos2.name, ST_REAL);
           P_setreal(CURRENT, tag->pos2.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->pos2.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->pos2.name);
   }
   if(strlen(tag->pos3.name) > 0 && P_getVarInfo(CURRENT, tag->pos3.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->pos3.name, ST_REAL);
           P_setreal(CURRENT, tag->pos3.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->pos3.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->pos3.name);
   }
   if(strlen(tag->pss.name) > 0 && P_getVarInfo(CURRENT, tag->pss.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->pss.name, ST_REAL);
           P_setreal(CURRENT, tag->pss.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->pss.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->pss.name);
   }
   if(strlen(tag->thk.name) > 0 && P_getVarInfo(CURRENT, tag->thk.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->thk.name, ST_REAL);
           P_setreal(CURRENT, tag->thk.name, (double)4, 1);
           // sprintf(pnewString,"1 %s",tag->thk.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->thk.name);
   }
   if(strlen(tag->gap.name) > 0 && P_getVarInfo(CURRENT, tag->gap.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->gap.name, ST_REAL);
           P_setreal(CURRENT, tag->gap.name, (double)0, 1);
           // sprintf(pnewString,"1 %s",tag->gap.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->gap.name);
   }
   if(strlen(tag->ns.name) > 0 && P_getVarInfo(CURRENT, tag->ns.name, &paraminfo) == -2) {
           P_creatvar(CURRENT, tag->ns.name, ST_REAL);
           P_setreal(CURRENT, tag->ns.name, (double)1, 1);
           // sprintf(pnewString,"1 %s",tag->ns.name);
           // writelineToVnmrJ("pnew",pnewString); 
           appendJvarlist(tag->ns.name);
   }
}
