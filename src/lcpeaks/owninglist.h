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

// File     : owninglist.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : This list ensures that the contained objects are
//            never reallocated. So, pointers on items are reliable.
//
//            WARNING : This class does not check for two pointers on the
//            same object. Such a situation may cause some
//            unexpected behavior.
//
// $History: owninglist.h $
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

#ifndef _OWNINGLIST_H
#define _OWNINGLIST_H

#include<vector>

template<class T>
class OwningList {
private:
    typedef vector<T*> _type_list;
    _type_list _list ;

    struct _delete_owned_object
    {
        void operator()(T* item){delete item;}
    };

public:
    OwningList();
    ~OwningList();            //destroys the items of the list

    int size() ;              //number of elements in the list
    T& operator[] (int i) ;
    T& push_back() ;          //the object is created and owned by the list
    T& push_back(T& item) ;   //the object is added at the end and owned by the list
    T& front();               //first object of the list
    T& back() ;               //last object of the list
    void erase(T& item) ;     //remove the object from the list
    void erase(int index) ;
    void pop_back();
    void clear();
    int indexof(T& item);
};

#ifdef TESTING
#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>
class TEST_OwningList_Item
{
public:
    int a,b ;
    TEST_OwningList_Item() { a=0; b=0;};
    ~TEST_OwningList_Item(){ };
    friend int operator== (TEST_OwningList_Item& lhs, TEST_OwningList_Item& rhs)
    { return (lhs.a == rhs.a) &&  (lhs.b == rhs.b) ; }
};

class TEST_OwningList : public CppUnit::TestFixture
{
private:
    typedef TEST_OwningList_Item         tested_item_type ;
    typedef OwningList<tested_item_type> tested_list_type ;

    tested_list_type* tstlist ;

public:
    void setUp()    { tstlist = new tested_list_type ;}
    void tearDown() { delete tstlist ; }

    void basic_things()
    {
        tstlist->push_back();
        CPPUNIT_ASSERT(tstlist->size()==1);
        tstlist->pop_back();
        CPPUNIT_ASSERT(tstlist->size()==0);
        CPPUNIT_ASSERT( &(tstlist->front()) == &((*tstlist)[0]) ) ;
        CPPUNIT_ASSERT(  (tstlist->front()) ==   (*tstlist)[0]  ) ;
    }

    void reliable_pointers()
    {
        TEST_OwningList_Item& anitem = tstlist->push_back();
        anitem.a = 10; anitem.b = 20;
        CPPUNIT_ASSERT(tstlist->size() == 1);

        TEST_OwningList_Item* pbfr = new TEST_OwningList_Item ;
        TEST_OwningList_Item& bfr = tstlist->push_back(*pbfr);
        bfr.a=1;  bfr.b=2;
        CPPUNIT_ASSERT(tstlist->size() == 2);

        for(int i=0; i<10000; ++i) tstlist->push_back() ; //to induce reallocation
        CPPUNIT_ASSERT(tstlist->size() == 10002);

        TEST_OwningList_Item& aftr = (*tstlist)[10];
        aftr.a=3;  aftr.b=4;

        CPPUNIT_ASSERT( &anitem == &(tstlist->front()) )  ; //same pointer
        CPPUNIT_ASSERT( (anitem.a==10) && (anitem.b==20) ); //content as expected
        CPPUNIT_ASSERT( anitem == tstlist->front() )      ; //same content
        CPPUNIT_ASSERT( bfr    == (*tstlist)[1] ) ;
        CPPUNIT_ASSERT( (aftr.a == 3) && (aftr.b == 4) ) ;
        CPPUNIT_ASSERT( aftr   == (*tstlist)[10] ) ;
        CPPUNIT_ASSERT( &aftr   == &(*tstlist)[10] ) ;

        tstlist->erase((*tstlist)[5]) ;

        CPPUNIT_ASSERT( &anitem == &(tstlist->front()) )  ; //same pointer
        CPPUNIT_ASSERT( (anitem.a==10) && (anitem.b==20) ); //content as expected
        CPPUNIT_ASSERT( anitem == tstlist->front() )      ; //same content
        CPPUNIT_ASSERT( bfr    == (*tstlist)[1] ) ;
        CPPUNIT_ASSERT( aftr   == (*tstlist)[9] ) ;
        CPPUNIT_ASSERT( &aftr   == &(*tstlist)[9] ) ;
        CPPUNIT_ASSERT( (aftr.a == 3) && (aftr.b == 4) ) ;

        tstlist->erase(5) ;

        CPPUNIT_ASSERT( &anitem == &(tstlist->front()) )  ; //same pointer
        CPPUNIT_ASSERT( (anitem.a==10) && (anitem.b==20) ); //content as expected
        CPPUNIT_ASSERT( anitem == tstlist->front() )      ; //same content
        CPPUNIT_ASSERT( bfr    == (*tstlist)[1] ) ;
        CPPUNIT_ASSERT( aftr   == (*tstlist)[8] ) ;
        CPPUNIT_ASSERT( &aftr   == &(*tstlist)[8] ) ;
        CPPUNIT_ASSERT( (aftr.a == 3) && (aftr.b == 4) ) ;

        tstlist->clear();
    }

    void indexof() {
        TEST_OwningList_Item *a,*b,*c,*d,*x ;
        a=new TEST_OwningList_Item ;
        b=new TEST_OwningList_Item ;
        c=new TEST_OwningList_Item ;
        d=new TEST_OwningList_Item ;
        x=new TEST_OwningList_Item ;

        tstlist->push_back(*a);
        tstlist->push_back();
        tstlist->push_back(*b);
        tstlist->push_back();
        tstlist->push_back(*c);
        tstlist->push_back();
        tstlist->push_back();
        tstlist->push_back(*d);
        tstlist->push_back();

        CPPUNIT_ASSERT(tstlist->indexof(*a) == 0);
        CPPUNIT_ASSERT(tstlist->indexof(*b) == 2);
        CPPUNIT_ASSERT(tstlist->indexof(*c) == 4);
        CPPUNIT_ASSERT(tstlist->indexof(*d) == 7);
        CPPUNIT_ASSERT(tstlist->indexof(*x) == -1);
        CPPUNIT_ASSERT(tstlist->size() == 9);

        tstlist->erase(1);

        CPPUNIT_ASSERT(tstlist->indexof(*a) == 0);
        CPPUNIT_ASSERT(tstlist->indexof(*b) == 1);
        CPPUNIT_ASSERT(tstlist->indexof(*c) == 3);
        CPPUNIT_ASSERT(tstlist->indexof(*d) == 6);
        CPPUNIT_ASSERT(tstlist->indexof(*x) == -1);
        CPPUNIT_ASSERT(tstlist->size() == 8);

        tstlist->erase(*b);

        CPPUNIT_ASSERT(tstlist->indexof(*a) == 0);
        CPPUNIT_ASSERT(tstlist->indexof(*c) == 2);
        CPPUNIT_ASSERT(tstlist->indexof(*d) == 5);
        CPPUNIT_ASSERT(tstlist->indexof(*x) == -1);
        CPPUNIT_ASSERT(tstlist->size() == 7);

        delete x;

    }

//Defines the test suite :
    CPPUNIT_TEST_SUITE(TEST_OwningList);
    CPPUNIT_TEST(basic_things);
    CPPUNIT_TEST(reliable_pointers);
    CPPUNIT_TEST(indexof);
    CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION(TEST_OwningList) ;

#endif

#include "owninglist.c"

#endif
