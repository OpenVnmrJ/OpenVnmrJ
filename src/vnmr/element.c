/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------------
|	element.c
|
|	This module contains routine to maintain a set of elements.
|
+----------------------------------------------------------------------------*/

#define CANVAS_OFFSET	4

#include "vnmrsys.h"
#include <stdio.h>
#include <string.h>

#include "element.h"
#include "tools.h"
#include "allocate.h"
#include "wjunk.h"

extern int    Tflag;
int           ElementBotHead;
int           ElementTopHead;
int           terminalElementActive = 0;

#ifndef VNMRJ
static Elist *displayElist;
static int    Cols;
static int    colGapWidth;
static int    displayLongest;  /* longest element lenght in element list */
static int    displayNumberOfElements;/* number of elements*/
static int    displayNumberElementPerScreen;
static int    displayNumberOfFields;/* number of fields*/
static int    displayScreen;
static int    Rows;
static int    rowGapHeight;
static int    startingElement;
static int    totalRows;
#endif

extern int      graf_height, graf_width;
extern int      xcharpixels, ycharpixels;
/*---------------------------------------------------------------------
|	initElementStruct
|
|	This routine initializes the element structure. It returns
|       null if there is a problem.
+--------------------------------------------------------------------*/
Elist *initElementStruct()
{  Elist *eptr;	/* pointer to element list packet */
   
   if ( (eptr = (Elist *)allocateWithId(sizeof(Elist),"initElementStruct")) )
   {  if (Tflag)
         fprintf(stderr,"initElementStruct: initializing element struct\n"); 
      eptr->firstElement = NULL;
      eptr->lastElement = NULL;
      eptr->size = 0;
      return(eptr);
   }
   else
   {  fprintf(stderr,"initElementStruct: out of memory (help!!)\n");
      return(NULL);
   }
}

/*---------------------------------------------------------------------
|	addElement
|
|	This routine adds an element to the Elist
+--------------------------------------------------------------------*/

