/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import vnmr.util.*;

import java.util.*;
import java.io.*;
import java.net.*;


/********************************************************** <pre>
 * Summary: Contain a list of all know mount points with the real/direct
 *      path and the mounted path equivalent.
 *
 *      When running with a network database, the file information in
 *      the DB should be host and the direct path on that host, not
 *      the path some remote host knows the data as through a mount point.
 *      Also, then, when trying to access data on a remote machine based on
 *      info from the DB, we need to know the mount point path to get
 *      there from the current machine.
 *
 *      This class will create a table of known mount point information
 *      and will despense that information as needed.
 *
 *      First check for a file '/usr/varian/mount_name_table'.
 *      If found, only use this file.  An example of this file is as follows:
 *            # Table of remote system mount names and paths
 *            # One line per entry, Syntax:
 *            #     host:direct_path  mount_path
 *            mongoose:/export/home    /mongoose_home
 *            voyager:/export/home     /voyager_home
 *
 *      If mount_name_table is not found, look in /etc/mnttab
 *      and in /etc/auto_direct and get relationships out of these.
 *
 </pre> **********************************************************/


public class MountPaths {
    static private MountPaths mountPaths = new MountPaths();
    static private Vector pathList=null;
    static private String hostName=null;
    static private boolean usingNetServer=false;

