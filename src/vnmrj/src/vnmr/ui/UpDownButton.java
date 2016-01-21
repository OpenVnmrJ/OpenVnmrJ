/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.AbstractBorder;
import javax.swing.plaf.basic.BasicButtonListener;

import vnmr.util.*;
import vnmr.ui.ThreeButtonModel;
import vnmr.ui.IntegerRangeModel;

public class UpDownButton extends JComponent {

    static private Cursor cursor = null;

    private ThreeButtonModel stateModel;
    private IntegerRangeModel dataModel;
    private String label = "";
    private ChangeListener stateListener = null;
    private ChangeListener dataListener = null;
    protected ActionListener actionListener = null;
    private boolean isWrapping = false;

    // These are static to force all the UpDownButtons to look the same.
    // See also VUpDownButton.java
    static private boolean isPointy = false;
    static private boolean isRocker = true;
    static private boolean isArrow = false;
    // static private Color arrowColor = new Color(0x0000ff);
    static private Color arrowColor = new Color(8, 187, 249);

    public UpDownButton() {
    setDataModel(new IntegerRangeModel());
    setStateModel(new ThreeButtonModel());
    stateModel.addChangeListener(new StateListener());
    dataModel.addChangeListener(new DataListener());
    setOpaque(false);
    // don't do setPreferredSize here.
    // setPreferredSize(new Dimension(100, 30));
    setMinimumSize(new Dimension(85, 25));
    setMaximumSize(new Dimension(115, 40));
    setName(" ");
    /*
    Toolkit tk = Toolkit.getDefaultToolkit();
    if (tk.getMaximumCursorColors() > 0) {
        if (cursor == null) {	// Make a custom cursor
        Image image = Util.getVnmrImage("upDownCursor.gif");
        try {
            cursor = tk.createCustomCursor(image,
                           new Point(8, 1),
                           "upDownCursor");
        } catch (NegativeArraySizeException e) {
            // No image file; forget it.
            cursor = null;
        }
        }
        if (cursor != null) {
        setCursor(cursor);
        }
    }
    */
    updateUI();
    }

    // Set/Get private variables
    public ThreeButtonModel getStateModel() { return stateModel; }
    public IntegerRangeModel getDataModel() {return dataModel; }
    public void setLabel(String label) { this.label = label; }
    public String getLabel() { return label; }
    public static void setPointy(boolean b) { isPointy = b; }
    public static boolean isPointy() { return isPointy; }
    public static void setRocker(boolean b) { isRocker = b; }
    public static boolean isRocker() { return isRocker; }
    public static void setArrow(boolean b) { isArrow = b; }
    public static boolean isArrow() { return isArrow; }
    public static void setArrowColor(Color c) { arrowColor = c; }
    public static Color getArrowColor() { return arrowColor; }
    public void setName(String name) {
    getDataModel().setName(name);
    super.setName(name);
    }
    public boolean isWrapping() { return isWrapping; }
    public void setWrapping(boolean b) { isWrapping = b; }

    // Set/Get model variables
    public void setValue(int n) {
        Messages.postDebug("shimButton", "UpDownButton.setValue(" + n + ")");
        if (isWrapping()) {
            while (n > getMaximum()) {
                n -= getMaximum() - getMinimum();
            }
            while (n < getMinimum()) {
                n += getMaximum() - getMinimum();
            }
            Messages.postDebug("shimButton", "UpDownButton.setValue(): n=" + n);
        }
        dataModel.setValue(n);
    }
    public int getValue() { return dataModel.getValue(); }
    public String getValueStr() {
    return Integer.toString(getValue()); }
    public void setIncrement(int v) { dataModel.setIncrement(v); }
    public void setIncrement(int n, int v) { dataModel.setIncrement(n, v); }
    public void setIncrements(int v1, int v2, int v3) {dataModel.setIncrements(v1, v2, v3);}
    public int getIncrement() { return dataModel.getIncrement(); }
    public int getIncrement(int n) { return dataModel.getIncrement(n); }
    public String getIncrementStr() {
    return Integer.toString(getIncrement()); }
    public void setMinimum(int n) { dataModel.setMinimum(n); };
    public int getMinimum() { return dataModel.getMinimum(); };
    public void setMaximum(int n) { dataModel.setMaximum(n); };
    public int getMaximum() { return dataModel.getMaximum(); };

