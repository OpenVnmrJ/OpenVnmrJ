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
import java.beans.*;
import javax.swing.*;
import vnmr.util.*;

/**
 * The container for the menubar.
 *
 */
public class MenuPanel extends JPanel implements PropertyChangeListener {
    private JComponent menu;
    private JPanel dummy;

    public MenuPanel(int top, int left, int bottom, int right) {
	// setBorder( BorderFactory.createEmptyBorder(top, left, bottom, right) );
	setLayout(new BorderLayout());
	setOpaque(true);
        dummy = new JPanel();
        dummy.setSize(left, 8);
	dummy.setOpaque(false);
	add(dummy, BorderLayout.WEST);
        DisplayOptions.addChangeListener(this);

    } // MenuPanel()

    public MenuPanel() {
        this(0, 10, 0, 0);
    } // MenuPanel()


    public JComponent getMenuBar() {
	return menu;
    }

    public void setMenuBar(JComponent obj) {
	if (menu != null)
	   remove(menu);
	menu = obj;
	add(menu, BorderLayout.CENTER);
	validate();
	repaint();
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
         if (DisplayOptions.isUpdateUIEvent(evt)) {
             setBackground(Util.getMenuBarBg());
         }
    }

/*
    public void paint(Graphics g) {
        super.paint(g);
        Dimension dim = getSize();
        g.setColor(getBackground().darker());
        g.drawLine(1, dim.height - 1, dim.width, dim.height - 1);
    }
*/
} // class MenuPanel
