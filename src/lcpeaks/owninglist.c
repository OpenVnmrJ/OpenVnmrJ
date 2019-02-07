/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* DISCLAIMER :
     * ------------
     *
 * This is a beta version of the GALAXIE integration library.
 * This code is under development and is provided for information purposes.
 * The classes names and interfaces as well as the file names and
 * organization is subject to changes. Moreover, this code has not been
 * fully tested.
 *
 * For any bug report, comment or suggestion please send an email to
 * gilles.orazi@varianinc.com
 *
 * Copyright Varian JMBS (2002)
 */

// $History: owninglist.cpp $
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "owninglist.h"
#include <algorithm>

template<class T>
OwningList<T>::OwningList()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
//nothing for the moment...
}

template<class T>
OwningList<T>::~OwningList()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    clear();
}

template<class T>
int OwningList<T>::size()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return _list.size();
}

template<class T>
T& OwningList<T>::operator[] (int i)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return *(_list[i]);
}

template<class T>
T& OwningList<T>::push_back()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    T* newitem = new T ;
    _list.push_back(newitem);
    return *newitem ;
}

template<class T>
T& OwningList<T>::push_back(T& item)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    _list.push_back(&item);
    return item;
}

template<class T>
void OwningList<T>::pop_back()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{ 
    erase(back()) ;
}

template<class T>
T& OwningList<T>::front()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return *(_list.front());
}

template<class T>
T& OwningList<T>::back()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return *(_list.back());
}

template<class T>
void OwningList<T>::erase(T& item)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    typename _type_list::iterator item_it = find(_list.begin(), _list.end(), &item);
    if (item_it != _list.end()) _list.erase(item_it);
    delete &item;
}

template<class T>
void OwningList<T>::erase(int index)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    typename _type_list::iterator it_item = _list.begin()+index ;
    T* pitem = *it_item;
    _list.erase(it_item);
    delete pitem;
}

template<class T>
int OwningList<T>::indexof(T& item)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    typename _type_list::iterator it = find(_list.begin(), _list.end(), &item);
    if (it!=_list.end())
        return it - _list.begin();
    else
        return -1 ;
}

template<class T>
void OwningList<T>::clear()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    for_each(_list.begin(), _list.end(), _delete_owned_object());
    _list.clear();
}

// template<class T>
// iterator OwningList::begin()
// {
//   return _list.begin();
// }

// template<class T>
// iterator OwningList::end()
// {
//   return _list.end();
// }


