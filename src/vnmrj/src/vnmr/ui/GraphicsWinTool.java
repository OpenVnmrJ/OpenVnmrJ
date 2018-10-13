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
import java.beans.*;
import javax.swing.*;
import javax.swing.border.*;

import vnmr.util.*;

public class GraphicsWinTool extends JWindow implements GraphicsToolIF, SwingConstants, PropertyChangeListener {
    private NavigateTool navigationObj;
    private Component toolBar;
    private boolean  bDocked = false;
    private int  mx = 0;
    private int  my = 0;
    private int  mRx = 0;
    private int  mRy = 0;
    private int  dx = 0;
    private int  dy = 0;
    private int  tx = -1;
    private int  ty = 0;
    private int  toolW = 30;
    private int  gap = 10;
    private int  dockTop = 10;
    private int  dockBottom = 800;
    private int  dockRTop = 10;
    private int  dockRBottom = 800;
    private int  dockEast = 900; // the point of the right most 
    private int  orientation = VERTICAL;
    private int  defaultOrient = HORIZONTAL;
    private int  dockPosition = 2;  // 0: no dock, 1: dock left, 2: dock right
    private int  topPanX = 0;
    private int  topPanY = 0;
    private JPanel  contPan;
    private LineBorder  border;
    private JPopupMenu  popup;
    private SubButtonPanel toolComp = null;
    private JComponent topPanel = null;

