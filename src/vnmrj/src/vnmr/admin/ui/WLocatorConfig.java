/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.admin.util.*;
import vnmr.ui.shuf.FillDBManager;


public class WLocatorConfig extends ModalEntryDialog
{

    private JCheckBox ckBox=null;

    public WLocatorConfig(String strhelpfile)
    {
        super(vnmr.util.Util.getLabel("_admin_Locator_Config_Title"), 
                vnmr.util.Util.getLabel("_admin_Age_Limit"), 
                strhelpfile);

    }

    public void setVisible(boolean bShow)
    {
        if (bShow)
        {
            dolayout();
            setLocationRelativeTo(getParent());
        }
        super.setVisible(bShow);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            saveAgeLimit();
            saveLocatorOff();
            setVisible(false);
        }
        super.actionPerformed(e);
    }

    protected void dolayout()
    {
        String value = FillDBManager.readDateLimit();
        // File has value of -1 to mean forever
        if(value.equals("-1"))
            value = "Forever";

        // One way or the other, we now have newValue set.  Set it into
        // the entry field.
        inputText.setText(value); 

        // Get the current locator off value
        boolean locatoroff = FillDBManager.readLocatorOff(false);
        

        // Add a Check box for LocatorOff
        if(ckBox == null) {
            ckBox = new JCheckBox(vnmr.util.Util.getLabel("_admin_Locator_Off"), locatoroff);
            ckBox.addMouseListener(new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    if(!okButton.isEnabled())
                        okButton.setEnabled(true);
                }
            });
            panelForText.add(ckBox);
            pack();
        }
        else
            ckBox.setSelected(locatoroff);
    }

    public void saveAgeLimit()
    {
        String strValue = inputText.getText();
        
        // File get written with -1 to mean forever
        if(strValue.equals("Forever") || strValue.equals("forever"))
            strValue = "-1";

        FillDBManager.setDateLimitMsFromDays(strValue);

    }

    public void saveLocatorOff() {
        boolean locatoroff = ckBox.isSelected();

        FillDBManager.writeLocatorOff(locatoroff);
    }

}
