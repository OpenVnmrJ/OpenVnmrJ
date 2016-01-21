/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.*;
import vnmr.ui.*;

public class XJComboBox extends JComboBox implements ComboBoxTitleIF
{
    public static String lfStr = null;
    public static boolean bMetalUI = true;

    public XJComboBox()
    {
        initUI();
    }

    public XJComboBox(Vector<?> items)
    {
        super(items);
        initUI();
    }

    public XJComboBox(final Object items[])
    {
        super(items);
        initUI();
    }

    public XJComboBox(ComboBoxModel aModel)
    {
        super(aModel);
    }

    private void  initUI() {
       
       setMaximumRowCount(12);
       setBorder(new VButtonBorder());
       ComboBoxUI ui;
       if (lfStr == null) {
             LookAndFeel lk = UIManager.getLookAndFeel();
             if (lk != null)
                lfStr = lk.getID();
             if (lfStr == null) {
                lfStr = "metal";
             }
             if (!lfStr.equalsIgnoreCase("metal"))
                bMetalUI = false;
        }
        if (bMetalUI) {
             // menuUi = (ComboBoxUI) new VComboMetalUI(this);
             ui = (ComboBoxUI) new VComboBasicUI(this);
        }
        else {
             ui = (ComboBoxUI) new VComboBasicUI(this);
        }
        setUI(ui);
    }

    public boolean getDefault() {
        return false;
    }

    public String getDefaultLabel() {
        return null;
    }

    public String getTitleLabel() {
        return null;
    }


    public void enterPopup() {
    }

    public void exitPopup() {
    }

    public void listBoxMouseAdded() {
    }
}
