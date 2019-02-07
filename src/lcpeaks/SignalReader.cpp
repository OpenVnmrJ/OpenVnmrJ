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

// $History: SignalReader.cpp $
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

#include "common.h"
#include <iostream>
#include <fstream>
#include "SignalReader.h"

/* --------------------------
     * TSignalReader::ReadFile
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Read data from file
 *           Instanciates a new signal array
 * History :
 *-------------------------- */
int TSignalReader::ReadFile(char* filename){
//cout<<"Reading file "<<filename<<endl;

//delete [] Signal ;

    std::ifstream datafile(filename);

    int size;
    datafile >> size ;

    if (datafile.good()) {
        Signal = new double[size];
        _size = size ;
    }

    int i = 0 ;
    while(datafile.good() && (i<size) ) datafile >> Signal[i++] ;

    return 0 ;
};
