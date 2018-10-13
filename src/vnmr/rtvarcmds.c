/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------*/
/* File rtvarcmds.c:  							*/
/*	Contains routines that implement changing rt variable values	*/
/*	from vnmr.  					 		*/
/*----------------------------------------------------------------------*/

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <search.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HASH_RTVARMAX 	1000	/* max number of expected rtvars for	*/
				/* short term.  Currently have ~100.	*/
				/* Current possible is 32K.		*/

#define OK	0
#define ERROR	0

struct info {
	int  index, max, min;
};

static int max_rtvars = 0;
static FILE *rtvarfile = 0;
static char *rtvarname_ptr = NULL;
static struct info *info_ptr = NULL;
static int rtvarinfo_exist = 0;
static char *default_rtvarfile = "/vnmr/acqqueue/rtvars.IPA";

/* extern void *memset(); */

int init_rtvars(char *filename, int number)
{
   int i,rtvar_value;
   ENTRY item;
   char *str_ptr;

   /* Initialize data areas */
   rtvarname_ptr = (char *)malloc(number*20);
   if (rtvarname_ptr == NULL) return (0);
   memset(rtvarname_ptr, 0, (number*20));

   info_ptr = (struct info *)malloc(sizeof(struct info)*number);

   /* Open rtvar info file */
   rtvarfile = fopen(filename,"r");
   if (rtvarfile == NULL)
   {  fprintf(stderr,"init_rtvars: cannot open %s\n",filename);
      return(0);			/* return failure */
   }

   /* create table */
   i = 0;
   str_ptr = rtvarname_ptr;
   hdestroy();			/* first destroy any previous table */
   (void) hcreate(number);
   while (fscanf(rtvarfile,"%s%d%d", str_ptr, &rtvar_value,
           &info_ptr->index) != EOF && i++ < number) {
              /* put info in structure, and structure in item */
         item.key = str_ptr;
         item.data = (void *)info_ptr;
         str_ptr += strlen(str_ptr) + 1;
         info_ptr++;
   	 /* put item into table */
         (void) hsearch(item, ENTER);
   }
   fclose(rtvarfile);
   if (i < number){
	 max_rtvars = i;
   }
   else {
	hdestroy();
	return(-1);
   }
   rtvarinfo_exist = 1;
   return(i);
}

int rtn_index(char *search_name)
{
   ENTRY item,*found_item;
   /* fprintf(stderr,"rtn_index: %s\n",search_name); */
   if (!rtvarinfo_exist)
   {
	if (init_rtvars(default_rtvarfile,HASH_RTVARMAX) <= 0)
	{
	   (void)printf("Default rtvar init failed.\n");
	   return(-1);
	}
   }
   /* fprintf(stderr,"search rtn_index: %s\n",search_name); */
   item.key = search_name;
   if ((found_item = hsearch(item, FIND)) != NULL) {
	return(((struct info *)found_item->data)->index);
   } else {
	(void)printf("No rtvar name found %s\n",search_name);
	return(-1);
   }

}
