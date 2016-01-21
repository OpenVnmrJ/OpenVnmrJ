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
import java.util.*;
import javax.swing.*;
import javax.swing.*;
import javax.swing.border.*;
import vnmr.util.*;


public class PushpinControler extends JPanel
{
    private int orientation = SwingConstants.TOP;
    private int  prefWidth = 0;
    private int  prefHeight = 0;
    private boolean  bWaitNext;
    private ArrayList  tabCompList;

    public PushpinControler(int orientation) {
         this.orientation = orientation;
         setLayout(new VTabPanLayout());
         tabCompList = new ArrayList();
    }


    public void addTab(String name, String sname, JComponent c) {
         if (c == null)
             return;
         if (!(c instanceof PushpinIF))
             return;
         for (int k = 0; k < tabCompList.size(); k++) {
            JComponent obj = (JComponent) tabCompList.get(k);
            if (obj != null) {
                if (c == obj)
                    return;
            }
         }
         VTextIcon icon = new VTextIcon(this, name, orientation);
         icon.setName(sname);
         PushpinTab tab = new PushpinTab();
         tab.setIcon(icon);
         tab.setName(name);
         tab.setOrientation(orientation); 
         tab.setLocation(10, 200);
         tab.setBorderInsets(new Insets(8, 2, 8, 0));
         tab.setContentAreaFilled(false);
         tab.setTabComp(c);
         tab.setVisible(false);
         tab.setControler(this);
         tab.onTop = ((PushpinIF)c).isTabOnTop();
         ((PushpinIF)c).setTab(tab);
         if (tab.onTop)
            add(tab, 0);
         else
            add(tab);
         tabCompList.add(c);
    }

    public void addTab(PushpinIF c) {
         if (c == null)
             return;
         addTab(c.getTitle(), c.getName(), (JComponent) c);
    }

    public void removeTab(PushpinIF c) {
         if (c == null)
             return;
         JComponent obj = null;
         for (int k = 0; k < tabCompList.size(); k++) {
            obj = (JComponent) tabCompList.get(k);
            if (c == obj)
               break;
            obj = null;
         }
         if (obj == null)
            return;
         if (c.getTab() != null)
             remove(c.getTab());
         c.setTab(null);
         tabCompList.remove(c);
         // revalidate();
    }

