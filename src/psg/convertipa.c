/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include "acodes.h"
#include "acqparms.h"
#include "abort.h"

extern int bgflag;

#define MAXSTRING	32

typedef struct ipalist {
struct ipalist	*nextitem;
int		index;
char		label[MAXSTRING];
double          value;
double          units;
int             min;
int             max;
int             type;
int             scale;
int             device;
codeint         counter;
} ipaList;

static ipaList *ipalisthead = NULL;


add_to_ipalist(label, value, units, min, max, type, scale, counter, device)
char           *label;
double          value;
double          units;
int             min;
int             max;
int             type;
int             scale;
int             device;
codeint         counter;
{
 ipaList *newipa, *curipa;
 int i;

   if (ipalisthead == NULL)
   {
	newipa = (ipaList *) malloc(sizeof(ipaList));
	if (newipa != NULL)
	   ipalisthead = newipa;
	else
	{
           abort_message("add_to_ipalist: Unable to create new item.\n");
	}
	newipa->index=0;
   }
   else
   {
	i = 1;
	curipa = ipalisthead;
	while(curipa->nextitem != NULL)
	{
	   i++;
	   curipa = curipa->nextitem;
	}
	newipa = (ipaList *) malloc(sizeof(ipaList));
	if (newipa != NULL)
	   curipa->nextitem = newipa;
	else
	{
           abort_message("add_to_ipalist: Unable to create new item.\n");
	}
	newipa->index=i;
   }
   strcpy(newipa->label,label);
   newipa->value = value;
   newipa->units = units;
   newipa->min = min;
   newipa->max = max;
   newipa->type = type;
   newipa->scale = scale;
   newipa->device = device;
   newipa->counter = counter;
   newipa->nextitem = NULL;

}

insertIPAcode(counter)
codeint counter;
{
   counter = counter*2;
   putcode(IPACODE);
   putcode(counter);
}

change_ipalocation(oldcounter,newcounter)
codeint	oldcounter,newcounter;
{
 ipaList *curipa;
   if (ipalisthead == NULL)
	return;

   curipa = ipalisthead;
   while((curipa->nextitem != NULL) && (curipa->counter != oldcounter))
   {
	curipa = curipa->nextitem;
   }
   if (curipa->counter == oldcounter)
   {
	curipa->counter = newcounter;
	if (bgflag)
	  fprintf(stderr,"change_ipalocation: oldcounter: %d newcounter: %d\n",
				oldcounter,newcounter);
   }
   else
   {
	if (bgflag)
	  fprintf(stderr,"Error oldcounter: %d \n",oldcounter);
   }
}

flush_ipalist(acqifile)
FILE *acqifile;
{
 ipaList *curipa;
   curipa = ipalisthead;
   while(curipa != NULL)
   {
	fprintf(acqifile,"%6s %g %g %d %d %d %d %d %d\n", 
		curipa->label, 
		curipa->value, 
		curipa->units, 
		curipa->min, 
		curipa->max, 
		curipa->type, 
		curipa->scale, 
		curipa->counter, 
		curipa->device);
	curipa = curipa->nextitem;
   }
   close(acqifile);

}
