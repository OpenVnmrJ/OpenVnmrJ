/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.plaf.*;
import javax.swing.plaf.basic.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VComboBoxButton extends VMenu
{


    public VComboBoxButton(SessionShare sshare, ButtonIF vif, String type)
    {
        super(sshare, vif, type);

        setUI(new VComboBoxButtonUI(new VButtonListener()));
    }

    protected void sendcmd()
    {
        if (inModalMode || inEditMode || vnmrCmd == null)
            return;
        if ((isActive < 0) || (vnmrIf == null))
            return;

        sendVnmrCmd();
    }

    public void updateUI()
    {
        setUI(getUI());
    }

    public  void setForeground(Color color)
    {
        super.setForeground(color);
        ComboBoxUI comboboxui = getUI();
        if (comboboxui != null && comboboxui instanceof VComboBoxButtonUI)
           ((VComboBoxButtonUI)comboboxui).setforeground(color);
    }

    protected void setPressed(boolean bShow)
    {
        if (inEditMode)
            return;
        if (bShow)
            setBorder(BorderFactory.createLoweredBevelBorder());
        else
            setBorder(BorderFactory.createRaisedBevelBorder());
    }

}

class VComboBoxButtonUI extends BasicComboBoxUI
{

    MouseListener comboboxListener;
    Color colorbg;
    Color colorfg;

    public VComboBoxButtonUI(MouseListener ml)
    {
        super();
        comboboxListener = ml;
        colorbg = Color.black;
        colorfg = Color.white;
    }

    protected void installListeners()
    {
        super.installListeners();

        comboBox.removeMouseListener(popupMouseListener);
        comboBox.addMouseListener(comboboxListener);

        if (listBox != null)
        {
            colorbg = listBox.getSelectionBackground();
            colorfg = listBox.getSelectionForeground();
        }
    }

    protected void setforeground(Color color)
    {
        if (arrowButton != null)
            arrowButton.setForeground(color);
    }

    protected void setbackground(Color colorbg)
    {
        if (arrowButton != null)
            arrowButton.setBackground(colorbg);
        if (listBox != null)
            listBox.setBackground(colorbg);
    }

    /**
     * Creates an button which will be used as the control to show or hide
     * the popup portion of the combo box.
     *
     * @return a button which represents the popup control
     */
    protected JButton createArrowButton() {
        return new VjButton();
    }

    public void paintCurrentValue(Graphics g, Rectangle bounds, boolean hasFocus)
    {
        listBox.setSelectionBackground(comboBox.getBackground());
        listBox.setSelectionForeground(comboBox.getForeground());
        super.paintCurrentValue(g,bounds,hasFocus);
        listBox.setSelectionBackground(colorbg);
        listBox.setSelectionForeground(colorfg);
    }

}

class VButtonListener implements MouseListener
{

    transient long lastPressedTimestamp = -1;
    transient boolean shouldDiscardRelease = false;

    public void mouseClicked(MouseEvent e)
    {
        VComboBoxButton b = (VComboBoxButton)e.getSource();
        b.sendcmd();
    };

    public void mousePressed(MouseEvent e) {
       if (SwingUtilities.isLeftMouseButton(e) ) {
           VComboBoxButton b = (VComboBoxButton) e.getSource();
               if (!b.isEnabled()) {
                   // Disabled buttons ignore all input...
                   return;
               }
               b.setPressed(true);
               if(!b.hasFocus() && b.isRequestFocusEnabled()) {
                   b.requestFocus();
               }

       }
    };

    public void mouseReleased(MouseEvent e) {
        if (SwingUtilities.isLeftMouseButton(e)) {
            VComboBoxButton b = (VComboBoxButton) e.getSource();
            b.setPressed(false);
        }
    };

    public void mouseEntered(MouseEvent e) {
    };

    public void mouseExited(MouseEvent e) {
    };


}

class VjButton extends JButton
{

    public void paint(Graphics g)
    {
        super.paint(g);

        Dimension  psize = getSize();
        int x = psize.width/2;
        int y = psize.height*3/4;
        int x1 = psize.width/4;
        int y1 = psize.height/4;
        int x2 = psize.width*3/4;
        int[] w = {x,x1,x2};
        int[] h = {y,y1,y1};
        g.drawLine(x,y,x1,y1);
        g.drawLine(x1,y1,x2,y1);
        g.drawLine(x,y,x2,y1);
        g.fillPolygon(w,h,3);

    }

}