    public void removeTab(String name) {
         if (name == null)
             return;
         for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF obj = (PushpinIF) tabCompList.get(k);
            if (obj != null) {
                if (name.equals(obj.getTitle())) {
                    removeTab(obj);
                    break;
                }
            }
         }
    }

    public void enterTab(PushpinTab tab) {
         if (bWaitNext) {
             bWaitNext = false;
             return;
         }
         PushpinIF obj = tab.getTabComp();
         if (obj != null)
            obj.pinShow(true); 
    }

    public void exitTab(PushpinTab tab) {
    }

    public void clickTab(PushpinTab tab) {
         PushpinIF obj = tab.getTabComp();
         if (obj == null)
             return;
         Point p0 = tab.getLocation();
         boolean bVertical = false;
         if (orientation == SwingConstants.LEFT ||
                            orientation == SwingConstants.RIGHT)
             bVertical = true;
         bWaitNext = false;
         int k;
         int num = getComponentCount();
         // this tab will be invisible, so the next tab will move
         // to this tab's location. the flag bWaitNext was set to 
         // avoid the false mouseEnter event.
         for (k = 0; k < num; k++) {
             Component comp = getComponent(k);
             if (comp != null) {
                Point p1 = comp.getLocation();
                if (bVertical) {
                    if (p1.y > p0.y) {
                        bWaitNext = true;
                        break;
                    }
                }
                else {
                    if (p1.x > p0.x) {
                        bWaitNext = true;
                        break;
                    }
                }
             }
         }
         obj.openFromHide(); 
    }


    public void setOrientation(int s) {
        orientation = s;
    }

    public int getOrientation() {
        return orientation;
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        super.paint(g);
        g.setColor(getBackground().darker());
        if (orientation == SwingConstants.LEFT) {
             g.drawLine(d.width - 1 , 0, d.width - 1, d.height);
        }
        else if (orientation == SwingConstants.RIGHT) {
             g.drawLine(0, 0, 0, d.height);
        }
    }

  class VTabPanLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
          Dimension dim = new Dimension(26, 26);
          synchronized (target.getTreeLock()) {
             int nmembers = target.getComponentCount();
             if (nmembers > 0) {
                 Component m = target.getComponent(0);
                 dim = m.getPreferredSize();
                 if (orientation == SwingConstants.LEFT || orientation == SwingConstants.RIGHT)

                 {
                     dim.width += 2;
                     dim.height = 500;
                 }
                 else
                 {
                  dim.width = 500;
                  dim.height += 2;
                 }
              }
          }
          return dim;
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0); // unused
       } // minimumLayoutSize()

       public void adjustVPos() {
           int y, y1, i;
           int h = 0;
           int x = 0;
           int t1 = 0;
           int t2 = 0;
           int gap = 8;
           int d, h2;
           int minH, maxH;
           Dimension pSize = getSize();
           Insets insets = getInsets();
           int th;
           int tw = pSize.width - insets.left * 2;
           int y0 = insets.top+6;
           Dimension dim;
           PushpinTab tab;
           if (pSize.height < 15 || pSize.width < 6)
                return;
           th = pSize.height - y0 - insets.bottom;
           Component comp = getComponent(0);
           dim = comp.getPreferredSize();
           if (orientation == SwingConstants.LEFT)
               x = pSize.width - insets.left - dim.width;
           else
               x = insets.left;

           y = y0 + 12;
           y1 = pSize.height;
           h = 0;
           minH = 500;
           maxH = 0;
           int nmembers = getComponentCount();
           for (i = 0; i < nmembers; i++) {
               comp = getComponent(i);
               tab = (PushpinTab) comp;
               tab.bSet = false;
               if (comp.isVisible()) {
                   dim = comp.getPreferredSize();
                   if (dim.height > maxH)
                        maxH = dim.height;
                   if (dim.height < minH)
                        minH = dim.height;
                   if (tab.onTop) {
                       if (t1 == 0) {
                          y0 = (int) ((float)th * tab.refY);
                          y = y0;
                       }
                       tab.posY = y;
                       y = y + dim.height + gap;
                       t1++;
                   }
                   else {
                       h = h + dim.height + gap;    
                       t2++;
                       tab.posY = (int) ((float)th * tab.refY);
                       if (tab.posY < y1)
                          y1 = tab.posY;
                   }
               }
           }
           d = 0;
           h2 = y + h;
           if (h2 > th) {
               y0 = 4;
               gap = 1;
               y = y - t1 * (gap - 1);
               h = h - t2 * (gap - 1);
               h2 = y + h;
           }
                 
           while (h2 > th) {
               d += 2;
               y = y - t1 * d;
               h = h - t2 * d;
               h2 = y + h;
               if (d > 10) {
                   maxH = minH + 4;
                   break;
               }
               y0 = 2;
           }
           if (t1 > 0) {
               y = y0;
               for (i = 0; i < nmembers; i++) {
                   comp = getComponent(i);
                   tab = (PushpinTab) comp;
                   if (comp.isVisible() && tab.onTop) {
                       dim = comp.getPreferredSize();
                       h2 = dim.height - d;
                       if (h2 > maxH)
                           h2 = maxH;
                       comp.setBounds(x, y, dim.width, h2);
                       tab.bSet = true;
                       y = y + h2 + gap;
                   }
               }
           } 
           if (t2 < 1)
               return;
           d = 0;
           y0 = y;
           if (y1 < 16)
               y1 = 16;
           if (y1 < y)
               y1 = y;
           y = y1;
           for (i = 0; i < nmembers; i++) {
               comp = getComponent(i);
               tab = (PushpinTab) comp;
               if (comp.isVisible() && !tab.bSet) {
                    dim = comp.getPreferredSize();
                    h2 = dim.height - d;
                    if (h2 > maxH)
                        h2 = maxH;
                    comp.setBounds(x, y, dim.width, h2);
                    tab.bSet = true;
                    y = y + h2 + gap;
               }
           }
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension pSize = target.getSize();
              Insets insets = target.getInsets();
              Dimension dim;
              int x, y;
              int  total;
              int nmembers = target.getComponentCount();
              total = 0;
              for (int i = 0; i < nmembers; i++) {
                  Component comp = target.getComponent(i);
                  if (comp.isVisible())
                      total++;
              }
              if (total <= 0) {
                  if (isVisible()) {
                      setVisible(false);
                      Util.getAppIF().validate();
                  }
                  return;
              }
              if (orientation == SwingConstants.LEFT || 
                     orientation == SwingConstants.RIGHT) {
                  adjustVPos();
              }
              if (!isVisible()) {
                  setVisible(true);
                  Util.getAppIF().validate();
              }
  
           } // synchronized 
       }
  }

}

