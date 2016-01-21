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
import java.awt.event.*;
import java.util.*;
import javax.swing.*;

public class VPopupContainer extends JComponent implements AWTEventListener, ActionListener {
    private Vector<ExpButton> butVector;
    private Vector<Window> winVector;
    private Runnable checkWinProc;

    
    public VPopupContainer() {
         super();
         this.butVector = new Vector<ExpButton>(8);
         this.winVector = new Vector<Window>(8);
         setLayout(new xyLayout());
    }

    public void eventDispatched(AWTEvent e) {
         if (!(e instanceof WindowEvent))
              return;
         Window win = null;
         Object obj = e.getSource();
         if (obj != null) {
              if (obj == Util.getMainFrame())
                  return;
              if (obj instanceof Window)
                  win = (Window) obj;
         }
         if (win == null)
              return;

         boolean bCheck = false;
         int id = e.getID();
         switch (id) {
            case WindowEvent.WINDOW_OPENED:  // 200
                     bCheck = true;
                     break;
            case WindowEvent.WINDOW_CLOSING:  // 201
            case WindowEvent.WINDOW_CLOSED:   // 202
                     bCheck = true;
                     break;
            case WindowEvent.WINDOW_ACTIVATED:  // 205
            case WindowEvent.WINDOW_DEACTIVATED:  // 206
                     bCheck = true;
                     break;
            default:
                     break;
         }
         if (!bCheck)
              return;
         if (checkWinProc == null) {
             checkWinProc = new Runnable() {
                 public void run() {
                     checkWinows();
                 }
             };
         }
         SwingUtilities.invokeLater(checkWinProc);
    }

    private void checkList() {
         int num = winVector.size();

         if (butVector.size() < num)
             butVector.setSize(num);

         num = 0;
         for (int i = 0; i < winVector.size(); i++) {
             Window win = winVector.elementAt(i);
             ExpButton button = butVector.elementAt(i);
             if (win == null || !win.isVisible()) {
                 if (button != null)
                      button.setVisible(false);
             }
             if (button != null) {
                 if (button.isVisible())
                     num++;
             }
         }
         if (num > 0)
             setVisible(true);
         else
             setVisible(false);
         revalidate();
    }

    private boolean addPopup(Window win) {
         int freeCell = -1;
         boolean bNewWin = false;

         String tip = win.getName();
         if (tip == null || (tip.length() < 1))
             tip = "Dialog Window";
         if (tip.startsWith("##") || tip.startsWith("win")) {  // HeavyWeightWindow
             return false;
         }

         for (int i = 0; i < winVector.size(); i++) {
             Window obj = winVector.elementAt(i);
             if (obj != null) {
                  if (obj == win) {
                      freeCell = i;
                      break;
                  }
             }
             else if (freeCell < 0)
                 freeCell = i;
         }
         if (freeCell < 0) {
             freeCell = winVector.size();
             if (freeCell > 20) // something wrong
                  return false;
             winVector.setSize(freeCell + 2);
         }
         if (butVector.size() <= freeCell)
             butVector.setSize(freeCell + 2);
         winVector.setElementAt(win, freeCell);
         ExpButton button = butVector.elementAt(freeCell);
         if (button == null) {
             button = new ExpButton(freeCell);
             button.setIconData("popupwindow.png");
             add(button);
             butVector.setElementAt(button, freeCell);
             button.addActionListener((ActionListener) this);
             bNewWin = true;
         }
         if (!button.isVisible())
             bNewWin = true;
         button.setTooltip(tip);
         button.setVisible(true);
         return bNewWin;
    }

    private void removePopup(Window win) {

         for (int i = 0; i < winVector.size(); i++) {
             Window obj = winVector.elementAt(i);
             if (obj != null && obj == win) {
                  winVector.setElementAt(null, i);
                  ExpButton button = butVector.elementAt(i);
                  button.setVisible(false);
             }
         }
    }

    private void checkWinows() {
         Window windows[] = Window.getWindows();
         Window newWin = null;

         for (int i = 0; i < windows.length; i++) {
             Window win = windows[i];
             if (win != null && (win != Util.getMainFrame())) {
                 if (win.isDisplayable()) {
                     if (win.isVisible()) {
                         if (addPopup(win))
                            newWin = win;
                     }
                     else
                         removePopup(win);
                 }
                 else
                     removePopup(win);
             }
         }

         checkList();
         if (newWin != null) {
              if (newWin instanceof Frame)
                  ((Frame)newWin).setState(Frame.NORMAL);
              newWin.toFront();
          }
    }

    public void actionPerformed(ActionEvent  evt) {
        Object obj = evt.getSource();
        if (!(obj instanceof ExpButton))
             return;
        ExpButton b = (ExpButton) obj;
        int id = b.getId();
        if (id >= winVector.size())
             return;

        Window win = winVector.elementAt(id);
        if (win == null || !win.isVisible()) {
             checkList();
             return;
        }
        if (win instanceof Frame)
             ((Frame)win).setState(Frame.NORMAL);
        win.toFront();
    }

    private class xyLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

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
                  w = w + dim.width + 2;
                  if (dim.height > h)
                      h = dim.height;
               }
            }
            if (count > 0) {
               w += 4;
               h += 2;
            }
            dim.width = w;
            dim.height = h;
            return dim;
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                int x = 4;
                int y = 1;
                int nmembers = target.getComponentCount();
                Dimension size = target.getSize();
                int dw = size.width;
                int dh = size.height - 2;

                if (dh < 6 || dw < 6)
                   return;
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    if (comp != null && comp.isVisible()) {
                        size = comp.getPreferredSize();
                        comp.setBounds(x, y, size.width, size.height);
                        x = x + size.width + 2;
                    }
                }
            }
        }
    }
}
