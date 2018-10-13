/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import vnmr.util.*;

public class ComboSlider extends JPanel {

    protected JSlider slider=null;
    protected JTextField entry=null;
    protected int entryLen = -1;
    protected int charWidth = 8;

    public ComboSlider() {
	setLayout(new MyLayout());
	slider = new JSlider(0, 100, 0);
	entry = new JTextField("0");
	setWidth(entry);
	// setBounds will make thumb to show
	// don't call setBounds now, because it is unnecessary for java 1.4
	// also, it may cause problem when change UI.

/**
	Dimension dim = slider.getPreferredSize();
        Insets insets = slider.getInsets();
        dim.width = dim.width - insets.left - insets.right - 8;
        dim.height = dim.height - insets.top - insets.bottom;
        slider.setBounds(0, 0, dim.width, dim.height);
**/

	add(entry);
	add(slider);
    }

    public boolean isAdjusting() {return slider.getValueIsAdjusting();}

    public void setMinimum(int min) { 
	slider.setMinimum(min); 
    }

    public void setMaximum(int max) { 
	slider.setMaximum(max); 
	setWidth(entry);
    }

    public void setValue(int v) {
	slider.setValue(v);
	entry.setText(String.valueOf(v));
    }

    public int getSliderValue() {return slider.getValue();}

    public String getEntryValue() {return entry.getText();}

    public void setForeground(Color c) {
	if (entry != null) entry.setForeground(c);
    }

    public JTextField getEntry()
    {
	return entry;
    }

    public JSlider getSlider()
    {
	return slider;
    }

    public void setFont(Font font) {
	if (entry != null) entry.setFont(font);
	FontMetrics fm = getFontMetrics(font);
        charWidth = fm.stringWidth("0");
    }

    public void setCursor(Cursor cursor) {
	entry.setCursor(cursor);
	slider.setCursor(cursor);
	super.setCursor(cursor);
    }

    public void addActionListener(ActionListener l) {
	entry.addActionListener(l);
    }

    public void addChangeListener(ChangeListener l) {
	slider.addChangeListener(l);
    }

    public void addMouseListener(MouseListener ml) {
	super.addMouseListener(ml);
	slider.addMouseListener(ml);
	entry.addMouseListener(ml);
    }

    public void removeMouseListener(MouseListener ml) {
	super.removeMouseListener(ml);
	slider.removeMouseListener(ml);
	entry.removeMouseListener(ml);
    }

    public void addMouseMotionListener(MouseMotionListener ml) {
	super.addMouseMotionListener(ml);
	slider.addMouseMotionListener(ml);
	entry.addMouseMotionListener(ml);
    }

    public void removeMouseMotionListener(MouseMotionListener ml) {
	super.removeMouseMotionListener(ml);
	slider.removeMouseMotionListener(ml);
	entry.removeMouseMotionListener(ml);
    }

    /**
     * The layout manager for ComboSlider.
     */
    class MyLayout implements LayoutManager {

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
		Dimension entryDim = entry.getPreferredSize();
		Dimension sliderDim = slider.getPreferredSize();
		int width = targetSize.width - insets.left - insets.right;
		int height = targetSize.height - insets.top - insets.bottom;
		int x1 = insets.left;
		entryDim.width = charWidth * entry.getColumns();
		int x2 = x1 + entryDim.width;
		int x3 = width - insets.right;
		int y1 = (height - entryDim.height) / 2;
		int y2 = y1 + entryDim.height;
		int y3 = (height - sliderDim.height) / 2;
		int y4 = y3 + sliderDim.height;
                if (slider.getOrientation() == JSlider.VERTICAL) {
                    y3 = insets.top;
                    y4 = y3 + height;
                }
		entry.setBounds(x1, y1, entryDim.width, y2-y1);
		if (entry.isVisible())
		   slider.setBounds(x2, y3, x3-x2, y4-y3);
		else
		   slider.setBounds(x1, y3, x3-x1, y4-y3);
	    }
	}
    }

/*
    public void validate() {
	setWidth(entry);
	super.validate();
    }
*/

    public void setPreferredSize(Dimension d) {
	super.setPreferredSize(d);
	validate();
    }

    private void setWidth(JTextField entry) {
	if (entryLen >= 0)
	   return;
	int l1 = String.valueOf(slider.getMinimum()).length();
	int l2 = String.valueOf(slider.getMaximum()).length();
	entry.setColumns(Math.max(l1, l2)+1);
    }
}
