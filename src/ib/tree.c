/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
   AVL (balanced) binary tree module. For a description of the algorithm, see
   "C Chest: An AVL Tree Database Package" by Alan Holub, Dr. Dobb's Journal,
   August 1986 (#118), pages 20-29 (text) and pages 86-102 (code).  The code
   contains a bug; the fix is at the end of "C Chest: More, a File-Browsing
   Utility" by Alan Holub, Dr. Dobb's Journal, October 1986 (#120), page 25.
 */


#include <memory.h>
#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif

/* these includes pick up the SID strings in the header files */
#define HEADER_ID
#include "tree.h"
#include "stack.h"
#undef HEADER_ID

#include "boolean.h"
#include "error.h"
#include "stack.h"
#include "storage.h"
#include "tree.h"
#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif

/* variables for various operations, to cut down on the overhead for
   recursive subroutine calls */

static STORAGE *S_store;  /* A pointer to header for requested storage area */
static void    *S_key;    /* A pointer to the key for the data being handled */
static void    *S_data;   /* A pointer to data being handled */
static int      S_action; /* The "action" for the insertion */

/* variable for holding the current node in a search of the tree, used
   by functions "Storage_first()" and "Storage_next()" */

static STK_ENTRY S_curr;

static int S_del;

/* functions local to this module */

#ifdef __STDC__
static int      S_insert (NODE **pp_node);
static unsigned S_log2 (unsigned count);
static int      S_delete (NODE **pp_root);
static int      S_balance_l (NODE **pp_node);
static int      S_balance_r (NODE **pp_node);
static int      S_descend (NODE **pp_node, NODE **pp_dnode);
static int      S_clear (NODE *p_node);
static int      S_output (NODE *p_node, int (*output)(void *key, void *data) );
#ifdef DEBUG
static int      S_tree (NODE *p_root, int amleft);
static void     S_print (NODE *p);
static void     S_setbit (int bit, int value);
#endif
#else
static int      S_insert( /* NODE **pp_node */ );
static unsigned S_log2( /* unsigned count */ );
static int      S_delete( /* NODE **pp_root */ );
static int      S_balance_l( /* NODE **pp_node */ );
static int      S_balance_r( /* NODE **pp_node */ );
static int      S_descend( /* NODE **pp_node, NODE **pp_dnode */ );
static int      S_clear( /* NODE *p_node */ );
static int      S_output( /* NODE *p_node, int (*output)() */ );
#ifdef DEBUG
static int      S_tree( /* NODE *p_root, int amleft */ );
static void     S_print( /* NODE *p */ );
static void     S_setbit( /* int bit, int value */ );
#endif
#endif

/****************************************************************************
 Storage_create

 This function allocates space for a storage area with space for "size" number
 of entries, and returns a reference "Storage" to the storage area created.

 INPUT ARGS:
   size        The number of entries the storage should expect to hold.
   compare     A pointer to a function used to compare storage keys, for
               searching for a specified key. If NULL, error.
   delete      A pointer to a function used to free the memory, allocated by
               the appplication, that holds the data, and possibly the key,
               for a storage entry.
               If NULL, entries cannot be deleted from the storage area, and
               other storage functions calls that need to delete or replace an
               entry will return an error.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   Storage     A reference for the storage area created.
               If an error occurred, (Storage)NULL (see ERRORS below).
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   (Storage)NULL  Returned on error:
                     no function pointer for comparing storage keys
                     couldn't allocate memory for the storage-area header
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
Storage Storage_create (unsigned size, int (*compare)(const char *key1, const char *key2),
                        int (*delete)(void *key, void *data) )
#else
Storage Storage_create (size, compare, delete)
 unsigned   size;
 int      (*compare)();
 int      (*delete)();
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   store    A pointer to memory allocated for this storage area.
   */
   STORAGE *store = (STORAGE *)NULL;

   /* check the pointer for the comparison function: MUST have it to continue */
   if (compare == (int(*)())NULL)
   {
      ERROR(("Storage_create: no pointer to a function for key comparison"));
   }
   /* allocate memory for the storage header structure */
   else if ( (store = (STORAGE *)malloc (sizeof(STORAGE))) == (STORAGE *)NULL)
   {
      SYS_ERROR(("Storage_create: malloc one storage header"));
   }
   else
   {
      /* clear the number of entries in this storage area */
      store->count   = 0;

      /* clear the node and stack pointers for this storage area */
      store->root    = (NODE *)NULL;
      store->stack   = (Stack)NULL;

      /* set the function pointers for this storage area */
      store->compare = compare;
      store->delete  = delete;
   }
   return ((Storage)store);

}  /* end of function "Storage_create" */

