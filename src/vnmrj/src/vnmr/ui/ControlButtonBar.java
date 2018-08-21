/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.util.Hashtable;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;

import vnmr.util.*;

/**
 * This button bar includes the tabs and buttons that line
 * the top of the ControlPanel.
 */
public class ControlButtonBar extends JPanel {
    /** middle buttons panel */
    private JPanel midSection;
    /** CardLayout for middle buttons section */
    private CardLayout midCardLayout;
    private JPanel tabSection;
    JToolBar rightSection;

    /**
     * constructor
     * @param sshare session share
     */
    public ControlButtonBar(SessionShare sshare) {
	setBorder(BorderFactory.createEmptyBorder());
	setLayout(new controlBarLayout());
        setBackground(null);

	// add the set of tabs
	tabSection = new JPanel();
	tabSection.setOpaque(false);
        tabSection.setBackground(null);
	tabSection.setLayout(new tabLayout());

	add(tabSection);

	String appType = sshare.user().getAppType();
	if (Global.WALKUPIF.equals(appType)) {
	    // add the middle section of buttons
	    midSection = new JPanel();
	    midSection.setOpaque(false);
            midSection.setBackground(null);
	    midSection.setLayout(new tabLayout());
	    add(midSection);

	}
    } // ControlButtonBar()

    public JComponent getTabArea() {
	return tabSection;
    }

    public JComponent getActionArea() {
	return midSection;
    }


class tabLayout implements LayoutManager {

	int	minW = 10;
	int	minH = 10;
        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
	    int nums = target.getComponentCount();
	    int totalW = 0;
	    int totalH = 0;
            Dimension dim;
	    for (int i = 0; i < nums; i++) {
                Component comp = target.getComponent(i);
                dim = comp.getPreferredSize();
		if (dim.height > totalH)
		    totalH = dim.height;
		totalW += dim.width;
		if (dim.width > minW)
		    minW = dim.width;
	    }
	    totalW = totalW + nums + 4;
	    minH = totalH;
            return new Dimension(totalW, totalH); // unused
        } // preferredLayoutSize()

       public Dimension minimumLayoutSize(Container target) {
            return new Dimension(minW, minH); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
		Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();
		int usableWidth = targetSize.width - insets.right;
		int usableHeight = targetSize.height - insets.top;
                Dimension dim;
                Component comp;
		int totalW = 0;
		int gapX = 0;
		int lastW = 0;
		int  x = 1;
		int  y = 0;
		int  k;
		int nums = target.getComponentCount();
		if (nums <= 0)
		    return;
        	for (k = 0; k < nums; k++) {
		    comp = target.getComponent(k);
                    dim = comp.getPreferredSize();
		    totalW += dim.width;
		    if (k == 0) {
			lastW = dim.width;
		    }
		}
		if (nums > 1 && (totalW > usableWidth)) {
		    x = usableWidth - lastW;
	  	    if (x < 0) 
			x = 0;
		    usableWidth -= lastW;
		    gapX = usableWidth / (nums - 1);
		    if (gapX < 1)
			gapX = 1;
		    comp = target.getComponent(0);
                    dim = comp.getPreferredSize();
		    y = usableHeight - dim.height;
		    comp.setBounds( new Rectangle(x, y, dim.width, dim.height));
		    x = 1;
        	    for (k = nums - 1; k > 0; k--) {
		        comp = target.getComponent(k);
                        dim = comp.getPreferredSize();
		        y = usableHeight - dim.height;
			comp.setBounds( new Rectangle(x, y, dim.width, dim.height));
			if (dim.width < gapX)
			   x += dim.width;
			else
			   x += gapX;
		    }
		    return;
		}
		else if (totalW > usableWidth) {
		    comp = target.getComponent(0);
                    dim = comp.getPreferredSize();
		    y = usableHeight - dim.height;
		    comp.setBounds( new Rectangle(x, y, usableWidth, dim.height));
		    return;
		}
		else {
        	    for (k = nums - 1; k >= 0; k--) {
		        comp = target.getComponent(k);
                        dim = comp.getPreferredSize();
		        y = usableHeight - dim.height;
			comp.setBounds( new Rectangle(x, y, dim.width, dim.height));
			x += dim.width + 1;
		    }
		}
            }
        }
}

class controlBarLayout implements LayoutManager {
	int	minW = 10;
	int	minH = 10;
       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
	    int nums = target.getComponentCount();
	    int totalW = 0;
	    int totalH = 0;
            Dimension dim;
	    for (int i = 0; i < nums; i++) {
                Component comp = target.getComponent(i);
                dim = comp.getPreferredSize();
		if (dim.height > totalH)
		    totalH = dim.height;
		totalW += dim.width;
		if (dim.width > minW)
		    minW = dim.width;
	    }
	    totalW = totalW + nums + 4;
	    if (totalH > minH)
	        minH = totalH;
            return new Dimension(totalW, totalH); // unused
	}
       public Dimension minimumLayoutSize(Container target) {
            return new Dimension(minW, minH); // unused
        } // minimumLayoutSize()

       public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
		Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();
		int usableWidth = targetSize.width - insets.right;
		int usableHeight = targetSize.height - insets.top;
                Dimension dim;
		int	x = 0;
		int	y = 0;
                dim = tabSection.getPreferredSize();
		y = targetSize.height - dim.height;
		tabSection.setBounds( new Rectangle(x, y, dim.width, dim.height));
		x += dim.width + 2;
/*
		if (rightSection != null) {
                    dim = rightSection.getPreferredSize();
		    int x2 = usableWidth - dim.width;
		    y = targetSize.height - dim.height;
		    usableWidth = usableWidth - dim.width;
		    rightSection.setBounds( new Rectangle(x2, y, dim.width, dim.height));
		}
*/
		if (midSection != null) {
                    dim = midSection.getPreferredSize();
		    y = targetSize.height - dim.height;
		    if (y < 0)
			y = 0;
		 //   if (x + dim.width > usableWidth)
			dim.width = usableWidth - x;
		    if (dim.width < 0)
			dim.width = 0;
		    midSection.setBounds( new Rectangle(x, y, dim.width, dim.height));
		}
	    }
	}
}

} // class ControlButtonBar
