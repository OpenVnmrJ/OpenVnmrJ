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

/**
 * The container for the display palette.
 *
 */
public class DisplayPalette extends JPanel implements GraphicsToolIF,
      DockConstants, PropertyChangeListener{
    // ==== instance variables
    /** button to toggle the presence of Shuffler */
    private JToggleButton shufflerToggle;
    /** button to toggle the presence of ControlPanel */
    private JToggleButton controlPanelToggle = null;
    // private NavigateTool navigationObj;
    private JLabel navigationObj;
    private JButton closeButton;
    private Component toolBar;
    private boolean  bDocked = false;
    private boolean  bShowAble = true;
    private boolean  bIsVisible = true;
    private int  mx = 0;
    private int  my = 0;
    private int  dx = 0;
    private int  dy = 0;
    private int  tx = -1;
    private int  ty = 0;
    private int  toolW = 30;
    private int  thisWidth = 30;
    private int  thisHeight = 30;
    private int  gap = 10;
    private int  dockTop = 10;
    private int  dockBottom = 800;
    private int  maxRight = 0;
    private int  maxBottom = 0;
    private int  orientation = VERTICAL;
    private int  defaultOrient = HORIZONTAL;
    private int  dockPosition = DOCK_EAST;
    private int  closeButtonH = 12;
    private LineBorder  border;
    private JPopupMenu  popup;
    private SubButtonPanel buttonPanel = null;
    private VnmrjIF vif;

    /**
     * constructor
     * @param colorable colorable object
     */
    public DisplayPalette() {
	/* Use a BorderLayout, and put display-control buttons at north,
	 * a blank panel at center, and the hide/expand toggles at south. */
	// setLayout(new BorderLayout());
	setLayout(new ftoolbarLayout());
        navigationObj = new JLabel("");
        add(navigationObj);
	setOpaque(true);
        closeButton = new VJButton(Util.getImageIcon("closepanelB.gif"));
        ((VJButton)closeButton).setBorderAlways(true);
        closeButton.setToolTipText("Close");
        // add(closeButton);
        closeButtonH = 0;

	// navigationObj.setOpaque(false);
        border = new LineBorder(Color.gray, 1, true);
        setBorder(null);
	// create toggle buttons
/**
	shufflerToggle = new JToggleButton();
	shufflerToggle.setBorder(BorderFactory.createEmptyBorder());
	shufflerToggle.setOpaque(false);
	shufflerToggle.setIcon(Util.getImageIcon("left.gif"));
	shufflerToggle.setSelectedIcon(Util.getImageIcon("right.gif"));
	controlPanelToggle = new JToggleButton();
	controlPanelToggle.setBorder(BorderFactory.createEmptyBorder());
	controlPanelToggle.setOpaque(false);
	controlPanelToggle.setIcon(Util.getImageIcon("down.gif"));
	controlPanelToggle.setSelectedIcon(Util.getImageIcon("up.gif"));
	shufflerToggle.setContentAreaFilled(false);
	controlPanelToggle.setContentAreaFilled(false);

	// create a panel to display "presence toggle" buttons
	JPanel togglePanel = new JPanel();
	togglePanel.setOpaque(false);
	togglePanel.setLayout(new BorderLayout());
	togglePanel.add(shufflerToggle, BorderLayout.WEST);
	togglePanel.add(controlPanelToggle, BorderLayout.EAST);

	add(togglePanel, BorderLayout.SOUTH);
**/
        navigationObj.setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));

        navigationObj.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                mx = e.getX();
                my = e.getY();
                if (e.getButton() == MouseEvent.BUTTON3) {
                    popupMenu(mx, my);
                }
                else if (e.getButton() == MouseEvent.BUTTON1) {
                    startMove();
                    dragTool(e);
                }
            }

            public void mouseReleased(MouseEvent e) {
                if (e.getButton() == MouseEvent.BUTTON1) {
                    dragTool(e);
                    stopMove();
                }
            }
        });

        navigationObj.addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent e)
            {
                int butInfo = e.getModifiersEx();
                // if ((butInfo & InputEvent.BUTTON3_DOWN_MASK) == 0)
                if ((butInfo & InputEvent.BUTTON1_DOWN_MASK) != 0)
                   dragTool(e);
            }
        });

        closeButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                closeAction();
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
	DisplayOptions.addChangeListener(this);
        navigationObj.setVerticalAlignment(SwingConstants.CENTER);
        navigationObj.setHorizontalAlignment(SwingConstants.CENTER);
        setNavigatorImage();

    } // DisplayPalette()

    private void setNavigatorImage() {
        ImageIcon icon = null;
        if (orientation == VERTICAL)
            icon = Util.getVerBumpIcon();
        else
            icon = Util.getHorBumpIcon();
        if (icon != null) {
            navigationObj.setIcon(icon);
            navigationObj.repaint();
         }
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
	   // setBackground(Util.getBgColor());
           // navigationObj.setBackground(Util.getToolBarBg());
           setNavigatorImage();
        }
    }

    private void popupMenu(int x, int y) {
        Dimension d0 = getParent().getSize();
        Dimension d1 = popup.getPreferredSize();
        Point pt = getLocation();

        x += 12;
        int x1 = x + d1.width + pt.x - d0.width;
        if (x1 > 0) {
             x = x - x1;
             y = y + 8;
        }
 
        popup.show(this, x, y);
    }

    public int getOrientation() {
        return orientation;
    }

    public void setOrientation(int o) {
        if (o != VERTICAL && o != HORIZONTAL)
           return;
        orientation = o;
        int oldOrient = -1;
        if (buttonPanel != null) {
            oldOrient = buttonPanel.getOrientation();
            buttonPanel.setOrientation(o);
        }
        if (oldOrient == o)
           return;
        setNavigatorImage();
        resetSize();
/*
        revalidate();
        repaint();
*/
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

    public int getDockPosition()
    {
        return dockPosition;
    }

    public void setDockPosition(int d)
    {
        int orient = defaultOrient;
        dockPosition = d;
        if (d > 0 && d < DOCK_NORTH_WEST) {
            if ((d == DOCK_ABOVE_EXP) || (d == DOCK_BELLOW_EXP))
                orient = HORIZONTAL;
            else
                orient = VERTICAL;
            setOrientation(orient);
        }
        if (d >= DOCK_NORTH_WEST) // dock at graphics canvas
            adjustDockLocation();
    }

    public void setDockTopPosition(int y) {
         dockTop = y;
    }

    public void setDockBottomPosition(int y) {
         dockBottom = y;
    }

    private void moveLocation(MouseEvent ev) {
         int x = ev.getX();
         int y = ev.getY();
         Point pt = getLocation();

         x = x + pt.x - mx;
         y = y + pt.y - my;
         if (dockPosition != DOCK_NONE) {
            if (x > 24 || x < maxRight) {
                setDockPosition(DOCK_NONE);
            }
            return;
         }

         if (y < 0)  y = 0;
         else if (y > maxBottom)  y = maxBottom;
         if (x < 0)  x = 0;
         else if (x > maxRight)   x = maxRight;
         setLocation(x, y);
    } 

    public void dragTool(MouseEvent ev) {
        if (vif == null)
            return;
        Point pt;
        if (tx < 0) { // first time of drag
            pt = getLocation();
            tx = pt.x;
            ty = pt.y;
            Dimension d = getParent().getSize();
            maxRight = d.width - 8;
            maxBottom = d.height - 20;
            return;
        }
        moveLocation(ev);
        pt = getLocation();
        vif.graphToolMoveTo(pt.x, pt.y);
    } 

    public void adjustDockLocation(Component canvas) {
         if (dockPosition < DOCK_NORTH_WEST)
            return;
         Point pt1 = canvas.getLocationOnScreen();
         Point pt2 = getParent().getLocationOnScreen();
         Dimension dim = canvas.getSize();
         int cX1 = pt1.x - pt2.x;
         int cY1 = pt1.y - pt2.y;
         int cX2 = cX1 + dim.width;
         int cY2 = cY1 + dim.height;
         if (dockPosition == DOCK_NORTH_WEST) {
            setLocation(cX1, cY1);
            return;
         }
         if (dockPosition == DOCK_SOUTH_WEST) {
            setLocation(cX1, cY2 - thisHeight);
            return;
         }
         cX2 = cX2 - thisWidth;
         if (dockPosition == DOCK_NORTH_EAST) {
            setLocation(cX2, cY1);
            return;
         }
         if (dockPosition == DOCK_SOUTH_EAST) {
            setLocation(cX2, cY2 - thisHeight);
            return;
         }
    }

    // dock at the corner of canvas
    public void adjustDockLocation() {
         if (dockPosition < DOCK_NORTH_WEST)
            return;
         ExpPanel exp = Util.getActiveView();
         if (exp == null)
            return;

         Component canvas = (Component) exp.getCanvas();
         if (!exp.isInActive() || !canvas.isShowing())
             return;
         adjustDockLocation(canvas);
	 validate();
    }

    private void startMove() {
        tx = -1;
        vif = Util.getVjIF();
        if (vif != null)
            vif.setGraphToolMoveStatus(true);
        
    }

    // if released position was in canvas, dock to the corner of canvas
    private void stopMove() {
         if (vif == null)
            return;
         vif.setGraphToolMoveStatus(false);
         if (tx < 0)
             return;
         setDockPosition(vif.getGraphToolDockPosition());
        /***************
         if (dockPosition < DOCK_NORTH_WEST)
             return;
         ExpPanel exp = Util.getActiveView();
         if (exp == null)
             return;
         Component canvas = (Component) exp.getCanvas();
         if (!exp.isInActive() || !canvas.isShowing())
            return;
	 if (buttonPanel == null)
             return;
         Point pt0 = getLocation();
         Point pt1 = canvas.getLocationOnScreen();
         Point pt2 = getParent().getLocationOnScreen();
         Dimension dim = canvas.getSize();
         int cX1 = pt1.x - pt2.x;
         int cY1 = pt1.y - pt2.y;
         int cX2 = cX1 + dim.width;
         int cY2 = cY1 + dim.height;
         int sX1 = pt0.x;
         int sX2 = pt0.x + thisWidth;
         int sY1 = pt0.y;
         int sY2 = pt0.y + thisHeight;
         int x1, x2;
         int y1, y2;
         int newOrient = HORIZONTAL;
         x1 = sX1 - cX1;
         y1 = sY1 - cY1;
         x2 = cX2 - sX1;
         y2 = cY2 - sY2;
         switch (dockPosition) {
             case DOCK_NORTH_WEST:
                              if (x1 < y1)
                                   newOrient = VERTICAL;
                              break;
             case DOCK_SOUTH_WEST:
                              if (x1 < y2)
                                   newOrient = VERTICAL;
                              break;
             case DOCK_NORTH_EAST:
                              if (x2 < y1)
                                   newOrient = VERTICAL;
                              break;
             case DOCK_SOUTH_EAST:
                              if (x2 < y2)
                                   newOrient = VERTICAL;
                              break;
         }
         defaultOrient = newOrient;
         setOrientation(newOrient);
         adjustDockLocation(canvas);
        ***************/
    }

    /**
     * get the button for toggling Shuffler presence
     */
    public JToggleButton getShufflerToggle() {
	return shufflerToggle;
    } // getShufflerToggle()

    /**
     * get the button for toggling presence of ControlPanel
     */
    public JToggleButton getControlPanelToggle() {
	return controlPanelToggle;
    } // getControlPanelToggle()

    public Component getToolBar() {
	return toolBar;
    }

    public void setToolBar(Component obj) {
	if (toolBar != null)
	   remove(toolBar);
	toolBar = obj;
	buttonPanel = null;
        if (toolBar == null) {
	   repaint();
           return;
        }
        if (toolBar instanceof SubButtonPanel) {
           buttonPanel = (SubButtonPanel) toolBar;
	   buttonPanel.setOrientation(orientation);
        }
	add(toolBar);
        resetSize();
        if (dockPosition >= DOCK_NORTH_WEST)
             adjustDockLocation();
    }

    public void resetSize() {
          int w = 0;
          int h = 0;
          Dimension d;

          if (dockPosition >= DOCK_WEST && dockPosition < DOCK_NORTH_WEST) {
	     repaint();
             return;
          }
          if (toolBar != null) {
             d = toolBar.getPreferredSize();
             h = d.height;
             w = d.width;
          }
          if (orientation == HORIZONTAL)
             w = w + gap + closeButtonH;
          else
             h = h + gap + closeButtonH;
          thisWidth = w;
          thisHeight = h;
          setSize(w, h);
	  validate();
	  // repaint();
    }

    public void setPosition(int x, int y) {
          setLocation(x, y);
          adjustDockLocation();
    }

    public Point getPosition() {
         return getLocation();
    }

    public void setShowAble(boolean s) {
        bShowAble = s;
        if (s && bIsVisible)
            setVisible(true);
        if (!s)
            setVisible(false);
    }
         

    public void setShow(boolean s) {
        bIsVisible = s;
        if (s && bShowAble)
            setVisible(true);
        else
            setVisible(false);
    }

    public boolean isShow() {
        return isVisible();
    }

    public int getPreferredWidth() {
        Dimension d = getPreferredSize();
        return d.width;
    }
    
    private void closeAction() {
        vif = Util.getVjIF();
        if (vif != null)
           vif.openComp("GraphicsTool", "close"); 
    }

    private void doAction(JMenuItem s) {
        String str = s.getText();
        if (str.equals("Close")) {
             closeAction();
             return;
        }
        if (str.equals("Vertical")) {
             // setDefaultOrientation(VERTICAL);
             defaultOrient = VERTICAL;
             if (dockPosition >= DOCK_WEST && dockPosition < DOCK_NORTH_WEST)
                return;
             setOrientation(defaultOrient);
             adjustDockLocation();
             return;
        }
        if (str.equals("Horizontal")) {
             defaultOrient = HORIZONTAL;
             if (dockPosition >= DOCK_WEST && dockPosition < DOCK_NORTH_WEST)
                return;
             setOrientation(defaultOrient);
             adjustDockLocation();
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
          int w = 30;
          int h = 30;
          Dimension d;
          if (toolBar != null) {
             d = toolBar.getPreferredSize();
             h = d.height;
             w = d.width;
          }
          if (orientation == HORIZONTAL)
             w = w + gap + closeButtonH;
          else
             h = h + gap + closeButtonH;
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
              thisWidth = dim.width;
              thisHeight = dim.height;
              if (orientation == HORIZONTAL) {
                 navigationObj.setBounds(0, 0, gap, h);
                 if (toolBar != null) {
                    toolBar.setBounds(gap, 0, w - gap - closeButtonH, h);
                    toolW = h;
                 }
                 closeButton.setBounds(w - 13, 0, closeButtonH, h);
              }
              else {
                 navigationObj.setBounds(0, 0, w, gap);
                 if (toolBar != null) {
                    toolBar.setBounds(0, gap, w, h - gap - closeButtonH);
                    toolW = w;
                 }
                 closeButton.setBounds(0, h - 13, w, closeButtonH);
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
             int k = 3;
             int y = 3;
             int s;
             
             if (w < 4 || h < 4)
                 return;
             if (dockPosition == DOCK_NONE) {
                g.setColor(getBackground().brighter());
                if (orientation == HORIZONTAL) {
                   g.drawLine(1, 1, w, 1);
                   g.drawLine(1, 1, 1, h-2);
                   g.setColor(getBackground().darker());
                      g.drawLine(1, h-2, w-1, h-2);
                }
                else {
                   g.drawLine(1, 1, w, 1);
                   g.drawLine(1, 1, 1, h-2);
                   g.setColor(getBackground().darker());
                      g.drawLine(w-2, 1, w-2, h-2);
                }
             }
             else {
                g.setColor(getBackground().darker());
                if (orientation == HORIZONTAL) {
                   g.drawLine(0, 0, 0, h-1);
                }
                else {
                   g.drawLine(1, 0, w - 1, 0);
                }
             }
             g.setColor(getBackground().darker());
             if (orientation == HORIZONTAL) {
                x = 5;
                k = h / 4;
                y = k;
                for (s = 0; s < 3; s++) {
                   g.drawLine(x, y, x, y+1);
                   g.drawLine(x+1, y, x+1, y+1);
                   y += k;
                }
             }
             else {
                y = 5;
                k = w / 4;
                x = k;
                for (s = 0; s < 3; s++) {
                   g.drawLine(x, y, x, y+1);
                   g.drawLine(x+1, y, x+1, y+1);
                   x += k;
                }
             }
        }
    }
} // class DisplayPalette