/****************************************************************************
 Storage_insert

 This function inserts the data for the input key into the input storage area.

 INPUT ARGS:
   store       The storage area where the data is to be put.
   key         A pointer to the storage key for this data.
   data        A pointer to the data that is to be stored.
   action      The action to take:
                  REPLACE => over-write data if key already exists
                  REPORT  => don't over-write data, and return error
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   S_store     A pointer to the STORAGE header for the current storage area.
   S_key       A pointer to the key for the data being handled.
   S_data      A pointer to the data being handled.
   S_action    The "action" for the insertion: REPORT or REPLACE.
 GLOBALS CHANGED:
   S_store     A pointer to the STORAGE header for the current storage area.
   S_key       A pointer to the key for the data being handled.
   S_data      A pointer to the data being handled.
   S_action    The "action" for the insertion: REPORT or REPLACE.
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
   S_ACT          invalid value for "action": not REPORT or REPLACE
   S_RPT          "action" is REPORT and "key" is already in storage
   S_REPL         "action" is REPLACE and error occurred replacing data
   S_INS          memory couldn't be allocated for storing the data
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_insert (Storage store, void *key, void *data, int action)
#else
int Storage_insert (store, key, data, action)
 Storage store;
 void   *key;
 void   *data;
 int     action;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the insertion operation.
   */
   int result = S_OK;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_insert: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if (action != REPLACE && action != REPORT)
   {
      ERROR(("Storage_insert: action '%d' is not REPLACE or REPORT",action));
      result = S_ACT;
   }
   else
   {
      /* set the pointer to the storage area being used */
      S_store = (STORAGE *)store;

      /* set the global variables required for insertion */
      S_key    = key;
      S_data   = data;
      S_action = action;

      if ( (result = S_insert (&(S_store->root)) ) == S_OK)
         ++S_store->count;
   }
   return (result);

}  /* end of function "Storage_insert" */

/****************************************************************************
 Storage_search

 This function searches the input storage area for the input search key, and
 returns a pointer to the data stored for that key.

 INPUT ARGS:
   store       The storage area to be searched for the key.
   key         A pointer to the storage key for this data.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   void *      A pointer to the data stored for the input key.
               If no entry was found, or an error occurred, (void *)NULL.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   (void *)NULL   Returned on error:
                     the reference to the storage area is (Storage)NULL
                     requested storage area is empty
                     requested key was not found in the storage area
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
void *Storage_search (Storage store, void *key)
#else
void *Storage_search (store, key)
 Storage store;
 void   *key;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_node   A pointer to the current node in the tree.
   which    The result of the comparison of keys.
   */
   NODE *p_node;
   int   which;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_search: NULL pointer to storage area"));
   }
   else if ( (p_node = ((STORAGE *)store)->root) == (NODE *)NULL)
   {
      ERROR(("Storage_search: requested storage area is empty"));
   }
   else
   {
      /* descend the tree, looking for the input key */
      do
      {
         if ( (which = (*((STORAGE *)store)->compare)(p_node->key, key)) > 0)
            p_node = p_node->left;
         else if (which < 0)
            p_node = p_node->right;
         else
            return (p_node->data);

      } while (p_node != (NODE *)NULL);
   }
   return ((void *)NULL);

}  /* end of function "Storage_search" */

