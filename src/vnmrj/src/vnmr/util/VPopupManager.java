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
import javax.swing.*;

public class VPopupManager {
    private static java.util.List<Dialog> popupList = null;
    private static Runnable resetWinowProc;
    private static Dialog lastDialog = null;
    private static VPopupContainer popupContainer = null;

    public static void addRemovePopup(Dialog dialog, boolean bAdd) {
        if (popupList == null)
            popupList = Collections.synchronizedList(new LinkedList<Dialog>());

        popupList.remove(dialog);
        if (dialog instanceof ModalDialog)
            return;
        boolean bOnTop = true;
        if (dialog instanceof VPopupIF)
            bOnTop = ((VPopupIF)dialog).isOnTopSet();
        if (!bOnTop)
            return;

        int num = 0;
        int x = 0;
        int y = 0;
        Point pt = new Point(0, 0);
        Iterator<Dialog> itr = popupList.iterator();
        Dialog curDialog = null;
        while (itr.hasNext()) {
            curDialog = itr.next();
            if (curDialog != null) {
               if (curDialog.isVisible()) {
                   num++;
                  // curDialog.setAlwaysOnTop(false);
                   pt = curDialog.getLocation();
                   x = pt.x;
                   y = pt.y;
               }
            }
        }
        if (num < 1)
            popupList.clear();

        if (resetWinowProc == null) {
            resetWinowProc = new Runnable() {
                  public void run() {
                      resetWinowLayer();
                  }
            };
        }

        // if (bAdd)
        //    SwingUtilities.invokeLater(resetWinowProc);

        if (!bAdd) {
            lastDialog = curDialog;
            // if (curDialog != null)
            //     curDialog.setAlwaysOnTop(true);
            return;
        }
        if (!popupList.contains(dialog))
            popupList.add(dialog);
        lastDialog = dialog;
        // dialog.setAlwaysOnTop(true);
        if (num < 1)
            return;
        pt = dialog.getLocation();
        if (pt.x > 1)
            return;

        if (x < 200)
            pt.x = x + 60;
        else
            pt.x = x - 60;
        if (y > 200)
            pt.y = y - 30;
        else
            pt.y = y + 30;
        dialog.setLocation(pt);
    }

    private static void resetWinowLayer() {
        if (lastDialog != null) {
             lastDialog.toFront();
        }
    }

    public static void init() {
         if (popupContainer == null) {
             popupContainer = new VPopupContainer(); 
             Toolkit.getDefaultToolkit().addAWTEventListener(popupContainer,
                     AWTEvent.WINDOW_EVENT_MASK);
         }
    }

    public static JComponent getPopupContainer() {
         return popupContainer;
    }
}
