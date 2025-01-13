/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.io.*;
import java.util.*;

import vnmr.util.*;


/********************************************************** <pre>
 * Summary: Maintain a maping of symbolic links.
 *
 *      After implementing the browser with the locator and having the locator
 *      limited to the directory where the browser is currently at, it was
 *      clear that there was a problem with directories which were symbolic
 *      links.  Since the locator has items listed by canonical path, it the
 *      locator limit was set to the absolute path, the items did not show up
 *      in the locator.  The problem was compounded when you add network DB
 *      to the equation.
 *
 *      So, I created this class to search out and maintain a list of all
 *      symbolic links in directories where the locator searched for files.
 *      Then when we need a 'limit' for the locator, we call 
 *      getHostFullpathForLinksBelow() to get a list of directories to use
 *      for the limit.
 *
 *      To handle network DB, the list is kept in the DB (so all systems can
 *      have access to the same list, and the list is kept in the form
 *      of dhost and dpath.  Then on the local machine, it is converted to the
 *      mpath for that machine.
 *      
 </pre> **********************************************************/

public class SymLinkMap {
    ArrayList linkList;
    ArrayList dirChecked;

    public SymLinkMap() {
        boolean inNetworkMode;
        linkList = new ArrayList();
        dirChecked = new ArrayList();
        FillDBManager dbm = FillDBManager.fillDBManager;


        // Fill linkList from the DB
        linkList = getSymLinksFromDB();

        if(DebugOutput.isSetFor("SymLinkMap"))
            Messages.postDebug(toString());

        // Start a thread to occasionally update the linkList ArrayList
        // from the contents of the DB.  This is for network DB which
        // may get changed from other machines.  If not in network mode,
        // do not bother with starting this thread.
        inNetworkMode = dbm.getNetworkModeFromDB();
        if(inNetworkMode) {
            UpdateSymLinksFromDB updateSL = new UpdateSymLinksFromDB();
            updateSL.start();
        }
    }



    public void checkForSymLinks(String dirToSearch) {
        String cmd;
        String strg=null;
        String absPath, conPath;
        FileReader fr;
        BufferedReader  in;
        String          inLine;
        StringTokenizer tok;
        String          linkPath;
        String          directory;

        if(dirToSearch == null || dirToSearch.length() == 0)
            return;

        if(DebugOutput.isSetFor("SymLinkMap"))
            Messages.postDebug("checkForSymLink Called for: " + dirToSearch);

        directory = new String(dirToSearch);

        // Lets be sure the directory exists.  Else, we get an exception.
        UNFile file = new UNFile(directory);
        if(!file.exists()) {
            // Silently return if no directory
            return;
        }

        // Have we already searched this directory or a parent of it?
        // Go through the list of dirChecked and see if any is a parent
        // of the arg dirToSearch
        for(int i=0; i < dirChecked.size(); i++) {
            String dir = (String) dirChecked.get(i);
            if(dirToSearch.startsWith(dir)) {
                // Already done, bail out
                return;
            }
        }


        // Save this directory for later comparisons
        dirChecked.add(dirToSearch);

        try {

            // Create a unique filename for the results to be put into.
            String thName = Thread.currentThread().getName();
            String sysdir = System.getProperty("sysdir");
            String filePath = sysdir +  "/tmp/findLinks" + thName;

            cmd = sysdir + "/bin/findLinks " 
                                           + directory + " " + filePath;

            Runtime rt = Runtime.getRuntime();
            Process prcs = null;
            try {
                // Start the C program
                prcs = rt.exec(cmd);
                // Wait for it to complete the writing out of its results
                prcs.waitFor();
            }
            finally {
                // It is now my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close the stupid things.
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }

            // Now open the result file and read it.
            file = new UNFile(filePath);
            if(file == null || !file.exists()) {
                Messages.postLog("Problem opening " + filePath);
                return;
            }

            fr = new FileReader(file);
            in = new BufferedReader(fr);

            // Check to see if the heading is correct.
            inLine = in.readLine();
            if(inLine != null) {
                if(!inLine.equals("Links List")) {
                    Messages.postLog("Locator problem with soft links");
                    file.delete();
                    return;
                }
            }
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1) {
                    tok = new StringTokenizer(inLine);
                    if(tok.countTokens() != 2) {
                        Messages.postLog("Locator format problem with soft links");
                        file.delete();
                        return;
                    }
                    linkPath = new String(tok.nextToken().trim());

                    // Avoid duplicates
                    if(!linkList.contains(linkPath)) {
                        linkList.add(linkPath);

                        // Add to the DB table
                        setSymLinkToDB(linkPath);

                        if(DebugOutput.isSetFor("SymLinkMap"))
                            Messages.postDebug("checkForSymLink adding "
                                               + linkPath);
                    }

                }
            }

