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
import javax.swing.plaf.*;
import javax.swing.event.*;
import vnmr.util.*;

public class ClockDial extends JPanel implements ChangeListener
{
    private IntegerRangeModel model;
    private boolean isShortHand = true;
    private boolean isLongHand = false;
    private boolean isTicked = true;
    private boolean isMinMarker = false;
    private boolean isMaxMarker = false;
    private boolean isDigitalReadout = true;
    private int unitsPerRev = 100;
    private double scale = 0.1;
    private boolean isPieSlice = true;
    private boolean isColorBars = false;
    private Color maxColor = Color.blue;
    private Color[] barColors = {
        Color.BLACK,
        new Color(0x555555),
        new Color(0x666666),
        new Color(0x808080),
        new Color(0x999999),
        new Color(0xaaaaaa),
        new Color(0xbbbbbb),
        new Color(0xcccccc),
        new Color(0xdddddd),
        Color.WHITE,
    };
    private JButton resetButton = null;

    public ClockDial() {
	model = new IntegerRangeModel();
	model.addChangeListener(this);
	// don't do setPreferredSize here.
	// if setPreferredSize was overridden, it will
	// hang up java. java bug.
	// setPreferredSize(new Dimension(100, 100));
	setOpaque(false);
	setLayout(new ClockDialLayout());
	resetButton = new JButton(new VOvalIcon(9, 9, maxColor));
	resetButton.setPressedIcon(new VOvalIcon(9, 9, maxColor.darker()));
	resetButton.setBorder(new VButtonOvalBorder());
	resetButton.setContentAreaFilled(false);
    resetButton.setRequestFocusEnabled(false);
	resetButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
		setMaxReading(getValue());
		if (getBoundsAreElastic()) {
		    setMaximum(model.getNextMaximum(getValue()));
		}
            }
        });
	updateUI();
    }

    public boolean isTicked() { return isTicked; }
    public void setTicked(boolean b) { isTicked = b; }
    public boolean isShortHand() { return isShortHand; }
    public void setShortHand(boolean b) { isShortHand = b; }
    public boolean isLongHand() { return isLongHand; }
    public void setLongHand(boolean b) { isLongHand = b; }
    public boolean isMinMarker() { return isMinMarker; }
    public void setMinMarker(boolean b) { isMinMarker = b; }
    public boolean isMaxMarker() { return isMaxMarker; }
    public void setMaxMarker(boolean b) {
	if (b != isMaxMarker) {
	    if (b) {
		add(resetButton);
	    }else{
		remove(resetButton);
	    }
	}
	isMaxMarker = b;
    }

    public boolean getBoundsAreElastic() { return model.getBoundsAreElastic(); }
    public void setBoundsAreElastic(boolean b) {
	model.setBoundsAreElastic(b);
    }
    public boolean isDigitalReadout() { return isDigitalReadout; }
    public void setDigitalReadout(boolean b) { isDigitalReadout = b; }
    public int getUnitsPerRev() { return unitsPerRev; }
    public void setUnitsPerRev(int n) { unitsPerRev = n; }
    public double getScaling() { return scale; }
    public void setScaling(double x) { scale = x; }
    public boolean isPieSlice() { return isPieSlice; }
    public void setPieSlice(boolean b) { isPieSlice = b; }
    public boolean isColorBars() { return isColorBars; }
    public void setColorBars(boolean b) { isColorBars = b; }
    public Color getBarColor(int i) { return barColors[i]; }
    public void setBarColor(int i, Color c) { barColors[i] = c; }
    public Color getMaxColor() { return maxColor; }
    public void setMaxColor(Color c) {
	resetButton.setIcon(new VOvalIcon(9, 9, c));
	resetButton.setPressedIcon(new VOvalIcon(9, 9, c.darker()));
	maxColor = c;
    }

    public int getValue() { return model.getValue(); }
    public void setValue(int n) { model.setValue(n); }
    public void setValue(double x) { setValue((int)Math.rint(x / scale)); }
    public int getMinimum() { return model.getMinimum(); }
    public void setMinimum(int n) { model.setMinimum(n); }
    public void setMinimum(double x) {
	setMinimum((int)Math.rint(x / scale)); }
    public int getMaximum() { return model.getMaximum(); }
    public void setMaximum(int n) { model.setMaximum(n); }
    public void setMaximum(double x) {
	setMaximum((int)Math.rint(x / scale)); }
    public int getMinReading() { return model.getMinReading(); }
    public void setMinReading(int n) { model.setMinReading(n); }
    public void setMinReading(double x) {
	setMinReading((int)Math.rint(x / scale)); }
    public int getMaxReading() { return model.getMaxReading(); }
    public void setMaxReading(int n) { model.setMaxReading(n); }
    public void setMaxReading(double x) {
	setMaxReading((int)Math.rint(x / scale)); }

    public IntegerRangeModel getModel() {
	return model;
    }

    public void setModel(IntegerRangeModel newModel)
    {
	if (model != null) {
	    model.removeChangeListener(this);
	}
	model = newModel;
	if (model != null) {
	    model.addChangeListener(this);
	}
    }

    public PanelUI getUI() {
	return (PanelUI) ui;
    }


    public void setUI(ClockDialUI ui) {
        super.setUI(ui);
    }

    /**
     * Called to replace the UI with the latest version from the
     * default UIFactory.
     */
    public void updateUI() {
	setUI((ClockDialUI)UIManager.getUI(this));
    }

    public String getUIClassID() { return "ClockDialUI"; }

    public void stateChanged(ChangeEvent e) {
	repaint();
    }

    /**
     * The layout manager for ClockDial's internal components.
     */
    class ClockDialLayout implements LayoutManager {

	public void addLayoutComponent(String name, Component comp) {}

	public void removeLayoutComponent(Component comp) {}

	/**
	 * Calculate the preferred size. (Unused)
	 * @param target component to be laid out
	 * @see #minimumLayoutSize
	 */
	public Dimension preferredLayoutSize(Container target) {
	    return new Dimension(0, 0); // unused
	} // preferredLayoutSize()

	/**
	 * Calculate the minimum size. (Unused)
	 * @param target component to be laid out
	 * @see #preferredLayoutSize
	 */
	public Dimension minimumLayoutSize(Container target) {
	    return new Dimension(0, 0); // unused
	} // minimumLayoutSize()

	/**
	 * do the layout
	 * @param target component to be laid out
	 */
	public void layoutContainer(Container target) {
	    synchronized (target.getTreeLock()) {
		Dimension targetSize = target.getSize();
		Insets insets = target.getInsets();
		int x1, y1;
		int width = getWidth()-insets.left-insets.right;
		int height = getHeight()-insets.top-insets.bottom;
		int radius = (Math.min(width, height) - 1) / 2;
		Dimension resetDim = resetButton.getPreferredSize();
		int dx = resetDim.width;
		int dy = resetDim.height;
		int offset = (int)((radius) / 1.414);
		x1 = width/2 + insets.left + offset;
		y1 = height/2 + insets.top + offset;
		resetButton.setBounds(x1, y1, dx, dy);
	    }
	}
    }

}
