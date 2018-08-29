/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
/********************************************************************/
/* Name        : constants.h                                        */
/* Description : Defines all the constants used in the storage and  */
/*                                         conversion applications  */
/* Created by  : Mindteck (India) Limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on :                                                    */
/********************************************************************/

/* Error codes */
#define SUCCESS              0
#define ERROR_FILE_OPEN     -1
#define ERROR_USAGE         -2

/* Path and names of configuration files */
#define STORE_IMAGE_CONF   "dicom_store.cfg"

/* Path and names of Log files */
#define START_SERVER_LOG       "/error.log"
#define STORE_IMAGE_LOG        "/error.log"
#define CREATE_CTN_INPUT_LOG   "/error.log"
#define CREATE_DICOM_LOG       "/error.log"
#define SPLIT_FDF_LOG          "/error.log"

/* Path and names of Output files */
#define CTN_INPUT_FILE         "ctn.input"

/* Names of temporary DICOM file */
#define TEMPORARY_DICOM_FILE   "temp.dcm"

/* Data Type */
#define BOOLEAN            int 
#define FALSE                0 
#define TRUE                 1 

/* Length of variables used */
#define TAG_LEN             21 
#define VALUE_LEN           81 
#define CONF_FILE_NAME_LEN 171 
#define CONTROL_DB_NAME_LEN 71 
#define LOG_FILE_NAME_LEN  256 
#define IP_LEN              71 
#define PORT_LEN            21 
#define CTN_ARGUMENT_LEN   501 
#define PATH_LEN           501 
#define COMMAND_LEN       1051 
#define AE_TITLE_LEN        71 
#define IMAGE_NAME_LEN     171 
#define MSG_LEN           1071 
#define BUFFER_LEN         256 
#define GROUP_LEN            5 
#define ELEMENT_LEN          5 
#define VR_LEN               3 
#define DATA_TYPE_LEN       51 
#define NAME_LEN            51
#define EQUAL_TO_LEN        51

/* Default values of variables in configuration file      */
#define DEFAULT_STR_DB      " -f CTNControl "
#define DEFAULT_STR_LOG     " -l /archive_server.log "

#define DEFAULT_STR_HOST    "localhost"
#define DEFAULT_STR_PORT    "1114"
#define DEFAULT_STR_CTN_BIN_PATH   ""

#define DEFAULT_STR_AE_TITLE_EXTERNAL   " "
#define DEFAULT_STR_AE_TITLE_CTN   "STORESCU"