int addElement(elist,name,info)	Elist *elist; char *name,*info;
{  Element *Aeptr;
   Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"addElement: add element %s\n",name);      
   if (elist) /* if elist exists, create element */ 
   {  if ( (eptr = (Element *)allocateWithId(sizeof(Element),"addElement")) )
      {  eptr->next = NULL;
	 if (name)
	 {  if ( (eptr->name = allocateWithId(strlen(name)+1,"addElement")) )
	       strcpy(eptr->name,name);
            else
	    {  Werrprintf("addElement:out of memory");
	       return(0);
	    }
	 }
	 else
	    eptr->name = NULL;
	 if (info)
	 {  if ( (eptr->info = allocateWithId(strlen(info)+1,"addElement")) )
	       strcpy(eptr->info,info);
	    else
	    {  Werrprintf("addElement:out of memory");
	       return(0);
	    }
	 }
	 else
	    eptr->info = NULL;
         eptr->marked = 0;
	 elist->size++;
	 /*  attach the element */
	 if ( (Aeptr = elist->lastElement) ) /* is there a last element */
	 {  Aeptr->next = eptr;
	    eptr->prev = Aeptr;
	    elist->lastElement = eptr;
	 }
	 else  /* make this the first and last Element */
	 {  elist->firstElement = eptr;
	    elist->lastElement  = eptr;
	    eptr->prev = NULL;
	 }
         return(1);
      }
      else
      {  fprintf(stderr,"addElement: out of memory\n");
         return(0);
      }
   }
   else
   {  fprintf(stderr,"addElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	remove element by element pointer
|
|	This routine removes an element from the Elist
+--------------------------------------------------------------------*/

int removeElement(Elist *elist, Element *eptr)
{
   if (Tflag)
      fprintf(stderr,"removeElement: remove element \n");      
   if (elist)
   {  if (elist->firstElement == eptr) /* is this the first element */
      {  elist->firstElement = eptr->next;
	 if (elist->lastElement == eptr) /* is this the last element */
	    elist->lastElement = NULL;
	 else
	    eptr->next->prev = NULL;
      }
      else  /* this is an element other than first element */
      {  eptr->prev->next = eptr->next;
         if (eptr->next)
            eptr->next->prev = eptr->prev;
         if (elist->lastElement == eptr) /* is this the last element */
	    elist->lastElement = eptr->prev;
      }
      /* release all space */
      if (eptr->info)
	 release(eptr->info);
      if (eptr->name)
	 release(eptr->name);
      release(eptr);  /*  free the space */
      elist->size--;
      return(1);
   }
   else
   {  fprintf(stderr,"removeElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	get number of selected elements
|
|	This routine determines the number of selected elements.
+--------------------------------------------------------------------*/
int getNumberOfSelect(Elist *elist)
{  Element *eptr;   
   int      theNum;
   
   theNum = 0;
   if (Tflag)
      fprintf(stderr,"getNumberOfSelect: starting\n");      
   if (elist)
   {  eptr = elist->firstElement;  
      while (eptr)
      {  if (eptr->marked)
	   theNum++; 
         eptr = eptr->next;
      }
      if (Tflag)
         fprintf(stderr,"getNumberOfSelect: num=%d\n",theNum);      
      return theNum;
   }
   else
   {  fprintf(stderr,"getNumberOfSelect: Elist has not been initialized\n");
      return 0;
   }
}

/*---------------------------------------------------------------------
|	get first selection
|
|	This routine finds the first selection (if any), it 
|       returns a pointer to the name, or null if no selection.
+--------------------------------------------------------------------*/
char *
getFirstSelection(elist)	Elist *elist;  
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"getFirstSelection: starting\n");      
   if (elist)
   {  eptr = elist->firstElement;  
      while (eptr)
      {  if (eptr->marked)
	    return(eptr->name);
         eptr = eptr->next;
      }
      return(NULL);
   }
   else
   {  fprintf(stderr,"getFirstSelection: Elist has not been initialized\n");
      return(NULL);
   }
}

/*---------------------------------------------------------------------
|	get next selection
|
|	This routine finds the next selection (if any), it 
|       returns a pointer to the name, or null if no selection.
|       The previous selection element name must be passed to this
|       routine.  Hopefully it is still there or this routine will
|       return with a null.
+--------------------------------------------------------------------*/
char *
getNextSelection(elist,prev)	Elist *elist;  char *prev;
{  Element         *eptr;   
   extern  Element *findElement();
   
   if (Tflag)
      fprintf(stderr,"getNextSelection: look for marked after %s\n",prev);      
   if (elist && prev)
   {  /* first find the pointer to the previous selection */
      if ( (eptr = findElement(elist,prev)) )
      {  eptr = eptr->next; /* start at next element */
         while (eptr)
         {  if (eptr->marked)
	       return(eptr->name);
            eptr = eptr->next;
         }
         return(NULL);
      }
      else
      {  if (Tflag)
	    fprintf(stderr,"getNextSelection: found no pointer to %s\n",prev);
         return NULL;
      }
   }
   else
   {  fprintf(stderr,"getNextSelection:Elist or prev not initialized\n");
      return(NULL);
   }
}

/*---------------------------------------------------------------------
|	find element
|
|	This routine finds an element in the Elist
+--------------------------------------------------------------------*/
Element *
findElement(elist,name)	Elist *elist;  char *name; 
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"findElement: find element %s\n",name);      
   if (elist)
   {  eptr = elist->firstElement;  
      while (eptr)
      {  if ( ! strcmp(eptr->name,name) )
	    return(eptr);
         eptr = eptr->next;
      }
      return(NULL);
   }
   else
   {  fprintf(stderr,"findElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	getLongLen
|
|	This routine finds the length of the  element with the 
|	longest name. 
+--------------------------------------------------------------------*/
int getLongLen(elist)	Elist *elist;  
{  Element *eptr;   
   int      curlen, maxlen;

   maxlen = 0;
   if (elist)
   {  eptr = elist->firstElement;  
      while (eptr)
      {
         curlen = strlen(eptr->name);
         if (strcmp(eptr->info,"d") == 0)
           curlen++;
         if (curlen > maxlen)
            maxlen = curlen;
         eptr = eptr->next;
      }
      if (Tflag)
         fprintf(stderr,"getLongLen: longest element is %d\n",maxlen);
      return(maxlen);
   }
   else
   {  fprintf(stderr,"getLongLen: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	release element list
|
|	This routine does everything necessary to release an element list
+--------------------------------------------------------------------*/
int releaseElementStruct(elist)	Elist *elist;  
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"releaseElementStruct:\n");      
   if (elist)
   {  eptr = elist->firstElement;  
      while (eptr)
      {  /* release all space */
         if (eptr->info)
	    release(eptr->info);
         if (eptr->name)
	    release(eptr->name);
         release(eptr);  
         eptr = eptr->next;
      }
      release(elist);
      return(1);
   }
   else
   {  fprintf(stderr,"releaseElementStruct: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	show elements in elist
|
|	This routine shows the variables in the elist
+--------------------------------------------------------------------*/
int showElement(Elist *elist)
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"showElement: show elements\n");      
   if (elist)
   {  printf("There are %d elements in the list\n",elist->size); 
      eptr = elist->firstElement;  
      while (eptr)
      {  if (eptr->name)
	    printf("'%s'  ",eptr->name);
         else
	    printf("Element has no name\n");
         if (eptr->info)
	    printf("info is '%s'\n",eptr->info);
	 else
	    printf("  has no info\n");
         eptr = eptr->next;
      }
      return(1);
   }
   else
   {  fprintf(stderr,"showElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	subtract element by name
|
|	This routine subtracts an element in the Elist by name
+--------------------------------------------------------------------*/
int subElement(Elist *elist, char *name)
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"subElement: sub element %s\n",name);      
   if (elist)
   {  if ( (eptr = findElement(elist,name)) )
      {  removeElement(elist,eptr);
	 return(1);
      }
      return(0);
   }
   else
   {  fprintf(stderr,"subElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	addinfo to element
|
|	This routine adds info to an element in elist
+--------------------------------------------------------------------*/
int addInfoToElement(Elist *elist, char *name, char *info)
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"addInfoToElemenent: element %s info %s\n",name,info);      
   if (elist)
   {  if ( (eptr = findElement(elist,name)) )
      {  if (eptr->info) /* if info is aready there */
            release(eptr->info);
	 if (info)
	 {  eptr->info = allocateWithId(strlen(info),"addInforToElement");
	    strcpy(eptr->info,info);
	 }
	 else
	    eptr->info = NULL;
	 return(1);
      }
      return(0);
   }
   else
   {  fprintf(stderr,"addInfoToElement: Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	getinfo from element
|
|	This routine gets info form an element in elist
+--------------------------------------------------------------------*/
char *
getInfoFromElement(elist,name)  Elist *elist; char *name;  
{  Element *eptr;   
   
   if (Tflag)
      fprintf(stderr,"getInfoFromElement: element %s\n",name);      
   if (elist)
   {  if ( (eptr = findElement(elist,name)) )
      {  if (eptr->info) /* if info is there */
            return(eptr->info);
	 else
	    return(NULL);
      }
      return(NULL);
   }
   else
   {  fprintf(stderr,"getInfoFromElement: Elist has not been initialized\n");
      return(NULL);
   }
}

/*---------------------------------------------------------------------
|	getNumberOfElements
|
|	This routine returns the number of elements in a list
+--------------------------------------------------------------------*/
int getNumberOfElements(Elist *elist)
{
   
   if (Tflag)
      fprintf(stderr,"getNumberOfElements: ");      
   if (elist)
   {  if (Tflag)
	 fprintf(stderr,"%d\n",elist->size);
      return(elist->size);  
   }
   else
   {  fprintf(stderr," Elist has not been initialized\n");
      return(0);
   }
}

/*---------------------------------------------------------------------
|	displayElements
|
|	This routine displays the elements on the canvas or GraphOn.
|	It first determines the width and length of screen and
|	the length of the longest element name.  It then determines where
|	to place the element names on the canvas.  If all the names
|	cannot fit into the screen, it displays all that it can
|	and uses the last row to indicate more element names exists.
|	Clicking on the last row, displayes new element names.  The
|	first row then contains a string to indicate there are previous
|	element names.  Clicking on the first row displays the previous
|	element names.   This routine also enables the mouse to
|	select and click on names.
+--------------------------------------------------------------------*/
#ifndef VNMRJ
static int _displayElements(Elist *elist, int screen)
{  Element *eptr;   
#ifdef SUN
   extern   ButSelect();
#endif 
   int      displayRows;
   int      endingElement;
   int      i;
   int      longest;
   int      numberElementPerScreen;
   int      numberOfElements;
   int      numberOfFields;
   int      offset;
   int      rowGap,colGap;
   int      startingRows;
   char     display_name[ MAXPATHL ];
   
   if (Tflag)
      fprintf(stderr,"displayElements: screen=%d\n",screen);      
   if (elist)
   {

/* Use setdisplay to verify all parts of program agree on the
   size of the display.  Corrects a problem on the Sun with
   files and large and files and small.				*/

      setdisplay();
      WblankCanvas();
      longest = getLongLen(elist);
      numberOfElements = getNumberOfElements(elist);

      /* Calculate Rows and Cols on the screen */

#ifdef SUN
      if (Wissun())
      {
         Rows = graf_height / ycharpixels;
         Cols = graf_width / xcharpixels;
/*
	  Rows = (int)window_get(canvas,WIN_ROWS);
         Cols = (int)window_get(canvas,WIN_COLUMNS);
*/
      }
      else
#endif 
      {  Rows = WscreenSize() + 1;
	 Cols = 80;
      }
#ifdef SUN
      if (Wissun())
      { 
         rowGapHeight = ycharpixels;
         rowGap = 0;

         colGapWidth  = xcharpixels;
         colGap = 0;

/*       rowGapHeight = (int) window_get(canvas,WIN_ROW_HEIGHT) +
			(int) window_get(canvas,WIN_ROW_GAP);
         rowGap =  (int) window_get(canvas,WIN_ROW_GAP);
 
         colGapWidth  = (int) window_get(canvas,WIN_COLUMN_WIDTH) +
			(int) window_get(canvas,WIN_COLUMN_GAP);
         colGap = (int) window_get(canvas,WIN_COLUMN_GAP);
*/
      }
      else
#endif 
      if (Wisgraphon())
      {  rowGapHeight = 16;
         rowGap = 3;
	 colGapWidth = 13;
	 colGap = 3;
      }
      else if (Wistek4x07())
      {
         rowGapHeight = 15;
         colGapWidth  = 8;
      }
      if (Tflag)
	 fprintf(stderr,"displayElement: rowGap=%d rowGapHeight=%d colGap=%d colGapWidth=%d\n",
	    rowGap,rowGapHeight,colGap,colGapWidth);
      /* calculate number of column fields */
      /* avoid numberOfFields being set to 0 by integer divide */
      if (longest > Cols)
      {
         longest = Cols;
      }
      numberOfFields = (Cols+1)/(longest+1);
      /* calculate number of total rows */
      totalRows = ((numberOfElements-1)/numberOfFields) + 1;
      if (Tflag) 
         fprintf(stderr,"displayElements:Rows=%d Cols=%d #fields=%d #totalrows=%d\n", 
                       Rows, Cols, numberOfFields, totalRows); 
      
      /* determine how many rows on screen and if there is a bottom 
	 and/or top header to scroll forward or back */
      if (screen == 1)
      {  if (totalRows <= Rows)
         {  ElementTopHead = ElementBotHead = 0;
	    displayRows = Rows;
         }
         else
         {  ElementTopHead = 0;  ElementBotHead = 1;
	    displayRows = Rows-1;
         }
      }
      else
      {  if (totalRows <= (screen * Rows)-(2*(screen-1)))
	 {  displayRows = Rows - 1;
	    ElementTopHead = 1;  ElementBotHead = 0;
	 }
	 else
	 {  displayRows = Rows - 2;
	    ElementTopHead = ElementBotHead = 1;
	 }
      }
      numberElementPerScreen =  displayRows * numberOfFields;
      if (screen == 1)
	 startingElement = 0; /* of course */
      else /* NOT SO OBVIOUS */
      {  startingRows = ((screen-1) * Rows) - (2*(screen-2));
	 startingElement = (startingRows -1)*numberOfFields;
      }
      endingElement   = startingElement + numberElementPerScreen;
      if (Tflag)
	 fprintf(stderr,"displayElements:Startel = %d Endel = %d els per screen=%d total ele=%d\n"
	 ,startingElement, endingElement,numberElementPerScreen,numberOfElements);

      /*  skip over elements */
      eptr = elist->firstElement;
      for (i=0 ; i<startingElement; i++)
      {  if (Tflag)
	    fprintf(stderr,"displayElements:skipping over %s\n",eptr->name);
	 eptr = eptr->next;
      }

      if (ElementTopHead)  /* make room for top header */
	 offset = numberOfFields;
      else
	 offset = 0;

      /* display elements */
      /* On the SunView canvas, the first column is numbered 0;
         on the display terminals the first column is numbered 1.  */

#ifdef SUN
      if (Wissun())
      {
         for (i=0 ; i<numberElementPerScreen && eptr; i++)
         {  int row,col;
	 
	    row = ((i+offset)/numberOfFields) + 1;
	    col = ((i+offset)%numberOfFields) * (longest+1);
	    strcpy( &display_name[ 0 ], eptr->name );
	    if (strcmp(eptr->info, "d") == 0)
	      strcat( &display_name[ 0 ],  "/" );
	    if (Tflag)
	       fprintf(stderr,"displayElements:name=%s row=%d col=%d\n"
	       ,eptr->name,row,col);
	    if (eptr->marked) /* display in inverse video */
	       Inversepw_text(
			 col*colGapWidth, row*rowGapHeight-CANVAS_OFFSET,
			&display_name[ 0 ]);
	    else
		dispw_text(col*colGapWidth, row*rowGapHeight - CANVAS_OFFSET,
			&display_name[ 0 ]);
	    eptr = eptr->next;
         }
      }
      else
#endif 
      if (Wisgraphon() || Wistek())
      {  for (i=0 ; i<numberElementPerScreen && eptr ; i++)
         {  int row,col;
	 
	    row = ((i+offset)/numberOfFields) + 1;
	    col = ((i+offset)%numberOfFields) * (longest+1) + 1;
	    if (Tflag)
	       fprintf(stderr,"displayElements:name=%s row=%d col=%d\n"
	       ,eptr->name,row,col);
	    strcpy( &display_name[ 0 ], eptr->name );
	    if (strcmp(eptr->info, "d") == 0)
	      strcat( &display_name[ 0 ],  "/" );
	    if (eptr->marked) /* display in inverse video */
	       Wprintfpos(4,row,col,"%s",&display_name[ 0 ] );
	     else
	       Wprintfpos(4,row,col,"%s",&display_name[ 0 ] );
	     eptr = eptr->next;
         }
      }
      displayScreen = screen; /* this is used for the Select routine */
      displayElist  = elist;  /* this is used for the Select routine */
      displayLongest = longest;
      displayNumberOfElements = numberOfElements;
      displayNumberElementPerScreen = numberElementPerScreen;
      displayNumberOfFields = numberOfFields;
      if (ElementTopHead)
      {
#ifdef SUN
	 if (Wissun())
            Inversepw_text(15*colGapWidth,rowGapHeight-CANVAS_OFFSET,
	        "Click HERE for previous elements");
	 else
#endif 
	    if (Wisgraphon() || Wistek())
               Wprintfpos(4,1,15,"\033[7m%s\033[27m",
	       "Click HERE for previous elements"); 
      }
      if (ElementBotHead)
      {
#ifdef SUN
	 if (Wissun())
            Inversepw_text(15*colGapWidth,Rows*rowGapHeight-CANVAS_OFFSET,
	        "Click HERE for more elements");
         else
#endif 
	    if (Wisgraphon() || Wistek())
               Wprintfpos(4,WscreenSize() + 1,15,"\033[7m%s\033[27m",
		   "Click HERE for more elements");
      }
#ifdef SUN
      if (Wissun())
         Wactivate_mouse(NULL,ButSelect,NULL);
      else
#endif 
	 if (Wisgraphon() || Wistek())
	    terminalElementActive = 1;
      return(1);
   }
   else
   {  fprintf(stderr,"displayElements: Elist has not been initialized\n");
      return(0);
   }
}
#endif 

void displayElements(Elist *elist)
{
#ifndef VNMRJ
   _displayElements(elist,1); 
#endif 
}


/*----------------------------------------------------------------------
|       sort elements by name
|
|       This routine sorts the elements by name.
+------------------------------------------------------------------------*/
int sortElementByName(Elist *elist)
{  Element *eptri;   
   Element *eptrj;   
   Element  tempelement;

   if (Tflag)
      fprintf(stderr,"sortElementByName:\n");
   if (elist)
   {  if ( (eptri = elist->firstElement) )
      {  while (eptri) 
         {  eptrj = eptri->next;
            while (eptrj)
            {  if (strcmp(eptri->name,eptrj->name) > 0)
               {  tempelement.info   = eptri->info;
                  tempelement.name   = eptri->name;
                  tempelement.marked = eptri->marked;
	          eptri->info        = eptrj->info;
	          eptri->name        = eptrj->name;
	          eptri->marked      = eptrj->marked;
	          eptri->info        = eptrj->info;
	          eptrj->info        = tempelement.info;
	          eptrj->name        = tempelement.name;
	          eptrj->marked      = tempelement.marked;
	       }
               eptrj = eptrj->next;
            }
            eptri = eptri->next;
         } 
      }
   }   
   else
   {  fprintf(stderr,"sortElementByName:\n");
      return(0);
   }
   return(0);
}

#ifndef VNMRJ
/*----------------------------------------------------------------------
|       Select button routine
|
|       This routine, called when a mouse button is clicked,
|       determines which element number has been clicked, inverts
|	the element name and sets that element as selected.
+------------------------------------------------------------------------*/
int
ButSelect(but,updown,x,y)  int but,updown,x,y;
{  char     dummy[256];
   int      elementIndex;
   int      entry_len;
   int      i;
   int      index;
   int      nx,ny;
   int      xindex;
   int      yindex;
   Element *eptr;	/* pointer to element list packet */

   if (updown)
   {  if (Tflag)
	 fprintf(stderr,"ButSelect: but pushed x=%d y=%d rGH*R=%do\n"
		,x,y,rowGapHeight*Rows);
      /*  Determine if we have clicked in a top or bottom header */
      if (ElementTopHead)
         if (y <= rowGapHeight - CANVAS_OFFSET)
	 {  if (Tflag)
	       fprintf(stderr,"ButSelect: calling prev screen display screen %d\n",
			     displayScreen-1);
            _displayElements(displayElist,displayScreen-1);
	    return;
         }
      if (ElementBotHead)
         if (y >= rowGapHeight * (Rows-1) - CANVAS_OFFSET)
	 {  if (Tflag)
	       fprintf(stderr,"ButSelect:calling next screen display screen %d\n",
			     displayScreen+1);
            _displayElements(displayElist,displayScreen+1);
	    return;
         }
      /* determine what field number we are in */
      xindex = x/colGapWidth/(displayLongest+1);
      if (xindex >= displayNumberOfFields)
	 xindex--;
      /* determine index number of element that we have clicked */
      yindex = (y + CANVAS_OFFSET)/rowGapHeight;
      if (ElementTopHead)
	 yindex--;
      index = xindex + yindex*displayNumberOfFields;
      elementIndex = index + startingElement;
      if (elementIndex >= displayNumberOfElements)
      {  if (Tflag)
	    fprintf(stderr,"ButSelect: no more elements index=%d elementIndex=%d\n",index,elementIndex);
	 return;
      }
      /* skip to that element */
      eptr = displayElist->firstElement;
      for (i=0 ; i<elementIndex; i++)
      {  if (Tflag)
	    fprintf(stderr,"ButSelect:skipping over %s\n",eptr->name);
	 eptr = eptr->next;
      }
      /* mark or clear the element */
      if (eptr->marked)
	 eptr->marked = 0;
      else
         eptr->marked = 1;
      if (Tflag)
         fprintf(stderr,"x=%d  y=%d index=%d elementIndex=%d name=%s len=%d\n",x,y,index,elementIndex,eptr->name,strlen(eptr->name));
      entry_len = strlen(eptr->name);
      if (strcmp(eptr->info,"d") == 0)
         entry_len++;
      for (i=0; i<entry_len; i++)
         dummy[i] = 'a';
      dummy[i] = '\0';
      ny = index/displayNumberOfFields*rowGapHeight + rowGapHeight;
      if (ElementTopHead)
	 ny = ny + rowGapHeight;
      nx = index % displayNumberOfFields * colGapWidth * (displayLongest+1);
      if (Tflag)
         fprintf(stderr,"ButSelect:index =%d nx=%d  ny=%d\n",index,nx,ny);
      Inversepw_box(nx,ny-CANVAS_OFFSET,dummy);
   }
}

/*----------------------------------------------------------------------
|       Activate Mouse
|
|       This routine activates the mouse, so elements can be selected.
+------------------------------------------------------------------------*/
elementTurnOnMouse()
{
    Wactivate_mouse(NULL,ButSelect,NULL);
}


/*----------------------------------------------------------------------
|       setItNow/4
|
|       This routine determines what element the graphon button  clicked,
|       highlights it and sets the button
+------------------------------------------------------------------------*/
setItNow(x,y,row,col)	int x,y,row,col;
{  char     dummy[256];
   Element *eptr;	/* pointer to element list packet */
   int      elementIndex;
   int      i;
   int      index;
   int      nx,ny;
   int      xindex;
   int      yindex;

      /* determine what field number we are in */
      xindex = x/colGapWidth/(displayLongest+1);
      if (xindex >= displayNumberOfFields)
	 xindex--;
      /* determine index number of element that we have clicked */
      yindex = row; /*(y + CANVAS_OFFSET)/rowGapHeight;*/
      if (ElementTopHead)
	 yindex--;
      index = xindex + yindex*displayNumberOfFields;
      elementIndex = index + startingElement;
      if (elementIndex >= displayNumberOfElements)
      {  if (Tflag)
	    fprintf(stderr,"setItNow: no more elements index=%d elementIndex=%d\n",index,elementIndex);
	 return;
      }
      /* skip to that element */
      eptr = displayElist->firstElement;
      for (i=0 ; i<elementIndex; i++)
      {  if (Tflag)
	    fprintf(stderr,"setItNow:skipping over %s\n",eptr->name);
	 eptr = eptr->next;
      }
      /* mark or clear the element */
      if (eptr->marked)
	 eptr->marked = 0;
      else
         eptr->marked = 1;
      if (Tflag)
         fprintf(stderr,"setItNow;x=%d  y=%d index=%d elementIndex=%d name=%s len=%d\n",x,y,index,elementIndex,eptr->name,strlen(eptr->name));
      for (i=0; i<strlen(eptr->name); i++)
         dummy[i] = 'a';
      dummy[i] = '\0';
      /*pw = canvas_pixwin(canvas);
      ny = index/displayNumberOfFields*rowGapHeight + rowGapHeight;
      if (ElementTopHead)
	 ny = ny + rowGapHeight; */
      nx = index % displayNumberOfFields * colGapWidth * (displayLongest+1);
      col = nx/colGapWidth + 1;
      if (eptr->marked)
         Wprintfpos(4,row+1,col,"\033[7m%s\033[27m",eptr->name);
      else
         Wprintfpos(4,row+1,col,"%s",eptr->name);
      if (Tflag)
         fprintf(stderr,"setItNow:index =%d row =%d col=%d \n",index,row, col);
      /*pw_text(pw,nx,ny-CANVAS_OFFSET,PIX_NOT(PIX_DST),NULL,dummy); */
   }
/*----------------------------------------------------------------------
|       DisplayElementsScreen/1
|
|       This routine displays the next or previous screen.
|       If n = -1, it is previous screen, +1 is next screen
+------------------------------------------------------------------------*/
DisplayElementScreen(n)  int n;
{
   _displayElements(displayElist,displayScreen + n); 
}

/**************************************************************************
 delem, eleminfo commands and related programs
 Adopted from files and filesinfo with a few key changes
 **************************************************************************/

extern int	 Bnmr;

static Elist	*elem_list_base = NULL;


/*  This program reports the error if it is not successful  */

static int
textfileLoad( file_name )
char *file_name;
{
	int	 ival, line_len, line_num, number_of_cols;
	char	*tptr;
	char	 text_buffer[ 122 ];
	FILE	*fptr;

	fptr = fopen( file_name, "r" );
	if (fptr == NULL) {
		Werrprintf( "Cannot open %s in display element program", file_name );
		return( -1 );
	}
	if (!(elem_list_base = initElementStruct())) {
		if (Tflag)
		  fprintf(stderr,"textfileLoad:out of memory\n");
		fclose(fptr);
		return( -1 );
	} 

	number_of_cols = graf_width / xcharpixels;

	while (tptr = fgets( &text_buffer[ 0 ], sizeof( text_buffer ) - 1, fptr )) {
		line_num++;
		line_len = strlen( &text_buffer[ 0 ] );
		if (line_len > 0)
		  if (text_buffer[ line_len-1 ] != '\n') {
			Werrprintf(
		   "line %d in %s is too long", line_num, file_name
			);
			return( -1 );
		  }
		  else
		    text_buffer[ line_len-1 ] = '\0';
		else
		  text_buffer[ 0 ] = '\0';			/* insurance */

		if (line_len >= number_of_cols)
		  text_buffer[ number_of_cols-1 ] = '\0';

		addElement(elem_list_base, &text_buffer[ 0 ], "n");
	}
}

static
CleanUp()		/* almost a duplicate of version in files.c */
{
	extern int terminalElementActive;
   
	if (Wissun())
	  Wturnoff_mouse();
	else
	  if (Wisgraphon()) {
		WblankCanvas();
		terminalElementActive = 0; /* make sure graphon elements not active */
	}
	if (elem_list_base) {
		releaseElementStruct(elem_list_base);
		elem_list_base = NULL;
	}
	if (WgraphicsdisplayValid( "delem" ))
	  Wsetgraphicsdisplay( "" );
}

	/* NOT a duplicate of files::display_the_files */

static int
display_the_elems( file_name )
char *file_name;
{
	int	ival;

	if (elem_list_base != NULL) {
		releaseElementStruct( elem_list_base );
		elem_list_base = NULL;
	}

	ival = textfileLoad( file_name );
	if (ival != 0) {
		return( -1 );
	}

	displayElements( elem_list_base );

	return( 0 );
}


static int
display_elems_menu( menu_name, menu_path, turnoff_routine )
char *menu_name;
char *menu_path;
int (*turnoff_routine)();
{
	int	ival;

	ival = read_menu_file( menu_path );
	if (ival != 0) {
		Werrprintf( "cannot open element menu %s", menu_path );
		return( -1 );
	}

	exec_menu_buf();
	display_menu( menu_name, turnoff_routine );

	return( 0 );
}

int
delem( argc, argv )
int argc;
char *argv[];
{
	int	ival, plan_to_display_elems, plan_to_display_menu,
		turnoff_flag;
	char	new_menu_name[ MAXPATHL ], new_menu_path[ MAXPATHL ];

static char	cur_menu_name[ MAXPATHL ] = {'\0'};
static char	cur_menu_path[ MAXPATHL ] = {'\0'};

/*  OK to run this program?  */

	if (Bnmr) {
		Werrprintf(
	    "%s:  not available in a background mode of VNMR", argv[ 0 ]
		);
		ABORT;
	}
	else if (Wistek4x05()) {
		Werrprintf(
	    "%s:  not available on a Tektronix 4105 or 4205", argv[ 0 ]
		);
		ABORT;
	}
	else if (argc < 2) {
		Werrprintf( "%s:  requires at least 1 argument", argv[ 0 ] );
		ABORT;
	}

	plan_to_display_elems = 131071;
	plan_to_display_menu  = 131071;
	turnoff_flag          = 0;

/*  Next program verifies/creates certain parameters for the menu system.  */

	ival = verify_menu_params();

/*  Name of menu to display  */

	if (argc > 2)
	  strcpy( &new_menu_name[ 0 ], argv[ 2 ] );
	else {
		if (strlen( &cur_menu_name[ 0 ] ) < 1)
		  strcpy( &new_menu_name[ 0 ], "files_main" );
		else
		  strcpy( &new_menu_name[ 0 ], &cur_menu_name[ 0 ] );
	}

	ival = locate_menufile( &new_menu_name[ 0 ], &new_menu_path[ 0 ] );
	if (ival != 0) {
		Werrprintf(
	    "%s:  menu %s does not exist", argv[ 0 ], &new_menu_name[ 0 ] );
		ABORT;
	}

	if ( !WgraphicsdisplayValid( argv[ 0 ] ) ) {
		plan_to_display_elems = 131071;
		plan_to_display_menu  = 131071;
		turnoff_flag          = 131071;
	}
	else {
		turnoff_flag = 0;
		if (strcmp( &new_menu_path[ 0 ], &cur_menu_path[ 0 ] ) != 0)
		  plan_to_display_menu = 131071;
		else
		  plan_to_display_menu = 0;
	}

/*  First turnoff the buttons (from previous VNMR command)
    Then display the menu
    Necessary to turn off the buttons BEFORE displaying the elements.	*/

	if (turnoff_flag)
	  Wturnoff_buttons();
	if (plan_to_display_elems) {
		ival = display_the_elems( argv[ 1 ] );
		if (ival != 0)
		  ABORT;
	}
	if (plan_to_display_menu) {
		ival = display_elems_menu(
		    &new_menu_name[ 0 ], &new_menu_path[ 0 ], CleanUp
		);
		if (ival != 0)
		  ABORT;
		if (turnoff_flag == 0)
		  resetButtonPushed();
	}

	strcpy( &cur_menu_path[ 0 ], &new_menu_path[ 0 ] );
	strcpy( &cur_menu_name[ 0 ], &new_menu_name[ 0 ] );
	Wsetgraphicsdisplay( argv[ 0 ] );
}

/*  These defines allow process args of element info to communicate
    the function the argument specifies to the element info command.	*/

#define  ERROR	-1
#define  RESET	0
#define  NUMBER 1
#define  VALUE  2

/*  This program is an exact duplicate of the
    (static) process_args_filesinfo in files.c  */

static int
process_args_eleminfo( argc, argv, retc )
int argc;
char *argv[];
int retc;
{
	int	retval;

	if (argc < 2) {
		Werrprintf( "%s:  this command requires an argument", argv[ 0 ] );
		return( ERROR );
	}

	if (strcmp( argv[ 1 ], "redisplay" ) == 0)
	  retval = RESET;
	else {
		if (strcmp( argv[ 1 ], "number" ) == 0)
		  retval = NUMBER;
		else if (strcmp( argv[ 1 ], "name" ) == 0 ||
			 strcmp( argv[ 1 ], "names" ) == 0)
		  retval = VALUE;
		else {
			Werrprintf(
		    "%s: argument of %s not recognized", argv[ 0 ], argv[ 1 ]
			);
			return( ERROR );
		}

		if (retc < 1) {
			Werrprintf(
	    "%s: you must return a value with the %s argument", argv[ 0 ], argv[ 1 ]
			);
			return( ERROR );
		}
	}

	return( retval );
}

int
eleminfo( argc, argv, retc, retv )
int argc;
char *argv[];
int retc;
char *retv[];
{
	int		 cur_function, elem_num, ival, num_active;
	char		*active_list;
	extern char	*get_one_active(), *get_all_active();

	if (!WgraphicsdisplayValid( "delem" )) {
		Werrprintf( "%s:  delem program is not active", argv[ 0 ] );
		ABORT;
	}

	cur_function = process_args_eleminfo( argc, argv, retc );
	if (cur_function == ERROR)
	  ABORT;

	if (cur_function == RESET) {
		Werrprintf( "No reset operation with %s", argv[ 0 ] );
		ABORT;
	}
	else if (cur_function == NUMBER) {
		num_active = getNumberOfSelect( elem_list_base );
                retv[ 0 ] = realString( (double) num_active );
        }
	else if (cur_function == VALUE) {
		active_list = NULL;
		if (argc < 3)
		  active_list = get_all_active( elem_list_base );
		else {

	   /*  Three ways the command can abort here:
	       1.  The 2nd argument is not a number
	       2.  The 2nd argument is a number but is out of range
	       3.  Something goes wrong in `get_one_active' and it
		   fails to return an element name.

	       In the second case we set the value of the return
	       argument to the 0-length string.  In the other cases
	       we do nothing to the return argument.  The first one
	       is likely from a problem in the user's programming;
               the last one would be from a problem in our programs.	*/

			if (!isReal( argv[ 2 ] )) {
				Werrprintf(
		    "%s:  must use a number to select an entry", argv[ 0 ]
				);
				ABORT;
			}

			elem_num = (int) stringReal( argv[ 2 ] );
			if (elem_num < 1) {
				Werrprintf(
   "%s:  you must use an index greater than zero to select an entry", argv[ 0 ]
				);
				retv[ 0 ] = newString( "" );
				ABORT;
			}
			num_active = getNumberOfSelect( elem_list_base );
			if (elem_num > num_active) {
				Werrprintf(
		   "%s:  you have not selected %d entries", argv[ 0 ], elem_num
				);
				retv[ 0 ] = newString( "" );
				ABORT;
			}
			active_list = get_one_active( elem_list_base, elem_num );
			if (active_list == NULL) {
				Werrprintf(
		   "%s:  selection %d does not exist", argv[ 0 ], elem_num
				);
				ABORT;
			}
		}

		if (active_list) {
			retv[ 0 ] = newString( active_list );
			release( active_list );
		}
		else
		  retv[ 0 ] = newString( "" );
	}

	RETURN;
}
#endif