/****************************************************************************
 Storage_first

 This function looks for entries in the input storage area, beginning at the
 start of the storage area. This function should be called to begin a search
 that may be continued with "Storage_next()".

 INPUT ARGS:
   store       The storage area to be searched.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   void *      A pointer to the data stored in the entry.
               If no entry was found, or an error occurred, (void *)NULL.
 GLOBALS USED:
   S_store     A pointer to the STORAGE header for the current storage area
               (for the user-supplied "compare()" and "delete()" functions.
 GLOBALS CHANGED:
   none
 ERRORS:
   (void *)NULL   Returned on error:
                     the reference to the storage area is (Storage)NULL
                     requested storage area is empty
                     invalid value for number of levels in tree
                     error initializing stack: see "Stack_init()"
                     error adjusting stack: see "Stack_adjust()"
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
void *Storage_first (Storage store)
#else
void *Storage_first (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   lvls     The number of levels in the tree, which is the number of entries
            needed for the stack.
   stk      A pointer to the stack area for this storage area.
   */
   unsigned lvls;
   Stack    stk;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_first: NULL pointer to storage area"));
      return ((void *)NULL);
   }
   if ( ((STORAGE *)store)->root == (NODE *)NULL)
   {
      ERROR(("Storage_first: requested storage area is empty"));
      return ((void *)NULL);
   }
   /* find the number of levels for the current size of the tree */
   if ( (lvls = S_log2 (((STORAGE *)store)->count)) <= 0)
   {
      ERROR(("Storage_first: power of 2 %d is not positive",lvls));
      return ((void *)NULL);
   }
   /* adjust for the worst case of imbalance */
   lvls += 2;

   /* if a stack has not been allocated, make one for the current size */
   if ( (stk = ((STORAGE *)store)->stack) == (Stack)NULL)
   {
      if ( (stk = Stack_init (lvls, sizeof (STK_ENTRY))) == (Stack)NULL)
      {
         ERROR(("Storage_first: Stack_init for %d stack entries",lvls));
         return ((void *)NULL);
      }
      ((STORAGE *)store)->stack = stk;
   }
   /* otherwise, adjust the stack to the current size */
   else
   {
      if ( (stk = Stack_adjust (stk, lvls)) == (Stack)NULL)
      {
         return ((void *)NULL);
      }
      ((STORAGE *)store)->stack = stk;
   }
   /* set the current entry to the root node */
   S_curr.p_node = ((STORAGE *)store)->root;
   S_curr.relation = ROOT;

   /* descend the tree to the left as far as possible */
   while (S_curr.p_node->left != (NODE *)NULL)
   {
      if (Stack_push (stk, (void *)&S_curr) == 0)
         return ((void *)NULL);

      /* set the current entry to this node */
      S_curr.p_node = S_curr.p_node->left;
      S_curr.relation = LEFT;
   }
   /* return the data for the entry at the top of the stack */
   return (S_curr.p_node->data);

}  /* end of function "Storage_first" */

/****************************************************************************
 Storage_next

 This function looks for entries in the input storage area, beginning where
 the previous search left off. This function should be called only after the
 search is begun with "Storage_first".

 INPUT ARGS:
   store       The storage area being searched.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   void *      A pointer to the data stored in the entry.
               If no entry was found, or an error occurred, (void *)NULL.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   (void *)NULL   Returned on error:
                     the reference to the storage area is (Storage)NULL
                     requested storage area is empty
                     nothing was found in the storage area
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
void *Storage_next (Storage store)
#else
void *Storage_next (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   stk      A pointer to the stack area for this storage area.
   */
   Stack stk;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_next: NULL pointer to storage area"));
      return ((void *)NULL);
   }
   if ( ((STORAGE *)store)->root == (NODE *)NULL)
   {
      ERROR(("Storage_next: requested storage area is empty"));
      return ((void *)NULL);
   }
   /* get the pointer to the stack for this storage area */
   stk = ((STORAGE *)store)->stack;

   /* if the pointer to the current node is NULL, don't start */
   if (S_curr.p_node == (NODE *)NULL)
      return ((void *)NULL);

   /* if the current node has a right child ... */
   else if (S_curr.p_node->right != (NODE *)NULL)
   {
      /* save the current node */
      if (Stack_push (stk, (void *)&S_curr) == 0)
         return ((void *)NULL);

      /* set the current node to the right node */
      S_curr.p_node = S_curr.p_node->right;
      S_curr.relation = RIGHT;

      /* descend the tree to the left as far as possible */
      while (S_curr.p_node->left != (NODE *)NULL)
      {
         if (Stack_push (stk, (void *)&S_curr) == 0)
            return ((void *)NULL);

         /* set the current node to the left node */
         S_curr.p_node = S_curr.p_node->left;
         S_curr.relation = LEFT;
      }
   }
   /* otherwise, go back up the tree (left children are already done!) */
   else
   {
      /* pop right nodes until a left node or the root is found */
      while (S_curr.relation == RIGHT)
      {
         /* pop the current node from the top-of-stack */
         if (Stack_pop (stk, (void *)&S_curr) != 1)
         {
            ERROR(("Storage_next: Stack_pop returned error"));
            return ((void *)NULL);
         }
      }
      /* if the current node is the root, done */
      if (S_curr.relation == ROOT)
      {
         /* clear the pointer to the current node */
         S_curr.p_node = (NODE *)NULL;
         return ((void *)NULL);
      }
      /* the current node is a left child, so go back to the parent */
      if (Stack_pop (stk, (void *)&S_curr) != 1)
      {
         ERROR(("Storage_next: Stack_pop returned error"));
         return ((void *)NULL);
      }
   }
   /* return the data for the current node */
   return (S_curr.p_node->data);

}  /* end of function "Storage_next" */

