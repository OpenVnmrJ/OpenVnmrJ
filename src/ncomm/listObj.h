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
01b,29Aug94    conforms to de facto WRS standards for include files
01a,23Aug94    written
*/

#ifndef __INClistObjh
#define __INClistObjh

#ifdef __cplusplus
extern "C" {
#endif


typedef void 		*ITEM;
typedef struct listNode *LIST_OBJ;

#if defined(__STDC__) || defined(__cplusplus)
extern LIST_OBJ appendItem( LIST_OBJ thisList, void *thisItem );
extern ITEM getItem( LIST_OBJ thisList, int sequence );
extern int searchItem( LIST_OBJ thisList, ITEM thisItem, int (*comparator)() );
extern LIST_OBJ deleteItem(
	LIST_OBJ thisList,
	ITEM thisItem,
	int (*comparator)(),
	void (*destructor)()
);
extern int destroyList( LIST_OBJ thisList, void (*destructor)() );
#else
extern LIST_OBJ appendItem();
extern ITEM getItem();
extern searchItem();
extern LIST_OBJ deleteItem();
extern int destroyList();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INClistObjh */
