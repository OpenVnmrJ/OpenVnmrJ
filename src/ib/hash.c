/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
	static char Sid[] = "@(#)hash.c 18.1 03/21/08 20:01:10 Copyright 1990 Spectroscopy Imaging Systems";
#endif (not) lint

#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif
#include <string.h>
#ifndef MSDOS
#include <unistd.h>
#endif

/* these includes pick up the SID strings in the header files */
#define HEADER_ID
#include "getprime.h"
#undef HEADER_ID

#include "error.h"
#include "storage.h"
#include "getprime.h"
#ifdef DEBUG
#include "debug_alloc.h"
#endif

/* The storage area is implemented here as a hash table: for an input key,
   the hash function computes the (offset) address in the table, which points
   to the head of a linked list of data structures (to handle hash-function
   collisions).  The structure of this table looks like:

     STORAGE
        |
        |
        V
    ---------
   | size    |  (the number of *ENTRY items allocated in the table)
   |---------|
   | compare |  (the function for comparing hash-table keys)
   |---------|
   | delete  |  (the function for deleting hash-table entries)
   |---------|
   | where   |  (the function for computing the hash-table index)
   |---------|
   | s_index |  (the index of the next entry to get, for scanning the table)
   |---------|
   | s_entry |  (the next *ENTRY pointer to reference, for scanning the table)
   |---------|
   | table   |----> --------
    ---------  (0) | *ENTRY |--> -------
                   |--------|   | *next |--> -------
               (1) | *ENTRY |   | *key  |   | *next |
                   |--------|   | *data |   | *key  |
               (2) | *ENTRY |    -------    | *data |
                   |--------|                -------
               (3) | *ENTRY |--> -------
                   |--------|   | *next |--> -------
                   |  ....  |   | *key  |   | *next |--> -------
                                | *data |   | *key  |   | *next |
                                 -------    | *data |   | *key  |
                                             -------    | *data |
                                                         -------
   NOTES:
   1. The "s_entry" pointer and the unsigned integer "s_index" items in the
      STORAGE structure are used by the "Storage_first()" and "Storage_next()"
      functions, to store the current position in the table between scans of
      the table entries.
   2. The "key" pointer in each ENTRY structure is typed "void *" for generality.
   3. The "data" pointer in each ENTRY structure is a pointer to the data stored
      for each "key", typed "void *" for generality.
 */

/* the data structure used to hold entries in the storage area */
typedef struct _entry {
   struct _entry *next;
   void          *key;
   void          *data;
} ENTRY;

typedef struct {
   unsigned size;
#  if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
   int      (*compare)(void *, void *);
   int      (*delete)(void *, void *);
   unsigned (*where)(void *, unsigned);
#  else
   int      (*compare)();
   int      (*delete)();
   unsigned (*where)();
#  endif
   unsigned   s_index;
   ENTRY     *s_entry;
   ENTRY    **table;
} STORAGE;

/* the number of prime numbers in the list used for finding the
   smallest prime greater than or equal to the requested table size */
#define LIST_SIZE 100

/* functions local to this module */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

static ENTRY   *S_search (STORAGE *store, void *key, ENTRY ***slot);
static unsigned S_hash (void *string, unsigned size);

#else

