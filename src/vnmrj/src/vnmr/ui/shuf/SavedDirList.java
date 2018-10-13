/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.util.*;
import java.io.*;

import vnmr.util.*;
import vnmr.bo.*;

/********************************************************** <pre>
 * Summary: Keep a list of directories where this user has saved
 *      files which are not standard dataDir directories.
 *
 *      Use the list in updateDB to regenerate the DB
 * 
 </pre> **********************************************************/


public class SavedDirList {
    private ArrayList savedDirList;
    private ArrayList dataDirList;
    private User      user;

    public SavedDirList(User user) {

        this.user = user;

        savedDirList = readSavedDirList();

        // Get this users dataDir list and save it for future use
        dataDirList = user.getDataDirectories();

    }


    /******************************************************************
     * Summary: If this dir is unique, add it to the list.
     *
     *  If this dir already in the list, return
     *  If this dir a child of a dir already in the list, return
     *  If this dir already in the dataDir list, return
     *  If this dir a child of a dir already in the dataDir list, return
     *  Else, add the dir to the list
     *****************************************************************/

    public void addToList(String dir) {
        String canonDir;

        try {
            UNFile file = new UNFile(dir);
            canonDir = file.getCanonicalPath();

            // Is this dir already in the list?
            if(savedDirList.contains(canonDir))
                return;

            // Is this dir a child of a dir already in the list?
            // Go through the items already in the list
            // and try to avoid subdirectories of parents
            // in the list.  ie.,  just keep the highest
            // level in the list because we fill the
            // DB from the list recursively.
            String dirInList;
            for(int i=0; i < savedDirList.size(); i++) {
                dirInList = (String)savedDirList.get(i);
                // If we have this parent or dup, don't add
                if(canonDir.startsWith(dirInList))
                    return;
                // If the new one is a parent, remove the
                // current item and add the new one.
                else if(dirInList.startsWith(canonDir)) {
                    savedDirList.remove(i);
                    savedDirList.add(canonDir);

                    writeSavedDirList();
                    return;
                }
            }

            // Is this dir already in the dataDir list?
            if(dataDirList.contains(canonDir))
                return;

            // Is this dir a child of a dir already in the dataDir list?
            for(int i=0; i < dataDirList.size(); i++) {
                dirInList = (String)dataDirList.get(i);
                // If we have this parent or dup, don't add
                if(canonDir.startsWith(dirInList))
                    return;
            }

        
            // None of the above, add it to the list and update the persistence
            // file.
            savedDirList.add(canonDir);

            // Update the persistence file
            writeSavedDirList();
        }
        catch (Exception e) {
            Messages.postError("Problem adding " + dir + 
                               "\n    to SavedDirList");
            Messages.writeStackTrace(e);
        }
        
    }

    // return a clone of the list
    public ArrayList getSavedDirList() {
        ArrayList cList = (ArrayList) savedDirList.clone();
        return cList;
    }


    /******************************************************************
     * Summary: Read the SavedDirList file for this user and return the dir 
     *  list.   If no file, return an empty list.
     *
     *****************************************************************/
    public ArrayList readSavedDirList() {

// Stop Using this file and use the Vnmrj adm Data Directories instead 7/13
ArrayList list = new ArrayList();
return list;

//        String filepath = FileUtil.userDir(user, "PERSISTENCE") +
//                          File.separator + "SavedDirList";
//
//        ArrayList list = new ArrayList();
//
//        if(filepath != null) {
//            BufferedReader in;
//            String line;
//            String path;
//            StringTokenizer tok;
//            try {
//                UNFile file = new UNFile(filepath);
//                in = new BufferedReader(new FileReader(file));
//                // File must start with 'Saved Data Directories'
//                if((line = in.readLine()) != null) {
//                    if(!line.startsWith("Saved Data Directories")) {
//                        Messages.postWarning("The " + filepath + " file is " +
//                                             "corrupted and being removed");
//                        file = new UNFile(filepath);
//                        // Remove the corrupted file.
//                        file.delete();
//                        return list;
//                    }
//                }
//                while ((line = in.readLine()) != null) {
//                    if(!line.startsWith("#")) {// skip comments.
//                        tok = new StringTokenizer(line, " :,\t\n");
//                        path = tok.nextToken();
//                        if(path.indexOf(File.separator) >= 0) {
//                            // See if the file/dir still exists.  Otherwise,
//                            // files which disappear will stay in this list
//                            // forever.
//                            file = new UNFile(path);
//                            if(file.exists()) {
//                                // It exists, add it
//                                list.add(path);
//                            }
//                        }
//                    }
//                }
//                in.close();
//            }
//            catch (Exception ioe) { }
//        }
//        return list;


    }

    /******************************************************************
     * Summary: Write out the SavedDirList file as persistence.
     *
     *****************************************************************/

    public void writeSavedDirList() {
       String filepath = FileUtil.savePath("USER/PERSISTENCE/SavedDirList");

        FileWriter fw;
        PrintWriter os;
        try {
              UNFile file = new UNFile(filepath);
              fw = new FileWriter(file);
              os = new PrintWriter(fw);
              os.println("Saved Data Directories");
              for(int i=0;i<savedDirList.size();i++) {
                  String s = (String) savedDirList.get(i);
                  os.println(s);
              }

              os.close();
        }
        catch(Exception er) {
             Messages.postError("Problem creating  " + filepath);
             Messages.writeStackTrace(er);
        }
    }

}
