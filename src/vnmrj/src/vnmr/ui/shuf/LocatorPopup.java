/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;


import javax.swing.event.*;
import javax.swing.*;

import java.awt.*;
import java.awt.datatransfer.*;
import java.awt.event.*;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.util.*;
import java.io.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;


/********************************************************** <pre>
 * Summary: Locator Popup
 *
 * Create a modeless popup frame for Locator/Shuffler.
 </pre> **********************************************************/


public class LocatorPopup extends JFrame implements PropertyChangeListener  {
    private Shuffler shuffler = null; 

    public LocatorPopup() {
        SessionShare sshare = ResultTable.getSshare();
        setTitle(Util.getLabel("_VJ_Locator"));

        DisplayOptions.addChangeListener(this);
        
        Image img = Util.getImage("OVJ.png");
        if (img != null)
            this.setIconImage(img);

/*
        // Do not allow more that one Shuffler.  Things are not set up
        // for multiple of these at once.
        if(Shuffler.shufflerInUse()) {
            Messages.postWarning("There can be only one Locator at a time");
            return;
        }

        shuffler = new Shuffler(sshare);

        add(shuffler);

        Dimension dim = new Dimension(500, 800);
        setSize(dim);
*/
        
        // Set busy signal.  This will be turned off in StartSearch when
        // the first search if finished.
/*
        ExpPanel exp=Util.getDefaultExp();
        exp.setBusy(true);
*/
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    public void dispose() {
        DisplayOptions.removeChangeListener(this);
        super.dispose();
    }

    // Show the panel
    public void showPanel() {
        if (Util.isShufflerInToolPanel() || FillDBManager.locatorOff())
            return;
        Shuffler sh = null;
         
        Container c = getContentPane();
        if (c != null) {
            int count = c.getComponentCount();
            for (int k = 0; k < count; k++) {
                Component comp = c.getComponent(k);
                if (comp != null) {
                   if (comp instanceof Shuffler) {
                      sh = (Shuffler) comp;
                      break;
                   }
               }
            }
        }
        if (sh == null) {
            sh = Util.getShuffler();
            if (sh == null) {
                SessionShare sshare = ResultTable.getSshare();
                if (sshare == null)
                     return;
                sh = new Shuffler(sshare);
            }
            add(sh);
            Dimension dim = new Dimension(500, 800);
            setSize(dim);
            validate();
        }
        setVisible(true);

            // Uniconify if necessary
        setExtendedState(Frame.NORMAL);
    }

}
