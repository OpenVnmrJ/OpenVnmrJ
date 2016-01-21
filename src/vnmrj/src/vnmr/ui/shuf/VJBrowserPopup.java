/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.*;

import java.awt.*;
import java.awt.event.ActionListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.util.*;

import vnmr.bo.VObjDef;
import vnmr.ui.*;
import vnmr.util.Util;
import vnmr.util.DisplayOptions;


/********************************************************** <pre>
 * Summary: File Browser Popup
 *
 * Create a Modless popup frame for VJFileBrowser for Browsing function
 </pre> **********************************************************/

public class VJBrowserPopup extends JFrame implements PropertyChangeListener {
    // Keep a static list of all browsers created.  Then we can call a
    // static updateAll() to go through the list and cause each of them
    // to update the directory listing.  (For example after dragging to Trash).
    static private ArrayList browserList=null;

    static private VJFileBrowser fileBrowser=null;


    public VJBrowserPopup(String type) {

        SessionShare sshare = ResultTable.getSshare();

        DisplayOptions.addChangeListener(this);
        
        Image img = Util.getImage("vnmrj.gif");
        if (img != null)
            this.setIconImage(img);

        setTitle(Util.getLabel("_VJ_Browser"));
        setName(Util.getLabel("_VJ_Browser"));

        // Create without giving a parent arg, That is the flag used
        // to know if we are creating a modal or modless dialog.
        fileBrowser = new VJFileBrowser(sshare, type, this);
        if(browserList == null)
            browserList = new ArrayList();
        browserList.add(fileBrowser);

        add(fileBrowser);

        Dimension dim = new Dimension(800, 500);
        setSize(dim);
        
        setVisible(false);
        
        ExpPanel.setBrowserPanel(fileBrowser);

    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    // Show the panel, set the title and set the dialogType
    public void showPanel(String type) {

        if(fileBrowser != null) {
            fileBrowser.showPanel(type);
            setVisible(true);

            // Uniconify if necessary
            setExtendedState(Frame.NORMAL);
            toFront();
        }
    }

    // Cause all browsers to update their directory lists
    static public void updateAll() {
        if(browserList != null) {
            for(int i=0; i < browserList.size(); i++) {
                VJFileBrowser browser = (VJFileBrowser)browserList.get(i);
                browser.rescanCurrentDirectory();
            }
        }
    }

    // Convenience method to call VJFileBrowser.setDirBtnPath()
    public void setDirBtnPath(int whichOne) {
        if(fileBrowser != null)
            fileBrowser.setDirBtnPath(whichOne);
    }

    
    // Convenience method to call VJFileBrowser.setBrowserToDirBtnPath()
    public void setBrowserToDirBtnPath(int whichOne) {
        if(fileBrowser != null)
            fileBrowser.setBrowserToDirBtnPath(whichOne);
    }

    public static void writePersistence() {
        if(fileBrowser != null)
            fileBrowser.writePersistence("Browser");
    }
    
    public void destroyPanel() {
        try {
            writePersistence();
            DisplayOptions.removeChangeListener(this);
            dispose();
            finalize();
        }
        catch (Throwable e) {
        }
 
    
    }

}