    /**
     * Set the state model that this button represents.
     * @param m the Model
     * @see #getStateModel
     */
    public void setStateModel(ThreeButtonModel m) {

    ThreeButtonModel oldModel = getStateModel();
    if (oldModel != null) {
        oldModel.removeChangeListener(stateListener);
        stateListener = null;
        oldModel.removeActionListener(actionListener);
        actionListener = null;
    }
    stateModel = m;
    if (m != null) {
        stateListener = new StateListener();
        m.addChangeListener(stateListener);
        actionListener = createActionListener();
        m.addActionListener(actionListener);
    }

    invalidate();
    repaint();
    }

    /**
     * Set the data model that this button represents.
     * @param m the Model
     * @see #getDataModel
     */
    public void setDataModel(IntegerRangeModel m) {

    IntegerRangeModel oldModel = getDataModel();
    if (oldModel != null) {
        oldModel.removeChangeListener(dataListener);
        dataListener = null;
    }
    dataModel = m;
    if (m != null) {
        dataListener = new DataListener();
        m.addChangeListener(dataListener);
    }

    invalidate();
    repaint();
    }

    class StateListener implements ChangeListener {
    public void stateChanged(ChangeEvent e) {
        // Either change the data model or repaint
        repaint();
    }
    }

    class DataListener implements ChangeListener {
    public void stateChanged(ChangeEvent e) {
        repaint();
    }
    }

    public UpDownButtonUI getUI() {
    return (UpDownButtonUI)ui;
    }

    public void setUI(UpDownButtonUI ui) {
        super.setUI(ui);
    }

    /**
     * Called to replace the UI with the latest version from the
     * default UIFactory.
     */
    public void updateUI() {
    setUI((UpDownButtonUI)UIManager.getUI(this));
    }

    public String getUIClassID() { return "UpDownButtonUI"; }

    /**
     * Notify all listeners that have registered interest for
     * notification on this event type.  The event instance
     * is lazily created using the parameters passed into
     * the fire method.
     * @see javax.swing.event.EventListenerList
     * @see #getActionCommand
     */
    protected void fireActionPerformed(ActionEvent event) {
    // Guaranteed to return a non-null array
    Object[] listeners = listenerList.getListenerList();
    ActionEvent e = null;
    // Process the listeners last to first, notifying
    // those that are interested in this event
    for (int i = listeners.length-2; i>=0; i-=2) {
        if (listeners[i]==ActionListener.class) {
        // Lazily create the event:
        if (e == null) {
            e = new ActionEvent(this,
                    ActionEvent.ACTION_PERFORMED,
                    getActionCommand());
        }
        ((ActionListener)listeners[i+1]).actionPerformed(e);
        }
    }
    }

    /**
     * adds an ActionListener to the button
     */
    public void addActionListener(ActionListener l) {
    listenerList.add(ActionListener.class, l);
    }

    /**
     * removes an ActionListener from the button
     */
    public void removeActionListener(ActionListener l) {
    listenerList.remove(ActionListener.class, l);
    }

    /**
     * Sets the action command for this button.
     */
    public void setActionCommand(String actionCommand) {
    getStateModel().setActionCommand(actionCommand);
    }

    /**
     * Returns the action command for this button.
     */
    public String getActionCommand() {
    String ac = getStateModel().getActionCommand();
    if(ac == null) {
        ac = getLabel();
    }
    return ac;
    }

    private class ForwardActionEvents implements ActionListener {
    public void actionPerformed(ActionEvent event) {
        fireActionPerformed(event);
    }
    }

    protected ActionListener createActionListener() {
    return new ForwardActionEvents();
    }

}
