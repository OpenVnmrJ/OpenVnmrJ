/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspCursor.h"

spAspCursor_t nullAspCursor = spAspCursor_t(NULL);

AspCursor::AspCursor(double f, string name) {
   rank=1;
   resonances = new aspResonance_t[1];
   resonances[0].name=string(name);
   resonances[0].freq=f; 
   resonances[0].assignedName="-";
}

AspCursor::AspCursor(double f1, double f2, string name1, string name2) {
   rank=2;
   resonances = new aspResonance_t[2];
   resonances[0].name=string(name1);
   resonances[0].freq=f1; 
   resonances[0].assignedName="-";
   resonances[1].name=string(name2);
   resonances[1].freq=f2; 
   resonances[1].assignedName="-";
}

AspCursor::~AspCursor() {
   delete[] resonances;
}

string AspCursor::toString() {
   char str[MAXSTR];
   if(rank==1) { 
      sprintf(str, "1 %s %s %f",resonances[0].name.c_str(), resonances[0].assignedName.c_str(),
	resonances[0].freq); 
   } else if(rank==2) {
      sprintf(str, "2 %s %s %s %s %f %f",resonances[0].name.c_str(), 
	resonances[1].name.c_str(), resonances[0].assignedName.c_str(),
	resonances[1].assignedName.c_str(), resonances[0].freq, resonances[1].freq); 
   }
   return string(str);
}