    public GraphicsWinTool(JFrame frame) {
        super(frame);
        super.setFocusableWindowState(false);

	// setLayout(new BorderLayout());
        contPan = new JPanel();
        getContentPane().add(contPan, BorderLayout.CENTER);

	contPan.setLayout(new ftoolbarLayout());
        navigationObj = new NavigateTool(""); 
        contPan.add(navigationObj);
	contPan.setOpaque(true);
        border = new LineBorder(Color.gray, 1, true);
        contPan.setBorder(null);
        navigationObj.setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));

        navigationObj.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                mx = e.getX();
                my = e.getY();
                PointerInfo ptInfo = MouseInfo.getPointerInfo();
                mRx = ptInfo.getLocation().x;
                mRy = ptInfo.getLocation().y;
                tx = -1;
                if (e.getButton() == MouseEvent.BUTTON3) {
                    popupMenu(mx, my);
                }
            }
        });

        navigationObj.addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent e)
            {
                  moveTool(e);
            }
        });

        popup = new JPopupMenu();
        PopActionEar ear = new PopActionEar();
        JMenuItem mi = new JMenuItem("Vertical");
        mi.addActionListener(ear);
        popup.add(mi);
        mi = new JMenuItem("Horizontal");
        mi.addActionListener(ear);
        popup.add(mi);
        mi = new JMenuItem("Close");
        mi.addActionListener(ear);
        popup.add(mi);
        navigationObj.setBackground(Util.getToolBarBg()); 
	// GraphicsWinTool.addChangeListener(this);
        pack();

    } // GraphicsWinTool()

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBackground(Util.getBgColor());
        navigationObj.setBackground(Util.getToolBarBg()); 
    }

    private void popupMenu(int x, int y) {
        popup.show(this, x+2, y);
    }

    public int getOrientation()
    {
        return orientation;
    }

    public void setOrientation( int o )
    {
        int oldVal = orientation;

        if (o != VERTICAL && o != HORIZONTAL)
           return;
        orientation = o;
        if (toolComp == null)
           return;
        oldVal = toolComp.getOrientation();
        if (oldVal == o)
           return;
	toolComp.setOrientation(orientation);
        resetSize();
    }

    public int getDockPosition()
    {
        return dockPosition;
    }

    public void setDockPosition(int d)
    {
        int oldDock = dockPosition;
        int oldVal = orientation;
        if (d > 0) {
            // setBorder(null);
            orientation = VERTICAL;
        }
        else {
            // if (dockPosition != 0)
            //    setBorder(border);
            orientation = defaultOrient;
        }
        dockPosition = d;
        if (toolComp == null)
           return;
        oldVal = toolComp.getOrientation();
        if (oldDock != d || oldVal != orientation) {
	    toolComp.setOrientation(orientation);
           ExpViewArea ev = Util.getViewArea();
           if (ev != null)
               ev.setResizeStatus(true);
            resetSize();
            validate();
            topPanel.doLayout();
            topPanel.repaint();
           if (ev != null)
               ev.setResizeStatus(false);
        }
    }

    public int getDefaultOrientation()
    {
        return defaultOrient;
    }

    public void setDefaultOrientation( int o )
    {
        defaultOrient = o;
        setOrientation(o);
    }

    public void setDockTopPosition(int y) {
         dockTop = y;
    }

    public void setDockBottomPosition(int y) {
         dockBottom = y;
    }


    public void moveTool(MouseEvent ev) {
         int x = ev.getX();
         int y = ev.getY();
         Point pt = getLocation();
         if (tx < 0) {
            tx = pt.x;
            ty = pt.y;
            topPanel = (JComponent)Util.getAppIF();
            Dimension d = topPanel.getSize();
            pt = topPanel.getLocationOnScreen();
            topPanX = pt.x;
            topPanY = pt.y;
            dockEast = pt.x + d.width - toolW;
            dockRTop = pt.y + dockTop;
            dockRBottom = pt.y + dockBottom;
            return;
         }
         PointerInfo ptInfo = MouseInfo.getPointerInfo();
         Point  mpt = ptInfo.getLocation();
         pt.x = mpt.x - mx;
         pt.y = mpt.y - my;
         if (pt.x < topPanX)  pt.x = topPanX;
         if (pt.x > dockEast)  pt.x = dockEast;
         if (pt.y > dockRBottom - toolW)  pt.y = dockRBottom - toolW;
         if (pt.y < topPanY) pt.y = topPanY;
         if (pt.x <= topPanX + 5) {
            if (pt.y > dockRTop) {
                 setDockPosition(1); // left
                 return;
            }
         }
         if (pt.x >= dockEast) {
            if (pt.y > dockRTop) {
                 setDockPosition(2); // right
                 return;
            }
         }
         if (dockPosition != 0)
            setDockPosition(0);
         setLocation(pt.x, pt.y);
    }

    public void adjustDockLocation() {
    }

    public Component getToolBar() {
	return toolBar;
    }

    public void setToolBar(Component obj) {
	if (toolBar != null)
	   contPan.remove(toolBar);
	toolBar = obj;
	toolComp = null;
        if (toolBar == null)
           return;
        if (toolBar instanceof SubButtonPanel) {
           toolComp = (SubButtonPanel) toolBar;
	   toolComp.setOrientation(orientation);
        }
	// add(toolBar, BorderLayout.NORTH);
	contPan.add(toolBar);
        resetSize();
    }

    public void resetSize() {
          int w = 0;
          int h = 0;
          Dimension d;
          if (toolBar != null) {
             d = toolBar.getPreferredSize();
             h = d.height;
             w = d.width;
          }
          if (orientation == HORIZONTAL)
             w = w + gap;
          else
             h = h + gap;
          setSize(w, h);
	  validate();
	  repaint();
    }

    public void setPosition(int x, int y) {
          setLocation(x, y);
    }

    public Point getPosition() {
         return getLocation();
    }

    public void setShow(boolean s) {
        setVisible(s);
    }

    public boolean isShow() {
        return isVisible();
    }

    public int getPreferredWidth() {
        Dimension d = getPreferredSize();
        return d.width;
    }
    
    private void doAction(JMenuItem s) {
        String str = s.getText();
        if (str.equals("Close")) {
             VnmrjIF vif = Util.getVjIF();
             if (vif != null)
                vif.openComp("GraphicsTool", "close"); 
             return;
        }
        if (str.equals("Vertical")) {
             setDefaultOrientation(VERTICAL);
             return;
        }
        if (str.equals("Horizontal")) {
             setDefaultOrientation(HORIZONTAL);
             return;
        }
    }

    private class PopActionEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            doAction((JMenuItem)ae.getSource());
        }
    }

    private class ftoolbarLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
          int w = 0;
          int h = 0;
          Dimension d;
          if (toolBar != null) {
             d = toolBar.getPreferredSize();
             h = d.height;
             w = d.width;
          }
          if (orientation == HORIZONTAL)
             w = w + gap;
          else
             h = h + gap;
          setSize(w, h);
          return new Dimension(w, h);
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0); // unused
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension dim = target.getSize();
              Insets insets = target.getInsets();
              int w = dim.width - insets.left - insets.right;
              int h = dim.height - insets.top - insets.bottom;
              if (orientation == HORIZONTAL) {
                 navigationObj.setBounds(0, 0, gap, h);
                 if (toolBar != null) {
                    toolBar.setBounds(gap, 0, w - gap, h);
                    toolW = h;
                 }
              }
              else {
                 navigationObj.setBounds(0, 0, w, gap);
                 if (toolBar != null) {
                    toolBar.setBounds(0, gap, w, h - gap);
                    toolW = w;
                 }
              }
           }
       }
    }

    private class NavigateTool extends JLabel {

        public  NavigateTool(String s) {
             super(s);
        }

        public void  paint(Graphics g) {
             Dimension d = getSize();
             g.setColor(getBackground().darker());
             int w = d.width;
             int h = d.height;
             int x = 3;
             int y = 3;
             
             if (w < 6 || h < 6)
                 return;
             g.setColor(getBackground().brighter());
             g.drawLine(1, 1, w, 1);
             g.drawLine(1, 1, 1, h-2);
             g.drawLine(1, h-2, w, h-2);

             g.setColor(getBackground().darker());
             if (orientation == HORIZONTAL) {
                x = 4;
                h = h - 3;
                while (y < h) {
                   g.drawLine(x, y, x, y+1);
                   y += 3;
                }
                x = 6;
                y = 4;
                while (y < h) {
                   g.drawLine(x, y, x, y+1);
                   y += 3;
                }
             }
             else {
                w = w - 3;
                y = 4;
                while (x < w) {
                   g.drawLine(x, y, x+1, y);
                   x += 3;
                }
                x = 4;
                y = 6;
                while (x < w) {
                   g.drawLine(x, y, x+1, y);
                   x += 3;
                }
             }
        }
    }
} // class GraphicsWinTool
