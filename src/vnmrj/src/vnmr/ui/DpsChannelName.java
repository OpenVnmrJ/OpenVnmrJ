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
import java.awt.dnd.*;
import java.util.*;
import javax.swing.*;


public class DpsChannelName extends JPanel {
    private Vector<ChannelName>  channels;
    private int fontH = 0;
    private int NUM = 14;
    private int preferedWidth = 14;
    private boolean bAdding = false;
    private boolean bMoving = false;
    private DpsScopePanel scopePan = null;

    public DpsChannelName() {
         NUM = DpsWindow.DPSCHANNELS;
         channels = new Vector<ChannelName>(NUM);
         setLayout(new DpsNameLayout());
    }

    public void startAdd() {
         channels.clear();
         removeAll();
         channels.setSize(NUM);
    }

    public void finishAdd() {
        int w;

        bAdding = false;
        w = 20;
        for (int k = 0; k < NUM; k++) {
             ChannelName label = channels.elementAt(k);
             if (label != null) {
                 Dimension dim = label.getPreferredSize();
                 if (dim.width > w)
                     w = dim.width;
             }
        }
        preferedWidth = w + 4;
        Dimension dim = getPreferredSize();
        if (dim.height < 100)
            dim.height = 400;
        dim.width = w + 4;
        setPreferredSize(dim);
    }

    public void addChannel(int id, String name) {
        if (id >= NUM)
            return;
        if (name == null || name.length() < 1)
            return;
        DpsOptions optPanel = DpsUtil.getOptionPanel();
        bAdding = true;
        ChannelName label = new ChannelName(name, id);
        add(label);
        if (fontH < 2) {
           Font ft = label.getFont();
           if (ft != null)
               fontH = ft.getSize() - 1;
        }
        label.setForeground(optPanel.getChannelColor(id));
        channels.setElementAt(label, id);
        label.setVisible(optPanel.isChannelVisible(id));
        bAdding = false;
    }

    public void setLocY(int id, int p) {
        ChannelName label = channels.elementAt(id);
        if (label != null)
            label.setLocY(p - fontH);
    }

    public void setChannelColor(int id, Color c) {
        if (id >= NUM)
            return;
        ChannelName label = channels.elementAt(id);
        if (label != null)
            label.setForeground(c);
    }

    public void setChannelVisible(int id, boolean b) {
        if (id >= NUM)
            return;
        ChannelName label = channels.elementAt(id);
        if (label != null)
            label.setVisible(b);
    }

    public void print(Graphics2D g) {
        int count = getComponentCount();
        boolean bOpaque = isOpaque();
        setOpaque(false);
        for (int k = 0; k < count; k++) {
             Component comp = getComponent(k);
             if (comp != null && comp.isVisible()) {
                Point pt = comp.getLocation();
                g.translate(pt.x, pt.y);
                comp.print(g);
                g.translate(-pt.x, -pt.y);
             }
         }
        setOpaque(bOpaque);
    }

    private class DpsNameLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
         
            return new Dimension(preferedWidth, 0);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(20, 20);
        }

        public void layoutContainer(Container target) {
            if (bAdding)
                return;
            synchronized(target.getTreeLock()) {
                int count = target.getComponentCount();
                int width = target.getWidth() - 2;
                int height = target.getHeight();
           
                if (count <= 0 || width < 2)
                     return;
                int gap = height / count;
                int y = gap / 2;

                for (int k = 0; k < count; k++) {
                    Component comp = target.getComponent(k);
                    if (comp != null) {
                         Dimension dim = comp.getPreferredSize();
                         Point pt = comp.getLocation();
                         comp.setBounds(2, pt.y, width, dim.height);
                         y = y + gap;
                    }
                }
            }
        }
    }

    private class ChannelName extends JLabel {
         int  id;
         
         boolean bPressed;

         public ChannelName(String text, int n) {
              super(text);
              this.id = n;
              setOpaque(true);
              setBackground(null);
              addMouseListener(new MouseAdapter() {
                  public void mousePressed(MouseEvent evt) {
                       // setOpaque(true);
                       // setBackground(Color.orange);
                       bPressed = true;
                       bMoving = true;
                       setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
                       startMoveChannel();              
                  }

                  public void mouseReleased(MouseEvent evt) {
                       // setOpaque(false);
                       // setBackground(null);
                       bMoving = false;
                       bPressed = false;
                       setCursor(null);
                       setBackground(null);
                       stopMoveChannel();              
                  }

                  public void mouseEntered(MouseEvent evt) {
                       if (!bMoving)
                          setBackground(Color.orange);
                  }

                  public void mouseExited(MouseEvent evt) {
                       if (!bPressed)
                           setBackground(null);
                  }
              });

              addMouseMotionListener(new MouseMotionAdapter() {
                  public void mouseDragged(MouseEvent evt) {
                       int y = evt.getY();
                       moveChannel(y);
                  }
              });
         }

         public void setId(int n) {
              id = n;
         }

         public int getId() {
              return id;
         }

         public void setLocY(int p) {
              setLocation(new Point(2, p));
         }

         private void startMoveChannel() {
              if (scopePan == null) {
                  scopePan = DpsUtil.getScopePanel();
                  if (scopePan == null)
                       return;
              }
              scopePan.startMoveChannel(id);
         }

         private void stopMoveChannel() {
              if (scopePan == null)
                 return;
              scopePan.stopMoveChannel(id);
         }

         private void moveChannel(int n) {
              if (scopePan == null)
                 return;
              scopePan.moveChannel(id, n);
         }
    }
}

