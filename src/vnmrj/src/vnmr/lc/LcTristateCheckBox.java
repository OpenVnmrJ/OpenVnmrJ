/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import javax.swing.*;
import javax.swing.plaf.ActionMapUIResource;


import java.awt.event.*;
import java.awt.*;

/**
 * Derived from Glenn's TristateCheckBox class.
 * COMMENTS FROM GLENN SULLIVAN:
 * Maintenance tip - There were some tricks to getting this code
 * working:
 *
 * 1. You have to overwite addMouseListener() to do nothing
 * 2. You have to add a mouse event on mousePressed by calling
 * super.addMouseListener()
 * 3. You have to replace the UIActionMap for the keyboard event
 * "pressed" with your own one.
 * 4. You have to remove the UIActionMap for the keyboard event
 * "released".
 * 5. You have to grab focus when the next state is entered,
 * otherwise clicking on the component won't get the focus.
 * 6. You have to make a TristateDecorator as a button model that
 * wraps the original button model and does state management.
 */
public class LcTristateCheckBox extends JCheckBox {
    private final LcTristateButtonModel model;

    public LcTristateCheckBox(String text, Icon icon, Boolean initial){
        super(text, icon);
        // Add a listener for when the mouse is pressed
        super.addMouseListener(new MouseAdapter(){
            public void mousePressed(MouseEvent e){
                boolean hasFocus = isFocusOwner();
                requestFocusInWindow();
                model.nextState();
            }
        });
        // Reset the keyboard action map
        ActionMap map = new ActionMapUIResource();
        map.put("pressed", new AbstractAction(){      //NOI18N
            public void actionPerformed(ActionEvent e){
                requestFocusInWindow();
                model.nextState();
            }
        });
        map.put("released", null);                     //NOI18N
        SwingUtilities.replaceUIActionMap(this, map);
        // set the model to the adapted model
        model = new LcTristateButtonModel();
        setModel(model);
        setState(initial);
    }

    public LcTristateCheckBox(String text, Boolean initial){
        this(text, null, initial);
    }

    public LcTristateCheckBox(String text){
        this(text, null);
    }

    public LcTristateCheckBox(){
        this(null);
        setBackground(Color.WHITE);
    }

    /** No one may add mouse listeners, not even Swing! */
    public void addMouseListener(MouseListener l) {
    }

    /**
     * Set the new state to true, false, or null
     * for SELECTED, NOT_SELECTED or DONT_CARE, respectively.
     */
    public void setState(Boolean state) {
        model.setState(state);
    }

    /**
     * Set the state without triggering any StateChanged events.
     * Set the new state to true, false, or null
     * for SELECTED, NOT_SELECTED or DONT_CARE, respectively.
     */
    public void setStateSilently(Boolean state) {
        model.setStateSilently(state);
    }

    /** Return the current state, which is determined by the
     * selection status of the model. */
    public Boolean getState() {
        return model.getState();
    }

    /**
     * For a "dont care" value, just show a blank table cell.
     */
    protected void paintComponent(Graphics g) {
        if (getState() != null) {
            super.paintComponent(g);
        } else {
            g.setColor(getBackground());
            g.fillRect(0, 0, getWidth(), getHeight());
        }
    }

    public static void main(String args[]) throws Exception {
        UIManager.setLookAndFeel
                (UIManager.getSystemLookAndFeelClassName());
        JFrame frame = new JFrame("TristateTableCheckBoxTest");     //NOI18N
        frame.getContentPane().setLayout(new GridLayout(0, 1, 5, 5));
        final LcTristateCheckBox swingBox = new LcTristateCheckBox(
                "Testing the tristate checkbox");  //NOI18N
        swingBox.setMnemonic('T');
        frame.getContentPane().add(swingBox);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);
    }
}