/****************************************************************************
 Storage_delete

 This function deletes the entry for the input key from the input storage area.

 INPUT ARGS:
   store       The storage area from which the entry is to be deleted.
   key         A pointer to the storage key for this data.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Entry deleted from the storage area.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   S_store     A pointer to the STORAGE header for the current storage area.
   S_key       A pointer to the key for the data being handled.
 GLOBALS CHANGED:
   S_store     A pointer to the STORAGE header for the current storage area.
   S_key       A pointer to the key for the data being handled.
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
   S_EMT          requested storage area is empty
   S_DEL          no delete function has been supplied
   S_DERR         user-supplied "delete" function returned an error
                  (see function "S_delete").
   S_SRCH         requested key not found in the storage area
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_delete (Storage store, void *key)
#else
int Storage_delete (store, key)
 Storage store;
 void   *key;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the delete operation.
   */
   int result = S_OK;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_delete: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if (((STORAGE *)store)->root == (NODE *)NULL)
   {
      ERROR(("Storage_delete: requested storage area is empty"));
      result = S_EMT;
   }
   else if ( ((STORAGE *)store)->delete == (int(*)())NULL)
   {
      ERROR(("Storage_delete: no delete function has been supplied"));
      result = S_DEL;
   }
   else
   {
      /* set the pointer to the storage area being used */
      S_store = (STORAGE *)store;

      /* set the attributes required for deletion */
      S_key = key;
      S_del = TRUE;

      /* delete the requested node */
      if ( (result = S_delete ( &(( (STORAGE *)S_store )->root) )) >= S_OK)
         result = (S_del == TRUE ? S_OK : S_SRCH);
   }
   return (result);

}  /* end of function "Storage_delete" */

/****************************************************************************
 Storage_clear

 This function removes all entries in the input storage area; the memory used
 for building the storage area structures is NOT released.  See also function
 "Storage_release()".

 INPUT ARGS:
   store       The storage area to be cleared.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
   S_EMT          requested storage area is empty
   S_DEL          no delete function has been supplied
   S_DERR         user-supplied "delete" function returned an error
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_clear (Storage store)
#else
int Storage_clear (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the clear operation.
   */
   int result = S_OK;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_clear: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if (((STORAGE *)store)->root == (NODE *)NULL)
   {
      ERROR(("Storage_clear: requested storage area is empty"));
      result = S_EMT;
   }
   else if ( ((STORAGE *)store)->delete == (int(*)())NULL)
   {
      ERROR(("Storage_clear: no delete function has been supplied"));
      result = S_DEL;
   }
   else
   {
      /* set the pointer to the storage area being used,
         for accessing the user-supplied delete function */
      S_store = (STORAGE *)store;

      /* clear out everything in the tree, except for the stack used
         by the "Storage_first()" and "Storage_next()" functions */
      if ( (result = S_clear (S_store->root)) == S_OK)
      {
         S_store->count = 0;
         S_store->root  = (NODE *)NULL;
      }
   }
   return (result);

}  /* end of function "Storage_clear" */

