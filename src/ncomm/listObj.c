/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
8-23-94       first draft
8-23-94       searchList returns index of item if found, or -1 if not found
8-29-94       puts out a sensible manual entry with mangen
4-19-95       fixed bug in deleteItem
*/


/*

DESCRIPTION

Procedures to maintain a linked list.

A linked list is similar to an array with two key differences.  One, a list
can expand or shrink dynamically with the requirements of the application.
Now an array allocated from the heap can do this too, but it typically is
a time-sink, since the entire contents of the array must be moved from the
old storage to the new storage.  Two, an element of a list can be quickly
deleted.  One can also do this with an array, but it also remains a time-
sink, since typically at least part of the array must be moved to erase
the deleted entry.

Each item or node in a linked list (typically) has some contents, which the
application will take interest in, and the address of one or more other
nodes, which the application should not be concerned with.  In this
implementation, the application determines the content of each list node;
these procedures see it only as a pointer to void.  If your data is the
size of a pointer to void or smaller, you can store it directly; otherwise
you must provide the address of the data to be stored.

The application MUST provide a comparison procedure or comparator to search
for an item on a list.  The comparator will be called with two arguments;
the first being the contents of the current node on the list, the second
being the argument the application passed to the procedure.  The comparator
is expected to return 0 if the two nodes match (following the example of
strcmp) and some non-zero value if the do not match.

The application CAN provide a destructor procedure to be called when a node
on a list is destroyed.  This is how the application is expected to free up
any space it dynamically allocated.  Suppose for example that the contents
of each node is a pointer into dynamic memory.  When any node is deleted,
this dynamic memory must be free'd.  Providing a destructor is the most
direct way to accomplish this.  It is not required however.  Two situations
would justify this.  First, the contents of a node never reference in any
way dynamic memory.  Second, the application has its own system for keeping
track of the addresses it allocates from the heap, and does not have to
rely on the list to track these addresses.

Never call appendItem, deleteItem OR destroyList from an interrupt or a
signal handler.

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "listObj.h"

/*  following is private to this source file.  */

struct listNode{
	void		*item;
	struct listNode	*next;
};


/**************************************************************
*
*   appendItem - append an item to the end of a list
*
*   returns:   list object if successful
*              NULL if not successful.  A return value
*                of NULL implies the process is out of
*                dynamic memory
*
*   Call appendItem with a list object as the 1st argument
*   to append an item to a preexisting list, or call it
*   with NULL as the 1st argument to start a new list.
*
*   You must save the return value when creating a new
*   list, as it is your list object.  You should check
*   the return value for a preexisting list, although
*   it will not change as new items are appended.
*/

LIST_OBJ appendItem( LIST_OBJ thisList, void *thisItem )
/* LIST_OBJ thisList - list object to append the item to, */
/*                     or NULL to start a new list */
/* void *thisItem    - item to be appended.  */
{
	LIST_OBJ	curItem, newList, newItem;

	if (thisList == NULL) {
		newList = (LIST_OBJ) malloc( sizeof( struct listNode ) );
		if (newList == NULL) {
			/* fprintf( stderr, "out of memory starting a new list\n" ); */
			errno = ENOMEM;
			return( NULL );
		}
		newList->next = NULL;
		newList->item = thisItem;

		return( newList );
	}
	else {
		newItem = (LIST_OBJ) malloc( sizeof( struct listNode ) );
		if (newItem == NULL) {
			/* fprintf( stderr, "out of memory adding a new item\n" ); */
			errno = ENOMEM;
			return( NULL );
		}
		newItem->next = NULL;
		newItem->item = thisItem;

		curItem = thisList;
		while (curItem->next != NULL)
		  curItem = curItem->next;

		curItem->next = newItem;

		return( thisList );
	}
}

/**************************************************************
*
*   getItem - get an item from a list
*
*   returns:  item from the list, or NULL if not found.
*             To distinguish from an item of NULL, errno
*             will be set to a non-zero value if and only
*             if no item is found.
*                 EFAULT  -  list object is NULL
*                 ENOENT  -  sequence number too large
*                 EINVAL  -  sequence number < 0
*
*             Notice this means getItem always sets a
*             value in errno.
*
*   You must use the sequence number to get an item from
*   a list.  Following an old established rule, items are
*   numbered starting at 0.  Use 0 to get the first item,
*   1 to get the 2nd item, etc.
*
*   How do you get the sequence number if you don't know
*   it?  See searchItem
*
*/

