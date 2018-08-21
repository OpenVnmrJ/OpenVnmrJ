/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;


public class SampleInfo implements SmsDef
{
   public String  user;
   public String  studyId;
   public String  name;
   public String  solvent;
   public String  notebook;
   public String  page;
   public String  statusStr;
   public String  date;
   public String  barcode;
   public String  fileName;
   public int     status;
   public long    iTime; // time saved
   public boolean bValidated;

   public SampleInfo() {
   }

   public void clear() {
        bValidated = false;
        status = OPEN;
        if (fileName == null)
           return;
        user = null;
        studyId = null;
        name = null;
        solvent = null;
        notebook = null;
        page = null;
        fileName = null;
        statusStr = null;
        date = null;
        barcode = null;
   }

   public void inValidate() {
        bValidated = false;
   }

   public void validate() {
        bValidated = true;
   }

   public boolean isValidated() {
        return bValidated;
   }
}