/****************************************************************************
 Storage_release

 This function removes all entries in the input storage area and releases all
 memory used for building the storage area structures.  See also function
 "Storage_clear()".

 INPUT ARGS:
   store       The storage area to be released.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
                  see function "Storage_clear"
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_release (Storage store)
#else
int Storage_release (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the release operation.
   */
   int result;

   if ( (result = Storage_clear (store)) == S_OK || result == S_EMT)
   {
      /* free the stack used by "Storage_first()" and "Storage_next()" */
      if ( ((STORAGE *)store)->stack != (Stack)NULL)
         (void)free ( ((STORAGE *)store)->stack );

      /* free the storage-area header */
      (void)free ( (char *)store );
   }
   return (result);

}  /* end of function "Storage_release" */

/****************************************************************************
 Storage_output

 This function outputs the contents of the data stored in the input storage
 area, using a user-supplied output routine.

 INPUT ARGS:
   store       The storage area where the data is to be put.
   compare     A pointer to a function used to display the data stored.
               If NULL, error.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
   S_EMT          requested storage area is empty
   S_SHO          no output function was specified
   other          the return from the output function: !0 => failed
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_output (Storage store, int (*output)(void *key, void *data) )
#else
int Storage_output (store, output)
 Storage store;
 int (*output)();
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the output operation.
   */
   int result;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_output: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if (output == (int(*)())NULL)
   {
      ERROR(("Storage_output: no pointer to a function for output"));
      result = S_SHO;
   }
   else if (((STORAGE *)store)->root == (NODE *)NULL)
   {
      ERROR(("Storage_output: requested storage area is empty"));
      result = S_EMT;
   }
   else
   {
      result = S_output (((STORAGE *)store)->root, output);
   }
   return (result);

}  /* end of function "Storage_output" */

#ifdef DEBUG

/****************************************************************************
 Storage_display

 This function displays the structure of the input storage area in graphical
 fashion.

 INPUT ARGS:
   store       The storage area to be displayed.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
   S_EMT          requested storage area is empty
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
int Storage_display (Storage store)
#else
int Storage_display (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_root   A pointer to the root node of the tree.
   result   The result of the display operation.
   */
   NODE *p_root;
   int   result = S_OK;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_display: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if ( (p_root = ((STORAGE *)store)->root) == (NODE *)NULL)
   {
      ERROR(("Storage_display: requested storage area is empty"));
      result = S_EMT;
   }
   else
   {
      S_tree (p_root, 0);
   }
   return (result);

}  /* end of function "Storage_display" */

#endif