    public MountPaths() {
        MountPath mp;
        StringTokenizer tok;
        String host, directPath, mountPath;
        BufferedReader in;
        UNFile file;
        String inLine;
        String filepath;
        String dbhost, dbnet_server;

        pathList = null;

        // If no Locator/DB, don't do this stuff
        if(FillDBManager.locatorOff())
            return;

        // Fill the hostname
        fillHostname();

        // If the user is not using a network DB server, then we do not
        // really need to do anything in this class except return his args
        // to him untouched.   Find out from the DB itself what mode it
        // is in.
        FillDBManager dbm = FillDBManager.fillDBManager;
        if(dbm !=null)
        	usingNetServer = dbm.getNetworkModeFromDB();

        if(DebugOutput.isSetFor("MountPaths"))
            Messages.postDebug("usingNetServer: " + usingNetServer);

        // Fill the mountPaths list with all know mount points
        // Start by looking in a user supplied file
        if(UtilB.OSNAME.startsWith("Windows"))
            filepath = UtilB.SFUDIR_WINDOWS + File.separator + "usr" +
                    File.separator + "varian" + File.separator + "mount_name_table";
        else
            filepath = "/usr/varian/mount_name_table";
        file = new UNFile(filepath);
        if(file.exists()) {
            // mount_name_table exists, use it to fill our list.
            // Format is similar to mnttab which is:
            //     host:direct_path   mount_path
            try {
                in = new BufferedReader(new FileReader(file));
            } catch(Exception e) {
                Messages.postError("Problem opening " + filepath);
                Messages.writeStackTrace(e, "Error caught in MountPaths");
                return;
            }

            pathList = new Vector();
            try {
                while((inLine = in.readLine()) != null) {
                    /* skip blank lines and comment lines. */
                    if (inLine.length() > 1 && !inLine.startsWith("#")) {
                        tok = new StringTokenizer(inLine, ": \t");

                        host = tok.nextToken().trim();
                        directPath = tok.nextToken().trim();
                        mountPath = tok.nextToken().trim();

                        mp = new MountPath(host, directPath, mountPath);
                        pathList.add(mp);
                    }
                }
            }
            catch(Exception e) {
                // Do not error out.  If pathList has things added to it,
                // it will be intact, just return unless debug set.
                if(DebugOutput.isSetFor("MountPaths")) {
                    Messages.postDebug("Problem parsing " + filepath);
                    Messages.writeStackTrace(e, "Error caught in MountPaths");
                }
                return;
            }
        }
        else {
            pathList = new Vector();
            // Try to get info from /etc/mnttab and auto_direct
            // On linux, mnttab is mtab, so try that also.
            filepath = "/etc/mnttab";
            file = new UNFile(filepath);
            if(!file.exists()) {
                filepath = "/etc/mtab";
                file = new UNFile(filepath);
            }

            if(file.exists()) {
                try {
                    in = new BufferedReader(new FileReader(file));
                } catch(Exception e) {
                    Messages.postError("Problem opening " + filepath);
                    Messages.writeStackTrace(e, "Error caught in MountPaths");
                    return;
                }
                try {
                    while((inLine = in.readLine()) != null) {
                        /* skip blank lines and comment lines. */
                        if (inLine.length() > 1 && !inLine.startsWith("#")) {
                            // Only parse lines which contain colons
                            if(inLine.indexOf(":") == -1)
                                continue;

                            tok = new StringTokenizer(inLine, ": \t");

                            host = tok.nextToken().trim();
                            // Do not allow ip addresses, they will not
                            // match with DB entries. If there is a period,
                            // see if there is a numeric before the period.
                         
                            int first = host.indexOf(".");
                            if(first != -1) {
                                // There is a period
                                String front = host.substring(0, first);
                                try {
                                    // I don't really want the number returned
                                    // If this is not a numeric, an exception
                                    // will be thrown.  
                                    Integer.parseInt(front);
                                    
                                    //There was no exception, this must be
                                    // a numeric, give an error.  Don't give
                                    // an error if managedb
                                    if(!FillDBManager.managedb) {
                                        Messages.postWarning("If an IP address is"
                                                + " used in mounting, then that\n"
                                                + "    directory will not be accessible"
                                                + " via the locator.  Try creating a\n"
                                                + "    /usr/varian/mount_name_table file"
                                                + " with mount entries in the form of\n"
                                                + "        \'host:direct_path mount_path\'"
                                                + " with one line per mounted directory.");
                                        // Now, just continue as normal
                                    }
                                    
                                    
                                }
                                catch (NumberFormatException e) {
                                    // There was an exception, so this must be
                                    // text, no error, just continue.
                                }
                            }
                            
                            // Skip entry for this host
                            if(host.equals(hostName))
                                continue;

                            directPath = tok.nextToken().trim();
                            mountPath = tok.nextToken().trim();

                            mp = new MountPath(host, directPath, mountPath);
                            pathList.add(mp);
                        }
                    }
                }
                catch(Exception e) {
                    // Do not error out.  If pathList has things added to it,
                    // it will be intact, just continue unless debug set.
                    if(DebugOutput.isSetFor("MountPaths")) {
                        Messages.postDebug("Problem parsing " + filepath);
                        Messages.writeStackTrace(e, "Error caught in MountPaths");
                    }
                }
            }
            filepath = "/etc/auto_direct";
            file = new UNFile(filepath);
            if(file.exists()) {
                try {
                    in = new BufferedReader(new FileReader(file));
                } catch(Exception e) {
                    // Do not error out.  If pathList has things added to it,
                    // it will be intact, just return unless debug set.
                    if(DebugOutput.isSetFor("MountPaths")) {
                        Messages.postDebug("Problem opening " + filepath);
                        Messages.writeStackTrace(e, "Error caught in MountPaths");
                    }
                    return;
                }
                try {
                    while((inLine = in.readLine()) != null) {
                        /* skip blank lines and comment lines. */
                        inLine = inLine.trim();

                        if (inLine.length() > 1 && !inLine.startsWith("#")) {
                            directPath=null;
                            host=null;
                            
                            tok = new StringTokenizer(inLine, ": \t");
                            mountPath = tok.nextToken().trim();
                            // Dump the next one
                            if(tok.hasMoreTokens())
                                tok.nextToken();
                            if(tok.hasMoreTokens()) {
                                host = tok.nextToken().trim();
                                // Do not allow ip addresses, they will not
                                // match with DB entries. Look for at lease
                                // 2 periods
                                int first = host.indexOf(".");
                                int last = host.lastIndexOf(".");
                                // Don't output msg for managedb.
                                if(first != -1  && first != last &&
                                        !FillDBManager.managedb) {
                                    Messages.postWarning("If an IP address is"
                                     + " used in mounting, then that\n"
                                     + "    directory will not be accessible"
                                     + " via the locator.  Try creating a\n"
                                     + "    /usr/varian/mount_name_table file"
                                     + " with mount entries in the form of\n"
                                     + "        \'host:direct_path mount_path\'"
                                     + " with one line per mounted directory.");

                                }
                            }
                            if(tok.hasMoreTokens())
                                directPath = tok.nextToken().trim();
                            // if this ends in '&', then fix the path
                            // by appending the mountPath
                            if(directPath != null) {
                                int index;
                                if((index = directPath.indexOf("&")) != -1) {
                                    // Get rid of &
                                    directPath = directPath.substring(0, index);
                                    // append mount path
                                    directPath = directPath.concat(mountPath);
                                }

                                mp = new MountPath(host, directPath, mountPath);

                                // Skip duplicates
                                if(!isDup(mp))
                                    pathList.add(mp);
                            }
                        }
                    }
                }
                catch(Exception e) {
                    if(DebugOutput.isSetFor("MountPaths")) {
                        Messages.postDebug("Problem parsing " + filepath);
                        Messages.writeStackTrace(e, "Error caught in MountPaths");
                    }
                    return;
                }

            }

        }

        if(DebugOutput.isSetFor("MountPaths")) {
            Messages.postDebug(toString());
        }


    }
    
