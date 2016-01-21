/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "graphics.h"
#include "vnmrsys.h"
#include "wjunk.h"

static char displayOptPath[MAXSTR];

// given the id of a graphicas property defined in graphics.h,
// return the corresponding name used in DisplayOptions.xml.
int getOptName(int id, char *name) {
   switch (id) {
   case SPEC_LINE_MIN:
      strcpy(name,"SpectrumMin");
      return 1;
   case SPEC_LINE_MAX:
      strcpy(name,"SpectrumMax");
      return 1;
   case AXIS_LINE:
      strcpy(name,"Axis");
      return 1;
   case CUSOR_LINE:
      strcpy(name,"Cursor");
      return 1;
   case CROSSHAIR:
      strcpy(name,"Crosshair");
      return 1;
   case THRESHOLD_LINE:
      strcpy(name,"Threshold");
      return 1;
   case INTEG_LINE:
      strcpy(name,"IntegralLine");
      return 1;
   case INTEG_MARK:
      strcpy(name,"IntegralMark");
      return 1;
   case PEAK_MARK:
      strcpy(name,"PeakMark");
      return 1;
   case AXIS_LABEL:
      strcpy(name,"AxisLabel");
      return 1;
   case AXIS_NUM:
      strcpy(name,"AxisNum");
      return 1;
   case PEAK_LABEL:
      strcpy(name,"PeakLabel");
      return 1;
   case PEAK_NUM:
      strcpy(name,"PeakNum");
      return 1;
   case INTEG_LABEL:
      strcpy(name,"IntegralLabel");
      return 1;
   case INTEG_NUM:
      strcpy(name,"IntegralNum");
      return 1;
   case TEXT:
      strcpy(name,"GraphText");
      return 1;
   case AVPeakBox:
      strcpy(name,"AVPeakBox");
      return 1;
   case PHPeakBox:
      strcpy(name,"PHPeakBox");
      return 1;
   }
   return 0;
}

int getOptID(char *name, int *id) {
   if(!strcasecmp(name,"Spectrum1")) *id = SPEC_COLOR;
   else if(!strcasecmp(name,"Spectrum2")) *id = SPEC2_COLOR;
   else if(!strcasecmp(name,"Spectrum3")) *id = SPEC3_COLOR;
   else if(!strcasecmp(name,"Spectrum4")) *id = SPEC4_COLOR;
   else if(!strcasecmp(name,"Spectrum5")) *id = SPEC5_COLOR;
   else if(!strcasecmp(name,"Spectrum6")) *id = SPEC6_COLOR;
   else if(!strcasecmp(name,"Spectrum7")) *id = SPEC7_COLOR;
   else if(!strcasecmp(name,"Spectrum8")) *id = SPEC8_COLOR;
   else if(!strcasecmp(name,"Spectrum9")) *id = SPEC9_COLOR;
   else return 0;

   return 1;
}

void readDisplayOpt(char *key1, char *key2, char *value) {
   FILE* fp;
   char buf[MAXSTR], key[MAXSTR];
   sprintf(key,"%s%s",key1,key2);

   if(displayOptPath == NULL || strstr(displayOptPath,"Graphics") == NULL) 
      sprintf(displayOptPath,"%s/persistence/Graphics",userdir);

   if((fp = fopen(displayOptPath, "r"))) {
      while (fgets(buf,sizeof(buf),fp)) 
	if(strstr(buf, key) == buf) {
	  char *ptr = buf + strlen(key);
	  strcpy(value,ptr);	
          fclose(fp);
	  return;
	} 
      strcpy(value,"");	
      fclose(fp);
   } 
     
}

void G_getThickness(char *key, int *th) {
   char value[MAXSTR];
   readDisplayOpt(key, "Thickness", value);
   if(strlen(value)>0) *th = atoi(value);
   *th=0;
}

void G_getSize(char *key, int *size) {
   char value[MAXSTR];
   readDisplayOpt(key, "Size", value);
   if(strlen(value)>0) *size = atoi(value);
   else *size=0;
}

void G_getStyle(char *key, char *style) {
   readDisplayOpt(key, "Style", style);
}

void G_getFont(char *key, char *font) {
   readDisplayOpt(key, "Font", font);
}

void G_getColor(char *key, char *color) {
   readDisplayOpt(key, "Color", color);
}

double G_getCharSize(char *key) {
  int size;
  G_getSize(key, &size);
  return (double)size*0.7/(double)12;
}

