/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <limits.h>
#include <float.h>
#include "aipSpecData.h"
/*
#include "aipCommands.h"
using namespace aip;
*/

SpecData::SpecData(string newKey, string fpath, DDLSymbolTable *newSt, specStruct_t *newSs) {
   path = fpath;
   key = newKey;
   st = newSt;
   specStruct = newSs;
   label="";
   color="";
}

bool SpecData::fillSpecStruct(DDLSymbolTable *newSt, specStruct_t *newSs) {
 
  bool ok = true;
  int i;
  char *str;

  if (!newSt->GetValue("rank", newSs->rank)) {
     ok = false;
     Werrprintf("No \"rank\" in FDF header\n");
  }

  if(ok)
  for(i=0; i<newSs->rank; i++) {
     newSs->matrix[i] = 1;
     if (!newSt->GetValue("matrix", newSs->matrix[i], i)) {
        ok = false;
        Werrprintf("No \"matrix[%d]\" in FDF header\n",i);
     }
  }

  if (!newSt->GetValue("spec_data_rank", newSs->spec_data_rank)) {
     ok = false;
     Werrprintf("No \"spec_data_rank\" in FDF header\n");
  }

  if (!newSt->GetValue("spec_display_rank", newSs->spec_display_rank)) {
     ok = false;
     Werrprintf("No \"spec_display_rank\" in FDF header\n");
  }

  if(ok && newSs->spec_display_rank > newSs->spec_data_rank)
	newSs->spec_display_rank = newSs->spec_data_rank;

  if (!newSt->GetValue("bits", newSs->bits)) {
     ok = false;
     Werrprintf("No \"bits\" in FDF header\n");
  }
  
  if (!newSt->GetValue("storage", str)) {
     ok = false;
     Werrprintf("No \"storage\" in FDF header\n");
  } else strcpy(newSs->storage, str);
  
  if (!newSt->GetValue("fidpath", str)) {
     strcpy(newSs->fidpath,"");
  } else strcpy(newSs->fidpath, str);
  
  if (!newSt->GetValue("type", str)) {
     strcpy(newSs->type,"absval");
  } else strcpy(newSs->type, str);
  
  if (!newSt->GetValue("dataType", str)) {
     strcpy(newSs->dataType,"spectrum");
  } else strcpy(newSs->dataType, str);
  
  
  if(ok)
  for(i=0; i<newSs->spec_data_rank; i++) {
     newSs->spec_matrix[i] = 0;
     if (!newSt->GetValue("spec_matrix", newSs->spec_matrix[i], i)) {
        ok = false;
        Werrprintf("No \"spec_matrix[%d]\" in FDF header\n",i);
     }
  }

  if(ok)
  for(i=0; i<newSs->spec_display_rank; i++) {

     if (!newSt->GetValue("sfrq", newSs->sfrq[i], i)) {
        newSs->sfrq[i] = 0;
     }

     if (!newSt->GetValue("reffrq", newSs->reffrq[i], i)) {
        newSs->reffrq[i] = 1.0;
	if(newSs->reffrq[i] < 1.0) newSs->reffrq[i] =1.0;
     }

     if (!newSt->GetValue("nucleus", newSs->nucleus[i], i)) {
	strcpy(newSs->nucleus[i], "H1");
     }

     if (!newSt->GetValue("sw", newSs->sw[i], i)) {
	newSs->sw[i] = newSs->spec_matrix[i];
     }

     if (!newSt->GetValue("upfield", newSs->upfield[i], i)) {
        newSs->upfield[i] = 0;
     }

     if (!newSt->GetValue("sp", newSs->sp[i], i)) {
        newSs->sp[i] = newSs->upfield[i];
     }

     if (!newSt->GetValue("wp", newSs->wp[i], i)) {
        newSs->wp[i] = newSs->sw[i];
     }

     if (!newSt->GetValue("rp", newSs->rp[i], i)) {
        newSs->rp[i] = 0;
     }

     if (!newSt->GetValue("lp", newSs->lp[i], i)) {
        newSs->lp[i] = 0;
     }
  }

   // note, if get here, st data is good.
   newSs->data = newSt->GetData();

   return ok;
}

SpecData::~SpecData() {

  if (specStruct->data) {
	// Only delete the data if the symtab is not going to delete the same data.
	if(st->GetData() != specStruct->data) delete[] specStruct->data;
  }
  if(specStruct) delete specStruct;

  if (st) st->Delete(); 
}

string SpecData::getDataType() {
   return string(specStruct->dataType); 
}

// ind start from zero
float *SpecData::getTrace(int ind, int npts) {

   float *data = specStruct->data;
   if(!data) {
	Winfoprintf("%s: No data!", key.c_str());
	return NULL;
   }

   int ntraces = getNumTraces();
   int np = specStruct->matrix[0];

   if(ind > ntraces) {
	Winfoprintf("Index %d out of range (1-%d)!", ind, ntraces);
	return NULL; 
   }
   if(np != npts) {
	Winfoprintf("Data size %d does not match %d!", npts, np);
	return NULL; 
   }

   return &(data[(ind)*np]);

}

void SpecData::getDataMax(float  *datapntr, int npnt, float  *max) {
   float        tmp, maxval;

  maxval = -0.1*FLT_MAX;
  while (--npnt)
  {
    tmp = *datapntr++;
    if (tmp > maxval) maxval = tmp;
  }
  *max = maxval;
}

void SpecData::getDataMin(float  *datapntr, int npnt, float  *min) {
   float        tmp, minval; 

  minval = FLT_MAX;
  while (--npnt)
  {
    tmp = *datapntr++;
    if (tmp < minval) minval = tmp;
  }
  *min = minval;
}

int SpecData::getNumTraces() {
   if(specStruct == NULL || specStruct->data == NULL || specStruct->rank <  1)
	return 0;
   int ntrace = 1;
   for(int i=1; i<specStruct->rank; i++)
	ntrace *= specStruct->matrix[i];   
   return ntrace;
}