ITEM getItem( LIST_OBJ thisList, int sequence )
/* LIST_OBJ thisList - list to get item from */
/* int sequence      - which item to get     */
{
	int	iter;

	errno = 0;
	if (thisList == NULL) {
		errno = EFAULT;
		return( NULL );
	}
	if (sequence < 0) {
		errno = EINVAL;
		return( NULL );
	}

	for (iter = 0; iter < sequence; iter++) {
		thisList = thisList->next;
		if (thisList == NULL) {
			errno = ENOENT;
			return( NULL );
		}
	}

	return( thisList->item );
}

/**************************************************************
*
*   searchItem - search for an item on a list
*
*   returns:  sequence number of item or -1 if not found.
*
*   Suppose you have a list where each item is a string.
*   (You would pass appendNode values of type char *).
*   Now suppose you want to know if the list has the
*   string "tenth" as one of its items.  You could call
*   searchItem (and getItem) like this:
*
*       seqn = searchItem( myList, "tenth", strcmp );
*       if (seqn < 0) {
*           printf( "could not find 'tenth' in your list\n" );
*       }
*       else {
*           myItem = getItem( myList, seqn );
*       }
*
*/

int searchItem( LIST_OBJ thisList, ITEM thisItem, int (*comparator)() )
{
	int		iter, ival;
	LIST_OBJ	currentItem;

	if (thisList == NULL)
	  return( -1 );
	if (comparator == NULL)
	  return( -1 );

	currentItem = thisList;
	iter = 0;
	while (currentItem != NULL) {
		ival = (*comparator) (currentItem->item, thisItem );
		if (ival == 0) {
			return( iter );
		} 

		iter++;
		currentItem = currentItem->next;
	}

	return( -1 );
}

/**************************************************************
*
*   deleteItem - delete an item on a list
*
*   returns:  list object or
*             NULL if you delete the sole item on your list.
*             NULL if an error occurs
*                 EFAULT - list object orignally was NULL
*                 EINVAL - no comparator routine
*
*   To distinguish the two cases where NULL is returned,
*   errno will be set if and only if an error occurs.  If
*   deleteItem returns NULL and errno is zero, then you just
*   deleted the last item on your list.  Notice this means
*   deleteItem always sets a value in errno.
*
*   Unless an error occurs,  you should always use the return
*   value for the list object; if you delete the first item
*   on your list, the value of the list object will change
*   and the old value will point to free'd dynamic memory.
*
*   The comparator is required; the destructor is optional.
*
*   Here is how to delete the third item (sequence number = 2)
*   on a list, which we assume is a list of strings, with each
*   string allocated from the heap:
*
*      item = getItem( myList, 2 );
*      myList = deleteItem( myList, item, strcmp, free );
*
*/

LIST_OBJ deleteItem(
	LIST_OBJ thisList,
	ITEM thisItem,
	int (*comparator)(),
	void (*destructor)()
)
/* LIST_OBJ thisList    - list object */
/* ITEM thisItem        - item to look for and delete */
/* int (*comparator)()  - comparison routine */
/* void (*destructor)() - destructor routine */
{
	int		ival;
	LIST_OBJ	currentItem, previousItem;

	errno = 0;
	if (thisList == NULL) {
		errno = EFAULT;
		return( NULL );
	}
	if (comparator == NULL) {
		errno = EINVAL;
		return( NULL );
	}

/*  Explanation of return values.

    If no items are deleted, the first argument becomes the return value.
    If it deletes an item and the item is not the first item on the list,
  the first argument becomes the return value.
    BUT, if it deletes the first item, the return value will be the next
  item on the list, which would well be NULL.					*/

	currentItem = thisList;
	previousItem = NULL;
	while (currentItem != NULL) {
		ival = (*comparator) (currentItem->item, thisItem );
		if (ival == 0) {
			if (destructor)
			  (*destructor)( currentItem->item );
			if (previousItem == NULL)
			  thisList = currentItem->next;
			else
			  previousItem->next = currentItem->next;
			free( currentItem );

			return( thisList );
		} 

		previousItem = currentItem;
		currentItem = currentItem->next;
	}

	return( thisList );
}

/**************************************************************
*
*   destroyList - free all the memory associated with a list
*
*/

int destroyList( LIST_OBJ thisList, void (*destructor)() )
/* LIST_OBJ thisList     - list to be destroyed */
/* voids (*destructor)() - destructor routine */
{
	LIST_OBJ	currentItem, nextItem;

	currentItem = thisList;

	while (currentItem != NULL) {
		if (destructor)
		  (*destructor)( thisList->item );
		nextItem = currentItem->next;
		free( currentItem );
		currentItem = nextItem;
	}

	return( 0 );
}