    private static void fillHostname() {
        // Get host name
        try{
            InetAddress inetAddress = InetAddress.getLocalHost();
            hostName = inetAddress.getHostName();
        }
        catch(Exception e)
        {
            Messages.postError("Problem getting host name");
            Messages.writeStackTrace(e);
            hostName = "";
        }
    }

    /******************************************************************
     * Summary: Check for duplicate of arg in current list.
     *
     *
     *****************************************************************/
    private boolean isDup(MountPath mp1) {
        MountPath mp2;

        for(int i=0; i < pathList.size(); i++) {
            mp2 = (MountPath) pathList.get(i);
            if(mp2.equals(mp1))
                return true;
        }
        return false;
    }

    /******************************************************************
     * Summary: Convert path arg to local mount path if host is not local.
     *
     *  If host is the localhost, then return the input path unchanged.
     *  If host is not the localhost, then look through the pathList
     *  for this host, then look for this directPath for that host.
     *  If found, return mountPath
     *****************************************************************/

    static public String getMountPath(String host, String fullpath) {
        MountPath mp;

        // If not doing net db server stuff, just return the users path
        if(FillDBManager.locatorOff() || !usingNetServer)
            return fullpath;

        // If on this host, just return the input fullpath
        if(host.equals(hostName)) {
            return fullpath;
        }
        else {
            // Go through the table
            for(int i=0; i < pathList.size(); i++) {
                mp = (MountPath) pathList.get(i);
                // Look for host match and directPath match
                if(mp.host.equals(host) && fullpath.startsWith(mp.directPath)){
                    // Replace the directPath part of fullpath with mountPath
                    String subString=fullpath.substring(mp.directPath.length());

                    String mpath = mp.mountPath + subString;

                    return mpath;
                }
            }
        }

        // If we get here, the host is not this host, and we have
        // no entry in the pathList table that matches.
// This happens any time there is a link on system x that is pointing to
// a partition that is not mounted on system y.
//         Messages.postError("Cannot find a mounted directory for host = "
//                            + host + " and \n    path = " + fullpath + "  .\n"
//                            + "    Be sure this directory is mounted. "
//                            + "If still not found, you may need to create\n"
//                            + "    a file \'/usr/varian/mount_name_table\' "
//                            + "with mount entries in the form of\n"
//                            + "        \'host:direct_path mount_path\'"
//                            + " with one line per mounted directory.");
        return "";

    }