/****************************************************************************
 S_insert

 This function inserts (recursively) a new node into the tree, then balances
 the nodes on the way back out.

 INPUT ARGS:
   pp_node     The address of a pointer to the current node in the tree.
 OUTPUT ARGS:
   pp_node     A pointer to the current node in the tree.
 RETURN VALUE:
   S_OK        Operation succeeded.
   ! S_OK      Operation failed: see ERRORS below.
 GLOBALS USED:
   S_key       A pointer to the key for the data being handle.
   S_data      A pointer to data being handled.
   S_action    The "action" for the insertion.
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
   S_INS          memory couldn't be allocated for storing the data
   S_RPT          "action" is REPORT and "key" is already in storage
   S_DERR         user-supplied "delete" function returned an error
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#ifdef __STDC__
static int S_insert (NODE **pp_node)
#else
static int S_insert (pp_node)
 NODE **pp_node;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   grown    A flag: indicates that this sub-tree has grown by one node.
   p        A pointer to the parent node for this sub-tree.
   which    The result of the comparison of keys.
   p1, p2   Pointers to tree nodes, used for balancing the sub-trees.
   result   The result of the insertion operation.
   */
   static short grown = FALSE;
   NODE *p;
   int   which;
   NODE *p1, *p2;
   int   result = S_OK;

   /* if this node pointer is empty, insert a new node into the tree */
   if ( (p = *pp_node) == (NODE *)NULL)
   {
      /* get memory for a new node structure */
      if ( (p = (NODE *)malloc (sizeof(NODE))) == (NODE *)NULL)
      {
         SYS_ERROR(("S_insert: malloc one tree node for key '%s'", S_key));
         result = S_INS;
      }
      else
      {
         /* set the fields in the new node */
         p->key  = S_key;
         p->data = S_data;
         p->left  = (NODE *)NULL;
         p->right = (NODE *)NULL;
         p->bal = B;

         /* tree has grown in this direction */
         grown = TRUE;
      }
   }
   /* if the key in this node matches the input key, perform the
      insertion as specified by the action */
   else if ( (which = (*(S_store->compare))(p->key, S_key)) == 0)
   {
      if (S_action == REPORT)
      {
         ERROR(("S_insert: key '%s' already exists", S_key));
         result = S_RPT;
      }
      /* free the current key and data, using the user's delete function */
      else if ((*(S_store->delete))(p->key, p->data) != 0)
      {
         ERROR(("S_insert: user-supplied delete failed for key '%s'",S_key));
         result = S_DERR;
      }
      else
      {
         p->key = S_key;
         p->data = S_data;
      }
      /* tree has not grown because of this insertion */
      grown = FALSE;
   }
   /* requested key is less than the current key, so go left */
   else if (which > 0)
   {
      if ( (result = S_insert (&p->left)) == S_OK)
      {
         /* if the new node was inserted into the left sub-tree ... */
         if (grown == TRUE)
         {
            switch (p->bal)
            {
               case R:
                  p->bal = B;
                  grown = FALSE;
                  break;

               case B:
                  p->bal = L;
                  break;

               /* tree is unbalanced to the left, so rebalance */
               case L:
                  p1 = p->left;
                  if (p1->bal == L)
                  {
                     p->left   = p1->right;
                     p1->right = p;
                     p->bal    = B;
                     p         = p1;
                  }
                  else
                  {
                     p2        = p1->right;
                     p1->right = p2->left;
                     p2->left  = p1;
                     p->left   = p2->right;
                     p2->right = p;
                     p->bal    = (p2->bal == L ? R : B);
                     p1->bal   = (p2->bal == R ? L : B);
                     p         = p2;
                  }
                  p->bal = B;
                  grown  = FALSE;
                  break;
            }
         }
      }
   }
   /* requested key is greater than the current key, so go right */
   else
   {
      if ( (result = S_insert (&p->right)) == S_OK)
      {
         /* if the new node was inserted into the right sub-tree ... */
         if (grown == TRUE)
         {
            switch (p->bal)
            {
               case L:
                  p->bal = B;
                  grown  = FALSE;
                  break;

               case B:
                  p->bal = R;
                  break;

               /* tree is unbalanced to the right, so rebalance */
               case R:
                  p1 = p->right;
                  if (p1->bal == R)
                  {
                     p->right  = p1->left;
                     p1->left  = p;
                     p->bal    = B;
                     p         = p1;
                  }
                  else
                  {
                     p2        = p1->left;
                     p1->left  = p2->right;
                     p2->right = p1;
                     p->right  = p2->left;
                     p2->left  = p;
                     p->bal    = (p2->bal == R ? L : B);
                     p1->bal   = (p2->bal == L ? R : B);
                     p         = p2;
                  }
                  p->bal = B;
                  grown  = FALSE;
                  break;
            }
         }
      }
   }
   *pp_node = p;

   return (result);

}  /* end of function "S_insert" */

/* find the smallest power of two that is larger than the input value */

#ifdef __STDC__
static unsigned S_log2 (unsigned count)
#else
static unsigned S_log2 (count)
 unsigned count;
#endif
{
   unsigned power = 0;

   if (count > 0)
      do
      {
         count >>= 1;
         ++power;
      }
      while (count != 0);

   return (power);

}  /* end of function "S_log2" */

