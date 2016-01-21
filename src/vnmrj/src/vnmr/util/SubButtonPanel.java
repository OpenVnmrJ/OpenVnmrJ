/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.ui.*;

public class SubButtonPanel extends JPanel implements DockConstants, ActionListener {
    private ButtonIF  buttonIf;
    private Vector  butVector;
    private int  butCode = VnmrKey.VMENU;
    private int  orientation = HORIZONTAL;
    private boolean bNative = false;
    private boolean bChanging = false;
    private ExpButtonLayout layout;

    /**
     * constructor
     */
    public SubButtonPanel(ButtonIF bif) {
	this.buttonIf = bif;
	this.butVector = new Vector();
	this.butVector.setSize(30);
	setOpaque(false);
        layout = new ExpButtonLayout();
	setLayout(layout);
	bNative = Util.isNativeGraphics();
/*
	setOrientation(JToolBar.VERTICAL);
	setFloatable(false);

	if (Util.isNativeGraphics())
	    ToolTipManager.sharedInstance().setLightWeightPopupEnabled(false);
	setBorderPainted(false);
        setMargin(new Insets(0,0,20,0));
*/
    } // SubButtonPanel()


    public void removeButton(int id) {
	if (id < butVector.size()) {
	    ExpButton but = (ExpButton) butVector.elementAt(id);
	    if (but != null) {
	    	but.setVisible(false);
	    }
	}
    }

    public void removeAllButtons() {
	int num = butVector.size();
	ExpButton but;
        bChanging = true;
	for (int k = num - 1; k >= 0; k--) {
	    but = (ExpButton) butVector.elementAt(k);
	    if (but != null) {
	    	but.setNeeded(false);
	    }
	}
/*
	validate();
	if (isVisible())
	   repaint();
*/
    }

    public void showAllButtons() {
	int num = butVector.size();
	ExpButton but;
	for (int k = num - 1; k >= 0; k--) {
	    but = (ExpButton) butVector.elementAt(k);
	    if (but != null) {
		if (but.isNeeded())
	    	    but.setVisible(true);
		else
	    	    but.setVisible(false);
	    }
	}
        if (isShowing()) {
            if (butCode == VnmrKey.VMENU) {
                GraphicsToolIF tool = Util.getGraphicsToolBar();
                if (tool != null) {
                     tool.resetSize();
                     tool.adjustDockLocation();
                }
            }
        }
        bChanging = false;
	layout.layoutContainer(this);
	repaint();
        // revalidate();
/* 
        if (getParent() != null) {
           Container p = getParent();
           if (p instanceof GraphicsToolIF) 
              ((GraphicsToolIF)p).resetSize();
        }
*/
    }

    private synchronized ExpButton getButton(int id) {
	int k = butVector.size();
	if (k <= id)
	    butVector.setSize(id + 6);
	ExpButton button = (ExpButton) butVector.elementAt(id);
	if (button == null) {
	    button = new ExpButton(id);
            button.setNeeded(false);
            button.setGraphicsCtrl(true);
	    add(button);
	    butVector.setElementAt(button, id);
	    if (buttonIf != null)
	   	button.addActionListener((ActionListener) this);
	    if (bNative) {
	    	VTipManager toolTipManager = VTipManager.tipManager();
            	toolTipManager.registerComponent(button);
	    }
	}
        return (button);
    }

    public void addButton(String gif, int id) {
	int k = butVector.size();
	if (k <= id)
	    butVector.setSize(id + 6);
	ExpButton but = getButton(id);
	if (but == null)
            return;
	but.setIconData(gif);
        but.setNeeded(true);
	// validate();
    }

    public void addButton(String gif, int id, int dir) {
	int k = butVector.size();
	if (k <= id)
	    butVector.setSize(id + 6);
	ExpButton but = getButton(id);
	if (but == null)
            return;
	boolean ispushed=false;
	boolean isenabled=true;
	int index=0;
	if((index=gif.indexOf(':'))>0){
	    String state=gif.substring(index+1);
	    gif=gif.substring(0, index);
	    if(state.equals("on"))
	    	ispushed=true;
	    else if(state.equals("off"))
	    	isenabled=false;
	}
		
	but.setIconData(gif, dir);
	//if(gif.equals("1Dspwp.gif"))
	//	isactive=true;
	but.setSelected(ispushed);
	//else
	but.setEnabled(isenabled);
        but.setNeeded(true);
	// validate();
    }