    /******************************************************************
     * Summary: Convert path arg to local mount path if host is not local..
     *
     *
     *****************************************************************/

    static public String getMountPath(String hostFullpath) {
        String host, fullpath;
        int index;
        
        if(FillDBManager.locatorOff())
            return hostFullpath;

        if(UtilB.iswindows()){
        	// on windows need a different delimiter to prepend host name
        	// since ':' catches "C:" etc. (i.e. local files and directories)
        	return hostFullpath;  // so remote file IO is not supported for now      
        }
        index = hostFullpath.indexOf(":");
        if(index<0)
            return hostFullpath;
        host = hostFullpath.substring(0, index);
        fullpath = hostFullpath.substring(index +1);

        return getMountPath(host, fullpath);

    }

    /******************************************************************
     * Summary: Convert local mount path to direct path on appropriate host.
     *  Return host and directpath
     *
     *****************************************************************/

    static public Vector getPathAndHost(String fullpath) {
        MountPath mp;
        Vector result= new Vector();
        
        if(hostName == null)
            fillHostname();

        // If doing net db server stuff, take care of it
        if(!FillDBManager.locatorOff() && usingNetServer) {
            // Go through the table
            for(int i=0; i < pathList.size(); i++) {
                mp = (MountPath) pathList.get(i);
                // Look for mountPath match
                if(fullpath.startsWith(mp.mountPath)) {
                    result.add(Shuf.HOST, mp.host);
                    // Replace the mountPath part of fullpath with directPath
                    String subString =fullpath.substring(mp.mountPath.length());
                    String dpath = mp.directPath + subString;
                    result.add(Shuf.PATH, dpath);
                    return result;
                }
            }
        }

        // Not found in table or not network db.  This must just be on the
        // local disk
        result.add(Shuf.HOST, hostName);
        result.add(Shuf.PATH, fullpath);
        return result;
    }


    static public Vector getCanonicalPathAndHost(String fullpath)
                                                      throws IOException{
        String cPath="";
        UNFile file;

        // First get the canonical path as on this system.
        try {
            file = new UNFile(fullpath);
            cPath = file.getCanonicalPath();
        }
        catch(Exception e) {
            Messages.writeStackTrace(e);
        }

        // Now translate to direct path and host
        return getPathAndHost(cPath);
    }


    // Get the Canonical direct path and host and return the combined
    // result in the form of host:fullpath
    static public String getCanonicalHostFullpath(String fullpath) {
        String hostFullpath;
        String    dhost, dpath;

        try {
            Vector mp = getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postError("Problem getting cononical path for\n    " +
                                 fullpath);
            Messages.writeStackTrace(e);
            return fullpath;
        }
        hostFullpath = new String(dhost + ":" + dpath);
        return hostFullpath;
    }

    public String toString() {
        MountPath mp;
        String out;

        out = "MountPaths Contents: \n     Host, directPath, mountPath";
        for(int i=0; i < pathList.size(); i++) {
            mp = (MountPath) pathList.get(i);
            out = out.concat("\n    " +  mp.host + ",  " + mp.directPath
                             + ",  " + mp.mountPath);
        }

        return out;
    }

    static public boolean getusingNetServer() {
        return usingNetServer;

    }


}


/********************************************************** <pre>
 * Summary: Container for a single host/mount point
 *
 *
 </pre> **********************************************************/

class MountPath {
    public String host;
    public String directPath;
    public String mountPath;

    public MountPath(String h, String d, String m) {
        host = h;
        directPath = d;
        mountPath = m;
    }

    public boolean equals(MountPath mp) {
        if(host.equals(mp.host) &&
           directPath.equals(mp.directPath) &&
           mountPath.equals(mp.mountPath))
            return true;
        else
            return false;
    }
}