#ifdef __STDC__
static int S_delete (NODE **pp_root)
#else
static int S_delete (pp_root)
 NODE **pp_root;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   which    The result of the comparison of keys.
   shrank   A flag: 1 => the sub-tree has shrunk.
                    0 => the sub-tree has not shrunk.
                   <0 => error occurred.
   p_node   A pointer to the node to delete from the tree.
   */
   int   which;
   int   shrank = 0;
   NODE *p_dnode;

   if (*pp_root == (NODE *)NULL)
   {
      S_del = FALSE;
   }
   else
   {
      /* go to the left */
      if ( (which = (*(S_store->compare))((*pp_root)->key, S_key)) > 0)
      {
         if ( (shrank = S_delete (&(*pp_root)->left)) == 1)
            shrank = S_balance_l (pp_root);
      }
      /* go to the right */
      else if (which < 0)
      {
         if ( (shrank = S_delete (&(*pp_root)->right)) == 1)
            shrank = S_balance_r (pp_root);
      }
      /* delete this one */
      else
      {
         p_dnode = *pp_root;

         if (p_dnode->right == (NODE *)NULL)
         {
            *pp_root = p_dnode->left;
            shrank   = 1;
         }
         else if (p_dnode->left == (NODE *)NULL)
         {
            *pp_root = p_dnode->right;
            shrank   = 1;
         }
         else if ( (shrank = S_descend (&(*pp_root)->left, &p_dnode)) == 1)
         {
            shrank = S_balance_l (pp_root);
         }
         /* delete the stored key and data with the user-supplied function */
         if (S_store->delete (p_dnode->key, p_dnode->data) != 0)
            shrank = S_DERR;

         /* free the node */
         else
         {
            free (p_dnode);
         }
      }
   }
   return (shrank);

}  /* end of function "S_delete" */

#ifdef __STDC__
static int S_balance_l (NODE **pp_node)
#else
static int S_balance_l (pp_node)
 NODE **pp_node;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   p        A pointer to a tree node.
   */
   register NODE *p;
   register NODE *p1, *p2;
   int            b1,  b2;
   int            shrank = TRUE;

   p = *pp_node;

   switch (p->bal)
   {
      case L:
         p->bal = B;
         break;

      case B:
         p->bal = R;
         shrank = FALSE;
         break;

      case R:
         p1 = p->right;
         b1 = p1->bal;

         if (b1 >= B)
         {
            p->right = p1->left;
            p1->left = p;

            if (b1 != B)
               p->bal = p1->bal = B;
            else
            {
               p->bal  = R;
               p1->bal = L;
               shrank  = FALSE;
            }
            p = p1;
         }
         else
         {
            p2        = p1->left;
            b2        = p2->bal;
            p1->left  = p2->right;
            p2->right = p1;
            p->right  = p2->left;
            p2->left  = p;
            p->bal    = (b2 == R ? L : B);
            p1->bal   = (b2 == L ? R : B);
            p         = p2;
            p2->bal   = B;
         }
   }
   *pp_node = p;

   return shrank;

}  /* end of function "S_balance_l" */

#ifdef __STDC__
static int S_balance_r (NODE **pp_node)
#else
static int S_balance_r (pp_node)
 NODE **pp_node;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   p        A pointer to a tree node.
   */
   register NODE *p;
   register NODE *p1, *p2;
   int            b1,  b2;
   int            shrank = TRUE;

   p = *pp_node;

   switch (p->bal)
   {
      case R:
         p->bal = B;
         break;

      case B:
         p->bal = L;
         shrank = FALSE;
         break;

      case L:
         p1 = p->left;
         b1 = p1->bal;

         if (b1 <= B)
         {
            p->left   = p1->right;
            p1->right = p;

            if (b1 != B)
               p->bal = p1->bal = B;
            else
            {
               p->bal  = L;
               p1->bal = R;
               shrank  = FALSE;
            }
            p = p1;
         }
         else
         {
            p2        = p1->right;
            b2        = p2->bal;
            p1->right = p2->left;
            p2->left  = p1;
            p->left   = p2->right;
            p2->right = p;
            p->bal    = (b2 == L ? R : B);
            p1->bal   = (b2 == R ? L : B);
            p         = p2;
            p2->bal   = B;
         }
   }
   *pp_node = p;

   return shrank;

}  /* end of function "S_balance_r" */

#ifdef __STDC__
static int S_descend (NODE **pp_node, NODE **pp_dnode)
#else
static int S_descend (pp_node, pp_dnode)
 NODE **pp_node;
 NODE **pp_dnode;