            // Close and remove the file
            fr.close();
            file = new UNFile(filePath);
            file.delete();
        }
        catch (Exception e) {
            Messages.postWarning("Problem executing findLinks or "
                                 + "reading results.");
            file.delete();
        }

    }

    /******************************************************************
     * Summary: Return list of canonical paths for all links know below
     *  the arg directory.
     *
     *  The input arg must not be canonical.
     *  The output list is dhost:dpath.  That is, hostFullpath in the
     *  form ready to compare to the DB entries.
     *
     *  Force returned directory to unix style no matter what was passed in.

     *****************************************************************/

    public ArrayList getHostFullpathForLinksBelow(String directory) {
        ArrayList dlist, alist;
        Vector hostNpath;
        String dhost, dpath, hostFullpath;
        UNFile file;
        String keyCpath, dir, end, key, subkey;
        int index;

        // dhost:dpath
        dlist = new ArrayList();


        // absolute path, not canonical nor direct
        alist = new ArrayList();

        // If the directory is at root, then forget putting in every link
        // in the known universe.  Just return 'all'.
        if(directory.equals(File.separator) || 
                            directory.equals(UtilB.SFUDIR_WINDOWS)) {
            dlist.add("all");
            return dlist;
        }


        try {
            // Start with dir equal to directory.  Cycle through until we have
            // resolved all links.  The do while is to catch multiple links
            // in the path.
            dir = new String(directory);

            // Convert to dhost and dpath and add this directory to the dlist
            hostNpath = MountPaths.getPathAndHost(dir);
            dhost = (String) hostNpath.get(Shuf.HOST);
            dpath = (String) hostNpath.get(Shuf.PATH);

            // force path to unix style
            dpath = UtilB.windowsPathToUnix(dpath);
            hostFullpath = dhost + ":" + dpath;

            dlist.add(hostFullpath);
            // Save the input dir for the secondary search below
            alist.add(dir);

            do {
                // Go through all of the known links
                end = "";
                for(int i=0; i < linkList.size(); i++) {
                    key = (String) linkList.get(i);
                    // Look for links at or above the input directory

                    if(dir.startsWith(key)) {
                        // Found one, get the canonical path on this host
                        file = new UNFile(key);
                        keyCpath = file.getCanonicalPath();

                        end = dir.substring(key.length(),
                                            dir.length());


                        // New dir using key canonical path with ending
                        // from input directory
                        dir = keyCpath.concat(end);

                        // Save this dir for the secondary search below
                        if(!alist.contains(dir))
                            alist.add(dir);

                        // Convert to dhost and dpath
                        hostNpath = MountPaths.getPathAndHost(dir);
                        dhost = (String) hostNpath.get(Shuf.HOST);
                        dpath = (String) hostNpath.get(Shuf.PATH);

                        // force path to unix style
                        dpath = UtilB.windowsPathToUnix(dpath);
                        hostFullpath = dhost + ":" + dpath;

                        if(!dlist.contains(hostFullpath))
                            dlist.add(hostFullpath);
                    }
                }
            }
            // Testing for empty end string, will catch when no links are
            // caught, and when we have found a link which is the entire path.
            while(end.length() > 0);



            // Now search for links below the directories we found
            // Note, alist is being added to on the fly in this loop,
            // so the number of items is increasing as the loop goes.
            for(int i=0; i < alist.size(); i++) {

                dir = (String) alist.get(i);

                // Go through all of the known links
                for(int k=0; k < linkList.size(); k++) {
                    key = (String) linkList.get(k);
                    subkey = new String(key);
                    // Look for links at or below the input directory
                    // If I do a startsWith(), I can match partial names,
                    // thus, I have to do an exact match on each parent dir
                    while(subkey.length() > 1) {
                        if(subkey.equals(dir)) {
                            // Found one, get the canonical path on this host
                            file = new UNFile(key);
                            keyCpath = file.getCanonicalPath();

                            // Convert to dhost and dpath
                            hostNpath = MountPaths.getPathAndHost(keyCpath);
                            dhost = (String) hostNpath.get(Shuf.HOST);
                            dpath = (String) hostNpath.get(Shuf.PATH);

                            // force path to unix style
                            dpath = UtilB.windowsPathToUnix(dpath);

                            hostFullpath = dhost + ":" + dpath;

                            // Add to the list of dhost:dpath items.
                            if(!dlist.contains(hostFullpath))
                                dlist.add(hostFullpath);

                            // Add keyCpath into the list to be further tested.
                            if(!alist.contains(keyCpath))
                                alist.add(keyCpath);

                            // No need to continue stripping the key,
                            // we have found the match.
                            break;
                        }
                        // Now we need to trim off the last part of the key
                        // path and try again.
                        index = subkey.lastIndexOf(File.separator);
                        if(index < 1)
                            break;
                        subkey = subkey.substring(0, index);

                    }
                }
            }

            if(DebugOutput.isSetFor("SymLinkMap"))
                Messages.postDebug("SymLinkMap entries for " + directory
                                   + "\n    are: " + dlist);
        }
        catch(Exception e) {
            Messages.postError("Problem resolving symbolic links for "
                               + directory);
            Messages.writeStackTrace(e);
        }
        return dlist;

    }



    // Clear the dirChecked list.  Cleanse linkList of items which no
    // longer exist.
    public void cleanDirChecked() {
        String key;
        Set set;
        Iterator iter;
        UNFile file;

        dirChecked.clear();

        for(int i=0; i < linkList.size(); i++) {
            key = (String) linkList.get(i);
            file = new UNFile(key);
            if(!file.exists()) {
                // This no longer exists, remove it from the map.
                linkList.remove(i);
                if(DebugOutput.isSetFor("SymLinkMap"))
                    Messages.postDebug("SymLinkMap cleanup removing " + key);
            }
        }
    }

    // Add a row to the DB_LINK_MAP table.
    // We need to keep info in the DB so that we can find these link from
    // any machine.  Keep in the DB (dhost_dpath, dhost, dpath)
    // The only use for dhost_dpath is as a unique key for the DB so that
    // we do not duplicate.  Then from whatever machine we are on, we can
    // take the dhost and dpath and get the mpath.
    public void setSymLinkToDB(String linkmpath) {
        String cmd;
        String dhost, dpath;
        String dhost_dpath;
        int    index;
        FillDBManager dbm = FillDBManager.fillDBManager;

        // We need to convert the input mount path to dhost:dpath for a
        // unique DB key
        Vector mp = MountPaths.getPathAndHost(linkmpath);
        dhost = (String) mp.get(Shuf.HOST);
        dpath = (String) mp.get(Shuf.PATH);
        dhost_dpath = dhost + ":" + dpath;


        try {
            cmd = "INSERT INTO " + Shuf.DB_LINK_MAP
                + " (dhost_dpath, dhost, dpath) VALUES (\'"
                + dhost_dpath + "\', \'" + dhost + "\', \'"
                + dpath + "\')";

            dbm.executeUpdate(cmd);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }

    }

    // We keep info in the DB so that we can find these link from
    // any machine.  Keep in the DB (dhost_dpath, dhost, dpath)
    // Then from whatever machine we are on, we can
    // take the dhost and dpath and get the mpath.
    public ArrayList getSymLinksFromDB() {
        java.sql.ResultSet rs;
        String cmd, dhost, dpath;
        String mpath;
        ArrayList results = new ArrayList();
        FillDBManager dbm = FillDBManager.fillDBManager;


        cmd = "SELECT dhost, dpath from sym_link_map";
        try {
            rs = dbm.executeQuery(cmd);
            if(rs != null) {
                while(rs.next()) {
                    dhost = rs.getString(1);
                    dpath = rs.getString(2);

                    // Now we have these items, we need to create the
                    // mount path
                    
// Calling MountPaths.getMountPath() is causing a problem with the ResultSet
// Just use dpath for now. ********
                    mpath = dpath;
//                    mpath = MountPaths.getMountPath(dhost, dpath);
                    if(mpath.length() > 0)
                        results.add(mpath);
                }
            }
        }
        catch (Exception e) {
            Messages.postError("Problem getting SymList from DB");
            Messages.writeStackTrace(e);
        }

        return results;
    }

    public String toString() {
        String result, key;


        result = "SymLinkMap:\n";
        for(int i=0; i < linkList.size(); i++) {
            key = (String) linkList.get(i);
            result = result.concat("    " + key + "\n");
        }

        return result;
    }


    // Update the member variable linkList now and then to keep
    // it up to date in case other machines add things to the list
    // that we don't know about.
    class UpdateSymLinksFromDB extends Thread {

        UpdateSymLinksFromDB() {
            setPriority(Thread.MIN_PRIORITY);
            setName("Update Sym Links From DB");
        }


        public void run() {

            if(DebugOutput.isSetFor("UpdateSymLinksFromDB"))
                Messages.postDebug("Starting UpdateSymLinksFromDB Thread");
            
            while(true) {
                try {
                    // Once per hour
                    sleep(3600000);
                    if(DebugOutput.isSetFor("UpdateSymLinksFromDB"))
                        Messages.postDebug("Running UpdateSymLinksFromDB");

                    // Get the current list from the DB and put into the
                    // member variable for quick access.
                    linkList = getSymLinksFromDB();

                }
                catch(Exception e) {
                    Messages.writeStackTrace(e);
                }
            }
        }

    }

}
