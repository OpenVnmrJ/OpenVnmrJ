/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.net.*;
import javax.help.*;

public class VnmrJHelp {

    private HelpSet    hs;
    private HelpBroker hb;
    String  jhelpDir = Util.VNMRDIR + File.separator + "jhelp";
    String  jhelpSet = "plotdesignerhelp.hs";
    String  defSet = "VnmrJ.hs";
    String  defDir = Util.VNMRDIR + File.separator + "jhelp";
    private Vector  vecHs;

    public VnmrJHelp() {
        this.vecHs = new Vector();
    }

    public void openHelp(String str)
    {
        openHelp(str, defSet);
    }

    public void openHelp(String str, String strSet) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        String fpath = null;
        String  jhelpFile = null;
        String  jhelpName = null;

        if (tok.hasMoreTokens()) {
             fpath = tok.nextToken().trim();
        }
        if (tok.hasMoreTokens()) {
             jhelpName = tok.nextToken().trim();
        }

        if (fpath == null || fpath.equals("jhelp"))
             fpath = defDir + File.separator + strSet;

        if (fpath.startsWith(File.separator))
             jhelpFile = fpath;
        else
             jhelpFile = defDir + File.separator+ fpath;

        for (Enumeration e = vecHs.elements(); e.hasMoreElements();) {
            vjHelpSet child = (vjHelpSet) e.nextElement();
            if (child.contains(jhelpFile)) {
                child.show(jhelpName);
                return;
            }
        }
        File file = new File(jhelpFile);
        if ((!file.exists()) || (!file.canRead())) {
             Messages.postError("File "+fpath+" not found");
             return;
        }
        jhelpDir = file.getParent();
        jhelpSet = file.getName();
        vjHelpSet vjHs = new vjHelpSet(jhelpDir, jhelpSet);
        vecHs.addElement(vjHs);
        vjHs.show(jhelpName);
    }


    public void createJHelp() {
/**
    String helpFile = jhelpDir+File.separator+ jhelpSet;
    File file = new File(helpFile);
        if(!file.exists()) {
             Messages.postError("Error: "+helpFile+" not exist ");
             return;
        }

        try {
             URL hsURL = new URL (new File(jhelpDir).toURL(), jhelpSet);
             hs = new HelpSet(null, hsURL);
//	     hs.setLocalMap(new TryMap());
             //System.out.println("Found HelpSet at " + hsURL);
         Messages.postDebug("Found HelpSet at " + hsURL);
        }
        catch (Exception ee) {
            //System.out.println("HelpSet not found");
            System.out.println("HelpSet not found");
         Messages.postError("HelpSet not found");
        return;
        }
        hb = hs.createHelpBroker();
**/
    }

    public void setJHelpVisible () {
        if (hb != null)
            hb.setDisplayed(true);
    }

    private class vjHelpSet {
        private HelpSet    vjHs;
        private HelpBroker vjHb;
        private String  strHelpSet = null;

        public vjHelpSet(String path, String name) {
            strHelpSet = path + File.separator + name;
            try {
                URL hsURL = new URL (new File(path).toURL(), name);
                vjHs = new HelpSet(null, hsURL);
                if (vjHs.getLocalMap() == null)
                    vjHs.setLocalMap(new TryMap());
            }
            catch (Exception e) {
               Messages.postError(" "+name+" is not legal help set ");
               Messages.writeStackTrace(e);
               return;
            }
            vjHb = vjHs.createHelpBroker();
        }

        public void show(String name) {
            if (vjHb != null) {
                if (name != null) {
                    try {
                        vjHb.setCurrentID(name);
                    }
                    catch (BadIDException e) {
                    }
                }
                vjHb.setDisplayed(true);
            }
        }

        public boolean contains(String name) {
            return (name.equals(strHelpSet));
        }

        public String getSetName() {
            return strHelpSet;
        }
    }
}