#endif
{
   /* descend to the right-most node */
   if ((*pp_node)->right != (NODE *)NULL)
   {
      return (S_descend(&(*pp_node)->right,pp_dnode) ? S_balance_r(pp_node) : 0);
   }
   else
   {
      void *temp;

      /* exchange contents of this (right-most) node with the node to delete */

      temp              = (*pp_dnode)->key;
      (*pp_dnode)->key  = (*pp_node)->key;
      (*pp_node)->key   = temp;

      temp              = (*pp_dnode)->data;
      (*pp_dnode)->data = (*pp_node)->data;
      (*pp_node)->data  = temp;

      /* reset the node-to-delete to this (right-most) node */
      *pp_dnode = *pp_node;

      /* attach the left sub-tree of this (right-most) node to its parent,
         which removes this node from the tree */
      *pp_node = (*pp_node)->left;

      return 1;
   }
}  /* end of function "S_descend" */

#ifdef __STDC__
static int S_clear (NODE *p_node)
#else
static int S_clear (p_node)
 NODE *p_node;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the clear operation.
   */
   int result = S_OK;

   if (p_node->left != (NODE *)NULL)
      result = S_clear (p_node->left);

   if (result == S_OK)
   {
      if (S_store->delete (p_node->key, p_node->data) != 0)
         result = S_DERR;
      else if (p_node->right != (NODE *)NULL)
         result = S_clear (p_node->right);

      /* free the node */
      if (result == S_OK)
         (void)free (p_node);
   }
   return (result);

}  /* end of function "S_clear" */

#ifdef __STDC__
static int S_output (NODE *p_node, int (*output)(void *key, void *data) )
#else
static int S_output (p_node, output)
 NODE *p_node;
 int (*output)();
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   result   The result of the output operation.
   */
   int result = S_OK;

   if (p_node->left != (NODE *)NULL)
      result = S_output (p_node->left, output);

   if (result == S_OK)
   {
      if ( (result = (*output)(p_node->key, p_node->data)) == S_OK &&
          p_node->right != (NODE *)NULL)

         result = S_output (p_node->right, output);
   }
   return (result);

}  /* end of function "S_output" */

#ifdef DEBUG

/****************************************************************************
 This is all stuff for displaying the contents and structure of the tree.
 */

#ifdef DEBUG
#   define PAD()    printf("   ");
#   define PBAL(r)  printf("(%c)", r->bal==B ? 'B' : (r->bal==L ? 'L' : 'R') );
#else
#   define PAD()
#   define PBAL(r)
#endif

/* bit map for the print levels */
static char Map[16];

#define testbit(bit)  (Map[bit >> 3] & (1 << (bit & 0x07)) )

#ifdef __STDC__
static int S_tree (NODE *p_root, int amleft)
#else
static int S_tree (p_root, amleft)
 NODE *p_root;
 int   amleft;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   depth    A static counter for the current depth in the tree.
   i        A counter for the printing of connectors at each level.
   */
   static int depth = -1;
   int i;

   if (p_root != (NODE *)NULL)
   {
      ++depth;

      if (p_root->right != (NODE *)NULL)
         S_tree (p_root->right, 0);
      else
         S_setbit (depth + 1, 1);

      for (i = 1; i <= depth; ++i)
      {
         S_print ((NODE *)NULL);
         PAD();
         if (i == depth)
            printf ("  +--");
         else if (testbit (i))
            printf ("  |  ");
         else
            printf ("     ");
      }
      S_print (p_root);
      PBAL (p_root);
      printf ("%s\n", (p_root->left ? "--+"
                                    : (p_root->right ? "--+" : "")) );

      S_setbit (depth, amleft ? 0 : 1);

      if (p_root->left)
         S_tree (p_root->left, 1);
      else
         S_setbit (depth + 1, 0);

      --depth;
   }
   return S_OK;

}  /* end of function "S_tree" */

#ifdef __STDC__
static void S_setbit (int bit, int value)
#else
static void S_setbit (bit, value)
 int bit;
 int value;
#endif
{
   if (value != 0)
      /* turn "bit" on */
      Map[bit >> 3] |=  (1 << (bit & 0x07));

   else
      /* turn "bit" off */
      Map[bit >> 3] &= ~(1 << (bit & 0x07));

}  /* end of function "S_setbit" */

#ifdef __STDC__
static void S_print (NODE *p)
#else
static void S_print (p)
 NODE *p;
#endif
{
   if (p != (NODE *)NULL)
      printf ("%2.2s", (char *)p->key);
   else
      printf ("  ");

}  /* end of function "S_print" */

#endif