static ENTRY   *S_search( /* STORAGE *store, void *key, ENTRY ***slot */ );
static unsigned S_hash( /* void *string, unsigned size */ );

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
               the appplication, that holds the data, and possbly the key,
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
                     couldn't allocate memory for the storage-area entries
                     couldn't find the next largest prime number for the "size"
                     (see function "GetPrime()")
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
Storage Storage_create (unsigned size, int (*compare)(void *key1, void *key2),
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
   /* get a prime number greater than or equal to "size" */
   else if ( (size = GetPrime (size)) == 0)
   {
      ;
   }
   /* allocate memory for the storage header structure */
   else if ( (store = (STORAGE *)calloc (1, sizeof(STORAGE))) == (STORAGE *)NULL)
   {
      SYS_ERROR(("Storage_create: calloc one storage header"));
   }
   /* allocate memory for the storage table */
   else if ( (store->table = (ENTRY **)calloc (size, sizeof(ENTRY *))) ==
            (ENTRY **)NULL)
   {
      SYS_ERROR(("Storage_create: calloc storage memory for %d entries",size));
      (void)free ( (char *)store );
      store = (STORAGE *)NULL;
   }
   else
   {
      /* set the size of this storage area */
      store->size = size;

      /* set the function pointers for this storage area */
      store->compare = compare;
      store->delete  = delete;
      store->where   = S_hash;
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
   none
 GLOBALS CHANGED:
   none
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
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

   entry    A pointer to the entry for this key in the storage area.
   slot     An pointer to the slot in the hash-table array for this key.
   result   The result of the insertion operation.
   */
   ENTRY  *entry;
   ENTRY **slot;
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
   /* search the collision list for this key */
   else if ( (entry = S_search ((STORAGE *)store, key, &slot)) != (ENTRY *)NULL)
   {
      /* if requested, replace the previous data for this key */
      if (action == REPLACE)
      {
         /* if the deletion succeeded, load the new data */
         if (((STORAGE *)store)->delete (entry->key, entry->data) == 0)
         {
            entry->key  = key;
            entry->data = data;

            PRINTF((" - REPLACED\n"));
         }
         /* otherwise, report an error */
         else
         {
            result = S_REPL;

            PRINTF(("\n"));
         }
      }
      else
         result = S_RPT;
   }
   /* allocate memory for a new entry in the collision list */
   else if ( (entry = (ENTRY *)malloc (sizeof(ENTRY))) != (ENTRY *)NULL)
   {
#ifdef DEBUG
      if (*slot != (ENTRY *)NULL)
         PRINTF((" - COLLISION %s\n",key));
#endif
      /* link the new entry into the head of the collision list */
      entry->next = *slot;
      *slot = entry;

      /* put the key and data pointers into the new entry */
      entry->key  = key;
      entry->data = data;

      PRINTF((" - ADDED\n"));
   }
   else
   {
      SYS_ERROR(("Storage_insert: malloc new key '%s'",(char *)key));
      result = S_INS;
   }
   return (result);

}  /* end of function "Storage_insert" */

/****************************************************************************
 Storage_search

 This function searches the input storage area for the input search key, and
 returns a pointer to the data stored for that key.

 INPUT ARGS:
   store       The storage area to be searched for the key.
   key         A pointer to the storage key.
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
void *Storage_search (Storage store, void *key)
#else
void *Storage_search (store, key)
 Storage store;
 void   *key;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   entry    A pointer to the entry for this key in the storage area.
   slot     An pointer to the slot in the hash-table array for the key.
   */
   ENTRY  *entry;
   ENTRY **slot;
   void   *data = (void *)NULL;

   if (store == (Storage)NULL)
      ERROR(("Storage_search: NULL pointer to storage area"));

   else if ( (entry = S_search ((STORAGE *)store, key, &slot)) != (ENTRY *)NULL)
      data = entry->data;

   return (data);

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
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   (void *)NULL   Returned on error:
                     the reference to the storage area is (Storage)NULL
                     nothing was found in the storage area
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
void *Storage_first (Storage store)
#else
void *Storage_first (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   table    A pointer to the start of the hash-table array in this storage area.
   index    The index of the current collision list in the hash-table array.
   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   data     A pointer to the data stored for an entry in the hash table.
   */
   ENTRY  **table;
   unsigned index;
   ENTRY   *entry;
   void    *data = (void *)NULL;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_first: NULL pointer to storage area"));
   }
   else
   {
      /* search through the elements in the hash-table array for an entry */
      for (table = ((STORAGE *)store)->table, index = 0;
           index < ((STORAGE *)store)->size; ++index)
      {
         /* if an entry was found ... */
         if ( (entry = *(table + index)) != (ENTRY *)NULL)
         {
            PRINTF(("index %3d: '%s'",index,entry->key));

            /* set the data pointer for the return */
            data = entry->data;

            /* set the current position for the next access */
            entry = entry->next;
            break;
         }
      }
      /* save the current position in the hash table */
      ((STORAGE *)store)->s_index = index;
      ((STORAGE *)store)->s_entry = entry;
   }
   return (data);

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
                     nothing was found in the storage area
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
void *Storage_next (Storage store)
#else
void *Storage_next (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   table    A pointer to the start of the hash-table array in this storage area.
   index    The index of the current collision list in the hash-table array.
   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   data     A pointer to the data stored for an entry in the hash table.
   */
   ENTRY  **table;
   unsigned index;
   ENTRY   *entry;
   void    *data = (void *)NULL;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_next: NULL pointer to storage area"));
   }
   else
   {
      /* get the pointer to the start of the hash-table array */
      table = ((STORAGE *)store)->table;

      /* get the saved index */
      index = ((STORAGE *)store)->s_index;

      /* get the saved link pointer */
      entry = ((STORAGE *)store)->s_entry;

      /* if nothing at this index, go to the next element in the array */
      if (entry == (ENTRY *)NULL)
      {
         while (++index < ((STORAGE *)store)->size)
            if ( (entry = *(table + index)) != (ENTRY *)NULL)
               break;
      }
      /* if an entry was found ... */
      if (entry != (ENTRY *)NULL)
      {
         PRINTF(("index %3d: '%s'",index,entry->key));

         /* set the data pointer for the return */
         data = entry->data;

         /* set the current position for the next access */
         entry = entry->next;
      }
      /* save the current position in the hash table */
      ((STORAGE *)store)->s_index = index;
      ((STORAGE *)store)->s_entry = entry;
   }
   return (data);

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
   none
 GLOBALS CHANGED:
   none
 ERRORS:
               Returned on error:
   S_BAD          the reference to the storage area is (Storage)NULL
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int Storage_delete (Storage store, void *key)
#else
int Storage_delete (store, key)
 Storage store;
 void   *key;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   pe       The address of the pointer in "entry", used for replacing the
            pointer to the next link in the list.
   result   The result of the delete operation.
   */
   ENTRY  *entry;
   ENTRY **pe;
   int     result = S_OK;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_delete: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if ( ((STORAGE *)store)->delete == (int(*)())NULL)
   {
      ERROR(("Storage_delete: no delete function has been supplied"));
      result = S_DEL;
   }
   else
   {
      /* set the location in the hash-table array of the pointer to the head
         of the collision list (if it exists) */
      pe = ((STORAGE *)store)->table +
           ((STORAGE *)store)->where (key, ((STORAGE *)store)->size);

      /* search the collision list for this entry in the table */
      for (entry = *pe; entry != (ENTRY *)NULL;
           pe = &(entry->next), entry = entry->next)
      {
         /* use the storage-area's compare function to check this entry */
         if (((STORAGE *)store)->compare (entry->key, key) == 0)
         {
            /* call the storage-area's delete function to free the memory
               used by this entry */
            if (((STORAGE *)store)->delete (entry->key, entry->data) != 0)
               return (S_DERR);

            /* replace the link pointer to eliminate this entry */
            *pe = entry->next;

            /* free the memory used by the list link */
            (void)free ( (char *)entry );

            return (S_OK);
         }
      }
      ERROR(("Storage_delete: key '%s' not in storage area",key));
      result = S_SRCH;
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
   ! S_OK      Operation failed: see ERRORS below
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int Storage_clear (Storage store)
#else
int Storage_clear (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   table    A pointer to the start of the hash-table array in this storage area.
   index    The index of the current collision list in the hash-table array.
   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   e        A working pointer for saving collision-list pointers.
   result   The result of the clear operation.
   */
   ENTRY  **table;
   unsigned index;
   ENTRY   *entry;
   ENTRY   *e;
   int      result = S_EMT;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_clear: NULL pointer to storage area"));
      result = S_BAD;
   }
   else if ( ((STORAGE *)store)->delete == (int(*)())NULL)
   {
      ERROR(("Storage_clear: no delete function has been supplied"));
      result = S_DEL;
   }
   else
   {
      /* search through the elements in the hash-table array for an entry */
      for (table = ((STORAGE *)store)->table, index = 0;
           index < ((STORAGE *)store)->size; ++index)
      {
         /* if an entry was found ... */
         if ( (entry = *(table + index)) != (ENTRY *)NULL)
         {
            /* clear all entries in this collision list */
            do
            {
               /* call the storage-area's delete function to free the memory
                  used by this entry */
               if (((STORAGE *)store)->delete (entry->key, entry->data) != 0)
                  return (S_DERR);

               /* save the pointer to the next link in the list */
               e = entry->next;

               /* free the memory used by the current link */
               (void)free ( (char *)entry );

            } while ( (entry = e) != (ENTRY *)NULL);

            /* clear the pointer in the hash-table array */
            *(table + index) = (ENTRY *)NULL;

            result = S_OK;
         }
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
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
      /* free the hash-table array */
      (void)free ( ((STORAGE *)store)->table );

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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int Storage_output (Storage store, int (*output)(void *key, void *data) )
#else
int Storage_output (store, output)
 Storage store;
 int (*output)();
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   table    A pointer to the start of the hash-table array in this storage area.
   index    The index of the current collision list in the hash-table array.
   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   data     A pointer to the data stored for an entry in the hash table.
   result   The result of the output operation.
   */
   ENTRY  **table;
   unsigned index;
   ENTRY   *entry;
   int      result = S_EMT;

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
   else
   {
      /* search through the elements in the hash-table array for an entry */
      for (table = ((STORAGE *)store)->table, index = 0;
           index < ((STORAGE *)store)->size; ++index)
      {
         /* if an entry was found ... */
         if ( (entry = *(table + index)) != (ENTRY *)NULL)
         {
            /* output all entries in this collision list */
            do
            {
               /* call the function to output this entry */
               if ( (result = (*output) (entry->key, entry->data)) != 0)
                  return (result);

            } while ( (entry = entry->next) != (ENTRY *)NULL);

            result = S_OK;
         }
      }
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int Storage_display (Storage store)
#else
int Storage_display (store)
 Storage store;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   i        A counter for the hash-table array.
   entry    A pointer to an entry in the collision list.
   result   The result of the display operation.
   */
   unsigned i;
   ENTRY   *entry;
   int      result = S_EMT;

   if (store == (Storage)NULL)
   {
      ERROR(("Storage_display: NULL pointer to storage area"));
      result = S_BAD;
   }
   else
   {
      for (i = 0; i != ((STORAGE *)store)->size; ++i)
      {
         PRINTF(("%3u ",i));

         for (entry = *(((STORAGE *)store)->table+i); entry != (ENTRY *)NULL;
              entry = entry->next)
         {
            PRINTF(("- %s ", entry->key));
            result = S_OK;
         }
         PRINTF(("\n"));
      }
   }
   return (result);

}  /* end of function "Storage_display" */

#endif

/****************************************************************************
 S_search

 This function searches for the input key in the input storage area.

 INPUT ARGS:
   store       The storage area in which to search for the key.
   key         A pointer to the storage key.
   slot        The address of a pointer to the location (offset) in the
               hash-table array that was computed for this key.
 OUTPUT ARGS:
   slot        A pointer to the location (offset) in the hash-table array
               that was computed for this key.
               NOTE: this is NOT the location of the entry in the storage
               area for this key.
 RETURN VALUE:
   ENTRY *     The location of the entry in the storage area; if the key was
               not found, NULL.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none        This is a local function: pointers are checked before calling.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
static ENTRY *S_search (STORAGE *store, void *key, ENTRY ***slot)
#else
static ENTRY *S_search (store, key, slot)
 STORAGE *store;
 void    *key;
 ENTRY ***slot;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   table    A pointer to the start of the hash-table array in this storage area.
   entry    A pointer into the collision list for the current entry in the
            hash-table array.
   */
   ENTRY **table;
   ENTRY  *entry;

   /* use the key and table size to compute the offset of this entry in
      the hash-table array, and report the location */
   *slot = table = store->table + store->where (key, store->size);

   PRINTF(("S_search: index %3d: '%s'",store->where(key,store->size),key));

   /* search the collision list for this entry in the table */
   for (entry = *table; entry != (ENTRY *)NULL; entry = entry->next)
   {
      /* use the storage-area's compare function to check this entry */
      if (store->compare (entry->key, key) == 0)
         break;
   }
   return (entry);

}  /* end of function "S_search" */

/****************************************************************************
 S_hash

 This function computes the address in the hash table, with the given size,
 of the input character string.

 INPUT ARGS:
   string      A character string for which a hash-table address is wanted.
   size        The size of the hash table; should be a prime number.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   (unsigned)  The offset in the hash table, with the input size, of the input
               character string. If an error was detected, zero.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   0           The pointer to the input string is NULL, or there are no
               characters in the string.
 EXIT VALUES:
   none
 NOTES:
   The hash function takes the value of each character and adds it to a
   running sum. Characters after the first one are shifted left by a certain
   proportion of the word size; carry-out into the high bits of the sum are
   randomized with an XOR algorithm. The final value of the sum, modulo the
   table size, is the value returned. See Aho's "dragon book" for analysis.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
static unsigned S_hash (void *string, unsigned size)
#else
static unsigned S_hash (string, size)
 void    *string;
 unsigned size;
#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   sum   The sum of the (shifted) characters in the string.
   temp  A temporary for randomizing a too-large hash value.
   p     A fast pointer for hashing the input string.
   */
   register unsigned sum = 0;
   register unsigned temp;
   register char    *p = (char *)string;

#define NBITS_IN_UNSIGNED    ( 8 * sizeof(unsigned int) )
#define SEVENTY_FIVE_PERCENT ( (int)(NBITS_IN_UNSIGNED * .75) )
#define TWELVE_PERCENT       ( (int)(NBITS_IN_UNSIGNED * .125) )
#define HIGH_BITS            (~( (unsigned)(~0) >> TWELVE_PERCENT) )

   if (p != (char *)NULL && *p != '\0')
   {
      for (sum = *p++ ; *p != '\0'; ++p)
      {
         sum = (sum << TWELVE_PERCENT) + *p;
         if (temp = sum & HIGH_BITS)
            sum = (sum ^ (temp >> SEVENTY_FIVE_PERCENT)) & ~HIGH_BITS;
      }
   }
   return (sum % size);

}  /* end of function "S_hash" */