    public void setButtonCode(int n) {
        butCode = n;
    }

    public void setButtonTip(String label, int id) {
	if (id < butVector.size()) {
	    ExpButton but = (ExpButton) butVector.elementAt(id);
	    if (but != null) {
	    	but.setTooltip(label);
	    }
	}
    }

    public void clearButtonCmds() {
	int num = butVector.size();
	for (int k = 0; k < num; k++) {
	    ExpButton but = (ExpButton) butVector.elementAt(k);
	    if (but != null)
	    	but.setCommand(null);
	}
    }

    public void setButtonCmd(String cmd, int id) {
	ExpButton but = getButton(id);
	if (but != null)
	    but.setCommand(cmd);
    }

    public void actionPerformed(ActionEvent  evt) {
	Object obj = evt.getSource();
        if (obj instanceof ExpButton) {
            ExpButton b = (ExpButton) obj;
            int id = b.getId() + 1;
            String cmd = "jFunc("+butCode+","+id+")\n"; 
            buttonIf.sendToVnmr(cmd); 

            /****
            String cmd = b.getCommand();
            if (cmd == null || cmd.length() < 1) {
                int id = b.getId() + 1;
                cmd = "jFunc("+butCode+","+id+")\n"; 
            }
            if (butCode == VnmrKey.AIPMENU) {
                buttonIf.sendToVnmr("menu('aip')\n"); 
            }
            ********/
	    // buttonIf.buttonActiveCall(b.getId());
        }
	buttonIf.setInputFocus();
    }

    public int getOrientation()
    {
        return orientation;
    }

    public void setOrientation( int o )
    {
        if (orientation == o)
           return;
        if (o != VERTICAL && o != HORIZONTAL)
           return;
        orientation = o;
        revalidate();
        // repaint();
    }


    class ExpButtonLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        /**
         * calculate the preferred size
         * @param target component to be laid out
         * @see #minimumLayoutSize
         */
        public Dimension preferredLayoutSize(Container target) {
	    int nmembers = target.getComponentCount();
	    Dimension dim = new Dimension(0,0);
            int w = 0;
            int h = 0;
            int count = 0;

            for (int i = 0; i < nmembers; i++) {
               Component comp = target.getComponent(i);
               if (comp != null && comp.isVisible()) {
                  count++;
                  dim = comp.getPreferredSize();
                  if (orientation == HORIZONTAL) {
	              w += dim.width;
                      if (dim.height > h)
                         h = dim.height;
                  }
                  else {
	              h += dim.height;
                      if (dim.width > w)
                         w = dim.width;
                  }
               }
	    }
            if (count > 0) {
               w += 2;
               h += 2;
            }
            else {
               w = 30;
               h = 30;
            }
            dim.width = w;
            dim.height = h;
            return dim;
        } // preferredLayoutSize()

	public Dimension minimumLayoutSize(Container target) {
            return new Dimension(20, 100); // unused
        } // minimumLayoutSize()

        /**
         * do the layout
         * @param target component to be laid out
         */
        public void layoutContainer(Container target) {
            if (bChanging)
                return;
            synchronized (target.getTreeLock()) {
                Insets insets = target.getInsets();
		int x, y;
		int nmembers = target.getComponentCount();
                Dimension size = target.getSize();
                if (orientation == HORIZONTAL) {
		   x = 0;
		   y = 1;
                }
                else {
		   x = 1;
		   y = 0;
                }
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
		    if (comp != null && comp.isVisible()) {
                       size = comp.getPreferredSize();
                       if (orientation == HORIZONTAL) {
                          comp.setBounds(x, y, size.width, size.height);
		          x += size.width;
                       }
                       else {
                          comp.setBounds(x, y, size.width, size.height);
		          y += size.height;
                       }
		    }
                }
	    }
	}
    }

} // class SubButtonPanel
