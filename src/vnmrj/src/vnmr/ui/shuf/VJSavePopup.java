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
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import vnmr.ui.*;
import vnmr.util.Util;
import vnmr.util.DisplayOptions;


/********************************************************** <pre>
 * Summary: File Save Popup
 *
 * Create a Modal popup frame for VJFileBrowser for SaveAs function
 </pre> **********************************************************/

public class VJSavePopup extends JDialog implements PropertyChangeListener {
    static private VJFileBrowser fileBrowser=null;


    public VJSavePopup() {
        // Set this JDialog as owned by the VNMRFrame and set modal to true
        super(VNMRFrame.getVNMRFrame(), Util.getLabel("blSave"), true);

        DisplayOptions.addChangeListener(this);

        SessionShare sshare = ResultTable.getSshare();
        // Make the modified JFileChooser and programmable dir buttons
        // Pass in 'this' as the parent so the panel will know how to
        // unshow this dialog
        fileBrowser = new VJFileBrowser(sshare, Util.getLabel("blSave"), this);
        add(fileBrowser);

        Dimension dim = new Dimension(800, 500);
        setPreferredSize(dim);
        setSize(dim);

        setVisible(false);

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
            setTitle(type);
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
            // The arg sent here is not a translated string
            fileBrowser.writePersistence((Util.getLabel("blSave")));
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
